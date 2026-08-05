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

#include <QtTest/QtTest>

#include <QElapsedTimer>
#include <QTimer>

#include "EventLoopPump.h"

/*
 * EventLoopPump exists because a nested QEventLoop::exec() entered from inside a
 * Qt timer callback stops seeing Qt timers on macOS, so a wait armed with a
 * QTimer never ends (issue #9670). The case that matters most here is therefore
 * pumpingFromInsideATimerCallback(): on the platform the bug lives on, running a
 * nested exec() in that position is what wedges, and these run on the macOS CI
 * legs.
 */
class EventLoopPumpTest : public QObject
{
    Q_OBJECT

private slots:
    void runsOutTheClockWithNoCondition();
    void makesOnePassForAZeroTimeout();
    void stopsAsSoonAsTheConditionHolds();
    void stopsWithoutPumpingWhenTheConditionAlreadyHolds();
    void deliversATimerThatComesDueWhilePumping();
    void pumpingFromInsideATimerCallback();

private:
    bool mDelivered = false;
};

void EventLoopPumpTest::runsOutTheClockWithNoCondition()
{
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(!EventLoopPump::pumpFor(120));
    QVERIFY2(elapsed.elapsed() >= 110, qPrintable(QString::number(elapsed.elapsed())));
}

void EventLoopPumpTest::makesOnePassForAZeroTimeout()
{
    bool delivered = false;
    QMetaObject::invokeMethod(
            this,
            [&delivered]() {
                delivered = true;
            },
            Qt::QueuedConnection);

    QVERIFY(!EventLoopPump::pumpFor(0));
    QVERIFY2(delivered, "a zero timeout did not deliver the already-posted event");
}

void EventLoopPumpTest::stopsAsSoonAsTheConditionHolds()
{
    bool done = false;
    QTimer::singleShot(50, this, [&done]() {
        done = true;
    });

    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(EventLoopPump::pumpFor(5000, [&done]() {
        return done;
    }));
    QVERIFY2(elapsed.elapsed() < 2000, qPrintable(QString::number(elapsed.elapsed())));
}

void EventLoopPumpTest::stopsWithoutPumpingWhenTheConditionAlreadyHolds()
{
    mDelivered = false;
    QMetaObject::invokeMethod(
            this,
            [this]() {
                mDelivered = true;
            },
            Qt::QueuedConnection);

    QVERIFY(EventLoopPump::pumpFor(1000, []() {
        return true;
    }));
    QVERIFY2(!mDelivered, "a condition that already held should not have pumped anything");

    // The event is still queued rather than lost, and draining it here keeps it
    // from turning up in the middle of the next test.
    QVERIFY(!EventLoopPump::pumpFor(20));
    QVERIFY(mDelivered);
}

void EventLoopPumpTest::deliversATimerThatComesDueWhilePumping()
{
    bool fired = false;
    QTimer::singleShot(40, this, [&fired]() {
        fired = true;
    });

    QVERIFY(!EventLoopPump::pumpFor(300));
    QVERIFY2(fired, "a timer that came due during the pump did not fire");
}

void EventLoopPumpTest::pumpingFromInsideATimerCallback()
{
    // The #9670 shape. If Qt timers stop being delivered once the pump is
    // entered from a timer callback, the inner timer never fires and the outer
    // callback never finishes.
    bool innerFired = false;
    bool firedDuringPump = false;
    bool outerDone = false;

    QTimer::singleShot(0, this, [&]() {
        QTimer::singleShot(40, this, [&innerFired]() {
            innerFired = true;
        });
        QVERIFY(!EventLoopPump::pumpFor(300));
        firedDuringPump = innerFired;
        outerDone = true;
    });

    QVERIFY(EventLoopPump::pumpFor(5000, [&outerDone]() {
        return outerDone;
    }));
    QVERIFY2(firedDuringPump, "a timer did not fire while pumping from inside a timer callback");
}

QTEST_MAIN(EventLoopPumpTest)
#include "EventLoopPumpTest.moc"
