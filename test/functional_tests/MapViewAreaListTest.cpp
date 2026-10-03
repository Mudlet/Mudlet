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
 * A map view opened with createMapView() lists the areas in its own dropdown,
 * so the Lua functions that add, rename or delete an area have to refresh it as
 * well as the mapper's, and move a view off an area that is deleted.
 *
 * The dropdown is private to the view and has no Lua getter, so this cannot be
 * a busted spec.
 *
 * Run with: ctest -R MapViewAreaListTest -V
 */

#include <QComboBox>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TMapView.h"
#include "TMapViewManager.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapViewAreaListTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapViewAreaList-Test");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TMapViewManager* viewManager() const { return mpHost->mpMap->getViewManager(); }
    bool runLua(const QString& code) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    int addAreaWithRoom(const QString& name, const int roomId)
    {
        const int areaId = roomDB()->addArea(name);
        if (areaId > 0 && map()->addRoom(roomId)) {
            map()->setRoomArea(roomId, areaId);
            map()->setRoomCoordinates(roomId, 0, 0, 0);
        }
        return areaId;
    }

    TMapView* openView(const int areaId) const
    {
        const auto [viewId, message] = viewManager()->createView(areaId);
        return viewId ? viewManager()->getView(viewId) : nullptr;
    }

    static QStringList areaListOf(TMapView* pView)
    {
        QStringList names;
        if (auto* comboBox = pView->findChild<QComboBox*>()) {
            for (int i = 0; i < comboBox->count(); ++i) {
                names << comboBox->itemText(i);
            }
        }
        return names;
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

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());
        QVERIFY(viewManager());
    }

    void cleanupTestCase()
    {
        if (mpHost && viewManager()) {
            viewManager()->closeAllViews();
        }
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_luaAreaChangesReachEveryMapView()
    {
        map()->mapClear();
        const int alpha = addAreaWithRoom(qsl("Alpha"), 1);
        const int doomed = addAreaWithRoom(qsl("Doomed"), 2);
        const int old = roomDB()->addArea(qsl("OldName"));
        QVERIFY(alpha > 0 && doomed > 0 && old > 0);

        QPointer<TMapView> alphaView = openView(alpha);
        QPointer<TMapView> doomedView = openView(doomed);
        QVERIFY(alphaView && doomedView);
        QCOMPARE(doomedView->getCurrentAreaId(), doomed);
        QVERIFY(areaListOf(alphaView).contains(qsl("Doomed")));

        QVERIFY(runLua(qsl("assert(addAreaName('NewArea'))")));
        QVERIFY(runLua(qsl("assert(setAreaName(%1, 'NewName'))").arg(old)));
        // By name, as the id it switches views off has to be looked up first
        QVERIFY(runLua(qsl("assert(deleteArea('Doomed'))")));

        QStringList expected{qsl("Alpha"), qsl("NewArea"), qsl("NewName")};
        if (map()->getDefaultAreaShown()) {
            expected << map()->getDefaultAreaName();
        }
        expected.sort(Qt::CaseInsensitive);
        QCOMPARE(areaListOf(alphaView), expected);
        QCOMPARE(areaListOf(doomedView), expected);
        QCOMPARE(alphaView->getCurrentAreaId(), alpha);
        QVERIFY2(doomedView->getCurrentAreaId() != doomed, "the view is still on the deleted area");
        QVERIFY(roomDB()->getAreaNamesMap().contains(doomedView->getCurrentAreaId()));
    }
};

#include "MapViewAreaListTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapViewAreaListTest)
