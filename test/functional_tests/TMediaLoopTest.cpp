/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QAudioOutput>
#include <QDeadlineTimer>
#include <QMediaPlayer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMedia.h"
#include "TMediaData.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForMediaLoop();

using namespace std::chrono_literals;

// A skip is how these tests stay honest on a backend that cannot stage what they need. It is
// also how the whole file could go green everywhere and mean nothing, if a CI image lost its
// codecs or swapped its default backend - macOS already skips most of them by design, so a
// second platform quietly joining it would look no different. Runners known to carry a backend
// that can demonstrate everything here set MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK, which turns
// every capability skip into a failure and makes that loss a red build instead of silence.
#define SKIP_OR_FAIL_WITHOUT(reason)                                                                                                                                                                   \
    do {                                                                                                                                                                                               \
        const QString incapable = (reason);                                                                                                                                                            \
        if (!incapable.isEmpty()) {                                                                                                                                                                    \
            if (qEnvironmentVariableIsSet("MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK")) {                                                                                                                    \
                QFAIL(qPrintable(qsl("MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK is set for this runner, so a backend that cannot do this is a failure and "                                                  \
                                     "not a skip: %1")                                                                                                                                                 \
                                         .arg(incapable)));                                                                                                                                            \
            }                                                                                                                                                                                          \
            QSKIP(qPrintable(incapable));                                                                                                                                                              \
        }                                                                                                                                                                                              \
    } while (false)

/*
 * Regression guard for "Client.Media loops=-1 plays once" (issue #9566): the deferred media
 * source release in TMedia, and the generation counters documented on TMediaPlayer that decide
 * whether it still applies by the time its turn comes. The obligations that follow:
 *
 *   - a looping track must survive the stop/restart cycle (the #9566 bug itself);
 *   - a finite loops=N track must reach every pass, which goes through the playlist
 *     branch of the same handler;
 *   - a track that genuinely finishes, or is stopped outright, must still release
 *     its source, or the resource release #9237 added is lost;
 *   - a source the backend cannot decode must release itself off the error signal, since
 *     the player was already stopped and no playback state change follows;
 *   - a player re-sourced during the deferred turn, by a different track or by a
 *     continue=false restart of the same one, must keep the source it was given;
 *   - each of those endings must raise sysMediaFinished exactly once, since releasing the
 *     source alone leaves a script chaining its next track off that event waiting forever,
 *     and announcing twice re-enters any handler that stops the media it was told about.
 *
 * Which of those a backend can demonstrate varies, so probeBackend() measures one up front and
 * each test skips with what it found. CMakeLists.txt pins QT_MEDIA_BACKEND on the platforms
 * where main.cpp does, and leaves it to Qt elsewhere, exactly as the shipped application does -
 * so a skip reflects what users actually get.
 */
class TMediaLoopTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "Test-Media-Loop";
    const QString mPort = "4012";
    const QString mLocalhost = "localhost";

    // Length of the generated clip. Long enough that "still playing" cannot be an
    // artefact of start-up latency, short enough to loop several times quickly.
    static constexpr int clipMs = 400;

    QTemporaryDir mProbeDir;
    // Set when the backend never reaches PlayingState, so nothing below can even be started.
    QString mCannotStartReason;
    // Set when the backend starts a clip but never decodes it through to EndOfMedia.
    QString mCannotPlayReason;
    // Set when the backend ends a track with EndOfMedia before StoppedState.
    QString mWrongOrderReason;
    // Set when the backend starts playing synchronously, so a player that has just been
    // re-sourced can never be mistaken for a stopped one - which is the whole race the
    // claim counter exists to settle.
    QString mSynchronousStartReason;
    // Set when the backend does not report an undecodable file as an error.
    QString mNoLoadErrorReason;

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForMediaLoop();
        probeBackend();
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // A looping track must still be playing well after its first pass would have
    // ended. Before the fix the first StoppedState cleared the source, EndOfMedia
    // was never delivered and the player dropped out of the playing set for good.
    void test_loopingTrackKeepsPlayingPastFirstPass()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotPlayReason);
        SKIP_OR_FAIL_WITHOUT(mWrongOrderReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("loop.wav"));
        QVERIFY(!fileName.isEmpty());

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The looping track never started playing.");

        // Span several passes so a single missed restart cannot pass by luck, and land off a
        // clip boundary: StoppedState and the EndOfMedia that restarts the loop are separate
        // signals, and in the window between them a healthy player reads as not playing.
        QTest::qWait(clipMs * 4 + clipMs / 2);

        QVERIFY2(waitForPlaying(media, fileName, 2s), "A loops=-1 track stopped after its first pass - the StoppedState cleanup suppressed EndOfMedia and the loop never restarted.");
    }

    // Every pass of a finite loops=N track after the first comes from the playlist branch of
    // the same EndOfMedia handler, which the indefinite-loop test never reaches. Its final
    // pass is also the one place a continuation ends and the deferred cleanup must take over.
    // Both hold whichever way round the backend emits EndOfMedia and StoppedState, so unlike
    // the loop test this one needs no mWrongOrderReason gate.
    void test_finiteLoopsReachEveryPass()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotPlayReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("finite.wav"));
        QVERIFY(!fileName.isEmpty());

        TMediaData data = clipData(fileName);
        data.setMediaLoops(3);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The finite-loop track never started playing.");

        // Into the second pass, which only happens if the playlist advanced.
        QTest::qWait(clipMs + clipMs / 2);
        QVERIFY2(waitForPlaying(media, fileName, 2s), "A loops=3 track stopped after its first pass - the playlist never advanced to the next entry.");

        // ...and the last pass must still hand back to the cleanup rather than loop forever.
        const bool cleanedUp = QTest::qWaitFor(
                [&]() {
                    return !playing(media, fileName) && media->playersHoldingSource() == 0;
                },
                QDeadlineTimer(10s));

        QVERIFY2(cleanedUp, "A loops=3 track never finished and released its source - the deferred cleanup did not take over from the last pass.");
    }

    // The deferred cleanup must still fire for a genuinely finished track, otherwise the
    // media source release added by #9237 is lost. Releasing the source is what this asserts
    // on because playingMedia() has already dropped the player by the time the cleanup runs.
    void test_oneShotTrackIsCleanedUpWhenItFinishes()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotPlayReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        watchMediaFinished();

        const QString fileName = writeClip(qsl("oneshot.wav"));
        QVERIFY(!fileName.isEmpty());

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsDefault);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The one-shot track never started playing.");

        const bool cleanedUp = QTest::qWaitFor(
                [&]() {
                    return !playing(media, fileName) && media->playersHoldingSource() == 0;
                },
                QDeadlineTimer(10s));

        QVERIFY2(cleanedUp, "A finished one-shot track never released its media source - the deferred cleanup did not run.");

        QVERIFY2(waitForMediaFinishedCount(1), "A finished one-shot track raised no single sysMediaFinished - a script chaining its next track off that event would wait forever.");
        QVERIFY2(mediaFinishedHolds(qsl("mediaFinishedNames[1] == 'oneshot.wav'")), "sysMediaFinished named the wrong file for a finished one-shot track.");
    }

    // The same obligation as above, reached by an explicit stop rather than by the clip
    // ending. It asks the least of the backend of any test here - only that playback starts -
    // so it is the one that still runs on a runner whose backend cannot decode a clip.
    //
    // Deliberately the weaker waitForPlaying(): on an asynchronous backend that lands the stop
    // while the track is still loading, which is a player Qt already considers stopped and so
    // one that reports no state change to end its playback. Holding a source for good is
    // exactly what that used to cost, so this is the case worth keeping.
    void test_stoppedTrackReleasesItsSource()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotStartReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        watchMediaFinished();

        const QString fileName = writeClip(qsl("stopped.wav"));
        QVERIFY(!fileName.isEmpty());

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The track never started playing.");

        TMediaData stop = clipData(fileName);
        media->stopMedia(stop);

        // Asserted separately from the release below: "still playing" and "still holding a
        // source" are different faults with different causes, and a combined wait cannot say
        // which of them a failure is.
        const bool stopped = QTest::qWaitFor(
                [&]() {
                    return !playing(media, fileName);
                },
                QDeadlineTimer(10s));

        QVERIFY2(stopped, "A stopped track was still reported as playing - stopMedia() did not take it out of the playing set.");

        const bool released = QTest::qWaitFor(
                [&]() {
                    return media->playersHoldingSource() == 0;
                },
                QDeadlineTimer(10s));

        QVERIFY2(released, "A stopped track never released its media source - the deferred release did not run.");

        QVERIFY2(waitForMediaFinishedCount(1), "A stopped track raised no single sysMediaFinished - the silent stop this test exists for is only half fixed if the release happens without it.");
        QVERIFY2(mediaFinishedHolds(qsl("mediaFinishedNames[1] == 'stopped.wav'")), "sysMediaFinished named the wrong file for a stopped track.");

        // Nothing more may be said about it afterwards. The source outlives the event by a
        // turn, so the stop, the error handler and a StoppedState report can each still find a
        // playback that looks live and announce the same ending over again.
        QTest::qWait(clipMs);
        QVERIFY2(mediaFinishedHolds(qsl("mediaFinishedCount == 1")), "A stopped track raised sysMediaFinished more than once for the same playback.");
    }

    // Two ways a stop can say something it should not. A bare stopMusic() matches every player
    // there is, including pooled ones between tracks that have nothing playing to end; and a
    // stop issued from inside a sysMediaFinished handler - the natural place for a script to
    // decide it has heard enough - lands on a player still holding the source of the track it
    // was just told about, which used to look exactly like one more playback to end. That
    // announced again, re-entered the same handler, and recursed until the stack gave out.
    void test_stopDoesNotAnnounceWhatIsNotPlaying()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotStartReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        watchMediaFinished(qsl("stopMusic()"));

        const QString fileName = writeClip(qsl("recursion.wav"));
        QVERIFY(!fileName.isEmpty());

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The track never started playing.");

        TMediaData stop = clipData(fileName);
        media->stopMedia(stop);

        QVERIFY2(waitForMediaFinishedCount(1), "A stopped track raised no single sysMediaFinished, so the handler that stops it again never ran and the recursion this test guards was never staged.");

        // Everything is idle by now, so a stop matching every player has nothing left to end.
        TMediaData stopEverything;
        stopEverything.setMediaProtocol(TMediaData::MediaProtocolAPI);
        media->stopMedia(stopEverything);

        QTest::qWait(clipMs);
        QVERIFY2(mediaFinishedHolds(qsl("mediaFinishedCount == 1")),
                 "A stop announced a playback that was already over - either the handler's own stopMusic() recursed back through it, or pooled players holding nothing were ended too.");
    }

    // A source that fails to load reports an error and no playback state change, because a
    // player that was already stopped - as every claimSource() on a new or finished player
    // leaves it, and as a loop restart or playlist advance finds it - has nothing to change
    // from. Without the error being acted on, the track falls silent still holding a source
    // nothing will ever release.
    void test_unplayableTrackReleasesItsSource()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotPlayReason);
        SKIP_OR_FAIL_WITHOUT(mNoLoadErrorReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        watchMediaFinished();

        const QString fileName = writeUnplayableClip(qsl("broken.wav"));
        QVERIFY(!fileName.isEmpty());

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        // Without this the wait below is satisfied at once by a play() that bailed out early,
        // and the error path this test exists for is never reached. claimSource() sets the
        // source inside playMedia() and the release is deferred, so the count is settled here.
        QCOMPARE(media->playersHoldingSource(), 1);

        const bool released = QTest::qWaitFor(
                [&]() {
                    return media->playersHoldingSource() == 0;
                },
                QDeadlineTimer(10s));

        QVERIFY2(released, "A track that could not be decoded held on to its media source - the playback error was never acted on.");

        QVERIFY2(waitForMediaFinishedCount(1), "A track that could not be decoded raised no single sysMediaFinished - a script chaining its next track off that event would wait forever.");
        QVERIFY2(mediaFinishedHolds(qsl("mediaFinishedNames[1] == 'broken.wav'")), "sysMediaFinished named the wrong file for a track that could not be decoded.");

        // The error and the StoppedState that can follow it are two reports of one failure.
        QTest::qWait(clipMs);
        QVERIFY2(mediaFinishedHolds(qsl("mediaFinishedCount == 1")), "A track that could not be decoded raised sysMediaFinished more than once for the same failure.");
    }

    // A player that is handed to a different track in the same event-loop turn, as
    // stopMusic{} followed by playMusic{} in one script does, must keep the new source. The
    // pending cleanup belongs to the track that stopped, and on an asynchronously starting
    // backend (see mSynchronousStartReason) the player still reads as stopped while it loads.
    void test_reusedPlayerKeepsTheTrackThatClaimedIt()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotStartReason);
        SKIP_OR_FAIL_WITHOUT(mSynchronousStartReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString firstFile = writeClip(qsl("first.wav"));
        const QString secondFile = writeClip(qsl("second.wav"));
        QVERIFY(!firstFile.isEmpty() && !secondFile.isEmpty());

        TMediaData first = clipData(firstFile);
        first.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(first);

        QVERIFY2(waitForPlaybackStarted(media, firstFile), "The first track never started playing.");

        const int playerCount = media->mediaPlayerCount();

        TMediaData stopFirst = clipData(firstFile);
        media->stopMedia(stopFirst);

        TMediaData second = clipData(secondFile);
        second.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(second);

        QVERIFY2(waitForPlaying(media, secondFile), "The replacement track never started playing.");

        // Without this the test passes vacuously on a second player, having never exercised
        // the claim the deferred cleanup has to notice.
        QCOMPARE(media->mediaPlayerCount(), playerCount);

        // Past the turn the stopped track's cleanup was scheduled for.
        QTest::qWait(clipMs);

        QVERIFY2(waitForPlaying(media, secondFile, 2s), "The replacement track was cut off - the previous track's deferred cleanup cleared the source out from under it.");
    }

    // continue=false restarts a track by stopping it and re-sourcing the same player inside
    // one call. That player is matched rather than newly acquired, so the restart has to
    // register the claim itself or the stop it just performed clears the source it just set.
    void test_restartedTrackKeepsItsNewSource()
    {
        SKIP_OR_FAIL_WITHOUT(mCannotStartReason);
        SKIP_OR_FAIL_WITHOUT(mSynchronousStartReason);

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("restart.wav"));
        QVERIFY(!fileName.isEmpty());

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        QVERIFY2(waitForPlaybackStarted(media, fileName), "The track never started playing.");

        TMediaData restart = clipData(fileName);
        restart.setMediaLoops(TMediaData::MediaLoopsRepeat);
        restart.setMediaContinue(TMediaData::MediaContinueRestart);
        media->playMedia(restart);

        // Past the turn the stop inside that restart scheduled its cleanup for.
        QTest::qWait(clipMs);

        QVERIFY2(waitForPlaying(media, fileName, 2s), "A restarted track was cut off - the cleanup deferred by its own stop cleared the source it had just been given.");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

