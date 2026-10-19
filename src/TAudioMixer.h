#ifndef MUDLET_TAUDIOMIXER_H
#define MUDLET_TAUDIOMIXER_H

/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QAudioFormat>
#include <QMutex>
#include <QObject>
#include <QThread>

#include <atomic>
#include <chrono>
#include <map>
#include <vector>

// The one audio output every profile's sounds and music play through. An output opened and
// closed for each sound makes macOS claim Bluetooth headphones for Mudlet every time, and a
// burst of them breaks up other apps' audio and can leave Mudlet itself silent. This stays
// open while anything plays and for a while after, mixing every voice into it.
//
// The output lives on a thread of its own and is fed straight from Qt's decoding threads, so a
// busy main thread neither cuts the sound out nor leaves it running late. Everything public is
// safe to call from any thread.
class TAudioMixer : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TAudioMixer)
    static TAudioMixer* self();
    // 48 kHz stereo float: what every voice is rendered in
    static QAudioFormat format();

    int addVoice();
    void removeVoice(int voice);
    // Interleaved stereo frames, appended to what the voice has queued
    void queue(int voice, const float* frames, qint64 frameCount);
    // Drops what the voice has queued and not yet played, for a stop
    void clear(int voice);
    // A paused voice keeps what it holds, unplayed, until it is released
    void hold(int voice, bool held);
    // The voice's track has reached its end: what it holds, and what still arrives for it, plays out
    // even short of the head start, until the next track starts
    void drain(int voice);
    void undrain(int voice);
    // The default output device changed
    void refreshDevice();

    // For the media tests
    bool outputOpen() const { return mOutputOpen; }
    int timesOpened() const { return mTimesOpened; }
    // How often a voice still playing ran out of audio, which is heard as a dropout
    int timesRanDry() const { return mTimesRanDry; }
    qint64 queuedSamples(int voice = 0);
    void setIdleCloseDelay(std::chrono::milliseconds delay);
    // Keeps the output shut, so a test can run the mix itself without an audio device
    void setOutputSuspended(bool suspended);
    void mixForTests(float* out, qint64 sampleCount) { mix(out, sampleCount); }

private:
    class Output;
    friend class Output;

    struct Voice
    {
        std::vector<float> samples;
        size_t readPosition = 0;
        // Joined the mix: a voice waits until it holds a head start, or its track has ended
        bool started = false;
        bool draining = false;
        bool held = false;

        size_t queuedSamples() const { return samples.size() - readPosition; }
    };

    TAudioMixer();
    ~TAudioMixer() override;
    // Runs on the output's thread, whenever the output wants more
    void mix(float* out, qint64 sampleCount);
    bool silent();
    void dropEverything();
    void requestOpen();

    QMutex mMutex;
    std::map<int, Voice> mVoices;
    int mNextVoice = 1;
    QThread mThread;
    Output* mpOutput = nullptr;
    std::atomic<bool> mOutputOpen = false;
    std::atomic<bool> mOpenRequested = false;
    std::atomic<bool> mOutputSuspended = false;
    std::atomic<int> mTimesOpened = 0;
    std::atomic<int> mTimesRanDry = 0;
};

#endif // MUDLET_TAUDIOMIXER_H
