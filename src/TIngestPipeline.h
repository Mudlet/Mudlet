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
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Host;
class QThread;

// A line the worker parsed ahead, as commitLine() would have been handed it
struct TSpeculatedLine
{
    QString text;
    TLineFormat format;
    TBuffer::ParseState stateAtCommit;
    // Offsets into the unit: where the line's first byte is, and its commit character
    size_t start = 0;
    size_t end = 0;
    char commitChar = 0;
    bool forcedLineBreak = false;
};

// One game unit being parsed ahead. Main fills it in and owns it except between begin() and end(); the
// worker publishes lines through mReady, and main takes them in order.
struct TIngestJob
{
    enum State : int { Idle, Queued, Running, Done };

    const std::string* mpUnit = nullptr;
    size_t mDecodableLength = 0;
    QVector<QChar> mLookupTable;
    QByteArray mEncoding;
    TBuffer::ParseState mStartState;
    TBuffer::ParseConfig mConfig;
    std::array<QColor, 16> mColours;
    // Never resized while the worker holds the job
    std::vector<TSpeculatedLine> mLines;
    std::atomic<int> mReady{0};
    std::atomic<bool> mCancel{false};
    std::atomic<int> mState{Idle};
    // Main's own: the next line it has not used or passed
    int mNext = 0;
    std::chrono::steady_clock::time_point mQueuedAt;
    std::chrono::steady_clock::time_point mWorkerStart;
    std::chrono::steady_clock::time_point mWorkerEnd;
};

// Parses game text one unit ahead of the main thread with a second TBuffer, so the main thread commits
// lines instead of decoding them. Everything with an effect stays on the main thread, in order: the
// worker's buffer stops at the first sequence whose handling has one, and main parses the rest itself.
class TIngestPipeline
{
public:
    static constexpr int csmMinimumLines = 16;

    explicit TIngestPipeline(Host*);
    ~TIngestPipeline();
    TIngestPipeline(const TIngestPipeline&) = delete;
    TIngestPipeline& operator=(const TIngestPipeline&) = delete;

    static bool enabled();
    static bool verifying();
    void noteVerified(bool same, const QString& text);
    TIngestJob* begin(const TBuffer& source, const std::string& unit, size_t decodableLength, const QVector<QChar>& lookupTable);
    // Returns once the worker no longer reads the unit; harmless on a job already ended
    void end(TIngestJob&);

    // Prototype statistics, main thread only
    quint64 mStatJobs = 0;
    quint64 mStatLines = 0;
    quint64 mStatReplayedFast = 0;
    quint64 mStatReplayedSlow = 0;
    quint64 mStatMismatch = 0;
    quint64 mStatVerified = 0;
    quint64 mStatVerifyFailed = 0;
    quint64 mStatNotReady = 0;
    quint64 mStatNeverStarted = 0;
    double mStatLatencyUs = 0;
    double mStatWorkerUs = 0;
    double mStatMainUs = 0;

private:
    void workerLoop();
    void parse(TIngestJob&);
    void printStats() const;

    std::unique_ptr<TBuffer> mpTwin;
    std::unique_ptr<QThread> mpThread;
    TIngestJob mJob;
    std::atomic<uint32_t> mEpoch{0};
    std::atomic<bool> mStop{false};
    std::atomic<quint64> mStatStopped{0};
};

#endif // MUDLET_TINGESTPIPELINE_H
