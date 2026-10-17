#ifndef MUDLET_MODERN_GLWIDGET_H
#define MUDLET_MODERN_GLWIDGET_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2020-2021 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

// (2 of 2) This must be included before any Qt library tries to include
// windows.h which pulls in winsock.h to avoid (multiple):
// "#warning Please include winsock2.h before windows.h [-Wcpp]" warnings
#if defined(INCLUDE_WINSOCK2)
#include <winsock2.h>
#endif

#include <QElapsedTimer>
#include <QHash>
#include <QMap>
#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QEasingCurve>

#include "GeometryManager.h"
#include "LabelTextureCache.h"
#include "MapAtlas.h"
#include "MapRoute.h"
#include "MapSearch.h"
#include "MapStitch.h"
#include "RenderCommandQueue.h"
#include "ResourceManager.h"
#include "RetainedMapScene.h"
#include "ShaderManager.h"
#include "CameraController.h"

#include <array>
#include <functional>
#include <optional>

class Host;
class MapSearchPanel;
class QFontMetricsF;
class QLabel;
class QPainter;
class T2DMap;
class TCommandLine;
class TMap;
class TRoom;
struct MapInfoProperties;
namespace ExitScene {
struct Options;
}

class ModernGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
    friend class ModernRoomTextureTest;
    friend class Map3DViewInteractionTest;
    friend class ModernMapperRenderTest;
    friend class ModernMapperSceneTest;

public:
    Q_DISABLE_COPY(ModernGLWidget)
    ModernGLWidget(TMap*, Host*, QWidget* parent = nullptr);
    ~ModernGLWidget() override;

    // The environment variables that made Qt compose every window with an API other than OpenGL
    static QStringList rhiForcingVariables();

    void wheelEvent(QWheelEvent* e) override;
    void setViewCenter(int, int, int, int);
    // Takes over another view's area and center; roomId is the player room that view last followed
    void syncView(int areaId, int x, int y, int z, int roomId);
    void followPlayer();
    bool followingPlayer() const;
    int shownAreaId() const { return mAID; }
    int areaShownNext() const;
    QVector3D viewCenter() const { return QVector3D(mMapCenterX, mMapCenterY, mMapCenterZ); }
    void shiftCamera(float, float, float);
    void setCameraPosition(float, float, float);
    // Frames as many rooms as the 2D map shows at this zoom, when areaId is the area on view
    void applyMapZoom(qreal zoom, int areaId);
    void setCameraDistance(float r);
    const QString& failureMessage() const { return mFailureMessage; }
    // A grid-mode area is always drawn looking straight down with north up
    bool cameraFixedByGridMode() const;
    // The mapper's 2D map, whose room selection, speedwalk and context menu this view shares
    void setCompanion2DMap(T2DMap* map);
    // The room drawn under this point of the view, or 0; orNear also takes a room seen too small to point at
    // whose middle is a few pixels off. With onLevel, only that level's rooms count, even under others
    int roomAt(const QPoint& position, bool orNear = true, std::optional<int> onLevel = std::nullopt);
    // The room in another area that the area-exit arrow drawn under this point of the view leads to, or 0
    int areaExitAt(const QPoint& position);
    // The room a pointer action at this position is about. As on the 2D map, which shows only the viewed level's
    // rooms, that level's room under the pointer wins, even beneath or seen through a room on another level; with
    // areaExits, where an arrow from the viewed level leads comes next, ahead of other rooms; with neighbours, a room
    // of a neighboring area drawn past this area's edge comes after the arrows
    int roomPickedAt(const QPoint& position, bool orNear = true, bool areaExits = false, bool neighbours = false);
    // The room a double click or the tooltip at this position is about: a room, or where an area exit leads
    int destinationAt(const QPoint& position) { return roomPickedAt(position, true, true, true); }
    // The room of a neighboring area drawn past this area's edge under this point of the view, or 0
    int stitchedRoomAt(const QPoint& position);
    // Moves the view into the neighboring area of a room drawn past this area's edge, centered on that room
    void enterStitchedRoom(int roomId);
    // A view in a map window of its own: it shows the area it is told to rather than following the player
    void setSecondaryView(bool secondary);
    bool secondaryView() const { return mSecondaryView; }
    // The rooms on show drawn wholly inside this rectangle of the view, on the level in the middle of
    // the view or, with allLevels, on any level shown
    QSet<int> roomsInRectangle(const QRect& rectangle, bool allLevels);
    // The room drawn raised and outlined, as the one under the pointer, or 0
    int hoveredRoomId() const { return mHoveredRoomId; }
    // Centers and zooms the view to show all the area's rooms on the levels shown
    void fitArea();
    // Where the compass is drawn, which turns the view north up when clicked; empty while the camera is fixed or the view is too small for it
    QRect compassRect() const;
    // The area drawn as a footprint or a node of the world graph under this point of the view, while zoomed out
    // far enough for a click to pick areas rather than rooms, or 0
    int areaAt(const QPoint& position);
    // Moves the view into the area, from where the area is drawn in the world around the one shown now, zooming in on it
    // With a landing room, comes down on that room at the zoom the view has now
    void flyToArea(int areaId, int landingRoomId = 0);
    // Shows the route from the player's room to this room, or to the nearest room of this area that can be reached,
    // at every zoom, keeping it up to date as the player moves. Returns what was told the user if no route was set
    QString planRouteToRoom(int roomId);
    QString planRouteToArea(int areaId);
    void clearRoute();
    const MapRoute& route() const { return mRoute; }
    // Opens the search box over the view, which searches every area of the map
    void openSearch();
    // Flies the view to a place the search found, switching area as needed, and marks a room found for a while
    void goToSearchResult(const MapSearch::Result& result);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void slot_shiftUp();
    void slot_shiftDown();
    void slot_shiftLeft();
    void slot_shiftRight();
    void slot_shiftCameraUp();
    void slot_shiftCameraDown();
    void slot_shiftCameraLeft();
    void slot_shiftCameraRight();
    void slot_shiftZup();
    void slot_shiftZdown();
    void slot_setCameraPositionX(int angle);
    void slot_setCameraPositionY(int angle);
    void slot_setCameraPositionZ(int angle);
    void slot_setScale(int);
    void slot_showAllLevels();
    void slot_singleLevelView();
    void slot_showMoreUpperLevels();
    void slot_showLessUpperLevels();
    void slot_showMoreLowerLevels();
    void slot_showLessLowerLevels();
    void slot_defaultView();
    void slot_sideView();
    void slot_topView();
    void slot_setPlayerIconHeight(int value);
    void slot_setPlayerIconRotationX(int angle);
    void slot_setPlayerIconRotationY(int angle);
    void slot_setPlayerIconRotationZ(int angle);
    void slot_setPlayerIconScale(int value);
    void slot_resetPlayerIcon();

