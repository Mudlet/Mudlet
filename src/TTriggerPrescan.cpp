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

#include "TTriggerPrescan.h"

#include "TTrigger.h"

#include <algorithm>
#include <limits>

static inline quint32 gramHash(const QChar* p)
{
    quint64 v = 0;
    for (int i = 0; i < TTriggerPrescan::scmGramLength; ++i) {
        v = v * 0x100000001b3ULL + p[i].unicode();
    }
    v ^= v >> 29;
    v *= 0xff51afd7ed558ccdULL;
    v ^= v >> 32;
    // 0 marks "no gram", so the one hash that would collide with it is nudged
    return static_cast<quint32>(v) | 1u;
}

// Letter frequencies in English text, with a space scored as commoner than any
// letter. The scale does not have to be right - it only has to make different
// patterns prefer different n-grams.
static inline double characterScore(const QChar c)
{
    static constexpr double letters[26] = {8.2, 1.5, 2.8, 4.3, 12.7, 2.2, 2.0, 6.1, 7.0, 0.15, 0.77, 4.0, 2.4, 6.7, 7.5, 1.9, 0.095, 6.0, 6.3, 9.1, 2.8, 0.98, 2.4, 0.15, 2.0, 0.074};
    const char16_t u = c.unicode();
    if (u == u' ') {
        return 20.0;
    }
    const char16_t lower = (u >= u'A' && u <= u'Z') ? (u + 32) : u;
    if (lower >= u'a' && lower <= u'z') {
        return letters[lower - u'a'];
    }
    return 0.5;
}

quint32 TTriggerPrescan::patternGram(const QString& pattern)
{
    if (pattern.size() < scmGramLength) {
        return 0;
    }
    const QChar* const data = pattern.constData();
    double bestScore = std::numeric_limits<double>::max();
    int bestPosition = 0;
    for (int i = 0; i + scmGramLength <= pattern.size(); ++i) {
        double score = 0.0;
        for (int k = 0; k < scmGramLength; ++k) {
            score += characterScore(data[i + k]);
        }
        if (score < bestScore) {
            bestScore = score;
            bestPosition = i;
        }
    }
    return gramHash(data + bestPosition);
}

void TTriggerPrescan::rebuild(const std::vector<TTrigger*>& roots)
{
    mActive = false;
    for (auto& bucket : mIndex) {
        bucket.second.clear();
    }
    mUnfiltered.clear();
    std::fill(mGramBits.begin(), mGramBits.end(), 0);

    int indexed = 0;
    const int rootCount = static_cast<int>(roots.size());
    for (int position = 0; position < rootCount; ++position) {
        const std::vector<quint32>& grams = roots[position]->prescanGrams();
        if (grams.empty()) {
            mUnfiltered.push_back(position);
            continue;
        }
        if (mGramBits.empty()) {
            mGramBits.assign((1u << scmGramBitsLog) / 64, 0);
        }
        for (const quint32 gram : grams) {
            mIndex[gram].push_back(position);
            const quint32 bit = gram & scmGramBitsMask;
            mGramBits[bit >> 6] |= (1ULL << (bit & 63));
        }
        ++indexed;
    }

    if (indexed < scmMinimumIndexedTriggers) {
        mIndex.clear();
        mUnfiltered.clear();
        mGramBits.clear();
        return;
    }
    mSeen.assign(roots.size(), 0);
    mGeneration = 0;
    mActive = true;
}

void TTriggerPrescan::candidates(const QString& line, std::vector<int>& scratch, std::vector<int>& out) const
{
    scratch.clear();
    out.clear();
    if (++mGeneration == 0) {
        // Wrapped, so every stamp left behind now reads as "this line"
        std::fill(mSeen.begin(), mSeen.end(), 0);
        mGeneration = 1;
    }

    const QChar* const data = line.constData();
    const int last = line.size() - scmGramLength;
    for (int i = 0; i <= last; ++i) {
        const quint32 gram = gramHash(data + i);
        const quint32 bit = gram & scmGramBitsMask;
        if (!(mGramBits[bit >> 6] & (1ULL << (bit & 63)))) {
            continue;
        }
        const auto bucket = mIndex.find(gram);
        if (bucket == mIndex.end()) {
            continue;
        }
        for (const int position : bucket->second) {
            if (mSeen[position] == mGeneration) {
                continue;
            }
            mSeen[position] = mGeneration;
            scratch.push_back(position);
        }
    }

    std::sort(scratch.begin(), scratch.end());
    out.resize(scratch.size() + mUnfiltered.size());
    std::merge(mUnfiltered.cbegin(), mUnfiltered.cend(), scratch.cbegin(), scratch.cend(), out.begin());
}
