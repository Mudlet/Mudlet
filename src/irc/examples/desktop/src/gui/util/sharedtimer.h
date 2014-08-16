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

#ifndef SHAREDTIMER_H
#define SHAREDTIMER_H

#include <QObject>
#include <QBasicTimer>
#include <QPair>

class SharedTimer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int interval READ interval WRITE setInterval)

public:
    static SharedTimer* instance();

    int interval() const;
    void setInterval(int interval);

    void registerReceiver(QObject* receiver, const char* member);
    void unregisterReceiver(QObject* receiver, const char* member = 0);

    void pause();
    void resume();

protected:
    void timerEvent(QTimerEvent* event);

private slots:
    void destroyed(QObject* object);

private:
    SharedTimer(QObject* parent = 0);

    typedef QPair<QObject*, const char*> Receiver;
    struct SharedTimerData
    {
        int interval;
        QBasicTimer timer;
        QList<Receiver> receivers;
    } d;
};

#endif // SHAREDTIMER_H
