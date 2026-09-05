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

#include "TriggerMatchPool.h"

#include "TTrigger.h"
#include "utils.h"

#include <QDebug>
#include <QThread>

#include <algorithm>

// A claim is one fetch_add on a 64-bit word. If that had to go through a lock
// the helpers would serialise on it and the whole design would be a mutex with
// extra steps; every target Mudlet ships to has a lock-free 64-bit RMW.
static_assert(std::atomic<uint64_t>::is_always_lock_free);

namespace {
// Tells the core this is a spin-wait, so it can slow the loop down and hand
// resources to a sibling hyperthread: PAUSE on x86, YIELD on AArch64. Every
// compiler Mudlet ships with takes GNU inline assembly. Anything else spins
// bare, which is only slower.
inline void cpuRelax()
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause" ::: "memory");
#elif defined(__aarch64__)
    __asm__ __volatile__("yield" ::: "memory");
#endif
}

// The cursor is epoch:32 | chunk count:16 | next chunk:16.
constexpr uint64_t kEpochShift = 32;
constexpr uint64_t kCountShift = 16;
constexpr uint64_t kFieldMask = 0xFFFF;
constexpr int kChunkSize = 8;
// Half the index field. Every participant can overshoot the count by one claim
// per batch, and the index has to keep those from spilling into the count.
constexpr int kMaxChunks = 32768;
// Both spin loops read the clock once per this many pauses: often enough that
// a budget is honoured to within a few microseconds, seldom enough that on x86
// the clock read is not what the spin spends its time on.
constexpr int kPausesPerClockCheck = 64;
// How long the caller pauses for a claimed chunk before it starts yielding the
// core instead, in case the helper holding the chunk was descheduled.
constexpr std::chrono::microseconds kYieldAfter{50};

uint64_t packCursor(const uint32_t epoch, const int chunkCount)
{
    Q_ASSERT(chunkCount >= 0 && chunkCount <= kMaxChunks);
    return (static_cast<uint64_t>(epoch) << kEpochShift) | (static_cast<uint64_t>(chunkCount) << kCountShift);
}

uint32_t epochOf(const uint64_t cursor)
{
    return static_cast<uint32_t>(cursor >> kEpochShift);
}

int chunkCountOf(const uint64_t cursor)
{
    return static_cast<int>((cursor >> kCountShift) & kFieldMask);
}

int chunkIndexOf(const uint64_t cursor)
{
    return static_cast<int>(cursor & kFieldMask);
}

// An absent variable means the default. One that is set but does not parse,
// or is below the floor, is refused out loud: a typo that quietly measured
// the default is the worst outcome for a tuning knob.
int envIntOr(const char* name, const int fallback, const int minimum)
{
    if (!qEnvironmentVariableIsSet(name)) {
        return fallback;
    }
    bool parsed = false;
    const int value = qEnvironmentVariableIntValue(name, &parsed);
    if (!parsed || value < minimum) {
        qWarning().nospace() << name << " is set to " << qEnvironmentVariable(name) << " but is not an integer of at least " << minimum << "; using " << fallback;
        return fallback;
    }
    return value;
}
} // namespace

TriggerMatchPool& TriggerMatchPool::instance()
{
    static TriggerMatchPool pool;
    return pool;
}

TriggerMatchPool::TriggerMatchPool()
{
    const int cores = std::max(1, QThread::idealThreadCount());
    // Half the machine, capped: past four the tail of the fork-join grows
    // faster than the share of work each extra thread takes away. Zero is how
    // the pool is turned off.
    const int wanted = std::min(envIntOr("MUDLET_MATCH_THREADS", std::min(4, cores / 2), 0), cores);
    mThreshold = envIntOr("MUDLET_MATCH_THRESHOLD", 32, 1);
    mFloodChunkLines = envIntOr("MUDLET_MATCH_FLOOD_LINES", 8, 1);
    // Zero parks a helper as soon as a batch is exhausted, which puts the
    // wake-up path under every line of a burst.
    mSpinBudget = std::chrono::microseconds(envIntOr("MUDLET_MATCH_SPIN_US", 100, 0));
    if (wanted < 2) {
        return;
    }

    mScratch.resize(wanted, nullptr);
    for (int i = 0; i < wanted; ++i) {
        // One ovector pair is all a yes/no answer needs, and a single one of
        // these then serves every pattern the slot ever matches: PCRE2 reports
        // a match it had no room to record as 0 rather than as a failure.
        mScratch[i] = pcre2_match_data_create(1, nullptr);
        if (!mScratch[i]) {
            // Without scratch a slot cannot answer for regex triggers, and a
            // pool that declines every batch is the same client as before,
            // just slower - so that is the fallback, not a slot that lies.
            qWarning() << "TriggerMatchPool: could not allocate match data; parallel prescan is off";
            for (auto* scratch : mScratch) {
                pcre2_match_data_free(scratch);
            }
            mScratch.clear();
            return;
        }
    }
    mThreads.reserve(wanted - 1);
    for (int slot = 1; slot < wanted; ++slot) {
        // A QThread rather than a std::thread for the name alone: it reaches
        // the OS on all three platforms (Windows since Qt 6.8, which is the
        // floor), so a profiler or a crash report shows which thread this is.
        std::unique_ptr<QThread> thread(QThread::create([this, slot] {
            workerLoop(slot);
        }));
        thread->setObjectName(qsl("TriggerMatch-%1").arg(slot));
        thread->start();
        if (!thread->isRunning()) {
            // Qt has already warned. The pool works with however many helpers
            // did start, as the caller claims every chunk nobody else does;
            // what must not happen is a dead slot counting as a worker.
            break;
        }
        mThreads.push_back(std::move(thread));
    }
}

