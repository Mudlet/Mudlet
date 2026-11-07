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
 * Each open profile draws its own 2D map, and what one profile's map has
 * drawn must not change how another profile's map draws.
 *
 * Why not a spec: a spec runs in the one self-test profile, and the symbol
 * cache and the mapper's area box font are not visible from Lua.
 *
 * Run with: ctest -R MapCrossProfileStateTest -V
 */

#include <QComboBox>
#include <QDir>
#include <QPixmap>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "T2DMap.h"
#include "TMap.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapCrossProfileStateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHostA = nullptr;
    Host* mpHostB = nullptr;
    const QString mProfileA = qsl("MapCrossProfileState-A");
    const QString mProfileB = qsl("MapCrossProfileState-B");

    static constexpr int kWidth = 300;
    static constexpr int kHeight = 200;

    static void deleteProfileDirectory(const QString& name)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, name));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    static void render(T2DMap* pMap)
    {
        QPixmap target(kWidth, kHeight);
        target.fill(Qt::black);
        pMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
    }

    static T2DMap* standalone2DMap(Host* pHost)
    {
        auto* pMap = new T2DMap();
        pMap->mpMap = pHost->mpMap.data();
        pMap->mpHost = pHost;
        pMap->resize(kWidth, kHeight);
        return pMap;
    }

    // One room in the default area, which the area box does not list, so the
    // mapper shows a map its area box cannot name.
    static dlgMapper* mapperShowingTheDefaultArea(Host* pHost)
    {
        TMap* pMap = pHost->mpMap.data();
        pMap->mapClear();
        if (!pMap->addRoom(1) || !pMap->setRoomArea(1, -1) || !pMap->setRoomCoordinates(1, 0, 0, 0)) {
            return nullptr;
        }
        pMap->mRoomIdHash[pMap->mProfileName] = 1;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);

        pHost->mMapViewOnly = false;
        if (!pMap->mapper()) {
            pHost->showHideOrCreateMapper(false);
        }
        dlgMapper* pMapper = pMap->mapper();
        if (!pMapper) {
            return nullptr;
        }
        T2DMap* p2dMap = pMapper->mp2dMap;
        p2dMap->init();
        p2dMap->resize(kWidth, kHeight);
        p2dMap->mRoomID = 1;
        p2dMap->mAreaID = -1;
        p2dMap->mShiftMode = true;
        return pMapper;
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

        mudlet::self()->mSkipDefaultPackageInstall = true;
        MudletApp::getQSettings()->setValue(qsl("autosaveIntervalMinutes"), 0);

        auto* hostManager = HostManager::self();
        for (const auto& name : {mProfileA, mProfileB}) {
            deleteProfileDirectory(name);
            QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, name)));
            QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
            QVERIFY(MudletApp::writeProfileData(name, qsl("port"), qsl("23")).first);
            QVERIFY2(mudlet::self()->openProfile(name, false), "failed to open the profile");
        }
        mpHostA = hostManager->getHost(mProfileA);
        mpHostB = hostManager->getHost(mProfileB);
        QVERIFY(mpHostA);
        QVERIFY(mpHostB);
    }

    void cleanupTestCase()
    {
        mpHostA = nullptr;
        mpHostB = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileA);
            deleteProfileDirectory(mProfileB);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Symbols are drawn smaller to fit inside round rooms, so turning round
    // rooms on has to redraw them, however the other profile's map is set.
    void test_roundRoomsInOneProfileStillRedrawTheOthersSymbols()
    {
        mpHostA->mpMap->mapClear();
        mpHostB->mpMap->mapClear();
        std::unique_ptr<T2DMap> pMapA(standalone2DMap(mpHostA));
        std::unique_ptr<T2DMap> pMapB(standalone2DMap(mpHostB));

        pMapA->mBubbleMode = false;
        render(pMapA.get());
        pMapA->addSymbolToPixmapCache(qsl("square-symbol"), qsl("A"), Qt::white, false);
        QCOMPARE(pMapA->symbolPixmapCacheCount(), 1);

        pMapB->mBubbleMode = true;
        render(pMapB.get());

        pMapA->mBubbleMode = true;
        render(pMapA.get());
        QCOMPARE(pMapA->symbolPixmapCacheCount(), 0);
    }

    void test_eachProfilesAreaBoxSaysItCannotNameTheDefaultArea()
    {
        dlgMapper* pMapperA = mapperShowingTheDefaultArea(mpHostA);
        QVERIFY(pMapperA);
        dlgMapper* pMapperB = mapperShowingTheDefaultArea(mpHostB);
        QVERIFY(pMapperB);
        QVERIFY(pMapperA != pMapperB);

        render(pMapperA->mp2dMap);
        QVERIFY2(pMapperA->comboBox_showArea->font().strikeOut(), "the first profile's area box was not struck out, so the second tells nothing");

        render(pMapperB->mp2dMap);
        QVERIFY(pMapperB->comboBox_showArea->font().strikeOut());
    }
};

#include "MapCrossProfileStateTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapCrossProfileStateTest)
