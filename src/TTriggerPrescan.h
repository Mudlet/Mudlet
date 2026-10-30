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

#include <array>
#include <unordered_map>
#include <vector>

class TTrigger;

// Answers "which triggers could match this line?" without touching the triggers, so thousands of
// plain-text patterns cost the line's length rather than the trigger count.
//
// A pattern can only match a line holding all the n-grams of the text its matches contain (see
// TPatternLiteral), so each is filed under one of them and offered only to lines that have it. The test
// is one-sided - it never hides a trigger that would match, and match() still decides every candidate -
// on a line whose UTF-8 holds the same text (see TUtf8Subject::dropsText()).
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

    // One trigger's worth of work each, so arming and killing triggers every line costs no rebuild. A
    // freed slot is handed out again; grams must not be empty.
    int fileSlot(const std::vector<quint64>& grams);
    void removeSlot(int slot);
    void refileSlot(int slot, const std::vector<quint64>& grams);
    // For freeing many at once: costs one pass over what stays filed, not one per slot
    void removeSlots(const std::vector<int>& freed);

    // The filed slots the line's n-grams hit, each once, in no particular order
    void hits(const QString& line, std::vector<int>& out) const;
    // Each refiled every slot, so a test can check the incremental path keeps them rare
    quint64 reindexCount() const { return mReindexes; }

private:
    // reindex() reclaims abandoned mGramPool entries once they outnumber both the filed ones and this
    static constexpr int scmMinimumWaste = 64;
    static constexpr quint32 scmGramBitsLog = 16;
    static constexpr quint32 scmGramBitsMask = (1u << scmGramBitsLog) - 1;

    static quint32 gramBit(quint64 gram);
    static int gramLength(const quint64 gram) { return static_cast<int>(gram >> 32); }

    // A slot's slice of mGramPool; a count of 0 marks a free slot
    struct GramRange
    {
        int mOffset = 0;
        int mCount = 0;
    };

    void fileGrams(int slot, const std::vector<quint64>& grams);
    void unfileGrams(int slot);
    void compactIfWasteful();
    void reindex();

    // Which n-grams anything is filed under: one bit test rejects most of a line's before the map lookup.
    // Unfiling leaves bits set, as grams share them, and keeps emptied buckets for the next trigger
    // filed under them, both until reindex().
    std::vector<quint64> mGramBits;
    std::unordered_map<quint64, std::vector<int>> mIndex;
    // Lets unfiling a slot find its buckets without a scan. Append-only, so a removed or refiled slot
    // abandons its entries until reindex() reclaims them.
    std::vector<GramRange> mSlotGrams;
    std::vector<quint64> mGramPool;
    int mFiledGrams = 0;
    std::vector<int> mFreeSlots;
    // Grams filed per length, so a line is only walked at lengths in use
    std::array<int, scmMaxGramLength + 1> mLengthUse{};
    mutable std::vector<quint32> mSeen;
    mutable quint32 mGeneration = 0;
    quint64 mReindexes = 0;
};

// A parent's children in walk order, with the ones a pass along the line has to enter, so it need not
// touch the rest. Built by TriggerUnit's tree index rebuild, and for the root list by its snapshot; see
// TriggerUnit::markTreeNode().
struct TChildWalk
{
    // Position of the next child to enter at or after position, or past the end when none is
    int nextToEnter(int position, quint32 lineStamp) const;
    // False when position was marked for this line already
    bool mark(int position, quint32 lineStamp);
    void setAlways(int position, bool always);

    std::vector<TTrigger*> mChildren;
    // One bit per position in mChildren. Entered on every line: an unfiled child, other than a folder
    // with nothing below it to enter every line
    std::vector<quint64> mAlways;
    // Entered on the line stamped mMarkedStamp only: a child the index hit or that acts without
    // matching, or a child above one
    std::vector<quint64> mMarked;
    quint32 mMarkedStamp = 0;
    // Cleared by any change to the children or to how one is filed: mChildren may then name freed
    // triggers, so the walk goes down the live list. Only an outermost pass's rebuild frees or replaces it.
    // Unused for the root list, whose snapshot nulls a freed trigger instead.
    bool mValid = false;
};

#endif // MUDLET_TTRIGGERPRESCAN_H
