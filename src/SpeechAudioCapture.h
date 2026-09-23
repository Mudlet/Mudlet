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

#ifndef MUDLET_SPEECHAUDIOCAPTURE_H
#define MUDLET_SPEECHAUDIOCAPTURE_H

#include <QAudioFormat>
#include <QAudioSource>
#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>

class QIODevice;

// Captures microphone audio and delivers it as 16kHz mono signed 16-bit PCM,
// the format speech recognition models consume. Owns the device negotiation,
// the resampling (with fractional phase and partial frames carried across
// reads), and the audio-fault handling, so recognizer backends only ever see
// ready-to-decode PCM and never touch Qt's audio classes themselves.

class SpeechAudioCapture : public QObject
{
    Q_OBJECT

public:
    explicit SpeechAudioCapture(QObject* parent = nullptr);
    ~SpeechAudioCapture() override;

    // Open the default input device and begin delivering PCM via pcm().
    // Returns false after emitting captureError() when no usable device or
    // sample format exists. Safe to call again after stop().
    bool start();

    // Stop capture and release the device. Idempotent.
    void stop();

    bool active() const { return mAudioSource != nullptr; }

    // Stop delivering audio after this many milliseconds of continuous
    // silence, announced via silenceTimedOut(). 0 (the default) disables the
    // timeout entirely, preserving open-ended capture.
    // Enabling this mid-session restarts the clock rather than judging the
    // session so far: the silence measurement only runs while a timeout is
    // set, so a session that began without one has been accumulating silence
    // since it started and would otherwise time out on the very next chunk -
    // ending an utterance the moment the timeout was configured.
    void setSilenceTimeout(int msec);
    int silenceTimeout() const { return mSilenceTimeoutMsec; }

    // The delivery format: pcm() payloads are this rate, mono, Int16
    static constexpr int scmSampleRate = 16000;

signals:
    // Converted audio ready for a recognizer: 16kHz mono signed 16-bit
    void pcm(const QByteArray& data);

    // Capture became unusable. The device has already been torn down; the
    // message is translated and ready to show a player.
    void captureError(const QString& message);

    // The configured silence timeout elapsed with no speech. Capture is still
    // running when this fires, so the receiver decides what stopping means -
    // typically finalising the utterance rather than discarding it.
    void silenceTimedOut();

private slots:
    void slot_poll();
    void slot_audioStateChanged(QAudio::State newState);

private:
    void convertAndEmit(const QByteArray& audioData);
    void teardown();

    QPointer<QAudioSource> mAudioSource;
    QPointer<QIODevice> mAudioDevice;
    QAudioFormat mTargetFormat; // what pcm() delivers (16kHz mono Int16)
    QAudioFormat mActualFormat; // what the device actually captures
    QTimer mPollTimer;

    // Source audio that has not been resampled yet: whole frames still needed
    // as interpolation input, plus any trailing bytes of an incomplete frame
    QByteArray mCarryBuffer;
    // Position within mCarryBuffer, in source frames, that the next output
    // sample reads from. Carried across reads so resampling does not restart
    // its phase at every buffer boundary.
    double mResamplePhase = 0.0;

    // Silence timeout state: level smoothing matches the recognizer's own
    // silence detection, and the clock restarts on every voiced chunk
    int mSilenceTimeoutMsec = 0;
    float mSmoothedLevel = 0.0f;
    QElapsedTimer mSinceVoiced;
};

#endif // MUDLET_SPEECHAUDIOCAPTURE_H
