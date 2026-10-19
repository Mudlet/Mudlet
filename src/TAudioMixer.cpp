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


#include "TAudioMixer.h"

#include <QAudioSink>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QIODevice>
#include <QMediaDevices>
#include <QTimer>

#include <algorithm>
#include <functional>
#include <limits>

using namespace std::chrono_literals;

namespace {
// FFmpeg hands audio over at playback speed with no lead, in pieces of up to about 85 ms, so a
// voice that joined with less would run dry between pieces and stutter
constexpr qint64 headStartMicroseconds = 100'000;
// Decoding runs on the system clock and the output on the device's, so a voice slowly gains or
// loses audio; past this much the oldest goes, rather than the sound falling ever further behind
constexpr qint64 maxQueuedMicroseconds = headStartMicroseconds + 200'000;
constexpr qint64 sinkBufferMicroseconds = 60'000;
constexpr auto idleCheckInterval = 1s;
constexpr auto reopenBackoff = 1s;
std::atomic<qint64> gIdleCloseDelayMs = 10'000;
// When the mix last carried any sound, in milliseconds of the steady clock
std::atomic<qint64> gLastSoundMs = 0;

qint64 nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Pull mode: the output reads the mix whenever it wants more, and gets silence when nothing plays
class MixSource : public QIODevice
{
public:
    explicit MixSource(std::function<void(float*, qint64)> mix)
    : mMix(std::move(mix))
    {
        // Unbuffered, or QIODevice reads a whole chunk ahead that a stop could no longer drop
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return std::numeric_limits<int>::max(); }

protected:
    qint64 readData(char* data, const qint64 maxSize) override
    {
        constexpr qint64 frameBytes = 2 * static_cast<qint64>(sizeof(float));
        const qint64 frames = maxSize / frameBytes;
        mMix(reinterpret_cast<float*>(data), frames * 2);
        return frames * frameBytes;
    }
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    std::function<void(float*, qint64)> mMix;
};
} // namespace

// Lives on the mixer's thread: the output is created there, so it pulls the mix there too
class TAudioMixer::Output : public QObject
{
public:
    explicit Output(TAudioMixer* mixer)
    : mpMixer(mixer)
    {
    }

    void open()
    {
        mpMixer->mOpenRequested = false;
        if (mpSink || mpMixer->mOutputSuspended) {
            return;
        }
        if (mSinceFailure.isValid() && mSinceFailure.durationElapsed() < reopenBackoff) {
            return;
        }

        if (!mpIdleTimer) {
            mpIdleTimer = new QTimer(this);
            mpIdleTimer->setInterval(idleCheckInterval);
            QObject::connect(mpIdleTimer, &QTimer::timeout, this, [this]() {
                idleCheck();
            });
        }

        const QAudioFormat format = TAudioMixer::format();
        mpSource = new MixSource([this](float* out, const qint64 samples) {
            mpMixer->mix(out, samples);
        });
        mpSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this);
        mpSink->setBufferSize(format.bytesForDuration(sinkBufferMicroseconds));
        QObject::connect(mpSink, &QAudioSink::stateChanged, this, [this](const QAudio::State state) {
            if (state == QAudio::StoppedState && mpSink && mpSink->error() != QAudio::NoError) {
                failed();
            }
        });
        mpSink->start(mpSource);
        if (mpSink->error() != QAudio::NoError) {
            failed();
            return;
        }

        mFailureReported = false;
        mSinceFailure.invalidate();
        gLastSoundMs = nowMs();
        mpMixer->mOutputOpen = true;
        ++mpMixer->mTimesOpened;
        mpIdleTimer->start();
    }

    void close()
    {
        if (mpIdleTimer) {
            mpIdleTimer->stop();
        }
        if (mpSink) {
            // Stopped before the source goes, as the source is what it reads from
            mpSink->stop();
            delete mpSink;
            mpSink = nullptr;
        }
        delete mpSource;
        mpSource = nullptr;
        mpMixer->mOutputOpen = false;
    }

    void reopen()
    {
        if (mpSink) {
            close();
            open();
        }
    }

private:
    // Retried with later sound, no more than once a second, which is what a device coming back needs
    void failed()
    {
        if (!mFailureReported) {
            mFailureReported = true;
            qWarning().noquote() << "TAudioMixer WARNING - the audio output failed, error" << (mpSink ? mpSink->error() : QAudio::NoError) << "- sounds are silent until it can be opened again.";
        }
        mSinceFailure.start();
        // Posted: this can be the output's own signal, which must not delete it
        QMetaObject::invokeMethod(
                this,
                [this]() {
                    close();
                    mpMixer->dropEverything();
                },
                Qt::QueuedConnection);
    }

    void idleCheck()
    {
        if (nowMs() - gLastSoundMs >= gIdleCloseDelayMs && mpMixer->silent()) {
            close();
        }
    }

    TAudioMixer* mpMixer;
    QAudioSink* mpSink = nullptr;
    MixSource* mpSource = nullptr;
    QTimer* mpIdleTimer = nullptr;
    QElapsedTimer mSinceFailure;
    bool mFailureReported = false;
};

TAudioMixer* TAudioMixer::self()
{
    static TAudioMixer* mixer = new TAudioMixer();
    return mixer;
}

QAudioFormat TAudioMixer::format()
{
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);
    return format;
}

TAudioMixer::TAudioMixer()
: QObject(QCoreApplication::instance())
{
    mThread.setObjectName(QStringLiteral("Mudlet audio"));
    mpOutput = new Output(this);
    mpOutput->moveToThread(&mThread);
    mThread.start(QThread::TimeCriticalPriority);
}

