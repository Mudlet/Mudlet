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
 * How far a tab has to be dragged before it detaches into its own window is
 * decided in TTabBar::mouseMoveEvent(), and nothing exercised it: the
 * detached-window tests all call mudlet::slot_tabDetachRequested() directly,
 * which is where that decision has already been made.
 *
 * The decision is a chain of gates - a press that landed on a tab, Qt's tab
 * reorder delay, leaving the tab bar's rectangle, a predominantly vertical move
 * - and only then how far the cursor has come since the press, against
 * DETACH_DISTANCE_THRESHOLD. Most drags here are straight down from a press on
 * a tab, so the gates ahead of the distance are satisfied the same way each time
 * and the distance is the only thing that can differ between a detach and no
 * detach; the cases named after a gate of their own are the exceptions.
 *
 * Run with: ctest -R TabDetachThresholdTest -V
 */

#include <QApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include "TTabBar.h"

#include "GroupedTest.h"

class TabDetachThresholdTest : public QObject
{
    Q_OBJECT

private:
    TTabBar* mpTabBar = nullptr;

    // TAB_REORDER_DELAY_MS in TTabBar.cpp: until a drag has outlasted it,
    // mouseMoveEvent() leaves the whole detach decision alone
    static constexpr int mReorderDelayMs = 150;
    // Between the 50px the threshold used to be and the 80px it is, so this is
    // the distance that tells the two apart
    static constexpr int mBetweenThresholdsDistance = 65;
    // Well short of the threshold, but far enough to clear the tab bar's bottom
    // edge - one of the gates ahead of the distance comparison
    static constexpr int mShortDistance = 30;
    // DETACH_DISTANCE_THRESHOLD itself. The comparison is >, so a drag reaching
    // exactly this far is still a short one
    static constexpr int mThresholdDistance = 80;
    static constexpr int mPastThresholdDistance = 90;
    // Short of the threshold on its own, but 60 of the 100 Manhattan pixels it
    // makes up with the horizontal leg below, which is exactly the share the
    // ratio gate asks for
    static constexpr int mDiagonalVerticalDistance = 60;
    static constexpr int mDiagonalHorizontalDistance = 40;
    // Past the threshold by any measure, but only 15% of it vertical
    static constexpr int mSidewaysDistance = 200;
    // Just past the tab bar's bottom edge, so a mostly sideways drag is outside
    // the bar and the vertical ratio is the only gate left to refuse it
    static constexpr int mJustBelowTheBarDistance = 35;

private slots:
    void init()
    {
        mpTabBar = new TTabBar(nullptr);
        // The two things mudlet.cpp sets that change how QTabBar handles a press
        // and a drag of its own accord, since that handling runs either side of
        // the detach check
        mpTabBar->setMovable(true);
        mpTabBar->setTabsClosable(true);
        // Three tabs of similarly sized names, so the centre of the bar - where
        // most of the drags below start - lands inside a tab rather than on the
        // seam between two of them
        for (const QString& profileName : {qsl("Alpha"), qsl("Bravo"), qsl("Delta")}) {
            mpTabBar->setTabData(mpTabBar->addTab(profileName), profileName);
        }
        // Tab rectangles are laid out against style and font metrics that are
        // not final until the widget has been polished: before that the bar can
        // report tabs wider than itself and a press aimed at a tab centre lands
        // beside it. Polishing is synchronous, so nothing here has to wait.
        mpTabBar->ensurePolished();
        // Twice the hint rather than exactly it: QTabBar stretches its tabs to
        // fill the width it is given, so the tabs tile the bar and every tab
        // centre is over its own tab whatever the hint came out as
        mpTabBar->resize(mpTabBar->sizeHint().width() * 2, mpTabBar->sizeHint().height());
    }

    void cleanup()
    {
        delete mpTabBar;
        mpTabBar = nullptr;
    }

    // The case the old threshold and the current one disagree about: 65px used to
    // be far enough to tear a tab out of the window, and now has to leave it be
    void test_aDragBetweenTheOldAndTheNewThresholdKeepsTheTabAttached()
    {
        const QSignalSpy detachSpy(mpTabBar, &TTabBar::tabDetachRequested);
        pressTheCentreTab();
        if (QTest::currentTestFailed()) {
            return;
        }

        dragTo(mBetweenThresholdsDistance);

        QVERIFY2(detachSpy.isEmpty(),
                 qPrintable(qsl("a drag of %1px from the tab bar's centre detached the tab, so the threshold in force is the old 50px rather than 80px").arg(mBetweenThresholdsDistance)));

        // Carrying the same drag past the threshold has to detach, otherwise the
        // silence above says nothing about the distance: it would mean this drag
        // never got through one of the gates ahead of the distance comparison
        dragTo(mPastThresholdDistance);
        QCOMPARE(detachSpy.count(), 1);
    }

