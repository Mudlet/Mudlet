#ifndef MUDLET_DLGMAPPER_H
#define MUDLET_DLGMAPPER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2021-2022 by Stephen Lyons                        *
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
    GLWidget* glWidget = nullptr;
#endif
    void updateAreaComboBox();
    void setDefaultAreaShown(bool);
    bool getDefaultAreaShown() { return mShowDefaultArea; }
    void resetAreaComboBoxToPlayerRoomArea();
    // The button is the goto source for this bit of information:
    bool isIn3DMode() const { return pushButton_3D->isDown(); }
    bool isFloatAndDockable() const;

public slots:
    void slot_toggleRoundRooms(const bool);
    void slot_toggleShowRoomIDs(int s);
    void slot_toggleShowRoomNames(int s);
    void slot_toggleStrongHighlight(int v);
    void slot_toggle3DView(const bool);
    void slot_togglePanel();
    void slot_setMapperPanelVisible(bool panelVisible);
    void slot_roomSize(int d);
    void slot_exitSize(int d);
    void slot_setRoomSize(int d);
    void slot_setExitSize(int d);
    void slot_setShowRoomIds(bool showRoomIds);
    void slot_updateInfoContributors();
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
    // Only used in newer Qt versions
    void slot_switchArea(const int);
#endif

private:
    TMap* mpMap = nullptr;
    QPointer<Host> mpHost;
    bool mShowDefaultArea = true;
};

#endif // MUDLET_DLGMAPPER_H
