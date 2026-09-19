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
 * Tests the replay toolbar's Pause/Resume and Stop buttons - issue #603.
 *
 * Until these existed a replay could only be got rid of by letting it run to
 * the end or by closing the profile: replayStart() built a toolbar of Faster,
 * Slower and two labels, and replayOver() was reached from the end of the file,
 * a corrupt-file abort and ~cTelnet(), but from nothing the user could press.
 *
 * The tests drive the QActions by the object name replayStart() puts on each of
 * them rather than calling cTelnet directly, because a pause that works
 * perfectly but is wired to nothing is the failure worth guarding against - and
 * severing only that wiring does turn them red.
 *
 * The ones that bite hardest:
 *   - resumeWaitsOnlyTheRemainderOfTheHeldGap() covers the remaining-time
 *     banking. Asserting only that a chunk eventually arrives cannot tell a
 *     100ms wait from a 4000ms one, so this one times it.
 *   - theToolbarDrivesTheReplayingProfileNotTheActiveOne() is the reason
 *     replayStart() takes a Host*. With one profile an implementation that used
 *     getActiveHost() would pass everything else here.
 *   - stopLeavesTheReplaySystemFree(): the replay file and the toolbar are
 *     global to the application rather than per profile, so a Stop that failed
 *     to release them would block every later replay in every profile.
 *
 * Run with: ctest -R ReplayPlaybackControlTest -V
 */

