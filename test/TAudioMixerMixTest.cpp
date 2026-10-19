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
#include "TMediaVoice.h"

#include <QtTest/QtTest>

#include <vector>

// The mix itself, run by hand with the output kept shut, so none of this needs an audio device
class TAudioMixerMixTest : public QObject
{
    Q_OBJECT

private:
    static constexpr qint64 framesPerMs = 48;
    std::vector<int> mVoices;

    int voice()
    {
        mVoices.push_back(TAudioMixer::self()->addVoice());
        return mVoices.back();
    }

    // Queues this many milliseconds of a steady level on both sides
    static void queue(const int voice, const qint64 ms, const float level = 0.5F)
    {
        const std::vector<float> frames(static_cast<size_t>(ms * framesPerMs * 2), level);
        TAudioMixer::self()->queue(voice, frames.data(), ms * framesPerMs);
    }

    // Mixes this many milliseconds and returns the loudest sample heard
    static float mix(const qint64 ms)
    {
        std::vector<float> out(static_cast<size_t>(ms * framesPerMs * 2));
        TAudioMixer::self()->mixForTests(out.data(), static_cast<qint64>(out.size()));
        return *std::max_element(out.cbegin(), out.cend());
    }

private slots:
    void initTestCase() { TAudioMixer::self()->setOutputSuspended(true); }

    void cleanup()
    {
        for (const int id : mVoices) {
            TAudioMixer::self()->removeVoice(id);
        }
        mVoices.clear();
    }

    // FFmpeg delivers at playback speed with no lead, so a voice that joined with less would stutter
    void aVoiceWaitsForItsHeadStart()
    {
        const int id = voice();
        queue(id, 50);
        QCOMPARE(mix(10), 0.0F);
        queue(id, 60);
        QCOMPARE(mix(10), 0.5F);
    }

    // A sound shorter than the head start is still heard once its track has ended
    void aShortSoundPlaysOnceItsTrackEnds()
    {
        const int id = voice();
        queue(id, 40);
        QCOMPARE(mix(10), 0.0F);
        TAudioMixer::self()->drain(id);
        QCOMPARE(mix(10), 0.5F);
    }

    // The last audio can arrive after the end was reported, and must not sit forever under the head start
    void audioArrivingAfterTheEndStillPlays()
    {
        const int id = voice();
        TAudioMixer::self()->drain(id);
        queue(id, 30);
        QCOMPARE(mix(10), 0.5F);
        mix(30);
        QCOMPARE(TAudioMixer::self()->queuedSamples(id), 0);
    }

    // The next track waits for a head start again
    void aNewTrackWaitsForItsHeadStartAgain()
    {
        const int id = voice();
        TAudioMixer::self()->drain(id);
        queue(id, 20);
        mix(20);
        TAudioMixer::self()->undrain(id);
        queue(id, 40);
        QCOMPARE(mix(10), 0.0F);
    }

    // Paused, a voice keeps its audio unplayed; resumed, it carries on from there
    void aHeldVoiceKeepsItsAudio()
    {
        const int id = voice();
        queue(id, 150);
        TAudioMixer::self()->hold(id, true);
        QCOMPARE(mix(50), 0.0F);
        QCOMPARE(TAudioMixer::self()->queuedSamples(id), 150 * framesPerMs * 2);
        TAudioMixer::self()->hold(id, false);
        QCOMPARE(mix(50), 0.5F);
    }

    void clearingOneVoiceLeavesTheOthers()
    {
        const int stopped = voice();
        const int playing = voice();
        queue(stopped, 150);
        queue(playing, 150);
        TAudioMixer::self()->clear(stopped);
        QCOMPARE(TAudioMixer::self()->queuedSamples(stopped), 0);
        QCOMPARE(TAudioMixer::self()->queuedSamples(playing), 150 * framesPerMs * 2);
    }

    // Decoding and the device keep slightly different time; a voice must not fall ever further behind
    void aVoiceNeverHoldsMoreThanItsCap()
    {
        const int id = voice();
        queue(id, 2000);
        QCOMPARE(TAudioMixer::self()->queuedSamples(id), 300 * framesPerMs * 2);
    }

    void aVoiceThatRanDryWaitsForItsHeadStartAgain()
    {
        const int id = voice();
        queue(id, 120);
        mix(120);
        queue(id, 50);
        QCOMPARE(mix(10), 0.0F);
    }

    // Loud sounds together sum past full scale, which an output would wrap or distort
    void theMixNeverGoesPastFullScale()
    {
        queue(voice(), 150, 0.8F);
        queue(voice(), 150, 0.8F);
        QCOMPARE(mix(10), 1.0F);
    }

    void aVoiceIsRenderedWithItsGain()
    {
        const std::vector<float> in{0.5F, -0.5F, 0.5F, -0.5F};
        std::vector<float> out(4);
        TMediaVoice::Glide gain;
        gain.glideTo(0.5F, 0);
        TMediaVoice::render(in.data(), out.data(), 2, gain);
        QCOMPARE(out[0], 0.25F);
        QCOMPARE(out[1], -0.25F);
    }

    // What a voice queued before it was muted is silenced too, and still used up, so unmuting does not replay it
    void aMutedVoiceIsSilentIncludingWhatItHadQueued()
    {
        const int id = voice();
        queue(id, 150);
        TAudioMixer::self()->setMuted(id, true);
        QCOMPARE(mix(50), 0.0F);
        QCOMPARE(TAudioMixer::self()->queuedSamples(id), 100 * framesPerMs * 2);
        TAudioMixer::self()->setMuted(id, false);
        QCOMPARE(mix(50), 0.5F);
    }

    // A paused sound holds its audio unheard, and must not keep the output open
    void aHeldVoiceDoesNotCountAsWaitingToBeHeard()
    {
        const int id = voice();
        queue(id, 150);
        QVERIFY(!TAudioMixer::self()->silent());
        TAudioMixer::self()->hold(id, true);
        QVERIFY(TAudioMixer::self()->silent());
        TAudioMixer::self()->hold(id, false);
        QVERIFY(!TAudioMixer::self()->silent());
    }

    void aGlideReachesItsTargetOnTheLastFrameAndNotBefore()
    {
        TMediaVoice::Glide glide;
        glide.glideTo(0.0F, 0);
        glide.glideTo(1.0F, 4);
        QVERIFY(glide.step() < 1.0F);
        glide.step();
        glide.step();
        QCOMPARE(glide.step(), 1.0F);
        QCOMPARE(glide.step(), 1.0F);
    }
};

QTEST_MAIN(TAudioMixerMixTest)
#include "TAudioMixerMixTest.moc"
