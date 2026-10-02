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


// Recycles capture list nodes between trigger fires: splice() moves nodes without the allocator and
// assigning into a recycled string reuses its buffer, so a fire shaped like the last allocates nothing.
//
// No lock: triggers and the Lua engine run on the main thread, and nothing else may reach this pool.
class TCaptureNodePool
{
public:
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
        // Else a line-sized capture would hold its buffer in the pool for the rest of the session
        for (auto it = used.begin(); it != used.end();) {
            if (it->capacity() > scmMaxPooledCapture) {
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
    static size_t roomFor(const size_t held) { return (held >= scmMaxPooledNodes) ? 0 : (scmMaxPooledNodes - held); }

    // Caps the high-water mark one match-all pattern over a hostile line would otherwise set for the
    // session; enough for any ordinary fire, including nested filter fires and open multiline states.
    static constexpr size_t scmMaxPooledNodes = 512;
    static constexpr std::string::size_type scmMaxPooledCapture = 1024;
    // Destroyed at static teardown, after every Host; a TMatchState outliving them (e.g. a
    // namespace-scope object in a test binary) would park into destroyed lists.
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

    // A copy would take nodes out of circulation without parking them; states are moved in unique_ptrs.
    TMatchState(const TMatchState&) = delete;
    TMatchState& operator=(const TMatchState&) = delete;

    // Destroyed only once nothing reads its captures: TTrigger removes a completed or expired
    // state from its condition map before running any script.
    ~TMatchState()
    {
        for (auto& captures : multiCaptureList) {
            TCaptureNodePool::park(captures);
        }
        for (auto& positions : multiCapturePosList) {
            TCaptureNodePool::park(positions);
        }
    }

    // Recycled nodes: copying the lists would allocate per capture on every condition an open trigger matches
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
