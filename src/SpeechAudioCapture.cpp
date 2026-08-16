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

#include "SpeechAudioCapture.h"

#include <QAudioDevice>
#include <QCoreApplication>
#include <QDebug>
#include <QIODevice>
#include <QMediaDevices>

// Human-readable description of a QAudio::Error, for messages a player sees.
// Each of these is substituted into "Audio input error occurred: %1", so they
// read as the tail of that sentence rather than as standalone messages.
static QString audioErrorText(const QAudio::Error error)
{
    switch (error) {
    case QAudio::NoError:
        //: Completes "Audio input error occurred: %1" - no fault was reported
        return QCoreApplication::translate("SpeechAudioCapture", "no error");
    case QAudio::OpenError:
        //: Completes "Audio input error occurred: %1" - the microphone could not be opened
        return QCoreApplication::translate("SpeechAudioCapture", "the microphone could not be opened");
    case QAudio::IOError:
        //: Completes "Audio input error occurred: %1" - reading from the microphone failed
        return QCoreApplication::translate("SpeechAudioCapture", "reading from the microphone failed");
    case QAudio::UnderrunError:
        //: Completes "Audio input error occurred: %1" - the microphone went silent mid-capture
        return QCoreApplication::translate("SpeechAudioCapture", "the microphone stopped supplying audio");
    case QAudio::FatalError:
        //: Completes "Audio input error occurred: %1" - the microphone can no longer be used
        return QCoreApplication::translate("SpeechAudioCapture", "the microphone became unusable");
    }
    //: Completes "Audio input error occurred: %1" - the fault could not be identified
    return QCoreApplication::translate("SpeechAudioCapture", "unknown error");
}

SpeechAudioCapture::SpeechAudioCapture(QObject* parent)
: QObject(parent)
{
    mTargetFormat.setSampleRate(scmSampleRate);
    mTargetFormat.setChannelCount(1);
    mTargetFormat.setSampleFormat(QAudioFormat::Int16);

    connect(&mPollTimer, &QTimer::timeout, this, &SpeechAudioCapture::slot_poll);
}

SpeechAudioCapture::~SpeechAudioCapture()
{
    stop();
}

bool SpeechAudioCapture::start()
{
    const QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull()) {
        //: Shown when speech recognition finds no audio input device at all
        emit captureError(tr("No microphone available"));
        return false;
    }

    // Prefer the device's own format for reliable capture (macOS in
    // particular); conversion to the target format happens per read...
    QAudioFormat formatToUse = inputDevice.preferredFormat();

    // ...but only Float and Int16 samples can be converted; anything else
    // would capture happily and then be discarded frame by frame, leaving a
    // recognizer listening and permanently deaf. Ask for Int16 at the
    // device's own rate and channel count, and failing that the target
    // format itself.
    if (formatToUse.sampleFormat() != QAudioFormat::Float && formatToUse.sampleFormat() != QAudioFormat::Int16) {
        QAudioFormat fallbackFormat = formatToUse;
        fallbackFormat.setSampleFormat(QAudioFormat::Int16);
        if (!inputDevice.isFormatSupported(fallbackFormat)) {
            fallbackFormat = mTargetFormat;
        }
        if (!inputDevice.isFormatSupported(fallbackFormat)) {
            //: Shown when the microphone offers no audio format speech recognition can convert
            emit captureError(tr("The microphone does not offer an audio format speech recognition can use"));
            return false;
        }
        formatToUse = fallbackFormat;
    }

    mActualFormat = formatToUse;

    // A source left behind by an attempt that failed after allocating one
    // would otherwise linger, holding the capture device open
    if (mAudioSource) {
        mAudioSource->stop();
        delete mAudioSource;
        mAudioSource = nullptr;
    }

    mAudioSource = new QAudioSource(inputDevice, formatToUse, this);
    connect(mAudioSource.data(), &QAudioSource::stateChanged, this, &SpeechAudioCapture::slot_audioStateChanged);

    mAudioSource->setVolume(1.0);
    // A larger buffer makes capture more reliable across poll intervals
    mAudioSource->setBufferSize(32000);

    // Pull mode - start() returns a QIODevice to read from
    mAudioDevice = mAudioSource->start();
    if (!mAudioDevice) {
        qWarning() << "SpeechAudioCapture: Failed to start audio source - start() returned null";
        // Released now rather than left for the next attempt to trip over: it
        // may still hold the capture device open
        mAudioSource->stop();
        mAudioSource->deleteLater();
        mAudioSource = nullptr;
        //: Shown when the microphone could not be opened for speech recognition
        emit captureError(tr("Failed to start audio capture"));
        return false;
    }

    mCarryBuffer.clear();
    mResamplePhase = 0.0;
    mPollTimer.start(50);
    return true;
}

void SpeechAudioCapture::stop()
{
    teardown();
}

void SpeechAudioCapture::teardown()
{
    mPollTimer.stop();
    if (mAudioSource) {
        // The member is cleared before stop() is called, so a stateChanged
        // emitted from within stop() re-enters to a null source and returns
        // rather than reporting a fault for a teardown we initiated
        QAudioSource* pDyingSource = mAudioSource;
        mAudioSource = nullptr;
        pDyingSource->stop();
        pDyingSource->deleteLater();
    }
    mAudioDevice = nullptr;
    mCarryBuffer.clear();
    mResamplePhase = 0.0;
}

