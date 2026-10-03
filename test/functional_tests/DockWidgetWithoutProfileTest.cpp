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
 * Run with: ctest -R DockWidgetWithoutProfileTest -V
 */

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QTemporaryDir>
#include <QWidget>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TDockWidget.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

class DockWidgetWithoutProfileTest : public QObject
{
    Q_OBJECT

private:
    // A member, so the directory outlives the profile that is pointed at it:
    // the application object the last test builds is never taken down.
    QTemporaryDir mConfigDir;

private slots:
    // Before #4892 the hide looked the user window up on the profile's main
    // console first, and by then there was no profile to ask.
    void test_aDockWidgetHidesItselfWhenItsProfileIsGone()
    {
        // A parent that is never shown, so no window is ever created for the
        // dock and the hide is the only thing under test here. Declared first,
        // so the dock is destroyed first and unparents itself; the other order
        // is a double free.
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

    // A dock that is resized or moved after its profile has gone used to report
    // the layout change to a profile that was no longer there.
    void test_aDockWidgetTakesAResizeWhenItsProfileIsGone()
    {
        TDockWidget dock(nullptr, qsl("userwindow"));
        QResizeEvent event(QSize(200, 100), QSize(100, 50));

        QVERIFY2(QCoreApplication::sendEvent(&dock, &event), "the resize never reached the dock");
    }

    void test_aDockWidgetTakesAMoveWhenItsProfileIsGone()
    {
        TDockWidget dock(nullptr, qsl("userwindow"));
        QMoveEvent event(QPoint(10, 10), QPoint(0, 0));

        QVERIFY2(QCoreApplication::sendEvent(&dock, &event), "the move never reached the dock");
    }

    // The other half of the same hide guard: the profile is still there but its
    // main console has already gone, which is the state a profile is left in
    // part way through being torn down. Last, because it builds the application
    // object, and a profile cannot be made without one.
    void test_aDockWidgetHidesItselfWhenItsProfilesConsoleIsGone()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        const QByteArray savedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString profileName = qsl("DockWidgetWithoutConsole-Test");
        QVERIFY2(HostManager::self()->addHost(profileName, QString(), QString(), QString()), "failed to put a profile in the pool");
        Host* pHost = HostManager::self()->getHost(profileName);
        QVERIFY(pHost);
        // A profile only gets its main console from the frontend, so one that
        // has not been given one stands in for one whose console has gone.
        QVERIFY2(!pHost->mpConsole, "the profile already has a main console, so the hide never reaches the guard");

        QWidget parent;
        TDockWidget dock(pHost, qsl("userwindow"));
        dock.setParent(&parent);
        dock.QWidget::setVisible(true);
        QVERIFY2(dock.testAttribute(Qt::WA_WState_ExplicitShowHide), "the dock was never put up, so hiding it proves nothing");

        dock.setVisible(false);

        savedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", savedXdg);
        QVERIFY2(dock.isHidden(), "the dock stayed up after its profile's main console went");
    }
};

#include "DockWidgetWithoutProfileTest.moc"
MUDLET_GROUPED_TEST_MAIN(DockWidgetWithoutProfileTest)
