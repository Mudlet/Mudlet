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
 * The 2D mapper sizes the room-ID text from the digit count of the biggest
 * room id in the drawn area, and caches that count against the area id plus
 * TArea::getRoomsVersion(). The version counts from zero on each TArea
 * instance while the key names an area id, so a different TArea that comes to
 * occupy the same id can present a key the cache already holds and be served
 * the previous area's digit count.
 *
 * Both tests here render the same map twice - once through a path that resets
 * the cache and once through a path that does not - and require the two frames
 * to match. mMaxRoomIdDigits is private, so the digit count is observed the
 * way a user would: in the pixels.
 *
 * Run with: ctest -R MapRoomIdDigitsCacheTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QImage>
#include <QPixmap>
#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "SettingsTestHelper.h"
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
// Big enough on screen that a six digit room id is still legible, so the
// digit count actually reaches the pixels.
const double kZoom = 40.0;
const int kSmallRoomId = 7;
const int kBigRoomId = 999999;
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

    // One area holding one room, so the only thing that varies between the
    // maps built here is the width of the room id.
    void buildMap(int roomId)
    {
        map()->mpRoomDB->clearMapDB();
        QVERIFY(roomDB()->addArea(kAreaId, qsl("area-%1").arg(roomId)));
        QVERIFY(map()->addRoom(roomId));
        QVERIFY(map()->setRoomCoordinates(roomId, 0, 0, 0));
        QVERIFY(map()->setRoomArea(roomId, kAreaId));
        map()->mRoomIdHash[map()->mProfileName] = roomId;
    }

    T2DMap* mapWidget() const
    {
        TMap* pMap = map();
        if (!pMap->mpMapper) {
            mpHost->showHideOrCreateMapper(false);
        }
        return pMap->mpMapper ? pMap->mpMapper->mp2dMap : nullptr;
    }

    // What prepareWidget() in MapLevelOfDetailTest sets up, minus init() -
    // whether init() runs is the whole point here, so the caller decides.
    void aimWidgetAtArea(T2DMap* p2dMap, int playerRoomId, bool showRoomIds = true)
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
        p2dMap->mShowRoomID = showRoomIds;
        TArea* pArea = roomDB()->getArea(kAreaId);
        if (pArea) {
            pArea->set2DMapZoom(kZoom);
        }
    }

    void setDeterministicRendering()
    {
        mpHost->mRoomSize = 1.0;
        mpHost->mRoomBorderSize = 0.5;
        mpHost->mLineSize = 10.0;
        mpHost->mMapperUseAntiAlias = false;
        mpHost->mMapperShowGrid = false;
        mpHost->mMapperShowRoomBorders = false;
        mpHost->mMapperCenterSmallAreas = false;
        mpHost->mBgColor_2 = QColor(0, 0, 0);
        mpHost->mFgColor_2 = QColor(255, 255, 255);
        mpHost->mMapInfoContributors.clear();
    }

    static QImage renderFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        return target.toImage();
    }

    // Renders the big-id map with the cache freshly reset, which is the answer
    // the other renders have to match. Also proves the room id reaches the
    // pixels at all, so a later comparison cannot pass by drawing nothing.
    QImage captureCorrectBigIdFrame(T2DMap* p2dMap, bool& ok)
    {
        buildMap(kBigRoomId);
        map()->audit();
        p2dMap->init();
        aimWidgetAtArea(p2dMap, kBigRoomId);
        const QImage withIds = renderFrame(p2dMap);

        p2dMap->init();
        aimWidgetAtArea(p2dMap, kBigRoomId, false);
        const QImage withoutIds = renderFrame(p2dMap);
        ok = (withIds != withoutIds);
        return withIds;
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
        setDeterministicRendering();
    }

    void cleanupTestCase()
    {
        if (mSavedXdg.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdg);
        }
    }

    // dlgMapper::slot_loadMap() and dlgProfilePreferences::loadMap() both do
    // readJsonMapFile() then audit() for a .json file, and neither calls
    // T2DMap::init().
    void jsonMapLoadResizesTheRoomIdText()
    {
        T2DMap* p2dMap = mapWidget();
        QVERIFY(p2dMap);

        bool idsAreDrawn = false;
        const QImage correctFrame = captureCorrectBigIdFrame(p2dMap, idsAreDrawn);
        QVERIFY2(idsAreDrawn, "the room id is not being drawn, so this test could not see a wrong digit count");

        // Write the big-id map out, so it can be loaded back later.
        const QString jsonPath = qsl("%1/big-ids.json").arg(mMapDir.path());
        {
            auto [ok, errorMessage] = map()->writeJsonMapFile(jsonPath);
            QVERIFY2(ok, qPrintable(qsl("could not write the JSON map: %1").arg(errorMessage)));
        }

        // Put the small-id map on screen so the cache holds its digit count.
        buildMap(kSmallRoomId);
        map()->audit();
        p2dMap->init();
        aimWidgetAtArea(p2dMap, kSmallRoomId);
        renderFrame(p2dMap);
        const quint32 smallVersion = roomDB()->getArea(kAreaId)->getRoomsVersion();

        // Now the load the two dialogs perform.
        {
            auto [ok, errorMessage] = map()->readJsonMapFile(jsonPath);
            QVERIFY2(ok, qPrintable(qsl("could not read the JSON map back: %1").arg(errorMessage)));
        }
        map()->audit();

        aimWidgetAtArea(p2dMap, kBigRoomId);
        const QImage loadedFrame = renderFrame(p2dMap);
        const quint32 bigVersion = roomDB()->getArea(kAreaId)->getRoomsVersion();

        qInfo().nospace() << "area " << kAreaId << " rooms version: " << smallVersion << " before the load, " << bigVersion << " after";
        if (smallVersion == bigVersion) {
            qInfo() << "both maps present the same version for the same area id, so the cache key collides";
        }

        QVERIFY2(loadedFrame == correctFrame, "the room ids of the loaded map are drawn at the previous map's digit width");
    }

    // Area ids are recycled - TRoomDB::createNewAreaID() hands back the lowest
    // free one - so deleting an area and making another gets the same id on a
    // new TArea whose version restarts at zero.
    void remakingAnAreaResizesTheRoomIdText()
    {
        T2DMap* p2dMap = mapWidget();
        QVERIFY(p2dMap);

        bool idsAreDrawn = false;
        const QImage correctFrame = captureCorrectBigIdFrame(p2dMap, idsAreDrawn);
        QVERIFY2(idsAreDrawn, "the room id is not being drawn, so this test could not see a wrong digit count");

        // Small-id map on screen, so the cache holds a one-digit count.
        buildMap(kSmallRoomId);
        p2dMap->init();
        aimWidgetAtArea(p2dMap, kSmallRoomId);
        renderFrame(p2dMap);
        const quint32 smallVersion = roomDB()->getArea(kAreaId)->getRoomsVersion();

        // deleteArea() then addAreaName(), the way a script would.
        QVERIFY(roomDB()->removeArea(kAreaId));
        QCOMPARE(roomDB()->addArea(qsl("remade")), kAreaId);
        QVERIFY(map()->addRoom(kBigRoomId));
        QVERIFY(map()->setRoomCoordinates(kBigRoomId, 0, 0, 0));
        QVERIFY(map()->setRoomArea(kBigRoomId, kAreaId));
        map()->mRoomIdHash[map()->mProfileName] = kBigRoomId;

        aimWidgetAtArea(p2dMap, kBigRoomId);
        const QImage remadeFrame = renderFrame(p2dMap);
        const quint32 remadeVersion = roomDB()->getArea(kAreaId)->getRoomsVersion();

        qInfo().nospace() << "area " << kAreaId << " rooms version: " << smallVersion << " before it was remade, " << remadeVersion << " after";
        if (smallVersion == remadeVersion) {
            qInfo() << "the remade area presents the same version as the one it replaced, so the cache key collides";
        }

        QVERIFY2(remadeFrame == correctFrame, "the room ids of the remade area are drawn at the previous area's digit width");
    }
};

#include "MapRoomIdDigitsCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapRoomIdDigitsCacheTest)
