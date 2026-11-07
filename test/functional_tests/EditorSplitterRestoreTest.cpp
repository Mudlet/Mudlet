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
 * Guards the right splitter's fit: the form snaps to its item's height on first
 * show and on every item change, shrinking as well as growing; only a view
 * dragged this session keeps its own height, and nothing survives a restart.
 *
 * Run with: ctest -R EditorSplitterRestoreTest -V
 */

#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "ProfileSessionTestHelper.h"
#include "TAlias.h"
#include "TTrigger.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorSplitterRestoreTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorSplitterRestore-Test-Profile");

    // Room for eleven pattern rows over a code pane clear of its floor, so no
    // case ends up measuring the clamp instead of the fit
    static constexpr int scmWindowWidth = 1000;
    static constexpr int scmWindowHeight = 1400;
    // Short enough that eleven rows scroll at both heights, so each measures the code pane's floor
    static constexpr int scmShortWindowHeight = 600;
    static constexpr int scmShortWindowGrowth = 80;
    // Mirrors scmEditorSourcePaneFloor in src/dlgTriggerEditor.cpp, which is
    // file-local to it
    static constexpr int scmSourcePaneFloor = 120;
    static constexpr int scmTallTriggerPatterns = 11;
    static constexpr int scmShortTriggerPatterns = 3;

    // The split from the reported profile, seeded as a returning user's
    // settings; nothing should read it
    static constexpr int scmSavedFormHeight = 630;
    static constexpr int scmSavedCodeHeight = 220;
    static constexpr int scmSavedErrorConsoleHeight = 30;
    QByteArray mSeededState;

    QSplitter* mpSplitter = nullptr;
    QScrollArea* mpPatterns = nullptr;

    TTrigger* mpTallTrigger = nullptr;
    TTrigger* mpShortTrigger = nullptr;
    TTrigger* mpOneRowTrigger = nullptr;
    TAlias* mpAlias = nullptr;

    // Taken in initTestCase(): no later case can put the editor back to never
    // having been shown
    QList<int> mFirstShowSizes;
    int mFirstShowFormWants = 0;
    int mFirstShowRowsWant = 0;
    int mFirstShowRowsShown = 0;
    bool mFirstShowScrolling = false;

    static QStringList retiredSplitterKeys()
    {
        return {qsl("mTriggerEditorSplitterState"),
                qsl("mAliasEditorSplitterState"),
                qsl("mScriptEditorSplitterState"),
                qsl("mActionEditorSplitterState"),
                qsl("mKeyEditorSplitterState"),
                qsl("mTimerEditorSplitterState"),
                qsl("mVarEditorSplitterState")};
    }

    // QSplitter stores the sizes it was asked for, not the ones it drew, so a
    // stand-in splitter produces the byte array a profile would have carried
    static QByteArray splitterStateFor(const QList<int>& sizes)
    {
        QSplitter splitter(Qt::Vertical);
        int total = 0;
        for (const int size : sizes) {
            splitter.addWidget(new QWidget(&splitter));
            total += size;
        }
        splitter.resize(400, total + splitter.handleWidth() * (sizes.size() - 1));
        splitter.setSizes(sizes);
        return splitter.saveState();
    }

    static QString describe(const QList<int>& sizes)
    {
        QStringList parts;
        for (const int size : sizes) {
            parts << QString::number(size);
        }
        return parts.join(qsl(" / "));
    }

    static TTrigger* registeredTrigger(const QString& name, const int patternCount, Host* pHost)
    {
        QStringList patterns;
        QList<int> kinds;
        for (int row = 0; row < patternCount; ++row) {
            patterns << qsl("pattern %1").arg(QString::number(row));
            kinds << REGEX_SUBSTRING;
        }
        auto* pTrigger = new TTrigger(name, patterns, kinds, false, pHost);
        pTrigger->registerTrigger();
        return pTrigger;
    }

    QTreeWidgetItem* triggerRowFor(const int id) const
    {
        QTreeWidgetItem* pBase = mpEditor->mpTriggerBaseItem;
        for (int row = 0; pBase && row < pBase->childCount(); ++row) {
            if (pBase->child(row)->data(0, Qt::UserRole).toInt() == id) {
                return pBase->child(row);
            }
        }
        return nullptr;
    }

    void enterTheTriggerView()
    {
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    void chooseTrigger(TTrigger* pTrigger)
    {
        QTreeWidgetItem* pRow = triggerRowFor(pTrigger->getID());
        QVERIFY2(pRow != nullptr, qPrintable(qsl("\"%1\" is not in the tree, so this case cannot choose it").arg(pTrigger->getName())));
        mpEditor->treeWidget_triggers->setCurrentItem(pRow);
        mpEditor->slot_triggerSelected(pRow);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    void chooseTheAlias()
    {
        mpEditor->slot_showAliases();
        QCoreApplication::processEvents();
        QTreeWidgetItem* pBase = mpEditor->mpAliasBaseItem;
        QVERIFY2(pBase != nullptr && pBase->childCount() > 0, "the aliases tree has nothing in it for this case to choose");
        QTreeWidgetItem* pRow = pBase->child(0);
        mpEditor->treeWidget_aliases->setCurrentItem(pRow);
        mpEditor->slot_aliasSelected(pRow);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    // QSplitter emits splitterMoved() for a drag and not for setSizes(), so the
    // slot is called by hand. The two panes keep their total, as in a drag.
    void dragFormPaneTo(const int formHeight)
    {
        QList<int> sizes = mpSplitter->sizes();
        const int panes = sizes.at(0) + sizes.at(1);
        mpSplitter->setSizes({formHeight, panes - formHeight, sizes.at(2)});
        QCoreApplication::processEvents();
        mpEditor->slot_rightSplitterMoved(0, 0);
        QCoreApplication::processEvents();
    }

    int panesTotal() const
    {
        const QList<int> sizes = mpSplitter->sizes();
        return sizes.at(0) + sizes.at(1);
    }

    int formWants() const { return mpEditor->formPaneHeightForItsContents(panesTotal()); }

    bool patternsScrolling() const { return mpPatterns->verticalScrollBar() && mpPatterns->verticalScrollBar()->isVisible(); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));

        // Seeded before openProfile(), which is what builds the editor
        mSeededState = splitterStateFor({scmSavedFormHeight, scmSavedCodeHeight, scmSavedErrorConsoleHeight});
        QVERIFY(!mSeededState.isEmpty());
        for (const QString& key : retiredSplitterKeys()) {
            MudletApp::getQSettings()->setValue(key, mSeededState);
        }
        MudletApp::getQSettings()->remove(qsl("script_editor_pos"));
        MudletApp::getQSettings()->sync();

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mpEditor = HostDialogs::editor(mpHost);
        QVERIFY2(mpEditor != nullptr, "the profile has no editor to build");
        QVERIFY2(!mpEditor->isVisible(), "the editor was already on screen, so the first-show case has nothing left to prove");

        mpTallTrigger = registeredTrigger(qsl("Eleven patterns"), scmTallTriggerPatterns, mpHost);
        mpShortTrigger = registeredTrigger(qsl("Three patterns"), scmShortTriggerPatterns, mpHost);
        mpOneRowTrigger = registeredTrigger(qsl("One pattern"), 1, mpHost);
        // An alias, for the view that is never dragged
        mpAlias = new TAlias(qsl("An alias"), mpHost);
        mpAlias->setRegexCode(qsl("^greet$"));
        mpAlias->setCommand(qsl("say hello"));
        mpAlias->registerAlias();
        // fillout_form() inserts a fresh base item into every one of the seven
        // trees, so all seven are emptied first or each gets a second one
        for (TTreeWidget* pTree : {mpEditor->treeWidget_triggers,
                                   mpEditor->treeWidget_aliases,
                                   mpEditor->treeWidget_timers,
                                   mpEditor->treeWidget_scripts,
                                   mpEditor->treeWidget_actions,
                                   mpEditor->treeWidget_keys,
                                   mpEditor->treeWidget_variables}) {
            pTree->clear();
        }
        mpEditor->fillout_form();

        // Filled while the window is still hidden, in the order mudlet::slot_showEditorDialog()
        // uses, so a fit run here has no panes to divide up
        QTreeWidgetItem* pTallRow = triggerRowFor(mpTallTrigger->getID());
        QVERIFY2(pTallRow != nullptr, "the eleven-row trigger is not in the tree");
        mpEditor->treeWidget_triggers->setCurrentItem(pTallRow);
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
        QVERIFY2(!mpEditor->isVisible(), "showing the trigger view put the editor on screen, which this case needs the show itself to do");

        // Taller than the offscreen platform's screen; with no window manager,
        // nothing refuses the size
        mpEditor->resize(scmWindowWidth, scmWindowHeight);
        mpEditor->show();
        QTest::qWait(200ms);

        mpSplitter = mpEditor->findChild<QSplitter*>(qsl("splitter_right"));
        QVERIFY2(mpSplitter != nullptr, "The editor should have a splitter_right");
        QCOMPARE(mpSplitter->count(), 3);
        mpPatterns = mpEditor->findChild<QScrollArea*>(qsl("editorPatternScroll"));
        QVERIFY2(mpPatterns != nullptr && mpPatterns->widget() != nullptr, "The trigger form has no pattern list to scroll");

        mFirstShowSizes = mpSplitter->sizes();
        mFirstShowFormWants = formWants();
        mFirstShowRowsWant = mpPatterns->widget()->sizeHint().height();
        mFirstShowRowsShown = mpPatterns->viewport()->height();
        mFirstShowScrolling = patternsScrolling();
    }

    void cleanupTestCase()
    {
        mpSplitter = nullptr;
        mpPatterns = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    // Each case is its own session: a height is remembered only for a view
    // dragged in that session
    void init()
    {
        if (!mpEditor) {
            return;
        }
        mpEditor->mDraggedFormPaneHeights.clear();
    }

    // Regression: the form is filled before the window is shown, and a fit run
    // then divided a splitter with no height, leaving eleven rows scrolling
    void test_theFirstItemShownGetsItsContentsHeight()
    {
        QVERIFY2(!mFirstShowSizes.isEmpty(), "the first show was never measured");
        qInfo().noquote() << qsl("  first item shown is a trigger of %1 pattern rows: the rows want %2px and are shown %3px, the form asks for %4px, the panes are at %5 - the rows %6")
                                     .arg(QString::number(scmTallTriggerPatterns),
                                          QString::number(mFirstShowRowsWant),
                                          QString::number(mFirstShowRowsShown),
                                          QString::number(mFirstShowFormWants),
                                          describe(mFirstShowSizes),
                                          mFirstShowScrolling ? qsl("scroll") : qsl("all fit"));

        QVERIFY2(mFirstShowRowsWant > 0, "the pattern list has no height at all, so this case is measuring nothing");
        // Asked of what the form wanted, not where the panes ended up, so a form
        // given the whole window fails as the fault, not as a too-short window
        const int panes = mFirstShowSizes.at(0) + mFirstShowSizes.at(1);
        QVERIFY2(mFirstShowFormWants < panes - scmSourcePaneFloor,
                 qPrintable(qsl("the form asked for %1px of the %2px the two panes have, so it was clamped rather than fitted - the window is too short for this case")
                                    .arg(QString::number(mFirstShowFormWants), QString::number(panes))));

        QVERIFY2(mFirstShowSizes.at(0) == mFirstShowFormWants,
                 qPrintable(qsl("the form asks for %1px and the first show left it at %2px").arg(QString::number(mFirstShowFormWants), QString::number(mFirstShowSizes.at(0)))));
        QVERIFY2(mFirstShowRowsShown >= mFirstShowRowsWant,
                 qPrintable(qsl("the pattern rows want %1px and are shown %2px, so the last of them is off the bottom of the form the editor opened with")
                                    .arg(QString::number(mFirstShowRowsWant), QString::number(mFirstShowRowsShown))));
        QVERIFY2(!mFirstShowScrolling, "the editor opened with the pattern rows scrolling inside the form pane");
        QVERIFY2(mFirstShowSizes.at(1) >= scmSourcePaneFloor,
                 qPrintable(qsl("the code pane opened at %1, under the %2px floor it keeps").arg(QString::number(mFirstShowSizes.at(1)), QString::number(scmSourcePaneFloor))));
    }

    // Regression: the fit only ever grew a pane, so a tall form stayed tall
    void test_choosingAShorterItemSnapsTheFormBackDown()
    {
        enterTheTriggerView();
        chooseTrigger(mpTallTrigger);
        const QList<int> tall = mpSplitter->sizes();
        chooseTrigger(mpShortTrigger);
        const QList<int> shortened = mpSplitter->sizes();
        const int wanted = formWants();
        qInfo().noquote() << qsl("  eleven rows left the panes at %1; three rows ask for %2px and the panes came out at %3").arg(describe(tall), QString::number(wanted), describe(shortened));

        QVERIFY2(tall.at(0) > wanted + 100,
                 qPrintable(qsl("the eleven-row form was only %1px against the three-row form's %2px - too close for this case to say anything about shrinking")
                                    .arg(QString::number(tall.at(0)), QString::number(wanted))));
        QVERIFY2(shortened.at(0) == wanted,
                 qPrintable(qsl("the form stayed at %1px for an item asking for %2px - %3px of empty pane between its last row and the code editor")
                                    .arg(QString::number(shortened.at(0)), QString::number(wanted), QString::number(shortened.at(0) - wanted))));
        QVERIFY2(shortened.at(1) == tall.at(1) + (tall.at(0) - shortened.at(0)),
                 qPrintable(qsl("the code pane should have grown by the %1px the form gave up and sat at %2px, it is at %3px")
                                    .arg(QString::number(tall.at(0) - shortened.at(0)), QString::number(tall.at(1) + tall.at(0) - shortened.at(0)), QString::number(shortened.at(1)))));
        // The form's height trades with the code pane only; the error console (index 2) stays put
        QCOMPARE(shortened.at(0) + shortened.at(1), tall.at(0) + tall.at(1));
        QCOMPARE(shortened.at(2), tall.at(2));
    }

    void test_aDraggedViewKeepsItsHeightAndOnlyThatView()
    {
        enterTheTriggerView();
        chooseTrigger(mpTallTrigger);
        const int tallestFormNeeds = mpSplitter->sizes().at(0);
        chooseTrigger(mpShortTrigger);
        const int panes = panesTotal();
        // Past what the tallest item fills, yet short of where the snap that
        // reads the height back clamps the code pane to its floor - a deeper
        // drag would measure the clamp instead
        dragFormPaneTo(std::min(tallestFormNeeds + 80, panes - scmSourcePaneFloor - 40));
        const int draggedTo = mpSplitter->sizes().at(0);
        QVERIFY2(mpEditor->mDraggedFormPaneHeights.value(EditorViewType::cmTriggerView) == draggedTo,
                 qPrintable(qsl("a drag on this view's handle left the form at %1px and was not recorded as the view's own height").arg(QString::number(draggedTo))));
        QVERIFY2(draggedTo > tallestFormNeeds,
                 qPrintable(qsl("the drag left the form at %1px of the %2px the panes have, which the tallest of these items fills on its own at %3px - the window is too short for this case")
                                    .arg(QString::number(draggedTo), QString::number(panes), QString::number(tallestFormNeeds))));

        chooseTrigger(mpOneRowTrigger);
        QVERIFY2(mpSplitter->sizes().at(0) == draggedTo,
                 qPrintable(qsl("a one-row trigger pulled the dragged form from %1px to %2px").arg(QString::number(draggedTo), QString::number(mpSplitter->sizes().at(0)))));
        chooseTrigger(mpTallTrigger);
        QVERIFY2(mpSplitter->sizes().at(0) == draggedTo,
                 qPrintable(qsl("an eleven-row trigger pulled the dragged form from %1px to %2px").arg(QString::number(draggedTo), QString::number(mpSplitter->sizes().at(0)))));

        chooseTheAlias();
        const QList<int> aliasSizes = mpSplitter->sizes();
        const int aliasFormWants = formWants();
        qInfo().noquote() << qsl("  the trigger view was dragged to %1px; the alias form asks for %2px and came out at %3")
                                     .arg(QString::number(draggedTo), QString::number(aliasFormWants), describe(aliasSizes));
        QVERIFY2(aliasSizes.at(0) == aliasFormWants,
                 qPrintable(qsl("the alias form asks for %1px and was given %2px - a view that was never dragged took the trigger view's height")
                                    .arg(QString::number(aliasFormWants), QString::number(aliasSizes.at(0)))));
        QVERIFY2(aliasSizes.at(0) != draggedTo, "the alias form happens to want exactly the height the trigger view was dragged to, so this case proves nothing");

        enterTheTriggerView();
        chooseTrigger(mpShortTrigger);
        QVERIFY2(mpSplitter->sizes().at(0) == draggedTo,
                 qPrintable(qsl("the trigger view came back at %1px rather than the %2px it was dragged to").arg(QString::number(mpSplitter->sizes().at(0)), QString::number(draggedTo))));
    }

    // Regression: the code pane kept a third of the panes, so a window with room for all eleven rows over
    // a code pane under a third opened with the last rows and the Add pattern button scrolled out of sight
    void test_aLongTriggerShowsEveryRowWhileTheCodePaneHasRoomToGive()
    {
        const auto restoreTheWindow = qScopeGuard([this]() {
            mpEditor->resize(scmWindowWidth, scmWindowHeight);
            QTest::qWait(50ms);
        });
        enterTheTriggerView();
        chooseTrigger(mpTallTrigger);
        const QList<int> roomy = mpSplitter->sizes();
        const int formNeeds = roomy.at(0);
        QVERIFY2(!patternsScrolling(), "the eleven rows scroll in the tall window, so there is no height of theirs to fit a shorter one to");
        // A quarter of the form's height left for the code pane, well under a third of the panes
        const int codeLeft = std::max(scmSourcePaneFloor + 10, formNeeds / 4);
        mpEditor->resize(scmWindowWidth, scmWindowHeight - (roomy.at(1) - codeLeft));
        QTest::qWait(50ms);
        chooseTrigger(mpShortTrigger);
        chooseTrigger(mpTallTrigger);

        const QList<int> sizes = mpSplitter->sizes();
        const int panes = sizes.at(0) + sizes.at(1);
        QWidget* pAddPattern = mpEditor->mpButton_addPattern;
        const int addPatternBottom = pAddPattern->mapTo(mpPatterns->viewport(), QPoint(0, pAddPattern->height())).y();
        qInfo().noquote() << qsl("  eleven rows need %1px; in the shorter window the panes are %2, and the Add pattern button ends at %3 of the %4px list")
                                     .arg(QString::number(formNeeds), describe(sizes), QString::number(addPatternBottom), QString::number(mpPatterns->viewport()->height()));
        QVERIFY2(formNeeds > panes - panes / 3,
                 qPrintable(qsl("eleven rows need %1px, no more than two thirds of the %2px the panes have, so this case says nothing about the code pane giving up its third")
                                    .arg(QString::number(formNeeds), QString::number(panes))));
        QVERIFY2(sizes.at(0) == formNeeds,
                 qPrintable(qsl("the form needs %1px and was given %2px of the %3px the panes have").arg(QString::number(formNeeds), QString::number(sizes.at(0)), QString::number(panes))));
        QVERIFY2(!patternsScrolling(), "the eleven rows scroll although the code pane had the room to give them");
        QVERIFY2(addPatternBottom <= mpPatterns->viewport()->height(), "the Add pattern button is cut off at the bottom of the pattern list");
    }

    // Regression: QSplitter gives a change in its height to the code pane alone, so a window made taller
    // kept the eleven rows scrolling above a code pane with room to spare
    void test_aNewWindowHeightIsSharedOutAgain()
    {
        const auto restoreTheWindow = qScopeGuard([this]() {
            mpEditor->resize(scmWindowWidth, scmWindowHeight);
            QTest::qWait(50ms);
        });
        enterTheTriggerView();
        mpEditor->resize(scmWindowWidth, scmShortWindowHeight);
        QTest::qWait(50ms);
        chooseTrigger(mpTallTrigger);
        const QList<int> chosen = mpSplitter->sizes();
        QVERIFY2(patternsScrolling(), "the eleven rows fit the short window, so the form was fitted rather than held to its share and this case says nothing");

        for (const int height : {scmShortWindowHeight + scmShortWindowGrowth, scmShortWindowHeight}) {
            mpEditor->resize(scmWindowWidth, height);
            QTest::qWait(50ms);
            const QList<int> sizes = mpSplitter->sizes();
            const int share = formWants();
            qInfo().noquote() << qsl("  chosen at %1, a %2px tall window left the panes at %3 where the form's share is %4px")
                                         .arg(describe(chosen), QString::number(height), describe(sizes), QString::number(share));
            QVERIFY2(sizes.at(0) == share,
                     qPrintable(qsl("in a %1px tall window the form is at %2px of the %3px the panes have, where its share is %4px")
                                        .arg(QString::number(height), QString::number(sizes.at(0)), QString::number(sizes.at(0) + sizes.at(1)), QString::number(share))));
        }
        QVERIFY2(mpSplitter->sizes().at(1) == chosen.at(1), "the window is back at the height the trigger was chosen in, and the code pane is not back where it was");
    }

    // Eleven rows, not three: a one-row trigger's options caption already leaves
    // room for three, so three would not prove the pane grows
    void test_choosingATallerItemGivesItsFormTheRoom()
    {
        enterTheTriggerView();
        chooseTrigger(mpOneRowTrigger);
        const QList<int> forOne = mpSplitter->sizes();
        const int panesForOne = forOne.at(0) + forOne.at(1);
        chooseTrigger(mpTallTrigger);
        const QList<int> sizes = mpSplitter->sizes();

        const int rowsWant = mpPatterns->widget()->sizeHint().height();
        const int rowsShown = mpPatterns->viewport()->height();
        const bool scrolling = patternsScrolling();
        qInfo().noquote() << qsl("  one pattern row left the panes at %1; eleven rows want %2px and are shown %3px, with the panes at %4 - the rows %5")
                                     .arg(describe(forOne), QString::number(rowsWant), QString::number(rowsShown), describe(sizes), scrolling ? qsl("scroll") : qsl("all fit"));

        QVERIFY2(rowsWant > forOne.at(0) - rowsShown, "The eleven-row list is no taller than the pane it is in, so this case says nothing about growing it");
        QVERIFY2(sizes.at(0) > forOne.at(0),
                 qPrintable(qsl("the form pane stayed at %1 for an item whose rows want %2px of the %3px it shows them in")
                                    .arg(QString::number(sizes.at(0)), QString::number(rowsWant), QString::number(rowsShown))));
        QVERIFY2(rowsShown >= rowsWant,
                 qPrintable(qsl("the pattern rows want %1px and are shown %2px, so the last of them is off the bottom of the form").arg(QString::number(rowsWant), QString::number(rowsShown))));
        QVERIFY2(!scrolling, "the pattern rows are still scrolling inside the form pane");
        QVERIFY2(sizes.at(1) >= scmSourcePaneFloor, qPrintable(qsl("the code pane was left at %1, under the floor it keeps").arg(QString::number(sizes.at(1)))));
        // The form's height trades with the code pane only; the error console (index 2) stays put
        QCOMPARE(sizes.at(0) + sizes.at(1), panesForOne);
    }

    // Last of the cases, because it writes settings and builds the editor a
    // restart would build
    void test_nothingOfASessionsDragSurvivesIntoTheNextOne()
    {
        enterTheTriggerView();
        chooseTrigger(mpTallTrigger);
        const int tallestFormNeeds = mpSplitter->sizes().at(0);
        chooseTrigger(mpShortTrigger);
        dragFormPaneTo(tallestFormNeeds + 80);
        const int draggedTo = mpSplitter->sizes().at(0);
        QVERIFY2(mpEditor->mDraggedFormPaneHeights.contains(EditorViewType::cmTriggerView), "the drag was not recorded, so there is nothing here for a restart to lose");

        // Seeded again: any save in between will have taken the seeded keys out
        for (const QString& key : retiredSplitterKeys()) {
            MudletApp::getQSettings()->setValue(key, mSeededState);
        }
        MudletApp::getQSettings()->sync();
        auto* pReopened = new dlgTriggerEditor(mpHost, nullptr);
        pReopened->fillout_form();
        pReopened->resize(scmWindowWidth, scmWindowHeight);
        QTreeWidgetItem* pBase = pReopened->mpTriggerBaseItem;
        QVERIFY2(pBase != nullptr && pBase->childCount() > 0, "the reopened editor has no triggers in its tree");
        for (int row = 0; row < pBase->childCount(); ++row) {
            if (pBase->child(row)->data(0, Qt::UserRole).toInt() == mpShortTrigger->getID()) {
                pReopened->treeWidget_triggers->setCurrentItem(pBase->child(row));
                break;
            }
        }
        pReopened->slot_showTriggers();
        pReopened->show();
        QTest::qWait(200ms);

        auto* pReopenedSplitter = pReopened->findChild<QSplitter*>(qsl("splitter_right"));
        QVERIFY2(pReopenedSplitter != nullptr && pReopenedSplitter->count() == 3, "the reopened editor has no splitter_right to measure");
        const QList<int> reopenedSizes = pReopenedSplitter->sizes();
        const int reopenedWants = pReopened->formPaneHeightForItsContents(reopenedSizes.at(0) + reopenedSizes.at(1));
        qInfo().noquote() << qsl("  the session dragged the trigger form to %1px; the editor built after it asks for %2px and came up at %3")
                                     .arg(QString::number(draggedTo), QString::number(reopenedWants), describe(reopenedSizes));

        QVERIFY2(pReopened->mDraggedFormPaneHeights.isEmpty(), "the new editor started out remembering a height, which only a drag inside it should give it");
        QVERIFY2(reopenedSizes.at(0) == reopenedWants,
                 qPrintable(qsl("the reopened form asks for %1px and came up at %2px").arg(QString::number(reopenedWants), QString::number(reopenedSizes.at(0)))));
        QVERIFY2(reopenedSizes.at(0) < draggedTo - 20,
                 qPrintable(qsl("the reopened form came up at %1px, which is the %2px the last session dragged to").arg(QString::number(reopenedSizes.at(0)), QString::number(draggedTo))));

        delete pReopened;
        QCoreApplication::processEvents();

        mpEditor->writeSettings();
        for (const QString& key : retiredSplitterKeys()) {
            QVERIFY2(!MudletApp::getQSettings()->contains(key), qPrintable(qsl("the editor still keeps %1, which it is now %2").arg(key, MudletApp::getQSettings()->value(key).toString())));
        }
    }
};

#include "EditorSplitterRestoreTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorSplitterRestoreTest)
