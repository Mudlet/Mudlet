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

#include "MudletApp.h"
#include "TTrigger.h"
#include "utils.h"

#include <QDebug>
#include <QSettings>
#include <QThread>

#include <algorithm>

// A claim is one 64-bit fetch_add; behind a lock the helpers would serialise on it.
static_assert(std::atomic<uint64_t>::is_always_lock_free);
// Before libc++ 22, std::atomic::wait on Windows polled with sleeps, waking every parked helper every 8ms.
#if defined(Q_OS_WIN) && defined(_LIBCPP_VERSION)
static_assert(_LIBCPP_VERSION >= 220000, "TriggerMatchPool needs libc++ 22 or newer on Windows: older ones poll in std::atomic::wait");
#endif

TriggerMatchPool* TriggerMatchPool::smpInstance = nullptr;

namespace {
// Spin-wait hint, freeing resources for a sibling hyperthread. Other architectures spin bare, only slower.
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
// Half the index field: each participant can overshoot the count by one claim per batch, and those
// must not spill into the count field.
constexpr int kMaxChunks = 32768;
// Spin loops read the clock once per this many pauses: often enough to honour a budget to within a few
// microseconds, seldom enough that the clock read does not dominate the spin on x86.
constexpr int kPausesPerClockCheck = 64;
// How long the caller pauses for a claimed chunk before yielding instead, in case its helper was descheduled
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

constexpr uint64_t kDoneShift = 32;

uint64_t packDone(const int searches)
{
    return (static_cast<uint64_t>(1) << kDoneShift) | static_cast<uint32_t>(searches);
}

int doneChunksOf(const uint64_t done)
{
    return static_cast<int>(done >> kDoneShift);
}

int searchesOf(const uint64_t done)
{
    return static_cast<int>(done & 0xFFFFFFFF);
}

// The environment beats Mudlet.ini so a test or benchmark can pin a knob for one run. An invalid value
// is refused out loud: a typo that quietly measured the default is the worst outcome for a tuning knob.
int knobOr(const char* envName, const QString& iniKey, const int fallback, const int minimum)
{
    if (qEnvironmentVariableIsSet(envName)) {
        bool parsed = false;
        const int value = qEnvironmentVariableIntValue(envName, &parsed);
        if (!parsed || value < minimum) {
            qWarning().nospace() << envName << " is set to " << qEnvironmentVariable(envName) << " but is not an integer of at least " << minimum << "; using " << fallback;
            return fallback;
        }
        return value;
    }
    // Null until mudlet::setupConfig() has settled the config root, so an early pool or a harness runs on the defaults
    QSettings* settings = MudletApp::getQSettings();
    if (settings && settings->contains(iniKey)) {
        bool parsed = false;
        const int value = settings->value(iniKey).toInt(&parsed);
        if (!parsed || value < minimum) {
            qWarning().nospace().noquote() << "Mudlet.ini sets " << iniKey << " to \"" << settings->value(iniKey).toString() << "\" but that is not an integer of at least " << minimum << "; using "
                                           << fallback;
            return fallback;
        }
        return value;
    }
    return fallback;
}
} // namespace

TriggerMatchPool& TriggerMatchPool::instance()
{
    static TriggerMatchPool pool;
    return pool;
}

void TriggerMatchPool::shutdown()
{
    if (smpInstance) {
        smpInstance->stopHelpers();
    }
}

