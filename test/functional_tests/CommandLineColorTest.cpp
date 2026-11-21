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

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "TCommandLine.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Lua cannot see the command line's pixels, so the colours from the preferences
// are checked here rather than in a spec.
class CommandLineColorTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "CommandLineColor-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    TCommandLine* commandLine() const { return mpHost->mainConsoleView()->mpCommandLine; }

    QColor paintedBackground() const
    {
        const QImage shot = commandLine()->grab().toImage();
        return shot.pixelColor(shot.width() / 2, shot.height() / 2);
    }

    void applyColors(const QColor& foreground, const QColor& background)
    {
        mpHost->mCommandLineFgColor = foreground;
        mpHost->mCommandLineBgColor = background;
        mpHost->mainConsoleView()->changeColors();
        QTest::qWait(50ms);
    }

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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
        QVERIFY(mpHost->mainConsoleView());
        QVERIFY(commandLine());
        QVERIFY(!commandLine()->size().isEmpty());
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        runLua(qsl("setAppStyleSheet('')"));
        QTest::qWait(50ms);
    }

    void test_backgroundChangesWithoutAnAppStyleSheet()
    {
        applyColors(Qt::white, QColor(0, 128, 255));
        QCOMPARE(paintedBackground(), QColor(0, 128, 255));
    }

    void test_backgroundChangesUnderAnAppStyleSheet()
    {
        runLua(qsl("setAppStyleSheet([[QScrollBar:vertical { background: green; width: 15px; }]])"));
        QTest::qWait(50ms);

        applyColors(Qt::white, QColor(200, 40, 40));
        QCOMPARE(paintedBackground(), QColor(200, 40, 40));
        applyColors(Qt::white, QColor(40, 200, 40));
        QCOMPARE(paintedBackground(), QColor(40, 200, 40));
    }

    void test_colorsSurviveTheNextAppStyleSheet()
    {
        runLua(qsl("setAppStyleSheet([[QScrollBar:vertical { background: green; width: 15px; }]])"));
        QTest::qWait(50ms);
        applyColors(QColor(255, 255, 0), QColor(40, 40, 200));

        runLua(qsl("setAppStyleSheet([[QScrollBar:vertical { background: red; width: 15px; }]])"));
        QTest::qWait(50ms);
        QCOMPARE(paintedBackground(), QColor(40, 40, 200));
        QCOMPARE(commandLine()->palette().color(QPalette::Text), QColor(255, 255, 0));
        QCOMPARE(mpHost->mainConsoleView()->mpButtonMainLayer->palette().color(QPalette::Window), QColor(40, 40, 200));

        runLua(qsl("setAppStyleSheet('')"));
        QTest::qWait(50ms);
        QCOMPARE(paintedBackground(), QColor(40, 40, 200));
        applyColors(QColor(255, 255, 0), QColor(90, 30, 120));
        QCOMPARE(paintedBackground(), QColor(90, 30, 120));
    }

    void test_aCommandLineStyleSheetStillWins()
    {
        runLua(qsl("setAppStyleSheet([[QScrollBar:vertical { background: green; width: 15px; }]])"));
        runLua(qsl("setCmdLineStyleSheet([[QPlainTextEdit { background-color: rgb(10, 20, 30); }]])"));
        QTest::qWait(50ms);

        applyColors(Qt::white, QColor(200, 100, 0));
        QCOMPARE(paintedBackground(), QColor(10, 20, 30));

        runLua(qsl("setCmdLineStyleSheet('')"));
        QTest::qWait(50ms);
        QCOMPARE(paintedBackground(), QColor(200, 100, 0));
    }
};

#include "CommandLineColorTest.moc"
MUDLET_GROUPED_TEST_MAIN(CommandLineColorTest)
