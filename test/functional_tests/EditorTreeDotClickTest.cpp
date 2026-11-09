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
 * Guards the viewport filter EditorTreeDelegate uses for clicks on a row's dot and
 * chevron: the view records a press before consulting a delegate, and consults one
 * only for events on a row, so editorEvent() alone cannot see a press through.
 *
 * Run with: ctest -R EditorTreeDotClickTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "EditorTreeDelegate.h"
#include "Host.h"
#include "TTreeWidget.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
// QAbstractItemView keeps state() and State protected; using-declarations in a
// subclass make them public, and &ViewStatePeek::state is still a pointer to a
// QAbstractItemView member, so the view is read with no cast of the object
struct ViewStatePeek : QTreeView
{
    using QAbstractItemView::NoState;
    using QAbstractItemView::State;

    static State of(const QAbstractItemView* pView)
    {
        State (QAbstractItemView::*pStateGetter)() const = &ViewStatePeek::state;
        return (pView->*pStateGetter)();
    }
};
} // namespace

class EditorTreeDotClickTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("TreeDotClick-Test-Profile");

    TTreeWidget* tree() const { return mpEditor->treeWidget_triggers; }
    QWidget* viewport() const { return tree()->viewport(); }
    QTreeWidgetItem* baseItem() const { return mpEditor->mpTriggerBaseItem; }

    // QTreeWidget::indexFromItem() is protected
    QModelIndex indexOf(QTreeWidgetItem* pItem) const { return tree()->indexAt(tree()->visualItemRect(pItem).center()); }

    uiDesign::EditorTreeDelegate* dotDelegate() const { return qobject_cast<uiDesign::EditorTreeDelegate*>(tree()->itemDelegateForIndex(QModelIndex())); }

    QRect dotRect(QTreeWidgetItem* pItem) const
    {
        const QModelIndex index = indexOf(pItem);
        return dotDelegate()->dotHitRect(dotDelegate()->rowOption(index), index);
    }

    QRect chevronRect(QTreeWidgetItem* pItem) const
    {
        const QModelIndex index = indexOf(pItem);
        return dotDelegate()->chevronHitRect(dotDelegate()->rowOption(index), index);
    }

    // addTrigger() adds inside the chosen folder and then chooses the new item,
    // so the trigger lands inside the group
    QTreeWidgetItem* aGroupHoldingATrigger()
    {
        tree()->setCurrentItem(baseItem());
        mpEditor->addTrigger(true);
        QTreeWidgetItem* pGroup = tree()->currentItem();
        mpEditor->addTrigger(false);
        // The expand is animated, and a view mid-animation answers a press
        // itself rather than passing it on
        QTest::qWait(400ms);
        return pGroup;
    }

    // Counts requests rather than reading the item: activeToggle_trigger() forces
    // a trigger with no patterns, as every new one is, straight back off.
    // toggleRequested and itemActivated both reach that one slot.
    struct ToggleCounter
    {
        ToggleCounter(uiDesign::EditorTreeDelegate* pDelegate, TTreeWidget* pTree)
        : delegateSpy(pDelegate, &uiDesign::EditorTreeDelegate::toggleRequested)
        , activatedSpy(pTree, &QTreeWidget::itemActivated)
        {
        }

        [[nodiscard]] int count() const { return delegateSpy.count() + activatedSpy.count(); }

        QSignalSpy delegateSpy;
        QSignalSpy activatedSpy;
    };

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);

        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->slot_showTriggers();
        QTest::qWait(50ms);
        QVERIFY2(dotDelegate() != nullptr, "The triggers tree should be drawn by an EditorTreeDelegate");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void init()
    {
        mpEditor->slot_showTriggers();
        tree()->clearSelection();
        tree()->setCurrentItem(nullptr);
        QCoreApplication::processEvents();

        while (baseItem()->childCount() > 0) {
            tree()->setCurrentItem(baseItem()->child(0));
            mpEditor->slot_deleteItemOrGroup();
        }
        tree()->setCurrentItem(nullptr);
        mpEditor->addTrigger(false);
        mpEditor->addTrigger(false);
        baseItem()->setExpanded(true);
        QCoreApplication::processEvents();

        QCOMPARE(baseItem()->childCount(), 2);
        QVERIFY2(!tree()->visualItemRect(baseItem()->child(1)).isEmpty(), "Both rows should be laid out and visible");
    }

    void testSingleClickOnDotTogglesOnce()
    {
        QTreeWidgetItem* pItem = baseItem()->child(1);
        tree()->setCurrentItem(baseItem()->child(0));
        QCoreApplication::processEvents();

        const QRect dot = dotRect(pItem);
        QVERIFY2(!dot.isEmpty(), "The row should report a dot to click");

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, dot.center());
        QCoreApplication::processEvents();

        QCOMPARE(toggles.count(), 1);
        // The toggle slot reads the tree's current item rather than being handed a row
        QCOMPARE(tree()->currentItem(), pItem);
    }

    void testDoubleClickOnDotTogglesOnce()
    {
        QTreeWidgetItem* pItem = baseItem()->child(1);
        const QRect dot = dotRect(pItem);
        QVERIFY(!dot.isEmpty());

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mousePress(viewport(), Qt::LeftButton, Qt::NoModifier, dot.center());
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, dot.center());
        QTest::mouseDClick(viewport(), Qt::LeftButton, Qt::NoModifier, dot.center());
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, dot.center());
        QCoreApplication::processEvents();

        QCOMPARE(toggles.count(), 1);
    }

    // A double click tolerates a few pixels of drift, so its second press can
    // land off the small dot; handed on, it reaches activated() and flips the row back
    void testDriftedDoubleClickTogglesOnce()
    {
        QTreeWidgetItem* pItem = baseItem()->child(1);
        const QRect dot = dotRect(pItem);
        QVERIFY(!dot.isEmpty());
        const QPoint onDot(dot.right() - 1, dot.center().y());
        const QPoint drifted = onDot + QPoint(4, 0);
        QVERIFY2(dot.contains(onDot), "The first press has to land on the dot");
        QVERIFY2(!dot.contains(drifted), "The second press has to land off the dot");
        QVERIFY2(tree()->visualItemRect(pItem).contains(drifted), "...but still on the same row");
        QCOMPARE(tree()->indexAt(drifted), indexOf(pItem));

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mousePress(viewport(), Qt::LeftButton, Qt::NoModifier, onDot);
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, onDot);
        QTest::mouseDClick(viewport(), Qt::LeftButton, Qt::NoModifier, drifted);
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, drifted);
        QCoreApplication::processEvents();

        QCOMPARE(toggles.count(), 1);
    }

    // The pointer leaves the rows too, where the view stops consulting the delegate.
    // The drag distance is raised so no real drag starts: QDrag::exec() runs a modal
    // loop only a release ends, and DraggingState is entered before distance is measured.
    void testPressOnDotDoesNotStartDrag()
    {
        QTreeWidgetItem* pItem = baseItem()->child(1);
        const QRect dot = dotRect(pItem);
        QVERIFY(!dot.isEmpty());
        const QRect lastRow = tree()->visualItemRect(pItem);
        const QPoint blank(lastRow.center().x(), lastRow.bottom() + 12);
        QVERIFY2(viewport()->rect().contains(blank), "The tree needs blank space below its last row for this case");
        QVERIFY2(!tree()->indexAt(blank).isValid(), "...which is what makes it blank");

        const int rowInParent = baseItem()->indexOfChild(pItem);

        const int savedDragDistance = QApplication::startDragDistance();
        QApplication::setStartDragDistance(10000);

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mousePress(viewport(), Qt::LeftButton, Qt::NoModifier, dot.center());
        QTest::mouseMove(viewport(), dot.center() + QPoint(2, 6));
        QTest::mouseMove(viewport(), lastRow.center());
        QTest::mouseMove(viewport(), blank);
        QTest::mouseMove(viewport(), blank + QPoint(20, 8));
        const ViewStatePeek::State stateWhileHeld = ViewStatePeek::of(tree());
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, blank + QPoint(20, 8));
        QCoreApplication::processEvents();

        QApplication::setStartDragDistance(savedDragDistance);

        QCOMPARE(stateWhileHeld, ViewStatePeek::NoState);
        QCOMPARE(toggles.count(), 1);
        QCOMPARE(pItem->parent(), baseItem());
        QCOMPARE(baseItem()->indexOfChild(pItem), rowInParent);
        QCOMPARE(baseItem()->childCount(), 2);
    }

    // The view never hands back a release on another row; a delegate that misses
    // it thinks the dot press is live and swallows every later drag-select's moves
    void testReleaseOffTheRowLeavesDragSelectWorking()
    {
        QTreeWidgetItem* pFirst = baseItem()->child(0);
        QTreeWidgetItem* pSecond = baseItem()->child(1);
        const QRect dot = dotRect(pFirst);
        QVERIFY(!dot.isEmpty());
        const QRect secondRow = tree()->visualItemRect(pSecond);
        const QPoint blank(secondRow.center().x(), secondRow.bottom() + 12);
        QVERIFY2(viewport()->rect().contains(blank), "The tree needs blank space below its last row for this case");
        QVERIFY2(!tree()->indexAt(blank).isValid(), "...which is what makes it blank");

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mousePress(viewport(), Qt::LeftButton, Qt::NoModifier, dot.center());
        QTest::mouseMove(viewport(), secondRow.center());
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, secondRow.center());
        QCoreApplication::processEvents();

        QCOMPARE(toggles.count(), 1);

        tree()->clearSelection();
        QCoreApplication::processEvents();
        QTest::mousePress(viewport(), Qt::LeftButton, Qt::NoModifier, blank);
        QTest::mouseMove(viewport(), secondRow.center());
        QTest::mouseMove(viewport(), tree()->visualItemRect(pFirst).center());
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, tree()->visualItemRect(pFirst).center());
        QCoreApplication::processEvents();

        const QList<QTreeWidgetItem*> selected = tree()->selectedItems();
        QVERIFY2(selected.contains(pFirst) && selected.contains(pSecond), qPrintable(qsl("A drag-select across both rows should have selected both, but selected %1 item(s)").arg(selected.size())));
    }

    void testClickOnChevronFoldsTheRowAndLeavesTheSelectionAlone()
    {
        QTreeWidgetItem* pGroup = aGroupHoldingATrigger();
        QTreeWidgetItem* pChosen = tree()->currentItem();
        QVERIFY2(pGroup && pGroup->childCount() == 1, "The group this case folds was not made");
        QVERIFY2(pGroup->isExpanded(), "The editor leaves a group it has just added to open");

        const QRect chevron = chevronRect(pGroup);
        QVERIFY2(!chevron.isEmpty(), "A group with something inside it should report a chevron to click");

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
        QCoreApplication::processEvents();
        QVERIFY2(!pGroup->isExpanded(), "A click on the chevron did not fold the group away");

        // The collapse is animated too
        QTest::qWait(400ms);
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, chevronRect(pGroup).center());
        QCoreApplication::processEvents();
        QVERIFY2(pGroup->isExpanded(), "A second click on the chevron did not open the group again");

        QCOMPARE(toggles.count(), 0);
        QCOMPARE(tree()->currentItem(), pChosen);
    }

    // Guards the delegate's viewport event filter: it eats some pointer events,
    // and a key has to pass straight through it
    void testArrowKeysStillFoldTheRow()
    {
        uiDesign::EditorTreeDelegate* pDelegate = dotDelegate();
        QVERIFY2(pDelegate != nullptr, "the triggers tree is not drawn by an EditorTreeDelegate, so there is no filter here for a key to be swallowed by");
        QVERIFY2(tree()->itemDelegate() == pDelegate, "the tree's rows are drawn by something other than the delegate that carries the filter");
        // Qt cannot list a widget's event filters, so the filter's place on the
        // viewport is not asserted

        QTreeWidgetItem* pGroup = aGroupHoldingATrigger();
        tree()->setCurrentItem(pGroup);
        tree()->setFocus();
        QCoreApplication::processEvents();
        QVERIFY(pGroup->isExpanded());

        QTest::keyClick(tree(), Qt::Key_Left);
        QCoreApplication::processEvents();
        QVERIFY2(!pGroup->isExpanded(), "The left arrow key did not fold the group away");

        QTest::keyClick(tree(), Qt::Key_Right);
        QCoreApplication::processEvents();
        QVERIFY2(pGroup->isExpanded(), "The right arrow key did not open the group again");
    }

    // The pair's second press, handed on, would reach activated() and switch the group
    void testDoubleClickOnChevronFoldsOnceAndSwitchesNothing()
    {
        QTreeWidgetItem* pGroup = aGroupHoldingATrigger();
        const QRect chevron = chevronRect(pGroup);
        QVERIFY(!chevron.isEmpty());
        QVERIFY(pGroup->isExpanded());

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mousePress(viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
        QTest::mouseDClick(viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, chevron.center());
        QCoreApplication::processEvents();

        QCOMPARE(toggles.count(), 0);
        QVERIFY2(!pGroup->isExpanded(), "A double click on the chevron should leave the group folded, having folded it once");
    }

    void testDoubleClickOnTheRowStillSwitchesIt()
    {
        QTreeWidgetItem* pGroup = aGroupHoldingATrigger();
        const QRect row = tree()->visualItemRect(pGroup);
        const QRect chevron = chevronRect(pGroup);
        const QPoint onTheName(row.right() - 6, row.center().y());
        QVERIFY2(!chevron.contains(onTheName) && !dotRect(pGroup).contains(onTheName), "The point this case double clicks has to be clear of both marks the delegate answers for");

        ToggleCounter toggles(dotDelegate(), tree());
        QTest::mousePress(viewport(), Qt::LeftButton, Qt::NoModifier, onTheName);
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, onTheName);
        QTest::mouseDClick(viewport(), Qt::LeftButton, Qt::NoModifier, onTheName);
        QTest::mouseRelease(viewport(), Qt::LeftButton, Qt::NoModifier, onTheName);
        QCoreApplication::processEvents();

        QCOMPARE(toggles.count(), 1);
    }
};

#include "EditorTreeDotClickTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorTreeDotClickTest)
