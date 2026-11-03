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
 * How much of a trigger's pattern list the editor shows, and which row it
 * opens on.
 *
 * With the advanced options collapsed the pattern list used to be given the
 * height of one and a half rows, so a three pattern trigger opened with its
 * second and third patterns behind a scrollbar - on the very screens the
 * collapse was there to make room on. On top of that the list scrolled to the
 * row after the last pattern on selection, which put pattern 1 out of sight.
 *
 * Run with: ctest -R TriggerPatternListLayoutTest -V
 */

#include <QLineEdit>
#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTreeWidgetItemIterator>
#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TriggerPatternListLayoutTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("TriggerPatternListLayout-Test");
    const QString mLocalhost = qsl("localhost");
    const QString mThreePatternTrigger = qsl("qaThreePatterns");
    const QString mNinePatternTrigger = qsl("qaNinePatterns");
    const QString mNinthPattern = qsl("qaNinthPattern");
    const QString mColourPatternTrigger = qsl("qaColourNinth");
    const QString mSpacerPatternTrigger = qsl("qaSpacerNinth");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void populateProfile()
    {
        auto* pThree = new TTrigger(nullptr, mpHost);
        pThree->setName(mThreePatternTrigger);
        pThree->setRegexCodeList({qsl("alpha"), qsl("beta"), qsl("gamma")}, {REGEX_PERL, REGEX_PERL, REGEX_PERL});
        pThree->setScript(qsl("echo(\"hi\")\n"));
        QVERIFY(pThree->registerTrigger());

        QStringList patterns;
        QList<int> kinds;
        for (int i = 1; i <= 9; ++i) {
            patterns << (i == 9 ? mNinthPattern : qsl("p%1").arg(i));
            kinds << REGEX_PERL;
        }
        auto* pNine = new TTrigger(nullptr, mpHost);
        pNine->setName(mNinePatternTrigger);
        pNine->setRegexCodeList(patterns, kinds);
        pNine->setScript(qsl("echo(\"hi\")\n"));
        QVERIFY(pNine->registerTrigger());

        patterns.clear();
        kinds.clear();
        for (int i = 1; i <= 8; ++i) {
            patterns << qsl("c%1").arg(i);
            kinds << REGEX_PERL;
        }
        patterns << TTrigger::createColorPatternText(1, TTrigger::scmIgnored);
        kinds << REGEX_COLOR_PATTERN;
        auto* pColour = new TTrigger(nullptr, mpHost);
        pColour->setName(mColourPatternTrigger);
        pColour->setRegexCodeList(patterns, kinds);
        pColour->setScript(qsl("echo(\"hi\")\n"));
        QVERIFY(pColour->registerTrigger());

        patterns.clear();
        kinds.clear();
        for (int i = 1; i <= 8; ++i) {
            patterns << qsl("s%1").arg(QChar(u'a' + i - 1));
            kinds << REGEX_PERL;
        }
        patterns << qsl("3");
        kinds << REGEX_LINE_SPACER;
        auto* pSpacer = new TTrigger(nullptr, mpHost);
        pSpacer->setName(mSpacerPatternTrigger);
        pSpacer->setRegexCodeList(patterns, kinds);
        pSpacer->setScript(qsl("echo(\"hi\")\n"));
        QVERIFY(pSpacer->registerTrigger());
    }

    QTreeWidgetItem* triggerItem(const QString& name) const
    {
        const auto items = mpEditor->treeWidget_triggers->findItems(name, Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);
        return items.isEmpty() ? nullptr : items.first();
    }

    // A click on the tree is what a user does; the tree's own signal is what
    // carries that into the editor, so calling the slot is the same journey
    // without needing the item to be under a synthetic cursor
    bool selectTrigger(const QString& name)
    {
        auto* pItem = triggerItem(name);
        if (!pItem) {
            return false;
        }
        mpEditor->treeWidget_triggers->setCurrentItem(pItem);
        mpEditor->slot_triggerSelected(pItem);
        QCoreApplication::processEvents();
        return true;
    }

    // Rows that a user can read and type into in full - a row clipped by the
    // top or bottom of the viewport is the "half row" the bug was about, so it
    // does not count. Only the vertical fit is asked about: a row wider than
    // the viewport is reached by scrolling sideways and is not this bug, and
    // testing both axes at once would report no visible rows at all whenever
    // the horizontal scrollbar is up.
    int fullyVisiblePatternRows() const
    {
        int count = 0;
        for (int i = 0; i < mpEditor->mVisiblePatternCount && i < mpEditor->mTriggerPatternEdit.size(); ++i) {
            auto* pRow = mpEditor->mTriggerPatternEdit.at(i);
            if (!pRow || !pRow->isVisible()) {
                continue;
            }
            if (rowIsFullyVisible(pRow)) {
                ++count;
            }
        }
        return count;
    }

    bool rowIsFullyVisible(const dlgTriggerPatternEdit* pRow) const
    {
        auto* viewport = mpEditor->mpScrollArea->viewport();
        const int top = pRow->mapTo(viewport, QPoint(0, 0)).y();
        return top >= 0 && top + pRow->height() <= viewport->height();
    }

    // By trigger and row, so a match in some other item cannot stand in for it
    QTreeWidgetItem* patternSearchResult(const QString& term, const QString& triggerName, const int row)
    {
        mpEditor->comboBox_searchTerms->clear();
        mpEditor->comboBox_searchTerms->addItem(term);
        mpEditor->slot_searchMudletItems(0);
        for (QTreeWidgetItemIterator it(mpEditor->treeWidget_searchResults); *it; ++it) {
            QTreeWidgetItem* pResult = *it;
            if (pResult->data(0, dlgTriggerEditor::TypeRole).toInt() == dlgTriggerEditor::SearchResultIsPattern && pResult->data(0, dlgTriggerEditor::NameRole).toString() == triggerName
                && pResult->data(0, dlgTriggerEditor::PatternOrLineRole).toInt() == row) {
                return pResult;
            }
        }
        return nullptr;
    }

    void clearSearch()
    {
        mpEditor->treeWidget_searchResults->clear();
        mpEditor->comboBox_searchTerms->clear();
    }

    // A 0ms timer queued now fires after any the editor queued before it
    bool runQueuedTimers()
    {
        bool ran = false;
        QTimer::singleShot(0ms, mpEditor, [&ran]() {
            ran = true;
        });
        return QTest::qWaitFor(
                [&ran]() {
                    return ran;
                },
                1s);
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
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");

        populateProfile();
        // The editor filled its trees when the profile loaded, so items added
        // behind its back are only in them after the same rebuild that Host
        // does when a package is installed
        mpEditor->doCleanReset();
        QVERIFY2(QTest::qWaitFor([this]() {
                     return triggerItem(mThreePatternTrigger) != nullptr && triggerItem(mNinePatternTrigger) != nullptr && triggerItem(mColourPatternTrigger) != nullptr
                            && triggerItem(mSpacerPatternTrigger) != nullptr;
                 }),
                 "the editor never rebuilt its trees around the triggers this test planted");

        // The size the issue was reported at. It is the small-screen end of
        // what Mudlet is used on, which is exactly where the advanced options
        // get collapsed to buy the pattern list room.
        mpEditor->resize(1100, 700);
        QVERIFY(QTest::qWaitForWindowExposed(mpEditor));
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()
    {
        // ~Host would do this, but only if the host is ever destroyed - deleting
        // the editor here keeps the leak checker satisfied either way
        if (mpHost && HostDialogs::of(mpHost).mpEditorDialog) {
            HostDialogs::of(mpHost).mpEditorDialog->deleteLater();
            HostDialogs::of(mpHost).mpEditorDialog = nullptr;
        }
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

    void init()
    {
        // Every test here is about the collapsed state - expanding the advanced
        // options gives the main area enough height that the list is fine. The
        // size is reset too, so a case that resizes cannot hand the next one a
        // window it did not ask for, even when it fails part way through.
        mpEditor->slot_showAllTriggerControls(false);
        mpEditor->resize(1100, 700);
        QCoreApplication::processEvents();
    }

    // A search result for a pattern has to bring that pattern into view, with
    // the caret where the match is. Below the fold, then above it, so scrolling
    // to one end of the list cannot pass for going to the row.
    void test_aPatternSearchResultScrollsToThatPattern()
    {
        const auto cleanup = qScopeGuard([this]() {
            clearSearch();
            selectTrigger(mThreePatternTrigger);
        });
        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");

        struct Jump
        {
            QString term;
            int row;
            int caret;
        };
        // Matched past each pattern's start, where the caret would not be anyway
        const QList<Jump> jumps{{mNinthPattern.mid(2), 8, 2}, {qsl("2"), 1, 1}};
        for (const auto& jump : jumps) {
            QTreeWidgetItem* pResult = patternSearchResult(jump.term, mNinePatternTrigger, jump.row);
            QVERIFY2(pResult, qPrintable(qsl("searching for \"%1\" found no result for pattern %2 of the nine pattern trigger").arg(jump.term, QString::number(jump.row + 1))));

            mpEditor->slot_itemSelectedInSearchResults(pResult);
            auto* pCurrent = mpEditor->treeWidget_triggers->currentItem();
            QVERIFY2(pCurrent, "choosing the search result selected no trigger");
            QCOMPARE(pCurrent->text(0), mNinePatternTrigger);
            auto* pRow = mpEditor->mTriggerPatternEdit.at(jump.row);
            // Until the layout has run the row still reports the place it had
            // before it was shown, which can read as in view
            QCoreApplication::processEvents();
            QVERIFY2(QTest::qWaitFor(
                             [this, pRow]() {
                                 return rowIsFullyVisible(pRow);
                             },
                             1s),
                     qPrintable(qsl("pattern %1 should be in view after its search result was chosen, but the list is scrolled to %2 of %3")
                                        .arg(QString::number(jump.row + 1),
                                             QString::number(mpEditor->mpScrollArea->verticalScrollBar()->value()),
                                             QString::number(mpEditor->mpScrollArea->verticalScrollBar()->maximum()))));
            QCOMPARE(QApplication::focusWidget(), pRow->singleLineTextEdit_pattern);
            QCOMPARE(pRow->singleLineTextEdit_pattern->textCursor().position(), jump.caret);
        }
    }

    // A colour pattern has no text to put the caret in, so its jump focuses
    // the first of its buttons instead
    void test_aColourPatternSearchResultFocusesThatPattern()
    {
        const auto cleanup = qScopeGuard([this]() {
            clearSearch();
            selectTrigger(mThreePatternTrigger);
        });
        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");
        QTreeWidgetItem* pResult = patternSearchResult(qsl("ANSI_COLORS"), mColourPatternTrigger, 8);
        QVERIFY2(pResult, "searching for the colour pattern found no result for it");

        mpEditor->slot_itemSelectedInSearchResults(pResult);
        auto* pRow = mpEditor->mTriggerPatternEdit.at(8);
        QVERIFY(runQueuedTimers());
        QVERIFY2(rowIsFullyVisible(pRow), "the colour pattern should be in view after its search result was chosen");
        QCOMPARE(QApplication::focusWidget(), pRow->pushButton_fgColor);
    }

    // A line spacer's count is selected as it gains focus, so a digit typed
    // straight after the jump replaces it rather than joining it
    void test_aLineSpacerPatternSearchResultSelectsItsCount()
    {
        const auto cleanup = qScopeGuard([this]() {
            clearSearch();
            selectTrigger(mThreePatternTrigger);
        });
        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");
        QTreeWidgetItem* pResult = patternSearchResult(qsl("3"), mSpacerPatternTrigger, 8);
        QVERIFY2(pResult, "searching for the line spacer's count found no result for it");

        mpEditor->slot_itemSelectedInSearchResults(pResult);
        auto* pSpinBox = mpEditor->mTriggerPatternEdit.at(8)->spinBox_lineSpacer;
        QVERIFY(runQueuedTimers());
        QVERIFY2(rowIsFullyVisible(mpEditor->mTriggerPatternEdit.at(8)), "the line spacer pattern should be in view after its search result was chosen");
        QCOMPARE(QApplication::focusWidget(), pSpinBox);
        auto* pSpinBoxEdit = pSpinBox->findChild<QLineEdit*>();
        QVERIFY(pSpinBoxEdit);
        QCOMPARE(pSpinBoxEdit->selectedText(), qsl("3"));
    }

    // The jump to a pattern runs a moment after the result is chosen, and by
    // then another trigger can be showing. Its rows are not the result's, so
    // the jump must leave them alone: "gamma" matches "ma" past the end of
    // "p3", the nine pattern trigger's third pattern.
    void test_aPatternSearchResultLeavesATriggerChosenBeforeItsJumpAlone()
    {
        const auto cleanup = qScopeGuard([this]() {
            clearSearch();
            selectTrigger(mThreePatternTrigger);
        });
        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");
        QTreeWidgetItem* pResult = patternSearchResult(qsl("ma"), mThreePatternTrigger, 2);
        QVERIFY2(pResult, "searching for \"ma\" found no result for the three pattern trigger's \"gamma\"");
        mpEditor->treeWidget_searchResults->setFocus();
        QTest::failOnWarning(QRegularExpression(qsl("out of range")));

        mpEditor->slot_itemSelectedInSearchResults(pResult);
        auto* pNine = triggerItem(mNinePatternTrigger);
        mpEditor->treeWidget_triggers->setCurrentItem(pNine);
        mpEditor->slot_triggerSelected(pNine);
        QWidget* pFocusBeforeJump = QApplication::focusWidget();
        const int scrollBeforeJump = mpEditor->mpScrollArea->verticalScrollBar()->value();
        QVERIFY(runQueuedTimers());
        QVERIFY2(QApplication::focusWidget() == pFocusBeforeJump, "the jump for a pattern of the three pattern trigger moved focus within the nine pattern trigger");
        QCOMPARE(mpEditor->mpScrollArea->verticalScrollBar()->value(), scrollBeforeJump);
    }

    // A three pattern trigger draws four rows: its patterns and the blank one
    // waiting for a fourth. All four have to be readable without scrolling.
    void test_collapsedAdvancedOptionsShowsAWholeSmallTrigger()
    {
        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");
        QVERIFY2(!mpEditor->mpTriggersMainArea->toolButton_toggleExtraControls->isChecked(), "this test is about the collapsed state and the advanced options are showing");

        QCOMPARE(mpEditor->mVisiblePatternCount, 4);
        const int shown = fullyVisiblePatternRows();
        QVERIFY2(shown >= 4,
                 qPrintable(qsl("all 4 rows of a three pattern trigger should be readable at 1100x700 with the advanced options collapsed, but only %1 of them are - the "
                                "pattern list viewport is %2 pixels tall and a row is %3")
                                    .arg(QString::number(shown), QString::number(mpEditor->mpScrollArea->viewport()->height()), QString::number(mpEditor->mTriggerPatternEdit.at(0)->height()))));
        QCOMPARE(mpEditor->mpScrollArea->verticalScrollBar()->maximum(), 0);
    }

    // However many patterns a trigger has, the list is never squeezed down to
    // the row and a half that started this
    void test_aLongPatternListStillShowsSeveralRows()
    {
        QVERIFY2(selectTrigger(mNinePatternTrigger), "could not select the nine pattern trigger");

        const int shown = fullyVisiblePatternRows();
        QVERIFY2(shown >= dlgTriggerEditor::csmMinimumVisiblePatternRows,
                 qPrintable(qsl("the pattern list should always have room for %1 rows, but shows %2").arg(QString::number(dlgTriggerEditor::csmMinimumVisiblePatternRows), QString::number(shown))));
    }

    // Selecting a trigger is the moment a user goes looking for pattern 1, so
    // that is the row the list opens on - whatever it was scrolled to before
    void test_theListOpensOnPatternOne()
    {
        QVERIFY2(selectTrigger(mNinePatternTrigger), "could not select the nine pattern trigger");
        auto* pScrollBar = mpEditor->mpScrollArea->verticalScrollBar();
        QVERIFY2(pScrollBar->maximum() > 0, "the nine pattern trigger should overflow the list, otherwise this test cannot tell a reset scroll position from no scrolling at all");

        // Leave it at the bottom - where scrolling to the row after the last
        // pattern used to leave it, and where a user may leave it by hand
        pScrollBar->setValue(pScrollBar->maximum());
        QCoreApplication::processEvents();
        QVERIFY(pScrollBar->value() > 0);

        QVERIFY2(selectTrigger(mThreePatternTrigger), "could not select the three pattern trigger");
        QVERIFY2(selectTrigger(mNinePatternTrigger), "could not re-select the nine pattern trigger");

        QCOMPARE(pScrollBar->value(), 0);
        QVERIFY2(rowIsFullyVisible(mpEditor->mTriggerPatternEdit.at(0)), "pattern 1 should be fully in view when a trigger is opened");
    }

    // A colour trigger's row carries two buttons captioned with the colours
    // they set, so it is far wider than a narrow editor and the list puts up a
    // horizontal scrollbar. That scrollbar is taken out of the viewport, so the
    // five rows have to be budgeted for with it there. The buttons also stand
    // taller than the pattern field they replace on some platforms, so the row
    // height the budget is built from has to be the tallest a row can be laid
    // out at rather than the one the pattern types that show a text field get.
    void test_aHorizontalScrollbarDoesNotEatARow()
    {
        QVERIFY2(selectTrigger(mNinePatternTrigger), "could not select the nine pattern trigger");
        for (int i = 0; i < mpEditor->mVisiblePatternCount; ++i) {
            mpEditor->mTriggerPatternEdit.at(i)->comboBox_patternType->setCurrentIndex(REGEX_COLOR_PATTERN);
        }
        auto* pFirstRow = mpEditor->mTriggerPatternEdit.at(0);
        QCoreApplication::processEvents();

        // Narrow enough that the colour trigger row no longer fits across, but
        // still wider than the editor will go - this is a reachable window size
        mpEditor->resize(800, 700);
        QCoreApplication::processEvents();
        auto* pScrollArea = mpEditor->mpScrollArea;
        QVERIFY2(pScrollArea->horizontalScrollBar()->isVisible(),
                 qPrintable(qsl("this test needs the horizontal scrollbar up to mean anything - the row wants %1 pixels across and the viewport has %2")
                                    .arg(QString::number(pFirstRow->minimumSizeHint().width()), QString::number(pScrollArea->viewport()->width()))));

        const int shown = fullyVisiblePatternRows();
        QVERIFY2(shown >= dlgTriggerEditor::csmMinimumVisiblePatternRows,
                 qPrintable(qsl("%1 rows should still be readable with the horizontal scrollbar up, but %2 are - the viewport is %3 pixels tall and a row is %4")
                                    .arg(QString::number(dlgTriggerEditor::csmMinimumVisiblePatternRows),
                                         QString::number(shown),
                                         QString::number(pScrollArea->viewport()->height()),
                                         QString::number(pFirstRow->height()))));
    }
};

#include "TriggerPatternListLayoutTest.moc"
MUDLET_GROUPED_TEST_MAIN(TriggerPatternListLayoutTest)
