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
    // Patterns shorter than this hold no n-gram to be filed under, and their
    // triggers are offered every line as before.
    static constexpr int scmGramLength = 5;

    // The n-gram a pattern is filed under, or 0 when it is too short. Picking
    // the rarest-looking one spreads patterns across the index, which is what
    // keeps the candidate list short.
    static quint32 patternGram(const QString& pattern);

    void rebuild(const std::vector<TTrigger*>& roots);
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

    bool mActive = false;
    // Which n-grams anything is filed under at all - one bit test rejects most
    // of a line's positions before the map is consulted.
    std::vector<quint64> mGramBits;
    std::unordered_map<quint32, std::vector<int>> mIndex;
    // Positions of the triggers that cannot be filtered, kept sorted so the
    // merge below hands processDataStream() its triggers in root-list order.
    std::vector<int> mUnfiltered;
    mutable std::vector<quint32> mSeen;
    mutable quint32 mGeneration = 0;
};

#endif // MUDLET_TTRIGGERPRESCAN_H
