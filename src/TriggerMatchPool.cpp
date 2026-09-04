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

#include <QThread>

#include <algorithm>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define MUDLET_CPU_RELAX() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define MUDLET_CPU_RELAX() asm volatile("yield" ::: "memory")
#else
#define MUDLET_CPU_RELAX() ((void)0)
#endif

TriggerMatchPool& TriggerMatchPool::instance()
{
    static TriggerMatchPool pool;
    return pool;
}

TriggerMatchPool::TriggerMatchPool()
{
    const int cores = std::max(1, QThread::idealThreadCount());
    bool configured = false;
    int wanted = qEnvironmentVariableIntValue("MUDLET_MATCH_THREADS", &configured);
    if (!configured) {
        // Half the machine, capped: past four the tail of the fork-join grows
        // faster than the share of work each extra thread takes away. An
        // unreadable value counts as unset, but a readable zero does not: that
        // is how the pool is turned off, so it has to survive to the check below
        // rather than be replaced by the default here.
        wanted = std::min(4, cores / 2);
    }
    wanted = std::min(wanted, cores);
    mThreshold = qEnvironmentVariableIntValue("MUDLET_MATCH_THRESHOLD");
    if (mThreshold <= 0) {
        mThreshold = 24;
    }
    mFloodChunkLines = qEnvironmentVariableIntValue("MUDLET_MATCH_FLOOD_LINES");
    if (mFloodChunkLines <= 0) {
        mFloodChunkLines = 8;
    }
    mSpinBudget = qEnvironmentVariableIntValue("MUDLET_MATCH_SPIN");
    if (mSpinBudget <= 0) {
        mSpinBudget = 12000;
    }
    if (wanted < 2) {
        return;
    }

    mChunkSize = 8;
    mScratch.resize(wanted, nullptr);
    for (int i = 0; i < wanted; ++i) {
        // One ovector pair is all a yes/no answer needs, and a single one of
        // these then serves every pattern the slot ever matches: PCRE2 reports
        // a match it had no room to record as 0 rather than as a failure.
        mScratch[i] = pcre2_match_data_create(1, nullptr);
    }
    const uint64_t startEpoch = mEpoch.load(std::memory_order_acquire);
    mThreads.reserve(wanted - 1);
    for (int slot = 1; slot < wanted; ++slot) {
        mThreads.emplace_back([this, slot, startEpoch] {
            workerLoop(slot, startEpoch);
        });
    }
}

TriggerMatchPool::~TriggerMatchPool()
{
    mStop.store(true, std::memory_order_relaxed);
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        mEpoch.fetch_add(1, std::memory_order_acq_rel);
        mCondition.notify_all();
    }
    for (auto& thread : mThreads) {
        thread.join();
    }
    for (auto* scratch : mScratch) {
        if (scratch) {
            pcre2_match_data_free(scratch);
        }
    }
}

void TriggerMatchPool::runChunks(int slot)
{
    pcre2_match_data* scratch = mScratch[slot];
    for (;;) {
        const int begin = mNextChunk.fetch_add(mChunkSize, std::memory_order_relaxed);
        if (begin >= mJob.count) {
            return;
        }
        const int end = std::min(begin + mChunkSize, mJob.count);
        for (int i = begin; i < end; ++i) {
            TTrigger* trigger = mJob.triggers[i];
            trigger->setPrescanVerdict(mJob.passId, trigger->prescanMayFire(mJob.subject, mJob.subjectLength, *mJob.haystack, scratch));
        }
    }
}

void TriggerMatchPool::workerLoop(int slot, uint64_t startEpoch)
{
    uint64_t seen = startEpoch;
    qint64 idle = 0;
    for (;;) {
        const uint64_t epoch = mEpoch.load(std::memory_order_acquire);
        if (epoch != seen) {
            seen = epoch;
            idle = 0;
            if (mStop.load(std::memory_order_relaxed)) {
                return;
            }
            runChunks(slot);
            mRemaining.fetch_sub(1, std::memory_order_release);
            continue;
        }
        if (mStop.load(std::memory_order_relaxed)) {
            return;
        }
        if (++idle < mSpinBudget) {
            MUDLET_CPU_RELAX();
            continue;
        }
        std::unique_lock<std::mutex> lock(mMutex);
        // Sequentially consistent, and likewise for the two in prescan() that
        // pair with these: this is a store-buffering handshake, and it is only
        // safe if at least one side of it sees the other's store. Weaker
        // orderings permit both loads to return the stale value, which parks
        // this worker on a job nobody will notify it about and leaves the
        // caller spinning on mRemaining for good.
        mParked.fetch_add(1, std::memory_order_seq_cst);
        mCondition.wait(lock, [this, &seen] {
            return mEpoch.load(std::memory_order_seq_cst) != seen || mStop.load(std::memory_order_relaxed);
        });
        mParked.fetch_sub(1, std::memory_order_release);
        idle = 0;
    }
}

bool TriggerMatchPool::prescan(TTrigger* const* triggers, int count, const quint32 passId, const char* subject, int subjectLength, const QString& haystack)
{
    if (mThreads.empty() || count < mThreshold) {
        return false;
    }

    ++mPrescanCount;
    mJob.triggers = triggers;
    mJob.count = count;
    mJob.passId = passId;
    mJob.subject = subject;
    mJob.subjectLength = subjectLength;
    mJob.haystack = &haystack;

    const int helpers = static_cast<int>(mThreads.size());
    mNextChunk.store(0, std::memory_order_relaxed);
    mRemaining.store(helpers, std::memory_order_relaxed);
    mEpoch.fetch_add(1, std::memory_order_seq_cst);
    if (mParked.load(std::memory_order_seq_cst) > 0) {
        const std::lock_guard<std::mutex> lock(mMutex);
        mCondition.notify_all();
    }

    // The caller takes a share as well, which is also what keeps the hand-off
    // cheap: by the time it has finished its own chunks the workers are long
    // since running, so only the last one's finish is ever waited on.
    runChunks(0);

    while (mRemaining.load(std::memory_order_acquire) != 0) {
        MUDLET_CPU_RELAX();
    }
    return true;
}
