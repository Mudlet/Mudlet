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
 * reorder delay, a predominantly vertical move, leaving the tab bar's rectangle
 * - and only then the distance from the tab bar's centre against
 * DETACH_DISTANCE_THRESHOLD. Every drag here is straight down from that centre,
 * so all the gates ahead of the distance are satisfied the same way each time
 * and the distance is the only thing that can differ between a detach and no
 * detach.
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
    // DETACH_DISTANCE_THRESHOLD itself. The comparison is >, so a drag reaching
    // exactly this far is still a short one
    static constexpr int mThresholdDistance = 80;
    static constexpr int mPastThresholdDistance = 90;

private slots:
    void init()
    {
        mpTabBar = new TTabBar(nullptr);
        // The two things mudlet.cpp sets that change how QTabBar handles a press
        // and a drag of its own accord, since that handling runs either side of
        // the detach check
        mpTabBar->setMovable(true);
        mpTabBar->setTabsClosable(true);
        // Three tabs of similarly sized names, so the centre of the bar - which
        // is what the detach distance is measured from - lands inside a tab
        // rather than on the seam between two of them
        for (const QString& profileName : {qsl("Alpha"), qsl("Bravo"), qsl("Delta")}) {
            mpTabBar->setTabData(mpTabBar->addTab(profileName), profileName);
        }
        // sizeHint() is the union of the tab rectangles, so at that size the tabs
        // cover the whole widget and its centre is over one of them
        mpTabBar->resize(mpTabBar->sizeHint());
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
        sendMouse(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, barCentre);
        QTest::qWait(mReorderDelayMs + 50);
        return pressedTab;
    }

    QPoint dragTarget(int distance) const { return mpTabBar->rect().center() + QPoint(0, distance); }

    // Straight down from the press, which was the bar's centre: with no
    // horizontal component the move is as vertical as the ratio gate can ask
    // for, and the distance measured from that centre is exactly the one asked
    // for here
    void dragTo(int distance)
    {
        const QPoint target = dragTarget(distance);
        // The last gate before the distance comparison, and then the distance
        // that comparison will see, so that a tab staying put cannot quietly be
        // down to either of those instead of to the threshold
        QVERIFY2(!mpTabBar->rect().contains(target), "the drag stayed inside the tab bar, which stops mouseMoveEvent() before it reaches the distance");
        const QRect barGlobalRect(mpTabBar->mapToGlobal(mpTabBar->rect().topLeft()), mpTabBar->rect().size());
        QCOMPARE((mpTabBar->mapToGlobal(target) - barGlobalRect.center()).manhattanLength(), distance);

        sendMouse(QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, target);
    }
};

#include "TabDetachThresholdTest.moc"
MUDLET_GROUPED_TEST_MAIN(TabDetachThresholdTest)
