#ifndef MUDLET_EXITSTREEWIDGET_H
#define MUDLET_EXITSTREEWIDGET_H

/***************************************************************************
 *   Copyright (C) 2012 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021-2022 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "pre_guard.h"
#include <QTreeWidget>
#include "post_guard.h"


class ExitsTreeWidget : public QTreeWidget
{
    Q_OBJECT

    friend class RoomIdLineEditDelegate;
    friend class dlgRoomExits;

    // The indexes that are used to identify the columns in the special exits
    // treewidget have been converted to constants so that we can
    // tweak them and change all of them correctly - and by making the
    // dlgRoomExits class a friend that can use the same set as defined here.
    // Note that if any of these numbers are modified/extended the
    // corresponding headings in the ./src/ui/room_exits.ui file will need
    // to be adjusted as well - and visa versa:
    static const int colIndex_exitRoomId = 0;
    static const int colIndex_exitStatus = 1;
    static const int colIndex_lockExit = 2;
    static const int colIndex_exitWeight = 3;
    static const int colIndex_doorNone = 4;
    static const int colIndex_doorOpen = 5;
    static const int colIndex_doorClosed = 6;
    static const int colIndex_doorLocked = 7;
    static const int colIndex_command = 8;

public:
    Q_DISABLE_COPY(ExitsTreeWidget)
    explicit ExitsTreeWidget(QWidget* pW);
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // MUDLET_EXITSTREEWIDGET_H
