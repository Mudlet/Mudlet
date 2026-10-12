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

#include <QtTest/QtTest>

#include <optional>

#include "CameraController.h"
#include "GeometryManager.h"

#include "GroupedTest.h"

namespace {
QPointF toScreen(const CameraController& camera, const QVector3D& point)
{
    const QVector3D projected = camera.getProjectionMatrix().map(camera.getViewMatrix().map(point));
    return QPointF(projected.x(), projected.y());
}

CameraController cameraLookingAt(const QVector3D& target)
{
    CameraController camera;
    camera.setViewportSize(800, 600);
    camera.setTarget(target.x(), target.y(), target.z());
    return camera;
}

// How far the screen's up has turned counterclockwise from north, seen from above (the map turns clockwise
// on screen by as much), in -180 to 180
float screenUpFromNorth(const CameraController& camera)
{
    const QVector3D up = camera.getUpVector();
    return qRadiansToDegrees(std::atan2(-up.x(), up.y()));
}
} // namespace

class CameraViewTest : public QObject
{
    Q_OBJECT

private:
    static void verifyNorthUpEastRight(CameraController& camera, const char* view)
    {
        camera.updateMatrices();
        const QVector3D target = camera.getTarget();
        const QPointF center = toScreen(camera, target);
        const QPointF north = toScreen(camera, target + QVector3D(0.0f, 1.0f, 0.0f));
        const QPointF east = toScreen(camera, target + QVector3D(1.0f, 0.0f, 0.0f));
        QVERIFY2(north.y() > center.y(), qPrintable(qsl("north is not up the screen in the %1 view").arg(view)));
        QVERIFY2(qAbs(north.x() - center.x()) < 1e-4, qPrintable(qsl("north is %1 off straight up in the %2 view").arg(north.x() - center.x()).arg(view)));
        QVERIFY2(east.x() > center.x(), qPrintable(qsl("east is not to the right in the %1 view").arg(view)));
        QVERIFY2(qAbs(east.y() - center.y()) < 1e-4, qPrintable(qsl("east is %1 off level in the %2 view").arg(east.y() - center.y()).arg(view)));
    }

    // The texture's bottom left must land at the label's bottom left on screen
    static void verifyLabelReadable(CameraController& camera, const QString& view)
    {
        camera.updateMatrices();
        const QVector3D target = camera.getTarget();
        GeometryManager geometryManager;
        const GeometryData label = geometryManager.generateBillboardGeometry(target.x(), target.y(), target.z(), 4.0f, 1.0f, camera.screenRight(), camera.screenUp(), 0);
        QCOMPARE(label.vertices.size(), 18);
        QCOMPARE(label.textureCoords.size(), 12);

        QPointF textureOrigin;
        QPointF textureFarCorner;
        for (int vertex = 0; vertex < 6; ++vertex) {
            const QVector3D position(label.vertices[vertex * 3], label.vertices[vertex * 3 + 1], label.vertices[vertex * 3 + 2]);
            const float u = label.textureCoords[vertex * 2];
            const float v = label.textureCoords[vertex * 2 + 1];
            if (u == 0.0f && v == 0.0f) {
                textureOrigin = toScreen(camera, position);
            } else if (u == 1.0f && v == 1.0f) {
                textureFarCorner = toScreen(camera, position);
            }
        }
        QVERIFY2(textureOrigin.x() < textureFarCorner.x(), qPrintable(qsl("the label reads mirrored in the %1 view").arg(view)));
        QVERIFY2(textureOrigin.y() < textureFarCorner.y(), qPrintable(qsl("the label is upside down in the %1 view").arg(view)));
    }

