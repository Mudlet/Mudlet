/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Developers - mudlet@mudlet.org           *
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

#ifndef MUDLET_TMAPVIEW_H
#define MUDLET_TMAPVIEW_H

#include <QComboBox>
#include <QPointer>
#include <QToolButton>
#include <QWidget>

class Host;
class T2DMap;
class TMap;

/**
 * A secondary map view widget that displays the map independently from the primary mapper.
 * Unlike the primary mapper (dlgMapper), secondary views maintain their own view position
 * independent of the player's current room, but do allow navigation (area selection,
 * z-level changes, zoom).
 */
class TMapView : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TMapView)
    TMapView(int viewId, Host* pHost, TMap* pMap, QWidget* parent = nullptr);
    ~TMapView() override;

    int getViewId() const { return mViewId; }
    T2DMap* get2DMap() { return mp2dMap; }

    void setArea(int areaId);
    std::pair<bool, QString> centerOnRoom(int roomId);
    std::pair<bool, QString> setZoom(qreal zoom);

    int getCurrentAreaId() const;
    int getCenteredRoomId() const;
    qreal getZoom() const;
    int getZLevel() const;

    void updateAreaComboBox();

private slots:
    void slot_switchArea(int index);

private:
    void setupUi();

    const int mViewId;
    QPointer<Host> mpHost;
    QPointer<TMap> mpMap;
    T2DMap* mp2dMap = nullptr;

    QComboBox* mpAreaComboBox = nullptr;
    QToolButton* mpZUpButton = nullptr;
    QToolButton* mpZDownButton = nullptr;
    QWidget* mpPanelWidget = nullptr;
    QToolButton* mpTogglePanelButton = nullptr;
};

#endif // MUDLET_TMAPVIEW_H
