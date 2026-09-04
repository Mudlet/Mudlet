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
#include <condition_variable>
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
// The threads spin for a short while before parking, because the fork-join has
// to cost less than the work it distributes: a line's worth of matching is only
// a microsecond or two, and a condition-variable wake-up alone is more than
// that. Text arrives in bursts of many lines, so spinning covers the gap
// between consecutive lines of one burst and the threads park between bursts.
class TriggerMatchPool
{
public:
    static TriggerMatchPool& instance();

    TriggerMatchPool(const TriggerMatchPool&) = delete;
    TriggerMatchPool& operator=(const TriggerMatchPool&) = delete;

    // Records on each trigger, under this pass id, whether it may fire on this
    // line. Returns false when it declined the batch (too few triggers to be
    // worth distributing, or no worker threads), in which case nothing was
    // written and the caller runs its ordinary sequential pass.
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

    void workerLoop(int slot, uint64_t startEpoch);
    void runChunks(int slot);

    struct Job
    {
        TTrigger* const* triggers = nullptr;
        int count = 0;
        quint32 passId = 0;
        const char* subject = nullptr;
        int subjectLength = 0;
        const QString* haystack = nullptr;
    };

    // One per worker plus one for the calling thread, which takes a share too.
    std::vector<pcre2_real_match_data_8*> mScratch;
    std::vector<std::thread> mThreads;

    Job mJob;
    alignas(64) std::atomic<uint64_t> mEpoch{0};
    alignas(64) std::atomic<int> mNextChunk{0};
    alignas(64) std::atomic<int> mRemaining{0};
    alignas(64) std::atomic<int> mParked{0};
    std::atomic<bool> mStop{false};
    std::mutex mMutex;
    std::condition_variable mCondition;

    quint64 mPrescanCount = 0;
    int mThreshold = 0;
    int mFloodChunkLines = 0;
    qint64 mSpinBudget = 0;
    int mChunkSize = 0;
};

#endif // MUDLET_TRIGGERMATCHPOOL_H
