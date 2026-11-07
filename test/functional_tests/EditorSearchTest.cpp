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
#include <QHeaderView>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QScopeGuard>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ActionUnit.h"
#include "AliasUnit.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "SearchResultDelegate.h"
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
#include "dlgSourceEditorFindArea.h"
#include "dlgTriggerEditor.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/textrange.h"
#include "edbee/texteditorcontroller.h"
#include "edbee/texteditorwidget.h"
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
    const QString mLoneNeedle = qsl("qaSolitary");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
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

    // Each matched item is a heading row with every match, the first included, as a
    // child; SearchResultDelegate paints the rows from the roles below, not from columns
    static QString typeOf(const QTreeWidgetItem* row) { return row->data(0, uiDesign::SearchResultDelegate::TypeLabelRole).toString(); }
    static QString titleOf(const QTreeWidgetItem* row) { return row->data(0, uiDesign::SearchResultDelegate::TitleRole).toString(); }
    static QString whereOf(const QTreeWidgetItem* row) { return row->data(0, uiDesign::SearchResultDelegate::WhereRole).toString(); }
    static bool isNotice(const QTreeWidgetItem* row) { return row->data(0, uiDesign::SearchResultDelegate::RowKindRole).toInt() == uiDesign::SearchResultDelegate::NoticeRow; }

    QStringList resultsFor(const QString& itemType) const
    {
        QStringList found;
        auto* results = mpEditor->treeWidget_searchResults;
        for (int i = 0, total = results->topLevelItemCount(); i < total; ++i) {
            QTreeWidgetItem* top = results->topLevelItem(i);
            if (isNotice(top) || typeOf(top) != itemType) {
                continue;
            }
            for (int j = 0, children = top->childCount(); j < children; ++j) {
                found << whereOf(top->child(j));
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
            QTreeWidgetItem* top = results->topLevelItem(i);
            if (!isNotice(top) && typeOf(top) == itemType) {
                return top;
            }
        }
        return nullptr;
    }

    // One line per row, children indented: the columns, the jump data bar the id (see verifyResultIds()), " +" if expanded
    QStringList describeResults() const
    {
        const auto describe = [](const QTreeWidgetItem* item, const QString& indent) {
            return qsl("%1%2|%3|%4|%5").arg(indent, item->text(0), item->text(1), item->text(2), item->text(3))
                   + qsl(" name=%1 view=%2 type=%3 line=%4 pos=%5 index=%6%7")
                             .arg(item->data(0, dlgTriggerEditor::NameRole).toString())
                             .arg(item->data(0, dlgTriggerEditor::ItemRole).toInt())
                             .arg(item->data(0, dlgTriggerEditor::TypeRole).toInt())
                             .arg(item->data(0, dlgTriggerEditor::PatternOrLineRole).toInt())
                             .arg(item->data(0, dlgTriggerEditor::PositionRole).toInt())
                             .arg(item->data(0, dlgTriggerEditor::IndexRole).toInt())
                             .arg(item->isExpanded() ? qsl(" +") : QString());
        };
        QStringList rows;
        auto* results = mpEditor->treeWidget_searchResults;
        for (int i = 0, total = results->topLevelItemCount(); i < total; ++i) {
            QTreeWidgetItem* top = results->topLevelItem(i);
            rows << describe(top, QString());
            for (int j = 0, children = top->childCount(); j < children; ++j) {
                rows << describe(top->child(j), qsl("  "));
            }
        }
        return rows;
    }

    void verifyResultIds() const
    {
        auto* results = mpEditor->treeWidget_searchResults;
        QList<QTreeWidgetItem*> rows;
        for (int i = 0, total = results->topLevelItemCount(); i < total; ++i) {
            rows << results->topLevelItem(i);
            for (int j = 0, children = results->topLevelItem(i)->childCount(); j < children; ++j) {
                rows << results->topLevelItem(i)->child(j);
            }
        }
        for (const auto* row : std::as_const(rows)) {
            const QString name = row->data(0, dlgTriggerEditor::NameRole).toString();
            const QVariant id = row->data(0, dlgTriggerEditor::IdRole);
            std::vector<int> ids;
            switch (static_cast<EditorViewType>(row->data(0, dlgTriggerEditor::ItemRole).toInt())) {
            case EditorViewType::cmTriggerView:
                ids = mpHost->getTriggerUnit()->findItems(name);
                break;
            case EditorViewType::cmAliasView:
                ids = mpHost->getAliasUnit()->findItems(name, true, true);
                break;
            case EditorViewType::cmScriptView:
                ids = mpHost->getScriptUnit()->findItems(name);
                break;
            case EditorViewType::cmActionView:
                ids = mpHost->getActionUnit()->findItems(name);
                break;
            case EditorViewType::cmTimerView:
                ids = mpHost->getTimerUnit()->findItems(name);
                break;
            case EditorViewType::cmKeysView:
                ids = mpHost->getKeyUnit()->findItems(name, true, true);
                break;
            case EditorViewType::cmVarsView:
                QCOMPARE(id.toStringList(), QStringList({name}));
                continue;
            default:
                QFAIL(qPrintable(qsl("the row for %1 names no item type").arg(name)));
            }
            QCOMPARE(ids.size(), size_t{1});
            QCOMPARE(id.toInt(), ids.front());
        }
    }

    // A search that finds nothing still leaves one notice row saying so, which is not a result
    int totalResultRows() const
    {
        auto* results = mpEditor->treeWidget_searchResults;
        int total = 0;
        for (int i = 0, tops = results->topLevelItemCount(); i < tops; ++i) {
            QTreeWidgetItem* top = results->topLevelItem(i);
            if (isNotice(top)) {
                continue;
            }
            total += 1 + top->childCount();
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

        auto* pLongScript = new TScript(qsl("qaMultiLineScript"), mpHost);
        pLongScript->setScript(qsl("local a = 1\n-- qaLater qaLaterWord\nlocal b = qaLater\n"));
        mpHost->getScriptUnit()->registerScript(pLongScript);

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

        // Each matches mLoneNeedle in one field only, one that is not searched first
        auto* pUpButton = new TAction(qsl("qaUpOnlyButton"), mpHost);
        pUpButton->setIsPushDownButton(true);
        pUpButton->setCommandButtonUp(qsl("%1 up").arg(mLoneNeedle));
        mpHost->getActionUnit()->registerAction(pUpButton);

        auto* pCssButton = new TAction(qsl("qaCssOnlyButton"), mpHost);
        pCssButton->css = qsl("color: %1;").arg(mLoneNeedle);
        mpHost->getActionUnit()->registerAction(pCssButton);

        auto* pPatternAlias = new TAlias(qsl("qaPatternOnlyAlias"), mpHost);
        pPatternAlias->setRegexCode(qsl("^%1$").arg(mLoneNeedle));
        mpHost->getAliasUnit()->registerAlias(pPatternAlias);

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("qaSearchVariable = \"%1 value\"").arg(mNeedle)));
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("%1Variable = \"%1 too\"").arg(mNeedle)));
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
        QVERIFY2(connectedSpy.wait(1s), "Could not connect with the host.");

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");

        populateProfile();
        // The editor filled its trees as it opened, so items added
        // behind its back are only in them after the same rebuild that Host
        // does when a package is installed
        mpEditor->doCleanReset();
        QVERIFY2(waitForTreeToHold(qsl("qaSearchTrigger")), "the editor never rebuilt its trees around the items this test planted");
        mpEditor->setSearchOptions(enums::EditorSearchOptionNone);
    }

    void cleanupTestCase()
    {
        // ~Host would do this, but only if the host is ever destroyed - deleting
        // the editor here keeps the leak checker satisfied either way
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

    void cleanup()
    {
        if (mpEditor) {
            mpEditor->setSearchOptions(enums::EditorSearchOptionNone);
        }
    }

    void test_findsTheNeedleInEveryItemType()
    {
        search(mNeedle);

        QCOMPARE(resultsFor(qsl("Trigger")), QStringList({qsl("Command"), qsl("Pattern 1"), qsl("Lua 1:4"), qsl("Pattern 1")}));
        QCOMPARE(resultsFor(qsl("Alias")), QStringList({qsl("Command"), qsl("Pattern")}));
        QCOMPARE(resultsFor(qsl("Script")), QStringList({qsl("Event"), qsl("Lua 1:4")}));
        QCOMPARE(resultsFor(qsl("Timer")), QStringList({qsl("Command")}));
        QCOMPARE(resultsFor(qsl("Key")), QStringList({qsl("Command")}));
        QCOMPARE(resultsFor(qsl("Button")), QStringList({qsl("Command (down)"), qsl("Command (up)"), qsl("Style 2:21")}));
    }

    // A match inside a folder is only found by the recursive walk, not by the
    // pass over the unit's root nodes
    void test_findsMatchesNestedInsideAFolder()
    {
        search(qsl("%1 nested").arg(mNeedle));

        auto* result = topLevelResultFor(qsl("Trigger"));
        QVERIFY2(result, "the nested trigger was not found at all");
        QCOMPARE(titleOf(result), qsl("qaNestedTrigger"));
        QCOMPARE(result->childCount(), 1);
        QCOMPARE(whereOf(result->child(0)), qsl("Pattern 1"));
    }

    // Every row carries the item type, its id and which field matched, so that
    // clicking it can jump to the right control
    void test_recordsWhereEachMatchCameFrom()
    {
        search(qsl("qaSearchButton"));

        auto* result = topLevelResultFor(qsl("Button"));
        QVERIFY(result);
        // Clicking the heading has to go where the first match under it goes
        QVERIFY(result->childCount() > 0);
        QCOMPARE(whereOf(result->child(0)), qsl("Name"));
        QCOMPARE(static_cast<EditorViewType>(result->data(0, dlgTriggerEditor::ItemRole).toInt()), EditorViewType::cmActionView);
        QCOMPARE(result->data(0, dlgTriggerEditor::TypeRole).toInt(), static_cast<int>(dlgTriggerEditor::SearchResultIsName));
        QCOMPARE(result->data(0, dlgTriggerEditor::NameRole).toString(), qsl("qaSearchButton"));

        TAction* pAction = mpHost->getActionUnit()->findAction(qsl("qaSearchButton"));
        QVERIFY(pAction);
        QCOMPARE(result->data(0, dlgTriggerEditor::IdRole).toInt(), pAction->getID());
    }

    // Sorted by type, descending, ties in the order the units are walked; only rows with children expanded
    // One heading per item, its matches under it, in the order the search finds them; the heading carries its first
    // match's jump data
    void test_resultsKeepTheOrderTheyWereFoundInAndParentsExpanded()
    {
        mpEditor->setSearchOptions(enums::EditorSearchOptionIncludeVariables);
        search(mNeedle);

        QVERIFY(!mpEditor->treeWidget_searchResults->isSortingEnabled());
        QCOMPARE(describeResults(),
                 QStringList({
                         qsl("qaSearchTrigger||| name=qaSearchTrigger view=1 type=4 line=0 pos=0 index=0 +"),
                         qsl("  Command qaHaystack command||| name=qaSearchTrigger view=1 type=4 line=0 pos=0 index=0"),
                         qsl("  Pattern 1 ^qaHaystack pattern$||| name=qaSearchTrigger view=1 type=3 line=0 pos=1 index=0"),
                         qsl("  Lua 1:4 -- qaHaystack in lua||| name=qaSearchTrigger view=1 type=1 line=0 pos=3 index=0"),
                         qsl("qaNestedTrigger||| name=qaNestedTrigger view=1 type=3 line=0 pos=0 index=0 +"),
                         qsl("  Pattern 1 qaHaystack nested||| name=qaNestedTrigger view=1 type=3 line=0 pos=0 index=0"),
                         qsl("qaSearchAlias||| name=qaSearchAlias view=3 type=4 line=0 pos=0 index=0 +"),
                         qsl("  Command qaHaystack command||| name=qaSearchAlias view=3 type=4 line=0 pos=0 index=0"),
                         qsl("  Pattern ^qaHaystack alias$||| name=qaSearchAlias view=3 type=3 line=0 pos=1 index=0"),
                         qsl("qaSearchScript||| name=qaSearchScript view=4 type=7 line=0 pos=0 index=0 +"),
                         qsl("  Event qaHaystackEvent||| name=qaSearchScript view=4 type=7 line=0 pos=0 index=0"),
                         qsl("  Lua 1:4 -- qaHaystack lives here too||| name=qaSearchScript view=4 type=1 line=0 pos=3 index=0"),
                         qsl("qaSearchButton||| name=qaSearchButton view=5 type=4 line=0 pos=0 index=0 +"),
                         qsl("  Command (down) qaHaystack down||| name=qaSearchButton view=5 type=4 line=0 pos=0 index=0"),
                         qsl("  Command (up) qaHaystack up||| name=qaSearchButton view=5 type=5 line=0 pos=0 index=0"),
                         qsl("  Style 2:21   border: 1px solid qaHaystack;||| name=qaSearchButton view=5 type=6 line=1 pos=20 index=0"),
                         qsl("qaSearchTimer||| name=qaSearchTimer view=2 type=4 line=0 pos=0 index=0 +"),
                         qsl("  Command qaHaystack command||| name=qaSearchTimer view=2 type=4 line=0 pos=0 index=0"),
                         qsl("qaSearchKey||| name=qaSearchKey view=6 type=4 line=0 pos=0 index=0 +"),
                         qsl("  Command qaHaystack command||| name=qaSearchKey view=6 type=4 line=0 pos=0 index=0"),
                         qsl("qaHaystackVariable||| name=qaHaystackVariable view=7 type=2 line=0 pos=0 index=0 +"),
                         qsl("  Name qaHaystackVariable||| name=qaHaystackVariable view=7 type=2 line=0 pos=0 index=0"),
                         qsl("  Value qaHaystack too||| name=qaHaystackVariable view=7 type=8 line=0 pos=0 index=0"),
                         qsl("qaSearchVariable||| name=qaSearchVariable view=7 type=8 line=0 pos=0 index=0 +"),
                         qsl("  Value qaHaystack value||| name=qaSearchVariable view=7 type=8 line=0 pos=0 index=0"),
                 }));
        verifyResultIds();
    }

    // Whichever field of an item matches first starts its row
    void test_aRowCanStartFromALaterField()
    {
        search(mLoneNeedle);

        QCOMPARE(describeResults(),
                 QStringList({
                         qsl("qaPatternOnlyAlias||| name=qaPatternOnlyAlias view=3 type=3 line=0 pos=1 index=0 +"),
                         qsl("  Pattern ^qaSolitary$||| name=qaPatternOnlyAlias view=3 type=3 line=0 pos=1 index=0"),
                         qsl("qaUpOnlyButton||| name=qaUpOnlyButton view=5 type=5 line=0 pos=0 index=0 +"),
                         qsl("  Command (up) qaSolitary up||| name=qaUpOnlyButton view=5 type=5 line=0 pos=0 index=0"),
                         qsl("qaCssOnlyButton||| name=qaCssOnlyButton view=5 type=6 line=0 pos=7 index=0 +"),
                         qsl("  Style 1:8 color: qaSolitary;||| name=qaCssOnlyButton view=5 type=6 line=0 pos=7 index=0"),
                 }));
        verifyResultIds();
    }

    // Added one at a time, each row costs a model signal and each role set on a child already
    // in the tree another, millions of them over a broad search
    void test_resultsJoinTheTreeInOneInsert()
    {
        auto* model = mpEditor->treeWidget_searchResults->model();
        QSignalSpy inserts(model, &QAbstractItemModel::rowsInserted);
        QSignalSpy changes(model, &QAbstractItemModel::dataChanged);

        mpEditor->setSearchOptions(enums::EditorSearchOptionIncludeVariables);
        search(mNeedle);

        QCOMPARE(inserts.count(), 1);
        QCOMPARE(changes.count(), 0);
    }

    // Clicking a column header re-sorts the results by it, and the next search keeps to that
    void test_resultsFollowTheColumnTheyWereSortedBy()
    {
        QSKIP("The results are one column without a header, in the order they were found, so there is no column to sort by");
        auto* results = mpEditor->treeWidget_searchResults;
        const int section = results->header()->sortIndicatorSection();
        const Qt::SortOrder order = results->header()->sortIndicatorOrder();
        auto restore = qScopeGuard([results, section, order]() {
            results->sortByColumn(section, order);
        });
        results->sortByColumn(1, Qt::AscendingOrder);

        mpEditor->setSearchOptions(enums::EditorSearchOptionIncludeVariables);
        search(mNeedle);

        QCOMPARE(results->header()->sortIndicatorSection(), 1);
        QCOMPARE(results->header()->sortIndicatorOrder(), Qt::AscendingOrder);
        QCOMPARE(describeResults(),
                 QStringList({
                         qsl("Variable|qaHaystackVariable|Name| name=qaHaystackVariable view=7 type=2 line=0 pos=0 index=0 +"),
                         qsl("  ||Value|qaHaystack too name=qaHaystackVariable view=7 type=8 line=0 pos=0 index=0"),
                         qsl("Trigger|qaNestedTrigger|Pattern {1}|qaHaystack nested name=qaNestedTrigger view=1 type=3 line=0 pos=0 index=0"),
                         qsl("Alias|qaSearchAlias|Command| name=qaSearchAlias view=3 type=4 line=0 pos=0 index=0 +"),
                         qsl("  ||Pattern|^qaHaystack alias$ name=qaSearchAlias view=3 type=3 line=0 pos=1 index=0"),
                         qsl("Button|qaSearchButton|Command {Down}| name=qaSearchButton view=5 type=4 line=0 pos=0 index=0 +"),
                         qsl("  ||Command {Up}| name=qaSearchButton view=5 type=5 line=0 pos=0 index=0"),
                         qsl("  ||Stylesheet {L: 2 C: 21}|  border: 1px solid qaHaystack; name=qaSearchButton view=5 type=6 line=1 pos=20 index=0"),
                         qsl("Key|qaSearchKey|Command| name=qaSearchKey view=6 type=4 line=0 pos=0 index=0"),
                         qsl("Script|qaSearchScript|Event Handler|qaHaystackEvent name=qaSearchScript view=4 type=7 line=0 pos=0 index=0 +"),
                         qsl("  ||Lua code (1:4)|-- qaHaystack lives here too name=qaSearchScript view=4 type=1 line=0 pos=3 index=0"),
                         qsl("Timer|qaSearchTimer|Command| name=qaSearchTimer view=2 type=4 line=0 pos=0 index=0"),
                         qsl("Trigger|qaSearchTrigger|Command| name=qaSearchTrigger view=1 type=4 line=0 pos=0 index=0 +"),
                         qsl("  ||Pattern {1}|^qaHaystack pattern$ name=qaSearchTrigger view=1 type=3 line=0 pos=1 index=0"),
                         qsl("  ||Lua code (1:4)|-- qaHaystack in lua name=qaSearchTrigger view=1 type=1 line=0 pos=3 index=0"),
                         qsl("Variable|qaSearchVariable|Value|qaHaystack value name=qaSearchVariable view=7 type=8 line=0 pos=0 index=0"),
                 }));
    }

    void test_caseSensitivityIsOptional()
    {
        search(qsl("QAHAYSTACK"));
        const int caseInsensitiveRows = totalResultRows();
        QVERIFY2(caseInsensitiveRows > 0, "a differently-cased needle found nothing while matching case-insensitively");

        mpEditor->setSearchOptions(enums::EditorSearchOptionCaseSensitive);
        search(qsl("QAHAYSTACK"));
        QCOMPARE(totalResultRows(), 0);

        search(mNeedle);
        QCOMPARE(totalResultRows(), caseInsensitiveRows);
    }

    void test_wholeWordSearchIgnoresMatchesInsideALongerWord()
    {
        search(qsl("qaSearchTrig"));
        QVERIFY2(totalResultRows() > 0, "a partial word found nothing while matching on substrings");

        mpEditor->setSearchOptions(enums::EditorSearchOptionWholeWord);
        search(qsl("qaSearchTrig"));
        QCOMPARE(totalResultRows(), 0);

        search(qsl("qaSearchTimer"));
        QVERIFY2(totalResultRows() > 0, "a whole word was not matched in whole-word mode");
    }

    // One search after another, with the needle or the case option changed in
    // between, must each match on their own terms
    void test_wholeWordSearchFollowsTheNeedleAndCaseOption()
    {
        mpEditor->setSearchOptions(enums::EditorSearchOptionWholeWord);
        search(qsl("qalater"));
        QCOMPARE(resultsFor(qsl("Script")), QStringList({qsl("Lua 2:4"), qsl("Lua 3:11")}));

        search(qsl("QALATERWORD"));
        QCOMPARE(resultsFor(qsl("Script")), QStringList({qsl("Lua 2:12")}));

        mpEditor->setSearchOptions(enums::EditorSearchOptionWholeWord | enums::EditorSearchOptionCaseSensitive);
        search(qsl("QALATERWORD"));
        QCOMPARE(resultsFor(qsl("Script")), QStringList());

        search(qsl("qaLater"));
        QCOMPARE(resultsFor(qsl("Script")), QStringList({qsl("Lua 2:4"), qsl("Lua 3:11")}));

        mpEditor->setSearchOptions(enums::EditorSearchOptionWholeWord);
        search(qsl("QALATER"));
        QCOMPARE(resultsFor(qsl("Script")), QStringList({qsl("Lua 2:4"), qsl("Lua 3:11")}));
    }

    // Variables live in the Lua state rather than in a unit, so they are only
    // walked when the option asks for it
    void test_variablesAreOnlySearchedWhenAskedFor()
    {
        search(qsl("qaSearchVariable"));
        QVERIFY2(!topLevelResultFor(qsl("Variable")), "a variable was searched without the option being set");

        mpEditor->setSearchOptions(enums::EditorSearchOptionIncludeVariables);
        search(qsl("qaSearchVariable"));
        auto* result = topLevelResultFor(qsl("Variable"));
        QVERIFY2(result, "the variable was not found with the option set");
        QCOMPARE(titleOf(result), qsl("qaSearchVariable"));
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

    // A script that adds an item marks every tree stale; showing the result's
    // view then rebuilds them, which must not free the item about to be selected
    void test_selectingAResultAfterAScriptAddedAnItemSelectsIt()
    {
        mpEditor->slot_showScripts();
        search(qsl("qaNestedTrigger"));
        QTreeWidgetItem* result = mpEditor->treeWidget_searchResults->topLevelItem(0);
        QVERIFY(result);

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("permAlias('qaStaleAlias', '', '^qaStale$', '')")));
        QVERIFY2(mpEditor->mNeedUpdateData, "the script did not mark the editor's trees as stale");

        mpEditor->slot_itemSelectedInSearchResults(result);

        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmTriggerView);
        QTreeWidgetItem* selected = mpEditor->treeWidget_triggers->currentItem();
        QVERIFY2(selected, "no trigger became current after its search result was chosen");
        QCOMPARE(selected->text(0), qsl("qaNestedTrigger"));
        QVERIFY(!mpEditor->treeWidget_aliases->findItems(qsl("qaStaleAlias"), Qt::MatchExactly | Qt::MatchRecursive).isEmpty());
    }

    void test_selectingAVariableResultAfterAScriptAddedAnItemSelectsIt()
    {
        mpEditor->setSearchOptions(enums::EditorSearchOptionIncludeVariables);
        mpEditor->slot_showScripts();
        search(qsl("qaSearchVariable"));
        QTreeWidgetItem* result = topLevelResultFor(qsl("Variable"));
        QVERIFY(result);

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("permAlias('qaStaleVarAlias', '', '^qaStaleVar$', '')")));
        QVERIFY2(mpEditor->mNeedUpdateData, "the script did not mark the editor's trees as stale");

        mpEditor->slot_itemSelectedInSearchResults(result);

        QCOMPARE(mpEditor->mCurrentView, EditorViewType::cmVarsView);
        QTreeWidgetItem* selected = mpEditor->treeWidget_variables->currentItem();
        QVERIFY2(selected, "no variable became current after its search result was chosen");
        QCOMPARE(selected->text(0), qsl("qaSearchVariable"));
    }

    // The find box inside the script pane marks every match as the term is
    // typed, so it waits until enough has been typed to be worth a pass over
    // the document - one or two characters match most of a script (#3847).
    void test_theScriptFindBoxIgnoresTermsOfTwoCharactersOrFewer()
    {
        // whatever item is selected owns this pane, and the next save writes
        // the pane back into it, so put the script back before leaving
        const QString script = mpEditor->mpSourceEditorEdbeeDocument->text();
        auto restore = qScopeGuard([this, script]() {
            mpEditor->mpSourceEditorEdbeeDocument->setText(script);
            mpEditor->mpSourceEditorFindArea->lineEdit_findText->clear();
            mpEditor->mpSourceEditorEdbee->controller()->borderedTextRanges()->clear();
        });

        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("aaa bbb aaa\n"));
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), qsl("aaa bbb aaa\n"));
        edbee::TextRangeSet* marked = mpEditor->mpSourceEditorEdbee->controller()->borderedTextRanges();
        marked->clear();

        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("aa"));
        QCOMPARE(marked->rangeCount(), size_t{0});

        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("aaa"));
        QCOMPARE(marked->rangeCount(), size_t{2});
    }

    void test_shorteningTheScriptFindTermDropsItsMarks()
    {
        const QString script = mpEditor->mpSourceEditorEdbeeDocument->text();
        auto restore = qScopeGuard([this, script]() {
            mpEditor->mpSourceEditorEdbeeDocument->setText(script);
            mpEditor->mpSourceEditorFindArea->lineEdit_findText->clear();
        });

        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("aaa bbb aaa\n"));
        edbee::TextRangeSet* marked = mpEditor->mpSourceEditorEdbee->controller()->borderedTextRanges();
        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("aaa"));
        QCOMPARE(marked->rangeCount(), size_t{2});

        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("aa"));
        QCOMPARE(marked->rangeCount(), size_t{0});
    }

    void test_scriptReplace_data()
    {
        QTest::addColumn<int>("anchor");
        QTest::addColumn<int>("caret");
        QTest::addColumn<QString>("expected");

        // "alpha beta gamma beta": the first "beta" spans 6..10, "alpha" 0..5
        QTest::newRow("selected left to right") << 6 << 10 << "alpha BETA gamma beta\n";
        QTest::newRow("selected right to left") << 10 << 6 << "alpha BETA gamma beta\n";
        QTest::newRow("selection differs in case") << 17 << 21 << "alpha beta gamma BETA\n";
        QTest::newRow("selection is not the term") << 0 << 5 << "alpha beta gamma beta\n";
        QTest::newRow("nothing selected") << 22 << 22 << "alpha beta gamma beta\n";
    }

    void test_scriptReplace()
    {
        QFETCH(int, anchor);
        QFETCH(int, caret);
        QFETCH(QString, expected);

        const QString script = mpEditor->mpSourceEditorEdbeeDocument->text();
        auto restore = qScopeGuard([this, script]() {
            mpEditor->mpSourceEditorEdbeeDocument->setText(script);
            mpEditor->mpSourceEditorFindArea->lineEdit_findText->clear();
            mpEditor->mpSourceEditorFindArea->lineEdit_replaceText->clear();
        });

        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("alpha beta gamma beta\n"));
        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("Beta"));
        mpEditor->mpSourceEditorFindArea->lineEdit_replaceText->setText(qsl("BETA"));
        edbee::TextRangeSet* selection = mpEditor->mpSourceEditorEdbee->controller()->textSelection();
        selection->clear();
        selection->addRange(anchor, caret);

        mpEditor->slot_sourceReplace();

        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), expected);
    }

    // The find matches case-insensitively with full Unicode case folding, so
    // a match it selects must count as one when Replace is pressed
    void test_scriptReplaceReplacesAMatchThatOnlyFoldsToTheTerm()
    {
        const QString script = mpEditor->mpSourceEditorEdbeeDocument->text();
        auto restore = qScopeGuard([this, script]() {
            mpEditor->mpSourceEditorEdbeeDocument->setText(script);
            mpEditor->mpSourceEditorFindArea->lineEdit_findText->clear();
            mpEditor->mpSourceEditorFindArea->lineEdit_replaceText->clear();
        });

        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("-- Strasse\n"));
        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("straße"));
        mpEditor->mpSourceEditorFindArea->lineEdit_replaceText->setText(qsl("road"));
        edbee::TextRangeSet* selection = mpEditor->mpSourceEditorEdbee->controller()->textSelection();
        selection->clear();
        selection->addRange(0, 0);

        mpEditor->slot_sourceFindNext();
        QCOMPARE(selection->range(0).min(), size_t{3});
        QCOMPARE(selection->range(0).max(), size_t{10});

        mpEditor->slot_sourceReplace();
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), qsl("-- road\n"));
    }

    // With nothing to put in its place, a match is deleted and the caret is
    // left with no selection - the next press has to carry on forwards
    void test_scriptReplaceWithNothingDeletesMatchesInOrder()
    {
        const QString script = mpEditor->mpSourceEditorEdbeeDocument->text();
        auto restore = qScopeGuard([this, script]() {
            mpEditor->mpSourceEditorEdbeeDocument->setText(script);
            mpEditor->mpSourceEditorFindArea->lineEdit_findText->clear();
        });

        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("x foo y foo z foo\n"));
        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("foo"));
        mpEditor->mpSourceEditorFindArea->lineEdit_replaceText->clear();
        edbee::TextRangeSet* selection = mpEditor->mpSourceEditorEdbee->controller()->textSelection();
        selection->clear();
        selection->addRange(0, 0);

        mpEditor->slot_sourceReplace();
        QCOMPARE(selection->range(0).min(), size_t{2});
        mpEditor->slot_sourceReplace();
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), qsl("x  y foo z foo\n"));
        mpEditor->slot_sourceReplace();
        mpEditor->slot_sourceReplace();
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), qsl("x  y  z foo\n"));
    }

    // Replace only steps over what is selected, so a selection that is not the
    // term is passed by to the next match, which the following Replace changes
    void test_scriptReplaceMovesPastANonMatchThenReplacesTheMatch()
    {
        const QString script = mpEditor->mpSourceEditorEdbeeDocument->text();
        auto restore = qScopeGuard([this, script]() {
            mpEditor->mpSourceEditorEdbeeDocument->setText(script);
            mpEditor->mpSourceEditorFindArea->lineEdit_findText->clear();
            mpEditor->mpSourceEditorFindArea->lineEdit_replaceText->clear();
        });

        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("alpha beta gamma beta\n"));
        mpEditor->mpSourceEditorFindArea->lineEdit_findText->setText(qsl("beta"));
        mpEditor->mpSourceEditorFindArea->lineEdit_replaceText->setText(qsl("delta"));
        edbee::TextRangeSet* selection = mpEditor->mpSourceEditorEdbee->controller()->textSelection();
        selection->clear();
        selection->addRange(5, 0);

        mpEditor->slot_sourceReplace();
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), qsl("alpha beta gamma beta\n"));
        QCOMPARE(selection->range(0).min(), size_t{6});
        QCOMPARE(selection->range(0).max(), size_t{10});

        mpEditor->slot_sourceReplace();
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), qsl("alpha delta gamma beta\n"));

        // the replacement is still selected, so the next press moves on to the second match
        mpEditor->slot_sourceReplace();
        mpEditor->slot_sourceReplace();
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), qsl("alpha delta gamma delta\n"));
    }

    void test_anEmptyOrUnknownTermProducesNoResults()
    {
        search(qsl("qaNothingMatchesThis"));
        QCOMPARE(totalResultRows(), 0);

        search(mNeedle);
        QVERIFY(totalResultRows() > 0);

        // Emptying the field puts the panel back to the profile's own items, so the results go with it
        mpEditor->comboBox_searchTerms->lineEdit()->clear();
        QCOMPARE(totalResultRows(), 0);

        // Unlike clearing the field, the slot itself ignores an empty or missing entry
        search(mNeedle);
        const int found = totalResultRows();
        QVERIFY(found > 0);

        auto* box = mpEditor->comboBox_searchTerms;
        box->addItem(QString());
        mpEditor->slot_searchMudletItems(box->count() - 1);
        QCOMPARE(totalResultRows(), found);

        mpEditor->slot_searchMudletItems(-1);
        QCOMPARE(totalResultRows(), found);
    }
};

#include "EditorSearchTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorSearchTest)
