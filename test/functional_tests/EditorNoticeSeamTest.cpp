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
 * Guards the edit column's notice and pattern list against the seam over the code pane:
 * a notice moves the form down without resizing it, pattern rows resize the pane, and a
 * dragged seam height is kept net of any notice.
 *
 * Run with: ctest -R EditorNoticeSeamTest -V
 */

#include <QColor>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitterHandle>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>

#include "EditorPlaceholderButton.h"
#include "GripSplitter.h"
#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "SingleLineTextEdit.h"
#include "dlgAliasMainArea.h"
#include "dlgSourceEditorArea.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "dlgTriggersMainArea.h"
#include "dlgVarsMainArea.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorNoticeSeamTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    // A fresh profile ships items of its own, so neither is the first row of its tree
    QTreeWidgetItem* mpTriggerRow = nullptr;
    QTreeWidgetItem* mpAliasRow = nullptr;
    const QString mProfileName = qsl("EditorNoticeSeam-Test-Profile");

    // In the small steps a real pointer arrives in
    static constexpr int scmDragTravel = 120;
    static constexpr int scmDragStep = 20;

    // A seam placed by hand lands a pixel off what was asked for
    static constexpr int scmSeamTolerance = 1;

    // Past the 1px hairline, inside the 3px band the legacy sheet drew
    static constexpr int scmInsideTheBand = 2;
    // Inside the gap the column holds the form off the notice by
    static constexpr int scmBelowTheNotice = 6;
    // uiDesign's accentSoft alpha, which a grab shows composited over the page
    static constexpr qreal scmAccentWashAlpha = 0.14;
    // Summed over three channels: antialiased corners blur an ink, and the inks told apart are hundreds apart
    static constexpr int scmInkSlack = 40;

    static constexpr int scmEditorWidth = 1200;
    static constexpr int scmEditorHeight = 800;

    // Mapped to the editor, since each widget is nested a different number of layouts deep
    int topEdgeOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).y(); }

    int bottomEdgeOf(const QWidget* pWidget) const { return topEdgeOf(pWidget) + pWidget->height() - 1; }

    int columnSpacing() const
    {
        QLayout* pLayout = mpEditor->mpNonCodeWidgets->layout();
        return pLayout ? pLayout->spacing() : 0;
    }

    // Counted into the height a column held to its contents is given
    int columnBottomMargin() const
    {
        QLayout* pLayout = mpEditor->mpNonCodeWidgets->layout();
        return pLayout ? pLayout->contentsMargins().bottom() : 0;
    }

    // The package warning word for word, so the notice wraps as it does for a reader
    void raiseTheNotice()
    {
        mpEditor->showWarning(qsl("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);
        TestPixels::settle();
    }

    void takeTheNoticeDown()
    {
        mpEditor->hideSystemMessageArea();
        TestPixels::settle();
    }

    void openTheAlias()
    {
        QVERIFY2(mpAliasRow != nullptr, "the alias these cases edit was never made");
        mpEditor->slot_showAliases();
        TestPixels::settle();
        mpEditor->treeWidget_aliases->setCurrentItem(mpAliasRow);
        mpEditor->slot_aliasSelected(mpAliasRow);
        TestPixels::settle();
        takeTheNoticeDown();
    }

    // Leaves up whatever notice choosing the row puts over the form
    void chooseTheTrigger()
    {
        QVERIFY2(mpTriggerRow != nullptr, "the trigger these cases edit was never made");
        mpEditor->slot_showTriggers();
        TestPixels::settle();
        mpEditor->treeWidget_triggers->setCurrentItem(mpTriggerRow);
        mpEditor->slot_triggerSelected(mpTriggerRow);
        TestPixels::settle();
    }

    void openTheTrigger()
    {
        chooseTheTrigger();
        takeTheNoticeDown();
        QVERIFY2(mpEditor->mVisiblePatternCount == 3,
                 qPrintable(qsl("the trigger these cases edit came back with %1 pattern rows rather than the three it was saved with").arg(mpEditor->mVisiblePatternCount)));
    }

    bool patternListScrolls() const { return mpEditor->mpScrollArea->verticalScrollBar()->isVisible(); }

    void takeTheEditorTo(const enums::Appearance appearance)
    {
        TestPixels::setAppearanceAndWait(appearance, mpEditor);
        TestPixels::settle();
    }

    // Off a grab of the window: a widget grabbed alone is drawn against its own
    // palette rather than the column behind it
    static QColor inkAt(const QImage& shot, const QRect& box, const int depth)
    {
        const qreal ratio = shot.devicePixelRatio();
        // The first device row, so depth 0 lands inside a 1px hairline at any device
        // ratio; the middle across, clear of the rounded corners
        const int y = std::min(static_cast<int>((box.top() + depth) * ratio), shot.height() - 1);
        const int x = std::min(static_cast<int>(box.center().x() * ratio), shot.width() - 1);
        return shot.pixelColor(x, y);
    }

    int addPatternButtonOverhang() const { return bottomEdgeOf(mpEditor->mpButton_addPattern) - bottomEdgeOf(mpEditor->mpScrollArea->viewport()); }

    // A scroll area's minimum hint counts its scroll bar in even while hidden
    int roomUnderAddPattern() const { return topEdgeOf(mpEditor->mpWidget_editorCodeHeader) - bottomEdgeOf(mpEditor->mpButton_addPattern) - 1; }

    QTreeWidgetItem* variableRow(const QString& name) const
    {
        QTreeWidgetItem* pBase = mpEditor->treeWidget_variables->topLevelItem(0);
        for (int i = 0; pBase && i < pBase->childCount(); ++i) {
            if (pBase->child(i)->text(0) == name) {
                return pBase->child(i);
            }
        }
        return nullptr;
    }

    void chooseTheVariable(QTreeWidgetItem* pRow)
    {
        mpEditor->treeWidget_variables->setCurrentItem(pRow);
        mpEditor->slot_variableSelected(pRow);
        TestPixels::settle();
    }

    // QTest::mouseMove reports no button down, and a splitter follows only a held one.
    // The global point leads: the handle moves out from under the pointer as panes resize.
    static void dragTo(QWidget* pHandle, const QPointF& global)
    {
        QMouseEvent move(QEvent::MouseMove, pHandle->mapFromGlobal(global), global, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pHandle, &move);
    }

    // Raised directly: a broken item's Lua is reported on the code heading, not the notice
    void raiseANotice()
    {
        mpEditor->showError(qsl("Unable to activate this item, so the notice says so over the form"));
        TestPixels::settle();
    }

    // Small steps, since only a real drag reaches slot_rightSplitterMoved() and records a height.
    // Stops short of the code pane's floor, past which a case measures the clamp; roomForANoticeToCome
    // is what a notice raised later takes out of the same pane.
    void dragTheSeamDown(const int roomForANoticeToCome = 0)
    {
        QSplitterHandle* pHandle = mpEditor->splitter_right->handle(1);
        QVERIFY2(pHandle != nullptr, "the right hand splitter has no handle over the code pane");
        const QList<int> sizes = mpEditor->splitter_right->sizes();
        QVERIFY2(sizes.size() >= 2, "the right hand splitter has lost a pane");
        const int paneTotal = sizes.at(0) + sizes.at(1);
        // A step spare: the notice measured now and the later one can differ by a pixel of wrapping
        const int room = paneTotal - dlgTriggerEditor::scmEditorSourcePaneFloor - sizes.at(0) - roomForANoticeToCome - scmDragStep;
        const int travel = std::min(scmDragTravel, room - room % scmDragStep);
        QVERIFY2(travel >= scmDragStep,
                 qPrintable(qsl("the form pane is at %1 of %2 with %3 to come for a notice, which leaves nothing to drag into before the seam's own cap")
                                    .arg(sizes.at(0))
                                    .arg(paneTotal)
                                    .arg(roomForANoticeToCome)));
        const QPoint on = pHandle->rect().center();
        QPointF at = pHandle->mapToGlobal(QPointF(on));

        QTest::mousePress(pHandle, Qt::LeftButton, Qt::NoModifier, on);
        for (int travelled = 0; travelled < travel; travelled += scmDragStep) {
            at += QPointF(0, scmDragStep);
            dragTo(pHandle, at);
        }
        QTest::mouseRelease(pHandle, Qt::LeftButton, Qt::NoModifier, pHandle->mapFromGlobal(at).toPoint());
        TestPixels::settle();
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(scmEditorWidth, scmEditorHeight);
        TestPixels::settle();

        // Three patterns, so one row more does not fit the room the pane was given
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        TestPixels::settle();
        mpTriggerRow = mpEditor->mpCurrentTriggerItem;
        QVERIFY2(mpTriggerRow != nullptr, "the trigger these cases edit could not be made");
        mpEditor->showPatternItems(3);
        mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern->setPlainText(qsl("^one$"));
        mpEditor->mTriggerPatternEdit.at(1)->singleLineTextEdit_pattern->setPlainText(qsl("^two$"));
        mpEditor->mTriggerPatternEdit.at(2)->singleLineTextEdit_pattern->setPlainText(qsl("^three$"));
        mpEditor->slot_saveEdits();
        TestPixels::settle();

        mpEditor->slot_showAliases();
        mpEditor->addAlias(false);
        TestPixels::settle();
        mpAliasRow = mpEditor->mpCurrentAliasItem;
        QVERIFY2(mpAliasRow != nullptr, "the alias these cases edit could not be made");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    // A word-wrapping label's minimum height is its text at a very narrow width, which spread the form apart
    void test_aNoticeMovesTheFormDownAndLeavesItAlone()
    {
        openTheAlias();
        QVERIFY2(mpEditor->mpAliasMainArea->isVisible(), "the alias form is not showing, so there is nothing to measure under a notice");

        const int formHeight = mpEditor->mpAliasMainArea->height();
        const int nameTop = topEdgeOf(mpEditor->mpAliasMainArea->lineEdit_alias_name);
        const int patternTop = topEdgeOf(mpEditor->mpAliasMainArea->lineEdit_alias_pattern);

        raiseTheNotice();
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "the notice did not come up, so this case says nothing");

        const int noticeHeight = mpEditor->mpSystemMessageArea->height();
        const int columnHeight = mpEditor->mpNonCodeWidgets->height();
        qInfo().noquote() << qsl("  notice %1, gap %2, form %3, margin under it %4 -> column %5").arg(noticeHeight).arg(columnSpacing()).arg(formHeight).arg(columnBottomMargin()).arg(columnHeight);

        QVERIFY2(mpEditor->mpAliasMainArea->height() == formHeight,
                 qPrintable(qsl("the alias form is %1 tall under a notice and %2 tall without one").arg(mpEditor->mpAliasMainArea->height()).arg(formHeight)));
        QVERIFY2(columnHeight == noticeHeight + columnSpacing() + formHeight + columnBottomMargin(),
                 qPrintable(qsl("the column is %1 tall while the notice, the gap, the form and the margin under it come to %2")
                                    .arg(columnHeight)
                                    .arg(noticeHeight + columnSpacing() + formHeight + columnBottomMargin())));

        const int shift = noticeHeight + columnSpacing();
        QVERIFY2(topEdgeOf(mpEditor->mpAliasMainArea->lineEdit_alias_name) == nameTop + shift,
                 qPrintable(qsl("the name field moved from %1 to %2 while the notice takes %3").arg(nameTop).arg(topEdgeOf(mpEditor->mpAliasMainArea->lineEdit_alias_name)).arg(shift)));
        QVERIFY2(topEdgeOf(mpEditor->mpAliasMainArea->lineEdit_alias_pattern) == patternTop + shift,
                 qPrintable(qsl("the pattern field moved from %1 to %2 while the notice takes %3").arg(patternTop).arg(topEdgeOf(mpEditor->mpAliasMainArea->lineEdit_alias_pattern)).arg(shift)));

        takeTheNoticeDown();
    }

    void test_aNoticeWithNothingChosenLeadsThePane()
    {
        for (const bool triggers : {false, true}) {
            const QString view = triggers ? qsl("triggers") : qsl("aliases");
            QTreeWidget* pTree = triggers ? mpEditor->treeWidget_triggers : mpEditor->treeWidget_aliases;

            if (triggers) {
                openTheTrigger();
            } else {
                openTheAlias();
            }
            raiseTheNotice();
            QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), qPrintable(qsl("no notice came up over the chosen %1 item, so there is nothing to measure against").arg(view)));
            const int noticeTopWithAnItem = topEdgeOf(mpEditor->mpSystemMessageArea);

            QTreeWidgetItem* pRoot = pTree->topLevelItem(0);
            QVERIFY2(pRoot != nullptr, qPrintable(qsl("the %1 tree has no root row").arg(view)));
            pTree->setCurrentItem(pRoot);
            if (triggers) {
                mpEditor->slot_showTriggers();
            } else {
                mpEditor->slot_showAliases();
            }
            TestPixels::settle();

            QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), qPrintable(qsl("the %1 root row put up no notice, so this case says nothing").arg(view)));
            QVERIFY2(!mpEditor->mpSourceEditorArea->isVisible(), qPrintable(qsl("the %1 root row left the code pane showing").arg(view)));

            const int noticeTop = topEdgeOf(mpEditor->mpSystemMessageArea);
            const int noticeHeight = mpEditor->mpSystemMessageArea->height();
            const int frameHeight = mpEditor->mpSystemMessageArea->frame_notificationArea->height();
            qInfo().noquote() << qsl("  %1: a notice over an item starts at %2, one with nothing chosen at %3 and is %4 tall around a %5 tall frame")
                                         .arg(view)
                                         .arg(noticeTopWithAnItem)
                                         .arg(noticeTop)
                                         .arg(noticeHeight)
                                         .arg(frameHeight);

            QVERIFY2(noticeTop == noticeTopWithAnItem,
                     qPrintable(qsl("the %1 notice starts at %2 with nothing chosen and at %3 over a chosen item").arg(view).arg(noticeTop).arg(noticeTopWithAnItem)));
            QVERIFY2(noticeHeight == frameHeight,
                     qPrintable(qsl("the %1 notice is %2 tall around a frame of %3, so it is being stretched past the words in it").arg(view).arg(noticeHeight).arg(frameHeight)));
        }
    }

    // A Lua table has no value, so no code pane; QSplitter centres a lone child
    // shorter than the pane, which leaves the Name field half way down the window
    void test_aFormWithNoCodePaneLeadsThePane()
    {
        QVERIFY2(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("seamString = \"north\"")), "the string variable this case measures against could not be made");
        QVERIFY2(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("seamTable = { a = 1 }")), "the table variable this case edits could not be made");
        mpEditor->slot_showVariables();
        TestPixels::settle();

        // A string has a value and so a code pane, which makes its form's top the reference
        QTreeWidgetItem* pString = variableRow(qsl("seamString"));
        QVERIFY2(pString != nullptr, "the string variable is not on the Variables tree");
        chooseTheVariable(pString);
        QVERIFY2(mpEditor->mpSourceEditorArea->isVisible(), "a string variable came up with no code pane, so there is nothing to measure against");
        const int formTopOverACodePane = topEdgeOf(mpEditor->mpVarsMainArea);

        QTreeWidgetItem* pTable = variableRow(qsl("seamTable"));
        QVERIFY2(pTable != nullptr, "the table variable is not on the Variables tree");
        chooseTheVariable(pTable);
        QVERIFY2(mpEditor->mpSourceEditorArea->isHidden(), "the table variable came up with a code pane, so this case says nothing");
        QVERIFY2(mpEditor->mpVarsMainArea->isVisible(), "the variable form is away, so there is nothing to measure");

        const int formTop = topEdgeOf(mpEditor->mpVarsMainArea);
        const int wanted = mpEditor->mpVarsMainArea->sizeHint().height();
        qInfo().noquote() << qsl("  the form starts at %1 over a code pane and at %2 with none, and is %3 tall where its fields ask for %4")
                                     .arg(formTopOverACodePane)
                                     .arg(formTop)
                                     .arg(mpEditor->mpVarsMainArea->height())
                                     .arg(wanted);

        QVERIFY2(formTop == formTopOverACodePane,
                 qPrintable(qsl("the form starts at %1 with no code pane under it and at %2 with one, so it is %3 pixels down the pane")
                                    .arg(formTop)
                                    .arg(formTopOverACodePane)
                                    .arg(formTop - formTopOverACodePane)));
        QVERIFY2(topEdgeOf(mpEditor->mpNonCodeWidgets) == formTop, qPrintable(qsl("the column starts at %1 and the form in it at %2").arg(topEdgeOf(mpEditor->mpNonCodeWidgets)).arg(formTop)));
        QVERIFY2(mpEditor->mpVarsMainArea->height() == wanted,
                 qPrintable(qsl("the form is %1 tall with the pane to itself, where its fields ask for %2").arg(mpEditor->mpVarsMainArea->height()).arg(wanted)));

        raiseTheNotice();
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "the notice did not come up over the form with no code pane, so the rest of this case says nothing");
        const int shift = mpEditor->mpSystemMessageArea->height() + columnSpacing();
        QVERIFY2(topEdgeOf(mpEditor->mpVarsMainArea) == formTop + shift,
                 qPrintable(qsl("the form moved from %1 to %2 while the notice over it takes %3").arg(formTop).arg(topEdgeOf(mpEditor->mpVarsMainArea)).arg(shift)));
        QVERIFY2(mpEditor->mpVarsMainArea->height() == wanted,
                 qPrintable(qsl("the form is %1 tall under a notice with no code pane, where its fields ask for %2").arg(mpEditor->mpVarsMainArea->height()).arg(wanted)));
        takeTheNoticeDown();
    }

    void test_theAddPatternButtonKeepsItsPlaceUnderANotice()
    {
        openTheTrigger();
        QVERIFY2(mpEditor->mpTriggersMainArea->isVisible(), "the trigger form is not showing, so there is nothing to measure");

        const int roomWithout = roomUnderAddPattern();
        raiseTheNotice();
        const int roomWith = roomUnderAddPattern();
        qInfo().noquote() << qsl("  Add pattern has %1 empty rows under it before the code heading without a notice and %2 with one, where the column holds it off by %3")
                                     .arg(roomWithout)
                                     .arg(roomWith)
                                     .arg(columnBottomMargin());

        QVERIFY2(roomWith >= 0 && roomWith <= columnBottomMargin() + scmSeamTolerance && qAbs(roomWith - roomWithout) <= 2,
                 qPrintable(qsl("Add pattern has %1 empty rows under it with a notice up and %2 without one, where the column holds it off the code heading by %3")
                                    .arg(roomWith)
                                    .arg(roomWithout)
                                    .arg(columnBottomMargin())));
        QVERIFY2(!patternListScrolls(), "the pattern list is scrolling with a notice up, so the rows lost the room the notice took");

        takeTheNoticeDown();
    }

    void test_addingAPatternRowMakesRoomForIt()
    {
        openTheTrigger();
        QVERIFY2(!patternListScrolls(), "the pattern list is already scrolling before a row is added, so this case says nothing");
        const int rowsBefore = mpEditor->mVisiblePatternCount;

        mpEditor->slot_addPattern();
        TestPixels::settle();

        qInfo().noquote() << qsl("  %1 rows became %2, and Add pattern overhangs the list by %3").arg(rowsBefore).arg(mpEditor->mVisiblePatternCount).arg(addPatternButtonOverhang());
        QCOMPARE(mpEditor->mVisiblePatternCount, rowsBefore + 1);
        QVERIFY2(addPatternButtonOverhang() <= 0, qPrintable(qsl("Add pattern hangs %1 pixels past the bottom of the pattern list after one row was added").arg(addPatternButtonOverhang())));
        QVERIFY2(!patternListScrolls(), "a scroll bar came up beside the pattern list after one row was added");
    }

    void test_deletingAPatternRowGivesTheRoomBack()
    {
        openTheTrigger();
        const int rowsBefore = mpEditor->mVisiblePatternCount;
        const int columnBefore = mpEditor->mpNonCodeWidgets->height();

        mpEditor->deletePatternRow(0);
        TestPixels::settle();

        qInfo().noquote()
                << qsl("  the column was %1 tall with %2 rows and is %3 tall with %4").arg(columnBefore).arg(rowsBefore).arg(mpEditor->mpNonCodeWidgets->height()).arg(mpEditor->mVisiblePatternCount);
        QVERIFY2(mpEditor->mVisiblePatternCount < rowsBefore, "the deleted row was not taken off the list");
        QVERIFY2(mpEditor->mpNonCodeWidgets->height() < columnBefore,
                 qPrintable(qsl("the column is still %1 tall with %2 rows, where %3 rows asked for %4")
                                    .arg(mpEditor->mpNonCodeWidgets->height())
                                    .arg(mpEditor->mVisiblePatternCount)
                                    .arg(rowsBefore)
                                    .arg(columnBefore)));
    }

    // The notice goes up before the pane is sized, the order a selection runs in:
    // sizing counts whatever notice is over the form into the column
    void test_aDraggedSeamKeepsItsHeightAcrossANotice()
    {
        chooseTheTrigger();
        raiseANotice();
        // Measured while up: the drag must stop short of the room it will ask for later
        const int noticeRoomToCome = mpEditor->noticeRoomInFormColumn();
        takeTheNoticeDown();

        dragTheSeamDown(noticeRoomToCome);
        const int draggedTo = mpEditor->splitter_right->sizes().at(0);
        QVERIFY2(mpEditor->mDraggedFormPaneHeights.contains(EditorViewType::cmTriggerView), "the drag never reached the splitter, so no height was written down and this case says nothing");

        chooseTheTrigger();
        raiseANotice();
        mpEditor->fitFormPaneToItsContents();
        TestPixels::settle();
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "the notice this case raised is not up, so the pane was not sized under one");
        const int noticeRoom = mpEditor->noticeRoomInFormColumn();
        const int underTheNotice = mpEditor->splitter_right->sizes().at(0);

        takeTheNoticeDown();
        const int afterTheNotice = mpEditor->splitter_right->sizes().at(0);

        qInfo().noquote() << qsl("  the seam was dragged to %1, an item chosen under a notice worth %2 left it at %3, and taking the notice down left %4")
                                     .arg(draggedTo)
                                     .arg(noticeRoom)
                                     .arg(underTheNotice)
                                     .arg(afterTheNotice);

        QVERIFY2(noticeRoom > 0, "the notice cost the column nothing, so this case says nothing");
        QVERIFY2(qAbs(underTheNotice - (draggedTo + noticeRoom)) <= scmSeamTolerance,
                 qPrintable(qsl("the item chosen under the notice left the seam at %1 where the dragged %2 and the notice's %3 come to %4 - the list paid for the notice twice")
                                    .arg(underTheNotice)
                                    .arg(draggedTo)
                                    .arg(noticeRoom)
                                    .arg(draggedTo + noticeRoom)));
        QVERIFY2(qAbs(afterTheNotice - draggedTo) <= scmSeamTolerance, qPrintable(qsl("the seam was dragged to %1 and a notice that came and went left it at %2").arg(draggedTo).arg(afterTheNotice)));

        // A dragged height lasts the session, and the other cases expect a seam that snaps to its item
        mpEditor->mDraggedFormPaneHeights.remove(EditorViewType::cmTriggerView);
    }

    void test_theHiddenVariablesCountLeavesWithTheVariablesView()
    {
        mpEditor->slot_showVariables();
        TestPixels::settle();
        QVERIFY2(mpEditor->mpLabel_hiddenVariablesCount->isVisible(), "nothing is hidden in this profile's variables, so the reading beside the switch was never shown and this case says nothing");

        mpEditor->slot_showTriggers();
        TestPixels::settle();
        QVERIFY2(!mpEditor->mpLabel_hiddenVariablesCount->isVisible(), qPrintable(qsl("the Triggers view is still being told \"%1\"").arg(mpEditor->mpLabel_hiddenVariablesCount->text())));

        QVERIFY2(!mpEditor->checkBox_displayAllVariables->isVisible(), "the switch itself is still showing outside the Variables view");
    }

    void test_aTriggerWithMoreRowsThanFitOpensAtItsFirstRow()
    {
        chooseTheTrigger();
        takeTheNoticeDown();
        constexpr int rows = 16;
        mpEditor->showPatternItems(rows);
        for (int row = 0; row < rows; ++row) {
            mpEditor->mTriggerPatternEdit.at(row)->singleLineTextEdit_pattern->setPlainText(qsl("^row %1$").arg(row + 1));
        }
        mpEditor->slot_saveEdits();
        TestPixels::settle();
        chooseTheTrigger();
        takeTheNoticeDown();

        QScrollBar* pBar = mpEditor->mpScrollArea->verticalScrollBar();
        QVERIFY2(pBar->maximum() > 0, "sixteen rows fit the pane, so there is nothing to scroll and this case says nothing");
        QVERIFY2(pBar->value() == 0, qPrintable(qsl("the pattern list opened scrolled to %1 of %2 rather than at its first row").arg(pBar->value()).arg(pBar->maximum())));

        // Back to the three rows the other cases are written for
        mpEditor->showPatternItems(3);
        mpEditor->slot_saveEdits();
        TestPixels::settle();
    }

    void test_aDragCannotTakeTheCodePaneUnderItsFloor()
    {
        chooseTheTrigger();
        takeTheNoticeDown();
        QSplitterHandle* pHandle = mpEditor->splitter_right->handle(1);
        QVERIFY2(pHandle != nullptr, "the right hand splitter has no handle over the code pane");
        const QList<int> before = mpEditor->splitter_right->sizes();
        QVERIFY2(before.size() >= 2, "the right hand splitter has lost a pane");
        const int paneTotal = before.at(0) + before.at(1);

        // Further than the pane can go, so wherever the drag stops is the limit
        const QPoint on = pHandle->rect().center();
        QPointF at = pHandle->mapToGlobal(QPointF(on));
        QTest::mousePress(pHandle, Qt::LeftButton, Qt::NoModifier, on);
        for (int travelled = 0; travelled < paneTotal; travelled += scmDragStep) {
            at += QPointF(0, scmDragStep);
            dragTo(pHandle, at);
        }
        QTest::mouseRelease(pHandle, Qt::LeftButton, Qt::NoModifier, pHandle->mapFromGlobal(at).toPoint());
        TestPixels::settle();

        const QList<int> after = mpEditor->splitter_right->sizes();
        qInfo().noquote() << qsl("  dragged the seam from %1 / %2 to %3 / %4").arg(before.at(0)).arg(before.at(1)).arg(after.at(0)).arg(after.at(1));
        QVERIFY2(after.at(0) > before.at(0), "the drag did not move the seam at all, so this case says nothing");
        QVERIFY2(after.at(1) >= dlgTriggerEditor::scmEditorSourcePaneFloor,
                 qPrintable(qsl("a drag took the code pane down to %1, under its floor of %2").arg(after.at(1)).arg(dlgTriggerEditor::scmEditorSourcePaneFloor)));
    }

    // Guards a second, legacy sheet on the frame that an appearance change left
    // showing as a 3px band in the text colour over the wash
    void test_theNoticeIsDrawnInTheAccentInEitherAppearance()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([appearanceBefore]() {
            mudlet::self()->setAppearance(appearanceBefore);
        });

        openTheAlias();
        raiseTheNotice();
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "the notice did not come up, so there is no edge to read");
        QFrame* pFrame = mpEditor->mpSystemMessageArea->frame_notificationArea;

        QStringList misses;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            takeTheEditorTo(appearance.second);
            QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "the notice went away over the appearance change");

            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QImage shot = mpEditor->grab().toImage();
            const QRect frameInEditor(pFrame->mapTo(mpEditor, QPoint(0, 0)), pFrame->size());
            const QColor border = inkAt(shot, frameInEditor, 0);
            const QColor justInside = inkAt(shot, frameInEditor, scmInsideTheBand);
            const QColor behindIt = inkAt(shot, frameInEditor, pFrame->height() + scmBelowTheNotice);
            const QColor wash = uiDesign::blend(behindIt, tokens.accent, scmAccentWashAlpha);

            qInfo().noquote() << qsl("  %1: the accent is %2, the frame's edge %3, %4px in %5 against a wash of %6 over the %7 behind it")
                                         .arg(appearance.first, tokens.accent.name(), border.name())
                                         .arg(scmInsideTheBand)
                                         .arg(justInside.name(), wash.name(), behindIt.name());

            if (TestPixels::distance(border, tokens.accent) > scmInkSlack) {
                misses << qsl("%1: the frame's edge is %2 against the accent %3 it is drawn in").arg(appearance.first, border.name(), tokens.accent.name());
            }
            // A 3px band reads the same at both depths, a hairline does not
            if (TestPixels::distance(justInside, border) <= scmInkSlack) {
                misses << qsl("%1: the frame is %2 at its edge and %3 %4px in, so what it carries is a band rather than a hairline")
                                  .arg(appearance.first, border.name(), justInside.name())
                                  .arg(scmInsideTheBand);
            }
            if (TestPixels::distance(justInside, wash) > scmInkSlack) {
                misses << qsl("%1: the notice is filled with %2 rather than the %3 the accent's wash comes to over the %4 behind it")
                                  .arg(appearance.first, justInside.name(), wash.name(), behindIt.name());
            }
            if (pFrame->styleSheet().contains(qsl("3px"))) {
                misses << qsl("%1: the frame still carries a 3px rule: %2").arg(appearance.first, pFrame->styleSheet());
            }
        }
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));

        takeTheNoticeDown();
    }
};

#include "EditorNoticeSeamTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorNoticeSeamTest)
