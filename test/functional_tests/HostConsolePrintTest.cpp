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

// Core code prints to the profile's main console through Host rather than
// through the TMainConsole widget. Each Host forwarder is exercised on a live
// profile and its effect read back off the main console's buffer; the telnet
// message printer, the one caller of the coloured print, is driven through
// Host::postMessage() so its prefix colouring is pinned as well.

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <string>
#include <vector>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TConsoleModel.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class HostConsolePrintTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-HostConsolePrint");
    const QString mLocalhost = qsl("localhost");

    TBuffer& buffer() { return mpHost->mainConsoleModel().buffer; }

    // The buffer keeps an empty line ready after the last line feed, so what
    // was printed most recently is the last non-empty line.
    bool bufferContains(const QString& text)
    {
        for (int i = 0; i <= buffer().getLastLineNumber(); ++i) {
            if (buffer().line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

    int lastTextLine()
    {
        for (int i = buffer().getLastLineNumber(); i >= 0; --i) {
            if (!buffer().line(i).isEmpty()) {
                return i;
            }
        }
        return -1;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        // Port 0 asks the OS for an ephemeral port so parallel test runs do
        // not collide on a hardcoded one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");
        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15000), "The test profile never connected to the stub server.");
        }
        QVERIFY(mpHost->mpConsole);
    }

    void cleanupTestCase()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        mpHost = nullptr;
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        QDir(profilePath).removeRecursively();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_plainPrintLandsOnTheMainConsole()
    {
        mpHost->printToMainConsole(qsl("plain text\n"));

        const int line = lastTextLine();
        QVERIFY(line >= 0);
        QCOMPARE(buffer().line(line), qsl("plain text"));
        QCOMPARE(buffer().mCursorY, buffer().size());
    }

    void test_colouredPrintKeepsItsColours()
    {
        const QColor fg(200, 10, 20);
        const QColor bg(10, 20, 200);
        mpHost->printToMainConsole(qsl("tinted\n"), fg, bg);

        const int line = lastTextLine();
        QVERIFY(line >= 0);
        QCOMPARE(buffer().line(line), qsl("tinted"));
        const std::vector<TChar>& chars = buffer().buffer.at(line);
        QCOMPARE(chars.front().foreground(), fg);
        QCOMPARE(chars.front().background(), bg);
        QCOMPARE(buffer().mCursorY, buffer().size());
    }

    void test_systemMessageIsLabelledAndColoured()
    {
        mpHost->printSystemMessage(qsl("careful\n"));

        const int line = lastTextLine();
        QVERIFY(line >= 0);
        const QString text = buffer().line(line);
        QVERIFY2(text.endsWith(qsl("careful")), qPrintable(text));
        QVERIFY2(text != qsl("careful"), "the system message label is missing");
        QCOMPARE(buffer().buffer.at(line).front().foreground(), mpHost->mpConsole->mSystemMessageFgColor);
        QCOMPARE(buffer().mCursorY, buffer().size());
    }

    void test_serverTextRunsThroughTheDisplayPipeline()
    {
        std::string data{"from the game\n"};
        mpHost->printOnDisplay(data, true);

        const int line = lastTextLine();
        QVERIFY(line >= 0);
        QCOMPARE(buffer().line(line), qsl("from the game"));
    }

    // Only showNewLines() advances buffer.mCursorY, so a line appended straight
    // into the buffer leaves it behind until finalize() catches the view up.
    void test_finalizeCatchesTheViewUpWithTheBuffer()
    {
        TBuffer& buf = buffer();
        const QString text = qsl("appended\n");
        buf.append(text, 0, text.size(), TChar(mpHost->mFgColor, mpHost->mBgColor));
        QVERIFY2(buf.mCursorY < buf.size(), "appending alone caught the view up, so finalize() has nothing left to prove");

        mpHost->finalizeMainConsole();

        QCOMPARE(buf.mCursorY, buf.size());
    }

    void test_timeStampsAreReadOffTheMainConsole()
    {
        QVERIFY(!mpHost->mainConsoleShowsTimeStamps());

        mpHost->mpConsole->slot_toggleTimeStamps(true);
        QVERIFY(mpHost->mainConsoleShowsTimeStamps());

        mpHost->mpConsole->slot_toggleTimeStamps(false);
        QVERIFY(!mpHost->mainConsoleShowsTimeStamps());
    }

    // cTelnet::postMessage() is what every "[ INFO ]  - ..." line goes through,
    // and it colours the prefix and the text after it separately.
    void test_telnetMessagesArePrintedWithTheirPrefixColours()
    {
        mpHost->postMessage(qsl("[ INFO ]  - hello from the test"));

        const int line = lastTextLine();
        QVERIFY(line >= 0);
        QCOMPARE(buffer().line(line), qsl("[ INFO ]  - hello from the test"));
        const std::vector<TChar>& chars = buffer().buffer.at(line);
        QCOMPARE(chars.front().foreground(), QColor(0, 150, 190));
        QCOMPARE(chars.back().foreground(), QColor(0, 160, 0));
    }

    // Recording is only reachable from the console's button and the menu, so
    // the file is read back here rather than replayed: the replay timer would
    // need MUDLET_TEST_MODE, and the bytes are what a user loses if the stream
    // is never wired to the file.
    void test_replayRecordingWritesWhatTheGameSent()
    {
        cTelnet& telnet = mpHost->mTelnet;
        mpHost->mpConsole->slot_toggleReplayRecording();
        QVERIFY(telnet.recordingReplay());
        const QString fileName = telnet.replayRecordingFileName();
        QVERIFY(!fileName.isEmpty());

        const QByteArray sent{"recorded line\r\n"};
        mpServer->sendRaw(sent);
        QTRY_VERIFY(bufferContains(qsl("recorded line")));

        mpHost->mpConsole->slot_toggleReplayRecording();
        QVERIFY(!telnet.recordingReplay());
        QVERIFY2(QFileInfo::exists(fileName), qPrintable(fileName));

        QFile replay(fileName);
        QVERIFY(replay.open(QIODevice::ReadOnly));
        QDataStream in(&replay);
        in.setVersion(QDataStream::Qt_5_12);
        QByteArray recorded;
        while (!in.atEnd()) {
            qint32 interval = 0;
            qint32 length = 0;
            in >> interval >> length;
            QVERIFY(length >= 0);
            QByteArray chunk(length, '\0');
            QCOMPARE(in.readRawData(chunk.data(), length), length);
            recorded += chunk;
        }
        QVERIFY2(recorded.contains(sent), recorded.constData());
    }

    void test_replayRecordingRefusesAnUnwritablePath()
    {
        cTelnet& telnet = mpHost->mTelnet;
        QVERIFY(!telnet.startReplayRecording(qsl("/nonexistent/directory/replay.dat")));
        QVERIFY(!telnet.recordingReplay());
        QVERIFY(!telnet.replayRecordingErrorString().isEmpty());
    }
};

#include "HostConsolePrintTest.moc"
MUDLET_GROUPED_TEST_MAIN(HostConsolePrintTest)
