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
// A parked helper sleeps in std::atomic::wait. Before libc++ 22 that was, on
// Windows, a loop of sleeps that re-checked the word rather than a kernel wait,
// which would have every helper waking every 8ms for as long as Mudlet ran;
// refuse the toolchain rather than ship that.
#if defined(Q_OS_WIN) && defined(_LIBCPP_VERSION)
static_assert(_LIBCPP_VERSION >= 220000, "TriggerMatchPool needs libc++ 22 or newer on Windows: older ones poll in std::atomic::wait");
#endif

TriggerMatchPool* TriggerMatchPool::smpInstance = nullptr;

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        // The caller spin-waits for the chunk this thread has claimed, and a
        // spin on an atomic gets none of the priority boost the kernel gives a
        // lock, so a helper scheduled below the main thread can be descheduled
        // underneath it - on Apple Silicon possibly onto an efficiency core.
        // High is the main thread's class on macOS (user-interactive) and no
        // power throttling on Windows.
        thread->setServiceLevel(QThread::QualityOfService::High);
#endif
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
    // An empty batch is what wakes a parked helper; it has nothing to claim.
    // The flag can be relaxed because every helper reads the cursor with
    // acquire on its way round the loop - in wait(), at the top of the spin
    // or in a claim - and that pairs with the store in publish(), so the flag
    // is in view by the time the loop checks it.
    publish(0);
    for (const auto& thread : mThreads) {
        thread->wait();
    }
    mThreads.clear();
}

// With nobody asleep the notify is a waiter-count check, no syscall. The store
// is seq_cst rather than release so that it cannot pass the library's read of
// that count: a helper that has just registered as a waiter and is about to
// sleep on the old word must be woken, and release plus a later load is the
// one ordering x86 does not keep. One xchg per batch buys that on every
// library path rather than only the proxy one a 64-bit word takes today.
void TriggerMatchPool::publish(const int chunkCount)
{
    mCursor.store(packCursor(++mEpoch, chunkCount));
    mCursor.notify_all();
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
        // Sleeps until a publish changes the word from the one just read. A
        // claim that moved it in between - each participant makes at most one
        // on a batch that is already over - returns at once instead; the loop
        // then reads the same epoch and comes back here.
        mCursor.wait(cursor, std::memory_order_acquire);
    }
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
    publish(chunkCount);

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
