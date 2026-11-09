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
 * The original 3D map's field of view is 60 times its scale in degrees: a
 * scale of 0 or below empties the view or turns it upside down, and one
 * towards 3 does the same, so the mouse wheel must stop short of both.
 *
 * Run with: ctest -R LegacyMapZoomTest -V
 */

#include <QtTest/QtTest>

#include "glwidget.h"

#include "GroupedTest.h"

class LegacyMapZoomTest : public QObject
{
    Q_OBJECT

private slots:
    void test_zoomingInStopsShortOfAnEmptyView()
    {
        float scale = 1.0f;
        for (int i = 0; i < 500; ++i) {
            scale = GLWidget::scaleAfterWheel(scale, -120);
        }
        QVERIFY2(scale > 0.0f, qPrintable(qsl("zooming in left a scale of %1, a field of view of %2 degrees").arg(scale).arg(60.0f * scale)));
    }

    void test_zoomingOutStopsShortOfAFlippedView()
    {
        float scale = 1.0f;
        for (int i = 0; i < 500; ++i) {
            scale = GLWidget::scaleAfterWheel(scale, 120);
        }
        QVERIFY2(60.0f * scale < 180.0f, qPrintable(qsl("zooming out left a field of view of %1 degrees").arg(60.0f * scale)));
    }

    void test_aTouchpadsSmallStepsAddUpToANotch()
    {
        float scale = 1.0f;
        for (int i = 0; i < 8; ++i) {
            scale = GLWidget::scaleAfterWheel(scale, 15);
        }
        QCOMPARE(scale, GLWidget::scaleAfterWheel(1.0f, 120));
    }
};

#include "LegacyMapZoomTest.moc"
MUDLET_GROUPED_TEST_MAIN(LegacyMapZoomTest)
