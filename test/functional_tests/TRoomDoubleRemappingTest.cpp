/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vperetokin@gmail.com          *
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

#include <QtTest/QtTest>

#include "TelnetServerStub.h"
#include "Host.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void        initializeQRCResources();

class TRoomDoubleRemappingTest : public QObject {
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-DoubleRemap";
    const QString mPort = "4001";
    const QString mLocalhost = "localhost";

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        if (!mudlet::self()->getActiveHost()) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mudlet::self()->getActiveHost()->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
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
        initializeQRCResources();
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // Regression test for TRoom.cpp:1543-1544 double remapping bug.
    //
    // When a room exit points to a room with an invalid (negative) ID, the
    // audit process remaps it to a valid positive ID via a roomRemapping hash.
    // The bug: the remapping lookup was done TWICE on the same line:
    //   exitRoomId = roomRemapping.value(exitRoomId);  // correct: -5 → 200
    //   exitRoomId = roomRemapping.value(exitRoomId);  // bug: 200 → 0 (not in hash)
    // The second lookup uses the already-remapped ID as key, which is NOT in
    // the hash, so QHash::value() returns 0, silently corrupting the exit.
    void test_auditExitRemapsOnce()
    {
        startProfile(mHostname, mLocalhost, mPort);

        auto* pHost = mudlet::self()->getActiveHost();
        QVERIFY2(pHost, "Host must exist");
        QVERIFY2(pHost->mpMap, "Map must exist");

        // Open the mapper so the map infrastructure is fully initialized
        pHost->openMapWidget({}, 0, 0, 0, 0);
        QTest::qWait(200);

        auto* pRoomDB = pHost->mpMap->mpRoomDB;
        QVERIFY2(pRoomDB, "RoomDB must exist");

        // Create the destination room that the exit should point to after remapping
        QVERIFY(pRoomDB->addRoom(200));

        // Create a room to call auditExit on (needs valid mpRoomDB chain)
        QVERIFY(pRoomDB->addRoom(10));
        auto* pRoom = pRoomDB->getRoom(10);
        QVERIFY(pRoom);

        // Simulate an exit that points to a room with a negative (invalid) ID.
        // The roomRemapping says: room -5 has been remapped to room 200.
        QHash<int, int> roomRemapping;
        roomRemapping.insert(-5, 200);

        // Set up the empty containers that auditExit needs as working copies
        QMap<QString, int> exitWeights;
        QSet<int> exitStubs;
        QSet<int> exitLocks;
        QMap<QString, int> doors;
        QMap<QString, QList<QPointF>> customLines;
        QMap<QString, QColor> customLinesColor;
        QMap<QString, Qt::PenStyle> customLinesStyle;
        QMap<QString, bool> customLinesArrow;

        // This is the exit value we're testing — it starts as -5 (the old invalid ID)
        int northExit = -5;

        // Call auditExit directly. This passes northExit by reference.
        // With the bug (duplicate line), northExit becomes 0.
        // With the fix (single line), northExit becomes 200.
        pRoom->auditExit(northExit,
                         DIR_NORTH,
                         QStringLiteral("North"),
                         QStringLiteral("n"),
                         exitWeights,
                         exitStubs,
                         exitLocks,
                         doors,
                         customLines,
                         customLinesColor,
                         customLinesStyle,
                         customLinesArrow,
                         roomRemapping);

        // THE CRITICAL ASSERTION: the exit must point to the remapped room 200
        QVERIFY2(northExit != 0,
                 "BUG: Exit was corrupted to 0 by double remapping lookup "
                 "(TRoom.cpp duplicate line at roomRemapping.value())");
        QCOMPARE(northExit, 200);
    }

    // Same test but with multiple exits to verify all directions are handled
    void test_auditExitRemapsMultipleExits()
    {
        // Reuse the profile from the previous test (init/cleanup cycle)
        startProfile(mHostname, mLocalhost, mPort);

        auto* pHost = mudlet::self()->getActiveHost();
        QVERIFY(pHost && pHost->mpMap);

        pHost->openMapWidget({}, 0, 0, 0, 0);
        QTest::qWait(200);

        auto* pRoomDB = pHost->mpMap->mpRoomDB;
        QVERIFY(pRoomDB);
        QVERIFY(pRoomDB->addRoom(300));
        QVERIFY(pRoomDB->addRoom(400));
        QVERIFY(pRoomDB->addRoom(20));
        auto* pRoom = pRoomDB->getRoom(20);
        QVERIFY(pRoom);

        QHash<int, int> roomRemapping;
        roomRemapping.insert(-10, 300);
        roomRemapping.insert(-20, 400);

        QMap<QString, int> exitWeights;
        QSet<int> exitStubs;
        QSet<int> exitLocks;
        QMap<QString, int> doors;
        QMap<QString, QList<QPointF>> customLines;
        QMap<QString, QColor> customLinesColor;
        QMap<QString, Qt::PenStyle> customLinesStyle;
        QMap<QString, bool> customLinesArrow;

        int northExit = -10;
        int southExit = -20;

        pRoom->auditExit(northExit, DIR_NORTH, QStringLiteral("North"), QStringLiteral("n"),
                         exitWeights, exitStubs, exitLocks, doors,
                         customLines, customLinesColor, customLinesStyle, customLinesArrow,
                         roomRemapping);

        pRoom->auditExit(southExit, DIR_SOUTH, QStringLiteral("South"), QStringLiteral("s"),
                         exitWeights, exitStubs, exitLocks, doors,
                         customLines, customLinesColor, customLinesStyle, customLinesArrow,
                         roomRemapping);

        QVERIFY2(northExit != 0, "North exit corrupted to 0 by double remapping");
        QCOMPARE(northExit, 300);

        QVERIFY2(southExit != 0, "South exit corrupted to 0 by double remapping");
        QCOMPARE(southExit, 400);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }
};

#include "TRoomDoubleRemappingTest.moc"
QTEST_MAIN(TRoomDoubleRemappingTest)
