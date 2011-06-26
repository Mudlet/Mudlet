/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#ifndef DLGROOMEXITS_H
#define DLGROOMEXITS_H

#include <QDialog>
#include "ui_room_exits.h"
#include "Host.h"

class Host;

class dlgRoomExits : public QDialog, public Ui::roomExits
{
    Q_OBJECT
public:
    explicit dlgRoomExits(Host *, QWidget *parent = 0);
    void init( int );
    Host * mpHost;
    int mRoomID;
    int mEditColumn;
    QTreeWidgetItem * mpEditItem;

signals:

public slots:
    void slot_editItem(QTreeWidgetItem * pI, int column );
    void save();
    void slot_addSpecialExit();
    //void cancel();

};

#endif // DLGROOMEXITS_H
