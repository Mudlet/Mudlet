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
#include "TMedia.h"
#include "TMediaVoice.h"

#include <QtTest/QtTest>

#include <cmath>
#include <vector>

// The mix and how each sound is placed in it, run by hand with the output kept shut, so none of this
// needs an audio device
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

    // Frames of the same sample on both sides, as a mono source arrives in the stereo format asked for
    static std::vector<float> steady(const qint64 frames, const float sample) { return std::vector<float>(static_cast<size_t>(frames * 2), sample); }

    static void renderAt(const float pan, std::vector<float>& audio)
    {
        TMediaVoice::Glide gain;
        gain.glideTo(1.0F, 0);
        TMediaVoice::Glide position;
        position.glideTo(pan, 0);
        TMediaVoice::Glide placement;
        placement.glideTo(1.0F, 0);
        TMediaVoice::render(audio.data(), audio.data(), static_cast<qint64>(audio.size() / 2), gain, position, placement);
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
        TMediaVoice::Glide pan;
        TMediaVoice::Glide placement;
        TMediaVoice::render(in.data(), out.data(), 2, gain, pan, placement);
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

    // A stop drops what a voice queued, not its mute
    void aStoppedVoiceStaysMuted()
    {
        const int id = voice();
        TAudioMixer::self()->setMuted(id, true);
        queue(id, 150);
        TAudioMixer::self()->clear(id);
        queue(id, 150);
        QCOMPARE(mix(50), 0.0F);
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

    // A change asked for at once still takes 20 ms, or the jump clicks
    void aChangeAskedForAtOnceStillGlides()
    {
        QCOMPARE(TMediaVoice::glideFramesFor(0), 20 * framesPerMs);
        QCOMPARE(TMediaVoice::glideFramesFor(100), 100 * framesPerMs);
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

    // Centred is 3 dB down on each side, so the same power as all on one side
    void centredIsThreeDecibelsDownOnBothSides()
    {
        auto audio = steady(4, 0.5F);
        renderAt(0.0F, audio);
        QVERIFY(std::abs(audio[0] - 0.5F * std::sqrt(0.5F)) < 1e-5F);
        QVERIFY(std::abs(audio[1] - 0.5F * std::sqrt(0.5F)) < 1e-5F);
    }

    // A loud sound panned hard to one side must not go past what it was, or past full scale
    void aFullPanNeverRaisesTheSource()
    {
        auto audio = steady(4, 1.0F);
        renderAt(1.0F, audio);
        QVERIFY(audio[1] <= 1.0F + 1e-6F);
    }

    void aFullPanSilencesTheOtherSide()
    {
        auto left = steady(4, 0.5F);
        renderAt(-1.0F, left);
        QVERIFY(std::abs(left[0] - 0.5F) < 1e-5F);
        QVERIFY(std::abs(left[1]) < 1e-5F);

        auto right = steady(4, 0.5F);
        renderAt(1.0F, right);
        QVERIFY(std::abs(right[0]) < 1e-5F);
        QVERIFY(std::abs(right[1] - 0.5F) < 1e-5F);
    }

    // The power across both sides is the same wherever the sound is, so moving it does not change how loud it is
    void thePowerStaysTheSameAcrossThePan()
    {
        for (const float pan : {-1.0F, -0.6F, -0.2F, 0.0F, 0.3F, 0.8F, 1.0F}) {
            auto audio = steady(1, 0.5F);
            renderAt(pan, audio);
            const float power = audio[0] * audio[0] + audio[1] * audio[1];
            QVERIFY2(std::abs(power - 0.25F) < 1e-4F, qPrintable(QString("power %1 at pan %2").arg(power).arg(pan)));
        }
    }

    // A stereo source is one point once placed, so a sound recorded off to one side can still be moved across
    void aStereoSourceIsMixedToOnePoint()
    {
        std::vector<float> audio{1.0F, 0.0F};
        renderAt(0.0F, audio);
        QVERIFY(std::abs(audio[0] - audio[1]) < 1e-5F);
        QVERIFY(audio[0] > 0.0F);
    }

    void aPanThatGlidesMovesAcrossTheBuffer()
    {
        auto audio = steady(100, 0.5F);
        TMediaVoice::Glide gain;
        gain.glideTo(1.0F, 0);
        TMediaVoice::Glide position;
        position.glideTo(-1.0F, 0);
        position.glideTo(1.0F, 100);
        TMediaVoice::Glide placement;
        placement.glideTo(1.0F, 0);
        TMediaVoice::render(audio.data(), audio.data(), 100, gain, position, placement);
        QVERIFY(audio.front() > audio[1]);
        QVERIFY(audio[198] < audio.back());
    }

    // Not placed, a sound keeps its own stereo image, at full level on each side
    void anUnplacedSoundKeepsItsStereoImage()
    {
        std::vector<float> audio{0.8F, 0.2F};
        TMediaVoice::Glide gain;
        gain.glideTo(1.0F, 0);
        TMediaVoice::Glide position;
        position.glideTo(1.0F, 0);
        TMediaVoice::Glide placement;
        TMediaVoice::render(audio.data(), audio.data(), 1, gain, position, placement);
        QCOMPARE(audio[0], 0.8F);
        QCOMPARE(audio[1], 0.2F);
    }

    // Placed as it plays, a sound moves out of its stereo image rather than jumping, which clicks
    void aSoundPlacedWhilePlayingMovesThereGradually()
    {
        auto audio = steady(100, 0.5F);
        TMediaVoice::Glide gain;
        gain.glideTo(1.0F, 0);
        TMediaVoice::Glide position;
        position.glideTo(-1.0F, 0);
        TMediaVoice::Glide placement;
        placement.glideTo(1.0F, 100);
        TMediaVoice::render(audio.data(), audio.data(), 100, gain, position, placement);
        QVERIFY(audio[1] > 0.45F);
        QVERIFY(audio[101] > 0.0F && audio[101] < audio[1]);
        QVERIFY(std::abs(audio[199]) < 1e-5F);
    }

    void aDirectionIsPlacedFromTheWayThePlayerFaces_data()
    {
        QTest::addColumn<QString>("direction");
        QTest::addColumn<QString>("facing");
        // An invalid QVariant for none: a direction with no bearing leaves the sound unplaced
        QTest::addColumn<QVariant>("pan");
        QTest::newRow("east, facing north") << "east" << "north" << QVariant(100);
        QTest::newRow("west, facing north") << "west" << "north" << QVariant(-100);
        QTest::newRow("north, facing north") << "north" << "north" << QVariant(0);
        QTest::newRow("south, facing north") << "south" << "north" << QVariant(0);
        QTest::newRow("45 degrees, facing north") << "45" << "north" << QVariant(71);
        QTest::newRow("north, facing east") << "north" << "east" << QVariant(-100);
        QTest::newRow("south, facing east") << "south" << "east" << QVariant(100);
        QTest::newRow("left, facing east") << "left" << "east" << QVariant(-100);
        QTest::newRow("right, facing south") << "right" << "south" << QVariant(100);
        QTest::newRow("ahead, facing west") << "ahead" << "west" << QVariant(0);
        QTest::newRow("up") << "up" << "north" << QVariant();
        QTest::newRow("down") << "down" << "east" << QVariant();
    }

    void aDirectionIsPlacedFromTheWayThePlayerFaces()
    {
        QFETCH(QString, direction);
        QFETCH(QString, facing);
        QFETCH(QVariant, pan);
        TMediaData data;
        data.setMediaDirection(direction);
        const std::optional<int> placed = TMedia::stereoPan(data, facing);
        QCOMPARE(placed ? QVariant(*placed) : QVariant(), pan);
    }

    // A game that sends both tunes each kind of client: a stereo one uses the pan
    void aPanTakesOverFromADirection()
    {
        TMediaData data;
        data.setMediaDirection(qsl("east"));
        data.setMediaPan(-30);
        QCOMPARE(TMedia::stereoPan(data, qsl("north")), std::optional<int>(-30));
        QCOMPARE(TMedia::stereoPan(TMediaData(), qsl("north")), std::nullopt);
    }

    void directionsAreKeptInOneSpelling()
    {
        QCOMPARE(TMedia::canonicalMediaDirection(QString("N")), QString("north"));
        QCOMPARE(TMedia::canonicalMediaDirection(QString(" Southwest ")), QString("southwest"));
        QCOMPARE(TMedia::canonicalMediaDirection(QString("u")), QString("up"));
        QCOMPARE(TMedia::canonicalMediaDirection(QString("Behind")), QString("behind"));
        QCOMPARE(TMedia::canonicalMediaDirection(QString("45")), QString("45"));
        QCOMPARE(TMedia::canonicalMediaDirection(450.0), QString("90"));
        QCOMPARE(TMedia::canonicalMediaDirection(-90.0), QString("270"));
        QCOMPARE(TMedia::canonicalMediaDirection(359.6), QString("0"));
        QVERIFY(TMedia::canonicalMediaDirection(QString("sideways")).isEmpty());
        // Exit names, not directions: a game says where such a sound comes from in its caption
        QVERIFY(TMedia::canonicalMediaDirection(QString("in")).isEmpty());
        QVERIFY(TMedia::canonicalMediaDirection(QString("out")).isEmpty());
        QVERIFY(TMedia::canonicalMediaDirection(QString()).isEmpty());
        QVERIFY(TMedia::canonicalMediaDirection(std::nan("")).isEmpty());
    }
};

QTEST_MAIN(TAudioMixerMixTest)
#include "TAudioMixerMixTest.moc"
