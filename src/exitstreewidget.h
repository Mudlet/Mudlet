#ifndef MUDLET_EXITSTREEWIDGET_H
#define MUDLET_EXITSTREEWIDGET_H

/***************************************************************************
 *   Copyright (C) 2012 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021 by Stephen Lyons - slysven@virginmedia.com         *
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

public:
    Q_DISABLE_COPY(ExitsTreeWidget)
    ExitsTreeWidget(QWidget* pW);
    void keyPressEvent(QKeyEvent* event) override;

protected:
    // The indexes that are used to identify the columns in the special exits
    // treewidget have been converted to constants so that we can
    // tweak them and change all of them correctly.
    // I'd like to find a way to only define them once but some are also needed
    // in the dlgRoomExit class as that also needs to access them but I can
    // not see how to do it... SlySven Oct 2021
    static const int colIndex_exitRoomId = 0;
    static const int colIndex_lockExit = 1;
    static const int colIndex_exitWeight = 2;
    static const int colIndex_doorNone = 3;
    static const int colIndex_doorOpen = 4;
    static const int colIndex_doorClosed = 5;
    static const int colIndex_doorLocked = 6;
    static const int colIndex_command = 7;
};

#endif // MUDLET_EXITSTREEWIDGET_H
