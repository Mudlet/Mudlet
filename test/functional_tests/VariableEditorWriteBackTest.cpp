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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "LuaInterface.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "VarUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "dlgVarsMainArea.h"
#include "edbee/models/textdocument.h"
#include "mudlet.h"

#include <QDropEvent>
#include <QMimeData>
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

class VariableEditorWriteBackTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    QTreeWidget* mpVariablesTree = nullptr;
    const QString mHostname = "VariableEditorWriteBack-Test";
    const QString mLocalhost = "localhost";

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
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
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

    // A global whose own name holds a dot is not the editor's to write either:
    // the saved and hidden bookkeeping keys a member by its dotted path, so this
    // global and the member that path names are one entry to it.
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

    // A key reaches Lua as bytes, so a quote in one is nothing the view has to
    // work around (#10114).
    void test_aMemberWhoseKeyHoldsAQuoteIsEditable()
    {
        execLua(qsl("quoteKeyTable = {} quoteKeyTable['say \"hi\"'] = 'quoted member value' quoteKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("quoteKeyTable"), qsl("say \"hi\"")});
        QVERIFY2(pMember, "the Variables view did not show the member under the name Lua gives its key");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("quoteKeyDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pMember);
        QVERIFY2(!bannerShowing(), "a variable the editor can write must not be reported as one it cannot");
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("edited member value"));
        selectVariable(pDecoy);
        QCOMPARE(luaMemberCount(qsl("quoteKeyTable")), 1);
        QVERIFY2(luaHolds(qsl("quoteKeyTable['say \"hi\"']"), qsl("edited member value")), "editing the member did not reach Lua");

        execLua(qsl("quoteKeyTable = nil quoteKeyDecoy = nil"));
    }

    // ...and a global's name is no different, identifier or not.
    void test_aGlobalWhoseNameIsNotAnIdentifierIsEditable()
    {
        execLua(qsl("_G['a global with spaces'] = 'spaced global value' spacedDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pGlobal = findVariableItem({qsl("a global with spaces")});
        QVERIFY2(pGlobal, "the Variables view did not show the global");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("spacedDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pGlobal);
        QVERIFY2(!bannerShowing(), "a variable the editor can write must not be reported as one it cannot");
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("edited global value"));
        selectVariable(pDecoy);
        QVERIFY2(luaHolds(qsl("_G['a global with spaces']"), qsl("edited global value")), "editing the global did not reach Lua");

        execLua(qsl("_G['a global with spaces'] = nil spacedDecoy = nil"));
    }

    // Dropping an item somewhere else in the Variables view rearranges the view
    // and nothing else - the variable stays in the table Lua has it in - so the
    // view does not offer the move at all (#9958).
    void test_theVariablesTreeDoesNotOfferAMoveItCannotMake()
    {
        QCOMPARE(mpVariablesTree->dragDropMode(), QAbstractItemView::NoDragDrop);
        QVERIFY2(!mpVariablesTree->dragEnabled(), "the Variables view still lets an item be picked up, and a move it makes is not made in Lua");
        QVERIFY2(!mpVariablesTree->acceptDrops(), "the Variables view still takes drops");
        QVERIFY2(!mpVariablesTree->viewport()->acceptDrops(), "the viewport is what the window system asks about a drop, and it still says yes");

        // the other views do carry a move through to what they are showing, so
        // this must not have taken the drag away from all of them
        QTreeWidget* pTriggers = mpEditor->findChild<QTreeWidget*>(qsl("treeWidget_triggers"));
        QVERIFY2(pTriggers, "the editor has no triggers tree widget");
        QCOMPARE(pTriggers->dragDropMode(), QAbstractItemView::InternalMove);
        QVERIFY2(pTriggers->dragEnabled(), "the Triggers view lost the drag it can carry through");
    }

    // ...and a drop that arrives at the widget anyway moves nothing. Note this
    // stands on its own only so far: QDropEvent takes its source() from the drag
    // currently in flight, so a synthesised one is sourceless and QTreeWidget
    // declines to move for it whatever the drop mode is. The widget state above
    // is what stops a real drag.
    void test_aDropOnTheVariablesTreeMovesNothing()
    {
        execLua(qsl("dropTargetTable = {} droppedGlobal = 'dropped value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pTarget = findVariableItem({qsl("dropTargetTable")});
        QVERIFY2(pTarget, "the Variables view did not show the table to drop onto");
        QTreeWidgetItem* pDragged = findVariableItem({qsl("droppedGlobal")});
        QVERIFY2(pDragged, "the Variables view did not show the variable to drag");

        mpVariablesTree->expandItem(mpVariablesTree->topLevelItem(0));
        mpVariablesTree->scrollToItem(pTarget);
        const QRect targetRect = mpVariablesTree->visualItemRect(pTarget);
        QVERIFY2(!targetRect.isEmpty(), "the tree has not laid the target out, so the drop would land nowhere");

        selectVariable(pDragged);
        const QModelIndex draggedIndex = mpVariablesTree->currentIndex();
        QVERIFY2(draggedIndex.isValid(), "the tree has no model index for the variable being dragged");
        QScopedPointer<QMimeData> mimeData(mpVariablesTree->model()->mimeData({draggedIndex}));
        QVERIFY2(mimeData, "the tree gave nothing to carry in the drop");
        QDropEvent dropEvent(targetRect.center(), Qt::MoveAction, mimeData.data(), Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(mpVariablesTree->viewport(), &dropEvent);

        QVERIFY2(!dropEvent.isAccepted(), "the Variables view took a drop it cannot carry through to Lua");
        QCOMPARE(pTarget->childCount(), 0);
        QCOMPARE(luaMemberCount(qsl("dropTargetTable")), 0);
        QVERIFY2(luaHolds(qsl("droppedGlobal"), qsl("dropped value")), "the variable did not stay where Lua has it");

        execLua(qsl("dropTargetTable = nil droppedGlobal = nil"));
    }

    // The key type shown belongs to the variable selected, and a boolean key is
    // a key type of its own: showing the one the variable looked at before it
    // had says something untrue about this one (#9959).
    void test_aBooleanKeyedMemberNamesItsOwnKeyType()
    {
        execLua(qsl("booleanKeyTable = {[true] = 'boolean member value', [2] = 'integer member value'} booleanKeyDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pBooleanMember = findVariableItem({qsl("booleanKeyTable"), qsl("true")});
        QVERIFY2(pBooleanMember, "the Variables view did not show the member under its boolean key");
        QTreeWidgetItem* pIntegerMember = findVariableItem({qsl("booleanKeyTable"), qsl("2")});
        QVERIFY2(pIntegerMember, "the Variables view did not show the member under its integer key");
        QTreeWidgetItem* pStringKeyed = findVariableItem({qsl("booleanKeyDecoy")});
        QVERIFY2(pStringKeyed, "the Variables view did not show the string-keyed variable");

        selectVariable(pIntegerMember);
        selectVariable(pBooleanMember);
        const QString afterAnIntegerKey = keyTypeShown();

        selectVariable(pStringKeyed);
        selectVariable(pBooleanMember);
        const QString afterAStringKey = keyTypeShown();

        QCOMPARE(afterAStringKey, afterAnIntegerKey);
        QVERIFY2(afterAnIntegerKey.contains(qsl("boolean"), Qt::CaseInsensitive),
                 qPrintable(qsl("the key type shown for a boolean key was \"%1\", which does not name a boolean").arg(afterAnIntegerKey)));

        selectVariable(pStringKeyed);
        execLua(qsl("booleanKeyTable = nil booleanKeyDecoy = nil"));
    }

    // ...and what is shown for it is a description, not an instruction: leaving
    // the member alone leaves the key it is under boolean.
    void test_leavingABooleanKeyedMemberKeepsItsKey()
    {
        execLua(qsl("booleanRoundTripTable = {[true] = 'boolean member value'} booleanRoundTripDecoy = 'decoy value'"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pMember = findVariableItem({qsl("booleanRoundTripTable"), qsl("true")});
        QVERIFY2(pMember, "the Variables view did not show the member under its boolean key");
        QTreeWidgetItem* pDecoy = findVariableItem({qsl("booleanRoundTripDecoy")});
        QVERIFY2(pDecoy, "the Variables view did not show the variable to click away to");

        selectVariable(pMember);
        selectVariable(pDecoy);

        QCOMPARE(luaMemberCount(qsl("booleanRoundTripTable")), 1);
        QVERIFY2(luaHolds(qsl("booleanRoundTripTable[true]"), qsl("boolean member value")), "the member did not stay under the boolean key it was read from");

        execLua(qsl("booleanRoundTripTable = nil booleanRoundTripDecoy = nil"));
    }

    // Qt's tristate cascade ticks rows the user cannot tick themselves, so a tick
    // on a table's parent reaches a table the size limit rules out - and what may
    // be saved has to be asked again rather than read off the check state
    // (#9957). The parent of an oversized table is over the limit itself, so
    // neither of them may be enrolled here.
    void test_tickingTheParentOfAnOversizedTableSavesNeitherOfThem()
    {
        execLua(qsl("bypassHolder = {small = 1, big = {}} for i = 1, 10001 do bypassHolder.big[i] = i end"));
        mpEditor->repopulateVars();

        QTreeWidgetItem* pParent = findVariableItem({qsl("bypassHolder")});
        QVERIFY2(pParent, "the Variables view did not show the holder table");
        QTreeWidgetItem* pBig = findVariableItem({qsl("bypassHolder"), qsl("big")});
        QVERIFY2(pBig, "the Variables view did not show the oversized table");

        VarUnit* pVarUnit = mpHost->getLuaInterface()->getVarUnit();
        pVarUnit->savedVars.clear();
        mpEditor->mpCurrentVarItem = nullptr; // nothing left over from an earlier test to be saved

        // the real signal path: setCheckState() cascades down and each change
        // reaches slot_variableChanged() through itemChanged
        pParent->setCheckState(0, Qt::Checked);

        QVERIFY2(!pVarUnit->savedVars.contains(qsl("bypassHolder")), "a table over the size limit was enrolled for saving by a tick on it");
        QVERIFY2(!pVarUnit->savedVars.contains(qsl("bypassHolder.big")), "the oversized table was enrolled for saving through its parent");
        QVERIFY2(pVarUnit->savedVars.contains(qsl("bypassHolder.small")), "a member that is saveable is still enrolled by the same tick");

        // ...and neither does clicking the row afterwards, which finds it ticked
        QCOMPARE(pBig->checkState(0), Qt::Checked);
        mpEditor->slot_variableSelected(pBig);
        QVERIFY2(!pVarUnit->savedVars.contains(qsl("bypassHolder.big")), "clicking a row left ticked from before enrolled the oversized table it stands for");
        QVERIFY2(!pVarUnit->savedVars.contains(qsl("bypassHolder")), "...and its parent with it");

        mpEditor->mpCurrentVarItem = nullptr;
        pVarUnit->savedVars.clear();
        execLua(qsl("bypassHolder = nil"));
        mpEditor->repopulateVars();
    }

private:
    QString keyTypeShown() const { return mpEditor->mpVarsMainArea->comboBox_variable_key_type->currentText(); }

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

#include "VariableEditorWriteBackTest.moc"
MUDLET_GROUPED_TEST_MAIN(VariableEditorWriteBackTest)
