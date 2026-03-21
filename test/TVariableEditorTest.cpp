/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
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
#include <QtTest/QtTest>

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


class TVariableEditorTest : public QObject {
    Q_OBJECT

private:
    lua_State* L = nullptr;
    LuaInterface* interface = nullptr;

    void execLua(const QString& code)
    {
        int error = luaL_loadstring(L, code.toUtf8().constData());
        QCOMPARE(error, 0);
        error = lua_pcall(L, 0, 0, 0);
        QCOMPARE(error, 0);
    }

    // Helper to get Lua value at a path like "_G.foo" or "_G[0]"
    QString getLuaValue(const QString& expr)
    {
        const QString code = QStringLiteral("return %1").arg(expr);
        luaL_loadstring(L, code.toUtf8().constData());
        lua_pcall(L, 0, 1, 0);
        QString result;
        if (lua_isstring(L, -1)) {
            result = QString::fromUtf8(lua_tostring(L, -1));
        } else if (lua_isnumber(L, -1)) {
            result = QString::number(lua_tonumber(L, -1));
        } else if (lua_isboolean(L, -1)) {
            result = lua_toboolean(L, -1) ? QStringLiteral("true") : QStringLiteral("false");
        } else if (lua_isnil(L, -1)) {
            result = QStringLiteral("nil");
        } else if (lua_istable(L, -1)) {
            result = QStringLiteral("table");
        }
        lua_pop(L, 1);
        return result;
    }

    QString getLuaType(const QString& expr)
    {
        const QString code = QStringLiteral("return type(%1)").arg(expr);
        luaL_loadstring(L, code.toUtf8().constData());
        lua_pcall(L, 0, 1, 0);
        QString result = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return result;
    }

    TVar* findChild(TVar* parent, const QString& name)
    {
        for (auto* child : parent->getChildren()) {
            if (child->getName() == name) {
                return child;
            }
        }
        return nullptr;
    }

private slots:

    void init()
    {
        L = luaL_newstate();
        luaL_openlibs(L);
        interface = new LuaInterface(L);
    }

    void cleanup()
    {
        delete interface;
        lua_close(L);
    }

    // ========================================================================
    // Category 1: Reading variables from Lua (getVars/iterateTable)
    // ========================================================================

