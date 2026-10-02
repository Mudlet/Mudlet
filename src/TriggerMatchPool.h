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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <QString>

class QThread;
class TBigramFilter;
class TTrigger;
struct pcre2_real_match_data_8;

// Evaluates "could this trigger fire on this line?" for a batch across a few threads, so the
// sequential pass that follows visits only those that can. The predicates read only the trigger and
// the line, which makes them safe off the main thread; anything with an effect (captures, colouring,
// Lua) stays on the main thread, in order.
//
// The caller takes chunks like any helper and only waits for chunks a helper has claimed. A helper
// asleep at publish joins whichever batch is current when it wakes, so the caller pays a notify,
// never a wake-up. Between lines helpers spin, as a wake-up costs more than the work handed over;
// the budget is a duration because PAUSE takes a few cycles on some cores and 100+ on others. Past
// it they sleep in std::atomic::wait on the cursor, so publishing is one store and one notify, lock-free.
class TriggerMatchPool
{
public:
    static TriggerMatchPool& instance();

    // Stops the helpers for good; every later batch is declined. main() calls it after the event loop
    // returns and before deleting the application, so the threads end while Qt is intact, not in a
    // static destructor. Main thread only, no prescan in flight. Uses smpInstance, as instance()
    // would construct an unused pool just to stop it.
    static void shutdown();

    TriggerMatchPool(const TriggerMatchPool&) = delete;
    TriggerMatchPool& operator=(const TriggerMatchPool&) = delete;

    // Records on each trigger, under passId, whether it may fire on this line. Returns false, having
    // written nothing, when declined (empty batch or no helpers); the caller then runs its sequential
    // pass. The caller decides, against threshold(), whether a batch is worth sharing. One caller at a
    // time: the batch lives in the pool until this returns.
    bool prescan(TTrigger* const* triggers, int count, quint32 passId, const char* subject, int subjectLength, const QString& haystack, const TBigramFilter& lineBigrams, bool dropsText);

    // Regex searches per line below which the fork-join costs more than it saves. Searches, not
    // triggers: disabled, multiline or already-settled triggers run none and should not open the pool.
    int threshold() const { return mThreshold; }
    // Across all threads; the caller weighs it against threshold() for the next line
    int regexSearchesInLastBatch() const { return mRegexSearchesInLastBatch; }
    // Lines a chunk must carry before its matching is worth sharing out; see TriggerUnit::processDataStream()
    int floodChunkLines() const { return mFloodChunkLines; }
    // Helpers plus the calling thread. Zero only when the pool is off, the one case where it declines
    // every batch, so this also says whether the parallel path is in use.
    int workerCount() const { return mThreads.empty() ? 0 : static_cast<int>(mThreads.size()) + 1; }
    // Batches actually taken since startup; main thread only. Tells a run that used the pool from one
    // that never qualified.
    quint64 prescanCount() const { return mPrescanCount; }

private:
    TriggerMatchPool();
    ~TriggerMatchPool();

    void publish(int chunkCount);
    void stopHelpers();
    void workerLoop(int slot);
    uint32_t runChunks(int slot);

    static TriggerMatchPool* smpInstance;

    struct Job
    {
        TTrigger* const* triggers = nullptr;
        int count = 0;
        int chunkSize = 0;
        quint32 passId = 0;
        const char* subject = nullptr;
        int subjectLength = 0;
        const QString* haystack = nullptr;
        const TBigramFilter* lineBigrams = nullptr;
        // Whether encoding haystack to subject lost text, which is the main
        // thread's to answer - see TTrigger::prescanMayFire()
        bool dropsText = false;
    };

    // Spacing for contended words. 128, not 64: Apple Silicon L2 lines are 128 bytes and Intel's
    // spatial prefetcher pulls 64-byte lines in pairs. Not std::hardware_destructive_interference_size:
    // GCC says 64 on x86 and libc++ does not define it.
    static constexpr std::size_t scmCacheLine = 128;

    // Read by every helper on every spin, written only at construction or shutdown, so this line stays
    // valid in every cache while the pool works.
    int mThreshold = 0;
    int mFloodChunkLines = 0;
    std::chrono::steady_clock::duration mSpinBudget{};
    std::atomic<bool> mStop{false};
    // macOS ThreadSanitizer does not treat QThread::wait() as a join; this edge
    // orders a helper's last reads before ~TriggerMatchPool() frees mScratch.
    std::atomic<int> mHelpersReturned{0};
    // One per participant, the calling thread included
    std::vector<pcre2_real_match_data_8*> mScratch;
    std::vector<std::unique_ptr<QThread>> mThreads;

    // Written by the caller before publishing; race-free because only a thread that has claimed a
    // chunk of that batch reads it.
    alignas(scmCacheLine) Job mJob;
    // Main thread only, on mJob's line; mEpoch reaches the helpers inside mCursor.
    quint64 mPrescanCount = 0;
    int mRegexSearchesInLastBatch = 0;
    uint32_t mEpoch = 0;

    // One word, so a single fetch_add claims a chunk and says which batch, and how large, it belongs to;
    // a late claim gets an index past the count and touches nothing. Epoch 0 means no batch yet, hence
    // helpers start with seen == 0 and publish() pre-increments.
    alignas(scmCacheLine) std::atomic<uint64_t> mCursor{0};
    // Chunks finished:32 | regex searches run:32, so a chunk reports both in the fetch_add the join waits on
    alignas(scmCacheLine) std::atomic<uint64_t> mDone{0};
};

#endif // MUDLET_TRIGGERMATCHPOOL_H
