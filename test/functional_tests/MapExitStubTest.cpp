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
 * grow with the room size setting, and a room next door covers whatever of
 * the stub reaches it. Each case here checks that at least as much of the
 * stub as its end dot covers still shows.
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

#include <numbers>

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
    // Eighty pixels a room, so the stub's dot is sixteen across
    static constexpr double kZoom = 5.0;
    static constexpr int kStubRoomId = 1;
    static constexpr int kNeighbourRoomId = 2;
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
        // A room in the next cell along the stub, which is painted over whatever of the stub reaches it
        QTest::addColumn<bool>("neighbour");

        QTest::newRow("default size, east") << 0.5 << int(DIR_EAST) << true << 0 << false << false;
        QTest::newRow("size 8, east") << 0.8 << int(DIR_EAST) << true << 0 << false << false;
        QTest::newRow("size 9, north") << 0.9 << int(DIR_NORTH) << true << 0 << false << false;
        QTest::newRow("size 10, west") << 1.0 << int(DIR_WEST) << true << 0 << false << false;
        QTest::newRow("size 11, south") << 1.1 << int(DIR_SOUTH) << true << 0 << false << false;
        QTest::newRow("size 9, northeast") << 0.9 << int(DIR_NORTHEAST) << true << 0 << false << false;
        QTest::newRow("size 10, southwest") << 1.0 << int(DIR_SOUTHWEST) << true << 0 << false << false;
        QTest::newRow("size 10, borders off") << 1.0 << int(DIR_EAST) << false << 0 << false << false;
        QTest::newRow("size 8, thick room border") << 0.8 << int(DIR_EAST) << true << 4 << false << false;
        QTest::newRow("size 10, round rooms") << 1.0 << int(DIR_EAST) << true << 0 << true << false;
        QTest::newRow("size 11, round rooms, northwest") << 1.1 << int(DIR_NORTHWEST) << true << 0 << true << false;
        // The gap between the two rooms only just holds the dot
        QTest::newRow("size 7, room next door") << 0.7 << int(DIR_EAST) << true << 0 << false << true;
        QTest::newRow("size 8, borders off, room next door") << 0.8 << int(DIR_EAST) << false << 0 << false << true;
        QTest::newRow("size 10, round rooms, room next door northeast") << 1.0 << int(DIR_NORTHEAST) << true << 0 << true << true;
    }

    void test_aStubsDotShowsPastItsRoom()
    {
        QFETCH(double, roomSize);
        QFETCH(int, direction);
        QFETCH(bool, bordersShown);
        QFETCH(int, roomBorderThickness);
        QFETCH(bool, round);
        QFETCH(bool, neighbour);

        TMap* pMap = map();
        pMap->mapClear();
        pMap->mCustomEnvColors[kEnvRoom] = QColor(255, 128, 0);
        const int areaId = pMap->mpRoomDB->addArea(qsl("Stub Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, kPlayerRoomY));
        QVERIFY(addRoomAt(kStubRoomId, areaId, 0, 0));
        if (neighbour) {
            const QVector3D step = TMap::scmUnitVectors.value(direction);
            // scmUnitVectors points north at -y, the drawing direction, but room coordinates have north at +y
            QVERIFY(addRoomAt(kNeighbourRoomId, areaId, qRound(step.x()), -qRound(step.y())));
        }
        TRoom* pRoom = pMap->mpRoomDB->getRoom(kStubRoomId);
        pRoom->setExitStub(direction, true);
        QVERIFY(pRoom->exitStubs.contains(direction));
        pRoom->mBorderThickness = roomBorderThickness;
        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);

        mpHost->mRoomSize = roomSize;
        mpHost->mRoomBorderSize = 10.0;
        // A hairline stub, so the pixels counted are the dot's rather than the line's
        mpHost->mLineSize = 100.0;
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

        // Rooms are painted over the stub, so every pixel left in its colour is a pixel of it that shows
        int stubPixelsShown = 0;
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                if (exitColoured(frame.pixelColor(x, y))) {
                    ++stubPixelsShown;
                }
            }
        }

        // The dot on the stub's end is a fifth of a room across
        const double dotRadius = 0.1 * cell;
        const int wholeDot = static_cast<int>(0.9 * std::numbers::pi * dotRadius * dotRadius);
        if (stubPixelsShown < wholeDot) {
            QFAIL(qPrintable(qsl("only %1 pixels of the stub show, short of the %2 its end dot covers - the frame is at %3")
                                     .arg(QString::number(stubPixelsShown), QString::number(wholeDot), saveFrame(frame, QString::fromLatin1(QTest::currentDataTag())))));
        }
    }
};

#include "MapExitStubTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapExitStubTest)