    // The comparison is > and not >=, so the threshold distance itself is still a
    // drag that keeps the tab where it is
    void test_aDragToExactlyTheThresholdKeepsTheTabAttached()
    {
        const QSignalSpy detachSpy(mpTabBar, &TTabBar::tabDetachRequested);
        pressTheCentreTab();
        if (QTest::currentTestFailed()) {
            return;
        }

        dragTo(mThresholdDistance);

        QVERIFY2(detachSpy.isEmpty(), qPrintable(qsl("a drag of exactly the %1px threshold detached the tab").arg(mThresholdDistance)));
    }

    void test_aDragPastTheThresholdDetachesTheTab()
    {
        const QSignalSpy detachSpy(mpTabBar, &TTabBar::tabDetachRequested);
        const int pressedTab = pressTheCentreTab();
        if (pressedTab < 0) {
            return;
        }

        dragTo(mPastThresholdDistance);

        QCOMPARE(detachSpy.count(), 1);
        // What mudlet::slot_tabDetachRequested() is handed: the tab the press
        // landed on, and the point the new window opens at
        QCOMPARE(detachSpy.at(0).at(0).toInt(), pressedTab);
        QCOMPARE(detachSpy.at(0).at(1).toPoint(), mpTabBar->mapToGlobal(dragTarget(mPastThresholdDistance)));

        // A drag is spent once it has asked for a window, so dragging on does not
        // ask for a second one
        dragTo(mPastThresholdDistance + 50);
        QCOMPARE(detachSpy.count(), 1);
    }

    // The threshold used to be compared against the cursor's distance from the
    // tab bar's centre, which added the press point's own offset from that
    // centre before the drag had gone anywhere: a tab near either end of the bar
    // tore out after a few pixels, and the same tab grabbed near its inner edge
    // needed far more, while a tab in the middle needed the whole 80px
    void test_theSameDragDetachesATabWhereverItSitsInTheBar()
    {
        for (int index = 0, total = mpTabBar->count(); index < total; ++index) {
            const QSignalSpy detachSpy(mpTabBar, &TTabBar::tabDetachRequested);
            // Held for the whole drag rather than read again from the tab, so
            // that the moves below stay straight down from where the press
            // landed however the bar lays itself out meanwhile
            const QPoint press = mpTabBar->tabRect(index).center();
            QVERIFY2(mpTabBar->tabAt(press) == index,
                     qPrintable(qsl("the centre of tab %1 is over tab %2 instead, so a press there drags the wrong tab or none at all").arg(index).arg(mpTabBar->tabAt(press))));
            pressAt(press);

            const QPoint shortTarget = press + QPoint(0, mShortDistance);
            QVERIFY2(!mpTabBar->rect().contains(shortTarget), "the short drag stayed inside the tab bar, which stops mouseMoveEvent() before it reaches the distance");
            // The tabs at either end have to sit far enough from the bar's
            // centre that the short drag is a long one by the old measurement
            // and a short one by the new: without that this case cannot tell the
            // two apart and proves nothing
            if (index == 0 || index == total - 1) {
                QVERIFY2(distanceFromBarCentre(shortTarget) > mThresholdDistance,
                         qPrintable(qsl("tab %1 sits within %2px of the tab bar's centre, so a %3px drag from it is short by either measurement")
                                            .arg(index)
                                            .arg(mThresholdDistance - mShortDistance)
                                            .arg(mShortDistance)));
            }

            moveTo(shortTarget);
            QVERIFY2(detachSpy.isEmpty(),
                     qPrintable(qsl("a %1px drag detached tab %2, so how far that tab has to be dragged still depends on where it sits in the bar").arg(mShortDistance).arg(index)));

            // The same drag that detaches the middle tab has to detach this one
            // too, otherwise the tab staying put above says nothing about the
            // distance: it would mean the drag never reached the comparison
            const QPoint longTarget = press + QPoint(0, mPastThresholdDistance);
            moveTo(longTarget);
            QVERIFY2(detachSpy.count() == 1, qPrintable(qsl("a %1px drag left tab %2 attached").arg(mPastThresholdDistance).arg(index)));

            sendMouse(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, longTarget);
        }
    }

    // Dragging a tab sideways is how it is reordered, so however far it goes
    // that drag must not tear it out of the window
    void test_aMostlySidewaysDragDoesNotDetachTheTab()
    {
        const QSignalSpy detachSpy(mpTabBar, &TTabBar::tabDetachRequested);
        const QPoint press = mpTabBar->tabRect(0).center();
        pressAt(press);

        const QPoint sidewaysTarget = press + QPoint(mSidewaysDistance, mJustBelowTheBarDistance);
        // Outside the bar and far past the threshold, so the vertical ratio is
        // the only gate left that can refuse this drag
        QVERIFY2(!mpTabBar->rect().contains(sidewaysTarget), "the sideways drag stayed inside the tab bar, which stops mouseMoveEvent() before it reaches the vertical ratio");
        QVERIFY((sidewaysTarget - press).manhattanLength() > mThresholdDistance);
        moveTo(sidewaysTarget);

        QVERIFY2(detachSpy.isEmpty(), "a mostly sideways drag detached the tab instead of reordering it");
    }

