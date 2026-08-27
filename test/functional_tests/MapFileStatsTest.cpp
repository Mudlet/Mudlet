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
 * far enough to say what it holds without loading it. Its one caller is
 * dlgProfilePreferences::slot_copyMap(), behind the preferences' "copy to N
 * destination(s)" button: before this profile's map is written into each
 * destination, that destination's own last saved map is read for the room its
 * player was standing in, so the copy does not move them. There is no Lua route
 * to any of it, so the tests below call it directly.
 *
 * Reading somebody else's file without becoming it is the whole point: the map
 * reported on must not replace the map the call was made on, and the player
 * room is keyed by profile name, so it has to be the requested profile's entry
 * rather than the reading profile's.
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

    // The player room has to be recorded against the profile the file will
    // claim to belong to, since that is the entry the read looks up.
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

    // The same QDataStream setup TMainConsole::saveMap uses. saveVersion 0 means
    // the map's own; anything else has to be within mMinVersion..mMaxVersion.
    bool writeMapFile(const QString& pathFileName, const int saveVersion = 0) const
    {
        QSaveFile file(pathFileName);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QDataStream out(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        return map()->serialize(out, saveVersion) && file.commit();
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
        // An older format than this build's own, so what comes back is read
        // from the file rather than being this map's version by coincidence:
        QVERIFY(writeMapFile(pathFileName, 19));

        // Every one of the five answers has a counterpart in this map, so the
        // live map is moved away from all of them before the read - otherwise a
        // reader that returned its own state instead of the file's would pass:
        QVERIFY(map()->addRoom(99));
        QVERIFY(map()->setRoomArea(99, 1));
        map()->mRoomIdHash[mOtherProfileName] = 99;
        map()->mRoomIdHash[mProfileName] = 99;
        const int ownRoomCount = map()->mpRoomDB->getRoomMap().size();
        QCOMPARE(ownRoomCount, 5);

        QString latestFileName;
        int fileVersion = 0;
        int roomId = 0;
        qsizetype areaCount = 0;
        qsizetype roomCount = 0;
        QVERIFY(map()->retrieveMapFileStats(mOtherProfileName, &latestFileName, &fileVersion, &roomId, &areaCount, &roomCount));

        QCOMPARE(latestFileName, pathFileName);
        QCOMPARE(fileVersion, 19);
        // The player room the file records for the profile asked about, not the
        // one this map now holds for either profile:
        QCOMPARE(roomId, scmPlayerRoomId);
        // The two areas made above plus the default area every map carries:
        QCOMPARE(areaCount, 3);
        QCOMPARE(roomCount, 4);

        // ...and reading it changed nothing here:
        QCOMPARE(map()->mpRoomDB->getRoomMap().size(), ownRoomCount);
        QCOMPARE(map()->mRoomIdHash.value(mProfileName), 99);
        QCOMPARE(map()->mRoomIdHash.value(mOtherProfileName), 99);
        QCOMPARE(map()->mVersion, map()->mDefaultVersion);
    }

    // The pick is by modification time. The names deliberately sort the other
    // way round, so a read that went by name - in either direction - would name
    // the wrong file rather than happen to agree.
    void test_theMostRecentlyWrittenMapFileIsTheOneReported()
    {
        buildMapToSave();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(QDir().mkpath(otherProfileMapDir()));
        const QString first = qsl("%1/20260819-01-01-01map").arg(otherProfileMapDir());
        const QString second = qsl("%1/20260101-01-01-01map").arg(otherProfileMapDir());
        QVERIFY(writeMapFile(first));
        QVERIFY(writeMapFile(second));

        // Both were written in the same instant, so say which is which rather
        // than trust the filesystem's timestamp resolution:
        QFile firstFile(first);
        QVERIFY(firstFile.open(QIODevice::ReadWrite));
        QVERIFY(firstFile.setFileTime(QDateTime::currentDateTime().addDays(-30), QFileDevice::FileModificationTime));
        firstFile.close();

        QString latestFileName;
        QVERIFY(map()->retrieveMapFileStats(mOtherProfileName, &latestFileName, nullptr, nullptr, nullptr, nullptr));
        QCOMPARE(latestFileName, second);
    }

    // A map saved by a newer Mudlet than this one: the read stops at the version
    // rather than try to make sense of a layout it does not know, and the caller
    // gets the version so it can say so. Which of the two early returns takes it
    // depends on the build - a release or public test build stops as soon as the
    // version is past mDefaultVersion, a development one goes on to compare it
    // against mMaxVersion - but both leave everything after the version unread.
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