    // A room's symbol lies flat on the room, but must still read upright and unmirrored
    static void verifyRoomDecalReadable(CameraController& camera, const QString& view)
    {
        camera.updateMatrices();
        const QVector3D center = camera.getTarget();
        GeometryData decal;
        GeometryManager::appendGroundQuad(decal, center, 0.5f, camera.screenRight(), camera.screenUp(), QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
        QCOMPARE(decal.vertices.size(), 18);
        QCOMPARE(decal.textureCoords.size(), 12);

        std::optional<QPointF> textureOrigin;
        std::optional<QPointF> textureRight;
        std::optional<QPointF> textureFarCorner;
        for (int vertex = 0; vertex < 6; ++vertex) {
            const QVector3D position(decal.vertices[vertex * 3], decal.vertices[vertex * 3 + 1], decal.vertices[vertex * 3 + 2]);
            QCOMPARE(position.z(), center.z());
            const float u = decal.textureCoords[vertex * 2];
            const float v = decal.textureCoords[vertex * 2 + 1];
            if (u == 0.0f && v == 0.0f) {
                textureOrigin = toScreen(camera, position);
            } else if (u == 1.0f && v == 0.0f) {
                textureRight = toScreen(camera, position);
            } else if (u == 1.0f && v == 1.0f) {
                textureFarCorner = toScreen(camera, position);
            }
        }
        QVERIFY2(textureOrigin && textureRight && textureFarCorner, "the room symbol's texture no longer has corners at 0 and 1");
        QVERIFY2(textureOrigin->x() < textureRight->x(), qPrintable(qsl("the room symbol reads mirrored in the %1 view").arg(view)));
        QVERIFY2(qAbs(textureRight->y() - textureOrigin->y()) < qAbs(textureRight->x() - textureOrigin->x()), qPrintable(qsl("the room symbol's baseline is not level in the %1 view").arg(view)));
        QVERIFY2(textureOrigin->y() < textureFarCorner->y(), qPrintable(qsl("the room symbol is upside down in the %1 view").arg(view)));
    }

private slots:
    void test_presetsAreNorthUpWithEastToTheRight()
    {
        CameraController camera = cameraLookingAt(QVector3D(12.0f, -7.0f, 2.0f));
        camera.setDefaultView();
        verifyNorthUpEastRight(camera, "default");
        camera.setTopView();
        verifyNorthUpEastRight(camera, "top");
        camera.setSideView();
        verifyNorthUpEastRight(camera, "side");
    }

    void test_roomSymbolsReadUprightFromAnyAngle()
    {
        CameraController camera = cameraLookingAt(QVector3D(-2.0f, 6.0f, 1.0f));
        camera.setDefaultView();
        verifyRoomDecalReadable(camera, qsl("default"));
        camera.setTopView();
        verifyRoomDecalReadable(camera, qsl("top"));
        camera.setSideView();
        verifyRoomDecalReadable(camera, qsl("side"));
        for (int azimuth = 0; azimuth < 360; azimuth += 45) {
            camera.setOrientation(40.0f, static_cast<float>(azimuth), 0.0f);
            verifyRoomDecalReadable(camera, qsl("turned to %1 degrees").arg(azimuth));
        }
        camera.setOrientation(30.0f, 120.0f, 170.0f);
        verifyRoomDecalReadable(camera, qsl("rolled nearly upside down"));
        // At the steepest tilt a quarter roll leaves the least of the screen's right lying flat on the map
        for (const float roll : {90.0f, -90.0f}) {
            camera.setOrientation(82.0f, 200.0f, roll);
            verifyRoomDecalReadable(camera, qsl("tilted fully and rolled %1 degrees").arg(roll));
        }
    }

    void test_labelsReadLeftToRightFromAnyAngle()
    {
        CameraController camera = cameraLookingAt(QVector3D(3.0f, 4.0f, 0.0f));
        camera.setDefaultView();
        verifyLabelReadable(camera, qsl("default"));
        camera.setTopView();
        verifyLabelReadable(camera, qsl("top"));
        camera.setSideView();
        verifyLabelReadable(camera, qsl("side"));
        camera.setSideView();
        camera.shiftPerspective(20.0f, 0.0f, 0.0f);
        verifyLabelReadable(camera, qsl("side, then tilted"));
        camera.setDefaultView();
        for (int turn = 1; turn <= 8; ++turn) {
            camera.shiftPerspective(7.0f, 45.0f, 11.0f);
            verifyLabelReadable(camera, qsl("default, then turned %1 times").arg(turn));
        }
        camera.setOrientation(30.0f, 120.0f, 170.0f);
        verifyLabelReadable(camera, qsl("rolled nearly upside down"));
    }

    void test_wheelZoomsInTheSameDirectionAsThe2DMap()
    {
        // A wheel turned away from the user (positive) zooms the 2D map in unless zoom is inverted
        QCOMPARE(CameraController::wheelZoomSteps(120, false, false), 1);
        QCOMPARE(CameraController::wheelZoomSteps(-120, false, false), -1);
        QCOMPARE(CameraController::wheelZoomSteps(120, false, true), -1);
        QCOMPARE(CameraController::wheelZoomSteps(240, true, false), 10);
        // Trackpads and smooth-scrolling wheels split a notch into small deltas, each of which must still count
        QCOMPARE(CameraController::wheelZoomSteps(30, false, false), 0.25f);
        CameraController smooth;
        smooth.setScale(2.0f);
        for (int delta = 0; delta < 8; ++delta) {
            smooth.zoomBy(CameraController::wheelZoomSteps(15, false, false));
        }
        CameraController notched;
        notched.setScale(2.0f);
        notched.zoomBy(CameraController::wheelZoomSteps(120, false, false));
        QVERIFY2(qAbs(smooth.getScale() - notched.getScale()) < 1e-4f, qPrintable(qsl("eight 15 unit deltas zoomed to %1, one 120 unit notch to %2").arg(smooth.getScale()).arg(notched.getScale())));

        CameraController camera;
        camera.setScale(2.0f);
        camera.zoomBy(1.0f);
        QVERIFY2(camera.getScale() < 2.0f, "a positive zoom step moved the camera away");
        camera.zoomBy(-2.0f);
        QVERIFY2(camera.getScale() > 2.0f, "a negative zoom step moved the camera closer");
    }

    void test_zoomStaysWithinLimits()
    {
        CameraController camera;
        camera.zoomBy(1000.0f);
        QCOMPARE(camera.getScale(), CameraController::scmMinDistance);
        camera.zoomBy(-1000.0f);
        QCOMPARE(camera.getScale(), CameraController::scmMaxDistance);
        camera.setPosition(0.0f, 45.0f, 270.0f);
        QCOMPARE(camera.getScale(), CameraController::scmMinDistance);
        camera.setPosition(-5.0f, 45.0f, 270.0f);
        QCOMPARE(camera.getScale(), CameraController::scmMinDistance);
    }

    void test_tiltTurnAndRollAreIndependent()
    {
        CameraController camera;
        camera.setOrientation(30.0f, 300.0f, 0.0f);
        camera.setOrientation(camera.getPosition().y(), camera.getPosition().z(), 25.0f);
        QVERIFY2(qAbs(camera.getRoll() - 25.0f) < 0.01f, qPrintable(qsl("roll read back as %1").arg(camera.getRoll())));
        QVERIFY2(qAbs(camera.getPosition().y() - 30.0f) < 0.01f, "setting the roll changed the tilt");
        QVERIFY2(qAbs(std::remainder(camera.getPosition().z() - 300.0f, 360.0f)) < 0.01f, "setting the roll changed the turn");

        camera.setOrientation(60.0f, camera.getPosition().z(), camera.getRoll());
        QVERIFY2(qAbs(camera.getRoll() - 25.0f) < 0.01f, "setting the tilt changed the roll");
        QVERIFY2(qAbs(std::remainder(camera.getPosition().z() - 300.0f, 360.0f)) < 0.01f, "setting the tilt changed the turn");
    }

    // The tilt, turn and roll sliders each set the camera from the other two as read back,
    // so the top view must read back as a tilt the camera can be set to again
    void test_topViewStaysStraightDownWhenTurnedOrRolled()
    {
        CameraController camera;
        camera.setTopView();
        const float straightDown = CameraController::scmMinTilt + 0.01f;
        QVERIFY2(camera.getPosition().y() < straightDown, qPrintable(qsl("the top view reads back a tilt of %1").arg(camera.getPosition().y())));
        camera.setOrientation(camera.getPosition().y(), camera.getPosition().z() + 30.0f, camera.getRoll());
        QVERIFY2(camera.getPosition().y() < straightDown, qPrintable(qsl("turning the top view tilted it to %1").arg(camera.getPosition().y())));
        camera.setOrientation(camera.getPosition().y(), camera.getPosition().z(), 20.0f);
        QVERIFY2(camera.getPosition().y() < straightDown, qPrintable(qsl("rolling the top view tilted it to %1").arg(camera.getPosition().y())));
    }

    // Looking straight down a turn and a roll turn the screen alike, but each must still read back as itself
    void test_rollingTheTopViewSetsTheRollRatherThanAddingToIt()
    {
        CameraController camera;
        camera.setTopView();
        // As the roll slider does while it is dragged
        for (int roll = 1; roll <= 20; ++roll) {
            const QVector3D position = camera.getPosition();
            camera.setOrientation(position.y(), position.z(), static_cast<float>(roll));
        }
        QVERIFY2(qAbs(screenUpFromNorth(camera) - 20.0f) < 0.1f, qPrintable(qsl("dragging the roll to 20 turned the map by %1").arg(screenUpFromNorth(camera))));
        QVERIFY2(qAbs(camera.getRoll() - 20.0f) < 0.1f, qPrintable(qsl("a roll of 20 reads back as %1").arg(camera.getRoll())));

        camera.setOrientation(camera.getPosition().y(), camera.getPosition().z(), 10.0f);
        QVERIFY2(qAbs(screenUpFromNorth(camera) - 10.0f) < 0.1f, qPrintable(qsl("rolling back to 10 left the map turned by %1").arg(screenUpFromNorth(camera))));

        // As the turn slider does
        camera.setOrientation(camera.getPosition().y(), CameraController::scmNorthUpAzimuth + 30.0f, camera.getRoll());
        QVERIFY2(qAbs(camera.getRoll() - 10.0f) < 0.1f, qPrintable(qsl("turning the rolled top view left a roll of %1").arg(camera.getRoll())));
        QVERIFY2(qAbs(screenUpFromNorth(camera) - 40.0f) < 0.1f, qPrintable(qsl("a turn of 30 and a roll of 10 turned the map by %1").arg(screenUpFromNorth(camera))));
    }

    void test_shiftingThePerspectiveStaysAboveTheMap()
    {
        for (const float verticalAngle : {-60.0f, 60.0f, -200.0f, 200.0f}) {
            CameraController camera;
            camera.shiftPerspective(verticalAngle, 0.0f, 0.0f);
            const float tilt = camera.getPosition().y();
            QVERIFY2(tilt >= CameraController::scmMinTilt - 0.01f && tilt <= CameraController::scmMaxTilt + 0.01f,
                     qPrintable(qsl("tilting the default view by %1 left it at %2").arg(verticalAngle).arg(tilt)));
            // Tipped over the top, the camera would read a roll of 180 and show the map upside down
            QVERIFY2(qAbs(camera.getRoll()) < 1.0f, qPrintable(qsl("tilting the default view by %1 left a roll of %2").arg(verticalAngle).arg(camera.getRoll())));
            QVERIFY2(qAbs(screenUpFromNorth(camera)) < 1.0f, qPrintable(qsl("tilting the default view by %1 turned the map by %2").arg(verticalAngle).arg(screenUpFromNorth(camera))));
        }
    }

    void test_tiltingStepByStepStopsLookingStraightDown()
    {
        for (const float startRoll : {0.0f, 30.0f}) {
            CameraController camera;
            camera.setOrientation(CameraController::scmDefaultTilt, CameraController::scmNorthUpAzimuth, startRoll);
            // As a script easing the view toward the top does, a tick at a time
            for (int step = 1; step <= 30; ++step) {
                camera.shiftPerspective(5.0f, 0.0f, 0.0f);
                QVERIFY2(qAbs(camera.getRoll() - startRoll) < 0.5f,
                         qPrintable(qsl("from a roll of %1, step %2 left a roll of %3 at a tilt of %4").arg(startRoll).arg(step).arg(camera.getRoll()).arg(camera.getPosition().y())));
                QVERIFY2(camera.getUpVector().z() > -0.01f, qPrintable(qsl("from a roll of %1, step %2 turned the camera upside down").arg(startRoll).arg(step)));
            }
            QVERIFY2(camera.getPosition().y() < CameraController::scmMinTilt + 0.01f,
                     qPrintable(qsl("from a roll of %1, tilting all the way stopped at %2").arg(startRoll).arg(camera.getPosition().y())));
        }
    }

    void test_turningStepByStepCirclesTheMapAtTheSameTilt()
    {
        for (const float startTilt : {CameraController::scmDefaultTilt, CameraController::scmMinTilt}) {
            for (const float startRoll : {0.0f, 30.0f}) {
                CameraController camera;
                camera.setOrientation(startTilt, CameraController::scmNorthUpAzimuth, startRoll);
                // As a script orbiting the map does, a tick at a time, once round
                for (int step = 1; step <= 72; ++step) {
                    camera.shiftPerspective(0.0f, 5.0f, 0.0f);
                    const QVector3D position = camera.getPosition();
                    const QString where = qsl("from a tilt of %1 and a roll of %2, step %3").arg(startTilt).arg(startRoll).arg(step);
                    QVERIFY2(qAbs(position.y() - startTilt) < 0.1f, qPrintable(qsl("%1 left a tilt of %2").arg(where).arg(position.y())));
                    QVERIFY2(qAbs(camera.getRoll() - startRoll) < 0.5f, qPrintable(qsl("%1 left a roll of %2").arg(where).arg(camera.getRoll())));
                    const float turned = std::remainder(position.z() - CameraController::scmNorthUpAzimuth - 5.0f * static_cast<float>(step), 360.0f);
                    QVERIFY2(qAbs(turned) < 0.5f, qPrintable(qsl("%1 turned %2 off five degrees a step").arg(where).arg(turned)));
                }
            }
        }
    }

    void test_panningLookingStraightDownMovesNorth()
    {
        CameraController camera;
        camera.setTarget(5.0f, 5.0f, 0.0f);
        camera.setTopView();
        QVERIFY2(qAbs(std::remainder(camera.getPosition().z() - CameraController::scmNorthUpAzimuth, 360.0f)) < 0.01f, "the top view does not report a north-up turn");
        camera.translateTargetForward();
        const QVector3D target = camera.getTarget();
        QVERIFY2(std::isfinite(target.x()) && std::isfinite(target.y()), "panning in the top view lost the target");
        QCOMPARE(target.x(), 5.0f);
        QVERIFY2(target.y() > 5.0f, "panning up the screen in the top view did not move north");
    }

    void test_worldUnitsPerPixelMatchesTheScreen()
    {
        CameraController camera = cameraLookingAt(QVector3D(2.0f, 3.0f, 0.0f));
        camera.setViewportSize(800, 600);
        for (const bool top : {true, false}) {
            top ? camera.setTopView() : camera.setDefaultView();
            camera.updateMatrices();
            const QVector3D target = camera.getTarget();
            const float perPixel = camera.worldUnitsPerPixel(target);
            const QPointF center = toScreen(camera, target);
            const QPointF hundredPixels = toScreen(camera, target + camera.screenRight() * (perPixel * 100.0f));
            const qreal pixels = (hundredPixels.x() - center.x()) * 800.0 / 2.0;
            QVERIFY2(qAbs(pixels - 100.0) < 1.0, qPrintable(qsl("%1 pixels rather than 100 in the %2 view").arg(pixels).arg(top ? qsl("top") : qsl("default"))));
        }
    }

    // What the 3D view stacks on a room's top, such as an arrowhead on its line,
    // is lifted by as little as half a hundredth of a room
    void test_depthSeparatesWhatIsStackedOnARoom()
    {
        CameraController camera = cameraLookingAt(QVector3D(0.0f, 0.0f, 0.0f));
        constexpr double depthStep = 1.0 / (1 << 24);
        for (const float distance : {1.0f, 2.0f, 10.0f}) {
            camera.setDefaultView();
            camera.setScale(distance);
            camera.updateMatrices();
            const QMatrix4x4 transform = camera.getProjectionMatrix() * camera.getViewMatrix();
            const QVector3D lower = transform.map(QVector3D(0.0f, 0.0f, 0.0f));
            const QVector3D upper = transform.map(QVector3D(0.0f, 0.0f, 0.005f));
            const double steps = (lower.z() - upper.z()) / 2.0 / depthStep;
            QVERIFY2(steps > 4.0, qPrintable(qsl("only %1 steps of a 24-bit depth buffer apart at distance %2").arg(steps).arg(distance)));
        }
    }

    void test_distanceToShowFitsThatManyRooms()
    {
        CameraController camera = cameraLookingAt(QVector3D(0.0f, 0.0f, 0.0f));
        camera.setViewportSize(600, 600);
        camera.setTopView();
        camera.setScale(CameraController::distanceToShow(20.0f, 1.0f));
        camera.updateMatrices();
        QVERIFY2(qAbs(toScreen(camera, QVector3D(10.0f, 0.0f, 0.0f)).x() - 1.0) < 0.01, "ten rooms east is not at the right edge");
        QVERIFY2(qAbs(toScreen(camera, QVector3D(0.0f, 10.0f, 0.0f)).y() - 1.0) < 0.01, "ten rooms north is not at the top edge");

        // The rooms fit across the shorter side of the viewport
        camera.setViewportSize(800, 600);
        camera.setScale(CameraController::distanceToShow(20.0f, 800.0f / 600.0f));
        camera.updateMatrices();
        QVERIFY2(qAbs(toScreen(camera, QVector3D(0.0f, 10.0f, 0.0f)).y() - 1.0) < 0.01, "ten rooms north is not at the top edge of a wide view");
        camera.setViewportSize(600, 800);
        camera.setScale(CameraController::distanceToShow(20.0f, 600.0f / 800.0f));
        camera.updateMatrices();
        QVERIFY2(qAbs(toScreen(camera, QVector3D(10.0f, 0.0f, 0.0f)).x() - 1.0) < 0.01, "ten rooms east is not at the right edge of a tall view");
    }
};

#include "CameraViewTest.moc"
MUDLET_GROUPED_TEST_MAIN(CameraViewTest)
