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
 * TLuaInterpreter::getLuaString() runs `return <expression>` on the profile's
 * global Lua state and answers the string it yields. That state is shared by
 * everything the profile runs, so each read has to answer its own value and
 * leave the stack as deep as it found it, whether the chunk ran or raised.
 *
 * Run with: ctest -R GetLuaStringTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TLuaInterpreter.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

#include "GroupedTest.h"

class GetLuaStringTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The main window owns the profile pool in the app; here nothing else would.
    std::unique_ptr<HostManager> mpHostManager;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("GetLuaString-Test");

    lua_State* globalState() { return mpHost->getLuaInterpreter()->getLuaGlobalState(); }

    void setGlobalString(const char* name, const char* value)
    {
        lua_State* L = globalState();
        lua_pushstring(L, value);
        lua_setglobal(L, name);
    }

    static QString stackDrift(lua_State* L, const int top) { return qsl("the global Lua stack went from %1 to %2 values").arg(top).arg(lua_gettop(L)); }

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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
        QVERIFY2(HostManager::self()->addHost(mHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        mpHost = HostManager::self()->getHost(mHostname);
        QVERIFY2(mpHost, "The profile is not in the pool.");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        mpHostManager.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void readsInSuccessionAnswerEachTheirOwnValue()
    {
        setGlobalString("getLuaStringProbeA", "first");
        setGlobalString("getLuaStringProbeB", "second");

        QCOMPARE(mpHost->getLuaInterpreter()->getLuaString(qsl("getLuaStringProbeA")), qsl("first"));
        QCOMPARE(mpHost->getLuaInterpreter()->getLuaString(qsl("getLuaStringProbeB")), qsl("second"));
    }

    void successfulReadLeavesTheStackUnchanged()
    {
        setGlobalString("getLuaStringProbeKept", "kept");
        lua_State* L = globalState();
        const int top = lua_gettop(L);

        const QString value = mpHost->getLuaInterpreter()->getLuaString(qsl("getLuaStringProbeKept"));

        QVERIFY2(lua_gettop(L) == top, qPrintable(stackDrift(L, top)));
        QCOMPARE(value, qsl("kept"));
    }

    void failedReadAnswersEmptyAndLeavesTheStackUnchanged()
    {
        lua_State* L = globalState();
        lua_pushnil(L);
        lua_setglobal(L, "getLuaStringNoSuchTable");
        const int top = lua_gettop(L);

        // Compiles, then raises when run: indexing a nil global is a runtime error
        const QString value = mpHost->getLuaInterpreter()->getLuaString(qsl("getLuaStringNoSuchTable.field"));

        QVERIFY2(lua_gettop(L) == top, qPrintable(stackDrift(L, top)));
        QVERIFY2(value.isEmpty(), qPrintable(qsl("a read that raised a Lua error answered \"%1\"").arg(value)));
    }

    void nonStringValueAnswersEmptyAndLeavesTheStackUnchanged()
    {
        lua_State* L = globalState();
        lua_newtable(L);
        lua_setglobal(L, "getLuaStringProbeTable");
        const int top = lua_gettop(L);

        const QString value = mpHost->getLuaInterpreter()->getLuaString(qsl("getLuaStringProbeTable"));

        QVERIFY2(lua_gettop(L) == top, qPrintable(stackDrift(L, top)));
        QVERIFY2(value.isEmpty(), qPrintable(qsl("a read of a table answered \"%1\"").arg(value)));
    }

    void emptyNameAnswersEmptyAndLeavesTheStackUnchanged()
    {
        lua_State* L = globalState();
        // "return " yields nothing, so a read of the top slot would answer this instead
        lua_pushstring(L, "below");
        const int top = lua_gettop(L);

        const QString value = mpHost->getLuaInterpreter()->getLuaString(QString());

        QVERIFY2(lua_gettop(L) == top, qPrintable(stackDrift(L, top)));
        QVERIFY2(value.isEmpty(), qPrintable(qsl("a read with no name answered \"%1\"").arg(value)));
        lua_pop(L, 1);
    }
};

#include "GetLuaStringTest.moc"
MUDLET_GROUPED_TEST_MAIN(GetLuaStringTest)
