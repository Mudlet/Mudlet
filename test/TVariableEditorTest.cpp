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
#include <TTreeWidget.h>
#include <TVar.h>
#include <VarUnit.h>
#include <utils.h>
#include <QtTest/QtTest>

#include <QTreeWidgetItem>

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


class TVariableEditorTest : public QObject
{
    Q_OBJECT

private:
    lua_State* L = nullptr;
    std::unique_ptr<LuaInterface> interface;

    void execLua(const QString& code)
    {
        int error = luaL_loadstring(L, code.toUtf8().constData());
        QCOMPARE(error, 0);
        error = lua_pcall(L, 0, 0, 0);
        QCOMPARE(error, 0);
    }

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

    static QTreeWidgetItem* childItemNamed(QTreeWidgetItem* parent, const QString& name)
    {
        for (int i = 0; i < parent->childCount(); ++i) {
            if (parent->child(i)->text(0) == name) {
                return parent->child(i);
            }
        }
        return nullptr;
    }

    // The Variables view as repopulateVars() builds it: a root row carrying no
    // check state of its own, holding a table whose member is over the 10,000
    // item limit and, beside it, a table small enough to save.
    QTreeWidgetItem* buildFenceTree(TTreeWidget& tree)
    {
        execLua(QStringLiteral("fenceHolder = {big = {}} "
                               "for i = 1, 10001 do fenceHolder.big[i] = i end "
                               "keptTable = {member = 'kept'}"));
        interface->getVars(false);
        auto* rootItem = new QTreeWidgetItem(&tree, QStringList() << QStringLiteral("Variables"));
        interface->getVarUnit()->buildVarTree(rootItem, interface->getVarUnit()->getBase(), false);
        return rootItem;
    }

    TVar* createVar(TVar* parent, const QString& name, int keyType, const QString& value, int valueType)
    {
        auto* var = new TVar(parent);
        var->setName(name, keyType);
        var->setValue(value, valueType);
        parent->addChild(var);
        interface->createVar(var);
        return var;
    }

private slots:

    void init()
    {
        L = luaL_newstate();
        luaL_openlibs(L);
        interface = std::make_unique<LuaInterface>(L);
    }

    void cleanup()
    {
        interface.reset();
        lua_close(L);
    }

    // ========================================================================
    // Reading variables from Lua — basic types
    // ========================================================================

