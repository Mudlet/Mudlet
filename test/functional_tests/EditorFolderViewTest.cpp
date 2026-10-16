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
 * The editor's Folders view, which lists every item type in one tree grouped
 * by folder name. It is all widget state behind the editor's toolbar, which
 * Lua has no way to see.
 */

#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "AliasUnit.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgAliasMainArea.h"
#include "dlgTriggerEditor.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/textundostack.h"
#include "edbee/texteditorcontroller.h"
#include "edbee/texteditorwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorFolderViewTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("EditorFolderView-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    QTreeWidgetItem* topLevelNamed(const QString& name) const
    {
        QTreeWidgetItem* pFound = nullptr;
        for (int i = 0, total = mpEditor->mpFolderTree->topLevelItemCount(); i < total; ++i) {
            QTreeWidgetItem* pItem = mpEditor->mpFolderTree->topLevelItem(i);
            if (pItem->text(0) == name) {
                if (pFound) {
                    return nullptr;
                }
                pFound = pItem;
            }
        }
        return pFound;
    }

    QStringList childNames(QTreeWidgetItem* pParent) const
    {
        QStringList names;
        for (int i = 0, total = pParent->childCount(); i < total; ++i) {
            names << pParent->child(i)->text(0);
        }
        return names;
    }

    QTreeWidgetItem* childNamed(QTreeWidgetItem* pParent, const QString& name) const
    {
        if (!pParent) {
            return nullptr;
        }
        for (int i = 0, total = pParent->childCount(); i < total; ++i) {
            if (pParent->child(i)->text(0) == name) {
                return pParent->child(i);
            }
        }
        return nullptr;
    }

    QList<QPushButton*> summaryButtons() const { return mpEditor->mpFolderSummaryButtons->findChildren<QPushButton*>(Qt::FindDirectChildrenOnly); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(1s), "Could not connect with the host.");

        QVERIFY(runLua(qsl(R"(
            permSubstringTrigger("qaCombat", "", {}, "")
            permSubstringTrigger("qaHit", "qaCombat", {"hits you"}, "qaHit = 1")
            permSubstringTrigger("qaMapping", "", {}, "")
            permSubstringTrigger("qaLooseTrigger", "", {"loose"}, "qaLoose = 1")
            permAlias("qaCombat", "", "", "")
            permAlias("qaKill", "qaCombat", "^kk$", "qaKill = 1")
            permScript("qaCombat", "", "", "")
            permScript("qaCombatHelpers", "qaCombat", "qaHelpers = 1")
            permSubstringTrigger("qaSub", "qaMapping", {}, "")
            permAlias("qaMapping", "", "", "")
            permAlias("qaSub", "qaMapping", "", "")
            permSubstringTrigger("qaOther", "", {}, "")
            permSubstringTrigger("qaSub", "qaOther", {}, "")
            permSubstringTrigger("qaTwin", "", {}, "")
            permSubstringTrigger("qaTwin", "", {}, "")
            permAlias("qaTwin", "", "", "")
            permSubstringTrigger("qaPair", "", {}, "")
            permAlias("qaPair", "", "", "")
            permAlias("qaSub", "", "", "")
            permSubstringTrigger("qaGone", "", {}, "")
            permAlias("qaGone", "", "", "")
            permSubstringTrigger("qaVanish", "", {}, "")
            permAlias("qaVanish", "", "", "")
            permSubstringTrigger("qaErr", "", {}, "")
            permSubstringTrigger("qaBad", "qaErr", {}, "")
            permAlias("qaErr", "", "", "")
        )")));

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");
        mpEditor->mNeedUpdateData = true;
    }

    void cleanupTestCase()
    {
        if (mpHost) {
            if (auto* pEditor = HostDialogs::of(mpHost).mpEditorDialog.data()) {
                HostDialogs::of(mpHost).mpEditorDialog = nullptr;
                delete pEditor;
            }
        }
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_theFoldersViewIsOffUntilTurnedOn()
    {
        QVERIFY(!mpHost->folderViewInEditor());
        QVERIFY(!mpEditor->mpViewFoldersAction->isVisible());
        mpEditor->slot_showFolders();
        QVERIFY2(!mpEditor->mFolderViewActive, "the Folders view opened while its setting was off");
        QVERIFY(mpEditor->mpFolderTree->isHidden());

        mpHost->setFolderViewInEditor(true);
        QVERIFY(mpEditor->mpViewFoldersAction->isVisible());
    }

    void test_foldersOfTheSameNameMergeAcrossTypes()
    {
        mpEditor->slot_showFolders();
        QVERIFY(mpEditor->mFolderViewActive);
        QVERIFY(!mpEditor->mpFolderTree->isHidden());
        QVERIFY2(mpEditor->treeWidget_triggers->isHidden(), "the triggers tree stayed up beside the Folders view");

        QTreeWidgetItem* pCombat = topLevelNamed(qsl("qaCombat"));
        QVERIFY2(pCombat, "qaCombat is not exactly one row at the top of the Folders view");
        QCOMPARE(pCombat->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 3);
        QCOMPARE(pCombat->text(1), qsl("Triggers, Aliases, Scripts"));
        QCOMPARE(childNames(pCombat), (QStringList{qsl("qaHit"), qsl("qaKill"), qsl("qaCombatHelpers")}));

        QTreeWidgetItem* pOther = topLevelNamed(qsl("qaOther"));
        QVERIFY(pOther);
        QCOMPARE(pOther->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 1);

        const int topLevelCount = mpEditor->mpFolderTree->topLevelItemCount();
        QTreeWidgetItem* pLoose = mpEditor->mpFolderTree->topLevelItem(topLevelCount - 1);
        QCOMPARE(pLoose->text(0), dlgTriggerEditor::tr("Not in a folder"));
        QVERIFY(childNames(pLoose).contains(qsl("qaLooseTrigger")));
        QVERIFY2(!childNames(pLoose).contains(qsl("qaHit")), "an item inside a folder was also listed as not in one");
    }

    void test_choosingAnItemOpensItInItsOwnEditor()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pKill = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaKill"));
        QVERIFY(pKill);
        mpEditor->mpFolderTree->setCurrentItem(pKill);

        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmAliasView);
        QVERIFY(mpEditor->treeWidget_aliases->currentItem());
        QCOMPARE(mpEditor->treeWidget_aliases->currentItem()->text(0), qsl("qaKill"));
        QCOMPARE(mpEditor->mpAliasMainArea->lineEdit_alias_name->text(), qsl("qaKill"));
        QVERIFY(!mpEditor->mpAliasMainArea->isHidden());
        QVERIFY2(mpEditor->treeWidget_aliases->isHidden(), "opening an alias switched away from the Folders view");
        QVERIFY(!mpEditor->mpFolderTree->isHidden());
    }

    void test_aMergedFolderAsksWhichTypeToOpen()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pCombat = topLevelNamed(qsl("qaCombat"));
        QVERIFY(pCombat);
        mpEditor->mpFolderTree->setCurrentItem(pCombat);

        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());
        QVERIFY(mpEditor->mpAliasMainArea->isHidden());
        QVERIFY2(!mpEditor->mAddItem->isEnabled(), "adding was left on with no item type to add to");
        const QList<QPushButton*> buttons = summaryButtons();
        QCOMPARE(buttons.size(), 3);

        buttons.at(1)->click();
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmAliasView);
        QCOMPARE(mpEditor->treeWidget_aliases->currentItem()->text(0), qsl("qaCombat"));
        QVERIFY(mpEditor->mpFolderSummaryArea->isHidden());
        QVERIFY(!mpEditor->mpAliasMainArea->isHidden());
        QVERIFY(mpEditor->mAddItem->isEnabled());
        QCOMPARE(mpEditor->mpFolderTree->currentItem(), pCombat);
    }

    void test_itemsMadeInTheEditorShowUpInTheFoldersView()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmTriggerView);

        mpEditor->slot_addNewItem();
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);
        QTreeWidgetItem* pCombat = topLevelNamed(qsl("qaCombat"));
        QVERIFY(pCombat);
        const QString newTrigger = dlgTriggerEditor::tr("New trigger");
        QVERIFY2(childNamed(pCombat, newTrigger), "a trigger added inside a folder did not appear under it");
        QCOMPARE(mpEditor->mpFolderTree->currentItem(), childNamed(pCombat, newTrigger));
    }

    void test_nestedFoldersMergeOnlyUnderTheSameParent()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pMappingSub = childNamed(topLevelNamed(qsl("qaMapping")), qsl("qaSub"));
        QVERIFY(pMappingSub);
        QCOMPARE(pMappingSub->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 2);
        QTreeWidgetItem* pOtherSub = childNamed(topLevelNamed(qsl("qaOther")), qsl("qaSub"));
        QVERIFY(pOtherSub);
        QCOMPARE(pOtherSub->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 1);
        QTreeWidgetItem* pTopSub = topLevelNamed(qsl("qaSub"));
        QVERIFY2(pTopSub, "a top-level alias folder merged into a trigger folder of the same name somewhere deeper");
        QCOMPARE(pTopSub->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 1);
    }

    // A package installed twice leaves two trigger folders of one name, and
    // merging those would mix their contents under one row
    void test_twoFoldersOfOneTypeStayApart()
    {
        mpEditor->slot_showFolders();
        QList<QTreeWidgetItem*> twins;
        for (int i = 0, total = mpEditor->mpFolderTree->topLevelItemCount(); i < total; ++i) {
            if (mpEditor->mpFolderTree->topLevelItem(i)->text(0) == qsl("qaTwin")) {
                twins << mpEditor->mpFolderTree->topLevelItem(i);
            }
        }
        QCOMPARE(twins.size(), 2);
        QCOMPARE(twins.at(0)->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 2);
        QCOMPARE(twins.at(1)->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 1);
    }

    // Saving the item that was open sets its icon again, which must not rebuild
    // the tree under the summary, or its buttons are replaced before a click lands
    void test_aMergedFolderSummaryStaysPut()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        QTest::qWait(50ms);
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaCombat")));
        QVERIFY(!summaryButtons().isEmpty());
        const QPointer<QPushButton> pButton = summaryButtons().constFirst();
        QSignalSpy refreshSpy(mpEditor->mpFolderTreeRefreshTimer, &QTimer::timeout);
        QTest::qWait(200ms);
        QVERIFY2(refreshSpy.count() <= 1, qPrintable(qsl("the Folders view rebuilt %1 times while idle").arg(refreshSpy.count())));
        QVERIFY2(pButton, "the summary's buttons were replaced while it sat on screen");
    }

    void test_aClickOnAnotherItemIsDelivered()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pCombat = topLevelNamed(qsl("qaCombat"));
        QVERIFY(pCombat);
        pCombat->setExpanded(true);
        QTreeWidgetItem* pHit = childNamed(pCombat, qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        // Offscreen windows start small enough for other widgets to cover the tree
        mpEditor->resize(1280, 800);
        QTest::qWait(50ms);
        QTreeWidgetItem* pKill = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaKill"));
        QVERIFY(pKill);
        const QPoint centre = mpEditor->mpFolderTree->visualItemRect(pKill).center();
        QSignalSpy clickSpy(mpEditor->mpFolderTree, &QTreeWidget::itemClicked);
        QSignalSpy pressSpy(mpEditor->mpFolderTree, &QTreeWidget::itemPressed);
        QTest::mousePress(mpEditor->mpFolderTree->viewport(), Qt::LeftButton, Qt::NoModifier, centre);
        QTest::qWait(50ms);
        QTest::mouseRelease(mpEditor->mpFolderTree->viewport(), Qt::LeftButton, Qt::NoModifier, centre);
        QCOMPARE(pressSpy.count(), 1);
        QCOMPARE(clickSpy.count(), 1);
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmAliasView);
    }

    void test_actionsAreOffWhenNoSingleItemIsChosen()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        QAction* pCopy = nullptr;
        for (QAction* pAction : mpEditor->mpFolderTree->actions()) {
            if (pAction->text() == dlgTriggerEditor::tr("Copy")) {
                pCopy = pAction;
            }
        }
        QVERIFY(pCopy);

        const int topLevelCount = mpEditor->mpFolderTree->topLevelItemCount();
        mpEditor->mpFolderTree->setCurrentItem(mpEditor->mpFolderTree->topLevelItem(topLevelCount - 1));
        QVERIFY2(!mpEditor->mDeleteItem->isEnabled(), "Delete would remove the hidden trigger behind the \"Not in a folder\" row");
        QVERIFY(!pCopy->isEnabled());

        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaCombat")));
        QVERIFY(!mpEditor->mDeleteItem->isEnabled());
        QVERIFY(!mpEditor->mSaveItem->isEnabled());
        QVERIFY(!mpEditor->mpAction_toggleActive->isEnabled());
        QVERIFY2(!pCopy->isEnabled(), "Copy would take the hidden item behind a merged folder");

        mpEditor->mpFolderTree->setCurrentItem(pHit);
        QVERIFY(mpEditor->mDeleteItem->isEnabled());
        QVERIFY(pCopy->isEnabled());
    }

    void test_aSummaryThatStopsBeingMergedOpensWhatIsLeft()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pPair = topLevelNamed(qsl("qaPair"));
        QVERIFY(pPair);
        mpEditor->mpFolderTree->setCurrentItem(pPair);
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());

        QTreeWidgetItem* pAliasRoot = mpEditor->treeWidget_aliases->topLevelItem(0);
        QList<QTreeWidgetItem*> aliasPair = mpEditor->treeWidget_aliases->findItems(qsl("qaPair"), Qt::MatchExactly | Qt::MatchRecursive);
        QCOMPARE(aliasPair.size(), 1);
        delete pAliasRoot->takeChild(pAliasRoot->indexOfChild(aliasPair.constFirst()));
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);

        QVERIFY(mpEditor->mpFolderSummaryArea->isHidden());
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmTriggerView);
        QVERIFY2(!mpEditor->mpTriggersMainArea->isHidden(), "the summary went away and left no form behind");
        QCOMPARE(mpEditor->treeWidget_triggers->currentItem()->text(0), qsl("qaPair"));
    }

    // The main window's own Triggers, Aliases... buttons switch the editor's view from outside it
    void test_aViewSwitchFromOutsideFollowsInTheFoldersView()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        mpEditor->mpFolderTree->setCurrentItem(childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaKill")));
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmAliasView);

        mpEditor->slot_showTriggers();
        QVERIFY(mpEditor->mFolderViewActive);
        QVERIFY(mpEditor->mpFolderTree->currentItem());
        QCOMPARE(mpEditor->mpFolderTree->currentItem()->text(0), qsl("qaHit"));
    }

    void test_ctrlTabCyclesFromTheFoldersView()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        mpEditor->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mpEditor));
        mpEditor->mpSourceEditorEdbee->setFocus();
        QTRY_VERIFY(mpEditor->mpSourceEditorEdbee->hasFocus() || QApplication::focusWidget());
        mpEditor->slot_nextSection();
        QTRY_VERIFY2(mpEditor->mpFolderTree->hasFocus(), "Ctrl+Tab from the code went to a hidden tree");
        mpEditor->slot_nextSection();
        QTRY_VERIFY2(mpEditor->mpTriggersMainArea->lineEdit_trigger_name->hasFocus(), "Ctrl+Tab from the Folders view went nowhere");
    }

    void test_aMultipleSelectionDoesNotFollowIntoTheFoldersView()
    {
        mpEditor->leaveFolderView();
        mpEditor->slot_showTriggers();
        QList<QTreeWidgetItem*> hits = mpEditor->treeWidget_triggers->findItems(qsl("qaHit"), Qt::MatchExactly | Qt::MatchRecursive);
        QList<QTreeWidgetItem*> loose = mpEditor->treeWidget_triggers->findItems(qsl("qaLooseTrigger"), Qt::MatchExactly | Qt::MatchRecursive);
        QCOMPARE(hits.size(), 1);
        QCOMPARE(loose.size(), 1);
        mpEditor->treeWidget_triggers->setCurrentItem(hits.constFirst());
        loose.constFirst()->setSelected(true);
        QCOMPARE(mpEditor->treeWidget_triggers->selectedItems().size(), 2);

        mpEditor->slot_showFolders();
        QVERIFY2(mpEditor->treeWidget_triggers->selectedItems().size() == 1, "Delete would remove a hidden second trigger along with the highlighted one");

        QTest::keyPress(mpEditor->mpFolderTree, Qt::Key_Control, Qt::ControlModifier);
        mpEditor->mpFolderTree->setCurrentItem(mpEditor->mFolderTreeNodes.value(dlgTriggerEditor::folderTreeItemKey(EditorViewType::cmTriggerView, loose.constFirst()->data(0, Qt::UserRole).toInt())));
        mpEditor->mpFolderTree->setCurrentItem(childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit")));
        QTest::keyRelease(mpEditor->mpFolderTree, Qt::Key_Control, Qt::ControlModifier);
        QVERIFY2(mpEditor->treeWidget_triggers->selectedItems().size() == 1, "a held Ctrl key gathered hidden triggers into the selection");
    }

    void test_rootSelectedShowsTheIntroInTheFoldersView()
    {
        mpEditor->leaveFolderView();
        mpEditor->slot_showTriggers();
        mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->treeWidget_triggers->topLevelItem(0));
        mpEditor->slot_showTriggers();
        QVERIFY(mpEditor->mpTriggersMainArea->isHidden());
        mpEditor->slot_showFolders();
        QVERIFY2(mpEditor->mpTriggersMainArea->isHidden(), "an empty trigger form came up for the Triggers root");
    }

    void test_aSummaryWhoseFolderIsGoneShowsTheIntro()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaGone")));
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());
        {
            // As a click on the root would have left it, without showing its intro over the summary
            const QSignalBlocker blocker(mpEditor->treeWidget_triggers);
            mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->treeWidget_triggers->topLevelItem(0));
            mpEditor->mpCurrentTriggerItem = mpEditor->treeWidget_triggers->topLevelItem(0);
        }
        for (QTreeWidget* pTree : {static_cast<QTreeWidget*>(mpEditor->treeWidget_triggers), static_cast<QTreeWidget*>(mpEditor->treeWidget_aliases)}) {
            QTreeWidgetItem* pRoot = pTree->topLevelItem(0);
            const QList<QTreeWidgetItem*> gone = pTree->findItems(qsl("qaGone"), Qt::MatchExactly | Qt::MatchRecursive);
            QCOMPARE(gone.size(), 1);
            delete pRoot->takeChild(pRoot->indexOfChild(gone.constFirst()));
        }
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);
        QVERIFY(mpEditor->mpFolderSummaryArea->isHidden());
        QVERIFY2(mpEditor->mpTriggersMainArea->isHidden(), "a trigger form bound to no trigger came up");
    }

    void test_undoOnASummaryLeavesTheHiddenScriptAlone()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        mpEditor->mpSourceEditorEdbee->controller()->replaceSelection(qsl("-- typed "));
        QVERIFY(mpEditor->mpTextUndoStack->canUndo());
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaCombat")));
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());
        const QString before = mpEditor->mpSourceEditorEdbee->textDocument()->text();
        mpEditor->slot_smartUndo();
        QVERIFY2(mpEditor->mpSourceEditorEdbee->textDocument()->text() == before, "undo changed the script of a trigger the summary hides");
    }

    // Lua changes rebuild every type tree the next time the mouse enters the editor
    void test_aRebuildKeepsTheOpenItemType()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pKill = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaKill"));
        QVERIFY(pKill);
        mpEditor->mpFolderTree->setCurrentItem(pKill);
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmAliasView);
        mpEditor->mNeedUpdateData = true;
        mpEditor->saveOpenChanges();
        mpEditor->rebuildStaleTrees();
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmAliasView);
        QVERIFY(mpEditor->mpFolderTree->currentItem());
        QCOMPARE(mpEditor->mpFolderTree->currentItem()->text(0), qsl("qaKill"));
        QCOMPARE(mpEditor->mpAliasMainArea->lineEdit_alias_name->text(), qsl("qaKill"));
    }

    void test_anIconChangeJustBeforeLeavingIsKept()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHitNode = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHitNode);
        const QList<QTreeWidgetItem*> hits = mpEditor->treeWidget_triggers->findItems(qsl("qaHit"), Qt::MatchExactly | Qt::MatchRecursive);
        QCOMPARE(hits.size(), 1);
        const QIcon newIcon(qsl(":/icons/folder-red.png"));
        hits.constFirst()->setIcon(0, newIcon);
        mpEditor->leaveFolderView();
        mpEditor->slot_showTriggers();
        mpEditor->slot_showFolders();
        pHitNode = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHitNode);
        QCOMPARE(pHitNode->icon(0).cacheKey(), hits.constFirst()->icon(0).cacheKey());
    }

    void test_clickingAMergedFolderAgainBringsBackItsSummary()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pCombat = topLevelNamed(qsl("qaCombat"));
        QVERIFY(pCombat);
        mpEditor->mpFolderTree->setCurrentItem(pCombat);
        QVERIFY(!summaryButtons().isEmpty());
        summaryButtons().constFirst()->click();
        QVERIFY(mpEditor->mpFolderSummaryArea->isHidden());
        QCOMPARE(mpEditor->mpFolderTree->currentItem(), pCombat);

        const QPoint centre = mpEditor->mpFolderTree->visualItemRect(pCombat).center();
        QTest::mouseClick(mpEditor->mpFolderTree->viewport(), Qt::LeftButton, Qt::NoModifier, centre);
        QVERIFY2(!mpEditor->mpFolderSummaryArea->isHidden(), "the merged folder's other types could not be reached again");
    }

    void test_activatingAnItemTogglesIt()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        const QStringList viewAndId = pHit->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().constFirst().split(QLatin1Char(':'));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->getTrigger(viewAndId.constLast().toInt());
        QVERIFY(pTrigger);
        const bool wasActive = pTrigger->isActive();
        emit mpEditor->mpFolderTree->itemActivated(pHit, 0);
        QCOMPARE(pTrigger->isActive(), !wasActive);
        emit mpEditor->mpFolderTree->itemActivated(pHit, 0);
        QCOMPARE(pTrigger->isActive(), wasActive);
    }

    void test_aRenameShowsInTheFoldersView()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pLoose = nullptr;
        for (int i = 0, total = mpEditor->mpFolderTree->topLevelItemCount(); i < total; ++i) {
            if (mpEditor->mpFolderTree->topLevelItem(i)->data(0, dlgTriggerEditor::FolderKeyRole).toString() == qsl("loose")) {
                pLoose = mpEditor->mpFolderTree->topLevelItem(i);
            }
        }
        QTreeWidgetItem* pItem = childNamed(pLoose, qsl("qaLooseTrigger"));
        QVERIFY(pItem);
        mpEditor->mpFolderTree->setCurrentItem(pItem);
        mpEditor->mpTriggersMainArea->lineEdit_trigger_name->setText(qsl("qaRenamedTrigger"));
        mpEditor->saveOpenChanges();
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);
        QTRY_VERIFY(mpEditor->mFolderTreeNodes.value(qsl("loose")) && childNamed(mpEditor->mFolderTreeNodes.value(qsl("loose")), qsl("qaRenamedTrigger")));

        mpEditor->mpTriggersMainArea->lineEdit_trigger_name->setText(qsl("qaLooseTrigger"));
        mpEditor->saveOpenChanges();
        QTRY_VERIFY(childNamed(mpEditor->mFolderTreeNodes.value(qsl("loose")), qsl("qaLooseTrigger")));
    }

    void test_aRebuildWithNothingChosenKeepsTheItemType()
    {
        mpEditor->leaveFolderView();
        mpEditor->slot_showTriggers();
        mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->treeWidget_triggers->topLevelItem(0));
        mpEditor->slot_showFolders();
        QVERIFY(!mpEditor->mpFolderTree->currentItem());
        mpEditor->mNeedUpdateData = true;
        mpEditor->saveOpenChanges();
        mpEditor->rebuildStaleTrees();
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);
        QVERIFY2(mpEditor->mCurrentView == EditorViewType::cmTriggerView, "Add, Delete and toggle would do nothing for an unknown item type");
    }

    void test_aSummaryWhoseFolderAScriptRemovedShowsTheIntro()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        QTreeWidgetItem* pVanish = topLevelNamed(qsl("qaVanish"));
        QVERIFY(pVanish);
        const QStringList parts = pVanish->data(0, dlgTriggerEditor::FolderPartsRole).toStringList();
        QCOMPARE(parts.size(), 2);
        mpEditor->mpFolderTree->setCurrentItem(pVanish);
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());

        delete mpHost->getTriggerUnit()->getTrigger(parts.at(0).split(QLatin1Char(':')).constLast().toInt());
        delete mpHost->getAliasUnit()->getAlias(parts.at(1).split(QLatin1Char(':')).constLast().toInt());
        mpEditor->mNeedUpdateData = true;
        mpEditor->saveOpenChanges();
        mpEditor->rebuildStaleTrees();
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);
        QVERIFY(mpEditor->mpFolderSummaryArea->isHidden());
        QVERIFY(mpEditor->mCurrentView != EditorViewType::cmUnknownView);
        QVERIFY2(mpEditor->mpSourceEditorArea->isHidden(), "an editor holding a script of no item was left on screen");
    }

    void test_theFoldersTreeSharesTheSearchSplitter()
    {
        mpEditor->slot_showFolders();
        QTest::qWait(50ms);
        QWidget* pItemPane = mpEditor->searchSplitter->widget(0);
        QVERIFY2(pItemPane->isAncestorOf(mpEditor->mpFolderTree), "the search results pane cannot be resized against the Folders tree");
    }

    void test_aSummaryHidesTheHiddenItemsBanner()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        mpEditor->showError(qsl("qaError"));
        QVERIFY(!mpEditor->mpSystemMessageArea->isHidden());
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaCombat")));
        QVERIFY2(mpEditor->mpSystemMessageArea->isHidden(), "the error of a trigger the summary hides stayed on screen");
    }

    void test_enterOnAMergedFolderBringsBackItsSummary()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pCombat = topLevelNamed(qsl("qaCombat"));
        QVERIFY(pCombat);
        mpEditor->mpFolderTree->setCurrentItem(pCombat);
        summaryButtons().constFirst()->click();
        QVERIFY(mpEditor->mpFolderSummaryArea->isHidden());
        const QStringList viewAndId = pCombat->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().constFirst().split(QLatin1Char(':'));
        TTrigger* pFolder = mpHost->getTriggerUnit()->getTrigger(viewAndId.constLast().toInt());
        QVERIFY(pFolder);
        const bool wasActive = pFolder->isActive();
        emit mpEditor->mpFolderTree->itemActivated(pCombat, 0);
        QVERIFY2(!mpEditor->mpFolderSummaryArea->isHidden(), "the keyboard could not bring the merged folder's summary back");
        QCOMPARE(pFolder->isActive(), wasActive);
    }

    void test_aToolbarViewButtonLeavesTheFoldersView()
    {
        mpEditor->slot_showFolders();
        QAction* pShowTriggers = nullptr;
        for (QAction* pAction : mpEditor->findChildren<QAction*>()) {
            if (pAction->statusTip() == dlgTriggerEditor::tr("Show Triggers")) {
                pShowTriggers = pAction;
            }
        }
        QVERIFY(pShowTriggers);
        pShowTriggers->trigger();
        QVERIFY(!mpEditor->mFolderViewActive);
        QVERIFY(!mpEditor->treeWidget_triggers->isHidden());
        QVERIFY(mpEditor->mpFolderTree->isHidden());
    }

    void test_aChangeWhileTheFoldersViewIsClosedShowsOnReturn()
    {
        mpEditor->leaveFolderView();
        mpEditor->slot_showTriggers();
        const QList<QTreeWidgetItem*> hits = mpEditor->treeWidget_triggers->findItems(qsl("qaHit"), Qt::MatchExactly | Qt::MatchRecursive);
        QCOMPARE(hits.size(), 1);
        hits.constFirst()->setIcon(0, QIcon(qsl(":/icons/folder-green.png")));
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHitNode = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHitNode);
        QCOMPARE(pHitNode->icon(0).cacheKey(), hits.constFirst()->icon(0).cacheKey());
    }

    void test_aSummaryHidesAnErrorInTheItemLeftBehind()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pBad = childNamed(topLevelNamed(qsl("qaErr")), qsl("qaBad"));
        QVERIFY(pBad);
        mpEditor->mpFolderTree->setCurrentItem(pBad);
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(qsl("qaBad = ("));
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaErr")));
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());
        const bool bannerShown = !mpEditor->mpSystemMessageArea->isHidden();
        mpEditor->mpFolderTree->setCurrentItem(pBad);
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(QString());
        mpEditor->saveOpenChanges();
        QVERIFY2(!bannerShown, "the error of a trigger the summary hides stayed on screen");
    }

    void test_aLuaToggleOfTheHiddenItemKeepsItsErrorOffASummary()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pBad = childNamed(topLevelNamed(qsl("qaErr")), qsl("qaBad"));
        QVERIFY(pBad);
        mpEditor->mpFolderTree->setCurrentItem(pBad);
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(qsl("qaBad = ("));
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaErr")));
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());
        QVERIFY(mpEditor->mpSystemMessageArea->isHidden());
        QVERIFY(runLua(qsl("disableTrigger('qaBad')")));
        QTest::qWait(50ms);
        const bool bannerShown = !mpEditor->mpSystemMessageArea->isHidden();
        QVERIFY(runLua(qsl("enableTrigger('qaBad')")));
        mpEditor->mpFolderTree->setCurrentItem(pBad);
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(QString());
        mpEditor->saveOpenChanges();
        QVERIFY2(!bannerShown, "a script toggling the hidden trigger brought its error back over the summary");
    }

    void test_createModuleIsOffOnASummary()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaCombat")));
        QVERIFY(mpEditor->mpCurrentTriggerItem);
        QVERIFY2(!mpEditor->mpCreateModuleAction->isEnabled(), "Create Module would preselect the trigger the summary hides");
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        QVERIFY(mpEditor->mpCreateModuleAction->isEnabled());
    }

    void test_ctrlTabOnASummaryStaysOutOfTheHiddenForm()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaCombat")));
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());
        mpEditor->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mpEditor));
        auto inHiddenForm = [this]() {
            QWidget* pFocus = QApplication::focusWidget();
            return pFocus && (mpEditor->mpTriggersMainArea->isAncestorOf(pFocus) || mpEditor->mpSourceEditorArea->isAncestorOf(pFocus));
        };
        mpEditor->mpFolderTree->setFocus();
        QTRY_VERIFY(mpEditor->mpFolderTree->hasFocus());
        mpEditor->slot_previousSection();
        QTest::qWait(20ms);
        QVERIFY2(!inHiddenForm(), "Ctrl+Shift+Tab put the keyboard into the script the summary hides");
        mpEditor->mpFolderTree->setFocus();
        QTRY_VERIFY(mpEditor->mpFolderTree->hasFocus());
        mpEditor->slot_nextSection();
        QTest::qWait(20ms);
        QVERIFY2(!inHiddenForm(), "Ctrl+Tab put the keyboard into the form the summary hides");
        QVERIFY2(summaryButtons().constFirst()->hasFocus(), "Ctrl+Tab could not reach the summary's buttons");
    }

    void test_aNewItemDoesNotGatherTheOneBeforeIt()
    {
        mpEditor->slot_showFolders();
        QTreeWidgetItem* pHit = childNamed(topLevelNamed(qsl("qaCombat")), qsl("qaHit"));
        QVERIFY(pHit);
        mpEditor->mpFolderTree->setCurrentItem(pHit);
        QTest::keyPress(mpEditor->mpFolderTree, Qt::Key_Control, Qt::ControlModifier);
        mpEditor->addTrigger(false);
        QTest::keyRelease(mpEditor->mpFolderTree, Qt::Key_Control, Qt::ControlModifier);
        QTRY_VERIFY(!mpEditor->mFolderTreeStale);
        const int selected = mpEditor->treeWidget_triggers->selectedItems().size();
        {
            const QSignalBlocker blocker(mpEditor->treeWidget_triggers);
            mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->treeWidget_triggers->currentItem(), 0, QItemSelectionModel::ClearAndSelect);
        }
        QVERIFY2(selected == 1, "Delete would also remove the trigger open before Ctrl+N");
    }

    void test_theFoldersTreeOffersToRestoreAHiddenToolbar()
    {
        mpEditor->slot_showFolders();
        mpEditor->toolBar2->hide();
        QTest::qWait(20ms);
        mpEditor->mpFolderTree->setCurrentItem(topLevelNamed(qsl("qaCombat")));
        QVERIFY(!mpEditor->mpFolderSummaryArea->isHidden());
        QAction* pRestore = mpEditor->mpAction_restoreEditorItemsToolbar;
        const bool offered = mpEditor->mpFolderTree->actions().contains(pRestore) && pRestore->isVisible() && pRestore->isEnabled();
        pRestore->setEnabled(true);
        pRestore->trigger();
        QTRY_VERIFY(mpEditor->toolBar2->isVisible());
        QVERIFY2(offered, "with the Items toolbar hidden the Folders view has no way back to it");
    }

    void test_turningTheSettingOffLeavesTheFoldersView()
    {
        mpEditor->slot_showFolders();
        QVERIFY(mpEditor->mFolderViewActive);
        mpHost->setFolderViewInEditor(false);

        QVERIFY(!mpEditor->mFolderViewActive);
        QVERIFY(!mpEditor->mpViewFoldersAction->isVisible());
        QVERIFY(mpEditor->mpFolderTree->isHidden());
        QVERIFY(!mpEditor->treeWidget_triggers->isHidden());
    }
};

#include "EditorFolderViewTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorFolderViewTest)
