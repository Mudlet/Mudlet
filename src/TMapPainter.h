#ifndef MUDLET_TMAPPAINTER_H
#define MUDLET_TMAPPAINTER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018-2019, 2022-2023, 2025 by Stephen Lyons       *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2021-2022 by Piotr Wilczynski - delwing@gmail.com       *
 *   Copyright (C) 2022 by Lecker Kebap - Leris@mudlet.org                 *
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

#include <QCache>
#include <QColor>
#include <QFont>
#include <QGradient>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QSize>
#include <QString>

#include <functional>
#include <optional>
#include <utility>

class Host;
class QElapsedTimer;
class QLineF;
class QObject;
class QPainter;
class QPen;
class QRect;
class TArea;
class TMap;
class TMapLabel;
class TRoom;
struct MapInfoProperties;

// What a TMapPainter draws: the map, which part of it and at what size, and the state of the view that marks it.
// The fields keep the names of T2DMap's members they are copied from, so the drawing code reads them unchanged.
struct TMapPainterView
{
    TMap* mpMap = nullptr;
    Host* mpHost = nullptr;
    // What is painted on, T2DMap's widget for one
    QSize mSize;
    // These are the on-screen width and height pixel numbers of the area for a
    // room symbol, (for the non-grid map mode case what gets filled in is
    // multiplied by rsize which is 1.0 to exactly fill space between adjacent
    // coordinates):
    float mRoomWidth = 0.0f;
    float mRoomHeight = 0.0f;
    // Where the map's origin is drawn
    int mRX = 0;
    int mRY = 0;
    double rSize = 0.5;
    double mBSize = 3.0;
    int mAreaID = 0;
    int mMapCenterZ = 0;
    bool mShowRoomID = false;
    // Digits in the drawn area's highest room id
    quint8 mMaxRoomIdDigits = 0;
    bool mBubbleMode = false;
    bool mMapperUseAntiAlias = true;
    bool mLargeAreaExitArrows = false;
    double eSize = 3.0;
    // The room the view was last centred on, and whether it has since been panned away from the player
    int mRoomID = 0;
    bool mShiftMode = false;
    qreal mMapCenterX = 0.0;
    qreal mMapCenterY = 0.0;
    // A secondary view does not follow the player
    bool mIsSecondaryView = false;
    // The area sysMapAreaChanged was last raised for
    int mLastViewedAreaID = -2;
    qreal xyzoom = 0.0;
    float xspan = 0.0f;
    float yspan = 0.0f;
    // The room size the symbol and label pixmaps were cached at
    float mPrevRoomWidth = 0.0f;
    float mPrevRoomHeight = 0.0f;
    // The TArea::getRoomsVersion() mMaxRoomIdDigits was counted at
    quint32 mCachedRoomIdDigitsVersion = 0;
    bool mDrawUpperLowerLevels = false;
    bool mShowGrid = false;
    bool mRoomBeingMoved = false;
    // The rectangle being dragged out, to select rooms or to size a label
    bool mMultiSelection = false;
    bool mSizeLabel = false;
    QRectF mMultiRect;
    int mMultiSelectionHighlightRoomId = 0;
    // The selection's room list, which the map info box is drawn to the right of
    bool mSelectionListVisible = false;
    int mSelectionListX = 0;
    int mSelectionListWidth = 0;
    QGradientStops mPlayerRoomColorGradientStops;
    // Leaves an empty map's area blank rather than saying why
    bool mSuppressEmptyStateMessage = false;
    QString mHelpMsg;
    int mFontHeight = 20;
    bool isCenterViewCall = false;
    // Drawn over the map but under the map info box
    std::function<void(QPainter&)> mPaintBelowInfoBox;
    // A click at mPHighlight that the drawing of a room resolves: it clears mPick, and mStartSpeedWalk when the
    // room it lands on is one to speedwalk to
    QPoint mPHighlight;
    bool mPick = false;
    bool mStartSpeedWalk = false;
    QSet<int> mMultiSelectionSet;
    // The custom line being drawn, or the one selected
    int mCustomLinesRoomTo = 0;
    int mCustomLineSelectedRoom = 0;
    QString mCustomLineSelectedExit;
    int mCustomLineSelectedPoint = -1;
    // Test-only: forces the every-room loop so a test can check the index path draws the same frame,
    // i.e. never hands over too few rooms.
    bool mLodExitIndexDisabled = false;
    // Lays the rooms out at xyzoom, leaving the area's zoom be: a click on a frame already drawn hits what it showed,
    // though the area's zoom has changed since
    bool mKeepZoom = false;
};

