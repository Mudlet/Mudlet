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
#include <cstdio>
#include <memory>
#include <numbers>

#include <QFileInfo>
#include <QOpenGLContext>
#include <QPainter>
#include <QPolygonF>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "DioramaLayout.h"
#include "EnvironmentKit.h"
#include "ExitAppearance.h"
#include "ExitScene.h"
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
    static constexpr int kKeptOutputEnvironment = 300;
    // As renderRooms sizes it: 2 / scale (4) / zFlattening (8)
    static constexpr float kRoomHeight = 2.0f / 4.0f / 8.0f;
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
    struct WashQuad
    {
        qsizetype command = -1;
        int variant = -1;
        QVector3D corner;
        QVector3D opposite;
        QPointF center() const { return QPointF((corner.x() + opposite.x()) / 2.0, (corner.y() + opposite.y()) / 2.0); }
    };

    static QList<WashQuad> washQuads(const ModernGLWidget& widget)
    {
        QHash<GLuint, int> variants;
        for (int variant = 0; variant < MapDecals::scmWashVariants; ++variant) {
            if (const GLuint texture = widget.mLabelTextureCache.imageTexture(qsl("wash|%1").arg(variant))) {
                variants.insert(texture, variant);
            }
        }
        QList<WashQuad> found;
        const auto& commands = widget.mRenderCommandQueue.commands();
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(commands.at(index).get());
            if (!textured || !variants.contains(textured->geometry().textureId)) {
                continue;
            }
            const QVector<float>& vertices = textured->geometry().vertices;
            for (qsizetype quad = 0; quad + 18 <= vertices.size(); quad += 18) {
                found.append({index,
                              variants.value(textured->geometry().textureId),
                              QVector3D(vertices.at(quad), vertices.at(quad + 1), vertices.at(quad + 2)),
                              QVector3D(vertices.at(quad + 6), vertices.at(quad + 7), vertices.at(quad + 8))});
            }
        }
        return found;
    }

    // Whether depth writes are on when the command at index is drawn
    static bool depthWritesAt(const ModernGLWidget& widget, const qsizetype index)
    {
        bool writes = true;
        const auto& commands = widget.mRenderCommandQueue.commands();
        for (qsizetype i = 0; i < index; ++i) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(commands.at(i).get())) {
                if (state->stateType() == GLStateCommand::ENABLE_DEPTH_WRITE) {
                    writes = true;
                } else if (state->stateType() == GLStateCommand::DISABLE_DEPTH_WRITE) {
                    writes = false;
                }
            }
        }
        return writes;
    }

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

    // What a view keeps from one frame to the next, written out so that two views can be compared
    static QStringList keptSnapshot(const ModernGLWidget& widget)
    {
        QStringList lines;
        const auto cubes = [&lines](const QString& what, const QVector<CubeInstanceData>& instances) {
            for (const CubeInstanceData& cube : instances) {
                QString line = what;
                for (int i = 0; i < 16; ++i) {
                    line += qsl(" %1").arg(cube.transform.constData()[i]);
                }
                for (int i = 0; i < 4; ++i) {
                    line += qsl(" %1").arg(cube.color[i]);
                }
                lines.append(line);
            }
        };
        const auto floats = [&lines](const QString& what, const QVector<float>& values) {
            QString line = what;
            for (const float value : values) {
                line += qsl(" %1").arg(value);
            }
            lines.append(line);
        };
        const ExitScene::Scene& scene = widget.mExitSceneCache.scene;
        // With no exits nothing is made from them, whatever is left over from before
        if (!scene.segments.isEmpty() || !scene.triangles.isEmpty()) {
            cubes(qsl("bar"), widget.mExitGeometry.bars);
            cubes(qsl("stroke"), widget.mExitGeometry.strokes);
            floats(qsl("vertices"), widget.mExitGeometry.vertices);
            floats(qsl("colors"), widget.mExitGeometry.colors);
            for (auto level = widget.mExitGeometry.seeThrough.cbegin(); level != widget.mExitGeometry.seeThrough.cend(); ++level) {
                cubes(qsl("see-through %1 bar").arg(level.key()), level->bars);
                cubes(qsl("see-through %1 stroke").arg(level.key()), level->strokes);
                floats(qsl("see-through %1 vertices").arg(level.key()), level->vertices);
                floats(qsl("see-through %1 colors").arg(level.key()), level->colors);
            }
        }
        for (const ExitScene::Glyph& glyph : widget.mExitGlyphs) {
            lines.append(qsl("glyph %1 %2 %3 %4 %5 %6 %7 %8")
                                 .arg(glyph.at.x())
                                 .arg(glyph.at.y())
                                 .arg(glyph.at.z())
                                 .arg(glyph.along.x())
                                 .arg(glyph.along.y())
                                 .arg(glyph.character.isNull() ? qsl("-") : QString(glyph.character))
                                 .arg(glyph.color.name(QColor::HexArgb))
                                 .arg(glyph.level));
        }
        if (widget.mapStyle().environmentKits) {
            QList<int> roomIds = widget.mDioramaPiecesCache.pieces.keys();
            std::sort(roomIds.begin(), roomIds.end());
            for (const int roomId : std::as_const(roomIds)) {
                const DioramaLayout::Piece& piece = widget.mDioramaPiecesCache.pieces.value(roomId);
                lines.append(qsl("piece %1 %2 %3 %4 %5 %6 %7 %8")
                                     .arg(roomId)
                                     .arg(static_cast<int>(piece.kit))
                                     .arg(piece.storey)
                                     .arg(piece.storeys)
                                     .arg(piece.bottom)
                                     .arg(piece.top)
                                     .arg(piece.peak)
                                     .arg(piece.halfWidth));
            }
        }
        return lines;
    }

    // The three rooms of addArea() joined by exits, the east one in an environment of its own with a custom line,
    // and another area to move rooms to; that area's ID
    int addKeptOutputArea(const int areaId) const
    {
        if (!(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST) && map()->setExit(kEastRoomId, kPlayerRoomId, DIR_WEST) && map()->setExit(kPlayerRoomId, kLowerRoomId, DIR_DOWN)
              && map()->setExit(kLowerRoomId, kPlayerRoomId, DIR_UP) && map()->setExit(kEastRoomId, kPlayerRoomId, DIR_IN))) {
            return 0;
        }
        TRoom* pEast = map()->mpRoomDB->getRoom(kEastRoomId);
        pEast->environment = kKeptOutputEnvironment;
        map()->mCustomEnvColors[kKeptOutputEnvironment] = QColor(200, 120, 40);
        pEast->customLines[qsl("w")] = QList<QPointF>{QPointF(1.0, 1.0)};
        pEast->customLinesColor[qsl("w")] = QColor(Qt::green);
        pEast->customLinesStyle[qsl("w")] = Qt::SolidLine;
        pEast->customLinesArrow[qsl("w")] = false;
        Q_UNUSED(areaId)
        return map()->mpRoomDB->addArea(qsl("Kept Output Other Area"));
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

        // Each room's outline is found by its room, so rooms coming and going leave the others' in other places
        constexpr int kWestRoomId = 6;
        QVERIFY(addRoomAt(kWestRoomId, areaId, -2, 0, 0));
        QVERIFY2(keptMatchesMade(), "after a room was added, the outlines differ from a fresh widget's");
        QVERIFY(map()->mpRoomDB->removeRoom(kEastRoomId));
        QVERIFY2(keptMatchesMade(), "after a room was removed, the outlines differ from a fresh widget's");
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
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        const QStringList diorama = frame();
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        const QStringList after = frame();

        QVERIFY(before != blueprint);
        QVERIFY(before != transit);
        QVERIFY(before != hologram);
        QVERIFY(before != inkwash);
        QVERIFY(before != diorama);
        QCOMPARE(after, before);
    }

    // The x of the middle of each exit bar queued: an instanced box far longer than it is thick
    static QList<float> exitBarMiddles(const ModernGLWidget& widget)
    {
        QList<float> middles;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get());
            if (!cubes || cubes->shape() != GeometryManager::InstanceShape::Box) {
                continue;
            }
            for (const CubeInstanceData& cube : cubes->instances()) {
                QList<float> extents;
                for (int axis = 0; axis < 3; ++axis) {
                    extents.append(cube.transform.column(axis).toVector3D().length());
                }
                std::sort(extents.begin(), extents.end());
                if (extents.at(1) < 0.1f && extents.at(2) > 0.3f) {
                    middles.append(cube.transform.column(3).x());
                }
            }
        }
        std::sort(middles.begin(), middles.end());
        return middles;
    }

    // Another frame from where the camera is now, turned a little, without queueFrame() resetting the camera
    static void queueTurnedFrame(ModernGLWidget& widget)
    {
        widget.turnCamera(37.0f);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
    }

    // The exits are built from the map once and drawn from that until the map, the area or level shown or the
    // style changes; turning or zooming the camera draws the same exits again
    void test_exitsAreBuiltAgainOnlyWhenTheMapOrTheirOptionsChangeWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        QVERIFY(map()->setExit(kEastRoomId, kPlayerRoomId, DIR_WEST));
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QCOMPARE(widget.mExitSceneBuilds, 1);
        QCOMPARE(exitBarMiddles(widget), QList<float>{1.0f});

        queueTurnedFrame(widget);
        widget.setCameraDistance(3.0f);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 1);
        QCOMPARE(exitBarMiddles(widget), QList<float>{1.0f});

        // A room moved on the map takes its exit with it
        const quint64 before = map()->revision();
        QVERIFY(map()->setRoomCoordinates(kEastRoomId, 4, 0, 0));
        QVERIFY(map()->revision() > before);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 2);
        QCOMPARE(exitBarMiddles(widget), QList<float>{2.0f});

        // A map a script changed and asked to see again, cleared or loaded, or the colors it is drawn in
        quint64 revision = map()->revision();
        map()->requestMapRepaint();
        QVERIFY(map()->revision() > revision);
        revision = map()->revision();
        map()->refreshMapperColours();
        QVERIFY(map()->revision() > revision);
        revision = map()->revision();
        map()->announceMapLoaded(false);
        QVERIFY(map()->revision() > revision);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 3);

        QVERIFY(mpHost->setMapStyle(qsl("blueprint")));
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 4);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 4);

        widget.slot_showLessUpperLevels();
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 5);

        // The level below, viewed, comes with its own exits
        QVERIFY(map()->setExit(kLowerRoomId, kEastRoomId, DIR_UP));
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 6);
        widget.jumpTo(areaId, 0, 0, -1);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 7);
        QVERIFY(widget.mExitSceneCache.scene.segments.size() > 1);
    }

    // Past the cap on room textures, one the last frame used is kept where there is anything else to drop, so a view
    // turning back and forth does not make the same textures again and again
    void test_texturesUsedLastFrameAreKeptPastTheCapWithoutGL()
    {
        ModernGLWidget widget(map(), mpHost);
        widget.mLabelTextureCache.useTextureStandIn(
                [lastTexture = GLuint{0}](const QImage&) mutable {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.mMaxRoomTextures = 12;
        const auto make = [&widget](const QString& key) {
            return widget.imageTexture(key, [] {
                return QImage(4, 4, QImage::Format_ARGB32);
            });
        };
        QStringList lastFrames;
        for (int i = 0; i < 8; ++i) {
            lastFrames.append(qsl("last|%1").arg(i));
            QVERIFY(make(lastFrames.constLast()));
        }
        for (int i = 0; i < 4; ++i) {
            QVERIFY(make(qsl("unused|%1").arg(i)));
        }
        // A new frame, after one that used only the first eight
        widget.mRoomTexturesKept = QSet<QString>(lastFrames.cbegin(), lastFrames.cend());
        widget.mRoomTexturesInUse.clear();
        for (int i = 0; i < 4; ++i) {
            QVERIFY(make(qsl("new|%1").arg(i)));
        }
        for (const QString& key : std::as_const(lastFrames)) {
            QVERIFY2(widget.mLabelTextureCache.imageTexture(key), qPrintable(qsl("%1, which the last frame used, was dropped").arg(key)));
        }
    }

    // Where the last frame's and this one's together pass the cap, the last frame's go, so this frame still shows as
    // many as fit rather than leaving out what it needs
    void test_aFrameStillFillsTheCapAfterOneThatUsedOthersWithoutGL()
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Named Area"));
        for (int i = 0; i < 30; ++i) {
            const int roomId = 100 + i;
            QVERIFY(addRoomAt(roomId, areaId, (i % 6) * 2, (i / 6) * 2, 0));
            map()->mpRoomDB->getRoom(roomId)->name = qsl("Room %1").arg(i);
        }
        map()->mRoomIdHash[map()->mProfileName] = 100;
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            map()->setRoomNamesShown(false);
        });
        ModernGLWidget widget(map(), mpHost);
        widget.mMaxRoomTextures = 12;
        queueFrame(widget, areaId);
        QCOMPARE(widget.mRoomTexturesInUse.size(), widget.mMaxRoomTextures);
        for (int i = 0; i < 30; ++i) {
            map()->mpRoomDB->getRoom(100 + i)->name = qsl("Other %1").arg(i);
        }
        queueTurnedFrame(widget);
        QCOMPARE(widget.mRoomTexturesInUse.size(), widget.mMaxRoomTextures);
    }

    // The redness of the exit bar between the two rooms of the level below, which fades toward the lower level color
    static float lowerExitRedness(const ModernGLWidget& widget)
    {
        float redness = std::numeric_limits<float>::lowest();
        const auto check = [&redness](const QVector<CubeInstanceData>& bars) {
            for (const CubeInstanceData& bar : bars) {
                const QVector4D middle = bar.transform.column(3);
                if (qAbs(middle.y() - 2.0f) < 0.01f && qAbs(middle.x() - 1.0f) < 0.01f && middle.z() < -0.5f) {
                    redness = qMax(redness, bar.color[0] - bar.color[1]);
                }
            }
        };
        check(widget.mExitGeometry.bars);
        for (const ModernGLWidget::LevelExits& exits : widget.mExitGeometry.seeThrough) {
            check(exits.bars);
        }
        return redness;
    }

    // What the exits are drawn as is made once and kept while the camera moves, and made again when anything it is
    // made from changes: the map, the level colors and whether they are drawn, the style or the levels shown
    void test_exitGeometryIsMadeAgainOnlyWhenWhatItIsMadeFromChangesWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int lowerEastRoomId = 4;
        QVERIFY(addRoomAt(lowerEastRoomId, areaId, 2, 2, -1));
        QVERIFY(map()->setExit(kLowerRoomId, lowerEastRoomId, DIR_EAST));
        QVERIFY(map()->setExit(lowerEastRoomId, kLowerRoomId, DIR_WEST));
        const QColor savedLower = mpHost->mLowerLevelColor;
        const bool savedShown = mudlet::self()->drawUpperLowerLevels();
        const auto restore = qScopeGuard([this, savedLower, savedShown]() {
            mpHost->mLowerLevelColor = savedLower;
            mudlet::self()->setDrawUpperLowerLevels(savedShown);
            mpHost->setMapStyle(qsl("tiles"));
        });
        mudlet::self()->setDrawUpperLowerLevels(true);
        mpHost->mLowerLevelColor = QColor(0, 0, 255, 128);
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QCOMPARE(widget.mExitGeometryBuilds, 1);
        const float blue = lowerExitRedness(widget);
        QVERIFY2(blue > std::numeric_limits<float>::lowest(), "no exit was drawn on the level below");
        queueTurnedFrame(widget);
        widget.setCameraDistance(3.0f);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 1);

        mpHost->mLowerLevelColor = QColor(255, 0, 0, 128);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 2);
        QVERIFY2(lowerExitRedness(widget) > blue + 0.1f, qPrintable(qsl("the exit below is %1 red after the lower level turned red, %2 before").arg(lowerExitRedness(widget)).arg(blue)));

        mudlet::self()->setDrawUpperLowerLevels(false);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 3);

        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 4);

        widget.slot_showLessLowerLevels();
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 5);

        // The mapper's colors set in the preferences, which only ask for the map to be drawn again
        const quint64 revision = map()->revision();
        map()->updateArea(-1);
        QVERIFY(map()->revision() > revision);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 6);
        const quint64 restored = map()->revision();
        map()->restore16ColorSet();
        QVERIFY(map()->revision() > restored);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 7);
    }

    // The hologram's exits glow, so all of them are drawn with their level's see-through rooms: from what was kept, on
    // a frame that makes nothing again, as on the one that made it
    void test_keptExitsAreStillDrawnSeeThroughWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        QVERIFY(map()->setExit(kEastRoomId, kPlayerRoomId, DIR_WEST));
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QCOMPARE(widget.mExitGeometryBuilds, 1);
        QVERIFY(widget.mExitGeometry.bars.isEmpty());
        QCOMPARE(exitBarMiddles(widget), QList<float>{1.0f});
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitGeometryBuilds, 1);
        QCOMPARE(exitBarMiddles(widget), QList<float>{1.0f});
    }

    // Rooms well outside the view are left out of the frame, but a room just past its edge is drawn whole, and the
    // level's floor plate still reaches under every room
    void test_roomsWellOutsideTheViewAreLeftOutWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int farRoomId = 9;
        QVERIFY(addRoomAt(farRoomId, areaId, 200, 0, 0));
        ModernGLWidget widget(map(), mpHost);
        const auto roomQueuedAt = [&widget](const QPointF& place) {
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && !roomInstances(*cubes, place).isEmpty()) {
                    return true;
                }
            }
            return false;
        };
        const auto plateReach = [&widget]() {
            float reach = std::numeric_limits<float>::lowest();
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get()); triangles && triangles->vertices().size() == 18) {
                    for (qsizetype i = 0; i < 18; i += 3) {
                        reach = qMax(reach, triangles->vertices().at(i));
                    }
                }
            }
            return reach;
        };
        queueFrame(widget, areaId);
        QVERIFY(roomQueuedAt(QPointF(2.0, 0.0)));
        QVERIFY2(!roomQueuedAt(QPointF(200.0, 0.0)), "a room far outside the view was queued");
        QVERIFY2(plateReach() > 200.0f, qPrintable(qsl("the floor plate reaches only to x %1, short of the room at 200").arg(plateReach())));

        // East along the row, the first place whose middle shows just past the view's right edge
        int justOutside = 0;
        for (int x = 4; x < 200 && !justOutside; ++x) {
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(static_cast<float>(x), 0.0f, 0.0f));
            if (pixel && pixel->x() > widget.width() + 10.0) {
                justOutside = x;
            }
        }
        QVERIFY(justOutside > 0);
        constexpr int edgeRoomId = 10;
        QVERIFY(addRoomAt(edgeRoomId, areaId, justOutside, 0, 0));
        queueFrame(widget, areaId);
        QVERIFY2(roomQueuedAt(QPointF(justOutside, 0.0)), "a room just past the view's edge was left out");
    }

    // A diorama's pieces are laid out again for another area, or another level of it
    void test_dioramaPiecesFollowTheAreaAndLevelShownWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Other Area"));
        constexpr int otherRoomId = 11;
        QVERIFY(addRoomAt(otherRoomId, otherAreaId, 0, 0, 0));
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QCOMPARE(widget.mDioramaLayouts, 1);
        QVERIFY(!widget.mDioramaPiecesCache.pieces.contains(otherRoomId));
        queueFrame(widget, otherAreaId);
        QCOMPARE(widget.mDioramaLayouts, 2);
        QVERIFY(widget.mDioramaPiecesCache.pieces.contains(otherRoomId));
        queueFrame(widget, areaId, -1);
        QCOMPARE(widget.mDioramaLayouts, 3);
        QCOMPARE(widget.mDioramaPiecesCache.centerZ, -1);
    }

    // Only a change to the map counts its revision up, not the repaint every move of the player asks for, so what the
    // view keeps is not made again on every move; a map read in, or user data a script clears, does
    void test_theMapsRevisionCountsChangesNotMovesWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QCOMPARE(widget.mExitSceneBuilds, 1);

        quint64 revision = map()->revision();
        map()->repaintArea(areaId);
        QCOMPARE(map()->revision(), revision);
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("centerview(%1) updateMap()").arg(kEastRoomId)));
        QCOMPARE(map()->revision(), revision);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mExitSceneBuilds, 1);

        map()->mUserData.insert(qsl("perf.test.item"), qsl("1"));
        revision = map()->revision();
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("clearMapUserDataItem('perf.test.item')")));
        QVERIFY2(map()->revision() > revision, "clearing a map user data item left the revision as it was");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString file = directory.filePath(qsl("map.json"));
        QVERIFY(map()->writeJsonMapFile(file).first);
        revision = map()->revision();
        QVERIFY(map()->readJsonMapFile(file).first);
        QVERIFY2(map()->revision() > revision, "reading a map in left the revision as it was");
    }

    void test_keptOutputMatchesAFreshViewAfterAnEditWithoutGL_data()
    {
        QTest::addColumn<QString>("style");
        QTest::addColumn<QString>("edit");
        const QList<std::pair<QString, QString>> edits{{qsl("setDoor"), qsl("setDoor(1, 'e', 2)")},
                                                       {qsl("lockExit"), qsl("lockExit(1, 'east', true)")},
                                                       {qsl("addCustomLine"), qsl("addCustomLine(1, {{1, 1, 0}, {2, 1, 0}}, 'e', 'dash line', {255, 0, 0}, true)")},
                                                       {qsl("removeCustomLine"), qsl("removeCustomLine(2, 'w')")},
                                                       {qsl("setExitStub"), qsl("setExitStub(2, 'n', true)")},
                                                       {qsl("addSpecialExit"), qsl("addSpecialExit(1, 3, 'jump')")},
                                                       {qsl("setRoomEnv"), qsl("setRoomEnv(2, 5)")},
                                                       {qsl("setCustomEnvColor"), qsl("setCustomEnvColor(300, 10, 200, 10, 255)")},
                                                       {qsl("setRoomCoordinates"), qsl("setRoomCoordinates(2, 4, 2, 0)")},
                                                       {qsl("deleteRoom"), qsl("deleteRoom(2)")},
                                                       {qsl("setRoomArea"), qsl("setRoomArea(2, %1)")},
                                                       {qsl("setMapEnvironmentKit"), qsl("setMapEnvironmentKit(300, 'water')")},
                                                       {qsl("setMapBackgroundColor"), qsl("setMapBackgroundColor(20, 40, 60)")},
                                                       {qsl("setMapRoomExitsColor"), qsl("setMapRoomExitsColor(200, 30, 30)")},
                                                       {qsl("highlightRoom"), qsl("highlightRoom(2, 255, 0, 0, 0, 0, 255, 1, 255, 255)")},
                                                       {qsl("auditAreas"), qsl("auditAreas()")},
                                                       {qsl("in-out stubs"), qsl("setConfig('experiment.render-in-out-exits', true)")},
                                                       {qsl("more transparent"), qsl("setConfig('experiment.rendering.more-transparent', true)")}};
        for (const QString& style : {qsl("tiles"), qsl("inkwash"), qsl("diorama"), qsl("hologram"), qsl("terminal"), qsl("transit")}) {
            for (const auto& [name, edit] : edits) {
                QTest::newRow(qPrintable(qsl("%1 %2").arg(style, name))) << style << edit;
            }
        }
    }

    // Whatever a script changes, a view that kept what it made from the map draws the same as one made afresh
    void test_keptOutputMatchesAFreshViewAfterAnEditWithoutGL()
    {
        QFETCH(QString, style);
        QFETCH(QString, edit);
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const int otherAreaId = addKeptOutputArea(areaId);
        QVERIFY(otherAreaId > 0);
        const QColor savedBackground = mpHost->mBgColor_2;
        const QColor savedExits = mpHost->mFgColor_2;
        const auto restore = qScopeGuard([this, savedBackground, savedExits]() {
            mpHost->setMapStyle(qsl("tiles"));
            mpHost->mBgColor_2 = savedBackground;
            mpHost->mFgColor_2 = savedExits;
            mpHost->setExperimentEnabled(qsl("experiment.render-in-out-exits"), false);
            mpHost->setExperimentEnabled(qsl("experiment.rendering.more-transparent"), false);
        });
        QVERIFY(mpHost->setMapStyle(style));
        ModernGLWidget kept(map(), mpHost);
        queueFrame(kept, areaId);
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(edit.arg(otherAreaId)));
        queueTurnedFrame(kept);
        ModernGLWidget fresh(map(), mpHost);
        queueFrame(fresh, areaId);
        QCOMPARE(keptSnapshot(kept), keptSnapshot(fresh));
    }

    // An audit may put rooms back in the area they name, bypassing the setters; what the view kept follows it
    void test_keptOutputFollowsAnAuditWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(addKeptOutputArea(areaId) > 0);
        TRoom* pEast = map()->mpRoomDB->getRoom(kEastRoomId);
        pEast->setExitStub(DIR_NORTH, true);
        // The area forgets the room, which still names it, so the room's stub is not drawn
        map()->mpRoomDB->getArea(areaId)->rooms.remove(kEastRoomId);
        ModernGLWidget kept(map(), mpHost);
        queueFrame(kept, areaId);
        const QStringList before = keptSnapshot(kept);
        map()->audit();
        queueTurnedFrame(kept);
        ModernGLWidget fresh(map(), mpHost);
        queueFrame(fresh, areaId);
        QVERIFY(keptSnapshot(fresh) != before);
        QCOMPARE(keptSnapshot(kept), keptSnapshot(fresh));
    }

    // Textures the last frame used survive making those of a frame that needs others, so going back to the last
    // frame's makes none again
    void test_goingBackToTheLastFramesNamesMakesNoTexturesWithoutGL()
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Renamed Area"));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(100 + i, areaId, (i % 3) * 3, (i / 3) * 3, 0));
        }
        map()->mRoomIdHash[map()->mProfileName] = 100;
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            map()->setRoomNamesShown(false);
        });
        const auto name = [this](const QString& set) {
            for (int i = 0; i < 6; ++i) {
                map()->mpRoomDB->getRoom(100 + i)->name = qsl("%1 %2").arg(set).arg(i);
            }
        };
        ModernGLWidget widget(map(), mpHost);
        widget.mMaxRoomTextures = 16;
        name(qsl("Alpha"));
        queueFrame(widget, areaId);
        const qint64 afterAlpha = widget.mLabelTextureCache.imageTexturesMade();
        name(qsl("Bravo"));
        queueFrame(widget, areaId);
        QCOMPARE(widget.mLabelTextureCache.imageTexturesMade(), afterAlpha + 6);
        name(qsl("Charlie"));
        queueFrame(widget, areaId);
        const qint64 afterCharlie = widget.mLabelTextureCache.imageTexturesMade();
        QCOMPARE(afterCharlie, afterAlpha + 12);
        name(qsl("Bravo"));
        queueFrame(widget, areaId);
        QCOMPARE(widget.mLabelTextureCache.imageTexturesMade(), afterCharlie);
    }

    // From above, at a slant from several sides, and from close to the horizon with rooms behind the camera, every
    // room any corner of which shows in the view is drawn
    void test_everyRoomShowingInTheViewIsDrawnWithoutGL()
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Culled Area"));
        constexpr int side = 24;
        for (int y = 0; y < side; ++y) {
            for (int x = 0; x < side; ++x) {
                QVERIFY(addRoomAt(1000 + y * side + x, areaId, (x - side / 2) * 2, (y - side / 2) * 2, 0));
            }
        }
        map()->mRoomIdHash[map()->mProfileName] = 1000 + (side / 2) * side + side / 2;
        ModernGLWidget widget(map(), mpHost);
        widget.mLabelTextureCache.useTextureStandIn(
                [lastTexture = GLuint{0}](const QImage&) mutable {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.setViewCenter(areaId, 0, 0, 0);
        struct Pose
        {
            float distance;
            float tilt;
            float azimuth;
        };
        const QList<Pose> poses{{1.0f, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth},
                                {1.0f, CameraController::scmDefaultTilt, CameraController::scmNorthUpAzimuth},
                                {1.0f, CameraController::scmDefaultTilt, 20.0f},
                                {1.0f, CameraController::scmDefaultTilt, 135.0f},
                                {0.3f, CameraController::scmMaxTilt, 200.0f}};
        for (const Pose& pose : poses) {
            widget.setCameraPosition(pose.distance, pose.tilt, pose.azimuth);
            widget.updateMatrices();
            widget.mRenderCommandQueue.clear();
            widget.queueScene();
            QSet<std::pair<float, float>> queued;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get())) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        if (cube.transform.column(0).toVector3D().length() > 0.2f) {
                            queued.insert({cube.transform.column(3).x(), cube.transform.column(3).y()});
                        }
                    }
                }
            }
            int showing = 0;
            int left = 0;
            for (int y = 0; y < side; ++y) {
                for (int x = 0; x < side; ++x) {
                    const auto rx = static_cast<float>((x - side / 2) * 2);
                    const auto ry = static_cast<float>((y - side / 2) * 2);
                    bool shows = false;
                    for (const QVector3D& corner :
                         {QVector3D(rx - 0.5f, ry - 0.5f, 0.0f), QVector3D(rx + 0.5f, ry - 0.5f, 0.0f), QVector3D(rx - 0.5f, ry + 0.5f, 0.0f), QVector3D(rx + 0.5f, ry + 0.5f, 0.0f)}) {
                        const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(corner);
                        shows = shows || (pixel && QRectF(widget.rect()).contains(*pixel));
                    }
                    if (shows) {
                        ++showing;
                        QVERIFY2(queued.contains({rx, ry}), qPrintable(qsl("the room at %1,%2 shows from tilt %3, azimuth %4 but was not drawn").arg(rx).arg(ry).arg(pose.tilt).arg(pose.azimuth)));
                    } else if (!queued.contains({rx, ry})) {
                        ++left;
                    }
                }
            }
            QVERIFY(showing > 0);
            if (pose.tilt > CameraController::scmDefaultTilt) {
                QVERIFY2(left > 0, "close to the horizon, no room behind the camera was left out");
            }
        }
    }

    void test_whatARoomOutOfViewDrawsIntoTheViewIsDrawnWithoutGL_data()
    {
        QTest::addColumn<int>("roomX");
        QTest::addColumn<QString>("edit");
        QTest::newRow("name moved into the view") << 60 << qsl("setRoomName(7000, 'Far Name') setRoomUserData(7000, 'room.ui_nameOffset', '-60 0')");
        QTest::newRow("highlight reaching into the view") << 60 << qsl("highlightRoom(7000, 255, 0, 0, 0, 0, 255, 80, 255, 255)");
        QTest::newRow("name of a room in view") << 1 << qsl("setRoomName(7000, 'Near Name')");
    }

    // A room's name may be moved far from it, and its highlight reach far around it: either is drawn while it shows
    void test_whatARoomOutOfViewDrawsIntoTheViewIsDrawnWithoutGL()
    {
        QFETCH(int, roomX);
        QFETCH(QString, edit);
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Far Decal Area"));
        QVERIFY(addRoomAt(7000, areaId, roomX, 0, 0));
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            map()->setRoomNamesShown(false);
        });
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(edit));
        ModernGLWidget widget(map(), mpHost);
        widget.mLabelTextureCache.useTextureStandIn(
                [lastTexture = GLuint{0}](const QImage&) mutable {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.setCameraPosition(0.3f, CameraController::scmMinTilt, CameraController::scmNorthUpAzimuth);
        widget.updateMatrices();
        const std::optional<QPointF> room = widget.mCameraController.pixelForPoint(QVector3D(static_cast<float>(roomX), 0.0f, 0.0f));
        QVERIFY(room);
        QCOMPARE(QRectF(widget.rect()).adjusted(-800.0, -600.0, 800.0, 600.0).contains(*room), roomX < 10);
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        bool drawnAtTheCenter = false;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get())) {
                const QVector<float>& vertices = textured->geometry().vertices;
                QRectF spans;
                for (qsizetype i = 0; i + 2 < vertices.size(); i += 3) {
                    spans = spans.united(QRectF(vertices.at(i), vertices.at(i + 1), 0.001, 0.001));
                }
                drawnAtTheCenter = drawnAtTheCenter || spans.intersects(QRectF(-0.5, -1.5, 1.0, 2.0));
            }
        }
        QVERIFY2(drawnAtTheCenter, "what the room draws at the view's center was left out");
    }

    // The terminal style's exit characters showing in the view are all drawn, as are their rooms
    void test_everyTerminalExitShowingInTheViewIsDrawnWithoutGL()
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Terminal Culled Area"));
        constexpr int side = 16;
        for (int y = 0; y < side; ++y) {
            for (int x = 0; x < side; ++x) {
                const int roomId = 1000 + y * side + x;
                QVERIFY(addRoomAt(roomId, areaId, (x - side / 2) * 3, (y - side / 2) * 3, 0));
                if (x > 0) {
                    QVERIFY(map()->setExit(roomId, roomId - 1, DIR_WEST));
                    QVERIFY(map()->setExit(roomId - 1, roomId, DIR_EAST));
                }
            }
        }
        map()->mRoomIdHash[map()->mProfileName] = 1000 + (side / 2) * side + side / 2;
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("terminal")));
        ModernGLWidget widget(map(), mpHost);
        // A whole frame first, as the exit characters come from the exits
        queueFrame(widget, areaId);
        const QList<DrawnGlyph> drawn = terminalGlyphs(widget, areaId);
        int showing = 0;
        for (const ExitScene::Glyph& glyph : std::as_const(widget.mExitGlyphs)) {
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(glyph.at);
            if (!pixel || !QRectF(widget.rect()).contains(*pixel)) {
                continue;
            }
            ++showing;
            const bool found = std::any_of(drawn.cbegin(), drawn.cend(), [&glyph](const DrawnGlyph& each) {
                return QLineF(each.center, glyph.at.toPointF()).length() < 0.05;
            });
            QVERIFY2(found, qPrintable(qsl("the exit character at %1,%2 shows but was not drawn").arg(glyph.at.x()).arg(glyph.at.y())));
        }
        QVERIFY(showing > 0);
    }

    // The diorama's pieces are laid out once a frame at most, and again only when the map, the levels shown or the
    // style changes
    void test_dioramaPiecesAreLaidOutAgainOnlyWhenTheMapOrLevelsChangeWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        QVERIFY(map()->setExit(kPlayerRoomId, kEastRoomId, DIR_EAST));
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        QCOMPARE(widget.mDioramaLayouts, 1);
        queueTurnedFrame(widget);
        widget.setCameraDistance(3.0f);
        queueTurnedFrame(widget);
        QCOMPARE(widget.mDioramaLayouts, 1);
        QVERIFY(widget.mDioramaPiecesCache.pieces.contains(kEastRoomId));
        const EnvironmentKit::Kit before = widget.mDioramaPiecesCache.pieces.value(kEastRoomId).kit;

        // Water, by its color
        const int water = 301;
        const auto forgetWater = qScopeGuard([this, water]() {
            map()->mCustomEnvColors.remove(water);
        });
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("setCustomEnvColor(%1, 30, 90, 220, 255) setRoomEnv(%2, %1)").arg(water).arg(kEastRoomId)));
        queueTurnedFrame(widget);
        QCOMPARE(widget.mDioramaLayouts, 2);
        QCOMPARE(widget.mDioramaPiecesCache.pieces.value(kEastRoomId).kit, EnvironmentKit::Kit::Water);
        QVERIFY(widget.mDioramaPiecesCache.pieces.value(kEastRoomId).kit != before);

        widget.slot_showLessUpperLevels();
        queueTurnedFrame(widget);
        QCOMPARE(widget.mDioramaLayouts, 3);

        // Another style has no pieces, and coming back to the diorama's finds them as they were
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        queueTurnedFrame(widget);
        QCOMPARE(widget.mDioramaLayouts, 3);
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        queueTurnedFrame(widget);
        QCOMPARE(widget.mDioramaLayouts, 3);
        QVERIFY(widget.mDioramaPiecesCache.pieces.contains(kEastRoomId));
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

        // A wash wider than the room under it, laid over the level below and under the one above
        const float roomSize = 2.0f * lozenge.first().transform.column(0).toVector3D().length();
        qsizetype lastBelow = -1;
        qsizetype firstAbove = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get())) {
                if (cubes->shading() == RenderCommand::Shading::InkDashed) {
                    lastBelow = index;
                } else if (cubes->shading() == RenderCommand::Shading::InkHatched && firstAbove < 0) {
                    firstAbove = index;
                }
            }
        }
        QVERIFY(lastBelow >= 0 && firstAbove > lastBelow);
        int washes = 0;
        qsizetype washedAt = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
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
                    washedAt = index;
                }
            }
        }
        QCOMPARE(washes, 1);
        QVERIFY2(washedAt > lastBelow && washedAt < firstAbove, "the wash is not between the levels below and above");

        // A room's own border still shows, under its line
        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderColor = QColor(200, 40, 40);
        queueFrame(widget, areaId);
        QVERIFY2(!atPlace(RenderCommand::Shading::InkBlot, QPointF(2.0, 0.0)).isEmpty(), "a room's own border was dropped");
        // See-through, it is still a blot rather than a second lozenge round the room
        map()->mpRoomDB->getRoom(kEastRoomId)->mBorderColor = QColor(200, 40, 40, 120);
        queueFrame(widget, areaId);
        QVERIFY2(!atPlace(RenderCommand::Shading::InkBlot, QPointF(2.0, 0.0)).isEmpty() && atPlace(RenderCommand::Shading::InkLozenge, QPointF(2.0, 0.0)).size() == 1,
                 "a see-through border was not a blot");
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

    // Only the viewed level's shown rooms are washed, each the same way every frame, and neither the washes nor the
    // marks, symbols and labels after them write depth, whose quads' clear margins would cut holes in what follows
    void test_inkwashWashesTheViewedLevelSteadilyWithoutWritingDepth()
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
        constexpr int kHiddenRoomId = 7;
        constexpr int kFirstRowId = 10;
        constexpr int kRowRooms = 8;
        QVERIFY(addRoomAt(kUpperRoomId, areaId, 2, 2, 1));
        QVERIFY(addRoomAt(kHiddenRoomId, areaId, 4, 0, 0));
        map()->mpRoomDB->getRoom(kHiddenRoomId)->setHidden(true);
        QList<QPointF> viewed{QPointF(0.0, 0.0), QPointF(2.0, 0.0)};
        for (int i = 0; i < kRowRooms; ++i) {
            QVERIFY(addRoomAt(kFirstRowId + i, areaId, 2 * i - 6, -2, 0));
            viewed.append(QPointF(2 * i - 6, -2));
        }
        map()->mpRoomDB->getRoom(kEastRoomId)->mSymbol = qsl("$");
        const auto clearSymbol = qScopeGuard([this]() {
            map()->mpRoomDB->getRoom(kEastRoomId)->mSymbol.clear();
        });
        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);

        const QList<WashQuad> washes = washQuads(widget);
        QList<QPointF> washed;
        QSet<int> variants;
        for (const WashQuad& wash : washes) {
            washed.append(wash.center());
            variants.insert(wash.variant);
        }
        QCOMPARE(washed.size(), viewed.size());
        for (const QPointF& place : std::as_const(viewed)) {
            QVERIFY2(std::any_of(washed.cbegin(),
                                 washed.cend(),
                                 [&place](const QPointF& center) {
                                     return QLineF(center, place).length() < 1e-3;
                                 }),
                     qPrintable(qsl("room at %1,%2 was not washed").arg(place.x()).arg(place.y())));
        }
        QVERIFY2(variants.size() >= 2, "every room was washed the same way");

        // Up to the level above, everything after the first wash is drawn without writing depth
        const auto& commands = widget.mRenderCommandQueue.commands();
        qsizetype firstAbove = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()) && firstAbove < 0; ++index) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get()); cubes && cubes->shading() == RenderCommand::Shading::InkHatched) {
                firstAbove = index;
            }
        }
        QVERIFY(!washes.isEmpty() && firstAbove > washes.first().command);
        int afterWashes = 0;
        for (qsizetype index = washes.first().command; index < firstAbove; ++index) {
            if (dynamic_cast<const RenderTexturedTrianglesCommand*>(commands.at(index).get())) {
                QVERIFY2(!depthWritesAt(widget, index), qPrintable(qsl("command %1, after the washes, writes depth").arg(index)));
                afterWashes += index > washes.last().command;
            }
        }
        QVERIFY2(afterWashes > 0, "nothing was drawn after the washes, so their depth writes went untested");

        queueFrame(widget, areaId);
        QCOMPARE(washQuads(widget).size(), washes.size());
        const QList<WashQuad> again = washQuads(widget);
        for (qsizetype i = 0; i < washes.size(); ++i) {
            QVERIFY2(again.at(i).variant == washes.at(i).variant && again.at(i).corner == washes.at(i).corner && again.at(i).opposite == washes.at(i).opposite, "a wash changed between frames");
        }
    }

    // More symbols than the view has textures for still leave every room its wash
    void test_inkwashWashesSurviveATextureCrowdWithoutGL()
    {
        constexpr int kRooms = 540;
        constexpr int kColumns = 30;
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Crowded Area"));
        QVERIFY(areaId > 0);
        for (int i = 0; i < kRooms; ++i) {
            QVERIFY(addRoomAt(1000 + i, areaId, i % kColumns - kColumns / 2, i / kColumns - kRooms / kColumns / 2, 0));
            map()->mpRoomDB->getRoom(1000 + i)->mSymbol = QString::number(i);
        }
        map()->mRoomIdHash[map()->mProfileName] = 0;
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));

        ModernGLWidget widget(map(), mpHost);
        widget.mLabelTextureCache.useTextureStandIn(
                [lastTexture = GLuint{0}](const QImage&) mutable {
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

        QCOMPARE(washQuads(widget).size(), kRooms);
    }

    // A forest or water room is marked only where it would read: on the viewed level, not the player's, with no symbol
    // or ID shown in its place, and big enough on screen
    void test_inkwashMarksOnlyRoomsThatShowThemWithoutGL()
    {
        constexpr int kForest = 310;
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mCustomEnvColors[kForest] = QColor(40, 140, 60);
        QVERIFY(EnvironmentKit::store(map()->mUserData, kForest, EnvironmentKit::Kit::Forest));
        for (const int roomId : {kPlayerRoomId, kEastRoomId, kLowerRoomId}) {
            map()->mpRoomDB->getRoom(roomId)->environment = kForest;
        }
        const bool savedLevels = mudlet::self()->drawUpperLowerLevels();
        const bool savedIds = mpHost->mShowRoomID;
        const auto restore = qScopeGuard([this, savedLevels, savedIds, kForest]() {
            map()->mCustomEnvColors.remove(kForest);
            map()->mUserData.remove(EnvironmentKit::userDataKey());
            map()->mpRoomDB->getRoom(kEastRoomId)->mSymbol.clear();
            mpHost->mShowRoomID = savedIds;
            mudlet::self()->setDrawUpperLowerLevels(savedLevels);
            mpHost->setMapStyle(qsl("tiles"));
        });
        mudlet::self()->setDrawUpperLowerLevels(true);
        mpHost->mShowRoomID = false;
        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        ModernGLWidget widget(map(), mpHost);
        const auto marks = [&widget]() {
            QList<QPointF> found;
            for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                if (!key.startsWith(qsl("mark|"))) {
                    continue;
                }
                const GLuint texture = widget.mLabelTextureCache.imageTexture(key);
                for (const auto& command : widget.mRenderCommandQueue.commands()) {
                    if (const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get()); textured && textured->geometry().textureId == texture) {
                        const QVector<float>& vertices = textured->geometry().vertices;
                        for (qsizetype quad = 0; quad + 18 <= vertices.size(); quad += 18) {
                            found.append(QPointF((vertices.at(quad) + vertices.at(quad + 6)) / 2.0, (vertices.at(quad + 1) + vertices.at(quad + 7)) / 2.0));
                        }
                    }
                }
            }
            return found;
        };

        queueFrame(widget, areaId);
        const QList<QPointF> shown = marks();
        QCOMPARE(shown.size(), 1);
        QVERIFY2(QLineF(shown.first(), QPointF(2.0, 0.0)).length() < 0.5, "the mark is not on the room east of the player");

        mpHost->mShowRoomID = true;
        queueFrame(widget, areaId);
        QVERIFY2(marks().isEmpty(), "a room was marked under its ID");
        mpHost->mShowRoomID = false;

        map()->mpRoomDB->getRoom(kEastRoomId)->mSymbol = qsl("$");
        queueFrame(widget, areaId);
        QVERIFY2(marks().isEmpty(), "a room was marked under its symbol");
        map()->mpRoomDB->getRoom(kEastRoomId)->mSymbol.clear();

        queueFrame(widget, areaId);
        QCOMPARE(marks().size(), 1);
        widget.mCameraController.setScale(CameraController::scmMaxDistance);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        QVERIFY2(marks().isEmpty(), "a room too small to read a mark on was marked");
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

    // A painted miniature: each room keeps its environment's hue, a little desaturated and kept from the darkest and
    // lightest, under a pale sky and in tilt-shift focus, with the player a red pawn
    void test_dioramaPaintsMiniatures()
    {
        const MapStyle* diorama = MapStyle::find(qsl("diorama"));
        QVERIFY(diorama);
        const QColor red = diorama->roomColor(QColor(230, 40, 40, 200));
        const QColor green = diorama->roomColor(QColor(40, 230, 40));
        const QColor blue = diorama->roomColor(QColor(40, 40, 230));
        const QColor black = diorama->roomColor(QColor(Qt::black));
        const QColor white = diorama->roomColor(QColor(Qt::white));
        QCOMPARE(red.alpha(), 200);
        QVERIFY2(qAbs(red.hue() - 0) < 15 || qAbs(red.hue() - 360) < 15, "a red room lost its hue");
        QVERIFY2(qAbs(green.hue() - 120) < 15, "a green room lost its hue");
        QVERIFY2(qAbs(blue.hue() - 240) < 15, "a blue room lost its hue");
        for (const QColor& room : {red, green, blue}) {
            QVERIFY2(room.saturation() < 170 && room.saturation() > 90, qPrintable(qsl("a bright room is %1 saturated").arg(room.saturation())));
        }
        QVERIFY2(black.lightness() >= 60, qPrintable(qsl("a black room is %1 light, too dark to shade").arg(black.lightness())));
        QVERIFY2(white.lightness() <= 215, qPrintable(qsl("a white room is %1 light, too light to shade").arg(white.lightness())));
        // Warmed, as model paints are
        QVERIFY2(white.red() > white.blue() + 8, "a white room is not warmed");
        QVERIFY2(difference(red, green) >= 60 && difference(green, blue) >= 60 && difference(blue, red) >= 60, "areas' rooms cannot be told apart");
        QVERIFY2(diorama->background.lightness() > 180, "the sky is not pale");
        const QColor token = diorama->playerRoomColor();
        QVERIFY2(token.red() > token.green() + 100 && token.red() > token.blue() + 100, "the token is not red");
        QCOMPARE(diorama->edges, MapStyle::Edges::ModelBlocks);
        QCOMPARE(diorama->playerMarker, MapStyle::PlayerMarker::Token);
        QVERIFY(diorama->postEffects.testFlag(MapStyle::PostEffect::Sky));
        QVERIFY(diorama->postEffects.testFlag(MapStyle::PostEffect::TiltShift));
        QVERIFY(diorama->environmentKits);
        QVERIFY(!diorama->postEffects.testFlag(MapStyle::PostEffect::PaperGrain));
        QVERIFY(diorama->lit);
        const MapStyle* inkwash = MapStyle::find(qsl("inkwash"));
        QVERIFY(inkwash);
        QVERIFY(!inkwash->postEffects.testFlag(MapStyle::PostEffect::Sky));
        QVERIFY(!inkwash->postEffects.testFlag(MapStyle::PostEffect::TiltShift));
        QVERIFY(!inkwash->environmentKits);
    }

    // The sky laid under everything, each room a building with a soft shadow on the ground beside it, the player's room
    // keeping its own color with a pawn standing on it and no beacon, and the exits low paths on the ground
    void test_dioramaBuildsATabletopModelWithoutGL()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kEastRoomId, DIR_EAST);
        map()->mpRoomDB->getRoom(kEastRoomId)->setExit(kPlayerRoomId, DIR_WEST);
        // Up to a room that is not straight above, so drawn as a shaft
        map()->mpRoomDB->getRoom(kLowerRoomId)->setExit(kEastRoomId, DIR_UP);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->setExit(kLowerRoomId, DIR_NORTH);
        map()->mpRoomDB->getRoom(kLowerRoomId)->setExit(kPlayerRoomId, DIR_SOUTH);
        // A door on the way east, whose strokes stand taller than the line; and a path on the level below
        map()->mpRoomDB->getRoom(kPlayerRoomId)->doors[qsl("e")] = 1;
        constexpr int kLowerEastRoomId = 4;
        QVERIFY(addRoomAt(kLowerEastRoomId, areaId, 2, 2, -1));
        map()->mpRoomDB->getRoom(kLowerRoomId)->setExit(kLowerEastRoomId, DIR_EAST);
        map()->mpRoomDB->getRoom(kLowerEastRoomId)->setExit(kLowerRoomId, DIR_WEST);
        ModernGLWidget widget(map(), mpHost);
        const auto& commands = widget.mRenderCommandQueue.commands();
        // Anything but a model's own shapes standing above the player's room: the beacon
        const auto beaconQueued = [&commands]() {
            for (const auto& command : commands) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() != RenderCommand::Shading::ModelBlock) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        const QVector4D center = cube.transform.column(3);
                        if (qAbs(center.x()) < 1e-4f && qAbs(center.y()) < 1e-4f && center.z() > kRoomHeight) {
                            return true;
                        }
                    }
                }
            }
            return false;
        };
        const auto atPlace = [&commands](const RenderCommand::Shading shading, const QPointF& place) {
            QList<CubeInstanceData> found;
            for (const auto& command : commands) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == shading) {
                    found.append(roomInstances(*cubes, place));
                }
            }
            return found;
        };
        const auto lastIndexOf = [&commands](const RenderCommand::Shading shading) {
            qsizetype last = -1;
            for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get()); cubes && cubes->shading() == shading) {
                    last = index;
                }
            }
            return last;
        };

        queueFrame(widget, areaId);
        QVERIFY2(beaconQueued(), "tiles raised no beacon, so its absence proves nothing");

        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        queueFrame(widget, areaId);
        qsizetype sky = -1;
        qsizetype firstDrawn = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(commands.at(index).get());
            if (triangles && triangles->shading() == RenderCommand::Shading::Sky) {
                QVERIFY2(sky < 0, "the sky was laid twice");
                sky = index;
                // Bluer at the top of the view than at its foot
                const QList<float>& vertices = triangles->vertices();
                const QList<float>& colors = triangles->colors();
                for (qsizetype vertex = 0; vertex * 3 + 1 < vertices.size(); ++vertex) {
                    const float blueOverRed = colors.at(vertex * 4 + 2) - colors.at(vertex * 4);
                    QVERIFY2(vertices.at(vertex * 3 + 1) > 0.0f ? blueOverRed > 0.05f : blueOverRed < 0.0f, "the sky has no gradient");
                }
            }
            QVERIFY2(!triangles || triangles->shading() != RenderCommand::Shading::Paper, "the sky is drawn as paper");
            if (firstDrawn < 0 && (triangles || dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get()))) {
                firstDrawn = index;
            }
        }
        QVERIFY2(sky >= 0, "no sky was laid");
        QCOMPARE(sky, firstDrawn);

        // As renderRooms sizes it: 2 / scale (3) / zFlattening (2)
        constexpr float roomHeight = 2.0f / 3.0f / 2.0f;
        const QList<CubeInstanceData> east = atPlace(RenderCommand::Shading::ModelBlock, QPointF(2.0, 0.0));
        QVERIFY2(!east.isEmpty(), "a room is not a block");
        QVERIFY2(atPlace(RenderCommand::Shading::Lit, QPointF(2.0, 0.0)).isEmpty() && atPlace(RenderCommand::Shading::Unlit, QPointF(2.0, 0.0)).isEmpty(), "a room was drawn as a tile");
        // The pawn stands there too, as other shapes than the room's block
        QList<CubeInstanceData> player;
        for (const auto& command : commands) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get());
                cubes && cubes->shading() == RenderCommand::Shading::ModelBlock && cubes->shape() == GeometryManager::InstanceShape::Box) {
                player.append(roomInstances(*cubes, QPointF(0.0, 0.0)));
            }
        }
        QVERIFY(!player.isEmpty());
        const QColor tokenColor = MapStyle::find(qsl("diorama"))->playerRoomColor();
        for (const CubeInstanceData& block : player) {
            QVERIFY2(!qFuzzyCompare(block.color[0], static_cast<float>(tokenColor.redF())) || !qFuzzyCompare(block.color[1], static_cast<float>(tokenColor.greenF())),
                     "the player's room was painted in the token's color");
        }

        // Toward the south-east, on the ground the room stands on, under where the block itself stands
        const auto shadowBeside = [&commands, roomHeight](const QPointF& place) {
            for (const auto& command : commands) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelShadow) {
                    for (const CubeInstanceData& cube : cubes->instances()) {
                        const QVector4D center = cube.transform.column(3);
                        if (center.x() > place.x() + 0.01 && center.x() < place.x() + 0.5 && center.y() < place.y() - 0.01 && center.y() > place.y() - 0.5
                            && qAbs(center.z() + roomHeight / 2.0f) < 0.01f && cube.color[3] > 0.3f && cube.transform.column(0).toVector3D().length() > 1.0f / 3.0f) {
                            return true;
                        }
                    }
                }
            }
            return false;
        };
        QVERIFY2(shadowBeside(QPointF(2.0, 0.0)), "a room casts no shadow");
        QVERIFY2(shadowBeside(QPointF(0.0, 0.0)), "the player's room casts no shadow");
        // The pawn's faint ghost is drawn last of all, over the shadows too
        qsizetype lastOpaqueBlock = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get());
                cubes && cubes->shading() == RenderCommand::Shading::ModelBlock && !cubes->instances().isEmpty() && cubes->instances().constFirst().color[3] >= 1.0f) {
                lastOpaqueBlock = index;
            }
        }
        QVERIFY2(lastIndexOf(RenderCommand::Shading::ModelShadow) > lastOpaqueBlock, "the shadows are drawn before the blocks that hide them");

        // A pawn in the player's color standing on the player's room's roof, and nowhere else: a round foot, a body
        // narrowing up to a ball for a head, each drawn once as it is and once faintly where something hides it
        QMap<GeometryManager::InstanceShape, int> pieces;
        float footTop = 0.0f;
        float headBottom = 0.0f;
        for (const auto& command : commands) {
            const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get());
            if (!cubes || cubes->shading() != RenderCommand::Shading::ModelBlock || cubes->shape() == GeometryManager::InstanceShape::Box
                || cubes->shape() == GeometryManager::InstanceShape::Pyramid) {
                continue;
            }
            for (const CubeInstanceData& cube : cubes->instances()) {
                const QVector4D center = cube.transform.column(3);
                QVERIFY2(qAbs(center.x()) < 1e-4f && qAbs(center.y()) < 1e-4f, "a pawn stands off the player's room");
                QVERIFY2(center.z() > roomHeight, "a pawn's piece is sunk in the building");
                QVERIFY(qFuzzyCompare(cube.color[0], static_cast<float>(tokenColor.redF())));
                if (cube.color[3] < 1.0f) {
                    continue;
                }
                ++pieces[cubes->shape()];
                if (cubes->shape() == GeometryManager::InstanceShape::Cylinder) {
                    footTop = center.z() + cube.transform.column(2).z();
                } else if (cubes->shape() == GeometryManager::InstanceShape::Sphere) {
                    headBottom = center.z() - cube.transform.column(2).z();
                }
            }
        }
        QCOMPARE(pieces.value(GeometryManager::InstanceShape::Cylinder), 1);
        QCOMPARE(pieces.value(GeometryManager::InstanceShape::Cone), 1);
        QCOMPARE(pieces.value(GeometryManager::InstanceShape::Sphere), 1);
        QVERIFY2(headBottom > footTop, "the pawn's head is not above its foot");
        QVERIFY2(!beaconQueued(), "a beacon stands over the token");

        // The path east from the player's room lies on the ground, not halfway up the blocks
        bool path = false;
        float pathFoot = 0.0f;
        for (const auto& command : commands) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelPath) {
                for (const CubeInstanceData& cube : cubes->instances()) {
                    const QVector4D center = cube.transform.column(3);
                    if (qAbs(center.x() - 1.0f) < 0.3f && qAbs(center.y()) < 1e-4f) {
                        QVERIFY2(center.z() < -roomHeight / 4.0f && center.z() > -roomHeight / 2.0f, qPrintable(qsl("the path's middle is at %1").arg(center.z())));
                        if (cube.color[3] >= 1.0f && qAbs(cube.transform.column(0).x()) > 0.2f) {
                            pathFoot = center.z() - cube.transform.column(2).z();
                            path = true;
                        }
                    }
                }
            }
        }
        QVERIFY2(path, "the exit between the rooms is not a path");

        // The same path, door and all, shows again as a faint line just over the thickest tile, so over the paving and
        // grass that would hide it: only the viewed level's, and only where it lies flat, not up a shaft or a slope
        const float pavedFoot = pathFoot + DioramaLayout::flatTileTop(roomHeight);
        bool pavedLine = false;
        bool pavedDoor = false;
        for (const auto& command : commands) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelPath) {
                for (const CubeInstanceData& cube : cubes->instances()) {
                    const QVector4D center = cube.transform.column(3);
                    const float halfHeight = cube.transform.column(2).z();
                    // Thinner than any line is drawn, so the faint copy
                    if (cube.transform.column(2).toVector3D().length() >= 0.01f * roomHeight) {
                        continue;
                    }
                    QVERIFY2(qAbs(cube.transform.column(0).z()) < 1e-5f, qPrintable(qsl("a faint line climbs or falls at %1,%2").arg(center.x()).arg(center.y())));
                    QVERIFY2(center.y() < 1.0f, qPrintable(qsl("the level below's path has a faint line, at %1").arg(center.z())));
                    QVERIFY2(qAbs(center.z() - halfHeight - pavedFoot) < 1e-4f,
                             qPrintable(qsl("a faint line's foot is at %1, not just over the tiles at %2").arg(center.z() - halfHeight).arg(pavedFoot)));
                    QVERIFY(cube.color[3] < 0.5f);
                    if (qAbs(center.x() - 1.0f) < 0.3f && qAbs(center.y()) < 1e-4f) {
                        pavedLine = true;
                    }
                    pavedDoor = pavedDoor || qAbs(cube.transform.column(0).y()) > qAbs(cube.transform.column(0).x());
                }
            }
        }
        QVERIFY2(pavedLine, "the path has no line over the paving");
        QVERIFY2(pavedDoor, "the door has no faint strokes over the paving");

        // The shaft up from the room below, one-way and so in two pieces, starts on the ground as the paths do but rises
        // from where it did, clear of the block's top
        float shaftTop = std::numeric_limits<float>::lowest();
        float shaftFoot = std::numeric_limits<float>::max();
        for (const auto& command : commands) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelPath) {
                for (const CubeInstanceData& cube : cubes->instances()) {
                    const QVector4D center = cube.transform.column(3);
                    const float halfLength = cube.transform.column(0).z();
                    if (qAbs(center.x()) < 1e-4f && qAbs(center.y() - 2.0f) < 1e-4f && halfLength > 0.01f) {
                        shaftTop = qMax(shaftTop, center.z() + halfLength);
                        shaftFoot = qMin(shaftFoot, center.z() - halfLength);
                    }
                }
            }
        }
        QVERIFY2(shaftTop > -1.0f + roomHeight / 2.0f + 0.1f, qPrintable(qsl("the shaft only reaches %1").arg(shaftTop)));
        QVERIFY2(shaftFoot < -1.0f - roomHeight / 4.0f, qPrintable(qsl("the shaft starts at %1, off the ground").arg(shaftFoot)));

        // The line slanting down north to the room below lies on the ground all along, as the flat ones do
        bool slant = false;
        for (const auto& command : commands) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelPath) {
                for (const CubeInstanceData& cube : cubes->instances()) {
                    const QVector4D center = cube.transform.column(3);
                    if (qAbs(center.x()) < 1e-4f && center.y() > 0.3f && center.y() < 1.7f) {
                        QVERIFY2(center.z() < -center.y() / 2.0f - roomHeight / 4.0f, qPrintable(qsl("the slanting line is at %1 where it is %2 north").arg(center.z()).arg(center.y())));
                        slant = true;
                    }
                }
            }
        }
        QVERIFY2(slant, "the exit to the room below has no line");
    }

    // A dotted custom line west to a room on the level below ends in a piece straight down, split into dots before they
    // are laid on the ground: each dot keeps most of its length, so none is lost and the line stays joined up
    void test_dioramaKeepsADottedLineToAnotherLevelWhole()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int kWestLowerRoomId = 4;
        QVERIFY(addRoomAt(kWestLowerRoomId, areaId, -2, 0, -1));
        const auto restore = qScopeGuard([this, lineSize = mpHost->mLineSize]() {
            mpHost->mLineSize = lineSize;
            mpHost->setMapStyle(qsl("tiles"));
        });
        mpHost->mLineSize = 18.0;
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        pPlayerRoom->setExit(kWestLowerRoomId, DIR_WEST);
        pPlayerRoom->customLines.insert(qsl("w"), {QPointF(-2.0, 0.0)});
        pPlayerRoom->customLinesColor.insert(qsl("w"), QColor(Qt::cyan));
        pPlayerRoom->customLinesStyle.insert(qsl("w"), Qt::DotLine);
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);

        const auto atShaft = [](const QVector3D& point) {
            return qAbs(point.x() + 2.0f) < 1e-4f && qAbs(point.y()) < 1e-4f;
        };
        QList<float> drawn;
        for (const ExitScene::Segment& segment : ExitScene::build(*map(), *mpHost, widget.exitSceneOptions()).segments) {
            if (atShaft(segment.from) && atShaft(segment.to)) {
                drawn.append((segment.to - segment.from).length());
            }
        }
        QVERIFY2(drawn.size() >= 2, qPrintable(qsl("the piece down is in %1 dots").arg(drawn.size())));

        QList<float> queued;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelPath) {
                for (const CubeInstanceData& cube : cubes->instances()) {
                    const QVector4D axis = cube.transform.column(0);
                    if (atShaft(cube.transform.column(3).toVector3D()) && qAbs(axis.x()) < 1e-4f && qAbs(axis.y()) < 1e-4f) {
                        queued.append(2.0f * qAbs(axis.z()));
                    }
                }
            }
        }
        QCOMPARE(queued.size(), drawn.size());
        for (qsizetype i = 0; i < drawn.size(); ++i) {
            QVERIFY2(queued.at(i) > drawn.at(i) / 2.0f, qPrintable(qsl("a dot %1 long is laid %2 long").arg(drawn.at(i)).arg(queued.at(i))));
        }
    }

    // Looking at the level under the player's, whose room is then see-through: the token's shadow is drawn over the
    // room's top, not before it, or the room's paint would blend over the shadow
    void test_dioramaLaysTheTokenShadowOverASeeThroughRoom()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId, -1);
        const auto& commands = widget.mRenderCommandQueue.commands();
        qsizetype playerBlock = -1;
        qsizetype tokenShadow = -1;
        for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
            const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(commands.at(index).get());
            if (!cubes) {
                continue;
            }
            if (cubes->shading() == RenderCommand::Shading::ModelBlock && cubes->shape() != GeometryManager::InstanceShape::Cylinder && !roomInstances(*cubes, QPointF(0.0, 0.0)).isEmpty()) {
                QVERIFY2(cubes->instances().first().color[3] < 1.0f, "the player's room is not see-through from the level below");
                playerBlock = index;
            }
            if (cubes->shading() == RenderCommand::Shading::ModelShadow && cubes->shape() == GeometryManager::InstanceShape::Cylinder) {
                tokenShadow = index;
            }
        }
        QVERIFY2(playerBlock >= 0, "the player's room is not drawn");
        QVERIFY2(tokenShadow >= 0, "the token casts no shadow");
        QVERIFY2(tokenShadow > playerBlock, qPrintable(qsl("the token's shadow is command %1, before the player's room at %2").arg(tokenShadow).arg(playerBlock)));
    }

    // Rooms straight above one on the viewed level stand as one solid building, a storey each with the roof on the top
    // one, and each storey is picked where it shows; water lies in a hole cut in the board, a highlighted room glows on
    // the board, and naming an environment's kit rebuilds its rooms
    void test_dioramaBuildsKitsAndTowers()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int kFirstStoreyId = 11;
        constexpr int kWaterRoomId = 20;
        constexpr int kWater = 310;
        constexpr int kForestRoomId = 22;
        constexpr int kForest = 311;
        for (int storey = 1; storey <= 3; ++storey) {
            QVERIFY(addRoomAt(kFirstStoreyId + storey - 1, areaId, 2, 0, storey));
        }
        QVERIFY(addRoomAt(kWaterRoomId, areaId, 4, 0, 0));
        map()->mCustomEnvColors[kWater] = QColor(60, 120, 200);
        map()->mpRoomDB->getRoom(kWaterRoomId)->environment = kWater;
        QVERIFY(addRoomAt(kForestRoomId, areaId, 6, 0, 0));
        map()->mCustomEnvColors[kForest] = QColor(90, 160, 70);
        map()->mpRoomDB->getRoom(kForestRoomId)->environment = kForest;
        QVERIFY(EnvironmentKit::store(map()->mUserData, kForest, EnvironmentKit::Kit::Forest));
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        pPlayerRoom->highlight = true;
        pPlayerRoom->highlightColor = QColor(Qt::red);
        pPlayerRoom->highlightColor2 = QColor(Qt::green);
        pPlayerRoom->highlightRadius = 1.0f;
        const auto restore = qScopeGuard([this, kWater, kForest]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->mUserData.remove(EnvironmentKit::userDataKey());
            map()->mCustomEnvColors.remove(kWater);
            map()->mCustomEnvColors.remove(kForest);
        });
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        const auto& commands = widget.mRenderCommandQueue.commands();
        constexpr float roomHeight = 2.0f / 3.0f / 2.0f;
        const float ground = -roomHeight / 2.0f;
        const auto shapesAt = [&commands](const RenderCommand::Shading shading, const GeometryManager::InstanceShape shape, const QPointF& place) {
            QList<CubeInstanceData> found;
            for (const auto& command : commands) {
                if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == shading && cubes->shape() == shape) {
                    found.append(roomInstances(*cubes, place));
                }
            }
            return found;
        };

        QList<CubeInstanceData> walls = shapesAt(RenderCommand::Shading::ModelBlock, GeometryManager::InstanceShape::Box, QPointF(2.0, 0.0));
        QCOMPARE(walls.size(), 4);
        std::sort(walls.begin(), walls.end(), [](const CubeInstanceData& a, const CubeInstanceData& b) {
            return a.transform.column(3).z() < b.transform.column(3).z();
        });
        float floor = ground;
        for (const CubeInstanceData& wall : std::as_const(walls)) {
            QCOMPARE(wall.color[3], 1.0f);
            const float half = wall.transform.column(2).z();
            QVERIFY2(qAbs(wall.transform.column(3).z() - half - floor) < 1e-4f, "a storey does not stand on the one below");
            floor = wall.transform.column(3).z() + half;
        }
        const QList<CubeInstanceData> roofs = shapesAt(RenderCommand::Shading::ModelBlock, GeometryManager::InstanceShape::Pyramid, QPointF(2.0, 0.0));
        QCOMPARE(roofs.size(), 1);
        QVERIFY2(qAbs(roofs.first().transform.column(3).z() - roofs.first().transform.column(2).z() - floor) < 1e-4f, "the roof is not on the top storey");
        for (const auto& command : commands) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelBlock) {
                for (const CubeInstanceData& cube : roomInstances(*cubes, QPointF(2.0, 0.0))) {
                    QVERIFY2(cube.color[3] == 1.0f, "a storey is drawn see-through, as a room on another level");
                }
            }
        }

        // Each storey picked through the side of it that faces the camera
        const TArea* pArea = map()->mpRoomDB->getArea(areaId);
        const QHash<int, DioramaLayout::Piece> pieces = widget.dioramaPieces(*pArea);
        const QVector3D eye = (widget.mCameraController.getViewMatrix() * widget.mCameraController.getModelMatrix()).inverted().map(QVector3D());
        for (const int roomId : {kEastRoomId, kFirstStoreyId, kFirstStoreyId + 1, kFirstStoreyId + 2}) {
            const DioramaLayout::Piece piece = pieces.value(roomId);
            QVERIFY(piece.storeys == 4);
            const QVector3D middle(2.0f, 0.0f, (piece.bottom + piece.peak) / 2.0f);
            QVector3D face;
            float facing = std::numeric_limits<float>::lowest();
            for (const QVector3D& normal : {QVector3D(1, 0, 0), QVector3D(-1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, -1, 0)}) {
                const QVector3D at = middle + normal * piece.halfWidth;
                if (const float toward = QVector3D::dotProduct(normal, (eye - at).normalized()); toward > facing) {
                    facing = toward;
                    face = at;
                }
            }
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(face);
            QVERIFY(pixel);
            QCOMPARE(widget.roomPickedAt(pixel->toPoint(), false), roomId);
        }

        // Water below the board, which has no top where it lies but has one between rooms
        const QList<CubeInstanceData> water = shapesAt(RenderCommand::Shading::ModelWater, GeometryManager::InstanceShape::Box, QPointF(4.0, 0.0));
        QCOMPARE(water.size(), 1);
        QVERIFY2(water.first().transform.column(3).z() + water.first().transform.column(2).z() < ground, "the water is not below the board");
        const RenderTrianglesCommand* board = nullptr;
        for (const auto& command : commands) {
            if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get()); triangles && triangles->shading() == RenderCommand::Shading::Unlit) {
                for (qsizetype i = 2; i < triangles->vertices().size(); i += 3) {
                    if (triangles->vertices().at(i) < ground - 0.3f) {
                        board = triangles;
                    }
                }
            }
        }
        QVERIFY2(board, "no board is laid under the rooms");
        const auto boardTopCovers = [board, ground](const QPointF& place) {
            const QList<float>& v = board->vertices();
            for (qsizetype t = 0; t + 8 < v.size(); t += 9) {
                if (qAbs(v.at(t + 2) - ground) > 1e-4f || qAbs(v.at(t + 5) - ground) > 1e-4f || qAbs(v.at(t + 8) - ground) > 1e-4f) {
                    continue;
                }
                const QPolygonF triangle({QPointF(v.at(t), v.at(t + 1)), QPointF(v.at(t + 3), v.at(t + 4)), QPointF(v.at(t + 6), v.at(t + 7))});
                if (triangle.containsPoint(place, Qt::OddEvenFill)) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(!boardTopCovers(QPointF(4.0, 0.1)), "the board covers the water");
        QVERIFY2(boardTopCovers(QPointF(3.0, 0.1)), "the board has a hole between rooms");
        QVERIFY2(boardTopCovers(QPointF(-0.9, -0.9)), "the board does not reach past the rooms");

        // The shared glow, in the script's color, on the board around the highlighted room, and on the board under a
        // building for a room storeys up
        const QColor blue(0, 0, 255);
        TRoom* pUpperStorey = map()->mpRoomDB->getRoom(kFirstStoreyId + 1);
        pUpperStorey->highlight = true;
        pUpperStorey->highlightColor = blue;
        pUpperStorey->highlightColor2 = QColor(Qt::green);
        pUpperStorey->highlightRadius = 1.0f;
        const auto unhighlight = qScopeGuard([pUpperStorey]() {
            pUpperStorey->highlight = false;
        });
        QCOMPARE(mpHost->mapStylePreset().highlightShape, MapStyle::HighlightShape::Glow);
        queueFrame(widget, areaId);
        const auto glowAt = [&widget](const QColor& color, const QPointF& place) -> std::optional<float> {
            const QString key = qsl("highlight|%1|%2|").arg(QString::number(static_cast<int>(MapStyle::HighlightShape::Glow)), color.name(QColor::HexArgb));
            for (const QString& used : std::as_const(widget.mRoomTexturesInUse)) {
                if (!used.startsWith(key)) {
                    continue;
                }
                const GLuint texture = widget.mLabelTextureCache.imageTexture(used);
                for (const auto& command : widget.mRenderCommandQueue.commands()) {
                    const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get());
                    if (!textured || textured->geometry().textureId != texture) {
                        continue;
                    }
                    const QVector<float>& vertices = textured->geometry().vertices;
                    for (qsizetype quad = 0; quad + 17 < vertices.size(); quad += 18) {
                        float x = 0.0f;
                        float y = 0.0f;
                        for (qsizetype vertex = quad; vertex < quad + 18; vertex += 3) {
                            x += vertices.at(vertex) / 6.0f;
                            y += vertices.at(vertex + 1) / 6.0f;
                        }
                        if (qAbs(x - place.x()) < 1e-3 && qAbs(y - place.y()) < 1e-3) {
                            return vertices.at(quad + 2);
                        }
                    }
                }
            }
            return std::nullopt;
        };
        const std::optional<float> playerGlow = glowAt(QColor(Qt::red), QPointF(0.0, 0.0));
        QVERIFY2(playerGlow, "the highlighted room has no glow in its script's color");
        QVERIFY2(qAbs(*playerGlow - ground) < 1e-4f, qPrintable(qsl("the glow is at height %1, the board at %2").arg(*playerGlow).arg(ground)));
        const std::optional<float> storeyGlow = glowAt(blue, QPointF(2.0, 0.0));
        QVERIFY2(storeyGlow, "a highlighted storey has no glow");
        QVERIFY2(qAbs(*storeyGlow - ground) < 1e-4f, qPrintable(qsl("a storey's glow is at height %1, not on the board at %2").arg(*storeyGlow).arg(ground)));

        // Named a road, the water's room is paved instead
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("setMapEnvironmentKit(%1, 'road')").arg(kWater)));
        queueFrame(widget, areaId);
        QVERIFY(shapesAt(RenderCommand::Shading::ModelWater, GeometryManager::InstanceShape::Box, QPointF(4.0, 0.0)).isEmpty());
        const QList<CubeInstanceData> paving = shapesAt(RenderCommand::Shading::ModelBlock, GeometryManager::InstanceShape::Box, QPointF(4.0, 0.0));
        QCOMPARE(paving.size(), 1);

        // A highlighted paved room glows on its paving, not on the board the paving would hide it under
        const QColor magenta(255, 0, 255);
        TRoom* pRoadRoom = map()->mpRoomDB->getRoom(kWaterRoomId);
        pRoadRoom->highlight = true;
        pRoadRoom->highlightColor = magenta;
        pRoadRoom->highlightColor2 = QColor(Qt::green);
        pRoadRoom->highlightRadius = 1.0f;
        const auto unhighlightRoad = qScopeGuard([pRoadRoom]() {
            pRoadRoom->highlight = false;
        });
        queueFrame(widget, areaId);
        const float pavingTop = paving.first().transform.column(3).z() + paving.first().transform.column(2).z();
        QVERIFY2(pavingTop > ground + 1e-3f, "the paving is no thicker than the board");
        const std::optional<float> roadGlow = glowAt(magenta, QPointF(4.0, 0.0));
        QVERIFY2(roadGlow, "a highlighted paved room has no glow");
        QVERIFY2(*roadGlow >= pavingTop - 1e-5f, qPrintable(qsl("a paved room's glow is at height %1, under its paving's top at %2").arg(*roadGlow).arg(pavingTop)));

        // A forest is a grass tile with from one to three trees standing on it, each a trunk under a crown
        const QList<CubeInstanceData> grass = shapesAt(RenderCommand::Shading::ModelBlock, GeometryManager::InstanceShape::Box, QPointF(6.0, 0.0));
        QCOMPARE(grass.size(), 1);
        const float grassTop = grass.first().transform.column(3).z() + grass.first().transform.column(2).z();
        QVERIFY2(grassTop < ground + DioramaLayout::flatTileTop(roomHeight) + 1e-4f, "the grass is a block, not a tile");
        QMap<GeometryManager::InstanceShape, int> treeParts;
        for (const auto& command : commands) {
            const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get());
            if (!cubes || cubes->shading() != RenderCommand::Shading::ModelBlock
                || (cubes->shape() != GeometryManager::InstanceShape::Cone && cubes->shape() != GeometryManager::InstanceShape::Cylinder)) {
                continue;
            }
            for (const CubeInstanceData& cube : cubes->instances()) {
                const QVector4D center = cube.transform.column(3);
                if (qAbs(center.x() - 6.0f) < 0.5f && qAbs(center.y()) < 0.5f) {
                    QVERIFY2(center.z() - cube.transform.column(2).z() >= grassTop - 1e-4f, "a tree is sunk in the grass");
                    ++treeParts[cubes->shape()];
                }
            }
        }
        const int trees = treeParts.value(GeometryManager::InstanceShape::Cone);
        QVERIFY2(trees >= 1 && trees <= 3, qPrintable(qsl("the forest has %1 trees").arg(trees)));
        QCOMPARE(treeParts.value(GeometryManager::InstanceShape::Cylinder), trees);

        // The shafts between the building's storeys are left out, but not one down out of its foot to a room below
        constexpr int kCellarId = 21;
        QVERIFY(addRoomAt(kCellarId, areaId, 2, 0, -1));
        map()->mpRoomDB->getRoom(kEastRoomId)->setExit(kCellarId, DIR_DOWN);
        map()->mpRoomDB->getRoom(kCellarId)->setExit(kEastRoomId, DIR_UP);
        map()->mpRoomDB->getRoom(kEastRoomId)->setExit(kFirstStoreyId, DIR_UP);
        map()->mpRoomDB->getRoom(kFirstStoreyId)->setExit(kEastRoomId, DIR_DOWN);
        queueFrame(widget, areaId);
        float shaftFoot = std::numeric_limits<float>::max();
        float shaftTop = std::numeric_limits<float>::lowest();
        for (const auto& command : commands) {
            if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && cubes->shading() == RenderCommand::Shading::ModelPath) {
                for (const CubeInstanceData& cube : cubes->instances()) {
                    const QVector4D center = cube.transform.column(3);
                    const float halfLength = qAbs(cube.transform.column(0).z());
                    if (qAbs(center.x() - 2.0f) < 1e-4f && qAbs(center.y()) < 1e-4f && halfLength > 0.01f) {
                        shaftFoot = qMin(shaftFoot, center.z() - halfLength);
                        shaftTop = qMax(shaftTop, center.z() + halfLength);
                    }
                }
            }
        }
        QVERIFY2(shaftFoot < -0.5f, "the shaft down to the room below the building is left out");
        QVERIFY2(shaftTop < roomHeight, qPrintable(qsl("a shaft between the building's storeys rises to %1").arg(shaftTop)));
    }

    // A symbol stands on a white tag, and a bold name in a white halo, where their color shows on it; else they go without
    void test_dioramaTagsSymbolsAndHalosBoldNamesInColorsThatShowOnThem()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        pPlayerRoom->mSymbol = qsl("$");
        pEastRoom->mSymbol = qsl("%");
        pEastRoom->mSymbolColor = QColor(250, 250, 210);
        pPlayerRoom->name = qsl("Market Cross");
        pEastRoom->name = qsl("Mill");
        pEastRoom->userData.insert(ROOM_UI_NAMECOLOR, qsl("#fafad2"));
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->setRoomNamesShown(false);
        });
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        const MapStyle& diorama = mpHost->mapStylePreset();
        QVERIFY(diorama.symbolPlate.isValid() && diorama.roomNameHalo.isValid());
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        // The key a texture of this kind was drawn under, for text which it ends with or which comes before its tag or halo
        const auto drawnKey = [&widget](const QString& kind, const QString& text) {
            for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                if (key.startsWith(kind) && (key.endsWith(qsl("|%1").arg(text)) || key.contains(qsl("|%1|").arg(text)))) {
                    return key;
                }
            }
            return QString();
        };
        const QString plate = qsl("|%1").arg(diorama.symbolPlate.name(QColor::HexArgb));
        QVERIFY2(drawnKey(qsl("symbol|"), qsl("$")).contains(plate), "a symbol in its usual color has no tag");
        QVERIFY2(!drawnKey(qsl("symbol|"), qsl("%")).isEmpty() && !drawnKey(qsl("symbol|"), qsl("%")).contains(plate), "a light symbol was put on a white tag");
        const QString name = drawnKey(qsl("name|"), qsl("Market Cross"));
        QVERIFY2(name.endsWith(qsl("|Market Cross|halo|%1").arg(diorama.roomNameHalo.name(QColor::HexArgb))), qPrintable(qsl("a name is drawn as %1, not in a white halo").arg(name)));
        QFont nameFont;
        QVERIFY(nameFont.fromString(name.section(QLatin1Char('|'), 2, 2)));
        QVERIFY2(nameFont.bold(), qPrintable(qsl("a name's font %1 is not bold").arg(nameFont.toString())));
        const QString lightName = drawnKey(qsl("name|"), qsl("Mill"));
        const QColor lightHalo(lightName.section(qsl("|halo|"), 1, 1));
        QVERIFY2(lightHalo.isValid() && MapStyle::readsOnPlate(QColor(250, 250, 210), lightHalo), qPrintable(qsl("a light name is drawn as %1, in no rim it shows on").arg(lightName)));
        QCOMPARE(lightHalo, mpHost->mapStylePreset().roomNameHaloFor(QColor(250, 250, 210)));

        // Without a halo of its own, a style rims names in the background where it gives labels or names halos
        QVERIFY(mpHost->setMapStyle(qsl("transit")));
        QVERIFY(mpHost->mapStylePreset().labelHalos && !mpHost->mapStylePreset().roomNameHalo.isValid());
        queueFrame(widget, areaId);
        const QString transitName = drawnKey(qsl("name|"), qsl("Market Cross"));
        QVERIFY2(transitName.endsWith(qsl("|halo|%1").arg(widget.backgroundColor().name(QColor::HexArgb))), qPrintable(qsl("a transit name is drawn as %1").arg(transitName)));
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        QVERIFY(!mpHost->mapStylePreset().labelHalos && mpHost->mapStylePreset().nameHalos);
        queueFrame(widget, areaId);
        const QString tilesName = drawnKey(qsl("name|"), qsl("Market Cross"));
        QVERIFY2(tilesName.endsWith(qsl("|halo|%1").arg(widget.backgroundColor().name(QColor::HexArgb))), qPrintable(qsl("a tiles name is drawn as %1").arg(tilesName)));
        // Soft, fading out from the letters, where a style asks for that
        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        queueFrame(widget, areaId);
        const QString inkwashName = drawnKey(qsl("name|"), qsl("Market Cross"));
        QVERIFY2(inkwashName.endsWith(qsl("|halo|%1|soft").arg(widget.backgroundColor().name(QColor::HexArgb))), qPrintable(qsl("an inkwash name is drawn as %1").arg(inkwashName)));
    }

    // Where a name of this room is drawn this frame, as the corners of its two triangles on screen and how opaque it
    // is; none for a name not drawn
    static std::optional<std::pair<QList<QPointF>, float>> drawnName(ModernGLWidget& widget, const QString& name)
    {
        for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
            if (!key.startsWith(qsl("name|")) || !(key.endsWith(qsl("|%1").arg(name)) || key.contains(qsl("|%1|").arg(name)))) {
                continue;
            }
            const GLuint texture = widget.mLabelTextureCache.imageTexture(key);
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get());
                if (!textured || textured->geometry().textureId != texture) {
                    continue;
                }
                const QVector<float>& vertices = textured->geometry().vertices;
                QList<QPointF> corners;
                for (qsizetype vertex = 0; vertex + 2 < vertices.size(); vertex += 3) {
                    const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(vertices.at(vertex), vertices.at(vertex + 1), vertices.at(vertex + 2)));
                    if (pixel) {
                        corners.append(*pixel);
                    }
                }
                return std::make_pair(corners, textured->geometry().colors.value(3));
            }
        }
        return std::nullopt;
    }

    // The index of the command drawing this room's name and the tint it is drawn with; -1 for a name not drawn
    static std::pair<qsizetype, QVector4D> nameCommand(ModernGLWidget& widget, const QString& name)
    {
        for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
            if (!key.startsWith(qsl("name|")) || !(key.endsWith(qsl("|%1").arg(name)) || key.contains(qsl("|%1|").arg(name)))) {
                continue;
            }
            const GLuint texture = widget.mLabelTextureCache.imageTexture(key);
            const auto& commands = widget.mRenderCommandQueue.commands();
            for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
                const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(commands.at(index).get());
                if (textured && textured->geometry().textureId == texture && textured->geometry().colors.size() >= 4) {
                    const QVector<float>& colors = textured->geometry().colors;
                    return {index, QVector4D(colors.at(0), colors.at(1), colors.at(2), colors.at(3))};
                }
            }
        }
        return {-1, QVector4D()};
    }

    // As a printed map's lettering: upright on screen however the map is tilted, and under the room it names, where a
    // style stands its names up; a style that lays them on the map draws them slanted by the tilt
    void test_namesStandUprightUnderTheirRoomWhereAStyleSetsThemUp()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("Market Cross");
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->setRoomNamesShown(false);
        });
        ModernGLWidget widget(map(), mpHost);
        // How far the quad's corners are from lying on an upright rectangle's edges, in pixels
        const auto slant = [&widget]() {
            const auto drawn = drawnName(widget, qsl("Market Cross"));
            if (!drawn || drawn->first.size() != 6) {
                return std::make_pair(std::numeric_limits<qreal>::max(), QRectF());
            }
            QRectF bounds;
            for (const QPointF& corner : drawn->first) {
                bounds |= QRectF(corner, QSizeF(1e-6, 1e-6));
            }
            qreal worst = 0.0;
            for (const QPointF& corner : drawn->first) {
                worst = qMax(worst, qMin(qAbs(corner.x() - bounds.left()), qAbs(corner.x() - bounds.right())));
                worst = qMax(worst, qMin(qAbs(corner.y() - bounds.top()), qAbs(corner.y() - bounds.bottom())));
            }
            return std::make_pair(worst, bounds);
        };
        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        QVERIFY(mpHost->mapStylePreset().uprightNames);
        queueFrame(widget, areaId);
        const auto [upright, bounds] = slant();
        QVERIFY2(upright < 0.5, qPrintable(qsl("a tiles name's corners are %1 pixels off an upright rectangle").arg(upright)));
        const std::optional<QPointF> room = widget.mCameraController.pixelForPoint(QVector3D(0.0f, 0.0f, 0.0f));
        QVERIFY(room);
        QVERIFY2(bounds.top() > room->y() && bounds.left() < room->x() && bounds.right() > room->x(),
                 qPrintable(qsl("a name drawn at %1,%2 to %3,%4 is not under its room at %5,%6")
                                    .arg(bounds.left())
                                    .arg(bounds.top())
                                    .arg(bounds.right())
                                    .arg(bounds.bottom())
                                    .arg(room->x())
                                    .arg(room->y())));

        // Under it on screen still, with the map turned round
        widget.setCameraPosition(1.0f, 40.0f, CameraController::scmNorthUpAzimuth + 180.0f);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        const auto [turned, turnedBounds] = slant();
        const std::optional<QPointF> turnedRoom = widget.mCameraController.pixelForPoint(QVector3D(0.0f, 0.0f, 0.0f));
        QVERIFY(turnedRoom);
        QVERIFY2(turned < 0.5 && turnedBounds.top() > turnedRoom->y(), qPrintable(qsl("turned round, a name is drawn from %1 down, its room at %2").arg(turnedBounds.top()).arg(turnedRoom->y())));

        QVERIFY(mpHost->setMapStyle(qsl("inkwash")));
        QVERIFY(!mpHost->mapStylePreset().uprightNames);
        queueFrame(widget, areaId);
        QVERIFY(drawnName(widget, qsl("Market Cross")));
        QVERIFY2(slant().first > 1.0, qPrintable(qsl("a name lying on a tilted map is drawn upright, %1 pixels off").arg(slant().first)));
    }

    // Drawn with as many texels as a name covers pixels, so that it stays sharp zoomed in or on a high resolution
    // screen, rather than a texture sized for a far view stretched over it
    void test_namesAreDrawnSharpAsCloseAsTheyAreSeen()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("Market Cross");
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            map()->setRoomNamesShown(false);
        });
        ModernGLWidget widget(map(), mpHost);
        // How wide the name is on the map, across the screen
        const auto nameWidth = [&widget]() {
            const qsizetype index = nameCommand(widget, qsl("Market Cross")).first;
            if (index < 0) {
                return 0.0f;
            }
            const QVector<float>& vertices = static_cast<const RenderTexturedTrianglesCommand*>(widget.mRenderCommandQueue.commands().at(index).get())->geometry().vertices;
            float least = std::numeric_limits<float>::max();
            float most = std::numeric_limits<float>::lowest();
            for (qsizetype vertex = 0; vertex + 2 < vertices.size(); vertex += 3) {
                least = qMin(least, vertices.at(vertex));
                most = qMax(most, vertices.at(vertex));
            }
            return most - least;
        };
        const auto nameFontSize = [&widget](const float distance) {
            widget.setCameraPosition(distance, 40.0f, CameraController::scmNorthUpAzimuth);
            widget.updateMatrices();
            widget.mRenderCommandQueue.clear();
            widget.queueScene();
            for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                if (key.startsWith(qsl("name|")) && key.contains(qsl("|Market Cross"))) {
                    QFont font;
                    return font.fromString(key.section(QLatin1Char('|'), 2, 2)) ? font.pointSizeF() : 0.0;
                }
            }
            return 0.0;
        };
        queueFrame(widget, areaId);
        const qreal farSize = nameFontSize(1.0f);
        const float farWidth = nameWidth();
        const qreal nearSize = nameFontSize(0.25f);
        const float nearWidth = nameWidth();
        QVERIFY2(farSize > 0.0 && nearSize >= 2.0 * farSize, qPrintable(qsl("a name is drawn at %1 points far off and %2 close up").arg(farSize).arg(nearSize)));
        QVERIFY2(nearSize <= 3.0 * farSize, "a name close up is drawn sharper than it can be seen");
        // Sharper, not larger: the same size on the map, but for rounding to whole texels
        QVERIFY2(farWidth > 0.0f && qAbs(nearWidth - farWidth) < 0.05f * farWidth, qPrintable(qsl("a name is %1 wide far off and %2 close up").arg(farWidth).arg(nearWidth)));

        // Each name as sharp as it is seen itself: one far off is not drawn as sharp as one close up
        constexpr int farRoomId = 4;
        QVERIFY(addRoomAt(farRoomId, areaId, 0, 6, 0));
        map()->mpRoomDB->getRoom(farRoomId)->name = qsl("Far Gate");
        widget.setCameraPosition(0.25f, 40.0f, CameraController::scmNorthUpAzimuth);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        QStringList drawnSizes;
        for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
            if (key.startsWith(qsl("name|"))) {
                QFont font;
                font.fromString(key.section(QLatin1Char('|'), 2, 2));
                drawnSizes.append(qsl("%1 %2").arg(key.section(QLatin1Char('|'), 3, 3)).arg(font.pointSizeF()));
            }
        }
        drawnSizes.sort();
        QCOMPARE(drawnSizes.size(), 2);
        QVERIFY2(drawnSizes.at(0).section(QLatin1Char(' '), -1).toDouble() < drawnSizes.at(1).section(QLatin1Char(' '), -1).toDouble(), qPrintable(drawnSizes.join(qsl(", "))));
    }

    // Where names overlap, the player's room's is kept over one nearer the camera, then a highlighted room's; without
    // either, the nearer the camera as before
    void test_theNamesThatMatterMostAreKeptWhereNamesOverlap()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int nearerRoomId = 4;
        QVERIFY(addRoomAt(nearerRoomId, areaId, 0, -3, 0));
        // Within reach of Alpha, not of Beta
        constexpr int besideRoomId = 5;
        QVERIFY(addRoomAt(besideRoomId, areaId, -2, 0, 0));
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        TRoom* pNearerRoom = map()->mpRoomDB->getRoom(nearerRoomId);
        pPlayerRoom->name = qsl("Alpha");
        pNearerRoom->name = qsl("Beta");
        // Moved up to just short of Alpha's, so they overlap, with Beta's the nearer the camera
        pNearerRoom->userData.insert(ROOM_UI_NAMEPOS, qsl("0 -2.9"));
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->setRoomNamesShown(false);
        });
        for (const QString& styleId : {qsl("tiles"), qsl("inkwash"), qsl("diorama")}) {
            QVERIFY(mpHost->setMapStyle(styleId));
            ModernGLWidget widget(map(), mpHost);
            widget.mNameFadeMilliseconds = 0;
            const auto kept = [&]() {
                queueFrame(widget, areaId);
                QStringList names;
                for (const QString& name : {qsl("Alpha"), qsl("Beta")}) {
                    if (drawnName(widget, name)) {
                        names.append(name);
                    }
                }
                return names.join(QLatin1Char(' '));
            };
            map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
            QCOMPARE(kept(), qsl("Alpha"));
            map()->mRoomIdHash[map()->mProfileName] = 0;
            QCOMPARE(kept(), qsl("Beta"));
            pPlayerRoom->highlight = true;
            QCOMPARE(kept(), qsl("Alpha"));
            pPlayerRoom->highlight = false;
            // Next to the player, though the player is in neither room
            map()->mRoomIdHash[map()->mProfileName] = besideRoomId;
            QCOMPARE(kept(), qsl("Alpha"));
            map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        }
    }

    // A name that loses its place to another fades out over a moment, as the other fades in, rather than blinking
    void test_aNameThatLosesItsPlaceFadesOut()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int nearerRoomId = 4;
        QVERIFY(addRoomAt(nearerRoomId, areaId, 0, -3, 0));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("Alpha");
        TRoom* pNearerRoom = map()->mpRoomDB->getRoom(nearerRoomId);
        pNearerRoom->name = qsl("Beta");
        pNearerRoom->userData.insert(ROOM_UI_NAMEPOS, qsl("0 -2.9"));
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            map()->setRoomNamesShown(false);
        });
        ModernGLWidget widget(map(), mpHost);
        widget.mNameFadeMilliseconds = 10000;
        queueFrame(widget, areaId);
        QVERIFY2(drawnName(widget, qsl("Alpha")) && !drawnName(widget, qsl("Beta")), "a view first drawn fades nothing in");
        map()->mRoomIdHash[map()->mProfileName] = 0;
        QTest::qWait(20);
        queueFrame(widget, areaId);
        const auto alpha = drawnName(widget, qsl("Alpha"));
        const auto beta = drawnName(widget, qsl("Beta"));
        QVERIFY2(alpha && alpha->second > 0.5f && alpha->second < 1.0f, qPrintable(qsl("the name losing its place is %1 opaque").arg(alpha ? alpha->second : -1.0f)));
        QVERIFY2(beta && beta->second > 0.0f && beta->second < 0.5f, qPrintable(qsl("the name winning a place is %1 opaque").arg(beta ? beta->second : -1.0f)));
        widget.mNameFadeMilliseconds = 1;
        QTest::qWait(10);
        queueFrame(widget, areaId);
        QVERIFY2(!drawnName(widget, qsl("Alpha")), "a name that lost its place is still drawn once faded out");
        const auto faded = drawnName(widget, qsl("Beta"));
        QVERIFY(faded);
        QCOMPARE(faded->second, 1.0f);
    }

    // Standing names are drawn over the viewed level's rooms and lines with the depth test off, turned back on before
    // anything after them, and under its labels shown on top, as on the 2D map; only its own names are drawn. Diorama's
    // names lie in its scene, so they keep the depth test
    void test_standingNamesGoOverTheirLevelButUnderItsLabelsShownOnTop()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("Market Cross");
        map()->mpRoomDB->getRoom(kLowerRoomId)->name = qsl("Cellar");
        TMapLabel label;
        label.text = qsl("Old Keep");
        label.pos = QVector3D(4.5f, 1.0f, 0.0f);
        label.size = QSizeF(1.5, 0.5);
        label.pix = QPixmap(60, 20);
        label.pix.fill(Qt::white);
        label.showOnTop = true;
        map()->mpRoomDB->getArea(areaId)->mMapLabels.insert(1, label);
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this, areaId]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->setRoomNamesShown(false);
            map()->mpRoomDB->getArea(areaId)->mMapLabels.clear();
        });
        // The depth test's state set nearest the command, before or after it, among the state changes next to it
        const auto depthTestBy = [](const ModernGLWidget& widget, qsizetype index, const int step) {
            const auto& commands = widget.mRenderCommandQueue.commands();
            for (index += step; index >= 0 && index < static_cast<qsizetype>(commands.size()); index += step) {
                const auto* state = dynamic_cast<const GLStateCommand*>(commands.at(index).get());
                if (!state) {
                    break;
                }
                if (state->stateType() == GLStateCommand::ENABLE_DEPTH_TEST || state->stateType() == GLStateCommand::DISABLE_DEPTH_TEST) {
                    return std::optional<GLStateCommand::StateType>(state->stateType());
                }
            }
            return std::optional<GLStateCommand::StateType>();
        };
        // The last textured quad lying wholly over the label's spot
        const auto labelCommand = [](const ModernGLWidget& widget) {
            const auto& commands = widget.mRenderCommandQueue.commands();
            qsizetype found = -1;
            for (qsizetype index = 0; index < static_cast<qsizetype>(commands.size()); ++index) {
                const auto* textured = dynamic_cast<const RenderTexturedTrianglesCommand*>(commands.at(index).get());
                if (!textured || textured->geometry().vertices.isEmpty()) {
                    continue;
                }
                bool there = true;
                const QVector<float>& vertices = textured->geometry().vertices;
                for (qsizetype vertex = 0; vertex + 2 < vertices.size(); vertex += 3) {
                    there = there && vertices.at(vertex) > 4.4f;
                }
                if (there) {
                    found = index;
                }
            }
            return found;
        };

        QVERIFY(mpHost->setMapStyle(qsl("tiles")));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId);
        const qsizetype name = nameCommand(widget, qsl("Market Cross")).first;
        QVERIFY(name >= 0);
        QCOMPARE(depthTestBy(widget, name, -1), std::optional(GLStateCommand::DISABLE_DEPTH_TEST));
        QCOMPARE(depthTestBy(widget, name, 1), std::optional(GLStateCommand::ENABLE_DEPTH_TEST));
        const qsizetype onTop = labelCommand(widget);
        QVERIFY2(onTop > name, qPrintable(qsl("the label shown on top is queued at %1, the name at %2").arg(onTop).arg(name)));
        QVERIFY2(!drawnName(widget, qsl("Cellar")), "a name on the level below is drawn");

        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget diorama(map(), mpHost);
        diorama.mNameFadeMilliseconds = 0;
        queueFrame(diorama, areaId);
        const qsizetype dioramaName = nameCommand(diorama, qsl("Market Cross")).first;
        QVERIFY(dioramaName >= 0);
        QVERIFY(depthTestBy(diorama, dioramaName, -1) != std::optional(GLStateCommand::DISABLE_DEPTH_TEST));
    }

    // What every style draws a name in is what the contrast audit measures: its color, its rim, whether that is soft,
    // and the tint over it
    void test_everyStyleDrawsNamesAsTheContrastAuditMeasuresThem()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("Market Cross");
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->setRoomNamesShown(false);
        });
        for (const MapStyle& style : MapStyle::all()) {
            QVERIFY(mpHost->setMapStyle(style.id));
            ModernGLWidget widget(map(), mpHost);
            queueFrame(widget, areaId);
            const QColor background = widget.backgroundColor();
            const QColor color = style.roomNameColorFor(QString(), background);
            QColor halo = style.roomNameHaloFor(color);
            if (!halo.isValid() && (style.labelHalos || style.nameHalos)) {
                halo = background;
            }
            QString expected = qsl("name|%1|").arg(color.name(QColor::HexArgb));
            QString ending = qsl("|Market Cross");
            if (halo.isValid()) {
                ending.append(qsl("|halo|%1").arg(halo.name(QColor::HexArgb)));
                if (style.softNameHalos) {
                    ending.append(qsl("|soft"));
                }
            }
            QString drawn;
            for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                if (key.startsWith(qsl("name|"))) {
                    drawn = key;
                }
            }
            QVERIFY2(drawn.startsWith(expected) && drawn.endsWith(ending), qPrintable(qsl("%1 draws %2, measured as %3...%4").arg(style.id, drawn, expected, ending)));
            const QVector4D tint = nameCommand(widget, qsl("Market Cross")).second;
            const QColor styleTint = style.labelTint.isValid() ? style.labelTint : QColor(Qt::white);
            QVERIFY2(qAbs(tint.x() - styleTint.redF()) < 0.01f && qAbs(tint.y() - styleTint.greenF()) < 0.01f && qAbs(tint.z() - styleTint.blueF()) < 0.01f
                             && qAbs(tint.w() - styleTint.alphaF()) < 0.01f,
                     qPrintable(qsl("%1 tints its names %2 %3 %4 %5").arg(style.id).arg(tint.x()).arg(tint.y()).arg(tint.z()).arg(tint.w())));
        }
    }

    // A soft halo fades out from the letters: thickest by them, thinning toward its edge
    void test_aSoftHaloFadesOutFromTheLetters()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("I");
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->setRoomNamesShown(false);
        });
        QVERIFY(mpHost->setMapStyle(qsl("hologram")));
        QVERIFY(mpHost->mapStylePreset().softNameHalos);
        ModernGLWidget widget(map(), mpHost);
        QHash<GLuint, QImage> images;
        widget.mLabelTextureCache.useTextureStandIn(
                [&images, lastTexture = GLuint{0}](const QImage& image) mutable {
                    images.insert(++lastTexture, image);
                    return lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.setCameraPosition(1.0f, 40.0f, CameraController::scmNorthUpAzimuth);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        QImage image;
        for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
            if (key.startsWith(qsl("name|"))) {
                image = images.value(widget.mLabelTextureCache.imageTexture(key));
            }
        }
        QVERIFY(!image.isNull());
        // Along the middle row, from the left edge in to where the letter is
        const int y = image.height() / 2;
        int letter = 0;
        while (letter < image.width() && qAlpha(image.pixel(letter, y)) < 250) {
            ++letter;
        }
        int outer = 0;
        while (outer < letter && qAlpha(image.pixel(outer, y)) == 0) {
            ++outer;
        }
        QVERIFY2(letter - outer >= 3, qPrintable(qsl("the halo is %1 texels wide").arg(letter - outer)));
        int previous = 0;
        for (int x = outer; x < letter; ++x) {
            const int alpha = qAlpha(image.pixel(x, y));
            QVERIFY2(alpha + 2 >= previous, qPrintable(qsl("the halo grows fainter toward the letter at %1: %2 after %3").arg(x).arg(alpha).arg(previous)));
            previous = alpha;
        }
        QVERIFY2(2 * qAlpha(image.pixel(outer, y)) < qAlpha(image.pixel(letter - 1, y)),
                 qPrintable(qsl("the halo's edge is %1 opaque and by the letter %2").arg(qAlpha(image.pixel(outer, y))).arg(qAlpha(image.pixel(letter - 1, y)))));
    }

    // While a name is still fading the view asks to be drawn again, so the fade runs without anything else moving
    void test_aFadingNameKeepsTheViewDrawing()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        constexpr int nearerRoomId = 4;
        QVERIFY(addRoomAt(nearerRoomId, areaId, 0, -3, 0));
        map()->mpRoomDB->getRoom(kPlayerRoomId)->name = qsl("Alpha");
        TRoom* pNearerRoom = map()->mpRoomDB->getRoom(nearerRoomId);
        pNearerRoom->name = qsl("Beta");
        pNearerRoom->userData.insert(ROOM_UI_NAMEPOS, qsl("0 -2.9"));
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            map()->setRoomNamesShown(false);
        });
        ModernGLWidget widget(map(), mpHost);
        widget.mNameFadeMilliseconds = 10000;
        queueFrame(widget, areaId);
        QVERIFY2(!widget.mNameFadeTimer->isActive(), "a settled view keeps drawing");
        map()->mRoomIdHash[map()->mProfileName] = 0;
        QTest::qWait(20);
        queueFrame(widget, areaId);
        QVERIFY2(widget.mNameFadeTimer->isActive(), "a fading name does not ask for the view to be drawn again");
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
    }

    // Names are kept apart by what they draw: two haloed names close enough for their halos to touch lose one, where
    // the same two, one too faint for a halo, both stay
    void test_dioramaDecluttersNamesByTheirOwnHalos()
    {
        const int areaId = addArea();
        QVERIFY(areaId > 0);
        TRoom* pPlayerRoom = map()->mpRoomDB->getRoom(kPlayerRoomId);
        TRoom* pEastRoom = map()->mpRoomDB->getRoom(kEastRoomId);
        pPlayerRoom->name = qsl("Alpha");
        pEastRoom->name = qsl("Beta");
        map()->setRoomNamesShown(true);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
            map()->setRoomNamesShown(false);
        });
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget widget(map(), mpHost);
        widget.mNameFadeMilliseconds = 0;
        const auto namesKept = [&widget]() {
            int kept = 0;
            for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
                kept += key.startsWith(qsl("name|")) && (key.contains(qsl("|Alpha")) || key.contains(qsl("|Beta")));
            }
            return kept;
        };
        // Beta's name moved from under its room to under Alpha's, then down by step
        const auto keptAt = [&](const double down, const QString& betaColor) {
            pEastRoom->userData.insert(ROOM_UI_NAMEPOS, qsl("-2 %1").arg(down));
            pEastRoom->userData.insert(ROOM_UI_NAMECOLOR, betaColor);
            queueFrame(widget, areaId);
            return namesKept();
        };
        const QString faint = qsl("#28000000");
        QVERIFY(!mpHost->mapStylePreset().roomNameHaloFor(QColor::fromString(faint)).isValid());
        QCOMPARE(keptAt(0.0, QString()), 1);
        QCOMPARE(keptAt(0.0, faint), 1);
        QCOMPARE(keptAt(2.0, QString()), 2);
        bool haloesDecide = false;
        for (double down = 0.0; down < 1.0 && !haloesDecide; down += 0.004) {
            haloesDecide = keptAt(down, QString()) == 1 && keptAt(down, faint) == 2;
        }
        QVERIFY2(haloesDecide, "a name without a halo was kept apart as if it had one");
    }

    // Looking down: the board is wood under a pale sky, a room's top is lighter on the edge toward the light than on the
    // far one, the ground just south-east of a room is in its shadow, the pawn shows red, and nothing changes between frames
    void test_dioramaShadesBlocksUnderSoftLight()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        const auto restore = qScopeGuard([this]() {
            mpHost->setMapStyle(qsl("tiles"));
        });
        // No borders: nothing around a room but its shadow
        const bool savedBorders = mpHost->mMapperShowRoomBorders;
        const auto restoreBorders = qScopeGuard([this, savedBorders]() {
            mpHost->mMapperShowRoomBorders = savedBorders;
        });
        mpHost->mMapperShowRoomBorders = false;
        // Plain blocks rather than the buildings the rooms' red would make, as their tops are what is looked at
        QVERIFY(EnvironmentKit::store(map()->mUserData, -1, EnvironmentKit::Kit::Plain));
        QVERIFY(mpHost->setMapStyle(qsl("diorama")));
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const QImage frame = widget.grabFramebuffer();
        QCOMPARE(widget.grabFramebuffer(), frame);
        constexpr float roomHeight = 2.0f / 3.0f / 2.0f;
        const auto pixelAt = [&widget](const float x, const float y, const float z) {
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(x, y, z));
            return pixel ? pixel->toPoint() : QPoint(-1, -1);
        };
        const auto topAt = [&](const float x, const float y) {
            return frame.pixelColor(pixelAt(x, y, roomHeight / 2.0f));
        };
        const auto groundAt = [&](const float x, const float y) {
            return frame.pixelColor(pixelAt(x, y, -roomHeight / 2.0f));
        };

        const QColor board = groundAt(1.0f, -0.9f);
        QVERIFY2(board.red() > board.blue() + 20 && board.lightness() > 120, qPrintable(qsl("the board is %1, not light wood").arg(board.name())));
        const QColor sky = frame.pixelColor(2, 2);
        QVERIFY2(sky.blue() > sky.red() + 10 && sky.lightness() > 180, qPrintable(qsl("the sky is %1, not pale blue").arg(sky.name())));

        // A room reaches a third of a unit from its middle; the light comes from the north-west
        const int middle = topAt(2.0f, 0.0f).lightness();
        const int lit = topAt(2.0f - 0.31f, 0.31f).lightness();
        const int shaded = topAt(2.0f + 0.31f, -0.31f).lightness();
        QVERIFY2(lit > shaded + 20, qPrintable(qsl("a room's top is %1 light toward the light and %2 away from it").arg(lit).arg(shaded)));
        QVERIFY2(middle > shaded, "a room's top is darker in its middle than its shaded edge");

        const int shadow = groundAt(2.4f, -0.4f).lightness();
        const int open = groundAt(1.6f, 0.4f).lightness();
        QVERIFY2(open > shadow + 10, qPrintable(qsl("the ground is %1 light in a room's shadow and %2 out of it").arg(shadow).arg(open)));

        const QColor token = frame.pixelColor(pixelAt(0.0f, 0.0f, roomHeight));
        QVERIFY2(token.red() > token.green() + 60 && token.red() > token.blue() + 60, "no red token stands on the player's room");
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

    // A grid of side by side rooms on three levels, joined by exits, with names, environments, some symbols, a few
    // doors and text labels, the player in the middle of the viewed level
    int addBenchArea(const int side) const
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Bench Area"));
        int id = 0;
        for (int z = -1; z <= 1; ++z) {
            for (int y = 0; y < side; ++y) {
                for (int x = 0; x < side; ++x) {
                    ++id;
                    if (!addRoomAt(id, areaId, x * 2, y * 2, z)) {
                        return 0;
                    }
                    TRoom* room = map()->mpRoomDB->getRoom(id);
                    room->name = qsl("Room %1").arg(id);
                    room->environment = (x + y) % 8 + 1;
                    if ((x * 7 + y) % 5 == 0) {
                        room->mSymbol = QString(QChar(u'A' + static_cast<char16_t>((x + y) % 26)));
                    }
                    if (x > 0) {
                        map()->setExit(id, id - 1, DIR_WEST);
                        map()->setExit(id - 1, id, DIR_EAST);
                        if ((x + y) % 11 == 0) {
                            room->setDoor(qsl("west"), (x + y) % 3 + 1);
                        }
                    }
                    if (y > 0) {
                        map()->setExit(id, id - side, DIR_NORTH);
                        map()->setExit(id - side, id, DIR_SOUTH);
                    }
                    if (z > -1 && (x + y) % 6 == 0) {
                        map()->setExit(id, id - side * side, DIR_DOWN);
                        map()->setExit(id - side * side, id, DIR_UP);
                    }
                }
            }
        }
        for (int label = 0; label < qMax(2, side / 2); ++label) {
            const float x = static_cast<float>((label * 7) % side) * 2.0f;
            const float y = static_cast<float>((label * 3) % side) * 2.0f;
            map()->createMapLabel(areaId, qsl("Label %1").arg(label), x, y, 0.0f, QColor(Qt::white), QColor(0, 0, 0, 0), label % 2 == 0, false);
        }
        map()->setRoomNamesShown(true);
        map()->mRoomIdHash[map()->mProfileName] = side * side + (side / 2) * side + side / 2;
        map()->mNewMove = false;
        return areaId;
    }

    static double percentile(const QList<double>& sorted, const double fraction)
    {
        const qsizetype index = qBound<qsizetype>(0, static_cast<qsizetype>(std::ceil(fraction * static_cast<double>(sorted.size()))) - 1, sorted.size() - 1);
        return sorted.at(index);
    }

    static void emitMetric(const QString& name, const double value) { std::printf("METRIC %s %.3f\n", qPrintable(name), value); }

    // Frame times per map style, as METRIC lines in milliseconds: on a small map and a dense one, at a slant and
    // looking down, while orbiting, panning and zooming, and the first frame after switching to the style. Whole
    // frames where there is GL, else only the commands a frame queues, with stand-in textures. Report-only and long,
    // so it runs only when asked:
    //   MUDLET_MAPPER_BENCH=1 QT_QPA_PLATFORM=xcb ./functional_map_tests ModernMapperRenderTest bench_frameTimesPerStyle
    // MUDLET_MAPPER_BENCH_STYLES, _MAPS, _VIEWS and _MOTIONS take comma-separated subsets, _FRAMES the frames per run,
    // and with GL, _DUMP a directory to write each run's last frame into, for comparing builds pixel by pixel;
    // _TRACE prints every frame's time and the textures it made, to see what a slow frame did
    void bench_frameTimesPerStyle()
    {
        if (!qEnvironmentVariableIsSet("MUDLET_MAPPER_BENCH")) {
            QSKIP("set MUDLET_MAPPER_BENCH=1 to run the frame-time benchmark");
        }
        const auto chosen = [](const char* variable, const QStringList& all) {
            const QString value = qEnvironmentVariable(variable);
            if (value.isEmpty()) {
                return all;
            }
            QStringList picked;
            for (const QString& id : value.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
                if (all.contains(id)) {
                    picked.append(id);
                }
            }
            return picked;
        };
        const QStringList styles = chosen("MUDLET_MAPPER_BENCH_STYLES", MapStyle::ids());
        const QStringList maps = chosen("MUDLET_MAPPER_BENCH_MAPS", {qsl("small"), qsl("dense")});
        const QStringList views = chosen("MUDLET_MAPPER_BENCH_VIEWS", {qsl("default"), qsl("top")});
        const QStringList motions = chosen("MUDLET_MAPPER_BENCH_MOTIONS", {qsl("orbit"), qsl("pan"), qsl("zoom"), qsl("walk")});
        const int frames = qMax(5, qEnvironmentVariableIntValue("MUDLET_MAPPER_BENCH_FRAMES"));
        constexpr int warmUp = 3;
        const bool gl = glAvailable();
        const QString dumpDirectory = qEnvironmentVariable("MUDLET_MAPPER_BENCH_DUMP");
        const bool trace = qEnvironmentVariableIsSet("MUDLET_MAPPER_BENCH_TRACE");
        if (gl && !dumpDirectory.isEmpty()) {
            QVERIFY(QDir().mkpath(dumpDirectory));
        }
        std::printf("METRIC bench_3d_whole_frames %d\n", gl ? 1 : 0);
        const auto restoreStyle = qScopeGuard([this] {
            mpHost->setMapStyle(qsl("tiles"));
        });

        for (const QString& mapName : maps) {
            const int side = mapName == qsl("dense") ? 40 : 8;
            const int areaId = addBenchArea(side);
            QVERIFY(areaId > 0);
            const int playerRoomId = map()->mRoomIdHash.value(map()->mProfileName);
            const TRoom* playerRoom = map()->mpRoomDB->getRoom(playerRoomId);
            QVERIFY(playerRoom);
            QVERIFY(mpHost->setMapStyle(qsl("tiles")));
            ModernGLWidget widget(map(), mpHost);
            widget.setSecondaryView(true);
            widget.resize(800, 600);
            if (gl) {
                widget.grabFramebuffer();
                if (!widget.isValid()) {
                    QSKIP("no OpenGL context on this platform");
                }
                QVERIFY2(widget.failureMessage().isEmpty(), qPrintable(widget.failureMessage()));
            } else {
                widget.mLabelTextureCache.useTextureStandIn(
                        [lastTexture = GLuint{0}](const QImage&) mutable {
                            return ++lastTexture;
                        },
                        [](GLuint) {});
            }
            widget.mCameraController.setViewportSize(widget.width(), widget.height());
            widget.setViewCenter(areaId, playerRoom->x(), playerRoom->y(), playerRoom->z());
            const float aspect = static_cast<float>(widget.width()) / static_cast<float>(widget.height());
            const float distance = CameraController::distanceToShow(mapName == qsl("dense") ? 24.0f : 10.0f, aspect);
            QImage lastFrame;
            const auto frame = [&widget, &lastFrame, gl] {
                QElapsedTimer timer;
                timer.start();
                if (gl) {
                    lastFrame = widget.grabFramebuffer();
                } else {
                    widget.updateMatrices();
                    widget.mRenderCommandQueue.clear();
                    widget.queueScene();
                }
                return static_cast<double>(timer.nsecsElapsed()) / 1.0e6;
            };

            for (const QString& styleId : styles) {
                widget.jumpTo(areaId, playerRoom->x(), playerRoom->y(), playerRoom->z());
                widget.setCameraPosition(distance, CameraController::scmDefaultTilt, CameraController::scmNorthUpAzimuth);
                frame();
                QVERIFY(mpHost->setMapStyle(styleId));
                emitMetric(qsl("bench_3d_%1_%2_switch_ms").arg(styleId, mapName), frame());

                for (const QString& view : views) {
                    const float tilt = view == qsl("top") ? CameraController::scmMinTilt : CameraController::scmDefaultTilt;
                    for (const QString& motion : motions) {
                        QList<double> times;
                        qint64 texturesMade = 0;
                        for (int step = -warmUp; step < frames; ++step) {
                            const int along = step + warmUp;
                            float azimuth = CameraController::scmNorthUpAzimuth;
                            float r = distance;
                            int centerX = playerRoom->x();
                            if (motion == qsl("orbit")) {
                                azimuth += 4.0f * static_cast<float>(along);
                            } else if (motion == qsl("pan")) {
                                // Back and forth across the middle half of the map, so the view never leaves it
                                const int reach = side / 2;
                                const int phase = along % (4 * reach);
                                centerX += phase < 2 * reach ? phase - reach : 3 * reach - phase;
                            } else if (motion == qsl("walk")) {
                                // The player steps a room east or west each frame, as a mapping script follows them
                                const int reach = side / 4;
                                const int phase = along % (4 * reach);
                                const int step = phase < 2 * reach ? phase - reach : 3 * reach - phase;
                                const int roomId = playerRoomId + step;
                                QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("centerview(%1)").arg(roomId)));
                                centerX = map()->mpRoomDB->getRoom(roomId)->x();
                            } else {
                                r = distance * (1.0f + 0.5f * std::sin(0.15f * static_cast<float>(along)));
                            }
                            widget.jumpTo(areaId, centerX, playerRoom->y(), playerRoom->z());
                            widget.setCameraPosition(r, tilt, azimuth);
                            const qint64 madeBefore = widget.mLabelTextureCache.imageTexturesMade();
                            const double ms = frame();
                            if (step >= 0) {
                                times.append(ms);
                                texturesMade += widget.mLabelTextureCache.imageTexturesMade() - madeBefore;
                            }
                            if (trace) {
                                std::printf("TRACE %s %s %s %s frame %d %.3f ms, %lld textures made, %lld in use, %zu commands\n",
                                            qPrintable(styleId),
                                            qPrintable(mapName),
                                            qPrintable(view),
                                            qPrintable(motion),
                                            step,
                                            ms,
                                            static_cast<long long>(widget.mLabelTextureCache.imageTexturesMade() - madeBefore),
                                            static_cast<long long>(widget.mRoomTexturesInUse.size()),
                                            widget.mRenderCommandQueue.getCommandCount());
                            }
                        }
                        if (motion == qsl("walk")) {
                            QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("centerview(%1)").arg(playerRoomId)));
                        }
                        std::sort(times.begin(), times.end());
                        const QString name = qsl("bench_3d_%1_%2_%3_%4").arg(styleId, mapName, view, motion);
                        if (gl && !dumpDirectory.isEmpty()) {
                            QVERIFY(lastFrame.save(qsl("%1/%2.png").arg(dumpDirectory, name)));
                        }
                        emitMetric(name + qsl("_p50_ms"), percentile(times, 0.5));
                        emitMetric(name + qsl("_p95_ms"), percentile(times, 0.95));
                        emitMetric(name + qsl("_p99_ms"), percentile(times, 0.99));
                        emitMetric(name + qsl("_max_ms"), times.last());
                        emitMetric(name + qsl("_textures_made"), static_cast<double>(texturesMade));
                    }
                }
            }
        }
    }
};

#include "ModernMapperRenderTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperRenderTest)