#include <QAction>
#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QTemporaryDir>
#include <QToolBar>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ReplayPlaybackControlTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mReplayDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    // Only the last test makes a second profile, to prove the toolbar reaches
    // the profile that started the replay rather than the one in front.
    Host* mpOtherHost = nullptr;
    const QString mHostname = qsl("ReplayControlTest-Host");
    const QString mOtherHostname = qsl("ReplayControlTest-Other");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // Long enough that a chunk cannot arrive by accident inside a pause, short
    // enough to keep the test a few seconds rather than a few minutes.
    static constexpr int scmChunkGapMsec = 600;
    // The gap the timing test holds a pause inside. It has to be long enough
    // that banking the remainder and restarting the whole wait are far apart.
    static constexpr int scmLongGapMsec = 4000;

    struct Chunk
    {
        qint32 gapMsec = 0;
        QByteArray bytes;
    };

    // Writes a replay in the format cTelnet::processSocketData() records: per
    // chunk a qint32 of milliseconds since the previous one, a qint32 length,
    // then that many raw bytes. The stream version is the one every Mudlet file
    // is pinned to.
    QString writeReplay(const QString& fileName, const QVector<Chunk>& chunks)
    {
        const QString path = qsl("%1/%2").arg(mReplayDir.path(), fileName);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            QWARN(qPrintable(qsl("could not write the replay fixture \"%1\": %2").arg(path, file.errorString())));
            return QString();
        }
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_5_12);
        for (const Chunk& chunk : chunks) {
            stream << chunk.gapMsec;
            stream << static_cast<qint32>(chunk.bytes.size());
            stream.writeRawData(chunk.bytes.constData(), static_cast<int>(chunk.bytes.size()));
        }
        file.close();
        // A short write makes a replay the reader calls corrupt, which would
        // otherwise surface much later as a baffling timing failure:
        if (stream.status() != QDataStream::Ok) {
            QWARN(qPrintable(qsl("the replay fixture \"%1\" was written short").arg(path)));
            return QString();
        }
        return path;
    }

    QString writeThreeChunkReplay(const QString& fileName)
    {
        return writeReplay(fileName, {{20, QByteArrayLiteral("REPLAY_ONE\r\n")}, {scmChunkGapMsec, QByteArrayLiteral("REPLAY_TWO\r\n")}, {scmChunkGapMsec, QByteArrayLiteral("REPLAY_THREE\r\n")}});
    }

    // The console lines carrying the given text, so a chunk that got played
    // twice shows up as two entries rather than merging into one.
    QStringList linesContaining(const QString& text, const Host* pHost = nullptr) const
    {
        QStringList lines;
        TMainConsole* console = (pHost ? pHost : mpHost)->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i).contains(text)) {
                lines << console->buffer.line(i);
            }
        }
        return lines;
    }

    bool bufferContains(const QString& text, const Host* pHost = nullptr) const { return !linesContaining(text, pHost).isEmpty(); }

    // The toolbar's controls as a user reaches them: by the object name
    // replayStart() puts on the action, not by a pointer the test was handed.
    static QAction* replayAction(const QString& objectName) { return mudlet::self()->findChild<QAction*>(objectName); }

    static QLabel* replayTimeLabel() { return mudlet::self()->findChild<QLabel*>(qsl("replay_time_label")); }

    // replayStart() does not name the toolbar, so it is reached through a
    // button it carries rather than looked up.
    static QToolBar* replayToolBar()
    {
        QAction* pause = replayAction(qsl("replay_pause_action"));
        if (!pause) {
            return nullptr;
        }
        for (QObject* object : pause->associatedObjects()) {
            if (auto* toolBar = qobject_cast<QToolBar*>(object)) {
                return toolBar;
            }
        }
        return nullptr;
    }

    // Opens a profile the way a user would, through the connection dialog. The
    // replay path never touches the socket, but the profile is not usable until
    // it has settled, and the suite's other tests wait the same way.
    Host* startProfile(const QString& hostname)
    {
        Host* pHost = TestProfile::create(hostname, mLocalhost, mPort);
        if (!pHost) {
            return nullptr;
        }
        QSignalSpy connected(&(pHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            return nullptr;
        }
        return pHost;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - see cTelnetBufferTest for why
        // sharing the developer's one breaks profile creation.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mReplayDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        for (const QString& name : {mHostname, mOtherHostname}) {
            QDir(MudletPaths::getMudletPath(enums::profileHomePath, name)).removeRecursively();
        }

        mpHost = startProfile(mHostname);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        mpHost->mpConsole->buffer.clear();
        // The replay file and the toolbar are one per application, so a test
        // that left one running would decide the next one's result.
        QVERIFY(!mpHost->mTelnet.isReplaying());
        QVERIFY(!replayAction(qsl("replay_pause_action")));
    }

    void cleanup()
    {
        // Leave nothing running even if an assertion above bailed out early.
        mpHost->mTelnet.stopReplay();
        if (mpOtherHost) {
            mpOtherHost->mTelnet.stopReplay();
        }
        QTRY_VERIFY(!replayAction(qsl("replay_stop_action")));
    }

    // The buttons exist at all, and they are on the replay toolbar rather than
    // merely parented to the main window where no user could reach them.
    void toolbarCarriesPauseAndStopButtons()
    {
        const QString file = writeThreeChunkReplay(qsl("buttons.dat"));
        QVERIFY(!file.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, file));

        QAction* pause = replayAction(qsl("replay_pause_action"));
        QAction* stop = replayAction(qsl("replay_stop_action"));
        QVERIFY(pause);
        QVERIFY(stop);
        QVERIFY(pause->isCheckable());
        QVERIFY(!pause->isChecked());
        QToolBar* toolBar = replayToolBar();
        QVERIFY(toolBar);
        QVERIFY(toolBar->actions().contains(pause));
        QVERIFY(toolBar->actions().contains(stop));
        // The pre-existing speed controls survived the two new buttons being
        // inserted ahead of them.
        QVERIFY(toolBar->actions().contains(replayAction(qsl("replay_speed_up_action"))));
        QVERIFY(toolBar->actions().contains(replayAction(qsl("replay_speed_down_action"))));
    }

    // A paused replay plays nothing more until it is resumed, and then carries
    // on from where it was rather than starting over.
    void pauseHoldsPlaybackAndResumeCarriesOn()
    {
        const QString file = writeThreeChunkReplay(qsl("pause.dat"));
        QVERIFY(!file.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, file));

        QTRY_VERIFY(bufferContains(qsl("REPLAY_ONE")));

        QAction* pause = replayAction(qsl("replay_pause_action"));
        QVERIFY(pause);
        // What was on screen when the pause landed, rather than an assumption
        // that the next chunk cannot already have played on a slow machine.
        const int playedWhenPaused = linesContaining(qsl("REPLAY_TWO")).size();
        pause->trigger();

        QVERIFY(pause->isChecked());
        QCOMPARE(pause->text(), qsl("Resume"));
        QVERIFY(mpHost->mTelnet.replayPaused());
        QVERIFY2(replayTimeLabel() && replayTimeLabel()->text().contains(qsl("(paused)")),
                 qPrintable(qsl("the time readout does not say the replay is held: \"%1\"").arg(replayTimeLabel() ? replayTimeLabel()->text() : qsl("<no label>"))));

        // Well past the gap the file asks for before the next chunk.
        QTest::qWait(scmChunkGapMsec * 3);
        QCOMPARE(linesContaining(qsl("REPLAY_TWO")).size(), playedWhenPaused);
        QVERIFY(mpHost->mTelnet.isReplaying());

        pause->trigger();
        QVERIFY(!pause->isChecked());
        QCOMPARE(pause->text(), qsl("Pause"));
        QVERIFY(!mpHost->mTelnet.replayPaused());
        QVERIFY(replayTimeLabel() && !replayTimeLabel()->text().contains(qsl("(paused)")));

        QTRY_VERIFY(bufferContains(qsl("REPLAY_TWO")));
        // Resuming fires the held chunk, it does not also re-arm the timer over
        // it and play it again.
        QCOMPARE(linesContaining(qsl("REPLAY_TWO")).size(), 1);
        QTRY_VERIFY(bufferContains(qsl("REPLAY_THREE")));
    }

    // Resuming waits out what was left of the interrupted gap, not the whole of
    // it again. Nothing else here can tell those apart: both end with the chunk
    // arriving, they just differ by seconds.
    void resumeWaitsOnlyTheRemainderOfTheHeldGap()
    {
        const QString file = writeReplay(qsl("remainder.dat"), {{20, QByteArrayLiteral("REPLAY_ONE\r\n")}, {scmLongGapMsec, QByteArrayLiteral("REPLAY_TWO\r\n")}});
        QVERIFY(!file.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, file));

        QTRY_VERIFY(bufferContains(qsl("REPLAY_ONE")));

        // Half way into the long gap, so roughly half of it is still owed.
        QTest::qWait(scmLongGapMsec / 2);
        QAction* pause = replayAction(qsl("replay_pause_action"));
        QVERIFY(pause);
        QVERIFY2(!bufferContains(qsl("REPLAY_TWO")), "the machine stalled for over half the gap, so there was no wait left to bank");
        pause->trigger();

        QTest::qWait(scmChunkGapMsec);
        QVERIFY(!bufferContains(qsl("REPLAY_TWO")));

        QElapsedTimer sinceResume;
        sinceResume.start();
        pause->trigger();
        // Owed about half the gap. Restarting the whole wait instead would take
        // the full gap, so anything under three quarters of it discriminates.
        QTRY_VERIFY_WITH_TIMEOUT(bufferContains(qsl("REPLAY_TWO")), scmLongGapMsec * 2);
        QVERIFY2(sinceResume.elapsed() < (scmLongGapMsec * 3) / 4,
                 qPrintable(qsl("resuming waited %1ms of a %2ms gap, so the remainder was not banked").arg(sinceResume.elapsed()).arg(scmLongGapMsec)));
    }

    // Stop ends the replay there and then - the rest of the file is not played,
    // and the toolbar it was driving goes away.
    void stopEndsTheReplayAndTakesTheToolbarDown()
    {
        const QString file = writeThreeChunkReplay(qsl("stop.dat"));
        QVERIFY(!file.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, file));

        QTRY_VERIFY(bufferContains(qsl("REPLAY_ONE")));

        QAction* stop = replayAction(qsl("replay_stop_action"));
        QVERIFY(stop);
        stop->trigger();

        // The two chunks still in the file stay unplayed.
        QTest::qWait(scmChunkGapMsec * 3);
        QVERIFY2(!bufferContains(qsl("REPLAY_TWO")), qPrintable(qsl("the replay kept playing past Stop: %1").arg(linesContaining(qsl("REPLAY_")).join(qsl(" | ")))));
        QVERIFY(!bufferContains(qsl("REPLAY_THREE")));

        QVERIFY(!mpHost->mTelnet.isReplaying());
        QVERIFY(bufferContains(qsl("The replay has been stopped")));
        QTRY_VERIFY(!replayAction(qsl("replay_pause_action")));
        QTRY_VERIFY(!replayAction(qsl("replay_stop_action")));
    }

    // Stopping while paused has to unwind the pause too, and either way the
    // replay file and toolbar - both application-wide - must be free for the
    // next replay to claim.
    void stopLeavesTheReplaySystemFree()
    {
        const QString first = writeThreeChunkReplay(qsl("first.dat"));
        QVERIFY(!first.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, first));
        QTRY_VERIFY(bufferContains(qsl("REPLAY_ONE")));

        QAction* pause = replayAction(qsl("replay_pause_action"));
        QVERIFY(pause);
        pause->trigger();
        QVERIFY(mpHost->mTelnet.replayPaused());
        QAction* stop = replayAction(qsl("replay_stop_action"));
        QVERIFY(stop);
        stop->trigger();

        QVERIFY(!mpHost->mTelnet.isReplaying());
        QVERIFY(!mpHost->mTelnet.replayPaused());
        QTRY_VERIFY(!replayAction(qsl("replay_pause_action")));

        mpHost->mpConsole->buffer.clear();
        const QString second = writeReplay(qsl("second.dat"), {{20, QByteArrayLiteral("REPLAY_AGAIN\r\n")}});
        QVERIFY(!second.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, second));
        QTRY_VERIFY(bufferContains(qsl("REPLAY_AGAIN")));
        // A one-chunk replay ends on its own, which reaches replayOver() from
        // loadReplayChunk() rather than from stopReplay() and has to leave the
        // same clean state behind.
        QTRY_VERIFY(!mpHost->mTelnet.isReplaying());
        QVERIFY(bufferContains(qsl("The replay has ended")));

        // Stopping a replay that already finished says nothing and breaks
        // nothing - cleanup() does this after every test.
        mpHost->mTelnet.stopReplay();
        QVERIFY2(!bufferContains(qsl("The replay has been stopped")), "stopping a finished replay posted a message anyway");
    }

    // A replay lua started still gets the toolbar, so Stop is a thing the user
    // can press on it - and unlike the end of the file, which stays quiet for
    // lua's sake, a button the user pressed has to report what it did.
    void stopReportsOnAReplayLuaStarted()
    {
        const QString file = writeThreeChunkReplay(qsl("lua.dat"));
        QVERIFY(!file.isEmpty());
        QString errMsg;
        // A non-null error string is what marks a replay as lua's, and it is
        // also what suppresses the loading notice - so its absence below is
        // what proves this really took the lua path.
        QVERIFY(mudlet::self()->loadReplay(mpHost, file, &errMsg));
        QVERIFY2(errMsg.isEmpty(), qPrintable(errMsg));
        QTRY_VERIFY(bufferContains(qsl("REPLAY_ONE")));
        QVERIFY(!bufferContains(qsl("Loading replay file")));

        QAction* stop = replayAction(qsl("replay_stop_action"));
        QVERIFY(stop);
        stop->trigger();

        QVERIFY(!mpHost->mTelnet.isReplaying());
        QVERIFY2(bufferContains(qsl("The replay has been stopped")), "a lua-started replay stopped by the user said nothing at all");
    }

    // The reason replayStart() takes a Host*: the buttons have to drive the
    // profile that started the replay, not whichever one happens to be in
    // front. With a single profile those are the same and nothing is proven.
    void theToolbarDrivesTheReplayingProfileNotTheActiveOne()
    {
        const QString file = writeThreeChunkReplay(qsl("twoprofiles.dat"));
        QVERIFY(!file.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, file));
        QTRY_VERIFY(bufferContains(qsl("REPLAY_ONE")));

        mpOtherHost = startProfile(mOtherHostname);
        QVERIFY(mpOtherHost);
        QVERIFY(mpOtherHost != mpHost);
        // The replay is still the first profile's, but the second is in front.
        QCOMPARE(mudlet::self()->getActiveHost(), mpOtherHost);
        QVERIFY(mpHost->mTelnet.isReplaying());
        QVERIFY(!mpOtherHost->mTelnet.isReplaying());

        QAction* pause = replayAction(qsl("replay_pause_action"));
        QVERIFY(pause);
        pause->trigger();
        QVERIFY2(mpHost->mTelnet.replayPaused(), "Pause went to the profile in front rather than the one replaying");

        pause->trigger();
        QVERIFY(!mpHost->mTelnet.replayPaused());

        QAction* stop = replayAction(qsl("replay_stop_action"));
        QVERIFY(stop);
        stop->trigger();
        QVERIFY2(!mpHost->mTelnet.isReplaying(), "Stop went to the profile in front, leaving the replay running and the toolbar stuck up");
        QTRY_VERIFY(!replayAction(qsl("replay_stop_action")));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        mpOtherHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            for (const QString& name : {mHostname, mOtherHostname}) {
                QDir(MudletPaths::getMudletPath(enums::profileHomePath, name)).removeRecursively();
            }
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }
};

#include "ReplayPlaybackControlTest.moc"
MUDLET_GROUPED_TEST_MAIN(ReplayPlaybackControlTest)
