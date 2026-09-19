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
 * Slower and two labels, and the only caller of replayOver() was the end of the
 * file. So the tests drive the QActions by the object names the toolbar gives
 * them rather than calling cTelnet directly - a pause that works but is wired
 * to nothing is the failure being guarded against.
 *
 * The discriminating one is pauseHoldsPlaybackAndResumeCarriesOn(): the replay
 * is paused after its first chunk has been played and then left alone for well
 * over the gap the file asks for before the next, which must not arrive until
 * Resume. stopLeavesTheReplaySystemFree() is the other one that bites - the
 * file and the toolbar are global to the application rather than per profile,
 * so a Stop that failed to release them would block every later replay.
 *
 * Run with: ctest -R ReplayPlaybackControlTest -V
 */

#include <QAction>
#include <QDataStream>
#include <QDir>
#include <QFile>
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
    const QString mHostname = qsl("ReplayControlTest-Host");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // Long enough that a chunk cannot arrive by accident inside a pause, short
    // enough to keep the test a few seconds rather than a few minutes.
    static constexpr int scmChunkGapMSec = 600;

    struct Chunk
    {
        qint32 gapMSec = 0;
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
            return QString();
        }
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_5_12);
        for (const Chunk& chunk : chunks) {
            stream << chunk.gapMSec;
            stream << static_cast<qint32>(chunk.bytes.size());
            stream.writeRawData(chunk.bytes.constData(), static_cast<int>(chunk.bytes.size()));
        }
        file.close();
        return path;
    }

    // The three-chunk replay the timing tests use. The first chunk is due
    // almost at once so that a test can get to a known point cheaply; the two
    // after it are a pause apart.
    QString writeThreeChunkReplay(const QString& fileName)
    {
        return writeReplay(fileName, {{20, QByteArrayLiteral("REPLAY_ONE\r\n")}, {scmChunkGapMSec, QByteArrayLiteral("REPLAY_TWO\r\n")}, {scmChunkGapMSec, QByteArrayLiteral("REPLAY_THREE\r\n")}});
    }

    bool bufferContains(const QString& text) const
    {
        TMainConsole* console = mpHost->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

    // The toolbar's buttons as a user reaches them: by the object name
    // replayStart() puts on the action, not by a pointer the test was handed.
    static QAction* replayAction(const QString& objectName) { return mudlet::self()->findChild<QAction*>(objectName); }

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

        const QString path = MudletPaths::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(500)) {
            QFAIL("Could not connect with the host.");
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
        QTRY_VERIFY(!replayAction(qsl("replay_stop_action")));
    }

    // The buttons exist at all, and they are on the replay toolbar rather than
    // somewhere the user cannot reach.
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
        QVERIFY(replayToolBar());
        QVERIFY(replayToolBar()->actions().contains(pause));
        QVERIFY(replayToolBar()->actions().contains(stop));
    }

    // The discriminating test: a paused replay plays nothing more until it is
    // resumed, and then carries on from where it was rather than starting over.
    void pauseHoldsPlaybackAndResumeCarriesOn()
    {
        const QString file = writeThreeChunkReplay(qsl("pause.dat"));
        QVERIFY(!file.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, file));

        QTRY_VERIFY(bufferContains(qsl("REPLAY_ONE")));

        QAction* pause = replayAction(qsl("replay_pause_action"));
        QVERIFY(pause);
        pause->trigger();
        QVERIFY(pause->isChecked());
        QVERIFY(mpHost->mTelnet.replayPaused());

        // Well past the gap the file asks for before the second chunk.
        QTest::qWait(scmChunkGapMSec * 3);
        QVERIFY(!bufferContains(qsl("REPLAY_TWO")));
        QVERIFY(mpHost->mTelnet.isReplaying());

        pause->trigger();
        QVERIFY(!pause->isChecked());
        QVERIFY(!mpHost->mTelnet.replayPaused());

        QTRY_VERIFY(bufferContains(qsl("REPLAY_TWO")));
        // The chunk the pause held is played once, not once per pause.
        QCOMPARE(linesContaining(qsl("REPLAY_TWO")).size(), 1);
        QTRY_VERIFY(bufferContains(qsl("REPLAY_THREE")));
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

        QVERIFY(!mpHost->mTelnet.isReplaying());
        QTRY_VERIFY(bufferContains(qsl("The replay has been stopped")));

        // The two chunks still in the file stay unplayed.
        QTest::qWait(scmChunkGapMSec * 3);
        QVERIFY(!bufferContains(qsl("REPLAY_TWO")));
        QVERIFY(!bufferContains(qsl("REPLAY_THREE")));

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

        replayAction(qsl("replay_pause_action"))->trigger();
        QVERIFY(mpHost->mTelnet.replayPaused());
        replayAction(qsl("replay_stop_action"))->trigger();

        QVERIFY(!mpHost->mTelnet.isReplaying());
        QVERIFY(!mpHost->mTelnet.replayPaused());
        QTRY_VERIFY(!replayAction(qsl("replay_pause_action")));

        mpHost->mpConsole->buffer.clear();
        const QString second = writeReplay(qsl("second.dat"), {{20, QByteArrayLiteral("REPLAY_AGAIN\r\n")}});
        QVERIFY(!second.isEmpty());
        QVERIFY(mudlet::self()->loadReplay(mpHost, second));
        QTRY_VERIFY(bufferContains(qsl("REPLAY_AGAIN")));
        // A one-chunk replay ends on its own, which is the other route into
        // replayOver() and has to leave the same clean state behind.
        QTRY_VERIFY(!mpHost->mTelnet.isReplaying());
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletPaths::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

private:
    // The console lines carrying the given text, so a chunk that got played
    // twice shows up as two entries rather than merging into one pass.
    QStringList linesContaining(const QString& text) const
    {
        QStringList lines;
        TMainConsole* console = mpHost->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i).contains(text)) {
                lines << console->buffer.line(i);
            }
        }
        return lines;
    }
};

#include "ReplayPlaybackControlTest.moc"
MUDLET_GROUPED_TEST_MAIN(ReplayPlaybackControlTest)
