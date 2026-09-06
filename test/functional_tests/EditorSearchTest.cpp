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
 * The editor's search box walks the seven item units itself and files each hit
 * into a result tree with private data roles saying which field it came from -
 * none of which Lua can see or drive, so a spec cannot check that a match in a
 * button's stylesheet or a script's event handler is found and labelled.
 */

#include <QComboBox>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ActionUnit.h"
#include "AliasUnit.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "TScript.h"
#include "TTimer.h"
#include "TimerUnit.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorSearchTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("EditorSearch-Test-Profile");
    const QString mLocalhost = qsl("localhost");
    // Deliberately a nonsense word, so every hit found is one this test planted
    const QString mNeedle = qsl("qaHaystack");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void search(const QString& term)
    {
        auto* box = mpEditor->comboBox_searchTerms;
        box->clear();
        box->addItem(term);
        mpEditor->slot_searchMudletItems(0);
    }

    // The result tree is two deep: the first field matched in an item becomes
    // that item's top-level row, the rest hang off it
    QStringList resultsFor(const QString& itemType) const
    {
        QStringList found;
        auto* results = mpEditor->treeWidget_searchResults;
        for (int i = 0, total = results->topLevelItemCount(); i < total; ++i) {
            QTreeWidgetItem* top = results->topLevelItem(i);
            if (top->text(0) != itemType) {
                continue;
            }
            found << top->text(2);
            for (int j = 0, children = top->childCount(); j < children; ++j) {
                found << top->child(j)->text(2);
            }
        }
        return found;
    }

    // The rebuild doCleanReset() asks for is only queued, so it is over when the
    // items planted behind the editor's back are in its trees - which is what
    // this waits for, rather than for a length of time the machine gets to
    // decide the meaning of.
    bool waitForTreeToHold(const QString& name) const
    {
        return QTest::qWaitFor([this, &name]() {
            return !mpEditor->treeWidget_triggers->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0).isEmpty();
        });
    }

    QTreeWidgetItem* topLevelResultFor(const QString& itemType) const
    {
        auto* results = mpEditor->treeWidget_searchResults;
        for (int i = 0, total = results->topLevelItemCount(); i < total; ++i) {
            if (results->topLevelItem(i)->text(0) == itemType) {
                return results->topLevelItem(i);
            }
        }
        return nullptr;
    }

    int totalResultRows() const
    {
        auto* results = mpEditor->treeWidget_searchResults;
        int total = 0;
        for (int i = 0, tops = results->topLevelItemCount(); i < tops; ++i) {
            total += 1 + results->topLevelItem(i)->childCount();
        }
        return total;
    }

    void populateProfile()
    {
        auto* pTrigger = new TTrigger(nullptr, mpHost);
        pTrigger->setName(qsl("qaSearchTrigger"));
        pTrigger->setRegexCodeList({qsl("^%1 pattern$").arg(mNeedle)}, {REGEX_PERL});
        pTrigger->setCommand(qsl("%1 command").arg(mNeedle));
        pTrigger->setScript(qsl("-- %1 in lua\nlocal unused = 1\n").arg(mNeedle));
        QVERIFY(pTrigger->registerTrigger());

        auto* pGroup = new TTrigger(nullptr, mpHost);
        pGroup->setName(qsl("qaSearchFolder"));
        pGroup->setIsFolder(true);
        QVERIFY(pGroup->registerTrigger());
        auto* pNested = new TTrigger(pGroup, mpHost);
        pNested->setName(qsl("qaNestedTrigger"));
        pNested->setRegexCodeList({qsl("%1 nested").arg(mNeedle)}, {REGEX_SUBSTRING});
        QVERIFY(pNested->registerTrigger());

        auto* pAlias = new TAlias(qsl("qaSearchAlias"), mpHost);
        pAlias->setRegexCode(qsl("^%1 alias$").arg(mNeedle));
        pAlias->setCommand(qsl("%1 command").arg(mNeedle));
        mpHost->getAliasUnit()->registerAlias(pAlias);

        auto* pScript = new TScript(qsl("qaSearchScript"), mpHost);
        pScript->setEventHandlerList({qsl("%1Event").arg(mNeedle)});
        pScript->setScript(qsl("-- %1 lives here too\n").arg(mNeedle));
        mpHost->getScriptUnit()->registerScript(pScript);

        auto* pTimer = new TTimer(qsl("qaSearchTimer"), QTime(0, 0, 30), mpHost);
        pTimer->setCommand(qsl("%1 command").arg(mNeedle));
        mpHost->getTimerUnit()->registerTimer(pTimer);

        auto* pKey = new TKey(qsl("qaSearchKey"), mpHost);
        pKey->setCommand(qsl("%1 command").arg(mNeedle));
        mpHost->getKeyUnit()->registerKey(pKey);

        auto* pAction = new TAction(qsl("qaSearchButton"), mpHost);
        pAction->setIsPushDownButton(true);
        pAction->setCommandButtonDown(qsl("%1 down").arg(mNeedle));
        pAction->setCommandButtonUp(qsl("%1 up").arg(mNeedle));
        pAction->css = qsl("QPushButton {\n  border: 1px solid %1;\n}").arg(mNeedle);
        mpHost->getActionUnit()->registerAction(pAction);

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("qaSearchVariable = \"%1 value\"").arg(mNeedle)));
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
        // The editor filled its trees when the profile loaded, so items added
        // behind its back are only in them after the same rebuild that Host
        // does when a package is installed
        mpEditor->doCleanReset();
        QVERIFY2(waitForTreeToHold(qsl("qaSearchTrigger")), "the editor never rebuilt its trees around the items this test planted");
        mpEditor->setSearchOptions(dlgTriggerEditor::SearchOptionNone);
    }

    void cleanupTestCase()
    {
        // ~Host would do this, but only if the host is ever destroyed - deleting
        // the editor here keeps the leak checker satisfied either way
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

    void cleanup()
    {
        if (mpEditor) {
            mpEditor->setSearchOptions(dlgTriggerEditor::SearchOptionNone);
        }
    }

    void test_findsTheNeedleInEveryItemType()
    {
        search(mNeedle);

        QCOMPARE(resultsFor(qsl("Trigger")), QStringList({qsl("Command"), qsl("Pattern {1}"), qsl("Lua code (1:4)"), qsl("Pattern {1}")}));
        QCOMPARE(resultsFor(qsl("Alias")), QStringList({qsl("Command"), qsl("Pattern")}));
        QCOMPARE(resultsFor(qsl("Script")), QStringList({qsl("Event Handler"), qsl("Lua code (1:4)")}));
        QCOMPARE(resultsFor(qsl("Timer")), QStringList({qsl("Command")}));
        QCOMPARE(resultsFor(qsl("Key")), QStringList({qsl("Command")}));
        QCOMPARE(resultsFor(qsl("Button")), QStringList({qsl("Command {Down}"), qsl("Command {Up}"), qsl("Stylesheet {L: 2 C: 21}")}));
    }

    // A match inside a folder is only found by the recursive walk, not by the
    // pass over the unit's root nodes
    void test_findsMatchesNestedInsideAFolder()
    {
        search(qsl("%1 nested").arg(mNeedle));

        auto* result = topLevelResultFor(qsl("Trigger"));
        QVERIFY2(result, "the nested trigger was not found at all");
        QCOMPARE(result->text(1), qsl("qaNestedTrigger"));
        QCOMPARE(result->text(2), qsl("Pattern {1}"));
    }

    // Every row carries the item type, its id and which field matched, so that
    // clicking it can jump to the right control
    void test_recordsWhereEachMatchCameFrom()
    {
        search(qsl("qaSearchButton"));

        auto* result = topLevelResultFor(qsl("Button"));
        QVERIFY(result);
        QCOMPARE(result->text(2), qsl("Name"));
        QCOMPARE(static_cast<EditorViewType>(result->data(0, dlgTriggerEditor::ItemRole).toInt()), EditorViewType::cmActionView);
        QCOMPARE(result->data(0, dlgTriggerEditor::TypeRole).toInt(), static_cast<int>(dlgTriggerEditor::SearchResultIsName));
        QCOMPARE(result->data(0, dlgTriggerEditor::NameRole).toString(), qsl("qaSearchButton"));

        TAction* pAction = mpHost->getActionUnit()->findAction(qsl("qaSearchButton"));
        QVERIFY(pAction);
        QCOMPARE(result->data(0, dlgTriggerEditor::IdRole).toInt(), pAction->getID());
    }

    void test_caseSensitivityIsOptional()
    {
        search(qsl("QAHAYSTACK"));
        const int caseInsensitiveRows = totalResultRows();
        QVERIFY2(caseInsensitiveRows > 0, "a differently-cased needle found nothing while matching case-insensitively");

        mpEditor->setSearchOptions(dlgTriggerEditor::SearchOptionCaseSensitive);
        search(qsl("QAHAYSTACK"));
        QCOMPARE(totalResultRows(), 0);

        search(mNeedle);
        QCOMPARE(totalResultRows(), caseInsensitiveRows);
    }

    void test_wholeWordSearchIgnoresMatchesInsideALongerWord()
    {
        search(qsl("qaSearchTrig"));
        QVERIFY2(totalResultRows() > 0, "a partial word found nothing while matching on substrings");

        mpEditor->setSearchOptions(dlgTriggerEditor::SearchOptionWholeWord);
        search(qsl("qaSearchTrig"));
        QCOMPARE(totalResultRows(), 0);

        search(qsl("qaSearchTimer"));
        QVERIFY2(totalResultRows() > 0, "a whole word was not matched in whole-word mode");
    }

    // Variables live in the Lua state rather than in a unit, so they are only
    // walked when the option asks for it
    void test_variablesAreOnlySearchedWhenAskedFor()
    {
        search(qsl("qaSearchVariable"));
        QVERIFY2(!topLevelResultFor(qsl("Variable")), "a variable was searched without the option being set");

        mpEditor->setSearchOptions(dlgTriggerEditor::SearchOptionIncludeVariables);
        search(qsl("qaSearchVariable"));
        auto* result = topLevelResultFor(qsl("Variable"));
        QVERIFY2(result, "the variable was not found with the option set");
        QCOMPARE(result->text(1), qsl("qaSearchVariable"));
        QCOMPARE(result->data(0, dlgTriggerEditor::TypeRole).toInt(), static_cast<int>(dlgTriggerEditor::SearchResultIsName));
    }

    void test_selectingAResultSwitchesToThatItemsView_data()
    {
        QTest::addColumn<QString>("itemName");
        QTest::addColumn<int>("expectedView");

        QTest::newRow("trigger") << "qaSearchTrigger" << static_cast<int>(EditorViewType::cmTriggerView);
        QTest::newRow("alias") << "qaSearchAlias" << static_cast<int>(EditorViewType::cmAliasView);
        QTest::newRow("script") << "qaSearchScript" << static_cast<int>(EditorViewType::cmScriptView);
        QTest::newRow("timer") << "qaSearchTimer" << static_cast<int>(EditorViewType::cmTimerView);
        QTest::newRow("key") << "qaSearchKey" << static_cast<int>(EditorViewType::cmKeysView);
        QTest::newRow("button") << "qaSearchButton" << static_cast<int>(EditorViewType::cmActionView);
    }

    void test_selectingAResultSwitchesToThatItemsView()
    {
        QFETCH(QString, itemName);
        QFETCH(int, expectedView);

        // start somewhere other than the view being switched to, so that the
        // switch itself is what the comparison below is measuring
        if (expectedView == static_cast<int>(EditorViewType::cmTriggerView)) {
            mpEditor->slot_showAliases();
        } else {
            mpEditor->slot_showTriggers();
        }
        QVERIFY2(static_cast<int>(mpEditor->mCurrentView) != expectedView, "the editor was already showing the view the result should have switched it to");

        search(itemName);
        QTreeWidgetItem* result = mpEditor->treeWidget_searchResults->topLevelItem(0);
        QVERIFY2(result, qPrintable(qsl("%1 was not found").arg(itemName)));

        mpEditor->slot_itemSelectedInSearchResults(result);

        QCOMPARE(static_cast<int>(mpEditor->mCurrentView), expectedView);
    }

    void test_selectingATriggerResultSelectsThatTriggerInItsTree()
    {
        mpEditor->slot_showScripts();
        search(qsl("qaNestedTrigger"));
        QTreeWidgetItem* result = mpEditor->treeWidget_searchResults->topLevelItem(0);
        QVERIFY(result);

        mpEditor->slot_itemSelectedInSearchResults(result);

        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmTriggerView);
        QTreeWidgetItem* selected = mpEditor->treeWidget_triggers->currentItem();
        QVERIFY2(selected, "no trigger became current after its search result was chosen");
        QCOMPARE(selected->text(0), qsl("qaNestedTrigger"));
    }

    void test_anEmptyOrUnknownTermProducesNoResults()
    {
        search(qsl("qaNothingMatchesThis"));
        QCOMPARE(totalResultRows(), 0);

        search(mNeedle);
        QVERIFY(totalResultRows() > 0);

        // An empty combo entry leaves the previous results alone rather than
        // clearing them, which is what makes the guard worth having
        mpEditor->comboBox_searchTerms->clear();
        mpEditor->comboBox_searchTerms->addItem(QString());
        mpEditor->slot_searchMudletItems(0);
        QVERIFY(totalResultRows() > 0);

        mpEditor->slot_searchMudletItems(-1);
        QVERIFY(totalResultRows() > 0);
    }
};

#include "EditorSearchTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorSearchTest)