private:
    TMedia* startProfileAndGetMedia()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QTest::qFail("No active host available for the test.", __FILE__, __LINE__);
            return nullptr;
        }
        auto* media = host->mpMedia.data();
        if (!media) {
            QTest::qFail("Host has no TMedia instance.", __FILE__, __LINE__);
            return nullptr;
        }
        mpHost = host;
        return media;
    }

    // sysMediaFinished is half of what the fixes here are for - a track that fails to load and
    // one stopped while it is still loading each used to end in silence, with a script chaining
    // its next track off that event waiting forever. Releasing the source, which is all the
    // tests otherwise assert on, happens either way, so nothing here would notice the event
    // going missing. Counted rather than merely seen: announcing the same ended playback more
    // than once is its own bug, and one of them recursed until the stack gave out.
    void watchMediaFinished(const QString& extraHandlerBody = QString())
    {
        if (!mpHost) {
            return;
        }

        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("mediaFinishedCount = 0\n"
                                                                 "mediaFinishedNames = {}\n"
                                                                 "registerAnonymousEventHandler('sysMediaFinished', function(_, fileName)\n"
                                                                 "  mediaFinishedCount = mediaFinishedCount + 1\n"
                                                                 "  mediaFinishedNames[#mediaFinishedNames + 1] = fileName\n"
                                                                 "  %1\n"
                                                                 "end)\n")
                                                                     .arg(extraHandlerBody));
    }

    // Runs a Lua assertion against what watchMediaFinished() recorded; compileAndExecuteScript()
    // reports a raised error as false, so a failed assert() comes back here as one.
    bool mediaFinishedHolds(const QString& luaCondition) const { return mpHost && mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(%1)").arg(luaCondition)); }

    bool waitForMediaFinishedCount(int count, std::chrono::milliseconds timeout = 10s) const
    {
        return QTest::qWaitFor(
                [&]() {
                    return mediaFinishedHolds(qsl("mediaFinishedCount == %1").arg(count));
                },
                QDeadlineTimer(timeout));
    }

    // Records what this backend is and is not able to demonstrate; each reason string set below
    // spells out what that costs the tests reading it. Probed once, because it has to wait out a
    // whole clip and the suite gives each test executable one wall-clock budget for all of its
    // slots. test_stoppedTrackReleasesItsSource needs no capability and always runs.
    void probeBackend()
    {
        if (!mProbeDir.isValid()) {
            // Not a backend capability, so not a skip: the harness cannot do its own setup.
            QFAIL("Could not create a temporary directory for the backend probe.");
        }

        const QString path = qsl("%1/probe.wav").arg(mProbeDir.path());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            QFAIL("Could not write the backend probe clip.");
        }
        file.write(wavBytes());
        file.close();

        QMediaPlayer probe;
        auto* output = new QAudioOutput(&probe);
        output->setMuted(true);
        probe.setAudioOutput(output);

        bool sawEndOfMedia = false;
        bool stoppedCameFirst = false;
        bool sawPlaying = false;
        connect(&probe, &QMediaPlayer::mediaStatusChanged, this, [&](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia) {
                sawEndOfMedia = true;
            }
        });
        connect(&probe, &QMediaPlayer::playbackStateChanged, this, [&](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::PlayingState) {
                sawPlaying = true;
            }
            if (state == QMediaPlayer::StoppedState && !sawEndOfMedia) {
                stoppedCameFirst = true;
            }
        });

        probe.setSource(QUrl::fromLocalFile(path));
        probe.play();
        // Whether a player that has just been handed a source still reads as stopped is
        // the whole reason the deferred cleanup needs to check who owns the player.
        const bool startsSynchronously = probe.playbackState() == QMediaPlayer::PlayingState;

        const bool finished = QTest::qWaitFor(
                [&]() {
                    return sawEndOfMedia;
                },
                QDeadlineTimer(10s));
        probe.stop();

        // Both recorded before the decode verdict below, because the tests that need them do
        // not need the backend to finish a clip - an early return here would leave them
        // believing this backend starts playback and loads asynchronously when it does neither.
        if (startsSynchronously) {
            mSynchronousStartReason = qsl("This Qt Multimedia backend reaches PlayingState synchronously, so a player that has just been claimed by another track never reads as stopped and cannot "
                                          "have its source cleared out from under it. Needs a backend that loads asynchronously, such as Qt's FFmpeg one.");
        }

        if (!sawPlaying && !startsSynchronously) {
            // Without this every test that only stops a track - the ones that need nothing else
            // of the backend - would fail its opening "never started playing" assertion rather
            // than skip, which is a red build on any runner without a usable backend.
            mCannotStartReason = qsl("This Qt Multimedia backend never reached PlayingState within 10s, so no playback can be started to act on. Backend: \"%1\", final media status: %2, error: "
                                     "\"%3\".")
                                         .arg(QString::fromLocal8Bit(qgetenv("QT_MEDIA_BACKEND")), QString::number(static_cast<int>(probe.mediaStatus())), probe.errorString());
        }

        if (!finished) {
            // Report what was measured rather than a cause that was not diagnosed - no audio
            // device, a missing codec and a stalled decoder all land here.
            mCannotPlayReason = qsl("This Qt Multimedia backend did not reach EndOfMedia within 10s, so anything that waits for a clip to finish cannot be observed. Backend: \"%1\", reached "
                                    "PlayingState: %2, final media status: %3, error: \"%4\".")
                                        .arg(QString::fromLocal8Bit(qgetenv("QT_MEDIA_BACKEND")),
                                             startsSynchronously ? qsl("yes") : qsl("no"),
                                             QString::number(static_cast<int>(probe.mediaStatus())),
                                             probe.errorString());
            return;
        }

        if (!stoppedCameFirst) {
            mWrongOrderReason = qsl("This Qt Multimedia backend emits EndOfMedia before StoppedState, so the loop restarts before any cleanup runs and issue #9566 cannot occur here. Needs a "
                                    "StoppedState-first backend such as Qt's FFmpeg one.");
        }

        // Only worth asking of a backend that got this far. Assumed, not measured: one that
        // cannot finish a valid clip is taken not to reject an invalid one either.
        probeLoadFailureReporting();
    }

    // Whether an undecodable file is reported as an error at all. A backend that stays silent
    // gives TMedia nothing to act on, so the release it cannot schedule cannot be asserted.
    void probeLoadFailureReporting()
    {
        const QString path = qsl("%1/unplayable.wav").arg(mProbeDir.path());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            QFAIL("Could not write the unplayable probe clip.");
        }
        file.write(QByteArray("not a WAV file, and not decodable as anything else"));
        file.close();

        QMediaPlayer probe;
        auto* output = new QAudioOutput(&probe);
        output->setMuted(true);
        probe.setAudioOutput(output);

        bool sawError = false;
        connect(&probe, &QMediaPlayer::errorOccurred, this, [&](QMediaPlayer::Error error, const QString&) {
            if (error != QMediaPlayer::NoError) {
                sawError = true;
            }
        });

        probe.setSource(QUrl::fromLocalFile(path));
        probe.play();

        const bool reported = QTest::qWaitFor(
                [&]() {
                    return sawError;
                },
                QDeadlineTimer(10s));
        probe.stop();

        if (!reported) {
            mNoLoadErrorReason = qsl("This Qt Multimedia backend does not report an error for an undecodable file within 10s, so there is no failure for TMedia to act on. Backend: \"%1\", final "
                                     "media status: %2.")
                                         .arg(QString::fromLocal8Bit(qgetenv("QT_MEDIA_BACKEND")), QString::number(static_cast<int>(probe.mediaStatus())));
        }
    }

    TMediaData clipData(const QString& fileName) const
    {
        TMediaData data;
        data.setMediaProtocol(TMediaData::MediaProtocolAPI);
        data.setMediaType(TMediaData::MediaTypeMusic);
        data.setMediaInput(TMediaData::MediaInputFile);
        data.setMediaFileName(fileName);
        data.setMediaVolume(1); // audible enough to play, quiet enough not to disturb a desktop run
        return data;
    }

    // TMedia reports a player only while it is actually playing, or still loading - the
    // carve-out that lets a stalled backend look busy, and the observable this turns on.
    bool playing(TMedia* media, const QString& fileName) const
    {
        TMediaData criteria = clipData(fileName);
        return !media->playingMedia(criteria).isEmpty();
    }

    bool waitForPlaying(TMedia* media, const QString& fileName, std::chrono::milliseconds timeout = 10s)
    {
        return QTest::qWaitFor(
                [&]() {
                    return playing(media, fileName);
                },
                QDeadlineTimer(timeout));
    }

    // Stronger than waitForPlaying(): a player that is still loading counts as playing to
    // TMedia, and it is not yet stoppable in the way a started one is - stopping it produces no
    // playback state change, and getMediaPlayer() will not hand it to another track.
    bool waitForPlaybackStarted(TMedia* media, const QString& fileName, std::chrono::milliseconds timeout = 10s)
    {
        return QTest::qWaitFor(
                [&]() {
                    return playing(media, fileName) && media->playersInPlayingState() > 0;
                },
                QDeadlineTimer(timeout));
    }

    // Passes the file-name checks in TMedia::play(), so it reaches the backend and fails
    // there, so the error path is reached the way a real undecodable file would reach it.
    QString writeUnplayableClip(const QString& fileName) const { return writeClip(fileName, QByteArray("not a WAV file, and not decodable as anything else")); }

    // Writes a clip into the profile media directory, returning {} if that fails.
    QString writeClip(const QString& fileName, const QByteArray& contents = wavBytes()) const
    {
        const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mHostname);
        if (!QDir().mkpath(mediaPath)) {
            QTest::qFail("Could not create the profile media directory.", __FILE__, __LINE__);
            return {};
        }

        QFile file(qsl("%1/%2").arg(mediaPath, fileName));
        if (!file.open(QIODevice::WriteOnly)) {
            QTest::qFail("Could not write the test media file.", __FILE__, __LINE__);
            return {};
        }
        file.write(contents);
        file.close();
        return fileName;
    }

    // A silent 16-bit mono PCM WAV. Silence is fine: the tests assert on playback state
    // transitions, not on what is heard.
    static QByteArray wavBytes()
    {
        constexpr int sampleRate = 8000;
        constexpr int bytesPerSample = 2;
        const int dataBytes = sampleRate * bytesPerSample * clipMs / 1000;

        QByteArray wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);

        out.writeRawData("RIFF", 4);
        out << static_cast<quint32>(36 + dataBytes);
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << static_cast<quint32>(16); // PCM header size
        out << static_cast<quint16>(1);  // PCM, uncompressed
        out << static_cast<quint16>(1);  // mono
        out << static_cast<quint32>(sampleRate);
        out << static_cast<quint32>(sampleRate * bytesPerSample); // byte rate
        out << static_cast<quint16>(bytesPerSample);              // block align
        out << static_cast<quint16>(8 * bytesPerSample);          // bits per sample
        out.writeRawData("data", 4);
        out << static_cast<quint32>(dataBytes);
        wav.append(QByteArray(dataBytes, '\0'));

        return wav;
    }

    // Utility function to manually start a profile like a user would do via the
    // GUI
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5s)) {
            QFAIL("Profile took too long to load.");
        }
    }

    // Utility function
    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            qInfo() << "Profile directory does not exist:" << path;
            return;
        }
        dir.removeRecursively();
    }
};

void initializeQRCResourcesForMediaLoop()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "TMediaLoopTest.moc"
QTEST_MAIN(TMediaLoopTest)