    // Qt gets its reorder delay before any of this is considered, so the very
    // same drag has to be refused until that delay has passed
    void test_aDragBeforeTheReorderDelayDoesNotDetachTheTab()
    {
        const QSignalSpy detachSpy(mpTabBar, &TTabBar::tabDetachRequested);
        const QPoint press = mpTabBar->rect().center();
        sendMouse(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, press);

        moveTo(press + QPoint(0, mPastThresholdDistance));
        QVERIFY2(detachSpy.isEmpty(), "a drag detached the tab before Qt's tab reorder delay had passed");

        // The identical drag once the delay is up, otherwise the silence above
        // says nothing about the delay
        QTest::qWait(mReorderDelayMs + 50);
        moveTo(press + QPoint(0, mPastThresholdDistance));
        QCOMPARE(detachSpy.count(), 1);
    }

    // The threshold is compared against a Manhattan length, so sideways travel
    // counts toward it as well and a diagonal drag detaches on less vertical
    // travel than a straight one
    void test_aDiagonalDragDetachesOnLessVerticalTravelThanAStraightOne()
    {
        const QPoint press = mpTabBar->rect().center();
        {
            const QSignalSpy straightSpy(mpTabBar, &TTabBar::tabDetachRequested);
            pressAt(press);
            const QPoint straightTarget = press + QPoint(0, mDiagonalVerticalDistance);
            moveTo(straightTarget);
            QVERIFY2(straightSpy.isEmpty(), qPrintable(qsl("%1px of vertical travel on its own reached the %2px threshold").arg(mDiagonalVerticalDistance).arg(mThresholdDistance)));
            sendMouse(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, straightTarget);
        }

        const QSignalSpy diagonalSpy(mpTabBar, &TTabBar::tabDetachRequested);
        pressAt(press);
        moveTo(press + QPoint(mDiagonalHorizontalDistance, mDiagonalVerticalDistance));

        QVERIFY2(
                diagonalSpy.count() == 1,
                qPrintable(qsl("the same %1px of vertical travel with %2px of sideways travel beside it did not reach the threshold").arg(mDiagonalVerticalDistance).arg(mDiagonalHorizontalDistance)));
    }

private:
    // mouseMoveEvent() maps the local position to global itself rather than
    // reading the event's, so the two are kept consistent here
    void sendMouse(QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons, const QPoint& localPos)
    {
        QMouseEvent event(type, QPointF(localPos), QPointF(mpTabBar->mapToGlobal(localPos)), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(mpTabBar, &event);
    }

    // Presses whichever tab the bar's centre is over and holds there until the
    // reorder delay has passed, leaving the move that follows as the only thing
    // that decides whether the tab detaches
    int pressTheCentreTab()
    {
        const QPoint barCentre = mpTabBar->rect().center();
        const int pressedTab = mpTabBar->tabAt(barCentre);
        if (pressedTab < 0) {
            QTest::qFail("the centre of the tab bar is not over a tab, so a press there starts no drag at all", __FILE__, __LINE__);
            return -1;
        }
        pressAt(barCentre);
        return pressedTab;
    }

    // Presses one named point, rather than the centre of the bar, and holds
    // there until the reorder delay has passed
    void pressAt(const QPoint& position)
    {
        sendMouse(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, position);
        QTest::qWait(mReorderDelayMs + 50);
    }

    void moveTo(const QPoint& position) { sendMouse(QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, position); }

    QPoint dragTarget(int distance) const { return mpTabBar->rect().center() + QPoint(0, distance); }

    // What mouseMoveEvent() used to compare against the threshold
    int distanceFromBarCentre(const QPoint& target) const { return (target - mpTabBar->rect().center()).manhattanLength(); }

    // Straight down from the press, which was the bar's centre: with no
    // horizontal component the move is as vertical as the ratio gate can ask
    // for, and the distance measured from that centre is exactly the one asked
    // for here
    void dragTo(int distance)
    {
        const QPoint target = dragTarget(distance);
        // One of the gates ahead of the distance comparison, and then the
        // distance that comparison will see, so that a tab staying put cannot
        // quietly be down to either of those instead of to the threshold
        QVERIFY2(!mpTabBar->rect().contains(target), "the drag stayed inside the tab bar, which stops mouseMoveEvent() before it reaches the distance");
        QCOMPARE((target - mpTabBar->rect().center()).manhattanLength(), distance);

        moveTo(target);
    }
};

#include "TabDetachThresholdTest.moc"
MUDLET_GROUPED_TEST_MAIN(TabDetachThresholdTest)