TriggerMatchPool::TriggerMatchPool()
{
    smpInstance = this;
    const int cores = std::max(1, QThread::idealThreadCount());
    // Past four threads the fork-join tail grows faster than each thread's share of work shrinks.
    // Zero turns the pool off.
    const int wanted = std::min(knobOr("MUDLET_MATCH_THREADS", qsl("triggerMatchThreads"), std::min(4, cores / 2), 0), cores);
    // Two-thread break-even on a Release build, in regex searches per line: below it the fork-join costs
    // the main thread as much as it hands away, while the helper spins a core for nothing.
    mThreshold = knobOr("MUDLET_MATCH_THRESHOLD", qsl("triggerMatchThreshold"), 128, 1);
    mFloodChunkLines = knobOr("MUDLET_MATCH_FLOOD_LINES", qsl("triggerMatchFloodLines"), 8, 1);
    // Zero parks a helper as soon as a batch is exhausted, putting a wake-up under every line of a burst
    mSpinBudget = std::chrono::microseconds(knobOr("MUDLET_MATCH_SPIN_US", qsl("triggerMatchSpinMicroseconds"), 100, 0));
    if (wanted < 2) {
        return;
    }

    mScratch.resize(wanted, nullptr);
    for (int i = 0; i < wanted; ++i) {
        // One ovector pair serves every pattern for a yes/no answer: PCRE2 returns 0, not failure,
        // for a match it had no room to record.
        mScratch[i] = pcre2_match_data_create(1, nullptr);
        if (!mScratch[i]) {
            // A slot without scratch cannot answer for regex triggers; declining every batch is only
            // slower, while a slot that lies is wrong.
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
        // QThread, not std::thread, for the name: it reaches the OS on all platforms (Windows since
        // Qt 6.8, our floor), so profilers and crash reports show it.
        std::unique_ptr<QThread> thread(QThread::create([this, slot] {
            workerLoop(slot);
            mHelpersReturned.fetch_add(1, std::memory_order_release);
        }));
        thread->setObjectName(qsl("TriggerMatch-%1").arg(slot));
        thread->start();
        if (!thread->isRunning()) {
            // Qt has already warned. The caller claims whatever chunks nobody else does, so the pool
            // works with fewer helpers, but a dead slot must not count as a worker.
            break;
        }
        mThreads.push_back(std::move(thread));
    }
}

TriggerMatchPool::~TriggerMatchPool()
{
    // For a process that never called shutdown().
    stopHelpers();
    for (auto* scratch : mScratch) {
        pcre2_match_data_free(scratch);
    }
    smpInstance = nullptr;
}

void TriggerMatchPool::stopHelpers()
{
    if (mThreads.empty()) {
        return;
    }
    mStop.store(true, std::memory_order_relaxed);
    // An empty batch wakes parked helpers. mStop can be relaxed: each helper acquire-reads the cursor
    // on every loop (in wait(), the spin or a claim), which pairs with publish()'s store.
    publish(0);
    for (const auto& thread : mThreads) {
        thread->wait();
    }
    [[maybe_unused]] const int returned = mHelpersReturned.load(std::memory_order_acquire);
    Q_ASSERT(returned == static_cast<int>(mThreads.size()));
    mThreads.clear();
}

// With nobody asleep the notify is a waiter-count check, no syscall. The store is seq_cst, not release,
// so it cannot pass the library's read of that count: a helper about to sleep on the old word must be
// woken, and x86 reorders a release store with a later load. One xchg per batch covers every library
// path, not only the proxy one a 64-bit word takes today.
void TriggerMatchPool::publish(const int chunkCount)
{
    mCursor.store(packCursor(++mEpoch, chunkCount));
    mCursor.notify_all();
}

// Returns the epoch of the last claim: the batch this thread has now seen.
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
        int searches = 0;
        for (int i = begin; i < end; ++i) {
            TTrigger* trigger = mJob.triggers[i];
            trigger->setPrescanVerdict(mJob.passId, trigger->prescanMayFire(mJob.subject, mJob.subjectLength, *mJob.haystack, *mJob.lineBigrams, scratch, searches));
        }
        mDone.fetch_add(packDone(searches), std::memory_order_release);
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
        const uint64_t cursor = mCursor.load(std::memory_order_acquire);
        if (epochOf(cursor) != seen) {
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
        // Sleeps until a publish changes the word. If a late claim (at most one per participant on a
        // finished batch) moved it first, this returns at once and the loop comes back here.
        mCursor.wait(cursor, std::memory_order_acquire);
    }
}

bool TriggerMatchPool::prescan(
        TTrigger* const* triggers, const int count, const quint32 passId, const char* subject, const int subjectLength, const QString& haystack, const TBigramFilter& lineBigrams)
{
    if (mThreads.empty() || count <= 0) {
        return false;
    }

    ++mPrescanCount;
    lineBigrams.prepareForSharing();
    const int chunkSize = std::max(kChunkSize, (count + kMaxChunks - 1) / kMaxChunks);
    const int chunkCount = (count + chunkSize - 1) / chunkSize;
    mJob.triggers = triggers;
    mJob.count = count;
    mJob.chunkSize = chunkSize;
    mJob.passId = passId;
    mJob.subject = subject;
    mJob.subjectLength = subjectLength;
    mJob.haystack = &haystack;
    mJob.lineBigrams = &lineBigrams;

    mDone.store(0, std::memory_order_relaxed);
    publish(chunkCount);

    runChunks(0);

    // Only claimed chunks are outstanding; a helper still waking holds none, so it is never waited for.
    int pauses = 0;
    std::chrono::steady_clock::time_point yieldAt{};
    while (doneChunksOf(mDone.load(std::memory_order_acquire)) != chunkCount) {
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
    uint64_t done = mDone.load(std::memory_order_acquire);
    while (doneChunksOf(done) != chunkCount) {
        QThread::yieldCurrentThread();
        done = mDone.load(std::memory_order_acquire);
    }
    mRegexSearchesInLastBatch = searchesOf(done);
    Q_ASSERT(chunkIndexOf(mCursor.load(std::memory_order_relaxed)) >= chunkCount);
    return true;
}
