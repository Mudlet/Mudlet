/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTIMERINFO_UNIX_P_H
#define QTIMERINFO_UNIX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

// #define QTIMERINFO_DEBUG

#include "qabstracteventdispatcher.h"

#include <sys/time.h> // struct timeval

QT_BEGIN_NAMESPACE

// internal timer info
struct QTimerInfo {
    int id;           // - timer identifier
    int interval;     // - timer interval in milliseconds
    Qt::TimerType timerType; // - timer type
    timespec timeout;  // - when to actually fire
    QObject *obj;     // - object to receive event
    QTimerInfo **activateRef; // - ref from activateTimers

#ifdef QTIMERINFO_DEBUG
    timeval expected; // when timer is expected to fire
    float cumulativeError;
    uint count;
#endif
};

class Q_CORE_EXPORT QTimerInfoList : public QList<QTimerInfo*>
{
#if ((_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(Q_OS_MAC)) || defined(QT_BOOTSTRAPPED)
    timespec previousTime;
    clock_t previousTicks;
    int ticksPerSecond;
    int msPerTick;

    bool timeChanged(timespec *delta);
    void timerRepair(const timespec &);
#endif

    // state variables used by activateTimers()
    QTimerInfo *firstTimerInfo;

public:
    QTimerInfoList();

    timespec currentTime;
    timespec updateCurrentTime();

    // must call updateCurrentTime() first!
    void repairTimersIfNeeded();

    bool timerWait(timespec &);
    void timerInsert(QTimerInfo *);

    int timerRemainingTime(int timerId);

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject *object) const;

    int activateTimers();
};

QT_END_NAMESPACE

#endif // QTIMERINFO_UNIX_P_H
