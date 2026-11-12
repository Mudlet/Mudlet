#ifndef MUDLET_TWINDOWNOTIFIER_H
#define MUDLET_TWINDOWNOTIFIER_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QObject>

// Tells, once a turn, that a call may have changed the windows the null view keeps for a profile's scripts.
// A listener compares what it reads then with what it read before, so a call that changed nothing tells nothing.
class TWindowNotifier : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    void note()
    {
        if (mPending) {
            return;
        }
        mPending = true;
        QMetaObject::invokeMethod(
                this,
                [this]() {
                    mPending = false;
                    emit windowsChanged();
                },
                Qt::QueuedConnection);
    }

signals:
    void windowsChanged();

private:
    bool mPending = false;
};

#endif // MUDLET_TWINDOWNOTIFIER_H
