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

#include <cstdlib>
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

// A Lua allocator that can be told to fail one allocation. Lua turns that into
// the panic an out-of-memory raises, which is how a test written in C++ can stop
// the variable walk part way through without the walk knowing it is a test.
struct AllocationBudget
{
    qint64 allocations = 0;
    qint64 failAt = -1;
};

static void* budgetedAllocator(void* userData, void* pointer, size_t /*oldSize*/, size_t newSize)
{
    auto* budget = static_cast<AllocationBudget*>(userData);
    if (!newSize) {
        free(pointer);
        return nullptr;
    }
    if (++budget->allocations == budget->failAt) {
        return nullptr;
    }
    return realloc(pointer, newSize);
}


class TVarTest : public QObject
{
    Q_OBJECT

private:
    lua_State* L = nullptr;
    std::unique_ptr<LuaInterface> interface;

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

    // getValue() walks down to the variable a push at a time, so a variable it
    // cannot reach leaves those pushes on the Lua stack of the profile's live
    // interpreter, where every trigger, alias and timer is then charged for them.
    void testGetValueOnAVariableThatIsGoneLeavesTheStackAsItFoundIt()
    {
        execLua("goneVar = 'here for now'");
        interface->getVars(false);
        TVar* goneVar = interface->getVarUnit()->getBase()->getChildren().first();
        QCOMPARE(goneVar->getName(), "goneVar");
        execLua("goneVar = nil");

        const int stackBefore = lua_gettop(L);
        for (int i = 0; i < 10; ++i) {
            QCOMPARE(interface->getValue(goneVar), QString());
        }
        QCOMPARE(lua_gettop(L), stackBefore);
    }

    // ...and the same for the descent into a table, which pushes once per level
    // and so leaves more behind the deeper the variable sits.
    void testGetValueOnAMemberThatIsGoneLeavesTheStackAsItFoundIt()
    {
        execLua("goneTable = {inner = {member = 'here for now'}}");
        interface->getVars(false);
        TVar* goneTable = interface->getVarUnit()->getBase()->getChildren().first();
        QCOMPARE(goneTable->getName(), "goneTable");
        TVar* innerTable = goneTable->getChildren().first();
        QCOMPARE(innerTable->getName(), "inner");
        TVar* member = innerTable->getChildren().first();
        QCOMPARE(member->getName(), "member");
        execLua("goneTable.inner = nil");

        const int stackBefore = lua_gettop(L);
        for (int i = 0; i < 10; ++i) {
            QCOMPARE(interface->getValue(member), QString());
        }
        QCOMPARE(lua_gettop(L), stackBefore);
    }

    // Which of the two pointers a variable node carries is which: the value's,
    // which is what identity-based hiding matches on, and the key's.
    void testVariableNodesCarryTheirValuesIdentity()
    {
        execLua("identityTable = {} aliasOfIdentityTable = identityTable");
        // both, or the walk keeps only the name that reached the table first
        interface->getVarUnit()->savedVars.insert(qsl("identityTable"));
        interface->getVarUnit()->savedVars.insert(qsl("aliasOfIdentityTable"));
        interface->getVars(false);

        const QList<TVar*> globals = interface->getVarUnit()->getBase()->getChildren();
        QCOMPARE(globals.size(), 2);
        QVERIFY2(globals.at(0)->pValue == globals.at(1)->pValue, "two globals holding one table must carry that table as their value identity");
        QVERIFY2(globals.at(0)->pValue, "a table has an address, so a node holding one has a value identity");
        // lua_topointer() has none to give for a string, which is what a global's key is
        QVERIFY2(!globals.at(0)->pKey, "a string-keyed node has no key identity");
    }

