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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "EditorUndoStack.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ScriptUnit.h"
#include "TScript.h"
#include "TTreeWidget.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgScriptsMainArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Run with: ctest -R ScriptEventHandlerLifetimeTest -V
//
// These pin down the noted "Add User Event" item being dropped whenever the "Registered
// Events" list is torn down, so "+" cannot reach a freed item (#9835). For why the note
// outlives the items at all, see the comment on the
// slot_scriptMainAreaClearHandlerSelection() call in dlgTriggerEditor::slot_scriptsSelected().
class ScriptEventHandlerLifetimeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ScriptEventHandlerLifetime-Test-Profile");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void startProfile(const QString& profileName, const QString& address, const QString& port)
    {
        mpHost = TestProfile::create(profileName, address, port);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    QListWidget* handlerList() const { return mpEditor->mpScriptsMainArea->listWidget_script_registered_event_handlers; }

    QLineEdit* handlerEntry() const { return mpEditor->mpScriptsMainArea->lineEdit_script_event_handler_entry; }

    QStringList savedHandlersOf(QTreeWidgetItem* pTreeItem) const
    {
        if (!pTreeItem) {
            return {};
        }
        TScript* pScript = mpHost->getScriptUnit()->getScript(pTreeItem->data(0, Qt::UserRole).toInt());
        return pScript ? pScript->getEventHandlerList() : QStringList{};
    }

    QTreeWidgetItem* addSavedScript(const QString& name, const QStringList& handlers)
    {
        mpEditor->treeWidget_scripts->setCurrentItem(mpEditor->mpScriptsBaseItem);
        mpEditor->addScript(false);
        QTest::qWait(50ms);

        QTreeWidgetItem* pTreeItem = mpEditor->mpCurrentScriptItem;
        if (!pTreeItem) {
            QTest::qFail("addScript() left no current script item", __FILE__, __LINE__);
            return nullptr;
        }
        mpEditor->mpScriptsMainArea->lineEdit_script_name->setText(name);
        for (const QString& handler : handlers) {
            handlerEntry()->setText(handler);
            mpEditor->slot_scriptMainAreaAddHandler();
        }
        if (handlerList()->count() != handlers.count()) {
            QTest::qFail("the handlers did not all reach the Registered Events list", __FILE__, __LINE__);
            return nullptr;
        }
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);
        return pTreeItem;
    }

    QTreeWidgetItem* findEventHandlerSearchResult() const
    {
        QList<QTreeWidgetItem*> pending;
        for (int i = 0; i < mpEditor->treeWidget_searchResults->topLevelItemCount(); ++i) {
            pending.append(mpEditor->treeWidget_searchResults->topLevelItem(i));
        }
        while (!pending.isEmpty()) {
            QTreeWidgetItem* pResult = pending.takeFirst();
            if (pResult->data(0, dlgTriggerEditor::TypeRole).toInt() == dlgTriggerEditor::SearchResultIsEventHandler) {
                return pResult;
            }
            for (int i = 0; i < pResult->childCount(); ++i) {
                pending.append(pResult->child(i));
            }
        }
        return nullptr;
    }

    void removeScripts(const QList<QTreeWidgetItem*>& treeItems)
    {
        for (QTreeWidgetItem* pTreeItem : treeItems) {
            mpEditor->treeWidget_scripts->setCurrentItem(pTreeItem);
            mpEditor->slot_deleteItemOrGroup();
            QTest::qWait(20ms);
        }
        mpEditor->mpUndoStack->clear();
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
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
        startProfile(mProfileName, mLocalhost, mPort);

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(200ms);

        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->slot_showScripts();
        QTest::qWait(100ms);
    }

    void init()
    {
        if (!mpEditor) {
            QFAIL("the editor was never created, the profile setup must have failed");
        }
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
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void testSwitchingScriptsDropsTheNotedHandler()
    {
        QTreeWidgetItem* pScriptA = addSavedScript(qsl("ScriptA"), {qsl("myTestEvent")});
        QTreeWidgetItem* pScriptB = addSavedScript(qsl("ScriptB"), {});

        mpEditor->treeWidget_scripts->setCurrentItem(pScriptA);
        QTest::qWait(50ms);
        QCOMPARE(handlerList()->count(), 1);

        // same signal path as a real click on the entry
        handlerList()->setCurrentRow(0);
        QTest::qWait(50ms);
        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, handlerList()->item(0));
        QCOMPARE(mpEditor->mIsScriptsMainAreaEditHandler, true);
        QCOMPARE(handlerEntry()->text(), qsl("myTestEvent"));

        mpEditor->treeWidget_scripts->setCurrentItem(pScriptB);
        QTest::qWait(50ms);

        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, nullptr);
        QCOMPARE(mpEditor->mIsScriptsMainAreaEditHandler, false);
        QCOMPARE(handlerEntry()->text(), QString());

        handlerEntry()->setText(qsl("otherEvent"));
        mpEditor->slot_scriptMainAreaAddHandler();
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);

        QCOMPARE(savedHandlersOf(pScriptB), QStringList{qsl("otherEvent")});
        QCOMPARE(savedHandlersOf(pScriptA), QStringList{qsl("myTestEvent")});

        removeScripts({pScriptA, pScriptB});
    }

    // One click on a tree entry emits itemSelectionChanged and then itemClicked, so
    // slot_scriptsSelected() runs twice and the second run, on the already-current item,
    // is an easy-to-miss second teardown. The replacement item can land on the freed one,
    // in which case "+" silently renames it instead of crashing.
    void testReselectingTheSameScriptDropsTheNotedHandler()
    {
        QTreeWidgetItem* pScript = addSavedScript(qsl("SoloScript"), {qsl("myTestEvent")});

        mpEditor->treeWidget_scripts->setCurrentItem(pScript);
        QTest::qWait(50ms);
        QCOMPARE(handlerList()->count(), 1);

        handlerList()->setCurrentRow(0);
        QTest::qWait(50ms);
        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, handlerList()->item(0));

        emit mpEditor->treeWidget_scripts->itemClicked(pScript, 0);
        QTest::qWait(50ms);

        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, nullptr);
        QCOMPARE(mpEditor->mIsScriptsMainAreaEditHandler, false);

        handlerEntry()->setText(qsl("secondEvent"));
        mpEditor->slot_scriptMainAreaAddHandler();
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);

        QCOMPARE(savedHandlersOf(pScript), (QStringList{qsl("myTestEvent"), qsl("secondEvent")}));

        removeScripts({pScript});
    }

    // Dropping the note takes the "Add User Event" text with it, so a half-typed name
    // cannot follow the user to the next script and land on that one instead.
    void testTypedButUnaddedTextDoesNotFollowToTheNextScript()
    {
        QTreeWidgetItem* pScriptA = addSavedScript(qsl("TypedTextA"), {});
        QTreeWidgetItem* pScriptB = addSavedScript(qsl("TypedTextB"), {});

        mpEditor->treeWidget_scripts->setCurrentItem(pScriptA);
        QTest::qWait(50ms);
        handlerEntry()->setText(qsl("neverAddedEvent"));

        mpEditor->treeWidget_scripts->setCurrentItem(pScriptB);
        QTest::qWait(50ms);
        QCOMPARE(handlerEntry()->text(), QString());

        mpEditor->slot_scriptMainAreaAddHandler();
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);

        QCOMPARE(savedHandlersOf(pScriptB), QStringList{});
        QCOMPARE(savedHandlersOf(pScriptA), QStringList{});

        removeScripts({pScriptA, pScriptB});
    }

    // addScript() points mpCurrentScriptItem at the new script before selecting it, so
    // that selection skips the save as well while still tearing the list down.
    void testAddingAScriptDropsTheNotedHandler()
    {
        QTreeWidgetItem* pScript = addSavedScript(qsl("BeforeNewScript"), {qsl("myTestEvent")});

        mpEditor->treeWidget_scripts->setCurrentItem(pScript);
        QTest::qWait(50ms);
        handlerList()->setCurrentRow(0);
        QTest::qWait(50ms);
        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, handlerList()->item(0));

        mpEditor->addScript(false);
        QTest::qWait(50ms);
        QTreeWidgetItem* pNewScript = mpEditor->mpCurrentScriptItem;
        QVERIFY(pNewScript != pScript);

        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, nullptr);
        QCOMPARE(mpEditor->mIsScriptsMainAreaEditHandler, false);

        mpEditor->mpScriptsMainArea->lineEdit_script_name->setText(qsl("AfterNewScript"));
        handlerEntry()->setText(qsl("brandNewEvent"));
        mpEditor->slot_scriptMainAreaAddHandler();
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);

        QCOMPARE(savedHandlersOf(pNewScript), QStringList{qsl("brandNewEvent")});
        QCOMPARE(savedHandlersOf(pScript), QStringList{qsl("myTestEvent")});

        removeScripts({pScript, pNewScript});
    }

    // Nothing tears the list down between selecting the entry and pressing "+", so the
    // note has to survive here - guards against dropping it too eagerly.
    void testRenamingASelectedHandlerStillWorks()
    {
        QTreeWidgetItem* pScript = addSavedScript(qsl("RenameScript"), {qsl("firstEvent")});

        mpEditor->treeWidget_scripts->setCurrentItem(pScript);
        QTest::qWait(50ms);
        handlerList()->setCurrentRow(0);
        QTest::qWait(50ms);

        handlerEntry()->setText(qsl("renamedEvent"));
        mpEditor->slot_scriptMainAreaAddHandler();
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);

        QCOMPARE(savedHandlersOf(pScript), QStringList{qsl("renamedEvent")});

        removeScripts({pScript});
    }

    // The "-" button takes the row out of the list widget, which hands its
    // ownership over, so this also holds the leak checker over that path.
    void testDeletingAHandlerReleasesIt()
    {
        QTreeWidgetItem* pScript = addSavedScript(qsl("DeleteScript"), {qsl("firstEvent"), qsl("secondEvent")});

        mpEditor->treeWidget_scripts->setCurrentItem(pScript);
        QTest::qWait(50ms);
        QCOMPARE(handlerList()->count(), 2);

        handlerList()->setCurrentRow(0);
        QTest::qWait(50ms);
        mpEditor->slot_scriptMainAreaDeleteHandler();
        QTest::qWait(50ms);

        QCOMPARE(handlerList()->count(), 1);
        QCOMPARE(handlerList()->item(0)->text(), qsl("secondEvent"));
        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, nullptr);
        QCOMPARE(mpEditor->mIsScriptsMainAreaEditHandler, false);

        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);
        QCOMPARE(savedHandlersOf(pScript), QStringList{qsl("secondEvent")});

        removeScripts({pScript});
    }

    // slot_itemSelectedInSearchResults() notes the item by hand rather than through the
    // list widget's selection, and only after every teardown of the list - so that note
    // has to survive, and the next script selection has to drop it.
    void testSearchResultNotesALiveHandler()
    {
        QTreeWidgetItem* pScriptA = addSavedScript(qsl("SearchScript"), {qsl("searchableEvent")});
        QTreeWidgetItem* pScriptB = addSavedScript(qsl("OtherScript"), {});

        mpEditor->treeWidget_scripts->setCurrentItem(pScriptB);
        QTest::qWait(50ms);

        mpEditor->comboBox_searchTerms->insertItem(0, qsl("searchableEvent"));
        mpEditor->slot_searchMudletItems(0);
        QTest::qWait(50ms);

        QTreeWidgetItem* pResult = findEventHandlerSearchResult();
        QVERIFY2(pResult != nullptr, "the search found no event handler result to jump to");

        mpEditor->slot_itemSelectedInSearchResults(pResult);
        QTest::qWait(50ms);

        QCOMPARE(mpEditor->mpCurrentScriptItem, pScriptA);
        QCOMPARE(handlerList()->count(), 1);
        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, handlerList()->item(0));
        QCOMPARE(mpEditor->mIsScriptsMainAreaEditHandler, true);
        QCOMPARE(handlerEntry()->text(), qsl("searchableEvent"));

        mpEditor->treeWidget_scripts->setCurrentItem(pScriptB);
        QTest::qWait(50ms);
        QCOMPARE(mpEditor->mpScriptsMainAreaEditHandlerItem, nullptr);
        QCOMPARE(mpEditor->mIsScriptsMainAreaEditHandler, false);

        handlerEntry()->setText(qsl("afterSearchEvent"));
        mpEditor->slot_scriptMainAreaAddHandler();
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);

        QCOMPARE(savedHandlersOf(pScriptB), QStringList{qsl("afterSearchEvent")});
        QCOMPARE(savedHandlersOf(pScriptA), QStringList{qsl("searchableEvent")});

        removeScripts({pScriptA, pScriptB});
    }
};

#include "ScriptEventHandlerLifetimeTest.moc"
MUDLET_GROUPED_TEST_MAIN(ScriptEventHandlerLifetimeTest)
