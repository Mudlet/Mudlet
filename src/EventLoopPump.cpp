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

// A nested QEventLoop::exec() cannot be used here. exec() sets
// QEventLoop::EventLoopExec, which QCocoaEventDispatcher answers by re-entering
// -[NSApplication run] and leaving Qt's timers to the platform run loop; nested
// inside a Qt timer callback that run loop never wakes it again, so not even
// the wait's own timeout fires (issue #9670). processEvents() instead takes the
// branch that drives the dispatcher's processTimers() on every pass.
bool EventLoopPump::pumpFor(const int timeoutMs, const std::function<bool()>& stopCondition)
{
    // processEvents() returns silently without a dispatcher, which would make
    // this a plain sleep that then reports a timeout as though it had waited.
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
        // A pass returns as soon as nothing is pending, so without this the loop
        // spins a core flat for the whole timeout.
        QThread::msleep(1);
    }
}
