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
#include <memory>
#include <numbers>

#include <QFileInfo>
#include <QOpenGLContext>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "EnvironmentKit.h"
#include "ExitAppearance.h"
#include "GeometryManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MapStyle.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "RenderCommand.h"
#include "RoomAppearance.h"
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
    // As renderRooms sizes the default style's rooms
    static float roomHeight()
    {
        const MapStyle* tiles = MapStyle::find(qsl("tiles"));
        return 2.0f / tiles->roomScale / tiles->roomFlattening;
    }

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

    struct DrawnGlyph
    {
        QPointF center;
        QSizeF size;
        QString text;
        QColor color;
    };

    // Seen from above, every quad the terminal style draws from its glyph atlas, with the character its cell held when
    // the texture was drawn; a symbol's underline and a reverse-video cell are drawn from the solid block
    static QList<DrawnGlyph> terminalGlyphs(ModernGLWidget& widget, const int areaId)
    {
        auto atlasTextures = std::make_shared<QHash<GLuint, QStringList>>();
        auto lastTexture = std::make_shared<GLuint>(0);
        widget.mLabelTextureCache.useTextureStandIn(
                [&widget, atlasTextures, lastTexture](const QImage& image) {
                    ++*lastTexture;
                    if (image.size() == QSize(1024, 1024)) {
                        atlasTextures->insert(*lastTexture, widget.mGlyphAtlasCells);
                    }
                    return *lastTexture;
                },
                [](GLuint) {});
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        // Drawn again, so that the stand-in sees which character each cell holds
        widget.mGlyphAtlasDrawnCells = -1;
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.mCameraController.setTopView();
        widget.setCameraDistance(1.0f);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.renderRooms();
        QList<DrawnGlyph> glyphs;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get());
            if (!textured || !atlasTextures->contains(textured->geometry().textureId)) {
                continue;
            }
            const GeometryData& batch = textured->geometry();
            const QStringList& cells = atlasTextures->value(batch.textureId);
            for (qsizetype quad = 0; quad * 18 + 8 < batch.vertices.size() && quad * 12 + 5 < batch.textureCoords.size() && quad * 24 + 3 < batch.colors.size(); ++quad) {
                const float* corner = batch.vertices.constData() + quad * 18;
                const float* texture = batch.textureCoords.constData() + quad * 12;
                const float* color = batch.colors.constData() + quad * 24;
                const int cell = (15 - static_cast<int>((texture[1] + texture[5]) / 2.0f * 16.0f)) * 16 + static_cast<int>((texture[0] + texture[2]) / 2.0f * 16.0f);
                glyphs.append({QPointF((corner[0] + corner[6]) / 2.0f, (corner[1] + corner[7]) / 2.0f),
                               QSizeF(qAbs(corner[6] - corner[0]), qAbs(corner[7] - corner[1])),
                               cells.value(cell),
                               QColor::fromRgbF(color[0], color[1], color[2], color[3])});
            }
        }
        return glyphs;
    }

    // The character drawn for the room at this place, not its underline nor a cell behind it
    static DrawnGlyph glyphAt(const QList<DrawnGlyph>& glyphs, const QPointF& place)
    {
        for (const DrawnGlyph& glyph : glyphs) {
            if (glyph.text != QString(QChar(0x2588)) && QLineF(glyph.center, place).length() < 0.05) {
                return glyph;
            }
        }
        return {};
    }

    static QList<DrawnGlyph> underlines(const QList<DrawnGlyph>& glyphs)
    {
        QList<DrawnGlyph> found;
        for (const DrawnGlyph& glyph : glyphs) {
            if (glyph.text == QString(QChar(0x2588)) && glyph.size.height() < glyph.size.width() / 2.0) {
                found.append(glyph);
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
        const QVector3D onALine(1.0f, 1.5f, -roomHeight() / 2.0f);

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
        const float halfRoom = 1.0f / MapStyle::find(qsl("tiles"))->roomScale;
        // The corner away from the view's middle, which sees the block's other sides, and from its shadow to the south east
        const QVector3D corner(2.0f + halfRoom * 0.9f, halfRoom * 0.9f, roomHeight() / 2.0f);
        // Between the rooms, on the level's floor
        const QVector3D empty(1.0f, halfRoom * 0.9f, roomHeight() / 2.0f);

        mpHost->mBubbleMode = false;
        const QColor square = colorAt(widget, corner);
        const QColor squareEmpty = colorAt(widget, empty);
        mpHost->mBubbleMode = true;
        const QColor round = colorAt(widget, corner);
        const QColor middle = colorAt(widget, QVector3D(2.0f, 0.0f, roomHeight() / 2.0f));
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
        const QVector3D lowerRoom(0.0f, 2.0f, -1.0f + roomHeight() / 2.0f);

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
                    gridAtTheRoomsBottoms = gridAtTheRoomsBottoms && qFuzzyCompare(triangles->vertices().at(i), -roomHeight() / 2.0f);
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
        const float floorZ = -1.0f - roomHeight();
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

    // Tiles keeps the profile's own colors, softened but not swapped for a palette of its own
    void test_tilesTakesTheProfilesColors()
    {
        const MapStyle* tiles = MapStyle::find(qsl("tiles"));
        QVERIFY(tiles);
        const QColor environment(10, 200, 30, 128);
        const QColor profileColor(1, 2, 3, 4);
        QCOMPARE(tiles->palette, MapStyle::Palette::Soft);
        QCOMPARE(tiles->roomColor(environment).alpha(), environment.alpha());
        QVERIFY(qAbs(tiles->roomColor(environment).hueF() - environment.hueF()) < 0.02f);
        QCOMPARE(tiles->backgroundColor(profileColor), profileColor);
        QCOMPARE(tiles->exitColor(profileColor), profileColor);
        QCOMPARE(tiles->symbolColor(QColor(), environment), RoomAppearance::symbolColor(QColor(), environment));
        QCOMPARE(tiles->symbolColor(QColor(Qt::yellow), environment), RoomAppearance::symbolColor(QColor(Qt::yellow), environment));
        QCOMPARE(tiles->roomIdColor(environment), ExitAppearance::contrastColor(environment));
        QCOMPARE(tiles->playerRoomColor(), QColor(Qt::red));
        QCOMPARE(tiles->doors, MapStyle::Doors::Strokes);
        QVERIFY(tiles->floorPlates);
        QVERIFY(!tiles->gridColor.isValid());
        QVERIFY(!tiles->ink.isValid());
        QCOMPARE(tiles->exitWidthFactor, 1.0f);
    }

    // Blueprint fills its rooms with little more than its sheet and draws them in its ink
    void test_blueprintDrawsInItsInk()
    {
        const MapStyle* blueprint = MapStyle::find(qsl("blueprint"));
        QVERIFY(blueprint);
        const QColor ink = blueprint->ink;
        QVERIFY(ink.isValid());
        const QColor environment(200, 40, 40, 200);
        const QColor profileColor(1, 2, 3);
        QCOMPARE(blueprint->exitColor(profileColor), ink);
        QCOMPARE(blueprint->roomIdColor(environment), ink);
        QCOMPARE(blueprint->symbolColor(QColor(), blueprint->roomColor(environment)), ink);
        // A room's own symbol color is the mapper's choice, not the automatic one
        QCOMPARE(blueprint->symbolColor(QColor(Qt::yellow), blueprint->roomColor(environment)), QColor(Qt::yellow));
        QCOMPARE(blueprint->backgroundColor(profileColor), blueprint->background);
        const QColor room = blueprint->roomColor(environment);
        const QColor sheet = blueprint->background;
        QVERIFY2(difference(room, sheet) < difference(room, environment), "a room is filled nearer its environment's color than the sheet's");
        QVERIFY2(room != sheet, "a room's environment does not show at all");
        QCOMPARE(room.alpha(), environment.alpha());
        QVERIFY(blueprint->playerRoomColor() != QColor(Qt::red));
        QCOMPARE(blueprint->doors, MapStyle::Doors::Swings);
    }

    // Outlines and room IDs in the ink in Blueprint, and none of either in Tiles
    void test_blueprintOutlinesRoomsInItsInkWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const bool savedIds = mpHost->mShowRoomID;
        const auto restore = qScopeGuard([this, savedIds]() {
            mpHost->setMapStyle(qsl("tiles"));
            mpHost->mShowRoomID = savedIds;
        });
        mpHost->mShowRoomID = true;
        ModernGLWidget widget(map(), mpHost);
        const QColor ink = MapStyle::find(qsl("blueprint"))->ink;
        const auto inkIdDrawn = [&widget, &ink]() {
            return std::any_of(widget.mRoomTexturesInUse.cbegin(), widget.mRoomTexturesInUse.cend(), [&ink](const QString& key) {
                return key.startsWith(qsl("digit|%1|").arg(ink.name()));
            });
        };

        queueFrame(widget, areaId);
        const qsizetype tilesOutlines = trianglesIn(widget, ink);
        const bool tilesInkIds = inkIdDrawn();
        QVERIFY(mpHost->setMapStyle(qsl("blueprint")));
        queueFrame(widget, areaId);
        const qsizetype blueprintOutlines = trianglesIn(widget, ink);
        const bool blueprintInkIds = inkIdDrawn();

        QCOMPARE(tilesOutlines, -1);
        QVERIFY2(!tilesInkIds, "tiles drew room IDs in the blueprint ink");
        QVERIFY2(blueprintOutlines >= 0, "blueprint queued no outlines in its ink");
        QVERIFY2(blueprintInkIds, "blueprint drew room IDs in another color than its ink");
    }

    // A blueprint room reads as line-work: its outline is drawn as wide as the style asks, and that is bolder than a
    // hairline
    void test_blueprintOutlinesRoomsAsBoldAsItsStyleAsksWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("blueprint")));
        const MapStyle* blueprint = MapStyle::find(qsl("blueprint"));
        QVERIFY(blueprint);
        QVERIFY(blueprint->inkOutlineWidth > MapStyle().inkOutlineWidth);
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);

        const auto level = widget.mInkOutlines.constFind(0);
        QVERIFY(level != widget.mInkOutlines.cend());
        int solid = 0;
        for (const auto& outline : level->outlines) {
            if (outline.dashLength > 0.0f) {
                continue;
            }
            ++solid;
            const float widthOfRoom = outline.width / (2.0f * outline.half);
            QVERIFY2(widthOfRoom >= blueprint->inkOutlineWidth * 0.99f, qPrintable(qsl("an outline %1 of its room's width, not %2").arg(widthOfRoom).arg(blueprint->inkOutlineWidth)));
        }
        QVERIFY(solid > 0);
    }

    // Blueprint's crosshair takes the place of the beacon, but where the profile's own marker is drawn instead the
    // beacon still helps find the player
    void test_blueprintDropsTheBeaconOnlyForItsOwnMarker()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const int savedRoomStyle = map()->mPlayerRoomStyle;
        const bool savedStrong = mpHost->mMapStrongHighlight;
        const auto restore = qScopeGuard([this, savedRoomStyle, savedStrong]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mPlayerRoomStyle = savedRoomStyle;
            mpHost->mMapStrongHighlight = savedStrong;
        });
        map()->mPlayerRoomStyle = 0;
        mpHost->mMapStrongHighlight = false;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        const auto beaconCommands = [&widget]() {
            widget.mRenderCommandQueue.clear();
            widget.renderPlayerBeacon();
            return std::count_if(widget.mRenderCommandQueue.commands().cbegin(), widget.mRenderCommandQueue.commands().cend(), [](const auto& command) {
                return dynamic_cast<const RenderInstancedCubesCommand*>(command.get()) != nullptr;
            });
        };

        const auto tilesBeacon = beaconCommands();
        QVERIFY(mpHost->setMapStyle(qsl("blueprint")));
        const auto blueprintBeacon = beaconCommands();
        map()->mPlayerRoomStyle = 1;
        const auto profileMarkerBeacon = beaconCommands();
        map()->mPlayerRoomStyle = 0;
        mpHost->mMapStrongHighlight = true;
        const auto strongHighlightBeacon = beaconCommands();

        QVERIFY2(tilesBeacon > 0, "tiles queued no beacon");
        QCOMPARE(blueprintBeacon, 0);
        QVERIFY2(profileMarkerBeacon > 0, "blueprint dropped the beacon though the profile's own player marker is drawn");
        QVERIFY2(strongHighlightBeacon > 0, "blueprint dropped the beacon though the profile's strong highlight is drawn");
    }

    // Outlines are kept from frame to frame, and pieced together from the ones that did not change, so after
    // each change they must be just what a widget with nothing kept makes
    void test_inkOutlinesFollowTheirRoomsWithoutGL()
    {
        constexpr int kFarEastRoomId = 4;
        constexpr int kFarthestEastRoomId = 5;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kFarEastRoomId, areaId, 4, 0, 0) && addRoomAt(kFarthestEastRoomId, areaId, 6, 0, 0));
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("blueprint")));
        ModernGLWidget widget(map(), mpHost);
        const auto triangles = [areaId](ModernGLWidget& from) {
            queueFrame(from, areaId);
            QVector<float> vertices;
            QVector<float> colors;
            for (const auto& command : from.mRenderCommandQueue.commands()) {
                if (const auto* drawn = dynamic_cast<const RenderTrianglesCommand*>(command.get())) {
                    vertices += drawn->vertices();
                    colors += drawn->colors();
                }
            }
            return std::make_pair(vertices, colors);
        };
        const auto keptMatchesMade = [this, &widget, &triangles]() {
            const auto kept = triangles(widget);
            ModernGLWidget made(map(), mpHost);
            return kept == triangles(made);
        };

        QVERIFY2(keptMatchesMade(), "the first frame's outlines differ from a fresh widget's");
        const auto first = triangles(widget);
        QVERIFY(!first.first.isEmpty());
        QVERIFY2(keptMatchesMade(), "the outlines kept unchanged differ from a fresh widget's");
        QCOMPARE(triangles(widget), first);

        QVERIFY(map()->setRoomCoordinates(kEastRoomId, 3, 0, 0));
        QVERIFY2(keptMatchesMade(), "after a room moved, the outlines differ from a fresh widget's");
        QVERIFY2(triangles(widget).first != first.first, "a room's outline stayed where the room was");

        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderThickness = 4;
        QVERIFY2(keptMatchesMade(), "after a border thickened, the outlines differ from a fresh widget's");

        const auto beforeColor = triangles(widget);
        map()->mpRoomDB->getRoom(kFarEastRoomId)->mBorderColor = QColor(255, 120, 120);
        QVERIFY2(keptMatchesMade(), "after a border's color changed, the outlines differ from a fresh widget's");
        QVERIFY2(triangles(widget).second != beforeColor.second, "a room's outline kept its color after its border's changed");

        // Then the room the outlines start with, so that both a kept first outline and a remade one are pieced onto
        const auto level = widget.mInkOutlines.constFind(0);
        QVERIFY(level != widget.mInkOutlines.cend() && !level->outlines.isEmpty());
        TRoom* pFirst = nullptr;
        for (const int roomId : {kPlayerRoomId, kEastRoomId, kFarEastRoomId, kFarthestEastRoomId}) {
            TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
            if (pRoom && static_cast<float>(pRoom->x()) == level->outlines.first().x) {
                pFirst = pRoom;
            }
        }
        QVERIFY(pFirst);
        pFirst->mBorderColor = QColor(120, 255, 120);
        QVERIFY2(keptMatchesMade(), "after the first room's border color changed, the outlines differ from a fresh widget's");
    }

    // Going to another style and back leaves tiles queueing just what it did before
    void test_tilesIsTheSameAfterAnotherStyleWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(map(), mpHost);
        const auto frame = [&widget, areaId]() {
            queueFrame(widget, areaId);
            QStringList commands;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                QString line = QString::fromLatin1(typeid(*command).name());
                if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get())) {
                    for (const float value : triangles->vertices() + triangles->colors()) {
                        line += qsl(" %1").arg(value);
                    }
                } else if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get())) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        for (int i = 0; i < 16; ++i) {
                            line += qsl(" %1").arg(cube.transform.constData()[i]);
                        }
                        line += qsl(" %1 %2 %3 %4").arg(cube.color[0]).arg(cube.color[1]).arg(cube.color[2]).arg(cube.color[3]);
                    }
                }
                commands.append(line);
            }
            return commands;
        };

        const QStringList before = frame();
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("blueprint")));
        const QStringList blueprint = frame();
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        const QStringList transit = frame();
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        const QStringList after = frame();

        QVERIFY(before != blueprint);
        QVERIFY(before != transit);
        QCOMPARE(after, before);
    }

    // Transit's stations are white, ringed in their environment's color where it stands out on its paper
    void test_transitDrawsStations()
    {
        const MapStyle* transit = MapStyle::find(qsl("transit"));
        QVERIFY(transit);
        const QColor environment(200, 40, 40, 200);
        QCOMPARE(transit->roomColor(environment), QColor(255, 255, 255, 200));
        QCOMPARE(transit->ringColor(environment, mpHost->mBgColor_2), environment);
        const QColor pale(250, 245, 200);
        QVERIFY2(ExitAppearance::brightness(transit->background) - ExitAppearance::brightness(transit->ringColor(pale, mpHost->mBgColor_2)) >= 75,
                 "a pale environment's ring does not stand out on the paper");
        QVERIFY(transit->roundRooms(false));
        QVERIFY(!transit->lit);
        QCOMPARE(transit->doors, MapStyle::Doors::Bars);
        QCOMPARE(transit->playerMarker, MapStyle::PlayerMarker::Flag);
        QVERIFY(transit->environmentLines);
        QVERIFY(transit->labelHalos);
        QVERIFY(transit->interchangeExits > 0);
        const MapStyle* tiles = MapStyle::find(qsl("tiles"));
        QVERIFY(tiles);
        QCOMPARE(tiles->playerMarker, MapStyle::PlayerMarker::Ring);
        QVERIFY(!tiles->environmentLines);
        QVERIFY(!tiles->labelHalos);
        QCOMPARE(tiles->interchangeExits, 0);
    }

    // A station is ringed in its environment's color, and an interchange in ink but no larger, as clicks and
    // the selection take a room's size; and the player's room has a flag rather than the profile's marker
    void test_transitRingsStationsAndFlagsThePlayerWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const QColor red(200, 30, 30);
        map()->mCustomEnvColors[301] = red;
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mCustomEnvColors.remove(301);
        });
        map()->mpRoomDB->getRoom(kEastRoomId)->environment = 301;
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        for (const int direction : {DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_DOWN}) {
            pPlayerRoom->setExit(kEastRoomId, direction);
        }
        ModernGLWidget widget(map(), mpHost);
        const auto colorQueued = [&widget](const QColor& color) {
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get());
                if (!triangles) {
                    continue;
                }
                const QVector<float>& colors = triangles->colors();
                for (qsizetype i = 0; i + 3 < colors.size(); i += 4) {
                    if (qAbs(colors.at(i) - color.redF()) < 0.01 && qAbs(colors.at(i + 1) - color.greenF()) < 0.01 && qAbs(colors.at(i + 2) - color.blueF()) < 0.01) {
                        return true;
                    }
                }
            }
            return false;
        };
        const auto flagDrawn = [&widget]() {
            return std::any_of(widget.mRoomTexturesInUse.cbegin(), widget.mRoomTexturesInUse.cend(), [](const QString& key) {
                return key.startsWith(qsl("flag|"));
            });
        };
        // A room's fill, the widest cube standing at its place, as its exits' bars stand between rooms
        const auto roomWidth = [this, &widget](const int roomId) {
            const TRoom* pRoom = map()->mpRoomDB->getRoom(roomId);
            float widest = 0.0f;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get())) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        const QVector4D center = cube.transform.column(3);
                        if (qAbs(center.x() - pRoom->x()) < 1e-4f && qAbs(center.y() - pRoom->y()) < 1e-4f) {
                            widest = qMax(widest, cube.transform.column(0).toVector3D().length());
                        }
                    }
                }
            }
            return widest;
        };

        queueFrame(widget, areaId);
        const bool tilesFlag = flagDrawn();
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        const MapStyle* transit = MapStyle::find(qsl("transit"));
        queueFrame(widget, areaId);

        QVERIFY2(!tilesFlag, "tiles flagged the player's room");
        QVERIFY2(flagDrawn(), "transit did not flag the player's room");
        QVERIFY2(colorQueued(transit->ringColor(red, mpHost->mBgColor_2)), "the station is not ringed in its environment's color");
        QVERIFY2(colorQueued(transit->ink), "the interchange is not ringed in ink");
        QVERIFY(roomWidth(kEastRoomId) > 0.0f);
        QCOMPARE(roomWidth(kPlayerRoomId), roomWidth(kEastRoomId));
    }

    // Zoomed out until a pixel is wider than a station, its ring fills it rather than turning inside out
    void test_transitRingsHoldTogetherFarOutWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        widget.mCameraController.setScale(CameraController::scmMaxDistance);
        widget.updateMatrices();
        widget.renderRooms();

        const auto level = widget.mInkOutlines.constFind(0);
        QVERIFY(level != widget.mInkOutlines.cend() && !level->outlines.isEmpty());
        for (const auto& outline : level->outlines) {
            QVERIFY2(outline.half - outline.width / 2.0f >= -1e-6f, qPrintable(qsl("a ring of width %1 around %2 turned inside out").arg(outline.width).arg(outline.half)));
        }
    }

    // However fine the label's ink, its halo covers everything within reach of it, and nothing past
    void test_labelHaloLeavesNoGapsAroundFineInk()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(map(), mpHost);
        QImage dot(200, 200, QImage::Format_ARGB32_Premultiplied);
        dot.fill(Qt::transparent);
        dot.setPixelColor(100, 100, QColor(Qt::black));
        const QColor halo(245, 243, 236);

        const auto& haloed = widget.haloedLabel(1, QPixmap::fromImage(dot), halo, 0);
        const QImage image = haloed.pixmap.toImage();
        const int reach = qRound(haloed.margin.y() * dot.height());
        QVERIFY(reach >= 3);
        QCOMPARE(image.size(), dot.size() + QSize(2 * reach, 2 * reach));
        const QPoint center(100 + reach, 100 + reach);
        int gaps = 0;
        int spill = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QPoint offset = QPoint(x, y) - center;
                const int squared = QPoint::dotProduct(offset, offset);
                const int alpha = image.pixelColor(x, y).alpha();
                if (squared <= (reach - 1) * (reach - 1) && alpha < 255) {
                    ++gaps;
                } else if (squared > (reach + 1) * (reach + 1) && alpha > 0) {
                    ++spill;
                }
            }
        }
        QCOMPARE(gaps, 0);
        QCOMPARE(spill, 0);
        QCOMPARE(image.pixelColor(center), QColor(Qt::black));
        QCOMPARE(image.pixelColor(center + QPoint(reach - 1, 0)), halo);
    }

    // On a translucent box the halo rims the letters, rather than growing a band all round the box
    void test_labelHaloRimsTheLettersNotTheirBox()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(map(), mpHost);
        constexpr int boxAlpha = 50;
        QImage box(200, 200, QImage::Format_ARGB32_Premultiplied);
        box.fill(QColor(0, 0, 0, boxAlpha));
        box.setPixelColor(100, 100, QColor(Qt::white));
        const QColor halo(245, 243, 236);

        const auto& haloed = widget.haloedLabel(1, QPixmap::fromImage(box), halo, boxAlpha);
        const QImage image = haloed.pixmap.toImage();
        const int reach = qRound(haloed.margin.y() * box.height());
        QVERIFY(reach >= 3);
        const QPoint center(100 + reach, 100 + reach);

        QCOMPARE(image.pixelColor(0, 0).alpha(), 0);
        QCOMPARE(image.pixelColor(center.x(), 0).alpha(), 0);
        QCOMPARE(image.pixelColor(center + QPoint(reach - 1, 0)).alpha(), 255);
        QCOMPARE(image.pixelColor(center), QColor(Qt::white));
    }

    // Halos of labels no longer on the map, or in another area, are let go even where the area shown has no labels
    void test_haloesOfLabelsGoneAreLetGoWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const int bareAreaId = map()->mpRoomDB->addArea(qsl("Bare Area"));
        QVERIFY(bareAreaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        TMapLabel label;
        label.text = qsl("Old Keep");
        label.pos = QVector3D(0.0f, 1.0f, 0.0f);
        label.size = QSizeF(2.0, 0.5);
        label.bgColor = QColor(0, 0, 0, 50);
        label.pix = QPixmap(80, 20);
        label.pix.fill(Qt::black);
        TArea* pArea = map()->mpRoomDB->getArea(areaId);
        pArea->mMapLabels.insert(1, label);
        pArea->mMapLabels.insert(2, label);
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        ModernGLWidget widget(map(), mpHost);

        queueFrame(widget, areaId);
        const qsizetype both = widget.mHaloedLabels.size();
        pArea->mMapLabels.remove(2);
        queueFrame(widget, areaId);
        const QList<std::pair<int, int>> kept = widget.mHaloedLabels.keys();
        queueFrame(widget, bareAreaId);

        QCOMPARE(both, 2);
        QCOMPARE(kept, (QList<std::pair<int, int>>{{areaId, 1}}));
        QVERIFY2(widget.mHaloedLabels.isEmpty(), "the halos of another area were kept where the area shown has no labels");
    }

    // A picture label, or text on an opaque box, is left as it is, while other text has a halo, so that lines do not
    // run through its letters
    void test_onlyTextLabelsHaveHalosWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        TMapLabel text;
        text.text = qsl("Old Keep");
        text.pos = QVector3D(0.0f, 1.0f, 0.0f);
        text.size = QSizeF(2.0, 0.5);
        text.bgColor = QColor(0, 0, 0, 50);
        text.pix = QPixmap(80, 20);
        text.pix.fill(Qt::black);
        TMapLabel picture = text;
        picture.text.clear();
        picture.pos = QVector3D(2.0f, 1.0f, 0.0f);
        picture.pix = QPixmap(2000, 2000);
        picture.pix.fill(Qt::black);
        TMapLabel boxed = text;
        boxed.pos = QVector3D(4.0f, 1.0f, 0.0f);
        boxed.bgColor = QColor(Qt::gray);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(1, text);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(2, picture);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(3, boxed);
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        ModernGLWidget widget(map(), mpHost);

        queueFrame(widget, areaId);
        const QList<std::pair<int, int>> haloed = widget.mHaloedLabels.keys();
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        queueFrame(widget, areaId);

        QCOMPARE(haloed, (QList<std::pair<int, int>>{{areaId, 1}}));
        QVERIFY2(widget.mHaloedLabels.isEmpty(), "halos were kept once no style showed them");
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

    void test_terminalStatusLineListsTheWaysOutWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int kTowerId = 4;
        QVERIFY(addRoomAt(kTowerId, areaId, 0, 0, 1));
        map()->mpRoomDB->getRoom(kTowerId)->name = qsl("tower");
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        pPlayerRoom->setExit(kEastRoomId, DIR_EAST);
        pPlayerRoom->setExit(kTowerId, DIR_UP);
        pPlayerRoom->setExit(kLowerRoomId, DIR_DOWN);
        pPlayerRoom->setSpecialExit(kEastRoomId, qsl("enter portal"));
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));

        ModernGLWidget widget(map(), mpHost);
        widget.setViewCenter(areaId, 0, 0, 0);
        QString line;
        QHash<QString, QColor> colors;
        for (const ModernGLWidget::StatusLinePart& part : widget.statusLineParts()) {
            line += part.text;
            colors.insert(part.text.trimmed(), part.color);
        }
        QVERIFY2(line.startsWith(qsl("[Render Area]")), qPrintable(line));
        QVERIFY2(line.contains(qsl("(0,0,0)")), qPrintable(line));
        QVERIFY2(line.contains(qsl("exits: e \"enter portal\" ")), qPrintable(line));
        // The stairs are named by where they lead, or by its ID where that has no name
        QVERIFY2(line.contains(qsl("< tower")), qPrintable(line));
        QVERIFY2(line.contains(qsl("> %1").arg(kLowerRoomId)), qPrintable(line));
        QCOMPARE(colors.value(qsl("Render Area")), mpHost->mLightCyan_2);
        QCOMPARE(colors.value(qsl("e \"enter portal\"")), mpHost->mLightWhite_2);
    }

    // A profile's ANSI colors darkened to near black still show on the status line's dark band
    void test_terminalStatusLineLiftsDarkColorsWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kEastRoomId, DIR_EAST);
        const QColor savedCyan = mpHost->mLightCyan_2;
        const QColor savedWhite = mpHost->mLightWhite_2;
        const auto restore = qScopeGuard([this, savedCyan, savedWhite]() {
            mpHost->mLightCyan_2 = savedCyan;
            mpHost->mLightWhite_2 = savedWhite;
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));
        const QColor dark(20, 20, 20);
        mpHost->mLightCyan_2 = dark;
        mpHost->mLightWhite_2 = dark;

        ModernGLWidget widget(map(), mpHost);
        widget.setViewCenter(areaId, 0, 0, 0);
        QHash<QString, QColor> colors;
        for (const ModernGLWidget::StatusLinePart& part : widget.statusLineParts()) {
            colors.insert(part.text.trimmed(), part.color);
        }
        const QColor lifted = MapStyle::findOrDefault(qsl("terminal")).roomColor(dark);
        QVERIFY(lifted.lightness() > dark.lightness() + 40);
        QCOMPARE(colors.value(qsl("Render Area")), lifted);
        QCOMPARE(colors.value(qsl("e")), lifted);
    }

    // Away from the player's area, only the area and the room under the pointer are named
    void test_terminalStatusLineOutsideThePlayersAreaWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kEastRoomId, DIR_EAST);
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Other Area"));
        constexpr int kOtherRoomId = 7;
        QVERIFY(addRoomAt(kOtherRoomId, otherAreaId, 0, 0, 0));
        map()->mpRoomDB->getRoom(kOtherRoomId)->name = qsl("cellar");
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));

        ModernGLWidget widget(map(), mpHost);
        const auto line = [&widget]() {
            QString text;
            for (const ModernGLWidget::StatusLinePart& part : widget.statusLineParts()) {
                text += part.text;
            }
            return text;
        };
        widget.setViewCenter(otherAreaId, 0, 0, 0);
        QVERIFY2(line().startsWith(qsl("[Other Area]")), qPrintable(line()));
        QVERIFY2(!line().contains(qsl("exits:")) && !line().contains(QLatin1Char('@')), qPrintable(line()));
        QVERIFY2(!line().contains(qsl("cursor:")), qPrintable(line()));
        widget.mHoveredRoomId = kOtherRoomId;
        QVERIFY2(line().contains(qsl("cursor: room %1 cellar").arg(kOtherRoomId)), qPrintable(line()));
        // Not for the player's own room, which the line already names
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.mHoveredRoomId = kPlayerRoomId;
        QVERIFY2(!line().contains(qsl("cursor:")), qPrintable(line()));
    }

    // A room shows the ground its environment is: guessed from a defined color, or named for the environment, and in
    // ASCII where the glyph font has no character for it
    void test_terminalRoomsShowTheirGroundWithoutGL()
    {
        constexpr int kBlue = 300;
        constexpr int kNamed = 301;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mRoomIdHash[map()->mProfileName] = 0;
        map()->mCustomEnvColors[kBlue] = QColor(40, 80, 220);
        map()->mCustomEnvColors[kNamed] = QColor(40, 80, 220);
        QVERIFY(EnvironmentKit::store(map()->mUserData, kNamed, EnvironmentKit::Kit::Plain));
        map()->mpRoomDB->getRoom(kEastRoomId)->environment = kBlue;
        map()->mpRoomDB->getRoom(kPlayerRoomId)->environment = kNamed;
        const auto restore = qScopeGuard([this, kBlue, kNamed]() {
            map()->mCustomEnvColors.remove(kBlue);
            map()->mCustomEnvColors.remove(kNamed);
            map()->mUserData.remove(EnvironmentKit::userDataKey());
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));

        ModernGLWidget widget(map(), mpHost);
        const QString water = RoomAppearance::groundGlyph(EnvironmentKit::Kit::Water, false);
        const bool waterShown = RoomAppearance::fontShows(widget.terminalGlyphFont(widget.mapStyle()), water);
        QList<DrawnGlyph> glyphs = terminalGlyphs(widget, areaId);
        QCOMPARE(glyphAt(glyphs, QPointF(2, 0)).text, waterShown ? water : qsl("~"));
        QCOMPARE(glyphAt(glyphs, QPointF(0, 0)).text, qsl("\""));

        // As a font without it would have it
        widget.mGroundGlyphsFontKey = widget.terminalGlyphFont(widget.mapStyle()).key();
        widget.mGroundGlyphShown.insert(water, false);
        glyphs = terminalGlyphs(widget, areaId);
        QCOMPARE(glyphAt(glyphs, QPointF(2, 0)).text, qsl("~"));
    }

    // Every style draws a highlighted room in the colors a script highlighted it in; the terminal's cell is filled
    // with it, its character and the symbol's underline in a color that reads on it
    void test_highlightKeepsTheScriptsColorWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        TRoom* pEast = map()->mpRoomDB->getRoom(kEastRoomId);
        pEast->highlight = true;
        pEast->highlightRadius = 1.0f;
        pEast->mSymbol = qsl("$");
        const auto restore = qScopeGuard([this, pEast]() {
            pEast->highlight = false;
            pEast->mSymbol.clear();
            mpHost->setMapStyle(qsl("tiles"));
        });
        for (const QColor& scriptColor : {QColor(0, 0, 160), QColor(255, 255, 85)}) {
            pEast->highlightColor = scriptColor;
            pEast->highlightColor2 = scriptColor;
            for (const QString& styleId : {qsl("tiles"), qsl("blueprint"), qsl("terminal")}) {
                QVERIFY(mpHost->setMapStyle(styleId));
                ModernGLWidget widget(map(), mpHost);
                queueFrame(widget, areaId);
                bool found = false;
                for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                    if (key.startsWith(qsl("highlight|"))) {
                        QVERIFY2(key.section(QLatin1Char('|'), 2, 2) == scriptColor.name(QColor::HexArgb), qPrintable(qsl("%1: %2").arg(styleId, key)));
                        found = true;
                    }
                }
                QVERIFY2(found, qPrintable(styleId));
            }
            ModernGLWidget widget(map(), mpHost);
            const QList<DrawnGlyph> glyphs = terminalGlyphs(widget, areaId);
            const QColor legible = ExitAppearance::contrastColor(scriptColor);
            const DrawnGlyph symbol = glyphAt(glyphs, QPointF(2, 0));
            QCOMPARE(symbol.text, qsl("$"));
            QCOMPARE(symbol.color.rgb(), legible.rgb());
            const QList<DrawnGlyph> lines = underlines(glyphs);
            QCOMPARE(lines.size(), 1);
            QCOMPARE(lines.constFirst().color.rgb(), legible.rgb());
        }
    }

    // Two areas' symbols together overflow the terminal style's glyph atlas, so each switch starts it again;
    // every room must still be drawn from the cell holding its own character, in a texture drawn with it there
    void test_terminalGlyphsComeFromTheirOwnAtlasCellsWithoutGL()
    {
        constexpr int kRoomsPerArea = 150;
        constexpr int kColumns = 15;
        map()->mapClear();
        QHash<int, QHash<QPair<int, int>, QString>> expected;
        QList<int> areaIds;
        for (const char16_t base : {u'\x4E00', u'\xAC00'}) {
            const int areaId = map()->mpRoomDB->addArea(qsl("Atlas Area %1").arg(areaIds.size()));
            QVERIFY(areaId > 0);
            for (int i = 0; i < kRoomsPerArea; ++i) {
                const int roomId = 1000 * (areaIds.size() + 1) + i;
                const int x = i % kColumns - kColumns / 2;
                const int y = i / kColumns - kRoomsPerArea / kColumns / 2;
                QVERIFY(addRoomAt(roomId, areaId, x, y, 0));
                const QString symbol(QChar(static_cast<char16_t>(base + i)));
                map()->mpRoomDB->getRoom(roomId)->mSymbol = symbol;
                expected[areaId].insert({x, y}, symbol);
            }
            areaIds.append(areaId);
        }
        // No player's room here, so that no cell is drawn in reverse video
        map()->mRoomIdHash[map()->mProfileName] = 0;
        const QFont savedFont = map()->mMapSymbolFont;
        const auto restore = qScopeGuard([this, savedFont]() {
            map()->mMapSymbolFont = savedFont;
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));

        ModernGLWidget widget(map(), mpHost);
        // The atlas's cells as each of its textures was drawn, which is what the texture shows
        QHash<GLuint, QStringList> atlasTextures;
        GLuint lastTexture = 0;
        widget.mLabelTextureCache.useTextureStandIn(
                [&widget, &atlasTextures, &lastTexture](const QImage& image) {
                    ++lastTexture;
                    if (image.size() == QSize(1024, 1024)) {
                        atlasTextures.insert(lastTexture, widget.mGlyphAtlasCells);
                    }
                    return lastTexture;
                },
                [](GLuint) {});
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());

        // The atlas texture the area was drawn with, or 0 where a room was drawn from another's cell
        const auto drawnFromOwnCells = [&](const int areaId) -> GLuint {
            widget.setViewCenter(areaId, 0, 0, 0);
            widget.frameRooms(30.0);
            widget.mCameraController.setTopView();
            widget.updateMatrices();
            widget.mRenderCommandQueue.clear();
            widget.renderRooms();
            GLuint atlas = 0;
            int glyphs = 0;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get());
                if (!textured || !atlasTextures.contains(textured->geometry().textureId)) {
                    continue;
                }
                const GeometryData& batch = textured->geometry();
                atlas = batch.textureId;
                const QStringList& cells = atlasTextures.value(atlas);
                for (qsizetype quad = 0; quad * 18 + 8 < batch.vertices.size() && quad * 12 + 5 < batch.textureCoords.size(); ++quad) {
                    // Each quad's first vertex is its bottom-left corner and its third its top-right; the atlas's rows run
                    // up the texture, as its image is uploaded upside down
                    const float* corner = batch.vertices.constData() + quad * 18;
                    const float* texture = batch.textureCoords.constData() + quad * 12;
                    const QPair<int, int> place{qRound((corner[0] + corner[6]) / 2.0f), qRound((corner[1] + corner[7]) / 2.0f)};
                    const int cell = (15 - static_cast<int>((texture[1] + texture[5]) / 2.0f * 16.0f)) * 16 + static_cast<int>((texture[0] + texture[2]) / 2.0f * 16.0f);
                    // A symbol's underline is drawn from the atlas's solid block
                    if (cell == 0) {
                        continue;
                    }
                    const QString symbol = expected.value(areaId).value(place);
                    if (symbol.isEmpty() || cell >= cells.size() || cells.at(cell) != symbol || widget.mGlyphAtlasIndex.value(symbol, -1) != cell) {
                        qWarning() << "the room at" << place << "shows" << cells.value(cell) << "for" << symbol;
                        return 0;
                    }
                    ++glyphs;
                }
            }
            if (glyphs != kRoomsPerArea) {
                qWarning() << glyphs << "glyphs drawn for" << kRoomsPerArea << "rooms";
                return 0;
            }
            return atlas;
        };

        const GLuint first = drawnFromOwnCells(areaIds.at(0));
        QVERIFY2(first, "the first area is drawn from the wrong cells");
        QVERIFY2(drawnFromOwnCells(areaIds.at(1)), "the second area is drawn from the first area's cells");
        const GLuint back = drawnFromOwnCells(areaIds.at(0));
        QVERIFY2(back, "back in the first area, it is drawn from the second area's cells");
        QVERIFY(back != first);

        map()->mMapSymbolFont.setItalic(!map()->mMapSymbolFont.italic());
        const GLuint restyled = drawnFromOwnCells(areaIds.at(0));
        QVERIFY2(restyled, "in another font, the area is drawn from the wrong cells");
        QVERIFY2(restyled != back, "the atlas was not drawn again in the new font");
        // Started again for each area, the atlas still keeps every ground's character
        for (const QString& kitName : EnvironmentKit::names()) {
            const QString ground = RoomAppearance::groundGlyph(*EnvironmentKit::fromName(kitName), false);
            QVERIFY2(widget.mGlyphAtlasIndex.contains(ground), qPrintable(ground));
        }
    }

    // A view of more symbols than the atlas holds still shows every room, the rest as plain rooms
    void test_terminalSymbolsPastTheAtlasShowAsPlainRoomsWithoutGL()
    {
        constexpr int kRooms = 200;
        constexpr int kColumns = 20;
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Crowded Area"));
        QVERIFY(areaId > 0);
        for (int i = 0; i < kRooms; ++i) {
            QVERIFY(addRoomAt(1000 + i, areaId, i % kColumns - kColumns / 2, i / kColumns - kRooms / kColumns / 2, 0));
            map()->mpRoomDB->getRoom(1000 + i)->mSymbol = QString(QChar(static_cast<char16_t>(0x4E00 + i)));
        }
        map()->mRoomIdHash[map()->mProfileName] = 0;
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));

        ModernGLWidget widget(map(), mpHost);
        QSet<GLuint> atlasTextures;
        GLuint lastTexture = 0;
        widget.mLabelTextureCache.useTextureStandIn(
                [&atlasTextures, &lastTexture](const QImage& image) {
                    if (image.size() == QSize(1024, 1024)) {
                        atlasTextures.insert(lastTexture + 1);
                    }
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.frameRooms(30.0);
        widget.mCameraController.setTopView();
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.renderRooms();

        qsizetype glyphs = 0;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get());
            if (!textured || !atlasTextures.contains(textured->geometry().textureId)) {
                continue;
            }
            const GeometryData& batch = textured->geometry();
            for (qsizetype quad = 0; quad * 12 + 5 < batch.textureCoords.size(); ++quad) {
                const float* texture = batch.textureCoords.constData() + quad * 12;
                // Not a symbol's underline, drawn from the atlas's solid block in its first cell
                if ((15 - static_cast<int>((texture[1] + texture[5]) / 2.0f * 16.0f)) * 16 + static_cast<int>((texture[0] + texture[2]) / 2.0f * 16.0f) != 0) {
                    ++glyphs;
                }
            }
        }
        QCOMPARE(glyphs, kRooms);
        // These rooms' environment is not one the map defines, so they show no ground
        QVERIFY(widget.mGlyphAtlasIndex.contains(qsl("#")));

        // An underline for each symbol shown, and none for one shown as a plain room
        const QList<DrawnGlyph> drawn = terminalGlyphs(widget, areaId);
        qsizetype symbols = 0;
        for (const DrawnGlyph& glyph : drawn) {
            symbols += glyph.text.size() == 1 && glyph.text.at(0).unicode() >= 0x4E00 && glyph.text.at(0).unicode() < 0x4E00 + kRooms;
        }
        QVERIFY(symbols > 0 && symbols < kRooms);
        QCOMPARE(underlines(drawn).size(), symbols);
    }

    // A terminal's room shows plain stairs in its character, but not an in or out exit or a door, which its markers show
    void test_terminalMarksOnlyWhatItsCharacterCannotShowWithoutGL()
    {
        constexpr int kStairsRoomId = 4;
        constexpr int kInRoomId = 5;
        constexpr int kDoorRoomId = 6;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kStairsRoomId, areaId, 4, 0, 0) && addRoomAt(kInRoomId, areaId, 6, 0, 0) && addRoomAt(kDoorRoomId, areaId, 8, 0, 0));
        map()->mpRoomDB->getRoom(kStairsRoomId)->setExit(kLowerRoomId, DIR_DOWN);
        map()->mpRoomDB->getRoom(kInRoomId)->setExit(kPlayerRoomId, DIR_IN);
        map()->mpRoomDB->getRoom(kDoorRoomId)->setExit(kLowerRoomId, DIR_DOWN);
        map()->mpRoomDB->getRoom(kDoorRoomId)->setDoor(qsl("down"), 3);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QStringList markers;
        for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
            if (key.startsWith(qsl("markers|"))) {
                markers.append(key.section(QLatin1Char('|'), 2));
            }
        }
        markers.sort();
        // Up, down, in and out, each followed by its door where there is an exit: the in exit and the locked door's stairs
        QCOMPARE(markers, (QStringList{qsl("00200"), qsl("02300")}));
    }

    // A script may fade a 2D highlight out from its center with a clear outer color, which a one-color shape would draw unseen
    void test_aClearHighlightColorGivesWayToItsCenterColorWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(mpHost->mapStylePreset().highlightShape != MapStyle::HighlightShape::Disc);
        TRoom* pEast = map()->mpRoomDB->getRoom(kEastRoomId);
        const auto restore = qScopeGuard([this, pEast]() {
            pEast->highlight = false;
            pEast->mSymbol.clear();
            mpHost->setMapStyle(qsl("tiles"));
        });
        pEast->highlight = true;
        pEast->highlightRadius = 1.0f;
        const QColor blue(0, 0, 255);
        pEast->highlightColor = QColor(255, 0, 0, 0);
        pEast->highlightColor2 = blue;
        const auto drawnColors = [this, areaId]() {
            ModernGLWidget widget(map(), mpHost);
            queueFrame(widget, areaId);
            QStringList colors;
            for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                if (key.startsWith(qsl("highlight|"))) {
                    colors.append(key.section(QLatin1Char('|'), 2, 2));
                }
            }
            return colors;
        };
        QCOMPARE(drawnColors(), QStringList{blue.name(QColor::HexArgb)});

        pEast->highlightColor = blue;
        pEast->highlightColor2 = QColor(255, 0, 0, 0);
        QCOMPARE(drawnColors(), QStringList{blue.name(QColor::HexArgb)});

        // The terminal's cell is filled in the center color, so its character contrasts with that
        const QColor navy(0, 0, 160);
        pEast->highlightColor = QColor(255, 255, 85, 0);
        pEast->highlightColor2 = navy;
        pEast->mSymbol = qsl("$");
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));
        ModernGLWidget widget(map(), mpHost);
        const DrawnGlyph symbol = glyphAt(terminalGlyphs(widget, areaId), QPointF(2, 0));
        QCOMPARE(symbol.text, qsl("$"));
        QCOMPARE(symbol.color.rgb(), ExitAppearance::contrastColor(navy).rgb());
    }
};

#include "ModernMapperRenderTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperRenderTest)
