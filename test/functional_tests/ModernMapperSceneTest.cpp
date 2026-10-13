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
 * The modern 3D view keeps the rooms and exits of the area it shows on the GPU
 * between frames, rebuilding only what a change to the map or the view touched,
 * leaves out what is off screen, and draws rooms too small to see in detail
 * without what is on them.
 *
 * Why not a spec: no Lua call reports what the 3D view kept or drew. The
 * "WithoutGL" cases queue frames with stand-in textures; the others read frames
 * back from GL, which is skipped where there is none, and run on a display:
 *   QT_QPA_PLATFORM=xcb MUDLET_TEST_MODE=1 ./functional_map_tests ModernMapperSceneTest
 *
 * Two report-only cases skip unless asked for, on a display:
 *   MUDLET_BENCH_3D=1 prints METRIC lines for a generated map of about 100,000
 *     rooms in five areas over several levels, at several zooms;
 *   MUDLET_BENCH_3D_FRAMES=<dir> saves frames of an ordinary map and of the big
 *     one as PNGs, so two builds' frames can be compared pixel for pixel.
 */

#include <ctime>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLContext>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "RenderCommand.h"
#include "RetainedMapScene.h"
#include "RoomAppearance.h"
#include "TArea.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ModernMapperSceneTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ModernMapperScene-Test");

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

    static bool glAvailable()
    {
        QOpenGLContext probe;
        return probe.create();
    }

    static double processCpuMs()
    {
        timespec now{};
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
        return static_cast<double>(now.tv_sec) * 1000.0 + static_cast<double>(now.tv_nsec) / 1.0e6;
    }

    // VmRSS or VmHWM from /proc, in MiB
    static double memoryMiB(const QByteArray& field)
    {
        QFile status(qsl("/proc/self/status"));
        if (!status.open(QIODevice::ReadOnly)) {
            return 0.0;
        }
        for (const QByteArray& line : status.readAll().split('\n')) {
            if (line.startsWith(field + ':')) {
                return line.mid(field.size() + 1).trimmed().split(' ').value(0).toDouble() / 1024.0;
            }
        }
        return 0.0;
    }

    // A grid of width by height rooms on each of levels levels, numbered on from firstId, each joined to its
    // east and north neighbors, with a way up every tenth room each way; returns the next free room ID
    int addGridArea(const QString& name, const int firstId, const int width, const int height, const int levels, const int lowestLevel) const
    {
        const int areaId = map()->mpRoomDB->addArea(name);
        const auto idAt = [&](const int x, const int y, const int level) {
            return firstId + (level * height + y) * width + x;
        };
        for (int level = 0; level < levels; ++level) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const int id = idAt(x, y, level);
                    map()->addRoom(id);
                    map()->setRoomArea(id, areaId);
                    map()->setRoomCoordinates(id, x - width / 2, y - height / 2, lowestLevel + level);
                    TRoom* room = map()->mpRoomDB->getRoom(id);
                    // The 16 colors mapClear() restores as custom environments, as env 1-16 unmapped all draw red
                    room->environment = 257 + (x / 5 + y / 7 + level) % 16;
                    if ((x + 2 * y) % 17 == 0) {
                        room->mSymbol = QString(QChar(u'A' + (x + y) % 26));
                    }
                }
            }
        }
        for (int level = 0; level < levels; ++level) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const int id = idAt(x, y, level);
                    if (x + 1 < width) {
                        map()->setExit(id, idAt(x + 1, y, level), DIR_EAST);
                        map()->setExit(idAt(x + 1, y, level), id, DIR_WEST);
                    }
                    if (y + 1 < height) {
                        map()->setExit(id, idAt(x, y + 1, level), DIR_NORTH);
                        map()->setExit(idAt(x, y + 1, level), id, DIR_SOUTH);
                    }
                    if (level + 1 < levels && x % 10 == 0 && y % 10 == 0) {
                        map()->setExit(id, idAt(x, y, level + 1), DIR_UP);
                        map()->setExit(idAt(x, y, level + 1), id, DIR_DOWN);
                    }
                }
            }
        }
        return idAt(0, 0, levels);
    }

    // About 100,000 rooms: one area of three levels of 141 by 141 and four of two levels of 71 by 71
    int addHugeMap() const
    {
        map()->mapClear();
        const int nextId = addGridArea(qsl("Bench Metropolis"), 1, 141, 141, 3, -1);
        int next = nextId;
        for (int area = 0; area < 4; ++area) {
            next = addGridArea(qsl("Bench Suburb %1").arg(area + 1), next, 71, 71, 2, 0);
        }
        // From the middle of the metropolis to the first suburb
        map()->setExit(1 + (141 + 70) * 141 + 70, nextId, DIR_NORTHEAST);
        map()->mRoomIdHash[map()->mProfileName] = 1 + (141 + 70) * 141 + 70;
        return map()->mpRoomDB->getRoom(1)->getArea();
    }

    // A few dozen rooms with something of everything the view draws: levels, symbols, highlights, borders,
    // doors, ways up and down, custom lines, labels and an exit to another area
    int addOrdinaryMap() const
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Ordinary"));
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Elsewhere"));
        int id = 1;
        for (int level = -1; level <= 1; ++level) {
            for (int y = -3; y <= 3; ++y) {
                for (int x = -4; x <= 4; ++x) {
                    if ((x * x + y * y) % 5 == 3 || (level != 0 && (x + y) % 3 == 0)) {
                        continue;
                    }
                    map()->addRoom(id);
                    map()->setRoomArea(id, areaId);
                    map()->setRoomCoordinates(id, x * 2, y * 2, level);
                    TRoom* room = map()->mpRoomDB->getRoom(id);
                    room->environment = 257 + (id * 7) % 16;
                    room->name = qsl("Room %1").arg(id);
                    if (id % 4 == 0) {
                        room->mSymbol = QString(QChar(u'a' + id % 26));
                    }
                    if (id % 9 == 0) {
                        room->highlight = true;
                        room->highlightColor = QColor(255, 200, 0);
                        room->highlightColor2 = QColor(255, 0, 100);
                        room->highlightRadius = 1.5f;
                    }
                    if (id % 11 == 0) {
                        room->mBorderColor = QColor(0, 255, 255);
                        room->mBorderThickness = 3;
                    }
                    ++id;
                }
            }
        }
        const QList<int> rooms = map()->mpRoomDB->getArea(areaId)->getAreaRooms().values();
        for (const int roomId : rooms) {
            TRoom* room = map()->mpRoomDB->getRoom(roomId);
            const int x = room->x();
            const int y = room->y();
            const int z = room->z();
            const auto at = [&](const int dx, const int dy, const int dz) {
                const QList<int> found = map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(x + dx, y + dy, z + dz);
                return found.isEmpty() ? 0 : found.first();
            };
            if (const int east = at(2, 0, 0)) {
                map()->setExit(roomId, east, DIR_EAST);
                if (roomId % 5 == 0) {
                    room->setDoor(qsl("e"), 1 + roomId % 3);
                }
            }
            if (const int north = at(0, 2, 0)) {
                map()->setExit(roomId, north, DIR_NORTH);
                map()->setExit(north, roomId, DIR_SOUTH);
            }
            if (const int northeast = at(2, 2, 0); northeast && roomId % 3 == 0) {
                map()->setExit(roomId, northeast, DIR_NORTHEAST);
            }
            if (const int up = at(0, 0, 1)) {
                map()->setExit(roomId, up, DIR_UP);
                map()->setExit(up, roomId, DIR_DOWN);
            }
        }
        const int first = rooms.first();
        TRoom* firstRoom = map()->mpRoomDB->getRoom(first);
        firstRoom->customLines[qsl("e")] = {QPointF(firstRoom->x() + 1.0, firstRoom->y() + 1.5), QPointF(firstRoom->x() + 3.0, firstRoom->y() + 1.5)};
        firstRoom->customLinesColor[qsl("e")] = QColor(255, 128, 0);
        firstRoom->customLinesStyle[qsl("e")] = Qt::DashLine;
        firstRoom->customLinesArrow[qsl("e")] = true;
        firstRoom->calcRoomDimensions();

        map()->addRoom(id);
        map()->setRoomArea(id, otherAreaId);
        map()->setRoomCoordinates(id, 0, 0, 0);
        map()->setExit(map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(0, 0, 0).value(0, first), id, DIR_WEST);

        TArea* area = map()->mpRoomDB->getArea(areaId);
        for (const bool onTop : {false, true}) {
            TMapLabel label;
            label.pos = QVector3D(onTop ? 1.0f : -6.0f, onTop ? 5.0f : -4.0f, 0.0f);
            label.size = QSizeF(4.0, 1.0);
            label.pix = QPixmap(64, 16);
            label.pix.fill(onTop ? QColor(40, 90, 200) : QColor(200, 60, 60));
            label.showOnTop = onTop;
            area->mMapLabels.insert(area->createLabelId(), label);
        }
        map()->mRoomIdHash[map()->mProfileName] = map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(0, 0, 0).value(0, first);
        return areaId;
    }

    // Each frame turned a little from the last, as dragging the view around would
    static void spin(ModernGLWidget& widget) { widget.turnCamera(2.0f); }

    // The commands one frame queues, looking at the middle of the area's level z from distance, with textures that need no GL context
    static void queueFrame(ModernGLWidget& widget, const int areaId, const float distance, const bool topView = true, const int z = 0)
    {
        widget.mLabelTextureCache.useTextureStandIn(
                [lastTexture = GLuint{0}](const QImage&) mutable {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.mShiftMode = true;
        widget.jumpTo(areaId, 0, 0, z);
        if (topView) {
            widget.mCameraController.setTopView();
        } else {
            widget.mCameraController.setDefaultView();
        }
        widget.setCameraDistance(distance);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
    }

    struct DrawnCube
    {
        QVector3D center;
        QVector4D color;
    };

    // The blocks queued this tall: a room's own block is 1/32, a level outline or border 0.6/32 and the hovered room's outline 0.75/32
    static QList<DrawnCube> drawnCubes(const ModernGLWidget& widget, const float height)
    {
        QList<DrawnCube> found;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get());
            if (!cubes) {
                continue;
            }
            for (const CubeInstanceData& cube : cubes->instances()) {
                if (qAbs(cube.transform.column(2).z() - height) < 1e-5f) {
                    found.append({cube.transform.column(3).toVector3D(), QVector4D(cube.color[0], cube.color[1], cube.color[2], cube.color[3])});
                }
            }
        }
        return found;
    }

    static QList<DrawnCube> drawnRooms(const ModernGLWidget& widget) { return drawnCubes(widget, 1.0f / 32.0f); }

    // The color the room standing at x, y on level 0 is drawn in, or a null vector unless it is drawn just once
    static QVector4D roomColorAt(const ModernGLWidget& widget, const int x, const int y)
    {
        QList<QVector4D> colors;
        for (const DrawnCube& room : drawnRooms(widget)) {
            if (qRound(room.center.x()) == x && qRound(room.center.y()) == y && qRound(room.center.z()) == 0) {
                colors.append(room.color);
            }
        }
        return colors.size() == 1 ? colors.first() : QVector4D();
    }

    static QVector4D tintOf(const QColor& color) { return QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF()); }

    static int texturedCommands(const ModernGLWidget& widget)
    {
        int count = 0;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get())) {
                ++count;
            }
        }
        return count;
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

    void test_roomsOutOfViewAreLeftOutWithoutGL()
    {
        map()->mapClear();
        addGridArea(qsl("Culled"), 1, 128, 128, 1, 0);
        const int areaId = map()->mpRoomDB->getRoom(1)->getArea();
        map()->mRoomIdHash[map()->mProfileName] = 0;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId, 1.0f);

        QSet<std::pair<int, int>> drawn;
        for (const DrawnCube& room : drawnRooms(widget)) {
            drawn.insert({qRound(room.center.x()), qRound(room.center.y())});
        }
        QVERIFY2(drawn.size() < 128 * 128 / 4, qPrintable(qsl("%1 of the 16384 rooms drawn, when only those near the view's middle show").arg(drawn.size())));
        int shown = 0;
        const QRectF view(0.0, 0.0, widget.width(), widget.height());
        for (const int roomId : map()->mpRoomDB->getArea(areaId)->getAreaRooms()) {
            const TRoom* room = map()->mpRoomDB->getRoom(roomId);
            const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(room->x(), room->y(), room->z()));
            if (pixel && view.contains(*pixel)) {
                ++shown;
                QVERIFY2(drawn.contains({room->x(), room->y()}), qPrintable(qsl("room %1 shows at %2, %3 but is not drawn").arg(roomId).arg(pixel->x()).arg(pixel->y())));
            }
        }
        QVERIFY(shown > 0);
    }

    void test_roomsTooSmallToSeeHaveNothingDrawnOnThemWithoutGL()
    {
        map()->mapClear();
        addGridArea(qsl("Tiny"), 1, 64, 64, 1, 0);
        const int areaId = map()->mpRoomDB->getRoom(1)->getArea();
        map()->mRoomIdHash[map()->mProfileName] = 0;
        ModernGLWidget widget(map(), mpHost);
        const bool savedBorders = mpHost->mMapperShowRoomBorders;
        mpHost->mMapperShowRoomBorders = false;
        const float levelOutline = 0.6f / 32.0f;

        queueFrame(widget, areaId, 1.0f);
        QVERIFY2(texturedCommands(widget) > 0, "near rooms show their symbols and exit markers");
        QVERIFY2(!drawnCubes(widget, levelOutline).isEmpty(), "near rooms are outlined");

        queueFrame(widget, areaId, 100.0f);
        const float roomPixels = 0.5f / widget.mCameraController.worldUnitsPerPixel(QVector3D(0.0f, 0.0f, 0.0f));
        QVERIFY2(roomPixels < widget.mDetailPixels, qPrintable(qsl("rooms %1 pixels across are not too small to see").arg(roomPixels)));
        QCOMPARE(drawnRooms(widget).size(), 64 * 64);
        QCOMPARE(texturedCommands(widget), 0);
        QCOMPARE(drawnCubes(widget, levelOutline).size(), 0);
        mpHost->mMapperShowRoomBorders = savedBorders;
    }

    void test_largeAreaExitArrowsShowOnRoomsTooSmallToSeeWithoutGL()
    {
        map()->mapClear();
        addGridArea(qsl("Tiny"), 1, 64, 64, 1, 0);
        const int areaId = map()->mpRoomDB->getRoom(1)->getArea();
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Elsewhere"));
        const int elsewhere = 64 * 64 + 1;
        map()->addRoom(elsewhere);
        map()->setRoomArea(elsewhere, otherAreaId);
        map()->setRoomCoordinates(elsewhere, 40, 0, 0);
        // The east edge's middle room
        map()->setExit(1 + 32 * 64 + 63, elsewhere, DIR_EAST);
        map()->mRoomIdHash[map()->mProfileName] = 0;
        const bool savedLargeArrows = mpHost->getLargeAreaExitArrows();
        mpHost->setLargeAreaExitArrows(true);
        ModernGLWidget widget(map(), mpHost);
        const auto triangleVertices = [&widget]() {
            qsizetype count = 0;
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                // Above the level, where arrowheads lie on the exit lines, rather than its floor under the rooms
                if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get())) {
                    for (qsizetype z = 2; z < triangles->vertices().size(); z += 3) {
                        count += triangles->vertices().at(z) > 0.0f ? 1 : 0;
                    }
                }
            }
            return count;
        };

        queueFrame(widget, areaId, 100.0f);
        const float roomPixels = 0.5f / widget.mCameraController.worldUnitsPerPixel(QVector3D(0.0f, 0.0f, 0.0f));
        QVERIFY2(roomPixels < widget.mDetailPixels, qPrintable(qsl("rooms %1 pixels across are not too small to see").arg(roomPixels)));
        QCOMPARE(texturedCommands(widget), 0);
        mpHost->setLargeAreaExitArrows(savedLargeArrows);
        // The arrowhead and its outline, two triangles
        QVERIFY2(triangleVertices() >= 2 * 3, "the large area exit arrowhead is left off when zoomed out, which is what the setting is for");
    }

    void test_aRoomJustOutOfViewIsJudgedNearOrFarAsTheViewMovesWithoutGL()
    {
        RetainedMapScene scene;
        RetainedMapScene::Room room;
        room.id = 1;
        room.decalReach = 0.5f;
        scene.addRoom(room);
        QMatrix4x4 body;
        body.scale(0.5f);
        scene.addInstance(RetainedMapScene::Layer::Rooms, 1, 0, QPointF(0.0, 0.0), CubeInstanceData(body, 1.0f, 1.0f, 1.0f, 1.0f));
        scene.finish();

        const auto view = [](const QVector3D& camera, const QVector3D& target) {
            RetainedMapScene::View view;
            view.viewport = QSizeF(800.0, 600.0);
            QMatrix4x4 projection;
            projection.perspective(60.0f, 800.0f / 600.0f, 0.1f, 10000.0f);
            view.eye.lookAt(camera, target, QVector3D(0.0f, 1.0f, 0.0f));
            view.projection = projection * view.eye;
            view.unitsPerPixelPerDepth = 2.0f * std::tan(qDegreesToRadians(30.0f)) / 600.0f;
            view.roomSize = 1.0f;
            view.detailPixels = 3.0f;
            return view;
        };
        // Far above, the room is under a pixel wide
        scene.cull(view(QVector3D(0.0f, 0.0f, 1000.0f), QVector3D(0.0f, 0.0f, 0.0f)));
        QVERIFY(scene.decalRooms(100.0, 0.0f, 0.0f, {}, {}).isEmpty());

        // Close by, about 50 pixels wide, just past the view's left edge, near enough for its text to reach in
        scene.cull(view(QVector3D(9.5f, 0.0f, 10.0f), QVector3D(9.5f, 0.0f, 0.0f)));
        const QList<RetainedMapScene::DecalRoom> near = scene.decalRooms(100.0, 0.0f, 0.0f, {}, {});
        QCOMPARE(near.size(), 1);
        QVERIFY2(near.first().detailed, "a room just out of view but close by is judged by the view it is now in, not the last one that showed it");
    }

    void test_roomsAreMadeAgainOnlyOnceTheMapSaysItChangedWithoutGL()
    {
        const int areaId = addOrdinaryMap();
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId, 1.5f);
        // Not the player's room, which is drawn afresh each frame
        TRoom* room = map()->mpRoomDB->getRoom(map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(2, 0, 0).value(0));
        QVERIFY(room);
        const QVector4D before = roomColorAt(widget, 2, 0);
        QVERIFY(!before.isNull());
        room->environment = room->environment == 257 ? 258 : 257;
        const QVector4D after = tintOf(RoomAppearance::environmentColor(*map(), *mpHost, room->environment));
        QVERIFY(after != before);

        queueFrame(widget, areaId, 1.5f);
        QCOMPARE(roomColorAt(widget, 2, 0), before);
        map()->setUnsaved(__func__);
        queueFrame(widget, areaId, 1.5f);
        QCOMPARE(roomColorAt(widget, 2, 0), after);
    }

    void test_changesTheMapAnnouncesAreDrawnWithoutGL()
    {
        const int areaId = addOrdinaryMap();
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId, 1.5f);
        TRoom* room = map()->mpRoomDB->getRoom(map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(2, 0, 0).value(0));
        QVERIFY(room);

        // As highlightRoom() and other scripted changes that do not mark the map changed announce theirs
        room->environment = room->environment == 257 ? 258 : 257;
        map()->updateArea(areaId);
        QCoreApplication::processEvents();
        queueFrame(widget, areaId, 1.5f);
        QCOMPARE(roomColorAt(widget, 2, 0), tintOf(RoomAppearance::environmentColor(*map(), *mpHost, room->environment)));

        // A custom environment color, set by setCustomEnvColor() without announcing it
        const QColor savedColor = map()->mCustomEnvColors.value(room->environment);
        map()->mCustomEnvColors[room->environment] = QColor(12, 34, 56);
        queueFrame(widget, areaId, 1.5f);
        QCOMPARE(roomColorAt(widget, 2, 0), tintOf(QColor(12, 34, 56)));
        map()->mCustomEnvColors[room->environment] = savedColor;
        queueFrame(widget, areaId, 1.5f);

        // Another level viewed fades this one
        queueFrame(widget, areaId, 1.5f, true, 1);
        const QVector4D faded = roomColorAt(widget, 2, 0);
        QVERIFY(!faded.isNull());
        QVERIFY(faded != tintOf(RoomAppearance::environmentColor(*map(), *mpHost, room->environment)));
    }

    void test_theHoveredRoomIsDrawnRaisedJustOnceWithoutGL()
    {
        const int areaId = addOrdinaryMap();
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId, 1.5f);
        const auto* keptRooms = widget.mScene.instances(RetainedMapScene::Layer::Rooms)->instances.constData();
        const int roomId = map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(2, 0, 0).value(0);
        const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(2.0f, 0.0f, 0.0f));
        QVERIFY(pixel);
        widget.mPointerInView = true;
        widget.mPointerPosition = pixel->toPoint();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        QCOMPARE(widget.mHoveredRoomId, roomId);

        QList<float> heights;
        for (const DrawnCube& room : drawnRooms(widget)) {
            if (qRound(room.center.x()) == 2 && qRound(room.center.y()) == 0 && qRound(room.center.z()) == 0) {
                heights.append(room.center.z());
            }
        }
        QCOMPARE(heights.size(), 1);
        // A room's height, 2 / scale / zFlattening
        QCOMPARE(heights.first(), 0.0625f);
        QCOMPARE(drawnCubes(widget, 0.75f / 32.0f).size(), 1);
        QVERIFY2(widget.mScene.instances(RetainedMapScene::Layer::Rooms)->instances.constData() == keptRooms, "hovering made the rooms again");
        widget.mPointerInView = false;
    }

    void test_thePlayerMovingLeavesTheRoomsAsTheyWereMadeWithoutGL()
    {
        const int areaId = addOrdinaryMap();
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areaId, 1.5f);
        const auto* keptRooms = widget.mScene.instances(RetainedMapScene::Layer::Rooms)->instances.constData();
        const int roomId = map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(2, 0, 0).value(0);
        QVERIFY(roomId);
        QVERIFY(roomId != map()->mRoomIdHash.value(map()->mProfileName));

        // As centerview() and a game's room number do
        map()->mRoomIdHash[map()->mProfileName] = roomId;
        map()->updatePlayerArea(areaId);
        QCoreApplication::processEvents();
        queueFrame(widget, areaId, 1.5f);
        QVERIFY2(widget.mScene.instances(RetainedMapScene::Layer::Rooms)->instances.constData() == keptRooms, "the player moving made the rooms again");
        QVERIFY2(!roomColorAt(widget, 2, 0).isNull(), "the player's new room is not drawn just once");
    }

    void test_aChangedRoomIsAllThatIsUploadedAgain()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        const int areaId = addOrdinaryMap();
        ModernGLWidget widget(map(), mpHost);
        widget.resize(800, 600);
        widget.grabFramebuffer();
        if (!widget.isValid() || !widget.failureMessage().isEmpty()) {
            QSKIP("no OpenGL context on this platform");
        }
        widget.mShiftMode = true;
        widget.jumpTo(areaId, 0, 0, 0);
        widget.grabFramebuffer();
        const RetainedInstances& rooms = *widget.mScene.instances(RetainedMapScene::Layer::Rooms);
        QVERIFY(!rooms.resized);

        TRoom* room = map()->mpRoomDB->getRoom(map()->mpRoomDB->getArea(areaId)->getRoomsByPosition(2, 0, 0).value(0));
        room->environment = room->environment == 257 ? 258 : 257;
        map()->setUnsaved(__func__);
        widget.makeCurrent();
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        widget.doneCurrent();
        QVERIFY(!rooms.resized);
        QCOMPARE(rooms.changedTo - rooms.changedFrom, qsizetype{1});
        widget.mRenderCommandQueue.clear();
    }

    void benchmark_hugeMap()
    {
        if (qEnvironmentVariableIsEmpty("MUDLET_BENCH_3D")) {
            QSKIP("set MUDLET_BENCH_3D=1 to run the 3D view benchmark");
        }
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QElapsedTimer generation;
        generation.start();
        mpHost->mMapInfoContributors.clear();
        const int areaId = addHugeMap();
        std::printf("METRIC map_rooms %d\n", static_cast<int>(map()->mpRoomDB->size()));
        std::printf("METRIC map_generation_ms %lld\n", generation.elapsed());
        std::printf("METRIC rss_after_map_mib %.1f\n", memoryMiB("VmRSS"));

        ModernGLWidget widget(map(), mpHost);
        widget.resize(1200, 800);
        widget.grabFramebuffer();
        if (!widget.isValid() || !widget.failureMessage().isEmpty()) {
            QSKIP("no OpenGL context on this platform");
        }
        widget.mShiftMode = true;
        widget.jumpTo(areaId, 0, 0, 0);

        struct Scenario
        {
            const char* name;
            float distance;
            bool topView;
        };
        // close is about the 2D map's default zoom; far shows the whole of the viewed level and more
        const QList<Scenario> scenarios{{"close", 1.0f, false}, {"mid", 8.0f, false}, {"far", 40.0f, false}, {"far_top", 40.0f, true}};
        const int frames = qEnvironmentVariableIntValue("MUDLET_BENCH_3D_FRAMES_PER_SCENARIO") > 0 ? qEnvironmentVariableIntValue("MUDLET_BENCH_3D_FRAMES_PER_SCENARIO") : 20;
        for (const Scenario& scenario : scenarios) {
            if (scenario.topView) {
                widget.mCameraController.setTopView();
            } else {
                widget.mCameraController.setDefaultView();
            }
            widget.setCameraDistance(scenario.distance);
            // The first frame builds whatever is kept between frames
            QElapsedTimer first;
            first.start();
            widget.grabFramebuffer();
            const qint64 firstMs = first.elapsed();

            QElapsedTimer wall;
            const double cpuBefore = processCpuMs();
            wall.start();
            for (int frame = 0; frame < frames; ++frame) {
                spin(widget);
                widget.grabFramebuffer();
            }
            const double wallMs = static_cast<double>(wall.nsecsElapsed()) / 1.0e6 / frames;
            const double cpuMs = (processCpuMs() - cpuBefore) / frames;

            // The CPU work of working out a frame, without drawing it
            widget.makeCurrent();
            QElapsedTimer build;
            build.start();
            for (int frame = 0; frame < frames; ++frame) {
                spin(widget);
                widget.updateMatrices();
                widget.mRenderCommandQueue.clear();
                widget.queueScene();
            }
            const double buildMs = static_cast<double>(build.nsecsElapsed()) / 1.0e6 / frames;
            widget.mRenderCommandQueue.clear();
            widget.doneCurrent();

            std::printf("METRIC %s_first_frame_ms %lld\n", scenario.name, firstMs);
            std::printf("METRIC %s_frame_ms %.2f\n", scenario.name, wallMs);
            std::printf("METRIC %s_frame_cpu_ms %.2f\n", scenario.name, cpuMs);
            std::printf("METRIC %s_scene_build_ms %.2f\n", scenario.name, buildMs);
        }

        // Walking: the player's room moves on each frame while the view stays put
        widget.mCameraController.setDefaultView();
        widget.setCameraDistance(8.0f);
        widget.grabFramebuffer();
        QElapsedTimer walk;
        const double walkCpuBefore = processCpuMs();
        walk.start();
        const int walkStart = map()->mRoomIdHash.value(map()->mProfileName);
        for (int frame = 0; frame < frames; ++frame) {
            const TRoom* room = map()->mpRoomDB->getRoom(walkStart + frame + 1);
            map()->mRoomIdHash[map()->mProfileName] = walkStart + frame + 1;
            widget.jumpTo(areaId, room->x(), room->y(), room->z());
            widget.grabFramebuffer();
        }
        std::printf("METRIC walk_frame_ms %.2f\n", static_cast<double>(walk.nsecsElapsed()) / 1.0e6 / frames);
        std::printf("METRIC walk_frame_cpu_ms %.2f\n", (processCpuMs() - walkCpuBefore) / frames);

        // Editing: one room's environment changes before each frame
        QElapsedTimer edit;
        const double editCpuBefore = processCpuMs();
        edit.start();
        for (int frame = 0; frame < frames; ++frame) {
            TRoom* room = map()->mpRoomDB->getRoom(walkStart + 141 + frame);
            room->environment = 257 + (room->environment - 256) % 16;
            map()->setUnsaved(__func__);
            widget.grabFramebuffer();
        }
        std::printf("METRIC edit_frame_ms %.2f\n", static_cast<double>(edit.nsecsElapsed()) / 1.0e6 / frames);
        std::printf("METRIC edit_frame_cpu_ms %.2f\n", (processCpuMs() - editCpuBefore) / frames);

        std::printf("METRIC rss_end_mib %.1f\n", memoryMiB("VmRSS"));
        std::printf("METRIC rss_peak_mib %.1f\n", memoryMiB("VmHWM"));
        std::fflush(stdout);
    }

    void frames_forComparison()
    {
        const QString directory = qEnvironmentVariable("MUDLET_BENCH_3D_FRAMES");
        if (directory.isEmpty()) {
            QSKIP("set MUDLET_BENCH_3D_FRAMES to a directory to save frames for comparing builds");
        }
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(QDir().mkpath(directory));
        // In a debug build the map info would show each frame's render time
        mpHost->mMapInfoContributors.clear();
        const bool savedIds = mpHost->mShowRoomID;
        const bool savedBubbles = mpHost->mBubbleMode;
        const bool savedGrid = mpHost->mMapperShowGrid;
        const auto save = [&](ModernGLWidget& widget, const QString& name) {
            QVERIFY(widget.grabFramebuffer().save(qsl("%1/%2.png").arg(directory, name)));
        };
        const auto views = [&](ModernGLWidget& widget, const QString& prefix, const QList<float>& distances) {
            for (const float distance : distances) {
                widget.mCameraController.setDefaultView();
                widget.setCameraDistance(distance);
                save(widget, qsl("%1-d%2-default").arg(prefix).arg(distance));
                widget.turnCamera(37.0f);
                save(widget, qsl("%1-d%2-turned").arg(prefix).arg(distance));
                widget.mCameraController.setTopView();
                widget.setCameraDistance(distance);
                save(widget, qsl("%1-d%2-top").arg(prefix).arg(distance));
                widget.mCameraController.setSideView();
                widget.setCameraDistance(distance);
                save(widget, qsl("%1-d%2-side").arg(prefix).arg(distance));
            }
        };

        {
            const int areaId = addOrdinaryMap();
            ModernGLWidget widget(map(), mpHost);
            widget.resize(900, 700);
            widget.grabFramebuffer();
            if (!widget.isValid() || !widget.failureMessage().isEmpty()) {
                QSKIP("no OpenGL context on this platform");
            }
            widget.mShiftMode = true;
            for (const int level : {0, -1, 1}) {
                widget.jumpTo(areaId, 0, 0, level);
                mpHost->mShowRoomID = false;
                mpHost->mBubbleMode = false;
                mpHost->mMapperShowGrid = false;
                views(widget, qsl("ordinary-z%1").arg(level), {0.6f, 1.5f, 4.0f});
                mpHost->mShowRoomID = true;
                mpHost->mBubbleMode = true;
                mpHost->mMapperShowGrid = true;
                views(widget, qsl("ordinary-ids-round-z%1").arg(level), {0.6f, 1.5f});
            }
            mpHost->mShowRoomID = false;
            mpHost->mBubbleMode = false;
            mpHost->mMapperShowGrid = false;
            map()->mpRoomDB->getArea(areaId)->gridMode = true;
            widget.jumpTo(areaId, 0, 0, 0);
            views(widget, qsl("ordinary-gridmode"), {1.5f});
            map()->mpRoomDB->getArea(areaId)->gridMode = false;
        }
        {
            const int areaId = addHugeMap();
            ModernGLWidget widget(map(), mpHost);
            widget.resize(1200, 800);
            widget.mShiftMode = true;
            widget.jumpTo(areaId, 0, 0, 0);
            views(widget, qsl("huge"), {1.0f, 3.0f});
        }
        mpHost->mShowRoomID = savedIds;
        mpHost->mBubbleMode = savedBubbles;
        mpHost->mMapperShowGrid = savedGrid;
    }
};

#include "ModernMapperSceneTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperSceneTest)
