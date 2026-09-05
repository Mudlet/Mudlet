#ifndef MUDLET_TRIGGERMATCHPOOL_H
#define MUDLET_TRIGGERMATCHPOOL_H

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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include <QString>

class TTrigger;
struct pcre2_real_match_data_8;

// Evaluates "could this trigger fire on this line?" for a batch of triggers
// across a few threads, so that the sequential pass that follows only has to
// visit the ones that can. The predicates it runs read the trigger and the line
// and touch nothing else, which is what makes them safe to run off the main
// thread; everything with an effect - captures, colouring, Lua - still happens
// on the main thread in the original order.
//
// The calling thread takes chunks of the batch like any helper, and it only
// ever waits for a chunk a helper has actually claimed. A helper that is asleep
// when a batch is published is simply absent from it: the caller sends one
// notify and carries on, and the helper wakes in its own time and joins
// whichever batch is current when it gets there - usually the next line's. So
// the first line of a burst costs the caller a notify, never a wake-up.
// Between lines the helpers spin rather than sleep, because a wake-up costs
// more than the microsecond or two of matching it would hand over; the budget
// is a duration rather than an iteration count because one PAUSE instruction
// is a couple of cycles on some cores and well over a hundred on others.
class TriggerMatchPool
{
public:
    static TriggerMatchPool& instance();

    TriggerMatchPool(const TriggerMatchPool&) = delete;
    TriggerMatchPool& operator=(const TriggerMatchPool&) = delete;

    // Records on each trigger, under this pass id, whether it may fire on this
    // line. Returns false when it declined the batch (too few triggers to be
    // worth distributing, or no worker threads), in which case nothing was
    // written and the caller runs its ordinary sequential pass. One caller at
    // a time: the batch lives in the pool until this returns.
    bool prescan(TTrigger* const* triggers, int count, quint32 passId, const char* subject, int subjectLength, const QString& haystack);

    // Below this many triggers the fork-join costs more than it saves.
    int threshold() const { return mThreshold; }
    // How many lines one chunk has to carry before its matching is worth
    // sharing out - see TriggerUnit::processDataStream().
    int floodChunkLines() const { return mFloodChunkLines; }
    // How many threads share a prescan: the helpers plus the calling thread,
    // which takes a share of the work too. Zero when the pool is off, which is
    // the only case in which it declines every batch, so this is also how a
    // caller asks whether the parallel path is in use at all.
    int workerCount() const { return mThreads.empty() ? 0 : static_cast<int>(mThreads.size()) + 1; }
    // How many batches have been shared out since the client started. Only
    // moves on the main thread, and only when the pool actually took a batch,
    // so a reader can tell a run that used the pool from one that never met the
    // conditions for it.
    quint64 prescanCount() const { return mPrescanCount; }

private:
    TriggerMatchPool();
    ~TriggerMatchPool();

    void workerLoop(int slot);
    uint32_t runChunks(int slot);
    void park(uint32_t seen);

    struct Job
    {
        TTrigger* const* triggers = nullptr;
        int count = 0;
        int chunkSize = 0;
        quint32 passId = 0;
        const char* subject = nullptr;
        int subjectLength = 0;
        const QString* haystack = nullptr;
    };

    // One per worker plus one for the calling thread, which takes a share too.
    std::vector<pcre2_real_match_data_8*> mScratch;
    std::vector<std::thread> mThreads;

    // Written by the caller before it publishes a batch, read by whoever
    // claims a chunk of that batch. Never read by a thread that has not first
    // claimed a chunk of it, which is what keeps the reads race-free.
    Job mJob;
    // Epoch, chunk count and next chunk index in one word, so a single
    // fetch_add both claims a chunk and says which batch, and how large a
    // batch, the claim belongs to: a thread that turns up after a batch is
    // over gets an index past its count and touches nothing. Epoch 0 is the
    // value before any batch, which is why helpers start with seen == 0 and
    // prescan() pre-increments.
    alignas(64) std::atomic<uint64_t> mCursor{0};
    alignas(64) std::atomic<int> mDone{0};
    alignas(64) std::atomic<int> mParked{0};
    std::atomic<bool> mStop{false};
    std::mutex mMutex;
    std::condition_variable mCondition;

    quint64 mPrescanCount = 0;
    // Main thread only; published to the helpers inside mCursor.
    uint32_t mEpoch = 0;
    int mThreshold = 0;
    int mFloodChunkLines = 0;
    std::chrono::steady_clock::duration mSpinBudget{};
};

#endif // MUDLET_TRIGGERMATCHPOOL_H
