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
 * Covers dlgProfilePreferences::slot_copyMap(), the preferences' "copy to N
 * destination(s)" button: it saves this profile's map, copies the file into each
 * destination's map folder and asks the open ones it reached to load it.
 * There is no Lua route to it.
 *
 * Real profiles rather than bare Hosts, because only a profile with a main
 * console hears the request to reload.
 *
 * Run with: ctest -R MapCopyTest -V
 */

#include <algorithm>

#include <QDir>
#include <QMenu>
#include <QPushButton>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class MapCopyTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpSource = nullptr;
    Host* mpDestination = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mSourceName = qsl("MapCopy-Source");
    const QString mDestinationName = qsl("MapCopy-Destination");
    const QString mNoMapYetName = qsl("MapCopy-NoMapYet");
    const QString mDeletedName = qsl("MapCopy-Deleted");
    const QString mLocalhost = qsl("localhost");
    const QString mCopiedArea = qsl("Copied from the source");
    static constexpr int scmCopiedRooms = 5;

    static QString newestMapFile(const QString& profileName)
    {
        return QDir(MudletApp::getMudletPath(enums::profileMapsPath, profileName)).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time).value(0);
    }

    static QStringList areaNames(Host* pHost) { return pHost->mpMap->mpRoomDB->getAreaNamesMap().values(); }

    // The names the source's save can get in the next few seconds
    static QStringList comingMapNames()
    {
        QStringList names;
        const QDateTime now = QDateTime::currentDateTime();
        for (int seconds = 0; seconds < 5; ++seconds) {
            names << qsl("%1map.dat").arg(now.addSecs(seconds).toString(qsl("yyyy-MM-dd#HH-mm-ss")));
        }
        return names;
    }

    static void buildMap(Host* pHost, const QString& areaName, const int rooms)
    {
        TMap* pMap = pHost->mpMap.data();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(areaName);
        QVERIFY(areaId > 0);
        for (int id = 1; id <= rooms; ++id) {
            QVERIFY(pMap->addRoom(id));
            QVERIFY(pMap->setRoomArea(id, areaId));
            QVERIFY(pMap->setRoomCoordinates(id, id, 0, 0));
        }
    }

    void openPreferencesAndTick(const QStringList& destinations)
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpSource);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        for (const auto& destination : destinations) {
            QAction* pDestination = nullptr;
            for (auto* pAction : mpPreferences->pushButton_chooseProfiles->menu()->actions()) {
                if (pAction->text() == destination) {
                    pDestination = pAction;
                }
            }
            QVERIFY2(pDestination, qPrintable(qsl("%1 is not offered as a destination").arg(destination)));
            pDestination->trigger();
        }
        QVERIFY(mpPreferences->pushButton_copyMap->isEnabled());
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
        // The progress label is read back, and a desktop locale would translate it
        QSettings language(qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()), QSettings::IniFormat);
        language.setValue(qsl("interfaceLanguage"), qsl("en_US"));
        language.sync();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        const QString port = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpDestination = TestProfile::create(mDestinationName, mLocalhost, port);
        QVERIFY2(mpDestination, "the destination profile could not be created");
        QSignalSpy destinationConnected(&(mpDestination->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(destinationConnected.count() == 1 || destinationConnected.wait(5s), "the destination profile never connected to the stub game");
        mpSource = TestProfile::create(mSourceName, mLocalhost, port);
        QVERIFY2(mpSource, "the source profile could not be created");
        QVERIFY(mpDestination->mainConsoleView());
    }

    void cleanupTestCase()
    {
        mpSource = nullptr;
        mpDestination = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            for (const auto& name : {mSourceName, mDestinationName, mNoMapYetName, mDeletedName}) {
                TestProfile::removeProfileDirectory(name);
            }
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // A profile made in the connection dialog and never played has no map folder
    void test_aProfileThatHasNeverSavedAMapGetsTheCopy()
    {
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mNoMapYetName)));
        QVERIFY(!QFileInfo::exists(MudletApp::getMudletPath(enums::profileMapsPath, mNoMapYetName)));
        buildMap(mpSource, mCopiedArea, scmCopiedRooms);
        openPreferencesAndTick({mNoMapYetName});
        if (QTest::currentTestFailed()) {
            return;
        }

        mpPreferences->pushButton_copyMap->click();

        const QString copied = newestMapFile(mNoMapYetName);
        QVERIFY2(!copied.isEmpty(), "no map was copied into the profile");
        QCOMPARE(copied, newestMapFile(mSourceName));
    }

    void test_anAutosaveInTheDestinationDuringTheCopyDoesNotUndoIt_data()
    {
        QTest::addColumn<QString>("dueWhileShowing");
        // On Windows and macOS the copy keeps the time of the file it was copied from, so an autosave just before it is the newer file
        QTest::newRow("before the copy") << qsl("Copying over map to %1").arg(mDestinationName);
        QTest::newRow("after the copy") << qsl("Map copied successfully to other profile %1").arg(mDestinationName);
    }

    void test_anAutosaveInTheDestinationDuringTheCopyDoesNotUndoIt()
    {
        QFETCH(QString, dueWhileShowing);
        QDir(MudletApp::getMudletPath(enums::profileMapsPath, mDestinationName)).removeRecursively();
        buildMap(mpDestination, qsl("The destination's own"), 2);
        QVERIFY(mpDestination->saveMapFile(MudletApp::getMudletPath(enums::profileMapPathFileName, mDestinationName, qsl("own.dat"))));
        QVERIFY(mpDestination->mpMap->addRoom(100));
        QVERIFY(mpDestination->mpMap->isUnsaved());
        buildMap(mpSource, mCopiedArea, scmCopiedRooms);
        openPreferencesAndTick({mDestinationName});
        if (QTest::currentTestFailed()) {
            return;
        }

        // The destination's autosave timer coming due while the copy pumps events
        bool timerDue = false;
        QTimer ticker;
        ticker.setInterval(0);
        connect(&ticker, &QTimer::timeout, this, [&]() {
            if (!timerDue && mpPreferences->label_mapFileActionResult->text().startsWith(dueWhileShowing)) {
                timerDue = true;
                // Either side of it, as files written within a millisecond of each other do not order by time
                QTest::qSleep(20);
                QTimerEvent due(0);
                QCoreApplication::sendEvent(mpDestination, &due);
                QTest::qSleep(20);
            }
        });
        ticker.start();
        mpPreferences->pushButton_copyMap->click();
        ticker.stop();

        QVERIFY2(timerDue, "the copy never pumped events at that point");
        QVERIFY2(QFileInfo::exists(MudletApp::getMudletPath(enums::profileMapPathFileName, mDestinationName, qsl("autosave.dat"))), "the destination did not autosave during the copy");
        QVERIFY2(areaNames(mpDestination).contains(mCopiedArea), qPrintable(qsl("the destination has %1 open instead of the copy").arg(areaNames(mpDestination).join(qsl(", ")))));

        // With no save in between, as after a crash
        QString nextOpened;
        qsizetype nextOpenedRooms = 0;
        QVERIFY(mpSource->mpMap->retrieveMapFileStats(mDestinationName, &nextOpened, nullptr, nullptr, nullptr, &nextOpenedRooms));
        QVERIFY2(nextOpenedRooms == scmCopiedRooms, qPrintable(qsl("the destination would open %1 next time, which holds %2 rooms rather than the copy").arg(nextOpened).arg(nextOpenedRooms)));
    }

    void test_aDestinationTheCopyCouldNotReachKeepsItsMap()
    {
        const QString mapDir = MudletApp::getMudletPath(enums::profileMapsPath, mDestinationName);
        QDir(mapDir).removeRecursively();
        buildMap(mpDestination, qsl("The destination's own"), 2);
        const QString own = MudletApp::getMudletPath(enums::profileMapPathFileName, mDestinationName, qsl("own.dat"));
        QVERIFY(mpDestination->saveMapFile(own));
        QVERIFY(mpDestination->mpMap->addRoom(100));
        buildMap(mpSource, mCopiedArea, scmCopiedRooms);
        openPreferencesAndTick({mDestinationName});
        if (QTest::currentTestFailed()) {
            return;
        }
        // Taken by folders, which a copy can neither remove nor write over
        for (const auto& name : comingMapNames()) {
            QVERIFY(QDir().mkpath(qsl("%1/%2").arg(mapDir, name)));
        }
        const int mark = std::max(0, mpSource->mainConsoleView()->buffer.getLastLineNumber());
        QSignalSpy reloads(mudlet::self(), &mudlet::signal_profileMapReloadRequested);

        mpPreferences->pushButton_copyMap->click();

        QVERIFY2(!areaNames(mpDestination).contains(mCopiedArea), "the copy went through, so the save landed outside the taken names");
        QVERIFY2(mpDestination->mpMap->mpRoomDB->getRoom(100), "the destination dropped the room it had not saved yet");
        // Loading one of the folders would fail and leave the map as it was, so the room alone cannot show this
        QVERIFY2(reloads.isEmpty() || !reloads.constLast().constFirst().toStringList().contains(mDestinationName), "the destination was asked to load a map that never reached it");
        const QString label = mpPreferences->label_mapFileActionResult->text();
        QVERIFY2(label.startsWith(qsl("Could not copy the map to %1").arg(mDestinationName)), qPrintable(qsl("the copy that failed ended on '%1'").arg(label)));
        QStringList lines;
        for (int line = mark; line <= mpSource->mainConsoleView()->buffer.getLastLineNumber(); ++line) {
            lines << mpSource->mainConsoleView()->buffer.line(line);
        }
        // As one line, since the reason sits on a line of its own
        const QString console = lines.join(QChar::Space).simplified();
        QVERIFY2(console.contains(qsl("Unable to copy the map to profile \"%1\". reason: Destination file exists").arg(mDestinationName)),
                 qPrintable(qsl("the console does not say why: '%1'").arg(console)));
    }

    // As when the button is clicked twice in a row
    void test_aCopyReplacesOneMadeEarlierInTheSameSecond()
    {
        const QString mapDir = MudletApp::getMudletPath(enums::profileMapsPath, mDestinationName);
        QDir(mapDir).removeRecursively();
        QVERIFY(QDir().mkpath(mapDir));
        buildMap(mpDestination, qsl("The destination's own"), 2);
        buildMap(mpSource, qsl("The earlier copy"), 2);
        const QString earlier = MudletApp::getMudletPath(enums::profileMapPathFileName, mSourceName, qsl("earlier.dat"));
        QVERIFY(mpSource->saveMapFile(earlier));
        buildMap(mpSource, mCopiedArea, scmCopiedRooms);
        openPreferencesAndTick({mDestinationName});
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList taken = comingMapNames();
        for (const auto& name : taken) {
            QVERIFY(QFile::copy(earlier, qsl("%1/%2").arg(mapDir, name)));
        }
        QVERIFY(QFile::remove(earlier));

        mpPreferences->pushButton_copyMap->click();

        QVERIFY2(taken.contains(newestMapFile(mSourceName)), "the save landed outside the taken names");
        QVERIFY2(areaNames(mpDestination).contains(mCopiedArea), qPrintable(qsl("the destination has %1 open instead of the copy").arg(areaNames(mpDestination).join(qsl(", ")))));
        QCOMPARE(mpPreferences->label_mapFileActionResult->text(), qsl("Map copied, now signalling other profiles to reload it."));
        QCOMPARE(QDir(mapDir).entryList(QDir::Files | QDir::NoDotAndDotDot).size(), taken.size());
    }