TriggerMatchPool::~TriggerMatchPool()
{
    mStop.store(true, std::memory_order_relaxed);
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        mCondition.notify_all();
    }
    for (const auto& thread : mThreads) {
        thread->wait();
    }
    for (auto* scratch : mScratch) {
        pcre2_match_data_free(scratch);
    }
}

// Claims chunks until the batch runs out, and returns the epoch its last claim
// landed on, which is the batch this thread has now seen.
uint32_t TriggerMatchPool::runChunks(const int slot)
{
    pcre2_match_data* scratch = mScratch[slot];
    for (;;) {
        const uint64_t claim = mCursor.fetch_add(1, std::memory_order_acq_rel);
        const int chunk = chunkIndexOf(claim);
        Q_ASSERT(chunk < chunkCountOf(claim) + static_cast<int>(mScratch.size()));
        if (chunk >= chunkCountOf(claim)) {
            return epochOf(claim);
        }
        const int begin = chunk * mJob.chunkSize;
        const int end = std::min(begin + mJob.chunkSize, mJob.count);
        for (int i = begin; i < end; ++i) {
            TTrigger* trigger = mJob.triggers[i];
            trigger->setPrescanVerdict(mJob.passId, trigger->prescanMayFire(mJob.subject, mJob.subjectLength, *mJob.haystack, scratch));
        }
        mDone.fetch_add(1, std::memory_order_release);
    }
}

void TriggerMatchPool::workerLoop(const int slot)
{
    uint32_t seen = 0;
    auto idleSince = std::chrono::steady_clock::now();
    int pauses = 0;
    for (;;) {
        if (mStop.load(std::memory_order_relaxed)) {
            return;
        }
        if (epochOf(mCursor.load(std::memory_order_acquire)) != seen) {
            seen = runChunks(slot);
            idleSince = std::chrono::steady_clock::now();
            pauses = 0;
            continue;
        }
        cpuRelax();
        if (++pauses < kPausesPerClockCheck) {
            continue;
        }
        pauses = 0;
        if (std::chrono::steady_clock::now() - idleSince < mSpinBudget) {
            continue;
        }
        park(seen);
    }
}

void TriggerMatchPool::park(const uint32_t seen)
{
    std::unique_lock<std::mutex> lock(mMutex);
    // Sequentially consistent, as are the store and load in prescan() that
    // pair with these: this is a store-buffering handshake, and it only works
    // if at least one side sees the other's store. Weaker orderings let both
    // loads return stale values, which parks this helper through a batch
    // nobody notifies it about. The caller never waits for a parked helper,
    // so that costs a batch's parallelism rather than a hang - but the
    // seq_cst is far cheaper than that.
    mParked.fetch_add(1, std::memory_order_seq_cst);
    mCondition.wait(lock, [this, seen] {
        return epochOf(mCursor.load(std::memory_order_seq_cst)) != seen || mStop.load(std::memory_order_relaxed);
    });
    mParked.fetch_sub(1, std::memory_order_release);
}

bool TriggerMatchPool::prescan(TTrigger* const* triggers, const int count, const quint32 passId, const char* subject, const int subjectLength, const QString& haystack)
{
    if (mThreads.empty() || count < mThreshold) {
        return false;
    }

    ++mPrescanCount;
    const int chunkSize = std::max(kChunkSize, (count + kMaxChunks - 1) / kMaxChunks);
    const int chunkCount = (count + chunkSize - 1) / chunkSize;
    mJob.triggers = triggers;
    mJob.count = count;
    mJob.chunkSize = chunkSize;
    mJob.passId = passId;
    mJob.subject = subject;
    mJob.subjectLength = subjectLength;
    mJob.haystack = &haystack;

    mDone.store(0, std::memory_order_relaxed);
    mCursor.store(packCursor(++mEpoch, chunkCount), std::memory_order_seq_cst);
    if (mParked.load(std::memory_order_seq_cst) > 0) {
        const std::lock_guard<std::mutex> lock(mMutex);
        mCondition.notify_all();
    }

    runChunks(0);

    // Only chunks a helper has actually claimed are outstanding here; a helper
    // still waking up holds none, so it is never waited for.
    int pauses = 0;
    std::chrono::steady_clock::time_point yieldAt{};
    while (mDone.load(std::memory_order_acquire) != chunkCount) {
        cpuRelax();
        if (++pauses < kPausesPerClockCheck) {
            continue;
        }
        pauses = 0;
        const auto now = std::chrono::steady_clock::now();
        if (yieldAt == std::chrono::steady_clock::time_point{}) {
            yieldAt = now + kYieldAfter;
        } else if (now >= yieldAt) {
            break;
        }
    }
    while (mDone.load(std::memory_order_acquire) != chunkCount) {
        QThread::yieldCurrentThread();
    }
    Q_ASSERT(chunkIndexOf(mCursor.load(std::memory_order_relaxed)) >= chunkCount);
    return true;
}
