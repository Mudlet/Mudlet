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
 * The 2D mapper caches the room-ID digit count per area, keyed on the area id
 * and TArea::getRoomsVersion(). Every TArea starts its version at 0, and a map
 * load builds brand new TArea objects, so a map loaded through a path that does
 * not call T2DMap::init() can present the very same (areaId, version) pair as
 * the map that was on screen before it - and be served the previous map's digit
 * count.
 *
 * The two JSON load paths that do exactly that are dlgMapper.cpp's "load map
 * file" and dlgProfilePreferences.cpp's: both call TMap::readJsonMapFile()
 * followed by TMap::audit(), and neither calls init(). This drives that same
 * pair of calls.
 *
 * Run with: ctest -R MapRoomIdDigitsCacheTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QTemporaryDir>

#include "PortableModeTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
const int kWidgetWidth = 640;
const int kWidgetHeight = 480;
const int kAreaId = 1;
} // namespace

class MapRoomIdDigitsCacheTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mMapDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapRoomIdDigitsCache-Test");

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    // Area kAreaId gets five rooms whose ids all start at firstRoomId, so the
    // digit count the mapper should show is the width of the largest of them.
    void buildMap(int firstRoomId)
    {
        map()->mpRoomDB->clearMapDB();
        QVERIFY(roomDB()->addArea(kAreaId, qsl("area-%1").arg(firstRoomId)));
        for (int i = 0; i < 5; ++i) {
            const int roomId = firstRoomId + i;
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomCoordinates(roomId, i * 2, 0, 0));
            QVERIFY(map()->setRoomArea(roomId, kAreaId));
        }
        map()->mRoomIdHash[map()->mProfileName] = firstRoomId;
    }

    T2DMap* mapWidget() const
    {
        TMap* pMap = map();
        if (!pMap->mpMapper) {
            mpHost->showHideOrCreateMapper(false);
        }
        return pMap->mpMapper ? pMap->mpMapper->mp2dMap : nullptr;
    }

    // Everything prepareWidget() in MapLevelOfDetailTest does, minus init() -
    // whether init() runs is the whole point here, so the caller decides.
    void aimWidgetAtArea(T2DMap* p2dMap, int playerRoomId)
    {
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        p2dMap->mRoomID = playerRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = kAreaId;
        p2dMap->mMapCenterX = 0;
        p2dMap->mMapCenterY = 0;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMultiSelectionSet.clear();
        p2dMap->mShowRoomID = true;
        TArea* pArea = roomDB()->getArea(kAreaId);
        if (pArea) {
            pArea->set2DMapZoom(3.0);
        }
    }

    static void renderFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(mMapDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->setShowMapAuditErrors(false);
        deleteProfileDirectory(mProfileName);

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());
    }

    void cleanupTestCase()
    {
        if (mSavedXdg.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdg);
        }
    }

    void digitCountFollowsTheLoadedMap()
    {
        const QString jsonPath = qsl("%1/big-ids.json").arg(mMapDir.path());

        // The map that gets loaded second: room ids 100001..100005, so six
        // digits.
        buildMap(100001);
        map()->audit();
        {
            auto [ok, errorMessage] = map()->writeJsonMapFile(jsonPath);
            QVERIFY2(ok, qPrintable(qsl("could not write the JSON map: %1").arg(errorMessage)));
        }

        // The map on screen first: room ids 1..5, so one digit.
        buildMap(1);
        map()->audit();

        T2DMap* p2dMap = mapWidget();
        QVERIFY(p2dMap);
        p2dMap->init();
        aimWidgetAtArea(p2dMap, 1);
        renderFrame(p2dMap);

        TArea* pSmallArea = roomDB()->getArea(kAreaId);
        QVERIFY(pSmallArea);
        const quint32 smallVersion = pSmallArea->getRoomsVersion();
        const int smallDigits = p2dMap->mMaxRoomIdDigits;
        qInfo().nospace() << "small-id map: rooms version " << smallVersion << ", digit count " << smallDigits;
        QCOMPARE(smallDigits, 1);

        // Exactly what dlgMapper::slot_loadMap() and
        // dlgProfilePreferences::loadMap() do for a .json file - note the
        // absence of any init() call.
        {
            auto [ok, errorMessage] = map()->readJsonMapFile(jsonPath);
            QVERIFY2(ok, qPrintable(qsl("could not read the JSON map back: %1").arg(errorMessage)));
        }
        map()->audit();

        aimWidgetAtArea(p2dMap, 100001);
        renderFrame(p2dMap);

        TArea* pBigArea = roomDB()->getArea(kAreaId);
        QVERIFY(pBigArea);
        const quint32 bigVersion = pBigArea->getRoomsVersion();
        const int bigDigits = p2dMap->mMaxRoomIdDigits;
        qInfo().nospace() << "big-id map: rooms version " << bigVersion << ", digit count " << bigDigits << " (should be 6)";

        // The two maps presenting the same version is what lets the stale
        // digit count survive; report it either way so a failure says why.
        if (smallVersion == bigVersion) {
            qInfo() << "both maps present rooms version" << bigVersion << "for area" << kAreaId << "- the cache key collides";
        }

        QCOMPARE(bigDigits, 6);
    }

    // The same collision without any file at all: delete the area and make a
    // new one, which TRoomDB::createNewAreaID() hands the lowest free id - so
    // the recycled id comes back on a brand new TArea whose version restarts
    // at 0 and reaches 1 again after a single setRoomArea().
    void digitCountFollowsAnAreaThatWasDeletedAndRemade()
    {
        map()->mpRoomDB->clearMapDB();
        QVERIFY(roomDB()->addArea(kAreaId, qsl("small")));
        QVERIFY(map()->addRoom(7));
        QVERIFY(map()->setRoomCoordinates(7, 0, 0, 0));
        QVERIFY(map()->setRoomArea(7, kAreaId));
        map()->mRoomIdHash[map()->mProfileName] = 7;

        T2DMap* p2dMap = mapWidget();
        QVERIFY(p2dMap);
        p2dMap->init();
        aimWidgetAtArea(p2dMap, 7);
        renderFrame(p2dMap);

        TArea* pSmallArea = roomDB()->getArea(kAreaId);
        QVERIFY(pSmallArea);
        const quint32 smallVersion = pSmallArea->getRoomsVersion();
        qInfo().nospace() << "before: area " << kAreaId << " version " << smallVersion << ", digit count " << p2dMap->mMaxRoomIdDigits;
        QCOMPARE(static_cast<int>(p2dMap->mMaxRoomIdDigits), 1);

        // deleteArea() then addAreaName(), the way a script would.
        QVERIFY(roomDB()->removeArea(kAreaId));
        const int remadeAreaId = roomDB()->addArea(qsl("large"));
        QCOMPARE(remadeAreaId, kAreaId);
        QVERIFY(map()->addRoom(999999));
        QVERIFY(map()->setRoomCoordinates(999999, 0, 0, 0));
        QVERIFY(map()->setRoomArea(999999, kAreaId));
        map()->mRoomIdHash[map()->mProfileName] = 999999;

        aimWidgetAtArea(p2dMap, 999999);
        renderFrame(p2dMap);

        TArea* pLargeArea = roomDB()->getArea(kAreaId);
        QVERIFY(pLargeArea);
        const quint32 largeVersion = pLargeArea->getRoomsVersion();
        qInfo().nospace() << "after : area " << kAreaId << " version " << largeVersion << ", digit count " << p2dMap->mMaxRoomIdDigits << " (should be 6)";
        if (smallVersion == largeVersion) {
            qInfo() << "the remade area presents the same rooms version" << largeVersion << "- the cache key collides";
        }

        QCOMPARE(static_cast<int>(p2dMap->mMaxRoomIdDigits), 6);
    }
};

#include "MapRoomIdDigitsCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapRoomIdDigitsCacheTest)
