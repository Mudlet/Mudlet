/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

// The starter UI builds its dock out of what the game sends, and it learns of
// that through events: gmcp.Char.Vitals for the gauges, sysProtocolEnabled for
// the channel capture and MSDP reporting. Events only fire once, so a package
// installed after the game has already spoken hears none of them - the data is
// sitting in the gmcp global, and the dock waits for the character to next take
// damage before it appears.
//
// That is the case pinned here: install on a live connection that has already
// sent vitals, and the dock has to be there when the install returns, without
// another byte from the game. The install-time catch-up at the end of the
// package's script is what makes that true.
//
// Everything is driven through the globals rather than the socket, so no part
// of this waits on a clock.

#include <QTemporaryDir>
#include <tuple>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class StarterUiProtocolCatchUpTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-StarterUiProtocolCatchUp");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    void test_installingOnAGameThatAlreadySentVitalsBuildsTheDockAtOnce()
    {
        Host* host = startProfileWithoutTheStarterUi();
        QVERIFY(host);

        // What a GMCP game has already put in the global by the time a player
        // gets around to installing the package.
        QVERIFY(runLua(host, qsl("gmcp.Char = { Vitals = { hp = 3600, maxhp = 3600, mp = 3400, maxmp = 3400 } }")));
        QVERIFY2(luaTrue(host, qsl("next(gmcp) ~= nil")), "precondition: the gmcp global has to look like a game that has already spoken");

        QVERIFY(installTheStarterUi(host));

        // No line is fed and no event is raised between the install and here, so
        // the only thing that can have built this is the catch-up.
        QVERIFY2(luaTrue(host, qsl("BaseUI.container ~= nil")), "the dock was not built from the vitals already in hand, so it waits for the character to next take damage");
        QVERIFY2(luaTrue(host, qsl("BaseUI.gauges.hp ~= nil and BaseUI.gauges.mp ~= nil")), "the gauges the vitals already describe were not created");
        QVERIFY2(luaTrue(host, qsl("BaseUI.sawGmcpVitals == true")),
                 "the package did not record that it has GMCP vitals, so the prompt-scraping ladder is still armed against a game that speaks GMCP");
    }

    // The counterpart, and the reason the catch-up can sit unconditionally at
    // the end of the script: on a game that has sent nothing there is nothing to
    // catch up on, and the package has to stay as quiet as it was before.
    void test_installingOnAGameThatHasSaidNothingBuildsNoDock()
    {
        Host* host = startProfileWithoutTheStarterUi();
        QVERIFY(host);
        QVERIFY2(luaTrue(host, qsl("next(gmcp) == nil")), "precondition: this game has sent no GMCP at all");

        QVERIFY(installTheStarterUi(host));

        QVERIFY2(luaTrue(host, qsl("BaseUI.container == nil")), "a dock was built for a game that has sent nothing to put in it");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sawGmcpVitals")), "the package recorded GMCP vitals it was never sent");
    }

    // gmcp is filled in place and never cleared, and msdp is never torn down, so
    // on a profile whose connection has ended they still describe whoever that
    // connection left behind. Catching up on them there would present a finished
    // session as current, which is worse than the wait this change removes.
    void test_installingWhileDisconnectedPaintsNothingFromTheLastConnection()
    {
        Host* host = startProfileWithoutTheStarterUi();
        QVERIFY(host);
        QVERIFY(runLua(host, qsl("gmcp.Char = { Vitals = { hp = 3600, maxhp = 3600, mp = 3400, maxmp = 3400 } }")));

        host->mTelnet.disconnectIt();
        QVERIFY2(QTest::qWaitFor(
                         [host]() {
                             return !std::get<2>(host->mTelnet.getConnectionInfo());
                         },
                         3000),
                 "the connection to the stub never went down, so this says nothing about a disconnected profile");

        QVERIFY(installTheStarterUi(host));

        QVERIFY2(luaTrue(host, qsl("BaseUI.container == nil")), "the dock was built out of vitals the ended connection left behind");
        QVERIFY2(luaTrue(host, qsl("not BaseUI.sawGmcpVitals")), "vitals from an ended connection were recorded as this connection's");
    }

private:
    // The starter UI is preinstalled for new users, and this test needs the
    // install to happen after the game has spoken rather than before, so it is
    // taken back out first.
    Host* startProfileWithoutTheStarterUi()
    {
        const QString port = QString::number(mPort);
        Host* host = TestProfile::create(mHostname, mLocalhost, port);
        if (!host) {
            QTest::qFail("No active host available for the test.", __FILE__, __LINE__);
            return nullptr;
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            QTest::qFail("Could not connect to the stub.", __FILE__, __LINE__);
            return nullptr;
        }
        host->mEchoLuaErrors = true;
        if (host->mInstalledPackages.contains(qsl("mudlet-base-ui")) && !host->uninstallPackage(qsl("mudlet-base-ui"), enums::PackageModuleType::Package)) {
            qWarning("could not uninstall the preinstalled starter UI");
            return nullptr;
        }
        if (!runLua(host, qsl("BaseUI = nil"))) {
            return nullptr;
        }
        return host;
    }

    bool installTheStarterUi(Host* host)
    {
        auto [installed, message] = host->installPackage(qsl(":/packages/mudlet-base-ui/mudlet-base-ui.mpackage"), enums::PackageModuleType::Package, true);
        if (!installed) {
            qWarning("%s", qPrintable(qsl("could not install the starter UI: %1").arg(message)));
            return false;
        }
        return luaTrue(host, qsl("type(BaseUI) == 'table'"));
    }

    bool runLua(Host* host, const QString& script) { return host->getLuaInterpreter()->compileAndExecuteScript(script); }

    bool luaTrue(Host* host, const QString& expression)
    {
        if (!runLua(host, qsl("__catchUpProbe = not not (%1)").arg(expression))) {
            qWarning("%s", qPrintable(qsl("probe did not compile: %1").arg(expression)));
            return false;
        }
        const bool result = runLua(host, qsl("assert(__catchUpProbe)"));
        if (!result) {
            qWarning("%s", qPrintable(qsl("probe is false: %1").arg(expression)));
        }
        return result;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "StarterUiProtocolCatchUpTest.moc"
MUDLET_GROUPED_TEST_MAIN(StarterUiProtocolCatchUpTest)
