#ifndef MUDLET_DLGROOMEXITS_H
#define MUDLET_DLGROOMEXITS_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021, 2024-2025 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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
#include <QSet>
#include <QStyledItemDelegate>
#include "post_guard.h"

class QAction;
class Host;
class TRoom;

// We need to forward reference the main class declared further down so these
// classes can refer to it:
class dlgRoomExits;

class WeightSpinBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit WeightSpinBoxDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

class RoomIdLineEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit RoomIdLineEditDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // We need to keep a pointer to the QLineEditor so we can tweak it whilst
    // text is being entered into it - as the methods we are overriding are
    // are all marked const we need to mark our additions as mutable so we can
    // modify them in use:
    mutable QPointer<QLineEdit> mpEditor;
    mutable dlgRoomExits* mpDlgRoomExits = nullptr;
    mutable QTreeWidgetItem* mpItem = nullptr;

    // We also need to access some external (to this class) things which we will
    // source by looking up our chain of ancestors to the dlgRoomExits instance:
    mutable QPointer<Host> mpHost;

    // The area ID of the room whose exits we are working on:
    mutable int mAreaID = 0;

private slots:
    void slot_specialRoomExitIdEdited(const QString&) const;
};

class TExit
{
public:
    bool hasNoRoute = false;
    int destination = 0; // -1 for no exit, 0 for stub, +ve for real room id
    int door = 0;
    int weight = 0;

    friend bool operator==( TExit &a, TExit &b )
    {
        return a.destination == b.destination
                 && a.door == b.door
                 && a.hasNoRoute == b.hasNoRoute
                 && a.weight == b.weight ;
    }

    friend bool operator!=( TExit &a, TExit &b )
    {
        return a.destination != b.destination
                 || a.door != b.door
                 || a.hasNoRoute != b.hasNoRoute
                 || a.weight != b.weight ;
    }
};

class dlgRoomExits : public QDialog, public Ui::room_exits
{
    Q_OBJECT
    friend class RoomIdLineEditDelegate;

public:
    Q_DISABLE_COPY(dlgRoomExits)
    explicit dlgRoomExits(Host*, const int, QWidget* parent = nullptr);
    ~dlgRoomExits();

    void setActionOnExit(QLineEdit*, QAction*) const;
    QAction* getActionOnExit(QLineEdit*) const;
    QPointer<Host> getHost() const { return mpHost; }
    int getAreaID() const { return mAreaID; }
    int convertNormelExitTextToNumber(const QString&) const;

    static bool textMatchesExitIdOrStub(const QString&);
    static bool textMatchesExitId(const QString&);
    static bool textMatchesStub(const QString&);

    QString mSpecialExitRoomIdPlaceholder;
    QString mSpecialExitCommandPlaceholder;
    QSet<QAction*> mAllExitActionsSet;
    QIcon mIcon_invalidExit;
    QIcon mIcon_inAreaExit;
    QIcon mIcon_otherAreaExit;
    QIcon mIcon_exitRoomLocked;
    QIcon mIcon_stub;
    QAction* mpAction_noExit = nullptr;
    QAction* mpAction_invalidExit = nullptr;
    QAction* mpAction_inAreaExit = nullptr;
    QAction* mpAction_otherAreaExit = nullptr;
    QAction* mpAction_exitRoomLocked = nullptr;
    QAction* mpAction_stub = nullptr;

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

private slots:
    void slot_checkModified();

private:
    static QString generateToolTip(const QString& exitRoomName, const QString& exitAreaName, const bool exitRoomLocked, const bool outOfAreaExit, const bool isStub, const int exitRoomWeight);
    void init();
    void initExit(int direction, int exitId, QLineEdit* exitLineEdit,
                  QCheckBox* noRoute,
                  QRadioButton* none, QRadioButton* open, QRadioButton* closed, QRadioButton* locked,
                  QSpinBox* weight, const QString &emptyExitToolTip);
    TExit* makeExitFromControls(int direction);
    void normalExitEdited(const QString& roomExitIdText,
                          QLineEdit* pExit,
                          QCheckBox* pNoRoute,
                          QSpinBox* pWeight,
                          QRadioButton* pDoorType_none,
                          QRadioButton* pDoorType_open,
                          QRadioButton* pDoorType_closed,
                          QRadioButton* pDoorType_locked,
                          const QString& invalidExitToolTipText,
                          const QString& noExitToolTipText);
    void setIconAndToolTipsOnSpecialExit(QTreeWidgetItem*, const bool);


    QPointer<Host> mpHost;
    QTreeWidgetItem* mpEditItem = nullptr;
    TRoom* pR = nullptr;
    int mRoomID = 0;
    int mAreaID = 0;
    int mEditColumn = -1;

    // key = (normal) exit DIR_***, value = exit class instance
    QMap<int, TExit*> originalExits;
    QMap<QString, TExit*> originalSpecialExits;
};

#endif // MUDLET_DLGROOMEXITS_H
