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
 * An embedded mapper and the dockable map widget are mutually exclusive, and an
 * embedded one cannot be undone once made, so the busted suite cannot go here
 * and each test method needs a mudlet of its own.
 */

#include <QDockWidget>
#include <QFileInfo>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletPaths.h"
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
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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

    // An embedded mapper stops redrawing once the toolbar's own map dock has
    // been opened and closed again. slot_showMapperDialog() repoints
    // TMap::mpMapper at the mapper it puts in that dock, and on hide restores
    // it only from mpConsole->mpDockableMapWidget - which is never where
    // createMapper() put the embedded one, so the restore matches nothing and
    // the QPointer is left null once the dock's own mapper dies with it. What
    // the player sees is rooms being created and never drawn until the profile
    // is reloaded.
    void test_theEmbeddedMapperSurvivesTheToolbarMapDockClosing()
    {
        // The toolbar refuses to build a dock over an existing embedded
        // mapper, so the dock has to predate it. Once both exist, toggling the
        // dock visible repoints TMap::mpMapper at the dock's own mapper, and
        // hiding it again has to hand the map back to the embedded one.
        mudlet::self()->slot_showMapperDialog();
        const QString mapKey = qsl("map_%1").arg(mHostname);
        QDockWidget* pDock = mudlet::self()->getMainWindowDockWidget(mapKey);
        QVERIFY2(pDock, "the toolbar action created no map dock, so this case covers nothing");

        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        dlgMapper* pEmbedded = mpHost->mpConsole->mpMapper.data();
        QVERIFY2(pEmbedded, "createMapper() left no embedded mapper to lose");
        QCOMPARE(mpHost->mpMap->mpMapper.data(), pEmbedded);

        // Toggle the dock off and on again through the toolbar entry point:
        // showing it is what takes TMap::mpMapper over.
        mudlet::self()->slot_showMapperDialog();
        qApp->processEvents();
        mudlet::self()->slot_showMapperDialog();
        qApp->processEvents();
        QVERIFY2(mpHost->mpMap->mpMapper.data() != pEmbedded, "the dock did not take the map over, so restoring it below would prove nothing");

        mudlet::self()->slot_showMapperDialog();
        qApp->processEvents();

        QVERIFY2(mpHost->mpMap->mpMapper, "closing the toolbar map dock left the map with no mapper at all, so nothing redraws it");
        QCOMPARE(mpHost->mpMap->mpMapper.data(), pEmbedded);
    }

    // A map widget that was opened and closed again used to refuse createMapper()
    // for the rest of the session, while Host::mapWidgetGeometry(), behind the
    // Lua getMapWidgetGeometry(), reported no map window at all: two answers to
    // the same question, with no Lua call that released the slot. A closed map
    // widget is not on screen, so the embedded mapper takes the slot and the
    // dock goes with it.
    void test_createMapperTakesOverFromAClosedMapWidget()
    {
        auto [opened, openMessage] = mpHost->openMapWidget(QString(), -1, -1, -1, -1);
        QVERIFY2(opened, qPrintable(openMessage));
        QVERIFY(mpHost->mpConsole->mpDockableMapWidget);
        QVERIFY2(mpHost->mapWidget(), "the map widget did not come up, so closing it below proves nothing");

        auto [closed, closeMessage] = mpHost->closeMapWidget();
        QVERIFY2(closed, qPrintable(closeMessage));
        QVERIFY2(!mpHost->mapWidget(), "closeMapWidget() left the map widget on screen");
        QPointer<QDockWidget> pDock = mpHost->mpConsole->mpDockableMapWidget;
        QPointer<QWidget> pDockMapper = pDock->widget();

        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        QVERIFY(mpHost->mpConsole->mpMapper);
        // the embedded mapper is what draws the map now, not the dock's own one
        QCOMPARE(mpHost->mpMap->mpMapper.data(), mpHost->mpConsole->mpMapper.data());

        // gone rather than merely hidden behind the embedded mapper - only the raw
        // pointer tells those two apart, the getter below reads the same "not on
        // screen" either way and would have passed before this was fixed
        QVERIFY(!mpHost->mpConsole->mpDockableMapWidget);
        QVERIFY(!mpHost->mapWidgetGeometry().has_value());
        auto [reopened, reopenMessage] = mpHost->openMapWidget(QString(), -1, -1, -1, -1);
        QVERIFY2(!reopened, "openMapWidget() built a second map over the embedded mapper");
        QCOMPARE(reopenMessage, qsl("cannot create map widget. Do you already use an embedded mapper?"));
        auto [closedAgain, closeAgainMessage] = mpHost->closeMapWidget();
        QVERIFY(!closedAgain);
        QCOMPARE(closeAgainMessage, qsl("no map widget found to close"));
        QCOMPARE(mpHost->mpMap->mpMapper.data(), mpHost->mpConsole->mpMapper.data());

        // openMapWidget() raised one and the mapper that replaced its widget raises
        // another, which is what tells a mapper package to set itself up again
        QVERIFY2(mapOpenEventCountIs(2), "the takeover did not raise mapOpenEvent for the mapper it put in the map widget's place");

        // deleteLater() posts a DeferredDelete that processEvents() will not deliver
        // at loop level 0, so forgetting the pointer and actually destroying the
        // widget are only told apart by sending that event by hand
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(pDock.isNull(), "the map widget was forgotten rather than destroyed, so it is still parented on the main window");
        QVERIFY2(pDockMapper.isNull(), "the map widget's own mapper outlived the dock that owned it");
        QVERIFY2(mpHost->mpMap->mpMapper, "the map widget's death took the map's mapper with it");
        QCOMPARE(mpHost->mpMap->mpMapper.data(), mpHost->mpConsole->mpMapper.data());
    }

    // The profile's own map dock is hidden rather than destroyed when the main
    // toolbar hands the map to a main window dock of its own, and that dock, not
    // the hidden one, is then what draws the map. createMapper() takes the hidden
    // dock away and the embedded mapper takes the map over - the same thing it
    // already did for a profile that never had a dock of its own, which
    // test_theEmbeddedMapperSurvivesTheToolbarMapDockClosing above pins - rather
    // than refusing because a map is on screen somewhere. Pinned because the
    // refusal is now keyed on the profile's own dock alone.
    void test_createMapperTakesOverADockHiddenByTheToolbarMapAction()
    {
        auto [opened, openMessage] = mpHost->openMapWidget(QString(), -1, -1, -1, -1);
        QVERIFY2(opened, qPrintable(openMessage));
        dlgMapper* pOwnDockMapper = mpHost->mpMap->mpMapper.data();
        QVERIFY(pOwnDockMapper);

        mudlet::self()->slot_showMapperDialog();
        qApp->processEvents();
        QVERIFY2(mudlet::self()->findChild<QDockWidget*>(qsl("dockMap_%1_main").arg(mHostname)), "the toolbar action built no main window map dock, so this covers nothing");
        QVERIFY2(mpHost->mpConsole->mpDockableMapWidget, "the toolbar action destroyed the profile's own map dock rather than hiding it");
        QVERIFY2(!mpHost->mapWidget(), "the toolbar action left the profile's own map dock on screen");
        QVERIFY2(mpHost->mpMap->mpMapper.data() != pOwnDockMapper, "the main window dock did not take the map over, so this covers nothing");

        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        QVERIFY(!mpHost->mpConsole->mpDockableMapWidget);
        QVERIFY(mpHost->mpConsole->mpMapper);
        QCOMPARE(mpHost->mpMap->mpMapper.data(), mpHost->mpConsole->mpMapper.data());

        // and closing the main window dock hands the map to the embedded mapper
        // rather than to the dock that is now gone
        mudlet::self()->slot_showMapperDialog();
        qApp->processEvents();
        QVERIFY2(mpHost->mpMap->mpMapper, "closing the main window map dock left the map with no mapper at all");
        QCOMPARE(mpHost->mpMap->mpMapper.data(), mpHost->mpConsole->mpMapper.data());
    }

    // Companion guard to the above rather than a guard for the bug: a fix that
    // dropped the check instead of narrowing it to an on-screen map widget would
    // let a profile hold two mappers, only one of which the map is drawn through.
    void test_createMapperStillRefusesAnOpenMapWidget()
    {
        auto [opened, openMessage] = mpHost->openMapWidget(QString(), -1, -1, -1, -1);
        QVERIFY2(opened, qPrintable(openMessage));
        QVERIFY2(mpHost->mapWidget(), "the map widget did not come up, so this covers nothing");
        dlgMapper* pDockMapper = mpHost->mpMap->mpMapper.data();
        QVERIFY(pDockMapper);

        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(!created, "createMapper() built an embedded mapper over a map widget that was on screen");
        QCOMPARE(message, qsl("cannot create mapper. Do you already use a map window?"));
        QVERIFY2(!mpHost->mpConsole->mpMapper, "the refused call left an embedded mapper behind");
        QVERIFY(mpHost->mpConsole->mpDockableMapWidget);
        QCOMPARE(mpHost->mpMap->mpMapper.data(), pDockMapper);
        QVERIFY2(mapOpenEventCountIs(1), "the refused createMapper() raised a mapOpenEvent of its own");
    }


    // Host::mapWidget() reads the dock's own hidden state rather than its
    // visibility, so a main window that is not on screen - minimised to the system
    // tray - must not make the map widget count as closed. Reading it the other way
    // round would destroy the map widget of anyone whose script ran while minimised.
    void test_createMapperStillRefusesAMapWidgetWhileTheMainWindowIsHidden()
    {
        mudlet::self()->show();
        auto [opened, openMessage] = mpHost->openMapWidget(QString(), -1, -1, -1, -1);
        QVERIFY2(opened, qPrintable(openMessage));
        mudlet::self()->hide();
        qApp->processEvents();

        QVERIFY2(!mpHost->mpConsole->mpDockableMapWidget->isVisible(), "the map widget stayed visible with the main window hidden, so this covers nothing");
        QVERIFY2(mpHost->mapWidget(), "a main window that is merely not on screen made the map widget itself count as closed");

        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(!created, "createMapper() took away the map widget of a profile whose main window was only minimised");
        QCOMPARE(message, qsl("cannot create mapper. Do you already use a map window?"));
        QVERIFY(mpHost->mpConsole->mpDockableMapWidget);
        QVERIFY2(!mpHost->mpConsole->mpMapper, "the refused call left an embedded mapper behind");
    }

    // TMap::mpMapper is what the map is painted through, and a window that
    // borrowed it can die without handing it back. A repeat createMapper() - which
    // Geyser.Mapper makes on every reposition - has to take the map back rather
    // than leave rooms being created and never drawn.
    void test_createMapperTakesTheMapBackWhenNothingIsDrawingIt()
    {
        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        dlgMapper* pEmbedded = mpHost->mpConsole->mpMapper.data();
        QVERIFY(pEmbedded);

        mpHost->mpMap->mpMapper = nullptr;

        auto [again, againMessage] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(again, qPrintable(againMessage));
        QVERIFY2(mpHost->mpMap->mpMapper, "a repeat createMapper() left the map with no mapper at all");
        QCOMPARE(mpHost->mpMap->mpMapper.data(), pEmbedded);
        QVERIFY2(mapOpenEventCountIs(1), "taking the map back raised a second mapOpenEvent");
    }

    // The two halves of the takeover only meet when the profile holds an embedded
    // mapper and a map widget at once, which it can: with TMap::mpMapper left null
    // by a window that died holding it, the toolbar map action builds the profile a
    // map widget again even though the embedded mapper is still there. Closing that
    // widget and creating the mapper again has to leave the embedded one drawing the
    // map - without the explicit null, TMap::mpMapper would still point at the
    // widget's mapper, so nothing would hand the map back and the map would stop
    // being drawn as soon as the event loop ran the deferred delete.
    void test_createMapperTakesOverAMapWidgetThatWasDrawingTheMap()
    {
        auto [created, message] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(created, qPrintable(message));
        dlgMapper* pEmbedded = mpHost->mpConsole->mpMapper.data();
        QVERIFY(pEmbedded);

        mpHost->mpMap->mpMapper = nullptr;
        mudlet::self()->slot_showMapperDialog();
        QVERIFY2(mpHost->mpConsole->mpDockableMapWidget, "no map widget was built, so this covers nothing");
        QPointer<QWidget> pDockMapper = mpHost->mpConsole->mpDockableMapWidget->widget();
        QCOMPARE(mpHost->mpMap->mpMapper.data(), qobject_cast<dlgMapper*>(pDockMapper.data()));
        mpHost->mpConsole->mpDockableMapWidget->hide();

        auto [again, againMessage] = mpHost->mpConsole->createMapper(QString(), 0, 0, 300, 300);
        QVERIFY2(again, qPrintable(againMessage));
        QCOMPARE(mpHost->mpMap->mpMapper.data(), pEmbedded);

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(pDockMapper.isNull(), "the map widget was forgotten rather than destroyed");
        QVERIFY2(mpHost->mpMap->mpMapper, "the map widget's death took the map's mapper with it");
        QCOMPARE(mpHost->mpMap->mpMapper.data(), pEmbedded);
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
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, mHostname));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "EmbeddedMapperCreationTest.moc"
MUDLET_GROUPED_TEST_MAIN(EmbeddedMapperCreationTest)
