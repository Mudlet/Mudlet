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
 * The strip of controls under the map: the level buttons, the area dropdown,
 * the arrow that folds the strip away, the menu behind the burger button and
 * the warning that comes up when an autosave fails.
 *
 * None of it has a Lua route - the controls only exist in dlgMapper, and the
 * settings they change are read straight off the profile by the next paint -
 * so this cannot be a busted spec.
 *
 * The burger and warning menus block in exec() until they are closed, so a
 * case arms a timer to work them before it clicks the button that opens them.
 *
 * Run with: ctest -R MapperPanelTest -V
 */

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QtTest/QtTest>

#include <functional>
#include <utility>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapperPanelTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgMapper* mpMapper = nullptr;
    T2DMap* mp2dMap = nullptr;
    QTimer* mpMenuTimer = nullptr;
    bool mMenuWorked = false;
    bool mSavedDrawUpperLowerLevels = true;
    int mGroundAreaId = 0;
    int mUpstairsAreaId = 0;
    const QString mProfileName = qsl("MapperPanel-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // Map units across the shorter dimension, which puts one map unit at 20
    // pixels in both directions.
    static constexpr double kZoom = 20.0;
    static constexpr double kPixelsPerMapUnit = 20.0;
    static constexpr double kRoomSize = 0.5;

    // The ground floor: the player's room at the origin and one directly
    // above it on the level up.
    static constexpr int kPlayerRoomId = 1;
    static constexpr int kRoomAboveId = 3;
    // Three rooms in a row in the other area; the dropdown centres its view on
    // the middle one.
    static constexpr int kUpstairsWestId = 10;
    static constexpr int kUpstairsMiddleId = 11;
    static constexpr int kUpstairsEastId = 12;
    static constexpr int kUpstairsY = 5;

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    bool addRoomAt(const int id, const int areaId, const int x, const int y, const int z) const
    {
        return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, z);
    }

    void buildMap()
    {
        TMap* pMap = map();
        pMap->mapClear();
        mGroundAreaId = pMap->mpRoomDB->addArea(qsl("Ground"));
        mUpstairsAreaId = pMap->mpRoomDB->addArea(qsl("Upstairs"));
        QVERIFY(mGroundAreaId > 0);
        QVERIFY(mUpstairsAreaId > 0);

        QVERIFY(addRoomAt(kPlayerRoomId, mGroundAreaId, 0, 0, 0));
        QVERIFY(addRoomAt(kRoomAboveId, mGroundAreaId, 0, 0, 1));
        QVERIFY(addRoomAt(kUpstairsWestId, mUpstairsAreaId, 5, kUpstairsY, 0));
        QVERIFY(addRoomAt(kUpstairsMiddleId, mUpstairsAreaId, 6, kUpstairsY, 0));
        QVERIFY(addRoomAt(kUpstairsEastId, mUpstairsAreaId, 7, kUpstairsY, 0));

        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);
    }

    // Points the mapper at the map buildMap() made, with the ground floor of
    // the player's area in the dropdown, and draws it once.
    void showMapper()
    {
        mpHost->mMapViewOnly = false;
        mpHost->mRoomSize = kRoomSize;
        if (!map()->mpMapper) {
            mpHost->showHideOrCreateMapper(false);
        }
        mpMapper = map()->mpMapper;
        QVERIFY(mpMapper);
        mp2dMap = mpMapper->mp2dMap;
        QVERIFY(mp2dMap);
        mpMapper->updateAreaComboBox();
        mpMapper->resetAreaComboBoxToPlayerRoomArea();
        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Ground"));
        mp2dMap->init();
        mp2dMap->resize(kWidgetWidth, kWidgetHeight);
        mp2dMap->mRoomID = kPlayerRoomId;
        mp2dMap->mAreaID = mGroundAreaId;
        // paintEvent() re-centres on the player and draws somewhere else unless
        // the player's room, mRoomID and mShiftMode all agree.
        mp2dMap->mShiftMode = true;
        mp2dMap->mPick = false;
        mp2dMap->mMapCenterX = 0;
        mp2dMap->mMapCenterY = 0;
        mp2dMap->mMapCenterZ = 0;
        mp2dMap->mMultiSelectionSet.clear();
        map()->mpRoomDB->getArea(mGroundAreaId)->set2DMapZoom(kZoom);
        map()->mpRoomDB->getArea(mUpstairsAreaId)->set2DMapZoom(kZoom);
        renderFrame();
        QCOMPARE(static_cast<double>(mp2dMap->mRoomWidth), kPixelsPerMapUnit);
    }

    void renderFrame() const
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        mp2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
    }

    QPoint viewCentre() const { return QPoint(kWidgetWidth / 2, kWidgetHeight / 2); }

    void clickMapAt(const QPoint& position) const
    {
        for (const QEvent::Type type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
            QMouseEvent event(type, QPointF(position), mp2dMap->mapToGlobal(QPointF(position)), Qt::LeftButton, type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(mp2dMap, &event);
        }
    }

    void clickButton(QAbstractButton* pButton) const { QTest::mouseClick(pButton, Qt::LeftButton); }

    static QAction* menuAction(const QMenu* pMenu, const QString& text)
    {
        for (QAction* pAction : pMenu->actions()) {
            if (pAction->text() == text) {
                return pAction;
            }
        }
        return nullptr;
    }

    // Clicks the button, which blocks in the menu's exec() until the menu goes
    // down, so the menu is worked from a timer that waits for it to come up,
    // hands it to work() and then closes it. Returns whether work() ran. If
    // what comes up is not the mapper's menu, it is closed anyway once the
    // timer gives up, so the click returns instead of blocking for good.
    bool workMenuBehind(QAbstractButton* pButton, const std::function<void(QMenu*)>& work)
    {
        stopWorkingMenus();
        mMenuWorked = false;
        int attemptsLeft = 100;
        mpMenuTimer = new QTimer(this);
        mpMenuTimer->setInterval(20);
        connect(mpMenuTimer, &QTimer::timeout, this, [this, work, attemptsLeft]() mutable {
            QWidget* pPopup = QApplication::activePopupWidget();
            auto* pMenu = qobject_cast<QMenu*>(pPopup);
            if (!pMenu || pMenu->parentWidget() != mpMapper) {
                if (--attemptsLeft <= 0) {
                    if (pPopup) {
                        pPopup->close();
                    }
                    stopWorkingMenus();
                }
                return;
            }
            work(pMenu);
            mMenuWorked = true;
            stopWorkingMenus();
            pMenu->close();
        });
        mpMenuTimer->start();
        clickButton(pButton);
        stopWorkingMenus();
        return mMenuWorked;
    }

    void stopWorkingMenus()
    {
        if (!mpMenuTimer) {
            return;
        }
        mpMenuTimer->stop();
        mpMenuTimer->deleteLater();
        mpMenuTimer = nullptr;
    }

    bool pickMapperMenuItem(const QString& text)
    {
        bool picked = false;
        const bool worked = workMenuBehind(mpMapper->toolButton_mapperMenu, [&picked, text](QMenu* pMenu) {
            if (QAction* pAction = menuAction(pMenu, text)) {
                pAction->trigger();
                picked = true;
            }
        });
        return worked && picked;
    }

    // Picks an entry of the Info overlays submenu, which is filled in when the
    // menu is built so it does not need opening.
    bool pickInfoOverlay(const QString& text)
    {
        bool picked = false;
        const bool worked = workMenuBehind(mpMapper->toolButton_mapperMenu, [&picked, text](QMenu* pMenu) {
            QAction* pSubmenu = menuAction(pMenu, qsl("Info overlays"));
            if (!pSubmenu || !pSubmenu->menu()) {
                return;
            }
            if (QAction* pAction = menuAction(pSubmenu->menu(), text)) {
                pAction->trigger();
                picked = true;
            }
        });
        return worked && picked;
    }

    // Opens the mapper menu and checks each item named in expected is a
    // checkable one ticked as given.
    void expectMapperMenuChecks(const QMap<QString, bool>& expected)
    {
        QMap<QString, bool> checks;
        QVERIFY2(workMenuBehind(mpMapper->toolButton_mapperMenu,
                                [&checks](QMenu* pMenu) {
                                    for (QAction* pAction : pMenu->actions()) {
                                        if (pAction->isCheckable()) {
                                            checks.insert(pAction->text(), pAction->isChecked());
                                        }
                                    }
                                }),
                 "the mapper menu did not open");
        for (const auto& [item, on] : expected.asKeyValueRange()) {
            QVERIFY2(checks.contains(item), qPrintable(qsl("no checkable '%1' item in the mapper menu").arg(item)));
            QVERIFY2(checks.value(item) == on,
                     qPrintable(qsl("'%1' is %2 in the menu but the setting is %3").arg(item, checks.value(item) ? qsl("ticked") : qsl("clear"), on ? qsl("on") : qsl("off"))));
        }
    }

    // Each drawing option as the next paint reads it, from the widget and
    // from the profile, which the menu has to keep in step. The ones kept in
    // a single place report it twice.
    std::pair<bool, bool> drawingOption(const QString& item) const
    {
        if (item == qsl("Round rooms")) {
            return {mp2dMap->mBubbleMode, mpHost->mBubbleMode};
        }
        if (item == qsl("Show room IDs")) {
            return {mp2dMap->mShowRoomID, mpHost->mShowRoomID};
        }
        if (item == qsl("Show room names")) {
            return {map()->getRoomNamesShown(), map()->getRoomNamesShown()};
        }
        if (item == qsl("Show map grid")) {
            return {mp2dMap->mShowGrid, mpHost->mMapperShowGrid};
        }
        if (item == qsl("Draw rooms on upper and lower levels")) {
            return {mudlet::self()->mDrawUpperLowerLevels, mudlet::self()->mDrawUpperLowerLevels};
        }
        return {false, true};
    }

    // Sets each option in both the places it is kept.
    void setDrawingOptions(const bool roundRooms, const bool roomIds, const bool roomNames, const bool grid, const bool upperLowerLevels)
    {
        mpHost->mBubbleMode = roundRooms;
        mp2dMap->mBubbleMode = roundRooms;
        mpHost->mShowRoomID = roomIds;
        mp2dMap->mShowRoomID = roomIds;
        map()->setRoomNamesShown(roomNames);
        mpHost->mMapperShowGrid = grid;
        mp2dMap->mShowGrid = grid;
        mudlet::self()->mDrawUpperLowerLevels = upperLowerLevels;
    }

    bool pickSaveWarningItem(const QString& text)
    {
        bool picked = false;
        const bool worked = workMenuBehind(mpMapper->toolButton_saveWarning, [&picked, text](QMenu* pMenu) {
            if (QAction* pAction = menuAction(pMenu, text)) {
                pAction->trigger();
                picked = true;
            }
        });
        return worked && picked;
    }

    QStringList savedMapFiles() const
    {
        const QDir dir(mudlet::getMudletPath(enums::profileMapsPath, mProfileName));
        return dir.entryList({qsl("*.dat")}, QDir::Files);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so this never reads or writes
        // the developer's ~/.config/mudlet. Since #9712 the opt-in that makes
        // setupConfig() adopt a directory is $XDG_CONFIG_HOME/mudlet/profiles,
        // not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        // The menu items are found by their text.
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        mSavedDrawUpperLowerLevels = mudlet::self()->mDrawUpperLowerLevels;
        // The map autosave writes into the directory the retry case counts
        // files in, and moves the save-error flag the warning cases read.
        mudlet::self()->getQSettings()->setValue(qsl("autosaveIntervalMinutes"), 0);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3000), "could not connect to the telnet stub");
    }

    void cleanupTestCase()
    {
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

    // The mapper is made once and reused, so the settings the menu changes
    // are put back to a known state before each case.
    void init()
    {
        buildMap();
        if (QTest::currentTestFailed()) {
            return;
        }
        showMapper();
        if (QTest::currentTestFailed()) {
            return;
        }
        setDrawingOptions(false, false, false, false, true);
        mpHost->mMapInfoContributors.clear();
    }

    // Some of the menu's settings live on the application rather than the
    // profile, and a case that fails partway leaves them as it set them.
    void cleanup()
    {
        stopWorkingMenus();
        if (mudlet::self()) {
            mudlet::self()->mDrawUpperLowerLevels = mSavedDrawUpperLowerLevels;
        }
        if (mpMapper && !mpMapper->widget_panel->isVisibleTo(mpMapper)) {
            mpMapper->slot_setMapperPanelVisible(true);
        }
        if (mpHost) {
            map()->setSaveError(false);
        }
    }

    void test_theLevelButtonsShowTheFloorAboveAndThenTheOneBelowAgain()
    {
        clickButton(mpMapper->toolButton_shiftZup);
        renderFrame();
        QCOMPARE(mp2dMap->mMapCenterZ, 1);
        clickMapAt(viewCentre());
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kRoomAboveId});

        clickButton(mpMapper->toolButton_shiftZdown);
        renderFrame();
        QCOMPARE(mp2dMap->mMapCenterZ, 0);
        clickMapAt(viewCentre());
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kPlayerRoomId});
    }

    void test_theFloorShownStaysPutWhenTheMapIsRedrawn()
    {
        clickButton(mpMapper->toolButton_shiftZup);
        renderFrame();
        renderFrame();
        QCOMPARE(mp2dMap->mMapCenterZ, 1);
        QCOMPARE(mp2dMap->mAreaID, mGroundAreaId);
    }

    void test_theArrowButtonFoldsThePanelAwayAndRemembersThatOnTheProfile()
    {
        QVERIFY(mpMapper->widget_panel->isVisibleTo(mpMapper));
        QVERIFY(mpHost->mShowPanel);

        clickButton(mpMapper->toolButton_togglePanel);
        QVERIFY2(!mpMapper->widget_panel->isVisibleTo(mpMapper), "the panel stayed up");
        QVERIFY2(!mpHost->mShowPanel, "the profile was not told the panel went away");

        clickButton(mpMapper->toolButton_togglePanel);
        QVERIFY2(mpMapper->widget_panel->isVisibleTo(mpMapper), "the panel did not come back");
        QVERIFY(mpHost->mShowPanel);
    }

    void test_pickingAnAreaInTheDropdownShowsItCentredOnItsRooms()
    {
        QCOMPARE(mpMapper->comboBox_showArea->count(), 2);

        QTest::keyClick(mpMapper->comboBox_showArea, Qt::Key_Down);

        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Upstairs"));
        QCOMPARE(mp2dMap->mAreaID, mUpstairsAreaId);
        QCOMPARE(mp2dMap->mMapCenterX, 6.0);
        QCOMPARE(mp2dMap->mMapCenterY, -5.0);
        renderFrame();
        QVERIFY2(mp2dMap->mAreaID == mUpstairsAreaId, "a repaint pulled the view back to the player's area");
        clickMapAt(viewCentre());
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{kUpstairsMiddleId});

        QTest::keyClick(mpMapper->comboBox_showArea, Qt::Key_Up);

        QCOMPARE(mpMapper->comboBox_showArea->currentText(), qsl("Ground"));
        QCOMPARE(mp2dMap->mAreaID, mGroundAreaId);
    }

    void test_theMapperMenuShowsWhichDrawingOptionsAreOn()
    {
        setDrawingOptions(true, false, true, false, true);
        expectMapperMenuChecks({
                {qsl("Round rooms"), true},
                {qsl("Show room IDs"), false},
                {qsl("Show room names"), true},
                {qsl("Show map grid"), false},
                {qsl("Draw rooms on upper and lower levels"), true},
        });

        setDrawingOptions(false, true, false, true, false);
        expectMapperMenuChecks({
                {qsl("Round rooms"), false},
                {qsl("Show room IDs"), true},
                {qsl("Show room names"), false},
                {qsl("Show map grid"), true},
                {qsl("Draw rooms on upper and lower levels"), false},
        });
    }

    void test_aDrawingOptionInTheMapperMenuTogglesTheSettingItNames_data()
    {
        QTest::addColumn<QString>("item");
        QTest::newRow("round rooms") << qsl("Round rooms");
        QTest::newRow("room ids") << qsl("Show room IDs");
        QTest::newRow("room names") << qsl("Show room names");
        QTest::newRow("grid") << qsl("Show map grid");
        QTest::newRow("upper and lower levels") << qsl("Draw rooms on upper and lower levels");
    }

    void test_aDrawingOptionInTheMapperMenuTogglesTheSettingItNames()
    {
        QFETCH(QString, item);
        const auto [widgetWasOn, profileWasOn] = drawingOption(item);
        QCOMPARE(widgetWasOn, profileWasOn);

        QVERIFY(pickMapperMenuItem(item));
        QCOMPARE(drawingOption(item), std::make_pair(!widgetWasOn, !widgetWasOn));

        QVERIFY(pickMapperMenuItem(item));
        QCOMPARE(drawingOption(item), std::make_pair(widgetWasOn, widgetWasOn));
    }

    void test_pickingAnInfoOverlayTurnsItOnAndNoneTurnsThemAllOff()
    {
        mpHost->mMapInfoContributors = {qsl("Short")};

        QVERIFY(pickInfoOverlay(qsl("Full")));
        QCOMPARE(mpHost->mMapInfoContributors, (QSet<QString>{qsl("Short"), qsl("Full")}));

        QVERIFY(pickInfoOverlay(qsl("None")));
        QVERIFY2(mpHost->mMapInfoContributors.isEmpty(), qPrintable(qsl("still showing: %1").arg(QStringList(mpHost->mMapInfoContributors.values()).join(qsl(", ")))));
    }

    void test_theSaveWarningComesUpWhenTheMapCannotBeSavedAndDismissTakesItDown()
    {
        QVERIFY(mpMapper->toolButton_saveWarning->isHidden());

        map()->setSaveError(true);
        QVERIFY2(!mpMapper->toolButton_saveWarning->isHidden(), "a failed save left the warning hidden");

        QVERIFY(pickSaveWarningItem(qsl("Dismiss warning")));
        QVERIFY2(!map()->hasSaveError(), "dismissing the warning left the map flagged as unsaveable");
        QVERIFY2(mpMapper->toolButton_saveWarning->isHidden(), "the warning stayed up after being dismissed");
    }

    void test_retryingFromTheSaveWarningSavesTheMapAndTakesTheWarningDown()
    {
        map()->setSaveError(true);
        const int savedBefore = savedMapFiles().size();

        QVERIFY(pickSaveWarningItem(qsl("Retry save")));

        QCOMPARE(savedMapFiles().size(), savedBefore + 1);
        QVERIFY2(!map()->hasSaveError(), "a save that went through left the map flagged as unsaveable");
        QVERIFY2(mpMapper->toolButton_saveWarning->isHidden(), "the warning stayed up after a save went through");
    }
};

#include "MapperPanelTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapperPanelTest)
