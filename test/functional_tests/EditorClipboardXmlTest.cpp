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
 * Copying an editor item puts Mudlet's own XML on the system clipboard and
 * pasting reads it back into a new item. Neither half is exposed to Lua - the
 * scripting API can create items but never round-trips one through the
 * clipboard - so only a functional test can check the transfer format, the
 * multi-item separator, and that a paste lands on the undo stack.
 */

#include <QClipboard>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "ActionUnit.h"
#include "AliasUnit.h"
#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorClipboardXmlTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("EditorClipboard-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    QTreeWidget* treeFor(EditorViewType view) const
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            return mpEditor->treeWidget_triggers;
        case EditorViewType::cmTimerView:
            return mpEditor->treeWidget_timers;
        case EditorViewType::cmAliasView:
            return mpEditor->treeWidget_aliases;
        case EditorViewType::cmScriptView:
            return mpEditor->treeWidget_scripts;
        case EditorViewType::cmActionView:
            return mpEditor->treeWidget_actions;
        case EditorViewType::cmKeysView:
            return mpEditor->treeWidget_keys;
        default:
            return nullptr;
        }
    }

    void showView(EditorViewType view)
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            mpEditor->slot_showTriggers();
            break;
        case EditorViewType::cmTimerView:
            mpEditor->slot_showTimers();
            break;
        case EditorViewType::cmAliasView:
            mpEditor->slot_showAliases();
            break;
        case EditorViewType::cmScriptView:
            mpEditor->slot_showScripts();
            break;
        case EditorViewType::cmActionView:
            mpEditor->slot_showActions();
            break;
        case EditorViewType::cmKeysView:
            mpEditor->slot_showKeys();
            break;
        default:
            break;
        }
    }

    int countNamed(EditorViewType view, const QString& name) const
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            return static_cast<int>(mpHost->getTriggerUnit()->findItems(name, true, true).size());
        case EditorViewType::cmTimerView:
            return static_cast<int>(mpHost->getTimerUnit()->findItems(name, true, true).size());
        case EditorViewType::cmAliasView:
            return static_cast<int>(mpHost->getAliasUnit()->findItems(name, true, true).size());
        case EditorViewType::cmScriptView:
            return static_cast<int>(mpHost->getScriptUnit()->findItems(name, true, true).size());
        case EditorViewType::cmActionView:
            return static_cast<int>(mpHost->getActionUnit()->findItems(name, true, true).size());
        case EditorViewType::cmKeysView:
            return static_cast<int>(mpHost->getKeyUnit()->findItems(name, true, true).size());
        default:
            return -1;
        }
    }

    bool selectInTree(EditorViewType view, const QStringList& names)
    {
        QTreeWidget* tree = treeFor(view);
        if (!tree) {
            return false;
        }
        tree->clearSelection();
        for (const QString& name : names) {
            const QList<QTreeWidgetItem*> found = tree->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
            if (found.isEmpty()) {
                return false;
            }
            found.first()->setSelected(true);
            tree->setCurrentItem(found.first(), 0, QItemSelectionModel::NoUpdate);
        }
        return true;
    }

    void populateProfile()
    {
        auto* pTrigger = new TTrigger(nullptr, mpHost);
        pTrigger->setName(qsl("qaClipTrigger"));
        pTrigger->setRegexCodeList({qsl("^qaClipTrigger$")}, {REGEX_PERL});
        pTrigger->setScript(qsl("echo(\"clip\")\n"));
        QVERIFY(pTrigger->registerTrigger());

        auto* pSecondTrigger = new TTrigger(nullptr, mpHost);
        pSecondTrigger->setName(qsl("qaClipTriggerTwo"));
        pSecondTrigger->setRegexCodeList({qsl("^qaClipTriggerTwo$")}, {REGEX_PERL});
        QVERIFY(pSecondTrigger->registerTrigger());

        auto* pAlias = new TAlias(qsl("qaClipAlias"), mpHost);
        pAlias->setRegexCode(qsl("^qaClipAlias$"));
        mpHost->getAliasUnit()->registerAlias(pAlias);

        auto* pScript = new TScript(qsl("qaClipScript"), mpHost);
        pScript->setScript(qsl("-- clip\n"));
        mpHost->getScriptUnit()->registerScript(pScript);

        auto* pTimer = new TTimer(qsl("qaClipTimer"), QTime(0, 0, 30), mpHost);
        mpHost->getTimerUnit()->registerTimer(pTimer);

        auto* pKey = new TKey(qsl("qaClipKey"), mpHost);
        pKey->setKeyCode(Qt::Key_F9);
        mpHost->getKeyUnit()->registerKey(pKey);

        auto* pAction = new TAction(qsl("qaClipButton"), mpHost);
        mpHost->getActionUnit()->registerAction(pAction);
    }

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
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(1000), "Could not connect with the host.");

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");

        populateProfile();
        // The editor built its trees while the profile loaded, so items added
        // behind its back only appear after the rebuild that Host asks for when
        // a package is installed - and that rebuild is only queued
        mpEditor->doCleanReset();
        QTest::qWait(100ms);
    }

    void cleanupTestCase()
    {
        if (mpHost) {
            if (auto* pEditor = mpHost->mpEditorDialog.data()) {
                mpHost->mpEditorDialog = nullptr;
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

    void test_copyPutsTheItemsOwnXmlOnTheClipboard_data()
    {
        QTest::addColumn<int>("view");
        QTest::addColumn<QString>("itemName");
        QTest::addColumn<QString>("packageTag");

        QTest::newRow("trigger") << static_cast<int>(EditorViewType::cmTriggerView) << "qaClipTrigger" << "TriggerPackage";
        QTest::newRow("timer") << static_cast<int>(EditorViewType::cmTimerView) << "qaClipTimer" << "TimerPackage";
        QTest::newRow("alias") << static_cast<int>(EditorViewType::cmAliasView) << "qaClipAlias" << "AliasPackage";
        QTest::newRow("script") << static_cast<int>(EditorViewType::cmScriptView) << "qaClipScript" << "ScriptPackage";
        QTest::newRow("button") << static_cast<int>(EditorViewType::cmActionView) << "qaClipButton" << "ActionPackage";
        QTest::newRow("key") << static_cast<int>(EditorViewType::cmKeysView) << "qaClipKey" << "KeyPackage";
    }

    void test_copyPutsTheItemsOwnXmlOnTheClipboard()
    {
        QFETCH(int, view);
        QFETCH(QString, itemName);
        QFETCH(QString, packageTag);
        const auto viewType = static_cast<EditorViewType>(view);

        QApplication::clipboard()->clear();
        showView(viewType);
        QVERIFY2(selectInTree(viewType, {itemName}), qPrintable(qsl("%1 is not in its tree").arg(itemName)));

        mpEditor->slot_copyXml();

        const QString clipboard = QApplication::clipboard()->text();
        QVERIFY2(clipboard.contains(qsl("<MudletPackage")), qPrintable(qsl("the clipboard does not hold a Mudlet package: %1").arg(clipboard.left(120))));
        QVERIFY2(clipboard.contains(qsl("<%1>").arg(packageTag)), qPrintable(qsl("the clipboard does not hold a %1").arg(packageTag)));
        QVERIFY2(clipboard.contains(itemName), "the copied XML does not name the item that was copied");
    }

    void test_pastingACopiedItemAddsASecondOne_data() { test_copyPutsTheItemsOwnXmlOnTheClipboard_data(); }

    void test_pastingACopiedItemAddsASecondOne()
    {
        QFETCH(int, view);
        QFETCH(QString, itemName);
        const auto viewType = static_cast<EditorViewType>(view);

        showView(viewType);
        QVERIFY(selectInTree(viewType, {itemName}));
        const int before = countNamed(viewType, itemName);
        QCOMPARE(before, 1);

        mpEditor->slot_copyXml();
        mpEditor->slot_pasteXml();

        QCOMPARE(countNamed(viewType, itemName), before + 1);
        QVERIFY2(treeFor(viewType)->findItems(itemName, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0).count() == before + 1, "the pasted item did not appear in the tree");
    }

    // Two selected items go onto the clipboard as one string split by a private
    // separator, which the paste has to notice and unpick
    void test_copyingTwoItemsPastesBothOfThem()
    {
        mpEditor->slot_showTriggers();
        QVERIFY(selectInTree(EditorViewType::cmTriggerView, {qsl("qaClipTriggerTwo")}));
        const int beforeTwo = countNamed(EditorViewType::cmTriggerView, qsl("qaClipTriggerTwo"));
        const int beforeOne = countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger"));
        QVERIFY(selectInTree(EditorViewType::cmTriggerView, {qsl("qaClipTrigger"), qsl("qaClipTriggerTwo")}));

        mpEditor->slot_copyXml();
        QVERIFY2(QApplication::clipboard()->text().contains(qsl("MUDLET_MULTI_ITEM_SEPARATOR")), "two copied items were not separated for a multiple paste");

        mpEditor->slot_pasteXml();

        QCOMPARE(countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger")), beforeOne + 1);
        QCOMPARE(countNamed(EditorViewType::cmTriggerView, qsl("qaClipTriggerTwo")), beforeTwo + 1);
        // The clipboard is rewritten one item at a time while pasting, so it has
        // to be put back the way it was found
        QVERIFY2(QApplication::clipboard()->text().contains(qsl("MUDLET_MULTI_ITEM_SEPARATOR")), "the multi-item clipboard was not restored after the paste");
    }

    void test_pastingSomethingThatIsNotAMudletItemChangesNothing()
    {
        mpEditor->slot_showTriggers();
        QVERIFY(selectInTree(EditorViewType::cmTriggerView, {qsl("qaClipTrigger")}));
        const int before = countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger"));

        QApplication::clipboard()->setText(qsl("just some text a user happened to copy"));
        mpEditor->slot_pasteXml();

        QCOMPARE(countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger")), before);
    }

    void test_aPasteCanBeUndoneAndRedone()
    {
        mpEditor->slot_showTriggers();
        QVERIFY(selectInTree(EditorViewType::cmTriggerView, {qsl("qaClipTrigger")}));
        const int before = countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger"));

        mpEditor->slot_copyXml();
        mpEditor->slot_pasteXml();
        QCOMPARE(countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger")), before + 1);

        QVERIFY2(mpEditor->mpUndoStack->canUndo(), "pasting an item did not put anything on the undo stack");
        mpEditor->mpUndoStack->undo();
        QCOMPARE(countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger")), before);

        mpEditor->mpUndoStack->redo();
        QCOMPARE(countNamed(EditorViewType::cmTriggerView, qsl("qaClipTrigger")), before + 1);
    }
};

#include "EditorClipboardXmlTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorClipboardXmlTest)
