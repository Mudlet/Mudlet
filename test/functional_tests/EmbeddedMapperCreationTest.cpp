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
 * Covers TMainConsole::createMapper() - the embedded mapper behind Lua
 * createMapper() and Geyser.Mapper{embedded = true} - on both sides of its
 * already-loaded-map branch, and that the main toolbar map action leaves an
 * embedded mapper in charge of TMap::mpMapper instead of building a
 * competing main window dock over it.
 *
 * An embedded mapper and the dockable map widget are mutually exclusive for the
 * life of a profile and neither can be destroyed, so the busted suite cannot go
 * here and each test method needs a mudlet of its own.
 */

#include <QDockWidget>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EmbeddedMapperCreationTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Embedded-Mapper-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstAreaName = qsl("AAArea");
    const QString mPlayerAreaName = qsl("QAArea");

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }

        QVERIFY(mpHost->mpConsole);
        watchMapOpenEvent();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        deleteProfileDirectory();
        delete mudlet::self();
    }

    void test_createMapperWithALoadedMap()
    {
        TMap* pMap = mpHost->mpMap.data();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();

        // the player's area has to sort after the one the dlgMapper constructor's own fill leaves selected
        QVERIFY(pRoomDB->addArea(mFirstAreaName) > 0);
        const int playerAreaId = pRoomDB->addArea(mPlayerAreaName);
        QVERIFY(playerAreaId > 0);
        QVERIFY(pMap->addRoom(1));
        QVERIFY(pMap->setRoomArea(1, playerAreaId));
        pMap->mRoomIdHash[pMap->mProfileName] = 1;
        pMap->setDefaultAreaShown(false);
        QVERIFY2(!pRoomDB->isEmpty(), "the map has to be non-empty for this to be the returning-user path");

        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        QVERIFY(mpHost->mpConsole->mpMapper);

        QVERIFY2(mapOpenEventCountIs(1), "createMapper() did not raise mapOpenEvent exactly once for an already-loaded map");

        auto* pComboBox = mpHost->mpConsole->mpMapper->comboBox_showArea;
        QCOMPARE(pComboBox->count(), 2); // the hidden default area is still in the constructor's fill
        QCOMPARE(pComboBox->currentText(), mPlayerAreaName);

        // Geyser.Mapper re-runs createMapper() on every reposition
        auto [recreated, recreateMessage] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(recreated, qPrintable(recreateMessage));
        QVERIFY2(mapOpenEventCountIs(1), "a repeat createMapper() raised mapOpenEvent again");
    }

    void test_createMapperWithNoMapToLoad()
    {
        QVERIFY2(mpHost->mpMap->mpRoomDB->isEmpty(), "a freshly created profile was expected to have no rooms");

        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        QVERIFY(mpHost->mpConsole->mpMapper);

        QVERIFY2(mapOpenEventCountIs(1), "createMapper() did not raise mapOpenEvent exactly once for a first-run profile");
    }

    // The main toolbar map button runs mudlet::slot_showMapperDialog(). With a
    // script-embedded mapper alive it must not build the per-profile main
    // window dock: that dock takes over TMap::mpMapper - the only widget map
    // updates are painted through - and the embedded mapper then only repaints
    // on direct interaction, even after the dock is closed again.
    void test_toolbarMapActionLeavesEmbeddedMapperInCharge()
    {
        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        QVERIFY(mpHost->mpConsole->mpMapper);
        QCOMPARE(mpHost->mpMap->mpMapper.data(), mpHost->mpConsole->mpMapper.data());

        mudlet::self()->slot_showMapperDialog();

        QVERIFY2(!mudlet::self()->findChild<QDockWidget*>(qsl("dockMap_%1_main").arg(mHostname)), "the toolbar map action built a competing main window map dock over an embedded mapper");
        QCOMPARE(mpHost->mpMap->mpMapper.data(), mpHost->mpConsole->mpMapper.data());
        QVERIFY2(mapOpenEventCountIs(1), "the toolbar map action raised mapOpenEvent over an existing embedded mapper");
    }

    // The Toolbox map entry's label is recomputed as the menu opens so that it
    // says what the next activation will do. The update slot is driven
    // directly here - opening the real menu needs a user.
    void test_showMapMenuLabelSaysWhatTheNextActivationDoes()
    {
        mudlet::self()->slot_updateShowMapActionText();
        QCOMPARE(mudlet::self()->dactionShowMap->text(), mudlet::tr("Show map"));

        mudlet::self()->show();
        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        qApp->processEvents();
        QVERIFY2(mpHost->mapperShown(), "the embedded mapper did not come up on screen, so the Hide map branch cannot be exercised");
        mudlet::self()->slot_updateShowMapActionText();
        QCOMPARE(mudlet::self()->dactionShowMap->text(), mudlet::tr("Hide map"));

        // What the menu entry itself runs - with a mapper alive this toggles it away
        mudlet::self()->slot_mapper();
        mudlet::self()->slot_updateShowMapActionText();
        QCOMPARE(mudlet::self()->dactionShowMap->text(), mudlet::tr("Show map"));
    }

private:
    void watchMapOpenEvent()
    {
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("mapOpenSeen = 0\n"
                                                                 "registerAnonymousEventHandler('mapOpenEvent', function() mapOpenSeen = mapOpenSeen + 1 end)"));
    }

    bool mapOpenEventCountIs(const int expected) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(mapOpenSeen == %1)").arg(expected)); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mHostname));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "EmbeddedMapperCreationTest.moc"
MUDLET_GROUPED_TEST_MAIN(EmbeddedMapperCreationTest)