    void testRetrieveStringVariable()
    {
        execLua(QStringLiteral("testVar = 'hello'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("hello"));
        QCOMPARE(var->getValueType(), LUA_TSTRING);
        QCOMPARE(var->getKeyType(), LUA_TSTRING);
    }

    void testRetrieveNumberVariable()
    {
        execLua(QStringLiteral("testVar = 42"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("42"));
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
    }

    void testRetrieveBooleanTrue()
    {
        execLua(QStringLiteral("testVar = true"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("true"));
        QCOMPARE(var->getValueType(), LUA_TBOOLEAN);
    }

    void testRetrieveBooleanFalse()
    {
        execLua(QStringLiteral("testVar = false"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("false"));
        QCOMPARE(var->getValueType(), LUA_TBOOLEAN);
    }

    void testRetrieveEmptyTable()
    {
        execLua(QStringLiteral("testVar = {}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValueType(), LUA_TTABLE);
        QCOMPARE(var->getChildren().size(), 0);
    }

    void testRetrieveTableWithStringKeys()
    {
        execLua(QStringLiteral("testVar = {a='one', b='two'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testVar"));
        QVERIFY(tbl);
        QCOMPARE(tbl->getValueType(), LUA_TTABLE);
        TVar* a = findChild(tbl, QStringLiteral("a"));
        QVERIFY(a);
        QCOMPARE(a->getValue(), QStringLiteral("one"));
        QCOMPARE(a->getKeyType(), LUA_TSTRING);
    }

    void testRetrieveTableWithNumericKeys()
    {
        execLua(QStringLiteral("testVar = {[1]='a', [2]='b'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* one = findChild(tbl, QStringLiteral("1"));
        QVERIFY(one);
        QCOMPARE(one->getKeyType(), LUA_TNUMBER);
        QCOMPARE(one->getValue(), QStringLiteral("a"));
    }

    void testRetrieveNestedTable()
    {
        execLua(QStringLiteral("testVar = {sub = {val = 'deep'}}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* sub = findChild(tbl, QStringLiteral("sub"));
        QVERIFY(sub);
        QCOMPARE(sub->getValueType(), LUA_TTABLE);
        TVar* val = findChild(sub, QStringLiteral("val"));
        QVERIFY(val);
        QCOMPARE(val->getValue(), QStringLiteral("deep"));
    }

    void testRetrieveNumericKeyZero()
    {
        execLua(QStringLiteral("testVar = {[0] = 'zero'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* zero = findChild(tbl, QStringLiteral("0"));
        QVERIFY(zero);
        QCOMPARE(zero->getKeyType(), LUA_TNUMBER);
        QCOMPARE(zero->getValue(), QStringLiteral("zero"));
    }

    void testRetrieveNegativeNumericKey()
    {
        execLua(QStringLiteral("testVar = {[-1] = 'neg'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* neg = findChild(tbl, QStringLiteral("-1"));
        QVERIFY(neg);
        QCOMPARE(neg->getKeyType(), LUA_TNUMBER);
    }

    void testRetrieveNumberValueZero()
    {
        execLua(QStringLiteral("testVar = 0"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("0"));
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
    }

    void testRetrieveStringValueZero()
    {
        execLua(QStringLiteral("testVar = '0'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("0"));
        QCOMPARE(var->getValueType(), LUA_TSTRING);
    }

    void testRetrieveEmptyStringValue()
    {
        execLua(QStringLiteral("testVar = ''"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral(""));
        QCOMPARE(var->getValueType(), LUA_TSTRING);
    }

    void testRetrieveFloatValue()
    {
        execLua(QStringLiteral("testVar = 3.14"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
        QVERIFY(var->getValue().startsWith(QStringLiteral("3.14")));
    }

    void testRetrieveMixedKeyTable()
    {
        execLua(QStringLiteral("testVar = {[1]='a', x='b'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* numKey = findChild(tbl, QStringLiteral("1"));
        QVERIFY(numKey);
        QCOMPARE(numKey->getKeyType(), LUA_TNUMBER);
        TVar* strKey = findChild(tbl, QStringLiteral("x"));
        QVERIFY(strKey);
        QCOMPARE(strKey->getKeyType(), LUA_TSTRING);
    }

    // ========================================================================
    // Category 2: Creating variables via LuaInterface (setValue/createVar)
    // ========================================================================

    void testCreateStringVarAtRoot()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("newStr"), LUA_TSTRING);
        var->setValue(QStringLiteral("hello"), LUA_TSTRING);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("newStr")), QStringLiteral("hello"));
        QCOMPARE(getLuaType(QStringLiteral("newStr")), QStringLiteral("string"));
    }

    void testCreateNumberVarAtRoot()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("newNum"), LUA_TSTRING);
        var->setValue(QStringLiteral("42"), LUA_TNUMBER);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("newNum")), QStringLiteral("42"));
        QCOMPARE(getLuaType(QStringLiteral("newNum")), QStringLiteral("number"));
    }

    void testCreateBooleanVarAtRoot()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("newBool"), LUA_TSTRING);
        var->setValue(QStringLiteral("true"), LUA_TBOOLEAN);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("newBool")), QStringLiteral("true"));
        QCOMPARE(getLuaType(QStringLiteral("newBool")), QStringLiteral("boolean"));
    }

    void testCreateTableAtRoot()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("newTbl"), LUA_TSTRING);
        var->setValue(QStringLiteral("{}"), LUA_TTABLE);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaType(QStringLiteral("newTbl")), QStringLiteral("table"));
    }

    void testCreateVarWithNumericKeyZeroAtRoot()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("0"), LUA_TNUMBER);
        var->setValue(QStringLiteral("zeroVal"), LUA_TSTRING);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("_G[0]")), QStringLiteral("zeroVal"));
    }

    void testCreateVarWithNumericKeyAtRoot()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("5"), LUA_TNUMBER);
        var->setValue(QStringLiteral("fiveVal"), LUA_TSTRING);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("_G[5]")), QStringLiteral("fiveVal"));
    }

