/***************************************************************************
 *   Copyright (C) 2026 by Piotr Wilczynski - delwing@gmail.com            *
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
 * A room selection belongs to the area it was made in: showing another area
 * has to drop it, or it stays live but off-screen and the context menu's
 * Delete then removes rooms the user cannot see (issue #9933).
 *
 * The selection is private to the mapper widget and there is no Lua way to
 * make one, so this cannot be a busted spec.
 *
 * Run with: ctest -R MapAreaSelectionTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapAreaSelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("MapAreaSelection-Test");
    const QString mAreaAName = qsl("area A");
    const QString mAreaBName = qsl("area B");
    int mAreaA = 0;
    int mAreaB = 0;
    static constexpr int scmRoomInA = 100;
    static constexpr int scmSecondRoomInA = 101;
    static constexpr int scmRoomInB = 200;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // One room in each of two areas, with the widget showing area A. A second
    // room in area A gives the multi-room case something to select.
    void buildTwoAreaMap()
    {
        map()->mapClear();
        QVERIFY(roomDB()->getRoomIDList().isEmpty());

        mAreaA = roomDB()->addArea(mAreaAName);
        mAreaB = roomDB()->addArea(mAreaBName);
        QVERIFY(mAreaA > 0);
        QVERIFY(mAreaB > 0);

        for (const int roomId : {scmRoomInA, scmSecondRoomInA, scmRoomInB}) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, roomId == scmRoomInB ? mAreaB : mAreaA));
            QVERIFY(map()->setRoomCoordinates(roomId, roomId % 10, 0, 0));
        }

        mp2dMap->switchArea(mAreaAName);
        QCOMPARE(mp2dMap->mAreaID, mAreaA);
    }

    void selectRooms(const QSet<int>& roomIds)
    {
        mp2dMap->mMultiSelectionSet = roomIds;
        QVERIFY(mp2dMap->getCenterSelection());
        QCOMPARE(mp2dMap->mMultiSelectionSet, roomIds);
        QVERIFY(roomIds.contains(mp2dMap->getCenterSelectedRoomId()));
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
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        // What dlgMapper's constructor does to the widget it owns; the dialog
        // itself is not needed for any of the paths under test
        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The area dropdown, which is what dlgMapper and TMapView both drive
    void switchingAreaDropsTheSelection()
    {
        buildTwoAreaMap();
        selectRooms({scmRoomInA});

        mp2dMap->switchArea(mAreaBName);

        QCOMPARE(mp2dMap->mAreaID, mAreaB);
        QVERIFY2(mp2dMap->mMultiSelectionSet.isEmpty(), "the room selected in area A survived the switch to area B");
        QCOMPARE(mp2dMap->getCenterSelectedRoomId(), 0);
    }

    void switchingAreaDropsAMultiRoomSelection()
    {
        buildTwoAreaMap();
        selectRooms({scmRoomInA, scmSecondRoomInA});

        mp2dMap->switchArea(mAreaB);

        QCOMPARE(mp2dMap->mAreaID, mAreaB);
        QVERIFY2(mp2dMap->mMultiSelectionSet.isEmpty(), "the rooms selected in area A survived the switch to area B");
        QCOMPARE(mp2dMap->getCenterSelectedRoomId(), 0);
    }

    // Lua centerview() on a room in another area
    void centerviewOnAnotherAreaDropsTheSelection()
    {
        buildTwoAreaMap();
        selectRooms({scmRoomInA});

        const auto [centered, message] = mp2dMap->centerview(scmRoomInB);
        QVERIFY2(centered, qPrintable(message));

        QCOMPARE(mp2dMap->mAreaID, mAreaB);
        QVERIFY2(mp2dMap->mMultiSelectionSet.isEmpty(), "the room selected in area A survived a centerview() into area B");
        QCOMPARE(mp2dMap->getCenterSelectedRoomId(), 0);
    }

    // Staying put is not an area change, so nothing is dropped
    void centerviewWithinTheSameAreaKeepsTheSelection()
    {
        buildTwoAreaMap();
        selectRooms({scmRoomInA});

        const auto [centered, message] = mp2dMap->centerview(scmSecondRoomInA);
        QVERIFY2(centered, qPrintable(message));

        QCOMPARE(mp2dMap->mAreaID, mAreaA);
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{scmRoomInA});
    }

    // "Move to area" moves the selected rooms and then follows them, so they
    // are on-screen in the new area and stay selected
    void selectionThatMovedToTheNewAreaIsKept()
    {
        buildTwoAreaMap();
        const QSet<int> selection{scmRoomInA, scmSecondRoomInA};
        selectRooms(selection);

        for (const int roomId : selection) {
            QVERIFY(map()->setRoomArea(roomId, mAreaB));
        }
        mp2dMap->switchArea(mAreaBName);

        QCOMPARE(mp2dMap->mAreaID, mAreaB);
        QCOMPARE(mp2dMap->mMultiSelectionSet, selection);
    }

    // Only the rooms left behind go
    void onlyTheRoomsOutsideTheNewAreaAreDropped()
    {
        buildTwoAreaMap();
        selectRooms({scmRoomInA, scmSecondRoomInA});
        QVERIFY(map()->setRoomArea(scmSecondRoomInA, mAreaB));

        mp2dMap->switchArea(mAreaBName);

        QCOMPARE(mp2dMap->mAreaID, mAreaB);
        QCOMPARE(mp2dMap->mMultiSelectionSet, QSet<int>{scmSecondRoomInA});
        QCOMPARE(mp2dMap->getCenterSelectedRoomId(), scmSecondRoomInA);
    }
};

#include "MapAreaSelectionTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapAreaSelectionTest)
