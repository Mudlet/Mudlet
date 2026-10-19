#ifndef MUDLET_TMEDIAVOICE_H
#define MUDLET_TMEDIAVOICE_H

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

#include <QMutex>
#include <QObject>
#include <QPointer>

#include <atomic>
#include <optional>

class QAudioBuffer;
class QAudioBufferOutput;
class QMediaPlayer;
class QTimer;

// Takes a QMediaPlayer's decoded sound into the shared output, TAudioMixer, in place of a
// QAudioOutput of its own. The player still decodes, seeks, loops and reports its state.
//
// Audio arrives on the player's decoding thread and is rendered and queued there, so the main
// thread being busy never holds it up; everything else happens on the main thread.
class TMediaVoice : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TMediaVoice)

    // A level that moves to a new value over a number of frames, since a jump clicks
    struct Glide
    {
        float current = 0.0F;
        float target = 0.0F;
        qint64 framesLeft = 0;

        void glideTo(float value, qint64 frames);
        float step();
    };

    explicit TMediaVoice(QMediaPlayer* player);
    ~TMediaVoice() override;

    // Linear, 0 to 1, as QAudioOutput's volume is
    void setGain(float gain, int glideMilliseconds);
    float gain() const { return mRequestedGain; }
    // -1 is fully left, 1 fully right. A placed sound is mixed down to one point and set there; an
    // unplaced one keeps its own stereo image.
    void setPan(float pan, int glideMilliseconds);
    float pan() const { return mRequestedPan; }
    bool placed() const { return mPlaced; }
    void unplace(int glideMilliseconds);
    // A new track, not a loop's next pass: drops whatever the last one still had waiting
    void beginTrack();
    void setMuted(bool muted);
    bool muted() const { return mMuted; }
    bool receivedAudio() const { return mReceivedAudio; }
    int mixerVoice() const { return mVoice; }

    // How many frames a change asked to take this long glides over, once a sound is playing: even one
    // asked for at once takes a moment, as a jump clicks
    static qint64 glideFramesFor(int milliseconds);
    // Interleaved stereo float in and out, which may be the same buffer. Placement blends the source's
    // own stereo image, at 0, into one point set at the pan with a constant-power pan, at 1.
    static void render(const float* in, float* out, qint64 frames, Glide& gain, Glide& pan, Glide& placement);

    // Whether the backend cannot feed the shared output. QAudioBufferOutput only delivers audio on Qt's
    // FFmpeg backend: so when another is named, and while one nobody named has not yet been seen to.
    // Asking starts that check, so Qt Multimedia is only loaded once something is played.
    static bool unsupported();
    // For the media tests
    static void probeBackend();
    static bool probeStarted() { return smProbeStarted; }
    static bool probePassed() { return smProbePassed; }

signals:
    // The player has played for a while without handing over any audio. Whoever owns this voice
    // should give the player a QAudioOutput back, so the sound is still heard.
    void signal_unsupported();

private:
    // A target set from the main thread, which the decoding thread picks up with the next audio
    struct Request
    {
        float target = 0.0F;
        qint64 glideFrames = 0;
        // Where a glide asked for before any audio sets out from
        std::optional<float> start;
        quint64 serial = 0;
        quint64 applied = 0;

        // Glides when heard. Otherwise it jumps, unless it follows a change still waiting to be made, as
        // a Play and an Update sent together ask: it then glides on from there.
        void set(float value, int glideMilliseconds, bool heard);
        void jumpTo(float value);
        std::optional<float> waitingStart() const;
        void applyTo(Glide& glide);
    };

    bool heardFor(int glideMilliseconds) const;
    void audioBufferReceived(const QAudioBuffer& buffer);
    void checkForSilence();
    static void probeUnnamedBackend();

    QPointer<QMediaPlayer> mpPlayer;
    QAudioBufferOutput* mpBufferOutput = nullptr;
    const int mVoice;
    // Rendering state, touched only on the decoding thread
    Glide mGain{1.0F, 1.0F, 0};
    Glide mPan;
    Glide mPlacement;
    QMutex mRequestMutex;
    Request mGainRequest{1.0F};
    Request mPanRequest;
    Request mPlacementRequest;
    // Under mRequestMutex. A loop passes through a stop between passes, so this outlasts mReceivedAudio.
    bool mTrackHeard = false;
    // Under mRequestMutex: the audio last handed over was placed at all, if only while blending out
    bool mPanHeard = false;
    std::atomic<float> mRequestedGain = 1.0F;
    std::atomic<float> mRequestedPan = 0.0F;
    std::atomic<bool> mPlaced = false;
    std::atomic<bool> mMuted = false;
    std::atomic<bool> mReceivedAudio = false;
    // Stopped on request rather than at the end, so audio still arriving is stale
    std::atomic<bool> mStopped = false;
    // Held by a decoding thread while it hands audio over, so the destructor can wait out one in progress
    QMutex mDeliveryMutex;
    std::atomic<bool> mShuttingDown = false;
    std::atomic<bool> mFormatRefusalReported = false;
    // The position has reached the duration: a stop that follows is the track ending, not a request
    bool mReachedEnd = false;
    QTimer* mpSilenceTimer = nullptr;

    static inline std::atomic<bool> smProbePassed = false;
    static inline std::atomic<bool> smProbeStarted = false;
};

#endif // MUDLET_TMEDIAVOICE_H
