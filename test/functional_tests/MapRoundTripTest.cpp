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
 * Round-trip tests for the binary map format (TMap::serialize -> restore).
 *
 * A small map is built in code on a source profile: two areas, rooms with
 * normal exits, exit locks, exit stubs, special exits (locked and unlocked),
 * custom exit lines, doors, exit weights, room/area/map-level userData, room
 * environments, non-ASCII names/symbols, negative coordinates and z-levels
 * plus an area map label. It is saved with TMap::serialize (the same
 * QDataStream setup TMainConsole::saveMap uses), loaded into a fresh Host's
 * map via TMap::restore + audit (the production load path minus the mapper
 * UI) and compared field by field.
 *
 * The map is additionally saved at every user-selectable older format
 * version (TMap::mMinVersion .. mDefaultVersion) and each file is verified
 * to load back correctly.
 *
 * Run with: ctest -R MapRoundTripTest -V
 */

#include <QtTest/QtTest>

#include <QSaveFile>
#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForMapRoundTripTest();

namespace {
const int scmRoom1 = 101;
const int scmRoom2 = 102;
const int scmRoom3 = 103;
const int scmRoom4 = 104;

const QString scmAreaAName = qsl("日本語エリア <&\"'>");
const QString scmAreaBName = qsl("émoji🎉 zone");
const QString scmRoom1Name = qsl("起点の部屋 <start> & \"here\"");
const QString scmRoom3Name = qsl("隠し部屋 émoji🎉");
const QString scmSpecialExitLocked = qsl("拉动 <lever> & \"push\" ]]>");
// Deliberately starts with a '1' as the pre-version-21 format encodes the
// special exit lock state as a '0'/'1' prefix on the command:
const QString scmSpecialExitUnlocked = qsl("1press <plate>");
const QString scmLabelText = qsl("ラベル <text> & \"stuff\"");
const QColor scmLabelFg(200, 50, 50);
const QColor scmLabelBg(20, 20, 60);
const QColor scmLabelOutline(1, 2, 3);
const QColor scmSymbolColor(200, 100, 50);
const QColor scmBorderColor(10, 200, 30, 128);
const QColor scmCustomLineColor(10, 20, 30);
} // namespace

class MapRoundTripTest : public QObject
{
    Q_OBJECT

private:
    Host* mpSource = nullptr;
    Host* mpTarget = nullptr;
    const QString mSourceName = qsl("MapRoundTripSource-Test");
    const QString mTargetName = qsl("MapRoundTripTarget-Test");
    QTemporaryDir mSaveDir;

    int mAreaA = -1;
    int mAreaB = -1;
    int mLabelId = -1;

    // Derived per-area values captured from the source map after building it;
    // TMap::restore() recalculates these via TArea::calcSpan() so the loaded
    // map must reproduce them exactly:
    struct AreaBounds
    {
        QSet<int> rooms;
        QList<int> zLevels;
        int minX = 0;
        int maxX = 0;
        int minY = 0;
        int maxY = 0;
        int minZ = 0;
        int maxZ = 0;
    };
    AreaBounds mBoundsA;
    AreaBounds mBoundsB;
    QImage mLabelImage;
    QSizeF mLabelSize;

    static QMap<QString, QString> expectedMapUserData() { return {{qsl("map.author 日本語"), qsl("величина <>&\"' ]]>")}, {qsl("plain"), qsl("value")}}; }

    static QMap<QString, QString> expectedAreaAUserData() { return {{qsl("zone.type"), qsl("都市 <city> & \"stuff\"")}, {qsl("custom"), qsl("value ]]>")}}; }

    static QMap<QString, QString> expectedRoom1UserData() { return {{qsl("quest.stage 日本語"), qsl("величина <>&\"' ]]>")}, {qsl("plain"), qsl("value")}}; }

    static QMap<QString, QString> expectedRoom2UserData()
    {
        // The border presentation properties travel inside userData (see
        // TMap::serialize()/TRoom::restore()) so they are part of the
        // expected representation on both sides of the round trip:
        return {{qsl("note"), qsl("second <room> & 'data'")}, {ROOM_UI_BORDERCOLOR, scmBorderColor.name(QColor::HexArgb)}, {ROOM_UI_BORDERTHICKNESS, qsl("4")}};
    }

