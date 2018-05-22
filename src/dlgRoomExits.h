#ifndef MUDLET_DLGROOMEXITS_H
#define MUDLET_DLGROOMEXITS_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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
    // Default constructor - we need this for the "empty" value() case in dlgRoomExits::originalExits
    TExit()
    : mHasNoRoute(false)
    , mHasStub(false)
    , mDestination(0)
    , mDoor(0)
    , mWeight(0)
    {}

    TExit(const QString& exitText, const bool hasExitStub, const bool hasNoRoute, const int weight, const int door)
    : mHasNoRoute(hasNoRoute)
    , mHasStub(hasExitStub)
    , mDoor(door)
    , mWeight(weight)
    {
        bool isOk;
        if (exitText.toInt(&isOk) && isOk) {
            // Exit destination is convertable to a non-zero integer
            mDestination = exitText.toInt();
            mHasStub = false;
        } else if (hasExitStub) {
            // Exit is a stub
            mHasNoRoute = false;
            mDestination = 0;
            mHasStub = true;
            mWeight = 0;
        } else {
            // Non-existant
            mHasNoRoute = false;
            mHasStub = false;
            mDestination = 0;
            mDoor = 0;
            mWeight = 0;
        }
    }

    friend bool operator==( TExit &a, TExit &b )
    {
        return a.mDestination == b.mDestination
                 && a.mDoor == b.mDoor
                 && a.mHasNoRoute == b.mHasNoRoute
                 && a.mHasStub == b.mHasStub
                 && a.mWeight == b.mWeight;
    }

    friend bool operator!=( TExit &a, TExit &b )
    {
        return a.mDestination != b.mDestination
                 || a.mDoor != b.mDoor
                 || a.mHasNoRoute != b.mHasNoRoute
                 || a.mHasStub != b.mHasStub
                 || a.mWeight != b.mWeight;
    }

    bool mHasNoRoute;
    bool mHasStub;
    int mDestination;
    int mDoor;
    int mWeight;
};

class dlgRoomExits : public QDialog, public Ui::room_exits
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgRoomExits)
    explicit dlgRoomExits(Host*, int, QWidget* parent = 0);
    ~dlgRoomExits();


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

private slots:
    void slot_checkModified();

private:
    QPointer<Host> mpHost;
    QTreeWidgetItem* mpEditItem;
    TRoom* pR;
    int mRoomID;
    int mEditColumn;

    // A dummy constructed NULL exit, the address of which is to be given as a
    // second argument and returned by the use of QMap<T1,T2>::value() on the
    // two QMaps following:
    TExit* mpNullExit;
    // key = (normal) exit DIR_***, value = exit class instance
    QMap<int, TExit*> originalExits;
    // key = (special exit name/command), value = exit class instance
    QMap<QString, TExit*> originalSpecialExits;

    void initExit(int direction,
                  int exitId,
                  QLineEdit* exitLineEdit,
                  QCheckBox* noRoute,
                  QCheckBox* stub,
                  QRadioButton* none,
                  QRadioButton* open,
                  QRadioButton* closed,
                  QRadioButton* locked,
                  QSpinBox* weight);
    TExit* makeExitFromControls(int direction);
};

#endif // MUDLET_DLGROOMEXITS_H
