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
 * Tests for XMLexport::writeVariablePackage(): variables created after the
 * VarUnit tree was last built (e.g. by scripts at runtime) must still be
 * written to the profile XML when they are marked as saved. The tree is
 * only (re)built at profile load and when the Variables view is populated,
 * so without a refresh at export time such variables silently vanish from
 * profile saves. Also covers members a script adds to a saved table at
 * runtime: they have no savedVars entry of their own but must be saved with
 * the table (issue #9517), while hidden and unsaveable members must not be.
 * Also covers a saved table that other globals reference, which must export in
 * full under every saved name that reaches it (issue #9755), and the boundary of
 * the fence that stops a table holding a function exporting in part (issue
 * #9857) - SavedVariableFenceTest covers that one in full. Then the two halves
 * of issue #9769 a save can show: a value the export cannot read a second time,
 * and a saved table holding one of Mudlet's own.
 *
 * Run with: ctest -R XMLexportVariablesTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "LuaInterface.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "VarUnit.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include <QRegularExpression>
#include <QTreeWidget>

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

class XMLexportVariablesTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mHostname = "XMLexportVars-Test";
    const QString mLocalhost = "localhost";

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        // port 0 asks the OS for an ephemeral port, so parallel test runs
        // (and other worktrees) cannot collide on a fixed one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "TelnetServerStub failed to bind a loopback port");
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A saved variable whose Lua value only comes into existence after the
    // variable tree was last built (profile load, Variables view opening)
    // must still be written out - the save path has to refresh the tree.
    void test_lateCreatedSavedVariableIsExported()
    {
        // QTest runs slots in declaration order and these stand for a profile
        // whose Variables view was never opened. Profile load builds the editor
        // dialog itself, so what matters is that no slot has shown it yet.
        QVERIFY2(!mpEditor, "a Variables-view test was declared before the ones that must run without it");
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        // build the tree directly, standing in for the initial build that
        // profile load performs (via Host::hideMudletsVariables())
        lI->getVars(false);
        QVERIFY(vu->getBase());

        // a script creates the variable after that; we mark its name as saved
        // to emulate a variable persisted in a previous session (savedVars is
        // name-keyed and persistent, so it survives a tree rebuild)
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "lateSavedTestVar = 'created after tree build'"), 0);
        vu->savedVars.insert(qsl("lateSavedTestVar"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("lateSavedTestVar")),
                 "saved variable created after the last variable-tree build should "
                 "still be exported to the profile XML");
        // the value is the payload of the save - make sure it is written, not
        // just an empty node with the right name
        QVERIFY2(xml.contains(qsl("created after tree build")), "the saved variable's value must be exported, not just its name");

        // mpHost is shared across the tests, so undo the state this one added
        vu->savedVars.remove(qsl("lateSavedTestVar"));
        QCOMPARE(luaL_dostring(L, "lateSavedTestVar = nil"), 0);
    }

    // The export-time refresh must not start saving variables that are not
    // marked as saved.
    void test_lateUnsavedVariableIsNotExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        lI->getVars(false);

        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "lateUnsavedTestVar = 'not marked saved'"), 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(!xml.contains(qsl("lateUnsavedTestVar")), "a variable not marked as saved must not be exported");
    }

    // A member a script adds to a saved table at runtime has no savedVars
    // entry of its own, but must still be saved with the table (issue #9517).
    void test_runtimeAddedMemberOfSavedTableIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "memberTestTable = {existing = 'existing member value'}"), 0);
        // ticking a table in the Variables view registers the table and the
        // members that exist at that moment
        vu->savedVars.insert(qsl("memberTestTable"));
        vu->savedVars.insert(qsl("memberTestTable.existing"));
        lI->getVars(false);

        // a script adds another member after that
        QCOMPARE(luaL_dostring(L, "memberTestTable.newcomer = 'runtime member value'"), 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("existing member value")), "member registered when the table was ticked must still be exported");
        QVERIFY2(xml.contains(qsl("runtime member value")), "member added to a saved table at runtime must be saved with the table");

        vu->savedVars.remove(qsl("memberTestTable"));
        vu->savedVars.remove(qsl("memberTestTable.existing"));
        QCOMPARE(luaL_dostring(L, "memberTestTable = nil"), 0);
    }

    // A nested table assigned into a saved table at runtime must be exported
    // recursively, right down to its innermost members.
    void test_nestedTableAddedToSavedTableIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "nestedTestTable = {}"), 0);
        vu->savedVars.insert(qsl("nestedTestTable"));
        lI->getVars(false);

        QCOMPARE(luaL_dostring(L, "nestedTestTable.inner = {deepest = 'nested member value'}"), 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("nested member value")), "members of a nested table added to a saved table at runtime must be exported");

        vu->savedVars.remove(qsl("nestedTestTable"));
        QCOMPARE(luaL_dostring(L, "nestedTestTable = nil"), 0);
    }

    // The most common shape of issue #9517: a list-style table grown with
    // table.insert at runtime. The numeric key must keep its key type so
    // import restores t[1] and not t["1"].
    void test_numericKeyMemberAddedAtRuntimeIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "numericListTable = {}"), 0);
        vu->savedVars.insert(qsl("numericListTable"));
        lI->getVars(false);

        QCOMPARE(luaL_dostring(L, "table.insert(numericListTable, 'numeric member value')"), 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("numeric member value")), "a numeric-keyed member added at runtime must be saved with its table");
        // LUA_TNUMBER == 3: the key type decides whether import restores t[1] or t["1"]
        QVERIFY2(xml.contains(qsl("<keyType>3</keyType>")), "the numeric member's key type must be numeric so import restores t[1], not t['1']");

        vu->savedVars.remove(qsl("numericListTable"));
        QCOMPARE(luaL_dostring(L, "numericListTable = nil"), 0);
    }

    // Design pin: un-ticking a single member in the Variables view only
    // removes its name from savedVars, which cannot be told apart from a
    // member added after the table was ticked. A saved table therefore
    // exports its members as they exist at save time; to keep a member out
    // of the profile, hide it, remove it, or stop saving the table.
    void test_untickedMemberOfSavedTableStillExports()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "untickedMemberTable = {kept = 'kept member value', unticked = 'unticked member value'}"), 0);
        // ticking the table registers it and both members...
        vu->savedVars.insert(qsl("untickedMemberTable"));
        vu->savedVars.insert(qsl("untickedMemberTable.kept"));
        vu->savedVars.insert(qsl("untickedMemberTable.unticked"));
        // ...and un-ticking one member only removes its name again
        vu->savedVars.remove(qsl("untickedMemberTable.unticked"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("kept member value")), "a ticked member of a saved table must be exported");
        QVERIFY2(xml.contains(qsl("unticked member value")), "a saved table exports members as they exist at save time, so an un-ticked member rides along");

        vu->savedVars.remove(qsl("untickedMemberTable"));
        vu->savedVars.remove(qsl("untickedMemberTable.kept"));
        QCOMPARE(luaL_dostring(L, "untickedMemberTable = nil"), 0);
    }

    // A global is free to hold a dot in its own name, and savedVars is keyed by
    // the dotted path - so such a global reads exactly like a member of a table
    // of that path. The save used to write that global out under the member's
    // entry, which handed the next session the wrong variable's value (#9954).
    void test_globalWithADotInItsNameIsNotExportedUnderAMembersSavedName()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L,
                               "dottedNameTable = {member = 'the member value'} "
                               "_G['dottedNameTable.member'] = 'the unrelated global value'"),
                 0);
        // what ticking the table and its member in the Variables view records
        vu->savedVars.insert(qsl("dottedNameTable"));
        vu->savedVars.insert(qsl("dottedNameTable.member"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("the member value")), "the member the user ticked has to be exported");
        QVERIFY2(!xml.contains(qsl("the unrelated global value")), "the unrelated global of that dotted name must not be exported in its place");

        vu->savedVars.remove(qsl("dottedNameTable"));
        vu->savedVars.remove(qsl("dottedNameTable.member"));
        QCOMPARE(luaL_dostring(L, "dottedNameTable = nil _G['dottedNameTable.member'] = nil"), 0);
    }

    // A member table beyond the 10,000-item save limit must not ride along -
    // it would bloat every profile save.
    void test_oversizedMemberTableIsNotExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L,
                               "oversizedHolderTable = {smallMember = 'small member value', bigMember = {}} "
                               "for i = 1, 10001 do oversizedHolderTable.bigMember[i] = 'oversized member value' end"),
                 0);
        vu->savedVars.insert(qsl("oversizedHolderTable"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("small member value")), "a plain member of a saved table must be exported");
        QVERIFY2(!xml.contains(qsl("oversized member value")), "a member table over the 10,000-item limit must not ride along with its saved table");

        vu->savedVars.remove(qsl("oversizedHolderTable"));
        QCOMPARE(luaL_dostring(L, "oversizedHolderTable = nil"), 0);
    }

    // A member whose key is a reference (e.g. a table used as a key) cannot
    // be restored from XML and must not ride along.
    void test_referenceKeyMemberIsNotExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "referenceKeyTable = {plainMember = 'plain member value'} referenceKeyTable[{}] = 'reference member value'"), 0);
        vu->savedVars.insert(qsl("referenceKeyTable"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("plain member value")), "a plain member of a saved table must be exported");
        QVERIFY2(!xml.contains(qsl("reference member value")), "a reference-keyed member must not ride along with its saved table");

        vu->savedVars.remove(qsl("referenceKeyTable"));
        QCOMPARE(luaL_dostring(L, "referenceKeyTable = nil"), 0);
    }

    // Members only ride along with tables that are marked saved.
    void test_memberOfUnsavedTableIsNotExported()
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "unsavedTestTable = {member = 'unsaved member value'}"), 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(!xml.contains(qsl("unsavedTestTable")), "a table not marked as saved must not be exported");
        QVERIFY2(!xml.contains(qsl("unsaved member value")), "members of a table not marked as saved must not be exported");

        QCOMPARE(luaL_dostring(L, "unsavedTestTable = nil"), 0);
    }

    // Hidden variables (Mudlet's internals, or ones the user hid) inside a
    // saved table keep needing their own explicit save mark, so internals
    // cannot leak into the profile XML through a saved parent.
    void test_hiddenMemberOfSavedTableIsNotExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "hiddenMemberTable = {visibleMember = 'visible member value', secretMember = 'secret member value'}"), 0);
        vu->savedVars.insert(qsl("hiddenMemberTable"));
        vu->addHidden(qsl("hiddenMemberTable.secretMember"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("visible member value")), "a plain member of a saved table must be exported");
        QVERIFY2(!xml.contains(qsl("secret member value")), "a hidden member must not ride along with its saved table");

        vu->savedVars.remove(qsl("hiddenMemberTable"));
        vu->removeHidden(qsl("hiddenMemberTable.secretMember"));
        QCOMPARE(luaL_dostring(L, "hiddenMemberTable = nil"), 0);
    }

    // A hidden member the user explicitly ticked stays exported - hiding only
    // blocks the ride-along, not an explicit save mark.
    void test_explicitlySavedHiddenMemberIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "explicitHiddenTable = {pinnedMember = 'pinned member value'}"), 0);
        vu->savedVars.insert(qsl("explicitHiddenTable"));
        vu->savedVars.insert(qsl("explicitHiddenTable.pinnedMember"));
        vu->addHidden(qsl("explicitHiddenTable.pinnedMember"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("pinned member value")), "a hidden member explicitly marked as saved must still be exported");

        vu->savedVars.remove(qsl("explicitHiddenTable"));
        vu->savedVars.remove(qsl("explicitHiddenTable.pinnedMember"));
        vu->removeHidden(qsl("explicitHiddenTable.pinnedMember"));
        QCOMPARE(luaL_dostring(L, "explicitHiddenTable = nil"), 0);
    }

    // Function members cannot be saved, and a table holding one therefore cannot
    // be written out as it stands: the ride-along is off for the whole variable,
    // which for a table ticked while empty leaves an empty group (#9857).
    // SavedVariableFenceTest covers that in full.
    void test_functionMemberBlocksTheRideAlong()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "callableHolderTable = {dataMember = 'data member value', callableMember = function() end}"), 0);
        vu->savedVars.insert(qsl("callableHolderTable"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("<name>callableHolderTable</name>")), "the saved table itself must still be exported, or the next session sees nil rather than an empty table");
        QVERIFY2(!xml.contains(qsl("dataMember")), "a table holding a function exports the members registered in savedVars and no others");
        QVERIFY2(!xml.contains(qsl("callableMember")), "a function member must not ride along with its saved table");

        vu->savedVars.remove(qsl("callableHolderTable"));
        QCOMPARE(luaL_dostring(L, "callableHolderTable = nil"), 0);
    }

    // The export-time refresh must keep writing the user's hidden-variable
    // preferences to the HiddenVariables node.
    void test_hiddenPreferenceStillExported()
    {
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        vu->addHidden(qsl("userHiddenPrefVar"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("userHiddenPrefVar")), "hiddenByUser names must still be written to HiddenVariables");

        vu->removeHidden(qsl("userHiddenPrefVar"));
    }

    // VarUnit has two hidden sets: hiddenByUser, and hidden, which
    // Host::hideMudletsVariables() fills with Mudlet's own Lua API. Both have to
    // reach the export's tree or a saved table drags the internals into the XML.
    void test_internallyHiddenMemberOfSavedTableIsNotExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "internalHiddenTable = {plainMember = 'plain member value', internalMember = 'internal member value'}"), 0);
        vu->savedVars.insert(qsl("internalHiddenTable"));
        // what addHidden(TVar*, 0) records - the non-user half of the pair
        vu->hidden.insert(qsl("internalHiddenTable.internalMember"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("plain member value")), "a plain member of a saved table must be exported");
        QVERIFY2(!xml.contains(qsl("internal member value")), "a member hidden by Mudlet itself must not ride along with its saved table");

        vu->savedVars.remove(qsl("internalHiddenTable"));
        vu->hidden.remove(qsl("internalHiddenTable.internalMember"));
        QCOMPARE(luaL_dostring(L, "internalHiddenTable = nil"), 0);
    }

    // A variable tree takes a Lua registry reference per reference-keyed entry.
    // The export throws its tree away, so if the references went with it the
    // registry would grow by that many slots on every save.
    void test_exportDoesNotLeakLuaRegistryReferences()
    {
        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        // several reference-keyed members, so a leak grows the registry visibly.
        // The table has to be saved for the export to read it at all, and each
        // of those keys costs a registry reference while it does
        QCOMPARE(luaL_dostring(L, "refKeyLeakTable = {} for i = 1, 20 do refKeyLeakTable[{}] = i end"), 0);
        vu->savedVars.insert(qsl("refKeyLeakTable"));

        // freed slots go on a free list and come straight back out, so the
        // number stops climbing once the registry fits one pass's worth.
        // Measuring after the first export leaves that one-off growth out.
        QVERIFY(!exportProfileXml().isEmpty());
        lua_pushboolean(L, 1);
        const int refAfterOne = luaL_ref(L, LUA_REGISTRYINDEX);
        luaL_unref(L, LUA_REGISTRYINDEX, refAfterOne);

        for (int i = 0; i < 5; ++i) {
            QVERIFY(!exportProfileXml().isEmpty());
        }

        lua_pushboolean(L, 1);
        const int refAfterSix = luaL_ref(L, LUA_REGISTRYINDEX);
        luaL_unref(L, LUA_REGISTRYINDEX, refAfterSix);

        // five more exports keeping 20 references each would put this 100 higher
        QVERIFY2(refAfterSix < refAfterOne + 20,
                 qPrintable(qsl("the exports pinned Lua registry slots: a reference taken after one export was %1, one taken after six was %2").arg(refAfterOne).arg(refAfterSix)));

        vu->savedVars.remove(qsl("refKeyLeakTable"));
        QCOMPARE(luaL_dostring(L, "refKeyLeakTable = nil"), 0);
    }

    // A saved table other globals also reference exports in full, whichever of
    // the names anything else happens to reach it by (issue #9755).
    void test_savedTableReachedByAnotherGlobalIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "aliasedSavedTable = {member = 'aliased member value'}"), 0);
        vu->savedVars.insert(qsl("aliasedSavedTable"));
        vu->savedVars.insert(qsl("aliasedSavedTable.member"));
        // seven aliases, not one: should the export ever go back to walking all
        // of _G, hash order picks which name wins, and one alias would then only
        // fail this test some of the time
        QCOMPARE(luaL_dostring(L,
                               "aaaAliasOfSaved = aliasedSavedTable bAliasOfSaved = aliasedSavedTable m1AliasOfSaved = aliasedSavedTable "
                               "xyzzyAliasOfSaved = aliasedSavedTable alphaAliasOfSaved = aliasedSavedTable ref1AliasOfSaved = aliasedSavedTable "
                               "A_1AliasOfSaved = aliasedSavedTable"),
                 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("aliasedSavedTable")), "a saved table must be exported however many other globals also reference it");
        QVERIFY2(xml.contains(qsl("aliased member value")), "the members of a saved table other globals reference must be exported too");
        QVERIFY2(!xml.contains(qsl("AliasOfSaved")), "the other globals are not saved themselves, so they must not be exported");

        vu->savedVars.remove(qsl("aliasedSavedTable"));
        vu->savedVars.remove(qsl("aliasedSavedTable.member"));
        QCOMPARE(luaL_dostring(L, "aliasedSavedTable, aaaAliasOfSaved, bAliasOfSaved, m1AliasOfSaved, xyzzyAliasOfSaved, alphaAliasOfSaved, ref1AliasOfSaved, A_1AliasOfSaved = nil"), 0);
    }

    // Two saved globals that are the same table. Neither may be reduced to an
    // empty group by the other having been read first.
    void test_twoSavedGlobalsSharingATableBothExport()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "sharedFirstTable = {member = 'shared member value'} sharedSecondTable = sharedFirstTable"), 0);
        for (const auto& name : {qsl("sharedFirstTable"), qsl("sharedFirstTable.member"), qsl("sharedSecondTable"), qsl("sharedSecondTable.member")}) {
            vu->savedVars.insert(name);
        }

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("sharedFirstTable")), "the first of two saved globals sharing a table must be exported");
        QVERIFY2(xml.contains(qsl("sharedSecondTable")), "the second of two saved globals sharing a table must be exported");
        QCOMPARE(xml.count(qsl("shared member value")), 2);

        for (const auto& name : {qsl("sharedFirstTable"), qsl("sharedFirstTable.member"), qsl("sharedSecondTable"), qsl("sharedSecondTable.member")}) {
            vu->savedVars.remove(name);
        }
        QCOMPARE(luaL_dostring(L, "sharedFirstTable, sharedSecondTable = nil"), 0);
    }

    // The shape a stock profile hits without anyone aliasing anything: a saved
    // table that a second saved table holds as a member, which is how the EMCO
    // and AdjustableContainer packages reach each other's tables.
    void test_savedTableHeldByAnotherSavedTableIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "innerSavedTable = {member = 'inner member value'} holderSavedTable = {inner = innerSavedTable}"), 0);
        for (const auto& name : {qsl("innerSavedTable"), qsl("innerSavedTable.member"), qsl("holderSavedTable"), qsl("holderSavedTable.inner")}) {
            vu->savedVars.insert(name);
        }

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("innerSavedTable")), "a saved table another saved table holds must still be exported in its own right");
        QCOMPARE(xml.count(qsl("inner member value")), 2);

        for (const auto& name : {qsl("innerSavedTable"), qsl("innerSavedTable.member"), qsl("holderSavedTable"), qsl("holderSavedTable.inner")}) {
            vu->savedVars.remove(name);
        }
        QCOMPARE(luaL_dostring(L, "innerSavedTable, holderSavedTable = nil"), 0);
    }

    // Design pin. Two members of one saved table that are the same Lua table
    // are not both exported: the profile XML has no way to say "these two names
    // are one table", so the export would have to write the subtree once per
    // name. On a profile whose saved table holds a UI object that multiplies
    // out - measured at 5.5x the file, and big enough for the 10,000-item limit
    // to then drop whole branches - so the walk keeps one copy per saved name
    // and no more. Members the user ticks individually are exported in full.
    void test_twoMembersOfASavedTableSharingATableExportOnce()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "memberShareTable = {} memberShareTable.first = {member = 'member share value'} memberShareTable.second = memberShareTable.first"), 0);
        vu->savedVars.insert(qsl("memberShareTable"));

        QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(xml.count(qsl("member share value")), 1);

        // ticking a member in the Variables view (which is what savedVars holds)
        // is what asks for it to be written under its own name as well
        vu->savedVars.insert(qsl("memberShareTable.first"));
        vu->savedVars.insert(qsl("memberShareTable.second"));
        xml = exportProfileXml();
        QCOMPARE(xml.count(qsl("member share value")), 2);

        for (const auto& name : {qsl("memberShareTable"), qsl("memberShareTable.first"), qsl("memberShareTable.second")}) {
            vu->savedVars.remove(name);
        }
        QCOMPARE(luaL_dostring(L, "memberShareTable = nil"), 0);
    }

    // A table that holds itself must not send the walk round for ever.
    void test_selfReferencingSavedTableIsExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "cyclicSavedTable = {member = 'cyclic member value'} cyclicSavedTable.self = cyclicSavedTable"), 0);
        vu->savedVars.insert(qsl("cyclicSavedTable"));
        vu->savedVars.insert(qsl("cyclicSavedTable.self"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("cyclic member value")), "a table that references itself must still export its other members");

        vu->savedVars.remove(qsl("cyclicSavedTable"));
        vu->savedVars.remove(qsl("cyclicSavedTable.self"));
        QCOMPARE(luaL_dostring(L, "cyclicSavedTable = nil"), 0);
    }

    // What keeps a save off the size of _G: the tree it builds holds the saved
    // globals and nothing else, so a global a profile does not save costs a key
    // conversion and a hash lookup instead of a walk of everything it reaches.
    void test_saveTimeTreeHoldsOnlyTheSavedGlobals()
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L,
                               "for i = 1, 500 do _G['walkNoiseTable' .. i] = {nested = {deeper = i}} end "
                               "walkSavedRootTable = {member = 'walk member value'}"),
                 0);

        LuaInterface saveTimeInterface(L);
        saveTimeInterface.getVarUnit()->savedVars.insert(qsl("walkSavedRootTable"));
        saveTimeInterface.getSavedVars();

        TVar* pBase = saveTimeInterface.getVarUnit()->getBase();
        QVERIFY(pBase);
        const QList<TVar*> roots = pBase->getChildren(false);
        QCOMPARE(roots.size(), 1);
        QCOMPARE(roots.constFirst()->getName(), qsl("walkSavedRootTable"));
        QCOMPARE(roots.constFirst()->getChildren(false).size(), 1);

        // the saved-globals-only mode must not outlive the call that asked for
        // it, or the Variables view built from this interface would show almost
        // nothing
        saveTimeInterface.getVars(false);
        QVERIFY2(saveTimeInterface.getVarUnit()->getBase()->getChildren(false).size() > 1, "a getVars() after getSavedVars() must go back to reading the whole of _G");
        saveTimeInterface.releaseVariableReferences();

        QCOMPARE(luaL_dostring(L, "for i = 1, 500 do _G['walkNoiseTable' .. i] = nil end walkSavedRootTable = nil"), 0);
    }

    // The export borrows the profile's live Lua state, so anything it leaves on
    // the stack is charged to every trigger, alias and timer for the rest of the
    // session. Covers the three ways out of the walk: nothing saved at all, a
    // normal walk, and one cut short by the nesting limit.
    void test_exportLeavesTheLuaStackAsItFoundIt()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();

        const QSet<QString> savedVarsBefore = vu->savedVars;
        vu->savedVars.clear();
        const int stackWithNothingSaved = lua_gettop(L);
        const QString emptyXml = exportProfileXml();
        QVERIFY(!emptyXml.isEmpty());
        QVERIFY2(!emptyXml.contains(qsl("<Variable>")), "with nothing marked as saved the VariablePackage must come out empty");
        QCOMPARE(lua_gettop(L), stackWithNothingSaved);
        vu->savedVars = savedVarsBefore;

        QCOMPARE(luaL_dostring(L,
                               "stackCheckTable = {member = 'stack check value', nested = {deeper = 'deeper stack value'}} "
                               "local t = stackCheckTable "
                               "for i = 1, 120 do t.nested = {} t = t.nested end"),
                 0);
        vu->savedVars.insert(qsl("stackCheckTable"));

        const int stackBefore = lua_gettop(L);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("nested more than 99 tables deep")));
        QVERIFY(!exportProfileXml().isEmpty());
        QCOMPARE(lua_gettop(L), stackBefore);

        vu->savedVars.remove(qsl("stackCheckTable"));
        QCOMPARE(luaL_dostring(L, "stackCheckTable = nil"), 0);
    }

    // A member whose key the walk can name but cannot look back up takes the
    // export's read of its value out through a failure exit, which has to unwind
    // the profile's live stack like every other one. One save's worth of these
    // left behind is already past the room the C API guarantees, so it overruns
    // that stack rather than merely growing it; the repeated saves are what
    // would catch a smaller leak (#9885).
    void test_exportOfUnlookupableKeysLeavesTheLuaStackAsItFoundIt()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L,
                               "unlookupableKeyTable = {plain = 'plain member value'} "
                               "unlookupableKeyTable[string.char(0xff)] = 'byte keyed value' "
                               "for i = 1, 200 do unlookupableKeyTable[i + 1/3] = 'fractional key value ' .. i end"),
                 0);
        vu->savedVars.insert(qsl("unlookupableKeyTable"));

        const int stackBefore = lua_gettop(L);
        for (int i = 0; i < 6; ++i) {
            const QString xml = exportProfileXml();
            // the members that can be read still have to export, or an
            // over-eager unwind would pass this test by exporting nothing
            QVERIFY2(xml.contains(qsl("plain member value")), "a member the export can read must still be written out alongside the ones it cannot");
            QCOMPARE(lua_gettop(L), stackBefore);
        }

        vu->savedVars.remove(qsl("unlookupableKeyTable"));
        QCOMPARE(luaL_dostring(L, "unlookupableKeyTable = nil"), 0);
    }

    // A table-keyed entry costs a Lua registry reference to name at all, and the
    // walk takes that reference before it knows whether the global is saved. The
    // ones it then skips still have to be handed back.
    void test_skippedGlobalsDoNotPinLuaRegistryReferences()
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "for i = 1, 20 do _G[{}] = 'unsaved table-keyed global' end"), 0);

        QVERIFY(!exportProfileXml().isEmpty());
        lua_pushboolean(L, 1);
        const int refAfterOne = luaL_ref(L, LUA_REGISTRYINDEX);
        luaL_unref(L, LUA_REGISTRYINDEX, refAfterOne);

        for (int i = 0; i < 5; ++i) {
            QVERIFY(!exportProfileXml().isEmpty());
        }

        lua_pushboolean(L, 1);
        const int refAfterSix = luaL_ref(L, LUA_REGISTRYINDEX);
        luaL_unref(L, LUA_REGISTRYINDEX, refAfterSix);

        QVERIFY2(refAfterSix < refAfterOne + 20,
                 qPrintable(qsl("the exports pinned Lua registry slots for globals they skipped: a reference taken after one export was %1, one taken after six was %2")
                                    .arg(refAfterOne)
                                    .arg(refAfterSix)));

        QCOMPARE(luaL_dostring(L,
                               "local deadKeys = {} for k in pairs(_G) do if type(k) == 'table' then deadKeys[#deadKeys + 1] = k end end "
                               "for _, k in ipairs(deadKeys) do _G[k] = nil end"),
                 0);
    }

    // The walk stops at 99 levels of nesting and hands back an empty table,
    // which for a saved variable means its contents are not in the save. That
    // has to be said out loud rather than left to be discovered.
    void test_tableNestedPastTheWalkLimitIsReportedNotSilentlyEmptied()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L,
                               "deeplyNestedSavedTable = {shallow = 'shallow member value'} "
                               "local t = deeplyNestedSavedTable "
                               "for i = 1, 120 do t.nested = {} t = t.nested end "
                               "t.deepest = 'past the limit value'"),
                 0);
        vu->savedVars.insert(qsl("deeplyNestedSavedTable"));

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("nested more than 99 tables deep")));
        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("shallow member value")), "the members the walk did reach must still be exported");
        QVERIFY2(!xml.contains(qsl("past the limit value")), "a member past the nesting limit is not exported - the warning above is what tells the user");

        vu->savedVars.remove(qsl("deeplyNestedSavedTable"));
        QCOMPARE(luaL_dostring(L, "deeplyNestedSavedTable = nil"), 0);
    }

    // The walk names a number-keyed member by converting the key to text, and
    // %.14g does not name every double exactly, so looking such a member up
    // again by that name finds nothing. A member must be saved with the value
    // the walk read rather than with what a second read makes of it: an empty
    // value is one a member can genuinely have (#9769).
    void test_memberWithAnImpreciseNumberKeyIsNotExportedBlank()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "impreciseKeyTable = {} impreciseKeyTable[1/3] = 'imprecise member value'"), 0);
        vu->savedVars.insert(qsl("impreciseKeyTable"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("impreciseKeyTable")), "the saved table itself must be exported");
        QVERIFY2(xml.contains(qsl("imprecise member value")), "a member the export cannot look up again must still be saved with the value the walk read");

        vu->savedVars.remove(qsl("impreciseKeyTable"));
        QCOMPARE(luaL_dostring(L, "impreciseKeyTable = nil"), 0);
    }

    // Mudlet's own tables are hidden by name at profile load, so a saved table
    // holding one of them reaches it under a name of the user's own that
    // nothing has hidden. Recognising it has to be by identity, or the whole of
    // Mudlet's table goes into the profile save behind it (#9769).
    void test_savedTableHoldingAHiddenTableDoesNotExportItsContents()
    {
        QVERIFY2(!mpEditor, "this test rebuilds the shared variable tree, so it must run before the Variables-view tests");
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "internalApiTable = {internalField = 'internal field value'}"), 0);

        const QSet<QString> hiddenBefore = vu->hidden;
        const QSet<const void*> hiddenTablesBefore = vu->hiddenTables;
        // the hiding half of what Host::hideMudletsVariables() does at profile
        // load: everything in _G at that moment is Mudlet's or a package's
        lI->getVars(true);

        QCOMPARE(luaL_dostring(L, "apiHolderTable = {plainMember = 'plain member value', api = internalApiTable}"), 0);
        vu->savedVars.insert(qsl("apiHolderTable"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("plain member value")), "a plain member of a saved table must be exported");
        QVERIFY2(!xml.contains(qsl("internal field value")), "a saved table holding a hidden table must not export that table's contents");

        vu->savedVars.remove(qsl("apiHolderTable"));
        vu->hidden = hiddenBefore;
        vu->hiddenTables = hiddenTablesBefore;
        QCOMPARE(luaL_dostring(L, "apiHolderTable, internalApiTable = nil"), 0);
    }

    // The other side of that: profile load hides every table in _G, the user's
    // own included, and then un-hides the ones the profile saves. Un-hiding has
    // to give a table its identity back as well as its name, or a saved table
    // that another one holds stops being written out with it.
    void test_unhiddenSavedTableStillRidesAlongInsideAnotherSavedTable()
    {
        QVERIFY2(!mpEditor, "this test rebuilds the shared variable tree, so it must run before the Variables-view tests");
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "unhiddenInnerTable = {member = 'unhidden member value'}"), 0);
        // both names, as a restored profile has them: XMLimport registers every
        // variable it reads back, members included
        vu->savedVars.insert(qsl("unhiddenInnerTable"));
        vu->savedVars.insert(qsl("unhiddenInnerTable.member"));

        const QSet<QString> hiddenBefore = vu->hidden;
        const QSet<const void*> hiddenTablesBefore = vu->hiddenTables;
        mpHost->hideMudletsVariables();

        // the holder comes after the hiding, as a package's would when it runs
        QCOMPARE(luaL_dostring(L, "unhiddenHolderTable = {inner = unhiddenInnerTable}"), 0);
        vu->savedVars.insert(qsl("unhiddenHolderTable"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QCOMPARE(xml.count(qsl("unhidden member value")), 2);

        vu->savedVars.remove(qsl("unhiddenInnerTable"));
        vu->savedVars.remove(qsl("unhiddenInnerTable.member"));
        vu->savedVars.remove(qsl("unhiddenHolderTable"));
        vu->hidden = hiddenBefore;
        vu->hiddenTables = hiddenTablesBefore;
        QCOMPARE(luaL_dostring(L, "unhiddenHolderTable, unhiddenInnerTable = nil"), 0);
    }

    // The save-time copy answers hiding from identities borrowed off the live
    // unit, and those decay the same way. A top-level saved global is safe
    // regardless - its savedVars name is honoured before hiding is - so the
    // exposed shape is a member riding along inside a saved table: one landing
    // on a collected hidden table's address must still be written out with its
    // table, not silently dropped from the save. This is the save half of the
    // recycled-address bug; the TLuaInterfaceTest unit tests cover the view
    // half. Whether an address is really handed out again is the allocator's
    // business, so the freed batch is opportunistic - the identity injected
    // behind the 'injected' member stands in for the allocator
    // deterministically, putting a fresh table's address in the state a
    // collected identity decays to: remembered, with nothing left to vouch
    // for it.
    void test_rideAlongMemberOnACollectedHiddenTableAddressIsStillExported()
    {
        QVERIFY2(!mpEditor, "this test rebuilds the shared variable tree, so it must run before the Variables-view tests");
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "recycledGroup = {} for i = 1, 512 do recycledGroup[i] = {'batch filler'} end"), 0);

        const QSet<QString> hiddenBefore = vu->hidden;
        const QSet<const void*> hiddenTablesBefore = vu->hiddenTables;
        mpHost->hideMudletsVariables();

        // one chunk, so nothing else allocates between the collect and the
        // fresh member tables that could take the freed blocks first
        QCOMPARE(luaL_dostring(L,
                               "recycledGroup = nil collectgarbage('collect') "
                               "freshHolderTable = {} "
                               "for i = 0, 63 do freshHolderTable['m' .. i] = {'fresh member value ' .. i} end "
                               "freshHolderTable.injected = {'injected member value'}"),
                 0);
        vu->savedVars.insert(qsl("freshHolderTable"));

        lua_getglobal(L, "freshHolderTable");
        lua_getfield(L, -1, "injected");
        vu->hiddenTables.insert(lua_topointer(L, -1));
        lua_pop(L, 2);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        for (int i = 0; i < 64; ++i) {
            QVERIFY2(xml.contains(qsl("fresh member value %1").arg(i)), "a fresh member of a saved table must ride along even when it lands on a collected hidden table's address");
        }
        QVERIFY2(xml.contains(qsl("injected member value")), "an identity nothing vouches for must not swallow the member now on that address");

        vu->savedVars.remove(qsl("freshHolderTable"));
        vu->hidden = hiddenBefore;
        vu->hiddenTables = hiddenTablesBefore;
        QCOMPARE(luaL_dostring(L, "freshHolderTable = nil"), 0);
    }

    // A script adds to a saved table while the editor sits on the Variables
    // view. A session's last save is taken with whatever view was left on
    // screen, so quitting from there is enough to reach this.
    void test_savedTableMemberIsExportedWithVariablesViewOpen()
    {
        QVERIFY2(showEditorOnVariablesView(), "the script editor could not be opened on the Variables view");

        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "varsViewTable = {seedMember = 'seed member value'}"), 0);
        vu->savedVars.insert(qsl("varsViewTable"));
        vu->savedVars.insert(qsl("varsViewTable.seedMember"));
        mpEditor->repopulateVars();

        // a script running afterwards, with the view still up
        QCOMPARE(luaL_dostring(L, "varsViewTable.lateMember = 'late member value'"), 0);
        QCOMPARE(luaL_dostring(L, "varsViewTable.seedMember = nil"), 0);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("late member value")), "a member added while the Variables view was open must still be saved");
        // secondary: the save reads a fresh tree, so a member that is gone by
        // then has no node in it to be written from
        QVERIFY2(!xml.contains(qsl("seed member value")), "a member a script removed while the Variables view was open must not be saved back");

        auto* pVariablesTree = mpEditor->findChild<QTreeWidget*>(qsl("treeWidget_variables"));
        QVERIFY2(pVariablesTree, "the editor has no variables tree widget");
        QTreeWidgetItem* pBaseItem = pVariablesTree->topLevelItem(0);
        QVERIFY2(pBaseItem && pBaseItem->childCount() > 0, "the Variables view did not populate");
        QVERIFY2(vu->getWVar(pBaseItem->child(0)), "a save taken with the Variables view on screen must leave its items resolving to their variables");

        vu->savedVars.remove(qsl("varsViewTable"));
        vu->savedVars.remove(qsl("varsViewTable.seedMember"));
        QCOMPARE(luaL_dostring(L, "varsViewTable = nil"), 0);
    }

    // ... and the same for a whole variable rather than a table member.
    void test_lateSavedVariableIsExportedWithVariablesViewOpen()
    {
        QVERIFY2(showEditorOnVariablesView(), "the script editor could not be opened on the Variables view");

        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        mpEditor->repopulateVars();

        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "varsViewLateVar = 'late variable value'"), 0);
        vu->savedVars.insert(qsl("varsViewLateVar"));

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("late variable value")), "a saved variable created while the Variables view was open must still be saved");

        vu->savedVars.remove(qsl("varsViewLateVar"));
        QCOMPARE(luaL_dostring(L, "varsViewLateVar = nil"), 0);
    }

    // The other side: a save must not pull the tree out from under the editor.
    // Its tree widget and search results resolve items through VarUnit's
    // item -> TVar map, which rebuilding the shared tree empties.
    void test_variablesEditorItemMappingSurvivesExport()
    {
        QVERIFY2(showEditorOnVariablesView(), "the script editor could not be opened on the Variables view");
        mpEditor->repopulateVars();

        VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
        auto* pVariablesTree = mpEditor->findChild<QTreeWidget*>(qsl("treeWidget_variables"));
        QVERIFY2(pVariablesTree, "the editor has no variables tree widget");
        QTreeWidgetItem* pBaseItem = pVariablesTree->topLevelItem(0);
        QVERIFY2(pBaseItem && pBaseItem->childCount() > 0, "the Variables view did not populate");
        QTreeWidgetItem* pVariableItem = pBaseItem->child(0);
        TVar* pMappedBefore = vu->getWVar(pVariableItem);
        QVERIFY2(pMappedBefore, "the Variables view's items should resolve to a variable");

        // any save does it: the Save Profile button, the autosave, a package change
        mpEditor->slot_showTriggers();
        QVERIFY(!exportProfileXml().isEmpty());

        QVERIFY2(vu->getWVar(pVariableItem) == pMappedBefore, "a profile save must leave the Variables editor's items resolving to their variables");
    }

    // What the user actually cares about: the data is there again next session.
    // Declared last because importing puts the whole package back into the live
    // profile, which the other tests would then be sharing.
    void test_savedTableReachedByAnotherGlobalSurvivesAReload()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        // as many aliases as test_savedTableReachedByAnotherGlobalIsExported
        // uses, and for the same reason
        QCOMPARE(luaL_dostring(L,
                               "reloadSavedTable = {member = 'reload member value', nest = {deep = 'reload deep value'}} "
                               "aaaAliasOfReload = reloadSavedTable bAliasOfReload = reloadSavedTable m1AliasOfReload = reloadSavedTable "
                               "xyzzyAliasOfReload = reloadSavedTable alphaAliasOfReload = reloadSavedTable ref1AliasOfReload = reloadSavedTable "
                               "A_1AliasOfReload = reloadSavedTable"),
                 0);
        for (const auto& name : {qsl("reloadSavedTable"), qsl("reloadSavedTable.member"), qsl("reloadSavedTable.nest"), qsl("reloadSavedTable.nest.deep")}) {
            vu->savedVars.insert(name);
        }

        const QString xmlPath = mudlet::getMudletPath(enums::profileHomePath, mHostname) + qsl("/reload-test.xml");
        auto writer = std::make_shared<XMLexport>(mpHost);
        QVERIFY2(writer->exportPackage(xmlPath, true, false), "the profile could not be exported");

        QCOMPARE(luaL_dostring(L, "reloadSavedTable, aaaAliasOfReload, bAliasOfReload, m1AliasOfReload, xyzzyAliasOfReload, alphaAliasOfReload, ref1AliasOfReload, A_1AliasOfReload = nil"), 0);
        QCOMPARE(luaL_dostring(L, "assert(reloadSavedTable == nil)"), 0);

        QFile file(xmlPath);
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text), qPrintable(file.errorString()));
        XMLimport importer(mpHost);
        auto [imported, importError] = importer.importPackage(&file);
        file.close();
        QFile::remove(xmlPath);
        QVERIFY2(imported, qPrintable(importError));

        QVERIFY2(luaL_dostring(L, "assert(reloadSavedTable.member == 'reload member value')") == 0, "the saved table did not come back with its member after a save and reload");
        QVERIFY2(luaL_dostring(L, "assert(reloadSavedTable.nest.deep == 'reload deep value')") == 0, "the saved table's nested member did not come back after a save and reload");

        for (const auto& name : {qsl("reloadSavedTable"), qsl("reloadSavedTable.member"), qsl("reloadSavedTable.nest"), qsl("reloadSavedTable.nest.deep")}) {
            vu->savedVars.remove(name);
        }
        QCOMPARE(luaL_dostring(L, "reloadSavedTable = nil"), 0);
    }

    // The session after the one test_savedTableReachedByAnotherGlobalSurvivesAReload
    // covers. Profile load hides everything in _G once the variables are back in
    // it, so the user's own tables go through the hiding as well, and the save
    // that follows only holds them because loading un-hides every name the
    // profile saves. Declared after the Variables-view tests because the hiding
    // walk rebuilds the tree their items point into.
    void test_savedTableSurvivesTheSaveAfterALoad()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "secondSessionTable = {member = 'second session value', nest = {deep = 'second session deep value'}}"), 0);
        vu->savedVars.insert(qsl("secondSessionTable"));

        const QString xmlPath = mudlet::getMudletPath(enums::profileHomePath, mHostname) + qsl("/second-session-test.xml");
        auto writer = std::make_shared<XMLexport>(mpHost);
        QVERIFY2(writer->exportPackage(xmlPath, true, false), "the profile could not be exported");
        QCOMPARE(luaL_dostring(L, "secondSessionTable = nil"), 0);

        QFile file(xmlPath);
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text), qPrintable(file.errorString()));
        XMLimport importer(mpHost);
        auto [imported, importError] = importer.importPackage(&file);
        file.close();
        QFile::remove(xmlPath);
        QVERIFY2(imported, qPrintable(importError));

        const QSet<QString> hiddenBefore = vu->hidden;
        const QSet<const void*> hiddenTablesBefore = vu->hiddenTables;
        // where profile load runs it: after the import has put the variables back
        mpHost->hideMudletsVariables();

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("second session value")), "a saved table's member must still be in the save the session after it was loaded");
        QVERIFY2(xml.contains(qsl("second session deep value")), "a saved table's nested member must still be in the save the session after it was loaded");

        vu->hidden = hiddenBefore;
        vu->hiddenTables = hiddenTablesBefore;
        for (const auto& name : {qsl("secondSessionTable"), qsl("secondSessionTable.member"), qsl("secondSessionTable.nest"), qsl("secondSessionTable.nest.deep")}) {
            vu->savedVars.remove(name);
        }
        QCOMPARE(luaL_dostring(L, "secondSessionTable = nil"), 0);
    }

private:
    // Returns false rather than asserting: a QVERIFY here would only return from
    // this helper, leaving the caller to dereference a null editor.
    bool showEditorOnVariablesView()
    {
        if (!mpEditor) {
            mudlet::self()->slot_showScriptDialog();
            QTest::qWait(100);
            mpEditor = mpHost->mpEditorDialog;
            if (!mpEditor) {
                return false;
            }
        }
        mpEditor->slot_showVariables();
        QTest::qWait(50);
        return true;
    }

    QString exportProfileXml()
    {
        const QString xmlPath = mudlet::getMudletPath(enums::profileHomePath, mHostname) + qsl("/xmlexport-test.xml");
        auto writer = std::make_shared<XMLexport>(mpHost);
        if (!writer->exportPackage(xmlPath, true, false)) {
            return {};
        }
        QFile file(xmlPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        const QString xml = QString::fromUtf8(file.readAll());
        file.close();
        QFile::remove(xmlPath);
        return xml;
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(1000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "XMLexportVariablesTest.moc"
MUDLET_GROUPED_TEST_MAIN(XMLexportVariablesTest)
