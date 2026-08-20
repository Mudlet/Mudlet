/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

/*
 * TMedia::signal_hideVideoOutput -> TMainConsole::hideVideoOutput, the wire that takes the
 * label a finished video was drawn into back off screen. Cut it and the last frame of every
 * video that asked to close stays over whatever that label was showing before.
 *
 * The slot hides the video widget's *parent* - the label - and never touches the widget
 * itself, which is what lets Media_spec.lua see the far end through windowVisible(). What that
 * cannot see is the widget: whether one was ever attached, and whether it is the label rather
 * than the widget that goes. This file holds both, and covers the two emit sites a spec cannot
 * reach at all under a leak-checked run, where Media_spec's video block skips itself.
 *
 * Covers the emits in releaseMediaSourceAfterEvents() (a stop) and stopAllMediaPlayers() (a
 * cache purge). The third, in play()'s setupVideo() failure path, is not reached from here.
 *
 * Run with: ctest -R VideoOutputHideTest -V
 */

#include <QMediaPlayer>
#include <QPointer>
#include <QTemporaryDir>
#include <QVideoWidget>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TMedia.h"
#include "TelnetServerStub.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// A backend that will not hold a source leaves nothing for a stop to end, and so no ending for
// the wire to report. Runners known to carry a working one set
// MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK, which turns the skip into a failure so that losing the
// backend is a red build rather than a file that goes green meaning nothing - the same bargain
// TMediaLoopTest strikes.
//
// Expand only in a test slot: QSKIP and QFAIL are both a bare return, so from a helper this
// would return from the helper and let the body run against a backend just declared incapable.
#define SKIP_OR_FAIL_WITHOUT(reason)                                                                                                                                                                   \
    do {                                                                                                                                                                                               \
        const QString incapable = (reason);                                                                                                                                                            \
        if (!incapable.isEmpty()) {                                                                                                                                                                    \
            if (qEnvironmentVariableIsSet("MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK")) {                                                                                                                    \
                QFAIL(qPrintable(qsl("MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK is set for this runner, so a backend that cannot do this is a failure and not a skip: %1").arg(incapable)));                 \
            }                                                                                                                                                                                          \
            QSKIP(qPrintable(incapable));                                                                                                                                                              \
        }                                                                                                                                                                                              \
    } while (false)

class VideoOutputHideTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;
    bool mRedirectedConfigDir = false;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("VideoOutputHide-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mClipName = qsl("videoseam.wav");
    // Set when this environment's Qt Multimedia will not even hold a source.
    QString mNoBackendReason;

    // A silent 8 bit mono WAV. Played as a video request, which is what decides the widget
    // setup this is about - a decodable picture would only change what gets drawn into it.
    // Ten seconds of it, so a stop lands on a track that is still going.
    static QByteArray wavBytes()
    {
        constexpr int sampleRate = 8000;
        constexpr int dataBytes = sampleRate * 10;

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
        out << static_cast<quint32>(sampleRate); // byte rate
        out << static_cast<quint16>(1);          // block align
        out << static_cast<quint16>(8);          // bits per sample
        out.writeRawData("data", 4);
        out << static_cast<quint32>(dataBytes);
        wav.append(QByteArray(dataBytes, static_cast<char>(128))); // 128 is silence, unsigned

        return wav;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so the profile opened below is never one of the
        // developer's. Since #9712 the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        mRedirectedConfigDir = true;
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        QString setupFailure;
        QVERIFY2(probeBackend(setupFailure), qPrintable(setupFailure));

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        QVERIFY2(mpHost, "the test profile did not load");
        QVERIFY2(mpHost->mpConsole, "the profile loaded without a main console, so none of the video seam exists");
        QVERIFY2(mpHost->mpMedia, "the profile loaded without a TMedia, so there is no emitter");
        QVERIFY2(writeClip(setupFailure), qPrintable(setupFailure));
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        // Only if this process is the one that redirected it: an initTestCase that skipped
        // before the qputenv never captured a value to put back.
        if (mRedirectedConfigDir) {
            mSavedXdgConfigHome.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
        }
    }

    // The wire itself, through the emit in TMedia::releaseMediaSourceAfterEvents(). A video
    // played into a label with close = true has asked for that label back when the track ends,
    // and stopping it is what ends it.
    void test_stoppingAVideoAskedToCloseTakesItsLabelOffScreen()
    {
        SKIP_OR_FAIL_WITHOUT(mNoBackendReason);

        const QString labelName = qsl("videoOutputHideLabel");
        TLabel* pLabel = nullptr;
        QVERIFY(createVideoLabel(labelName, pLabel));

        QVERIFY(playVideo(labelName, true));

        QPointer<QVideoWidget> pVideoWidget = attachedVideoWidget(pLabel);
        QVERIFY2(pVideoWidget, "no video widget was attached to the label, so TMainConsole::setupVideoOutput never ran and there is nothing on the label for the stop to take away");
        QVERIFY2(pLabel->isVisible(), "the label was not on screen while the video played, so hiding it again could not be observed and this test would pass severed");
        QVERIFY2(pVideoWidget->isVisible(), "the video widget was not on screen while the video played, so this test would pass severed");

        QVERIFY(stopVideos());

        // playersHoldingSource() drops to 0 in releaseSource(), the statement immediately
        // before the emit, so this is the ending having reached the gate rather than a guess at
        // how long the deferred turn takes.
        QVERIFY2(waitForEndingToComplete(), "the stop never completed, so the wire was never reached and neither outcome below means anything");

        QVERIFY2(!pLabel->isVisible(),
                 "A stopped video left its label on screen. TMedia::signal_hideVideoOutput no longer reaches TMainConsole::hideVideoOutput, so the last frame of every video that asked to close "
                 "stays over whatever its label was showing.");
        // Deliberately separate: the slot hides the parent, and the widget only follows because
        // it is a child. A slot rewritten to hide the widget instead would leave the label up.
        QVERIFY2(pVideoWidget, "the video widget was destroyed rather than hidden with its label");
        QVERIFY2(!pVideoWidget->isVisible(), "the label went off screen but its video widget did not follow");
    }

    // The other side of the same gate: close is off by default, and a video that did not ask
    // for it must leave its label alone. Without this, a hideVideoOutput() that fired for every
    // ending - or a label taken off screen by something else entirely - would satisfy the test
    // above just as well. mediaClose() is the only discriminating half of the condition at
    // TMedia.cpp:1690, since mMediaWidget defaults to MediaWidgetLabel, so this guards it alone.
    void test_stoppingAVideoThatDidNotAskToCloseLeavesTheLabelAlone()
    {
        SKIP_OR_FAIL_WITHOUT(mNoBackendReason);

        const QString labelName = qsl("videoOutputKeepLabel");
        TLabel* pLabel = nullptr;
        QVERIFY(createVideoLabel(labelName, pLabel));

        QVERIFY(playVideo(labelName, false));

        QPointer<QVideoWidget> pVideoWidget = attachedVideoWidget(pLabel);
        QVERIFY2(pVideoWidget, "no video widget was attached to the label, so this test is not comparing against a video that was really set up");
        QVERIFY2(pLabel->isVisible(), "the label was not on screen while the video played, so it staying on screen afterwards proves nothing");
        QVERIFY2(pVideoWidget->isVisible(), "the video widget was not on screen while the video played, so it staying proves nothing");

        QVERIFY(stopVideos());
        QVERIFY2(waitForEndingToComplete(), "the stop never completed, so the close gate was never evaluated and this test would pass vacuously");

        QVERIFY2(pLabel->isVisible(), "a video that never asked to close still took its label off screen when it stopped");
        QVERIFY2(pVideoWidget && pVideoWidget->isVisible(), "a video that never asked to close still had its picture taken away when it stopped");
    }

    // The second emit site, in TMedia::stopAllMediaPlayers(), which purgeMediaCache() reaches.
    // Not a duplicate of the first: it reads whether there was a video output *before*
    // releaseSource() (TMedia.cpp:650) where the deferred release reads it after (:1690), so a
    // Qt that started clearing the video output with the source would silently stop firing one
    // and not the other. It also ends the playback synchronously rather than a turn later.
    //
    // Runs last because it empties the profile's media directory, which the clip lives in.
    void test_purgingTheMediaCacheTakesAPlayingVideosLabelOffScreen()
    {
        SKIP_OR_FAIL_WITHOUT(mNoBackendReason);

        const QString labelName = qsl("videoOutputPurgeLabel");
        TLabel* pLabel = nullptr;
        QVERIFY(createVideoLabel(labelName, pLabel));

        QVERIFY(playVideo(labelName, true));

        QPointer<QVideoWidget> pVideoWidget = attachedVideoWidget(pLabel);
        QVERIFY2(pVideoWidget, "no video widget was attached to the label, so there is nothing on it for the purge to take away");
        QVERIFY2(pLabel->isVisible(), "the label was not on screen while the video played, so this test would pass severed");

        // Not assert()ed like the other calls: what it returns is whether the media directory
        // was emptied, and on Windows the backend can still hold the clip open at that moment,
        // so a purge that stopped everything correctly still answers false. The stop happens
        // first either way, and the released source is the witness that it did. It also ends
        // every playback outright rather than deferring, so no wait is needed.
        QVERIFY(runLua(qsl("purgeMediaCache()")));
        QCOMPARE(mpHost->mpMedia->playersHoldingSource(), 0);

        QVERIFY2(!pLabel->isVisible(), "a purge stopped a video that had asked to close and left its label on screen - the stopAllMediaPlayers() emit no longer reaches hideVideoOutput()");
    }

private:
    // A plain QMediaPlayer, before any Mudlet object exists, so nothing in the seam under test
    // can influence the verdict. Returns false only for a harness failure, which is not a
    // backend capability and so not a skip.
    bool probeBackend(QString& failure)
    {
        QTemporaryDir probeDir;
        if (!probeDir.isValid()) {
            failure = qsl("could not create a temporary directory for the backend probe");
            return false;
        }

        const QString path = qsl("%1/probe.wav").arg(probeDir.path());
        if (!writeWav(path, failure)) {
            return false;
        }

        QMediaPlayer probe;
        probe.setSource(QUrl::fromLocalFile(path));

        if (probe.source().isEmpty()) {
            mNoBackendReason = qsl("This Qt Multimedia backend did not hold a source that was set on it, so no playback can be started and nothing can be stopped. Backend: \"%1\", error: \"%2\".")
                                       .arg(QString::fromLocal8Bit(qgetenv("QT_MEDIA_BACKEND")), probe.errorString());
        }
        return true;
    }

    // assert() around the call, so a request the API merely refuses comes back as a raised
    // error rather than as a script that ran fine - compileAndExecuteScript() only reports the
    // latter. The same trick TMediaLoopTest::mediaFinishedHolds() uses.
    bool playVideo(const QString& labelName, const bool close)
    {
        const QString request = close ? qsl("assert(playVideoFile({name = '%1', key = '%2', close = true}))").arg(mClipName, labelName)
                                      : qsl("assert(playVideoFile({name = '%1', key = '%2'}))").arg(mClipName, labelName);
        if (!runLua(request)) {
            return false;
        }
        if (mpHost->mpMedia->playersHoldingSource() == 0) {
            QTest::qFail("The video request reached no player holding a source, so there is nothing to stop and nothing to observe.", __FILE__, __LINE__);
            return false;
        }
        return true;
    }

    bool stopVideos() { return runLua(qsl("assert(stopVideos())")); }

    bool waitForEndingToComplete() const
    {
        return QTest::qWaitFor(
                [this]() {
                    return mpHost->mpMedia->playersHoldingSource() == 0;
                },
                QDeadlineTimer(10s));
    }

    // The Lua error text goes to the profile's error console, which no one reads in an
    // offscreen ctest run, so the script itself is the only thing worth reporting back.
    bool runLua(const QString& script) const
    {
        if (!mpHost->getLuaInterpreter()->compileAndExecuteScript(script)) {
            QTest::qFail(qPrintable(qsl("Lua raised an error running: %1").arg(script)), __FILE__, __LINE__);
            return false;
        }
        return true;
    }

    bool createVideoLabel(const QString& name, TLabel*& pLabel) const
    {
        auto [created, message] = mpHost->createLabel(qsl("main"), name, 10, 10, 60, 30, true, false);
        if (!created) {
            QTest::qFail(qPrintable(message), __FILE__, __LINE__);
            return false;
        }
        pLabel = mpHost->mpConsole->mLabelMap.value(name);
        if (!pLabel) {
            QTest::qFail(qPrintable(qsl("createLabel() reported success but no label called '%1' is in the console's label map").arg(name)), __FILE__, __LINE__);
            return false;
        }
        return true;
    }

    // TMainConsole::setupVideoOutput() parents the widget it makes to the label, and reads
    // TLabel::mpVideoWidget - which nothing in the tree ever assigns - to decide whether to
    // make one, so every request adds a fresh child and the newest is the one it set up.
    // The setter beside that read, player->mediaData().setMediaWidget(), writes into a
    // temporary because TMediaPlayer::mediaData() returns by value, so the
    // mediaWidget() == MediaWidgetLabel half of the emit's gate only ever passes because
    // TMediaData defaults it that way. Give mediaData() a reference return, or change that
    // default, and these tests stop covering what they say they cover.
    static QVideoWidget* attachedVideoWidget(TLabel* pLabel)
    {
        const auto widgets = pLabel->findChildren<QVideoWidget*>(QString(), Qt::FindDirectChildrenOnly);
        return widgets.isEmpty() ? nullptr : widgets.constLast();
    }

    bool writeClip(QString& failure) const
    {
        const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mHostname);
        if (!QDir().mkpath(mediaPath)) {
            failure = qsl("could not create the profile media directory %1").arg(mediaPath);
            return false;
        }
        return writeWav(qsl("%1/%2").arg(mediaPath, mClipName), failure);
    }

    // Checked to the byte: a clip that is short or empty still gives a player a source to hold,
    // so a half-written fixture would surface much later as the seam apparently not working.
    static bool writeWav(const QString& path, QString& failure)
    {
        const QByteArray contents = wavBytes();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            failure = qsl("could not open %1 for writing: %2").arg(path, file.errorString());
            return false;
        }
        const bool written = file.write(contents) == contents.size();
        file.close();
        if (!written || file.error() != QFileDevice::NoError) {
            failure = qsl("could not write %1: %2").arg(path, file.errorString());
            return false;
        }
        return true;
    }
};

#include "VideoOutputHideTest.moc"
MUDLET_GROUPED_TEST_MAIN(VideoOutputHideTest)