    static AreaBounds captureBounds(TArea* pArea)
    {
        pArea->calcSpan();
        AreaBounds bounds;
        bounds.rooms = pArea->rooms;
        bounds.zLevels = pArea->zLevels;
        bounds.minX = pArea->min_x;
        bounds.maxX = pArea->max_x;
        bounds.minY = pArea->min_y;
        bounds.maxY = pArea->max_y;
        bounds.minZ = pArea->min_z;
        bounds.maxZ = pArea->max_z;
        return bounds;
    }

    void buildSourceMap()
    {
        TMap* pMap = mpSource->mpMap.data();
        TRoomDB* pDB = pMap->mpRoomDB.get();

        mAreaA = pDB->addArea(scmAreaAName);
        QVERIFY(mAreaA > 0);
        mAreaB = pDB->addArea(scmAreaBName);
        QVERIFY(mAreaB > 0);

        TArea* pAreaA = pDB->getArea(mAreaA);
        QVERIFY(pAreaA);
        pAreaA->gridMode = true;
        pAreaA->mUserData = expectedAreaAUserData();

        TArea* pAreaB = pDB->getArea(mAreaB);
        QVERIFY(pAreaB);
        pAreaB->isZone = true;
        pAreaB->zoneAreaRef = mAreaA;

        for (const int id : {scmRoom1, scmRoom2, scmRoom3, scmRoom4}) {
            QVERIFY(pMap->addRoom(id));
            QVERIFY(pMap->setRoomArea(id, id == scmRoom4 ? mAreaB : mAreaA, false));
        }
        QVERIFY(pMap->setRoomCoordinates(scmRoom1, 0, 0, 0));
        QVERIFY(pMap->setRoomCoordinates(scmRoom2, -3, 7, 0));
        QVERIFY(pMap->setRoomCoordinates(scmRoom3, 2, -5, -3));
        QVERIFY(pMap->setRoomCoordinates(scmRoom4, 0, 0, 5));

        QVERIFY(pMap->setExit(scmRoom1, scmRoom2, DIR_NORTH));
        QVERIFY(pMap->setExit(scmRoom1, scmRoom3, DIR_SOUTH));
        QVERIFY(pMap->setExit(scmRoom1, scmRoom4, DIR_UP));
        QVERIFY(pMap->setExit(scmRoom3, scmRoom1, DIR_WEST));
        QVERIFY(pMap->setExit(scmRoom4, scmRoom1, DIR_IN));
        QVERIFY(pMap->setExit(scmRoom4, scmRoom2, DIR_OUT));
        QVERIFY(pMap->setExit(scmRoom4, scmRoom3, DIR_NORTHWEST));

        TRoom* pR1 = pDB->getRoom(scmRoom1);
        QVERIFY(pR1);
        pR1->name = scmRoom1Name;
        pR1->environment = 300;
        pR1->setWeight(3);
        pR1->mSymbol = qsl("⚔");
        pR1->mSymbolColor = scmSymbolColor;
        pR1->setExitLock(DIR_NORTH, true);
        QVERIFY(pR1->setDoor(qsl("n"), 2));
        pR1->setExitWeight(qsl("n"), 7);
        pR1->userData = expectedRoom1UserData();
        pR1->customLines.insert(qsl("n"), {QPointF(1.5, -2.5), QPointF(3.0, 4.0)});
        pR1->customLinesColor.insert(qsl("n"), scmCustomLineColor);
        pR1->customLinesStyle.insert(qsl("n"), Qt::DashLine);
        pR1->customLinesArrow.insert(qsl("n"), true);

        TRoom* pR2 = pDB->getRoom(scmRoom2);
        QVERIFY(pR2);
        pR2->environment = 12;
        pR2->setWeight(2);
        pR2->isLocked = true;
        pR2->setExitStub(DIR_EAST, true);
        pR2->setExitStub(DIR_SOUTHWEST, true);
        pR2->setSpecialExit(scmRoom1, scmSpecialExitLocked);
        QVERIFY(pR2->setSpecialExitLock(scmSpecialExitLocked, true));
        pR2->setSpecialExit(scmRoom3, scmSpecialExitUnlocked);
        QVERIFY(pR2->setDoor(scmSpecialExitLocked, 3));
        pR2->mBorderColor = scmBorderColor;
        pR2->mBorderThickness = 4;
        pR2->userData = expectedRoom2UserData();

        TRoom* pR3 = pDB->getRoom(scmRoom3);
        QVERIFY(pR3);
        pR3->name = scmRoom3Name;
        pR3->environment = 5;
        pR3->setWeight(10);
        pR3->setHidden(true);

        TRoom* pR4 = pDB->getRoom(scmRoom4);
        QVERIFY(pR4);
        pR4->setWeight(1);

        mLabelId = pMap->createMapLabel(mAreaA, scmLabelText, 1.5f, -2.5f, 0.0f, scmLabelFg, scmLabelBg, true, false, false, 30.0, 12, qsl("DejaVu Sans"), scmLabelOutline);
        QVERIFY(mLabelId >= 0);
        const TMapLabel sourceLabel = pAreaA->mMapLabels.value(mLabelId);
        mLabelImage = sourceLabel.pix.toImage().convertToFormat(QImage::Format_ARGB32);
        mLabelSize = sourceLabel.size;

        pMap->mUserData = expectedMapUserData();
        pMap->mEnvColors[5] = 2;
        pMap->mEnvColors[12] = 7;
        // Ids 257-272 are reserved for the 16 ANSI colors and get rewritten
        // from the Host palette on every load (TMap::restore16ColorSet()), so
        // only ids outside that range are expected to round-trip:
        pMap->mCustomEnvColors[300] = QColor(12, 34, 56);
        pMap->mCustomEnvColors[301] = QColor(255, 0, 255);
        pMap->mRoomIdHash[mSourceName] = scmRoom1;
        pMap->mRoomIdHash[qsl("OtherProfile")] = scmRoom3;
        pDB->hashToRoomID[qsl("hash-abc")] = scmRoom1;
        pDB->hashToRoomID[qsl("hash-日本語")] = scmRoom2;
        pMap->mMapSymbolFont = QFont(qsl("DejaVu Serif"), 14);
        pMap->mMapSymbolFontFudgeFactor = 1.25;
        pMap->mIsOnlyMapSymbolFontToBeUsed = true;

        mBoundsA = captureBounds(pAreaA);
        mBoundsB = captureBounds(pAreaB);
    }

