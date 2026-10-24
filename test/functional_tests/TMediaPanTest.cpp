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

#include <QDataStream>
#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

#include "Host.h"
#include "MediaBackendTestHelper.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TAudioMixer.h"
#include "TMedia.h"
#include "TMediaData.h"
#include "TMediaVoice.h"
#include "TelnetServerStub.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

/*
 * Client.Media 2 places a sound left or right. QAudioOutput has no pan, so a sound is placed by
 * its TMediaVoice on the way into the shared output, which only Qt's FFmpeg backend feeds. These
 * pin that a placed sound really is heard placed, that one started unplaced can be moved while it
 * plays, and that nothing else is placed.
 */
class TMediaPanTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "Test-Media-Pan";
    const QString mLocalhost = "localhost";
    static constexpr int clipMs = 6000;
    static constexpr int shortClipMs = 600;

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mHostname);
        if (!TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()))) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY(mpHost && mpHost->mpMedia);
        // Settled first, or on a backend nobody named the first sound plays unmixed while the probe runs.
        // Except in the test of what starts that probe.
        if (qstrcmp(QTest::currentTestFunction(), "test_aGameThatSendsMediaHasItsFirstSoundPlaced") != 0) {
            backendPlaces();
        }
    }

    void cleanup()
    {
        if (mpHost && mpHost->mpMedia) {
            mpHost->mpMedia->stopAllMediaPlayers();
        }
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        delete mudlet::self();
        TestProfile::removeProfileDirectory(mHostname);
    }

    // First, so nothing has checked the backend yet. A game that sends media has it checked before its
    // first sound, so even that one is placed.
    void test_aGameThatSendsMediaHasItsFirstSoundPlaced()
    {
        if (!qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty()) {
            QSKIP("A backend is named, so there is nothing to check before the first sound.");
        }
        QVERIFY2(!TMediaVoice::probeStarted(), "Something checked the backend before the game said anything.");
        QString package = qsl("Client.Media.Default");
        QString message = qsl("{\"url\": \"https://example.com/media/\"}");
        media()->parseGMCP(package, message);
        QVERIFY(TMediaVoice::probeStarted());
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     []() {
                                         return TMediaVoice::probePassed();
                                     },
                                     QDeadlineTimer(6s))
                                     ? QString()
                                     : noPanningReason());

        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });
        package = qsl("Client.Media.Play");
        message = qsl(R"({"name": "%1", "key": "first", "volume": 100, "direction": "east"})").arg(writeClip(qsl("first.wav"), clipMs, 8000));
        media()->parseGMCP(package, message);
        QVERIFY2(waitForStarted(), "The game's first sound never started playing.");
        QCOMPARE(media()->playersPlaced(), 1);
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return r > 0.1F && l < 1e-3F;
                         },
                         heard),
                 qPrintable(qsl("Played east, the game's first sound came out at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    void test_aSoundGivenAPanIsHeardPlaced()
    {
        TMediaData data = soundData(writeClip(qsl("panned.wav")));
        data.setMediaPan(-60);
        media()->playMedia(data);

        QVERIFY2(waitForStarted(), "The panned sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        QCOMPARE(media()->playersPlaced(), 1);
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
    }

    void test_aSoundGivenADirectionIsHeardPlaced()
    {
        TMediaData data = soundData(writeClip(qsl("directed.wav")));
        data.setMediaDirection(qsl("east"));
        media()->playMedia(data);

        QVERIFY2(waitForStarted(), "The directed sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
    }

    // Moving a sound is the point of an update, and a game need not have placed it to begin with
    void test_anUnplacedSoundIsMovedByAnUpdate()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });
        const QString fileName = writeClip(qsl("unplaced.wav"), clipMs, 8000);
        TMediaData data = soundData(fileName);
        data.setMediaKey(qsl("unplaced"));
        data.setMediaVolume(100);
        media()->playMedia(data);

        QVERIFY2(waitForStarted(), "The unplaced sound never started playing.");
        QCOMPARE(media()->playersPlaced(), 0);
        // Heard first, so the update finds it playing rather than still loading
        float left = 0.0F;
        float right = 0.0F;
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             std::tie(left, right) = loudestOnEachSide(20);
                             return left > 0.1F;
                         },
                         QDeadlineTimer(5s)),
                 "The unplaced sound's audio never reached the shared output.");
        QVERIFY2(std::abs(left - right) < 1e-3F, qPrintable(qsl("Unplaced, the mono sound came out at %1 left and %2 right.").arg(left).arg(right)));

        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("unplaced"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("west"), TMedia::defaultGlideMilliseconds);

        QCOMPARE(media()->playersPlaced(), 1);
        QCOMPARE(media()->placedSound(qsl("unplaced"))->pan(), -1.0F);
        QVERIFY2(stillPlaying(fileName), "Moving a playing sound stopped it.");
        QCOMPARE(playingData(fileName).mediaDirection(), qsl("west"));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return l > 0.1F && r < 1e-3F;
                         },
                         heard),
                 qPrintable(qsl("Moved west, the sound came out at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    // Every pass of a loop is a new source on the same player, and each has to reach the voice
    void test_everyPassOfALoopIsHeardPlaced()
    {
        TMediaData data = soundData(writeClip(qsl("loop.wav"), shortClipMs));
        data.setMediaPan(40);
        data.setMediaLoops(3);
        media()->playMedia(data);

        QVERIFY2(waitForStarted(), "The looping sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());

        int passesHeard = 1;
        bool heardThisPass = true;
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             const bool heard = media()->playersPlacedWithAudio() > 0;
                             if (heard && !heardThisPass) {
                                 ++passesHeard;
                             }
                             heardThisPass = heard;
                             return passesHeard >= 3;
                         },
                         QDeadlineTimer(10s)),
                 qPrintable(qsl("Only %1 of the loop's 3 passes reached the voice.").arg(passesHeard)));
    }

    // A sound given its own pan stays where it was put; one given a direction turns with the player
    void test_turningThePlayerMovesTheSoundsItPlaces()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TMediaData north = soundData(writeClip(qsl("north.wav")));
        north.setMediaKey(qsl("north"));
        north.setMediaDirection(qsl("north"));
        media()->playMedia(north);
        TMediaData fixed = soundData(writeClip(qsl("fixed.wav")));
        fixed.setMediaKey(qsl("fixed"));
        fixed.setMediaPan(40);
        media()->playMedia(fixed);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return media()->placedSound(qsl("north")) && media()->placedSound(qsl("fixed"));
                         },
                         QDeadlineTimer(10s)),
                 "The placed sounds never started.");
        QCOMPARE(media()->placedSound(qsl("north"))->pan(), 0.0F);

        QVERIFY(media()->setListenerFacing(qsl("east"), TMedia::FacingSetBy::Game));
        QCOMPARE(media()->placedSound(qsl("north"))->pan(), -1.0F);
        QCOMPARE(media()->placedSound(qsl("fixed"))->pan(), 0.4F);
        QVERIFY(media()->setListenerFacing(qsl("north"), TMedia::FacingSetBy::Game));
    }

    // Muted before it was placed, a sound moved mid-play is still muted, and follows the switch after
    void test_muteReachesASoundPlacedWhilePlaying()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TMediaData data = soundData(writeClip(qsl("muted.wav")));
        data.setMediaKey(qsl("muted"));
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The sound never started playing.");
        media()->muteMedia(TMediaData::MediaProtocolAPI);

        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("muted"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("east"), 0);
        QVERIFY(media()->placedSound(qsl("muted")));
        QVERIFY2(media()->placedSound(qsl("muted"))->muted(), "A muted sound was heard once it was placed.");

        media()->unmuteMedia(TMediaData::MediaProtocolAPI);
        QVERIFY(!media()->placedSound(qsl("muted"))->muted());
    }

    // The shared output holds audio ahead of what is heard; a stop must not leave that playing
    void test_aStopSilencesAPlacedSoundAtOnce()
    {
        // Shut, so the queue holds steady: an output that cannot open drops everything queued
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });
        TMediaData data = soundData(writeClip(qsl("stopped.wav")));
        data.setMediaKey(qsl("stopped"));
        data.setMediaPan(-50);
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The placed sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const TMediaVoice* voice = media()->placedSound(qsl("stopped"));
        QVERIFY(voice);
        const int mixerVoice = voice->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(mixerVoice) > 0;
                         },
                         QDeadlineTimer(3s)),
                 "The placed sound's audio never reached the shared output.");

        TMediaData stop = soundData(qsl("stopped.wav"));
        media()->stopMedia(stop);
        QVERIFY2(mixer->queuedSamples(mixerVoice) == 0, "A stopped placed sound kept its queued audio playing.");
    }

    // A fade steps the volume as the track plays, and an update moves it; both have to reach the voice
    void test_fadesAndUpdatesSetThePlacedSoundsLevel()
    {
        TMediaData data = soundData(writeClip(qsl("faded.wav")));
        data.setMediaKey(qsl("faded"));
        data.setMediaPan(20);
        data.setMediaVolume(50);
        data.setMediaFadeIn(4000);
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The placed sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        QVERIFY(media()->placedSound(qsl("faded")));
        QVERIFY2(media()->placedSound(qsl("faded"))->gain() < 0.5F, "A placed sound fading in started at its full volume.");

        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("faded"));
        media()->updateMedia(selection, 90, std::nullopt, QString(), 0);
        QVERIFY2(media()->placedSound(qsl("faded"))->gain() >= 0.9F - 1e-5F, "An update's volume never reached the placed sound.");
    }

    // A placed sound on a pooled player must still be cut by a stop: reusing the player reconnects TMedia's handlers
    void test_aPlacedSoundOnAReusedPlayerIsStillStoppedAtOnce()
    {
        TMediaData plain = soundData(writeClip(qsl("before.wav"), shortClipMs));
        media()->playMedia(plain);
        QVERIFY2(waitForStarted(), "The unplaced sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     [&]() {
                                         return media()->playersHoldingSource() == 0;
                                     },
                                     QDeadlineTimer(10s))
                                     ? QString()
                                     : qsl("This Qt Multimedia backend never finished the clip, so no player comes free to reuse."));

        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });
        TMediaData placed = soundData(writeClip(qsl("reused.wav")));
        placed.setMediaKey(qsl("reused"));
        placed.setMediaPan(70);
        media()->playMedia(placed);
        QVERIFY2(waitForStarted(), "The placed sound never started playing.");
        QCOMPARE(media()->mediaPlayerCount(), 1);
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const TMediaVoice* voice = media()->placedSound(qsl("reused"));
        QVERIFY(voice);
        const int mixerVoice = voice->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(mixerVoice) > 0;
                         },
                         QDeadlineTimer(3s)),
                 "The placed sound's audio never reached the shared output.");

        TMediaData stop = soundData(qsl("reused.wav"));
        media()->stopMedia(stop);
        QVERIFY2(mixer->queuedSamples(mixerVoice) == 0, "A placed sound on a reused player kept playing its queued audio after a stop.");
    }

    // A pooled player plays a sound that is not placed as it was recorded
    void test_aPlayerReusedForAnUnplacedSoundDoesNotPlaceIt()
    {
        TMediaData placed = soundData(writeClip(qsl("first.wav"), shortClipMs));
        placed.setMediaPan(-80);
        media()->playMedia(placed);
        QVERIFY2(waitForStarted(), "The placed sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     [&]() {
                                         return media()->playersHoldingSource() == 0;
                                     },
                                     QDeadlineTimer(10s))
                                     ? QString()
                                     : qsl("This Qt Multimedia backend never finished the clip, so no player comes free to reuse."));

        TMediaData plain = soundData(writeClip(qsl("second.wav")));
        media()->playMedia(plain);
        QVERIFY2(waitForStarted(), "The unplaced sound never started playing.");
        QCOMPARE(media()->mediaPlayerCount(), 1);
        QCOMPARE(media()->playersPlaced(), 0);
    }

    // Paused within the time a backend is given to hand over audio, a placed sound is not taken for one that feeds no voice
    void test_aPausedPlacedSoundIsNotTakenForSilence()
    {
        TMediaData data = soundData(writeClip(qsl("paused.wav")));
        data.setMediaKey(qsl("paused"));
        data.setMediaPan(60);
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The placed sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        media()->pauseMedia(data);
        QTest::qWait(800ms);
        QCOMPARE(media()->playersPlaced(), 1);
    }

    void test_aSoundWithNeitherPanNorDirectionIsNotPlaced()
    {
        TMediaData data = soundData(writeClip(qsl("plain.wav")));
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The sound never started playing.");
        QCOMPARE(media()->playersPlaced(), 0);
    }

    // Up and down name no side to put a sound on, so it plays as with no direction
    void test_aDirectionWithNoBearingLeavesTheSoundUnplaced()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TMediaData data = soundData(writeClip(qsl("upstairs.wav")));
        data.setMediaKey(qsl("upstairs"));
        data.setMediaDirection(qsl("up"));
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The sound never started playing.");
        QCOMPARE(media()->playersPlaced(), 0);
        QCOMPARE(playingData(qsl("upstairs.wav")).mediaDirection(), qsl("up"));

        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("upstairs"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("east"), 0);
        QCOMPARE(media()->playersPlaced(), 1);
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("down"), 0);
        QCOMPARE(media()->playersPlaced(), 0);
    }

    // The voice's settings alone would pass every other test here; this one listens to what it renders
    void test_aPlacedSoundComesOutOfOneSideAndMovesToTheOther()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("tone.wav"), clipMs, 8000));
        data.setMediaKey(qsl("tone"));
        data.setMediaVolume(100);
        data.setMediaPan(-100);
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The tone never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const int voice = media()->placedSound(qsl("tone"))->mixerVoice();
        // The output is shut, so the queue fills to the voice's cap, well past the head start it needs to be mixed
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The tone's audio never reached the shared output.");

        auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(left > 0.1F && right < 1e-3F, qPrintable(qsl("Panned hard left, the tone came out at %1 left and %2 right.").arg(left).arg(right)));

        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("tone"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("east"), 0);
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return r > 0.1F && l < 1e-3F;
                         },
                         heard),
                 qPrintable(qsl("Moved east, the tone came out at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    // updateMusic() reaches the music's audio, not only the volume it reports, and glides over its duration
    void test_updateMusicGlidesTheMusicsVolume()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("theme.wav"), clipMs, 8000));
        data.setMediaType(TMediaData::MediaTypeMusic);
        data.setMediaKey(qsl("theme"));
        data.setMediaVolume(100);
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The music never started playing.");
        // Before the first audio there is nothing to glide from, and a change is made at once
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     [this]() {
                                         return media()->firstMixedVoice() && media()->firstMixedVoice()->receivedAudio();
                                     },
                                     QDeadlineTimer(5s))
                                     ? QString()
                                     : noPanningReason());
        const int voice = media()->firstMixedVoice()->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The music's audio never reached the shared output.");
        const float start = loudestOnEachSide(20).first;
        QVERIFY2(start > 0.1F, qPrintable(qsl("At volume 100 the music came out at only %1.").arg(start)));

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("updateMusic({key = 'theme', volume = 10, duration = 3000})")));
        QElapsedTimer sinceUpdate;
        sinceUpdate.start();
        // Well into the glide; what is mixed lags what is decoded by up to a voice's 300 ms queue
        QTest::qWait(1500ms);
        const float partway = loudestOnEachSide(20).first;
        // Read only once the glide is over, or it is caught passing through the band on its way somewhere else
        QTest::qWait(std::max<qint64>(0, 3700 - sinceUpdate.elapsed()));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         // A band, not a ceiling: a read that lands between two pieces of audio is silence
                         [start](const float l, const float) {
                             return l > start * 0.05F && l < start * 0.15F;
                         },
                         heard,
                         2s),
                 qPrintable(qsl("Updated to volume 10, the music stayed at %1 (from %2).").arg(heard.first).arg(start)));
        const float settled = heard.first;
        QVERIFY2(partway < start * 0.85F && partway > settled * 1.5F,
                 qPrintable(qsl("Partway through a 3 s glide the music was at %1, between %2 at the start and %3 at the end.").arg(partway).arg(start).arg(settled)));
    }

    // A game moves a sound by sending its play and an update together, before any of it is heard
    void test_anUpdateSentWithThePlayMovesFromWhereItWasPlayed()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("passing.wav"), clipMs, 8000));
        data.setMediaKey(qsl("passing"));
        data.setMediaVolume(100);
        data.setMediaDirection(qsl("east"));
        media()->playMedia(data);
        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("passing"));
        media()->updateMedia(selection, 20, std::nullopt, qsl("west"), 2000);

        QVERIFY2(waitForStarted(), "The sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const int voice = media()->placedSound(qsl("passing"))->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The sound's audio never reached the shared output.");

        // Its first moments, still close to where it was played: on the right, and loud
        const auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(right > left && right > 0.1F, qPrintable(qsl("Played east and moved west over 2 s, it began at %1 left and %2 right.").arg(left).arg(right)));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return l > 0.03F && l < 0.04F && r < 1e-3F;
                         },
                         heard,
                         5s),
                 qPrintable(qsl("Moved west at volume 20, the sound ended at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    // Two moves sent before any audio: the second sets out from where the first left the sound
    void test_aSecondUpdateBeforeAnyAudioMovesFromWhereThePlayPutIt()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("flier.wav"), clipMs, 8000));
        data.setMediaKey(qsl("flier"));
        data.setMediaVolume(100);
        data.setMediaPan(100);
        media()->playMedia(data);
        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("flier"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("up"), 2000);
        media()->updateMedia(selection, std::nullopt, -100, QString(), 2000);

        QVERIFY2(waitForStarted(), "The sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const int voice = media()->placedSound(qsl("flier"))->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The sound's audio never reached the shared output.");

        const auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(right > left, qPrintable(qsl("Played right and moved left over 2 s, it began at %1 left and %2 right.").arg(left).arg(right)));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return l > 0.1F && r < 1e-3F;
                         },
                         heard,
                         5s),
                 qPrintable(qsl("Moved left, the sound ended at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    // Turning before any audio leaves an update's blend out of the sound's own image to run its course
    void test_aTurnBeforeAnyAudioLeavesAnUpdatesBlendRunning()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("overhead.wav"), clipMs, 8000));
        data.setMediaKey(qsl("overhead"));
        data.setMediaVolume(100);
        media()->playMedia(data);
        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("overhead"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("east"), 3000);
        QVERIFY(media()->setListenerFacing(qsl("north"), TMedia::FacingSetBy::Game));

        QVERIFY2(waitForStarted(), "The sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const int voice = media()->placedSound(qsl("overhead"))->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The sound's audio never reached the shared output.");

        const auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(left > 0.1F, qPrintable(qsl("Blending east over 3 s from as it was recorded, it began at %1 left and %2 right.").arg(left).arg(right)));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return l < 1e-3F && r > 0.1F;
                         },
                         heard,
                         6s),
                 qPrintable(qsl("Moved east, the sound ended at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    // A track stopped before it was heard leaves no glide behind for the next one on its player
    void test_theNextSoundOnAPlayerWhoseTrackWasNeverHeardStartsPlaced()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData unheard = soundData(writeClip(qsl("unheard.wav"), clipMs, 8000));
        unheard.setMediaKey(qsl("unheard"));
        media()->playMedia(unheard);
        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("unheard"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("east"), 5000);
        media()->stopMedia(selection);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return media()->playersHoldingSource() == 0;
                         },
                         QDeadlineTimer(5s)),
                 "The stopped sound never gave up its player.");

        TMediaData next = soundData(writeClip(qsl("next.wav"), clipMs, 8000));
        next.setMediaKey(qsl("next"));
        next.setMediaVolume(100);
        next.setMediaPan(-100);
        media()->playMedia(next);
        QCOMPARE(media()->mediaPlayerCount(), 1);
        QVERIFY2(waitForStarted(), "The next sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const int voice = media()->placedSound(qsl("next"))->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The next sound's audio never reached the shared output.");

        const auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(left > 0.1F && right < 1e-3F, qPrintable(qsl("Panned hard left, the next sound began at %1 left and %2 right.").arg(left).arg(right)));
    }

    // A loop's next pass is the same track: a move sent between passes glides on into it
    void test_aMoveSentBetweenPassesOfALoopGlides()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("ambience.wav"), shortClipMs, 8000));
        data.setMediaKey(qsl("ambience"));
        data.setMediaVolume(100);
        data.setMediaDirection(qsl("east"));
        data.setMediaLoops(-1);
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The looping sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const TMediaVoice* voice = media()->placedSound(qsl("ambience"));
        QVERIFY(voice);
        const int mixerVoice = voice->mixerVoice();

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return !voice->receivedAudio();
                         },
                         QDeadlineTimer(5s)),
                 "The loop never reached the gap between two passes.");
        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("ambience"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("west"), 3000);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return voice->receivedAudio();
                         },
                         QDeadlineTimer(5s)),
                 "The loop's next pass never reached the voice.");
        // Long enough for the cap to drop the last pass's tail, short enough to still be on the right
        QTest::qWait(400ms);
        QVERIFY(mixer->queuedSamples(mixerVoice) > 0);

        const auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(right > left, qPrintable(qsl("Moved from east to west over 3 s between passes, the next pass began at %1 left and %2 right.").arg(left).arg(right)));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return l > 0.1F && r < 1e-3F;
                         },
                         heard,
                         6s),
                 qPrintable(qsl("Moved west between passes, the loop ended up at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    void test_eachPassOfALoopThatFadesInStartsSilent_data()
    {
        QTest::addColumn<int>("fadeOut");
        QTest::newRow("fading in") << static_cast<int>(TMediaData::MediaFadeNotSet);
        QTest::newRow("fading in and out") << 200;
    }

    // Each pass of a loop that fades in starts silent, not from the level the last pass ended at
    void test_eachPassOfALoopThatFadesInStartsSilent()
    {
        QFETCH(int, fadeOut);
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("swell.wav"), shortClipMs, 8000));
        data.setMediaVolume(100);
        data.setMediaFadeIn(100);
        data.setMediaFadeOut(fadeOut);
        data.setMediaLoops(-1);
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The looping sound never started playing.");
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return media()->firstMixedVoice() && media()->firstMixedVoice()->receivedAudio();
                         },
                         QDeadlineTimer(5s)),
                 "The looping sound's audio never reached its voice.");
        const TMediaVoice* voice = media()->firstMixedVoice();
        const int mixerVoice = voice->mixerVoice();

        // Mixed away during the gap, so what is queued next is only the new pass
        bool emptiedInTheGap = false;
        for (int attempt = 0; attempt < 5 && !emptiedInTheGap; ++attempt) {
            QVERIFY2(QTest::qWaitFor(
                             [&]() {
                                 return voice->receivedAudio();
                             },
                             QDeadlineTimer(5s)),
                     "The loop's pass never reached the voice.");
            QVERIFY2(QTest::qWaitFor(
                             [&]() {
                                 return !voice->receivedAudio();
                             },
                             QDeadlineTimer(5s)),
                     "The loop never reached the gap between two passes.");
            std::vector<float> lastPass(static_cast<size_t>(mixer->queuedSamples(mixerVoice)));
            mixer->mixForTests(lastPass.data(), static_cast<qint64>(lastPass.size()));
            emptiedInTheGap = !voice->receivedAudio() && mixer->queuedSamples(mixerVoice) == 0;
        }
        QVERIFY2(emptiedInTheGap, "Never emptied the shared output between two passes.");
        // Just past the head start the mixer waits for, so a stall has the most room before its cap drops the pass's start
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(mixerVoice) >= 110 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The loop's next pass never reached the shared output.");

        const auto [left, right] = loudestOnEachSide(5);
        QVERIFY2(left < 0.02F && right < 0.02F, qPrintable(qsl("Fading in from silence, the next pass began at %1 left and %2 right.").arg(left).arg(right)));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return l > 0.1F && r > 0.1F;
                         },
                         heard),
                 qPrintable(qsl("Faded in, the next pass rose only to %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    // Overhead and then off to one side: placed again before it has finished blending back, it still glides
    void test_aSoundPlacedAgainWhileLeavingItsPlaceGlidesThere()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData data = soundData(writeClip(qsl("bird.wav"), clipMs, 8000));
        data.setMediaKey(qsl("bird"));
        data.setMediaVolume(100);
        data.setMediaDirection(qsl("east"));
        media()->playMedia(data);
        QVERIFY2(waitForStarted(), "The sound never started playing.");
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        const int voice = media()->placedSound(qsl("bird"))->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The sound's audio never reached the shared output.");

        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("bird"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("up"), 3000);
        QTest::qWait(100ms);
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("west"), 3000);
        QTest::qWait(700ms);

        const auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(right > left, qPrintable(qsl("Moved from east to west over 3 s, a moment in it was at %1 left and %2 right.").arg(left).arg(right)));
        std::pair<float, float> heard;
        QVERIFY2(comesOut(
                         [](const float l, const float r) {
                             return l > 0.1F && r < 1e-3F;
                         },
                         heard,
                         5s),
                 qPrintable(qsl("Moved west, the sound ended at %1 left and %2 right.").arg(heard.first).arg(heard.second)));
    }

    // An output of its own glides its volume on a timer, which must not outlive the track it was for
    void test_aVolumeGlideOnAnOutputOfItsOwnEndsWithItsTrack()
    {
        TMediaData data = soundData(writeClip(qsl("glide.wav"), shortClipMs, 8000));
        TMediaPlayer player(mpHost, data);
        QAudioOutput* output = player.mediaPlayer()->audioOutput();
        QVERIFY(output);
        player.setVolume(10);
        player.glideVolume(80, 10000);
        QTest::qWait(100ms);
        player.claimSource(QUrl::fromLocalFile(qsl("%1/glide.wav").arg(MudletApp::getMudletPath(enums::profileMediaPath, mHostname))));
        player.setVolume(80);
        QTest::qWait(100ms);
        QVERIFY2(std::abs(output->volume() - 0.8F) < 0.01F, qPrintable(qsl("A new track set to 80 played at %1.").arg(output->volume())));

        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        player.setVolume(10);
        player.glideVolume(80, 10000);
        QTest::qWait(100ms);
        QVERIFY(player.setMixed(true));
        player.setVolume(80);
        QVERIFY2(std::abs(player.voice()->gain() - 0.8F) < 0.01F, qPrintable(qsl("Mixed and set to 80, the sound played at %1.").arg(player.voice()->gain())));
    }

    // A pooled player stopped part way into a long glide starts its next sound at that sound's own volume
    void test_aReusedPlayerStartsItsNextSoundAtItsOwnVolume()
    {
        SKIP_OR_FAIL_WITHOUT(backendPlaces() ? QString() : noPanningReason());
        TAudioMixer* mixer = TAudioMixer::self();
        mixer->setOutputSuspended(true);
        auto resume = qScopeGuard([mixer]() {
            mixer->setOutputSuspended(false);
        });

        TMediaData first = soundData(writeClip(qsl("gliding.wav"), clipMs, 8000));
        first.setMediaKey(qsl("gliding"));
        first.setMediaVolume(100);
        media()->playMedia(first);
        QVERIFY2(waitForStarted(), "The first tone never started playing.");
        // Only a sound that is heard glides; before its first audio a change is made at once
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     [this]() {
                                         return media()->firstMixedVoice() && media()->firstMixedVoice()->receivedAudio();
                                     },
                                     QDeadlineTimer(5s))
                                     ? QString()
                                     : noPanningReason());
        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("gliding"));
        media()->updateMedia(selection, 20, std::nullopt, QString(), 10000);
        QTest::qWait(200ms);
        TMediaData stop = soundData(qsl("gliding.wav"));
        media()->stopMedia(stop);
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return media()->playersHoldingSource() == 0;
                         },
                         QDeadlineTimer(5s)),
                 "The stopped tone never let go of its player.");

        TMediaData second = soundData(writeClip(qsl("steady-tone.wav"), clipMs, 8000));
        second.setMediaVolume(20);
        media()->playMedia(second);
        QVERIFY2(waitForStarted(), "The second tone never started playing.");
        QCOMPARE(media()->mediaPlayerCount(), 1);
        SKIP_OR_FAIL_WITHOUT(media()->firstMixedVoice() ? QString() : noPanningReason());
        const int voice = media()->firstMixedVoice()->mixerVoice();
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return mixer->queuedSamples(voice) >= 200 * 48 * 2;
                         },
                         QDeadlineTimer(5s)),
                 "The second tone's audio never reached the shared output.");

        // The tone is about 0.24 at full volume, so about 0.05 at 20
        const auto [left, right] = loudestOnEachSide(20);
        QVERIFY2(left < 0.07F && left > 0.03F, qPrintable(qsl("Played at volume 20 on a reused player, the tone came out at %1.").arg(left)));
    }

    // The protocol lets a client ignore placement for music and video, and Mudlet does
    void test_musicIsNeverPanned()
    {
        TMediaData data = soundData(writeClip(qsl("music.wav")));
        data.setMediaType(TMediaData::MediaTypeMusic);
        data.setMediaPan(80);
        media()->playMedia(data);

        QVERIFY2(waitForStarted(), "The music never started playing.");
        QCOMPARE(media()->playersPlaced(), 0);
    }

    void test_aGmcpPlayAndUpdatePlaceTheSound()
    {
        const QString fileName = writeClip(qsl("gmcp.wav"));
        QString package = qsl("Client.Media.Play");
        QString message = qsl(R"({"name": "%1", "key": "dragon", "volume": 5, "direction": "N"})").arg(fileName);
        media()->parseGMCP(package, message);

        TMediaData criteria;
        criteria.setMediaProtocol(TMediaData::MediaProtocolGMCP);
        criteria.setMediaType(TMediaData::MediaTypeSound);
        criteria.setMediaKey(qsl("dragon"));
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return !media()->playingMedia(criteria).isEmpty();
                         },
                         QDeadlineTimer(10s)),
                 "The Client.Media.Play never started playing.");
        QCOMPARE(media()->playingMedia(criteria).at(0).mediaDirection(), qsl("north"));
        QCOMPARE(media()->playersPlaced(), backendPlaces() ? 1 : 0);

        package = qsl("Client.Media.Update");
        message = qsl(R"({"key": "dragon", "pan": -100.4, "volume": "8", "duration": 0})");
        media()->parseGMCP(package, message);
        const TMediaData moved = media()->playingMedia(criteria).at(0);
        QCOMPARE(moved.mediaPan(), std::optional<int>(-100));
        QCOMPARE(moved.mediaVolume(), 8);

        message = qsl(R"({"name": "%1", "direction": "w"})").arg(fileName);
        media()->parseGMCP(package, message);
        QCOMPARE(media()->playingMedia(criteria).at(0).mediaDirection(), qsl("west"));
        QCOMPARE(media()->playingMedia(criteria).at(0).mediaPan(), std::nullopt);

        // Without a key or a name it would move every sound the game has, so it moves none
        message = qsl(R"({"pan": 50})");
        media()->parseGMCP(package, message);
        QCOMPARE(media()->playingMedia(criteria).at(0).mediaPan(), std::nullopt);
    }

    // As a stop's, an update's tag chooses the sounds and its type narrows them
    void test_aGmcpUpdateChoosesByTagAndType()
    {
        QString package = qsl("Client.Media.Play");
        for (const auto& [key, tag, type] :
             {std::tuple{qsl("rain"), qsl("Weather"), qsl("sound")}, std::tuple{qsl("sword"), qsl("combat"), qsl("sound")}, std::tuple{qsl("storm"), qsl("weather"), qsl("music")}}) {
            QString message = qsl(R"({"name": "%1", "key": "%2", "tag": "%3", "type": "%4", "volume": 5})").arg(writeClip(key + qsl(".wav")), key, tag, type);
            media()->parseGMCP(package, message);
            QVERIFY2(QTest::qWaitFor(
                             [&]() {
                                 return gmcpPlaying(key).mediaKey() == key;
                             },
                             QDeadlineTimer(10s)),
                     qPrintable(qsl("The Client.Media.Play of %1 never started playing.").arg(key)));
        }

        package = qsl("Client.Media.Update");
        QString message = qsl(R"({"tag": "WEATHER", "pan": 40, "volume": 7, "duration": 0})");
        media()->parseGMCP(package, message);
        QCOMPARE(gmcpPlaying(qsl("rain")).mediaPan(), std::optional<int>(40));
        QCOMPARE(gmcpPlaying(qsl("rain")).mediaVolume(), 7);
        QCOMPARE(gmcpPlaying(qsl("storm")).mediaVolume(), 7);
        QCOMPARE(gmcpPlaying(qsl("storm")).mediaPan(), std::nullopt);
        QCOMPARE(gmcpPlaying(qsl("sword")).mediaPan(), std::nullopt);
        QCOMPARE(gmcpPlaying(qsl("sword")).mediaVolume(), 5);

        message = qsl(R"({"tag": "weather", "type": "sound", "volume": 9, "duration": 0})");
        media()->parseGMCP(package, message);
        QCOMPARE(gmcpPlaying(qsl("rain")).mediaVolume(), 9);
        QCOMPARE(gmcpPlaying(qsl("storm")).mediaVolume(), 7);

        // An unknown type is refused rather than read as every type, and a type alone chooses nothing
        for (const auto& refused : {qsl(R"({"tag": "weather", "type": "film", "volume": 3, "duration": 0})"), qsl(R"({"type": "sound", "volume": 3, "duration": 0})")}) {
            message = refused;
            media()->parseGMCP(package, message);
            QCOMPARE(gmcpPlaying(qsl("rain")).mediaVolume(), 9);
            QCOMPARE(gmcpPlaying(qsl("sword")).mediaVolume(), 5);
        }
    }

    // A game can pass any exit name: one that is no direction plays the sound as with none
    void test_aGmcpPlayWithAnUnknownDirectionStillPlays()
    {
        const QString fileName = writeClip(qsl("portal.wav"));
        QString package = qsl("Client.Media.Play");
        QString message = qsl(R"({"name": "%1", "key": "portal", "direction": "portal"})").arg(fileName);
        media()->parseGMCP(package, message);

        TMediaData criteria;
        criteria.setMediaProtocol(TMediaData::MediaProtocolGMCP);
        criteria.setMediaType(TMediaData::MediaTypeSound);
        criteria.setMediaKey(qsl("portal"));
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return !media()->playingMedia(criteria).isEmpty();
                         },
                         QDeadlineTimer(10s)),
                 "A Client.Media.Play with a direction that is no direction never played.");
        QVERIFY(media()->playingMedia(criteria).at(0).mediaDirection().isEmpty());
        QCOMPARE(media()->playersPlaced(), 0);
    }

    void test_aGmcpListenerTurnsTheCompass()
    {
        QCOMPARE(media()->listenerFacing(), qsl("north"));
        QString package = qsl("Client.Media.Listener");
        QString message = qsl(R"({"facing": "e"})");
        media()->parseGMCP(package, message);
        QCOMPARE(media()->listenerFacing(), qsl("east"));

        message = qsl(R"({"facing": 200})");
        media()->parseGMCP(package, message);
        QCOMPARE(media()->listenerFacing(), qsl("200"));

        // The player turns to a compass direction, not to up or to one of the relative words
        for (const auto& refused : {qsl(R"({"facing": "up"})"), qsl(R"({"facing": "left"})"), qsl(R"({"facing": "sideways"})")}) {
            message = refused;
            media()->parseGMCP(package, message);
            QCOMPARE(media()->listenerFacing(), qsl("200"));
        }
    }

    void test_aFacingTheGameGaveEndsWithItsConnection_data()
    {
        QTest::addColumn<QString>("scriptFacing");
        QTest::addColumn<QString>("gameFacing");
        QTest::addColumn<QString>("afterwards");
        QTest::newRow("the game's") << QString() << qsl("east") << qsl("north");
        QTest::newRow("a script's") << qsl("east") << QString() << qsl("east");
        QTest::newRow("a script's, then the game's") << qsl("east") << qsl("200") << qsl("east");
    }

    // The game sends its facing again when it connects again, but nothing would give a script's back
    void test_aFacingTheGameGaveEndsWithItsConnection()
    {
        QFETCH(QString, scriptFacing);
        QFETCH(QString, gameFacing);
        QFETCH(QString, afterwards);

        // Mudlet's side, not the server's: the server can see the connection before Mudlet has taken it up
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState;
                         },
                         QDeadlineTimer(5s)),
                 "The profile never connected to the test server.");
        if (!scriptFacing.isEmpty()) {
            QVERIFY(media()->setListenerFacing(scriptFacing, TMedia::FacingSetBy::Script));
        }
        if (!gameFacing.isEmpty()) {
            QVERIFY(media()->setListenerFacing(gameFacing, TMedia::FacingSetBy::Game));
        }
        QSignalSpy disconnected(&mpHost->mTelnet, &cTelnet::signal_disconnected);
        mpHost->mTelnet.disconnectIt();
        QVERIFY2(QTest::qWaitFor(
                         [&disconnected]() {
                             return !disconnected.isEmpty();
                         },
                         QDeadlineTimer(8s)),
                 "The profile never noticed its own disconnect.");
        QCOMPARE(media()->listenerFacing(), afterwards);
    }

