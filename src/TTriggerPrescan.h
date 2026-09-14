#ifndef MUDLET_TTRIGGERPRESCAN_H
#define MUDLET_TTRIGGERPRESCAN_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
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

#include <QString>

#include <algorithm>
#include <array>
#include <unordered_map>
#include <vector>

class TTrigger;

// Answers "which of these triggers could possibly match this line?" without
// touching the triggers themselves, so a profile with thousands of plain-text
// patterns pays for the line's length rather than for its trigger count.
//
// A pattern that matches by containment - substring, start of line, exact match -
// can only match a line that holds every one of the pattern's own character
// n-grams. Each such pattern is filed under one n-gram it contains, and a line
// only reaches the triggers filed under n-grams the line actually has. The test
// is one-sided: it never hides a trigger that would have matched, and the
// ordinary match() still decides every candidate it lets through.
class TTriggerPrescan
{
public:
    // A pattern is filed under an n-gram of its own length, up to this. Longer
    // n-grams are rarer and so spread patterns further apart, which is the whole
    // point, but capping the length is what lets a short pattern be filed at all.
    static constexpr int scmMaxGramLength = 5;
    // Under two characters an n-gram rejects nothing: a line holds so many of
    // them that every trigger filed under one comes back as a candidate anyway.
    // Those triggers are offered every line instead, as they were before.
    static constexpr int scmMinGramLength = 2;

    // The n-gram a pattern is filed under, or 0 when it is too short. Picking
    // the rarest-looking one spreads patterns across the index, which is what
    // keeps the candidate list short.
    static quint64 patternGram(const QString& pattern);

    void rebuild(const std::vector<TTrigger*>& roots);
    // Maintenance for the root-list changes that leave every existing position
    // where it was, so that registering or killing one trigger costs one
    // trigger's work rather than a rebuild of the whole index. A slot is never
    // reused once removed, which is what keeps the positions stable; a rebuild
    // reclaims them once shouldRebuild() says the accumulated holes have grown
    // to outweigh what the incremental path saves.
    void appendSlot(const std::vector<quint64>& grams);
    void removeSlot(int position);
    void refileSlot(int position, const std::vector<quint64>& grams);
    bool shouldRebuild() const { return mMutations >= std::max(scmMinimumIndexedTriggers, mLiveSlots); }
    bool active() const { return mActive; }

    // Fills, in root-list order, the positions of every trigger this line has
    // to be offered. `scratch` is caller-owned so that a trigger script feeding
    // more text back through the pipeline cannot disturb an outer pass.
    void candidates(const QString& line, std::vector<int>& scratch, std::vector<int>& out) const;

private:
    // Below this the plain walk is cheaper than filtering it.
    static constexpr int scmMinimumIndexedTriggers = 32;
    static constexpr quint32 scmGramBitsLog = 16;
    static constexpr quint32 scmGramBitsMask = (1u << scmGramBitsLog) - 1;

    static quint32 gramBit(quint64 gram);
    static int gramLength(const quint64 gram) { return static_cast<int>(gram >> 32); }

    // Where in mGramPool one position's grams live. A count of -1 marks a
    // position whose trigger is gone, which is not the same as the 0 of a
    // trigger that is present but cannot be filtered.
    struct GramRange
    {
        int mOffset = 0;
        int mCount = 0;
    };
    static constexpr int scmRemovedSlot = -1;

    void fileSlot(int position, const std::vector<quint64>& grams);
    void unfileSlot(int position);

    bool mActive = false;
    // Which n-grams anything is filed under at all - one bit test rejects most
    // of a line's positions before the map is consulted.
    std::vector<quint64> mGramBits;
    std::unordered_map<quint64, std::vector<int>> mIndex;
    // Positions of the triggers that cannot be filtered, kept sorted so the
    // merge below hands processDataStream() its triggers in root-list order.
    std::vector<int> mUnfiltered;
    // Every position's grams, so that removing one trigger can find the buckets
    // it is filed in without a scan. Append-only: a refiled position points at
    // fresh entries and abandons its old ones until the next rebuild.
    std::vector<GramRange> mSlotGrams;
    std::vector<quint64> mGramPool;
    // How many grams of each length are filed, so that a line is only walked
    // again at a length something is actually filed under.
    std::array<int, scmMaxGramLength + 1> mLengthUse{};
    int mIndexedSlots = 0;
    int mLiveSlots = 0;
    int mMutations = 0;
    mutable std::vector<quint32> mSeen;
    mutable quint32 mGeneration = 0;
};

#endif // MUDLET_TTRIGGERPRESCAN_H
