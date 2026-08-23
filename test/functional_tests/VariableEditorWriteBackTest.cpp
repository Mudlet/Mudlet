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
 * Tests for what the Variables editor writes back to Lua when the user leaves a
 * variable. The editor knows a variable by the name the variable tree gives it,
 * and for a string or number key that name is the text Lua has for the key -
 * which not every key comes back from. Lua names a number key with "%.14g", so
 * a key of 1/3 is shown as "0.33333333333333", which is a different key, and a
 * write made through the shown name lands beside the real variable instead of
 * on it.
 *
 * Run with: ctest -R VariableEditorWriteBackTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgTriggerEditor.h"
#include "dlgVarsMainArea.h"
#include "mudlet.h"

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

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForVariableEditorWriteBackTest();

class VariableEditorWriteBackTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    QTreeWidget* mpVariablesTree = nullptr;
    const QString mHostname = "VariableEditorWriteBack-Test";
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForVariableEditorWriteBackTest();

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
        QVERIFY2(showEditorOnVariablesView(), "the script editor could not be opened on the Variables view");
        mpVariablesTree = mpEditor->findChild<QTreeWidget*>(qsl("treeWidget_variables"));
        QVERIFY2(mpVariablesTree, "the editor has no variables tree widget");
    }

    void cleanupTestCase()
    {
        mpVariablesTree = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // Selecting a member whose key the tree can only name approximately and
    // then clicking away: the editor has nothing it can write, and must not put
    // what it read under the name it has.
    void test_leavingAFractionKeyedMemberAddsNoSecondMember()
    {
        execLua(qsl("fractionKeyTable = {[1/3] = 'fraction member value'} fractionKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("fractionKeyTable"), qsl("0.33333333333333")});
        QVERIFY2(pMember, "the Variables view did not show the member under the name Lua gives its key");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("fractionKeyDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pMember);
        QVERIFY2(bannerShowing(), "selecting a variable the editor cannot write has to say so");
        QVERIFY2(bannerText().contains(qsl("0.33333333333333")), "the message should name the variable it is about");

        selectVariable(pDecoy);
        QCOMPARE(luaMemberCount(qsl("fractionKeyTable")), 1);
        QVERIFY2(luaHolds(qsl("fractionKeyTable[1/3]"), qsl("fraction member value")), "leaving the member changed the value it was showing");
        QVERIFY2(!bannerShowing(), "the message belongs to the variable it is about, not to the next one");

        execLua(qsl("fractionKeyTable = nil fractionKeyDecoy = nil"));
    }

    // ...and it stays out of Lua when the user does type something, rather than
    // going to the key the name spells out.
    void test_editingAFractionKeyedMemberChangesNothing()
    {
        execLua(qsl("editedKeyTable = {[1/3] = 'fraction member value'} editedKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("editedKeyTable"), qsl("0.33333333333333")});
        QVERIFY2(pMember, "the Variables view did not show the member under the name Lua gives its key");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("editedKeyDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pMember);
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("edited member value"));
        selectVariable(pDecoy);

        QCOMPARE(luaMemberCount(qsl("editedKeyTable")), 1);
        QVERIFY2(luaHolds(qsl("editedKeyTable[1/3]"), qsl("fraction member value")), "the edit reached a variable it was not meant to reach");

        execLua(qsl("editedKeyTable = nil editedKeyDecoy = nil"));
    }

    // Leaving the Variables view saves what is on screen as well, and that path
    // does not go through clicking another variable.
    void test_switchingViewsWithAnEditPendingChangesNothing()
    {
        execLua(qsl("viewSwitchTable = {[1/3] = 'fraction member value'}"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("viewSwitchTable"), qsl("0.33333333333333")});
        QVERIFY2(pMember, "the Variables view did not show the member under the name Lua gives its key");

        selectVariable(pMember);
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("edited member value"));
        mpEditor->slot_showTriggers();
        QTest::qWait(20);

        QCOMPARE(luaMemberCount(qsl("viewSwitchTable")), 1);
        QVERIFY2(luaHolds(qsl("viewSwitchTable[1/3]"), qsl("fraction member value")), "the edit reached a variable it was not meant to reach");

        QVERIFY2(showEditorOnVariablesView(), "the editor could not be put back on the Variables view");
        execLua(qsl("viewSwitchTable = nil"));
    }

    // A rename goes through the same name to find what it is renaming, so it is
    // refused on the same terms - and the tree keeps the name it had.
    void test_renamingAFractionKeyedMemberIsRefused()
    {
        execLua(qsl("renamedKeyTable = {[1/3] = 'fraction member value'} renamedKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("renamedKeyTable"), qsl("0.33333333333333")});
        QVERIFY2(pMember, "the Variables view did not show the member under the name Lua gives its key");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("renamedKeyDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pMember);
        mpEditor->mpVarsMainArea->lineEdit_var_name->setText(qsl("renamedMember"));
        selectVariable(pDecoy);

        QCOMPARE(luaMemberCount(qsl("renamedKeyTable")), 1);
        QVERIFY2(luaHolds(qsl("renamedKeyTable[1/3]"), qsl("fraction member value")), "the member did not survive the rename attempt");
        QCOMPARE(pMember->text(0), qsl("0.33333333333333"));

        execLua(qsl("renamedKeyTable = nil renamedKeyDecoy = nil"));
    }

    // The same for a table member under a string key that does not survive the
    // trip through the C string the tree names it with.
    void test_leavingAMemberWithANulInItsKeyAddsNoSecondMember()
    {
        execLua(qsl("nulKeyTable = {['before\\0after'] = 'nul member value'} nulKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("nulKeyTable"), qsl("before")});
        QVERIFY2(pMember, "the Variables view did not show the member under the name Lua gives its key");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("nulKeyDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pMember);
        selectVariable(pDecoy);

        QCOMPARE(luaMemberCount(qsl("nulKeyTable")), 1);
        QVERIFY2(luaHolds(qsl("nulKeyTable['before\\0after']"), qsl("nul member value")), "leaving the member changed the value it was showing");

        execLua(qsl("nulKeyTable = nil nulKeyDecoy = nil"));
    }

    // A global whose name is not an identifier is not reached by that name
    // either: the write paths put a root into Lua source bare, so a dot in it
    // reads as an index into whatever global comes before the dot.
    void test_editingAGlobalWithADotInItsNameLeavesOtherTablesAlone()
    {
        execLua(qsl("dotted = {} _G['dotted.global'] = 'dotted global value' dottedDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pGlobal = findVariableItem({qsl("dotted.global")});
        QVERIFY2(pGlobal, "the Variables view did not show the global");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("dottedDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pGlobal);
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("edited dotted value"));
        selectVariable(pDecoy);

        QCOMPARE(luaMemberCount(qsl("dotted")), 0);
        QVERIFY2(luaHolds(qsl("_G['dotted.global']"), qsl("dotted global value")), "the global was changed under a name that does not reach it");

        execLua(qsl("dotted = nil _G['dotted.global'] = nil dottedDecoy = nil"));
    }

    // The control: an integer key is named exactly, so this member is the
    // editor's to write - both when it is left alone and when it is edited.
    void test_anIntegerKeyedMemberIsStillEditable()
    {
        execLua(qsl("integerKeyTable = {[2] = 'integer member value'} integerKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("integerKeyTable"), qsl("2")});
        QVERIFY2(pMember, "the Variables view did not show the member");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("integerKeyDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pMember);
        QVERIFY2(!bannerShowing(), "a variable the editor can write must not be reported as one it cannot");
        selectVariable(pDecoy);
        QCOMPARE(luaMemberCount(qsl("integerKeyTable")), 1);
        QVERIFY2(luaHolds(qsl("integerKeyTable[2]"), qsl("integer member value")), "leaving the member alone changed its value");

        selectVariable(pMember);
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("edited member value"));
        selectVariable(pDecoy);
        QCOMPARE(luaMemberCount(qsl("integerKeyTable")), 1);
        QVERIFY2(luaHolds(qsl("integerKeyTable[2]"), qsl("edited member value")), "editing the member did not reach Lua");

        execLua(qsl("integerKeyTable = nil integerKeyDecoy = nil"));
    }

    // ...and the same for a plain global, which the editor reaches without a
    // key of its own to name.
    void test_aStringKeyedGlobalIsStillEditable()
    {
        execLua(qsl("stringKeyGlobal = 'global value' stringKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pGlobal = findVariableItem({qsl("stringKeyGlobal")});
        QVERIFY2(pGlobal, "the Variables view did not show the global");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("stringKeyDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pGlobal);
        QVERIFY2(!bannerShowing(), "a variable the editor can write must not be reported as one it cannot");
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("edited global value"));
        selectVariable(pDecoy);
        QVERIFY2(luaHolds(qsl("stringKeyGlobal"), qsl("edited global value")), "editing the global did not reach Lua");

        execLua(qsl("stringKeyGlobal = nil stringKeyDecoy = nil"));
    }

private:
    void execLua(const QString& code)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        QCOMPARE(luaL_dostring(L, code.toUtf8().constData()), 0);
    }

    // How many keys the table has, so that a member the editor added beside the
    // real one is caught whatever it was named.
    int luaMemberCount(const QString& tableName)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        const QString code = qsl("local n = 0 for _ in pairs(%1) do n = n + 1 end return n").arg(tableName);
        if (luaL_dostring(L, code.toUtf8().constData()) != 0) {
            lua_pop(L, 1);
            return -1;
        }
        const int count = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return count;
    }

    bool luaHolds(const QString& expression, const QString& value)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        const QString code = qsl("return %1").arg(expression);
        if (luaL_dostring(L, code.toUtf8().constData()) != 0) {
            // otherwise a typo in the expression reads as a product bug
            qWarning().noquote().nospace() << "luaHolds() could not run \"" << code << "\": " << lua_tostring(L, -1);
            lua_pop(L, 1);
            return false;
        }
        const bool held = lua_isstring(L, -1) && QString::fromUtf8(lua_tostring(L, -1)) == value;
        lua_pop(L, 1);
        return held;
    }

    bool bannerShowing() const { return !mpEditor->mpSystemMessageArea->isHidden(); }

    QString bannerText() const { return mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text(); }

    QTreeWidgetItem* findVariableItem(const QStringList& namePath)
    {
        QTreeWidgetItem* pItem = mpVariablesTree->topLevelItem(0);
        for (const QString& name : namePath) {
            QTreeWidgetItem* pNext = nullptr;
            for (int i = 0; pItem && i < pItem->childCount(); ++i) {
                if (pItem->child(i)->text(0) == name) {
                    pNext = pItem->child(i);
                    break;
                }
            }
            if (!pNext) {
                return nullptr;
            }
            pItem = pNext;
        }
        return pItem;
    }

    // What clicking an item in the Variables view does, which is also what saves
    // whatever was selected before it. Both calls are deliberate: setting the
    // current item already reaches slot_variableSelected() through
    // itemSelectionChanged, and the explicit call stands in for the itemClicked
    // that follows on the mouse release.
    void selectVariable(QTreeWidgetItem* pItem)
    {
        mpVariablesTree->setCurrentItem(pItem);
        mpEditor->slot_variableSelected(pItem);
    }

    // Returns false rather than asserting: QVERIFY expands to a bare return,
    // which would leave the caller to dereference a null editor.
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

void initializeQRCResourcesForVariableEditorWriteBackTest()
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

#include "VariableEditorWriteBackTest.moc"
QTEST_MAIN(VariableEditorWriteBackTest)