void SpeechAudioCapture::slot_poll()
{
    if (!mAudioDevice || !mAudioSource) {
        return;
    }

    const qint64 bytesAvailable = mAudioDevice->bytesAvailable();
    if (bytesAvailable <= 0) {
        return;
    }

    const QByteArray audioData = mAudioDevice->read(bytesAvailable);
    if (audioData.isEmpty()) {
        return;
    }

    convertAndEmit(audioData);
}

void SpeechAudioCapture::convertAndEmit(const QByteArray& audioData)
{
    const int srcRate = mActualFormat.sampleRate();
    const int srcChannels = mActualFormat.channelCount();
    const int dstRate = mTargetFormat.sampleRate();
    const QAudioFormat::SampleFormat srcFormat = mActualFormat.sampleFormat();

    // start() settles on a format this can convert, so reaching either of
    // these means the device gave QAudioSource something else. Report once
    // and stop, rather than warning 20 times a second while a recognizer
    // appears to listen.
    if (srcFormat != QAudioFormat::Float && srcFormat != QAudioFormat::Int16) {
        qWarning() << "SpeechAudioCapture: Unsupported audio format:" << srcFormat;
        teardown();
        //: %1 is a technical audio sample format name, e.g. "Int32"
        emit captureError(tr("The microphone is supplying audio in a format speech recognition cannot read (%1).").arg(QString::number(static_cast<int>(srcFormat))));
        return;
    }

    if (srcRate <= 0 || srcChannels <= 0) {
        qWarning() << "SpeechAudioCapture: capture format has no usable sample rate or channel count:" << mActualFormat;
        teardown();
        //: Shown when the microphone reports an audio format speech recognition cannot work with
        emit captureError(tr("The microphone reported an unusable audio format - try selecting a different input device."));
        return;
    }

    const int bytesPerSample = (srcFormat == QAudioFormat::Float) ? static_cast<int>(sizeof(float)) : static_cast<int>(sizeof(qint16));
    const int frameBytes = bytesPerSample * srcChannels;

    // Carry over what the previous call could not consume, so a frame split
    // across two reads is resampled rather than discarded
    mCarryBuffer.append(audioData);
    const int availableFrames = mCarryBuffer.size() / frameBytes;

    // Interpolation reads the frame after the one it sits on, so the last
    // frame has to wait for the next call to supply its partner
    if (availableFrames < 2) {
        return;
    }

    const double ratio = static_cast<double>(srcRate) / dstRate;
    const char* const bufferData = mCarryBuffer.constData();

    // First channel only - recognizers want mono
    const auto frameValue = [bufferData, srcChannels, srcFormat](const int frame) -> double {
        if (srcFormat == QAudioFormat::Float) {
            return static_cast<double>(reinterpret_cast<const float*>(bufferData)[frame * srcChannels]) * 32767.0;
        }
        return static_cast<double>(reinterpret_cast<const qint16*>(bufferData)[frame * srcChannels]);
    };

    QByteArray convertedData;
    convertedData.reserve(static_cast<int>(availableFrames / ratio + 1) * static_cast<int>(sizeof(qint16)));

    double phase = mResamplePhase;
    while (phase + 1.0 < availableFrames) {
        const int frame = static_cast<int>(phase);
        const double frac = phase - frame;
        const double sample = frameValue(frame) + frac * (frameValue(frame + 1) - frameValue(frame));
        // Clamped before rounding: qRound() returns int, so an out-of-range
        // float sample would overflow the conversion before a later clamp
        // could help
        const qint16 converted = static_cast<qint16>(qRound(qBound(-32768.0, sample, 32767.0)));
        convertedData.append(reinterpret_cast<const char*>(&converted), sizeof(qint16));
        phase += ratio;
    }

    // Drop only the frames no longer needed: the frame the phase now sits on
    // is the next interpolation's left-hand sample, and any partial frame's
    // bytes stay put for the read that completes them.
    // The phase can end up past the last whole frame when downsampling, and
    // removing more bytes than the buffer holds would take the partial frame
    // with them - which desynchronises every read that follows, since what is
    // left no longer starts on a frame boundary. Any phase beyond the buffer
    // is carried instead, and the loop condition consumes it against the next
    // read.
    const int consumedFrames = qMin(static_cast<int>(phase), availableFrames);
    if (consumedFrames > 0) {
        mCarryBuffer.remove(0, consumedFrames * frameBytes);
    }
    mResamplePhase = phase - consumedFrames;

    if (convertedData.isEmpty()) {
        return;
    }

    emit pcm(convertedData);
}

void SpeechAudioCapture::slot_audioStateChanged(QAudio::State newState)
{
    // IdleState is included: a device that stops supplying audio reports the
    // fault there as readily as it does on StoppedState, and leaving it
    // unhandled means polling a dead microphone while a recognizer still
    // appears to listen
    if ((newState == QAudio::StoppedState || newState == QAudio::IdleState) && mAudioSource) {
        const QAudio::Error audioError = mAudioSource->error();
        if (audioError != QAudio::NoError) {
            qWarning() << "SpeechAudioCapture: Audio error:" << audioError;
            teardown();
            //: %1 is a description of what went wrong with the microphone
            emit captureError(tr("Audio input error occurred: %1").arg(audioErrorText(audioError)));
        }
    }
}