// Draws the 2D map onto any QPainter, with QtGui alone, for T2DMap and for an app that draws the map itself.
class TMapPainter : private TMapPainterView
{
public:
    using View = TMapPainterView;

    void setView(const View& view) { static_cast<View&>(*this) = view; }
    const View& view() const { return *this; }

    // What painting a view resolved, for its caller to act on: no Lua runs in here but the map info contributors'
    struct PaintResult
    {
        // The view as the frame left it: centred on the player, zoomed to the area's zoom, the click used up
        View view;
        // The area shown changed, from the second to the first: the caller raises sysMapAreaChanged
        std::optional<std::pair<int, int>> areaChanged;
        // A click to speedwalk from the first room to the second
        std::optional<std::pair<int, int>> pendingSpeedWalk;
        // Rooms outside the area now shown left the selection, so its room list is stale
        bool selectionPruned = false;
        int lodExitIndexRoomsHandedOver = -1;
    };
    // Paints the whole view, the map info box included, at view.mSize. viewSettled is handed the frame's view just
    // before the info box's contributors run, so the Lua they run can read this frame's area and zoom from its caller.
    void paintView(QPainter&, const View&, PaintResult&, const std::function<void(const View&)>& viewSettled = {});
    // Returns where the box ends, down from the top
    static int paintMapInfo(const QElapsedTimer& renderTimer,
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
                            int fontHeight,
                            // Invalid for the profile's info box background
                            const QColor& background = QColor(),
                            // Empty for the profile's display font's
                            const QString& fontFamily = QString());
    static int paintMapInfoContributor(QPainter& painter, int xOffset, int yOffset, const MapInfoProperties& properties, QColor bgColor, int fontHeight, int widgetWidth);
    // Whether any selected room was dropped
    static bool pruneSelectionToArea(TMap*, QSet<int>& selection, int& highlightRoomId, int areaId);
    // The selected room nearest the selection's mean position, 0 when none is left
    static int selectionCentreRoom(TMap*, const QSet<int>& selection);
    // What the mouse does to a view, for T2DMap and for an app that draws the map itself:
    // the rooms of the view's area and level under a click
    static QList<int> roomsAtPoint(const View&, const TArea&, const QPoint& position);
    // A click's rooms added to the selection: Ctrl toggles a room in or out, unless Shift is held as well
    static void addRoomsToSelection(TMap*, QSet<int>& selection, bool& multiSelection, int& highlightRoomId, const QList<int>& rooms, Qt::KeyboardModifiers);
    // A wheel turn's zoom, keeping the point under the pointer in place: none for less than half a notch, else whether
    // the zoom changed
    static std::optional<bool>
    zoomAtPoint(TArea&, qreal& xyzoom, qreal& mapCenterX, qreal& mapCenterY, const QSize& size, const QPointF& position, const QPoint& angleDelta, Qt::KeyboardModifiers, bool invertZoom);
    static void startSpeedWalk(Host*, TMap*, int fromRoomId, int toRoomId);
    static void raiseMousePressEvent(Host*);

    // A highlight reaches over the rooms around it but stays under their symbols, ids, names and exit markers, so
    // with one in view every room's body is drawn first, then the highlights, then everything else.
    enum class RoomDrawPass { Whole, Body, Highlight, Labels };
    // Carries the room to save the drawing pass a second room-database probe per exit.
    // Valid only within the paintRoomExits() room iteration that gathered it.
    struct ExitToPaint
    {
        const TRoom* destination = nullptr;
        int destinationId = 0;
        // The destination's reverse exit doesn't lead back. Not the sole arrow test: an exit also gets
        // an arrow when a sibling exit to the same room is one-way.
        bool oneWay = false;
        // A straight custom line between the two rooms draws this exit's line, but not this room's door on it.
        bool drawnByCustomLine = false;
    };