private:
    TMedia* media() const { return mpHost->mpMedia.data(); }

    TMediaData soundData(const QString& fileName) const
    {
        TMediaData data;
        data.setMediaProtocol(TMediaData::MediaProtocolAPI);
        data.setMediaType(TMediaData::MediaTypeSound);
        data.setMediaInput(TMediaData::MediaInputFile);
        data.setMediaFileName(fileName);
        data.setMediaVolume(1); // quiet enough not to disturb a desktop run
        return data;
    }

    bool waitForStarted() const
    {
        return QTest::qWaitFor(
                [&]() {
                    return media()->playersInPlayingState() > 0;
                },
                QDeadlineTimer(10s));
    }

    bool waitForPlacedAudio() const
    {
        return QTest::qWaitFor(
                [&]() {
                    return media()->playersPlacedWithAudio() > 0;
                },
                QDeadlineTimer(3s));
    }

    // Measured, not assumed: QAudioBufferOutput is documented to work only on Qt's FFmpeg backend
    QString noPanningReason() const
    {
        return qsl("This Qt Multimedia backend (\"%1\") hands no audio to a voice, so a placed sound plays as recorded here. Needs Qt's FFmpeg backend.")
                .arg(QString::fromLocal8Bit(qgetenv("QT_MEDIA_BACKEND")));
    }

    // Asking starts the one-time probe of a backend nobody named, which takes a moment to answer
    static bool backendPlaces()
    {
        if (!qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty()) {
            return !TMediaVoice::unsupported();
        }
        return QTest::qWaitFor(
                []() {
                    return !TMediaVoice::unsupported();
                },
                QDeadlineTimer(6s));
    }

    TMediaData playingData(const QString& fileName) const
    {
        TMediaData criteria = soundData(fileName);
        const QList<TMediaData> playing = media()->playingMedia(criteria);
        return playing.isEmpty() ? TMediaData() : playing.at(0);
    }

    TMediaData gmcpPlaying(const QString& key) const
    {
        TMediaData criteria;
        criteria.setMediaProtocol(TMediaData::MediaProtocolGMCP);
        criteria.setMediaKey(key);
        for (const TMediaData& playing : media()->playingMedia(criteria)) {
            if (playing.mediaKey() == key) {
                return playing;
            }
        }
        return {};
    }

    bool stillPlaying(const QString& fileName) const
    {
        TMediaData criteria = soundData(fileName);
        return !media()->playingMedia(criteria).isEmpty();
    }

    // Mixes 20 ms at a time until the check passes. Polled rather than waited for, as a runner stalled past
    // a fixed wait still holds audio from before a move.
    static bool comesOut(const std::function<bool(float, float)>& where, std::pair<float, float>& heard, const std::chrono::milliseconds limit = 3s)
    {
        return QTest::qWaitFor(
                [&]() {
                    heard = loudestOnEachSide(20);
                    return where(heard.first, heard.second);
                },
                QDeadlineTimer(limit));
    }

    // Mixes this many milliseconds by hand and returns the loudest sample on each side
    static std::pair<float, float> loudestOnEachSide(const int ms)
    {
        std::vector<float> out(static_cast<size_t>(ms * 48 * 2));
        TAudioMixer::self()->mixForTests(out.data(), static_cast<qint64>(out.size()));
        float left = 0.0F;
        float right = 0.0F;
        for (size_t i = 0; i + 1 < out.size(); i += 2) {
            left = std::max(left, std::abs(out[i]));
            right = std::max(right, std::abs(out[i + 1]));
        }
        return {left, right};
    }

    QString writeClip(const QString& fileName, const int durationMs = clipMs, const qint16 level = 0) const
    {
        const QString mediaPath = MudletApp::getMudletPath(enums::profileMediaPath, mHostname);
        if (!QDir().mkpath(mediaPath)) {
            QTest::qFail("Could not create the profile media directory.", __FILE__, __LINE__);
            return {};
        }
        QFile file(qsl("%1/%2").arg(mediaPath, fileName));
        if (!file.open(QIODevice::WriteOnly)) {
            QTest::qFail("Could not write the test media file.", __FILE__, __LINE__);
            return {};
        }
        file.write(wavBytes(durationMs, level));
        return fileName;
    }

    // 16-bit mono PCM at one steady level, silent unless a test listens to where it comes out
    static QByteArray wavBytes(const int durationMs, const qint16 level)
    {
        constexpr int sampleRate = 8000;
        constexpr int bytesPerSample = 2;
        const int dataBytes = sampleRate * bytesPerSample * durationMs / 1000;

        QByteArray wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData("RIFF", 4);
        out << static_cast<quint32>(36 + dataBytes);
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << static_cast<quint32>(16);
        out << static_cast<quint16>(1);
        out << static_cast<quint16>(1);
        out << static_cast<quint32>(sampleRate);
        out << static_cast<quint32>(sampleRate * bytesPerSample);
        out << static_cast<quint16>(bytesPerSample);
        out << static_cast<quint16>(8 * bytesPerSample);
        out.writeRawData("data", 4);
        out << static_cast<quint32>(dataBytes);
        for (int sample = 0; sample < dataBytes / bytesPerSample; ++sample) {
            out << level;
        }
        return wav;
    }
};

#include "TMediaPanTest.moc"
MUDLET_GROUPED_TEST_MAIN(TMediaPanTest)
