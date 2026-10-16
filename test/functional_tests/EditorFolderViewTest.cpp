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
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgAliasMainArea.h"
#include "dlgTriggerEditor.h"
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

        QTreeWidgetItem* pMapping = topLevelNamed(qsl("qaMapping"));
        QVERIFY(pMapping);
        QCOMPARE(pMapping->data(0, dlgTriggerEditor::FolderPartsRole).toStringList().size(), 1);

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
