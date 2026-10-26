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
#include <string_view>
#include <vector>

#include <QString>
#include <QPair>
#include <QVector>

// One fire's capture groups as UTF-8, back to back in a single buffer, so a fire
// shaped like the last allocates nothing and no capture is a string of its own
class TCaptureGroups
{
public:
    std::size_t size() const { return mEnds.size(); }
    bool empty() const { return mEnds.empty(); }

    std::string_view operator[](const std::size_t index) const
    {
        const std::size_t begin = index ? mEnds[index - 1] : 0;
        return {mBytes.data() + begin, mEnds[index] - begin};
    }

    std::string_view back() const { return (*this)[mEnds.size() - 1]; }

    void append(const char* bytes, const std::size_t length)
    {
        mBytes.append(bytes, length);
        mEnds.push_back(mBytes.size());
    }

    void append(const std::string_view capture) { append(capture.data(), capture.size()); }

    void appendEmpty() { mEnds.push_back(mBytes.size()); }

    // Encoded exactly as QString::toUtf8() would, so an unpaired surrogate
    // cannot shift the byte offset the capture reports, and cut at the first
    // NUL, matching what pushMatchesTable() hands Lua
    void append(const QStringView capture)
    {
        const std::size_t begin = mBytes.size();
        for (const QChar character : capture) {
            const char16_t unit = character.unicode();
            if (unit >= 0x80) {
                mBytes.resize(begin);
                const QByteArray utf8 = capture.toUtf8();
                mBytes.append(utf8.constData(), qstrnlen(utf8.constData(), static_cast<std::size_t>(utf8.size())));
                break;
            }
            if (unit == u'\0') {
                break;
            }
            mBytes.push_back(static_cast<char>(unit));
        }
        mEnds.push_back(mBytes.size());
    }

    void clear()
    {
        mBytes.clear();
        mEnds.clear();
    }

    // Well past what an ordinary fire needs, so a busier trigger keeps its storage instead of reallocating every fire
    static constexpr std::size_t scmMaxKeptCaptures = 2048;
    static constexpr std::size_t scmMaxKeptBytes = 32 * 1024;

    // Empties it, and gives back whatever a fire past the bounds grew it to,
    // which would otherwise stay for the session
    void clearAndTrim()
    {
        clear();
        if (mBytes.capacity() > scmMaxKeptBytes) {
            std::string().swap(mBytes);
        }
        if (mEnds.capacity() > scmMaxKeptCaptures) {
            std::vector<std::size_t>().swap(mEnds);
        }
    }

    std::size_t bytesCapacity() const { return mBytes.capacity(); }
    std::size_t capturesCapacity() const { return mEnds.capacity(); }

private:
    std::string mBytes;
    // Where each capture ends in mBytes; it starts where the one before ends
    std::vector<std::size_t> mEnds;
};

// Recycles the capture list nodes open multiline states hold: splice() moves nodes without the allocator
// and assigning into a recycled string reuses its buffer, so a condition shaped like the last allocates nothing.
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

    // Caps the high-water mark a match-all condition over a hostile line would otherwise set for the
    // session; enough for the open multiline states of any ordinary profile.
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
    void addCaptures(const TCaptureGroups& captures, const std::vector<int>& positions)
    {
        auto& targetCaptures = multiCaptureList.emplace_back();
        for (std::size_t i = 0; i < captures.size(); ++i) {
            const std::string_view capture = captures[i];
            TCaptureNodePool::takeCapture(targetCaptures).assign(capture.data(), capture.size());
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
