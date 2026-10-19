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


#include "TMediaVoice.h"

#include "TAudioMixer.h"
#include "utils.h"

#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QMediaPlayer>
#include <QTemporaryFile>
#include <QTimer>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <vector>

using namespace std::chrono_literals;

namespace {
// FFmpeg delivers the first audio within milliseconds of playing media it has ready, so this long
// with none is a stall, and the sound is better heard through an output of its own
constexpr auto silentPlayLimit = 500ms;
constexpr auto probeLimit = 5s;
constexpr int minimumGlideMilliseconds = 20;

bool otherBackendNamed()
{
    static const bool named = !qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty() && qEnvironmentVariable("QT_MEDIA_BACKEND") != qsl("ffmpeg");
    return named;
}

// A fifth of a second of silence, for the probe to play
QByteArray probeWav()
{
    constexpr int sampleRate = 48000;
    constexpr int dataBytes = sampleRate / 5 * 2;
    QByteArray wav;
    QDataStream out(&wav, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << static_cast<quint32>(36 + dataBytes);
    out.writeRawData("WAVEfmt ", 8);
    out << static_cast<quint32>(16) << static_cast<quint16>(1) << static_cast<quint16>(1) << static_cast<quint32>(sampleRate) << static_cast<quint32>(sampleRate * 2) << static_cast<quint16>(2)
        << static_cast<quint16>(16);
    out.writeRawData("data", 4);
    out << static_cast<quint32>(dataBytes);
    wav.append(QByteArray(dataBytes, '\0'));
    return wav;
}
} // namespace

void TMediaVoice::Glide::glideTo(const float value, const qint64 frames)
{
    target = value;
    framesLeft = std::max<qint64>(frames, 0);
    if (framesLeft == 0) {
        current = value;
    }
}

float TMediaVoice::Glide::step()
{
    if (framesLeft > 0) {
        current += (target - current) / static_cast<float>(framesLeft);
        --framesLeft;
    }
    return current;
}

void TMediaVoice::Request::set(const float value, const int glideMilliseconds, const bool heard)
{
    const std::optional<float> from = heard ? std::nullopt : waitingStart();
    const bool glides = heard || (from && glideMilliseconds > 0);
    target = value;
    glideFrames = glides ? glideFramesFor(glideMilliseconds) : 0;
    start = glides ? from : std::nullopt;
    ++serial;
}

void TMediaVoice::Request::jumpTo(const float value)
{
    target = value;
    glideFrames = 0;
    start.reset();
    ++serial;
}

std::optional<float> TMediaVoice::Request::waitingStart() const
{
    if (applied == serial) {
        return std::nullopt;
    }
    if (start) {
        return start;
    }
    return glideFrames == 0 ? std::optional<float>(target) : std::nullopt;
}

void TMediaVoice::Request::applyTo(Glide& glide)
{
    if (applied != serial) {
        applied = serial;
        if (start) {
            glide.glideTo(*start, 0);
        }
        glide.glideTo(target, glideFrames);
    }
}

bool TMediaVoice::unsupported()
{
    if (otherBackendNamed()) {
        return true;
    }
    if (qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty() && !smProbePassed) {
        probeUnnamedBackend();
        return true;
    }
    return false;
}

void TMediaVoice::probeBackend()
{
    if (qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty()) {
        probeUnnamedBackend();
    }
}

// Plays the probe sound once, through a buffer output alone: if any audio comes out, the backend
// feeds the shared output and later players are mixed. Until then they keep outputs of their own.
void TMediaVoice::probeUnnamedBackend()
{
    if (smProbeStarted.exchange(true)) {
        return;
    }

    auto* player = new QMediaPlayer(QCoreApplication::instance());
    // A file, not a QBuffer: Qt before 6.12.1 never frees the 32 KB it reads a device through
    auto* source = new QTemporaryFile(QDir::temp().filePath(qsl("mudlet-media-probe-XXXXXX.wav")), player);
    if (!source->open() || source->write(probeWav()) == -1 || !source->flush()) {
        qWarning().noquote() << "TMediaVoice WARNING - could not write the sound that checks the media backend, so each sound plays through an output of its own.";
        player->deleteLater();
        return;
    }
    auto* output = new QAudioBufferOutput(TAudioMixer::format(), player);
    player->setAudioBufferOutput(output);
    connect(output, &QAudioBufferOutput::audioBufferReceived, player, [player](const QAudioBuffer& buffer) {
        if (buffer.isValid()) {
            smProbePassed = true;
            player->stop();
            player->deleteLater();
        }
    });
    QTimer::singleShot(probeLimit, player, [player]() {
        qWarning().noquote() << "TMediaVoice WARNING - this media backend hands over no audio to mix, so each sound plays through an output of its own.";
        player->deleteLater();
    });
    player->setSource(QUrl::fromLocalFile(source->fileName()));
    player->play();
}

TMediaVoice::TMediaVoice(QMediaPlayer* player)
: mpPlayer(player)
, mVoice(TAudioMixer::self()->addVoice())
{
    mpBufferOutput = new QAudioBufferOutput(TAudioMixer::format(), this);
    // Direct: rendered on the decoding thread, so a busy main thread cannot hold the sound up
    connect(mpBufferOutput, &QAudioBufferOutput::audioBufferReceived, this, &TMediaVoice::audioBufferReceived, Qt::DirectConnection);

    mpSilenceTimer = new QTimer(this);
    mpSilenceTimer->setSingleShot(true);
    mpSilenceTimer->setInterval(silentPlayLimit);
    connect(mpSilenceTimer, &QTimer::timeout, this, [this]() {
        if (!mReceivedAudio && mpPlayer && mpPlayer->playbackState() == QMediaPlayer::PlayingState) {
            emit signal_unsupported();
        }
    });

    if (!mpPlayer) {
        return;
    }

    mpPlayer->setAudioBufferOutput(mpBufferOutput);

    // The position reaches the duration just before the stop that ends a track, and is gone by the
    // time a handler of that stop may have handed the player its next track
    connect(mpPlayer, &QMediaPlayer::positionChanged, this, [this](const qint64 position) {
        if (mpPlayer && mpPlayer->duration() > 0 && position >= mpPlayer->duration()) {
            mReachedEnd = true;
        }
    });
    connect(mpPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](const QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            mReachedEnd = true;
            TAudioMixer::self()->drain(mVoice);
        }
    });
    connect(mpPlayer, &QMediaPlayer::playbackStateChanged, this, [this](const QMediaPlayer::PlaybackState state) {
        TAudioMixer* mixer = TAudioMixer::self();
        switch (state) {
        case QMediaPlayer::StoppedState:
            mReceivedAudio = false;
            mixer->hold(mVoice, false);
            // A track that ran to its end, or a loop pass, plays out the little still queued; a stop
            // is meant to silence it now
            if (mReachedEnd) {
                mixer->drain(mVoice);
            } else {
                mStopped = true;
                mixer->clear(mVoice);
            }
            break;
        case QMediaPlayer::PausedState:
            mixer->hold(mVoice, true);
            break;
        case QMediaPlayer::PlayingState:
            mStopped = false;
            mReachedEnd = false;
            mixer->hold(mVoice, false);
            mixer->undrain(mVoice);
            break;
        }
    });
    connect(mpPlayer, &QMediaPlayer::playbackStateChanged, this, &TMediaVoice::checkForSilence);
    connect(mpPlayer, &QMediaPlayer::mediaStatusChanged, this, &TMediaVoice::checkForSilence);
    checkForSilence();
}

TMediaVoice::~TMediaVoice()
{
    // No new delivery can start after the disconnect, and the lock waits out one already under way
    disconnect(mpBufferOutput, nullptr, this, nullptr);
    mShuttingDown = true;
    {
        const QMutexLocker delivery(&mDeliveryMutex);
    }
    if (mpPlayer && mpPlayer->audioBufferOutput() == mpBufferOutput) {
        mpPlayer->setAudioBufferOutput(nullptr);
    }
    TAudioMixer::self()->removeVoice(mVoice);
}

void TMediaVoice::checkForSilence()
{
    // Only media the backend has ready: a slow load or a stream still buffering is no sign of anything
    const QMediaPlayer::MediaStatus status = mpPlayer ? mpPlayer->mediaStatus() : QMediaPlayer::NoMedia;
    const bool ready = status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia;

    if (mpPlayer && mpPlayer->playbackState() == QMediaPlayer::PlayingState && ready && !mReceivedAudio) {
        if (!mpSilenceTimer->isActive()) {
            mpSilenceTimer->start();
        }
    } else {
        mpSilenceTimer->stop();
    }
}

void TMediaVoice::setGain(const float gain, const int glideMilliseconds)
{
    const float level = std::clamp(gain, 0.0F, 1.0F);
    const QMutexLocker lock(&mRequestMutex);
    // A fade sets the same level on every tick, which must not cut short a longer glide to it. A new
    // track, with no audio yet, starts at its own level, not part way into the last one's glide.
    if (level == mRequestedGain && mTrackHeard) {
        return;
    }
    mRequestedGain = level;
    mGainRequest.set(level, glideMilliseconds, heardFor(glideMilliseconds));
}

// Under mRequestMutex. Between a loop's passes only a change given time glides on from the last pass:
// one asked for at once, as a fade's first tick, is where the next pass starts.
bool TMediaVoice::heardFor(const int glideMilliseconds) const
{
    return mTrackHeard && (mReceivedAudio || glideMilliseconds > 0);
}

