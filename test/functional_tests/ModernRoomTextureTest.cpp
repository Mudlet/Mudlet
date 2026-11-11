/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

/*
 * The modern 3D view keeps at most a fixed number of room textures (symbols,
 * ID digits, markers, names), and making room for another must not evict one
 * this frame has already drawn with.
 *
 * Why not a spec: no Lua call reports which textures a frame used. The room
 * pass is run without GL, with stand-in textures, so ctest's offscreen platform
 * runs it; the GL frame is skipped there, and runs on a display:
 *   QT_QPA_PLATFORM=xcb MUDLET_TEST_MODE=1 ./functional_map_tests ModernRoomTextureTest
 */

#include <functional>
#include <limits>

#include <QFileInfo>
#include <QOpenGLContext>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "GeometryManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MapStyle.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "RenderCommand.h"
#include "TArea.h"
#include "TMapLabel.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

// The depth and color writes, and the depth test, as the queued state commands leave them
struct GLDrawState
{
    bool depthWrite = true;
    bool colorWrite = true;
    bool depthLess = true;

    void follow(const GLStateCommand* state)
    {
        switch (state->stateType()) {
        case GLStateCommand::ENABLE_DEPTH_WRITE:
            depthWrite = true;
            break;
        case GLStateCommand::DISABLE_DEPTH_WRITE:
            depthWrite = false;
            break;
        case GLStateCommand::ENABLE_COLOR_WRITE:
            colorWrite = true;
            break;
        case GLStateCommand::DISABLE_COLOR_WRITE:
            colorWrite = false;
            break;
        case GLStateCommand::DEPTH_FUNC_LESS:
            depthLess = true;
            break;
        case GLStateCommand::DEPTH_FUNC_LEQUAL:
            depthLess = false;
            break;
        default:
            break;
        }
    }

    // As the next frame, and the overlay painted over this one, expect to find it
    bool restored() const { return depthWrite && colorWrite && depthLess; }
};

class ModernRoomTextureTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ModernRoomTexture-Test");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // More differently named rooms than the view has textures for, so names have to make room
    int addCrowdedArea() const
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(qsl("Crowded Area"));
        for (int id = 1; id <= 600; ++id) {
            if (!(pMap->addRoom(id) && pMap->setRoomArea(id, areaId) && pMap->setRoomCoordinates(id, id % 25 * 2, id / 25, 0))) {
                return 0;
            }
            // Short, and two rooms apart across, so that every name fits and none is decluttered away
            pMap->mpRoomDB->getRoom(id)->name = qsl("R%1").arg(id);
        }
        // A font of the map's own keeps names at their full size, whatever a map style would shrink them to,
        // and in a font that ships with Mudlet rather than the platform's
        pMap->mUserData.insert(ROOM_UI_NAMEFONT, QFont(Host::scmDefaultFontFamily).toString());
        mpHost->mShowRoomID = true;
        pMap->setRoomNamesShown(true);
        pMap->mRoomIdHash[pMap->mProfileName] = 312;
        return areaId;
    }

    static void verifyDigitsKept(const ModernGLWidget& widget)
    {
        QStringList digitKeys;
        for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
            if (key.startsWith(qsl("digit|"))) {
                digitKeys.append(key);
            }
        }
        QVERIFY2(widget.mRoomTexturesInUse.size() > 20, "the frame did not ask for the rooms' names");
        // Black and white, as the rooms' colors need, ten digits each
        QCOMPARE(digitKeys.size(), 20);
        for (const QString& key : std::as_const(digitKeys)) {
            QVERIFY2(widget.mLabelTextureCache.imageTexture(key), qPrintable(qsl("%1 was evicted in the frame that drew with it").arg(key)));
        }
    }

    // The room pass itself, with textures that need no GL context, so this runs wherever ctest does
    void test_roomIdDigitsKeepTheirTexturesWithoutGL()
    {
        const int areaId = addCrowdedArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        GLuint lastTexture = 0;
        widget.mLabelTextureCache.useTextureStandIn(
                [&lastTexture](const QImage&) {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 24, 12, 0);
        // Never shown, so nothing sizes it: big enough for every name to be legible, with the whole area in view
        widget.resize(1600, 1600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.frameRooms(52.0);
        // As paintGL() does first: names are placed through the camera's matrices
        widget.updateMatrices();

        // The second frame starts with a full cache, so it shows whether the first left the digits usable
        for (int frame = 1; frame <= 2; ++frame) {
            widget.renderRooms();
            verifyDigitsKept(widget);
            if (QTest::currentTestFailed()) {
                qWarning().nospace() << "in frame " << frame;
                return;
            }
        }
        QVERIFY2(lastTexture >= 512, "the view never reached its texture budget");
    }

    // A room with a symbol under one of the level above, which has a border and an exit; and beside it on the
    // viewed level, a room whose environment color is wholly see-through. The level above can be further up, and
    // the label shown on top on another level
    int addLayeredArea(const int upperLevel = 1, const int labelLevel = 0) const
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(qsl("Layered Area"));
        const QList<QVector3D> places{{0, 0, 0}, {2, 0, 0}, {0, 0, static_cast<float>(upperLevel)}, {2, 0, static_cast<float>(upperLevel)}};
        for (int id = 1; id <= places.size(); ++id) {
            const QVector3D& place = places.at(id - 1);
            if (!(pMap->addRoom(id) && pMap->setRoomArea(id, areaId) && pMap->setRoomCoordinates(id, place.x(), place.y(), place.z()))) {
                return 0;
            }
        }
        pMap->mpRoomDB->getRoom(1)->mSymbol = qsl("W");
        constexpr int clearEnvironment = 500;
        pMap->mCustomEnvColors[clearEnvironment] = QColor(0, 0, 0, 0);
        pMap->mpRoomDB->getRoom(2)->environment = clearEnvironment;
        pMap->mpRoomDB->getRoom(3)->mBorderColor = QColor(Qt::red);
        pMap->mpRoomDB->getRoom(3)->mBorderThickness = 4;
        if (!(pMap->setExit(3, 4, DIR_EAST) && pMap->setExit(4, 3, DIR_WEST))) {
            return 0;
        }
        // Under the bordered room above
        TMapLabel label;
        label.pos = QVector3D(-0.5f, 0.5f, static_cast<float>(labelLevel));
        label.size = QSizeF(1.0, 1.0);
        label.pix = QPixmap(8, 8);
        label.pix.fill(Qt::white);
        label.showOnTop = true;
        pMap->mpRoomDB->getArea(areaId)->mMapLabels.insert(1, label);
        mpHost->mShowRoomID = false;
        pMap->setRoomNamesShown(false);
        pMap->mRoomIdHash[pMap->mProfileName] = 1;
        return areaId;
    }

    // The commands one frame queues for the layered area, seen at a slant
    static void queueLayeredFrame(ModernGLWidget& widget, const int areaId, const int viewedLevel = 0)
    {
        widget.mLabelTextureCache.useTextureStandIn(
                [lastTexture = GLuint{0}](const QImage&) mutable {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 0, 0, viewedLevel);
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.setCameraPosition(1.0f, 40.0f, CameraController::scmNorthUpAzimuth);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
    }

    // As renderRooms sizes the default style's rooms
    static float roomHeight()
    {
        const MapStyle* tiles = MapStyle::find(qsl("tiles"));
        return 2.0f / tiles->roomScale / tiles->roomFlattening;
    }

    static bool allZ(const QVector<float>& vertices, const std::function<bool(float)>& test)
    {
        if (vertices.isEmpty()) {
            return false;
        }
        for (qsizetype i = 2; i < vertices.size(); i += 3) {
            if (!test(vertices.at(i))) {
                return false;
            }
        }
        return true;
    }

    static float spanX(const QVector<float>& vertices)
    {
        float lowest = std::numeric_limits<float>::max();
        float highest = std::numeric_limits<float>::lowest();
        for (qsizetype i = 0; i < vertices.size(); i += 3) {
            lowest = std::min(lowest, vertices.at(i));
            highest = std::max(highest, vertices.at(i));
        }
        return highest - lowest;
    }

    static QPoint pixelOf(ModernGLWidget& widget, const QVector3D& point)
    {
        const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(point);
        return pixel ? pixel->toPoint() : QPoint(-1, -1);
    }

    static void pointAt(ModernGLWidget& widget, const QPoint& pointer)
    {
        widget.mPointerPosition = pointer;
        widget.mPointerInView = true;
        widget.mPointerButtons = Qt::NoButton;
    }

    static bool seeThrough(const RenderCommand* command)
    {
        if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command)) {
            return std::any_of(cubes->instances().cbegin(), cubes->instances().cend(), [](const CubeInstanceData& cube) {
                return cube.color[3] < 1.0f;
            });
        }
        if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command)) {
            for (qsizetype i = 3; i < triangles->colors().size(); i += 4) {
                if (triangles->colors().at(i) < 1.0f) {
                    return true;
                }
            }
        }
        return false;
    }

    // Where the clear room beside the symbol is colored in, or -1
    static qsizetype clearRoomColored(const ModernGLWidget& widget)
    {
        GLDrawState draw;
        qsizetype index = 0;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get())) {
                draw.follow(state);
            } else if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && draw.colorWrite && seeThrough(cubes)) {
                if (std::any_of(cubes->instances().cbegin(), cubes->instances().cend(), [](const CubeInstanceData& cube) {
                        const QVector4D center = cube.transform.column(3);
                        return qFuzzyCompare(center.x(), 2.0f) && qFuzzyIsNull(center.y()) && center.z() < 0.5f;
                    })) {
                    return index;
                }
            }
            ++index;
        }
        return -1;
    }

    // The room with a border above the symbol, rather than the exit beside it
    static bool theBorderedRoomAbove(const RenderInstancedCubesCommand* cubes)
    {
        return std::any_of(cubes->instances().cbegin(), cubes->instances().cend(), [](const CubeInstanceData& cube) {
            const QVector4D center = cube.transform.column(3);
            return qFuzzyIsNull(center.x()) && qFuzzyIsNull(center.y()) && center.z() > 0.5f;
        });
    }

    // Colored while writing depth, a see-through block would hide its own far faces, and what is behind it
    // on its level, from its own blend
    void test_nothingSeeThroughIsColoredWhileWritingDepth()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);

        GLDrawState draw;
        int seeThroughDraws = 0;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get())) {
                draw.follow(state);
                continue;
            }
            if (seeThrough(command.get())) {
                ++seeThroughDraws;
                QVERIFY2(!(draw.depthWrite && draw.colorWrite), qPrintable(qsl("a see-through %1 is colored while writing depth").arg(QLatin1String(command->getCommandName()))));
            }
        }
        QVERIFY2(seeThroughDraws > 0, "nothing see-through was drawn");
        QVERIFY2(draw.restored(), "the frame leaves depth or color writes off, or the depth test changed");
    }

    // From the lowest level up, each level with what is on it, so a symbol below shows only through the room above
    void test_eachLevelIsDrawnWithWhatIsOnItBeforeTheLevelAbove()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);

        GLDrawState draw;
        qsizetype index = 0;
        qsizetype lastLowerDecal = -1;
        qsizetype upperExit = -1;
        qsizetype upperDepthOnly = -1;
        qsizetype upperColored = -1;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get())) {
                draw.follow(state);
            } else if (const auto* decal = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get())) {
                if (allZ(decal->geometry().vertices, [](const float z) {
                        return z < 0.5f;
                    })) {
                    lastLowerDecal = index;
                }
            } else if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && seeThrough(cubes)) {
                const bool exitAbove = std::any_of(cubes->instances().cbegin(), cubes->instances().cend(), [](const CubeInstanceData& cube) {
                    const QVector4D center = cube.transform.column(3);
                    return qFuzzyCompare(center.x(), 1.0f) && qFuzzyIsNull(center.y()) && center.z() > 0.5f;
                });
                if (exitAbove && upperExit < 0) {
                    upperExit = index;
                }
                if (theBorderedRoomAbove(cubes)) {
                    if (draw.depthWrite && !draw.colorWrite && upperDepthOnly < 0) {
                        upperDepthOnly = index;
                    } else if (draw.colorWrite && upperColored < 0) {
                        upperColored = index;
                    }
                }
            }
            ++index;
        }
        QVERIFY2(lastLowerDecal >= 0, "nothing was drawn on the viewed level's rooms");
        QVERIFY2(upperColored >= 0, "the level above was not drawn");
        QVERIFY2(upperExit >= 0, "the exit between the rooms above is not drawn");
        QVERIFY2(lastLowerDecal < upperColored, "something on the viewed level's rooms is drawn over the level above");
        // Each pixel then takes the color of the nearest face of the see-through blocks only, not of those behind it too
        QVERIFY2(upperDepthOnly >= 0 && upperDepthOnly < upperColored, "the level above's see-through blocks are not first drawn into the depth buffer alone");
        QVERIFY2(lastLowerDecal < upperExit && upperExit < upperColored, "the exit above is not drawn with its own level");
        QVERIFY2(draw.restored(), "the frame leaves depth or color writes off, or the depth test changed");
    }

    // Names, highlights and the player's ring hang over a room's edges onto the floor, which must not wash them out
    void test_theFloorsAreDrawnUnderWhatIsOnTheViewedLevelsRooms()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);

        const float plateZ = -roomHeight();
        qsizetype index = 0;
        qsizetype plate = -1;
        qsizetype firstDecal = -1;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get())) {
                if (triangles->vertices().size() == 18 && allZ(triangles->vertices(), [plateZ](const float z) {
                        return qFuzzyCompare(z, plateZ);
                    })) {
                    plate = index;
                }
            } else if (const auto* decal = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get()); decal && firstDecal < 0) {
                if (allZ(decal->geometry().vertices, [](const float z) {
                        return z > -0.1f && z < 0.5f;
                    })) {
                    firstDecal = index;
                }
            }
            ++index;
        }
        QVERIFY2(plate >= 0, "the viewed level's floor is not drawn");
        QVERIFY2(firstDecal >= 0, "nothing was drawn on the viewed level's rooms");
        QVERIFY2(plate < firstDecal, "the viewed level's floor is drawn over what is on its rooms");
    }

    // Seen from the level above the clear room, whose floor lies over it
    void test_aFloorIsDrawnOverTheSeeThroughRoomsBelowIt()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId, 1);

        const float plateZ = 1.0f - roomHeight();
        qsizetype index = 0;
        qsizetype plate = -1;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get())) {
                if (triangles->vertices().size() == 18 && allZ(triangles->vertices(), [plateZ](const float z) {
                        return qFuzzyCompare(z, plateZ);
                    })) {
                    plate = index;
                }
            }
            ++index;
        }
        const qsizetype clearRoom = clearRoomColored(widget);
        QVERIFY2(plate >= 0, "the viewed level's floor is not drawn");
        QVERIFY2(clearRoom >= 0, "the clear room below is not drawn");
        QVERIFY2(clearRoom < plate, "the see-through room below is drawn over the floor above it");
    }

    // A level above lays its rooms' depth down before coloring them, which would cut a label below out where it covers it
    void test_aLabelOnTopIsDrawnBeforeTheLevelAboveCoversIt_data()
    {
        QTest::addColumn<int>("upperLevel");
        QTest::addColumn<int>("labelLevel");
        QTest::newRow("on the viewed level") << 1 << 0;
        QTest::newRow("on an empty level between") << 2 << 1;
    }

    void test_aLabelOnTopIsDrawnBeforeTheLevelAboveCoversIt()
    {
        QFETCH(int, upperLevel);
        QFETCH(int, labelLevel);
        const int areaId = addLayeredArea(upperLevel, labelLevel);
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);

        const float labelZ = static_cast<float>(labelLevel) + roomHeight() / 2.0f;
        GLDrawState draw;
        qsizetype index = 0;
        qsizetype label = -1;
        qsizetype upperDepthOnly = -1;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get())) {
                draw.follow(state);
            } else if (const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get())) {
                const QVector<float>& vertices = textured->geometry().vertices;
                if (spanX(vertices) > 0.9f && allZ(vertices, [labelZ](const float z) {
                        return qFuzzyCompare(z, labelZ);
                    })) {
                    label = index;
                }
            } else if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && upperDepthOnly < 0 && theBorderedRoomAbove(cubes)) {
                if (draw.depthWrite && !draw.colorWrite) {
                    upperDepthOnly = index;
                }
            }
            ++index;
        }
        QVERIFY2(label >= 0, "the label is not drawn");
        QVERIFY2(upperDepthOnly >= 0, "the level above's depth is not laid down");
        QVERIFY2(label < upperDepthOnly, "the label is drawn after the level above has laid its depth down over it");
    }

    // Shown on top means over the see-through rooms of its own level too
    void test_aLabelOnTopIsDrawnOverTheSeeThroughRoomsOfItsLevel()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);

        const float labelZ = roomHeight() / 2.0f;
        qsizetype index = 0;
        qsizetype label = -1;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get())) {
                const QVector<float>& vertices = textured->geometry().vertices;
                if (spanX(vertices) > 0.9f && allZ(vertices, [labelZ](const float z) {
                        return qFuzzyCompare(z, labelZ);
                    })) {
                    label = index;
                }
            }
            ++index;
        }
        const qsizetype clearRoom = clearRoomColored(widget);
        QVERIFY2(label >= 0, "the label is not drawn");
        QVERIFY2(clearRoom >= 0, "the clear room is not drawn");
        QVERIFY2(clearRoom < label, "the see-through room on the label's level is drawn over the label");
    }

    // The pointer stays still while a key or the wheel moves the view under it
    void test_theRaisedRoomFollowsTheViewMovingUnderAStillPointer()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);
        pointAt(widget, pixelOf(widget, QVector3D(2.0f, 0.0f, 0.0f)));
        widget.mRenderCommandQueue.clear();
        widget.renderRooms();
        QCOMPARE(widget.hoveredRoomId(), 2);

        widget.setCameraPosition(8.0f, 40.0f, CameraController::scmNorthUpAzimuth);
        widget.mRenderCommandQueue.clear();
        widget.renderRooms();
        QCOMPARE(widget.hoveredRoomId(), 0);
    }

    // The raised room leaves bare the near side it shows when resting, at a slant
    void test_theRoomUnderThePointerStaysRaisedOnItsNearSide()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);
        // On the side facing the camera, which is to the south, just under the middle of the room's height
        pointAt(widget, pixelOf(widget, QVector3D(0.0f, -0.25f, -0.01f)));
        QCOMPARE(widget.roomAt(widget.mPointerPosition), 1);
        for (int frame = 0; frame < 3; ++frame) {
            widget.mRenderCommandQueue.clear();
            widget.renderRooms();
            QVERIFY2(widget.hoveredRoomId() == 1, qPrintable(qsl("the room is not raised in frame %1").arg(frame)));
        }
    }

    void test_roomIdDigitsKeepTheirTexturesInAGLFrame()
    {
        // Probed first, so that where there is no GL no widget ever tries for a context
        if (QOpenGLContext probe; !probe.create()) {
            QSKIP("no OpenGL context on this platform");
        }
        const int areaId = addCrowdedArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        widget.resize(400, 300);
        widget.grabFramebuffer();
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        // A context that will not draw, such as one whose shaders do not compile, is a failure
        QVERIFY2(widget.failureMessage().isEmpty(), qPrintable(widget.failureMessage()));
        verifyDigitsKept(widget);
    }
};

#include "ModernRoomTextureTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernRoomTextureTest)
