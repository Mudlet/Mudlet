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
 * Adding an item through the script editor. The editor makes the item itself,
 * calls it "New ..." and only finishes it on the first save, which is a path
 * the scripting API has no way into: permTrigger() and friends hand over a
 * finished item behind the editor's back.
 */

#include <QAction>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>

#include "AliasUnit.h"
#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "HostDialogs.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ScriptUnit.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgActionMainArea.h"
#include "dlgAliasMainArea.h"
#include "dlgKeysMainArea.h"
#include "dlgScriptsMainArea.h"
#include "dlgTimersMainArea.h"
#include "dlgTriggersMainArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorAddItemTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("EditorAddItem-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    int countNamedInTree(QTreeWidget* pTree, const QString& name) const { return pTree->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0).count(); }

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

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");
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

    // The editor opens sitting on a view it has not recorded, and the add button
    // used to read that unset view and silently do nothing (#8753).
    void test_addingAnItemWorksWhileNoViewIsRecorded()
    {
        const QString newTrigger = dlgTriggerEditor::tr("New trigger");
        mpEditor->slot_showTriggers();
        QVERIFY2(mpEditor->isVisible(), "the fallback reads which tree is on screen, so the editor has to be shown");
        QVERIFY2(mpEditor->mpTriggerBaseItem, "the triggers tree has no root to select");
        mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->mpTriggerBaseItem);
        QCOMPARE(countNamedInTree(mpEditor->treeWidget_triggers, newTrigger), 0);

        mpEditor->mCurrentView = EditorViewType::cmUnknownView;
        mpEditor->slot_addNewItem();

        QCOMPARE(countNamedInTree(mpEditor->treeWidget_triggers, newTrigger), 1);
        QCOMPARE(static_cast<int>(mpHost->getTriggerUnit()->findItems(newTrigger, true, true).size()), 1);
        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmTriggerView);
    }

    // A new item's first save belongs to adding it, so one undo has to take the
    // item away (#10774).
    void test_oneUndoTakesBackAnItemAddedAndSaved()
    {
        const QString newTrigger = dlgTriggerEditor::tr("New trigger");
        mpEditor->slot_showTriggers();
        mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->mpTriggerBaseItem);
        const auto existing = mpHost->getTriggerUnit()->findItems(newTrigger, true, true);
        const int before = static_cast<int>(existing.size());
        const int commandsBefore = mpEditor->mpUndoStack->index();

        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_triggers->currentItem();
        QVERIFY2(pItem, "adding a trigger left nothing selected to save");
        TTrigger* pTrigger = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pTrigger);
        mpEditor->mpTriggersMainArea->lineEdit_trigger_command->setText(qsl("qaCommandSavedWithTheAdd"));
        mpEditor->slot_saveEdits();
        QCOMPARE(pTrigger->getCommand(), qsl("qaCommandSavedWithTheAdd"));
        const QString asSaved = exportTriggerToXML(pTrigger, SnapshotScope::ItemOnly);

        QCOMPARE(static_cast<int>(mpHost->getTriggerUnit()->findItems(newTrigger, true, true).size()), before + 1);
        QCOMPARE(mpEditor->mpUndoStack->index(), commandsBefore + 1);

        mpEditor->mpUndoStack->undo();
        QCOMPARE(static_cast<int>(mpHost->getTriggerUnit()->findItems(newTrigger, true, true).size()), before);

        // redo brings back the item as it was saved, not as it was first made
        mpEditor->mpUndoStack->redo();
        TTrigger* pRedone = nullptr;
        for (const int id : mpHost->getTriggerUnit()->findItems(newTrigger, true, true)) {
            if (std::find(existing.cbegin(), existing.cend(), id) == existing.cend()) {
                QVERIFY2(!pRedone, "redo brought back more than one trigger");
                pRedone = mpHost->getTriggerUnit()->getTrigger(id);
            }
        }
        QVERIFY2(pRedone, "redo did not bring the trigger back");
        QCOMPARE(exportTriggerToXML(pRedone, SnapshotScope::ItemOnly), asSaved);
    }

    // A new script's first save need not change it (the add already marked it
    // to run), and then pushes nothing; an edit saved after that is still the
    // player's own undo step, not part of the add
    void test_anEditAfterAFirstSaveThatChangedNothingIsUndoneOnItsOwn()
    {
        const QString newScript = dlgTriggerEditor::tr("New script");
        const QString renamed = qsl("qaRenamedNewScript");
        mpEditor->slot_showScripts();
        mpEditor->treeWidget_scripts->setCurrentItem(mpEditor->mpScriptsBaseItem);
        const int before = static_cast<int>(mpHost->getScriptUnit()->findItems(newScript).size());

        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_scripts->currentItem();
        QVERIFY2(pItem, "adding a script left nothing selected to save");
        const int scriptID = pItem->data(0, Qt::UserRole).toInt();
        TScript* pScript = mpHost->getScriptUnit()->getScript(scriptID);
        QVERIFY(pScript);
        const QString asAdded = exportScriptToXML(pScript, SnapshotScope::ItemOnly);
        mpEditor->slot_saveEdits();
        QVERIFY2(exportScriptToXML(pScript, SnapshotScope::ItemOnly) == asAdded, "the first save of an untouched script changed it, so this case cannot reach a save that pushes nothing");

        mpEditor->mpScriptsMainArea->lineEdit_script_name->setText(renamed);
        mpEditor->slot_saveEdits();
        QCOMPARE(pScript->getName(), renamed);

        mpEditor->mpUndoStack->undo();
        QVERIFY2(static_cast<int>(mpHost->getScriptUnit()->findItems(newScript).size()) == before + 1, "one undo of a rename took away the whole script it was made to");
        QCOMPARE(static_cast<int>(mpHost->getScriptUnit()->findItems(renamed).size()), 0);

        mpEditor->mpUndoStack->undo();
        QCOMPARE(static_cast<int>(mpHost->getScriptUnit()->findItems(newScript).size()), before);
    }

    // Each view's save has to finish its new item, or every later save merges into
    // the add and one undo takes the whole item away
    void test_aNewItemsSecondSaveIsUndoneOnItsOwn_data()
    {
        QTest::addColumn<int>("view");
        QTest::newRow("trigger") << static_cast<int>(EditorViewType::cmTriggerView);
        QTest::newRow("timer") << static_cast<int>(EditorViewType::cmTimerView);
        QTest::newRow("alias") << static_cast<int>(EditorViewType::cmAliasView);
        QTest::newRow("script") << static_cast<int>(EditorViewType::cmScriptView);
        QTest::newRow("key") << static_cast<int>(EditorViewType::cmKeysView);
        QTest::newRow("button") << static_cast<int>(EditorViewType::cmActionView);
    }

    void test_aNewItemsSecondSaveIsUndoneOnItsOwn()
    {
        QFETCH(int, view);
        QTreeWidget* pTree = nullptr;
        QTreeWidgetItem* pBase = nullptr;
        QLineEdit* pName = nullptr;
        switch (static_cast<EditorViewType>(view)) {
        case EditorViewType::cmTriggerView:
            mpEditor->slot_showTriggers();
            pTree = mpEditor->treeWidget_triggers;
            pBase = mpEditor->mpTriggerBaseItem;
            pName = mpEditor->mpTriggersMainArea->lineEdit_trigger_name;
            break;
        case EditorViewType::cmTimerView:
            mpEditor->slot_showTimers();
            pTree = mpEditor->treeWidget_timers;
            pBase = mpEditor->mpTimerBaseItem;
            pName = mpEditor->mpTimersMainArea->lineEdit_timer_name;
            break;
        case EditorViewType::cmAliasView:
            mpEditor->slot_showAliases();
            pTree = mpEditor->treeWidget_aliases;
            pBase = mpEditor->mpAliasBaseItem;
            pName = mpEditor->mpAliasMainArea->lineEdit_alias_name;
            break;
        case EditorViewType::cmScriptView:
            mpEditor->slot_showScripts();
            pTree = mpEditor->treeWidget_scripts;
            pBase = mpEditor->mpScriptsBaseItem;
            pName = mpEditor->mpScriptsMainArea->lineEdit_script_name;
            break;
        case EditorViewType::cmKeysView:
            mpEditor->slot_showKeys();
            pTree = mpEditor->treeWidget_keys;
            pBase = mpEditor->mpKeyBaseItem;
            pName = mpEditor->mpKeysMainArea->lineEdit_key_name;
            break;
        case EditorViewType::cmActionView:
            mpEditor->slot_showActions();
            pTree = mpEditor->treeWidget_actions;
            pBase = mpEditor->mpActionBaseItem;
            pName = mpEditor->mpActionsMainArea->lineEdit_action_name;
            break;
        default:
            QFAIL("no such view");
        }
        const QString firstName = qsl("qaFirstSave%1").arg(view);
        const QString secondName = qsl("qaSecondSave%1").arg(view);
        pTree->setCurrentItem(pBase);
        mpEditor->slot_addNewItem();
        pName->setText(firstName);
        mpEditor->slot_saveEdits();
        QVERIFY2(mpEditor->mpUndoStack->undoText().contains(firstName), qPrintable(qsl("the undo step for the new item is called \"%1\"").arg(mpEditor->mpUndoStack->undoText())));

        pName->setText(secondName);
        mpEditor->slot_saveEdits();
        QCOMPARE(countNamedInTree(pTree, secondName), 1);

        mpEditor->mpUndoStack->undo();
        QVERIFY2(countNamedInTree(pTree, firstName) == 1, "one undo of a second save took away the whole new item");
        mpEditor->mpUndoStack->undo();
        QCOMPARE(countNamedInTree(pTree, firstName), 0);
    }

    // Switching a new trigger off before saving it puts a step between the add
    // and its first save; undoing back past both must still leave the add finished
    void test_aSaveAfterUndoingBackToAnAddThatWasSavedIsUndoneOnItsOwn()
    {
        mpEditor->slot_showTriggers();
        mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->mpTriggerBaseItem);
        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_triggers->currentItem();
        QVERIFY2(pItem, "adding a trigger left nothing selected to save");
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        mpEditor->activeToggle_trigger();
        mpEditor->mpTriggersMainArea->lineEdit_trigger_command->setText(qsl("qaSavedAfterAToggle"));
        mpEditor->slot_saveEdits();
        mpEditor->mpUndoStack->undo();
        mpEditor->mpUndoStack->undo();
        QVERIFY2(mpHost->getTriggerUnit()->getTrigger(triggerID), "undoing the save and the toggle took away the trigger as well");

        mpEditor->mpTriggersMainArea->lineEdit_trigger_command->setText(qsl("qaSavedAfterTheUndos"));
        mpEditor->slot_saveEdits();
        mpEditor->mpUndoStack->undo();
        QVERIFY2(mpHost->getTriggerUnit()->getTrigger(triggerID), "one undo of a later save took away the whole trigger");
    }

    // Redo brings an unsaved new item back from a snapshot, and its first save
    // after that still belongs to adding it
    void test_oneUndoTakesBackAnItemRedoneAndThenSaved()
    {
        const QString command = qsl("qaSavedAfterARedo");
        mpEditor->slot_showTriggers();
        mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->mpTriggerBaseItem);
        mpEditor->slot_addNewItem();
        QVERIFY2(mpEditor->treeWidget_triggers->currentItem(), "adding a trigger left nothing selected");
        const int addedID = mpEditor->treeWidget_triggers->currentItem()->data(0, Qt::UserRole).toInt();
        const int commandsBefore = mpEditor->mpUndoStack->index();
        mpEditor->mpUndoStack->undo();
        mpEditor->mpUndoStack->redo();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_triggers->currentItem();
        QVERIFY2(pItem && pItem->data(0, Qt::UserRole).toInt() != addedID, "redo did not select the trigger it brought back");
        const int redoneID = pItem->data(0, Qt::UserRole).toInt();

        mpEditor->mpTriggersMainArea->lineEdit_trigger_command->setText(command);
        mpEditor->slot_saveEdits();
        QCOMPARE(mpEditor->mpUndoStack->index(), commandsBefore);

        mpEditor->mpUndoStack->undo();
        QVERIFY2(!mpHost->getTriggerUnit()->getTrigger(redoneID), "one undo left the redone trigger in place");

        // redo has to bring back the save as well, not the snapshot the first undo took
        mpEditor->mpUndoStack->redo();
        pItem = mpEditor->treeWidget_triggers->currentItem();
        QVERIFY(pItem);
        TTrigger* pTrigger = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pTrigger);
        QCOMPARE(pTrigger->getCommand(), command);
    }

    // A key made in the editor is only switched on by its first save, and that
    // save switches on nothing the editor has not marked as wanting to run. The
    // editor used to create keys without that mark, so every key made this way
    // stayed off, and its row said so to a screen reader (#8529).
    void test_aNewKeyIsActiveOnceItIsSaved()
    {
        mpEditor->slot_showKeys();
        mpEditor->treeWidget_keys->setCurrentItem(mpEditor->mpKeyBaseItem);
        mpEditor->slot_addNewItem();

        QTreeWidgetItem* pItem = mpEditor->treeWidget_keys->currentItem();
        QVERIFY2(pItem, "adding a key left nothing selected to save");
        TKey* pKey = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pKey, "the new row does not stand for a key");
        QVERIFY2(!pKey->isActive(), "a key was already running before it had been saved");

        // a key with no binding is in error, and a save leaves an item in error
        // switched off whatever else it is worth
        mpEditor->keyGrabCallback(Qt::Key_F9, Qt::NoModifier);
        mpEditor->mpKeysMainArea->lineEdit_key_name->setText(qsl("qaNewKey"));
        mpEditor->slot_saveEdits();

        QVERIFY2(pKey->isActive(), "saving a newly made key did not switch it on");
        QCOMPARE(pItem->data(0, Qt::AccessibleDescriptionRole).toString(), mpEditor->descActive);
    }

    // A new key takes its binding's name as soon as the binding is grabbed, in
    // the name box as well as the tree, and a name taken that way follows a new
    // binding
    void test_grabbingABindingNamesANewKey()
    {
        mpEditor->slot_showKeys();
        mpEditor->treeWidget_keys->setCurrentItem(mpEditor->mpKeyBaseItem);
        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_keys->currentItem();
        QVERIFY2(pItem, "adding a key left nothing selected");
        TKey* pKey = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pKey);

        mpEditor->keyGrabCallback(Qt::Key_F7, Qt::NoModifier);
        QCOMPARE(mpEditor->mpKeysMainArea->lineEdit_key_name->text(), qsl("F7"));
        QCOMPARE(pItem->text(0), qsl("F7"));
        QCOMPARE(pKey->getName(), qsl("F7"));

        mpEditor->keyGrabCallback(Qt::Key_F8, Qt::NoModifier);
        QCOMPARE(pKey->getName(), qsl("F8"));
        QCOMPARE(pItem->text(0), qsl("F8"));

        mpEditor->mpKeysMainArea->lineEdit_key_name->clear();
        mpEditor->slot_saveEdits();
        QCOMPARE(pKey->getName(), qsl("F8"));
        QCOMPARE(mpEditor->mpKeysMainArea->lineEdit_key_name->text(), qsl("F8"));
        QVERIFY2(pKey->isActive(), "a key named by its grab was not switched on by its first save");
    }

    void test_oneUndoTakesBackAGrabAndTheNameItGave()
    {
        mpEditor->slot_showKeys();
        mpEditor->treeWidget_keys->setCurrentItem(mpEditor->mpKeyBaseItem);
        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_keys->currentItem();
        QVERIFY2(pItem, "adding a key left nothing selected");
        const int keyID = pItem->data(0, Qt::UserRole).toInt();
        mpEditor->mpKeysMainArea->lineEdit_key_name->clear();
        mpEditor->slot_saveEdits();
        // saved unbound, a key is named after the binding box's "no key chosen" text, its placeholder while it is empty
        const QString savedName = mpHost->getKeyUnit()->getKey(keyID)->getName();
        const QLineEdit* pBindingField = mpEditor->mpKeysMainArea->lineEdit_key_binding;
        QCOMPARE(savedName, pBindingField->text().isEmpty() ? pBindingField->placeholderText() : pBindingField->text());
        const Qt::Key codeBefore = mpHost->getKeyUnit()->getKey(keyID)->getKeyCode();

        mpEditor->keyGrabCallback(Qt::Key_F4, Qt::NoModifier);
        QCOMPARE(mpHost->getKeyUnit()->getKey(keyID)->getName(), qsl("F4"));
        mpEditor->mpUndoStack->undo();
        TKey* pKey = mpHost->getKeyUnit()->getKey(keyID);
        QVERIFY2(pKey, "undoing the grab took away the key");
        QVERIFY2(pKey->getName() == savedName, "undoing the grab kept the name it gave");
        QCOMPARE(pKey->getKeyCode(), codeBefore);
    }

    void test_grabbingABindingKeepsANameTypedButNotYetSaved()
    {
        mpEditor->slot_showKeys();
        mpEditor->treeWidget_keys->setCurrentItem(mpEditor->mpKeyBaseItem);
        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_keys->currentItem();
        QVERIFY2(pItem, "adding a key left nothing selected");
        TKey* pKey = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pKey);
        const QString newKey = dlgTriggerEditor::tr("New key");

        mpEditor->mpKeysMainArea->lineEdit_key_name->setText(qsl("qaTypedKeyName"));
        mpEditor->keyGrabCallback(Qt::Key_F6, Qt::NoModifier);
        QCOMPARE(mpEditor->mpKeysMainArea->lineEdit_key_name->text(), qsl("qaTypedKeyName"));
        QCOMPARE(pKey->getName(), newKey);
        QCOMPARE(pItem->text(0), newKey);
    }

    // Scripts can find a key by its name, so a saved one keeps a name that
    // matches its binding when it is bound to another key
    void test_rebindingASavedKeyKeepsItsName()
    {
        mpEditor->slot_showKeys();
        mpEditor->treeWidget_keys->setCurrentItem(mpEditor->mpKeyBaseItem);
        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_keys->currentItem();
        QVERIFY2(pItem, "adding a key left nothing selected");
        TKey* pKey = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pKey);
        mpEditor->keyGrabCallback(Qt::Key_F3, Qt::NoModifier);
        mpEditor->slot_saveEdits();
        QCOMPARE(pKey->getName(), qsl("F3"));

        mpEditor->keyGrabCallback(Qt::Key_F2, Qt::NoModifier);
        QCOMPARE(pKey->getName(), qsl("F3"));
        QCOMPARE(mpEditor->mpKeysMainArea->lineEdit_key_name->text(), qsl("F3"));
    }

    void test_grabbingABindingDoesNotRenameAKeyGroup()
    {
        const QString newGroup = dlgTriggerEditor::tr("New key group");
        mpEditor->slot_showKeys();
        mpEditor->treeWidget_keys->setCurrentItem(mpEditor->mpKeyBaseItem);
        mpEditor->slot_addNewGroup();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_keys->currentItem();
        QVERIFY2(pItem, "adding a key group left nothing selected");
        TKey* pGroup = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pGroup && pGroup->isFolder());
        // a group can be called "New key" too, if the player typed that in
        mpEditor->mpKeysMainArea->lineEdit_key_name->setText(dlgTriggerEditor::tr("New key"));

        mpEditor->keyGrabCallback(Qt::Key_F5, Qt::NoModifier);
        QCOMPARE(pGroup->getName(), newGroup);
        QCOMPARE(pItem->text(0), newGroup);
        QCOMPARE(mpEditor->mpKeysMainArea->lineEdit_key_name->text(), dlgTriggerEditor::tr("New key"));
    }

    void test_aNewAliasShowsItsPatternAsItsNameOnceSaved()
    {
        const QString pattern = qsl("^qaAliasPattern$");
        mpEditor->slot_showAliases();
        mpEditor->treeWidget_aliases->setCurrentItem(mpEditor->mpAliasBaseItem);
        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_aliases->currentItem();
        QVERIFY2(pItem, "adding an alias left nothing selected");
        mpEditor->mpAliasMainArea->lineEdit_alias_pattern->setText(qsl("^qaAliasPatternFirst"));
        mpEditor->slot_saveProperty_AliasPattern();

        mpEditor->mpAliasMainArea->lineEdit_alias_pattern->setText(pattern);
        mpEditor->slot_saveEdits();
        QCOMPARE(pItem->text(0), pattern);
        QCOMPARE(mpEditor->mpAliasMainArea->lineEdit_alias_name->text(), pattern);
    }

    void test_aNewAliasIsNamedAsSoonAsItsPatternIsEntered()
    {
        const QString pattern = qsl("^qaAliasPatternEntered$");
        mpEditor->slot_showAliases();
        mpEditor->treeWidget_aliases->setCurrentItem(mpEditor->mpAliasBaseItem);
        mpEditor->slot_addNewItem();
        QTreeWidgetItem* pItem = mpEditor->treeWidget_aliases->currentItem();
        QVERIFY2(pItem, "adding an alias left nothing selected");
        TAlias* pAlias = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY(pAlias);

        mpEditor->mpAliasMainArea->lineEdit_alias_pattern->setText(qsl("^qaAliasPat"));
        mpEditor->slot_saveProperty_AliasPattern();
        QCOMPARE(pAlias->getName(), qsl("^qaAliasPat"));

        mpEditor->mpAliasMainArea->lineEdit_alias_pattern->clear();
        mpEditor->slot_saveProperty_AliasPattern();
        QCOMPARE(pAlias->getName(), dlgTriggerEditor::tr("New alias"));

        // a name the pattern gave follows the pattern until the alias is saved
        mpEditor->mpAliasMainArea->lineEdit_alias_pattern->setText(pattern);
        mpEditor->slot_saveProperty_AliasPattern();
        QCOMPARE(pAlias->getName(), pattern);
        QCOMPARE(pItem->text(0), pattern);
        QCOMPARE(mpEditor->mpAliasMainArea->lineEdit_alias_name->text(), pattern);
    }

    // Grabbing a key switches the toolbars' shortcuts off so the grab sees the
    // key press; however the grab ends, every one of them has to come back (#7812)
    void test_toolbarShortcutsComeBackAfterGrabbingAKey_data()
    {
        QTest::addColumn<int>("key");
        QTest::newRow("grabbed") << static_cast<int>(Qt::Key_F10);
        QTest::newRow("cancelled") << static_cast<int>(Qt::Key_Escape);
    }

    void test_toolbarShortcutsComeBackAfterGrabbingAKey()
    {
        QFETCH(int, key);
        mpEditor->slot_showKeys();
        const QList<QAction*> actions = mpEditor->toolbarActions();
        QHash<QAction*, QList<QKeySequence>> before;
        for (auto* pAction : actions) {
            before.insert(pAction, pAction->shortcuts());
        }
        QCOMPARE(mpEditor->mAddItem->shortcut(), QKeySequence(QKeySequence::New));

        mpEditor->slot_keyGrab();
        QVERIFY2(mpEditor->mAddItem->shortcut().isEmpty(), "the add shortcut stayed live while a key was being grabbed");
        QTest::keyClick(mpEditor, static_cast<Qt::Key>(key));
        QVERIFY2(!mpEditor->mIsGrabKey, "the key press did not end the grab");

        for (auto* pAction : actions) {
            QVERIFY2(pAction->shortcuts() == before.value(pAction), qPrintable(qsl("\"%1\" lost its shortcut after grabbing a key").arg(pAction->text())));
        }

        // clicking "grab key" again mid-grab must not hold aside the already cleared shortcuts
        mpEditor->slot_keyGrab();
        mpEditor->slot_keyGrab();
        QTest::keyClick(mpEditor, Qt::Key_Escape);
        QCOMPARE(mpEditor->mAddItem->shortcut(), QKeySequence(QKeySequence::New));
    }

    // The steps from #7812, by keyboard only: a grab leaves focus on the binding
    // field, outside the tree that Ctrl+N is scoped to
    void test_ctrlNAddsAnotherKeyStraightAfterGrabbingOne()
    {
        // counts rows, not "New key"s: adding the second key saves the first under its binding's name
        auto countKeys = [this]() {
            int count = 0;
            for (QTreeWidgetItemIterator it(mpEditor->treeWidget_keys); *it; ++it) {
                ++count;
            }
            return count;
        };
        mpEditor->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mpEditor));
        mpEditor->slot_showKeys();
        mpEditor->treeWidget_keys->setCurrentItem(mpEditor->mpKeyBaseItem);
        QTRY_VERIFY(mpEditor->treeWidget_keys->hasFocus());
        const int before = countKeys();

        QTest::keyClick(QApplication::focusWidget(), Qt::Key_N, Qt::ControlModifier);
        QCOMPARE(countKeys(), before + 1);

        auto* pField = mpEditor->mpKeysMainArea->lineEdit_key_binding;
        pField->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY(pField->hasFocus());
        QTest::keyClick(pField, Qt::Key_Space);
        QVERIFY2(mpEditor->mIsGrabKey, "pressing Space on the binding field did not start a grab");
        QTest::keyClick(pField, Qt::Key_F10);
        QVERIFY2(!mpEditor->mIsGrabKey, "the key press did not end the grab");
        QCOMPARE(mpEditor->mpKeysMainArea->lineEdit_key_binding->text(), qsl("F10"));

        QTest::keyClick(QApplication::focusWidget(), Qt::Key_N, Qt::ControlModifier);
        QVERIFY2(countKeys() == before + 2, "Ctrl+N straight after grabbing a key did not add another key");
    }
};

#include "EditorAddItemTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorAddItemTest)
