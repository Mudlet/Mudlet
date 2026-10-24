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
 * An editor view whose tree has items, but only its root or nothing current,
 * shows its help text rather than a form, since a form tied to no item can be
 * typed into but never saved - and showing the help must not write the emptied
 * form over the item that was on screen before.
 *
 * Run with: ctest -R EditorEmptySelectionTest -V
 */

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgActionMainArea.h"
#include "dlgAliasMainArea.h"
#include "dlgKeysMainArea.h"
#include "dlgScriptsMainArea.h"
#include "dlgSourceEditorArea.h"
#include "dlgSystemMessageArea.h"
#include "dlgTimersMainArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggersMainArea.h"
#include "dlgVarsMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorEmptySelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorEmptySelection-Test-Profile");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    dlgTriggerEditor* editor() const { return HostDialogs::of(mpHost).mpEditorDialog; }

    struct ViewParts
    {
        QTreeWidget* tree = nullptr;
        QWidget* form = nullptr;
    };

    ViewParts partsOf(const EditorViewType view) const
    {
        dlgTriggerEditor* pEditor = editor();
        switch (view) {
        case EditorViewType::cmTriggerView:
            return {pEditor->treeWidget_triggers, pEditor->mpTriggersMainArea};
        case EditorViewType::cmTimerView:
            return {pEditor->treeWidget_timers, pEditor->mpTimersMainArea};
        case EditorViewType::cmAliasView:
            return {pEditor->treeWidget_aliases, pEditor->mpAliasMainArea};
        case EditorViewType::cmScriptView:
            return {pEditor->treeWidget_scripts, pEditor->mpScriptsMainArea};
        case EditorViewType::cmKeysView:
            return {pEditor->treeWidget_keys, pEditor->mpKeysMainArea};
        case EditorViewType::cmActionView:
            return {pEditor->treeWidget_actions, pEditor->mpActionsMainArea};
        case EditorViewType::cmVarsView:
            return {pEditor->treeWidget_variables, pEditor->mpVarsMainArea};
        case EditorViewType::cmUnknownView:
            break;
        }
        return {};
    }

    void showView(const EditorViewType view)
    {
        dlgTriggerEditor* pEditor = editor();
        switch (view) {
        case EditorViewType::cmTriggerView:
            pEditor->slot_showTriggers();
            return;
        case EditorViewType::cmTimerView:
            pEditor->slot_showTimers();
            return;
        case EditorViewType::cmAliasView:
            pEditor->slot_showAliases();
            return;
        case EditorViewType::cmScriptView:
            pEditor->slot_showScripts();
            return;
        case EditorViewType::cmKeysView:
            pEditor->slot_showKeys();
            return;
        case EditorViewType::cmActionView:
            pEditor->slot_showActions();
            return;
        case EditorViewType::cmVarsView:
            pEditor->slot_showVariables();
            return;
        case EditorViewType::cmUnknownView:
            return;
        }
    }

    // Forms and help both live in the editor, so what counts is which of them it would draw
    static bool shownInEditor(const QWidget* pWidget, const dlgTriggerEditor* pEditor) { return pWidget->isVisibleTo(pEditor); }

    void verifyShowsTheHelp(const EditorViewType view)
    {
        dlgTriggerEditor* pEditor = editor();
        const ViewParts parts = partsOf(view);
        const QTreeWidgetItem* pRoot = parts.tree->topLevelItem(0);
        QVERIFY2(pRoot && pRoot->childCount(), "the tree has no items under its root, so the help would be shown for that reason alone");
        QVERIFY2(!shownInEditor(parts.form, pEditor), "the view shows a form that belongs to no item");
        QVERIFY2(!shownInEditor(pEditor->mpSourceEditorArea, pEditor), "the view shows a script editor that belongs to no item");
        QCOMPARE(pEditor->mCurrentBannerKey, qsl("intro"));
        QVERIFY2(shownInEditor(pEditor->mpSystemMessageArea, pEditor), "the help text is not on screen");
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
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "no active host after creating the profile");
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "could not connect the profile to the stub server");

        mudlet::self()->slot_showTriggerDialog();
        QVERIFY2(editor(), "the profile has no editor");
        const QList<EditorViewType> itemViews{EditorViewType::cmTimerView, EditorViewType::cmAliasView, EditorViewType::cmScriptView, EditorViewType::cmKeysView, EditorViewType::cmActionView};
        for (const auto view : itemViews) {
            showView(view);
            QTreeWidget* pTree = partsOf(view).tree;
            pTree->setCurrentItem(pTree->topLevelItem(0));
            editor()->slot_addNewItem();
            editor()->slot_saveEdits();
        }
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("qaEmptySelectionVariable = 1")));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mProfileName);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_openingTheEditorShowsTheTriggerHelp()
    {
        delete editor();
        QVERIFY(!editor());
        mudlet::self()->slot_showTriggerDialog();
        QVERIFY2(editor(), "opening the editor built no editor");
        verifyShowsTheHelp(EditorViewType::cmTriggerView);
    }

    void test_showingAViewWithNoItemSelectedShowsItsHelp_data()
    {
        QTest::addColumn<int>("view");
        QTest::addColumn<bool>("rootIsCurrent");
        const QList<std::pair<const char*, EditorViewType>> views{{"trigger", EditorViewType::cmTriggerView},
                                                                  {"timer", EditorViewType::cmTimerView},
                                                                  {"alias", EditorViewType::cmAliasView},
                                                                  {"script", EditorViewType::cmScriptView},
                                                                  {"key", EditorViewType::cmKeysView},
                                                                  {"button", EditorViewType::cmActionView}};
        for (const auto& [name, view] : views) {
            QTest::addRow("%s, root current", name) << static_cast<int>(view) << true;
            QTest::addRow("%s, nothing current", name) << static_cast<int>(view) << false;
        }
        // Showing the variables refills their tree, which leaves nothing current
        QTest::addRow("variable") << static_cast<int>(EditorViewType::cmVarsView) << false;
    }

    void test_showingAViewWithNoItemSelectedShowsItsHelp()
    {
        QFETCH(int, view);
        QFETCH(bool, rootIsCurrent);
        const auto viewType = static_cast<EditorViewType>(view);
        dlgTriggerEditor* pEditor = editor();
        // A trigger of this row's own, so a row that wipes one leaves the others theirs
        const QString triggerName = qsl("qaEmptySelection%1%2").arg(view).arg(rootIsCurrent ? qsl("Root") : qsl("None"));
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("permRegexTrigger(\"%1\", \"\", {\"^%1$\"}, \"\")").arg(triggerName)));
        pEditor->doCleanReset();
        QVERIFY(QTest::qWaitFor([pEditor]() {
            return !pEditor->mCleanResetQueued;
        }));

        // Coming from a trigger that is on screen, so its form and script editor are up. One
        // with no error or package warning, as either keeps the place the help is checked for.
        const QList<QTreeWidgetItem*> found = pEditor->treeWidget_triggers->findItems(triggerName, Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
        QCOMPARE(found.size(), 1);
        QTreeWidgetItem* pTrigger = found.constFirst();
        TTrigger* pTriggerItem = mpHost->getTriggerUnit()->getTrigger(pTrigger->data(0, Qt::UserRole).toInt());
        QVERIFY(pTriggerItem);
        const int triggerID = pTriggerItem->getID();
        const QStringList triggerPatterns = pTriggerItem->getPatternsList();
        pEditor->treeWidget_triggers->setCurrentItem(pTrigger);
        pEditor->slot_triggerSelected(pTrigger);
        QVERIFY2(shownInEditor(pEditor->mpSourceEditorArea, pEditor), "the trigger's script editor is not up, so nothing is left for the next view to hide");

        QTreeWidget* pTree = partsOf(viewType).tree;
        if (viewType != EditorViewType::cmVarsView) {
            QTreeWidgetItem* pRoot = pTree->topLevelItem(0);
            QVERIFY(pRoot);
            // Unblocked, the selection handler would show the help before the view is asked to
            const QSignalBlocker blocker(pTree->selectionModel());
            pTree->setCurrentItem(rootIsCurrent ? pRoot : nullptr);
        }
        showView(viewType);
        QVERIFY(!pTree->currentItem() || !pTree->currentItem()->parent());
        verifyShowsTheHelp(viewType);
        if (QTest::currentTestFailed()) {
            return;
        }

        pEditor->slot_saveEdits();
        TTrigger* pSaved = mpHost->getTriggerUnit()->getTrigger(triggerID);
        QVERIFY2(pSaved, "the trigger that was on screen is gone after saving");
        QCOMPARE(pSaved->getName(), triggerName);
        QCOMPARE(pSaved->getPatternsList(), triggerPatterns);
    }
};

#include "EditorEmptySelectionTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorEmptySelectionTest)
