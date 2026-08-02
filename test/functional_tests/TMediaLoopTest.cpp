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

private slots:
    void initTestCase() { initializeQRCResourcesForMediaLoop(); }

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
        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("loop.wav"));
        const QString skipReason = backendSkipReason(fileName);
        if (!skipReason.isEmpty()) {
            QSKIP(qPrintable(skipReason));
        }

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The looping track never started playing.");

        // Span several passes so a single missed restart cannot pass by luck.
        QTest::qWait(clipMs * 4);

        QVERIFY2(playing(media, fileName), "A loops=-1 track stopped after its first pass - the StoppedState cleanup suppressed EndOfMedia and the loop never restarted.");
    }

    // The deferred cleanup must still fire for a genuinely finished track, otherwise
    // the resource release that #9237 added would be lost.
    void test_oneShotTrackIsCleanedUpWhenItFinishes()
    {
        auto* media = startProfileAndGetMedia();
        QVERIFY(media);

        const QString fileName = writeClip(qsl("oneshot.wav"));
        const QString skipReason = backendSkipReason(fileName);
        if (!skipReason.isEmpty()) {
            QSKIP(qPrintable(skipReason));
        }

        TMediaData data = clipData(fileName);
        data.setMediaLoops(TMediaData::MediaLoopsDefault);
        media->playMedia(data);

        QVERIFY2(waitForPlaying(media, fileName), "The one-shot track never started playing.");

        const bool stopped = QTest::qWaitFor(
                [&]() {
                    return !playing(media, fileName);
                },
                QDeadlineTimer(10s));

        QVERIFY2(stopped, "A one-shot track never left the playing set - the deferred cleanup did not run.");
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

    // Returns an empty string when this backend can both finish a clip and end it in
    // the signal order that the bug depends on, otherwise the reason to skip.
    //
    // Two ways the assertions below would otherwise be meaningless:
    //   - A backend that stalls in LoadingMedia (macOS darwin under
    //     QT_QPA_PLATFORM=offscreen) never stops, and TMedia reports a stalled player
    //     as still playing, so the looping assertion would hold on broken code.
    //   - A backend that emits EndOfMedia *before* StoppedState (macOS darwin under
    //     cocoa) restarts the loop before any cleanup can run, so the bug cannot occur
    //     and the looping assertion would again hold on broken code. Only the
    //     StoppedState-first ordering (Qt's FFmpeg backend) can reproduce it.
    QString backendSkipReason(const QString& fileName)
    {
        const QString path = qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileMediaPath, mHostname), fileName);

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

        const bool finished = QTest::qWaitFor(
                [&]() {
                    return sawEndOfMedia;
                },
                QDeadlineTimer(10s));
        probe.stop();

        if (!finished) {
            return qsl("This Qt Multimedia backend cannot decode a clip to completion here (it stalls before EndOfMedia), so media playback behaviour cannot be observed.");
        }
        if (!stoppedCameFirst) {
            return qsl("This Qt Multimedia backend emits EndOfMedia before StoppedState, so the loop restarts before any cleanup runs and issue #9566 cannot occur here. Needs a StoppedState-first "
                       "backend such as Qt's FFmpeg one.");
        }
        return {};
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
        file.write(wav);
        file.close();
        return fileName;
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
