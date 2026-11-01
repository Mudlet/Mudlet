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
 * A user window's TDockWidget is hidden while its profile is being torn down,
 * after the profile's main console has gone.
 *
 * Run with: ctest -R DockWidgetWithoutProfileTest -V
 */

#include <QDir>
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
    // the application object the test builds is never taken down.
    QTemporaryDir mConfigDir;

private slots:
    // The profile is still there but its main console has already gone, which
    // is the state a profile is left in part way through being torn down.
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
        QVERIFY2(!pHost->mainConsoleView(), "the profile already has a main console, so the hide never reaches the guard");

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
