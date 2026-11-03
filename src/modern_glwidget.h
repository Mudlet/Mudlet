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
#include <QImage>
#include <QMap>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <QQuaternion>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QEasingCurve>

#include "BloomPass.h"
#include "DioramaLayout.h"
#include "ExitScene.h"
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
#include <memory>
#include <optional>

class Host;
class MapSearchPanel;
class QFontMetricsF;
class QLabel;
class QOpenGLFramebufferObject;
class QPainter;
class T2DMap;
class TArea;
class TCommandLine;
class TMap;
class TMapLabel;
class TRoom;
struct MapInfoProperties;
struct MapStyle;

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
    // The room sizes, and whether the view is opaque, from the profile's style
    void applyStyle();
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
    // Where a style with a status line draws it, along the view's foot; empty for any other style
    QRect statusLineRect() const;

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
    // A click on the atlas, or zooming in on it, took this view into another area, which the 2D map has not been switched to
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
    // The diorama's tilt-shift blur: the frame is copied out of the view's framebuffer, which may be multisampled and
    // so cannot be read as a texture, then blurred back into it
    std::unique_ptr<QOpenGLFramebufferObject> mTiltShiftScene;
    std::unique_ptr<QOpenGLShaderProgram> mTiltShiftProgram;
    QOpenGLVertexArrayObject mTiltShiftVAO;
    bool mTiltShiftFailed = false;
    // The depth where the sharp text drawn after the blur starts, so that only what was drawn before it hides it
    DepthSnapshot mDepthSnapshot;

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
    BloomPass mBloom;
    // Room textures this frame has used so far and the last frame used, which making room for another must keep
    QSet<QString> mRoomTexturesInUse;
    QSet<QString> mRoomTexturesKept;
    // Past this many, room textures neither this frame nor the last used are dropped to make room for another, and
    // past this frame's own the rest are not drawn. It must hold the names, IDs and symbols a dense area seen from
    // above shows at once, or a turn of the view would drop textures it asks for again a moment later
    static constexpr qsizetype scmMaxRoomTextures = 1024;
    // Lowered by tests
    qsizetype mMaxRoomTextures = scmMaxRoomTextures;
    // The text in each cell of the terminal style's glyph atlas, in order, and how many of them and in which font
    // its texture was last drawn with
    QStringList mGlyphAtlasCells;
    QHash<QString, qsizetype> mGlyphAtlasIndex;
    qsizetype mGlyphAtlasDrawnCells = 0;
    QString mGlyphAtlasFontKey;
    // Whether the glyph font, by its key, shows each ground's character, which a font without one would draw as a box
    QString mGroundGlyphsFontKey;
    QHash<QString, bool> mGroundGlyphShown;
    // A room's ink outline, by everything it is made from
    struct InkOutline
    {
        int roomId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float top = 0.0f;
        float half = 0.0f;
        float width = 0.0f;
        float dashLength = 0.0f;
        QVector4D color;
        bool operator==(const InkOutline&) const = default;
    };
    // A level's ink outlines and the triangles made from them, where each outline's are remade only when it
    // changes, as making them all for a large area costs more each frame than all the rest of its rooms
    struct InkOutlineLevel
    {
        QVector<InkOutline> outlines;
        bool round = false;
        QVector<float> vertices;
        QVector<float> colors;
        // Where each outline's vertices end, and which outline is each room's
        QVector<int> ends;
        QHash<int, int> indexOfRoom;
    };
    QHash<int, InkOutlineLevel> mInkOutlines;
    // A map label's image with a halo around it, by its area and ID, and what it was made from
    struct HaloedLabel
    {
        qint64 sourceKey = 0;
        QRgb halo = 0;
        QPixmap pixmap;
        // How far the halo reaches past each side of the label, as a part of its width and height
        QPointF margin;
    };
    QHash<std::pair<int, int>, HaloedLabel> mHaloedLabels;
    bool mRoomTextureLimitWarned = false;
    // A map label drawn again in the map style's font and color, by area and label ID
    struct ThemedLabel
    {
        // The label's own picture and the style it was drawn again from, and how many times its resolution
        qint64 sourceKey = 0;
        QString styleId;
        int sharpness = 1;
        // Null where the style leaves the label as it is
        QPixmap pixmap;
    };
    QHash<std::pair<int, int>, ThemedLabel> mThemedLabels;
    static constexpr qsizetype scmThemedLabelsKept = 256;
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
    // How far off the grid point mMapCenterX/Y the camera looks, where zooming about a point can leave it
    QPointF mViewCenterOffset;
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
    // Apart from mPreviousRID, which a view moved by hand or a second map window stops following
    int mPulsedRoomId = 0;
    // The pulse the hologram style sends out from the player's room when the player moves: when it began on
    // mPulseReference, none when none is under way, and what repaints the view while it spreads
    static constexpr qint64 scmPulseMilliseconds = 1200;
    QElapsedTimer mPulseReference;
    std::optional<qint64> mPulseStartedAt;
    QTimer* mPulseTimer = nullptr;
    // Room names that lose a place to another fade out over this long, and those that win one fade in, rather than
    // blinking as the view moves; by room, how far each has faded in, timed on mNameFadeClock, for mNameFadeArea
    int mNameFadeMilliseconds = 200;
    QHash<int, float> mNameOpacity;
    QElapsedTimer mNameFadeClock;
    int mNameFadeArea = -1;
    // Repaints while a name or a piece seen through is still fading
    QTimer* mNameFadeTimer = nullptr;
    // A diorama's pieces that stand between the camera and the player's room or a highlighted one fade to see-through
    // and back over this long; by room, how far each has faded, for those not solid, timed and kept as the names' are
    int mPieceFadeMilliseconds = 250;
    QHash<int, float> mPieceOpacity;
    QElapsedTimer mPieceFadeClock;
    int mPieceFadeArea = -1;
    // Read from the context once it is made; the least any OpenGL 3.3 driver offers until then
    int mMaxTextureSize = 1024;
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
    // A see-through level's exit lines and arrowheads, drawn with its rooms
    struct LevelExits
    {
        QVector<CubeInstanceData> bars;
        // Those the inkwash style brushes on, tapered
        QVector<CubeInstanceData> strokes;
        QVector<float> vertices;
        QVector<float> colors;
        // Whether each triangle shows in a tile too far away for detail
        QVector<bool> distant;
    };
    // What of a see-through level's exits this frame shows, drawn by renderRooms() with the level's rooms
    struct LevelExitRuns
    {
        QList<InstanceRun> bars;
        QList<InstanceRun> strokes;
        QVector<float> vertices;
        QVector<float> colors;
    };
    // Made by renderConnections() for renderRooms(), lowest level first
    QMap<int, LevelExitRuns> mSeeThroughExits;
    // Everything the kept scene's rooms and exits were made from; when any of it differs, the scene is made again
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
        QString styleId;
        int exitGeometryBuilds = 0;
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
    // By text, plain then in bold for the area shown: the names atlasLabels() last measured, as boundingRect() gives them
    std::array<QHash<QString, QRectF>, 2> mAtlasLabelBounds;
    // Halos are slow to stroke every frame. One sheet, not an image per name: the GL paint engine caches a texture per
    // image, and the driver gives each far more memory than a name takes
    struct AtlasLabelSheet
    {
        QImage image;
        // Where each name is in the image, in its pixels, by text, plain then in bold
        std::array<QHash<QString, QRect>, 2> places;
    };
    AtlasLabelSheet mAtlasLabelSheet;
    QFont mAtlasLabelFont;
    QColor mAtlasLabelBackground;
    qreal mAtlasLabelPixelRatio = 0.0;
    std::shared_ptr<RetainedTriangles> mAtlasFootprints = std::make_shared<RetainedTriangles>();
    std::shared_ptr<RetainedTriangles> mAtlasWorld = std::make_shared<RetainedTriangles>();
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
    // Zoomed in from one area into another, see MapAtlas::handoverRadius()
    struct ZoomHandover
    {
        float fromRadius = 0.0f;
        float atDistance = 1.0f;
        float span = 1.0f;
    };
    std::optional<ZoomHandover> mZoomHandover;
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
    // The terminal style's exit characters, made by renderConnections() for renderRooms() to draw with their levels' rooms
    QList<ExitScene::Glyph> mExitGlyphs;
    // The exits as last built and what they were built from. Building them walks every room and exit of the area,
    // which costs more than drawing them, and only a change to the map or to these options changes them
    struct ExitSceneCache
    {
        bool valid = false;
        quint64 mapRevision = 0;
        ExitScene::Options options;
        ExitScene::Scene scene;
    };
    ExitSceneCache mExitSceneCache;
    // How many times the exits have been built: what is made from them is made again whenever this changes
    int mExitSceneBuilds = 0;
    // The bars, strokes and triangles renderConnections() makes from the exits, kept until the exits or anything else
    // they are made from changes, as none of it depends on the camera
    struct ExitGeometry
    {
        struct Inputs
        {
            int sceneBuilds = 0;
            QString styleId;
            bool moreTransparent = false;
            bool upperLowerLevels = false;
            QColor upperLevelColor;
            QColor lowerLevelColor;
            int levelsAbove = 0;
            int levelsBelow = 0;
            bool operator==(const Inputs&) const = default;
        };
        bool valid = false;
        Inputs inputs;
        QVector<CubeInstanceData> bars;
        QVector<CubeInstanceData> strokes;
        QVector<float> vertices;
        QVector<float> colors;
        QMap<int, LevelExits> seeThrough;
        // The level of each opaque bar, stroke and triangle, for the kept scene to file them by
        QVector<int> barLevels;
        QVector<int> strokeLevels;
        QVector<int> triangleLevels;
        QVector<bool> distant;
    };
    ExitGeometry mExitGeometry;
    int mExitGeometryBuilds = 0;
    // What the diorama's pieces were last laid out from, as above; the pieces too change only with the map, the
    // levels shown and the style, and both passes of a frame and picking a room ask for them
    struct DioramaPiecesCache
    {
        bool valid = false;
        quint64 mapRevision = 0;
        int areaId = 0;
        int centerZ = 0;
        int levelsAbove = 0;
        int levelsBelow = 0;
        QString styleId;
        QHash<int, DioramaLayout::Piece> pieces;
    };
    DioramaPiecesCache mDioramaPiecesCache;
    int mDioramaLayouts = 0;

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
    // The radius of the area shown, or during a flight or a zoom into it between it and the one left; 0 when none is shown
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
    // Lays the names out in a new sheet, copying those the old one has and drawing the rest
    void paintAtlasLabelSheet(const QList<AtlasLabel>& labels, qreal pixelRatio);
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
    void addRoomCubes(const TRoom& room, bool hovered, bool solidPlayer, const std::function<void(RetainedMapScene::Layer, const CubeInstanceData&)>& add) const;
    // The paper or the felt behind the map, for a style drawn on either
    void queueBackdrop();
    // The 2D map's grid, at the viewed level's room bottoms, writing no depth
    void queueGrid();
    // MapStyle::washReach's washes under the viewed level's rooms, writing no depth, under the exits and labels
    void queueWashes();
    GLuint washTexture(int variant);
    // Map labels lie flat at their place on the map; those shown on top over the rooms, the others under the exits
    // What a frame draws, in order, into the render command queue
    void queueScene();
    // The labels of the levels from lowestLevel to highestLevel, either beneath the rooms or on top of them
    void renderLabels(bool onTop, int lowestLevel, int highestLevel);
    // A column of light over the player's room, seen faintly through anything in front of it
    void renderPlayerBeacon();
    // A style with a pulse's marker: an outlined diamond of light hovering over the player's room, on a stem from its top
    void renderPlayerDiamond(const QVector3D& roomTop, float roomSize);
    void paintCompass(QPainter& painter);
    void paintScaleBar(QPainter& painter);
    void paintVignette(QPainter& painter);
    void paintPlayerNote(QPainter& painter);
    // A run of the status line's text in one color
    struct StatusLinePart
    {
        QString text;
        QColor color;
    };
    QList<StatusLinePart> statusLineParts() const;
    void paintStatusLine(QPainter& painter);
    // A style with a scanner overlay's corner brackets and readout, the readout starting top pixels down
    void paintScannerOverlay(QPainter& painter, int top);
    // The style's lettering at the size of the text drawn over the view
    QFont sheetFont() const;
    // Where it was drawn, below top, or an empty rectangle where the style draws none or the view has no room for it
    QRect paintTitleBlock(QPainter& painter, const TArea& area, int top);
    // Below top, and above the title block
    void paintLevelCallouts(QPainter& painter, const TArea& area, int top, const QRect& titleBlock);
    // A row of the line legend: a line's kind of ground in the color of its most used line, or a door's status
    struct LegendEntry
    {
        QString text;
        QColor color;
        // 1 to 3 for a door's entry, drawn as a bar in color across a line; 0 for a line's
        int doorStatus = 0;
    };
    // For the lines and doors on the viewed level, the most used ground first; empty where the style draws no legend
    QList<LegendEntry> lineLegend(const TArea& area) const;
    // Below top; empty where there is no legend or the view has no room for it
    QRect lineLegendRect(const TArea& area, int top) const;
    QRect lineLegendRect(const QList<LegendEntry>& entries, int top) const;
    void paintLineLegend(QPainter& painter, const TArea& area, int top);
    struct LevelPlatform
    {
        int level = 0;
        int rooms = 0;
        // The level's name, and the name of the room the stair reaches on it where it has one
        QString label;
        // The line through its stations, in the color of its most used environment
        QColor lineColor;
        QRectF pill;
        // How many stations it shows, and the width of the count of those it leaves out
        int stations = 0;
        qreal moreWidth = 0.0;
        QPointF station(int index) const;
    };
    // Platforms on one side of the viewed level, listed top to bottom, on a shaft rising or falling from the stair
    // that reaches them, if the view shows one
    struct PlatformShaft
    {
        std::optional<QPointF> stair;
        qreal x = 0.0;
        QList<LevelPlatform> platforms;
    };
    // Above and below the viewed level, below top; empty where the style has no platforms
    std::pair<PlatformShaft, PlatformShaft> levelPlatforms(const TArea& area, int top) const;
    void paintLevelPlatforms(QPainter& painter, const std::pair<PlatformShaft, PlatformShaft>& shafts);
    // On the player's station, or the first station of the platform of the level the player is on
    void paintPlayerPill(QPainter& painter, int top, const std::pair<PlatformShaft, PlatformShaft>& shafts);
    void turnNorthUp();
    void setHoveredRoom(int roomId);
    void applyTiltShift();
    // Worked out afresh, as the view can move under a pointer that stays still
    int roomUnderPointer();
    // What the diorama style builds each of the area's shown rooms as, laid out again only when that would differ
    const QHash<int, DioramaLayout::Piece>& dioramaPieces(const TArea& area);
    // By room, how solid each piece that is not is drawn, as it fades to or from being seen through
    QHash<int, float> pieceOpacities(const TArea& area, const QHash<int, DioramaLayout::Piece>& pieces, int playerRoomId);
    // The exits of the area shown, built again only when the map or the options they are built from change
    const ExitScene::Scene& exitScene(const ExitScene::Options& options);
    void buildExitGeometry(const ExitScene::Scene& scene, const ExitScene::Options& options, bool moreTransparent);
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
    // makeImage is only called when the texture is not already made
    GLuint imageTexture(const QString& key, const std::function<QImage()>& makeImage);
    // nullptr where the map style leaves the label as it is
    const QPixmap* themedLabel(int labelId, const TMapLabel& label, const MapStyle& style, int sharpness);
    // The 2D map's up/down/in/out markers for the room, or 0 when it has none
    GLuint exitMarkerTexture(const TRoom& room, const QColor& roomColor);
    // Text on a clear background, with a rim of halo around it if that is valid; size is set to the texture's size in pixels
    GLuint
    textTexture(const QString& key, const QString& text, const QFont& font, const QFontMetricsF& metrics, const QColor& color, QSize& size, const QColor& halo = QColor(), bool softHalo = false);
    // The label's image with a halo around it, made again only when the label or the halo changes
    const HaloedLabel& haloedLabel(int labelId, const QPixmap& pixmap, const QColor& halo, int backgroundAlpha);
    // The cell of the glyph atlas holding text, added if there is room, else -1; with add false, -1 for any not in it
    qsizetype glyphAtlasCell(const QString& text, bool add = true);
    // Every cell's text in white on clear, to be tinted as drawn, in one texture; 0 if it could not be made
    GLuint glyphAtlasTexture(const QFont& font);
    QFont terminalGlyphFont(const MapStyle& style) const;
    bool groundGlyphShown(const QFont& font, const QString& glyph);
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
    // Within the levels shown above and below the viewed one
    bool levelInRange(int z) const;
    // Drawn in place: in range, and the viewed level where the style puts the others on platforms
    bool levelShown(int z) const;
    // What rooms and exits on level z recede toward: the mapper's upper or lower level color, or the background
    QColor levelFadeTarget(int z) const;
    const MapStyle& mapStyle() const;
    void applyStyleGeometry();
    void stopPulse();
    // The style's background, else the profile's mapper background color
    QColor backgroundColor() const;
    // Of the beacon over the player's room, and of the style's thin ring around it
    QColor playerMarkerColor() const;
    ExitScene::Options exitSceneOptions() const;
    // The shading of the exits not brushed on as strokes
    RenderCommand::Shading exitShading() const;
    // How far below where ExitScene puts them exits are drawn, so that a model's paths lie on the ground
    float exitSink(const ExitScene::Options&) const;
    // About the point at, else the view's center, keeping what is under it in place, which off the center stops following
    // the player; zooming in while areas are picked goes into the area there
    void zoomBySteps(float steps, std::optional<QPoint> at = std::nullopt);
    // While areas are picked, makes the one under this point, or else the nearest, the area shown, keeping the view where it is
    void zoomIntoAreaAt(const QPoint& position);
    // As areaAt(), with this much slop in pixels
    int areaAt(const QPoint& position, float slopPixels);
    // Points the camera at this x and y exactly, on the nearest level
    void centerViewOn(const QVector3D& point);
    // Once zoomed in all the way through a handover, the zoom levels go by the area shown alone, zooming back out too
    void settleZoomHandover();
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
