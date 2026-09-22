/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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
 * A user window's TDockWidget keeps its profile in a QPointer, so closing the
 * profile leaves the dock holding nothing while Qt is still sending it the
 * close and the hide that shutting the profile down produces.
 *
 * Both guards under test have a second half these cases do not reach - the
 * profile still there but its main console already gone - because standing a
 * real Host up and taking its console away is beyond what this fixture can do.
 *
 * Run with: ctest -R DockWidgetWithoutProfileTest -V
 */

#include <QCloseEvent>
#include <QCoreApplication>
#include <QWidget>
#include <QtTest/QtTest>

#include "TDockWidget.h"
#include "utils.h"

#include "GroupedTest.h"

class DockWidgetWithoutProfileTest : public QObject
{
    Q_OBJECT

private slots:
    // Before #4892 the hide looked the user window up on the profile's main
    // console first, and by then there was no profile to ask.
    void test_aDockWidgetHidesItselfWhenItsProfileIsGone()
    {
        // A parent that is never shown, so no window is ever created for the
        // dock and Qt sends it no resize or move - TDockWidget still walks
        // into its profile unguarded in both of those. Declared first, so the
        // dock is destroyed first and unparents itself; the other order is a
        // double free.
        QWidget parent;
        TDockWidget dock(nullptr, qsl("userwindow"));
        dock.setParent(&parent);
        // How the dock went up while there was still a profile behind it.
        dock.QWidget::setVisible(true);
        QVERIFY2(dock.testAttribute(Qt::WA_WState_ExplicitShowHide), "the dock was never put up, so hiding it proves nothing");

        dock.setVisible(false);

        QVERIFY2(dock.isHidden(), "the dock stayed up after its profile went");
    }

    // Before the guard that rode along in #8314 the close asked the profile
    // whether it was shutting down, and there was no profile left to ask.
    void test_aDockWidgetTakesItsCloseWhenItsProfileIsGone()
    {
        TDockWidget dock(nullptr, qsl("userwindow"));
        QCloseEvent event;
        event.ignore();

        QCoreApplication::sendEvent(&dock, &event);

        QVERIFY2(event.isAccepted(), "the close was turned down, with no profile left to hide the user window on");
    }
};

#include "DockWidgetWithoutProfileTest.moc"
MUDLET_GROUPED_TEST_MAIN(DockWidgetWithoutProfileTest)
