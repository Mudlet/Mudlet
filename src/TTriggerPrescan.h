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

// Answers "which triggers could match this line?" without touching the triggers, so thousands of
// plain-text patterns cost the line's length rather than the trigger count.
//
// A containment pattern (substring, start of line, exact) can only match a line holding all of its
// n-grams, so each is filed under one of them and offered only to lines that have it. The test is
// one-sided: it never hides a trigger that would match, and match() still decides every candidate.
class TTriggerPrescan
{
public:
    // A pattern is filed under an n-gram of its own length, up to this; longer n-grams are rarer and
    // spread patterns further apart.
    static constexpr int scmMaxGramLength = 5;
    // Shorter n-grams reject nothing since nearly every line holds them; such triggers are offered every line.
    static constexpr int scmMinGramLength = 2;

    // The pattern's rarest-looking n-gram, which keeps candidate lists short, or 0 when it is too short
    static quint64 patternGram(const QString& pattern);

    void rebuild(const std::vector<TTrigger*>& roots);
    // Incremental updates for root-list changes that keep existing positions, so one trigger costs one
    // trigger's work. Removed slots are never reused, keeping positions stable, until shouldRebuild()
    // says the holes outweigh what the incremental path saves.
    void appendSlot(const std::vector<quint64>& grams);
    void removeSlot(int position);
    void refileSlot(int position, const std::vector<quint64>& grams);
    bool shouldRebuild() const { return mMutations >= std::max(scmMinimumIndexedTriggers, mLiveSlots); }
    bool active() const { return mActive; }

    // Fills `out` in root-list order. `scratch` is caller-owned so a trigger script feeding text back
    // through the pipeline cannot disturb an outer pass.
    void candidates(const QString& line, std::vector<int>& scratch, std::vector<int>& out) const;

private:
    // Below this the plain walk is cheaper than filtering it.
    static constexpr int scmMinimumIndexedTriggers = 32;
    static constexpr quint32 scmGramBitsLog = 16;
    static constexpr quint32 scmGramBitsMask = (1u << scmGramBitsLog) - 1;

    static quint32 gramBit(quint64 gram);
    static int gramLength(const quint64 gram) { return static_cast<int>(gram >> 32); }

    // A position's slice of mGramPool. A count of -1 marks a removed trigger; 0 one that cannot be filtered.
    struct GramRange
    {
        int mOffset = 0;
        int mCount = 0;
    };
    static constexpr int scmRemovedSlot = -1;

    void fileSlot(int position, const std::vector<quint64>& grams);
    void unfileSlot(int position);

    bool mActive = false;
    // Which n-grams anything is filed under: one bit test rejects most of a line's before the map lookup
    std::vector<quint64> mGramBits;
    std::unordered_map<quint64, std::vector<int>> mIndex;
    // Sorted, so the merge hands processDataStream() its triggers in root-list order
    std::vector<int> mUnfiltered;
    // Lets removing a trigger find its buckets without a scan. Append-only: a refiled position abandons
    // its old entries until the next rebuild.
    std::vector<GramRange> mSlotGrams;
    std::vector<quint64> mGramPool;
    // Grams filed per length, so a line is only walked at lengths in use
    std::array<int, scmMaxGramLength + 1> mLengthUse{};
    int mIndexedSlots = 0;
    int mLiveSlots = 0;
    int mMutations = 0;
    mutable std::vector<quint32> mSeen;
    mutable quint32 mGeneration = 0;
};

#endif // MUDLET_TTRIGGERPRESCAN_H
