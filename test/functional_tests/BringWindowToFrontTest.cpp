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

// Picking a profile in the Window menu brings its window up even when it was minimized, as show() alone leaves it so.

#include <QWidget>
#include <QtTest/QtTest>

#include "mudlet.h"

#include "GroupedTest.h"

class BringWindowToFrontTest : public QObject
{
    Q_OBJECT

private slots:
    void test_minimizedWindowIsRestored()
    {
        QWidget window;
        window.show();
        window.showMinimized();
        QTRY_VERIFY(window.isMinimized());

        mudlet::bringWindowToFront(&window);

        QTRY_VERIFY(!window.isMinimized());
        QVERIFY(window.isVisible());
    }

    void test_minimizedMaximizedWindowStaysMaximized()
    {
        QWidget window;
        window.showMaximized();
        QTRY_VERIFY(window.isMaximized());
        window.setWindowState(window.windowState() | Qt::WindowMinimized);
        QTRY_VERIFY(window.isMinimized());

        mudlet::bringWindowToFront(&window);

        QTRY_VERIFY(!window.isMinimized());
        QVERIFY(window.isMaximized());
    }
};

#include "BringWindowToFrontTest.moc"
MUDLET_GROUPED_TEST_MAIN(BringWindowToFrontTest)
