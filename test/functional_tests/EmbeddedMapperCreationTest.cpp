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
 * already-loaded-map branch.
 *
 * An embedded mapper and the dockable map widget are mutually exclusive for the
 * life of a profile and neither can be destroyed, so the busted suite cannot go
 * here and each test method needs a mudlet of its own.
 */

#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>

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

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForEmbeddedMapperTest();

class EmbeddedMapperCreationTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Embedded-Mapper-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstAreaName = qsl("AAArea");
    const QString mPlayerAreaName = qsl("QAArea");

private slots:
    void initTestCase() { initializeQRCResourcesForEmbeddedMapperTest(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        QTimer::singleShot(0ms, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
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
        QVERIFY(pMap->setRoomArea(1, playerAreaId, false));
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

void initializeQRCResourcesForEmbeddedMapperTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "EmbeddedMapperCreationTest.moc"
QTEST_MAIN(EmbeddedMapperCreationTest)
