#ifndef MUDLET_TMATCHSTATE_H
#define MUDLET_TMATCHSTATE_H

/***************************************************************************
 *   Copyright (C) 2008-2010 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022, 2026 by Stephen Lyons - slysven@virginmedia.com   *
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

#include "utils.h" // For NameGroupMatches

#include <iterator>
#include <list>
#include <string>

#include <QString>
#include <QPair>
#include <QVector>


// Trigger captures are built and thrown away again on every fire, which costs
// a list node per capture plus a buffer for any capture the small string
// optimisation cannot hold. The nodes of a finished fire, and of a multiline
// state that has closed, are parked here rather than freed: splice() moves a
// node between lists without going near the allocator, and assigning a capture
// into a recycled string reuses the buffer it already has, so a fire that
// follows one of the same shape allocates nothing at all.
//
// No lock is needed because Mudlet runs every profile's triggers, and the Lua
// engine they call into, on the main thread; nothing else reaches this pool.
class TCaptureNodePool
{
public:
    // Both return a reference to a node now owned by `into`, recycled if the
    // pool has one and freshly made if it does not
    static std::string& takeCapture(std::list<std::string>& into)
    {
        if (smSpareCaptures.empty()) {
            into.emplace_back();
        } else {
            into.splice(into.end(), smSpareCaptures, smSpareCaptures.begin());
        }
        return into.back();
    }

    static int& takePosition(std::list<int>& into)
    {
        if (smSparePositions.empty()) {
            into.emplace_back();
        } else {
            into.splice(into.end(), smSparePositions, smSparePositions.begin());
        }
        return into.back();
    }

    static void park(std::list<std::string>& used)
    {
        // A capture as big as a whole line would otherwise hold its buffer in
        // the pool for the rest of the session
        for (auto it = used.begin(); it != used.end();) {
            if (it->capacity() > smMaxPooledCapture) {
                it = used.erase(it);
            } else {
                ++it;
            }
        }
        const size_t room = roomFor(smSpareCaptures.size());
        if (used.size() <= room) {
            smSpareCaptures.splice(smSpareCaptures.end(), used);
            return;
        }
        auto last = used.begin();
        std::advance(last, room);
        smSpareCaptures.splice(smSpareCaptures.end(), used, used.begin(), last);
    }

    static void park(std::list<int>& used)
    {
        const size_t room = roomFor(smSparePositions.size());
        if (used.size() <= room) {
            smSparePositions.splice(smSparePositions.end(), used);
            return;
        }
        auto last = used.begin();
        std::advance(last, room);
        smSparePositions.splice(smSparePositions.end(), used, used.begin(), last);
    }

private:
    // Whatever the pool has no room for is left behind for the list being
    // emptied to free in the ordinary way
    static size_t roomFor(const size_t held) { return (held >= smMaxPooledNodes) ? 0 : (smMaxPooledNodes - held); }

    // The pool only ever reaches the most nodes that were in use at once, but
    // one match-all pattern over a hostile line would set that high water mark
    // for the rest of the session. Hold enough for any ordinary fire, the
    // nested ones a filter trigger makes and any multiline state still open
    // included, and free the rest. Past the cap the cost is the allocation
    // this pool exists to save, never unbounded memory.
    static constexpr size_t smMaxPooledNodes = 512;
    static constexpr std::string::size_type smMaxPooledCapture = 1024;
    inline static std::list<std::string> smSpareCaptures;
    inline static std::list<int> smSparePositions;
};


class TMatchState
{
public:
    TMatchState(int numberOfConditions, int delta)
    : mNumberOfConditions(numberOfConditions)
    , mDelta(delta)
    {
    }

    // Copy constructor - deliberately does not carry over the capture
    // containers, so a copied state starts with empty captures:
    TMatchState(const TMatchState& ms)
    : mNumberOfConditions(ms.mNumberOfConditions)
    , mNextCondition(ms.mNextCondition)
    , mLineCount(ms.mLineCount)
    , mDelta(ms.mDelta)
    , mSpacer(ms.mSpacer)
    {
    }

    // Pair the user-defined copy constructor with an explicit copy assignment
    // (Rule of Two). Note the two are deliberately asymmetric: unlike the
    // constructor above, this defaulted assignment copies every member,
    // capture containers included. That reproduces the previously implicit
    // assignment exactly, so behaviour is unchanged:
    TMatchState& operator=(const TMatchState& ms) = default;

    // A state is only ever destroyed once nothing reads its captures any more:
    // TTrigger takes a completed one out of its condition map before running
    // any script, and drops an expired one before that
    ~TMatchState()
    {
        for (auto& captures : multiCaptureList) {
            TCaptureNodePool::park(captures);
        }
        for (auto& positions : multiCapturePosList) {
            TCaptureNodePool::park(positions);
        }
    }

    // Takes recycled nodes rather than copy-constructing the lists, which
    // would allocate one per capture on every condition a still-open trigger
    // matches
    void addCaptures(const std::list<std::string>& captures, const std::list<int>& positions)
    {
        auto& targetCaptures = multiCaptureList.emplace_back();
        for (const auto& capture : captures) {
            TCaptureNodePool::takeCapture(targetCaptures).assign(capture);
        }
        auto& targetPositions = multiCapturePosList.emplace_back();
        for (const int position : positions) {
            TCaptureNodePool::takePosition(targetPositions) = position;
        }
    }

    int nextCondition() { return mNextCondition; }
    void conditionMatched() { mNextCondition++; }
    bool isComplete() { return (mNextCondition >= mNumberOfConditions); }
    void newLineArrived() { mLineCount++; }
    bool newLine() { return !(mLineCount > mDelta); }

    bool lineSpacerMatch(int lines)
    {
        if (mSpacer >= lines) {
            mSpacer = 0;
            return true;
        }
        ++mSpacer;
        return false;
    }

    std::list<std::list<std::string>> multiCaptureList;
    std::list<std::list<int>> multiCapturePosList;
    QVector<NameGroupMatches> nameCaptures;
    int mNumberOfConditions = 0;
    // first condition was true when the state was created
    int mNextCondition = 1;
    int mLineCount = 1;
    int mDelta = 0;
    int mSpacer = 0;
};

#endif // MUDLET_TMATCHSTATE_H
