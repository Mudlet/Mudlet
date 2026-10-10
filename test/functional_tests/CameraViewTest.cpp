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
        QVERIFY2(qAbs(camera.getPosition().y()) < 0.01f, qPrintable(qsl("the top view reads back a tilt of %1").arg(camera.getPosition().y())));
        camera.setOrientation(camera.getPosition().y(), camera.getPosition().z() + 30.0f, camera.getRoll());
        QVERIFY2(qAbs(camera.getPosition().y()) < 0.01f, qPrintable(qsl("turning the top view tilted it to %1").arg(camera.getPosition().y())));
        camera.setOrientation(camera.getPosition().y(), camera.getPosition().z(), 20.0f);
        QVERIFY2(qAbs(camera.getPosition().y()) < 0.01f, qPrintable(qsl("rolling the top view tilted it to %1").arg(camera.getPosition().y())));
    }

    void test_shiftingThePerspectiveStaysAboveTheMap()
    {
        for (const float verticalAngle : {-60.0f, 60.0f, -200.0f, 200.0f}) {
            CameraController camera;
            camera.shiftPerspective(verticalAngle, 0.0f, 0.0f);
            const float tilt = camera.getPosition().y();
            QVERIFY2(tilt >= CameraController::scmMinTilt - 0.01f && tilt <= CameraController::scmMaxTilt + 0.01f,
                     qPrintable(qsl("tilting the default view by %1 left it at %2").arg(verticalAngle).arg(tilt)));
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