    // Below this many pixels per room, both room loops just fill the environment colour, dropping border,
    // symbol, name and vertical-exit markers: still visible, but far too costly there (see the note on
    // QPainterPath::addRect() in drawGridModeRooms()).
    static constexpr float cLodPixelThreshold = 4.0f;
    static int clampedToIntRange(double value);
    static QRect viewportRoomBounds(float rx0, float ry0, float roomWidth, float roomHeight, float widgetWidth, float widgetHeight);
    static bool sizeFontToFitTextInRect(QFont&, const QRectF&, const QString&, const quint8 percentageMargin = 10, const qreal minFontSize = 7.0);
    static void drawPixmapScaledTo(QPainter& painter, const QRectF& target, const QPixmap& pixmap);
    static void paintLabelHighlight(QPainter& painter, QRectF labelPaintRectangle, const QSizeF& clickSize);
    static int customLineDestination(const TRoom& room, const QString& key);
    static bool customLineRunsStraightTo(const TRoom& owner, const TRoom& other);

    void flushSymbolPixmapCache() { mSymbolPixmapCache.clear(); }
    qsizetype symbolPixmapCacheCount() const { return mSymbolPixmapCache.size(); }
    void addSymbolToPixmapCache(const QString, const QString, const QColor, const bool);
    void resetSymbolFontSize() { mSymbolFontSize = 1; }
    void flushTextLabelPixmapCache()
    {
        mTextLabelPixmapCache.clear();
        mTextLabelPixmapCache.setMaxCost(mTextLabelPixmapBudgetKiB);
        mTextLabelPixmapFrameKiB = 0;
        mTextLabelPixmapLastFrameKiB = 0;
    }
    void startTextLabelFrame();
    qsizetype textLabelPixmapCacheCount() const { return mTextLabelPixmapCache.size(); }
    static constexpr qsizetype scmTextLabelPixmapBudgetKiB = 32 * 1024;
    void setTextLabelPixmapBudgetKiB(qsizetype budgetKiB)
    {
        mTextLabelPixmapBudgetKiB = budgetKiB;
        flushTextLabelPixmapCache();
    }
    // Costed in KiB, not entries, which thrash once more labels are on screen; grows past
    // the budget to hold those, up to four labels of the largest size it caches.
    static constexpr qsizetype scmTextLabelPixmapCeilingKiB = 256 * 1024;
    void setTextLabelPixmapCeilingKiB(qsizetype ceilingKiB)
    {
        mTextLabelPixmapCeilingKiB = ceilingKiB;
        flushTextLabelPixmapCache();
    }
    // Which pixmap the cache holds for a label of that area drawn at this size, 0 for none
    qint64 textLabelPixmapCacheKey(int areaId, int labelId, const QSize& size) const
    {
        const LabelPixmap* cached = mTextLabelPixmapCache.object(LabelPixmapKey{areaId, labelId, size});
        return cached ? cached->pixmap.cacheKey() : 0;
    }
    void drawScaledLabel(QPainter& painter, const QPointF& position, TMapLabel& label, int labelKey, const QRectF& paintRect);

    void drawRoom(QPainter&,
                  QFont&,
                  QFont&,
                  QPen&,
                  TRoom*,
                  const bool isGridMode,
                  const bool areRoomIdsLegible,
                  const bool showRoomNames,
                  const int,
                  const float,
                  const float,
                  const bool showRoomCollision,
                  const RoomDrawPass pass = RoomDrawPass::Whole);
    void drawRoomHighlightsAndLabels(QPainter&,
                                     QFont& roomVNumFont,
                                     QFont& mapNameFont,
                                     QPen&,
                                     const QList<QPair<TRoom*, QPointF>>& rooms,
                                     const bool isGridMode,
                                     const bool areRoomIdsLegible,
                                     const bool showRoomNames,
                                     const int speedWalkStartRoomId);
    bool anyRoomHighlighted(const QList<int>& roomIds) const;
    // The grid and reduced-detail loops draw their highlights before the player's room is drawn, so they put its body under them first
    void drawPlayerRoomBody(QPainter&, TRoom* pPlayerRoom, const QPointF& roomCenter, const bool isGridMode, const bool showRoomCollision);
    // Batch rendering for large grid mode areas - draws rooms grouped by color
    void drawGridModeRooms(QPainter&,
                           const TArea* pDrawnArea,
                           const int zLevel,
                           const int playerRoomId,
                           const float mRX,
                           const float mRY,
                           const float mRoomWidth,
                           const float mRoomHeight,
                           const float widgetWidth,
                           const float widgetHeight,
                           QFont& roomVNumFont,
                           bool& isPlayerRoomVisible,
                           QPointF& playerRoomOnWidgetCoordinates,
                           bool& isPlayerRoomBodyDrawn,
                           bool areRoomIdsLegible,
                           QString* profileOutput = nullptr);
    // The non-grid room loop for zooms where a room is a few pixels across.
    void drawNonGridModeRoomsLod(QPainter&,
                                 const TArea* pDrawnArea,
                                 int zLevel,
                                 int playerRoomId,
                                 const QList<int>& viewportRooms,
                                 float widgetWidth,
                                 float widgetHeight,
                                 bool& isPlayerRoomVisible,
                                 QPointF& playerRoomOnWidgetCoordinates,
                                 bool& isPlayerRoomBodyDrawn,
                                 QString* profileOutput = nullptr);
    QColor environmentColor(int environmentId) const;
    QSize lodRoomBlobSize() const;
    // Whether drawRoom() strokes a border around the room's fill outside grid mode, bar a collision's
    bool roomBorderShown(const TRoom&) const;
    // Width in pixels of the border drawRoom() strokes around a room's fill
    int roomBorderWidth(const TRoom&) const;
    void paintRoomExits(QPainter&, QPen&, QList<ExitToPaint>& exitList, const TArea*, int zLevel, const QRect& roomBounds, const QList<int>& viewportRooms, float exitWidth, QMap<int, QPointF>&);
    void drawDoor(QPainter&, const TRoom&, const QString&, const QLineF&);

