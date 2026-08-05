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

#include "EventLoopPump.h"

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDebug>
#include <QEventLoop>
#include <QThread>

// Why this pumps rather than running a nested QEventLoop::exec():
//
// On macOS a nested exec() entered from inside a Qt timer callback stops seeing
// Qt timers, so a wait armed with a QTimer never ends. Stacks captured from a
// wedged CI run (issue #9670) show the main thread in
// QTimerInfoList::activateTimers() -> a Qt timer's slot -> a nested
// QEventLoop::exec(), which QCocoaEventDispatcher services by re-entering
// -[NSApplication run] from inside the run loop callout the outer loop is
// already in. That nested [NSApp run] sat in
// _BlockUntilNextEventMatchingListInModeWithFilter for every one of 3314
// samples, using 0.02s of CPU in 86 seconds: no Qt timer fired again, including
// the single-shot one whose timeout was the only way out.
//
// The difference is that QEventLoop::exec() asks the event dispatcher to
// process events with QEventLoop::EventLoopExec set, and QCocoaEventDispatcher
// answers that by handing control to AppKit and relying on the platform run
// loop to wake it for the single CFRunLoopTimer that drives every Qt timer.
// QCoreApplication::processEvents() runs the dispatcher without EventLoopExec,
// and that branch calls the dispatcher's own processTimers() - i.e.
// QTimerInfoList::activateTimers() - directly on each pass, so Qt timers are
// serviced whatever the platform run loop is or is not doing.
bool EventLoopPump::pumpFor(const int timeoutMs, const std::function<bool()>& stopCondition)
{
    // Without a dispatcher QCoreApplication::processEvents() returns silently,
    // which would turn this into a plain sleep that reports a timeout as though
    // it had been waiting for something.
    if (!QThread::currentThread()->eventDispatcher()) {
        qWarning() << "EventLoopPump::pumpFor() called with no event dispatcher on this thread, no events can be delivered";
        return false;
    }

    QDeadlineTimer deadline(qMax(timeoutMs, 0));
    if (stopCondition && stopCondition()) {
        return true;
    }

    while (true) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if (stopCondition && stopCondition()) {
            return true;
        }
        if (deadline.hasExpired()) {
            return false;
        }
        // A pass returns as soon as nothing more is pending, so without a pause
        // this would spin a core flat for the whole timeout.
        QThread::msleep(1);
    }
}
