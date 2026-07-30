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

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

// Logging defers each received line (TBuffer::lastTextToLog) so a trigger can
// still gag it with deleteLine(). Stopping a log flushes that pending text via
// logRemainingOutput() - but the pending state must be reset afterwards, or
// restarting logging replays the previous session's last line into the new log
// through log()'s deferred-flush path, duplicating it.
class LogRestartDuplicateLineTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-LogRestartDuplicate";
    QString mPort; // assigned the stub's actual loopback port in init()
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Port 0 asks the OS for an ephemeral port so parallel test runs do
        // not collide on a hardcoded one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // Stopping a log flushes the pending line; restarting must not write that
    // same line a second time into the new session.
    void test_restartDoesNotDuplicateLastLine()
    {
        auto* host = startLoggingProfile();

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("feedTelnet('Session one final line.\\n')"));
        QVERIFY2(bufferContains(qsl("Session one final line.")), "Fed line did not reach the console buffer");

        // Stop logging - this flushes the pending line into the log
        host->mpConsole->toggleLogging(false);
        QVERIFY(!host->mpConsole->mLogToLogFile);

        // Restart logging - same fixed file name, so the same file is appended to
        host->mpConsole->toggleLogging(false);
        QVERIFY(host->mpConsole->mLogToLogFile);

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("feedTelnet('Session two line.\\n')"));

        const QString log = stopLoggingAndReadLog(host);
        QCOMPARE(static_cast<int>(log.count(qsl("Session one final line."))), 1);
        QVERIFY2(log.contains(qsl("Session two line.")), "Line of the second logging session is missing from the log");
    }

    // The behaviour #9429 fixed must be preserved: a line gagged by a trigger's
    // deleteLine() stays out of the log while its neighbours are still logged.
    void test_gaggedLineStaysOutOfLog()
    {
        auto* host = startLoggingProfile();

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("tempRegexTrigger('^Top secret plans$', [[deleteLine()]])"));
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("feedTelnet('Before the gag.\\n')"));
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("feedTelnet('Top secret plans\\n')"));
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("feedTelnet('After the gag.\\n')"));

        const QString log = stopLoggingAndReadLog(host);
        QVERIFY2(log.contains(qsl("Before the gag.")), "Line before the gagged one is missing from the log");
        QVERIFY2(!log.contains(qsl("Top secret plans")), "Gagged line leaked into the log");
        QVERIFY2(log.contains(qsl("After the gag.")), "Line after the gagged one is missing from the log");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

private:
    // Starts a profile, takes it offline (feedTelnet() requires that) and turns
    // on plain-text logging to a known file name.
    Host* startLoggingProfile()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto* host = mudlet::self()->getActiveHost();
        host->mEchoLuaErrors = true;

        host->mTelnet.disconnectIt();
        if (!QTest::qWaitFor(
                    [host]() {
                        return host->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                    },
                    5000)) {
            qWarning() << "Profile did not go offline in time; feedTelnet() calls will fail";
        }

        host->mLogDir.clear();
        host->mLogFileNameFormat.clear();
        host->mLogFileName = qsl("log-restart-test");
        host->mIsNextLogFileInHtmlFormat = false;
        host->mpConsole->toggleLogging(false);
        return host;
    }

    QString stopLoggingAndReadLog(Host* host)
    {
        const QString logFileName = host->mpConsole->mLogFileName;
        host->mpConsole->toggleLogging(false);

        QFile logFile(logFileName);
        if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        return QString::fromUtf8(logFile.readAll());
    }

    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // TelnetTextDisplayedTest).
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    QString joinedBuffer()
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QString allText;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            allText.append(console->buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
    }

    bool bufferContains(const QString& needle) { return joinedBuffer().contains(needle); }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

void initializeQRCResources()
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

#include "LogRestartDuplicateLineTest.moc"
QTEST_MAIN(LogRestartDuplicateLineTest)