    // Draws an area at a size of its own into an image file, written on a pool thread; how that goes is reported to
    // the profile. The returned pair says whether the export started, or why not.
    std::pair<bool, QString> exportArea(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels, QObject* saveOwner);

    // A double-clicked room's speedwalk runs Lua, which may delete rooms, so it waits until no room is held for drawing
    std::optional<std::pair<int, int>> takePendingSpeedWalk() { return std::exchange(mPendingSpeedWalk, std::nullopt); }
    // Rooms the exit index handed paintRoomExits() last frame, -1 if unused
    int lodExitIndexRoomsHandedOver() const { return mLodExitIndexRoomsHandedOver; }

private:
    void paintFrame(QPainter&, PaintResult&, const std::function<void(const View&)>& viewSettled);
    void resolveAreaExitClick(QPainter&, const QMap<int, QPointF>& areaExitsMap, const int speedWalkStartRoomId);
    void spendClick();
    static std::pair<bool, QString> performImageSave(const QPixmap& pixmap, const QString& filePath, const QString& format);
    int width() const { return mSize.width(); }
    int height() const { return mSize.height(); }

    std::optional<std::pair<int, int>> mPendingSpeedWalk;
    int mLodExitIndexRoomsHandedOver = -1;

    QCache<QString, QPixmap> mSymbolPixmapCache;
    // The round-room setting the symbol pixmaps were drawn for
    bool mCachedBubbleMode = false;
    ushort mSymbolFontSize = 1;
    QFont mMapSymbolFont;

    struct LabelPixmapKey
    {
        int areaId = 0;
        int labelId = 0;
        QSize size;

        friend bool operator==(const LabelPixmapKey&, const LabelPixmapKey&) = default;
        friend size_t qHash(const LabelPixmapKey& key, size_t seed = 0) noexcept { return qHashMulti(seed, key.areaId, key.labelId, key.size.width(), key.size.height()); }
    };
    // A label drawn at its zoomed size, with what it was drawn from so an edited
    // label is redrawn rather than served stale.
    struct LabelPixmap
    {
        QPixmap pixmap;
        QString text;
        QFont font;
        QRgb fgColor = 0;
        QRgb bgColor = 0;
        QRgb outlineColor = 0;
        qint64 sourcePixmapKey = 0;
        qreal devicePixelRatio = 1.0;
        qsizetype costKiB = 1;
    };
    static bool labelPixmapIsCurrent(const LabelPixmap& cached, const TMapLabel& label, qreal devicePixelRatio);
    const LabelPixmap* addTextLabelToCache(const LabelPixmapKey& key, const TMapLabel& label, qreal devicePixelRatio);
    qsizetype mTextLabelPixmapBudgetKiB = scmTextLabelPixmapBudgetKiB;
    qsizetype mTextLabelPixmapCeilingKiB = scmTextLabelPixmapCeilingKiB;
    QCache<LabelPixmapKey, LabelPixmap> mTextLabelPixmapCache{scmTextLabelPixmapBudgetKiB};
    qsizetype mTextLabelPixmapFrameKiB = 0;
    qsizetype mTextLabelPixmapLastFrameKiB = 0;
};

#endif // MUDLET_TMAPPAINTER_H
