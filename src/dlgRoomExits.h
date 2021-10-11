#ifndef MUDLET_DLGROOMEXITS_H
#define MUDLET_DLGROOMEXITS_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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
#include "ui_room_exits.h"
#include <QCheckBox>
#include <QDialog>
#include <QPointer>
#include "post_guard.h"

class Host;
class TRoom;

class TExit
{
public:
    bool hasNoRoute = false;
    bool hasStub = false;
    int destination = 0;
    int door = 0;
    int weight = 0;

    friend bool operator==( TExit &a, TExit &b )
    {
        return a.destination == b.destination
                 && a.door == b.door
                 && a.hasNoRoute == b.hasNoRoute
                 && a.hasStub == b.hasStub
                 && a.weight == b.weight ;
    }

    friend bool operator!=( TExit &a, TExit &b )
    {
        return a.destination != b.destination
                 || a.door != b.door
                 || a.hasNoRoute != b.hasNoRoute
                 || a.hasStub != b.hasStub
                 || a.weight != b.weight ;
    }
};

class dlgRoomExits : public QDialog, public Ui::room_exits
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgRoomExits)
    explicit dlgRoomExits(Host*, const int, QWidget* parent = nullptr);

public slots:
    void save();
    void slot_addSpecialExit();
    void slot_editSpecialExit(QTreeWidgetItem*, int);
    void slot_endEditSpecialExits();
    void slot_ne_textEdited(const QString&);
    void slot_n_textEdited(const QString&);
    void slot_nw_textEdited(const QString&);
    void slot_up_textEdited(const QString&);
    void slot_w_textEdited(const QString&);
    void slot_e_textEdited(const QString&);
    void slot_down_textEdited(const QString&);
    void slot_sw_textEdited(const QString&);
    void slot_s_textEdited(const QString&);
    void slot_se_textEdited(const QString&);
    void slot_in_textEdited(const QString&);
    void slot_out_textEdited(const QString&);
    void slot_stub_ne_stateChanged(int);
    void slot_stub_n_stateChanged(int);
    void slot_stub_nw_stateChanged(int);
    void slot_stub_up_stateChanged(int);
    void slot_stub_w_stateChanged(int);
    void slot_stub_e_stateChanged(int);
    void slot_stub_down_stateChanged(int);
    void slot_stub_sw_stateChanged(int);
    void slot_stub_s_stateChanged(int);
    void slot_stub_se_stateChanged(int);
    void slot_stub_in_stateChanged(int);
    void slot_stub_out_stateChanged(int);

protected:
    // The indexes that are used to identify the columns in the special exits
    // treewidget have been converted to constants so that we can
    // tweak them and change all of them correctly.
    // I'd like to find a way to only define them once but some are also needed
    // in the exitstreewidget class as that also needs to access them but I can
    // not see how to do it...
    // Also, it has not proven possible to move the definitions of the columns
    // in the special exitstreewidget into this class so if the details of the
    // columns change it is also needed to revise the ./src/ui/room_exits.ui
    // file - SlySven Oct 2021
    static const int colIndex_exitRoomId = 0;
    static const int colIndex_lockExit = 1;
    static const int colIndex_exitWeight = 2;
    static const int colIndex_doorNone = 3;
    static const int colIndex_doorOpen = 4;
    static const int colIndex_doorClosed = 5;
    static const int colIndex_doorLocked = 6;
    static const int colIndex_command = 7;

private slots:
    void slot_checkModified();

private:
    void init();
    void initExit(int direction, int exitId, QLineEdit* exitLineEdit,
                  QCheckBox* noRoute, QCheckBox* stub,
                  QRadioButton* none, QRadioButton* open, QRadioButton* closed, QRadioButton* locked,
                  QSpinBox* weight);
    TExit* makeExitFromControls(int direction);

    QPointer<Host> mpHost;
    QTreeWidgetItem* mpEditItem = nullptr;
    TRoom* pR = nullptr;
    int mRoomID = 0;
    int mEditColumn = -1;

    // key = (normal) exit DIR_***, value = exit class instance
    QMap<int, TExit*> originalExits;
    QMap<QString, TExit*> originalSpecialExits;
};

#endif // MUDLET_DLGROOMEXITS_H
