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
 * The modern 3D view draws what the 2D map's mapper settings ask for: the grid,
 * round rooms and the upper and lower levels' colors, and it leaves out room IDs
 * that would be unreadable or overlap one kept before them, as it does names.
 *
 * Why not a spec: no Lua call reports what the 3D view drew. The "WithoutGL" cases
 * queue a frame with stand-in textures and read the queued commands, so ctest's
 * offscreen platform runs them; the others read frames back from GL, which is
 * skipped where there is none, and run on a display:
 *   QT_QPA_PLATFORM=xcb MUDLET_TEST_MODE=1 ./functional_map_tests ModernMapperRenderTest
 */

#include <algorithm>
#include <numbers>

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
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ModernMapperRenderTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ModernMapperRender-Test");
    static constexpr int kPlayerRoomId = 1;
    static constexpr int kEastRoomId = 2;
    static constexpr int kLowerRoomId = 3;
    // As renderRooms sizes it: 2 / scale (4) / zFlattening (8)
    static constexpr float kRoomHeight = 2.0f / 4.0f / 8.0f;

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

    TMap* map() const { return mpHost->mpMap.data(); }

    bool addRoomAt(const int id, const int areaId, const int x, const int y, const int z) const
    {
        return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, z);
    }

    // The player's room and one east of it on level 0, and one on the level below, north of them
    int addArea() const
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Render Area"));
        if (!(addRoomAt(kPlayerRoomId, areaId, 0, 0, 0) && addRoomAt(kEastRoomId, areaId, 2, 0, 0) && addRoomAt(kLowerRoomId, areaId, 0, 2, -1))) {
            return 0;
        }
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        return areaId;
    }

    // Probed first, so that where there is no GL no widget ever tries for a context
    static bool glAvailable()
    {
        QOpenGLContext probe;
        return probe.create();
    }

    // Looking straight down on the player's room from a fixed distance
    static void lookDown(ModernGLWidget& widget)
    {
        widget.resize(600, 600);
        widget.grabFramebuffer();
        widget.mCameraController.setTopView();
        // About a hundred pixels to a room's spacing
        widget.setCameraDistance(0.6f);
    }

    // The color of the frame where this point of the map shows
    static QColor colorAt(ModernGLWidget& widget, const QVector3D& point)
    {
        const QImage frame = widget.grabFramebuffer();
        const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(point);
        if (!pixel || !frame.rect().contains(pixel->toPoint())) {
            return QColor();
        }
        return frame.pixelColor(pixel->toPoint());
    }

    static int difference(const QColor& a, const QColor& b) { return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue()); }

    // The commands one frame queues, at a slant, with textures that need no GL context
    static void queueFrame(ModernGLWidget& widget, const int areaId, const int z = 0)
    {
        widget.mLabelTextureCache.useTextureStandIn(
                [lastTexture = GLuint{0}](const QImage&) mutable {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 0, 0, z);
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.setCameraPosition(1.0f, 40.0f, CameraController::scmNorthUpAzimuth);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
    }

    // The index of the triangles drawn all in this color, or -1
    static qsizetype trianglesIn(const ModernGLWidget& widget, const QColor& color)
    {
        const auto& commands = widget.mRenderCommandQueue.commands();
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(commands.at(index).get());
            if (!triangles || triangles->colors().isEmpty()) {
                continue;
            }
            bool all = true;
            for (qsizetype i = 0; all && i + 3 < triangles->colors().size(); i += 4) {
                all = qFuzzyCompare(triangles->colors().at(i), static_cast<float>(color.redF())) && qFuzzyCompare(triangles->colors().at(i + 1), static_cast<float>(color.greenF()))
                      && qFuzzyCompare(triangles->colors().at(i + 2), static_cast<float>(color.blueF())) && qFuzzyCompare(triangles->colors().at(i + 3), static_cast<float>(color.alphaF()));
            }
            if (all) {
                return index;
            }
        }
        return -1;
    }

    // The room-sized instances, not exit bars or the beacon, standing at a room's place
    static QList<CubeInstanceData> roomInstances(const RenderInstancedCubesCommand& cubes, const QPointF& place)
    {
        QList<CubeInstanceData> found;
        for (const CubeInstanceData& cube : cubes.instances()) {
            const QVector4D center = cube.transform.column(3);
            if (cube.transform.column(0).toVector3D().length() > 0.2f && qFuzzyCompare(center.x() + 1.0f, static_cast<float>(place.x()) + 1.0f)
                && qFuzzyCompare(center.y() + 1.0f, static_cast<float>(place.y()) + 1.0f)) {
                found.append(cube);
            }
        }
        return found;
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
        mpHost->mBgColor_2 = QColor(Qt::black);
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

    // As on the 2D map, the grid's lines run along whole coordinates, here between the two rooms
    void test_theGridIsDrawnWhenShown()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY2(widget.failureMessage().isEmpty(), qPrintable(widget.failureMessage()));
        mpHost->mMapGridColor = QColor(Qt::white);
        mpHost->mMapGridLineSize = 5.0;
        const QVector3D onALine(1.0f, 1.5f, -kRoomHeight / 2.0f);

        mpHost->mMapperShowGrid = false;
        const QColor hidden = colorAt(widget, onALine);
        mpHost->mMapperShowGrid = true;
        const QColor shown = colorAt(widget, onALine);
        mpHost->mMapperShowGrid = false;
        QVERIFY(hidden.isValid() && shown.isValid());
        QVERIFY2(shown.lightness() - hidden.lightness() > 100, qPrintable(qsl("the grid line is %1 shown and %2 hidden").arg(shown.name(), hidden.name())));
    }

    // Round rooms are cylinders, so a square room's corner is background on a round one
    void test_roundRoomsLeaveTheirCornersEmpty()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const QVector3D corner(2.22f, 0.22f, kRoomHeight / 2.0f);
        const QVector3D empty(2.4f, 0.4f, kRoomHeight / 2.0f);

        mpHost->mBubbleMode = false;
        const QColor square = colorAt(widget, corner);
        const QColor squareEmpty = colorAt(widget, empty);
        mpHost->mBubbleMode = true;
        const QColor round = colorAt(widget, corner);
        const QColor middle = colorAt(widget, QVector3D(2.0f, 0.0f, kRoomHeight / 2.0f));
        mpHost->mBubbleMode = false;
        QVERIFY(square.isValid() && squareEmpty.isValid() && round.isValid() && middle.isValid());
        QVERIFY2(difference(square, squareEmpty) > 60, qPrintable(qsl("a square room's corner is %1, beside it %2").arg(square.name(), squareEmpty.name())));
        QVERIFY2(difference(round, squareEmpty) < 30, qPrintable(qsl("a round room's corner is %1, beside it %2").arg(round.name(), squareEmpty.name())));
        QVERIFY2(difference(middle, squareEmpty) > 60, qPrintable(qsl("a round room's middle is %1, beside it %2").arg(middle.name(), squareEmpty.name())));
    }

    // With the upper and lower levels shown, as on the 2D map, the level below takes on the lower level color
    void test_otherLevelsTakeOnTheUpperAndLowerLevelColors()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const QColor savedLower = mpHost->mLowerLevelColor;
        const bool savedShown = mudlet::self()->drawUpperLowerLevels();
        mpHost->mLowerLevelColor = QColor(Qt::red);
        const QVector3D lowerRoom(0.0f, 2.0f, -1.0f + kRoomHeight / 2.0f);

        mudlet::self()->setDrawUpperLowerLevels(false);
        const QColor plain = colorAt(widget, lowerRoom);
        mudlet::self()->setDrawUpperLowerLevels(true);
        const QColor colored = colorAt(widget, lowerRoom);
        mudlet::self()->setDrawUpperLowerLevels(savedShown);
        mpHost->mLowerLevelColor = savedLower;
        QVERIFY(plain.isValid() && colored.isValid());
        const int plainRedness = plain.red() - plain.green();
        const int coloredRedness = colored.red() - colored.green();
        QVERIFY2(coloredRedness - plainRedness > 30, qPrintable(qsl("the lower room is %1 with the levels' colors and %2 without").arg(colored.name(), plain.name())));
    }

    // As on the 2D map the grid is drawn first, so the other levels and the labels beneath the rooms, which
    // write no depth either, paint over it
    void test_theGridIsQueuedUnderEverythingWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        TMapLabel label;
        label.pos = QVector3D(1.0f, 1.0f, 0.0f);
        label.size = QSizeF(1.0, 1.0);
        label.pix = QPixmap(8, 8);
        label.pix.fill(Qt::white);
        label.showOnTop = false;
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(1, label);
        const QColor savedColor = mpHost->mMapGridColor;
        const bool savedShown = mpHost->mMapperShowGrid;
        const QColor gridColor(10, 20, 30, 200);
        mpHost->mMapGridColor = gridColor;
        ModernGLWidget widget(map(), mpHost);

        mpHost->mMapperShowGrid = false;
        queueFrame(widget, areaId);
        const qsizetype hidden = trianglesIn(widget, gridColor);
        mpHost->mMapperShowGrid = true;
        queueFrame(widget, areaId);
        const qsizetype grid = trianglesIn(widget, gridColor);
        qsizetype firstOther = -1;
        bool gridAtTheRoomsBottoms = false;
        bool writesNoDepth = false;
        const auto& commands = widget.mRenderCommandQueue.commands();
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            const RenderCommand* command = commands.at(index).get();
            if (firstOther < 0 && (dynamic_cast<const RenderInstancedCubesCommand*>(command) || dynamic_cast<const RenderTexturedTrianglesCommand*>(command))) {
                firstOther = index;
            }
            if (index == grid) {
                const auto* triangles = static_cast<const RenderTrianglesCommand*>(command);
                gridAtTheRoomsBottoms = !triangles->vertices().isEmpty();
                for (qsizetype i = 2; i < triangles->vertices().size(); i += 3) {
                    gridAtTheRoomsBottoms = gridAtTheRoomsBottoms && qFuzzyCompare(triangles->vertices().at(i), -kRoomHeight / 2.0f);
                }
                const auto* before = index > 0 ? dynamic_cast<const GLStateCommand*>(commands.at(index - 1).get()) : nullptr;
                const auto* after = index + 1 < static_cast<qsizetype>(commands.size()) ? dynamic_cast<const GLStateCommand*>(commands.at(index + 1).get()) : nullptr;
                writesNoDepth = before && before->stateType() == GLStateCommand::DISABLE_DEPTH_WRITE && after && after->stateType() == GLStateCommand::ENABLE_DEPTH_WRITE;
            }
        }
        // A level with no rooms has its grid too
        queueFrame(widget, areaId, 5);
        const qsizetype onAnEmptyLevel = trianglesIn(widget, gridColor);
        mpHost->mMapGridColor = QColor(10, 20, 30, 0);
        queueFrame(widget, areaId);
        const qsizetype clear = trianglesIn(widget, QColor(10, 20, 30, 0));
        mpHost->mMapGridColor = savedColor;
        mpHost->mMapperShowGrid = savedShown;

        QCOMPARE(hidden, -1);
        QVERIFY2(grid >= 0, "the grid was not drawn");
        QVERIFY2(firstOther >= 0, "neither rooms nor labels were drawn");
        QVERIFY2(grid < firstOther, "the grid is drawn over rooms or labels");
        QVERIFY2(gridAtTheRoomsBottoms, "the grid is not at the viewed level's room bottoms");
        QVERIFY2(writesNoDepth, "the grid writes depth, so the levels below cannot paint over it");
        QVERIFY2(onAnEmptyLevel >= 0, "a level with no rooms has no grid");
        QCOMPARE(clear, -1);
    }

    // However low the camera looks, the grid reaches no further than the fog
    void test_theGridStopsShortOfTheHorizonWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const bool savedShown = mpHost->mMapperShowGrid;
        mpHost->mMapperShowGrid = true;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        // Just short of where the top of the view, half the field of view above its middle, meets the horizon,
        // its ray meets the grid hundreds of eye distances away
        constexpr float grazing = 90.0f - CameraController::scmFieldOfView / 2.0f - 0.1f;
        QList<std::pair<float, float>> views{{1.0f, CameraController::scmMaxTilt},
                                             {CameraController::scmMaxDistance, CameraController::scmMaxTilt},
                                             {CameraController::scmMaxDistance, CameraController::scmMinTilt},
                                             {1.0f, grazing},
                                             {CameraController::scmMaxDistance, grazing}};
        for (const auto& [distance, tilt] : views) {
            widget.setCameraPosition(distance, tilt, CameraController::scmNorthUpAzimuth);
            widget.updateMatrices();
            widget.mRenderCommandQueue.clear();
            widget.queueGrid();
            const QVector3D target = widget.mCameraController.getTarget();
            const qreal eyeDistance = widget.mCameraController.rayThroughPixel(QPointF(widget.width() / 2.0, widget.height() / 2.0)).first.distanceToPoint(target);
            const RenderTrianglesCommand* grid = nullptr;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get())) {
                    grid = triangles;
                }
            }
            QVERIFY2(grid && !grid->vertices().isEmpty(), qPrintable(qsl("no grid at distance %1, tilt %2").arg(distance).arg(tilt)));
            qreal farthest = 0.0;
            for (qsizetype i = 0; i + 1 < grid->vertices().size(); i += 3) {
                farthest = qMax(farthest, QLineF(QPointF(target.x(), target.y()), QPointF(grid->vertices().at(i), grid->vertices().at(i + 1))).length());
            }
            // The fog ends at four eye distances; the lines run on to the next whole coordinate past it
            QVERIFY2(farthest < eyeDistance * 4.0 * std::numbers::sqrt2 + 2.0,
                     qPrintable(qsl("at distance %1, tilt %2 the grid reaches %3 for an eye distance of %4").arg(distance).arg(tilt).arg(farthest).arg(eyeDistance)));
        }
        mpHost->mMapperShowGrid = savedShown;
    }

    // Every room's block is a cylinder with round rooms, the see-through levels' and the borders' too
    void test_roundRoomsAreQueuedAsCylindersWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        // Above the viewed level, so see-through
        QVERIFY(map()->setRoomCoordinates(kLowerRoomId, 0, 2, 1));
        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderColor = QColor(Qt::red);
        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderThickness = 4;
        const bool savedBorders = mpHost->mMapperShowRoomBorders;
        const bool savedRound = mpHost->mBubbleMode;
        mpHost->mMapperShowRoomBorders = true;
        ModernGLWidget widget(map(), mpHost);
        const auto shapesOfRooms = [&widget]() {
            QList<GeometryManager::InstanceShape> shapes;
            int roomBlocks = 0;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get());
                if (!cubes) {
                    continue;
                }
                const qsizetype blocks = roomInstances(*cubes, QPointF(0.0, 0.0)).size() + roomInstances(*cubes, QPointF(2.0, 0.0)).size() + roomInstances(*cubes, QPointF(0.0, 2.0)).size();
                if (blocks) {
                    shapes.append(cubes->shape());
                    roomBlocks += blocks;
                }
            }
            return std::make_pair(shapes, roomBlocks);
        };

        mpHost->mBubbleMode = true;
        queueFrame(widget, areaId);
        const auto [roundShapes, roundBlocks] = shapesOfRooms();
        mpHost->mBubbleMode = false;
        queueFrame(widget, areaId);
        const auto [squareShapes, squareBlocks] = shapesOfRooms();
        mpHost->mBubbleMode = savedRound;
        mpHost->mMapperShowRoomBorders = savedBorders;

        // The two rooms and their borders on the viewed level, and the room above twice: its depth, then its color
        QVERIFY2(roundBlocks >= 6, qPrintable(qsl("only %1 room blocks were drawn").arg(roundBlocks)));
        QCOMPARE(squareBlocks, roundBlocks);
        QVERIFY2(!roundShapes.contains(GeometryManager::InstanceShape::Box), "a round room's block is drawn as a box");
        QVERIFY2(!squareShapes.contains(GeometryManager::InstanceShape::Cylinder), "a square room's block is drawn as a cylinder");
    }

    // With the upper and lower levels shown, the level below and its floor fade toward the lower level color
    void test_otherLevelsFadeTowardTheLevelColorsWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const QColor savedLower = mpHost->mLowerLevelColor;
        const bool savedShown = mudlet::self()->drawUpperLowerLevels();
        mpHost->mLowerLevelColor = QColor(255, 0, 0, 64);
        ModernGLWidget widget(map(), mpHost);
        const float floorZ = -1.0f - kRoomHeight;
        const auto redness = [&widget, floorZ]() {
            float room = std::numeric_limits<float>::lowest();
            float floor = std::numeric_limits<float>::lowest();
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get())) {
                    for (const CubeInstanceData& cube : roomInstances(*cubes, QPointF(0.0, 2.0))) {
                        room = qMax(room, cube.color[0] - cube.color[1]);
                    }
                } else if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get()); triangles && triangles->vertices().size() == 18) {
                    if (qFuzzyCompare(triangles->vertices().at(2), floorZ)) {
                        floor = triangles->colors().at(0) - triangles->colors().at(1);
                    }
                }
            }
            return std::make_pair(room, floor);
        };

        mudlet::self()->setDrawUpperLowerLevels(false);
        queueFrame(widget, areaId);
        const auto [plainRoom, plainFloor] = redness();
        const QColor plainTarget = widget.levelFadeTarget(-1);
        mudlet::self()->setDrawUpperLowerLevels(true);
        queueFrame(widget, areaId);
        const auto [coloredRoom, coloredFloor] = redness();
        const QColor lowerTarget = widget.levelFadeTarget(-1);
        const QColor upperTarget = widget.levelFadeTarget(1);
        const QColor viewedTarget = widget.levelFadeTarget(0);
        mudlet::self()->setDrawUpperLowerLevels(savedShown);
        mpHost->mLowerLevelColor = savedLower;

        const QColor background = mpHost->mapStylePreset().backgroundColor(mpHost->mBgColor_2);
        QCOMPARE(plainTarget, background);
        QCOMPARE(lowerTarget, GeometryManager::levelFadeTarget(-1, background, mpHost->mUpperLevelColor, QColor(255, 0, 0, 64)));
        QCOMPARE(upperTarget, GeometryManager::levelFadeTarget(1, background, mpHost->mUpperLevelColor, QColor(255, 0, 0, 64)));
        QCOMPARE(viewedTarget, background);
        QVERIFY2(plainRoom > std::numeric_limits<float>::lowest() && plainFloor > std::numeric_limits<float>::lowest(), "the room or floor below was not drawn");
        QVERIFY2(coloredRoom > plainRoom + 0.02f, qPrintable(qsl("the room below is %1 red with the levels' colors and %2 without").arg(coloredRoom).arg(plainRoom)));
        QVERIFY2(coloredFloor > plainFloor + 0.002f, qPrintable(qsl("the floor below is %1 red with the levels' colors and %2 without").arg(coloredFloor).arg(plainFloor)));
    }

    // As names are: where IDs overlap one is kept, the viewed level's as only it has IDs on the 2D map,
    // and none so small as to be unreadable. As on the 2D map, a room whose ID is left out shows its symbol
    void test_roomIdsAreDeclutteredWithoutGL()
    {
        constexpr int kAboveRoomId = 4;
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Stacked Area"));
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0, 0) && addRoomAt(kAboveRoomId, areaId, 0, 0, 1));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->mSymbol = qsl("A");
        map()->mpRoomDB->getRoom(kAboveRoomId)->mSymbol = qsl("B");
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        mpHost->mShowRoomID = true;
        ModernGLWidget widget(map(), mpHost);
        const auto symbolDrawn = [&widget](const QString& symbol) {
            return std::any_of(widget.mRoomTexturesInUse.cbegin(), widget.mRoomTexturesInUse.cend(), [&symbol](const QString& key) {
                return key.startsWith(qsl("symbol|")) && key.endsWith(qsl("|%1").arg(symbol));
            });
        };
        GLuint lastTexture = 0;
        widget.mLabelTextureCache.useTextureStandIn(
                [&lastTexture](const QImage&) {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.resize(600, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.frameRooms(3.0);
        // Straight down, so the room above overlaps the viewed one and is nearer
        widget.mCameraController.setTopView();
        widget.updateMatrices();
        widget.renderRooms();
        const QSet<int> overlapping = widget.mRoomIdsDrawn;
        const bool viewedSymbolWithItsId = symbolDrawn(qsl("A"));
        const bool aboveSymbolWithoutItsId = symbolDrawn(qsl("B"));

        map()->setRoomCoordinates(kAboveRoomId, 1, 0, 1);
        widget.renderRooms();
        const QSet<int> apart = widget.mRoomIdsDrawn;
        const bool symbolWithAnId = symbolDrawn(qsl("A")) || symbolDrawn(qsl("B"));

        widget.mCameraController.setScale(CameraController::scmMaxDistance);
        widget.updateMatrices();
        widget.renderRooms();
        const QSet<int> tooSmall = widget.mRoomIdsDrawn;
        const bool symbolsWithoutIds = symbolDrawn(qsl("A")) && symbolDrawn(qsl("B"));
        mpHost->mShowRoomID = false;

        QCOMPARE(overlapping, QSet<int>{kPlayerRoomId});
        QVERIFY2(!viewedSymbolWithItsId, "a room showing its ID shows its symbol too");
        QVERIFY2(aboveSymbolWithoutItsId, "the room whose ID was left out shows no symbol");
        QCOMPARE(apart, (QSet<int>{kPlayerRoomId, kAboveRoomId}));
        QVERIFY2(!symbolWithAnId, "a room showing its ID shows its symbol too");
        QVERIFY2(tooSmall.isEmpty(), "IDs too small to read were drawn");
        QVERIFY2(symbolsWithoutIds, "rooms whose IDs are too small to read show no symbols");
    }
};

#include "ModernMapperRenderTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperRenderTest)
