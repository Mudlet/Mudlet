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
 * The 2D mapper paints rooms over the exit lines, stubs included, so a stub
 * only shows where it reaches past its room and the border around it. Those
 * grow with the room size setting; each case here checks that the dot on the
 * end of a stub still shows in full outside them.
 *
 * Why not a spec: the only evidence is pixels, and the area image export a
 * script can ask for draws no stubs.
 *
 * Run with: ctest -R MapExitStubTest -V
 */

#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <cmath>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class MapExitStubTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapExitStub-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // Forty pixels a room, so the room, its border and the stub's dot are each several pixels across
    static constexpr double kZoom = 10.0;
    static constexpr int kStubRoomId = 1;
    // Parked off screen, so its highlight cannot cover the stub
    static constexpr int kPlayerRoomId = 999;
    static constexpr int kPlayerRoomY = 1000;
    // Ids above 255 take their colour from mCustomEnvColors
    static constexpr int kEnvRoom = 501;

    static QColor exitColour() { return QColor(0, 255, 128); }

    // Antialiasing is off, so the stub is painted in its own colour
    static bool exitColoured(const QColor& pixel)
    {
        const QColor exit = exitColour();
        return qAbs(pixel.red() - exit.red()) <= 8 && qAbs(pixel.green() - exit.green()) <= 8 && qAbs(pixel.blue() - exit.blue()) <= 8;
    }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    bool addRoomAt(const int id, const int areaId, const int x, const int y) const
    {
        if (!map()->addRoom(id) || !map()->setRoomArea(id, areaId) || !map()->setRoomCoordinates(id, x, y, 0)) {
            return false;
        }
        map()->mpRoomDB->getRoom(id)->environment = kEnvRoom;
        return true;
    }

    QString saveFrame(const QImage& frame, const QString& label) const
    {
        const QString framePath = qsl("%1/MapExitStubTest-%2.png").arg(QDir::tempPath(), label);
        return frame.save(framePath) ? framePath : qsl("(could not be saved to %1)").arg(framePath);
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3s), "could not connect to the telnet stub");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aStubsDotShowsPastItsRoom_data()
    {
        QTest::addColumn<double>("roomSize");
        QTest::addColumn<int>("direction");
        QTest::addColumn<bool>("bordersShown");
        QTest::addColumn<int>("roomBorderThickness");
        QTest::addColumn<bool>("round");

        QTest::newRow("default size, east") << 0.5 << int(DIR_EAST) << true << 0 << false;
        QTest::newRow("size 8, east") << 0.8 << int(DIR_EAST) << true << 0 << false;
        QTest::newRow("size 9, north") << 0.9 << int(DIR_NORTH) << true << 0 << false;
        QTest::newRow("size 10, west") << 1.0 << int(DIR_WEST) << true << 0 << false;
        QTest::newRow("size 11, south") << 1.1 << int(DIR_SOUTH) << true << 0 << false;
        QTest::newRow("size 8, northeast") << 0.8 << int(DIR_NORTHEAST) << true << 0 << false;
        QTest::newRow("size 10, southwest") << 1.0 << int(DIR_SOUTHWEST) << true << 0 << false;
        QTest::newRow("size 10, borders off") << 1.0 << int(DIR_EAST) << false << 0 << false;
        QTest::newRow("size 8, thick room border") << 0.8 << int(DIR_EAST) << true << 4 << false;
        QTest::newRow("size 10, round rooms") << 1.0 << int(DIR_EAST) << true << 0 << true;
    }

    void test_aStubsDotShowsPastItsRoom()
    {
        QFETCH(double, roomSize);
        QFETCH(int, direction);
        QFETCH(bool, bordersShown);
        QFETCH(int, roomBorderThickness);
        QFETCH(bool, round);

        TMap* pMap = map();
        pMap->mapClear();
        pMap->mCustomEnvColors[kEnvRoom] = QColor(255, 128, 0);
        const int areaId = pMap->mpRoomDB->addArea(qsl("Stub Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, kPlayerRoomY));
        QVERIFY(addRoomAt(kStubRoomId, areaId, 0, 0));
        TRoom* pRoom = pMap->mpRoomDB->getRoom(kStubRoomId);
        pRoom->setExitStub(direction, true);
        QVERIFY(pRoom->exitStubs.contains(direction));
        pRoom->mBorderThickness = roomBorderThickness;
        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);

        const double borderSize = 10.0;
        mpHost->mRoomSize = roomSize;
        mpHost->mRoomBorderSize = borderSize;
        mpHost->mLineSize = 10.0;
        mpHost->mMapperUseAntiAlias = false;
        mpHost->mMapperShowGrid = false;
        mpHost->mMapperShowRoomBorders = bordersShown;
        mpHost->mRoomBorderColor = QColor(Qt::white);
        mpHost->mMapperCenterSmallAreas = false;
        mpHost->mBgColor_2 = QColor(Qt::black);
        mpHost->mFgColor_2 = exitColour();
        mpHost->mMapInfoContributors.clear();

        if (!pMap->mapper()) {
            mpHost->showHideOrCreateMapper(false);
        }
        QVERIFY(pMap->mapper());
        T2DMap* p2dMap = pMap->mapper()->mp2dMap;
        QVERIFY(p2dMap);
        p2dMap->init();
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        // paintEvent() re-centres on the player unless the player's room, mRoomID and mShiftMode all agree
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = areaId;
        p2dMap->mMapCenterX = 0;
        p2dMap->mMapCenterY = 0;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMultiSelectionSet.clear();
        p2dMap->mShowRoomID = false;
        p2dMap->mBubbleMode = round;
        pMap->mpRoomDB->getArea(areaId)->set2DMapZoom(kZoom);

        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        p2dMap->mBubbleMode = false;
        const QImage frame = target.toImage();

        const double cell = p2dMap->mRoomWidth;
        QCOMPARE(cell, kWidgetHeight / kZoom);
        const double thickness = roomBorderThickness > 0 ? roomBorderThickness : 1.0;
        const int borderWidth = qMax(1, static_cast<int>(thickness / borderSize * cell * roomSize));
        // Where the room's fill and border stop, with a pixel for rounding
        const double roomReach = roomSize * cell / 2.0 + borderWidth + 1.0;
        const QPointF centre(p2dMap->mRX, p2dMap->mRY);

        int stubPixelsPastRoom = 0;
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                if ((std::abs(x - centre.x()) > roomReach || std::abs(y - centre.y()) > roomReach) && exitColoured(frame.pixelColor(x, y))) {
                    ++stubPixelsPastRoom;
                }
            }
        }

        // The dot on the stub's end is a fifth of a room across
        const double dotRadius = 0.1 * cell;
        const int wholeDot = static_cast<int>(0.9 * M_PI * dotRadius * dotRadius);
        if (stubPixelsPastRoom < wholeDot) {
            QFAIL(qPrintable(qsl("only %1 pixels of the stub show past the room, short of the %2 its end dot covers - the frame is at %3")
                                     .arg(QString::number(stubPixelsPastRoom), QString::number(wholeDot), saveFrame(frame, QString::fromLatin1(QTest::currentDataTag())))));
        }
    }
};

#include "MapExitStubTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapExitStubTest)
