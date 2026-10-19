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
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <chrono>
#include <cmath>
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
        // Settled first, or on a backend nobody named the first sound plays unmixed while the probe runs
        backendPlaces();
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
        const QString fileName = writeClip(qsl("unplaced.wav"));
        TMediaData data = soundData(fileName);
        data.setMediaKey(qsl("unplaced"));
        media()->playMedia(data);

        QVERIFY2(waitForStarted(), "The unplaced sound never started playing.");
        QCOMPARE(media()->playersPlaced(), 0);

        TMediaData selection;
        selection.setMediaProtocol(TMediaData::MediaProtocolAPI);
        selection.setMediaType(TMediaData::MediaTypeSound);
        selection.setMediaKey(qsl("unplaced"));
        media()->updateMedia(selection, std::nullopt, std::nullopt, qsl("west"), TMedia::defaultGlideMilliseconds);

        QCOMPARE(media()->playersPlaced(), 1);
        QCOMPARE(media()->placedSound(qsl("unplaced"))->pan(), -1.0F);
        SKIP_OR_FAIL_WITHOUT(waitForPlacedAudio() ? QString() : noPanningReason());
        QVERIFY2(stillPlaying(fileName), "Moving a playing sound stopped it.");
        QCOMPARE(playingData(fileName).mediaDirection(), qsl("west"));
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

        QVERIFY(media()->setListenerFacing(qsl("east")));
        QCOMPARE(media()->placedSound(qsl("north"))->pan(), -1.0F);
        QCOMPARE(media()->placedSound(qsl("fixed"))->pan(), 0.4F);
        QVERIFY(media()->setListenerFacing(qsl("north")));
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
        QVERIFY(TAudioMixer::self()->queuedSamples(mixerVoice) > 0);

        TMediaData stop = soundData(qsl("stopped.wav"));
        media()->stopMedia(stop);
        QVERIFY2(TAudioMixer::self()->queuedSamples(mixerVoice) == 0, "A stopped placed sound kept its queued audio playing.");
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
        media()->updateMedia(selection, 10, std::nullopt, QString(), 0);
        QVERIFY2(media()->placedSound(qsl("faded"))->gain() <= 0.1F + 1e-5F, "An update's volume never reached the placed sound.");
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
        QVERIFY(TAudioMixer::self()->queuedSamples(mixerVoice) > 0);

        TMediaData stop = soundData(qsl("reused.wav"));
        media()->stopMedia(stop);
        QVERIFY2(TAudioMixer::self()->queuedSamples(mixerVoice) == 0, "A placed sound on a reused player kept playing its queued audio after a stop.");
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
        // What was queued before the move is dropped oldest first as the voice's cap is reached
        QTest::qWait(700ms);
        std::tie(left, right) = loudestOnEachSide(20);
        QVERIFY2(right > 0.1F && left < 1e-3F, qPrintable(qsl("Moved east, the tone came out at %1 left and %2 right.").arg(left).arg(right)));
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

    void test_theFacingGoesBackToNorthWhenTheGameDisconnects()
    {
        // Mudlet's side, not the server's: the server can see the connection before Mudlet has taken it up
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState;
                         },
                         QDeadlineTimer(5s)),
                 "The profile never connected to the test server.");
        QVERIFY(media()->setListenerFacing(qsl("east")));
        QSignalSpy disconnected(&mpHost->mTelnet, &cTelnet::signal_disconnected);
        mpHost->mTelnet.disconnectIt();
        QVERIFY2(QTest::qWaitFor(
                         [&disconnected]() {
                             return !disconnected.isEmpty();
                         },
                         QDeadlineTimer(8s)),
                 "The profile never noticed its own disconnect.");
        QCOMPARE(media()->listenerFacing(), qsl("north"));
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

    bool stillPlaying(const QString& fileName) const
    {
        TMediaData criteria = soundData(fileName);
        return !media()->playingMedia(criteria).isEmpty();
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
