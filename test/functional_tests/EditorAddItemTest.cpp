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

#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>

#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "HostDialogs.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ScriptUnit.h"
#include "TKey.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgKeysMainArea.h"
#include "dlgScriptsMainArea.h"
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

    // The first save finishes a new item, switching it on even when nothing was
    // typed, so it belongs to the add: one undo has to take the item away (#10774).
    void test_oneUndoTakesBackAnItemAddedAndSaved()
    {
        const QString newTrigger = dlgTriggerEditor::tr("New trigger");
        mpEditor->slot_showTriggers();
        mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->mpTriggerBaseItem);
        const auto existing = mpHost->getTriggerUnit()->findItems(newTrigger, true, true);
        const int before = static_cast<int>(existing.size());
        const int commandsBefore = mpEditor->mpUndoStack->count();

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
        QCOMPARE(mpEditor->mpUndoStack->count(), commandsBefore + 1);

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
};

#include "EditorAddItemTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorAddItemTest)
