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
#include <cmath>
#include <memory>
#include <numbers>

#include <QFileInfo>
#include <QOpenGLContext>
#include <QPainter>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "EnvironmentKit.h"
#include "ExitAppearance.h"
#include "GeometryManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MapDecals.h"
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
    // As renderRooms sizes a style's rooms, the default style's unless named
    static float roomHeight(const QString& styleId = qsl("tiles"))
    {
        const MapStyle* style = MapStyle::find(styleId);
        return 2.0f / style->roomScale / style->roomFlattening;
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

    // Whether any triangle is drawn in this color, whatever its alpha
    static bool colorQueued(const ModernGLWidget& widget, const QColor& color)
    {
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
    }

    // Whether any instanced bar, as exit lines are drawn, is in this color
    static bool barQueued(const ModernGLWidget& widget, const QColor& color)
    {
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get())) {
                for (const CubeInstanceData& cube : cubes->instances()) {
                    if (qAbs(cube.color[0] - color.redF()) < 0.01 && qAbs(cube.color[1] - color.greenF()) < 0.01 && qAbs(cube.color[2] - color.blueF()) < 0.01) {
                        return true;
                    }
                }
            }
        }
        return false;
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
        mpHost->mMapStrongHighlight = false;
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        const auto transitBeacon = beaconCommands();

        QVERIFY2(tilesBeacon > 0, "tiles queued no beacon");
        QCOMPARE(blueprintBeacon, 0);
        QVERIFY2(profileMarkerBeacon > 0, "blueprint dropped the beacon though the profile's own player marker is drawn");
        QVERIFY2(strongHighlightBeacon > 0, "blueprint dropped the beacon though the profile's strong highlight is drawn");
        QVERIFY2(transitBeacon == 0, "transit raised a beacon over the player's pill");
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
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        const QStringList hologram = frame();
        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        const QStringList inkwash = frame();
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        const QStringList after = frame();

        QVERIFY(before != blueprint);
        QVERIFY(before != transit);
        QVERIFY(before != hologram);
        QVERIFY(before != inkwash);
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
        QCOMPARE(transit->playerMarker, MapStyle::PlayerMarker::Pill);
        QCOMPARE(transit->highlightShape, MapStyle::HighlightShape::Halo);
        QVERIFY(transit->levelPlatforms && transit->lineLegend && transit->inkRimsOnLines);
        QVERIFY(transit->mixedLineColor.isValid());
        QVERIFY(transit->environmentLines);
        QVERIFY(transit->labelHalos);
        QVERIFY(transit->interchangeExits > 0);
        const MapStyle* tiles = MapStyle::find(qsl("tiles"));
        QVERIFY(tiles);
        QCOMPARE(tiles->playerMarker, MapStyle::PlayerMarker::Ring);
        QVERIFY(!tiles->environmentLines);
        QVERIFY(!tiles->labelHalos);
        QCOMPARE(tiles->interchangeExits, 0);
        QVERIFY(!tiles->levelPlatforms && !tiles->lineLegend && !tiles->inkRimsOnLines);
    }

    // A station is ringed in its environment's color, and an interchange in ink but no larger, as clicks and
    // the selection take a room's size; the player's room has a bullseye rather than the profile's marker; and a station
    // on a line of its own environment is ringed in ink, as the line shows the environment
    void test_transitRingsStationsAndMarksThePlayerWithoutGL()
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
        const auto bullseyeDrawn = [&widget]() {
            return std::any_of(widget.mRoomTexturesInUse.cbegin(), widget.mRoomTexturesInUse.cend(), [](const QString& key) {
                return key.startsWith(qsl("bullseye|"));
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
        const bool tilesBullseye = bullseyeDrawn();
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        const MapStyle* transit = MapStyle::find(qsl("transit"));
        queueFrame(widget, areaId);
        const bool ringedInRed = colorQueued(widget, transit->ringColor(red, mpHost->mBgColor_2));
        const bool interchangeInInk = colorQueued(widget, transit->ink);
        const bool transitBullseye = bullseyeDrawn();
        const float playerWidth = roomWidth(kPlayerRoomId);
        const float eastWidth = roomWidth(kEastRoomId);
        const auto eastRing = [this, &widget]() {
            const TRoom* pEast = map()->mpRoomDB->getRoom(kEastRoomId);
            for (const auto& outline : widget.mInkOutlines.value(0).outlines) {
                if (qAbs(outline.x - pEast->x()) < 1e-4f && qAbs(outline.y - pEast->y()) < 1e-4f) {
                    return QColor::fromRgbF(outline.color.x(), outline.color.y(), outline.color.z());
                }
            }
            return QColor();
        };
        const QColor apart = eastRing();
        pPlayerRoom->environment = 301;
        map()->mpRoomDB->getRoom(kEastRoomId)->setExit(kPlayerRoomId, DIR_WEST);
        queueFrame(widget, areaId);
        const QColor onLine = eastRing();

        QVERIFY2(!tilesBullseye, "tiles put a bullseye on the player's room");
        QVERIFY2(transitBullseye, "transit did not put a bullseye on the player's room");
        QVERIFY2(ringedInRed, "the station is not ringed in its environment's color");
        QVERIFY2(interchangeInInk, "the interchange is not ringed in ink");
        const auto alike = [](const QColor& a, const QColor& b) {
            return a.isValid() && qAbs(a.red() - b.red()) <= 1 && qAbs(a.green() - b.green()) <= 1 && qAbs(a.blue() - b.blue()) <= 1;
        };
        QVERIFY2(alike(apart, transit->ringColor(red, mpHost->mBgColor_2)), "a station apart from its environment's lines is not ringed in its color");
        QVERIFY2(alike(onLine, transit->ink), "a station on a line of its own environment is not ringed in ink");
        QVERIFY(eastWidth > 0.0f);
        QCOMPARE(playerWidth, eastWidth);
    }

    // In transit other levels are not drawn in place but as platforms, the nearest by the stair that reaches them, each
    // named by the room the stair reaches there
    void test_transitShowsOtherLevelsAsPlatformsWithoutGL()
    {
        constexpr int kTowerId = 40;
        constexpr int kBelfryId = 41;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(addRoomAt(kTowerId, areaId, 0, 0, 1) && addRoomAt(kBelfryId, areaId, 0, 0, 2));
        map()->mpRoomDB->getRoom(kTowerId)->name = qsl("tower");
        map()->mpRoomDB->getRoom(kBelfryId)->name = qsl("belfry");
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kTowerId, DIR_UP);
        map()->mpRoomDB->getRoom(kTowerId)->setExit(kBelfryId, DIR_UP);
        ModernGLWidget widget(map(), mpHost);
        TArea* pArea = map()->mpRoomDB->getArea(areaId);

        queueFrame(widget, areaId);
        const bool tilesShowsAbove = widget.levelShown(1);
        const bool tilesPlatforms = !widget.levelPlatforms(*pArea, 0).first.platforms.isEmpty();
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        queueFrame(widget, areaId);
        const auto [above, below] = widget.levelPlatforms(*pArea, 0);

        QVERIFY(tilesShowsAbove && !tilesPlatforms);
        QVERIFY2(!widget.levelShown(1) && widget.levelInRange(1) && widget.levelShown(0), "transit drew another level in place");
        QCOMPARE(above.platforms.size(), 2);
        QCOMPARE(above.platforms.at(0).label, qsl("L2 belfry"));
        QCOMPARE(above.platforms.at(1).label, qsl("L1 tower"));
        QVERIFY2(above.stair.has_value(), "the shaft does not rise from the stair");
        QVERIFY2(above.platforms.at(0).pill.bottom() < above.platforms.at(1).pill.top(), "the platforms overlap or are out of order");
        QVERIFY2(above.platforms.at(1).pill.bottom() < above.stair->y(), "the nearest platform is not above the stair");
        QCOMPARE(below.platforms.size(), 1);
        QCOMPARE(below.platforms.at(0).label, qsl("B1"));
        QVERIFY2(!below.stair.has_value(), "a shaft falls from a stair the level does not have");
        QVERIFY(QRectF(widget.rect()).contains(below.platforms.at(0).pill));

        // A player on another level is pointed out on that level's platform
        map()->mRoomIdHash[map()->mProfileName] = kTowerId;
        QImage overlay(widget.size(), QImage::Format_ARGB32_Premultiplied);
        overlay.fill(Qt::transparent);
        QPainter painter(&overlay);
        widget.paintPlayerPill(painter, 0, widget.levelPlatforms(*pArea, 0));
        painter.end();
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        QVERIFY2(overlay.pixelColor(above.platforms.at(1).station(0).toPoint()).alpha() == 255, "the player's level's platform is not marked");
    }

    // The legend keys the lines of the viewed level by their kind of ground, with gray for those joining two
    // environments, and the doors on them
    void test_transitLegendKeysTheViewedLevelsLinesWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mCustomEnvColors[301] = QColor(30, 60, 220);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mCustomEnvColors.remove(301);
        });
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        pPlayerRoom->setExit(kEastRoomId, DIR_EAST);
        pEastRoom->setExit(kPlayerRoomId, DIR_WEST);
        pPlayerRoom->setDoor(qsl("e"), 3);
        ModernGLWidget widget(map(), mpHost);
        TArea* pArea = map()->mpRoomDB->getArea(areaId);
        const auto texts = [&widget, pArea]() {
            QStringList found;
            for (const auto& entry : widget.lineLegend(*pArea)) {
                found.append(entry.text);
            }
            return found;
        };

        queueFrame(widget, areaId);
        const QStringList tilesLegend = texts();
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        pPlayerRoom->environment = 301;
        queueFrame(widget, areaId);
        const QStringList mixed = texts();
        const bool mixedQueued = barQueued(widget, MapStyle::find(qsl("transit"))->mixedLineColor);
        pEastRoom->environment = 301;
        queueFrame(widget, areaId);
        const QList<ModernGLWidget::LegendEntry> same = widget.lineLegend(*pArea);

        QVERIFY(tilesLegend.isEmpty());
        QCOMPARE(mixed, (QStringList{qsl("mixed"), qsl("locked door")}));
        QVERIFY2(mixedQueued, "the line joining two environments is not drawn in the style's mixed color");
        QCOMPARE(same.size(), 2);
        QCOMPARE(same.at(0).text, qsl("water"));
        QCOMPARE(same.at(1).doorStatus, 3);
    }

    // The legend keys only the lines the map draws: none for an exit drawn as a custom line, or to a hidden room, and no
    // door on an exit that draws no line on the level
    void test_transitLegendKeysOnlyLinesDrawnWithoutGL()
    {
        constexpr int kAboveId = 40;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mCustomEnvColors[301] = QColor(30, 60, 220);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mCustomEnvColors.remove(301);
        });
        QVERIFY(addRoomAt(kAboveId, areaId, 0, 0, 1));
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        pPlayerRoom->environment = 301;
        pPlayerRoom->setExit(kEastRoomId, DIR_EAST);
        pEastRoom->setExit(kPlayerRoomId, DIR_WEST);
        pPlayerRoom->setExit(kAboveId, DIR_UP);
        pPlayerRoom->setDoor(qsl("up"), 2);
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        ModernGLWidget widget(map(), mpHost);
        TArea* pArea = map()->mpRoomDB->getArea(areaId);
        const auto texts = [&widget, pArea]() {
            QStringList found;
            for (const auto& entry : widget.lineLegend(*pArea)) {
                found.append(entry.text);
            }
            return found;
        };

        queueFrame(widget, areaId);
        const QStringList drawn = texts();
        pPlayerRoom->customLines.insert(qsl("e"), {QPointF(1.0, 1.0), QPointF(2.0, 0.0)});
        pEastRoom->customLines.insert(qsl("w"), {QPointF(1.0, -1.0), QPointF(0.0, 0.0)});
        const QStringList customLine = texts();
        pPlayerRoom->customLines.clear();
        pEastRoom->customLines.clear();
        pEastRoom->setHidden(true);
        const QStringList hidden = texts();

        QCOMPARE(drawn, (QStringList{qsl("mixed")}));
        QVERIFY2(customLine.isEmpty(), qPrintable(qsl("exits drawn as custom lines are keyed: %1").arg(customLine.join(qsl(", ")))));
        QVERIFY2(hidden.isEmpty(), qPrintable(qsl("a line to a hidden room is keyed: %1").arg(hidden.join(qsl(", ")))));
    }

    // Other levels are left to their platforms: none of their rooms is drawn, or can be clicked or picked in a rectangle
    void test_transitNeitherDrawsNorPicksOtherLevelsWithoutGL()
    {
        constexpr int kTowerId = 40;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(addRoomAt(kTowerId, areaId, 2, 2, 1));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kTowerId, DIR_UP);
        ModernGLWidget widget(map(), mpHost);
        const auto towerDrawn = [&widget]() {
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && !roomInstances(*cubes, QPointF(2.0, 2.0)).isEmpty()) {
                    return true;
                }
            }
            // A station is drawn by its rim
            const auto& rims = widget.mInkOutlines.value(1).outlines;
            return std::any_of(rims.cbegin(), rims.cend(), [](const auto& rim) {
                return qAbs(rim.x - 2.0f) < 1e-4f && qAbs(rim.y - 2.0f) < 1e-4f;
            });
        };
        const auto towerClicked = [&widget]() {
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(2.0f, 2.0f, 1.0f));
            return pixel && widget.roomAt(pixel->toPoint(), false) == kTowerId;
        };

        queueFrame(widget, areaId);
        const bool tilesDrawn = towerDrawn();
        const bool tilesClicked = towerClicked();
        const bool tilesPicked = widget.roomsInRectangle(widget.rect(), true).contains(kTowerId);
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        queueFrame(widget, areaId);

        QVERIFY(tilesDrawn && tilesClicked && tilesPicked);
        QVERIFY2(!towerDrawn(), "transit drew a room of another level");
        QVERIFY2(!towerClicked(), "a room of another level can be clicked");
        QVERIFY2(!widget.roomsInRectangle(widget.rect(), true).contains(kTowerId), "a room of another level can be picked");
    }

    // Where the view has no room for every platform the farthest are left out, but never the player's level, even
    // past the levels shown; and a level with more rooms than its platform has stations shows how many more
    void test_transitFitsThePlatformsAndKeepsThePlayersWithoutGL()
    {
        constexpr int kFirstFloorId = 40;
        constexpr int kFloors = 6;
        constexpr int kTopFloorId = kFirstFloorId + kFloors - 1;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        });
        int below = kPlayerRoomId;
        for (int floor = 1; floor <= kFloors; ++floor) {
            const int id = kFirstFloorId + floor - 1;
            QVERIFY(addRoomAt(id, areaId, 0, 0, floor));
            map()->mpRoomDB->getRoom(below)->setExit(id, DIR_UP);
            below = id;
        }
        for (int i = 1; i <= 4; ++i) {
            QVERIFY(addRoomAt(kFirstFloorId + 100 + i, areaId, 2 * i, 2, 1));
        }
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        ModernGLWidget widget(map(), mpHost);
        TArea* pArea = map()->mpRoomDB->getArea(areaId);
        queueFrame(widget, areaId);
        const qsizetype roomy = widget.levelPlatforms(*pArea, 0).first.platforms.size();
        widget.resize(800, 240);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.updateMatrices();
        const auto levels = [](const ModernGLWidget::PlatformShaft& shaft) {
            QList<int> found;
            for (const auto& platform : shaft.platforms) {
                found.append(platform.level);
            }
            return found;
        };
        const auto apartInView = [&widget](const ModernGLWidget::PlatformShaft& shaft) {
            for (qsizetype i = 0; i < shaft.platforms.size(); ++i) {
                const QRectF& pill = shaft.platforms.at(i).pill;
                if (!QRectF(widget.rect()).contains(pill) || (i > 0 && shaft.platforms.at(i - 1).pill.bottom() >= pill.top())) {
                    return false;
                }
            }
            return true;
        };

        const ModernGLWidget::PlatformShaft cramped = widget.levelPlatforms(*pArea, 0).first;
        map()->mRoomIdHash[map()->mProfileName] = kTopFloorId;
        const ModernGLWidget::PlatformShaft withPlayer = widget.levelPlatforms(*pArea, 0).first;
        QImage overlay(widget.size(), QImage::Format_ARGB32_Premultiplied);
        overlay.fill(Qt::transparent);
        QPainter painter(&overlay);
        widget.paintPlayerPill(painter, 0, widget.levelPlatforms(*pArea, 0));
        painter.end();
        widget.mShowTopLevels = 2;
        const QList<int> outOfRange = levels(widget.levelPlatforms(*pArea, 0).first);
        widget.mShowTopLevels = 999999;

        QCOMPARE(roomy, kFloors);
        QVERIFY2(cramped.platforms.size() < kFloors && !cramped.platforms.isEmpty(), qPrintable(qsl("%1 platforms in a cramped view").arg(cramped.platforms.size())));
        QVERIFY2(apartInView(cramped), "the platforms overlap or leave the view");
        QCOMPARE(cramped.platforms.constLast().level, 1);
        QCOMPARE(cramped.platforms.constLast().stations, 3);
        QVERIFY2(cramped.platforms.constLast().moreWidth > 0.0, "a level with more rooms than stations does not say how many more");
        QCOMPARE(withPlayer.platforms.size(), cramped.platforms.size());
        QVERIFY2(apartInView(withPlayer), "the platforms overlap or leave the view");
        QVERIFY2(levels(withPlayer).contains(kFloors), "the player's level was left out of the platforms");
        QCOMPARE(withPlayer.platforms.constLast().level, 1);
        bool marked = false;
        for (const auto& platform : withPlayer.platforms) {
            if (platform.level == kFloors) {
                marked = overlay.pixelColor(platform.station(0).toPoint()).alpha() == 255;
            }
        }
        QVERIFY2(marked, "the player's level's platform is not marked");
        QVERIFY2(outOfRange.contains(kFloors), "the player's level was left out for being past the levels shown");
    }

    // In grid mode, with no compass, the bottom platforms stand clear of a status line
    void test_transitPlatformsStandClearOfTheStatusLineWithoutGL()
    {
        constexpr int kCellarId = 40;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        MapStyle* transit = const_cast<MapStyle*>(MapStyle::find(qsl("transit")));
        QVERIFY(transit);
        TArea* pArea = map()->mpRoomDB->getArea(areaId);
        const auto restore = qScopeGuard([this, transit, pArea]() {
            transit->statusLine = false;
            pArea->gridMode = false;
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(addRoomAt(kCellarId, areaId, 0, 0, -2));
        pArea->gridMode = true;
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        transit->statusLine = true;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);

        const ModernGLWidget::PlatformShaft below = widget.levelPlatforms(*pArea, 0).second;
        const QRect statusLine = widget.statusLineRect();

        QVERIFY(widget.compassRect().isEmpty() && !statusLine.isEmpty());
        QVERIFY(!below.platforms.isEmpty());
        for (const auto& platform : below.platforms) {
            QVERIFY2(platform.pill.bottom() < statusLine.top(), qPrintable(qsl("a platform reaching %1 is under the status line at %2").arg(platform.pill.bottom()).arg(statusLine.top())));
        }
    }

    // Platforms stacked up to the top of the view by a stair at its left stand clear of the legend there
    void test_transitPlatformsStandClearOfTheLegendWithoutGL()
    {
        constexpr int kFirstFloorId = 40;
        constexpr int kFloors = 5;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mCustomEnvColors[301] = QColor(30, 60, 220);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mCustomEnvColors.remove(301);
        });
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        pPlayerRoom->environment = 301;
        pPlayerRoom->setExit(kEastRoomId, DIR_EAST);
        pEastRoom->setExit(kPlayerRoomId, DIR_WEST);
        pPlayerRoom->setDoor(qsl("e"), 1);
        int below = kPlayerRoomId;
        for (int floor = 1; floor <= kFloors; ++floor) {
            const int id = kFirstFloorId + floor - 1;
            QVERIFY(addRoomAt(id, areaId, 0, 0, floor));
            map()->mpRoomDB->getRoom(below)->setExit(id, DIR_UP);
            below = id;
        }
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        widget.resize(800, 400);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.updateMatrices();
        // The player's stair a little way in from the left edge
        const std::optional<QPointF> origin = widget.mCameraController.pixelForPoint(QVector3D(0.0f, 0.0f, 0.0f));
        const std::optional<QPointF> east = widget.mCameraController.pixelForPoint(QVector3D(1.0f, 0.0f, 0.0f));
        QVERIFY(origin && east && east->x() > origin->x());
        widget.syncView(areaId, qRound((origin->x() - 60.0) / (east->x() - origin->x())), 0, 0, kPlayerRoomId);
        widget.updateMatrices();
        TArea* pArea = map()->mpRoomDB->getArea(areaId);

        const ModernGLWidget::PlatformShaft above = widget.levelPlatforms(*pArea, 0).first;
        const QRect legend = widget.lineLegendRect(*pArea, 0);

        QVERIFY(!legend.isEmpty());
        QVERIFY(above.stair.has_value());
        QCOMPARE(above.platforms.size(), kFloors);
        QFont font = widget.sheetFont();
        font.setBold(true);
        const QFontMetrics metrics(font);
        for (const auto& platform : above.platforms) {
            const QRectF named = platform.pill.adjusted(-8.0 - metrics.horizontalAdvance(platform.label) - 5.0, 0.0, 0.0, 0.0);
            QVERIFY2(!named.intersects(QRectF(legend)), qPrintable(qsl("the platform %1 at %2,%3 is over the legend").arg(platform.label).arg(named.left()).arg(named.top())));
        }
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

    // Hologram draws glowing edges tinted by the environment, with scanlines, and pulses from the player's room
    // Each environment glows in its own hue, and only a gray one, which has none, in the ink
    void test_hologramGlowsInEachEnvironmentsHue()
    {
        const MapStyle* hologram = MapStyle::find(qsl("hologram"));
        QVERIFY(hologram);
        const QColor ink = hologram->ink;
        QVERIFY(ink.isValid());
        const QColor red(200, 40, 40, 200);
        const QColor green(40, 200, 40, 200);
        const QColor room = hologram->roomColor(red);
        const QColor other = hologram->roomColor(green);
        QVERIFY2(qAbs(room.hslHue() - red.hslHue()) <= 2, qPrintable(qsl("a red environment glows at hue %1").arg(room.hslHue())));
        QVERIFY2(qAbs(other.hslHue() - green.hslHue()) <= 2, qPrintable(qsl("a green environment glows at hue %1").arg(other.hslHue())));
        QVERIFY2(room.hslSaturation() > 200, "a room's environment is washed out");
        QCOMPARE(room.alpha(), red.alpha());
        const QColor gray = hologram->roomColor(QColor(128, 128, 128));
        QVERIFY2(qAbs(gray.hslHue() - ink.hslHue()) <= 2, "a gray environment does not glow in the ink");
        QVERIFY2(hologram->roomColor(QColor(Qt::black)).lightness() >= 150, "a dark environment's edges do not glow");
        QVERIFY2(room.lightness() >= 150, "a room's edges do not glow");
        QCOMPARE(hologram->edges, MapStyle::Edges::GlowingWireframes);
        QCOMPARE(hologram->playerMarker, MapStyle::PlayerMarker::Pulse);
        QVERIFY(hologram->postEffects.testFlag(MapStyle::PostEffect::Scanlines));
        QVERIFY(hologram->postEffects.testFlag(MapStyle::PostEffect::Bloom));
        QVERIFY(hologram->labelTint.isValid());
        QVERIFY(hologram->scannerOverlay);
        QVERIFY(hologram->themedMapLabels);
        QVERIFY(hologram->compassNeedle.isValid() && hologram->infoBackground.isValid() && hologram->infoTextColor.isValid());
        QVERIFY(!hologram->floorPlates);
        QVERIFY(!hologram->lit);
        QVERIFY(hologram->background.lightness() < 30);
        const MapStyle* tiles = MapStyle::find(qsl("tiles"));
        QVERIFY(tiles);
        QCOMPARE(tiles->postEffects, MapStyle::PostEffects(MapStyle::PostEffect::NoPostEffects));
        QVERIFY(!tiles->labelTint.isValid());
        QVERIFY(!tiles->scannerOverlay);
    }

    // Every room, on every level, is drawn as glowing edges added onto what is under it, with no fill to hide the
    // levels behind it, and the player's room brightest
    void test_hologramAddsGlowingEdgesWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const bool savedLevels = mudlet::self()->drawUpperLowerLevels();
        const auto restore = qScopeGuard([this, savedLevels]() {
            mpHost->setMapStyle(qsl("tiles"));
            mudlet::self()->setDrawUpperLowerLevels(savedLevels);
        });
        mudlet::self()->setDrawUpperLowerLevels(true);
        constexpr int kUpperRoomId = 6;
        QVERIFY(addRoomAt(kUpperRoomId, areaId, 2, 2, 1));
        const bool savedBorders = mpHost->mMapperShowRoomBorders;
        const auto restoreBorders = qScopeGuard([this, savedBorders]() {
            mpHost->mMapperShowRoomBorders = savedBorders;
        });
        mpHost->mMapperShowRoomBorders = true;
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kEastRoomId, DIR_EAST);
        // Added onto the background as it is, black would leave nothing of the room
        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderColor = QColor(Qt::black);
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);

        bool additive = false;
        int additiveSections = 0;
        QList<const RenderInstancedCubesCommand*> wires;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get())) {
                if (state->stateType() == GLStateCommand::ADDITIVE_BLEND) {
                    QVERIFY2(!additive, "additive blending was turned on twice");
                    additive = true;
                    ++additiveSections;
                } else if (state->stateType() == GLStateCommand::ALPHA_BLEND) {
                    additive = false;
                }
            } else if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get())) {
                QVERIFY2(additive, "blocks were drawn over what is under them rather than added onto it");
                if (cubes->shading() == RenderCommand::Shading::Wireframe) {
                    wires.append(cubes);
                }
            }
        }
        QVERIFY2(!additive, "additive blending was left on");
        QVERIFY(additiveSections > 0);

        const auto wireAt = [&wires](const QPointF& place) {
            QList<CubeInstanceData> found;
            for (const RenderInstancedCubesCommand* cubes : std::as_const(wires)) {
                found.append(roomInstances(*cubes, place));
            }
            return found;
        };
        const QList<CubeInstanceData> player = wireAt(QPointF(0.0, 0.0));
        const QList<CubeInstanceData> east = wireAt(QPointF(2.0, 0.0));
        QCOMPARE(player.size(), 1);
        QCOMPARE(east.size(), 1);
        const QList<CubeInstanceData> lower = wireAt(QPointF(0.0, 2.0));
        QVERIFY2(lower.size() == 1, "the room on the level below has no edges");
        const auto brightness = [](const CubeInstanceData& cube) {
            return (cube.color[0] + cube.color[1] + cube.color[2]) * cube.color[3];
        };
        QVERIFY2(brightness(player.first()) > brightness(east.first()), "the player's room is no brighter than another");
        const QColor eastWire = QColor::fromRgbF(east.first().color[0], east.first().color[1], east.first().color[2]);
        QVERIFY2(eastWire.lightness() >= 150, qPrintable(qsl("a room with a black border glows only %1 bright").arg(eastWire.lightness())));
        const bool exitWired = std::any_of(wires.cbegin(), wires.cend(), [](const RenderInstancedCubesCommand* cubes) {
            return std::any_of(cubes->instances().cbegin(), cubes->instances().cend(), [](const CubeInstanceData& cube) {
                return qAbs(cube.transform.column(3).x() - 1.0f) < 0.3f && qAbs(cube.transform.column(3).y()) < 1e-4f;
            });
        });
        QVERIFY2(exitWired, "the exit between the rooms does not glow");

        // Dimmed toward the dark background as with no level colors, not mixed toward the lower level's gray, which would glow gray
        mudlet::self()->setDrawUpperLowerLevels(false);
        queueFrame(widget, areaId);
        QList<CubeInstanceData> plainLower;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::Wireframe) {
                plainLower.append(roomInstances(*cubes, QPointF(0.0, 2.0)));
            }
        }
        QCOMPARE(plainLower.size(), 1);
        for (int channel = 0; channel < 4; ++channel) {
            QVERIFY2(qFuzzyCompare(lower.first().color[channel], plainLower.first().color[channel]), "the level below took on the lower level's color");
        }
    }

    // The player is marked by a diamond of glowing lines over their room, in the player's color, not a column
    void test_hologramMarksThePlayerWithADiamondWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        const MapStyle& hologram = mpHost->mapStylePreset();
        const QColor color = hologram.playerRoomColor();
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);

        QList<CubeInstanceData> lines;
        float highest = 0.0f;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get());
            if (!cubes || cubes->shading() != RenderCommand::Shading::Unlit) {
                continue;
            }
            for (const CubeInstanceData& cube : cubes->instances()) {
                const QVector4D center = cube.transform.column(3);
                if (qAbs(center.x()) < 0.5f && qAbs(center.y()) < 0.5f && center.z() > roomHeight(qsl("hologram")) / 2.0f && qAbs(cube.color[0] - color.redF()) < 1e-4f
                    && qAbs(cube.color[1] - color.greenF()) < 1e-4f && qAbs(cube.color[2] - color.blueF()) < 1e-4f) {
                    lines.append(cube);
                    highest = qMax(highest, center.z());
                }
            }
        }
        // Its twelve edges and the stem down to the room, each a core and a glow
        QCOMPARE(lines.size(), 26);
        QVERIFY2(highest < 1.0f, "the diamond rises out of sight like the column");
        const bool wide = std::any_of(lines.cbegin(), lines.cend(), [](const CubeInstanceData& cube) {
            return qAbs(cube.transform.column(3).x()) > 0.1f || qAbs(cube.transform.column(3).y()) > 0.1f;
        });
        QVERIFY2(wide, "the marker has no corners off its middle, as a diamond does");
    }

    // A highlighted room is ringed on the ground in the script's color, not covered by the 2D map's disc
    void test_hologramRingsHighlightedRoomsWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        TRoom* east = map()->mpRoomDB->getRoom(kEastRoomId);
        const auto restore = qScopeGuard([this, east]() {
            mpHost->setMapStyle(qsl("tiles"));
            east->highlight = false;
        });
        const QColor blue(0, 0, 255);
        east->highlight = true;
        east->highlightColor = blue;
        east->highlightColor2 = QColor(0, 255, 0);
        east->highlightRadius = 1.0f;
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        const MapStyle& hologram = mpHost->mapStylePreset();
        QCOMPARE(hologram.highlightShape, MapStyle::HighlightShape::Rings);
        QVERIFY(MapDecals::highlightOnGround(hologram.highlightShape));
        ModernGLWidget widget(map(), mpHost);
        const auto ringKeys = [&widget]() {
            QStringList found;
            for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                if (key.startsWith(qsl("highlight|%1|").arg(static_cast<int>(MapStyle::HighlightShape::Rings)))) {
                    found.append(key);
                }
            }
            return found;
        };
        queueFrame(widget, areaId);
        const QStringList ringed = ringKeys();
        QCOMPARE(ringed.size(), 1);
        QCOMPARE(ringed.constFirst().section(QLatin1Char('|'), 2, 2), blue.name(QColor::HexArgb));

        // On the ground under the room, centered on it and reaching past it
        const GLuint texture = widget.mLabelTextureCache.imageTexture(ringed.constFirst());
        QVERIFY(texture);
        const float foot = -roomHeight(qsl("hologram")) / 2.0f;
        const float roomWidth = 2.0f / hologram.roomScale;
        int quads = 0;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get());
            if (!textured || textured->geometry().textureId != texture) {
                continue;
            }
            const QVector<float>& vertices = textured->geometry().vertices;
            QCOMPARE(vertices.size(), 18);
            float least = vertices.at(0);
            float most = vertices.at(0);
            float middle = 0.0f;
            for (qsizetype vertex = 0; vertex < vertices.size(); vertex += 3) {
                QVERIFY2(qAbs(vertices.at(vertex + 2) - foot) < 1e-4f, qPrintable(qsl("the ring is at height %1, the room's foot at %2").arg(vertices.at(vertex + 2)).arg(foot)));
                least = std::min(least, vertices.at(vertex));
                most = std::max(most, vertices.at(vertex));
                middle += vertices.at(vertex + 1);
            }
            QVERIFY2(qAbs((least + most) / 2.0f - 2.0f) < 1e-4f && qAbs(middle / 6.0f) < 1e-4f, "the ring is not centered on its room");
            QVERIFY2(most - least > roomWidth, qPrintable(qsl("the ring is %1 across and its room %2").arg(most - least).arg(roomWidth)));
            ++quads;
        }
        QCOMPARE(quads, 1);

        east->highlight = false;
        queueFrame(widget, areaId);
        QVERIFY2(ringKeys().isEmpty(), "a room no longer highlighted is still ringed");
    }

    // The readout has a panel behind it to read over a crowded map, in the profile's own info box color when it has one
    void test_hologramReadoutReadsOnItsPanelWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            mpHost->mMapInfoBg = Host::scmDefaultMapInfoBg;
        });
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        ModernGLWidget widget(map(), mpHost);
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.resize(800, 600);
        const auto paintReadout = [&widget]() {
            QImage image(widget.size(), QImage::Format_ARGB32);
            image.fill(Qt::black);
            QPainter painter(&image);
            widget.paintScannerOverlay(painter, 0);
            return image;
        };
        // Where the panel's top left corner and the first line's text fall, at the readout's 12px margin and 11px font
        const QPoint panelCorner(17, 14);
        const QRect firstLine(20, 16, 80, 9);
        const auto extremeLightness = [](const QImage& image, const QRect& area, const bool darkest) {
            int found = darkest ? 255 : 0;
            for (int y = area.top(); y <= area.bottom(); ++y) {
                for (int x = area.left(); x <= area.right(); ++x) {
                    const int lightness = QColor(image.pixel(x, y)).lightness();
                    found = darkest ? std::min(found, lightness) : std::max(found, lightness);
                }
            }
            return found;
        };

        const QImage themed = paintReadout();
        QVERIFY2(QColor(themed.pixel(panelCorner)) != QColor(Qt::black), "the readout has no panel behind it");
        QVERIFY2(extremeLightness(themed, firstLine, false) > 150, "the readout's text does not stand out from its dark panel");

        mpHost->mMapInfoBg = QColor(Qt::white);
        const QImage own = paintReadout();
        QCOMPARE(QColor(own.pixel(panelCorner)), QColor(Qt::white));
        QVERIFY2(extremeLightness(own, firstLine, true) < 100, "the readout's text does not stand out from a light panel");
    }

    // A map label is drawn dimmer and tinted toward the ink, so that it does not outshine the rooms
    void test_hologramDimsLabelsWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this, areaId]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mpRoomDB->getArea(areaId)->mMapLabels.clear();
        });
        TMapLabel text;
        text.text = qsl("Old Keep");
        text.pos = QVector3D(0.0f, 1.0f, 0.0f);
        text.size = QSizeF(2.0, 0.5);
        text.pix = QPixmap(80, 20);
        text.pix.fill(Qt::white);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(1, text);
        ModernGLWidget widget(map(), mpHost);
        // The tint of the first of the label's corners
        const auto labelTint = [&widget]() {
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* triangles = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get()); triangles && triangles->geometry().colors.size() >= 4) {
                    const QVector<float>& colors = triangles->geometry().colors;
                    return QVector4D(colors.at(0), colors.at(1), colors.at(2), colors.at(3));
                }
            }
            return QVector4D();
        };
        queueFrame(widget, areaId);
        const QVector4D plain = labelTint();
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        queueFrame(widget, areaId);
        const QVector4D tinted = labelTint();
        QVERIFY2(plain.w() > 0.0f && tinted.w() > 0.0f, "the label was not drawn");
        QVERIFY2(tinted.w() < plain.w(), "the label is no dimmer");
        QVERIFY2(tinted.z() > tinted.x(), "the label is not tinted toward the ink");
    }

    // Each room's ID is drawn, though hologram tints every room its own color
    void test_hologramShowsRoomIdsWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const bool savedIds = mpHost->mShowRoomID;
        const auto restore = qScopeGuard([this, savedIds]() {
            mpHost->setMapStyle(qsl("tiles"));
            mpHost->mShowRoomID = savedIds;
        });
        mpHost->mShowRoomID = true;
        map()->mpRoomDB->getRoom(kEastRoomId)->environment = 5;
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QVERIFY2(widget.mRoomIdsDrawn.contains(kEastRoomId), "the room's ID was not drawn");
    }

    // A move sends a pulse out from the player's room, wider and fainter as it spreads, and only in hologram
    void test_hologramPulsesFromThePlayersRoomWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        ModernGLWidget widget(map(), mpHost);
        // The glowing edges at the player's room wider than the room
        const auto pulses = [&widget]() {
            QList<CubeInstanceData> found;
            float roomWidth = std::numeric_limits<float>::max();
            QList<CubeInstanceData> atPlayer;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::Wireframe) {
                    for (const CubeInstanceData& cube : roomInstances(*cubes, QPointF(0.0, 0.0))) {
                        roomWidth = qMin(roomWidth, cube.transform.column(0).toVector3D().length());
                        atPlayer.append(cube);
                    }
                }
            }
            for (const CubeInstanceData& cube : std::as_const(atPlayer)) {
                if (cube.transform.column(0).toVector3D().length() > roomWidth * 1.1f) {
                    found.append(cube);
                }
            }
            return found;
        };

        // Set as if begun that long ago, so that only one frame's queueing passes in real time
        const auto pulseBegan = [&widget](const qint64 ago) {
            widget.mPulseStartedAt = widget.mPulseReference.elapsed() - ago;
        };
        pulseBegan(300);
        queueFrame(widget, areaId);
        QVERIFY2(pulses().isEmpty(), "tiles sent out a pulse");

        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        pulseBegan(100);
        queueFrame(widget, areaId);
        const QList<CubeInstanceData> early = pulses();
        QCOMPARE(early.size(), 1);
        pulseBegan(500);
        queueFrame(widget, areaId);
        const QList<CubeInstanceData> later = pulses();
        QCOMPARE(later.size(), 1);
        QVERIFY2(later.first().transform.column(0).toVector3D().length() > early.first().transform.column(0).toVector3D().length(), "the pulse did not spread");
        QVERIFY2(later.first().color[3] < early.first().color[3], "the pulse did not fade as it spread");

        widget.mPulseStartedAt.reset();
        queueFrame(widget, areaId);
        QVERIFY2(pulses().isEmpty(), "a pulse was drawn with none under way");

        // Nor left repainting once the style is one without it
        pulseBegan(100);
        widget.mPulseTimer->start();
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        widget.applyStyle();
        QVERIFY2(!widget.mPulseTimer->isActive() && !widget.mPulseStartedAt.has_value(), "a pulse kept going after a change to a style without one");
    }

    // A room's edge glows over its middle, every third row of pixels is darker, and a move starts a pulse
    void test_hologramGlowsAtTheEdgesWithScanlines()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        });
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const float top = roomHeight(qsl("hologram")) / 2.0f;
        const QImage frame = widget.grabFramebuffer();
        const auto pixelAt = [&widget](const QVector3D& point) {
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(point);
            // The camera's pixels are the widget's, the framebuffer's the device's
            return pixel ? (*pixel * widget.devicePixelRatioF()).toPoint() : QPoint(-1, -1);
        };
        const auto brightness = [&frame](const QPoint& pixel) {
            const QColor color = frame.pixelColor(pixel);
            return color.red() + color.green() + color.blue();
        };
        // Over three rows, so that a scanline does not decide it
        const auto brightest = [&](const QPoint& pixel) {
            return qMax(brightness(pixel), qMax(brightness(pixel + QPoint(0, 1)), brightness(pixel + QPoint(0, 2))));
        };
        // Just inside the east room's east edge, and in its middle
        const QPoint edge = pixelAt(QVector3D(2.24f, 0.0f, top));
        const QPoint middle = pixelAt(QVector3D(2.0f, 0.0f, top));
        QVERIFY(frame.rect().contains(edge) && frame.rect().contains(middle));
        QVERIFY2(brightest(edge) > brightest(middle) + 150, qPrintable(qsl("a room's edge is %1 bright and its middle %2").arg(brightest(edge)).arg(brightest(middle))));

        // Down the edge, the rows a scanline darkens against the others: a few pixels inside it, as the glow is added after the scanlines
        const int column = edge.x() - qRound(3.0 * widget.devicePixelRatioF());
        const int from = pixelAt(QVector3D(2.24f, 0.15f, top)).y();
        const int to = pixelAt(QVector3D(2.24f, -0.15f, top)).y();
        QVERIFY(to - from > 12);
        qint64 dark = 0;
        qint64 light = 0;
        int darkRows = 0;
        int lightRows = 0;
        const float period = qMax(3.0f, std::round(3.0f * static_cast<float>(widget.devicePixelRatioF())));
        for (int row = from; row <= to; ++row) {
            // A scanline is the first third of each period counted up from the bottom, through the pixel's center, as GL counts them
            if (std::fmod(static_cast<float>(frame.height() - 1 - row) + 0.5f, period) < period / 3.0f) {
                dark += brightness(QPoint(column, row));
                ++darkRows;
            } else {
                light += brightness(QPoint(column, row));
                ++lightRows;
            }
        }
        QVERIFY2(dark * lightRows * 100 < light * darkRows * 85, qPrintable(qsl("scanlines average %1 bright and the rows between them %2").arg(dark / darkRows).arg(light / lightRows)));

        widget.mPulseStartedAt.reset();
        map()->mRoomIdHash[map()->mProfileName] = kEastRoomId;
        widget.grabFramebuffer();
        QVERIFY2(widget.mPulseStartedAt.has_value() && widget.mPulseTimer->isActive(), "a move did not start a pulse");

        widget.setSecondaryView(true);
        widget.stopPulse();
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        widget.grabFramebuffer();
        QVERIFY2(widget.mPulseStartedAt.has_value() && widget.mPulseTimer->isActive(), "a move seen in a second map window did not start a pulse");

        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        widget.stopPulse();
        map()->mRoomIdHash[map()->mProfileName] = kEastRoomId;
        widget.grabFramebuffer();
        QVERIFY2(!widget.mPulseStartedAt.has_value() && !widget.mPulseTimer->isActive(), "a move in tiles started a pulse");
    }

    // Ink wash lays each room pale in its environment's hue, enough to tell areas apart while the ink on it reads, and
    // seals the player's room
    void test_inkwashWashesRoomsPaleInTheirHue()
    {
        const MapStyle* inkwash = MapStyle::find(qsl("inkwash"));
        QVERIFY(inkwash);
        const QColor red = inkwash->roomColor(QColor(230, 40, 40, 200));
        const QColor green = inkwash->roomColor(QColor(40, 230, 40, 200));
        const QColor blue = inkwash->roomColor(QColor(40, 40, 230, 200));
        const QColor white = inkwash->roomColor(QColor(Qt::white));
        QCOMPARE(red.alpha(), 200);
        for (const QColor& room : {red, green, blue, white}) {
            QVERIFY2(room.lightness() >= 170, qPrintable(qsl("a room's wash is only %1 light").arg(room.lightness())));
            QVERIFY2(qAbs(inkwash->background.lightness() - room.lightness()) <= 60, "a room is not a wash on the paper");
        }
        QVERIFY2(red.saturation() > 30, qPrintable(qsl("a bright red room's wash is %1 saturated").arg(red.saturation())));
        QVERIFY2(qAbs(red.hue() - 0) < 20 || qAbs(red.hue() - 360) < 20, "a red room's wash lost its hue");
        // Each toward the warm paper's hue a little, but nearer its own
        QVERIFY2(qAbs(green.hue() - 120) < 30, "a green room's wash lost its hue");
        QVERIFY2(qAbs(blue.hue() - 240) < 30, "a blue room's wash lost its hue");
        QVERIFY2(difference(red, green) >= 25 && difference(green, blue) >= 25 && difference(blue, red) >= 25, "areas' rooms cannot be told apart");
        QVERIFY(inkwash->washReach > 1.0f);
        QVERIFY(inkwash->environmentMarks);
        QCOMPARE(inkwash->compass, MapStyle::Compass::Arrow);
        QVERIFY(inkwash->scaleBar && inkwash->playerNote && inkwash->labelItalic);
        QVERIFY(inkwash->postEffects.testFlag(MapStyle::PostEffect::Vignette));
        QVERIFY(!MapStyle::find(qsl("tiles"))->postEffects.testFlag(MapStyle::PostEffect::Vignette));
        QVERIFY(inkwash->background.lightness() >= 200);
        const QColor seal = inkwash->playerRoomColor();
        QVERIFY2(seal.red() > seal.green() + 80 && seal.red() > seal.blue() + 80 && seal != QColor(Qt::red), "the seal is not a muted vermilion");
        QCOMPARE(inkwash->edges, MapStyle::Edges::InkLozenges);
        QCOMPARE(inkwash->playerMarker, MapStyle::PlayerMarker::Seal);
        QVERIFY(inkwash->postEffects.testFlag(MapStyle::PostEffect::PaperGrain));
        QVERIFY(!inkwash->lit);
        const MapStyle* hologram = MapStyle::find(qsl("hologram"));
        QVERIFY(hologram);
        QVERIFY(!hologram->postEffects.testFlag(MapStyle::PostEffect::PaperGrain));
    }

    // Paper laid under everything, washes on it under the exits, the viewed level's rooms as lozenges with no border
    // but their own, those above hatched and those below dashed, exits as strokes, the player's room as a seal with no
    // beacon, and the other levels pale toward the paper rather than taking on the level colors
    void test_inkwashBrushesOnPaperWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const bool savedLevels = mudlet::self()->drawUpperLowerLevels();
        const auto restore = qScopeGuard([this, savedLevels]() {
            mpHost->setMapStyle(qsl("tiles"));
            mudlet::self()->setDrawUpperLowerLevels(savedLevels);
        });
        mudlet::self()->setDrawUpperLowerLevels(true);
        constexpr int kUpperRoomId = 6;
        QVERIFY(addRoomAt(kUpperRoomId, areaId, 2, 2, 1));
        const bool savedBorders = mpHost->mMapperShowRoomBorders;
        const auto restoreBorders = qScopeGuard([this, savedBorders]() {
            mpHost->mMapperShowRoomBorders = savedBorders;
        });
        mpHost->mMapperShowRoomBorders = true;
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kEastRoomId, DIR_EAST);
        map()->mpRoomDB->getRoom(kEastRoomId)->setExit(kPlayerRoomId, DIR_WEST);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setDoor(qsl("e"), 2);
        ModernGLWidget widget(map(), mpHost);
        // The player's beacon, a column standing on the player's room
        const auto beaconQueued = [&widget]() {
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get())) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        const QVector4D center = cube.transform.column(3);
                        if (qAbs(center.x()) < 1e-4f && qAbs(center.y()) < 1e-4f && center.z() > roomHeight()) {
                            return true;
                        }
                    }
                }
            }
            return false;
        };
        const auto atPlace = [&widget](const RenderCommand::Shading shading, const QPointF& place) {
            QList<CubeInstanceData> found;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == shading) {
                    found.append(roomInstances(*cubes, place));
                }
            }
            return found;
        };
        const auto strokeBetweenRooms = [&widget]() {
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::InkStroke) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        if (qAbs(cube.transform.column(3).x() - 1.0f) < 0.3f && qAbs(cube.transform.column(3).y()) < 1e-4f) {
                            return true;
                        }
                    }
                }
            }
            return false;
        };
        const auto doorBars = [&widget](const RenderCommand::Shading shading) {
            const QColor door = ExitAppearance::doorColor(2);
            int found = 0;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == shading) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        if (qAbs(cube.color[0] - door.redF()) < 1e-3f && qAbs(cube.color[1] - door.greenF()) < 1e-3f && qAbs(cube.color[2] - door.blueF()) < 1e-3f) {
                            ++found;
                        }
                    }
                }
            }
            return found;
        };

        queueFrame(widget, areaId);
        QVERIFY2(beaconQueued(), "tiles raised no beacon, so its absence proves nothing");

        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        queueFrame(widget, areaId);
        const auto& commands = widget.mRenderCommandQueue.commands();
        qsizetype paper = -1;
        qsizetype firstDrawn = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(commands.at(index).get());
            if (triangles && triangles->shading() == RenderCommand::Shading::Paper) {
                QVERIFY2(paper < 0, "the paper was laid twice");
                paper = index;
            }
            if (firstDrawn < 0 && (triangles || dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get()))) {
                firstDrawn = index;
            }
        }
        QVERIFY2(paper >= 0, "no paper was laid");
        QCOMPARE(paper, firstDrawn);

        QCOMPARE(atPlace(RenderCommand::Shading::Seal, QPointF(0.0, 0.0)).size(), 1);
        QVERIFY2(atPlace(RenderCommand::Shading::InkLozenge, QPointF(0.0, 0.0)).isEmpty(), "the player's room was drawn under its seal");
        QVERIFY2(atPlace(RenderCommand::Shading::Seal, QPointF(2.0, 0.0)).isEmpty(), "another room was sealed");
        const QList<CubeInstanceData> lozenge = atPlace(RenderCommand::Shading::InkLozenge, QPointF(2.0, 0.0));
        QCOMPARE(lozenge.size(), 1);
        // The ink line is the room's border: the profile's, which every room has, would only blot round it
        QVERIFY2(atPlace(RenderCommand::Shading::InkBlot, QPointF(2.0, 0.0)).isEmpty(), "the profile's border was drawn round a lozenge");
        QVERIFY2(!atPlace(RenderCommand::Shading::InkHatched, QPointF(2.0, 2.0)).isEmpty() && atPlace(RenderCommand::Shading::InkLozenge, QPointF(2.0, 2.0)).isEmpty(),
                 "the room above is not hatched");
        QVERIFY2(!atPlace(RenderCommand::Shading::InkDashed, QPointF(0.0, 2.0)).isEmpty() && atPlace(RenderCommand::Shading::InkLozenge, QPointF(0.0, 2.0)).isEmpty(), "the room below is not dashed");

        // A wash wider than the room under it, laid before any exit's stroke
        const float roomSize = 2.0f * lozenge.first().transform.column(0).toVector3D().length();
        qsizetype firstStroke = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()) && firstStroke < 0; ++index) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get()); cubes && cubes->shading() == RenderCommand::Shading::InkStroke) {
                firstStroke = index;
            }
        }
        QVERIFY(firstStroke > 0);
        int washes = 0;
        for (qsizetype index = 0; index < firstStroke; ++index) {
            const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(commands.at(index).get());
            if (!textured) {
                continue;
            }
            const QVector<float>& vertices = textured->geometry().vertices;
            for (qsizetype quad = 0; quad + 18 <= vertices.size(); quad += 18) {
                const QVector3D corner(vertices.at(quad), vertices.at(quad + 1), vertices.at(quad + 2));
                const QVector3D opposite(vertices.at(quad + 6), vertices.at(quad + 7), vertices.at(quad + 8));
                const QVector3D middle = (corner + opposite) / 2.0f;
                if (qAbs(middle.x() - 2.0f) < 1e-3f && qAbs(middle.y()) < 1e-3f && qAbs(corner.x() - opposite.x()) > 1.5f * roomSize) {
                    ++washes;
                }
            }
        }
        QCOMPARE(washes, 1);

        // A room's own border still shows, under its line
        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderColor = QColor(200, 40, 40);
        queueFrame(widget, areaId);
        QVERIFY2(!atPlace(RenderCommand::Shading::InkBlot, QPointF(2.0, 0.0)).isEmpty(), "a room's own border was dropped");
        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderColor = QColor();
        queueFrame(widget, areaId);
        QVERIFY2(atPlace(RenderCommand::Shading::Unlit, QPointF(2.0, 0.0)).isEmpty() && atPlace(RenderCommand::Shading::Lit, QPointF(2.0, 0.0)).isEmpty(), "a room was drawn as a block");
        QVERIFY2(strokeBetweenRooms(), "the exit between the rooms is not a brush stroke");
        // A door's bar tapered would be a wedge on one side of the line
        QVERIFY2(doorBars(RenderCommand::Shading::Unlit) > 0, "the door's bars were not drawn plain");
        QCOMPARE(doorBars(RenderCommand::Shading::InkStroke), 0);
        QVERIFY2(!beaconQueued(), "a beacon stands on the seal");

        // Ink thinned toward the paper, the same as with no level colors
        const QList<CubeInstanceData> lower = atPlace(RenderCommand::Shading::InkDashed, QPointF(0.0, 2.0));
        QVERIFY(!lower.isEmpty());
        mudlet::self()->setDrawUpperLowerLevels(false);
        queueFrame(widget, areaId);
        const QList<CubeInstanceData> plainLower = atPlace(RenderCommand::Shading::InkDashed, QPointF(0.0, 2.0));
        QCOMPARE(plainLower.size(), lower.size());
        for (qsizetype i = 0; i < lower.size(); ++i) {
            for (int channel = 0; channel < 4; ++channel) {
                QVERIFY2(qFuzzyCompare(lower.at(i).color[channel], plainLower.at(i).color[channel]), "the level below took on the lower level's color");
            }
        }
    }

    // On the paper, a room is a pale lozenge in a dark line wobbling in a way of its own, laid in a wash that bleeds past
    // it, the player's room is vermilion, and an exit thins along its length; and nothing changes from one frame to the next
    void test_inkwashSketchesLozengesInWashesOnPaper()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        constexpr int kWestRoomId = 4;
        constexpr int kSouthWestRoomId = 5;
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(addRoomAt(kWestRoomId, map()->mpRoomDB->getRoom(kPlayerRoomId)->getArea(), -2, 0, 0));
        QVERIFY(addRoomAt(kSouthWestRoomId, map()->mpRoomDB->getRoom(kPlayerRoomId)->getArea(), -2, -2, 0));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kEastRoomId, DIR_EAST);
        // Both ways, as a one-way exit is dotted
        map()->mpRoomDB->getRoom(kEastRoomId)->setExit(kPlayerRoomId, DIR_WEST);
        // No borders: every room alike but for where it stands
        const bool savedBorders = mpHost->mMapperShowRoomBorders;
        const auto restoreBorders = qScopeGuard([this, savedBorders]() {
            mpHost->mMapperShowRoomBorders = savedBorders;
        });
        mpHost->mMapperShowRoomBorders = false;
        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const QImage frame = widget.grabFramebuffer();
        QCOMPARE(widget.grabFramebuffer(), frame);
        const auto pixelAt = [&widget](const float x, const float y) {
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(x, y, roomHeight(qsl("inkwash")) / 2.0f));
            return pixel ? pixel->toPoint() : QPoint(-1, -1);
        };
        const auto lightnessAt = [&](const float x, const float y) {
            return frame.pixelColor(pixelAt(x, y)).lightness();
        };
        const QColor paper = MapStyle::find(qsl("inkwash"))->background;

        QVERIFY2(lightnessAt(2.0f, 0.0f) >= 170, qPrintable(qsl("a room's middle is only %1 light").arg(lightnessAt(2.0f, 0.0f))));
        QVERIFY2(qAbs(lightnessAt(3.0f, 1.0f) - paper.lightness()) <= 25, qPrintable(qsl("the paper is %1 light").arg(lightnessAt(3.0f, 1.0f))));
        // Its line, somewhere across it from its middle out to past its edge
        int darkest = 255;
        for (int i = 0; i <= 50; ++i) {
            darkest = qMin(darkest, lightnessAt(2.0f + static_cast<float>(i) * 0.01f, 0.0f));
        }
        QVERIFY2(darkest <= 110, qPrintable(qsl("a room's line is %1 light at its darkest").arg(darkest)));
        // The wash, past the room where no exit runs, tinted in its environment's red
        const QColor washed = frame.pixelColor(pixelAt(2.0f, 0.38f));
        const QColor bare = frame.pixelColor(pixelAt(3.0f, 1.6f));
        QVERIFY2(washed.red() - washed.blue() > bare.red() - bare.blue() + 12, qPrintable(qsl("no wash bleeds past the room: %1 on %2").arg(washed.name(), bare.name())));

        // Not a flat color, but grain
        QSet<QRgb> grain;
        const QPoint paperPixel = pixelAt(3.0f, 1.0f);
        for (int dx = 0; dx < 16; ++dx) {
            grain.insert(frame.pixel(paperPixel + QPoint(dx, 0)));
        }
        QVERIFY2(grain.size() >= 4, "the paper has no grain");

        const QColor seal = frame.pixelColor(pixelAt(0.0f, 0.0f));
        QVERIFY2(seal.red() > seal.green() + 50 && seal.red() > seal.blue() + 50, "the player's room is not sealed in vermilion");

        // Two rooms' lines, with no exit over either, at the same places around each: their wobble apart in places,
        // not merely by the paper's grain
        int inked = 0;
        int unlike = 0;
        for (int i = -20; i <= 20; ++i) {
            for (int j = -20; j <= 20; ++j) {
                const float dx = static_cast<float>(i) * 0.015f;
                const float dy = static_cast<float>(j) * 0.015f;
                const bool west = lightnessAt(-2.0f + dx, 0.0f + dy) < 120;
                const bool southWest = lightnessAt(-2.0f + dx, -2.0f + dy) < 120;
                inked += west ? 1 : 0;
                unlike += west != southWest ? 1 : 0;
            }
        }
        QVERIFY2(inked > 40, qPrintable(qsl("the room's line covers only %1 points").arg(inked)));
        QVERIFY2(unlike >= 10, qPrintable(qsl("two rooms' lines differ at only %1 points").arg(unlike)));

        // Across the stroke from the player's room east, at its start and near its end: any ink at all, so that the dry
        // brush's streaks toward the end do not pass for its thinning
        const auto widthAt = [&](const float x) {
            int dark = 0;
            for (int j = -40; j <= 40; ++j) {
                dark += lightnessAt(x, static_cast<float>(j) * 0.002f) < 130 ? 1 : 0;
            }
            return dark;
        };
        const int start = widthAt(0.45f);
        const int end = widthAt(1.55f);
        QVERIFY2(end > 0 && start > end + end / 2, qPrintable(qsl("the stroke is %1 wide at its start and %2 near its end").arg(start).arg(end)));
    }

    // Bloom spreads a soft glow a few pixels out past a room's edge, onto background nothing is drawn on
    void test_hologramGlowsPastTheEdges()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const float top = roomHeight(qsl("hologram")) / 2.0f;
        const QImage frame = widget.grabFramebuffer();
        const auto brightness = [&widget, &frame](const QVector3D& point) {
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(point);
            if (!pixel) {
                return -1;
            }
            const QColor color = frame.pixelColor((*pixel * widget.devicePixelRatioF()).toPoint());
            return color.red() + color.green() + color.blue();
        };
        // The east room's east edge is at 2.25; about seven pixels out of it, and well clear of it, between grid lines
        const int halo = brightness(QVector3D(2.32f, 0.15f, top));
        const int clear = brightness(QVector3D(2.65f, 0.15f, top));
        QVERIFY(halo >= 0 && clear >= 0);
        QVERIFY2(halo > clear + 20, qPrintable(qsl("just past a room's edge is %1 bright, and clear of it %2").arg(halo).arg(clear)));
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
        boxed.bgColor = QColor(Qt::darkBlue);
        // Transit draws a gray box as no box, as the 2D map's default label box is gray
        TMapLabel grayBoxed = boxed;
        grayBoxed.pos = QVector3D(6.0f, 1.0f, 0.0f);
        grayBoxed.bgColor = QColor(Qt::gray);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(1, text);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(2, picture);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(3, boxed);
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(4, grayBoxed);
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        ModernGLWidget widget(map(), mpHost);

        queueFrame(widget, areaId);
        QList<std::pair<int, int>> haloed = widget.mHaloedLabels.keys();
        std::sort(haloed.begin(), haloed.end());
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        queueFrame(widget, areaId);

        QCOMPARE(haloed, (QList<std::pair<int, int>>{{areaId, 1}, {areaId, 4}}));
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
            for (const QString& styleId : {qsl("tiles"), qsl("blueprint"), qsl("hologram"), qsl("terminal")}) {
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
