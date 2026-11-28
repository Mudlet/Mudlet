#ifndef MUDLET_TNULLMAPVIEW_H
#define MUDLET_TNULLMAPVIEW_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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
#include "utils.h"

#include "TMapPainter.h"

#include <QObject>

class Host;
class QPainter;
class QPoint;
class QPointF;
class QSize;

// The 2D map of a map widget or embedded mapper that the null view records. It follows the player once the
// event loop runs, as T2DMap does as it paints, but its view moves only when the app draws it.
class TNullMapView final : public QObject, public TMapViewFrontend
{
public:
    // One that raisesEvents is a script's map; one that does not is an app's own, which scripts do not hear of: it
    // raises neither sysMapAreaChanged nor sysMapWindowMousePressEvent
    explicit TNullMapView(Host* pHost, QObject* parent = nullptr, bool raisesEvents = true);
    // A hidden mapper does not paint
    void setShown(bool shown);
    // What T2DMap would draw at that size, whether or not the mapper is shown, raising sysMapAreaChanged as it does
    void render(QPainter&, const QSize&);
    // What a mouse press, the second press of a double-click and a wheel turn at that point of the last render() do
    // to T2DMap, menus and dragging aside
    bool rendered() const { return !mView.mSize.isEmpty(); }
    QSize renderedSize() const { return mView.mSize; }
    // Whether the last render() laid out an area's rooms, which a press and a wheel turn act on
    bool showsArea() const;
    void press(const QPoint&, Qt::MouseButton, Qt::KeyboardModifiers);
    void doubleClick(const QPoint&);
    // False for less than half a notch, which T2DMap ignores
    bool wheel(const QPointF&, const QPoint& angleDelta, Qt::KeyboardModifiers);
    // Centres the next render() on the room and selects it, as a pan and a click on T2DMap do, until the player
    // moves or the map is centred again; false for a room that is not in the map
    bool showRoom(int roomId);
    // What a drag and a pinch on T2DMap do, for a touch screen: what was drawn moved by pan, in its pixels, then
    // zoomed by zoom (2 shows the rooms twice as large) about the point about, which stays where it is, within
    // [scmMinXYZoom, maxZoom()], as the area's zoom; the next render() lays it out. A move that leaves the view off
    // the player stops following, until the player moves to another room or follow(). False while the last render()
    // laid out no area
    bool move(const QPointF& pan, qreal zoom, const QPointF& about);
    // Centres the next render() on the player's room and follows the player again; false with no room to follow
    bool follow();
    // Whether the view follows the player: not from a move() or showRoom() that left it off the player, until the
    // player moves to another room or follow()
    bool following() const { return mFollowing; }
    // The most a move() zooms the area out to: half as much again as fits its widest side, and at least 30 rooms
    // across, or the zoom the area is at where that is more already
    qreal maxZoom() const;
    // A room the last render() drew, at its middle in the frame's pixels
    struct DrawnRoom
    {
        int id = 0;
        QPointF position;
        QString name;
    };
    // The rooms of the area and level the last render() laid out, at most maxDrawnRooms of them, those nearest the
    // middle where it drew more; none where it laid out no area
    static constexpr int maxDrawnRooms = 500;
    const QList<DrawnRoom>& drawnRooms() const { return mDrawnRooms; }
    // What the last render() laid out: the area, the level, the zoom (rooms across the narrow side) and a room's
    // cell in the frame's pixels
    int drawnAreaId() const { return mView.mAreaID; }
    int drawnLevel() const { return mView.mMapCenterZ; }
    qreal drawnZoom() const { return mView.xyzoom; }
    qreal drawnRoomSize() const { return mView.mRoomWidth; }
    // The room the map takes as the player's: paint() and TMapPainter take the first room where the player's is gone
    int playerRoomId() const;
    // As T2DMap::slot_toggleMapViewOnly(), which raises mapModeChangeEvent
    void setViewOnly(bool viewOnly);
    // As T2DMap::init() switches a new map to editing where the profile was saved so, which raises the event too:
    // called before the map is made, as a GUI handler of it finds no mapper yet
    static void announceEditing(Host*);
    // Sets the profile's mode and raises mapModeChangeEvent, with no map view needed
    static void switchMode(Host*, bool viewOnly);
    bool shown() const { return mShown; }

    bool onScreen() const override { return false; }
    void showMapProgress(const QString&, bool) override {}
    void setMapProgressLabel(const QString&) override {}
    void setMapProgressRange(int, int) override {}
    void setMapProgressValue(int) override {}
    int mapProgressMaximum() const override { return 0; }
    void setMapProgressCancelable(bool) override {}
    void hideMapProgress() override {}
    bool isMapProgressVisible() const override { return false; }
    // Only a user selects rooms
    bool selectingRooms() const override { return false; }
    QSet<int> selectedRooms() const override { return mView.mMultiSelectionSet; }
    int centerSelectedRoom() const override { return mView.mMultiSelectionHighlightRoomId; }
    void clearRoomSelection() override;
    // The area the view follows, which the next render() draws
    int shownAreaId() const override { return mRecentre ? mFollowedAreaId : mView.mAreaID; }
    std::pair<bool, QString> setMapZoom(qreal zoom, int areaId) override;
    std::pair<bool, QString> centerViewOn(int roomId) override;
    std::pair<bool, QString> exportAreaToImage(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels) override;
    std::pair<bool, QString> show3DView(bool shown) override;
    // As dlgMapper answers while it shows its 2D map, the only one this keeps
    bool showing3DView() const override { return false; }
    void recreate3DView() override {}
    std::pair<bool, QString> shift3DViewCamera(float, float, float) override { return {false, qsl("the modern 3D map view is not showing")}; }
    std::pair<bool, QString> set3DViewCameraPosition(float, float, float) override { return {false, qsl("the modern 3D map view is not showing")}; }

private:
    void schedulePaint();
    void paint();
    void requestRepaint();
    void raiseAreaChanged(int areaId, int lastViewedAreaId);
    void forgetDeletedRoom(int roomId);
    // What paint() does once the player is in another room, or follow() asks: the next render() centres on it
    void followRoom(int roomId, int areaId);
    void collectDrawnRooms();
    int centredRoomId() const { return mRecentre ? mFollowedRoomId : mView.mRoomID; }

    Host* mpHost = nullptr;
    const bool mRaisesEvents = true;
    bool mShown = false;
    bool mPaintScheduled = false;
    // What T2DMap keeps between frames: the view last drawn, as the pointer and the selection have changed it since,
    // and the area last raised for
    TMapPainter::View mView;
    // paint() followed the player to this room and area, which the next render() centres on as T2DMap would, and
    // for a small area may centre on the area
    bool mRecentre = false;
    int mFollowedRoomId = 0;
    int mFollowedAreaId = 0;
    bool mLaidOut = false;
    // Cleared by a move() or showRoom() that leaves the view off the player, set again as it follows the player
    bool mFollowing = true;
    QList<DrawnRoom> mDrawnRooms;
    // A wheel turn's zoom and centre, which only the next render() lays out, so the pointer acts on what was drawn
    struct PendingZoom
    {
        qreal xyzoom = 0.0;
        qreal centreX = 0.0;
        qreal centreY = 0.0;
    };
    std::optional<PendingZoom> mPendingZoom;
    TMapPainter mPainter;
};

#endif // MUDLET_TNULLMAPVIEW_H
