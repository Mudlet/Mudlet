/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * A map view opened with createMapView() draws a TMap it does not own, so it
 * has to be gone by the time the profile frees that map: an event reaching it
 * afterwards reads freed memory (#11869). Closing the profile kills the Lua
 * state, so this cannot be a busted spec.
 *
 * Run with: ctest -R MapViewTeardownTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TMapView.h"
#include "TMapViewManager.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapViewTeardownTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    const QString mProfileName = qsl("MapViewTeardown-Test");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_mapViewDiesWithTheProfile()
    {
        auto* hostManager = HostManager::self();
        QVERIFY(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()));
        Host* pHost = hostManager->getHost(mProfileName);
        QVERIFY(pHost);
        TMap* pMap = pHost->mpMap.data();
        // A Host added without a console gets no view manager from mudlet::addConsoleForNewHost()
        pMap->setViewManager(new TMapViewManager(pHost, pMap));

        const int areaId = pMap->mpRoomDB->addArea(qsl("Scrolled"));
        QVERIFY(areaId > 0);
        QVERIFY(pMap->addRoom(1));
        pMap->setRoomArea(1, areaId);
        pMap->setRoomCoordinates(1, 0, 0, 0);

        const auto [viewId, message] = pMap->getViewManager()->createView(areaId);
        QVERIFY2(viewId, qPrintable(message));
        QPointer<TMapView> view = pMap->getViewManager()->getView(viewId);
        QVERIFY(view);
        QPointer<T2DMap> map2D = view->get2DMap();
        QVERIFY(map2D);

        hostManager->deleteHost(mProfileName);

        // Checked before any event can reach a survivor, which would read the freed TMap
        QVERIFY2(!view, "the map view outlived the profile whose map it draws");
        QVERIFY(!map2D);
    }

    // A sysExitEvent handler can close the views while the profile is being
    // deleted, which leaves them to a deleteLater() that has not run yet
    void test_closedMapViewDiesWithTheProfile()
    {
        auto* hostManager = HostManager::self();
        QVERIFY(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()));
        Host* pHost = hostManager->getHost(mProfileName);
        QVERIFY(pHost);
        TMap* pMap = pHost->mpMap.data();
        pMap->setViewManager(new TMapViewManager(pHost, pMap));

        const auto [viewId, message] = pMap->getViewManager()->createView(0);
        QVERIFY2(viewId, qPrintable(message));
        QPointer<TMapView> view = pMap->getViewManager()->getView(viewId);
        QVERIFY(view);
        QVERIFY(pMap->getViewManager()->closeView(viewId).first);

        hostManager->deleteHost(mProfileName);

        QVERIFY2(!view, "a map view closed but not yet deleted outlived the profile whose map it draws");
    }
};

#include "MapViewTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapViewTeardownTest)