    // A Lua panic while the saved variables are being read cannot be resumed
    // from, so the walk has to go round again without the global it died inside.
    // Ending the walk there instead leaves a profile save holding the saved
    // globals it had reached and quietly short of the rest.
    void testAPanicInOneSavedGlobalStillLeavesTheOthersInTheTree()
    {
        // Six globals of number-keyed members, each key unique across the lot:
        // naming one costs a fresh Lua string, and those are the allocations the
        // failure below is aimed at, so it lands inside a global rather than
        // between two of them.
        const char* buildSavedGlobals = "for i = 1, 6 do "
                                        "  local t = {} "
                                        "  for j = 1, 60 do t[i * 1000 + j] = 'saved value ' .. i end "
                                        "  _G['savedRoot' .. i] = t "
                                        "end";

        AllocationBudget budget;
        lua_State* countingState = lua_newstate(&budgetedAllocator, &budget);
        QVERIFY(countingState);
        qint64 walkAllocations = 0;
        {
            luaL_openlibs(countingState);
            LuaInterface countingInterface(countingState);
            QCOMPARE(luaL_dostring(countingState, buildSavedGlobals), 0);
            markSavedRoots(countingInterface);

            const qint64 before = budget.allocations;
            countingInterface.getSavedVars();
            walkAllocations = budget.allocations - before;

            QVERIFY2(countingInterface.unreadableSavedRoots().isEmpty(), "the walk with nothing failing must read every saved global");
            QCOMPARE(savedRootsRead(countingInterface), 6);
            QVERIFY2(walkAllocations > 100, "the walk is supposed to name a few hundred number keys, each of which costs an allocation");
            countingInterface.releaseVariableReferences();
        }
        lua_close(countingState);

        AllocationBudget failingBudget;
        lua_State* failingState = lua_newstate(&budgetedAllocator, &failingBudget);
        QVERIFY(failingState);
        {
            luaL_openlibs(failingState);
            LuaInterface failingInterface(failingState);
            QCOMPARE(luaL_dostring(failingState, buildSavedGlobals), 0);
            markSavedRoots(failingInterface);

            // half way through the walk: far enough in to be inside a global,
            // far enough from the end that giving up there loses several more
            failingBudget.failAt = failingBudget.allocations + walkAllocations / 2;
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("Lua panicked")));
            failingInterface.getSavedVars();

            // every saved global is either in the tree or named as missing from
            // it - which of the six the failure lands in is Lua's business
            const QStringList unreadable = failingInterface.unreadableSavedRoots();
            QVERIFY2(!unreadable.isEmpty(), "a walk a panic cut short has to say what the save will be short of");
            QCOMPARE(savedRootsRead(failingInterface), 6 - unreadable.size());
            const QList<TVar*> readRoots = failingInterface.getVarUnit()->getBase()->getChildren(false);
            for (TVar* root : readRoots) {
                QVERIFY2(!unreadable.contains(root->getName()), "a global in the tree must not also be reported as missing from it");
                QCOMPARE(root->getChildren(false).size(), 60);
            }

