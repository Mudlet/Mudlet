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

// Window > Always on Top must change the flag in place: hiding and recreating the native window drops floating docks
// tied to the old one behind it.

#include <QSignalSpy>
#include <QWidget>
#include <QWindow>
#include <QtTest/QtTest>

#include "mudlet.h"

#include "GroupedTest.h"

class WindowStaysOnTopTest : public QObject
{
    Q_OBJECT

private slots:
    void test_togglingKeepsTheNativeWindowAndItsFloatingChildren()
    {
        QWidget window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QWindow* pHandle = window.windowHandle();
        QVERIFY(pHandle);
        const WId nativeId = window.winId();

        QWidget floating(&window, Qt::Tool);
        floating.show();
        QVERIFY(QTest::qWaitForWindowExposed(&floating));
        QCOMPARE(floating.windowHandle()->transientParent(), pHandle);
        QSignalSpy visibilitySpy(pHandle, &QWindow::visibleChanged);

        mudlet::setWindowStaysOnTop(&window, true);
        QCOMPARE(window.winId(), nativeId);
        QVERIFY(window.windowFlags() & Qt::WindowStaysOnTopHint);
        QVERIFY(pHandle->flags() & Qt::WindowStaysOnTopHint);
        QVERIFY(window.isVisible());

        mudlet::setWindowStaysOnTop(&window, false);
        QCOMPARE(window.winId(), nativeId);
        QVERIFY(!(window.windowFlags() & Qt::WindowStaysOnTopHint));
        QVERIFY(!(pHandle->flags() & Qt::WindowStaysOnTopHint));
        QCOMPARE(floating.windowHandle()->transientParent(), pHandle);
        QCOMPARE(visibilitySpy.count(), 0);
    }
};

#include "WindowStaysOnTopTest.moc"
MUDLET_GROUPED_TEST_MAIN(WindowStaysOnTopTest)
