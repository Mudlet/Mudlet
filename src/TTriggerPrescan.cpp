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

#include <algorithm>
#include <bit>
#include <limits>

// Length in the high half, so the same characters at two lengths are different grams and 0 stays
// free for "no gram" (a real one's length is at least scmMinGramLength).
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

// English letter frequencies, space commonest. Need not be accurate, only make different patterns
// prefer different n-grams.
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
    // Fold the length in too, or a short and a long gram that hashed alike would share a bit.
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

int TTriggerPrescan::fileSlot(const std::vector<quint64>& grams)
{
    int slot;
    if (mFreeSlots.empty()) {
        slot = static_cast<int>(mSlotGrams.size());
        mSlotGrams.emplace_back();
        mSeen.push_back(0);
    } else {
        slot = mFreeSlots.back();
        mFreeSlots.pop_back();
    }
    fileGrams(slot, grams);
    return slot;
}

void TTriggerPrescan::removeSlot(const int slot)
{
    unfileGrams(slot);
    mFreeSlots.push_back(slot);
    compactIfWasteful();
}

void TTriggerPrescan::removeSlots(const std::vector<int>& freed)
{
    const bool reindexing = freed.size() * 2 >= mSlotGrams.size() - mFreeSlots.size();
    for (const int slot : freed) {
        if (reindexing) {
            GramRange& range = mSlotGrams[slot];
            mFiledGrams -= range.mCount;
            range = GramRange{};
        } else {
            unfileGrams(slot);
        }
        mFreeSlots.push_back(slot);
    }
    if (reindexing) {
        reindex();
    } else {
        compactIfWasteful();
    }
}

void TTriggerPrescan::refileSlot(const int slot, const std::vector<quint64>& grams)
{
    const GramRange range = mSlotGrams[slot];
    if (range.mCount == static_cast<int>(grams.size()) && std::equal(grams.cbegin(), grams.cend(), mGramPool.cbegin() + range.mOffset)) {
        return;
    }
    unfileGrams(slot);
    fileGrams(slot, grams);
    compactIfWasteful();
}

void TTriggerPrescan::fileGrams(const int slot, const std::vector<quint64>& grams)
{
    if (mGramBits.empty()) {
        mGramBits.assign((1u << scmGramBitsLog) / 64, 0);
    }
    GramRange& range = mSlotGrams[slot];
    range.mOffset = static_cast<int>(mGramPool.size());
    range.mCount = static_cast<int>(grams.size());
    mGramPool.insert(mGramPool.end(), grams.cbegin(), grams.cend());
    mFiledGrams += range.mCount;
    for (const quint64 gram : grams) {
        ++mLengthUse[gramLength(gram)];
        std::vector<int>& bucket = mIndex[gram];
        bucket.insert(std::upper_bound(bucket.begin(), bucket.end(), slot), slot);
        const quint32 bit = gramBit(gram);
        mGramBits[bit >> 6] |= (1ULL << (bit & 63));
    }
}

void TTriggerPrescan::unfileGrams(const int slot)
{
    GramRange& range = mSlotGrams[slot];
    for (int i = 0; i < range.mCount; ++i) {
        const quint64 gram = mGramPool[range.mOffset + i];
        --mLengthUse[gramLength(gram)];
        const auto bucket = mIndex.find(gram);
        if (bucket == mIndex.end()) {
            continue;
        }
        std::vector<int>& filed = bucket->second;
        const auto at = std::lower_bound(filed.begin(), filed.end(), slot);
        if (at != filed.end() && *at == slot) {
            filed.erase(at);
        }
    }
    mFiledGrams -= range.mCount;
    range = GramRange{};
}

void TTriggerPrescan::compactIfWasteful()
{
    if (static_cast<int>(mGramPool.size()) - mFiledGrams > std::max(mFiledGrams, scmMinimumWaste)) {
        reindex();
    }
}

void TTriggerPrescan::reindex()
{
    ++mReindexes;
    for (auto& entry : mIndex) {
        entry.second.clear();
    }
    mGramBits.assign((1u << scmGramBitsLog) / 64, 0);
    mLengthUse.fill(0);
    std::vector<quint64> pool;
    pool.reserve(mFiledGrams);
    const int slotCount = static_cast<int>(mSlotGrams.size());
    for (int slot = 0; slot < slotCount; ++slot) {
        GramRange& range = mSlotGrams[slot];
        const int offset = static_cast<int>(pool.size());
        for (int i = 0; i < range.mCount; ++i) {
            const quint64 gram = mGramPool[range.mOffset + i];
            pool.push_back(gram);
            ++mLengthUse[gramLength(gram)];
            mIndex[gram].push_back(slot);
            const quint32 bit = gramBit(gram);
            mGramBits[bit >> 6] |= (1ULL << (bit & 63));
        }
        range.mOffset = offset;
    }
    mGramPool = std::move(pool);
    std::erase_if(mIndex, [](const auto& entry) {
        return entry.second.empty();
    });
}

void TTriggerPrescan::hits(const QString& line, std::vector<int>& out) const
{
    out.clear();
    if (!mFiledGrams) {
        return;
    }
    if (++mGeneration == 0) {
        // Wrapped: old stamps would otherwise read as "this line"
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
            for (const int slot : bucket->second) {
                if (mSeen[slot] == mGeneration) {
                    continue;
                }
                mSeen[slot] = mGeneration;
                out.push_back(slot);
            }
        }
    }
}

int TChildWalk::nextToEnter(const int position, const quint32 lineStamp) const
{
    const int words = static_cast<int>(mAlways.size());
    int word = position / 64;
    if (word >= words) {
        return std::numeric_limits<int>::max();
    }
    const bool marked = mMarkedStamp == lineStamp;
    quint64 bits = (mAlways[word] | (marked ? mMarked[word] : 0)) & (~quint64(0) << (position % 64));
    while (!bits) {
        if (++word >= words) {
            return std::numeric_limits<int>::max();
        }
        bits = mAlways[word] | (marked ? mMarked[word] : 0);
    }
    return word * 64 + std::countr_zero(bits);
}

bool TChildWalk::mark(const int position, const quint32 lineStamp)
{
    if (mMarkedStamp != lineStamp) {
        std::fill(mMarked.begin(), mMarked.end(), 0);
        mMarkedStamp = lineStamp;
    }
    quint64& word = mMarked[position / 64];
    const quint64 bit = quint64(1) << (position % 64);
    if (word & bit) {
        return false;
    }
    word |= bit;
    return true;
}

void TChildWalk::setAlways(const int position, const bool always)
{
    const size_t words = static_cast<size_t>(position) / 64 + 1;
    if (mAlways.size() < words) {
        mAlways.resize(words, 0);
        mMarked.resize(words, 0);
    }
    const quint64 bit = quint64(1) << (position % 64);
    if (always) {
        mAlways[position / 64] |= bit;
    } else {
        mAlways[position / 64] &= ~bit;
    }
}
