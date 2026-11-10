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
 * The original 3D map's field of view is 60 times its scale in degrees, and
 * the view empties or flips at 0 and at 180 degrees, so the mouse wheel must
 * stop short of both.
 *
 * Run with: ctest -R LegacyMapZoomTest -V
 */

#include <QtTest/QtTest>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

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
        QCOMPARE(scale, 0.05f);
    }

    void test_zoomingOutStopsShortOfAFlippedView()
    {
        float scale = 1.0f;
        for (int i = 0; i < 500; ++i) {
            scale = GLWidget::scaleAfterWheel(scale, 120);
        }
        QVERIFY2(60.0f * scale < 180.0f, qPrintable(qsl("zooming out left a field of view of %1 degrees").arg(60.0f * scale)));
        QCOMPARE(scale, 2.5f);
    }

    void test_aNotchZoomsByAFixedStep()
    {
        QCOMPARE(GLWidget::scaleAfterWheel(1.0f, -120), 0.97f);
        QCOMPARE(GLWidget::scaleAfterWheel(1.0f, 120), 1.03f);
        QCOMPARE(GLWidget::scaleAfterWheel(0.2f, -120), 0.19f);
    }

    void test_aTouchpadsSmallStepsAddUpToANotch()
    {
        float scale = 1.0f;
        for (int i = 0; i < 8; ++i) {
            scale = GLWidget::scaleAfterWheel(scale, 15);
        }
        QCOMPARE(scale, GLWidget::scaleAfterWheel(1.0f, 120));
    }

    void test_theMapTakesATouchpadsSmallStepButNotASidewaysScroll()
    {
        GLWidget view(nullptr, nullptr);
        auto wheel = [&view](const QPoint angleDelta) {
            QWheelEvent event(QPointF(1, 1), QPointF(1, 1), QPoint(), angleDelta, Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QCoreApplication::sendEvent(&view, &event);
            return event.isAccepted();
        };
        QVERIFY2(wheel(QPoint(0, 15)), "an eighth of a notch from a touchpad was passed on instead of zooming the map");
        QVERIFY2(!wheel(QPoint(120, 0)), "a sideways scroll was kept from the widgets around the map");
    }

    void test_zoomingBeforeTheMapHasAContextLeavesOtherContextsAlone()
    {
        QOffscreenSurface surface;
        surface.create();
        QOpenGLContext other;
        if (!other.create() || !other.makeCurrent(&surface)) {
            QSKIP("no OpenGL context can be made here");
        }
        auto* gl = other.functions();
        gl->glViewport(1, 2, 3, 4);

        GLWidget view(nullptr, nullptr);
        view.slot_setScale(500);
        view.slot_topView();

        QVERIFY(QOpenGLContext::currentContext() == &other);
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        other.doneCurrent();
        QVERIFY2(viewport[0] == 1 && viewport[1] == 2 && viewport[2] == 3 && viewport[3] == 4,
                 qPrintable(qsl("another context's viewport became %1,%2,%3,%4").arg(viewport[0]).arg(viewport[1]).arg(viewport[2]).arg(viewport[3])));
    }
};

#include "LegacyMapZoomTest.moc"
MUDLET_GROUPED_TEST_MAIN(LegacyMapZoomTest)
