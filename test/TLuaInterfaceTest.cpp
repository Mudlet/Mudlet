/***************************************************************************
 *   Copyright (C) 2021 by Chris Mitchell - chrismit7@gmail.com            *
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

#include <LuaInterface.h>
#include <TVar.h>
#include <VarUnit.h>
#include <utils.h>
#include <QtTest/QtTest>

#include <QRegularExpression>

#include <memory>

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


class TVarTest : public QObject
{
    Q_OBJECT

private:
    lua_State* L = nullptr;
    std::unique_ptr<LuaInterface> interface;

    // The global the stack tests build, as the variable walk produced it.
    TVar* testGlobal()
    {
        for (TVar* root : interface->getVarUnit()->getBase()->getChildren(false)) {
            if (root->getName() == qsl("test")) {
                return root;
            }
        }
        return nullptr;
    }

    // A save runs underneath whatever called into Mudlet, so the stack these
    // reads borrow is rarely empty. These stand in for the caller's own values:
    // unwinding has to stop at the top it found rather than empty the stack.
    void pushSentinels()
    {
        lua_pushstring(L, "sentinel one");
        lua_pushnumber(L, 42);
    }

    bool sentinelsIntact()
    {
        return lua_gettop(L) >= 2 && lua_type(L, -1) == LUA_TNUMBER && lua_tonumber(L, -1) == 42 && lua_type(L, -2) == LUA_TSTRING && QString::fromUtf8(lua_tostring(L, -2)) == qsl("sentinel one");
    }

private slots: // NOLINT(readability-redundant-access-specifiers)

    void init()
    {
        L = luaL_newstate();
        interface = std::make_unique<LuaInterface>(L);
    }

    void cleanup()
    {
        interface.reset();
        lua_close(L);
    }

    void execLua(const QString& string)
    {
        luaL_loadstring(L, string.toUtf8().constData());
        lua_pcall(L, 0, 0, 0);
    }

    void testRetrieveStrings()
    {
        execLua("test = '1'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* base = vu->getBase();
        QList<TVar*> children = base->getChildren();
        TVar* testVar = children.first();
        QCOMPARE(testVar->getName(), "test");
        QCOMPARE(testVar->getValue(), "1");
        QCOMPARE(testVar->getValueType(), LUA_TSTRING);
    }

    void testRetrieveNumber()
    {
        execLua("test = 1");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* base = vu->getBase();
        QList<TVar*> children = base->getChildren();
        TVar* testVar = children.first();
        QCOMPARE(testVar->getName(), "test");
        QCOMPARE(testVar->getValue(), "1");
        QCOMPARE(testVar->getValueType(), LUA_TNUMBER);
    }

    void testGetValueLeavesTheStackAsItFoundIt_data()
    {
        QTest::addColumn<QString>("setup");
        QTest::addColumn<QString>("expectedValue");

        QTest::newRow("string key") << qsl("test = {} test.plain = 'v'") << qsl("v");
        QTest::newRow("number key") << qsl("test = {} test[7] = 'v'") << qsl("v");
        QTest::newRow("boolean key") << qsl("test = {} test[true] = 'v'") << qsl("v");
        // Keys the walk can name but not look back up, so the lookup ends on a
        // nil and leaves getValue() through its failure exit. Lua names a number
        // with "%.14g", which is what makes anything fractional or past 14
        // digits one of these; a key is named and pushed back through a C
        // string, so bytes that are not valid UTF-8 come back as U+FFFD and
        // anything past an embedded NUL is cut off.
        QTest::newRow("fractional number key") << qsl("test = {} test[1/3] = 'v'") << QString();
        QTest::newRow("number key past 14 digits") << qsl("test = {} test[12345678901234567] = 'v'") << QString();
        QTest::newRow("key that is not valid UTF-8") << qsl("test = {} test[\"\\255\"] = 'v'") << QString();
        QTest::newRow("key holding a NUL") << qsl("test = {} test[\"a\\0b\"] = 'v'") << QString();
    }

    // getValue() reaches the variable over the profile's live Lua stack, and a
    // save calls it once per exported variable, so a slot left on a failure exit
    // is charged to the state for the rest of the session (#9885).
    void testGetValueLeavesTheStackAsItFoundIt()
    {
        QFETCH(QString, setup);
        QFETCH(QString, expectedValue);

        execLua(setup);
        interface->getVars(false);
        TVar* root = testGlobal();
        QVERIFY2(root, "the walk did not produce the global the setup builds");
        const QList<TVar*> members = root->getChildren(false);
        QCOMPARE(members.size(), 1);

        pushSentinels();
        const int stackBefore = lua_gettop(L);
        QCOMPARE(interface->getValue(members.constFirst()), expectedValue);
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY2(sentinelsIntact(), "the read unwound past the top it was handed");
    }

    // Another failure exit, reached without any odd key at all: the root lookup
    // itself comes back nil, because the Variables view hands getValue() a
    // variable from the tree it last built and a script is free to have deleted
    // it since.
    void testGetValueOnADeletedGlobalLeavesTheStackAsItFoundIt()
    {
        execLua(qsl("test = 'v'"));
        interface->getVars(false);
        TVar* global = testGlobal();
        QVERIFY(global);
        execLua(qsl("test = nil"));

        pushSentinels();
        const int stackBefore = lua_gettop(L);
        QCOMPARE(interface->getValue(global), QString());
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY2(sentinelsIntact(), "the read unwound past the top it was handed");
    }

    // A global under a table key. The root lookup handles string, number and
    // boolean keys, so this one puts nothing on the stack at all and the value
    // the caller left on top must not be taken for the root.
    void testGetValueOnATableKeyedGlobalLeavesTheStackAsItFoundIt()
    {
        // the base library is not open in this state, so there is no _G to
        // subscript from Lua
        lua_newtable(L);
        lua_newtable(L);
        lua_pushstring(L, "member");
        lua_pushstring(L, "v");
        lua_settable(L, -3);
        lua_settable(L, LUA_GLOBALSINDEX);
        interface->getVars(false);
        TVar* root = nullptr;
        for (TVar* global : interface->getVarUnit()->getBase()->getChildren(false)) {
            if (global->getKeyType() == LUA_TTABLE) {
                root = global;
            }
        }
        QVERIFY2(root, "the walk did not produce the table-keyed global");
        QCOMPARE(root->getChildren(false).size(), 1);

        pushSentinels();
        const int stackBefore = lua_gettop(L);
        QCOMPARE(interface->getValue(root->getChildren(false).constFirst()), QString());
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY2(sentinelsIntact(), "the read unwound past the top it was handed");
    }

    // The table a variable sits in is no longer a table, which the walk's tree
    // has no way of knowing.
    void testGetValueWhereTheParentIsNoLongerATableLeavesTheStackAsItFoundIt()
    {
        execLua(qsl("test = {member = 'v'}"));
        interface->getVars(false);
        TVar* root = testGlobal();
        QVERIFY(root);
        QCOMPARE(root->getChildren(false).size(), 1);
        execLua(qsl("test = 'no longer a table'"));

        pushSentinels();
        const int stackBefore = lua_gettop(L);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("is not a table for variable")));
        QCOMPARE(interface->getValue(root->getChildren(false).constFirst()), QString());
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY2(sentinelsIntact(), "the read unwound past the top it was handed");
    }

    // Reaching a nested variable pushes one slot per level, and Lua grows the
    // stack for a C caller only when asked, so the read has to reserve the room
    // it needs before it starts pushing.
    void testGetValueOnADeeplyNestedVariableLeavesTheStackAsItFoundIt()
    {
        execLua(qsl("test = {} local t = test for i = 1, 90 do t.nested = {} t = t.nested end t.leaf = 'deep value'"));
        interface->getVars(false);
        TVar* node = testGlobal();
        QVERIFY(node);
        while (!node->getChildren(false).isEmpty()) {
            node = node->getChildren(false).constFirst();
        }
        QCOMPARE(node->getName(), qsl("leaf"));

        pushSentinels();
        const int stackBefore = lua_gettop(L);
        QCOMPARE(interface->getValue(node), qsl("deep value"));
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY2(sentinelsIntact(), "the read unwound past the top it was handed");
    }
};

#include "TLuaInterfaceTest.moc"
QTEST_MAIN(TVarTest)
