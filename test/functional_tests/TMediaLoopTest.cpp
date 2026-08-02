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

/*
 * Regression guard for "Client.Media loops=-1 plays once" (issue #9566).
 *
 * Qt's FFmpeg backend ends a track by emitting StoppedState first and only then
 * EndOfMedia, and it skips the EndOfMedia notification if the playback engine
 * disappeared in between. Mudlet restarts a loop from its EndOfMedia handler, so
 * when the StoppedState handler cleared the source immediately (added by #9237 as
 * a cleanup measure) it destroyed the engine, EndOfMedia never arrived and an
 * indefinitely looping track played exactly once.
 *
 * The cleanup is therefore deferred by one event-loop turn and re-checks the
 * playback state, which lets a loop restart claim the player first. Both halves of
 * that contract are covered here: a looping track must survive the stop/restart
 * cycle, and a one-shot track must still be torn down so #9237 is not regressed.
 *
 * Both assertions depend on the platform actually decoding a clip through to
 * EndOfMedia. Some setups cannot - notably the macOS darwin backend under
 * QT_QPA_PLATFORM=offscreen, which the functional suite sets, stalls in
 * LoadingMedia indefinitely. TMedia reports a stalled player as still playing, so
 * without a capability check the looping assertion would pass on broken code too.
 * Each test therefore probes a plain QMediaPlayer first and skips if the backend
 * cannot finish a clip.
 */
class TMediaLoopTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-Media-Loop";
    const QString mPort = "4012";
    const QString mLocalhost = "localhost";

    // Length of the generated clip. Long enough that "still playing" cannot be an
    // artefact of start-up latency, short enough to loop several times quickly.
    static constexpr int clipMs = 400;

    // Probed once, because the probe has to wait out a whole clip and the suite gives
    // every functional test a single wall-clock budget for all of its slots.
    QTemporaryDir mProbeDir;
    // Set when the backend cannot decode a clip through to EndOfMedia at all.
    QString mCannotPlayReason;
    // Set when the backend ends a track with EndOfMedia before StoppedState.
    QString mWrongOrderReason;
    // Set when the backend starts playing synchronously, so a player that has just been
    // re-sourced can never be mistaken for a stopped one.
    QString mSynchronousStartReason;

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
        if (!mCannotPlayReason.isEmpty()) {
            QSKIP(qPrintable(mCannotPlayReason));
        }
        if (!mWrongOrderReason.isEmpty()) {
            QSKIP(qPrintable(mWrongOrderReason));
        }

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("loop.wav"));

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The looping track never started playing.");

        // Span several passes so a single missed restart cannot pass by luck.
        QTest::qWait(clipMs * 4);

        QVERIFY2(playing(media, fileName), "A loops=-1 track stopped after its first pass - the StoppedState cleanup suppressed EndOfMedia and the loop never restarted.");
    }

    // The deferred cleanup must still fire for a genuinely finished track, otherwise
    // the resource release that #9237 added would be lost. Releasing the source is the
    // only observable effect of the deferred stop cleanup: playingMedia() drops a player
    // the moment it reports StoppedState, well before that cleanup runs.
    void test_oneShotTrackIsCleanedUpWhenItFinishes()
    {
        if (!mCannotPlayReason.isEmpty()) {
            QSKIP(qPrintable(mCannotPlayReason));
        }

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("oneshot.wav"));

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
    }

    // A player that is handed to a different track in the same event-loop turn, as
    // stopMusic() followed by playMusic{} in one script does, must keep the new source.
    // The pending cleanup belongs to the track that stopped, and on this backend the
    // player still reads as stopped while it loads the new one.
    void test_reusedPlayerKeepsTheTrackThatClaimedIt()
    {
        if (!mCannotPlayReason.isEmpty()) {
            QSKIP(qPrintable(mCannotPlayReason));
        }
        if (!mSynchronousStartReason.isEmpty()) {
            QSKIP(qPrintable(mSynchronousStartReason));
        }

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString firstFile = writeClip(qsl("first.wav"));
        const QString secondFile = writeClip(qsl("second.wav"));

        TMediaData first = clipData(firstFile);
        first.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(first);

        QVERIFY2(waitForPlaying(media, firstFile), "The first track never started playing.");

        TMediaData stopFirst = clipData(firstFile);
        media->stopMedia(stopFirst);

        TMediaData second = clipData(secondFile);
        second.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(second);

        QVERIFY2(waitForPlaying(media, secondFile), "The replacement track never started playing.");

        // Past the turn the stopped track's cleanup was scheduled for.
        QTest::qWait(clipMs);

        QVERIFY2(playing(media, secondFile), "The replacement track was cut off - the previous track's deferred cleanup cleared the source out from under it.");
    }

    // continue=false restarts a track by stopping it and re-sourcing the same player
    // inside one call. That player is matched, not claimed, so nothing in the reuse path
    // tells the pending cleanup that the track it belongs to has already been replaced.
    void test_restartedTrackKeepsItsNewSource()
    {
        if (!mCannotPlayReason.isEmpty()) {
            QSKIP(qPrintable(mCannotPlayReason));
        }
        if (!mSynchronousStartReason.isEmpty()) {
            QSKIP(qPrintable(mSynchronousStartReason));
        }

        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("restart.wav"));

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The track never started playing.");

        TMediaData restart = clipData(fileName);
        restart.setMediaLoops(TMediaData::MediaLoopsRepeat);
        restart.setMediaContinue(TMediaData::MediaContinueRestart);
        media->playMedia(restart);

        // Past the turn the stop inside that restart scheduled its cleanup for.
        QTest::qWait(clipMs);

        QVERIFY2(playing(media, fileName), "A restarted track was cut off - the cleanup deferred by its own stop cleared the source it had just been given.");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
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
        return media;
    }

    // Records what this backend is and is not able to demonstrate.
    //
    //   - A backend that stalls in LoadingMedia (macOS darwin under
    //     QT_QPA_PLATFORM=offscreen, which the functional suite sets) never stops, and
    //     TMedia reports a stalled player as still playing, so nothing below is
    //     observable at all.
    //   - A backend that emits EndOfMedia *before* StoppedState (macOS darwin under
    //     cocoa) restarts a loop before any cleanup can run, so issue #9566 cannot occur
    //     and the looping assertion would hold on broken code. Only the
    //     StoppedState-first ordering (Qt's FFmpeg backend) can reproduce it. The other
    //     two tests turn on an explicit stop, so they hold on any backend that plays.
    void probeBackend()
    {
        if (!mProbeDir.isValid()) {
            mCannotPlayReason = qsl("Could not create a temporary directory for the backend probe.");
            return;
        }

        const QString path = qsl("%1/probe.wav").arg(mProbeDir.path());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            mCannotPlayReason = qsl("Could not write the backend probe clip.");
            return;
        }
        file.write(wavBytes());
        file.close();

        QMediaPlayer probe;
        auto* output = new QAudioOutput(&probe);
        output->setMuted(true);
        probe.setAudioOutput(output);

        bool sawEndOfMedia = false;
        bool stoppedCameFirst = false;
        connect(&probe, &QMediaPlayer::mediaStatusChanged, this, [&](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia) {
                sawEndOfMedia = true;
            }
        });
        connect(&probe, &QMediaPlayer::playbackStateChanged, this, [&](QMediaPlayer::PlaybackState state) {
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

        if (!finished) {
            mCannotPlayReason = qsl("This Qt Multimedia backend cannot decode a clip to completion here (it stalls before EndOfMedia), so media playback behaviour cannot be observed.");
            return;
        }
        if (!stoppedCameFirst) {
            mWrongOrderReason = qsl("This Qt Multimedia backend emits EndOfMedia before StoppedState, so the loop restarts before any cleanup runs and issue #9566 cannot occur here. Needs a "
                                    "StoppedState-first backend such as Qt's FFmpeg one.");
        }
        if (startsSynchronously) {
            mSynchronousStartReason = qsl("This Qt Multimedia backend reaches PlayingState synchronously, so a player that has just been claimed by another track never reads as stopped and cannot "
                                          "have its source cleared out from under it. Needs a backend that loads asynchronously, such as Qt's FFmpeg one.");
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

    // TMedia reports a player only while it is actually playing (or still loading),
    // which is the observable this regression turns on.
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

    // Writes a silent 16-bit mono PCM WAV into the profile media directory. Silence is
    // fine: the test asserts on playback state transitions, not on what is heard.
    QString writeClip(const QString& fileName) const
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
        file.write(wavBytes());
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
