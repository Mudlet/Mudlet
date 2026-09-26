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
 * setPosition() clamps the vertical angle it is handed, so a camera placed
 * from a saved view or a keyboard shortcut cannot end up under the map with
 * the rooms seen from underneath (#8189). shiftPerspective(), which drags the
 * camera round, carries no such clamp.
 *
 * Run with: ctest -R CameraAngleLimitTest -V
 */

#include <QtTest/QtTest>

#include "CameraController.h"

#include "GroupedTest.h"

class CameraAngleLimitTest : public QObject
{
    Q_OBJECT

private slots:
    void test_aCameraPositionCannotBeSetUnderTheMap()
    {
        CameraController camera;

        camera.setPosition(100.0f, 170.0f, 45.0f);

        // getPosition() answers distance, vertical angle and horizontal angle,
        // and a vertical angle past 90 degrees is below the map's plane
        const float verticalAngle = camera.getPosition().y();
        QVERIFY2(verticalAngle < 90.0f, qPrintable(qsl("the camera settled %1 degrees from straight up, which is under the map").arg(verticalAngle)));
    }
};

#include "CameraAngleLimitTest.moc"
MUDLET_GROUPED_TEST_MAIN(CameraAngleLimitTest)