qint64 TMediaVoice::glideFramesFor(const int milliseconds)
{
    return TAudioMixer::format().framesForDuration(qint64{std::max(milliseconds, minimumGlideMilliseconds)} * 1000);
}

void TMediaVoice::setPan(const float pan, const int glideMilliseconds)
{
    const float position = std::clamp(pan, -1.0F, 1.0F);
    const QMutexLocker lock(&mRequestMutex);
    const bool heard = heardFor(glideMilliseconds);
    // A sound moves on from its last pan where that is heard: placed or blending out as it plays, or,
    // before any audio, where a pan still to be applied starts it placed. Otherwise it moves out of its
    // own stereo image to the pan, not in from the centre.
    const bool movesOn = heard ? (mPlaced || mPanHeard) : (mPanRequest.waitingStart() && mPlacementRequest.waitingStart().value_or(0.0F) > 0.0F);
    if (movesOn) {
        mPanRequest.set(position, glideMilliseconds, heard);
    } else {
        mPanRequest.jumpTo(position);
    }
    // A new track starts fully placed, even on a player whose last track stopped part way into a glide
    if (!mPlaced || (!mTrackHeard && !mPlacementRequest.waitingStart())) {
        mPlacementRequest.set(1.0F, glideMilliseconds, heard);
    }
    mRequestedPan = position;
    mPlaced = true;
}

void TMediaVoice::unplace(const int glideMilliseconds)
{
    const QMutexLocker lock(&mRequestMutex);
    mPlacementRequest.set(0.0F, glideMilliseconds, heardFor(glideMilliseconds));
    mPlaced = false;
}

void TMediaVoice::beginTrack()
{
    const QMutexLocker lock(&mRequestMutex);
    mTrackHeard = false;
    for (Request* request : {&mGainRequest, &mPanRequest, &mPlacementRequest}) {
        request->applied = request->serial;
    }
}

void TMediaVoice::setMuted(const bool muted)
{
    mMuted = muted;
    TAudioMixer::self()->setMuted(mVoice, muted);
}

void TMediaVoice::render(const float* in, float* out, const qint64 frames, Glide& gain, Glide& pan, Glide& placement)
{
    for (qint64 frame = 0; frame < frames; ++frame) {
        const float level = gain.step();
        const float place = placement.step();
        const float position = pan.step();
        // Read before writing, as out may be in
        float left = in[2 * frame];
        float right = in[2 * frame + 1];
        if (place > 0.0F) {
            const float mono = (left + right) * 0.5F;
            const float angle = (std::clamp(position, -1.0F, 1.0F) + 1.0F) * std::numbers::pi_v<float> / 4.0F;
            // Constant power, so a sound keeps its loudness as it moves; centred, each side is 3 dB down,
            // as a boost there would push a sound panned hard to one side past full scale
            left += (mono * std::cos(angle) - left) * place;
            right += (mono * std::sin(angle) - right) * place;
        }
        out[2 * frame] = left * level;
        out[2 * frame + 1] = right * level;
    }
}

void TMediaVoice::audioBufferReceived(const QAudioBuffer& buffer)
{
    // An empty buffer comes with the end of a track, and carries no audio to mix
    if (!buffer.isValid()) {
        return;
    }

    const QAudioFormat format = buffer.format();
    if (format.sampleFormat() != QAudioFormat::Float || format.channelCount() != 2) {
        if (!mFormatRefusalReported.exchange(true)) {
            qWarning().noquote() << "TMediaVoice WARNING - the media backend handed over audio in a format that cannot be mixed: sample format" << format.sampleFormat() << "with"
                                 << format.channelCount() << "channels.";
        }
        return;
    }

    const QMutexLocker delivery(&mDeliveryMutex);
    // Decoded before a stop and delivered after it, or arriving as this voice goes
    if (mStopped || mShuttingDown) {
        return;
    }

    {
        // Under the lock, so a change asked for now is either applied here or made knowing audio plays
        const QMutexLocker lock(&mRequestMutex);
        mGainRequest.applyTo(mGain);
        mPanRequest.applyTo(mPan);
        mPlacementRequest.applyTo(mPlacement);
        mPanHeard = mPlacement.current > 0.0F || mPlacement.target > 0.0F;
        mTrackHeard = true;
        mReceivedAudio = true;
    }

    const qint64 frames = buffer.frameCount();
    std::vector<float> rendered(static_cast<size_t>(frames * 2));
    render(buffer.constData<float>(), rendered.data(), frames, mGain, mPan, mPlacement);
    TAudioMixer::self()->queue(mVoice, rendered.data(), frames);
}
