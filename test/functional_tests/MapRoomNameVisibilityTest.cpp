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
 * With room names shown on the map, a room whose ROOM_UI_SHOWNAME is off
 * (setRoomNameVisible(room, false)) keeps its name off the 2D map, and every
 * other room shows its name.
 *
 * Why not a spec: the only evidence is pixels, and nothing in the Lua API
 * reads back what the mapper drew; exportAreaImage() draws no room names.
 *
 * Run with: ctest -R MapRoomNameVisibilityTest -V
 */

#include <QPixmap>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

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

class MapRoomNameVisibilityTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapRoomNameVisibility-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // Forty pixels a room, so names are drawn well above the smallest size
    static constexpr double kZoom = 10.0;
    static constexpr int kWestRoomId = 1;
    static constexpr int kEastRoomId = 2;
    // Parked off screen, so its ring cannot be mistaken for a name
    static constexpr int kPlayerRoomId = 3;
    static constexpr int kEnvRoom = 501;

    static QColor nameColour() { return QColor(255, 0, 255); }

    TMap* map() const { return mpHost->mpMap.data(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TRoom* addRoomAt(const int id, const int areaId, const int x, const int y, const QString& name) const
    {
        if (!map()->addRoom(id) || !map()->setRoomArea(id, areaId) || !map()->setRoomCoordinates(id, x, y, 0)) {
            return nullptr;
        }
        TRoom* pRoom = map()->mpRoomDB->getRoom(id);
        if (pRoom) {
            pRoom->environment = kEnvRoom;
            pRoom->name = name;
            pRoom->userData.insert(ROOM_UI_NAMECOLOR, nameColour().name());
        }
        return pRoom;
    }

    T2DMap* prepareWidget(const int areaId) const
    {
        mpHost->mMapperShowGrid = false;
        mpHost->mMapperShowRoomBorders = false;
        mpHost->mMapperCenterSmallAreas = false;
        mpHost->mBgColor_2 = QColor(0, 0, 0);
        mpHost->mMapInfoContributors.clear();

        TMap* pMap = map();
        if (!pMap->mapper()) {
            mpHost->showHideOrCreateMapper(false);
        }
        if (!pMap->mapper() || !pMap->mapper()->mp2dMap) {
            return nullptr;
        }
        T2DMap* p2dMap = pMap->mapper()->mp2dMap;
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
        pMap->mpRoomDB->getArea(areaId)->set2DMapZoom(kZoom);
        return p2dMap;
    }

    static QImage renderFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        return target.toImage();
    }

    // Where the name's pixels are: antialiased magenta on black keeps red and blue level and green well below them
    static QRect nameArea(const QImage& image, const int fromX, const int toX)
    {
        QRect area;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = fromX; x < toX; ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.red() > 60 && pixel.blue() > 60 && qAbs(pixel.red() - pixel.blue()) < 40 && pixel.green() < qMin(pixel.red(), pixel.blue()) / 2) {
                    area |= QRect(x, y, 1, 1);
                }
            }
        }
        return area;
    }

    QString savedFor(const QImage& frame, const QString& label) const
    {
        const QString framePath = qsl("%1/MapRoomNameVisibilityTest-%2.png").arg(QDir::tempPath(), label);
        return frame.save(framePath) ? qsl("The frame is at %1").arg(framePath) : qsl("The frame could not be saved to %1").arg(framePath);
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

    void test_aRoomWithItsNameTurnedOffShowsNoName_data()
    {
        QTest::addColumn<QString>("westFlag");
        QTest::addColumn<bool>("westShown");

        QTest::newRow("unset") << QString() << true;
        QTest::newRow("on") << qsl("1") << true;
        QTest::newRow("off") << qsl("0") << false;
    }

    void test_aRoomWithItsNameTurnedOffShowsNoName()
    {
        QFETCH(QString, westFlag);
        QFETCH(bool, westShown);

        TMap* pMap = map();
        pMap->mapClear();
        pMap->mCustomEnvColors[kEnvRoom] = QColor(0, 160, 0);
        const int areaId = pMap->mpRoomDB->addArea(qsl("Names"));
        QVERIFY(areaId > 0);
        TRoom* pWestRoom = addRoomAt(kWestRoomId, areaId, -3, 0, qsl("West Hall\nUpper Floor"));
        QVERIFY(pWestRoom);
        QVERIFY(addRoomAt(kEastRoomId, areaId, 3, 0, qsl("East Hall")));
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 1000, QString()));
        if (!westFlag.isNull()) {
            pWestRoom->userData.insert(ROOM_UI_SHOWNAME, westFlag);
        }
        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);
        pMap->setRoomNamesShown(true);
        const auto restore = qScopeGuard([pMap]() {
            pMap->setRoomNamesShown(false);
        });

        T2DMap* p2dMap = prepareWidget(areaId);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);
        const QRect westName = nameArea(frame, 0, kWidgetWidth / 2);
        const QRect eastName = nameArea(frame, kWidgetWidth / 2, kWidgetWidth);

        if (eastName.width() <= 20) {
            QFAIL(qPrintable(qsl("a room with no name flag of its own shows no name. %1").arg(savedFor(frame, qsl("east")))));
        }
        if (westShown && westName.width() <= 20) {
            QFAIL(qPrintable(qsl("the west room's name is not drawn. %1").arg(savedFor(frame, qsl("west-shown")))));
        }
        // A name taller than the one-room-tall box under the room is drawn in full, not cut to the box
        const int nameBoxHeight = qRound(p2dMap->mRoomHeight * mpHost->mRoomSize);
        if (westShown && westName.height() <= nameBoxHeight) {
            QFAIL(qPrintable(qsl("a name on two lines is drawn %1 pixels tall, no taller than the %2 pixel box under its room. %3")
                                     .arg(westName.height())
                                     .arg(nameBoxHeight)
                                     .arg(savedFor(frame, qsl("two-lines")))));
        }
        if (!westShown && !westName.isNull()) {
            QFAIL(qPrintable(qsl("the west room's name is drawn though it is turned off. %1").arg(savedFor(frame, qsl("west-hidden")))));
        }

        // The map-wide switch still hides every name
        pMap->setRoomNamesShown(false);
        const QImage namesOff = renderFrame(p2dMap);
        if (!nameArea(namesOff, 0, kWidgetWidth).isNull()) {
            QFAIL(qPrintable(qsl("names are drawn with room names turned off. %1").arg(savedFor(namesOff, qsl("names-off")))));
        }
    }
};

#include "MapRoomNameVisibilityTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapRoomNameVisibilityTest)
