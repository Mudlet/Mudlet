/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "TIngestPipeline.h"

#include "utils.h"

#include <QThread>

#include <chrono>
#include <cstdio>

namespace {
inline void cpuRelax()
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause" ::: "memory");
#elif defined(__aarch64__)
    __asm__ __volatile__("yield" ::: "memory");
#endif
}

constexpr int kPausesPerClockCheck = 64;
// Units of a flood arrive back to back; sleeping between them would cost a wake-up at the start of each
constexpr std::chrono::microseconds kIdleSpin{300};
constexpr std::chrono::microseconds kEndSpin{50};
} // namespace

bool TIngestPipeline::enabled()
{
    static const bool on = qEnvironmentVariableIntValue("MUDLET_INGEST_THREAD") == 1;
    return on;
}

bool TIngestPipeline::verifying()
{
    static const bool on = qEnvironmentVariableIntValue("MUDLET_INGEST_THREAD_VERIFY") == 1;
    return on;
}

void TIngestPipeline::noteVerified(const bool same, const QString& text)
{
    ++mStatVerified;
    if (!same) {
        if (++mStatVerifyFailed <= 20) {
            std::fprintf(stderr, "ingest-thread: VERIFY MISMATCH on line \"%s\"\n", text.toUtf8().constData());
        }
    }
}

TIngestPipeline::TIngestPipeline(Host* pHost)
: mpTwin(std::make_unique<TBuffer>(pHost))
{
    mpThread.reset(QThread::create([this] {
        workerLoop();
    }));
    mpThread->setObjectName(qsl("Ingest"));
    mpThread->start();
}

TIngestPipeline::~TIngestPipeline()
{
    mStop.store(true, std::memory_order_relaxed);
    mEpoch.fetch_add(1);
    mEpoch.notify_one();
    mpThread->wait();
    printStats();
}

TIngestJob* TIngestPipeline::begin(const TBuffer& source, const std::string& unit, const size_t decodableLength, const QVector<QChar>& lookupTable)
{
    if (!mpThread->isRunning()) {
        return nullptr;
    }
    TIngestJob& job = mJob;
    job.mpUnit = &unit;
    job.mDecodableLength = decodableLength;
    job.mLookupTable = lookupTable;
    job.mEncoding = source.mEncoding;
    job.mStartState = source.parseState();
    job.mConfig = source.parseConfig();
    job.mColours = {source.mBlack,
                    source.mLightBlack,
                    source.mRed,
                    source.mLightRed,
                    source.mGreen,
                    source.mLightGreen,
                    source.mBlue,
                    source.mLightBlue,
                    source.mYellow,
                    source.mLightYellow,
                    source.mCyan,
                    source.mLightCyan,
                    source.mMagenta,
                    source.mLightMagenta,
                    source.mWhite,
                    source.mLightWhite};
    const size_t capacity = static_cast<size_t>(source.mPendingChunkLines) + 2;
    if (job.mLines.size() < capacity) {
        job.mLines.resize(capacity);
    }
    job.mReady.store(0, std::memory_order_relaxed);
    job.mCancel.store(false, std::memory_order_relaxed);
    job.mNext = 0;
    job.mState.store(TIngestJob::Queued, std::memory_order_relaxed);
    job.mQueuedAt = std::chrono::steady_clock::now();
    // seq_cst: publishes the job, and cannot pass the library's read of the waiter count
    mEpoch.fetch_add(1);
    mEpoch.notify_one();

    ++mStatJobs;
    mStatLines += static_cast<quint64>(source.mPendingChunkLines);
    if ((mStatJobs & 63) == 0) {
        printStats();
    }
    return &job;
}

void TIngestPipeline::end(TIngestJob& job)
{
    int state = TIngestJob::Queued;
    if (job.mState.compare_exchange_strong(state, TIngestJob::Idle, std::memory_order_acq_rel) || state == TIngestJob::Idle) {
        if (state == TIngestJob::Queued) {
            ++mStatNeverStarted;
            mStatMainUs += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - job.mQueuedAt).count();
        }
        return;
    }
    job.mCancel.store(true, std::memory_order_relaxed);
    int pauses = 0;
    std::chrono::steady_clock::time_point waitFrom{};
    while ((state = job.mState.load(std::memory_order_acquire)) == TIngestJob::Running) {
        cpuRelax();
        if (++pauses < kPausesPerClockCheck) {
            continue;
        }
        pauses = 0;
        const auto now = std::chrono::steady_clock::now();
        if (waitFrom == std::chrono::steady_clock::time_point{}) {
            waitFrom = now;
        } else if (now - waitFrom >= kEndSpin) {
            job.mState.wait(TIngestJob::Running, std::memory_order_acquire);
        }
    }
    job.mState.store(TIngestJob::Idle, std::memory_order_relaxed);
    const auto now = std::chrono::steady_clock::now();
    mStatMainUs += std::chrono::duration<double, std::micro>(now - job.mQueuedAt).count();
    mStatLatencyUs += std::chrono::duration<double, std::micro>(job.mWorkerStart - job.mQueuedAt).count();
    mStatWorkerUs += std::chrono::duration<double, std::micro>(job.mWorkerEnd - job.mWorkerStart).count();
}