            // the saved-globals-only filter must not outlive the failed walk
            // either, or the Variables view shows almost nothing afterwards
            failingInterface.getVars(false);
            QVERIFY2(savedRootsRead(failingInterface) > 6, "a getVars() after a panicked getSavedVars() must go back to reading the whole of _G");
            failingInterface.releaseVariableReferences();
        }
        lua_close(failingState);
    }

    // A hiding walk remembers every table by address, but an address is only an
    // identity while its table is alive: once the table is collected, Lua hands
    // the address to the next table it makes - often a fresh variable of the
    // user's, which then inherited the hiddenness and vanished from the
    // Variables view and from profile saves. Whether the address is in fact
    // recycled is the allocator's business (under AddressSanitizer it rarely
    // is), so the loop below takes whichever fresh table lands on it - and on a
    // plain allocator the very first one does.
    void testRecycledHiddenTableAddressDoesNotHideAFreshVariable()
    {
        execLua("hiddenGroup = {} for i = 1, 512 do hiddenGroup[i] = {'payload'} end");
        interface->getVars(true); // the hiding walk Mudlet runs at profile load
        VarUnit* vu = interface->getVarUnit();
        // hiddenGroup itself plus its 512 members
        QVERIFY2(vu->hiddenTables.size() >= 513, "the hiding walk is supposed to remember every table it finds");
        const QSet<const void*> oldAddresses = vu->hiddenTables;

        execLua("hiddenGroup = nil");
        lua_gc(L, LUA_GCCOLLECT, 0);

        // Deterministic on every allocator: an address the collector has
        // certainly reclaimed must be disproved as an identity when asked.
        TVar probe;
        probe.setName(qsl("probeVar"), LUA_TSTRING);
        probe.pValue = *oldAddresses.constBegin();
        QVERIFY2(!vu->isHidden(&probe), "a collected table's address must stop counting as a hidden identity");
        QVERIFY(!vu->hiddenTables.contains(probe.pValue));

        for (int i = 0; i < 64; ++i) {
            execLua(qsl("fresh%1 = {'payload'}").arg(i));
        }
        interface->getVars(false);

        bool recycled = false;
        for (int i = 0; i < 64; ++i) {
            TVar* fresh = findGlobal(qsl("fresh%1").arg(i));
            QVERIFY(fresh);
            recycled = recycled || oldAddresses.contains(fresh->pValue);
            QVERIFY2(!vu->isHidden(fresh), "a fresh variable must not inherit hiddenness from a collected table whose address it landed on");
        }
        qDebug() << "a collected table's address was" << (recycled ? "recycled and checked" : "not handed out again, so end-to-end decay went unexercised");
    }

    // What the identity is for (#9769): a saved variable of the user's holding
    // one of the hidden tables reaches it under a name no name-keyed lookup
    // matches. While that table is alive, the anchor must confirm it rather
    // than get in its way.
    void testASavedAliasOfALiveHiddenTableStaysHidden()
    {
        execLua("apiT = {'api table'}");
        interface->getVars(true);
        VarUnit* vu = interface->getVarUnit();
        vu->savedVars.insert(qsl("userAlias")); // or the walk drops the second name to reach the table
        execLua("userAlias = apiT");
        interface->getVars(false);

        TVar* alias = findGlobal(qsl("userAlias"));
        QVERIFY(alias);
        QVERIFY2(vu->isHidden(alias), "a saved alias of a live hidden table has to be recognised by identity");
    }

    // The write used to be a generated line of Lua source with the value spliced
    // into a [[...]] literal, which a value holding "]]" closes early: the chunk
    // did not parse, the write was quietly dropped and the parse error was left
    // on the Lua stack of the profile's live interpreter.
    void testSetValueWritesAStringHoldingClosingLongBrackets()
    {
        execLua("bracketVar = 'placeholder'");
        interface->getVars(false);
        TVar* var = findGlobal(qsl("bracketVar"));
        QVERIFY(var);
        var->setValue(qsl("a]]b"), LUA_TSTRING);

        const int stackBefore = lua_gettop(L);
        QVERIFY(interface->setValue(var));
        QCOMPARE(lua_gettop(L), stackBefore);
        QCOMPARE(globalAsString(qsl("bracketVar")), qsl("a]]b"));
    }

    // ...and a value ending in a single "]" closed the literal against the
    // bracket the generated line ended with.
    void testSetValueWritesAStringEndingInABracket()
    {
        execLua("trailVar = 'placeholder'");
        interface->getVars(false);
        TVar* var = findGlobal(qsl("trailVar"));
        QVERIFY(var);
        var->setValue(qsl("foo]"), LUA_TSTRING);

        QVERIFY(interface->setValue(var));
        QCOMPARE(globalAsString(qsl("trailVar")), qsl("foo]"));
    }

    // A long-bracket literal swallows a newline it starts with, so this write
    // reported success and stored a different string than it was given.
    void testSetValueKeepsALeadingNewlineInAString()
    {
        execLua("newlineVar = 'placeholder'");
        interface->getVars(false);
        TVar* var = findGlobal(qsl("newlineVar"));
        QVERIFY(var);
        var->setValue(qsl("\nfoo"), LUA_TSTRING);

        QVERIFY(interface->setValue(var));
        QCOMPARE(globalAsString(qsl("newlineVar")), qsl("\nfoo"));
    }

    // The keys went into that generated source as text too, inside a quoted
    // literal - so a member whose key holds a quote was lost. XMLimport reaches
    // this directly for every member it reads out of a profile save, which is
    // where such a key comes from.
    void testSetValueWritesAMemberWhoseKeyHoldsAQuote()
    {
        execLua("importHolder = {}");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("importHolder"));
        QVERIFY(holder);
        // what XMLimport::readVariable() builds before calling setValue()
        TVar member;
        member.setParent(holder);
        member.setName(qsl("a\"b"), LUA_TSTRING);
        member.setValue(qsl("42"), LUA_TNUMBER);

        QVERIFY(interface->setValue(&member));
        lua_getglobal(L, "importHolder");
        lua_pushstring(L, "a\"b");
        lua_gettable(L, -2);
        QVERIFY2(lua_isnumber(L, -1), "the member has to be written under the key it is named by, quote and all");
        QCOMPARE(lua_tonumber(L, -1), 42.0);
        lua_pop(L, 2);
    }

    // A table already there is the table the editor has just made a node for, so
    // writing it back must not empty it - the generated source assigned "= {}",
    // which did.
    void testSetValueOnATableAlreadyThereKeepsItsContents()
    {
        execLua("keepTable = {member = 'still here'}");
        interface->getVars(false);
        TVar* var = findGlobal(qsl("keepTable"));
        QVERIFY(var);
        QCOMPARE(var->getValueType(), LUA_TTABLE);

        QVERIFY(interface->setValue(var));
        lua_getglobal(L, "keepTable");
        QVERIFY(lua_istable(L, -1));
        lua_pushstring(L, "member");
        lua_gettable(L, -2);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), qsl("still here"));
        lua_pop(L, 2);
    }

    // A write that cannot get to the variable is a write that has to leave the
    // interpreter as it found it: the generated source failed inside a pcall,
    // which left the error message behind on the stack every trigger, alias and
    // timer of the profile then runs on.
    void testSetValueOnAMemberThatIsGoneLeavesTheStackAsItFoundIt()
    {
        execLua("goneHolder = {inner = {member = 'here for now'}}");
        interface->getVars(false);
        TVar* member = findGlobal(qsl("goneHolder"))->getChildren().first()->getChildren().first();
        QCOMPARE(member->getName(), qsl("member"));
        member->setValue(qsl("written"), LUA_TSTRING);
        execLua("goneHolder.inner = nil");

        const int stackBefore = lua_gettop(L);
        for (int i = 0; i < 10; ++i) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("could not reach")));
            QVERIFY(!interface->setValue(member));
        }
        QCOMPARE(lua_gettop(L), stackBefore);
    }

    // ...and the same when the write itself raises, which a table with a
    // __newindex of its own can do at any time.
    void testSetValueThatPanicsLeavesTheStackAsItFoundIt()
    {
        execLua("guardedHolder = {}");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("guardedHolder"));
        QVERIFY(holder);
        TVar member;
        member.setParent(holder);
        member.setName(qsl("blocked"), LUA_TSTRING);
        member.setValue(qsl("value"), LUA_TSTRING);

        lua_getglobal(L, "guardedHolder");
        lua_newtable(L);
        lua_pushcfunction(L, &raiseOnWrite);
        lua_setfield(L, -2, "__newindex");
        lua_setmetatable(L, -2);
        lua_pop(L, 1);

        const int stackBefore = lua_gettop(L);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("Lua panicked while writing")));
        QVERIFY(!interface->setValue(&member));
        QCOMPARE(lua_gettop(L), stackBefore);
    }

    // A member reached by a table key is named in the tree by the number of the
    // registry reference holding that key. The delete used to put that number
    // into generated source as a string key, so it nil'ed t["1"] while the
    // member itself stayed where it was and came back with the next walk.
    void testDeleteVarRemovesAMemberReachedByATableKey()
    {
        execLua("delHolder = {} do local key = {} delHolder[key] = 'keepme' end");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("delHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QVERIFY(members.first()->isReference());

        const int stackBefore = lua_gettop(L);
        interface->deleteVar(members.first());
        QCOMPARE(lua_gettop(L), stackBefore);

        lua_getglobal(L, "delHolder");
        lua_pushnil(L);
        QVERIFY2(lua_next(L, -2) == 0, "the member reached by a table key has to be the one that is deleted");
        lua_pop(L, 1);
    }

    // A rename copies the value onto the new key and only then nils the old one,
    // so renaming onto a name a sibling already answers to used to overwrite that
    // sibling's value without a word about it.
    void testRenameOntoASiblingThatIsAlreadyThereIsRefused()
    {
        execLua("renHolder = {a = 'aval', b = 'bval'}");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("renHolder"));
        QVERIFY(holder);
        TVar* memberA = nullptr;
        for (TVar* member : holder->getChildren(false)) {
            if (member->getName() == qsl("a")) {
                memberA = member;
            }
        }
        QVERIFY(memberA);

        memberA->setNewName(qsl("b"), LUA_TSTRING);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("would destroy it")));
        interface->renameVar(memberA);

        QCOMPARE(memberAsString(qsl("renHolder"), qsl("b")), qsl("bval"));
        QCOMPARE(memberAsString(qsl("renHolder"), qsl("a")), qsl("aval"));
        QCOMPARE(memberA->getName(), qsl("a"));
    }

    // A name a rename has moved off has to stop being the name the user's
    // hiding decision is remembered by, both so the renamed variable keeps that
    // decision and so an unrelated variable born under the old name does not
    // inherit it and vanish from the Variables view.
    void testRenameTakesTheUserHiddenMarkToTheNewName()
    {
        defineGlobalsTable(); // renaming a global is written as _G["new"] = old
        execLua("hiddenRenamed = 1");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* var = findGlobal(qsl("hiddenRenamed"));
        QVERIFY(var);
        vu->addHidden(var, 1); // the user's own hide, rather than a hiding walk's
        QVERIFY(vu->isHidden(qsl("hiddenRenamed")));

        var->setNewName(qsl("hiddenRenamedNew"), LUA_TSTRING);
        interface->renameVar(var);

        QVERIFY2(vu->isHidden(qsl("hiddenRenamedNew")), "the user's hiding decision has to follow the variable it was made about");
        QVERIFY2(!vu->isHidden(qsl("hiddenRenamed")), "the name the variable no longer has must stop being hidden");

        // ...which is what stops the next variable born there from inheriting it
        execLua("hiddenRenamed = 2");
        interface->getVars(false);
        TVar* fresh = findGlobal(qsl("hiddenRenamed"));
        QVERIFY(fresh);
        QVERIFY2(!vu->isHidden(fresh), "a fresh variable must not be born hidden under a name a rename left behind");
    }

    // ...and the same for a saved variable: the profile saves it by name, so a
    // mark left on the old name saves whatever turns up there instead.
    void testRenameTakesTheSavedMarkToTheNewName()
    {
        defineGlobalsTable();
        execLua("savedRenamed = 'value'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* var = findGlobal(qsl("savedRenamed"));
        QVERIFY(var);
        vu->addSavedVar(var);

        var->setNewName(qsl("savedRenamedNew"), LUA_TSTRING);
        interface->renameVar(var);

        QVERIFY(vu->savedVars.contains(qsl("savedRenamedNew")));
        QVERIFY2(!vu->savedVars.contains(qsl("savedRenamed")), "the name the variable no longer has must stop being saved");
    }

private:
    // The base library, which these tests do without, is what usually puts the
    // globals table in _G - and a rename of a global is written as _G["new"].
    void defineGlobalsTable()
    {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        lua_setglobal(L, "_G");
    }

    static int raiseOnWrite(lua_State* state) { return luaL_error(state, "this table cannot be written to"); }

    QString globalAsString(const QString& name)
    {
        lua_getglobal(L, name.toUtf8().constData());
        const QString value = QString::fromUtf8(lua_tostring(L, -1), static_cast<int>(lua_strlen(L, -1)));
        lua_pop(L, 1);
        return value;
    }

    QString memberAsString(const QString& tableName, const QString& memberName)
    {
        lua_getglobal(L, tableName.toUtf8().constData());
        lua_pushstring(L, memberName.toUtf8().constData());
        lua_gettable(L, -2);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 2);
        return value;
    }

    TVar* findGlobal(const QString& name)
    {
        TVar* base = interface->getVarUnit()->getBase();
        if (!base) {
            return nullptr;
        }
        const QList<TVar*> globals = base->getChildren(false);
        for (TVar* global : globals) {
            if (global->getName() == name) {
                return global;
            }
        }
        return nullptr;
    }

    static void markSavedRoots(LuaInterface& luaInterface)
    {
        for (int i = 1; i <= 6; ++i) {
            luaInterface.getVarUnit()->savedVars.insert(qsl("savedRoot%1").arg(i));
        }
    }

    static int savedRootsRead(LuaInterface& luaInterface)
    {
        TVar* base = luaInterface.getVarUnit()->getBase();
        return base ? base->getChildren(false).size() : 0;
    }
};

#include "TLuaInterfaceTest.moc"
QTEST_MAIN(TVarTest)
