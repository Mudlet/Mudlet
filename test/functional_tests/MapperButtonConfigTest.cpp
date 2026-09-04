/***************************************************************************
 *   Copyright (C) 2026 by Morquin - morquin@morquin.dk                    *
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
 * Covers the setConfig("mapperButton", ...) modes that let a UI package claim
 * the built-in map buttons: "scripted" turns a press into a
 * sysMapperButtonAction event and "disabled" swallows it, in both cases
 * without a mapper or map dock being created; "default" restores the built-in
 * behavior. The get/set round-trip itself is spec-tested in Other_spec.lua -
 * this needs C++ because pressing the buttons has no Lua entry point.
 *
 * Mapper creation cannot be undone for the life of a profile, so each test
 * method needs a mudlet of its own.
 */

#include <QDockWidget>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "utils.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapperButtonConfigTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Mapper-Button-Config-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

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
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(utils::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }

        QVERIFY(mpHost->mpConsole);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        deleteProfileDirectory();
        delete mudlet::self();
    }

    void test_scriptedModeRoutesButtonToEvent()
    {
        QVERIFY(runLua(qsl("buttonEvents = 0\n"
                           "registerAnonymousEventHandler('sysMapperButtonAction', function() buttonEvents = buttonEvents + 1 end)")));
        QVERIFY(runLua(qsl("assert(setConfig('mapperButton', 'scripted'))")));

        mudlet::self()->slot_mapper();
        mudlet::self()->slot_showMapperDialog();

        QVERIFY2(runLua(qsl("assert(buttonEvents == 2)")), "expected each map button entry point to raise sysMapperButtonAction exactly once");
        QVERIFY2(!mpHost->mpConsole->mpDockableMapWidget, "scripted mode still created the default map dock");
        QVERIFY2(!mpHost->mpMap->mpMapper, "scripted mode still created a mapper");
        QVERIFY2(!mudlet::self()->findChild<QDockWidget*>(qsl("dockMap_%1_main").arg(mHostname)), "scripted mode still created a main window map dock");
    }

    void test_disabledModeSwallowsButtonUntilDefaultRestores()
    {
        QVERIFY2(mudlet::self()->dactionShowMap->isEnabled(), "the Show Map action was expected to start out enabled with a profile loaded");

        QVERIFY(runLua(qsl("assert(setConfig('mapperButton', 'disabled'))")));
        QVERIFY2(!mudlet::self()->dactionShowMap->isEnabled(), "disabled mode did not grey out the Show Map action");

        // The shortcut and menu still land in the slots even while the
        // actions are greyed, so the interception has to hold there too
        mudlet::self()->slot_mapper();
        mudlet::self()->slot_showMapperDialog();
        QVERIFY2(!mpHost->mpMap->mpMapper, "disabled mode still created a mapper");
        QVERIFY2(!mudlet::self()->findChild<QDockWidget*>(qsl("dockMap_%1_main").arg(mHostname)), "disabled mode still created a main window map dock");

        QVERIFY(runLua(qsl("assert(setConfig('mapperButton', 'default'))")));
        QVERIFY2(mudlet::self()->dactionShowMap->isEnabled(), "returning to default mode did not re-enable the Show Map action");

        mudlet::self()->slot_mapper();
        QVERIFY2(mpHost->mpMap->mpMapper, "default mode no longer lets the map button create the mapper");
    }

private:
    bool runLua(const QString& code) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    void deleteProfileDirectory() const
    {
        QDir dir(utils::getMudletPath(enums::profileHomePath, mHostname));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "MapperButtonConfigTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapperButtonConfigTest)
