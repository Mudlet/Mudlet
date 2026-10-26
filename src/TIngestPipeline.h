#ifndef MUDLET_TINGESTPIPELINE_H
#define MUDLET_TINGESTPIPELINE_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TBuffer.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

class Host;
class QThread;

// A line the worker parsed ahead, as commitLine() would have been handed it; its text and, unless the
// whole line is in one format, its formats are in the job's arenas
struct TSpeculatedLine
{
    TBuffer::ParseState stateAtCommit;
    TChar uniformFormat;
    // Offsets into the unit: where the line's first byte is, and its commit character
    size_t start = 0;
    size_t end = 0;
    quint32 textOffset = 0;
    quint32 textLength = 0;
    quint32 formatOffset = 0;
    quint32 formatLength = 0;
    char commitChar = 0;
    bool forcedLineBreak = false;
    bool uniform = true;
};

// Up to csmMaxLines lines of one game unit, parsed ahead from mStartPosition. Main fills it in while
// it is neither Queued nor Running; the worker publishes lines through mReady, and main takes them in
// order. The job never grows: lines go into fixed arenas, and when those are full the worker stops
// and main queues the job again from where it has got to.
struct TIngestJob
{
    enum State : int { Idle, Queued, Running, Done };
    static constexpr size_t csmMaxLines = 512;
    static constexpr size_t csmMaxTextChars = 32768;
    static constexpr size_t csmMaxFormatChars = 8192;
    static constexpr size_t csmNoResume = std::numeric_limits<size_t>::max();

    TIngestJob();

    // A copy, so main never waits for the worker to let go of its own
    std::string mUnit;
    size_t mDecodableLength = 0;
    size_t mStartPosition = 0;
    QVector<QChar> mLookupTable;
    TBuffer::ParseState mStartState;
    TBuffer::ParseConfig mConfig;
    std::array<QColor, 16> mColours;
    std::vector<TSpeculatedLine> mLines;
    std::vector<QChar> mText;
    std::vector<TChar> mFormats;
    // Worker only
    size_t mTextUsed = 0;
    size_t mFormatUsed = 0;
    // Where main may queue the job again for the rest of the unit, once the worker is Done
    size_t mResumeFrom = csmNoResume;

    alignas(64) std::atomic<int> mReady{0};
    std::atomic<bool> mCancel{false};
    std::atomic<int> mState{Idle};
    // Main's own: the next line it has not used or passed
    alignas(64) int mNext = 0;
};

// Parses a game unit on a worker thread, lines ahead of the main thread, which commits them instead of
// decoding them. The worker stops before a sequence whose handling has an effect; main parses that line
// and may hand it the rest back.
class TIngestPipeline
{
public:
    static constexpr int csmMinimumLines = 16;
    static constexpr size_t csmMinimumResumeBytes = 512;

    explicit TIngestPipeline(Host*);
    ~TIngestPipeline();
    TIngestPipeline(const TIngestPipeline&) = delete;
    TIngestPipeline& operator=(const TIngestPipeline&) = delete;

    static bool enabled();
    // MUDLET_INGEST_THREAD=verify: main parses every line as well, and warns where the worker's differs
    static bool verifying();
    TIngestJob* begin(const TBuffer& source, const std::string& unit, size_t decodableLength, const QVector<QChar>& lookupTable);
    // Queues the job again from position, if the worker has finished with it and stopped short of the unit's end
    bool resume(const TBuffer& source, TIngestJob&, size_t position);
    // Under MUDLET_TEST_MODE main waits for the worker rather than parse a line itself, so that a spec's
    // burst is parsed ahead however the threads are scheduled. False once the job has no more lines.
    bool awaitLines(const TIngestJob&, int ready) const;
    // The worker is left to notice a cancelled job and finish on its own
    void end(TIngestJob&);
    void noteReplayed() { ++mReplayedLines; }
    quint64 replayedLines() const { return mReplayedLines; }

private:
    void queue(const TBuffer& source, TIngestJob&, size_t position);
    void workerLoop();
    void parse(TIngestJob&);

    std::unique_ptr<TBuffer> mpTwin;
    std::unique_ptr<QThread> mpThread;
    TIngestJob mJob;
    std::atomic<uint32_t> mEpoch{0};
    std::atomic<bool> mStop{false};
    const bool mWaitForWorker;
    quint64 mReplayedLines = 0;
};

#endif // MUDLET_TINGESTPIPELINE_H
