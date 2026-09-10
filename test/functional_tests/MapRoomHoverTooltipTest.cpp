/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet authors                              *
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
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * Hovering the mouse over a room in the 2D mapper shows a tooltip with the
 * room number and name on the first line and its exits on the second; when
 * several rooms are stacked under the cursor they are separated by a
 * horizontal rule. The tooltip text is built by T2DMap::roomHoverTooltip(),
 * which is the pure, testable seam - the async QToolTip popup itself is
 * platform/timing dependent and not asserted here.
 *
 * Run with: ctest -R MapRoomHoverTooltipTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapRoomHoverTooltipTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("MapRoomHoverTooltip-Test");
    const QString mAreaName = qsl("hover area");
    int mAreaId = 0;

    static constexpr int kNamedRoom = 1;
    static constexpr int kBlankRoom = 2;
    static constexpr int kSpecialRoom = 3;

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void addRoom(int id, int x, int y, int z)
    {
        QVERIFY(map()->addRoom(id));
        QVERIFY(map()->setRoomArea(id, mAreaId));
        QVERIFY(map()->setRoomCoordinates(id, x, y, z));
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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;

        map()->mapClear();
        mAreaId = roomDB()->addArea(mAreaName);
        QVERIFY(mAreaId > 0);
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void namedRoomShowsNumberNameAndExits()
    {
        addRoom(kNamedRoom, 0, 0, 0);
        TRoom* room = roomDB()->getRoom(kNamedRoom);
        room->name = qsl("Town Square");
        QVERIFY(room->setExit(kNamedRoom + 10, DIR_NORTH));
        QVERIFY(room->setExit(kNamedRoom + 11, DIR_EAST));
        room->setUp(kNamedRoom + 12);

        const QString tip = mp2dMap->roomHoverTooltip({kNamedRoom});
        QVERIFY(!tip.isEmpty());
        QVERIFY(tip.contains(qsl("white-space:pre")));
        QVERIFY(tip.contains(qsl("#%1: Town Square").arg(kNamedRoom)));
        QVERIFY(tip.contains(qsl("n")));
        QVERIFY(tip.contains(qsl("e")));
        QVERIFY(tip.contains(qsl("up")));
        QVERIFY(tip.contains(qsl("Exits:"))); // the exits line is labelled and translatable
        QVERIFY(!tip.contains(qsl("<hr>")));
    }

    void blankRoomShowsNumberAndNoExits()
    {
        addRoom(kBlankRoom, 1, 0, 0);
        TRoom* room = roomDB()->getRoom(kBlankRoom);
        room->name.clear();

        const QString tip = mp2dMap->roomHoverTooltip({kBlankRoom});
        QVERIFY(!tip.isEmpty());
        QVERIFY(tip.contains(qsl("#%1").arg(kBlankRoom)));
        QVERIFY(!tip.contains(qsl("#%1:").arg(kBlankRoom))); // no name, so no colon after the number
        QVERIFY(tip.contains(qsl("(no exits)")));
    }

    void specialExitsAreIncluded()
    {
        addRoom(kSpecialRoom, 2, 0, 0);
        TRoom* room = roomDB()->getRoom(kSpecialRoom);
        room->name = qsl("Portal Room");
        room->setSpecialExit(kSpecialRoom + 1, qsl("enter portal"));

        const QString tip = mp2dMap->roomHoverTooltip({kSpecialRoom});
        QVERIFY(tip.contains(qsl("enter portal")));
    }

    void stackedRoomsAreSeparatedByHorizontalRule()
    {
        // Two rooms on the same coordinate - the cursor covers both.
        addRoom(10, 5, 5, 0);
        TRoom* a = roomDB()->getRoom(10);
        a->name = qsl("Alpha");
        QVERIFY(a->setExit(11, DIR_SOUTH));
        addRoom(11, 5, 5, 0);
        TRoom* b = roomDB()->getRoom(11);
        b->name = qsl("Beta");
        QVERIFY(b->setExit(10, DIR_NORTH));

        const QString tip = mp2dMap->roomHoverTooltip({10, 11});
        QVERIFY(tip.contains(qsl("<hr>")));
        QVERIFY(tip.contains(qsl("#10")));
        QVERIFY(tip.contains(qsl("Alpha")));
        QVERIFY(tip.contains(qsl("#11")));
        QVERIFY(tip.contains(qsl("Beta")));
    }

    void roomNameIsHtmlEscaped()
    {
        addRoom(20, 6, 0, 0);
        TRoom* room = roomDB()->getRoom(20);
        room->name = qsl("<script>x</script>");
        const QString tip = mp2dMap->roomHoverTooltip({20});
        QVERIFY(tip.contains(qsl("&lt;script&gt;")));
        QVERIFY(!tip.contains(qsl("<script>x</script>")));
    }

    void emptyOrNullReturnsEmpty()
    {
        QCOMPARE(mp2dMap->roomHoverTooltip({}), QString());
        QCOMPARE(mp2dMap->roomHoverTooltip({999999}), QString()); // nonexistent room
    }

    void zeroDelayDisablesMouseTracking()
    {
        // A delay of 0 turns the feature off: mouse tracking goes off so plain
        // motion generates no events, and any pending hover state is cleared.
        mp2dMap->setRoomHoverDelay(300);
        QVERIFY(mp2dMap->hasMouseTracking());
        mp2dMap->setRoomHoverDelay(0);
        QVERIFY(!mp2dMap->hasMouseTracking());
        // Re-enabling restores tracking.
        mp2dMap->setRoomHoverDelay(300);
        QVERIFY(mp2dMap->hasMouseTracking());
    }
};

#include "MapRoomHoverTooltipTest.moc"

MUDLET_GROUPED_TEST_MAIN(MapRoomHoverTooltipTest)
