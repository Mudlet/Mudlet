#ifndef MUDLET_DLGMAPPER_H
#define MUDLET_DLGMAPPER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "ui_mapper.h"
#include <QDir>
#include <QMainWindow>
#include <QPointer>
#include "post_guard.h"


class Host;
class TMap;
#if defined(INCLUDE_3DMAPPER)
class GLWidget;
#endif


class dlgMapper : public QWidget, public Ui::mapper
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgMapper)
    dlgMapper(QWidget*, Host*, TMap*);
#if defined(INCLUDE_3DMAPPER)
    GLWidget* glWidget;
#endif
    void updateAreaComboBox();
    void setDefaultAreaShown(bool);
    bool getDefaultAreaShown() { return mShowDefaultArea; }
    void resetAreaComboBoxToPlayerRoomArea();

public slots:
    void slot_bubbles();
    void slot_info();
    void slot_toggleShowRoomIDs(int s);
    void slot_toggleStrongHighlight(int v);
    void show2dView();
    void slot_togglePanel();
    void goRoom();
    void choseRoom(QListWidgetItem*);
    void slot_roomSize(int d);
    void slot_lineSize(int d);

private:
    TMap* mpMap;
    QPointer<Host> mpHost;
    bool mShowDefaultArea;
};

#endif // MUDLET_DLGMAPPER_H