    bool saveMapToFile(TMap* pMap, const QString& fileName, int saveVersion)
    {
        QSaveFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QDataStream out(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        if (!pMap->serialize(out, saveVersion)) {
            return false;
        }
        return file.commit();
    }

    void verifyArea(TArea* pArea, const AreaBounds& bounds, const QString& areaLabel)
    {
        QVERIFY2(pArea, qPrintable(qsl("%1 is missing").arg(areaLabel)));
        QCOMPARE(pArea->rooms, bounds.rooms);
        QCOMPARE(pArea->zLevels, bounds.zLevels);
        QCOMPARE(pArea->min_x, bounds.minX);
        QCOMPARE(pArea->max_x, bounds.maxX);
        QCOMPARE(pArea->min_y, bounds.minY);
        QCOMPARE(pArea->max_y, bounds.maxY);
        QCOMPARE(pArea->min_z, bounds.minZ);
        QCOMPARE(pArea->max_z, bounds.maxZ);
    }

    void verifyMap(TMap* pMap, int savedVersion, const QString& ownProfileKey)
    {
        QCOMPARE(pMap->mVersion, savedVersion);
        TRoomDB* pDB = pMap->mpRoomDB.get();

        QCOMPARE(pDB->getAreaNamesMap().value(mAreaA), scmAreaAName);
        QCOMPARE(pDB->getAreaNamesMap().value(mAreaB), scmAreaBName);

        TArea* pAreaA = pDB->getArea(mAreaA);
        verifyArea(pAreaA, mBoundsA, qsl("area A"));
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(pAreaA->gridMode);
        QCOMPARE(pAreaA->mUserData, expectedAreaAUserData());

        TArea* pAreaB = pDB->getArea(mAreaB);
        verifyArea(pAreaB, mBoundsB, qsl("area B"));
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(pAreaB->isZone);
        QCOMPARE(pAreaB->zoneAreaRef, mAreaA);

        QCOMPARE(pAreaA->mMapLabels.size(), 1);
        QVERIFY(pAreaA->mMapLabels.contains(mLabelId));
        const TMapLabel label = pAreaA->mMapLabels.value(mLabelId);
        QCOMPARE(label.text, scmLabelText);
        QCOMPARE(label.pos, QVector3D(1.5f, -2.5f, 0.0f));
        QCOMPARE(label.size, mLabelSize);
        QCOMPARE(label.fgColor, scmLabelFg);
        QCOMPARE(label.bgColor, scmLabelBg);
        QCOMPARE(label.outlineColor, scmLabelOutline);
        QVERIFY(label.showOnTop);
        QVERIFY(!label.noScaling);
        QCOMPARE(label.font.family(), qsl("DejaVu Sans"));
        QCOMPARE(label.font.pointSize(), 12);
        QCOMPARE(label.font.weight(), QFont(qsl("DejaVu Sans"), 12).weight());
        QVERIFY(!label.font.italic());
        QCOMPARE(label.pix.toImage().convertToFormat(QImage::Format_ARGB32), mLabelImage);

        TRoom* pR1 = pDB->getRoom(scmRoom1);
        QVERIFY(pR1);
        QCOMPARE(pR1->getArea(), mAreaA);
        QCOMPARE(pR1->x(), 0);
        QCOMPARE(pR1->y(), 0);
        QCOMPARE(pR1->z(), 0);
        QCOMPARE(pR1->name, scmRoom1Name);
        QCOMPARE(pR1->environment, 300);
        QCOMPARE(pR1->getWeight(), 3);
        QCOMPARE(pR1->mSymbol, qsl("⚔"));
        QCOMPARE(pR1->mSymbolColor, scmSymbolColor);
        QCOMPARE(pR1->getNorth(), scmRoom2);
        QCOMPARE(pR1->getSouth(), scmRoom3);
        QCOMPARE(pR1->getUp(), scmRoom4);
        QCOMPARE(pR1->getEast(), -1);
        QCOMPARE((QSet<int>{pR1->exitLocks.begin(), pR1->exitLocks.end()}), QSet<int>{DIR_NORTH});
        QCOMPARE(pR1->doors, (QMap<QString, int>{{qsl("n"), 2}}));
        QCOMPARE(pR1->getExitWeights(), (QMap<QString, int>{{qsl("n"), 7}}));
        QCOMPARE(pR1->customLines, (QMap<QString, QList<QPointF>>{{qsl("n"), {QPointF(1.5, -2.5), QPointF(3.0, 4.0)}}}));
        QCOMPARE(pR1->customLinesColor, (QMap<QString, QColor>{{qsl("n"), scmCustomLineColor}}));
        QCOMPARE(pR1->customLinesStyle, (QMap<QString, Qt::PenStyle>{{qsl("n"), Qt::DashLine}}));
        QCOMPARE(pR1->customLinesArrow, (QMap<QString, bool>{{qsl("n"), true}}));
        // The format 19 leg runs after the format 17/18 ones, so this also
        // guards against a < 19 save leaving a stray system.fallback_symbol
        // entry behind in the live source room's user data:
        QCOMPARE(pR1->userData, expectedRoom1UserData());

        TRoom* pR2 = pDB->getRoom(scmRoom2);
        QVERIFY(pR2);
        QCOMPARE(pR2->getArea(), mAreaA);
        QCOMPARE(pR2->x(), -3);
        QCOMPARE(pR2->y(), 7);
        QCOMPARE(pR2->z(), 0);
        QCOMPARE(pR2->environment, 12);
        QCOMPARE(pR2->getWeight(), 2);
        QVERIFY(pR2->isLocked);
        // TRoomDB::auditRooms() deduplicates exit stubs by passing them
        // through a QSet, which does not preserve order - so only the
        // membership is stable across a load:
        QCOMPARE((QSet<int>{pR2->exitStubs.begin(), pR2->exitStubs.end()}), (QSet<int>{DIR_EAST, DIR_SOUTHWEST}));
        QCOMPARE(pR2->getSpecialExits(), (QMap<QString, int>{{scmSpecialExitLocked, scmRoom1}, {scmSpecialExitUnlocked, scmRoom3}}));
        QCOMPARE(pR2->getSpecialExitLocks(), QSet<QString>{scmSpecialExitLocked});
        QCOMPARE(pR2->doors, (QMap<QString, int>{{scmSpecialExitLocked, 3}}));
        QCOMPARE(pR2->mBorderColor, scmBorderColor);
        QCOMPARE(pR2->mBorderThickness, 4);
        QCOMPARE(pR2->userData, expectedRoom2UserData());

        TRoom* pR3 = pDB->getRoom(scmRoom3);
        QVERIFY(pR3);
        QCOMPARE(pR3->getArea(), mAreaA);
        QCOMPARE(pR3->x(), 2);
        QCOMPARE(pR3->y(), -5);
        QCOMPARE(pR3->z(), -3);
        QCOMPARE(pR3->name, scmRoom3Name);
        QCOMPARE(pR3->environment, 5);
        QCOMPARE(pR3->getWeight(), 10);
        QVERIFY(pR3->isHidden());
        QCOMPARE(pR3->getWest(), scmRoom1);

        TRoom* pR4 = pDB->getRoom(scmRoom4);
        QVERIFY(pR4);
        QCOMPARE(pR4->getArea(), mAreaB);
        QCOMPARE(pR4->z(), 5);
        QCOMPARE(pR4->getIn(), scmRoom1);
        QCOMPARE(pR4->getOut(), scmRoom2);
        QCOMPARE(pR4->getNorthwest(), scmRoom3);

        // The format 19 leg runs after the format 17/18 ones, so this also
        // guards against a < 19 save leaving stray
        // system.fallback_mapSymbolFont* entries behind in the live source
        // map's user data:
        QCOMPARE(pMap->mUserData, expectedMapUserData());
        QCOMPARE(pMap->mEnvColors, (QMap<int, int>{{5, 2}, {12, 7}}));
        QCOMPARE(pMap->mCustomEnvColors.value(300), QColor(12, 34, 56));
        QCOMPARE(pMap->mCustomEnvColors.value(301), QColor(255, 0, 255));
        QCOMPARE(pDB->hashToRoomID, (QMap<QString, int>{{qsl("hash-abc"), scmRoom1}, {qsl("hash-日本語"), scmRoom2}}));
        if (savedVersion >= 18) {
            // Since format 18 the player location is stored per profile:
            QCOMPARE(pMap->mRoomIdHash, (QHash<QString, int>{{mSourceName, scmRoom1}, {qsl("OtherProfile"), scmRoom3}}));
        } else {
            // Format 17 only stores the saving profile's own location and the
            // loader files it under the loading profile's name:
            QCOMPARE(pMap->mRoomIdHash.value(ownProfileKey), scmRoom1);
        }
        QCOMPARE(pMap->mMapSymbolFont.family(), qsl("DejaVu Serif"));
        QCOMPARE(pMap->mMapSymbolFont.pointSize(), 14);
        QCOMPARE(pMap->mMapSymbolFontFudgeFactor, 1.25);
        QVERIFY(pMap->mIsOnlyMapSymbolFontToBeUsed);
    }

    void roundTripAtVersion(int saveVersion)
    {
        const QString fileName = qsl("%1/map_v%2.dat").arg(mSaveDir.path()).arg(saveVersion);
        QVERIFY2(saveMapToFile(mpSource->mpMap.data(), fileName, saveVersion), qPrintable(qsl("failed to save map at format version %1").arg(saveVersion)));

        TMap* pTargetMap = mpTarget->mpMap.data();
        pTargetMap->mapClear();
        QVERIFY2(pTargetMap->restore(fileName), qPrintable(qsl("failed to restore map saved at format version %1").arg(saveVersion)));
        pTargetMap->audit();

        verifyMap(pTargetMap, saveVersion, mTargetName);
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForMapRoundTripTest();

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mSourceName);
        deleteProfileDirectory(mTargetName);

        QVERIFY(mSaveDir.isValid());

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mSourceName, qsl("23"), QString(), QString()), "failed to create the source Host");
        mpSource = hostManager.getHost(mSourceName);
        QVERIFY(mpSource);
        QVERIFY2(hostManager.addHost(mTargetName, qsl("23"), QString(), QString()), "failed to create the target Host");
        mpTarget = hostManager.getHost(mTargetName);
        QVERIFY(mpTarget);

        buildSourceMap();
        if (QTest::currentTestFailed()) {
            return;
        }

        // Sanity check: the freshly built source map itself matches all
        // expectations before any save/load is involved:
        verifyMap(mpSource->mpMap.data(), mpSource->mpMap->mDefaultVersion, mSourceName);
    }

    void cleanupTestCase()
    {
        mpSource = nullptr;
        mpTarget = nullptr;
        deleteProfileDirectory(mSourceName);
        deleteProfileDirectory(mTargetName);
        delete mudlet::self();
    }

    void test_roundTripAtDefaultVersion() { roundTripAtVersion(mpSource->mpMap->mDefaultVersion); }

    void test_roundTripAtOlderVersions_data()
    {
        QTest::addColumn<int>("saveVersion");
        for (int version = mpSource->mpMap->mMinVersion; version < mpSource->mpMap->mDefaultVersion; ++version) {
            QTest::newRow(qPrintable(qsl("format %1").arg(version))) << version;
        }
    }

    void test_roundTripAtOlderVersions()
    {
        QFETCH(int, saveVersion);
        roundTripAtVersion(saveVersion);
    }
};

void initializeQRCResourcesForMapRoundTripTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "MapRoundTripTest.moc"
QTEST_MAIN(MapRoundTripTest)
