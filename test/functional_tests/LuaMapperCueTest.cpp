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
 * What the Lua mapper functions make the mapper drawing the map do: relist
 * its areas, follow a renamed or centred area, show itself after a map load
 * and repaint after a display setting changes. A second mapper of the same
 * profile, which is not drawing the map, is left alone.
 *
 * The area dropdown, visibility and repaints are widget state no Lua function
 * reads, so this cannot be a busted spec.
 *
 * Run with: ctest -R LuaMapperCueTest -V
 */

#include <QComboBox>
#include <QDir>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class LuaMapperCueTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mMapDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgMapper* mpMapper = nullptr;
    T2DMap* mp2dMap = nullptr;
    int mPaintCount = 0;
    const QString mProfileName = qsl("LuaMapperCue-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    bool lua(const QString& code) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    void buildMap()
    {
        TMap* pMap = map();
        pMap->setDefaultAreaShown(false);
        pMap->mapClear();
        const int groundAreaId = pMap->mpRoomDB->addArea(qsl("Ground"));
        const int upstairsAreaId = pMap->mpRoomDB->addArea(qsl("Upstairs"));
        QVERIFY(groundAreaId > 0);
        QVERIFY(upstairsAreaId > 0);
        QVERIFY(pMap->addRoom(1) && pMap->setRoomArea(1, groundAreaId) && pMap->setRoomCoordinates(1, 0, 0, 0));
        QVERIFY(pMap->addRoom(2) && pMap->setRoomArea(2, upstairsAreaId) && pMap->setRoomCoordinates(2, 0, 0, 1));
        pMap->mRoomIdHash[pMap->mProfileName] = 1;
    }

    int areaIdOf(const QString& name) const { return map()->mpRoomDB->getAreaNamesMap().key(name, 0); }

    static QStringList areaListOf(const dlgMapper* pMapper)
    {
        QStringList names;
        for (int i = 0; i < pMapper->comboBox_showArea->count(); ++i) {
            names << pMapper->comboBox_showArea->itemText(i);
        }
        return names;
    }

    // Lets any repaint already on its way land, so that a later count only
    // sees what the call under test asked for.
    void settlePaints()
    {
        int before = -1;
        while (before != mPaintCount) {
            before = mPaintCount;
            QTest::qWait(50ms);
        }
        mPaintCount = 0;
    }

    void makeTheMapper()
    {
        mpHost->showHideOrCreateMapper(false);
        mpMapper = map()->mpMapper;
        QVERIFY2(mpMapper, "the profile has no mapper to take the cues");
        mp2dMap = mpMapper->mp2dMap;
        QVERIFY(mp2dMap);
        mp2dMap->installEventFilter(this);
    }

    // Delivers the repaints already asked for and nothing else: no timer or
    // later event gets to run, so a paint counted here was requested by the time
    // this is called, not by something that happened to repaint afterwards.
    bool paintRequested()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::UpdateRequest);
        return mPaintCount > 0;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == mp2dMap && event->type() == QEvent::Paint) {
            ++mPaintCount;
        }
        return QObject::eventFilter(watched, event);
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        MudletApp::getQSettings()->setValue(qsl("autosaveIntervalMinutes"), 0);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3s), "could not connect to the telnet stub");
        mudlet::self()->show();
    }

    void cleanupTestCase()
    {
        if (mp2dMap) {
            mp2dMap->removeEventFilter(this);
        }
        mp2dMap = nullptr;
        mpMapper = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        buildMap();
        if (QTest::currentTestFailed() || qstrcmp(QTest::currentTestFunction(), "test_withoutAMapperTheDefaultAreaIsLeftAloneAndAreasStillWork") == 0) {
            return;
        }
        // Made here rather than by a test of its own, so any one test can be run alone
        if (!mpMapper) {
            makeTheMapper();
            if (QTest::currentTestFailed()) {
                return;
            }
        }
        QVERIFY2(map()->mpMapper == mpMapper, "the mapper made for this test is no longer the one drawing the map");
        mpMapper->show();
        mpMapper->updateAreaComboBox();
        mpMapper->comboBox_showArea->setCurrentText(qsl("Ground"));
        QCOMPARE(areaListOf(mpMapper), (QStringList{qsl("Ground"), qsl("Upstairs")}));
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Ground"));
    }

    // Needs a profile with no mapper yet, so it must stay the first test.
    void test_withoutAMapperTheDefaultAreaIsLeftAloneAndAreasStillWork()
    {
        QVERIFY2(map()->mpMapper.isNull(), "the profile already has a mapper");

        QVERIFY(lua(qsl("assert(setDefaultAreaVisible(true) == false)")));
        QVERIFY2(!map()->getDefaultAreaShown(), "setDefaultAreaVisible changed the map with no mapper open");

        QVERIFY(lua(qsl("local id = addAreaName('Cellar'); assert(type(id) == 'number' and id > 0)")));
        QVERIFY(lua(qsl("assert(setAreaName('Cellar', 'Crypt') == true)")));
        QVERIFY(lua(qsl("assert(deleteArea('Crypt') == true)")));
        QCOMPARE(areaIdOf(qsl("Crypt")), 0);
    }

    void test_addAreaNameListsTheAreaInTheMapper()
    {
        QVERIFY(lua(qsl("assert(addAreaName('Cellar') > 0)")));

        QCOMPARE(areaListOf(mpMapper), (QStringList{qsl("Cellar"), qsl("Ground"), qsl("Upstairs")}));
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Ground"));
    }

    void test_deleteAreaDropsTheAreaFromTheMapper()
    {
        QVERIFY(lua(qsl("assert(deleteArea('Upstairs') == true)")));

        QCOMPARE(areaListOf(mpMapper), QStringList{qsl("Ground")});
    }

    void test_renamingTheShownAreaKeepsTheMapperOnIt()
    {
        // Sorts after Upstairs, so relisting alone would leave Upstairs, the
        // first entry, showing.
        QVERIFY(lua(qsl("assert(setAreaName(%1, 'Zenith') == true)").arg(areaIdOf(qsl("Ground")))));

        QCOMPARE(areaListOf(mpMapper), (QStringList{qsl("Upstairs"), qsl("Zenith")}));
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Zenith"));
    }

    void test_renamingAnotherAreaLeavesTheMapperWhereItIs()
    {
        QVERIFY(lua(qsl("assert(setAreaName('Upstairs', 'Attic') == true)")));

        QCOMPARE(areaListOf(mpMapper), (QStringList{qsl("Attic"), qsl("Ground")}));
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Ground"));
    }

    void test_centerviewShowsThePlayersNewAreaInTheMapper()
    {
        QVERIFY(lua(qsl("assert(centerview(2) == true)")));

        // At once, not on the deferred repaint that follows a move.
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Upstairs"));
    }

    void test_loadJsonMapRelistsAndShowsTheMapperOnThePlayersArea()
    {
        map()->mRoomIdHash[map()->mProfileName] = 2;
        const QString fileName = qsl("%1/cue.json").arg(mMapDir.path());
        QVERIFY(lua(qsl("assert(saveJsonMap([[%1]]) == true)").arg(fileName)));
        mpMapper->comboBox_showArea->clear();
        mpMapper->hide();

        QVERIFY(lua(qsl("assert(loadJsonMap([[%1]]) == true)").arg(fileName)));

        QCOMPARE(map()->mRoomIdHash.value(map()->mProfileName), 2);
        QCOMPARE(areaListOf(mpMapper), (QStringList{qsl("Ground"), qsl("Upstairs")}));
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Upstairs"));
        QVERIFY2(!mpMapper->isHidden(), "loading a map did not show the mapper");
    }

    // The 2D map can be showing the default area while it is left out of the
    // area list; showing the default area again must put the list on it.
    void test_showingTheDefaultAreaWhileOnItPutsTheMapperOnItAndRepaintsAtOnce()
    {
        auto hideDefault = qScopeGuard([this]() {
            map()->setDefaultAreaShown(false);
        });
        // After the settling, as a paint moves the 2D map to the player's area.
        settlePaints();
        mp2dMap->mAreaID = -1;

        QVERIFY(lua(qsl("assert(setDefaultAreaVisible(true) == true)")));

        QVERIFY(map()->getDefaultAreaShown());
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), map()->getDefaultAreaName());
        QVERIFY2(mPaintCount > 0, "the 2D map was not repainted before the call returned");
    }

    void test_showingTheDefaultAreaElsewhereLeavesTheMapperWhereItIs()
    {
        auto hideDefault = qScopeGuard([this]() {
            map()->setDefaultAreaShown(false);
        });
        mp2dMap->mAreaID = areaIdOf(qsl("Ground"));

        QVERIFY(lua(qsl("assert(setDefaultAreaVisible(true) == true)")));

        QVERIFY(areaListOf(mpMapper).contains(map()->getDefaultAreaName()));
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Ground"));
    }

    void test_drawingUpperAndLowerLevelsRepaintsTheMapper()
    {
        QVERIFY(lua(qsl("mapperCueUpperLower = getConfig('showUpperLowerLevels')")));
        auto restore = qScopeGuard([this]() {
            lua(qsl("setConfig('showUpperLowerLevels', mapperCueUpperLower)"));
        });
        settlePaints();

        QVERIFY(lua(qsl("assert(setConfig('showUpperLowerLevels', not mapperCueUpperLower) == true)")));

        QVERIFY2(paintRequested(), "the mapper was not repainted");
    }

    void test_showingAndHidingMapInfoRepaintsTheMapper()
    {
        settlePaints();
        QVERIFY(lua(qsl("assert(setConfig('showMapInfo', 'Short') == true)")));
        QVERIFY(mpHost->mMapInfoContributors.contains(qsl("Short")));
        QVERIFY2(paintRequested(), "the mapper was not repainted for a shown info overlay");

        settlePaints();
        QVERIFY(lua(qsl("assert(setConfig('hideMapInfo', 'Short') == true)")));
        QVERIFY(!mpHost->mMapInfoContributors.contains(qsl("Short")));
        QVERIFY2(paintRequested(), "the mapper was not repainted for a hidden info overlay");
    }

    void test_mapRoomSizeResizesTheDrawnRoomsAndFlushesTheSymbolCache()
    {
        const double before = mpHost->mRoomSize;
        auto restore = qScopeGuard([this, before]() {
            lua(qsl("setConfig('mapRoomSize', %1)").arg(qRound(before * 10)));
        });
        const int target = qRound(before * 10) == 3 ? 7 : 3;
        mp2dMap->addSymbolToPixmapCache(qsl("test-key"), qsl("A"), Qt::white, false);
        QVERIFY2(mp2dMap->symbolPixmapCacheCount() > 0, "nothing was cached, so a flush would prove nothing");
        map()->resetUnsaved();
        settlePaints();

        QVERIFY(lua(qsl("assert(setConfig('mapRoomSize', %1) == true)").arg(target)));

        // Rounded through float, which is what the profile saves.
        const double expected = static_cast<float>(target / 10.0);
        QCOMPARE(mpHost->mRoomSize, expected);
        QCOMPARE(mp2dMap->rSize, expected);
        QCOMPARE(mp2dMap->symbolPixmapCacheCount(), 0);
        QVERIFY2(map()->isUnsaved(), "a new room size did not mark the map unsaved");
        QVERIFY2(paintRequested(), "the mapper was not repainted");
    }

    void test_mapExitSizeSetsTheDrawnExitWidth()
    {
        const double before = mpHost->mLineSize;
        auto restore = qScopeGuard([this, before]() {
            mpHost->mLineSize = before;
            mp2dMap->eSize = before;
        });
        const int target = qRound(before) == 4 ? 6 : 4;

        QVERIFY(lua(qsl("assert(setConfig('mapExitSize', %1) == true)").arg(target)));

        QCOMPARE(mpHost->mLineSize, static_cast<double>(target));
        QCOMPARE(mp2dMap->eSize, static_cast<double>(target));
    }

    void test_theBooleanMapKeysReachTheDrawnMap_data()
    {
        QTest::addColumn<QString>("key");
        QTest::newRow("mapRoundRooms") << qsl("mapRoundRooms");
        QTest::newRow("showRoomIdsOnMap") << qsl("showRoomIdsOnMap");
        QTest::newRow("mapShowGrid") << qsl("mapShowGrid");
    }

    void test_theBooleanMapKeysReachTheDrawnMap()
    {
        QFETCH(QString, key);
        const QMap<QString, std::pair<bool*, bool*>> copies{
                {qsl("mapRoundRooms"), {&mpHost->mBubbleMode, &mp2dMap->mBubbleMode}},
                {qsl("showRoomIdsOnMap"), {&mpHost->mShowRoomID, &mp2dMap->mShowRoomID}},
                {qsl("mapShowGrid"), {&mpHost->mMapperShowGrid, &mp2dMap->mShowGrid}},
        };
        auto [hostCopy, mapCopy] = copies.value(key);
        const bool before = *hostCopy;
        QCOMPARE(*mapCopy, before);
        auto restore = qScopeGuard([this, key, before]() {
            lua(qsl("setConfig('%1', %2)").arg(key, before ? qsl("true") : qsl("false")));
        });
        settlePaints();

        QVERIFY(lua(qsl("assert(setConfig('%1', %2) == true)").arg(key, before ? qsl("false") : qsl("true"))));

        QCOMPARE(*hostCopy, !before);
        QCOMPARE(*mapCopy, !before);
        QVERIFY2(paintRequested(), "the mapper was not repainted");
    }

    // A second mapper of the same profile - one in a detached window, say -
    // is not the one drawing the map.
    void test_onlyTheMapperDrawingTheMapFollowsTheLuaFunctions()
    {
        auto other = std::make_unique<dlgMapper>(nullptr, mpHost, map());
        QVERIFY2(map()->mpMapper == mpMapper, "making a second mapper took the map over");
        other->updateAreaComboBox();
        other->comboBox_showArea->setCurrentText(qsl("Ground"));
        const QStringList otherAreasBefore = areaListOf(other.get());
        const bool gridBefore = mpHost->mMapperShowGrid;
        auto restoreGrid = qScopeGuard([this, gridBefore]() {
            lua(qsl("setConfig('mapShowGrid', %1)").arg(gridBefore ? qsl("true") : qsl("false")));
        });
        QCOMPARE(other->mp2dMap->mShowGrid, gridBefore);

        QVERIFY(lua(qsl("assert(addAreaName('Cellar') > 0)")));
        QVERIFY(lua(qsl("assert(setAreaName('Ground', 'Zenith') == true)")));
        QVERIFY(lua(qsl("assert(centerview(2) == true)")));
        QVERIFY(lua(qsl("assert(setConfig('mapShowGrid', %1) == true)").arg(gridBefore ? qsl("false") : qsl("true"))));

        QCOMPARE(areaListOf(mpMapper), (QStringList{qsl("Cellar"), qsl("Upstairs"), qsl("Zenith")}));
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Upstairs"));
        QCOMPARE(mp2dMap->mShowGrid, !gridBefore);
        QCOMPARE(areaListOf(other.get()), otherAreasBefore);
        QCOMPARE(other->comboBox_showArea->currentText(), qsl("Ground"));
        QCOMPARE(other->mp2dMap->mShowGrid, gridBefore);
    }
};

#include "LuaMapperCueTest.moc"
MUDLET_GROUPED_TEST_MAIN(LuaMapperCueTest)