#if !defined(Q_OS_WIN)
    // As when the destination's disk has no room for the copy
    void test_aCopyThatFailsKeepsTheFileItWouldHaveReplaced()
    {
        const QString mapDir = MudletApp::getMudletPath(enums::profileMapsPath, mDestinationName);
        QDir(mapDir).removeRecursively();
        QVERIFY(QDir().mkpath(mapDir));
        buildMap(mpDestination, qsl("The destination's own"), 2);
        buildMap(mpSource, qsl("The earlier copy"), 2);
        const QString earlier = MudletApp::getMudletPath(enums::profileMapPathFileName, mSourceName, qsl("earlier.dat"));
        QVERIFY(mpSource->saveMapFile(earlier));
        buildMap(mpSource, mCopiedArea, scmCopiedRooms);
        openPreferencesAndTick({mDestinationName});
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList taken = comingMapNames();
        for (const auto& name : taken) {
            QVERIFY(QFile::copy(earlier, qsl("%1/%2").arg(mapDir, name)));
        }
        QVERIFY(QFile::remove(earlier));

        // Made unreadable once saved, so the copy fails after the destination's file is out of its way
        QString unreadable;
        auto readable = qScopeGuard([&]() {
            if (!unreadable.isEmpty()) {
                QFile::setPermissions(unreadable, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            }
        });
        QTimer ticker;
        ticker.setInterval(0);
        connect(&ticker, &QTimer::timeout, this, [&]() {
            if (unreadable.isEmpty() && mpPreferences->label_mapFileActionResult->text().startsWith(qsl("Copying over map to %1").arg(mDestinationName))) {
                unreadable = MudletApp::getMudletPath(enums::profileMapPathFileName, mSourceName, newestMapFile(mSourceName));
                QFile::setPermissions(unreadable, QFileDevice::Permissions());
            }
        });
        ticker.start();
        mpPreferences->pushButton_copyMap->click();
        ticker.stop();

        QVERIFY2(!unreadable.isEmpty(), "the copy never pumped events before copying");
        if (QFile probe(unreadable); probe.open(QIODevice::ReadOnly)) {
            QSKIP("this user can read a file it has no permission to read");
        }
        const QString name = QFileInfo(unreadable).fileName();
        QVERIFY2(taken.contains(name), "the save landed outside the taken names");
        QVERIFY2(mpPreferences->label_mapFileActionResult->text().startsWith(qsl("Could not copy the map to %1").arg(mDestinationName)), "the copy did not fail");
        QVERIFY2(QFileInfo::exists(qsl("%1/%2").arg(mapDir, name)), "the failed copy deleted the file the destination held under that name");
        QCOMPARE(QDir(mapDir).entryList(QDir::Files | QDir::NoDotAndDotDot).size(), taken.size());
    }

    // Players link one profile's map folder into another to share a map
    void test_aSharedMapFolderKeepsTheMapJustSaved()
    {
        const QString mapDir = MudletApp::getMudletPath(enums::profileMapsPath, mDestinationName);
        QDir(mapDir).removeRecursively();
        const QString sharedDir = MudletApp::getMudletPath(enums::profileMapsPath, mSourceName);
        QVERIFY(QDir().mkpath(sharedDir));
        QVERIFY(QFile::link(sharedDir, mapDir));
        auto unlink = qScopeGuard([&]() {
            QFile::remove(mapDir);
        });
        buildMap(mpDestination, qsl("The destination's own"), 2);
        buildMap(mpSource, mCopiedArea, scmCopiedRooms);
        openPreferencesAndTick({mDestinationName});
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList taken = comingMapNames();

        mpPreferences->pushButton_copyMap->click();

        const QString saved = newestMapFile(mSourceName);
        QVERIFY2(taken.contains(saved), qPrintable(qsl("the map just saved is gone, leaving %1 newest").arg(saved)));
        QVERIFY2(areaNames(mpDestination).contains(mCopiedArea), "the destination was not asked to load the map it shares");
        QCOMPARE(mpPreferences->label_mapFileActionResult->text(), qsl("Map copied, now signalling other profiles to reload it."));
    }
#endif

    // Deleted in the connection dialog while these preferences were open
    void test_aDeletedDestinationIsNotBroughtBack()
    {
        const QString home = MudletApp::getMudletPath(enums::profileHomePath, mDeletedName);
        QVERIFY(QDir().mkpath(home));
        // Whatever earlier tests left in the destination
        QVERIFY(QDir(MudletApp::getMudletPath(enums::profileMapsPath, mDestinationName)).removeRecursively());
        buildMap(mpDestination, qsl("The destination's own"), 2);
        buildMap(mpSource, mCopiedArea, scmCopiedRooms);
        openPreferencesAndTick({mDeletedName, mDestinationName});
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(TestProfile::removeProfileDirectory(mDeletedName));

        mpPreferences->pushButton_copyMap->click();

        QVERIFY2(!QFileInfo::exists(home), "the copy made a folder for the deleted profile");
        QVERIFY2(areaNames(mpDestination).contains(mCopiedArea), "the profile that was still there did not get the copy");
    }
};

#include "MapCopyTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapCopyTest)
