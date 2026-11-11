/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org     *
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

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <QtTest/QtTest>

/*
 * Which stores move the counter lua_watch() hands out (3rdparty/lua51/README.md).
 * Through the C API on a state of its own rather than in a spec: several sites -
 * lua_replace() of a pseudo-index, a metatable taken off a table whose metatable
 * is watched - are out of a script's reach, and a library with its own Lua core
 * blinds the barrier for the rest of the state.
 */
class LuaWriteBarrierTest : public QObject
{
    Q_OBJECT

    lua_State* L = nullptr;
    const size_t* mEpoch = nullptr;
    size_t mLast = 0;

    bool moved()
    {
        const bool result = *mEpoch != mLast;
        mLast = *mEpoch;
        return result;
    }

    bool run(const char* chunk)
    {
        if (luaL_dostring(L, chunk)) {
            qWarning() << lua_tostring(L, -1);
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

private slots:
    void init()
    {
        L = luaL_newstate();
        luaL_openlibs(L);
        lua_newtable(L);
        mEpoch = lua_watch(L, -1, 1);
        lua_setglobal(L, "W");
        lua_newtable(L);
        lua_setglobal(L, "P");
        // U carries the watched metatable M
        lua_newtable(L);
        lua_watch(L, -1, 1);
        lua_newtable(L);
        lua_pushvalue(L, -2);
        lua_setmetatable(L, -2);
        lua_setglobal(L, "U");
        lua_setglobal(L, "M");
        lua_pushliteral(L, "watchedkey");
        lua_watch(L, -1, 1);
        lua_pop(L, 1);
        mLast = *mEpoch;
    }

    void cleanup()
    {
        lua_close(L);
        L = nullptr;
    }

    void scriptStores_data()
    {
        QTest::addColumn<QByteArray>("chunk");
        QTest::addColumn<bool>("moves");
        QTest::newRow("assignment into a watched table") << QByteArray("W.x = 1") << true;
        QTest::newRow("assignment into a plain table") << QByteArray("P.x = 1") << false;
        QTest::newRow("assignment under a watched key") << QByteArray("P.watchedkey = 1") << true;
        QTest::newRow("constructor with a watched key") << QByteArray("local t = {watchedkey = 1}") << true;
        QTest::newRow("list constructor") << QByteArray("local t = {1, 2, 3}") << false;
        QTest::newRow("table.insert() into a watched table") << QByteArray("table.insert(W, 1)") << true;
        QTest::newRow("rawset() into a watched table") << QByteArray("rawset(W, 'r', 1)") << true;
        QTest::newRow("rawset() into a plain table") << QByteArray("rawset(P, 'r', 1)") << false;
        QTest::newRow("setmetatable() on a watched table") << QByteArray("setmetatable(W, {})") << true;
        QTest::newRow("setmetatable() on a plain table") << QByteArray("setmetatable(P, {})") << false;
        QTest::newRow("setmetatable() off a watched metatable") << QByteArray("setmetatable(U, nil)") << true;
        QTest::newRow("setfenv() on a function") << QByteArray("setfenv(function() end, {})") << false;
        QTest::newRow("a full collection") << QByteArray("collectgarbage() collectgarbage()") << false;
    }

    void scriptStores()
    {
        QFETCH(QByteArray, chunk);
        QFETCH(bool, moves);
        QVERIFY(run(chunk.constData()));
        QCOMPARE(moved(), moves);
    }

    void apiStores()
    {
        lua_getglobal(L, "W");
        lua_getglobal(L, "P");
        const int watched = lua_gettop(L) - 1;
        const int plain = lua_gettop(L);
        mLast = *mEpoch;

        lua_pushinteger(L, 1);
        lua_setfield(L, watched, "a");
        QVERIFY(moved());
        lua_pushliteral(L, "b");
        lua_pushinteger(L, 1);
        lua_rawset(L, watched);
        QVERIFY(moved());
        lua_pushinteger(L, 1);
        lua_rawseti(L, watched, 5);
        QVERIFY(moved());
        lua_pushinteger(L, 1);
        lua_setfield(L, plain, "a");
        QVERIFY(!moved());
        lua_pushinteger(L, 1);
        lua_rawseti(L, plain, 1);
        QVERIFY(!moved());
        lua_pushliteral(L, "watchedkey");
        lua_pushinteger(L, 1);
        lua_rawset(L, plain);
        QVERIFY(moved());
        lua_settop(L, 0);
    }

    void stateSwaps()
    {
        lua_pushthread(L);
        lua_newtable(L);
        lua_setfenv(L, -2);
        lua_pop(L, 1);
        QVERIFY(moved());
        lua_newtable(L);
        lua_replace(L, LUA_GLOBALSINDEX);
        QVERIFY(moved());
        lua_pushvalue(L, LUA_REGISTRYINDEX);
        lua_replace(L, LUA_REGISTRYINDEX);
        QVERIFY(moved());
        lua_pushnil(L);
        lua_newtable(L);
        lua_replace(L, 1);
        QVERIFY(!moved());
        lua_settop(L, 0);
    }

    void unwatching()
    {
        lua_getglobal(L, "W");
        lua_watch(L, -1, 0);
        lua_pushinteger(L, 1);
        lua_setfield(L, -2, "unwatched");
        lua_pop(L, 1);
        QVERIFY(!moved());
        // A watched string stays fixed, so one rebuilt after a collection is still watched
        QVERIFY(run("collectgarbage() collectgarbage() local t = {} t['watched' .. 'key'] = 1"));
        QVERIFY(moved());
    }

    void libraryWithItsOwnCore()
    {
#if defined(Q_OS_LINUX)
        QVERIFY(lua_watch(L, LUA_REGISTRYINDEX, 1));
        if (!run("assert(package.loadlib('liblua5.1.so.0', 'luaopen_base'))")) {
            QSKIP("no system liblua5.1.so.0 to load");
        }
        QVERIFY(moved());
        QVERIFY(!lua_watch(L, LUA_REGISTRYINDEX, 1));
#else
        QSKIP("loads the system Lua library, which only Linux is sure to have");
#endif
    }
};

QTEST_APPLESS_MAIN(LuaWriteBarrierTest)
#include "LuaWriteBarrierTest.moc"
