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

#include <algorithm>
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

    void append(const TCaptureGroups& captures)
    {
        const std::size_t offset = mBytes.size();
        mBytes.append(captures.mBytes);
        for (const std::size_t end : captures.mEnds) {
            mEnds.push_back(offset + end);
        }
    }

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

    // Empties it, and frees the storage of a fire past the bounds, which would
    // otherwise stay for the session. Judged by size, not capacity: growing
    // doubles the storage, so a fire within them can leave up to twice them.
    void clearAndTrim(const std::size_t maxCaptures = scmMaxKeptCaptures, const std::size_t maxBytes = scmMaxKeptBytes)
    {
        if (mBytes.size() > maxBytes) {
            std::string().swap(mBytes);
        }
        if (mEnds.size() > maxCaptures) {
            std::vector<std::size_t>().swap(mEnds);
        }
        clear();
    }

    void reserve(const std::size_t captures, const std::size_t bytes)
    {
        mEnds.reserve(captures);
        mBytes.reserve(bytes);
    }

    std::size_t bytes() const { return mBytes.size(); }
    std::size_t bytesCapacity() const { return mBytes.capacity(); }
    std::size_t capturesCapacity() const { return mEnds.capacity(); }

private:
    std::string mBytes;
    // Where each capture ends in mBytes; it starts where the one before ends
    std::vector<std::size_t> mEnds;
};

// A multiline match's captures, condition by condition, as flat as
// TCaptureGroups, so a match shaped like the last allocates nothing
class TMultiCaptureGroups
{
public:
    // One per condition met, those that capture nothing included
    std::size_t conditions() const { return mConditionEnds.size(); }
    bool empty() const { return mConditionEnds.empty(); }

    const TCaptureGroups& captures() const { return mCaptures; }
    std::size_t capturesBegin(const std::size_t condition) const { return condition ? mConditionEnds[condition - 1].captures : 0; }
    std::size_t capturesEnd(const std::size_t condition) const { return mConditionEnds[condition].captures; }

    // Each named group as its name followed by its capture
    const TCaptureGroups& namedCaptures() const { return mNamedCaptures; }
    std::size_t namedBegin(const std::size_t condition) const { return condition ? mConditionEnds[condition - 1].namedCaptures : 0; }
    std::size_t namedEnd(const std::size_t condition) const { return mConditionEnds[condition].namedCaptures; }

    // Room for this many conditions shaped like the one given, so a fresh state
    // does not regrow condition by condition; capped at the bounds, or a guess
    // past a match that fits them would be kept with it
    void reserve(const std::size_t conditions, const TCaptureGroups& captures, const NameGroupMatches* namedCaptures, const std::size_t maxCaptures, const std::size_t maxBytes)
    {
        mConditionEnds.reserve(std::min(conditions, maxCaptures));
        mCaptures.reserve(std::min(conditions * captures.size(), maxCaptures), std::min(conditions * captures.bytes(), maxBytes));
        if (namedCaptures) {
            std::size_t bytes = 0;
            for (const auto& [name, capture] : *namedCaptures) {
                bytes += static_cast<std::size_t>(name.size() + capture.size());
            }
            mNamedCaptures.reserve(std::min(conditions * 2 * static_cast<std::size_t>(namedCaptures->size()), maxCaptures), std::min(conditions * bytes, maxBytes));
        }
    }

    void addCondition(const TCaptureGroups& captures, const NameGroupMatches* namedCaptures)
    {
        mCaptures.append(captures);
        if (namedCaptures) {
            for (const auto& [name, capture] : *namedCaptures) {
                mNamedCaptures.append(QStringView(name));
                mNamedCaptures.append(QStringView(capture));
            }
        }
        mConditionEnds.push_back({mCaptures.size(), mNamedCaptures.size()});
    }

    void clearAndTrim(const std::size_t maxCaptures = TCaptureGroups::scmMaxKeptCaptures, const std::size_t maxBytes = TCaptureGroups::scmMaxKeptBytes)
    {
        mCaptures.clearAndTrim(maxCaptures, maxBytes);
        mNamedCaptures.clearAndTrim(maxCaptures, maxBytes);
        if (mConditionEnds.size() > maxCaptures) {
            std::vector<ConditionEnd>().swap(mConditionEnds);
        }
        mConditionEnds.clear();
    }

private:
    struct ConditionEnd
    {
        std::size_t captures;
        std::size_t namedCaptures;
    };
    TCaptureGroups mCaptures;
    TCaptureGroups mNamedCaptures;
    std::vector<ConditionEnd> mConditionEnds;
};


class TMatchState
{
public:
    TMatchState(int numberOfConditions, int delta)
    : mNumberOfConditions(numberOfConditions)
    , mDelta(delta)
    {
    }

    // Every multiline trigger keeps a spare state, so it keeps far less than the
    // interpreter's one copy: enough for an ordinary match over long lines
    static constexpr std::size_t scmMaxSpareCaptures = 64;
    static constexpr std::size_t scmMaxSpareBytes = 4096;

    void addCaptures(const TCaptureGroups& captures, const NameGroupMatches* namedCaptures)
    {
        if (mCaptures.empty()) {
            mCaptures.reserve(static_cast<std::size_t>(mNumberOfConditions), captures, namedCaptures, scmMaxSpareCaptures, scmMaxSpareBytes);
        }
        mCaptures.addCondition(captures, namedCaptures);
    }
    const TMultiCaptureGroups& captures() const { return mCaptures; }

    // Keeps the emptied storage of a match within the spare bounds for reopen()
    void recycle() { mCaptures.clearAndTrim(scmMaxSpareCaptures, scmMaxSpareBytes); }

    // Resets what the constructor sets; recycle() must already have emptied the captures
    void reopen(const int numberOfConditions, const int delta)
    {
        mNumberOfConditions = numberOfConditions;
        mNextCondition = 1;
        mLineCount = 1;
        mDelta = delta;
        mSpacer = 0;
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

    int mNumberOfConditions = 0;
    // first condition was true when the state was created
    int mNextCondition = 1;
    int mLineCount = 1;
    int mDelta = 0;
    int mSpacer = 0;

private:
    TMultiCaptureGroups mCaptures;
};

#endif // MUDLET_TMATCHSTATE_H