void TIngestPipeline::workerLoop()
{
    uint32_t seen = 0;
    auto idleSince = std::chrono::steady_clock::now();
    int pauses = 0;
    for (;;) {
        const uint32_t epoch = mEpoch.load(std::memory_order_acquire);
        if (mStop.load(std::memory_order_relaxed)) {
            return;
        }
        if (epoch != seen) {
            seen = epoch;
            int state = TIngestJob::Queued;
            if (mJob.mState.compare_exchange_strong(state, TIngestJob::Running, std::memory_order_acq_rel)) {
                mJob.mWorkerStart = std::chrono::steady_clock::now();
                parse(mJob);
                mJob.mWorkerEnd = std::chrono::steady_clock::now();
                mJob.mState.store(TIngestJob::Done);
                mJob.mState.notify_one();
            }
            idleSince = std::chrono::steady_clock::now();
            pauses = 0;
            continue;
        }
        cpuRelax();
        if (++pauses < kPausesPerClockCheck) {
            continue;
        }
        pauses = 0;
        if (std::chrono::steady_clock::now() - idleSince < kIdleSpin) {
            continue;
        }
        mEpoch.wait(epoch, std::memory_order_acquire);
    }
}

void TIngestPipeline::parse(TIngestJob& job)
{
    TBuffer& twin = *mpTwin;
    twin.setParseState(job.mStartState);
    twin.mpParseConfig = &job.mConfig;
    twin.mBlack = job.mColours[0];
    twin.mLightBlack = job.mColours[1];
    twin.mRed = job.mColours[2];
    twin.mLightRed = job.mColours[3];
    twin.mGreen = job.mColours[4];
    twin.mLightGreen = job.mColours[5];
    twin.mBlue = job.mColours[6];
    twin.mLightBlue = job.mColours[7];
    twin.mYellow = job.mColours[8];
    twin.mLightYellow = job.mColours[9];
    twin.mCyan = job.mColours[10];
    twin.mLightCyan = job.mColours[11];
    twin.mMagenta = job.mColours[12];
    twin.mLightMagenta = job.mColours[13];
    twin.mWhite = job.mColours[14];
    twin.mLightWhite = job.mColours[15];
    twin.mEncoding = job.mEncoding;
    twin.mMudLine.resize(0);
    twin.mMudBuffer.clear();
    twin.mSpeculatedLineStart = 0;
    twin.mSpeculationStopped = false;
    twin.mpSpeculationSink = &job;
    // Read only: the speculating subset has no path that writes into the unit
    twin.parseChunk(const_cast<std::string&>(*job.mpUnit), true, job.mLookupTable, job.mDecodableLength, nullptr);
    if (twin.mSpeculationStopped) {
        mStatStopped.fetch_add(1, std::memory_order_relaxed);
    }
    twin.mpSpeculationSink = nullptr;
    twin.mpParseConfig = nullptr;
}

void TIngestPipeline::printStats() const
{
    static const bool wanted = qEnvironmentVariableIntValue("MUDLET_INGEST_THREAD_STATS") == 1;
    if (!wanted) {
        return;
    }
    std::fprintf(stderr,
                 "ingest-thread: jobs %llu lines %llu fast %llu slow %llu mismatch %llu notready %llu stopped %llu verified %llu failed %llu neverstarted %llu latency_us %.0f worker_us %.0f main_us "
                 "%.0f\n",
                 static_cast<unsigned long long>(mStatJobs),
                 static_cast<unsigned long long>(mStatLines),
                 static_cast<unsigned long long>(mStatReplayedFast),
                 static_cast<unsigned long long>(mStatReplayedSlow),
                 static_cast<unsigned long long>(mStatMismatch),
                 static_cast<unsigned long long>(mStatNotReady),
                 static_cast<unsigned long long>(mStatStopped.load(std::memory_order_relaxed)),
                 static_cast<unsigned long long>(mStatVerified),
                 static_cast<unsigned long long>(mStatVerifyFailed),
                 static_cast<unsigned long long>(mStatNeverStarted),
                 mStatLatencyUs,
                 mStatWorkerUs,
                 mStatMainUs);
}
