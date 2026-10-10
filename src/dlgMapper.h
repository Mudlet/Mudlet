#ifndef MUDLET_DLGMAPPER_H
#define MUDLET_DLGMAPPER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2021-2022, 2026 by Stephen Lyons                  *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2026 by Ethan Hussong - ethan@ethanhussong.com          *
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


#include "TMapViewFrontend.h"
#include "ui_mapper.h"
#include <QPointer>

class Host;
class TMap;
class QFrame;
class QLabel;
class QProgressBar;
class QPushButton;
struct MapInfoProperties;
#if defined(INCLUDE_3DMAPPER)
#include "glwidget_integration.h"
class QOpenGLWidget;
#endif


class dlgMapper : public QWidget, public Ui::mapper, public TMapViewFrontend
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgMapper)
    dlgMapper(QWidget*, Host*, TMap*);
#if defined(INCLUDE_3DMAPPER)
    QOpenGLWidget* glWidget = nullptr;
#endif
    void updateAreaComboBox();
    void resetAreaComboBoxToPlayerRoomArea();
    // The member variable is the source for this bit of information:
    bool isIn3DMode() const { return mIs3DMode; }
    bool isFloatAndDockable() const;
    int getCurrentShownAreaIndex();
    void setFont(const QFont&);
    void refreshColours();
    void recreate3DWidget();

    bool onScreen() const override { return isVisible(); }
    void showMapProgress(const QString& label, bool cancelable) override;
    void setMapProgressLabel(const QString& text) override;
    void setMapProgressRange(int minimum, int maximum) override;
    void setMapProgressValue(int value) override;
    int mapProgressMaximum() const override;
    void setMapProgressCancelable(bool cancelable) override;
    void hideMapProgress() override;
    bool isMapProgressVisible() const override;
    void show3DView(bool shown) override { slot_toggle3DView(shown); }
    bool showing3DView() const override;
    void recreate3DView() override { recreate3DWidget(); }
    bool shift3DViewCamera(float verticalAngle, float horizontalAngle, float rotationAngle) override;
    bool set3DViewCameraPosition(float r, float theta, float phi) override;
    bool selectingRooms() const override { return mp2dMap->mMultiSelection; }
    QSet<int> selectedRooms() const override { return mp2dMap->mMultiSelectionSet; }
    int centerSelectedRoom() const override { return mp2dMap->getCenterSelectedRoomId(); }
    void clearRoomSelection() override { mp2dMap->clearSelection(); }
    int shownAreaId() const override { return mp2dMap->getAreaId(); }
    std::pair<bool, QString> setMapZoom(qreal zoom, int areaId) override { return mp2dMap->setMapZoom(zoom, areaId); }
    std::pair<bool, QString> exportAreaToImage(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels) override
    {
        return mp2dMap->exportAreaToImage(areaId, filePath, zLevel, zoom, exportAllZLevels);
    }

signals:
    void signal_mapProgressCanceled();

public slots:
    void updateEmptyStateOverlay();
    void slot_toggleRoundRooms(const bool);
    void slot_toggleShowRoomIDs(int toggle);
    void slot_toggleStrongHighlight(int toggle);
    void slot_toggle3DView(const bool);
    void slot_togglePanel();
    void slot_setMapperPanelVisible(bool panelVisible);
    void slot_roomSize(int size);
    void slot_setShowRoomIds(bool showRoomIds);
    void slot_setShowGrid(bool showGrid);
    void slot_updateInfoContributors();
    void slot_switchArea(const int);
    void slot_setupMapperMenu();
    void slot_toggleUpperLowerLevels(bool enabled);
    void slot_toggleShowRoomIDsFromMenu(bool enabled);
    void slot_toggleShowRoomNames(const bool);
    void updateInfoMenu();
    void slot_showSaveWarningMenu();
    void slot_saveErrorChanged(bool hasError);

    static void paintMapInfo(const QElapsedTimer& renderTimer,
                             QPainter& painter,
                             Host* pHost,
                             TMap* pMap,
                             int roomID,
                             int displayAreaId,
                             int selectionSize,
                             QColor& infoColor,
                             int xOffset,
                             int yOffset,
                             int widgetWidth,
                             int fontHeight);
    static int paintMapInfoContributor(QPainter& painter, int xOffset, int yOffset, const MapInfoProperties& properties, QColor bgColor, int fontHeight, int widgetWidth);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // Only the mapper TMap::mpMapper names acts on the map's cues, as other
    // mappers of the same profile (in a detached window, say) are not drawing it.
    bool drawsTheMap() const;
    void connectMapCues();
    void setupEmptyStateOverlay();
    void repositionEmptyStateOverlay();
    void setupProgressOverlay();
    void repositionProgressOverlay();
    void loadMapFromFile();
#if defined(INCLUDE_3DMAPPER)
    void connect3DViewControls();
    void sync3DViewFrom2D();
    void sync2DViewFrom3D();
#endif

    TMap* mpMap = nullptr;
    QPointer<Host> mpHost;
    QPointer<QMenu> mpInfoMenu;
    QFrame* mpEmptyStateOverlay = nullptr;
    QPushButton* mpEmptyStateDownloadButton = nullptr;
    QFrame* mpProgressOverlay = nullptr;
    QLabel* mpProgressLabel = nullptr;
    QProgressBar* mpProgressBar = nullptr;
    QPushButton* mpProgressCancelButton = nullptr;
    bool mEmptyStateDismissed = false;
    bool mIs3DMode = false;
};

#endif // MUDLET_DLGMAPPER_H
