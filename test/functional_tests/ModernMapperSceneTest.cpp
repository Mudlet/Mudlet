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
 * With MUDLET_BENCH_3D=1 a third prints METRIC lines for searching the big map.
 * MUDLET_BENCH_3D_STITCH=0 turns neighboring areas off for both, and
 * MUDLET_BENCH_3D_LINKS=1 joins the big map's first suburb beside its metropolis.
 */

#include <ctime>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QOpenGLContext>
#include <QPainter>
#include <QProxyStyle>
#include <QScrollBar>
#include <QStyleOptionViewItem>
#include <QToolButton>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "RenderCommand.h"
#include "MapRoute.h"
#include "MapSearch.h"
#include "MapSearchPanel.h"
#include "RetainedMapScene.h"
#include "RoomAppearance.h"
#include "TArea.h"
#include "TAreaGraph.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Counts the events of one type that reach the object it watches
class EventCounter : public QObject
{
public:
    EventCounter(QObject* watched, QEvent::Type type)
    : mType(type)
    {
        watched->installEventFilter(this);
    }
    int count = 0;

protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        if (event->type() == mType) {
            ++count;
        }
        return false;
    }

private:
    QEvent::Type mType;
};

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

    // CPU time of every thread, not wall time; clock_gettime() is missing on Windows, where std::clock() counts wall time instead
    static double processCpuMs() { return static_cast<double>(std::clock()) * 1000.0 / CLOCKS_PER_SEC; }

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
        if (!qEnvironmentVariableIsEmpty("MUDLET_BENCH_3D_LINKS")) {
            // Its whole east side on the middle level east into the first suburb's west side, which places it beside
            for (int y = -35; y <= 35; ++y) {
                map()->setExit(1 + (141 + y + 70) * 141 + 140, nextId + (y + 35) * 71, DIR_EAST);
            }
        }
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

    // Two areas of 20 by 20 rooms, joined by an exit; returns their IDs
    std::pair<int, int> addTwoAreas(const int side = 20) const
    {
        map()->mapClear();
        map()->mUserData.remove(TAreaGraph::cLayoutUserDataKey);
        const int next = addGridArea(qsl("Westmarch"), 1, side, side, 1, 0);
        addGridArea(qsl("Eastvale"), next, side, side, 1, 0);
        map()->setExit(1, next, DIR_EAST);
        map()->mRoomIdHash[map()->mProfileName] = 0;
        return {map()->mpRoomDB->getRoom(1)->getArea(), map()->mpRoomDB->getRoom(next)->getArea()};
    }

    // The opacities of the layers the atlas queued, in order
    static QList<float> atlasLayerOpacities(const ModernGLWidget& widget)
    {
        QList<float> opacities;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get()); state && state->stateType() == GLStateCommand::BLEND_BY_OPACITY) {
                opacities.append(state->opacity());
            }
        }
        return opacities;
    }

    // Where the middle of an area of the atlas is in the view
    static QPoint pixelOfArea(ModernGLWidget& widget, const int areaId, const QPointF& fromMiddle = QPointF())
    {
        const MapAtlas::Area* pArea = widget.mAtlas.area(areaId);
        const MapAtlas::Area* pShown = widget.mAtlas.area(widget.mAID);
        if (!pArea || !pShown) {
            return QPoint(-1, -1);
        }
        const QPointF local = pArea->center + fromMiddle - pShown->offset;
        const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(QVector3D(static_cast<float>(local.x()), static_cast<float>(local.y()), widget.atlasLevel()));
        return pixel ? pixel->toPoint() : QPoint(-1, -1);
    }


    struct ThreeAreas
    {
        int west = 0;
        int middle = 0;
        int east = 0;
        // The first room of each, at its southwest corner; rooms run west to east then north, five to a row
        int westFirst = 0;
        int middleFirst = 0;
        int eastFirst = 0;
    };

    // Three areas of side by side rooms in a row, each joined to the next both ways from the middle of its east side to
    // the middle of the next one's west side, with the player in the west area's first room
    ThreeAreas addThreeAreas(const int side = 5) const
    {
        map()->mapClear();
        map()->mUserData.remove(TAreaGraph::cLayoutUserDataKey);
        ThreeAreas areas;
        areas.westFirst = 1;
        areas.middleFirst = addGridArea(qsl("Westmarch"), areas.westFirst, side, side, 1, 0);
        areas.eastFirst = addGridArea(qsl("Midlands"), areas.middleFirst, side, side, 1, 0);
        addGridArea(qsl("Eastvale"), areas.eastFirst, side, side, 1, 0);
        const int wayIn = side / 2 * side;
        const int wayOut = wayIn + side - 1;
        for (const auto& [from, to] : {std::pair{areas.westFirst, areas.middleFirst}, std::pair{areas.middleFirst, areas.eastFirst}}) {
            map()->setExit(from + wayOut, to + wayIn, DIR_EAST);
            map()->setExit(to + wayIn, from + wayOut, DIR_WEST);
        }
        areas.west = map()->mpRoomDB->getRoom(areas.westFirst)->getArea();
        areas.middle = map()->mpRoomDB->getRoom(areas.middleFirst)->getArea();
        areas.east = map()->mpRoomDB->getRoom(areas.eastFirst)->getArea();
        map()->mRoomIdHash[map()->mProfileName] = areas.westFirst;
        return areas;
    }

    // The route's bars over the rooms, which are a quarter as tall as a room
    static QList<DrawnCube> drawnRoute(const ModernGLWidget& widget) { return drawnCubes(widget, 1.0f / 128.0f); }

    // MUDLET_BENCH_3D_STITCH=0 turns stitching off for the report-only cases, to compare with a build without it
    void applyBenchStitching() const { mpHost->mMapperStitchAreas = qEnvironmentVariable("MUDLET_BENCH_3D_STITCH") != qsl("0"); }

    // As queueFrame(), centered on a place in the area rather than on 0,0
    static void queueFrameAt(ModernGLWidget& widget, const int areaId, const float distance, const QPoint& center)
    {
        queueFrame(widget, areaId, distance);
        widget.jumpTo(areaId, center.x(), center.y(), 0);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
    }

    static QPoint pixelOf(ModernGLWidget& widget, const QVector3D& point)
    {
        const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(point);
        return pixel ? pixel->toPoint() : QPoint(-1, -1);
    }

    static void finishFlight(ModernGLWidget& widget)
    {
        for (int tick = 0; tick < 1000 && widget.mFlight; ++tick) {
            widget.onCameraAnimationTick();
        }
    }

    static void clickAt(ModernGLWidget& widget, const QPoint& position)
    {
        // A plain press over a neighboring area waits for its release, see mousePressEvent()
        widget.mClickPending = true;
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(position), widget.mapToGlobal(QPointF(position)), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        widget.mouseReleaseEvent(&release);
    }

    // The names of what a search found, best first
    static QStringList foundNames(const QList<MapSearch::Result>& results)
    {
        QStringList names;
        for (const MapSearch::Result& result : results) {
            names.append(result.name);
        }
        return names;
    }

    // The search's mark around a room, whose bars are a third as tall as a room
    static QList<DrawnCube> drawnSearchMark(const ModernGLWidget& widget) { return drawnCubes(widget, 1.0f / 96.0f); }

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
        const QList<RetainedMapScene::DecalRoom> nearRooms = scene.decalRooms(100.0, 0.0f, 0.0f, {}, {});
        QCOMPARE(nearRooms.size(), 1);
        QVERIFY2(nearRooms.first().detailed, "a room just out of view but close by is judged by the view it is now in, not the last one that showed it");
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

    void test_zoomLevelsComeInAsRoomsShrinkWithoutGL()
    {
        const auto roughly = [](const float a, const float b) {
            return qAbs(a - b) < 1e-5f;
        };
        const float farOut = MapAtlas::scmFootprintsOnlyDistance;
        MapAtlas::Weights weights = MapAtlas::weightsFor(MapAtlas::scmRoomsAlonePixels, 100.0f, farOut);
        QVERIFY(roughly(weights.footprint, 0.0f) && roughly(weights.world, 0.0f));
        QVERIFY(weights.roomsShown() && !weights.areasPicked());
        weights = MapAtlas::weightsFor((MapAtlas::scmRoomsAlonePixels + MapAtlas::scmFootprintsOnlyPixels) / 2.0f, 100.0f, farOut);
        QVERIFY2(roughly(weights.footprint, 0.5f), qPrintable(qsl("half way the footprints are %1 in").arg(weights.footprint)));
        QVERIFY(weights.roomsShown() && weights.areasPicked());
        weights = MapAtlas::weightsFor(MapAtlas::scmFootprintsOnlyPixels, MapAtlas::scmWorldFromPixels, farOut);
        QVERIFY(roughly(weights.footprint, 1.0f) && roughly(weights.world, 0.0f));
        QVERIFY(!weights.roomsShown() && weights.areasPicked() && !weights.nodesPicked());
        weights = MapAtlas::weightsFor(0.1f, MapAtlas::scmWorldOnlyPixels, farOut);
        QVERIFY(roughly(weights.footprint, 1.0f) && roughly(weights.world, 1.0f));
        QVERIFY(weights.nodesPicked());
        // However small the rooms, as far as the camera could go before the atlas they show alone
        weights = MapAtlas::weightsFor(0.1f, 100.0f, CameraController::scmMaxDistance);
        QVERIFY(roughly(weights.footprint, 0.0f));
        weights = MapAtlas::weightsFor(0.1f, 100.0f, (MapAtlas::scmRoomsAloneDistance + MapAtlas::scmFootprintsOnlyDistance) / 2.0f);
        QVERIFY2(roughly(weights.footprint, 0.5f), qPrintable(qsl("half way out the footprints are %1 in").arg(weights.footprint)));
        float lastFootprint = 0.0f;
        float lastWorld = 0.0f;
        for (float roomPixels = 4.0f; roomPixels > 0.05f; roomPixels *= 0.9f) {
            weights = MapAtlas::weightsFor(roomPixels, roomPixels * 10.0f, 100.0f / roomPixels);
            QVERIFY2(weights.footprint >= lastFootprint && weights.world >= lastWorld, "zooming out took a level back out");
            lastFootprint = weights.footprint;
            lastWorld = weights.world;
        }

        const auto [westId, eastId] = addTwoAreas(60);
        ModernGLWidget widget(map(), mpHost);
        // The 2D map's usual framing, and as far out as the zoom slider goes, at 600 pixels high
        for (const float distance : {1.5f, 20.0f, CameraController::scmMaxDistance}) {
            queueFrame(widget, westId, distance);
            QVERIFY2(atlasLayerOpacities(widget).isEmpty(), qPrintable(qsl("the atlas is drawn at distance %1").arg(distance)));
            QVERIFY(!drawnRooms(widget).isEmpty());
        }
        QVERIFY2(!map()->mUserData.contains(TAreaGraph::cLayoutUserDataKey), "the area graph was laid out, and the map changed, without zooming out to it");
        QVERIFY(widget.mAtlas.areas().isEmpty());

        // Fading in over the rooms
        queueFrame(widget, westId, 150.0f);
        QList<float> opacities = atlasLayerOpacities(widget);
        QVERIFY(!drawnRooms(widget).isEmpty());
        QVERIFY2(opacities.size() == 1 && opacities.first() > 0.0f && opacities.first() < 1.0f, qPrintable(qsl("%1 layers queued").arg(opacities.size())));
        QCOMPARE(widget.mAtlas.areas().size(), 2);

        queueFrame(widget, westId, 200.0f);
        QVERIFY(drawnRooms(widget).isEmpty());
        QCOMPARE(atlasLayerOpacities(widget), QList<float>{1.0f});

        // The world graph and the ring around the area shown, without the footprints
        queueFrame(widget, westId, 800.0f);
        QCOMPARE(atlasLayerOpacities(widget), (QList<float>{1.0f, 1.0f}));
        const auto* world = [&widget]() -> const RenderTrianglesCommand* {
            for (const auto& command : widget.mRenderCommandQueue.commands()) {
                if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command.get())) {
                    return triangles;
                }
            }
            return nullptr;
        }();
        QVERIFY(world);
        QCOMPARE(world->vertices().size(), widget.mAtlas.worldVertices().size());
        QVERIFY(widget.mAtlas.area(eastId));
    }

    void test_anAreaFootprintCoversItsRoomsWithoutGL()
    {
        QCOMPARE(MapAtlas::hull({QPointF(0, 0), QPointF(1, 0), QPointF(2, 0), QPointF(0, 1), QPointF(1, 1), QPointF(2, 1), QPointF(0, 2), QPointF(1, 2), QPointF(2, 2), QPointF(2, 2)}),
                 QPolygonF({QPointF(0, 0), QPointF(2, 0), QPointF(2, 2), QPointF(0, 2)}));

        map()->mapClear();
        map()->mUserData.remove(TAreaGraph::cLayoutUserDataKey);
        const int areaId = map()->mpRoomDB->addArea(qsl("Ell"));
        // An L: three along the bottom, then two up the left side
        const QList<QPoint> places{{0, 0}, {1, 0}, {2, 0}, {0, 1}, {0, 2}};
        for (int i = 0; i < places.size(); ++i) {
            const int id = i + 1;
            map()->addRoom(id);
            map()->setRoomArea(id, areaId);
            map()->setRoomCoordinates(id, places.at(i).x(), places.at(i).y(), 0);
            map()->mpRoomDB->getRoom(id)->environment = id <= 3 ? 300 : 301;
        }
        map()->areaGraph()->refresh();
        MapAtlas atlas;
        atlas.build(
                *map(),
                [](const int environment) {
                    return environment == 300 ? QColor(10, 200, 30) : QColor(200, 10, 30);
                },
                QColor(Qt::white));
        const MapAtlas::Area* pArea = atlas.area(areaId);
        QVERIFY(pArea);
        QCOMPARE(pArea->roomCount, 5);
        QCOMPARE(pArea->color, QColor(10, 200, 30));
        QCOMPARE(pArea->name, qsl("Ell"));
        QCOMPARE(pArea->offset, pArea->center - QPointF(1.0, 1.0));
        // Cut across the inside of the L, over the corners of the rooms' cells
        QCOMPARE(pArea->footprint.translated(-pArea->offset), QPolygonF({QPointF(-0.5, -0.5), QPointF(2.5, -0.5), QPointF(2.5, 0.5), QPointF(0.5, 2.5), QPointF(-0.5, 2.5)}));
        // Of the two rooms nearest the middle, the lower ID
        QCOMPARE(pArea->landingRoomId, 2);
        QCOMPARE(pArea->landing, QPoint(1, 0));
        // A fan of three triangles, of seven floats a corner
        QCOMPARE(atlas.footprintVertices().size(), 3 * 3 * 3);
        QCOMPARE(atlas.footprintColors().size(), 3 * 3 * 4);
        QCOMPARE(atlas.footprintColors().at(1), QColor(10, 200, 30).greenF());
    }

    void test_areasArePlacedByTheStoredLayoutOrElseLaidOutWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas();
        ModernGLWidget widget(map(), mpHost);
        widget.refreshAtlas();
        const MapAtlas::Area* pWest = widget.mAtlas.area(westId);
        const MapAtlas::Area* pEast = widget.mAtlas.area(eastId);
        QVERIFY(pWest && pEast);
        QVERIFY2(QLineF(pWest->center, pEast->center).length() >= pWest->radius + pEast->radius, "two areas laid out on each other");
        const QPointF westCenter = pWest->center;
        const QPointF eastCenter = pEast->center;

        // Laid out the same way again
        map()->mUserData.remove(TAreaGraph::cLayoutUserDataKey);
        widget.refreshAtlas();
        QCOMPARE(widget.mAtlas.area(westId)->center, westCenter);
        QCOMPARE(widget.mAtlas.area(eastId)->center, eastCenter);

        map()->mUserData[TAreaGraph::cLayoutUserDataKey] = qsl(R"({"areas":{"%1":{"x":-300,"y":40,"pinned":true}}})").arg(westId);
        widget.refreshAtlas();
        QCOMPARE(widget.mAtlas.area(westId)->center, QPointF(-300.0, 40.0));
        // Rooms run from -10 to 9 each way, so the middle of the area is at -0.5, -0.5
        QCOMPARE(widget.mAtlas.area(westId)->offset, QPointF(-299.5, 40.5));

        QVERIFY(map()->areaGraph()->setPosition(eastId, QPointF(500.0, -500.0)));
        widget.refreshAtlas();
        QCOMPARE(widget.mAtlas.area(eastId)->center, QPointF(500.0, -500.0));
        QCOMPARE(widget.mAtlas.area(westId)->center, QPointF(-300.0, 40.0));
    }

    void test_aClickFliesIntoTheAreaUnderItWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas(60);
        // The east area straight up the view from the west one, far enough for both names to show
        map()->areaGraph()->refresh();
        QVERIFY(map()->areaGraph()->setPosition(westId, QPointF(0.0, 0.0)));
        QVERIFY(map()->areaGraph()->setPosition(eastId, QPointF(0.0, 150.0)));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, westId, 1.5f);
        QVERIFY2(widget.roomAt(QPoint(400, 300)), "no room in the middle of the view");
        QCOMPARE(widget.areaAt(QPoint(400, 300)), 0);

        queueFrame(widget, westId, 200.0f);
        const MapAtlas::Area* pWest = widget.mAtlas.area(westId);
        const MapAtlas::Area* pEast = widget.mAtlas.area(eastId);
        QVERIFY(pWest && pEast);
        QCOMPARE(widget.areaAt(pixelOfArea(widget, westId)), westId);
        QCOMPARE(widget.areaAt(pixelOfArea(widget, eastId)), eastId);
        QCOMPARE(widget.roomAt(pixelOfArea(widget, westId)), 0);
        QCOMPARE(widget.roomPickedAt(pixelOfArea(widget, westId)), 0);
        // Out the far side of the west area from the east one, where there is nothing
        const QPointF away = (pWest->center - pEast->center) / QLineF(pWest->center, pEast->center).length();
        QCOMPARE(widget.areaAt(pixelOfArea(widget, westId, away * (pWest->radius + 40.0))), 0);
        const QList<ModernGLWidget::AtlasLabel> labels = widget.atlasLabels(widget.mZoomWeights);
        QCOMPARE(labels.size(), 2);
        QCOMPARE(labels.first().text, qsl("Westmarch"));
        QVERIFY(labels.first().shownArea && !labels.last().shownArea);

        // A node is picked a few pixels off its rim, but not well clear of it
        queueFrame(widget, westId, 800.0f);
        QVERIFY(widget.mZoomWeights.nodesPicked());
        const float unitsPerPixel = widget.mCameraController.worldUnitsPerPixel(widget.mCameraController.getTarget());
        QCOMPARE(widget.areaAt(pixelOfArea(widget, eastId, -away * (pEast->radius + 3.0f * unitsPerPixel))), eastId);
        QCOMPARE(widget.areaAt(pixelOfArea(widget, eastId, -away * (pEast->radius + 20.0f * unitsPerPixel))), 0);

        const QVector3D targetBefore = widget.mCameraController.getTarget();
        const QPointF westOffset = pWest->offset;
        const QPointF eastOffset = pEast->offset;
        const QPoint eastLanding = pEast->landing;
        widget.flyToArea(widget.areaAt(pixelOfArea(widget, eastId)));
        QCOMPARE(widget.mAID, eastId);
        QVERIFY(widget.mFlight);
        // The same place in the world, in the east area's coordinates, so the view does not jump
        const QVector3D targetAfter = widget.mCameraController.getTarget();
        QVERIFY2(QLineF(QPointF(targetAfter.x(), targetAfter.y()) + eastOffset, QPointF(targetBefore.x(), targetBefore.y()) + westOffset).length() < 1e-3, "the view jumped as the flight began");
        float lastScale = widget.mCameraController.getScale();
        for (int tick = 0; tick < 1000 && widget.mFlight; ++tick) {
            widget.onCameraAnimationTick();
            QVERIFY2(widget.mCameraController.getScale() <= lastScale, "the flight zoomed out");
            lastScale = widget.mCameraController.getScale();
        }
        QVERIFY(!widget.mFlight);
        QVERIFY(!widget.mCameraSmoothAnimating);
        QCOMPARE(widget.mCameraController.getTarget(), QVector3D(eastLanding.x(), eastLanding.y(), 0.0f));
        QCOMPARE(QPoint(widget.mMapCenterX, widget.mMapCenterY), eastLanding);
        QCOMPARE(widget.mCameraController.getScale(), CameraController::distanceToShow(static_cast<float>(map()->mpRoomDB->get2DMapZoom(eastId)), 800.0f / 600.0f));
        widget.updateMatrices();
        QVERIFY(!widget.zoomWeights().areasPicked());
        QVERIFY(widget.roomAt(QPoint(400, 300)));
    }

    void test_theWorldViewIsNotKeptAsAnAreasCameraWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas();
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, westId, 800.0f);
        QVERIFY(widget.mZoomWeights.nodesPicked());
        widget.mFramePending = false;
        widget.jumpTo(eastId, 0, 0, 0);
        QVERIFY(widget.mSavedCameras.contains(westId));
        QVERIFY2(widget.mSavedCameras.value(westId).distance <= CameraController::scmMaxDistance, "coming back to the area would show the world, not its rooms");
        widget.jumpTo(westId, 0, 0, 0);
        widget.updateMatrices();
        QCOMPARE(widget.zoomWeights().footprint, 0.0f);

        // The zoom slider is not overridden by a flight still under way
        widget.setCameraDistance(800.0f);
        widget.flyToArea(westId);
        QVERIFY(widget.mFlight);
        widget.slot_setScale(50);
        QVERIFY(!widget.mFlight);
        const float sliderScale = widget.mCameraController.getScale();
        widget.onCameraAnimationTick();
        QCOMPARE(widget.mCameraController.getScale(), sliderScale);
    }

    void test_aFlightGivesWayToOtherCameraMovesWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas();
        ModernGLWidget widget(map(), mpHost);
        const auto settle = [&widget] {
            for (int tick = 0; tick < 1000 && widget.mCameraSmoothAnimating; ++tick) {
                widget.onCameraAnimationTick();
            }
        };
        queueFrame(widget, westId, 800.0f);
        widget.flyToArea(eastId);
        widget.onCameraAnimationTick();
        // As set3DViewCameraPosition() would
        widget.setCameraDistance(30.0f);
        settle();
        QCOMPARE(widget.mCameraController.getScale(), 30.0f);

        // As setMapZoom() would
        widget.setCameraDistance(800.0f);
        widget.flyToArea(westId);
        widget.onCameraAnimationTick();
        widget.applyMapZoom(13.0, westId);
        const float framedScale = widget.mCameraController.getScale();
        settle();
        QCOMPARE(widget.mCameraController.getScale(), framedScale);

        // As centerview() or the arrow keys would
        widget.setCameraDistance(800.0f);
        widget.flyToArea(westId);
        widget.onCameraAnimationTick();
        widget.setViewCenter(westId, 2, 3, 0);
        settle();
        QCOMPARE(widget.mCameraController.getTarget(), QVector3D(2.0f, 3.0f, 0.0f));
    }

    void test_aRouteIsFoundAcrossAreasWithTheMapsOwnPathfindingWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        // What a speedwalk or getPath() found last stays as it was
        map()->mPathList = {7, 8};
        map()->mDirList = {qsl("n")};
        ModernGLWidget widget(map(), mpHost);
        const int destination = areas.eastFirst + 24;
        QCOMPARE(widget.planRouteToRoom(destination), QString());
        const MapRoute& route = widget.route();
        QCOMPARE(route.status(), MapRoute::Status::Found);
        QCOMPARE(route.rooms().first(), areas.westFirst);
        QCOMPARE(route.rooms().last(), destination);
        // Six steps across the west area to its way out, one into the middle and four across it, then one in and six across the east
        QCOMPARE(route.steps(), 18);
        QCOMPARE(route.totalWeight(), 18);
        QCOMPARE(route.areaRuns(), (QList<MapRoute::AreaRun>{{areas.west, 6}, {areas.middle, 5}, {areas.east, 7}}));
        QCOMPARE(route.stepsByArea().value(areas.middle), 5);
        QCOMPARE(map()->mPathList, (QList<int>{7, 8}));
        QCOMPARE(map()->mDirList, QList<QString>{qsl("n")});

        // A locked room is kept out of the way, as a speedwalk would; locking it changes no revision, only the graph
        map()->mpRoomDB->getRoom(areas.middleFirst + 10)->isLocked = true;
        map()->mMapGraphNeedsUpdate = true;
        QVERIFY(route.stale(*map()));
        widget.refreshRoute();
        QCOMPARE(widget.route().status(), MapRoute::Status::Unreachable);
        QVERIFY(widget.route().rooms().isEmpty());
        QVERIFY(widget.route().hasDestination());
        map()->mpRoomDB->getRoom(areas.middleFirst + 10)->isLocked = false;
        map()->mMapGraphNeedsUpdate = true;
        widget.refreshRoute();
        QCOMPARE(widget.route().status(), MapRoute::Status::Found);
        QCOMPARE(widget.route().steps(), 18);
    }

    void test_theAreasARoutePassesThroughAreTakenInOrderWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        TRoomDB& roomDB = *map()->mpRoomDB;
        const int w = areas.westFirst;
        const int m = areas.middleFirst;
        const int e = areas.eastFirst;
        QCOMPARE(MapRoute::areaRunsOf(roomDB, {w}), (QList<MapRoute::AreaRun>{{areas.west, 0}}));
        // Out of the west area, back into it and on to the east; a room no longer in the map is passed over
        QCOMPARE(MapRoute::areaRunsOf(roomDB, {w, w + 1, m, m + 1, 99999, w + 2, e}), (QList<MapRoute::AreaRun>{{areas.west, 1}, {areas.middle, 2}, {areas.west, 1}, {areas.east, 1}}));
        QVERIFY(MapRoute::areaRunsOf(roomDB, {}).isEmpty());
    }

    void test_aRouteToAnAreaEndsAtTheNearestOfItsRoomsWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        // A one-way shortcut from the west area's northeast corner straight into the east area's northwest corner
        map()->setExit(areas.westFirst + 24, areas.eastFirst + 20, DIR_NORTHEAST);
        ModernGLWidget widget(map(), mpHost);
        QCOMPARE(widget.planRouteToArea(areas.east), QString());
        QCOMPARE(widget.route().status(), MapRoute::Status::Found);
        QCOMPARE(widget.route().endRoomId(), areas.eastFirst + 20);
        QCOMPARE(widget.route().steps(), 9);
        QCOMPARE(widget.route().areaRuns(), (QList<MapRoute::AreaRun>{{areas.west, 8}, {areas.east, 1}}));

        // Without it, by way of the middle
        QVERIFY(map()->setExit(areas.westFirst + 24, -1, DIR_NORTHEAST));
        widget.refreshRoute();
        QCOMPARE(widget.route().endRoomId(), areas.eastFirst + 10);
        QCOMPARE(widget.route().steps(), 12);

        // Already in the area: nothing to show, and the user is told so
        QVERIFY(!widget.planRouteToArea(areas.west).isEmpty());
        QVERIFY(!widget.route().hasDestination());
    }

    void test_theRouteIsFoundAgainOnlyWhenThePlayerMovesOrTheMapChangesWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        ModernGLWidget widget(map(), mpHost);
        const int destination = areas.eastFirst + 24;
        widget.planRouteToRoom(destination);
        QCOMPARE(widget.route().steps(), 18);

        // Frames that change nothing leave it alone
        widget.mRouteRefreshTimer->stop();
        queueFrame(widget, areas.west, 1.5f);
        queueFrame(widget, areas.west, 1.5f);
        QVERIFY2(!widget.mRouteRefreshTimer->isActive(), "a frame would find the route again though nothing changed");
        QVERIFY(!widget.mRoute.refresh(*map()));

        // A move: the next frame has it found again, outside painting
        map()->mRoomIdHash[map()->mProfileName] = areas.westFirst + 14;
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        QVERIFY(widget.mRouteRefreshTimer->isActive());
        QCOMPARE(widget.route().rooms().first(), areas.westFirst);
        widget.mRouteRefreshTimer->stop();
        widget.refreshRoute();
        QCOMPARE(widget.route().rooms().first(), areas.westFirst + 14);
        QCOMPARE(widget.route().steps(), 12);

        // A change to the map
        map()->setUnsaved(__func__);
        QVERIFY(widget.route().stale(*map()));
        widget.refreshRoute();
        QVERIFY(!widget.route().stale(*map()));

        // Arriving ends it
        map()->mRoomIdHash[map()->mProfileName] = destination;
        widget.refreshRoute();
        QVERIFY(!widget.route().hasDestination());
        QCOMPARE(widget.route().status(), MapRoute::Status::NoDestination);

        // As does a new map, whose room IDs are not the old one's
        map()->mRoomIdHash[map()->mProfileName] = areas.westFirst;
        widget.planRouteToRoom(destination);
        QVERIFY(widget.route().hasDestination());
        addThreeAreas();
        widget.refreshRoute();
        QVERIFY(!widget.route().hasDestination());
    }

    void test_editsToTheMapWaitBeforeTheRouteRebuildsThePathGraphWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        ModernGLWidget widget(map(), mpHost);
        widget.planRouteToRoom(areas.eastFirst + 24);
        QVERIFY(widget.mRouteGraphBuilt.isValid());
        QVERIFY(!map()->mMapGraphNeedsUpdate);
        widget.mRouteRefreshTimer->stop();
        const auto queueScene = [&widget]() {
            widget.updateMatrices();
            widget.mRenderCommandQueue.clear();
            widget.queueScene();
        };

        // A move alone is found again straight away
        map()->mRoomIdHash[map()->mProfileName] = areas.westFirst + 1;
        queueScene();
        QVERIFY(widget.mRouteRefreshTimer->isActive());
        QCOMPARE(widget.mRouteRefreshTimer->interval(), 0);
        widget.mRouteRefreshTimer->stop();
        widget.refreshRoute();

        // An exit added, as a mapping script does on each move, waits at least a second after the graph was built
        map()->setExit(areas.westFirst + 1, areas.westFirst + 7, DIR_NORTHEAST);
        queueScene();
        QVERIFY(widget.mRouteRefreshTimer->isActive());
        QVERIFY2(widget.mRouteRefreshTimer->interval() > 500, "an edit to the exits rebuilt the path graph straight away");
        const int remaining = widget.mRouteRefreshTimer->remainingTime();

        // Further edits leave that wait as it is, rather than putting the route off for as long as they come
        map()->setExit(areas.westFirst + 7, areas.westFirst + 1, DIR_SOUTHWEST);
        queueScene();
        QVERIFY(widget.mRouteRefreshTimer->remainingTime() <= remaining);

        // Built long enough ago, the graph is built again straight away
        widget.mRouteRefreshTimer->stop();
        widget.mRouteGraphBuilt.invalidate();
        queueScene();
        QCOMPARE(widget.mRouteRefreshTimer->interval(), 0);
        widget.mRouteRefreshTimer->stop();
        widget.refreshRoute();
        QVERIFY(!map()->mMapGraphNeedsUpdate);
        QVERIFY(widget.mRouteGraphBuilt.isValid());
        QCOMPARE(widget.route().status(), MapRoute::Status::Found);
    }

    void test_aRouteIsDroppedWhenAMapIsImportedWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString file = directory.filePath(qsl("map.json"));
        QVERIFY(map()->writeJsonMapFile(file).first);
        // Imported twice, as a map only ever imported over has the same generation as the map it replaces
        QVERIFY(map()->readJsonMapFile(file).first);
        ModernGLWidget widget(map(), mpHost);
        QVERIFY(widget.planRouteToRoom(areas.eastFirst).isEmpty());
        QCOMPARE(widget.route().status(), MapRoute::Status::Found);
        QVERIFY(map()->readJsonMapFile(file).first);
        widget.refreshRoute();
        QVERIFY(!widget.route().hasDestination());
    }

    void test_aRouteToAnAreaWeighsEntrancesBeyondTheRangeOfAnIntWithoutGL()
    {
        map()->mapClear();
        map()->mUserData.remove(TAreaGraph::cLayoutUserDataKey);
        addGridArea(qsl("Westmarch"), 1, 3, 1, 1, 0);
        addGridArea(qsl("Highvale"), 10, 2, 1, 1, 0);
        QVERIFY(map()->setExit(10, -1, DIR_EAST));
        QVERIFY(map()->setExit(11, -1, DIR_WEST));
        // From the middle room: east through two rooms each weighted two billion, or west through two of weight one
        QVERIFY(map()->setExit(3, 10, DIR_NORTH));
        QVERIFY(map()->setExit(1, 11, DIR_NORTH));
        map()->mpRoomDB->getRoom(3)->setWeight(2000000000);
        map()->mpRoomDB->getRoom(10)->setWeight(2000000000);
        map()->mRoomIdHash[map()->mProfileName] = 2;
        ModernGLWidget widget(map(), mpHost);
        QCOMPARE(widget.planRouteToArea(map()->mpRoomDB->getRoom(10)->getArea()), QString());
        QCOMPARE(widget.route().endRoomId(), 11);
        QCOMPARE(widget.route().totalWeight(), 2);
    }

    void test_aPlaceThatCannotBeReachedIsReportedWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        const int islandArea = map()->mpRoomDB->addArea(qsl("Island"));
        const int island = 900;
        map()->addRoom(island);
        map()->setRoomArea(island, islandArea);
        map()->setRoomCoordinates(island, 0, 0, 0);
        ModernGLWidget widget(map(), mpHost);
        QString message = widget.planRouteToRoom(island);
        QVERIFY2(message.contains(qsl("room 900")), qPrintable(message));
        QCOMPARE(widget.route().status(), MapRoute::Status::Unreachable);
        message = widget.planRouteToArea(islandArea);
        QVERIFY2(message.contains(qsl("Island")), qPrintable(message));
        queueFrame(widget, areas.west, 1.5f);
        QVERIFY(drawnRoute(widget).isEmpty());
        queueFrame(widget, areas.west, 800.0f);
        const qsizetype layers = atlasLayerOpacities(widget).size();
        widget.clearRoute();
        queueFrame(widget, areas.west, 800.0f);
        QCOMPARE(atlasLayerOpacities(widget).size(), layers);

        QVERIFY(!widget.planRouteToRoom(424242).isEmpty());
        QVERIFY(!widget.route().hasDestination());
        QVERIFY(widget.planRouteToRoom(areas.eastFirst + 1).isEmpty());
        map()->mpRoomDB->removeRoom(areas.eastFirst + 1);
        widget.refreshRoute();
        QVERIFY(!widget.route().hasDestination());
        map()->mRoomIdHash[map()->mProfileName] = 0;
        QVERIFY(!widget.planRouteToRoom(areas.eastFirst).isEmpty());
        QVERIFY(!widget.route().hasDestination());
    }

    void test_theRouteIsDrawnAtEveryZoomOnlyWhileThereIsOneWithoutGL()
    {
        // Large enough for the footprints to show before the world graph takes over, and far enough apart for every name to
        const int side = 41;
        const ThreeAreas areas = addThreeAreas(side);
        map()->areaGraph()->refresh();
        QVERIFY(map()->areaGraph()->setPosition(areas.west, QPointF(0.0, 0.0)));
        QVERIFY(map()->areaGraph()->setPosition(areas.middle, QPointF(0.0, 400.0)));
        QVERIFY(map()->areaGraph()->setPosition(areas.east, QPointF(0.0, 800.0)));
        ModernGLWidget widget(map(), mpHost);
        const auto labelTexts = [&widget]() {
            QStringList texts;
            for (const ModernGLWidget::AtlasLabel& label : widget.atlasLabels(widget.mZoomWeights)) {
                texts.append(label.text);
            }
            return texts;
        };
        // The layers a route adds: one over the footprints and one along the world graph, for each that shows
        const auto routeLayers = [&widget]() {
            const MapAtlas::Weights& weights = widget.mZoomWeights;
            return (weights.footprint * (1.0f - weights.world) > 0.0f ? 1 : 0) + (weights.footprint * weights.world > 0.0f ? 1 : 0);
        };
        const QList<float> distances{160.0f, 200.0f, 800.0f};
        QList<qsizetype> layersWithout;
        queueFrame(widget, areas.west, 1.5f);
        QVERIFY(drawnRoute(widget).isEmpty());
        const qsizetype roomsWithout = drawnRooms(widget).size();
        for (const float distance : distances) {
            queueFrame(widget, areas.west, distance);
            layersWithout.append(atlasLayerOpacities(widget).size());
            QCOMPARE(labelTexts().size(), 3);
            for (const QString& text : labelTexts()) {
                QVERIFY2(!text.contains(qsl("step")), qPrintable(text));
            }
        }

        widget.planRouteToRoom(areas.eastFirst + side * side - 1);
        QCOMPARE(widget.route().status(), MapRoute::Status::Found);
        queueFrame(widget, areas.west, 1.5f);
        const QList<DrawnCube> bars = drawnRoute(widget);
        QVERIFY(!bars.isEmpty());
        const QColor routeColor = mpHost->mLightYellow_2;
        QVERIFY(std::any_of(bars.cbegin(), bars.cend(), [&](const DrawnCube& bar) {
            return bar.color == tintOf(routeColor);
        }));
        // Only on the rooms on the way that are drawn: the west area's, from its southwest corner to the way out in
        // the middle of its east side, then the middle area's rooms drawn beside it, along its middle row
        const float edge = side / 2 + 0.7f;
        for (const DrawnCube& bar : bars) {
            const bool inWest = bar.center.x() < edge && bar.center.y() > -edge && bar.center.y() < 0.7f;
            const bool inMiddle = std::abs(bar.center.y()) < 0.7f && std::any_of(widget.mStitch.rooms().cbegin(), widget.mStitch.rooms().cend(), [&](const MapStitch::Room& room) {
                                      return std::abs(room.at.x() - bar.center.x()) <= 0.6f && std::abs(room.at.y() - bar.center.y()) <= 0.6f;
                                  });
            QVERIFY2(inWest || inMiddle, qPrintable(qsl("a bar at %1, %2").arg(bar.center.x()).arg(bar.center.y())));
        }
        QVERIFY(std::any_of(bars.cbegin(), bars.cend(), [&](const DrawnCube& bar) {
            return bar.center.x() > edge;
        }));
        QCOMPARE(drawnRooms(widget).size(), roomsWithout);

        bool sawFootprints = false;
        bool sawWorld = false;
        for (qsizetype i = 0; i < distances.size(); ++i) {
            queueFrame(widget, areas.west, distances.at(i));
            QVERIFY(routeLayers() > 0);
            sawFootprints = sawFootprints || widget.mZoomWeights.world < 1.0f;
            sawWorld = sawWorld || widget.mZoomWeights.world > 0.0f;
            QCOMPARE(atlasLayerOpacities(widget).size(), layersWithout.at(i) + routeLayers());
            const QString steps = qsl("Midlands (%1 step").arg(side);
            QVERIFY2(labelTexts().filter(steps).size() == 1, qPrintable(labelTexts().join(qsl(", "))));
        }
        QVERIFY(sawFootprints && sawWorld);

        widget.clearRoute();
        queueFrame(widget, areas.west, 1.5f);
        QVERIFY(drawnRoute(widget).isEmpty());
        for (qsizetype i = 0; i < distances.size(); ++i) {
            queueFrame(widget, areas.west, distances.at(i));
            QCOMPARE(atlasLayerOpacities(widget).size(), layersWithout.at(i));
        }
    }

    void test_aNeighbouringAreaIsPlacedWhereTheExitsBetweenThemSayWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        MapStitch stitch;
        stitch.build(*map(), areas.west, 0, [](int) {
            return QColor(Qt::gray);
        });
        QCOMPARE(stitch.spacing(), 1);
        // The east area is joined only to the middle one, so is not beside the west one
        QCOMPARE(stitch.neighbours().size(), 1);
        const MapStitch::Neighbour* pMiddle = stitch.neighbour(areas.middle);
        QVERIFY(pMiddle);
        QCOMPARE(pMiddle->placement, MapStitch::Placement::Stitched);
        // The way east from the middle of its east side, at 2,0, comes into the middle area at -2,0
        QCOMPARE(pMiddle->offset, (MapStitch::Offset{5, 0, 0}));
        QCOMPARE(pMiddle->links, 2);
        QCOMPARE(pMiddle->agreeing, 2);
        QCOMPARE(pMiddle->collisions, 0);
        QCOMPARE(stitch.rooms().size(), 25);
        QVERIFY(stitch.portals().isEmpty());
        const MapStitch::Room* pWayIn = stitch.room(areas.middleFirst + 10);
        QVERIFY(pWayIn);
        QCOMPARE(pWayIn->at, QVector3D(3.0f, 0.0f, 0.0f));

        mpHost->mMapperStitchAreas = true;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areas.west, 2.0f);
        const QList<DrawnCube> rooms = drawnRooms(widget);
        QCOMPARE(rooms.size(), 50);
        // The middle area's arrow back west is left to the west area's own arrow east, drawn over the same gap
        const QVector<CubeInstanceData>& stitchedBars = widget.mStitchScene.instances(RetainedMapScene::Layer::ExitBars)->instances;
        QVERIFY(!stitchedBars.isEmpty());
        for (const CubeInstanceData& bar : stitchedBars) {
            QVERIFY2(bar.transform.column(3).x() > 2.9f, qPrintable(qsl("an exit bar at x %1").arg(bar.transform.column(3).x())));
        }
        // Muted toward the background, so the area shown stands out from those around it
        const QVector4D plain = tintOf(RoomAppearance::environmentColor(*map(), *mpHost, map()->mpRoomDB->getRoom(areas.middleFirst + 10)->environment));
        bool mutedWayIn = false;
        for (const DrawnCube& room : rooms) {
            if (qFuzzyCompare(room.center.x(), 3.0f) && qFuzzyIsNull(room.center.y())) {
                mutedWayIn = room.color != plain && room.color.toVector3D().length() < plain.toVector3D().length();
            }
        }
        QVERIFY2(mutedWayIn, "the middle area's way in is not drawn beside the west area, muted");

        // A route goes on into the rooms drawn beside the area
        QVERIFY(widget.planRouteToRoom(areas.middleFirst + 12).isEmpty());
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        const QList<DrawnCube> route = drawnRoute(widget);
        QVERIFY(std::any_of(route.cbegin(), route.cend(), [](const DrawnCube& bar) {
            return bar.center.x() > 3.5f;
        }));
        widget.clearRoute();
    }

    void test_linksThatDisagreeAreSettledTheSameWayEachTimeWithoutGL()
    {
        // Most votes win; a tie goes to the smallest move, then the lowest x
        QCOMPARE(MapStitch::chooseOffset({{5, 0, 0}, {5, -3, 0}, {5, 0, 0}}), (MapStitch::Offset{5, 0, 0}));
        QCOMPARE(MapStitch::chooseOffset({{5, -3, 0}, {5, 0, 0}}), (MapStitch::Offset{5, 0, 0}));
        QCOMPARE(MapStitch::chooseOffset({{3, 0, 0}, {0, 3, 0}}), (MapStitch::Offset{0, 3, 0}));
        QCOMPARE(MapStitch::chooseOffset({{0, 3, 0}, {3, 0, 0}}), (MapStitch::Offset{0, 3, 0}));
        int agreeing = 0;
        MapStitch::chooseOffset({{1, 1, 0}, {2, 2, 0}, {2, 2, 0}}, &agreeing);
        QCOMPARE(agreeing, 2);

        const ThreeAreas areas = addThreeAreas();
        // A third way east, from the west area's southeast corner, that puts the middle area three rooms further south
        map()->setExit(areas.westFirst + 4, areas.middleFirst + 15, DIR_EAST);
        MapStitch stitch;
        stitch.build(*map(), areas.west, 0, [](int) {
            return QColor(Qt::gray);
        });
        const MapStitch::Neighbour* pMiddle = stitch.neighbour(areas.middle);
        QVERIFY(pMiddle);
        QCOMPARE(pMiddle->offset, (MapStitch::Offset{5, 0, 0}));
        QCOMPARE(pMiddle->links, 3);
        QCOMPARE(pMiddle->agreeing, 2);
        // The exit the area is not placed by leads nowhere near where it is drawn, so it shows a preview
        QCOMPARE(stitch.portals().size(), 1);
        QCOMPARE(stitch.portals().first().fromRoomId, areas.westFirst + 4);
        QCOMPARE(stitch.portals().first().toAreaId, areas.middle);
    }

    void test_aNeighbourThatWouldLieOverTheAreaIsNotPlacedWithoutGL()
    {
        // The way east from the west area's corner comes into the east area's same corner, one room on
        const auto [westId, eastId] = addTwoAreas(20);
        MapStitch stitch;
        stitch.build(*map(), westId, 0, [](int) {
            return QColor(Qt::gray);
        });
        const MapStitch::Neighbour* pEast = stitch.neighbour(eastId);
        QVERIFY(pEast);
        QCOMPARE(pEast->offset, (MapStitch::Offset{1, 0, 0}));
        QCOMPARE(pEast->collisions, 19 * 20);
        QCOMPARE(pEast->placement, MapStitch::Placement::Overlaps);
        QVERIFY(stitch.rooms().isEmpty());
        QCOMPARE(stitch.portals().size(), 1);
        const MapStitch::Portal& portal = stitch.portals().first();
        QCOMPARE(portal.fromRoomId, 1);
        QCOMPARE(portal.direction, DIR_EAST);
        QCOMPARE(portal.toAreaId, eastId);
        // The rooms within three of the way in, nearest first
        QCOMPARE(portal.preview.size(), 16);
        QVERIFY(portal.preview.first().target);
        QCOMPARE(portal.preview.first().at, QPointF());
    }

    void test_onlyTheNeighboursRoomsNearTheAreaAreKeptWithoutGL()
    {
        map()->mapClear();
        map()->mUserData.remove(TAreaGraph::cLayoutUserDataKey);
        const int longFirst = addGridArea(qsl("Westmarch"), 1, 5, 5, 1, 0);
        addGridArea(qsl("Long Road"), longFirst, 100, 1, 1, 0);
        // East from 2,0 into the road's west end at -50,0, so the road runs out to 102,0
        map()->setExit(1 + 14, longFirst, DIR_EAST);
        const int westId = map()->mpRoomDB->getRoom(1)->getArea();
        const int roadId = map()->mpRoomDB->getRoom(longFirst)->getArea();
        MapStitch stitch;
        stitch.build(*map(), westId, 0, [](int) {
            return QColor(Qt::gray);
        });
        QCOMPARE(stitch.neighbour(roadId)->offset, (MapStitch::Offset{53, 0, 0}));
        // Within the squares of ten rooms holding the area's rooms and those around them: up to x 19
        QCOMPARE(stitch.rooms().size(), 17);
        for (const MapStitch::Room& room : stitch.rooms()) {
            QVERIFY(room.at.x() >= 3.0f && room.at.x() < 20.0f);
        }

        // Nor do the road's exits run on past its last room drawn
        mpHost->mMapperStitchAreas = true;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, westId, 2.0f);
        const QVector<CubeInstanceData>& bars = widget.mStitchScene.instances(RetainedMapScene::Layer::ExitBars)->instances;
        QVERIFY(!bars.isEmpty());
        for (const CubeInstanceData& bar : bars) {
            const float x = bar.transform.column(3).x();
            QVERIFY2(x > 3.0f && x < 19.0f, qPrintable(qsl("an exit bar at x %1").arg(x)));
        }
    }

    void test_aNeighboursExitOverAnEmptySquareIsDrawnWholeWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        // A one-way exit from the middle area's middle room over the empty square at 1,0, drawn at 6,0
        map()->mpRoomDB->removeRoom(areas.middleFirst + 13);
        map()->setExit(areas.middleFirst + 12, areas.middleFirst + 14, DIR_EAST);
        mpHost->mMapperStitchAreas = true;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areas.west, 2.0f);
        const auto overTheGap = [](const float x, const float y) {
            return x > 5.5f && x < 6.5f && std::abs(y) < 0.5f;
        };
        const QVector<CubeInstanceData>& bars = widget.mStitchScene.instances(RetainedMapScene::Layer::ExitBars)->instances;
        QVERIFY2(std::any_of(bars.cbegin(),
                             bars.cend(),
                             [&](const CubeInstanceData& bar) {
                                 return overTheGap(bar.transform.column(3).x(), bar.transform.column(3).y());
                             }),
                 "the exit's dots over the empty square are not drawn");
        QVector<float> vertices;
        QVector<float> colors;
        widget.mStitchScene.appendTriangles(false, std::nullopt, vertices, colors);
        bool arrow = false;
        for (qsizetype i = 0; i + 2 < vertices.size(); i += 3) {
            arrow = arrow || overTheGap(vertices.at(i), vertices.at(i + 1));
        }
        QVERIFY2(arrow, "the exit's one-way arrow over the empty square is not drawn");
    }

    void test_aClickOnANeighboursRoomGoesIntoItsAreaWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        mpHost->mMapperStitchAreas = true;
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, areas.west, 2.0f);
        const int wayIn = areas.middleFirst + 10;
        const QPoint pixel = pixelOf(widget, QVector3D(3.0f, 0.0f, 0.0f));
        QCOMPARE(widget.stitchedRoomAt(pixel), wayIn);
        QCOMPARE(widget.roomPickedAt(pixel, true, false, true), wayIn);
        // Where it was always picked, the area's own rooms win
        QCOMPARE(widget.roomPickedAt(pixel), 0);
        QCOMPARE(widget.roomPickedAt(pixelOf(widget, QVector3D(2.0f, 0.0f, 0.0f)), true, false, true), areas.westFirst + 14);
        QCOMPARE(widget.destinationAt(pixel), wayIn);

        const QVector3D before = widget.mCameraController.getTarget();
        const float scaleBefore = widget.mCameraController.getScale();
        clickAt(widget, pixel);
        QCOMPARE(widget.mAID, areas.middle);
        QVERIFY(widget.mFlight);
        // The same place, in the middle area's coordinates, so the view does not jump
        QCOMPARE(widget.mCameraController.getTarget(), before - QVector3D(5.0f, 0.0f, 0.0f));
        finishFlight(widget);
        QCOMPARE(widget.mCameraController.getTarget(), QVector3D(-2.0f, 0.0f, 0.0f));
        QCOMPARE(QPoint(widget.mMapCenterX, widget.mMapCenterY), QPoint(-2, 0));
        QCOMPARE(widget.mCameraController.getScale(), scaleBefore);
    }

    void test_anExitIntoAnAreaThatCannotBePlacedShowsAPreviewWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas(20);
        mpHost->mMapperStitchAreas = true;
        ModernGLWidget widget(map(), mpHost);
        queueFrameAt(widget, westId, 2.0f, QPoint(-8, -8));
        QVERIFY(widget.roomPixels() >= 10.0f);
        const QList<ModernGLWidget::PortalCard> cards = widget.portalCards();
        QCOMPARE(cards.size(), 1);
        const ModernGLWidget::PortalCard& card = cards.first();
        QCOMPARE(card.name, qsl("Eastvale"));
        // Out along the exit, east of where its arrow ends
        const float arrow = mpHost->getLargeAreaExitArrows() ? 2.0f : 1.0f;
        const QPoint tip = pixelOf(widget, QVector3D(-10.0f + arrow, -10.0f, 0.0f));
        QCOMPARE(card.anchor.toPoint(), tip);
        QVERIFY(card.rect.left() > tip.x());
        QCOMPARE(widget.portalAt(card.rect.center().toPoint()), card.portal);
        QCOMPARE(widget.portalAt(QPoint(card.rect.left() - 5, card.rect.top() - 5)), -1);
        QVERIFY(widget.portalToolTip(card.portal).contains(qsl("Eastvale")));
        // Pointing at it does not raise the room under it
        widget.mPointerInView = true;
        widget.mPointerPosition = card.rect.center().toPoint();
        QCOMPARE(widget.roomUnderPointer(), 0);
        widget.mPointerInView = false;

        // Not drawn while the rooms are too small to read it by
        queueFrameAt(widget, westId, 60.0f, QPoint(-8, -8));
        QVERIFY(widget.portalCards().isEmpty());

        queueFrameAt(widget, westId, 2.0f, QPoint(-8, -8));
        const int wayIn = widget.mStitch.portals().at(card.portal).toRoomId;
        const float scaleBefore = widget.mCameraController.getScale();
        clickAt(widget, card.rect.center().toPoint());
        QCOMPARE(widget.mAID, eastId);
        QVERIFY(widget.mFlight);
        finishFlight(widget);
        const TRoom* pWayIn = map()->mpRoomDB->getRoom(wayIn);
        QCOMPARE(widget.mCameraController.getTarget(), QVector3D(pWayIn->x(), pWayIn->y(), 0.0f));
        QCOMPARE(widget.mCameraController.getScale(), scaleBefore);
    }

    void test_stitchingTurnedOffLeavesTheViewAsItWasWithoutGL()
    {
        const ThreeAreas areas = addThreeAreas();
        ModernGLWidget widget(map(), mpHost);
        mpHost->mMapperStitchAreas = false;
        queueFrame(widget, areas.west, 2.0f);
        QCOMPARE(drawnRooms(widget).size(), 25);
        QCOMPARE(widget.stitchedRoomAt(pixelOf(widget, QVector3D(3.0f, 0.0f, 0.0f))), 0);
        const qsizetype commandsOff = widget.mRenderCommandQueue.commands().size();

        mpHost->mMapperStitchAreas = true;
        queueFrame(widget, areas.west, 2.0f);
        QCOMPARE(drawnRooms(widget).size(), 50);
        QVERIFY(widget.mRenderCommandQueue.commands().size() > commandsOff);

        // Nor does an area with no exits to others change for it
        map()->mapClear();
        addGridArea(qsl("Island"), 1, 5, 5, 1, 0);
        const int islandId = map()->mpRoomDB->getRoom(1)->getArea();
        queueFrame(widget, islandId, 2.0f);
        const qsizetype commandsOn = widget.mRenderCommandQueue.commands().size();
        mpHost->mMapperStitchAreas = false;
        queueFrame(widget, islandId, 2.0f);
        QCOMPARE(widget.mRenderCommandQueue.commands().size(), commandsOn);

        // And no previews either
        addTwoAreas(20);
        queueFrameAt(widget, map()->mpRoomDB->getRoom(1)->getArea(), 2.0f, QPoint(-8, -8));
        QVERIFY(widget.portalCards().isEmpty());
        mpHost->mMapperStitchAreas = true;
    }

    void test_theSearchRanksExactThenPrefixThenSubstringWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas(4);
        const int eastFirst = 1 + 4 * 4;
        TRoomDB& roomDB = *map()->mpRoomDB;
        roomDB.getRoom(1)->name = qsl("Old Bakery");
        roomDB.getRoom(2)->name = qsl("Bakery");
        roomDB.getRoom(3)->name = qsl("Bakery Lane");
        roomDB.getRoom(eastFirst)->name = qsl("Bakery");
        roomDB.getRoom(eastFirst + 1)->name = qsl("Bakery Lane");
        roomDB.getRoom(eastFirst + 2)->name = qsl("Old Bakery");
        map()->setUnsaved(__func__);
        MapSearch search;
        QVERIFY(search.refresh(*map()));
        QVERIFY(!search.refresh(*map()));

        // Exact, then prefix, then substring, ignoring case; among those as good, the area the view shows first
        QList<MapSearch::Result> found = search.find(qsl("bAKERY"), eastId, true);
        QCOMPARE(foundNames(found), (QStringList{qsl("Bakery"), qsl("Bakery"), qsl("Bakery Lane"), qsl("Bakery Lane"), qsl("Old Bakery"), qsl("Old Bakery")}));
        QCOMPARE(found.at(0).id, eastFirst);
        QCOMPARE(found.at(0).areaId, eastId);
        QCOMPARE(found.at(0).match, MapSearch::Match::Exact);
        QCOMPARE(found.at(1).id, 2);
        QCOMPARE(found.at(2).id, eastFirst + 1);
        QCOMPARE(found.at(2).match, MapSearch::Match::Prefix);
        QCOMPARE(found.at(4).id, eastFirst + 2);
        QCOMPARE(found.at(4).match, MapSearch::Match::Substring);
        found = search.find(qsl("bakery"), westId, true);
        QCOMPARE(found.at(0).id, 2);
        QCOMPARE(found.at(1).id, eastFirst);
        QCOMPARE(found.at(4).id, 1);

        // Areas by name, ahead of rooms as good a match in the same area
        roomDB.getRoom(4)->name = qsl("Westmarch Gate");
        map()->setUnsaved(__func__);
        search.refresh(*map());
        found = search.find(qsl("westmarch"), eastId, true);
        QCOMPARE(found.size(), 2);
        QCOMPARE(found.at(0).kind, MapSearch::Kind::Area);
        QCOMPARE(found.at(0).id, westId);
        QCOMPARE(found.at(1).id, 4);

        // A room by its ID, named or not, comes first; others with the digits in their names follow
        roomDB.getRoom(5)->name = qsl("Room 12 annex");
        map()->setUnsaved(__func__);
        search.refresh(*map());
        found = search.find(qsl("12"), westId, true);
        QCOMPARE(found.size(), 2);
        QCOMPARE(found.at(0).kind, MapSearch::Kind::Room);
        QCOMPARE(found.at(0).id, 12);
        QCOMPARE(found.at(0).match, MapSearch::Match::Exact);
        QCOMPARE(found.at(1).id, 5);

        // Labels only when asked for, and at most as many as the limit, with how many there were in all
        TMapLabel label;
        label.text = qsl("Bakery district");
        label.pos = QVector3D(1.0f, 2.0f, 0.0f);
        label.size = QSizeF(4.0, 2.0);
        TArea* pEast = roomDB.getArea(eastId);
        const int labelId = pEast->createLabelId();
        pEast->mMapLabels.insert(labelId, label);
        map()->setUnsaved(__func__);
        search.refresh(*map());
        QCOMPARE(search.find(qsl("bakery"), westId, false).size(), 6);
        found = search.find(qsl("bakery"), westId, true);
        QCOMPARE(found.size(), 7);
        QCOMPARE(found.at(4).kind, MapSearch::Kind::Label);
        QCOMPARE(found.at(4).id, labelId);
        qsizetype total = 0;
        QCOMPARE(search.find(qsl("bakery"), westId, true, 3, &total).size(), 3);
        QCOMPARE(total, 7);
        QVERIFY(search.find(qsl("   "), westId, true).isEmpty());
        QVERIFY(search.find(qsl("nowhere"), westId, true).isEmpty());
    }

    void test_theSearchIndexIsMadeAgainOnceTheMapChangesWithoutGL()
    {
        addTwoAreas(4);
        MapSearch search;
        search.refresh(*map());
        QVERIFY(search.find(qsl("Cellar"), 0, true).isEmpty());

        TRoom* pRoom = map()->mpRoomDB->getRoom(7);
        pRoom->name = qsl("Cellar");
        map()->setUnsaved(__func__);
        QVERIFY(search.stale(*map()));
        QVERIFY(search.refresh(*map()));
        QCOMPARE(search.find(qsl("Cellar"), 0, true).value(0).id, 7);

        map()->mpRoomDB->removeRoom(7);
        map()->setUnsaved(__func__);
        search.refresh(*map());
        QVERIFY(search.find(qsl("Cellar"), 0, true).isEmpty());

        // Hidden rooms are not given away
        map()->mpRoomDB->getRoom(9)->name = qsl("Secret Cellar");
        map()->mpRoomDB->getRoom(9)->setHidden(true);
        search.refresh(*map());
        QVERIFY(search.find(qsl("Cellar"), 0, true).isEmpty());
        QVERIFY(search.find(qsl("9"), 0, true).isEmpty());
        map()->mpRoomDB->getRoom(9)->setHidden(false);
        search.refresh(*map());
        QCOMPARE(search.find(qsl("Cellar"), 0, true).value(0).id, 9);
        // Nor an area all of whose rooms are hidden, which would be flown to only to show nothing
        const int hiddenAreaId = map()->mpRoomDB->getRoom(1)->getArea();
        const QString hiddenAreaName = map()->mpRoomDB->getAreaNamesMap().value(hiddenAreaId);
        QVERIFY(!search.find(hiddenAreaName, 0, true).isEmpty());
        for (const int roomId : map()->mpRoomDB->getArea(hiddenAreaId)->getAreaRooms()) {
            map()->mpRoomDB->getRoom(roomId)->setHidden(true);
        }
        search.refresh(*map());
        QVERIFY(search.find(hiddenAreaName, 0, true).isEmpty());
        for (const int roomId : map()->mpRoomDB->getArea(hiddenAreaId)->getAreaRooms()) {
            map()->mpRoomDB->getRoom(roomId)->setHidden(false);
        }
        map()->mpRoomDB->getRoom(9)->name.clear();
        map()->setUnsaved(__func__);

        // Spaces count once, wherever they are
        map()->mpRoomDB->getRoom(8)->name = qsl(" Wine  Cellar ");
        map()->setUnsaved(__func__);
        search.refresh(*map());
        QCOMPARE(search.find(qsl("wine cellar"), 0, true).value(0).match, MapSearch::Match::Exact);
        QCOMPARE(search.find(qsl("wine cellar"), 0, true).value(0).name, qsl("Wine Cellar"));
        map()->mpRoomDB->getRoom(8)->name.clear();
        map()->setUnsaved(__func__);
        search.refresh(*map());

        // Not made again for what changes only how the map looks, which scripts do all the time
        map()->updateArea(map()->mpRoomDB->getRoom(1)->getArea());
        QVERIFY(!search.stale(*map()));

        // A label made for this session only, which bumps only how the area looks, as does taking it away
        const int areaId = map()->mpRoomDB->getRoom(1)->getArea();
        TArea* pArea = map()->mpRoomDB->getArea(areaId);
        TMapLabel label;
        label.text = qsl("Cellar door");
        label.temporary = true;
        const int labelId = pArea->createLabelId();
        pArea->mMapLabels.insert(labelId, label);
        map()->updateArea(areaId);
        search.refresh(*map());
        QCOMPARE(search.find(qsl("Cellar"), 0, true).size(), 1);
        map()->deleteMapLabel(areaId, labelId);
        map()->updateArea(areaId);
        search.refresh(*map());
        QVERIFY(search.find(qsl("Cellar"), 0, true).isEmpty());

        // Another map read in starts again, even with the same revisions
        addTwoAreas(4);
        map()->mpRoomDB->getRoom(3)->name = qsl("Cellar");
        search.refresh(*map());
        QCOMPARE(search.find(qsl("Cellar"), 0, true).value(0).id, 3);
    }

    void test_aPlaceFoundIsFlownToAndMarkedWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas(6);
        const int target = 1 + 6 * 6 + 14;
        map()->mpRoomDB->getRoom(target)->name = qsl("Lighthouse");
        map()->mpRoomDB->getRoom(3)->name = qsl("Lighthouse Keeper's Cottage");
        map()->setUnsaved(__func__);
        ModernGLWidget widget(map(), mpHost);
        QSignalSpy flownInto(&widget, &ModernGLWidget::areaFlownInto);
        queueFrame(widget, westId, 1.5f);
        QVERIFY(!widget.mpSearchPanel);

        // Ctrl+F on the view, with its keys its own, opens the search
        widget.mKeysNavigate = true;
        QKeyEvent find(QEvent::KeyPress, Qt::Key_F, Qt::ControlModifier);
        widget.keyPressEvent(&find);
        QVERIFY(widget.mpSearchPanel);
        QVERIFY(!widget.mpSearchPanel->isHidden());
        widget.mpSearchPanel->mpEdit->setText(qsl("lighthouse"));
        QCOMPARE(widget.mSearchResults.size(), 2);
        QCOMPARE(widget.mSearchResults.at(0).id, target);
        QCOMPARE(widget.mpSearchPanel->mpList->count(), 2);
        const QListWidgetItem* pFirst = widget.mpSearchPanel->mpList->item(0);
        QCOMPARE(pFirst->text(), qsl("Lighthouse"));
        QCOMPARE(pFirst->data(MapSearchPanel::DetailRole).toString(), qsl("Eastvale · room %1").arg(target));
        QCOMPARE(pFirst->data(Qt::AccessibleTextRole).toString(), qsl("Lighthouse in Eastvale (room %1)").arg(target));
        QCOMPARE(pFirst->data(MapSearchPanel::ChipColorRole).value<QColor>(), RoomAppearance::environmentColor(*map(), *mpHost, map()->mpRoomDB->getRoom(target)->environment));
        QCOMPARE(widget.mpSearchPanel->currentRow(), 0);

        // Keys the box leaves, and the mouse over the panel, stay with it rather than reaching the map under it
        EventCounter keysToView(&widget, QEvent::KeyPress);
        QKeyEvent shift(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier);
        QCoreApplication::sendEvent(widget.mpSearchPanel->mpEdit, &shift);
        QCOMPARE(keysToView.count, 0);
        // Laid out, as it would be were the view shown
        widget.mpSearchPanel->layout()->activate();
        EventCounter pressesToView(&widget, QEvent::MouseButtonPress);
        // In the card's padding, by the search box
        const QPointF onCard(MapSearchPanel::scmShadowMargin + 4.0, widget.mpSearchPanel->mpEdit->geometry().center().y());
        QMouseEvent press(QEvent::MouseButtonPress, onCard, widget.mpSearchPanel->mapToGlobal(onCard), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(widget.mpSearchPanel, &press);
        QCOMPARE(pressesToView.count, 0);
        EventCounter tooltipsToView(&widget, QEvent::ToolTip);
        QHelpEvent tooltip(QEvent::ToolTip, QPoint(2, 2), widget.mapToGlobal(QPoint(2, 2)));
        QCoreApplication::sendEvent(widget.mpSearchPanel->mpList->viewport(), &tooltip);
        QCOMPARE(tooltipsToView.count, 0);
        // The shadow around the card is the map's
        QMouseEvent pressOnShadow(QEvent::MouseButtonPress, QPointF(2.0, 2.0), widget.mpSearchPanel->mapToGlobal(QPointF(2.0, 2.0)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(widget.mpSearchPanel, &pressOnShadow);
        QCOMPARE(pressesToView.count, 1);
        QMouseEvent releaseOnCard(QEvent::MouseButtonRelease, onCard, widget.mpSearchPanel->mapToGlobal(onCard), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        EventCounter releasesToView(&widget, QEvent::MouseButtonRelease);
        QCoreApplication::sendEvent(widget.mpSearchPanel, &releaseOnCard);
        QCOMPARE(releasesToView.count, 1);
        QHelpEvent tooltipOnShadow(QEvent::ToolTip, QPoint(2, 2), widget.mpSearchPanel->mapToGlobal(QPoint(2, 2)));
        QCoreApplication::sendEvent(widget.mpSearchPanel, &tooltipOnShadow);
        QCOMPARE(tooltipsToView.count, 1);

        // Enter flies to the first, in the other area, and the area lists follow
        QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::sendEvent(widget.mpSearchPanel->mpEdit, &enter);
        QCOMPARE(widget.mAID, eastId);
        QVERIFY(widget.mFlight);
        QCOMPARE(flownInto.size(), 1);
        QCOMPARE(flownInto.first().first().toInt(), eastId);
        finishFlight(widget);
        const TRoom* pTarget = map()->mpRoomDB->getRoom(target);
        QCOMPARE(widget.mCameraController.getTarget(), QVector3D(pTarget->x(), pTarget->y(), pTarget->z()));
        QCOMPARE(QPoint(widget.mMapCenterX, widget.mMapCenterY), QPoint(pTarget->x(), pTarget->y()));
        QVERIFY(widget.mCameraController.getScale() <= 1.5f);

        // Marked for a while, around that room only
        QCOMPARE(widget.mSearchMarkedRoomId, target);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        QList<DrawnCube> mark = drawnSearchMark(widget);
        QCOMPARE(mark.size(), 8);
        for (const DrawnCube& bar : mark) {
            QVERIFY(qAbs(bar.center.x() - pTarget->x()) <= 0.5f && qAbs(bar.center.y() - pTarget->y()) <= 0.5f);
        }
        QTRY_COMPARE_WITH_TIMEOUT(widget.mSearchMarkedRoomId, 0, 5000);
        QVERIFY(!widget.mSearchMarkTimer->isActive());
        widget.mRenderCommandQueue.clear();
        widget.queueScene();
        QVERIFY(drawnSearchMark(widget).isEmpty());

        // Ctrl+Enter plans a route there rather than going
        map()->mRoomIdHash[map()->mProfileName] = 1;
        widget.mpSearchPanel->mpList->setCurrentRow(1);
        QKeyEvent plan(QEvent::KeyPress, Qt::Key_Return, Qt::ControlModifier);
        QCoreApplication::sendEvent(widget.mpSearchPanel->mpEdit, &plan);
        QCOMPARE(widget.route().destinationRoomId(), 3);
        QCOMPARE(widget.mAID, eastId);
        // Not to the player's own room, which would only drop that route
        map()->mpRoomDB->getRoom(1)->name = qsl("Lantern Room");
        map()->setUnsaved(__func__);
        widget.mpSearchPanel->mpEdit->setText(qsl("lantern"));
        widget.chooseSearchResult(0, true);
        QCOMPARE(widget.route().destinationRoomId(), 3);

        // An area is flown into, back where the view shows its rooms, and the room found before is no longer marked
        widget.mpSearchPanel->mpEdit->setText(qsl("lighthouse"));
        widget.chooseSearchResult(0, false);
        QCOMPARE(widget.mSearchMarkedRoomId, target);
        finishFlight(widget);
        widget.mpSearchPanel->mpEdit->setText(qsl("westmarch"));
        QCOMPARE(widget.mSearchResults.value(0).kind, MapSearch::Kind::Area);
        const QListWidgetItem* pArea = widget.mpSearchPanel->mpList->item(0);
        QCOMPARE(pArea->text(), qsl("Westmarch"));
        QCOMPARE(pArea->data(MapSearchPanel::KindRole).toInt(), static_cast<int>(MapSearch::Kind::Area));
        QCOMPARE(pArea->data(MapSearchPanel::DetailRole).toString(), ModernGLWidget::tr("%n room(s)", nullptr, 36));
        QVERIFY(pArea->data(Qt::AccessibleTextRole).toString().contains(qsl("Westmarch")));
        widget.chooseSearchResult(0, false);
        QCOMPARE(widget.mAID, westId);
        QCOMPARE(widget.mSearchMarkedRoomId, 0);
        QCOMPARE(flownInto.size(), 3);
        finishFlight(widget);

        // A room with no name goes by its number
        widget.mpSearchPanel->mpEdit->setText(qsl("7"));
        QCOMPARE(widget.mSearchResults.value(0).id, 7);
        const QListWidgetItem* pNumbered = widget.mpSearchPanel->mpList->item(0);
        QCOMPARE(pNumbered->text(), qsl("Room 7"));
        QCOMPARE(pNumbered->data(MapSearchPanel::KindRole).toInt(), static_cast<int>(MapSearch::Kind::Room));
        QCOMPARE(pNumbered->data(MapSearchPanel::DetailRole).toString(), qsl("Westmarch"));
        QVERIFY(pNumbered->data(Qt::AccessibleTextRole).toString().contains(qsl("7")));

        // Nor one hidden since
        widget.mpSearchPanel->mpEdit->setText(qsl("lighthouse"));
        map()->mpRoomDB->getRoom(target)->setHidden(true);
        widget.chooseSearchResult(0, false);
        QCOMPARE(widget.mAID, westId);
        QVERIFY(!widget.mFlight);
        map()->mpRoomDB->getRoom(target)->setHidden(false);
        widget.mpSearchPanel->mpEdit->clear();

        // A room taken off the map since it was found is not gone to
        widget.mpSearchPanel->mpEdit->setText(qsl("lighthouse"));
        map()->mpRoomDB->removeRoom(target);
        map()->setUnsaved(__func__);
        widget.chooseSearchResult(0, false);
        QCOMPARE(widget.mAID, westId);
        QVERIFY(!widget.mFlight);
        QCOMPARE(widget.mSearchResults.size(), 1);

        // Escape clears what was typed, then closes it
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QCoreApplication::sendEvent(widget.mpSearchPanel->mpEdit, &escape);
        QVERIFY(!widget.mpSearchPanel->isHidden());
        QVERIFY(widget.mpSearchPanel->text().isEmpty());
        QCoreApplication::sendEvent(widget.mpSearchPanel->mpEdit, &escape);
        QVERIFY(widget.mpSearchPanel->isHidden());
        // Wherever in the panel the focus is
        widget.openSearch();
        QCoreApplication::sendEvent(widget.mpSearchPanel->mpLabels, &escape);
        QVERIFY(widget.mpSearchPanel->isHidden());

        map()->mRoomIdHash[map()->mProfileName] = 0;
    }

    void test_theSearchPanelShowsWhatItFoundAndMovesThroughItWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas(6);
        Q_UNUSED(eastId)
        for (TRoom* pRoom : map()->mpRoomDB->getRoomMap()) {
            pRoom->name = qsl("Hall %1").arg(pRoom->getId());
        }
        map()->mpRoomDB->getRoom(3)->name = qsl("Lighthouse");
        map()->mpRoomDB->getRoom(4)->name = qsl("Lighthouse Keeper's Cottage");
        map()->setUnsaved(__func__);
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, westId, 1.5f);
        widget.openSearch();
        MapSearchPanel* pPanel = widget.mpSearchPanel;
        QVERIFY(pPanel);

        // Named for screen readers
        const QStringList names{pPanel->mpEdit->accessibleName(), pPanel->mpList->accessibleName(), pPanel->mpLabels->accessibleName(), pPanel->mpClose->accessibleName()};
        for (const QString& name : names) {
            QVERIFY(!name.isEmpty());
        }
        QCOMPARE(QSet<QString>(names.cbegin(), names.cend()).size(), names.size());

        // Drawn, not taken from a theme, and sharp on a screen at twice the scale
        for (const QIcon& icon : {pPanel->mpClose->icon(), pPanel->mpSearchIcon->icon()}) {
            QVERIFY(!icon.isNull());
            QCOMPARE(icon.pixmap(QSize(16, 16), 2.0).devicePixelRatio(), 2.0);
            QVERIFY(icon.availableSizes().contains(QSize(32, 32)));
        }

        // Before anything is typed, a hint and nothing counted
        QVERIFY(!pPanel->mpMessage->isHidden());
        QCOMPARE(pPanel->mpMessage->text(), qsl("Search the whole map"));
        QVERIFY(pPanel->mpList->isHidden());
        QVERIFY(pPanel->mpCount->text().isEmpty());
        QCOMPARE(pPanel->mpKeys->text(), qsl("Esc close"));
        // Nor is a query of spaces anything, for Escape too
        pPanel->mpEdit->setText(qsl("   "));
        QCOMPARE(pPanel->mpKeys->text(), qsl("Esc close"));
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QCoreApplication::sendEvent(pPanel->mpEdit, &escape);
        QVERIFY(pPanel->isHidden());
        widget.openSearch();
        pPanel->mpEdit->clear();

        // What was typed is marked in each name, whatever its case
        pPanel->mpEdit->setText(qsl("KEEPER"));
        QCOMPARE(pPanel->mpList->count(), 1);
        QCOMPARE(pPanel->mpList->item(0)->data(MapSearchPanel::MatchStartRole).toInt(), 11);
        QCOMPARE(pPanel->mpList->item(0)->data(MapSearchPanel::MatchLengthRole).toInt(), 6);
        // Not where it is found only by number
        pPanel->mpEdit->setText(qsl("4"));
        QCOMPARE(pPanel->mpList->item(0)->text(), qsl("Lighthouse Keeper's Cottage"));
        QCOMPARE(pPanel->mpList->item(0)->data(MapSearchPanel::MatchStartRole).toInt(), -1);

        // Counted, and the arrows go round
        pPanel->mpEdit->setText(qsl("lighthouse"));
        QVERIFY(pPanel->mpMessage->isHidden());
        QCOMPARE(pPanel->mpCount->text(), qsl("2 found"));
        QCOMPARE(pPanel->mpList->item(0)->data(MapSearchPanel::MatchStartRole).toInt(), 0);
        QCOMPARE(pPanel->currentRow(), 0);
        const QString routeKeys = qsl("↑↓ choose · ↵ go · %1↵ route · Esc clear").arg(QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText));
        const QString goKeys = qsl("↑↓ choose · ↵ go · Esc clear");
        QCOMPARE(pPanel->mpKeys->text(), routeKeys);
        QKeyEvent down(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
        QKeyEvent up(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QCoreApplication::sendEvent(pPanel->mpEdit, &down);
        QCOMPARE(pPanel->currentRow(), 1);
        QCoreApplication::sendEvent(pPanel->mpEdit, &down);
        QCOMPARE(pPanel->currentRow(), 0);
        QCoreApplication::sendEvent(pPanel->mpEdit, &up);
        QCOMPARE(pPanel->currentRow(), 1);
        // From the toggle or the close button too
        QCoreApplication::sendEvent(pPanel->mpLabels, &down);
        QCOMPARE(pPanel->currentRow(), 0);
        QCoreApplication::sendEvent(pPanel->mpClose, &up);
        QCOMPARE(pPanel->currentRow(), 1);
        // Enter on the toggle turns it, and on the close button closes; only the box chooses a place
        QSignalSpy chosen(pPanel, &MapSearchPanel::resultChosen);
        QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::sendEvent(pPanel->mpLabels, &enter);
        QVERIFY(!pPanel->labelsIncluded());
        QCOMPARE(chosen.size(), 0);
        QCoreApplication::sendEvent(pPanel->mpLabels, &enter);
        QVERIFY(pPanel->labelsIncluded());
        QCOMPARE(pPanel->currentRow(), 0);
        QCoreApplication::sendEvent(pPanel->mpEdit, &down);
        QCoreApplication::sendEvent(pPanel->mpClose, &enter);
        QVERIFY(pPanel->isHidden());
        QCOMPARE(chosen.size(), 0);
        widget.openSearch();
        QKeyEvent space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, qsl(" "));
        QKeyEvent spaceUp(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier, qsl(" "));
        QCoreApplication::sendEvent(pPanel->mpClose, &space);
        QCoreApplication::sendEvent(pPanel->mpClose, &spaceUp);
        QVERIFY(pPanel->isHidden());
        widget.openSearch();
        QCoreApplication::sendEvent(pPanel->mpEdit, &down);
        QCoreApplication::sendEvent(pPanel->mpEdit, &enter);
        QCOMPARE(chosen.size(), 1);
        QCOMPARE(chosen.first().first().toInt(), 1);
        finishFlight(widget);

        // No route offered to where the player is
        map()->mRoomIdHash[map()->mProfileName] = 3;
        pPanel->mpEdit->setText(qsl("lighthouse "));
        QCOMPARE(widget.mSearchResults.value(0).id, 3);
        QCOMPARE(pPanel->mpKeys->text(), goKeys);
        QCoreApplication::sendEvent(pPanel->mpEdit, &down);
        QCOMPARE(pPanel->mpKeys->text(), routeKeys);
        map()->mRoomIdHash[map()->mProfileName] = 0;

        // More than are listed
        pPanel->mpEdit->setText(qsl("hall"));
        QCOMPARE(pPanel->mpList->count(), MapSearch::scmMaxResults);
        QCOMPARE(pPanel->mpCount->text(), qsl("Showing the best %1 of 70").arg(MapSearch::scmMaxResults));
        QKeyEvent pageDown(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
        QCoreApplication::sendEvent(pPanel->mpLabels, &pageDown);
        QVERIFY(pPanel->currentRow() > 0);

        // Nothing at all
        pPanel->mpEdit->setText(qsl("zzz"));
        QVERIFY(pPanel->mpList->isHidden());
        QVERIFY(!pPanel->mpMessage->isHidden());
        QCOMPARE(pPanel->mpMessage->text(), qsl("No matches for “zzz”"));
        QVERIFY(pPanel->mpCount->text().isEmpty());
        QCOMPARE(pPanel->mpKeys->text(), qsl("Esc clear"));
        const QString hint = pPanel->mpHint->text();
        pPanel->mpLabels->setChecked(false);
        QVERIFY(pPanel->mpHint->text() != hint);

        // A key typed on the labels toggle goes to the search box
        pPanel->mpEdit->clear();
        QKeyEvent letter(QEvent::KeyPress, Qt::Key_L, Qt::NoModifier, qsl("l"));
        QCoreApplication::sendEvent(pPanel->mpLabels, &letter);
        QCOMPARE(pPanel->text(), qsl("l"));

        // Ctrl+F in the open search selects what is typed, to be typed over
        pPanel->mpEdit->setText(qsl("light"));
        pPanel->mpEdit->deselect();
        QKeyEvent find(QEvent::KeyPress, Qt::Key_F, Qt::ControlModifier);
        QCoreApplication::sendEvent(pPanel->mpEdit, &find);
        QCOMPARE(pPanel->mpEdit->selectedText(), qsl("light"));

        // Escape on the toggle clears what was typed before it closes the search
        QCoreApplication::sendEvent(pPanel->mpLabels, &escape);
        QVERIFY(!pPanel->isHidden());
        QVERIFY(pPanel->text().isEmpty());
        QCoreApplication::sendEvent(pPanel->mpLabels, &escape);
        QVERIFY(pPanel->isHidden());
    }

    void test_theSearchPanelDrawsEachKindOfPlaceWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas(6);
        map()->mpRoomDB->getRoom(3)->name = qsl("Lighthouse");
        map()->mpRoomDB->getRoom(4)->name = qsl("The Lighthouse Keeper's Long Forgotten Cottage Beyond The Northern Cliffs");
        TMapLabel label;
        label.text = qsl("Lighthouse district");
        label.pos = QVector3D(1.0f, 2.0f, 0.0f);
        label.size = QSizeF(4.0, 2.0);
        TArea* pEast = map()->mpRoomDB->getArea(eastId);
        pEast->mMapLabels.insert(pEast->createLabelId(), label);
        map()->setUnsaved(__func__);
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, westId, 1.5f);
        widget.openSearch();
        MapSearchPanel* pPanel = widget.mpSearchPanel;

        // A map label, with where it is and no route offered to it
        pPanel->mpEdit->setText(qsl("district"));
        QCOMPARE(pPanel->mpList->count(), 1);
        const QListWidgetItem* pLabel = pPanel->mpList->item(0);
        QCOMPARE(pLabel->text(), qsl("Lighthouse district"));
        QCOMPARE(pLabel->data(MapSearchPanel::KindRole).toInt(), static_cast<int>(MapSearch::Kind::Label));
        QCOMPARE(pLabel->data(MapSearchPanel::DetailRole).toString(), qsl("Eastvale"));
        QVERIFY(pLabel->data(Qt::AccessibleTextRole).toString().contains(qsl("Lighthouse district")));
        QCOMPARE(pPanel->mpKeys->text(), qsl("↑↓ choose · ↵ go · Esc clear"));

        // Each kind drawn, and drawn differently hovered and chosen
        pPanel->mpEdit->setText(qsl("l"));
        QAbstractItemDelegate* pDelegate = pPanel->mpList->itemDelegate();
        const auto drawn = [&](const QModelIndex& index, const QStyle::State state, const int width) {
            QStyleOptionViewItem option;
            option.font = pPanel->mpList->font();
            option.state = QStyle::State_Enabled | state;
            option.rect = QRect(0, 0, width, pDelegate->sizeHint(option, index).height());
            QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            pDelegate->paint(&painter, option, index);
            painter.end();
            return image;
        };
        QSet<int> kinds;
        for (int row = 0; row < pPanel->mpList->count(); ++row) {
            const QModelIndex index = pPanel->mpList->model()->index(row, 0);
            kinds.insert(index.data(MapSearchPanel::KindRole).toInt());
            const QImage plain = drawn(index, QStyle::State_None, 360);
            const QImage hovered = drawn(index, QStyle::State_MouseOver, 360);
            const QImage chosen = drawn(index, QStyle::State_Selected, 360);
            QVERIFY(plain != QImage(plain.size(), plain.format()));
            QVERIFY(hovered != plain);
            QVERIFY(chosen != hovered);
            // A long name in a narrow list
            QVERIFY(!drawn(index, QStyle::State_Selected, 120).isNull());
        }
        QCOMPARE(kinds.size(), 3);
        pPanel->resize(240, pPanel->sizeHint().height());
        QVERIFY(!pPanel->grab().isNull());
    }

    void test_theSearchPanelKeepsItsLookUnderAProfileStyleSheetWithoutGL()
    {
        // The panel on its own, under a window with a profile's dark style sheet as it reaches the map
        QWidget window;
        window.setStyleSheet(qsl("QWidget { background-color: #1e1e1e; color: #dddddd; }"));
        window.resize(900, 700);
        auto* pPanel = new MapSearchPanel(&window);
        const auto fit = [pPanel]() {
            pPanel->setGeometry(0, 0, pPanel->preferredWidth(), pPanel->sizeHint().height());
        };
        fit();
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QColor card = QApplication::palette().color(QPalette::Window);
        const auto colorAt = [pPanel](const QImage& image, const QWidget* pChild, const QPoint& where) {
            return image.pixelColor(pChild->mapTo(pPanel, where));
        };
        QImage image = pPanel->grab().toImage();
        // Its shadow, not the sheet's background, around the card
        QVERIFY(image.pixelColor(1, 1).alpha() < 32);
        QCOMPARE(colorAt(image, pPanel->mpMessage, QPoint(1, 1)), card);
        QCOMPARE(colorAt(image, pPanel->mpHint, QPoint(1, 1)), card);
        QCOMPARE(colorAt(image, pPanel->mpKeys, QPoint(1, 1)), card);

        // Nor behind the list or its scroll bar
        QList<MapSearchPanel::Row> rows;
        for (int number = 1; number <= 50; ++number) {
            MapSearchPanel::Row row;
            row.title = qsl("Hall %1").arg(number);
            row.detail = qsl("Westmarch · room %1").arg(number);
            rows.append(row);
        }
        pPanel->setResults(rows, rows.size());
        fit();
        QCoreApplication::processEvents();
        image = pPanel->grab().toImage();
        const QWidget* pViewport = pPanel->mpList->viewport();
        QCOMPARE(colorAt(image, pViewport, QPoint(pViewport->width() - 2, pViewport->height() - 2)), card);
        const QScrollBar* pBar = pPanel->mpList->verticalScrollBar();
        QVERIFY(pBar->isVisible());
        QCOMPARE(colorAt(image, pBar->parentWidget(), QPoint(1, pBar->parentWidget()->height() - 2)), card);

        // Colored from the application's palette, not the window's
        QPalette windowPalette = window.palette();
        windowPalette.setColor(QPalette::WindowText, QColor(255, 0, 0));
        window.setPalette(windowPalette);
        QVERIFY(!pPanel->mpMessage->styleSheet().contains(qsl("rgba(255, 0, 0")));
        // And again once that changes, with a new style as Mudlet's light/dark switch sets; a palette change on its own
        // Qt passes down only where no style sheet pins the palette
        const QPalette before = QApplication::palette();
        auto restore = qScopeGuard([&before]() {
            QApplication::setPalette(before);
        });
        QPalette changed = before;
        changed.setColor(QPalette::WindowText, QColor(18, 52, 86));
        changed.setColor(QPalette::Highlight, QColor(101, 67, 33));
        QApplication::setPalette(changed);
        QStyle* pStyle = QApplication::style();
        // Kept from being deleted with the one put in its place
        pStyle->setParent(nullptr);
        auto restoreStyle = qScopeGuard([pStyle]() {
            QApplication::setStyle(pStyle);
        });
        QApplication::setStyle(new QProxyStyle(pStyle->name()));
        QVERIFY(pPanel->mpMessage->styleSheet().contains(qsl("rgba(18, 52, 86, 255)")));
        QVERIFY(pPanel->mpEdit->styleSheet().contains(qsl("rgba(101, 67, 33, 255)")));
        QVERIFY(pPanel->mpLabels->styleSheet().contains(qsl("rgba(101, 67, 33, 255)")));
        QVERIFY(!pPanel->mpMessage->styleSheet().contains(qsl("rgba(255, 0, 0")));
    }

    void test_theSearchPanelsCardFollowsItsSizeAndColorsWithoutGL()
    {
        QWidget window;
        auto* pPanel = new MapSearchPanel(&window);
        const auto backdrop = [](MapSearchPanel* pWhich) {
            QImage image(pWhich->size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            pWhich->render(&image, {}, {}, QWidget::RenderFlags());
            return image;
        };
        pPanel->resize(400, 200);
        backdrop(pPanel);
        pPanel->resize(300, 260);
        auto* pFresh = new MapSearchPanel(&window);
        pFresh->resize(300, 260);
        QCOMPARE(backdrop(pPanel), backdrop(pFresh));

        const QPalette before = QApplication::palette();
        auto restore = qScopeGuard([&before]() {
            QApplication::setPalette(before);
        });
        QPalette changed = before;
        changed.setColor(QPalette::Window, QColor(12, 120, 34));
        QApplication::setPalette(changed);
        QCOMPARE(backdrop(pPanel).pixelColor(150, 130), QColor(12, 120, 34));
    }

    void test_theSearchPanelKeepsItsInputFromTheMapWithoutGL()
    {
        // The window stands in for the map under the panel
        QWidget window;
        window.resize(900, 700);
        auto* pPanel = new MapSearchPanel(&window);
        pPanel->setGeometry(0, 0, pPanel->preferredWidth(), pPanel->sizeHint().height());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        EventCounter menus(&window, QEvent::ContextMenu);
        EventCounter presses(&window, QEvent::MouseButtonPress);
        // What Qt makes of a second touch so soon after the first
        EventCounter doublePresses(&window, QEvent::MouseButtonDblClick);
        EventCounter keys(&window, QEvent::KeyPress);
        const auto menuOn = [](QWidget* pWidget, const QPoint& where, const QContextMenuEvent::Reason reason) {
            QContextMenuEvent event(reason, where, pWidget->mapToGlobal(where));
            QCoreApplication::sendEvent(pWidget, &event);
        };

        // A right click or the Menu key anywhere on the card is the panel's
        for (QWidget* pWidget : std::initializer_list<QWidget*>{pPanel->mpLabels, pPanel->mpClose, pPanel->mpMessage, pPanel->mpHint, pPanel->mpKeys}) {
            menuOn(pWidget, pWidget->rect().center(), QContextMenuEvent::Mouse);
        }
        menuOn(pPanel->mpLabels, pPanel->mpLabels->rect().center(), QContextMenuEvent::Keyboard);
        menuOn(pPanel, QPoint(MapSearchPanel::scmShadowMargin + 4, pPanel->height() / 2), QContextMenuEvent::Mouse);
        QCOMPARE(menus.count, 0);
        // But on the shadow around it, the map's
        menuOn(pPanel, QPoint(2, 2), QContextMenuEvent::Mouse);
        QCOMPARE(menus.count, 1);

        // A touch on the card works its buttons and goes no further; on the shadow, it reaches the map
        QPointingDevice* pTouch = QTest::createTouchDevice();
        const QPoint onLabels = pPanel->mpLabels->mapTo(&window, pPanel->mpLabels->rect().center());
        QTest::touchEvent(&window, pTouch).press(0, onLabels);
        QTest::touchEvent(&window, pTouch).release(0, onLabels);
        QCoreApplication::processEvents();
        QVERIFY(!pPanel->labelsIncluded());
        QCOMPARE(presses.count + doublePresses.count, 0);
        QTest::touchEvent(&window, pTouch).press(0, QPoint(2, 2));
        QTest::touchEvent(&window, pTouch).release(0, QPoint(2, 2));
        QCoreApplication::processEvents();
        QCOMPARE(presses.count + doublePresses.count, 1);

        // A key the box leaves is not handed back to it, over and over
        pPanel->mpEdit->setReadOnly(true);
        pPanel->mpLabels->setFocus(Qt::OtherFocusReason);
        QKeyEvent letter(QEvent::KeyPress, Qt::Key_L, Qt::NoModifier, qsl("l"));
        QCoreApplication::sendEvent(pPanel->mpLabels, &letter);
        QVERIFY(pPanel->text().isEmpty());
        QCOMPARE(keys.count, 0);
        pPanel->mpEdit->setReadOnly(false);

        // Not restyled by a change that leaves its colors and font as they were
        pPanel->mpCount->setStyleSheet(qsl("QLabel { }"));
        QEvent styleChange(QEvent::StyleChange);
        QCoreApplication::sendEvent(pPanel, &styleChange);
        QEvent paletteChange(QEvent::PaletteChange);
        QCoreApplication::sendEvent(pPanel, &paletteChange);
        QCOMPARE(pPanel->mpCount->styleSheet(), qsl("QLabel { }"));

        // Sized from the font the profile gives it, as that changes
        QFont larger = window.font();
        larger.setPointSizeF(21.0);
        window.setFont(larger);
        QCOMPARE(pPanel->mpMessage->font().pointSizeF(), 21.0);
        QCOMPARE(pPanel->mpMessage->font().weight(), QFont::DemiBold);
        QCOMPARE(pPanel->mpEdit->font().pointSizeF(), 21.0 * 1.1);
        QCOMPARE(pPanel->mpCount->font().pointSizeF(), 21.0 * 0.85);
        QCOMPARE(pPanel->mpHint->font().pointSizeF(), 21.0);
        QCOMPARE(pPanel->mpList->font().pointSizeF(), 21.0);
        QCOMPARE(pPanel->mpLabels->font().pointSizeF(), 21.0);
        QCOMPARE(pPanel->mpKeys->font().pointSizeF(), 21.0 * 0.85);
        QVERIFY(pPanel->mpCount->styleSheet() != qsl("QLabel { }"));
    }

    void test_theSearchPanelIsTallEnoughForItsHintInANarrowViewWithoutGL()
    {
        const auto [westId, eastId] = addTwoAreas(6);
        Q_UNUSED(eastId)
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, westId, 1.5f);
        widget.openSearch();
        MapSearchPanel* pPanel = widget.mpSearchPanel;
        QVERIFY(!pPanel->mpHint->isHidden());
        // The hint wraps onto more lines than the panel's size hint allows for
        widget.resize(240, 700);
        widget.placeSearchPanel();
        QCOMPARE(pPanel->width(), 240);
        QVERIFY(pPanel->heightForWidth(240) > pPanel->sizeHint().height());
        QCOMPARE(pPanel->height(), pPanel->heightForWidth(240));
        pPanel->layout()->setGeometry(pPanel->rect());
        QVERIFY(pPanel->mpHint->height() >= pPanel->mpHint->heightForWidth(pPanel->mpHint->width()));
        QVERIFY(pPanel->mpHint->geometry().bottom() < pPanel->mpKeys->geometry().top());
        QVERIFY(pPanel->mpMessage->geometry().bottom() < pPanel->mpHint->geometry().top());
    }

    void test_theSearchOffersARouteOnlyWhileThePlayerIsElsewhere()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        const auto [westId, eastId] = addTwoAreas(6);
        Q_UNUSED(westId)
        Q_UNUSED(eastId)
        map()->mpRoomDB->getRoom(3)->name = qsl("Lighthouse");
        map()->setUnsaved(__func__);
        ModernGLWidget widget(map(), mpHost);
        widget.resize(800, 600);
        widget.grabFramebuffer();
        if (!widget.isValid() || !widget.failureMessage().isEmpty()) {
            QSKIP("no OpenGL context on this platform");
        }
        widget.openSearch();
        MapSearchPanel* pPanel = widget.mpSearchPanel;
        pPanel->mpEdit->setText(qsl("lighthouse"));
        QCOMPARE(widget.mSearchResults.value(0).id, 3);
        const QString routeKeys = qsl("↑↓ choose · ↵ go · %1↵ route · Esc clear").arg(QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText));
        QCOMPARE(pPanel->mpKeys->text(), routeKeys);
        // The player walks into it, and the map is drawn again as it always is for a move
        map()->mRoomIdHash[map()->mProfileName] = 3;
        widget.grabFramebuffer();
        QCOMPARE(pPanel->mpKeys->text(), qsl("↑↓ choose · ↵ go · Esc clear"));
        map()->mRoomIdHash[map()->mProfileName] = 0;
        widget.grabFramebuffer();
        QCOMPARE(pPanel->mpKeys->text(), routeKeys);
    }

    void benchmark_worldSearch()
    {
        if (qEnvironmentVariableIsEmpty("MUDLET_BENCH_3D")) {
            QSKIP("set MUDLET_BENCH_3D=1 to run the 3D view benchmark");
        }
        addHugeMap();
        static const QStringList words{qsl("Market"), qsl("Street"), qsl("Alley"), qsl("Temple"), qsl("Harbor"), qsl("Gate"), qsl("Tower"), qsl("Garden")};
        for (TRoom* pRoom : map()->mpRoomDB->getRoomMap()) {
            pRoom->name = qsl("%1 %2 %3").arg(words.at(pRoom->getId() % words.size()), words.at(pRoom->getId() / 7 % words.size())).arg(pRoom->getId() % 1000);
        }
        map()->setUnsaved(__func__);
        MapSearch search;
        QElapsedTimer timer;
        timer.start();
        search.refresh(*map());
        std::printf("METRIC search_entries %lld\n", static_cast<long long>(search.size()));
        std::printf("METRIC search_index_ms %.2f\n", static_cast<double>(timer.nsecsElapsed()) / 1.0e6);
        for (const QString& query : {qsl("a"), qsl("market"), qsl("temple gate 12"), qsl("12345"), qsl("nowhere")}) {
            constexpr int runs = 20;
            qsizetype total = 0;
            timer.restart();
            for (int run = 0; run < runs; ++run) {
                search.find(query, 1, true, MapSearch::scmMaxResults, &total);
            }
            std::printf("METRIC search_query_ms{%s} %.2f (%lld found)\n", qPrintable(query), static_cast<double>(timer.nsecsElapsed()) / 1.0e6 / runs, static_cast<long long>(total));
        }
        std::fflush(stdout);
    }

    void test_theWorldStaysPutAsAFlightToAnotherLevelSetsOffWithoutGL()
    {
        map()->mapClear();
        map()->mUserData.remove(TAreaGraph::cLayoutUserDataKey);
        const int next = addGridArea(qsl("Westmarch"), 1, 20, 20, 1, 0);
        addGridArea(qsl("Highvale"), next, 20, 20, 1, 40);
        map()->setExit(1, next, DIR_EAST);
        const int westId = map()->mpRoomDB->getRoom(1)->getArea();
        const int eastId = map()->mpRoomDB->getRoom(next)->getArea();
        map()->mRoomIdHash[map()->mProfileName] = 0;
        map()->areaGraph()->refresh();
        QVERIFY(map()->areaGraph()->setPosition(westId, QPointF(0.0, 0.0)));
        QVERIFY(map()->areaGraph()->setPosition(eastId, QPointF(0.0, 150.0)));
        ModernGLWidget widget(map(), mpHost);
        queueFrame(widget, westId, 800.0f, false);
        const QPoint before = pixelOfArea(widget, eastId);
        QVERIFY(QRect(0, 0, widget.width(), widget.height()).contains(before));

        widget.flyToArea(eastId);
        widget.updateMatrices();
        const QPoint after = pixelOfArea(widget, eastId);
        QVERIFY2((after - before).manhattanLength() <= 1,
                 qPrintable(qsl("the area flown to jumped from %1,%2 to %3,%4 as the flight set off").arg(before.x()).arg(before.y()).arg(after.x()).arg(after.y())));

        for (int tick = 0; tick < 1000 && widget.mCameraSmoothAnimating; ++tick) {
            widget.onCameraAnimationTick();
        }
        QCOMPARE(widget.mCameraController.getTarget().z(), 40.0f);
        widget.updateMatrices();
        QVERIFY2(QRect(0, 0, widget.width(), widget.height()).contains(pixelOfArea(widget, eastId)), "the area flown to is not in view once landed");
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
        applyBenchStitching();
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
        const bool routed = !qEnvironmentVariableIsEmpty("MUDLET_BENCH_3D_ROUTE");
        if (routed) {
            // Across the metropolis to its corner, found again on every move of the walk below
            QElapsedTimer routeTimer;
            routeTimer.start();
            widget.planRouteToRoom(1 + 141 * 141);
            std::printf("METRIC route_find_ms %lld\n", routeTimer.elapsed());
            std::printf("METRIC route_steps %d\n", widget.route().steps());
        }

        struct Scenario
        {
            const char* name;
            float distance;
            bool topView;
        };
        // close is about the 2D map's default zoom; far shows the whole of the viewed level and more; areas and
        // world are zoomed out to the areas' footprints and to the world graph
        const QList<Scenario> scenarios{{"close", 1.0f, false}, {"mid", 8.0f, false}, {"far", 40.0f, false}, {"far_top", 40.0f, true}, {"areas", 200.0f, false}, {"world", 800.0f, false}};
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
            if (routed) {
                widget.refreshRoute();
            }
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

        // Mapping: as a mapping script does on each move, a room is added east of the metropolis, joined to the last
        // and the player moved into it, then the frame is drawn and its events run
        widget.mCameraController.setDefaultView();
        widget.setCameraDistance(8.0f);
        int mappedFrom = 1 + (141 + 70) * 141 + 140;
        QElapsedTimer mapping;
        const double mappingCpuBefore = processCpuMs();
        mapping.start();
        for (int frame = 0; frame < frames; ++frame) {
            const int roomId = 500000 + frame;
            map()->addRoom(roomId);
            map()->setRoomArea(roomId, areaId);
            map()->setRoomCoordinates(roomId, 71 + frame, 0, 0);
            map()->setExit(mappedFrom, roomId, DIR_EAST);
            map()->setExit(roomId, mappedFrom, DIR_WEST);
            map()->setUnsaved(__func__);
            map()->mRoomIdHash[map()->mProfileName] = roomId;
            mappedFrom = roomId;
            widget.grabFramebuffer();
            QCoreApplication::processEvents();
        }
        std::printf("METRIC mapping_frame_ms %.2f\n", static_cast<double>(mapping.nsecsElapsed()) / 1.0e6 / frames);
        std::printf("METRIC mapping_frame_cpu_ms %.2f\n", (processCpuMs() - mappingCpuBefore) / frames);

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
        applyBenchStitching();
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
            // Zoomed out to the areas' footprints and to the world graph
            views(widget, qsl("huge-atlas"), {200.0f, 800.0f});
        }
        mpHost->mShowRoomID = savedIds;
        mpHost->mBubbleMode = savedBubbles;
        mpHost->mMapperShowGrid = savedGrid;
    }
};

#include "ModernMapperSceneTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperSceneTest)
