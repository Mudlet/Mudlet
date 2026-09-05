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
// message printer is driven through Host::postMessage() so its prefix
// colouring is pinned as well, and the Lua error printers through the Lua
// interpreter, since they read the buffer off the model to decide whether a
// fresh line is needed first.

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QToolButton>
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

    bool runLua(const QString& script) { return mpHost->getLuaInterpreter()->compileAndExecuteScript(script); }

    int lineContaining(const QString& needle)
    {
        for (int i = buffer().getLastLineNumber(); i >= 0; --i) {
            if (buffer().line(i).contains(needle)) {
                return i;
            }
        }
        return -1;
    }

    // Every Lua error printer writes a "[  LUA  ]" header line and the error
    // text under it, and has to start the header on a fresh line when the
    // console is mid-line - which the set-up here leaves it.
    void expectLuaErrorOnItsOwnLine(const QString& script, const QString& errorText)
    {
        mpHost->mEchoLuaErrors = true;
        mpHost->printToMainConsole(qsl("mid-line"));
        QVERIFY(runLua(script));

        const int line = lineContaining(errorText);
        QVERIFY2(line >= 2, qPrintable(qsl("no line carries '%1'").arg(errorText)));
        QVERIFY2(buffer().line(line - 1).startsWith(qsl("[  LUA  ]")), qPrintable(buffer().line(line - 1)));
        QCOMPARE(buffer().line(line - 2), qsl("mid-line"));
    }

    // The buffer keeps an empty line ready after the last line feed, so what
    // was printed most recently is the last non-empty line.
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

    void cleanup()
    {
        if (!mpHost) {
            return;
        }
        mpHost->mEchoLuaErrors = false;
        mpHost->mLogDir.clear();
        if (mpHost->mainConsoleModel().mLogToLogFile) {
            mpHost->mainConsoleModel().toggleLogging(false);
        }
        // A test that failed mid-line must not glue the next test's text onto its own
        if (!buffer().line(buffer().getLastLineNumber()).isEmpty()) {
            mpHost->printToMainConsole(qsl("\n"));
        }
    }

    void test_plainPrintLandsOnTheMainConsole()
    {
        mpHost->printToMainConsole(qsl("plain text\n"));

        const int line = lastTextLine();
        QVERIFY(line >= 0);
        QCOMPARE(buffer().line(line), qsl("plain text"));
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

    // startLogging() used to check and uncheck the toolbar button itself; the
    // button now follows the model's state whichever way logging is toggled.
    void test_luaStartLoggingDrivesTheLogButton()
    {
        QToolButton* button = mpHost->mpConsole->logButton;
        QVERIFY(!button->isChecked());

        QVERIFY(runLua(qsl("startLogging(true)")));
        QVERIFY(mpHost->mainConsoleModel().mLogToLogFile);
        QVERIFY2(button->isChecked(), "the log button did not follow logging being started from Lua");

        QVERIFY(runLua(qsl("startLogging(false)")));
        QVERIFY(!mpHost->mainConsoleModel().mLogToLogFile);
        QVERIFY2(!button->isChecked(), "the log button did not follow logging being stopped from Lua");
    }

    // A click flips a checkable button before its slot runs, so a start that
    // cannot open its file has to report the state it left behind.
    void test_failedLogStartLeavesTheButtonUnchecked()
    {
        QFile blocker(qsl("%1/not-a-directory").arg(mConfigDir.path()));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        mpHost->mLogDir = qsl("%1/logs").arg(blocker.fileName());

        QToolButton* button = mpHost->mpConsole->logButton;
        QVERIFY(!button->isChecked());
        button->click();

        QVERIFY(!mpHost->mainConsoleModel().mLogToLogFile);
        QVERIFY2(!button->isChecked(), "the log button stayed checked although no log was started");
    }

    void test_luaErrorsArePrintedWithTheirColours()
    {
        mpHost->mEchoLuaErrors = true;
        QVERIFY(runLua(qsl("printError('hcpt tinted error')")));

        const int line = lineContaining(qsl("hcpt tinted error"));
        QVERIFY(line >= 1);
        QCOMPARE(buffer().buffer.at(line).front().foreground(), QColor(200, 50, 42));
        const QString header = buffer().line(line - 1);
        QVERIFY2(header.startsWith(qsl("[  LUA  ] - ERROR: ")), qPrintable(header));
        QCOMPARE(buffer().buffer.at(line - 1).front().foreground(), QColor(80, 160, 255));
    }

    void test_printErrorStartsOnAFreshLine() { expectLuaErrorOnItsOwnLine(qsl("printError('hcpt errorc boom')"), qsl("hcpt errorc boom")); }

    void test_scriptErrorStartsOnAFreshLine() { expectLuaErrorOnItsOwnLine(qsl("tempAlias('^hcpt-err$', [[error('hcpt alias boom')]])\nexpandAlias('hcpt-err', false)"), qsl("hcpt alias boom")); }

    void test_handlerErrorStartsOnAFreshLine() { expectLuaErrorOnItsOwnLine(qsl("showHandlerError('hcpt-event', 'hcpt handler boom')"), qsl("hcpt handler boom")); }

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
};

#include "HostConsolePrintTest.moc"
MUDLET_GROUPED_TEST_MAIN(HostConsolePrintTest)
