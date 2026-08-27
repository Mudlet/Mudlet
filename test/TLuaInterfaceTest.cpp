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
#include <vector>

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


// The comparator TVar::getChildren() sorts the members of a table with. It is a
// free function in TVar.cpp that TVar.h does not declare, so it is named here.
bool TVarLessThan(TVar*, TVar*);


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

    // A hidden table is remembered by address as well as by name, and the name
    // half is what un-hiding is asked by. Left on the old name, the address goes
    // on answering for the table under a name nothing can give it back from - so
    // the table can no longer be un-hidden at all.
    void testRenameTakesAHiddenTablesIdentityToTheNewName()
    {
        execLua("hiddenTableRenamed = {member = 'value'}");
        interface->getVars(true); // the hiding walk Mudlet runs at profile load
        VarUnit* vu = interface->getVarUnit();
        TVar* var = findGlobal(qsl("hiddenTableRenamed"));
        QVERIFY(var);
        QVERIFY2(vu->isHidden(var), "the hiding walk is supposed to have hidden the table");

        var->setNewName(qsl("hiddenTableRenamedNew"), LUA_TSTRING);
        QVERIFY(interface->renameVar(var));

        QVERIFY2(vu->isHidden(var), "a rename does not change the table, so its identity still answers for it");
        vu->removeHidden(qsl("hiddenTableRenamedNew"));
        QVERIFY2(!vu->isHidden(var), "un-hiding the name the table now has has to give its identity up as well");
    }

    // ...and a member of a renamed table is remembered by a name beginning with
    // the table's, so a rename of the table has to take those with it too - the
    // saved mark was left pointing at a path with no variable at the end of it.
    void testRenamingATableTakesItsMembersSavedMarksWithIt()
    {
        execLua("renHolder = {a = 'aval'} renHolderOther = 'unrelated'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* holder = findGlobal(qsl("renHolder"));
        QVERIFY(holder);
        vu->addSavedVar(holder->getChildren(false).first());
        vu->addSavedVar(findGlobal(qsl("renHolderOther")));
        QVERIFY(vu->savedVars.contains(qsl("renHolder.a")));

        holder->setNewName(qsl("renHolderNew"), LUA_TSTRING);
        QVERIFY(interface->renameVar(holder));

        QVERIFY2(vu->savedVars.contains(qsl("renHolderNew.a")), "what is saved inside a renamed table has to be saved under the name the table now has");
        QVERIFY2(!vu->savedVars.contains(qsl("renHolder.a")), "and must stop being saved under a path that now names nothing");
        QVERIFY2(vu->savedVars.contains(qsl("renHolderOther")), "a variable that merely starts with the same text is a variable of its own");
    }

    // A member whose key is a table of its own is named in the tree by the
    // number of the registry reference holding that key, and that number is not
    // a name a rename can write: renaming such a member deleted it outright.
    void testRenamingATableKeyedMemberIsRefusedRatherThanDestroyingIt()
    {
        execLua("tableKeyHolder = {} do local key = {} tableKeyHolder[key] = 'keepme' end");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("tableKeyHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QVERIFY(members.first()->isReference());

        members.first()->setNewName(qsl("renamedKey"), LUA_TSTRING);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("no name to change")));
        QVERIFY(!interface->renameVar(members.first()));

        QCOMPARE(luaMemberCount(qsl("tableKeyHolder")), 1);
        QVERIFY2(memberAsString(qsl("tableKeyHolder"), qsl("renamedKey")).isEmpty(), "the rename must not have made a member of that name either");
    }

    // ...and the same for a function used as a key, which left the member alone
    // but moved the saved mark onto a path naming nothing.
    void testRenamingAFunctionKeyedMemberLeavesItsSavedMarkAlone()
    {
        execLua("fnKeyHolder = {} fnKeyHolder[function() end] = 'keepme'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* holder = findGlobal(qsl("fnKeyHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QVERIFY(members.first()->isReference());
        const QString keyName = members.first()->getName();
        vu->savedVars.insert(qsl("fnKeyHolder.%1").arg(keyName));

        members.first()->setNewName(qsl("renamedFn"), LUA_TSTRING);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("no name to change")));
        QVERIFY(!interface->renameVar(members.first()));

        QCOMPARE(luaMemberCount(qsl("fnKeyHolder")), 1);
        QVERIFY2(!vu->savedVars.contains(qsl("fnKeyHolder.renamedFn")), "a refused rename must not move what is remembered onto a name that names nothing");
        QVERIFY2(vu->savedVars.contains(qsl("fnKeyHolder.%1").arg(keyName)), "the mark stays on the member the user made it about");
        QCOMPARE(members.first()->getName(), keyName);
    }

    // The name a rename would land on is looked up with the key type the rename
    // is giving it, not the one the variable has: t[5] and t["5"] are two
    // members, and asking about the wrong one waved the rename through.
    void testRenameOntoANumberKeyedSiblingIsRefused()
    {
        execLua("numKeyHolder = {[5] = 'five', other = 'otherval'}");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("numKeyHolder"));
        QVERIFY(holder);
        TVar* other = nullptr;
        for (TVar* member : holder->getChildren(false)) {
            if (member->getName() == qsl("other")) {
                other = member;
            }
        }
        QVERIFY(other);

        other->setNewName(qsl("5"), LUA_TNUMBER);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("would destroy it")));
        QVERIFY(!interface->renameVar(other));

        QCOMPARE(numberKeyedMemberAsString(qsl("numKeyHolder"), 5), qsl("five"));
        QCOMPARE(memberAsString(qsl("numKeyHolder"), qsl("other")), qsl("otherval"));
        QCOMPARE(other->getName(), qsl("other"));
    }

    // A write to a member sitting under a table used as a key has to put that
    // key back on the stack out of the registry to get to the member at all -
    // the text of the reference number reaches a member of its own instead.
    void testSetValueReachesAMemberUnderATableKeyedIntermediate()
    {
        execLua("refHolder = {} do local key = {} refHolder[key] = {inner = 'placeholder'} end");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("refHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QVERIFY(members.first()->isReference());
        TVar* inner = members.first()->getChildren(false).first();
        QCOMPARE(inner->getName(), qsl("inner"));
        inner->setValue(qsl("written"), LUA_TSTRING);

        const int stackBefore = lua_gettop(L);
        QVERIFY(interface->setValue(inner));
        QCOMPARE(lua_gettop(L), stackBefore);

        QCOMPARE(luaMemberCount(qsl("refHolder")), 1);
        QCOMPARE(onlyMembersMemberAsString(qsl("refHolder"), qsl("inner")), qsl("written"));
    }

    // A rename that was refused leaves a new name pending on the node, and the
    // editor goes on to its no-change path - which used to commit that pending
    // name. The node then named the sibling the rename was refused over, and
    // the next write through it went there.
    void testARefusedRenameLeavesTheNodeNamingItsOwnVariable()
    {
        execLua("landmineHolder = {a = 'aval', b = 'bval'}");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("landmineHolder"));
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
        QVERIFY(!interface->renameVar(memberA));

        memberA->abandonNewName(); // what the editor does when nothing changed
        QCOMPARE(memberA->getName(), qsl("a"));

        memberA->setValue(qsl("written"), LUA_TSTRING);
        QVERIFY(interface->setValue(memberA));
        QCOMPARE(memberAsString(qsl("landmineHolder"), qsl("a")), qsl("written"));
        QVERIFY2(memberAsString(qsl("landmineHolder"), qsl("b")) == qsl("bval"), "the write went to the sibling the rename was refused over");
    }

    // A rename was generated Lua source with every key along the path spliced
    // into it inside a quoted literal, and a key holding a quote closes that
    // literal: neither line parsed, so the rename was dropped.
    void testRenameMovesAMemberWhoseKeyHoldsAQuote()
    {
        execLua(qsl("quoteKeyHolder = {} quoteKeyHolder['a\"b'] = 'keepme'"));
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("quoteKeyHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        TVar* member = members.constFirst();
        QCOMPARE(member->getName(), qsl("a\"b"));

        member->setNewName(qsl("plain"), LUA_TSTRING);
        pushSentinels();
        const int stackBefore = lua_gettop(L);
        QVERIFY(interface->renameVar(member));
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY2(sentinelsIntact(), "the rename unwound past the top it was handed");

        QCOMPARE(memberAsString(qsl("quoteKeyHolder"), qsl("plain")), qsl("keepme"));
        QCOMPARE(luaMemberCount(qsl("quoteKeyHolder")), 1);
    }

    // ...and the same for a key part way along the path, which the rename only
    // passes through on its way to the member being renamed.
    void testRenameReachesAMemberUnderATableWhoseKeyHoldsAQuote()
    {
        execLua(qsl("quotePathHolder = {} quotePathHolder['a\"b'] = {inner = 'keepme'}"));
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("quotePathHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        const QList<TVar*> inners = members.constFirst()->getChildren(false);
        QCOMPARE(inners.size(), 1);
        TVar* inner = inners.constFirst();
        QCOMPARE(inner->getName(), qsl("inner"));

        inner->setNewName(qsl("renamed"), LUA_TSTRING);
        QVERIFY(interface->renameVar(inner));

        lua_getglobal(L, "quotePathHolder");
        lua_pushstring(L, "a\"b");
        lua_gettable(L, -2);
        lua_pushstring(L, "renamed");
        lua_gettable(L, -2);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), qsl("keepme"));
        lua_pop(L, 1);
        lua_pushstring(L, "inner");
        lua_gettable(L, -2);
        QVERIFY2(lua_isnil(L, -1), "the name the member was renamed off has to be free");
        lua_pop(L, 3);
    }

    // The other half of splicing a key into source: a key that closes the
    // subscript it is written into leaves the rest of itself parsed as
    // statements, so a key a script stored - a line captured from the game, say
    // - ran as Lua when the member holding it was renamed.
    void testRenameDoesNotRunAKeyAsCode()
    {
        execLua(qsl("codeKeyHolder = {} codeKeyHolder['a\"]; evilRan = \"yes\" --'] = 'keepme'"));
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("codeKeyHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);

        members.constFirst()->setNewName(qsl("plain"), LUA_TSTRING);
        QVERIFY(interface->renameVar(members.constFirst()));

        lua_getglobal(L, "evilRan");
        const bool keyRan = !lua_isnil(L, -1);
        lua_pop(L, 1);
        QVERIFY2(!keyRan, "a key is data, and renaming the member it belongs to must not run it");
        QCOMPARE(memberAsString(qsl("codeKeyHolder"), qsl("plain")), qsl("keepme"));
        QCOMPARE(luaMemberCount(qsl("codeKeyHolder")), 1);
    }

    // ...and a key holding a backslash became an escape in that literal, so the
    // rename read one key and nil'ed another - the member it was asked about
    // stayed where it was, under a name the Variables view had stopped showing.
    void testRenameMovesAMemberWhoseKeyHoldsABackslash()
    {
        execLua(qsl("escapeKeyHolder = {} escapeKeyHolder['a\\\\b'] = 'keepme'"));
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("escapeKeyHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QCOMPARE(members.constFirst()->getName(), qsl("a\\b"));

        members.constFirst()->setNewName(qsl("plain"), LUA_TSTRING);
        QVERIFY(interface->renameVar(members.constFirst()));

        QCOMPARE(memberAsString(qsl("escapeKeyHolder"), qsl("plain")), qsl("keepme"));
        QCOMPARE(luaMemberCount(qsl("escapeKeyHolder")), 1);
    }

    // Renaming a global was written as _G["new"] = old, so it went through
    // whatever the name _G held at the time rather than through the globals
    // table itself - and a script is free to have pointed _G somewhere else.
    void testRenameOfAGlobalDoesNotGoThroughTheNameG()
    {
        execLua(qsl("_G = {} strayGlobal = 'keepme'"));
        interface->getVars(false);
        TVar* var = findGlobal(qsl("strayGlobal"));
        QVERIFY(var);

        var->setNewName(qsl("strayGlobalNew"), LUA_TSTRING);
        QVERIFY(interface->renameVar(var));

        QCOMPARE(globalAsString(qsl("strayGlobalNew")), qsl("keepme"));
        lua_getglobal(L, "strayGlobal");
        QVERIFY2(lua_isnil(L, -1), "the name the variable was renamed off has to be free");
        lua_pop(L, 1);
        QCOMPARE(luaMemberCount(qsl("_G")), 0);
    }

    // A table used as a key is only nameable through the registry reference the
    // tree holds it by, which is why renaming a member underneath one was the
    // one path that already went through the C API. It goes through the same
    // walk as every other path now.
    void testRenameReachesAMemberUnderATableKeyedIntermediate()
    {
        execLua("refRenameHolder = {} do local key = {} refRenameHolder[key] = {inner = 'keepme'} end");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("refRenameHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QVERIFY(members.constFirst()->isReference());
        const QList<TVar*> inners = members.constFirst()->getChildren(false);
        QCOMPARE(inners.size(), 1);
        TVar* inner = inners.constFirst();
        QCOMPARE(inner->getName(), qsl("inner"));

        inner->setNewName(qsl("renamed"), LUA_TSTRING);
        QVERIFY(interface->renameVar(inner));

        QCOMPARE(luaMemberCount(qsl("refRenameHolder")), 1);
        QCOMPARE(onlyMembersMemberAsString(qsl("refRenameHolder"), qsl("renamed")), qsl("keepme"));
        QCOMPARE(onlyMembersMemberAsString(qsl("refRenameHolder"), qsl("inner")), QString());
    }

    // The editor decides the key type from what the user typed, so the new key
    // is not always of the type the old one was. Written as the old type, a
    // name of "five" would be pushed as the number it does not parse to and
    // land the value on t[0].
    void testRenameChangesAMembersKeyType()
    {
        execLua("keyTypeHolder = {} keyTypeHolder[5] = 'keepme'");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("keyTypeHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QCOMPARE(members.constFirst()->getKeyType(), LUA_TNUMBER);

        members.constFirst()->setNewName(qsl("five"), LUA_TSTRING);
        QVERIFY(interface->renameVar(members.constFirst()));

        QCOMPARE(memberAsString(qsl("keyTypeHolder"), qsl("five")), qsl("keepme"));
        QCOMPARE(numberKeyedMemberAsString(qsl("keyTypeHolder"), 5), QString());
        QCOMPARE(luaMemberCount(qsl("keyTypeHolder")), 1);
    }

    // A boolean key is named "true" or "false" in the tree, so pushing it as
    // the text of that name reaches a string key of its own instead - the
    // rename would report success having moved nothing.
    void testRenameMovesABooleanKeyedMember()
    {
        execLua("boolKeyHolder = {} boolKeyHolder[true] = 'keepme'");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("boolKeyHolder"));
        QVERIFY(holder);
        const QList<TVar*> members = holder->getChildren(false);
        QCOMPARE(members.size(), 1);
        QCOMPARE(members.constFirst()->getKeyType(), LUA_TBOOLEAN);

        members.constFirst()->setNewName(qsl("plain"), LUA_TSTRING);
        QVERIFY(interface->renameVar(members.constFirst()));

        QCOMPARE(memberAsString(qsl("boolKeyHolder"), qsl("plain")), qsl("keepme"));
        QCOMPARE(luaMemberCount(qsl("boolKeyHolder")), 1);
    }

    // A refused rename is where the generated source left its parse error
    // behind: the chunk never ran, nothing popped it, and the profile's live
    // interpreter carried the slot for the rest of the session (#9885).
    void testARenameThatCannotReachTheVariableLeavesTheStackAsItFoundIt()
    {
        execLua("goneHolder = {inner = {member = 'keepme'}}");
        interface->getVars(false);
        TVar* holder = findGlobal(qsl("goneHolder"));
        QVERIFY(holder);
        const QList<TVar*> inners = holder->getChildren(false);
        QCOMPARE(inners.size(), 1);
        const QList<TVar*> members = inners.constFirst()->getChildren(false);
        QCOMPARE(members.size(), 1);
        TVar* member = members.constFirst();
        // a script replaced the table the tree walked, so the path no longer
        // reaches the member the rename was asked about
        execLua("goneHolder.inner = 5");

        member->setNewName(qsl("renamed"), LUA_TSTRING);
        pushSentinels();
        const int stackBefore = lua_gettop(L);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("could not reach")));
        QVERIFY(!interface->renameVar(member));
        QCOMPARE(lua_gettop(L), stackBefore);
        QVERIFY2(sentinelsIntact(), "the refused rename unwound past the top it was handed");
        QCOMPARE(member->getName(), qsl("member"));
    }

    // The three names below are the cycle a real table of mixed names produced:
    // every one of the three comparisons answered true, so TVar::getChildren()
    // handed std::sort() an ordering that contradicted itself (#9956).
    void testTVarLessThanPlacesNumbersAndNamesWithoutACycle()
    {
        const auto two = namedVar(qsl("2"));
        const auto ten = namedVar(qsl("10"));
        const auto elevenA = namedVar(qsl("11a"));

        QVERIFY2(TVarLessThan(two.get(), ten.get()), "two numbers compare by value, so 2 comes before 10");
        QVERIFY2(TVarLessThan(ten.get(), elevenA.get()), "a number comes before a name that is not one");
        QVERIFY2(!TVarLessThan(elevenA.get(), two.get()), "so that name cannot also come before a number - that closes a cycle");
    }

    void testTVarLessThanIsAntisymmetric()
    {
        const QStringList names{qsl("0"), qsl("2"), qsl("10"), qsl("-1"), qsl("11a"), qsl("0abc"), qsl("a"), qsl("A"), qsl("zebra"), QString()};
        for (const QString& first : names) {
            const auto firstVar = namedVar(first);
            QVERIFY2(!TVarLessThan(firstVar.get(), firstVar.get()), qPrintable(qsl("\"%1\" must not come before itself").arg(first)));
            for (const QString& second : names) {
                if (first == second) {
                    continue;
                }
                const auto secondVar = namedVar(second);
                const bool firstFirst = TVarLessThan(firstVar.get(), secondVar.get());
                const bool secondFirst = TVarLessThan(secondVar.get(), firstVar.get());
                QVERIFY2(!(firstFirst && secondFirst), qPrintable(qsl("\"%1\" and \"%2\" each came before the other").arg(first, second)));
            }
        }
    }

    // Folding the case leaves "A" and "a" equivalent, and equivalent names are
    // left wherever the sort happens to put them - which is not the same place
    // every time, so the view would reorder them for no reason the user made.
    void testTVarLessThanSeparatesNamesThatOnlyDifferInCase()
    {
        const auto upper = namedVar(qsl("A"));
        const auto lower = namedVar(qsl("a"));

        QCOMPARE(TVarLessThan(upper.get(), lower.get()), true);
        QCOMPARE(TVarLessThan(lower.get(), upper.get()), false);
    }

    // A strict weak ordering in full: both "comes before" and "neither comes
    // before the other" have to carry across a third name, or std::sort() is
    // free to read past the ends of the list it is sorting.
    void testTVarLessThanIsTransitiveOverEveryTripleOfMixedNames()
    {
        const QStringList names{
                qsl("0"), qsl("2"), qsl("10"), qsl("11"), qsl("-1"), qsl("-10"), qsl("2b"), qsl("11a"), qsl("a"), qsl("A"), qsl("_G"), qsl("zebra"), QString(), qsl("99999999999999999999")};
        std::vector<std::unique_ptr<TVar>> vars;
        vars.reserve(names.size());
        for (const QString& name : names) {
            vars.push_back(namedVar(name));
        }

        for (int i = 0; i < names.size(); ++i) {
            for (int j = 0; j < names.size(); ++j) {
                for (int k = 0; k < names.size(); ++k) {
                    TVar* first = vars.at(i).get();
                    TVar* second = vars.at(j).get();
                    TVar* third = vars.at(k).get();
                    const QString trio = qsl("\"%1\", \"%2\", \"%3\"").arg(names.at(i), names.at(j), names.at(k));
                    if (TVarLessThan(first, second) && TVarLessThan(second, third)) {
                        QVERIFY2(TVarLessThan(first, third), qPrintable(qsl("%1: the first must come before the third").arg(trio)));
                    }
                    const bool firstTwoTie = !TVarLessThan(first, second) && !TVarLessThan(second, first);
                    const bool lastTwoTie = !TVarLessThan(second, third) && !TVarLessThan(third, second);
                    if (firstTwoTie && lastTwoTie) {
                        const bool endsTie = !TVarLessThan(first, third) && !TVarLessThan(third, first);
                        QVERIFY2(endsTie, qPrintable(qsl("%1: names that tie with a third have to tie with each other").arg(trio)));
                    }
                }
            }
        }
    }

    // A global is free to have a dot in its own name, and everything the
    // Variables view remembers about a variable it remembers by the dotted path
    // the tree gives it - so such a global reads exactly like a member of a
    // table that happens to be named the same way. Both are real variables of
    // their own, and both have to be in the tree with their own values (#9954).
    void testADottedGlobalAndTheMatchingMemberAreBothInTheTree()
    {
        defineGlobalsTable();
        execLua("colT = {b = 5} _G['colT.b'] = 'unrelated'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();

        TVar* dottedRoot = findGlobal(qsl("colT.b"));
        QVERIFY2(dottedRoot, "a global whose own name holds a dot has to be in the tree");
        QCOMPARE(dottedRoot->getValue(), qsl("unrelated"));
        QVERIFY2(!vu->isHidden(dottedRoot), "and nothing has hidden it");

        TVar* table = findGlobal(qsl("colT"));
        QVERIFY(table);
        const QList<TVar*> members = table->getChildren(false);
        QCOMPARE(members.size(), 1);
        QCOMPARE(members.first()->getName(), qsl("b"));
        QCOMPARE(members.first()->getValue(), qsl("5"));
        QVERIFY2(members.first() != dottedRoot, "the member and the global of that name are two different nodes");
    }

    // The way a user meets this: the hiding walk at profile load records the
    // member path "apiHolder.load", and a global of that name made afterwards
    // then answered to the mark left for the member - so it was born hidden and
    // never appeared in the Variables view.
    void testAGlobalMadeAfterAHideWalkIsNotHiddenByAMatchingMemberPath()
    {
        defineGlobalsTable();
        execLua("apiHolder = {load = 5}");
        interface->getVars(true); // the hiding walk Mudlet runs at profile load
        VarUnit* vu = interface->getVarUnit();
        QVERIFY2(vu->hidden.contains(qsl("apiHolder.load")), "the walk is supposed to record the member path");

        execLua("_G['apiHolder.load'] = 7");
        interface->getVars(false);

        TVar* dottedRoot = findGlobal(qsl("apiHolder.load"));
        QVERIFY(dottedRoot);
        QVERIFY2(!vu->isHidden(dottedRoot), "a global made after the hiding walk must not inherit a member path's hiding");
        QCOMPARE(dottedRoot->getValue(), qsl("7"));

        TVar* member = findGlobal(qsl("apiHolder"))->getChildren(false).first();
        QCOMPARE(member->getName(), qsl("load"));
        QVERIFY2(vu->isHidden(member), "the member the walk did hide has to stay hidden");
    }

    // ...and the same collision the other way round: the saved mark the user put
    // on the member is keyed by that same dotted path, so the unrelated global
    // read as saved too - and a profile save wrote it out in the member's place.
    void testSavingAMemberDoesNotSaveTheGlobalOfThatDottedName()
    {
        defineGlobalsTable();
        execLua("cfgT = {port = 23} _G['cfgT.port'] = 'not the member'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();

        TVar* member = findGlobal(qsl("cfgT"))->getChildren(false).first();
        QCOMPARE(member->getName(), qsl("port"));
        vu->addSavedVar(member);

        QVERIFY2(vu->isSaved(member), "the member the user ticked is saved");
        TVar* dottedRoot = findGlobal(qsl("cfgT.port"));
        QVERIFY(dottedRoot);
        QVERIFY2(!vu->isSaved(dottedRoot), "an unrelated global must not be saved by the member's mark");
    }

    // Nor may going the other way take the member's mark away: clicking such a
    // global's row in the Variables view removed the savedVars entry the user
    // had made about the member.
    void testTouchingTheDottedGlobalLeavesTheMembersSavedMarkAlone()
    {
        defineGlobalsTable();
        execLua("keepT = {port = 23} _G['keepT.port'] = 'not the member'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* member = findGlobal(qsl("keepT"))->getChildren(false).first();
        vu->addSavedVar(member);
        TVar* dottedRoot = findGlobal(qsl("keepT.port"));
        QVERIFY(dottedRoot);

        vu->addSavedVar(dottedRoot);
        vu->removeSavedVar(dottedRoot);

        QVERIFY2(vu->savedVars.contains(qsl("keepT.port")), "the member's saved mark is the user's and has to survive");
        QVERIFY2(vu->isSaved(member), "so the member is still saved");
    }

    // Saving is fenced off for such a global rather than made to work: savedVars
    // is keyed by the dotted path, and a name that reads as a member path cannot
    // be told apart from one in it - so a save would restore the wrong variable.
    void testADottedGlobalIsFencedOffFromBeingSaved()
    {
        defineGlobalsTable();
        execLua("fenceT = {port = 23} _G['fenceT.port'] = 'not the member' plainGlobal = 'saveable'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();

        TVar* dottedRoot = findGlobal(qsl("fenceT.port"));
        QVERIFY(dottedRoot);
        QVERIFY2(!vu->shouldSave(dottedRoot), "a global whose own name holds a dot cannot be saved");
        QVERIFY2(!vu->getUnsaveableReason(dottedRoot).isEmpty(), "and the view has to say why");
        QVERIFY(vu->getUnsaveableReason(dottedRoot).contains(qsl("dot")));

        QVERIFY2(vu->shouldSave(findGlobal(qsl("plainGlobal"))), "a global with no dot in its name is saveable as before");
        QVERIFY2(vu->shouldSave(findGlobal(qsl("fenceT"))->getChildren(false).first()), "and so is the member whose path reads the same");
        QVERIFY2(vu->getUnsaveableReason(findGlobal(qsl("fenceT"))->getChildren(false).first()).isEmpty(), "the member has nothing to explain away");
    }

    // The fence is about the name a root is reached by, so it must not reach a
    // member whose own key holds a dot - that member is found through its table.
    void testAMemberWhoseOwnKeyHoldsADotIsStillSaveable()
    {
        defineGlobalsTable();
        execLua("dotHolder = {} dotHolder['a.b'] = 'member value'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();

        TVar* member = findGlobal(qsl("dotHolder"))->getChildren(false).first();
        QCOMPARE(member->getName(), qsl("a.b"));
        QVERIFY2(vu->shouldSave(member), "a member of a table is reached through that table, dot in its key or not");
        vu->addSavedVar(member);
        QVERIFY(vu->isSaved(member));
        QVERIFY(vu->savedVars.contains(qsl("dotHolder.a.b")));
    }

    // A member's key can itself hold a dot, so "gt.a.b" names both a member
    // "a.b" of gt and a member b of gt.a - the rename must move marks for what
    // is really inside the renamed variable, not for whatever path reads as if
    // it were.
    void testRenameDoesNotMoveADottedKeySiblingsMark()
    {
        defineGlobalsTable();
        execLua("gt = {a = 5} gt['a.b'] = 'sibling with a dotted key'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* holder = findGlobal(qsl("gt"));
        QVERIFY(holder);
        TVar* dottedSibling = nullptr;
        TVar* plainA = nullptr;
        for (TVar* kid : holder->getChildren(false)) {
            if (kid->getName() == qsl("a.b")) {
                dottedSibling = kid;
            } else if (kid->getName() == qsl("a")) {
                plainA = kid;
            }
        }
        QVERIFY(dottedSibling && plainA);
        vu->addSavedVar(dottedSibling); // savedVars = {"gt.a.b"} - the sibling's mark
        QVERIFY(vu->savedVars.contains(qsl("gt.a.b")));

        plainA->setNewName(qsl("x"), LUA_TSTRING);
        QVERIFY(interface->renameVar(plainA));
        qDebug() << "savedVars after renaming gt.a:" << vu->savedVars;
        QVERIFY2(vu->savedVars.contains(qsl("gt.a.b")), "the dotted-key sibling's mark must not be dragged along by a rename of gt.a");
        QVERIFY2(!vu->savedVars.contains(qsl("gt.x.b")), "no mark may be invented for a name nothing has");
    }

private:
    static std::unique_ptr<TVar> namedVar(const QString& name)
    {
        auto var = std::make_unique<TVar>();
        var->setName(name, LUA_TSTRING);
        return var;
    }

    // The base library, which these tests do without, is what usually puts the
    // globals table in _G, and a global of a name only a subscript can write is
    // made through it.
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

    QString numberKeyedMemberAsString(const QString& tableName, const double key)
    {
        lua_getglobal(L, tableName.toUtf8().constData());
        lua_pushnumber(L, key);
        lua_gettable(L, -2);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 2);
        return value;
    }

    // walked with lua_next() rather than pairs(), which these tests do without
    // the base library to provide
    int luaMemberCount(const QString& tableName)
    {
        lua_getglobal(L, tableName.toUtf8().constData());
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return -1;
        }
        int count = 0;
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            ++count;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        return count;
    }

    // ...and the same for reading a member of the sole member of a table, whose
    // key is a table of its own and so has no name to look it up by
    QString onlyMembersMemberAsString(const QString& tableName, const QString& memberName)
    {
        lua_getglobal(L, tableName.toUtf8().constData());
        lua_pushnil(L);
        if (!lua_next(L, -2)) {
            lua_pop(L, 1);
            return QString();
        }
        lua_pushstring(L, memberName.toUtf8().constData());
        lua_gettable(L, -2);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 4);
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
