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

#include <QFileInfo>
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

bool near(const QVector2D& a, const QVector2D& b)
{
    return (a - b).length() < scmTolerance;
}

QVector2D flat(const QVector3D& point)
{
    return point.toVector2D();
}

bool joins(const ExitScene::Segment& segment, const QVector2D& a, const QVector2D& b)
{
    return (near(flat(segment.from), a) && near(flat(segment.to), b)) || (near(flat(segment.from), b) && near(flat(segment.to), a));
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

    // Unless the room at the far end will not draw it: it is on a level not shown
    void test_twoWayExitToALevelNotShownIsStillDrawn()
    {
        TRoom* pLow = addRoomAt(1, mAreaId, 0, 0, 0);
        TRoom* pHigh = addRoomAt(2, mAreaId, 0, 0, 1);
        QVERIFY(pLow && pHigh);
        pLow->setExit(2, DIR_UP);
        pHigh->setExit(1, DIR_DOWN);

        ExitScene::Options sceneOptions = options();
        sceneOptions.levelsAbove = 0;
        const QList<ExitScene::Segment> lines = segmentsIn(build(sceneOptions), mExitColor);
        QCOMPARE(lines.size(), 1);
        QCOMPARE(lines.first().level, 0);
    }

    void test_oneWayExitIsDottedWithAnArrowInTheMiddle()
    {
        TRoom* pWest = addRoomAt(1, mAreaId, 0, 0);
        QVERIFY(addRoomAt(2, mAreaId, 1, 0));
        QVERIFY(pWest);
        pWest->setExit(2, DIR_EAST);

        const ExitScene::Scene scene = build();
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
        QVERIFY(near(flat(furthest(arrow.first(), QVector3D(1, 0, 0))), QVector2D(1, 1)));
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
            if (near(flat(line.from), QVector2D(2, 0)) && near(flat(line.to), QVector2D(2, 0)) && qAbs(qAbs(line.to.z() - line.from.z()) - 2.0f) < scmTolerance) {
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
