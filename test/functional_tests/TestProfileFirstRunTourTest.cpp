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

/*
 * TestProfile::create() in a config dir that reads as a brand new installation,
 * with nothing written to the settings first. The first-run interface tour opens
 * a second after such a profile loads and takes the window's keyboard, so a test
 * sending keys after that lost them. See issue #10932.
 *
 * Run with: ctest -R TestProfileFirstRunTourTest -V
 */

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TCommandLine.h"
#include "TMainConsole.h"
#include "TUiTour.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TestProfileFirstRunTourTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mProfileName = qsl("FirstRunTour-Test");

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
        mpServer->start(qsl("127.0.0.1"), 0);

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(TUiTour::shouldShowOnFirstProfile(), "this config dir does not read as a first run, so the case below cannot tell anything apart");
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_keysStillReachTheProfileAfterTheTourWouldHaveOpened()
    {
        Host* host = TestProfile::create(mProfileName, qsl("127.0.0.1"), QString::number(mpServer->serverPort()));
        QVERIFY2(host, "Could not create the test profile - see the warning above for the step that timed out.");

        // The tour is scheduled for a second after the profile loads
        const bool tourOpened = QTest::qWaitFor(
                []() {
                    return mudlet::self()->findChild<TUiTour*>() != nullptr;
                },
                2500ms);
        QVERIFY2(!tourOpened, "the first-run interface tour opened over a profile made by TestProfile::create()");

        TCommandLine* commandLine = host->mainConsoleView()->mpCommandLine;
        commandLine->clear();
        QTest::keyClicks(commandLine, qsl("look"));
        QCOMPARE(commandLine->toPlainText(), qsl("look"));
    }
};

#include "TestProfileFirstRunTourTest.moc"
MUDLET_GROUPED_TEST_MAIN(TestProfileFirstRunTourTest)
