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
 * lazyCaptureGlobals=false in Mudlet.ini, or MUDLET_LAZY_GLOBALS=0, turns off
 * leaving "matches", "multimatches" and "line" out of the globals table until
 * a script reads them. It is read as a profile's Lua starts, before any spec
 * could set it, so it is tested here: the profile is started with the
 * environment variable at 0, and a raw read has to find what a fire sets there
 * up front, as Mudlet did before the deferral existed.
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

#include "GroupedTest.h"

class LazyGlobalsSwitchTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QByteArray mSavedSwitch;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "LazyGlobalsSwitch-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    void runLua(const QString& code)
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) != 0) {
            const QString error = QString::fromUtf8(lua_tostring(L, -1));
            lua_pop(L, 1);
            QFAIL(qPrintable(qsl("Lua error running test script: %1").arg(error)));
        }
    }

    QString luaString(const QString& global)
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const QString value = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return value;
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
        // Before the profile's Lua starts, which is when the switch is read
        mSavedSwitch = qgetenv("MUDLET_LAZY_GLOBALS");
        qputenv("MUDLET_LAZY_GLOBALS", "0");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedSwitch.isNull() ? qunsetenv("MUDLET_LAZY_GLOBALS") : qputenv("MUDLET_LAZY_GLOBALS", mSavedSwitch);
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_capturesAreSetUpFrontWhenSwitchedOff()
    {
        // The generic mapper reads "line" on every line, which would put it in
        // the table whatever the switch says
        runLua(qsl("disableTrigger('onNewLine Trigger')\n"
                   "local id = tempRegexTrigger([[^LazySwitchOff (\\w+)$]], function()\n"
                   "  switchSeen = tostring(rawget(_G, 'matches') ~= nil) .. ' ' .. tostring(rawget(_G, 'line'))\n"
                   "end)\n"
                   "feedTriggers('\\nLazySwitchOff word\\n')\n"
                   "killTrigger(id)\n"
                   "switchLineAfter = tostring(rawget(_G, 'line'))\n"
                   "enableTrigger('onNewLine Trigger')\n"));

        QCOMPARE(luaString(qsl("switchSeen")), qsl("true LazySwitchOff word"));
        QCOMPARE(luaString(qsl("switchLineAfter")), qsl("LazySwitchOff word"));
    }

    void test_globalsMetatableIsLeftAsItWasWhenSwitchedOff()
    {
        runLua(qsl("local metatable = getmetatable(_G)\n"
                   "switchHandlers = tostring(metatable ~= nil and (rawget(metatable, '__index') ~= nil or rawget(metatable, '__newindex') ~= nil))\n"));

        QCOMPARE(luaString(qsl("switchHandlers")), qsl("false"));
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = MudletPaths::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "LazyGlobalsSwitchTest.moc"
MUDLET_GROUPED_TEST_MAIN(LazyGlobalsSwitchTest)
