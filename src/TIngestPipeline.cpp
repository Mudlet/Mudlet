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

#include "MudletApp.h"
#include "utils.h"

#include <QSettings>
#include <QThread>

#include <chrono>
#include <thread>

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
} // namespace

bool TIngestPipeline::enabled()
{
    static const bool on = [] {
        if (qEnvironmentVariableIsSet("MUDLET_INGEST_THREAD")) {
            const QByteArray value = qgetenv("MUDLET_INGEST_THREAD").toLower();
            return value != "0" && value != "false" && value != "off";
        }
        if (QThread::idealThreadCount() < 2) {
            return false;
        }
        QSettings* settings = MudletApp::getQSettings();
        return !settings || settings->value(qsl("ingestThread"), true).toBool();
    }();
    return on;
}

bool TIngestPipeline::verifying()
{
    static const bool on = qgetenv("MUDLET_INGEST_THREAD") == "verify";
    return on;
}

TIngestJob::TIngestJob()
: mLines(csmMaxLines)
{
}

TIngestPipeline::TIngestPipeline(Host* pHost)
: mpTwin(std::make_unique<TBuffer>(pHost))
, mWaitForWorker(qEnvironmentVariableIsSet("MUDLET_TEST_MODE"))
{
    mpThread.reset(QThread::create([this] {
        workerLoop();
    }));
    mpThread->setObjectName(qsl("Ingest"));
    mpThread->start();
}

TIngestPipeline::~TIngestPipeline()
{
    mJob.mCancel.store(true, std::memory_order_relaxed);
    mStop.store(true, std::memory_order_relaxed);
    mEpoch.fetch_add(1);
    mEpoch.notify_one();
    mpThread->wait();
}

TIngestJob* TIngestPipeline::begin(const TBuffer& source, const std::string& unit, const size_t decodableLength, const QVector<QChar>& lookupTable)
{
    if (!mpThread->isRunning()) {
        return nullptr;
    }
    TIngestJob& job = mJob;
    // Still busy with a unit main has finished with: main parses this one alone rather than wait
    if (job.mState.load(std::memory_order_acquire) == TIngestJob::Running) {
        return nullptr;
    }
    if (job.mUnit.capacity() > csmMaxRetainedUnitBytes) {
        std::string().swap(job.mUnit);
    }
    job.mUnit.assign(unit);
    job.mDecodableLength = decodableLength;
    job.mLookupTable = lookupTable;
    queue(source, job, 0);
    return &job;
}

bool TIngestPipeline::resume(const TBuffer& source, TIngestJob& job, const size_t position)
{
    // State first: mResumeFrom is the worker's until it reads Done
    if (job.mState.load(std::memory_order_acquire) != TIngestJob::Done || job.mResumeFrom > position || position + csmMinimumResumeBytes > job.mDecodableLength || !source.speculationEligible()) {
        return false;
    }
    queue(source, job, position);
    return true;
}

bool TIngestPipeline::awaitLines(const TIngestJob& job, const int ready) const
{
    if (!mWaitForWorker) {
        return false;
    }
    for (;;) {
        // State first: once it reads Done, the mReady read after it is final
        const int state = job.mState.load(std::memory_order_acquire);
        if (job.mReady.load(std::memory_order_acquire) > ready) {
            return true;
        }
        if (state != TIngestJob::Queued && state != TIngestJob::Running) {
            return false;
        }
        std::this_thread::yield();
    }
}

void TIngestPipeline::queue(const TBuffer& source, TIngestJob& job, const size_t position)
{
    job.mStartPosition = position;
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
    job.mWrap = source.wrapSettings();
    job.mReady.store(0, std::memory_order_relaxed);
    job.mCancel.store(false, std::memory_order_relaxed);
    job.mNext = 0;
    job.mSeenReady = 0;
    // Release: the worker can claim the job from a stale epoch, and must still see all of the above
    job.mState.store(TIngestJob::Queued, std::memory_order_release);
    mEpoch.fetch_add(1);
    mEpoch.notify_one();
}

void TIngestPipeline::end(TIngestJob& job)
{
    int state = TIngestJob::Queued;
    if (job.mState.compare_exchange_strong(state, TIngestJob::Idle, std::memory_order_acq_rel)) {
        return;
    }
    if (state == TIngestJob::Running) {
        job.mCancel.store(true, std::memory_order_relaxed);
    }
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
                parse(mJob);
                mJob.mState.store(TIngestJob::Done, std::memory_order_release);
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
    twin.mMudLine.resize(0);
    twin.mMudBuffer.clear();
    twin.mSpeculatedLineStart = job.mStartPosition;
    twin.mSpeculationStopped = false;
    twin.mpSpeculationSink = &job;
    job.mTextUsed = 0;
    job.mFormatUsed = 0;
    job.mResumeFrom = TIngestJob::csmNoResume;
    // Must stay read only: the lines' offsets index main's copy of the unit, and resume() parses this copy again
    twin.parseChunk(job.mUnit, true, job.mLookupTable, job.mDecodableLength, nullptr, job.mStartPosition);
    if (twin.mSpeculationStopped && job.mResumeFrom == TIngestJob::csmNoResume && !job.mCancel.load(std::memory_order_relaxed) && job.mReady.load(std::memory_order_relaxed) >= csmMinimumLines) {
        // Main parses the line that stopped it, then may hand the rest back
        job.mResumeFrom = twin.mSpeculatedLineStart + 1;
    }
    twin.mpSpeculationSink = nullptr;
    twin.mpParseConfig = nullptr;
    // Main reads no line past mReady, and what it did not take of an earlier run's would otherwise stay
    // allocated until a run as long overwrote it
    const int ready = job.mReady.load(std::memory_order_relaxed);
    for (int i = ready; i < job.mLinesHeld; ++i) {
        TSpeculatedLine& line = job.mLines[static_cast<size_t>(i)];
        line.text = QString();
        line.format = TLineFormat();
        line.wrapBreaks = QList<WrapInfo>();
    }
    job.mLinesHeld = ready;
    if (static_cast<size_t>(twin.mMudLine.capacity()) > TBuffer::csmMaxRetainedLineCapacity) {
        QString().swap(twin.mMudLine);
    }
    if (twin.mMudBuffer.capacity() > TBuffer::csmMaxRetainedLineCapacity) {
        TLineFormat().swap(twin.mMudBuffer);
    }
}