TAudioMixer::~TAudioMixer()
{
    QMetaObject::invokeMethod(
            mpOutput,
            [this]() {
                mpOutput->close();
            },
            Qt::BlockingQueuedConnection);
    mThread.quit();
    mThread.wait();
    delete mpOutput;
}

int TAudioMixer::addVoice()
{
    QMutexLocker lock(&mMutex);
    const int voice = mNextVoice++;
    mVoices.emplace(voice, Voice());
    return voice;
}

void TAudioMixer::removeVoice(const int voice)
{
    QMutexLocker lock(&mMutex);
    mVoices.erase(voice);
}

void TAudioMixer::queue(const int voice, const float* frames, const qint64 frameCount)
{
    {
        QMutexLocker lock(&mMutex);
        const auto found = mVoices.find(voice);
        if (found == mVoices.end()) {
            return;
        }
        Voice& queued = found->second;
        queued.samples.insert(queued.samples.end(), frames, frames + frameCount * 2);
        const size_t maxQueued = static_cast<size_t>(format().framesForDuration(maxQueuedMicroseconds) * 2);
        if (queued.queuedSamples() > maxQueued) {
            queued.readPosition += queued.queuedSamples() - maxQueued;
        }
    }

    if (!mOutputOpen) {
        requestOpen();
    }
}

void TAudioMixer::clear(const int voice)
{
    QMutexLocker lock(&mMutex);
    const auto found = mVoices.find(voice);
    if (found != mVoices.end()) {
        found->second = Voice();
    }
}

void TAudioMixer::hold(const int voice, const bool held)
{
    QMutexLocker lock(&mMutex);
    const auto found = mVoices.find(voice);
    if (found != mVoices.end()) {
        found->second.held = held;
    }
}

void TAudioMixer::drain(const int voice)
{
    QMutexLocker lock(&mMutex);
    const auto found = mVoices.find(voice);
    if (found != mVoices.end()) {
        found->second.draining = true;
    }
}

void TAudioMixer::undrain(const int voice)
{
    QMutexLocker lock(&mMutex);
    const auto found = mVoices.find(voice);
    if (found != mVoices.end()) {
        found->second.draining = false;
    }
}

void TAudioMixer::refreshDevice()
{
    QMetaObject::invokeMethod(
            mpOutput,
            [this]() {
                mpOutput->reopen();
            },
            Qt::QueuedConnection);
}

qint64 TAudioMixer::queuedSamples(const int voice)
{
    QMutexLocker lock(&mMutex);
    qint64 queued = 0;
    for (const auto& [id, queuedVoice] : mVoices) {
        if (voice == 0 || id == voice) {
            queued += static_cast<qint64>(queuedVoice.queuedSamples());
        }
    }
    return queued;
}

void TAudioMixer::setIdleCloseDelay(const std::chrono::milliseconds delay)
{
    gIdleCloseDelayMs = delay.count();
}

void TAudioMixer::setOutputSuspended(const bool suspended)
{
    mOutputSuspended = suspended;
    if (suspended) {
        QMetaObject::invokeMethod(
                mpOutput,
                [this]() {
                    mpOutput->close();
                },
                Qt::BlockingQueuedConnection);
    }
}

void TAudioMixer::requestOpen()
{
    if (mOutputSuspended || mOpenRequested.exchange(true)) {
        return;
    }
    QMetaObject::invokeMethod(
            mpOutput,
            [this]() {
                mpOutput->open();
            },
            Qt::QueuedConnection);
}

void TAudioMixer::mix(float* out, const qint64 sampleCount)
{
    std::fill(out, out + sampleCount, 0.0F);
    const size_t headStart = static_cast<size_t>(format().framesForDuration(headStartMicroseconds) * 2);
    bool sounded = false;

    QMutexLocker lock(&mMutex);
    for (auto& [id, voice] : mVoices) {
        if (voice.held) {
            continue;
        }
        if (!voice.started) {
            if (voice.queuedSamples() == 0 || (voice.queuedSamples() < headStart && !voice.draining)) {
                continue;
            }
            voice.started = true;
        }

        const size_t take = std::min(static_cast<size_t>(sampleCount), voice.queuedSamples());
        const float* source = voice.samples.data() + voice.readPosition;
        for (size_t sample = 0; sample < take; ++sample) {
            out[sample] += source[sample];
        }
        voice.readPosition += take;
        sounded = sounded || take > 0;

        if (voice.queuedSamples() == 0) {
            // Ran dry: one still playing builds its head start again rather than stutter
            if (!voice.draining) {
                ++mTimesRanDry;
            }
            voice.samples.clear();
            voice.readPosition = 0;
            voice.started = false;
        } else if (voice.readPosition > voice.samples.size() / 2) {
            voice.samples.erase(voice.samples.begin(), voice.samples.begin() + static_cast<std::ptrdiff_t>(voice.readPosition));
            voice.readPosition = 0;
        }
    }
    lock.unlock();

    if (sounded) {
        gLastSoundMs = nowMs();
    }
    // Several loud sounds together can sum past full scale
    for (qint64 sample = 0; sample < sampleCount; ++sample) {
        out[sample] = std::clamp(out[sample], -1.0F, 1.0F);
    }
}

bool TAudioMixer::silent()
{
    QMutexLocker lock(&mMutex);
    return std::all_of(mVoices.cbegin(), mVoices.cend(), [](const auto& entry) {
        return entry.second.queuedSamples() == 0;
    });
}

void TAudioMixer::dropEverything()
{
    QMutexLocker lock(&mMutex);
    for (auto& [id, voice] : mVoices) {
        voice.samples.clear();
        voice.readPosition = 0;
        voice.started = false;
    }
}
