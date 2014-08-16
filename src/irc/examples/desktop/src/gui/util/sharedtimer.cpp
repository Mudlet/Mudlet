/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "sharedtimer.h"
#include <QTimerEvent>

SharedTimer::SharedTimer(QObject* parent) : QObject(parent)
{
    d.interval = 500;
}

SharedTimer* SharedTimer::instance()
{
    static SharedTimer timer;
    return &timer;
}

int SharedTimer::interval() const
{
    return d.interval;
}

void SharedTimer::setInterval(int interval)
{
    if (d.timer.isActive())
        qWarning("SharedTimer::setInterval(): timer active");
    d.interval = interval;
}

void SharedTimer::registerReceiver(QObject* receiver, const char* member)
{
    if (!receiver || !member)
        return;

    Receiver pair = qMakePair(receiver, member);
    if (!d.receivers.contains(pair))
    {
        if (d.receivers.isEmpty())
            d.timer.start(d.interval, this);

        d.receivers.append(pair);
        connect(receiver, SIGNAL(destroyed(QObject*)), this, SLOT(destroyed(QObject*)));
    }
}

void SharedTimer::unregisterReceiver(QObject* receiver, const char* member)
{
    if (!receiver)
        return;

    Receiver pair = qMakePair(receiver, member);
    if (!member)
    {
        QMutableListIterator<Receiver> it(d.receivers);
        while (it.hasNext())
        {
            if (it.next().first == receiver)
            {
                disconnect(receiver, SIGNAL(destroyed(QObject*)), this, SLOT(destroyed(QObject*)));
                it.remove();
            }
        }
    }
    else if (d.receivers.contains(pair))
    {
        disconnect(receiver, SIGNAL(destroyed(QObject*)), this, SLOT(destroyed(QObject*)));
        d.receivers.removeAll(pair);
    }

    if (d.receivers.isEmpty())
        d.timer.stop();
}

void SharedTimer::pause()
{
    if (d.timer.isActive())
        d.timer.stop();
}

void SharedTimer::resume()
{
    if (!d.receivers.isEmpty() && !d.timer.isActive())
        d.timer.start(d.interval, this);
}

void SharedTimer::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == d.timer.timerId())
    {
        foreach (Receiver receiver, d.receivers)
            QMetaObject::invokeMethod(receiver.first, receiver.second);
    }
}

void SharedTimer::destroyed(QObject* object)
{
    unregisterReceiver(object);
}
