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
 * Covers getWindowGeometry() and windowVisible() for a profile that is not the
 * front tab. Mudlet hides a backgrounded profile's whole console, so the busted
 * suite structurally cannot reach this: it always runs as the only, front,
 * profile.
 */

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class WindowStateGettersTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpBackgroundHost = nullptr;
    Host* mpFrontHost = nullptr;
    const QString mBackgroundHostname = qsl("WindowStateGetters-Background");
    const QString mFrontHostname = qsl("WindowStateGetters-Front");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mLabelName = qsl("wsgLabel");
    const QString mConsoleName = qsl("wsgConsole");
    const QString mScrollBoxName = qsl("wsgScrollBox");
    const QString mCmdLineName = qsl("wsgCmdLine");
    const QString mTextEditName = qsl("wsgTextEdit");
    const QString mUserWindowName = qsl("wsgUserWindow");
    const QString mChildLabelName = qsl("wsgChildLabel");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
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
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        deleteProfileDirectory(mBackgroundHostname);
        deleteProfileDirectory(mFrontHostname);

        startProfile(mBackgroundHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        mpBackgroundHost = mudlet::self()->getHostManager().getHost(mBackgroundHostname);
        QVERIFY(mpBackgroundHost);
        QVERIFY(mpBackgroundHost->mpConsole);

        startProfile(mFrontHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        mpFrontHost = mudlet::self()->getHostManager().getHost(mFrontHostname);
        QVERIFY(mpFrontHost);
        QVERIFY(mpFrontHost->mpConsole);

        QVERIFY2(mpBackgroundHost->mpConsole->isHidden(), "opening a second profile did not background the first one, so there is nothing to test here");
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpBackgroundHost = nullptr;
        mpFrontHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mBackgroundHostname);
            deleteProfileDirectory(mFrontHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_backgroundProfileReportsEveryElementTypeAsVisible()
    {
        buildElements(mpBackgroundHost);

        for (const QString& name : elementNames()) {
            assertVisibility(mpBackgroundHost, name, true, qsl("%1 of a backgrounded profile").arg(name));
        }

        // a hidden user window still has to take its children with it
        QVERIFY(mpBackgroundHost->hideWindow(mUserWindowName));
        assertVisibility(mpBackgroundHost, mUserWindowName, false, qsl("a hidden user window"));
        assertVisibility(mpBackgroundHost, mChildLabelName, false, qsl("a child of a hidden user window"));

        QVERIFY(mpBackgroundHost->hideWindow(mLabelName));
        assertVisibility(mpBackgroundHost, mLabelName, false, qsl("a label hidden on a backgrounded profile"));
        QVERIFY(mpBackgroundHost->showWindow(mLabelName));
        assertVisibility(mpBackgroundHost, mLabelName, true, qsl("a label shown again on a backgrounded profile"));
    }

    void test_frontProfileReportsEveryElementTypeAsVisible()
    {
        buildElements(mpFrontHost);

        for (const QString& name : elementNames()) {
            assertVisibility(mpFrontHost, name, true, qsl("%1 of the front profile").arg(name));
        }

        QVERIFY(mpFrontHost->hideWindow(mLabelName));
        assertVisibility(mpFrontHost, mLabelName, false, qsl("a label hidden on the front profile"));
    }

    void test_mainWindowAnswersBothOfItsNames()
    {
        for (const QString& name : {qsl("main"), QString()}) {
            const auto geometry = mpFrontHost->windowGeometry(name);
            QVERIFY2(geometry.has_value(), qPrintable(qsl("getWindowGeometry(\"%1\") did not recognise the main window").arg(name)));
            QCOMPARE(geometry->topLeft(), QPoint(0, 0));
            QCOMPARE(geometry->size(), mpFrontHost->mpConsole->getMainWindowSize());
            QVERIFY2(geometry->width() > 0 && geometry->height() > 0, qPrintable(qsl("the main window reported an empty geometry: %1x%2").arg(geometry->width()).arg(geometry->height())));

            assertVisibility(mpFrontHost, name, true, qsl("the front profile's main window"));
        }
    }

    void test_backgroundProfileAnswersForItsOwnMainWindow()
    {
        assertVisibility(mpBackgroundHost, qsl("main"), true, qsl("a backgrounded profile's main window"));

        // getMainWindowSize() falls back to a cached size while the console is hidden
        const auto geometry = mpBackgroundHost->windowGeometry(qsl("main"));
        QVERIFY(geometry.has_value());
        QVERIFY2(geometry->width() > 0 && geometry->height() > 0,
                 qPrintable(qsl("a backgrounded profile's main window reported an empty geometry: %1x%2").arg(geometry->width()).arg(geometry->height())));
    }

private:
    QStringList elementNames() const { return {mLabelName, mConsoleName, mScrollBoxName, mCmdLineName, mTextEditName, mUserWindowName, mChildLabelName}; }

    // built through the Lua API so each profile's own interpreter creates them
    void buildElements(Host* pHost) const
    {
        pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createLabel('%1', 0, 0, 50, 50, 1)\n"
                                                                "createMiniConsole('%2', 0, 60, 100, 50)\n"
                                                                "createScrollBox('%3', 0, 120, 100, 50)\n"
                                                                "createCommandLine('%4', 0, 180, 100, 30)\n"
                                                                "createTextEdit('%5', 0, 220, 100, 50)\n"
                                                                "openUserWindow('%6')\n"
                                                                "createLabel('%6', '%7', 5, 5, 40, 20, 1)")
                                                                    .arg(mLabelName, mConsoleName, mScrollBoxName, mCmdLineName, mTextEditName, mUserWindowName, mChildLabelName));
    }

    void assertVisibility(Host* pHost, const QString& name, const bool expected, const QString& what) const
    {
        const auto visible = pHost->windowVisible(name);
        QVERIFY2(visible.has_value(), qPrintable(qsl("windowVisible() reported %1 as not found").arg(what)));
        QVERIFY2(*visible == expected, qPrintable(qsl("windowVisible() reported %1 as %2").arg(what, *visible ? qsl("visible") : qsl("hidden"))));
    }

    void startProfile(const QString& hostname)
    {
        const QString address = mLocalhost;
        const QString port = mPort;
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "WindowStateGettersTest.moc"
MUDLET_GROUPED_TEST_MAIN(WindowStateGettersTest)