signals:
    void resetPlayerIconSliders(int height, int rotX, int rotY, int rotZ, int scale);
    // Slider positions for the camera as it now is, in the ranges of the mapper's 3D controls
    void cameraControlsChanged(int scale, int tilt, int roll, int azimuth);
    // The user asked to see this room on the 2D map, or this view's center when roomId is 0
    void editIn2DRequested(int roomId);
    // A click on the world flew this view into another area, which the 2D map has not been switched to
    void areaFlownInto(int areaId);

private slots:
    void onCameraAnimationTick();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

public:
    TMap* mpMap = nullptr;

private:
    friend class Map3DViewSyncTest;

    // Shader and rendering resources
    ShaderManager mShaderManager;
    QOpenGLBuffer mVertexBuffer;
    QOpenGLBuffer mColorBuffer;
    QOpenGLBuffer mNormalBuffer;
    QOpenGLBuffer mIndexBuffer;
    QOpenGLBuffer mTexCoordBuffer;
    QOpenGLVertexArrayObject mVAO;

    // Geometry management
    GeometryManager mGeometryManager;

    // Render command queue
    RenderCommandQueue mRenderCommandQueue;

    // Resource management
    ResourceManager mResourceManager;

    // Camera management
    CameraController mCameraController;

    // Label texture cache
    LabelTextureCache mLabelTextureCache;
    // Room textures this frame has used so far, which making room for another must keep
    QSet<QString> mRoomTexturesInUse;
    bool mRoomTextureLimitWarned = false;
    bool mSolidDashWarned = false;

    // Host reference
    QPointer<Host> mpHost;

    QString mFailureMessage;
    QLabel* mpFailureLabel = nullptr;

    // View state
    bool is2DView = false;
    bool mPanMode = false;
    float mPanXStart = 0;
    float mPanYStart = 0;
    float zmax = 9999999.0;
    float zmin = 9999999.0;

    // Map state
    int mRID = 0;
    int mAID = 0;
    int mMapCenterX = 0;
    int mMapCenterY = 0;
    int mMapCenterZ = 0;
    bool mShiftMode = false;
    int mFontHeight = 20;

    // Scales the size of rooms compared to the space between them - currently
    // hard coded to be a quarter (would be equivalent to a 2D room size setting
    // of "2.5"):
    float scale = 4;
    // Room size reduction factor on the z-axis (stacks with scale -> 1/32)
    float zFlattening = 8;
    int mShowTopLevels = 999999;
    int mShowBottomLevels = 999999;

    // Player icon adjustment settings
    float mPlayerIconHeight = 0.41f;     // Default height above room (in units)
    float mPlayerIconRotationX = -54.0f; // Rotation around X axis (degrees)
    float mPlayerIconRotationY = 100.0f; // Rotation around Y axis (degrees)
    float mPlayerIconRotationZ = 37.0f;  // Rotation around Z axis (degrees)
    float mPlayerIconScale = 0.0055f;    // Default scale factor

    // Frame timing for benchmarking
    QElapsedTimer mFrameTimer;

    // Smooth camera animation
    QTimer* mCameraAnimationTimer = nullptr;
    int mTargetAID = 0;
    float mTargetMapCenterX = 0.0f;
    float mTargetMapCenterY = 0.0f;
    float mTargetMapCenterZ = 0.0f;
    float mStartMapCenterX = 0.0f;
    float mStartMapCenterY = 0.0f;
    float mStartMapCenterZ = 0.0f;
    float mCurrentAnimationX = 0.0f; // Floating point current position during animation
    float mCurrentAnimationY = 0.0f;
    float mCurrentAnimationZ = 0.0f;
    qreal mAnimationProgress = 0.0;
    int mAnimationDuration = 100; // 100ms animation duration for smooth movement
    QEasingCurve mEasingCurve;
    bool mCameraSmoothAnimating = false; // Dedicated flag for smooth camera animation
    int mPreviousRID = 0;                // Track previous room ID to detect changes
    bool mFramePending = true;
    // Set by a zoom or a change of area while hidden: the shown area is brought back to its saved distance, or else framed
    // at its stored zoom, read then so a later 2D zoom counts, for the size the view is first seen at, unless a camera
    // distance is set first
    bool mFrameWhenShown = false;
    // A distance a script asked for since the last paint wins over framing a newly shown area
    bool mDistanceRequested = false;

    QPointer<T2DMap> mp2DMap;
    bool mSecondaryView = false;
    QPoint mPressPosition;
    // In editing mode a left drag from empty space, or with Ctrl or Shift, draws a selection rectangle, as on the 2D map
    bool mRectangleSelecting = false;
    bool mRectangleDragged = false;
    QRect mSelectionRectangle;
    // In editing mode a left drag from a room moves the selected rooms, on that room's level
    bool mMovingRooms = false;
    QPointF mRoomMoveLastPoint;
    int mRoomMoveLevel = 0;
    bool mRoomsMoved = false;
    // A click that did not pick its room on the press does so on the release, unless it became a drag
    bool mClickPending = false;
    // The button of a press whose sysMapWindowMousePressEvent waits for its selection to settle, on the release or the drag
    Qt::MouseButton mPressEventButton = Qt::NoButton;
    // Only focus reached from the keyboard, not by a click, gives the view the keys it navigates with
    bool mKeysNavigate = false;
    struct SavedCamera
    {
        float distance = 0.0f;
        float tilt = 0.0f;
        float azimuth = 0.0f;
        float roll = 0.0f;
    };
    QHash<int, SavedCamera> mSavedCameras;
    // The room drawn raised, or to be at the next frame; roomAt() picks it where it rests or is raised
    int mHoveredRoomId = 0;
    QPoint mPointerPosition;
    bool mPointerInView = false;
    Qt::MouseButtons mPointerButtons;
    // The exit lines and arrowheads of see-through levels, made by renderConnections() for renderRooms()
    // to draw with their levels' rooms, lowest level first
    struct LevelExits
    {
        QList<InstanceRun> bars;
        QVector<float> vertices;
        QVector<float> colors;
    };
    QMap<int, LevelExits> mSeeThroughExits;
    // Everything the kept scene's rooms and exits were made from; when any of it differs, the scene is made again.
    // A map style drawing rooms its own way would add itself here, and its rooms' blocks to the scene
    struct SceneInputs
    {
        int areaId = 0;
        quint64 mapRevision = 0;
        quint64 exitsRevision = 0;
        quint32 roomsVersion = 0;
        quint64 appearanceRevision = 0;
        int centerZ = 0;
        int levelsAbove = 0;
        int levelsBelow = 0;
        float scale = 0.0f;
        float zFlattening = 0.0f;
        bool gridMode = false;
        bool showBorders = false;
        double borderSize = 0.0;
        double lineSize = 0.0;
        bool moreTransparent = false;
        bool drawUpperLowerLevels = false;
        bool inOutStubs = false;
        bool largeAreaExitArrows = false;
        QColor borderColor;
        QColor background;
        QColor foreground;
        QColor upperLevelColor;
        QColor lowerLevelColor;
        std::array<QColor, 16> hostColors;
        QMap<int, int> envColors;
        QMap<int, QColor> customEnvColors;
        bool operator==(const SceneInputs&) const = default;
    };
    std::optional<SceneInputs> mSceneInputs;
    // Everything the atlas of areas was made from; it does not depend on the area shown, which only moves it
    struct AtlasInputs
    {
        quint64 mapRevision = 0;
        quint64 appearanceRevision = 0;
        quint64 exitsRevision = 0;
        unsigned int mapGeneration = 0;
        size_t roomsVersions = 0;
        QString layout;
        QColor foreground;
        std::array<QColor, 16> hostColors;
        QMap<int, int> envColors;
        QMap<int, QColor> customEnvColors;
        bool operator==(const AtlasInputs&) const = default;
    };
    std::optional<AtlasInputs> mAtlasInputs;
    MapAtlas mAtlas;
    // What the last frame queued showed of the rooms, the areas' footprints and the world graph
    MapAtlas::Weights mZoomWeights;
    struct AtlasLabel
    {
        QString text;
        QRectF rect;
        bool shownArea = false;
    };
    // A view flying into an area: it glides across the map, then zooms in, in the new area's coordinates
    struct Flight
    {
        QVector3D from;
        QVector3D to;
        float fromDistance = 1.0f;
        float toDistance = 1.0f;
        qreal progress = 0.0;
        // The radius of the area it left, or 0
        float fromRadius = 0.0f;
        // A flight only zooms in, so it shows nothing further out than these
        MapAtlas::Weights fromWeights{1.0f, 1.0f};
    };
    std::optional<Flight> mFlight;
    MapRoute mRoute;
    // Everything the neighboring areas drawn past the shown area's edges were made from
    struct StitchInputs
    {
        int areaId = 0;
        int level = 0;
        quint64 mapRevision = 0;
        quint64 appearanceRevision = 0;
        quint64 exitsRevision = 0;
        unsigned int mapGeneration = 0;
        size_t roomsVersions = 0;
        float scale = 0.0f;
        float zFlattening = 0.0f;
        double lineSize = 0.0;
        bool inOutStubs = false;
        bool largeAreaExitArrows = false;
        QColor background;
        QColor foreground;
        std::array<QColor, 16> hostColors;
        QMap<int, int> envColors;
        QMap<int, QColor> customEnvColors;
        bool operator==(const StitchInputs&) const = default;
    };
    std::optional<StitchInputs> mStitchInputs;
    MapStitch mStitch;
    RetainedMapScene mStitchScene;
    // A neighboring area's name and a few of its rooms, shown where an exit leads to it but its rooms are not drawn
    struct PortalCard
    {
        QRectF rect;
        // The tip of the exit's stub, in the view
        QPointF anchor;
        qsizetype portal = 0;
        QString name;
    };
    // Finds a stale route soon after a frame saw it was, outside painting, as finding it can run a script
    QTimer* mRouteRefreshTimer = nullptr;
    QElapsedTimer mRouteGraphBuilt;
    qint64 mRouteGraphBuildMs = 0;
    MapSearch mSearch;
    MapSearchPanel* mpSearchPanel = nullptr;
    QList<MapSearch::Result> mSearchResults;
    // The room a search went to, marked until the time is up
    int mSearchMarkedRoomId = 0;
    QElapsedTimer mSearchMarkTime;
    QTimer* mSearchMarkTimer = nullptr;
    RetainedMapScene mScene;
    int mSceneMaxRoomId = 0;
    // Rooms whose blocks are drawn narrower than this many pixels, in tiles of the scene so far away, are drawn
    // without what is on them; such marks would be a pixel or less across
    float mDetailPixels = 3.0f;
    // The rooms whose IDs the last room pass drew: those too small to read, or overlapping one kept before them
    // (the viewed level's first, then the nearest), are left out
    QSet<int> mRoomIdsDrawn;

    // Private methods for modern OpenGL
    void updateMatrices();
    void renderRooms();
    void renderConnections();
    SceneInputs sceneInputs() const;
    // What a frame sees, for culling the kept scenes
    RetainedMapScene::View sceneView() const;
    // Makes the kept scene again if anything it was made from changed, then sees what of it this frame shows
    void refreshScene();
    // How many pixels apart neighboring rooms are, where the camera looks
    float roomPixels() const;
    // How much this view, as of the last updateMatrices(), shows of the rooms, the footprints and the world graph
    MapAtlas::Weights zoomWeights();
    // The radius of the area shown, or during a flight between it and the one left; 0 when none is shown
    float shownRadius() const;
    // The furthest out a view of the area shows its rooms alone, at most CameraController::scmMaxDistance
    float roomsAloneDistance(int areaId) const;
    void refreshAtlas();
    // Where the atlas lies: the plane the camera looks at, under its rooms
    float atlasLevel() const;
    // Places the world so that the area shown sits on its own rooms
    QMatrix4x4 atlasModelMatrix() const;
    void queueAtlas(const MapAtlas::Weights& weights);
    // The area names to draw over the view, largest areas first, none overlapping another
    QList<AtlasLabel> atlasLabels(const MapAtlas::Weights& weights);
    void paintAtlasLabels(QPainter& painter);
    // Ends a flight into an area where it got to
    void stopFlight();
    // A flight that set off among rooms, cut short, comes in to where the area's rooms show
    void closeInOnRooms();
    void flightTick();
    // Finds the route again if the player moved or the map changed since it was found
    void refreshRoute();
    void scheduleRouteRefresh();
    bool findRoute();
    QString reportRoute();
    // The name of the room or area the route goes to
    QString routeDestinationName() const;
    // The route's color, and the color of the halo that keeps it apart from what it is drawn over
    QColor routeColor() const;
    QColor routeHaloColor() const;
    // Over the rooms of the area shown: the route's rooms and the steps between them
    void queueRouteInRooms();
    // The route over the areas' footprints, and along the world graph, its lines this many world units a pixel
    void appendRouteOnFootprints(float unitsPerPixel, QVector<float>& vertices, QVector<float>& colors) const;
    void appendRouteOnWorld(float unitsPerPixel, QVector<float>& vertices, QVector<float>& colors) const;
    void paintRouteStatus(QPainter& painter);
    // Places the neighboring areas again if the area, the level or the map changed, or clears them if not to be shown
    void refreshStitch();
    // The room drawn past this area's edge with this id, if the stitch is of the area shown
    const MapStitch::Room* stitchedRoom(int roomId) const;
    void queueStitchedAreas();
    // The portals this frame shows, none overlapping another
    QList<PortalCard> portalCards();
    void paintPortals(QPainter& painter);
    // The portal under this point of the view, as an index into the stitch's portals, or -1
    qsizetype portalAt(const QPoint& position);
    QString portalToolTip(qsizetype portal) const;
    // The name an area is shown by
    QString areaName(int areaId) const;
    // A flight into the area, begun from where flight.from is
    void startFlight(int areaId, Flight flight);
    // How far from the area a view coming into it looks from: as it last did, or else as its 2D zoom shows it
    float arrivalDistance(int areaId) const;
    // A flight to this point of the area, from wherever the view is now, coming no further out than distance
    void flyToPoint(int areaId, const QVector3D& point, float distance);
    bool searchKeyed(const QKeyEvent* event) const;
    void runSearch();
    void chooseSearchResult(int row, bool planRoute);
    // A room or area still on the map, and not where the player already is
    bool routableSearchResult(const MapSearch::Result& result) const;
    void showSearchResultMenu(int row, const QPoint& globalPosition);
    QString searchResultLine(const MapSearch::Result& result) const;
    void placeSearchPanel();
    void queueSearchMark();
    // The blocks drawn for a room: its border, its outline, then the room itself, each with the layer it is drawn in
    void addRoomCubes(const TRoom& room, bool hovered, bool solidRed, const std::function<void(RetainedMapScene::Layer, const CubeInstanceData&)>& add) const;
    // The 2D map's grid, at the viewed level's room bottoms, writing no depth
    void queueGrid();
    // Map labels lie flat at their place on the map; those shown on top over the rooms, the others under the exits
    // What a frame draws, in order, into the render command queue
    void queueScene();
    // The labels of the levels from lowestLevel to highestLevel, either beneath the rooms or on top of them
    void renderLabels(bool onTop, int lowestLevel, int highestLevel);
    // A column of light over the player's room, seen faintly through anything in front of it
    void renderPlayerBeacon();
    void paintCompass(QPainter& painter);
    void turnNorthUp();
    void setHoveredRoom(int roomId);
    // Worked out afresh, as the view can move under a pointer that stays still
    int roomUnderPointer();
    void endPan();
    void renderCube(float x, float y, float z, float size, float r, float g, float b, float a);
    void renderText(const QString& text, float x, float y);
    void setupBuffers();
    void cleanup();
    void showFailure(const QString& message, const QString& advice = QString());
    GLuint symbolTexture(const QString& symbol, const QColor& color);
    // A disc filling the texture, shaded by a radial gradient over its radius
    GLuint discTexture(const QString& key, const QGradientStops& stops);
    GLuint squareTexture(const QColor& color);
    GLuint cachedRoomTexture(const QString& key);
    bool roomForAnotherTexture(const QString& key);
    GLuint addRoomTexture(const QString& key, const QImage& image);
    // The 2D map's up/down/in/out markers for the room, or 0 when it has none
    GLuint exitMarkerTexture(const TRoom& room, const QColor& roomColor);
    // Text on a clear background; size is set to the texture's size in pixels
    GLuint textTexture(const QString& key, const QString& text, const QFont& font, const QFontMetricsF& metrics, const QColor& color, QSize& size);
    void reportContextCreationFailure();
    bool contextCreationFailed() const;
    void startSmoothTransition(int targetAID, int targetX, int targetY, int targetZ);
    void stopSmoothTransition();
    void jumpTo(int areaId, int x, int y, int z);
    void frameArea();
    // The area's saved distance, or else its framing, for the size the view has now
    void settleCamera();
    void frameRooms(qreal zoom, float furthest = CameraController::scmMaxDistance);
    void emitCameraControls();
    void setOrientationKeepingRoll(float theta, float phi);
    // Turns the camera around the view's center, by degrees of azimuth
    void turnCamera(float degrees);
    bool levelShown(int z) const;
    // What rooms and exits on level z recede toward: the mapper's upper or lower level color, or the background
    QColor levelFadeTarget(int z) const;
    ExitScene::Options exitSceneOptions() const;
    void zoomBySteps(float steps);
    void panByRooms(int right, int up);
    void selectRoom(int roomId, Qt::KeyboardModifiers modifiers);
    bool editingMap() const;
    // Where the pointer at this point of the view meets the plane of map level z, unless behind the camera or far off
    std::optional<QPointF> mapPointOnLevel(const QPoint& position, int z);
    void startRoomMove(int roomId);
    void endEditGesture();
    void holdView();
    void showContextMenu(const QPoint& position);
    void raiseMousePressEvent();
    // Raised with the press still on, which ends here if a script handling it takes the release
    void raiseMousePressEventMidPress();
    void raisePendingMousePressEvent();
    // Ends the press whose release went elsewhere, as its release would have, but selects nothing
    void endLostPress();
    TCommandLine* commandLine() const;
    QString roomToolTip(int roomId) const;
    GLuint selectionTexture(bool round);
    void saveCamera();
    bool restoreCamera(bool distanceOnly);
    void loadSavedCameras();
    void writeSavedCameras() const;
};

#endif // MUDLET_MODERN_GLWIDGET_H
