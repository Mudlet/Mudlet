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
 *
 * Run with: ctest -R XMLexportVariablesTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "LuaInterface.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "VarUnit.h"
#include "XMLexport.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

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

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForXMLexportVariablesTest();

class XMLexportVariablesTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "XMLexportVars-Test";
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForXMLexportVariablesTest();

        mpServer = new TelnetServerStub(qApp);
        // port 0 asks the OS for an ephemeral port, so parallel test runs
        // (and other worktrees) cannot collide on a fixed one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "TelnetServerStub failed to bind a loopback port");
        mudlet::start();
        mudlet::self()->setupConfig();
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
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // A saved variable whose Lua value only comes into existence after the
    // variable tree was last built (profile load, Variables view opening)
    // must still be written out - the save path has to refresh the tree.
    void test_lateCreatedSavedVariableIsExported()
    {
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

    // Function members cannot be saved, so they must not ride along either.
    void test_functionMemberOfSavedTableIsNotExported()
    {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, "callableHolderTable = {dataMember = 'data member value', callableMember = function() end}"), 0);
        vu->savedVars.insert(qsl("callableHolderTable"));
        lI->getVars(false);

        const QString xml = exportProfileXml();
        QVERIFY(!xml.isEmpty());
        QVERIFY2(xml.contains(qsl("data member value")), "a plain member of a saved table must be exported");
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

private:
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
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(2000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
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

void initializeQRCResourcesForXMLexportVariablesTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "XMLexportVariablesTest.moc"
QTEST_MAIN(XMLexportVariablesTest)