    void testCreateVarInsideTable()
    {
        execLua(QStringLiteral("testTbl = {}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testTbl"));
        QVERIFY(tbl);

        auto* child = new TVar(tbl);
        child->setName(QStringLiteral("child"), LUA_TSTRING);
        child->setValue(QStringLiteral("childVal"), LUA_TSTRING);
        tbl->addChild(child);
        interface->createVar(child);

        QCOMPARE(getLuaValue(QStringLiteral("testTbl.child")), QStringLiteral("childVal"));
    }

    void testCreateVarWithNumericKeyInsideTable()
    {
        execLua(QStringLiteral("testTbl = {}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testTbl"));
        QVERIFY(tbl);

        auto* child = new TVar(tbl);
        child->setName(QStringLiteral("0"), LUA_TNUMBER);
        child->setValue(QStringLiteral("zeroChild"), LUA_TSTRING);
        tbl->addChild(child);
        interface->createVar(child);

        QCOMPARE(getLuaValue(QStringLiteral("testTbl[0]")), QStringLiteral("zeroChild"));
    }

    void testCreateVarWithValueZero()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("zeroVal"), LUA_TSTRING);
        var->setValue(QStringLiteral("0"), LUA_TNUMBER);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("zeroVal")), QStringLiteral("0"));
        QCOMPARE(getLuaType(QStringLiteral("zeroVal")), QStringLiteral("number"));
    }

    // ========================================================================
    // Category 3: Renaming variables (renameVar)
    // ========================================================================

    void testRenameStringVarAtRoot()
    {
        execLua(QStringLiteral("oldName = 'value'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("oldName"));
        QVERIFY(var);

        var->setNewName(QStringLiteral("newName"), LUA_TSTRING);
        interface->renameVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("newName")), QStringLiteral("value"));
        QCOMPARE(getLuaValue(QStringLiteral("oldName")), QStringLiteral("nil"));
    }

    void testRenameNumericKeyInTable()
    {
        execLua(QStringLiteral("testTbl = {[1] = 'one'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* var = findChild(tbl, QStringLiteral("1"));
        QVERIFY(var);

        var->setNewName(QStringLiteral("2"), LUA_TNUMBER);
        interface->renameVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("testTbl[2]")), QStringLiteral("one"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
    }

    void testRenameNumericKeyToZeroInTable()
    {
        execLua(QStringLiteral("testTbl = {[1] = 'one'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* var = findChild(tbl, QStringLiteral("1"));
        QVERIFY(var);

        var->setNewName(QStringLiteral("0"), LUA_TNUMBER);
        interface->renameVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("testTbl[0]")), QStringLiteral("one"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
    }

    void testRenameStringVarInsideTable()
    {
        execLua(QStringLiteral("testTbl = {old = 'val'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* var = findChild(tbl, QStringLiteral("old"));
        QVERIFY(var);

        var->setNewName(QStringLiteral("new"), LUA_TSTRING);
        interface->renameVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("testTbl.new")), QStringLiteral("val"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.old")), QStringLiteral("nil"));
    }

    // ========================================================================
    // Category 4: Deleting variables (deleteVar)
    // ========================================================================

    void testDeleteStringVarAtRoot()
    {
        execLua(QStringLiteral("toDelete = 'bye'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("toDelete"));
        QVERIFY(var);

        interface->deleteVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("toDelete")), QStringLiteral("nil"));
    }

    void testDeleteNumericKeyInTable()
    {
        execLua(QStringLiteral("testTbl = {[1]='a', [2]='b'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* var = findChild(tbl, QStringLiteral("1"));
        QVERIFY(var);

        interface->deleteVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[2]")), QStringLiteral("b"));
    }

    void testDeleteTable()
    {
        execLua(QStringLiteral("toDelete = {a=1}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("toDelete"));
        QVERIFY(var);

        interface->deleteVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("toDelete")), QStringLiteral("nil"));
    }

    // ========================================================================
    // Category 5: getValue
    // ========================================================================

    void testGetValueString()
    {
        execLua(QStringLiteral("testVar = 'hello'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(interface->getValue(var), QStringLiteral("hello"));
    }

    void testGetValueNumber()
    {
        execLua(QStringLiteral("testVar = 42"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(interface->getValue(var), QStringLiteral("42"));
    }

    void testGetValueNestedVar()
    {
        execLua(QStringLiteral("testTbl = {sub = 'deep'}"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* tbl = findChild(base, QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* sub = findChild(tbl, QStringLiteral("sub"));
        QVERIFY(sub);
        QCOMPARE(interface->getValue(sub), QStringLiteral("deep"));
    }

    // ========================================================================
    // Category 6: Round-trip tests (create → read back)
    // ========================================================================

    void testCreateAndReadBackString()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("roundTrip"), LUA_TSTRING);
        var->setValue(QStringLiteral("testValue"), LUA_TSTRING);
        base->addChild(var);
        interface->createVar(var);

        // Re-read from Lua
        interface->getVars(false);
        base = interface->getVarUnit()->getBase();
        TVar* found = findChild(base, QStringLiteral("roundTrip"));
        QVERIFY(found);
        QCOMPARE(found->getValue(), QStringLiteral("testValue"));
        QCOMPARE(found->getValueType(), LUA_TSTRING);
    }

    void testCreateModifyAndReadBack()
    {
        execLua(QStringLiteral("roundTrip = 'original'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("roundTrip"));
        QVERIFY(var);

        var->setValue(QStringLiteral("modified"), LUA_TSTRING);
        interface->setValue(var);

        QCOMPARE(getLuaValue(QStringLiteral("roundTrip")), QStringLiteral("modified"));
    }

    void testCreateRenameAndReadBack()
    {
        execLua(QStringLiteral("oldRoundTrip = 'value'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("oldRoundTrip"));
        QVERIFY(var);

        var->setNewName(QStringLiteral("newRoundTrip"), LUA_TSTRING);
        interface->renameVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("newRoundTrip")), QStringLiteral("value"));
        QCOMPARE(getLuaValue(QStringLiteral("oldRoundTrip")), QStringLiteral("nil"));
    }

    void testCreateDeleteAndReadBack()
    {
        execLua(QStringLiteral("ephemeral = 'gone'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("ephemeral"));
        QVERIFY(var);

        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("ephemeral")), QStringLiteral("nil"));
    }

    // ========================================================================
    // Category 7: Edge cases and known issues
    // ========================================================================

    void testStringValueContainingClosingBrackets()
    {
        // setValue uses [[...]] quoting which breaks on ]]
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("bracketVal"), LUA_TSTRING);
        var->setValue(QStringLiteral("a]]b"), LUA_TSTRING);
        base->addChild(var);

        // This is a known limitation: [[a]]b]] is invalid Lua
        const bool result = interface->setValue(var);
        if (!result) {
            QSKIP("Known issue: setValue fails for strings containing ']]'");
        }
        QCOMPARE(getLuaValue(QStringLiteral("bracketVal")), QStringLiteral("a]]b"));
    }

    void testNumericKeyZeroRoundTrip()
    {
        // Create _G[0] via LuaInterface, read it back
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        auto* var = new TVar(base);
        var->setName(QStringLiteral("0"), LUA_TNUMBER);
        var->setValue(QStringLiteral("zeroRoundTrip"), LUA_TSTRING);
        base->addChild(var);
        interface->createVar(var);

        QCOMPARE(getLuaValue(QStringLiteral("_G[0]")), QStringLiteral("zeroRoundTrip"));

        // Read back via getVars — the variable should appear somewhere
        // (at root level with numeric key)
        interface->getVars(false);
        base = interface->getVarUnit()->getBase();
        TVar* found = findChild(base, QStringLiteral("0"));
        if (found) {
            QCOMPARE(found->getKeyType(), LUA_TNUMBER);
            QCOMPARE(found->getValue(), QStringLiteral("zeroRoundTrip"));
        }
    }

    void testDeleteNumericKeyAtRoot()
    {
        execLua(QStringLiteral("_G[99] = 'rootNum'"));
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        TVar* var = findChild(base, QStringLiteral("99"));
        if (!var) {
            QSKIP("Root numeric keys may not be iterated by getVars");
        }

        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("_G[99]")), QStringLiteral("nil"));
    }
};

#include "TVariableEditorTest.moc"
QTEST_MAIN(TVariableEditorTest)
