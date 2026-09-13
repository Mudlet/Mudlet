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

// The length rides in the high half, both so that the same characters read at
// two lengths are two different grams and so that 0 stays free to mean "no gram"
// - a real one always carries a length of at least scmMinGramLength.
static inline quint64 gramHash(const QChar* p, const int length)
{
    quint64 v = 0;
    for (int i = 0; i < length; ++i) {
        v = v * 0x100000001b3ULL + p[i].unicode();
    }
    v ^= v >> 29;
    v *= 0xff51afd7ed558ccdULL;
    v ^= v >> 32;
    return (static_cast<quint64>(length) << 32) | static_cast<quint32>(v);
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

quint32 TTriggerPrescan::gramBit(const quint64 gram)
{
    // The length has to reach the bit too, or a short gram and a long one that
    // hashed alike would share it and each keep the other's bucket warm.
    const quint32 folded = static_cast<quint32>(gram ^ (gram >> 32));
    return (folded ^ (folded >> 16)) & scmGramBitsMask;
}

quint64 TTriggerPrescan::patternGram(const QString& pattern)
{
    if (pattern.size() < scmMinGramLength) {
        return 0;
    }
    const int length = std::min(static_cast<int>(pattern.size()), scmMaxGramLength);
    const QChar* const data = pattern.constData();
    double bestScore = std::numeric_limits<double>::max();
    int bestPosition = 0;
    for (int i = 0; i + length <= pattern.size(); ++i) {
        double score = 0.0;
        for (int k = 0; k < length; ++k) {
            score += characterScore(data[i + k]);
        }
        if (score < bestScore) {
            bestScore = score;
            bestPosition = i;
        }
    }
    return gramHash(data + bestPosition, length);
}

void TTriggerPrescan::fileSlot(const int position, const std::vector<quint64>& grams)
{
    GramRange range;
    range.mCount = static_cast<int>(grams.size());
    if (grams.empty()) {
        const auto at = std::lower_bound(mUnfiltered.begin(), mUnfiltered.end(), position);
        mUnfiltered.insert(at, position);
    } else {
        range.mOffset = static_cast<int>(mGramPool.size());
        mGramPool.insert(mGramPool.end(), grams.cbegin(), grams.cend());
        for (const quint64 gram : grams) {
            ++mLengthUse[gramLength(gram)];
            std::vector<int>& bucket = mIndex[gram];
            const auto at = std::lower_bound(bucket.begin(), bucket.end(), position);
            bucket.insert(at, position);
            const quint32 bit = gramBit(gram);
            mGramBits[bit >> 6] |= (1ULL << (bit & 63));
        }
        ++mIndexedSlots;
    }
    mSlotGrams[position] = range;
}

void TTriggerPrescan::unfileSlot(const int position)
{
    const GramRange range = mSlotGrams[position];
    if (range.mCount == scmRemovedSlot) {
        return;
    }
    if (range.mCount == 0) {
        const auto at = std::lower_bound(mUnfiltered.begin(), mUnfiltered.end(), position);
        if (at != mUnfiltered.end() && *at == position) {
            mUnfiltered.erase(at);
        }
        return;
    }
    for (int i = 0; i < range.mCount; ++i) {
        const quint64 gram = mGramPool[range.mOffset + i];
        --mLengthUse[gramLength(gram)];
        const auto bucket = mIndex.find(gram);
        if (bucket == mIndex.end()) {
            continue;
        }
        std::vector<int>& positions = bucket->second;
        const auto at = std::lower_bound(positions.begin(), positions.end(), position);
        if (at != positions.end() && *at == position) {
            positions.erase(at);
        }
    }
    --mIndexedSlots;
    // The gram's bit stays set: bits are shared between grams, so clearing one
    // could hide another gram that is still filed. A bit left set only costs a
    // map lookup that finds an empty bucket, and the next rebuild clears it.
}

void TTriggerPrescan::rebuild(const std::vector<TTrigger*>& roots)
{
    mIndex.clear();
    mUnfiltered.clear();
    mGramPool.clear();
    mSlotGrams.assign(roots.size(), GramRange{});
    mGramBits.assign((1u << scmGramBitsLog) / 64, 0);
    mLengthUse.fill(0);
    mSeen.assign(roots.size(), 0);
    mGeneration = 0;
    mIndexedSlots = 0;
    mLiveSlots = static_cast<int>(roots.size());
    mMutations = 0;

    const int rootCount = static_cast<int>(roots.size());
    for (int position = 0; position < rootCount; ++position) {
        fileSlot(position, roots[position]->prescanGrams());
    }
    mActive = mIndexedSlots >= scmMinimumIndexedTriggers;
}

void TTriggerPrescan::appendSlot(const std::vector<quint64>& grams)
{
    mSlotGrams.emplace_back();
    mSeen.push_back(0);
    fileSlot(static_cast<int>(mSlotGrams.size()) - 1, grams);
    ++mLiveSlots;
    ++mMutations;
    mActive = mIndexedSlots >= scmMinimumIndexedTriggers;
}

void TTriggerPrescan::removeSlot(const int position)
{
    unfileSlot(position);
    mSlotGrams[position].mCount = scmRemovedSlot;
    --mLiveSlots;
    ++mMutations;
    mActive = mIndexedSlots >= scmMinimumIndexedTriggers;
}

void TTriggerPrescan::refileSlot(const int position, const std::vector<quint64>& grams)
{
    unfileSlot(position);
    fileSlot(position, grams);
    ++mMutations;
    mActive = mIndexedSlots >= scmMinimumIndexedTriggers;
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
    for (int length = scmMinGramLength; length <= scmMaxGramLength; ++length) {
        if (!mLengthUse[length]) {
            continue;
        }
        const int last = line.size() - length;
        for (int i = 0; i <= last; ++i) {
            const quint64 gram = gramHash(data + i, length);
            const quint32 bit = gramBit(gram);
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
    }

    std::sort(scratch.begin(), scratch.end());
    out.resize(scratch.size() + mUnfiltered.size());
    std::merge(mUnfiltered.cbegin(), mUnfiltered.cend(), scratch.cbegin(), scratch.cend(), out.begin());
}
