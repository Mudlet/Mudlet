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
 * Covers TMap::retrieveMapFileStats(), which reads another profile's saved map
 * far enough to say what it holds without loading it. The preferences' Mapper
 * tab is its only caller - it fills in the "copy a map from another profile"
 * list - so there is no Lua route to it, and the tests below call it directly.
 *
 * Reading somebody else's file with somebody else's serializer is the whole
 * point of it: the map it reports on must not become the map it is called on.
 * The player's room is looked up by profile name, so it also has to be the
 * requested profile's room rather than the reading profile's.
 *
 * Run with: ctest -R MapFileStatsTest -V
 */

#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapFileStatsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapFileStats-Test");
    // The profile whose map file is being reported on. It never needs to be a
    // real profile - only its map directory is ever read.
    const QString mOtherProfileName = qsl("MapFileStatsOther-Test");
    static constexpr int scmPlayerRoomId = 3;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void deleteProfileDirectory(const QString& profileName) const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    QString otherProfileMapDir() const { return mudlet::getMudletPath(enums::profileMapsPath, mOtherProfileName); }

    // Two areas, four rooms, and a player room recorded against the profile the
    // file claims to belong to.
    void buildMapToSave() const
    {
        map()->mapClear();
        TRoomDB* pDB = map()->mpRoomDB.get();
        const int areaA = pDB->addArea(qsl("Area A"));
        const int areaB = pDB->addArea(qsl("Area B"));
        QVERIFY(areaA > 0);
        QVERIFY(areaB > 0);
        int id = 1;
        for (const int areaId : {areaA, areaB}) {
            for (int i = 0; i < 2; ++i, ++id) {
                QVERIFY(map()->addRoom(id));
                QVERIFY(map()->setRoomArea(id, areaId));
                QVERIFY(map()->setRoomCoordinates(id, i, i, 0));
            }
        }
        map()->mRoomIdHash[mOtherProfileName] = scmPlayerRoomId;
        // The reading profile's own player room, which must not be the one
        // reported back for the other profile:
        map()->mRoomIdHash[mProfileName] = 1;
    }

    // The same QDataStream setup TMainConsole::saveMap uses
    bool writeMapFile(const QString& pathFileName) const
    {
        QSaveFile file(pathFileName);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QDataStream out(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        return map()->serialize(out) && file.commit();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
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
        deleteProfileDirectory(mProfileName);
        deleteProfileDirectory(mOtherProfileName);

        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = hostManager.getHost(mProfileName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            deleteProfileDirectory(mOtherProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QDir dir(otherProfileMapDir());
        if (dir.exists()) {
            QVERIFY(dir.removeRecursively());
        }
    }

    void test_noProfileNameIsRejected() { QVERIFY(!map()->retrieveMapFileStats(QString(), nullptr, nullptr, nullptr, nullptr, nullptr)); }

    void test_profileWithNoSavedMapIsRejected()
    {
        QVERIFY2(!QFileInfo::exists(otherProfileMapDir()), "the other profile's map directory should not exist yet");
        QVERIFY(!map()->retrieveMapFileStats(mOtherProfileName, nullptr, nullptr, nullptr, nullptr, nullptr));

        // ...and an empty map directory is no better than a missing one:
        QVERIFY(QDir().mkpath(otherProfileMapDir()));
        QVERIFY(!map()->retrieveMapFileStats(mOtherProfileName, nullptr, nullptr, nullptr, nullptr, nullptr));
    }

    void test_savedMapIsReportedWithoutDisturbingThisMap()
    {
        buildMapToSave();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(QDir().mkpath(otherProfileMapDir()));
        const QString pathFileName = qsl("%1/20260819-01-01-01map").arg(otherProfileMapDir());
        QVERIFY(writeMapFile(pathFileName));

        // What this map holds before the read, to compare against afterwards:
        const int ownRoomCount = map()->mpRoomDB->getRoomMap().size();
        const int ownPlayerRoom = map()->mRoomIdHash.value(mProfileName);

        QString latestFileName;
        int fileVersion = 0;
        int roomId = 0;
        qsizetype areaCount = 0;
        qsizetype roomCount = 0;
        QVERIFY(map()->retrieveMapFileStats(mOtherProfileName, &latestFileName, &fileVersion, &roomId, &areaCount, &roomCount));

        QCOMPARE(latestFileName, pathFileName);
        QCOMPARE(fileVersion, map()->mVersion);
        // The player room recorded for the profile asked about, not for this one:
        QCOMPARE(roomId, scmPlayerRoomId);
        // The two areas made above plus the default area every map carries:
        QCOMPARE(areaCount, 3);
        QCOMPARE(roomCount, 4);

        QCOMPARE(map()->mpRoomDB->getRoomMap().size(), ownRoomCount);
        QCOMPARE(map()->mRoomIdHash.value(mProfileName), ownPlayerRoom);
    }

    void test_theMostRecentlyWrittenMapFileIsTheOneReported()
    {
        buildMapToSave();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(QDir().mkpath(otherProfileMapDir()));
        const QString older = qsl("%1/20260101-01-01-01map").arg(otherProfileMapDir());
        const QString newer = qsl("%1/20260819-01-01-01map").arg(otherProfileMapDir());
        QVERIFY(writeMapFile(older));
        QVERIFY(writeMapFile(newer));

        // Both were written in the same instant, so say which is which rather
        // than trust the filesystem's timestamp resolution:
        QFile olderFile(older);
        QVERIFY(olderFile.open(QIODevice::ReadWrite));
        QVERIFY(olderFile.setFileTime(QDateTime::currentDateTime().addDays(-30), QFileDevice::FileModificationTime));
        olderFile.close();

        QString latestFileName;
        QVERIFY(map()->retrieveMapFileStats(mOtherProfileName, &latestFileName, nullptr, nullptr, nullptr, nullptr));
        QCOMPARE(latestFileName, newer);
    }

    // A map saved by a newer Mudlet than this one: the version is all that can
    // be said about it, and saying it is how the preferences dialog knows to
    // refuse the copy rather than read a format it does not understand.
    void test_aMapFromANewerMudletIsReportedByVersionAlone()
    {
        QVERIFY(QDir().mkpath(otherProfileMapDir()));
        const QString pathFileName = qsl("%1/20260819-02-02-02map").arg(otherProfileMapDir());
        QSaveFile file(pathFileName);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QDataStream out(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        const int impossibleVersion = map()->mVersion + 1;
        out << impossibleVersion;
        QVERIFY(file.commit());

        QString latestFileName;
        int fileVersion = 0;
        qsizetype areaCount = -1;
        qsizetype roomCount = -1;
        QVERIFY(map()->retrieveMapFileStats(mOtherProfileName, &latestFileName, &fileVersion, nullptr, &areaCount, &roomCount));

        QCOMPARE(latestFileName, pathFileName);
        QCOMPARE(fileVersion, impossibleVersion);
        // Nothing past the version was read, so the counts are untouched:
        QCOMPARE(areaCount, -1);
        QCOMPARE(roomCount, -1);
    }
};

#include "MapFileStatsTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapFileStatsTest)
