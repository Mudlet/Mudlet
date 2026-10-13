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
 * ExitScene is what the modern 3D view draws a map's exits from: lines, doors,
 * stubs, custom lines and area-exit arrows, as bars and triangles.
 *
 * Why not a spec: the scene only reaches the screen through OpenGL, and no Lua
 * call reports what was drawn.
 *
 * Run with: ctest -R ExitSceneTest -V
 */

#include <algorithm>

#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ExitAppearance.h"
#include "ExitScene.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "RoomAppearance.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
constexpr float scmTolerance = 1e-3f;

QList<ExitScene::Segment> segmentsIn(const ExitScene::Scene& scene, const QColor& color)
{
    QList<ExitScene::Segment> found;
    for (const ExitScene::Segment& segment : scene.segments) {
        if (segment.color == color) {
            found.append(segment);
        }
    }
    return found;
}

QList<ExitScene::Triangle> trianglesIn(const ExitScene::Scene& scene, const QColor& color)
{
    QList<ExitScene::Triangle> found;
    for (const ExitScene::Triangle& triangle : scene.triangles) {
        if (triangle.color == color) {
            found.append(triangle);
        }
    }
    return found;
}

bool closeTo(const QVector2D& a, const QVector2D& b)
{
    return (a - b).length() < scmTolerance;
}

QVector2D flat(const QVector3D& point)
{
    return point.toVector2D();
}

bool joins(const ExitScene::Segment& segment, const QVector2D& a, const QVector2D& b)
{
    return (closeTo(flat(segment.from), a) && closeTo(flat(segment.to), b)) || (closeTo(flat(segment.from), b) && closeTo(flat(segment.to), a));
}

float totalLength(const QList<ExitScene::Segment>& segments)
{
    float length = 0.0f;
    for (const ExitScene::Segment& segment : segments) {
        length += (segment.to - segment.from).length();
    }
    return length;
}

// The point of a triangle furthest along direction
QVector3D furthest(const ExitScene::Triangle& triangle, const QVector3D& direction)
{
    QVector3D best = triangle.a;
    for (const QVector3D& corner : {triangle.b, triangle.c}) {
        if (QVector3D::dotProduct(corner, direction) > QVector3D::dotProduct(best, direction)) {
            best = corner;
        }
    }
    return best;
}
} // namespace

class ExitSceneTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ExitScene-Test");
    int mAreaId = 0;
    const QColor mExitColor = QColor(1, 2, 3);

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

    TRoom* addRoomAt(const int id, const int areaId, const int x, const int y, const int z = 0) const
    {
        if (!(map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, z))) {
            return nullptr;
        }
        return map()->mpRoomDB->getRoom(id);
    }

    ExitScene::Options options() const
    {
        ExitScene::Options result;
        result.areaId = mAreaId;
        result.levelsAbove = 1;
        result.levelsBelow = 1;
        result.lineWidth = 0.05f;
        result.lineHeight = 0.04f;
        result.exitColor = mExitColor;
        result.background = QColor(Qt::black);
        return result;
    }

    ExitScene::Scene build(const ExitScene::Options& sceneOptions) const { return ExitScene::build(*map(), *mpHost, sceneOptions); }
    ExitScene::Scene build() const { return build(options()); }

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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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

    void init()
    {
        map()->mapClear();
        mAreaId = map()->mpRoomDB->addArea(qsl("Exit Area"));
        QVERIFY(mAreaId > 0);
    }

    void test_twoWayExitIsDrawnOnce()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, 1, 0);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);

        const ExitScene::Scene scene = build();
        const QList<ExitScene::Segment> lines = segmentsIn(scene, mExitColor);
        QCOMPARE(lines.size(), 1);
        QVERIFY(joins(lines.first(), QVector2D(0, 0), QVector2D(1, 0)));
        QVERIFY2(scene.triangles.isEmpty(), "a two-way exit has no arrow");
    }

    // Unless the room at the far end, the one with the lower id, will not draw it: it is on a level not shown
    void test_twoWayExitToALevelNotShownIsStillDrawn()
    {
        TRoom* pHigh = addRoomAt(1, mAreaId, 0, 0, 1);
        TRoom* pLow = addRoomAt(2, mAreaId, 0, 0, 0);
        QVERIFY(pLow && pHigh);
        pLow->setExit(1, DIR_UP);
        pHigh->setExit(2, DIR_DOWN);

        ExitScene::Options sceneOptions = options();
        sceneOptions.levelsAbove = 0;
        const QList<ExitScene::Segment> lines = segmentsIn(build(sceneOptions), mExitColor);
        QCOMPARE(lines.size(), 1);
        QCOMPARE(lines.first().level, 0);
    }

    // The lower-numbered room draws its two-way exits, but dots them all when it has a one-way one to the same room
    void test_twoWayExitIsDrawnWhenTheOtherEndDotsItsLine()
    {
        TRoom* pNorth = addRoomAt(3, mAreaId, 0, 1);
        TRoom* pSouth = addRoomAt(5, mAreaId, 0, 0);
        QVERIFY(pNorth && pSouth);
        pSouth->setExit(3, DIR_NORTH);
        pNorth->setExit(5, DIR_SOUTH);
        pNorth->setExit(5, DIR_UP);

        bool solid = false;
        for (const ExitScene::Segment& line : segmentsIn(build(), mExitColor)) {
            solid = solid || joins(line, QVector2D(0, 0), QVector2D(0, 1));
        }
        QVERIFY2(solid, "neither end draws the two-way exit as a whole line");
    }

    // Levels are counted from the one viewed, and a line between two takes the fading of the nearer
    void test_levelsAreCountedFromTheViewedLevel()
    {
        TRoom* pGround = addRoomAt(1, mAreaId, 0, 0, 0);
        TRoom* pUpstairs = addRoomAt(2, mAreaId, 0, 0, 1);
        TRoom* pAttic = addRoomAt(3, mAreaId, 5, 5, 2);
        QVERIFY(pGround && pUpstairs && pAttic);
        pGround->setExit(2, DIR_UP);
        pUpstairs->setExit(1, DIR_DOWN);
        pAttic->setExitStub(DIR_EAST, true);
        pUpstairs->setExitStub(DIR_WEST, true);

        ExitScene::Options sceneOptions = options();
        sceneOptions.centerZ = 1;
        sceneOptions.levelsAbove = 0;
        sceneOptions.levelsBelow = 0;
        const QList<ExitScene::Segment> lines = segmentsIn(build(sceneOptions), mExitColor);
        QCOMPARE(lines.size(), 2);
        bool stub = false;
        bool stairs = false;
        for (const ExitScene::Segment& line : lines) {
            QVERIFY2(!closeTo(flat(line.from), QVector2D(5, 5)), "a stub on a level not shown is drawn");
            QCOMPARE(line.level, 1);
            stub = stub || joins(line, QVector2D(0, 0), QVector2D(-0.5f, 0));
            stairs = stairs || qAbs(qAbs(line.to.z() - line.from.z()) - 1.0f) < scmTolerance;
        }
        QVERIFY(stub && stairs);
    }

    // Up and down exits that do not lead straight up or down are short shafts at each end, not slanting lines
    void test_offsetStairsAreShortShaftsAtEachEnd()
    {
        TRoom* pLow = addRoomAt(1, mAreaId, 0, 0, 0);
        TRoom* pHigh = addRoomAt(2, mAreaId, 3, 2, 1);
        QVERIFY(pLow && pHigh);
        pLow->setExit(2, DIR_UP);
        pHigh->setExit(1, DIR_DOWN);

        const QList<ExitScene::Segment> lines = segmentsIn(build(), mExitColor);
        QCOMPARE(lines.size(), 2);
        bool rising = false;
        bool dropping = false;
        for (const ExitScene::Segment& line : lines) {
            QVERIFY2(closeTo(flat(line.from), flat(line.to)), "a shaft slants");
            const float rise = line.to.z() - line.from.z();
            QVERIFY2(qAbs(rise) > 0.1f && qAbs(rise) < 0.5f, qPrintable(qsl("a shaft %1 levels tall").arg(rise)));
            rising = rising || (closeTo(flat(line.from), QVector2D(0, 0)) && line.level == 0 && rise > 0.0f);
            dropping = dropping || (closeTo(flat(line.from), QVector2D(3, 2)) && line.level == 1 && rise < 0.0f);
        }
        QVERIFY(rising && dropping);
    }

    // A one-way one is a shaft from its own room only, tipped in the one-way arrow's color
    void test_oneWayOffsetStairsAreAShaftWithAColoredTip()
    {
        TRoom* pLow = addRoomAt(1, mAreaId, 0, 0, 0);
        TRoom* pHigh = addRoomAt(2, mAreaId, 3, 2, 1);
        QVERIFY(pLow && pHigh);
        pHigh->setExit(1, DIR_DOWN);

        const ExitScene::Scene scene = build();
        const QList<ExitScene::Segment> shaft = segmentsIn(scene, mExitColor);
        const QList<ExitScene::Segment> tip = segmentsIn(scene, ExitAppearance::oneWayArrowColor());
        QCOMPARE(shaft.size(), 1);
        QCOMPARE(tip.size(), 1);
        for (const ExitScene::Segment& line : {shaft.first(), tip.first()}) {
            QVERIFY2(closeTo(flat(line.from), QVector2D(3, 2)) && closeTo(flat(line.to), QVector2D(3, 2)), "the shaft is not upright at the room with the exit");
            QVERIFY(line.to.z() < line.from.z());
        }
        QVERIFY(qAbs(shaft.first().to.z() - tip.first().from.z()) < scmTolerance);
        QVERIFY(scene.triangles.isEmpty());
    }

    void test_dashesTooFineForTheLineAreDrawnSolid_data()
    {
        QTest::addColumn<float>("lineWidth");
        QTest::addColumn<int>("eastX");
        QTest::newRow("exit lines too thin") << 1e-6f << 1;
        QTest::newRow("rooms too far apart") << 0.05f << 20000;
    }

    // As Qt's own dash stroker does, a pattern that would repeat too often along the line is drawn solid, rather than without end
    void test_dashesTooFineForTheLineAreDrawnSolid()
    {
        QFETCH(float, lineWidth);
        QFETCH(int, eastX);
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        QVERIFY(addRoomAt(2, mAreaId, eastX, 0));
        QVERIFY(pWest);
        pWest->setExit(2, DIR_EAST);

        ExitScene::Options sceneOptions = options();
        sceneOptions.lineWidth = lineWidth;
        const ExitScene::Scene scene = build(sceneOptions);
        const QList<ExitScene::Segment> lines = segmentsIn(scene, mExitColor);
        QCOMPARE(lines.size(), 1);
        QVERIFY(joins(lines.first(), QVector2D(0, 0), QVector2D(eastX, 0)));
        QCOMPARE(scene.solidDashRoomId, 1);
    }

    void test_oneWayExitIsDottedWithAnArrowInTheMiddle()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        QVERIFY(addRoomAt(2, mAreaId, 1, 0));
        QVERIFY(pWest);
        pWest->setExit(2, DIR_EAST);

        const ExitScene::Scene scene = build();
        QCOMPARE(scene.solidDashRoomId, 0);
        const QList<ExitScene::Segment> dots = segmentsIn(scene, mExitColor);
        QVERIFY2(dots.size() > 3, qPrintable(qsl("%1 pieces is not a dotted line").arg(dots.size())));
        for (const ExitScene::Segment& dot : dots) {
            QVERIFY(dot.from.x() >= -scmTolerance && dot.to.x() <= 1.0f + scmTolerance);
            QVERIFY(qAbs(dot.from.y()) < scmTolerance && qAbs(dot.to.y()) < scmTolerance);
        }
        QVERIFY2(totalLength(dots) < 0.9f, "the dots leave no gaps");

        const QList<ExitScene::Triangle> arrow = trianglesIn(scene, ExitAppearance::oneWayArrowColor());
        QVERIFY(!arrow.isEmpty());
        const QVector3D tip = furthest(arrow.first(), QVector3D(1, 0, 0));
        // Centered on the middle of the line, pointing to where the exit leads
        QVERIFY2(tip.x() > 0.5f && tip.x() < 0.7f, qPrintable(qsl("the arrow's tip is at x %1").arg(tip.x())));
        const QVector3D back = furthest(arrow.first(), QVector3D(-1, 0, 0));
        QVERIFY(back.x() < 0.5f);
    }

    void test_doorsAreColoredAsOnThe2DMap_data()
    {
        QTest::addColumn<int>("status");
        QTest::newRow("open") << 1;
        QTest::newRow("closed") << 2;
        QTest::newRow("locked") << 3;
    }

    void test_doorsAreColoredAsOnThe2DMap()
    {
        QFETCH(int, status);
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, 1, 0);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);
        pWest->setDoor(qsl("e"), status);

        const QList<ExitScene::Segment> strokes = segmentsIn(build(), ExitAppearance::doorColor(status));
        QCOMPARE(strokes.size(), 2);
        for (const ExitScene::Segment& stroke : strokes) {
            // On the room's own half of the line
            QVERIFY(stroke.from.x() > 0.0f && stroke.from.x() < 0.5f);
            QVERIFY(stroke.to.x() > 0.0f && stroke.to.x() < 0.5f);
            QVERIFY2(stroke.height > options().lineHeight, "the door does not stand above the line");
        }
        // Mirror images either side of the line
        QVERIFY(qAbs(strokes.at(0).from.y() + strokes.at(1).from.y()) < scmTolerance);
        QVERIFY(qAbs(strokes.at(0).from.y()) > scmTolerance);
    }

    void test_doorSwingsAreAPlansDoor_data()
    {
        QTest::addColumn<int>("status");
        QTest::addColumn<int>("distance");
        QTest::newRow("open, next door") << 1 << 1;
        QTest::newRow("closed, next door") << 2 << 1;
        QTest::newRow("locked, next door") << 3 << 1;
        QTest::newRow("open, far apart") << 1 << 4;
        QTest::newRow("closed, far apart") << 2 << 4;
        QTest::newRow("locked, far apart") << 3 << 4;
    }

    void test_doorSwingsAreAPlansDoor()
    {
        QFETCH(int, status);
        QFETCH(int, distance);
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, distance, 0);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);
        pWest->setDoor(qsl("e"), status);
        ExitScene::Options swings = options();
        swings.doorSwings = true;

        QList<ExitScene::Segment> door;
        float widest = 0.0f;
        for (const ExitScene::Segment& segment : build(swings).segments) {
            if (segment.color != mExitColor) {
                door.append(segment);
                widest = qMax(widest, segment.width);
            }
        }
        // The leaf (and a locked door's bolt) and the arc it sweeps through
        QCOMPARE(door.size(), status == 3 ? 10 : 9);
        const float half = static_cast<float>(distance) / 2.0f;
        QList<ExitScene::Segment> heavy;
        for (const ExitScene::Segment& segment : door) {
            QCOMPARE(segment.color, door.first().color);
            for (const QVector3D& end : {segment.from, segment.to}) {
                QVERIFY2(end.x() > 0.0f && end.x() < half, qPrintable(qsl("the swing reaches x %1, outside its half of the exit").arg(end.x())));
            }
            QVERIFY2(segment.height > options().lineHeight, "the door does not stand above the line");
            if (qFuzzyCompare(segment.width, widest)) {
                heavy.append(segment);
            }
        }
        QCOMPARE(heavy.size(), status == 3 ? 2 : 1);
        if (status == 1) {
            const ExitScene::Segment& leaf = heavy.first();
            const bool besideTheLine = qAbs(leaf.from.y() - leaf.to.y()) < scmTolerance && qAbs(leaf.from.y()) > scmTolerance;
            QVERIFY2(besideTheLine, "an open door's leaf is not swung back beside the line");
        } else {
            const ExitScene::Segment& leaf = heavy.first();
            const bool acrossTheLine = qAbs(leaf.from.x() - leaf.to.x()) < scmTolerance && leaf.from.y() * leaf.to.y() < 0.0f;
            QVERIFY2(acrossTheLine, "a shut door's leaf does not close the opening");
        }
        if (status == 3) {
            const ExitScene::Segment& bolt = heavy.last();
            QVERIFY2(qAbs(bolt.from.y()) < scmTolerance && qAbs(bolt.to.y()) < scmTolerance, "a locked door has no bolt along the line");
        }
    }

    void test_doorSwingsClearTheirRoom_data()
    {
        QTest::addColumn<int>("eastward");
        QTest::addColumn<int>("northward");
        QTest::addColumn<bool>("round");
        QTest::newRow("east, square") << 1 << 0 << false;
        QTest::newRow("northeast, square") << 1 << 1 << false;
        QTest::newRow("northeast, round") << 1 << 1 << true;
        QTest::newRow("southeast, square, further") << 2 << -2 << false;
    }

    // A room hides whatever of a door is inside it, which on a diagonal from a square room is further out
    void test_doorSwingsClearTheirRoom()
    {
        QFETCH(int, eastward);
        QFETCH(int, northward);
        QFETCH(bool, round);
        TRoom* pFrom = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pTo = addRoomAt(2, mAreaId, eastward, northward);
        QVERIFY(pFrom && pTo);
        const int direction = northward == 0 ? DIR_EAST : (northward > 0 ? DIR_NORTHEAST : DIR_SOUTHEAST);
        pFrom->setExit(2, direction);
        pTo->setExit(1, TMap::scmReverseDirections.value(direction));
        pFrom->setDoor(TRoom::dirCodeToShortString(direction), 3);
        ExitScene::Options swings = options();
        swings.doorSwings = true;
        swings.roomSize = 0.5f;
        swings.roundRooms = round;
        const float half = swings.roomSize / 2.0f;
        const auto inside = [half, round](const QVector3D& point) {
            return round ? std::hypot(point.x(), point.y()) < half : qMax(std::abs(point.x()), std::abs(point.y())) < half;
        };

        int doorSegments = 0;
        for (const ExitScene::Segment& segment : build(swings).segments) {
            if (segment.color == mExitColor) {
                continue;
            }
            ++doorSegments;
            for (const QVector3D& point : {segment.from, segment.to, (segment.from + segment.to) / 2.0f}) {
                QVERIFY2(!inside(point), qPrintable(qsl("the door reaches (%1, %2), inside its room").arg(point.x()).arg(point.y())));
            }
        }
        QCOMPARE(doorSegments, 10);
    }

    void test_doorSwingsOfEachStatusDiffer()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, 1, 0);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);
        ExitScene::Options swings = options();
        swings.doorSwings = true;
        QSet<QRgb> colors;
        for (const int status : {1, 2, 3}) {
            pWest->setDoor(qsl("e"), status);
            for (const ExitScene::Segment& segment : build(swings).segments) {
                if (segment.color != mExitColor) {
                    colors.insert(segment.color.rgba());
                }
            }
        }
        QCOMPARE(colors.size(), 3);
    }

    void test_stubIsAHalfLineEndingInADot()
    {
        TRoom* pRoom = addRoomAt(1, mAreaId, 3, 4);
        QVERIFY(pRoom);
        pRoom->setExitStub(DIR_NORTH, true);
        pRoom->setDoor(qsl("n"), 2);

        const ExitScene::Scene scene = build();
        const QList<ExitScene::Segment> lines = segmentsIn(scene, mExitColor);
        QCOMPARE(lines.size(), 1);
        QVERIFY(joins(lines.first(), QVector2D(3, 4), QVector2D(3, 4.5f)));
        const QList<ExitScene::Triangle> dot = trianglesIn(scene, mExitColor);
        QVERIFY(!dot.isEmpty());
        for (const ExitScene::Triangle& triangle : dot) {
            for (const QVector3D& corner : {triangle.a, triangle.b, triangle.c}) {
                QVERIFY((flat(corner) - QVector2D(3, 4.5f)).length() <= 0.1f + scmTolerance);
            }
        }
        QCOMPARE(segmentsIn(scene, ExitAppearance::doorColor(2)).size(), 2);
    }

    void test_glyphsFillTheCellsBetweenRoomsWithTheDoorNextToItsRoom()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, 2, 0);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);
        pWest->setDoor(qsl("e"), 2);
        ExitScene::Options glyphOptions = options();
        glyphOptions.glyphs = true;

        const ExitScene::Scene scene = build(glyphOptions);
        QVERIFY(segmentsIn(scene, mExitColor).isEmpty());
        QCOMPARE(scene.glyphs.size(), 3);
        for (const ExitScene::Glyph& glyph : scene.glyphs) {
            QVERIFY(qAbs(glyph.at.y()) < scmTolerance);
            if (qAbs(glyph.at.x() - 0.5f) < scmTolerance) {
                QCOMPARE(glyph.character, QChar(u'+'));
                QCOMPARE(glyph.color, ExitAppearance::doorColor(2));
            } else {
                QVERIFY2(qAbs(glyph.at.x() - 1.0f) < scmTolerance || qAbs(glyph.at.x() - 1.5f) < scmTolerance, "a line's character is outside the cells between the rooms");
                QVERIFY(glyph.character.isNull());
                QCOMPARE(glyph.color, mExitColor);
            }
        }
    }

    // A room misplaced far along its exit's line must not make a character for every cell of it, which froze the view
    void test_aGlyphLineTooLongIsDrawnAsABar()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, 1000000, 0);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);
        ExitScene::Options glyphOptions = options();
        glyphOptions.glyphs = true;

        const ExitScene::Scene scene = build(glyphOptions);
        QVERIFY2(scene.glyphs.size() < 10000, qPrintable(qsl("%1 characters for one line").arg(scene.glyphs.size())));
        QVERIFY2(!segmentsIn(scene, mExitColor).isEmpty(), "the line is not drawn at all");
    }

    void test_glyphDoorStaysOnASlantedLine()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, 4, 2);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);
        ExitScene::Options glyphOptions = options();
        glyphOptions.glyphs = true;
        QList<QVector2D> lineCells;
        for (const ExitScene::Glyph& glyph : build(glyphOptions).glyphs) {
            lineCells.append(flat(glyph.at));
        }

        pWest->setDoor(qsl("e"), 1);
        const ExitScene::Scene scene = build(glyphOptions);
        QCOMPARE(scene.glyphs.size(), lineCells.size());
        int doors = 0;
        for (const ExitScene::Glyph& glyph : scene.glyphs) {
            if (glyph.character == QChar(u'+')) {
                ++doors;
                QVERIFY2(std::any_of(lineCells.cbegin(),
                                     lineCells.cend(),
                                     [&glyph](const QVector2D& cell) {
                                         return closeTo(cell, flat(glyph.at));
                                     }),
                         "the door is off its line");
            }
        }
        QCOMPARE(doors, 1);
    }

    void test_glyphStubIsOneCharacterAtItsEnd()
    {
        TRoom* pRoom = addRoomAt(1, mAreaId, 3, 4);
        QVERIFY(pRoom);
        pRoom->setExitStub(DIR_NORTH, true);
        ExitScene::Options glyphOptions = options();
        glyphOptions.glyphs = true;

        const ExitScene::Scene scene = build(glyphOptions);
        QVERIFY(segmentsIn(scene, mExitColor).isEmpty());
        QVERIFY(trianglesIn(scene, mExitColor).isEmpty());
        QCOMPARE(scene.glyphs.size(), 1);
        QVERIFY(closeTo(flat(scene.glyphs.first().at), QVector2D(3, 4.5f)));
        QVERIFY(scene.glyphs.first().character.isNull());
    }

    void test_terminalGlyphMarksThePlayerStairsAndSymbol()
    {
        TRoom* pRoom = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pAbove = addRoomAt(2, mAreaId, 0, 0, 1);
        TRoom* pBelow = addRoomAt(3, mAreaId, 0, 0, -1);
        QVERIFY(pRoom && pAbove && pBelow);
        RoomAppearance::Look look;
        QCOMPARE(RoomAppearance::terminalGlyph(*pRoom, look, false), qsl("#"));
        QCOMPARE(RoomAppearance::terminalGlyph(*pRoom, look, true), qsl("."));
        pRoom->setExit(2, DIR_UP);
        QCOMPARE(RoomAppearance::terminalGlyph(*pRoom, look, false), qsl("<"));
        pRoom->setExitStub(DIR_DOWN, true);
        QCOMPARE(RoomAppearance::terminalGlyph(*pRoom, look, false), qsl("X"));
        pRoom->setExit(-1, DIR_UP);
        QCOMPARE(RoomAppearance::terminalGlyph(*pRoom, look, false), qsl(">"));
        look.symbol = qsl("$");
        QCOMPARE(RoomAppearance::terminalGlyph(*pRoom, look, false), qsl("$"));
        look.playerRoom = true;
        QCOMPARE(RoomAppearance::terminalGlyph(*pRoom, look, false), qsl("@"));
    }

    void test_areaExitPointsOutOfTheArea_data()
    {
        QTest::addColumn<bool>("large");
        QTest::newRow("small arrows") << false;
        QTest::newRow("large arrows") << true;
    }

    void test_areaExitPointsOutOfTheArea()
    {
        QFETCH(bool, large);
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Other Area"));
        TRoom* pHere = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pThere = addRoomAt(2, otherAreaId, 5, 0);
        QVERIFY(pHere && pThere);
        pThere->environment = 4;
        pHere->setExit(2, DIR_EAST);
        pThere->setExit(1, DIR_WEST);

        ExitScene::Options sceneOptions = options();
        sceneOptions.largeAreaExitArrows = large;
        const ExitScene::Scene scene = build(sceneOptions);
        const float reach = large ? 2.0f : 1.0f;

        const QColor color = RoomAppearance::environmentColor(*map(), *mpHost, 4);
        const QList<ExitScene::Segment> lines = segmentsIn(scene, color);
        QCOMPARE(lines.size(), 1);
        QVERIFY(joins(lines.first(), QVector2D(0, 0), QVector2D(reach, 0)));
        const QList<ExitScene::Segment> outline = segmentsIn(scene, ExitAppearance::contrastColor(sceneOptions.background));
        QCOMPARE(outline.size(), 1);
        QVERIFY(outline.first().width > lines.first().width);

        const QList<ExitScene::Triangle> arrow = trianglesIn(scene, color);
        QVERIFY(!arrow.isEmpty());
        QVERIFY(qAbs(furthest(arrow.first(), QVector3D(1, 0, 0)).x() - reach) < scmTolerance);
        QVERIFY2(segmentsIn(scene, mExitColor).isEmpty(), "the exit is drawn as an ordinary line too");

        // What a click on the arrow walks to
        QCOMPARE(scene.areaExits.size(), 1);
        QCOMPARE(scene.areaExits.first().destinationId, 2);
        QCOMPARE(scene.areaExits.first().from, QVector3D(0, 0, 0));
        QCOMPARE(scene.areaExits.first().to, QVector3D(reach, 0, 0));
        QCOMPARE(scene.areaExits.first().level, 0);
    }

    void test_onlyExitsOutOfTheAreaAreClickedThrough()
    {
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Other Area"));
        TRoom* pHere = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pNeighbor = addRoomAt(2, mAreaId, 0, 1);
        TRoom* pThere = addRoomAt(3, otherAreaId, 5, 0);
        QVERIFY(pHere && pNeighbor && pThere);
        pHere->setExit(2, DIR_NORTH);
        // In and out have no direction on the map to point to
        pHere->setExit(3, DIR_IN);
        QVERIFY(build().areaExits.isEmpty());
        pHere->setExit(3, DIR_SOUTH);
        const QList<ExitScene::AreaExit> areaExits = build().areaExits;
        QCOMPARE(areaExits.size(), 1);
        QCOMPARE(areaExits.first().destinationId, 3);
        QCOMPARE(areaExits.first().to, QVector3D(0, -1, 0));
    }

    // Picking asks for the area exits alone, which must be the very ones the drawn scene has
    void test_areaExitsAloneAreTheDrawnScenesAreaExits()
    {
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Other Area"));
        TRoom* pHere = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pNeighbor = addRoomAt(2, mAreaId, 0, 1);
        TRoom* pThere = addRoomAt(3, otherAreaId, 5, 0);
        TRoom* pAbove = addRoomAt(4, mAreaId, 0, 0, 1);
        QVERIFY(pHere && pNeighbor && pThere && pAbove);
        pHere->setExit(2, DIR_NORTH);
        pNeighbor->setExit(1, DIR_SOUTH);
        pHere->setExit(3, DIR_EAST);
        pHere->setExit(3, DIR_IN);
        pHere->exitStubs.append(DIR_WEST);
        pAbove->setExit(3, DIR_SOUTH);
        pNeighbor->setExit(3, DIR_WEST);
        pNeighbor->customLines.insert(qsl("w"), {QPointF(-0.5, 1.0), QPointF(-1.0, 1.0)});
        pNeighbor->customLinesColor.insert(qsl("w"), QColor(Qt::cyan));
        pNeighbor->customLinesStyle.insert(qsl("w"), Qt::SolidLine);

        ExitScene::Options sceneOptions = options();
        sceneOptions.largeAreaExitArrows = true;
        const ExitScene::Scene drawn = build(sceneOptions);
        sceneOptions.areaExitsOnly = true;
        const ExitScene::Scene picked = build(sceneOptions);

        QCOMPARE(drawn.areaExits.size(), 2);
        QCOMPARE(picked.areaExits.size(), drawn.areaExits.size());
        for (qsizetype index = 0; index < drawn.areaExits.size(); ++index) {
            QCOMPARE(picked.areaExits.at(index).destinationId, drawn.areaExits.at(index).destinationId);
            QCOMPARE(picked.areaExits.at(index).from, drawn.areaExits.at(index).from);
            QCOMPARE(picked.areaExits.at(index).to, drawn.areaExits.at(index).to);
            QCOMPARE(picked.areaExits.at(index).level, drawn.areaExits.at(index).level);
        }
        QVERIFY(picked.segments.isEmpty());
        QVERIFY(picked.triangles.isEmpty());
    }

    // An upright arrow's fill stands off its outline's plane on both sides, so that it shows from either
    void test_uprightAreaExitArrowShowsItsFillFromEitherSide()
    {
        const int otherAreaId = map()->mpRoomDB->addArea(qsl("Other Area"));
        TRoom* pHere = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pThere = addRoomAt(2, otherAreaId, 0, 0, 1);
        QVERIFY(pHere && pThere);
        pThere->environment = 4;
        pHere->setExit(2, DIR_UP);

        const ExitScene::Options sceneOptions = options();
        const ExitScene::Scene scene = build(sceneOptions);
        const QList<ExitScene::Triangle> outlines = trianglesIn(scene, ExitAppearance::contrastColor(sceneOptions.background));
        const QList<ExitScene::Triangle> fills = trianglesIn(scene, RoomAppearance::environmentColor(*map(), *mpHost, 4));
        QVERIFY(!outlines.isEmpty() && !fills.isEmpty());
        for (const ExitScene::Triangle& outline : outlines) {
            const QVector3D normal = QVector3D::crossProduct(outline.b - outline.a, outline.c - outline.a).normalized();
            bool front = false;
            bool back = false;
            for (const ExitScene::Triangle& fill : fills) {
                const QVector3D fillNormal = QVector3D::crossProduct(fill.b - fill.a, fill.c - fill.a).normalized();
                if (qAbs(QVector3D::dotProduct(fillNormal, normal)) < 0.99f) {
                    continue;
                }
                const float offset = QVector3D::dotProduct(fill.a - outline.a, normal);
                front = front || offset > scmTolerance / 10.0f;
                back = back || offset < -scmTolerance / 10.0f;
            }
            QVERIFY2(front && back, "the fill shares its outline's plane on one side, where the outline hides it");
        }
    }

    void test_customLineTakesThePlaceOfTheExit()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pEast = addRoomAt(2, mAreaId, 1, 0);
        QVERIFY(pWest && pEast);
        pWest->setExit(2, DIR_EAST);
        pEast->setExit(1, DIR_WEST);
        const QColor cyan(0, 200, 255);
        pWest->customLines.insert(qsl("e"), {QPointF(0.5, 1.0), QPointF(1.0, 1.0)});
        pWest->customLinesColor.insert(qsl("e"), cyan);
        pWest->customLinesStyle.insert(qsl("e"), Qt::DashLine);
        pWest->customLinesArrow.insert(qsl("e"), true);

        const ExitScene::Scene scene = build();
        // The far room still draws its own line back, being two-way
        const QList<ExitScene::Segment> plain = segmentsIn(scene, mExitColor);
        QCOMPARE(plain.size(), 1);
        QVERIFY(joins(plain.first(), QVector2D(0, 0), QVector2D(1, 0)));

        const QList<ExitScene::Segment> dashes = segmentsIn(scene, cyan);
        QVERIFY2(dashes.size() > 3, "the custom line is not dashed");
        // From the room, out half a room east, then through its points
        const float pathLength = 0.5f + (QVector2D(0.5f, 1.0f) - QVector2D(0.5f, 0.0f)).length() + 0.5f;
        QVERIFY(totalLength(dashes) < pathLength * 0.9f);
        const QList<ExitScene::Triangle> arrow = trianglesIn(scene, cyan);
        QVERIFY(!arrow.isEmpty());
        QVERIFY(closeTo(flat(furthest(arrow.first(), QVector3D(1, 0, 0))), QVector2D(1, 1)));
    }

    void test_customLineWithNoPenIsNotDrawn()
    {
        TRoom* pRoom = addRoomAt(1, mAreaId, 0, 0);
        QVERIFY(pRoom && addRoomAt(2, mAreaId, 1, 0));
        pRoom->setExit(2, DIR_EAST);
        const QColor green(0, 255, 0);
        pRoom->customLines.insert(qsl("e"), {QPointF(1.0, 0.0)});
        pRoom->customLinesColor.insert(qsl("e"), green);

        pRoom->customLinesStyle.insert(qsl("e"), Qt::SolidLine);
        const QList<ExitScene::Segment> solid = segmentsIn(build(), green);
        QCOMPARE(solid.size(), 2);
        QCOMPARE(totalLength(solid), 1.0f);

        pRoom->customLinesStyle.insert(qsl("e"), Qt::NoPen);
        QVERIFY(segmentsIn(build(), green).isEmpty());
    }

    void test_customLineEndingOverARoomOnAnotherLevelReachesIt()
    {
        TRoom* pGround = addRoomAt(1, mAreaId, 0, 0, 0);
        TRoom* pTower = addRoomAt(2, mAreaId, 2, 0, 2);
        QVERIFY(pGround && pTower);
        pGround->setSpecialExit(2, qsl("climb"));
        const QColor yellow(255, 255, 0);
        pGround->customLines.insert(qsl("climb"), {QPointF(2.0, 0.0)});
        pGround->customLinesColor.insert(qsl("climb"), yellow);
        pGround->customLinesStyle.insert(qsl("climb"), Qt::SolidLine);

        ExitScene::Options sceneOptions = options();
        sceneOptions.levelsAbove = 2;
        const QList<ExitScene::Segment> lines = segmentsIn(build(sceneOptions), yellow);
        bool upright = false;
        for (const ExitScene::Segment& line : lines) {
            if (closeTo(flat(line.from), QVector2D(2, 0)) && closeTo(flat(line.to), QVector2D(2, 0)) && qAbs(qAbs(line.to.z() - line.from.z()) - 2.0f) < scmTolerance) {
                upright = true;
            }
        }
        QVERIFY2(upright, "no line rises to the room the custom line ends under");

        // One that ends elsewhere stays on its own level
        pGround->customLines.insert(qsl("climb"), {QPointF(1.0, 0.0)});
        for (const ExitScene::Segment& line : segmentsIn(build(sceneOptions), yellow)) {
            QCOMPARE(line.to.z(), line.from.z());
        }
    }

    void test_inAndOutExitsAreStubsOnlyWhenAsked()
    {
        TRoom* pOutside = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pInside = addRoomAt(2, mAreaId, 2, 0);
        QVERIFY(pOutside && pInside);
        pOutside->setExit(2, DIR_IN);
        pInside->setExit(1, DIR_OUT);

        const QList<ExitScene::Segment> full = segmentsIn(build(), mExitColor);
        QCOMPARE(full.size(), 1);
        QVERIFY(joins(full.first(), QVector2D(0, 0), QVector2D(2, 0)));

        ExitScene::Options sceneOptions = options();
        sceneOptions.inOutStubs = true;
        const QList<ExitScene::Segment> stubs = segmentsIn(build(sceneOptions), mExitColor);
        QCOMPARE(stubs.size(), 2);
        for (const ExitScene::Segment& stub : stubs) {
            QCOMPARE((stub.to - stub.from).length(), 0.5f);
        }
    }

    void test_exitsToHiddenRoomsAreLeftOut()
    {
        TRoom* pRoom = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pSecret = addRoomAt(2, mAreaId, 1, 0);
        QVERIFY(pRoom && pSecret);
        pRoom->setExit(2, DIR_EAST);
        pSecret->setExit(1, DIR_WEST);
        pSecret->setHidden(true);

        QVERIFY(build().segments.isEmpty());
    }

    // The markers are the 2D map's, painted on the room's top; a real exit's has the bolder outline of the two
    void test_markerForARealExitIsBolderThanForAStub_data()
    {
        QTest::addColumn<int>("direction");
        QTest::newRow("up") << static_cast<int>(DIR_UP);
        QTest::newRow("down") << static_cast<int>(DIR_DOWN);
        QTest::newRow("in") << static_cast<int>(DIR_IN);
        QTest::newRow("out") << static_cast<int>(DIR_OUT);
    }

    void test_markerForARealExitIsBolderThanForAStub()
    {
        QFETCH(int, direction);
        TRoom* pStubbed = addRoomAt(1, mAreaId, 0, 0);
        TRoom* pExited = addRoomAt(2, mAreaId, 1, 0);
        QVERIFY(pStubbed && pExited);
        pStubbed->setExitStub(direction, true);
        QVERIFY(pExited->setExit(1, direction));

        const QColor roomColor(200, 200, 200);
        const auto outlinePixels = [&roomColor](const TRoom& room) {
            QImage image(128, 128, QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            QPen pen;
            ExitAppearance::paintExitMarkers(painter, pen, room, 64.0f, 64.0f, 128.0, 128.0, roomColor, false);
            painter.end();
            int count = 0;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    count += image.pixelColor(x, y) == QColor(Qt::black) ? 1 : 0;
                }
            }
            return count;
        };
        const int stubPixels = outlinePixels(*pStubbed);
        const int exitPixels = outlinePixels(*pExited);
        QVERIFY2(stubPixels > 0, "no marker for the stub");
        QVERIFY2(exitPixels > stubPixels, qPrintable(qsl("%1 outline pixels for the exit against %2 for the stub").arg(exitPixels).arg(stubPixels)));
    }

    void test_levelsNotShownAreLeftOut()
    {
        TRoom* pRoom = addRoomAt(1, mAreaId, 0, 0, 3);
        QVERIFY(pRoom);
        pRoom->setExitStub(DIR_EAST, true);
        QVERIFY(build().segments.isEmpty());

        ExitScene::Options sceneOptions = options();
        sceneOptions.levelsAbove = 3;
        QCOMPARE(segmentsIn(build(sceneOptions), mExitColor).size(), 1);
    }
};

#include "ExitSceneTest.moc"
MUDLET_GROUPED_TEST_MAIN(ExitSceneTest)