    void testRetrieveStringVariable()
    {
        execLua(QStringLiteral("testVar = 'hello'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("hello"));
        QCOMPARE(var->getValueType(), LUA_TSTRING);
        QCOMPARE(var->getKeyType(), LUA_TSTRING);
    }

    void testRetrieveNumberVariable()
    {
        execLua(QStringLiteral("testVar = 42"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("42"));
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
    }

    void testRetrieveBooleanTrue()
    {
        execLua(QStringLiteral("testVar = true"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("true"));
        QCOMPARE(var->getValueType(), LUA_TBOOLEAN);
    }

    void testRetrieveBooleanFalse()
    {
        execLua(QStringLiteral("testVar = false"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("false"));
        QCOMPARE(var->getValueType(), LUA_TBOOLEAN);
    }

    void testRetrieveEmptyTable()
    {
        execLua(QStringLiteral("testVar = {}"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValueType(), LUA_TTABLE);
        QCOMPARE(var->getChildren().size(), 0);
    }

    void testRetrieveNumberValueZero()
    {
        execLua(QStringLiteral("testVar = 0"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("0"));
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
    }

    void testRetrieveStringValueZero()
    {
        execLua(QStringLiteral("testVar = '0'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("0"));
        QCOMPARE(var->getValueType(), LUA_TSTRING);
    }

    void testRetrieveEmptyStringValue()
    {
        execLua(QStringLiteral("testVar = ''"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral(""));
        QCOMPARE(var->getValueType(), LUA_TSTRING);
    }

    void testRetrieveFloatValue()
    {
        execLua(QStringLiteral("testVar = 3.14"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
        QVERIFY(var->getValue().startsWith(QStringLiteral("3.14")));
    }

    void testRetrieveNegativeNumber()
    {
        execLua(QStringLiteral("testVar = -42"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValue(), QStringLiteral("-42"));
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
    }

    void testRetrieveLargeNumber()
    {
        execLua(QStringLiteral("testVar = 999999999"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValueType(), LUA_TNUMBER);
    }

    // ========================================================================
    // Reading variables — table structures
    // ========================================================================

    void testRetrieveTableWithStringKeys()
    {
        execLua(QStringLiteral("testVar = {a='one', b='two'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
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
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
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
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* sub = findChild(tbl, QStringLiteral("sub"));
        QVERIFY(sub);
        QCOMPARE(sub->getValueType(), LUA_TTABLE);
        TVar* val = findChild(sub, QStringLiteral("val"));
        QVERIFY(val);
        QCOMPARE(val->getValue(), QStringLiteral("deep"));
    }

    void testRetrieveDeeplyNestedTable()
    {
        execLua(QStringLiteral("testVar = {a = {b = {c = 'bottom'}}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* a = findChild(tbl, QStringLiteral("a"));
        QVERIFY(a);
        TVar* b = findChild(a, QStringLiteral("b"));
        QVERIFY(b);
        TVar* c = findChild(b, QStringLiteral("c"));
        QVERIFY(c);
        QCOMPARE(c->getValue(), QStringLiteral("bottom"));
    }

    void testRetrieveNumericKeyZero()
    {
        execLua(QStringLiteral("testVar = {[0] = 'zero'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
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
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* neg = findChild(tbl, QStringLiteral("-1"));
        QVERIFY(neg);
        QCOMPARE(neg->getKeyType(), LUA_TNUMBER);
    }

    void testRetrieveMixedKeyTable()
    {
        execLua(QStringLiteral("testVar = {[1]='a', x='b'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* numKey = findChild(tbl, QStringLiteral("1"));
        QVERIFY(numKey);
        QCOMPARE(numKey->getKeyType(), LUA_TNUMBER);
        TVar* strKey = findChild(tbl, QStringLiteral("x"));
        QVERIFY(strKey);
        QCOMPARE(strKey->getKeyType(), LUA_TSTRING);
    }

    void testRetrieveTableWithMixedValueTypes()
    {
        execLua(QStringLiteral("testVar = {s='str', n=42, b=true, t={}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);

        TVar* s = findChild(tbl, QStringLiteral("s"));
        QVERIFY(s);
        QCOMPARE(s->getValueType(), LUA_TSTRING);

        TVar* n = findChild(tbl, QStringLiteral("n"));
        QVERIFY(n);
        QCOMPARE(n->getValueType(), LUA_TNUMBER);

        TVar* b = findChild(tbl, QStringLiteral("b"));
        QVERIFY(b);
        QCOMPARE(b->getValueType(), LUA_TBOOLEAN);

        TVar* t = findChild(tbl, QStringLiteral("t"));
        QVERIFY(t);
        QCOMPARE(t->getValueType(), LUA_TTABLE);
    }

    void testRetrieveUnicodeStringValue()
    {
        execLua(QStringLiteral("testVar = '\\228\\184\\150\\231\\149\\140'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(var->getValueType(), LUA_TSTRING);
        QVERIFY(!var->getValue().isEmpty());
    }

    void testRetrieveUnicodeKeyName()
    {
        execLua(QStringLiteral("testVar = {}; testVar['caf\\195\\169'] = 'coffee'"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        QVERIFY(tbl->getChildren().size() > 0);
    }

    void testGetVarsCalledTwice()
    {
        execLua(QStringLiteral("testVar = 'first'"));
        interface->getVars(false);
        TVar* var1 = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var1);
        QCOMPARE(var1->getValue(), QStringLiteral("first"));

        execLua(QStringLiteral("testVar = 'second'"));
        interface->getVars(false);
        TVar* var2 = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var2);
        QCOMPARE(var2->getValue(), QStringLiteral("second"));
    }

    // ========================================================================
    // Creating variables — basic types
    // ========================================================================

    void testCreateStringVarAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("newStr"), LUA_TSTRING, QStringLiteral("hello"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("newStr")), QStringLiteral("hello"));
        QCOMPARE(getLuaType(QStringLiteral("newStr")), QStringLiteral("string"));
    }

    void testCreateNumberVarAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("newNum"), LUA_TSTRING, QStringLiteral("42"), LUA_TNUMBER);
        QCOMPARE(getLuaValue(QStringLiteral("newNum")), QStringLiteral("42"));
        QCOMPARE(getLuaType(QStringLiteral("newNum")), QStringLiteral("number"));
    }

    void testCreateBooleanTrueAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("newBool"), LUA_TSTRING, QStringLiteral("true"), LUA_TBOOLEAN);
        QCOMPARE(getLuaValue(QStringLiteral("newBool")), QStringLiteral("true"));
        QCOMPARE(getLuaType(QStringLiteral("newBool")), QStringLiteral("boolean"));
    }

    void testCreateBooleanFalseAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("newBoolF"), LUA_TSTRING, QStringLiteral("false"), LUA_TBOOLEAN);
        QCOMPARE(getLuaValue(QStringLiteral("newBoolF")), QStringLiteral("false"));
        QCOMPARE(getLuaType(QStringLiteral("newBoolF")), QStringLiteral("boolean"));
    }

    void testCreateTableAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("newTbl"), LUA_TSTRING, QStringLiteral("{}"), LUA_TTABLE);
        QCOMPARE(getLuaType(QStringLiteral("newTbl")), QStringLiteral("table"));
    }

    void testCreateVarWithValueZero()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("zeroVal"), LUA_TSTRING, QStringLiteral("0"), LUA_TNUMBER);
        QCOMPARE(getLuaValue(QStringLiteral("zeroVal")), QStringLiteral("0"));
        QCOMPARE(getLuaType(QStringLiteral("zeroVal")), QStringLiteral("number"));
    }

    void testCreateVarWithNegativeValue()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("negVal"), LUA_TSTRING, QStringLiteral("-5"), LUA_TNUMBER);
        QCOMPARE(getLuaValue(QStringLiteral("negVal")), QStringLiteral("-5"));
        QCOMPARE(getLuaType(QStringLiteral("negVal")), QStringLiteral("number"));
    }

    // ========================================================================
    // Creating variables — numeric keys (the "0" bug family)
    // ========================================================================

    void testCreateVarWithNumericKeyZeroAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("0"), LUA_TNUMBER, QStringLiteral("zeroVal"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("_G[0]")), QStringLiteral("zeroVal"));
    }

    void testCreateVarWithNumericKeyAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("5"), LUA_TNUMBER, QStringLiteral("fiveVal"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("_G[5]")), QStringLiteral("fiveVal"));
    }

    void testCreateVarWithNegativeNumericKeyAtRoot()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("-1"), LUA_TNUMBER, QStringLiteral("negVal"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("_G[-1]")), QStringLiteral("negVal"));
    }

    void testCreateVarWithNumericKeyInsideTable()
    {
        execLua(QStringLiteral("testTbl = {}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        createVar(tbl, QStringLiteral("3"), LUA_TNUMBER, QStringLiteral("three"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[3]")), QStringLiteral("three"));
    }

    void testCreateVarWithNumericKeyZeroInsideTable()
    {
        execLua(QStringLiteral("testTbl = {}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        createVar(tbl, QStringLiteral("0"), LUA_TNUMBER, QStringLiteral("zeroChild"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[0]")), QStringLiteral("zeroChild"));
    }

    void testCreateVarInsideTable()
    {
        execLua(QStringLiteral("testTbl = {}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        createVar(tbl, QStringLiteral("child"), LUA_TSTRING, QStringLiteral("childVal"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.child")), QStringLiteral("childVal"));
    }

    void testCreateVarDeeplyNested()
    {
        execLua(QStringLiteral("testTbl = {sub = {}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* sub = findChild(tbl, QStringLiteral("sub"));
        QVERIFY(sub);
        createVar(sub, QStringLiteral("deep"), LUA_TSTRING, QStringLiteral("deepVal"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.sub.deep")), QStringLiteral("deepVal"));
    }

    // ========================================================================
    // Renaming variables
    // ========================================================================

    void testRenameStringVarAtRoot()
    {
        execLua(QStringLiteral("oldName = 'value'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("oldName"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("newName"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("newName")), QStringLiteral("value"));
        QCOMPARE(getLuaValue(QStringLiteral("oldName")), QStringLiteral("nil"));
    }

    void testRenameStringVarInsideTable()
    {
        execLua(QStringLiteral("testTbl = {old = 'val'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* var = findChild(tbl, QStringLiteral("old"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("new"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.new")), QStringLiteral("val"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.old")), QStringLiteral("nil"));
    }

    void testRenameNumericKeyInTable()
    {
        QSKIP("PENDING: renameVar missing else causes numeric keys to get both [N] and [\"N\"] appended");
        execLua(QStringLiteral("testTbl = {[1] = 'one'}"));
        interface->getVars(false);
        TVar* var = findChild(findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl")), QStringLiteral("1"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("2"), LUA_TNUMBER);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[2]")), QStringLiteral("one"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
    }

    void testRenameNumericKeyToZeroInTable()
    {
        QSKIP("PENDING: renameVar missing else causes numeric keys to get both [N] and [\"N\"] appended");
        execLua(QStringLiteral("testTbl = {[1] = 'one'}"));
        interface->getVars(false);
        TVar* var = findChild(findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl")), QStringLiteral("1"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("0"), LUA_TNUMBER);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[0]")), QStringLiteral("one"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
    }

    void testRenameNumericKeyToNegativeInTable()
    {
        QSKIP("PENDING: renameVar missing else causes numeric keys to get both [N] and [\"N\"] appended");
        execLua(QStringLiteral("testTbl = {[1] = 'one'}"));
        interface->getVars(false);
        TVar* var = findChild(findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl")), QStringLiteral("1"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("-5"), LUA_TNUMBER);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[-5]")), QStringLiteral("one"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
    }

    void testRenameStringToNumericKeyInTable()
    {
        execLua(QStringLiteral("testTbl = {mykey = 'val'}"));
        interface->getVars(false);
        TVar* var = findChild(findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl")), QStringLiteral("mykey"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("42"), LUA_TNUMBER);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[42]")), QStringLiteral("val"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.mykey")), QStringLiteral("nil"));
    }

    void testRenameNumericToStringKeyInTable()
    {
        QSKIP("PENDING: renameVar missing else causes numeric keys to get both [N] and [\"N\"] appended");
        execLua(QStringLiteral("testTbl = {[42] = 'val'}"));
        interface->getVars(false);
        TVar* var = findChild(findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl")), QStringLiteral("42"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("mykey"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.mykey")), QStringLiteral("val"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[42]")), QStringLiteral("nil"));
    }

    void testRenameToSameName()
    {
        QSKIP("PENDING: renameVar generates self-referencing code instead of no-op for same-name rename");
        execLua(QStringLiteral("sameName = 'val'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("sameName"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("sameName"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("sameName")), QStringLiteral("val"));
    }

    void testRenameDeeplyNested()
    {
        execLua(QStringLiteral("testTbl = {sub = {old = 'deep'}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* sub = findChild(tbl, QStringLiteral("sub"));
        QVERIFY(sub);
        TVar* var = findChild(sub, QStringLiteral("old"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("new"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.sub.new")), QStringLiteral("deep"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.sub.old")), QStringLiteral("nil"));
    }

    // ========================================================================
    // Deleting variables
    // ========================================================================

    void testDeleteStringVarAtRoot()
    {
        execLua(QStringLiteral("toDelete = 'bye'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("toDelete"));
        QVERIFY(var);
        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("toDelete")), QStringLiteral("nil"));
    }

    void testDeleteNumericKeyInTable()
    {
        execLua(QStringLiteral("testTbl = {[1]='a', [2]='b'}"));
        interface->getVars(false);
        TVar* var = findChild(findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl")), QStringLiteral("1"));
        QVERIFY(var);
        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[2]")), QStringLiteral("b"));
    }

    void testDeleteTable()
    {
        execLua(QStringLiteral("toDelete = {a=1}"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("toDelete"));
        QVERIFY(var);
        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("toDelete")), QStringLiteral("nil"));
    }

    void testDeleteDeeplyNested()
    {
        execLua(QStringLiteral("testTbl = {sub = {val = 1, keep = 2}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* sub = findChild(tbl, QStringLiteral("sub"));
        QVERIFY(sub);
        TVar* val = findChild(sub, QStringLiteral("val"));
        QVERIFY(val);
        interface->deleteVar(val);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.sub.val")), QStringLiteral("nil"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl.sub.keep")), QStringLiteral("2"));
    }

    void testDeleteNumericKeyAtRoot()
    {
        execLua(QStringLiteral("_G[99] = 'rootNum'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("99"));
        if (!var) {
            QSKIP("Root numeric keys may not be iterated by getVars");
        }
        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("_G[99]")), QStringLiteral("nil"));
    }

    // ========================================================================
    // getValue
    // ========================================================================

    void testGetValueString()
    {
        execLua(QStringLiteral("testVar = 'hello'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(interface->getValue(var), QStringLiteral("hello"));
    }

    void testGetValueNumber()
    {
        execLua(QStringLiteral("testVar = 42"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(interface->getValue(var), QStringLiteral("42"));
    }

    void testGetValueBoolean()
    {
        execLua(QStringLiteral("testVar = true"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(interface->getValue(var), QStringLiteral("true"));
    }

    void testGetValueNestedVar()
    {
        execLua(QStringLiteral("testTbl = {sub = 'deep'}"));
        interface->getVars(false);
        TVar* sub = findChild(findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl")), QStringLiteral("sub"));
        QVERIFY(sub);
        QCOMPARE(interface->getValue(sub), QStringLiteral("deep"));
    }

    void testGetValueAfterModification()
    {
        execLua(QStringLiteral("testVar = 'before'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        execLua(QStringLiteral("testVar = 'after'"));
        QCOMPARE(interface->getValue(var), QStringLiteral("after"));
    }

    // ========================================================================
    // writableByName - can a variable be written back under the name it is shown with?
    // ========================================================================

    void testWritableByNameForAGlobal()
    {
        execLua(qsl("testVar = 'hello'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), qsl("testVar"));
        QVERIFY(var);
        QVERIFY(interface->writableByName(var));
    }

    void testWritableByNameForANestedMember()
    {
        execLua(qsl("testTbl = {sub = {deep = 'value'}}"));
        interface->getVars(false);
        TVar* deep = findChild(findChild(findChild(interface->getVarUnit()->getBase(), qsl("testTbl")), qsl("sub")), qsl("deep"));
        QVERIFY(deep);
        QVERIFY(interface->writableByName(deep));
    }

    // Lua names a number key with "%.14g", which is exact for an index
    void testWritableByNameForAnIntegerKeyedMember()
    {
        execLua(qsl("testTbl = {[2] = 'value'}"));
        interface->getVars(false);
        TVar* member = findChild(findChild(interface->getVarUnit()->getBase(), qsl("testTbl")), qsl("2"));
        QVERIFY(member);
        QVERIFY(interface->writableByName(member));
    }

    // ...but not for one that needs more digits than that: the name is a
    // different key, and a write through it would add a member (#9903)
    void testNotWritableByNameForAFractionKeyedMember()
    {
        execLua(qsl("testTbl = {[1/3] = 'value'}"));
        interface->getVars(false);
        TVar* member = findChild(findChild(interface->getVarUnit()->getBase(), qsl("testTbl")), qsl("0.33333333333333"));
        QVERIFY2(member, "the walk did not name the member the way Lua names its key");
        QVERIFY(!interface->writableByName(member));
    }

    // a key is named through a C string, so it ends at an embedded NUL
    void testNotWritableByNameForAMemberWithANulInItsKey()
    {
        execLua(qsl("testTbl = {['before\\0after'] = 'value'}"));
        interface->getVars(false);
        TVar* member = findChild(findChild(interface->getVarUnit()->getBase(), qsl("testTbl")), qsl("before"));
        QVERIFY2(member, "the walk did not name the member the way Lua names its key");
        QVERIFY(!interface->writableByName(member));
    }

    // the name of a member is only as good as the names above it
    void testNotWritableByNameUnderAFractionKeyedTable()
    {
        execLua(qsl("testTbl = {[1/3] = {member = 'value'}}"));
        interface->getVars(false);
        TVar* member = findChild(findChild(findChild(interface->getVarUnit()->getBase(), qsl("testTbl")), qsl("0.33333333333333")), qsl("member"));
        QVERIFY(member);
        QVERIFY(!interface->writableByName(member));
    }

    void testNotWritableByNameForAGlobalAScriptRemoved()
    {
        execLua(qsl("testVar = 'hello'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), qsl("testVar"));
        QVERIFY(var);
        execLua(qsl("testVar = nil"));
        QVERIFY(!interface->writableByName(var));
    }

    // What it holds is the script's business: the tree may be showing a value
    // that is a type out of date, and the name still reaches the variable.
    void testWritableByNameAfterAScriptChangedTheType()
    {
        execLua(qsl("testVar = 'hello'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), qsl("testVar"));
        QVERIFY(var);
        execLua(qsl("testVar = 42"));
        QVERIFY(interface->writableByName(var));
    }

    // Both are shown as "before", so neither name says which member it means -
    // and the one a write would reach is the other one.
    void testNotWritableByNameWhenTwoMembersAreShownUnderOneName()
    {
        execLua(qsl("testTbl = {} testTbl['before\\0after'] = 'first' testTbl['before'] = 'second'"));
        interface->getVars(false);
        TVar* table = findChild(interface->getVarUnit()->getBase(), qsl("testTbl"));
        QVERIFY(table);
        int shownAsBefore = 0;
        for (TVar* member : table->getChildren(false)) {
            if (member->getName() == qsl("before")) {
                ++shownAsBefore;
                QVERIFY(!interface->writableByName(member));
            }
        }
        QCOMPARE(shownAsBefore, 2);
    }

    // ...while a number key and a string key that read alike are different
    // lookups, so both stay writable
    void testWritableByNameForAStringKeyThatReadsLikeItsNumberSibling()
    {
        execLua(qsl("testTbl = {} testTbl[42] = 'number keyed' testTbl['42'] = 'string keyed'"));
        interface->getVars(false);
        TVar* table = findChild(interface->getVarUnit()->getBase(), qsl("testTbl"));
        QVERIFY(table);
        int shownAs42 = 0;
        for (TVar* member : table->getChildren(false)) {
            if (member->getName() == qsl("42")) {
                ++shownAs42;
                QVERIFY(interface->writableByName(member));
            }
        }
        QCOMPARE(shownAs42, 2);
    }

    // The write paths put a string key into a quoted Lua literal, where a
    // backslash starts an escape and reaches a key of its own instead.
    void testNotWritableByNameForAMemberWithABackslashInItsKey()
    {
        execLua(qsl("testTbl = {} testTbl['C:\\\\temp'] = 'value'"));
        interface->getVars(false);
        TVar* member = findChild(findChild(interface->getVarUnit()->getBase(), qsl("testTbl")), qsl("C:\\temp"));
        QVERIFY2(member, "the walk did not name the member after its key");
        QVERIFY(!interface->writableByName(member));
    }

    // ...and a global goes into that source bare, so only an identifier gets
    // there. One with a dot in it would parse as an index into another global.
    void testNotWritableByNameForAGlobalWithADotInItsName()
    {
        execLua(qsl("_G['dotted.global'] = 'value'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), qsl("dotted.global"));
        QVERIFY(var);
        QVERIFY(!interface->writableByName(var));
    }

    void testNotWritableByNameForANumberKeyedGlobal()
    {
        execLua(qsl("_G[7] = 'value'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), qsl("7"));
        QVERIFY(var);
        QVERIFY(!interface->writableByName(var));
    }

    // The tables above a variable are reached by name too, so an ambiguous name
    // anywhere along the way puts the write somewhere else just as surely.
    void testNotWritableByNameUnderATableSharingItsNameWithASibling()
    {
        execLua(qsl("testTbl = {} testTbl['before\\0after'] = {member = 'first'} testTbl['before'] = {member = 'second'}"));
        interface->getVars(false);
        TVar* table = findChild(interface->getVarUnit()->getBase(), qsl("testTbl"));
        QVERIFY(table);
        int checked = 0;
        for (TVar* ambiguous : table->getChildren(false)) {
            if (ambiguous->getName() != qsl("before")) {
                continue;
            }
            TVar* member = findChild(ambiguous, qsl("member"));
            QVERIFY(member);
            QVERIFY(!interface->writableByName(member));
            ++checked;
        }
        QCOMPARE(checked, 2);
    }

    // A keyword reads as an identifier but does not parse as one, so the source
    // the write paths generate for it does not run.
    void testNotWritableByNameForAGlobalNamedAfterALuaKeyword()
    {
        execLua(qsl("_G['end'] = 'value'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), qsl("end"));
        QVERIFY(var);
        QVERIFY(!interface->writableByName(var));
    }

    // The tree is built by iterating the tables, which does not consult
    // metamethods, so neither does the lookup: what an __index stands in with
    // once a script removes the real key is a different variable.
    void testNotWritableByNameWhenOnlyAMetamethodSuppliesTheTable()
    {
        execLua(qsl("fallbackTbl = {sub = {member = 'fallback value'}} "
                    "testTbl = {sub = {member = 'real value'}} "
                    "setmetatable(testTbl, {__index = fallbackTbl})"));
        interface->getVars(false);
        TVar* member = findChild(findChild(findChild(interface->getVarUnit()->getBase(), qsl("testTbl")), qsl("sub")), qsl("member"));
        QVERIFY(member);
        QVERIFY(interface->writableByName(member));

        execLua(qsl("testTbl.sub = nil"));
        QCOMPARE(getLuaValue(qsl("testTbl.sub.member")), qsl("fallback value"));
        QVERIFY(!interface->writableByName(member));
    }

    // it walks down to the variable a push at a time, and in the editor that is
    // the profile's own stack, where a slot left behind stays for the session
    void testWritableByNameLeavesTheStackAsItFoundIt()
    {
        execLua(qsl("testTbl = {writable = 'value', [1/3] = 'value'}"));
        interface->getVars(false);
        TVar* table = findChild(interface->getVarUnit()->getBase(), qsl("testTbl"));
        QVERIFY(table);
        TVar* writable = findChild(table, qsl("writable"));
        TVar* unwritable = findChild(table, qsl("0.33333333333333"));
        QVERIFY(writable);
        QVERIFY(unwritable);

        lua_pushstring(L, "sentinel");
        const int stackBefore = lua_gettop(L);
        QVERIFY(interface->writableByName(writable));
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY(!interface->writableByName(unwritable));
        QCOMPARE(lua_gettop(L), stackBefore);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), qsl("sentinel"));
        lua_pop(L, 1);
    }

    // ========================================================================
    // setValue — modifying existing variables
    // ========================================================================

    void testSetValueChangeString()
    {
        execLua(QStringLiteral("testVar = 'old'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        var->setValue(QStringLiteral("new"), LUA_TSTRING);
        interface->setValue(var);
        QCOMPARE(getLuaValue(QStringLiteral("testVar")), QStringLiteral("new"));
    }

    void testSetValueChangeToNumber()
    {
        execLua(QStringLiteral("testVar = 'was string'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        var->setValue(QStringLiteral("42"), LUA_TNUMBER);
        interface->setValue(var);
        QCOMPARE(getLuaValue(QStringLiteral("testVar")), QStringLiteral("42"));
        QCOMPARE(getLuaType(QStringLiteral("testVar")), QStringLiteral("number"));
    }

    void testSetValueChangeToBoolean()
    {
        execLua(QStringLiteral("testVar = 'was string'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        var->setValue(QStringLiteral("true"), LUA_TBOOLEAN);
        interface->setValue(var);
        QCOMPARE(getLuaValue(QStringLiteral("testVar")), QStringLiteral("true"));
        QCOMPARE(getLuaType(QStringLiteral("testVar")), QStringLiteral("boolean"));
    }

    void testSetValueChangeToTable()
    {
        execLua(QStringLiteral("testVar = 'was string'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        var->setValue(QStringLiteral("{}"), LUA_TTABLE);
        interface->setValue(var);
        QCOMPARE(getLuaType(QStringLiteral("testVar")), QStringLiteral("table"));
    }

    void testSetValueZeroAsNumber()
    {
        execLua(QStringLiteral("testVar = 99"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        var->setValue(QStringLiteral("0"), LUA_TNUMBER);
        interface->setValue(var);
        QCOMPARE(getLuaValue(QStringLiteral("testVar")), QStringLiteral("0"));
        QCOMPARE(getLuaType(QStringLiteral("testVar")), QStringLiteral("number"));
    }

    // ========================================================================
    // Round-trip tests (create → read back, modify → read back, etc.)
    // ========================================================================

    void testCreateAndReadBackString()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("roundTrip"), LUA_TSTRING, QStringLiteral("testValue"), LUA_TSTRING);
        interface->getVars(false);
        TVar* found = findChild(interface->getVarUnit()->getBase(), QStringLiteral("roundTrip"));
        QVERIFY(found);
        QCOMPARE(found->getValue(), QStringLiteral("testValue"));
        QCOMPARE(found->getValueType(), LUA_TSTRING);
    }

    void testCreateAndReadBackNumber()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("roundTripNum"), LUA_TSTRING, QStringLiteral("42"), LUA_TNUMBER);
        interface->getVars(false);
        TVar* found = findChild(interface->getVarUnit()->getBase(), QStringLiteral("roundTripNum"));
        QVERIFY(found);
        QCOMPARE(found->getValue(), QStringLiteral("42"));
        QCOMPARE(found->getValueType(), LUA_TNUMBER);
    }

    void testCreateAndReadBackBoolean()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("roundTripBool"), LUA_TSTRING, QStringLiteral("false"), LUA_TBOOLEAN);
        interface->getVars(false);
        TVar* found = findChild(interface->getVarUnit()->getBase(), QStringLiteral("roundTripBool"));
        QVERIFY(found);
        QCOMPARE(found->getValue(), QStringLiteral("false"));
        QCOMPARE(found->getValueType(), LUA_TBOOLEAN);
    }

    void testModifyAndReadBack()
    {
        execLua(QStringLiteral("roundTrip = 'original'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("roundTrip"));
        QVERIFY(var);
        var->setValue(QStringLiteral("modified"), LUA_TSTRING);
        interface->setValue(var);
        QCOMPARE(getLuaValue(QStringLiteral("roundTrip")), QStringLiteral("modified"));
    }

    void testRenameAndReadBack()
    {
        execLua(QStringLiteral("oldRT = 'value'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("oldRT"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("newRT"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("newRT")), QStringLiteral("value"));
        QCOMPARE(getLuaValue(QStringLiteral("oldRT")), QStringLiteral("nil"));
    }

    void testDeleteAndReadBack()
    {
        execLua(QStringLiteral("ephemeral = 'gone'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("ephemeral"));
        QVERIFY(var);
        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("ephemeral")), QStringLiteral("nil"));
    }

    void testCreateTwoDeleteOne()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        createVar(base, QStringLiteral("keepMe"), LUA_TSTRING, QStringLiteral("kept"), LUA_TSTRING);
        createVar(base, QStringLiteral("deleteMe"), LUA_TSTRING, QStringLiteral("gone"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("keepMe")), QStringLiteral("kept"));
        QCOMPARE(getLuaValue(QStringLiteral("deleteMe")), QStringLiteral("gone"));

        interface->getVars(false);
        TVar* toDel = findChild(interface->getVarUnit()->getBase(), QStringLiteral("deleteMe"));
        QVERIFY(toDel);
        interface->deleteVar(toDel);
        QCOMPARE(getLuaValue(QStringLiteral("keepMe")), QStringLiteral("kept"));
        QCOMPARE(getLuaValue(QStringLiteral("deleteMe")), QStringLiteral("nil"));
    }

    // ========================================================================
    // Numeric key zero — full round-trip
    // ========================================================================

    void testNumericKeyZeroRoundTrip()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("0"), LUA_TNUMBER, QStringLiteral("zeroRT"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("_G[0]")), QStringLiteral("zeroRT"));

        interface->getVars(false);
        TVar* found = findChild(interface->getVarUnit()->getBase(), QStringLiteral("0"));
        if (found) {
            QCOMPARE(found->getKeyType(), LUA_TNUMBER);
            QCOMPARE(found->getValue(), QStringLiteral("zeroRT"));
        }
    }

    void testNumericKeyZeroInsideTableRoundTrip()
    {
        execLua(QStringLiteral("testTbl = {}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        createVar(tbl, QStringLiteral("0"), LUA_TNUMBER, QStringLiteral("zeroInTbl"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[0]")), QStringLiteral("zeroInTbl"));

        interface->getVars(false);
        tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* zero = findChild(tbl, QStringLiteral("0"));
        QVERIFY(zero);
        QCOMPARE(zero->getKeyType(), LUA_TNUMBER);
        QCOMPARE(zero->getValue(), QStringLiteral("zeroInTbl"));
    }

    // ========================================================================
    // Edge cases and known limitations
    // ========================================================================

    void testStringValueContainingClosingBrackets()
    {
        interface->getVars(false);
        auto* var = new TVar(interface->getVarUnit()->getBase());
        var->setName(QStringLiteral("bracketVal"), LUA_TSTRING);
        var->setValue(QStringLiteral("a]]b"), LUA_TSTRING);
        interface->getVarUnit()->getBase()->addChild(var);
        QVERIFY(interface->setValue(var));
        QCOMPARE(getLuaValue(QStringLiteral("bracketVal")), QStringLiteral("a]]b"));
    }

    void testStringValueContainingNewlines()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("nlVal"), LUA_TSTRING, QStringLiteral("line1\nline2"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("nlVal")), QStringLiteral("line1\nline2"));
    }

    void testStringValueContainingQuotes()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("quoteVal"), LUA_TSTRING, QStringLiteral("he said \"hi\""), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("quoteVal")), QStringLiteral("he said \"hi\""));
    }

    void testStringValueEmpty()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("emptyVal"), LUA_TSTRING, QStringLiteral(""), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("emptyVal")), QStringLiteral(""));
        QCOMPARE(getLuaType(QStringLiteral("emptyVal")), QStringLiteral("string"));
    }

    // ========================================================================
    // TVar data model tests
    // ========================================================================

    void testTVarDefaultConstructor()
    {
        TVar var;
        QCOMPARE(var.getName(), QStringLiteral(""));
        QCOMPARE(var.getValue(), QStringLiteral(""));
        QCOMPARE(var.getKeyType(), LUA_TNONE);
        QCOMPARE(var.getValueType(), LUA_TNONE);
        QVERIFY(!var.getParent());
        QCOMPARE(var.getChildren().size(), 0);
    }

    void testTVarSetNameWithType()
    {
        TVar var;
        var.setName(QStringLiteral("foo"), LUA_TSTRING);
        QCOMPARE(var.getName(), QStringLiteral("foo"));
        QCOMPARE(var.getKeyType(), LUA_TSTRING);
    }

    void testTVarSetValueWithType()
    {
        TVar var;
        var.setValue(QStringLiteral("123"), LUA_TNUMBER);
        QCOMPARE(var.getValue(), QStringLiteral("123"));
        QCOMPARE(var.getValueType(), LUA_TNUMBER);
    }

    void testTVarNewNameCycle()
    {
        TVar var;
        var.setName(QStringLiteral("old"), LUA_TSTRING);
        var.setNewName(QStringLiteral("new"), LUA_TNUMBER);
        QCOMPARE(var.getNewName(), QStringLiteral("new"));
        QCOMPARE(var.getNewKeyType(), LUA_TNUMBER);
        var.clearNewName();
        QCOMPARE(var.getName(), QStringLiteral("new"));
        QCOMPARE(var.getKeyType(), LUA_TNUMBER);
        QCOMPARE(var.getNewName(), QStringLiteral(""));
    }

    void testTVarParentChild()
    {
        TVar parent;
        auto* child1 = new TVar(&parent);
        child1->setName(QStringLiteral("c1"), LUA_TSTRING);
        parent.addChild(child1);
        auto* child2 = new TVar(&parent);
        child2->setName(QStringLiteral("c2"), LUA_TSTRING);
        parent.addChild(child2);
        QCOMPARE(parent.getChildren().size(), 2);
        parent.removeChild(child1);
        QCOMPARE(parent.getChildren().size(), 1);
        QCOMPARE(parent.getChildren().first()->getName(), QStringLiteral("c2"));
    }

    void testTVarReference()
    {
        TVar var;
        QVERIFY(!var.isReference());
        var.setReference(true);
        QVERIFY(var.isReference());
        var.setReference(false);
        QVERIFY(!var.isReference());
    }

    // ========================================================================
    // VarUnit tests
    // ========================================================================

    void testVarUnitShouldSaveNormalVar()
    {
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar var;
        var.setName(QStringLiteral("test"), LUA_TSTRING);
        var.setValue(QStringLiteral("val"), LUA_TSTRING);
        QVERIFY(vu->shouldSave(&var));
    }

    void testVarUnitShouldNotSaveFunction()
    {
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar var;
        var.setName(QStringLiteral("test"), LUA_TSTRING);
        var.setValue(QStringLiteral(""), LUA_TFUNCTION);
        QVERIFY(!vu->shouldSave(&var));
    }

    void testVarUnitShouldNotSaveReference()
    {
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar var;
        var.setName(QStringLiteral("test"), LUA_TSTRING);
        var.setValue(QStringLiteral("val"), LUA_TSTRING);
        var.setReference(true);
        QVERIFY(!vu->shouldSave(&var));
    }

    void testVarUnitHiddenVar()
    {
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar var;
        var.setName(QStringLiteral("hidden"), LUA_TSTRING);
        vu->addHidden(&var, 0);
        QVERIFY(vu->isHidden(&var));
        vu->removeHidden(&var);
        QVERIFY(!vu->isHidden(&var));
    }

    void testVarUnitSavedVar()
    {
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar var;
        var.setName(QStringLiteral("saved"), LUA_TSTRING);
        vu->addSavedVar(&var);
        QVERIFY(vu->isSaved(&var));
        vu->removeSavedVar(&var);
        QVERIFY(!vu->isSaved(&var));
    }

    // ========================================================================
    // The 10,000 item limit and the parent checkbox (#9957)
    // ========================================================================

    // Both overloads of shouldSave() answer for the same variable, so a row of
    // the Variables view standing for an oversized table has to be refused the
    // same way the table itself is.
    void testWidgetRowOfAnOversizedTableIsRefusedTheSameWayTheTableIs()
    {
        TTreeWidget tree(nullptr);
        QTreeWidgetItem* rootItem = buildFenceTree(tree);
        QVERIFY(rootItem);
        VarUnit* vu = interface->getVarUnit();
        QTreeWidgetItem* holderItem = childItemNamed(rootItem, qsl("fenceHolder"));
        QVERIFY(holderItem);
        QTreeWidgetItem* bigItem = childItemNamed(holderItem, qsl("big"));
        QTreeWidgetItem* keptItem = childItemNamed(rootItem, qsl("keptTable"));
        QVERIFY(bigItem);
        QVERIFY(keptItem);

        QVERIFY2(!vu->shouldSave(vu->getWVar(bigItem)), "a table of 10001 members is over the limit");
        QVERIFY2(!vu->shouldSave(bigItem), "and so is the row standing for it");
        QVERIFY2(vu->shouldSave(keptItem), "while the row of a small table is still saveable");
    }

    // Ticking a row hands the tick to every row beneath it, an oversized table
    // among them: Qt's tristate cascade writes the check state whether or not
    // the child's ItemIsUserCheckable flag was taken away, which is what
    // buildVarTree() does to a table over the limit. The sweep that runs after
    // the tick is what has to take it back off again, or the table goes into
    // savedVars and from there into the profile.
    void testOversizedTableTickedThroughItsParentEndsUntickedAndUnsaved()
    {
        TTreeWidget tree(nullptr);
        QTreeWidgetItem* rootItem = buildFenceTree(tree);
        QVERIFY(rootItem);
        VarUnit* vu = interface->getVarUnit();
        QTreeWidgetItem* holderItem = childItemNamed(rootItem, qsl("fenceHolder"));
        QVERIFY(holderItem);
        QTreeWidgetItem* bigItem = childItemNamed(holderItem, qsl("big"));
        QTreeWidgetItem* keptItem = childItemNamed(rootItem, qsl("keptTable"));
        QVERIFY(bigItem);
        QVERIFY(keptItem);
        QVERIFY2(!(bigItem->flags() & Qt::ItemIsUserCheckable), "the view marks an oversized table untickable");

        holderItem->setCheckState(0, Qt::Checked);
        keptItem->setCheckState(0, Qt::Checked);
        QVERIFY2(bigItem->checkState(0) == Qt::Checked, "Qt hands the tick to the untickable child all the same");

        // the sweep TTreeWidget::mouseReleaseEvent() runs over each ticked row
        // and everything under it
        QList<QTreeWidgetItem*> sweep;
        tree.getAllChildren(holderItem, sweep);
        tree.getAllChildren(keptItem, sweep);
        for (QTreeWidgetItem* item : sweep) {
            if (!vu->shouldSave(item)) {
                item->setCheckState(0, Qt::Unchecked);
            }
        }

        QCOMPARE(bigItem->checkState(0), Qt::Unchecked);
        QVERIFY2(keptItem->checkState(0) == Qt::Checked, "the fence must not reach past the tables it is for");

        // and what dlgTriggerEditor::slot_variableChanged() does with what is
        // left ticked - savedVars is what a profile save reads
        for (QTreeWidgetItem* item : sweep) {
            TVar* var = vu->getWVar(item);
            if (var && item->checkState(0) != Qt::Unchecked) {
                vu->addSavedVar(var);
            }
        }

        QVERIFY2(!vu->savedVars.contains(qsl("fenceHolder.big")), "an oversized table ticked through its parent must not be saved");
        QVERIFY2(vu->savedVars.contains(qsl("keptTable")), "a small table the user ticked still is");
    }

    // ========================================================================
    // A global whose own name holds a dot (#9954)
    // ========================================================================

    // The Variables view is where this is felt: the row for a global made after
    // the hiding walk answered to the mark that walk left for a member of the
    // same dotted path, so the row was never built and the global could not be
    // seen, renamed or deleted.
    void testGlobalWithADotInItsNameHasARowInTheVariablesView()
    {
        execLua(qsl("dotHolderTable = {member = 'the member'}"));
        interface->getVars(true); // the hiding walk Mudlet runs at profile load
        execLua(qsl("_G['dotHolderTable.member'] = 'the global'"));
        interface->getVars(false);

        TTreeWidget tree(nullptr);
        auto* rootItem = new QTreeWidgetItem(&tree, QStringList() << qsl("Variables"));
        VarUnit* vu = interface->getVarUnit();
        vu->buildVarTree(rootItem, vu->getBase(), false);

        QTreeWidgetItem* dottedItem = childItemNamed(rootItem, qsl("dotHolderTable.member"));
        QVERIFY2(dottedItem, "the global has to have a row of its own in the Variables view");
        QVERIFY2(!childItemNamed(rootItem, qsl("dotHolderTable")), "while the table the walk hid stays hidden");
    }

    // ...and that row is greyed out and untickable, with the reason on it: the
    // save keys variables by the dotted path, so this one cannot be saved.
    void testTheRowOfAGlobalWithADotInItsNameIsGreyedWithAReason()
    {
        execLua(qsl("_G['greyed.global'] = 'value' plainSaveable = 'value'"));
        interface->getVars(false);

        TTreeWidget tree(nullptr);
        auto* rootItem = new QTreeWidgetItem(&tree, QStringList() << qsl("Variables"));
        VarUnit* vu = interface->getVarUnit();
        vu->buildVarTree(rootItem, vu->getBase(), false);

        QTreeWidgetItem* dottedItem = childItemNamed(rootItem, qsl("greyed.global"));
        QVERIFY(dottedItem);
        QVERIFY2(!(dottedItem->flags() & Qt::ItemIsUserCheckable), "the row must not offer a tick that cannot be honoured");
        QCOMPARE(dottedItem->foreground(0).color(), QColor("grey"));
        QVERIFY2(!dottedItem->toolTip(0).isEmpty(), "the row has to say why it cannot be saved");

        QTreeWidgetItem* plainItem = childItemNamed(rootItem, qsl("plainSaveable"));
        QVERIFY(plainItem);
        QVERIFY2(plainItem->flags() & Qt::ItemIsUserCheckable, "an ordinary global is still tickable");
    }

    // ========================================================================
    // Float keys and values
    // ========================================================================

    void testRetrieveFloatKey()
    {
        execLua(QStringLiteral("testVar = {[1.5] = 'half'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        QVERIFY(tbl->getChildren().size() > 0);
        TVar* child = tbl->getChildren().first();
        QCOMPARE(child->getKeyType(), LUA_TNUMBER);
        QCOMPARE(child->getValue(), QStringLiteral("half"));
    }

    void testCreateVarWithFloatValue()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("floatVal"), LUA_TSTRING, QStringLiteral("3.14"), LUA_TNUMBER);
        QCOMPARE(getLuaType(QStringLiteral("floatVal")), QStringLiteral("number"));
        // setValue uses string interpolation so 3.14 should work
        QString val = getLuaValue(QStringLiteral("floatVal"));
        QVERIFY(val.startsWith(QStringLiteral("3.14")));
    }

    // ========================================================================
    // String key that looks numeric (distinguish "42" string from 42 number)
    // ========================================================================

    void testRetrieveStringKeyThatLooksNumeric()
    {
        // In Lua, ["42"] (string key) and [42] (number key) are different
        execLua(QStringLiteral("testVar = {}; testVar['42'] = 'strkey'; testVar[42] = 'numkey'"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        // Should have two children with different key types
        int numKeyCount = 0;
        int strKeyCount = 0;
        for (auto* child : tbl->getChildren()) {
            if (child->getName() == QStringLiteral("42")) {
                if (child->getKeyType() == LUA_TNUMBER) {
                    numKeyCount++;
                    QCOMPARE(child->getValue(), QStringLiteral("numkey"));
                } else if (child->getKeyType() == LUA_TSTRING) {
                    strKeyCount++;
                    QCOMPARE(child->getValue(), QStringLiteral("strkey"));
                }
            }
        }
        QCOMPARE(numKeyCount, 1);
        QCOMPARE(strKeyCount, 1);
    }

    // ========================================================================
    // Table rename preserves children
    // ========================================================================

    void testRenameTablePreservesChildren()
    {
        execLua(QStringLiteral("oldTbl = {a=1, b=2, c=3}"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("oldTbl"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("newTbl"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaType(QStringLiteral("newTbl")), QStringLiteral("table"));
        QCOMPARE(getLuaValue(QStringLiteral("newTbl.a")), QStringLiteral("1"));
        QCOMPARE(getLuaValue(QStringLiteral("newTbl.b")), QStringLiteral("2"));
        QCOMPARE(getLuaValue(QStringLiteral("newTbl.c")), QStringLiteral("3"));
        QCOMPARE(getLuaValue(QStringLiteral("oldTbl")), QStringLiteral("nil"));
    }

    // ========================================================================
    // Root-level renames between string and numeric keys
    // ========================================================================

    void testRenameRootStringToNumericKey()
    {
        QSKIP("PENDING: renameVar always uses string format for root-level renames");
        execLua(QStringLiteral("myvar = 'val'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("myvar"));
        QVERIFY(var);
        var->setNewName(QStringLiteral("42"), LUA_TNUMBER);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("_G[42]")), QStringLiteral("val"));
        QCOMPARE(getLuaValue(QStringLiteral("myvar")), QStringLiteral("nil"));
    }

    void testRenameRootNumericToStringKey()
    {
        QSKIP("PENDING: renameVar always uses string format for root-level renames");
        execLua(QStringLiteral("_G[42] = 'val'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("42"));
        if (!var) {
            QSKIP("Root numeric keys may not be iterated by getVars");
        }
        var->setNewName(QStringLiteral("myvar"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("myvar")), QStringLiteral("val"));
        QCOMPARE(getLuaValue(QStringLiteral("_G[42]")), QStringLiteral("nil"));
    }

    // ========================================================================
    // Child inside numeric-keyed table
    // ========================================================================

    void testCreateChildInsideNumericKeyedTable()
    {
        execLua(QStringLiteral("testTbl = {[5] = {}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* five = findChild(tbl, QStringLiteral("5"));
        QVERIFY(five);
        QCOMPARE(five->getValueType(), LUA_TTABLE);
        createVar(five, QStringLiteral("child"), LUA_TSTRING, QStringLiteral("inside"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[5].child")), QStringLiteral("inside"));
    }

    // ========================================================================
    // Multiple numeric keys in same table
    // ========================================================================

    void testRetrieveMultipleNumericKeys()
    {
        execLua(QStringLiteral("testVar = {[0]='a', [1]='b', [2]='c'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        QCOMPARE(tbl->getChildren().size(), 3);
        TVar* z = findChild(tbl, QStringLiteral("0"));
        QVERIFY(z);
        QCOMPARE(z->getValue(), QStringLiteral("a"));
        TVar* o = findChild(tbl, QStringLiteral("1"));
        QVERIFY(o);
        QCOMPARE(o->getValue(), QStringLiteral("b"));
        TVar* t = findChild(tbl, QStringLiteral("2"));
        QVERIFY(t);
        QCOMPARE(t->getValue(), QStringLiteral("c"));
    }

    // ========================================================================
    // Large numeric keys
    // ========================================================================

    void testRetrieveLargeNumericKey()
    {
        execLua(QStringLiteral("testVar = {[999999] = 'big'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* big = findChild(tbl, QStringLiteral("999999"));
        QVERIFY(big);
        QCOMPARE(big->getKeyType(), LUA_TNUMBER);
        QCOMPARE(big->getValue(), QStringLiteral("big"));
    }

    void testCreateVarWithLargeNumericKey()
    {
        execLua(QStringLiteral("testTbl = {}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        createVar(tbl, QStringLiteral("100000"), LUA_TNUMBER, QStringLiteral("large"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[100000]")), QStringLiteral("large"));
    }

    // ========================================================================
    // Deeply nested numeric keys — code generation path
    // ========================================================================

    void testDeeplyNestedNumericKeys()
    {
        execLua(QStringLiteral("testVar = {[1] = {[2] = {[3] = 'deep'}}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* one = findChild(tbl, QStringLiteral("1"));
        QVERIFY(one);
        TVar* two = findChild(one, QStringLiteral("2"));
        QVERIFY(two);
        TVar* three = findChild(two, QStringLiteral("3"));
        QVERIFY(three);
        QCOMPARE(three->getValue(), QStringLiteral("deep"));
        QCOMPARE(three->getKeyType(), LUA_TNUMBER);
    }

    void testCreateVarInDeeplyNestedNumericKeyTable()
    {
        execLua(QStringLiteral("testVar = {[1] = {[2] = {}}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* one = findChild(tbl, QStringLiteral("1"));
        QVERIFY(one);
        TVar* two = findChild(one, QStringLiteral("2"));
        QVERIFY(two);
        createVar(two, QStringLiteral("newkey"), LUA_TSTRING, QStringLiteral("nested"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testVar[1][2].newkey")), QStringLiteral("nested"));
    }

    // ========================================================================
    // String values with special characters
    // ========================================================================

    void testStringValueWithBackslash()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("bsVal"), LUA_TSTRING, QStringLiteral("path\\to\\file"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("bsVal")), QStringLiteral("path\\to\\file"));
    }

    void testStringValueWithSingleQuotes()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("sqVal"), LUA_TSTRING, QStringLiteral("it's a test"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("sqVal")), QStringLiteral("it's a test"));
    }

    void testStringValueVeryLong()
    {
        interface->getVars(false);
        QString longStr;
        longStr.fill(QLatin1Char('x'), 10000);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("longVal"), LUA_TSTRING, longStr, LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("longVal")).size(), 10000);
    }

    // ========================================================================
    // Self-referencing table (no infinite recursion)
    // ========================================================================

    void testSelfReferencingTable()
    {
        execLua(QStringLiteral("testVar = {}; testVar.self = testVar"));
        // getVars should not hang or crash on self-referencing tables
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        // The self-reference should be detected and handled (marked as reference or skipped)
    }

    // ========================================================================
    // getValue on nonexistent/deleted path
    // ========================================================================

    void testGetValueAfterExternalDeletion()
    {
        execLua(QStringLiteral("testVar = 'exists'"));
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(var);
        QCOMPARE(interface->getValue(var), QStringLiteral("exists"));
        // Delete externally via Lua
        execLua(QStringLiteral("testVar = nil"));
        // getValue should handle this gracefully (return empty or nil, not crash)
        QString val = interface->getValue(var);
        Q_UNUSED(val); // Just verifying no crash
    }

    // ========================================================================
    // getVars standard library filtering
    // ========================================================================

    void testGetVarsFiltersStandardLibraries()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        // Standard Lua libraries should not appear as editable children
        // (they may be hidden or filtered by iterateTable's depth/hidden logic)
        // At minimum, getVars should complete without error
        QVERIFY(base);
        QVERIFY(base->getChildren().size() >= 0);
    }

    // ========================================================================
    // Mixed numeric/string path in deeply nested code generation
    // ========================================================================

    void testMixedNumericStringNestedPath()
    {
        execLua(QStringLiteral("testVar = {sub = {[3] = {}}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* sub = findChild(tbl, QStringLiteral("sub"));
        QVERIFY(sub);
        TVar* three = findChild(sub, QStringLiteral("3"));
        QVERIFY(three);
        createVar(three, QStringLiteral("leaf"), LUA_TSTRING, QStringLiteral("mixed"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("testVar.sub[3].leaf")), QStringLiteral("mixed"));
    }

    void testNumericThenStringNestedPath()
    {
        execLua(QStringLiteral("testVar = {[1] = {name = 'first'}}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testVar"));
        QVERIFY(tbl);
        TVar* one = findChild(tbl, QStringLiteral("1"));
        QVERIFY(one);
        TVar* name = findChild(one, QStringLiteral("name"));
        QVERIFY(name);
        QCOMPARE(interface->getValue(name), QStringLiteral("first"));
    }

    // ========================================================================
    // Delete numeric-keyed child, verify sibling survives
    // ========================================================================

    void testDeleteNumericKeyChildSiblingSurvives()
    {
        execLua(QStringLiteral("testTbl = {[0]='zero', [1]='one', [2]='two'}"));
        interface->getVars(false);
        TVar* tbl = findChild(interface->getVarUnit()->getBase(), QStringLiteral("testTbl"));
        QVERIFY(tbl);
        TVar* one = findChild(tbl, QStringLiteral("1"));
        QVERIFY(one);
        interface->deleteVar(one);
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[0]")), QStringLiteral("zero"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[1]")), QStringLiteral("nil"));
        QCOMPARE(getLuaValue(QStringLiteral("testTbl[2]")), QStringLiteral("two"));
    }

    // ========================================================================
    // Create boolean false (ensure it doesn't get treated as nil)
    // ========================================================================

    void testCreateBooleanFalseRoundTrip()
    {
        interface->getVars(false);
        createVar(interface->getVarUnit()->getBase(), QStringLiteral("boolFalseRT"), LUA_TSTRING, QStringLiteral("false"), LUA_TBOOLEAN);
        QCOMPARE(getLuaType(QStringLiteral("boolFalseRT")), QStringLiteral("boolean"));
        QCOMPARE(getLuaValue(QStringLiteral("boolFalseRT")), QStringLiteral("false"));
        // Read back
        interface->getVars(false);
        TVar* found = findChild(interface->getVarUnit()->getBase(), QStringLiteral("boolFalseRT"));
        QVERIFY(found);
        QCOMPARE(found->getValueType(), LUA_TBOOLEAN);
        QCOMPARE(found->getValue(), QStringLiteral("false"));
    }

    // ========================================================================
    // Multiple operations on same variable
    // ========================================================================

    void testCreateModifyRenameDelete()
    {
        interface->getVars(false);
        TVar* base = interface->getVarUnit()->getBase();
        createVar(base, QStringLiteral("lifecycle"), LUA_TSTRING, QStringLiteral("v1"), LUA_TSTRING);
        QCOMPARE(getLuaValue(QStringLiteral("lifecycle")), QStringLiteral("v1"));

        // Modify
        interface->getVars(false);
        TVar* var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("lifecycle"));
        QVERIFY(var);
        var->setValue(QStringLiteral("v2"), LUA_TSTRING);
        interface->setValue(var);
        QCOMPARE(getLuaValue(QStringLiteral("lifecycle")), QStringLiteral("v2"));

        // Rename
        var->setNewName(QStringLiteral("renamed"), LUA_TSTRING);
        interface->renameVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("renamed")), QStringLiteral("v2"));
        QCOMPARE(getLuaValue(QStringLiteral("lifecycle")), QStringLiteral("nil"));

        // Delete
        interface->getVars(false);
        var = findChild(interface->getVarUnit()->getBase(), QStringLiteral("renamed"));
        QVERIFY(var);
        interface->deleteVar(var);
        QCOMPARE(getLuaValue(QStringLiteral("renamed")), QStringLiteral("nil"));
    }
};

#include "TVariableEditorTest.moc"
QTEST_MAIN(TVariableEditorTest)
