/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet developers                           *
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
 *   Free Software Foundation, Inc.,                                        *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * Regression test for a crash when a docked user window (Geyser.UserWindow) is
 * deleted and then a window of the same name is recreated - e.g. when a package
 * is reinstalled.
 *
 * Root cause: deleteMiniConsole() (where Geyser.UserWindow:delete() ends up)
 * freed the inner TConsole but left the TDockWidget orphaned in mDockWidgetMap.
 * Once the console's deferred deleteLater() fired, the dock's widget() became
 * null; recreating a same-named window then dereferenced it in
 * getUserWindowSize() -> SIGSEGV.
 *
 * Bootstrap mirrors the other functional tests (e.g. TOscTest): start mudlet,
 * create a profile, and drive the real Host/TMainConsole user-window API.
 */

#include <QSignalSpy>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForUserWindowTest();

class TUserWindowTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "UserWindow-Test-Host";
    const QString mPort = "4004";
    const QString mLocalhost = "localhost";

private slots:
    // Start mudlet and create a profile once for all tests.
    void initTestCase()
    {
        initializeQRCResourcesForUserWindowTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        QTimer::singleShot(0, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();
        delete mudlet::self();
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
    }

    // Deleting a docked user window must remove BOTH its console and its dock
    // widget, so that a same-named window can be recreated and queried without
    // crashing. Pre-fix the dock was orphaned and getUserWindowSize() crashed on
    // recreation.
    void test_deleteUserWindowRemovesDockAndAllowsRecreate()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString name = qsl("RegressionUserWindow");

        // Create a docked user window (what Lua openUserWindow(name, ...) does).
        auto [created, createMsg] = mpHost->openWindow(name, /*loadLayout=*/false, /*autoDock=*/true, qsl("l"));
        QVERIFY2(created, qPrintable(createMsg));
        QVERIFY2(console->mSubConsoleMap.contains(name), "user window console not registered after creation");
        QVERIFY2(console->mDockWidgetMap.contains(name), "user window dock not registered after creation");

        // Delete it the way Geyser.UserWindow:delete() does.
        auto [deleted, deleteMsg] = console->deleteMiniConsole(name);
        QVERIFY2(deleted, qPrintable(deleteMsg));

        // The regression: deleteMiniConsole() must remove the dock too, not just the
        // console. Pre-fix the dock lingered in mDockWidgetMap and later crashed.
        QVERIFY2(!console->mSubConsoleMap.contains(name), "console still registered after deleting user window");
        QVERIFY2(!console->mDockWidgetMap.contains(name), "dock widget orphaned after deleting user window (regression)");

        // Let the deferred deleteLater() run - this is when the orphaned dock's
        // widget() used to become null.
        QTest::qWait(50);
        QCoreApplication::processEvents();

        // Querying the (now absent) window must not crash - it falls back to the
        // main window size.
        const QSize sizeAfterDelete = console->getUserWindowSize(name);
        QVERIFY(sizeAfterDelete.isValid());

        // Recreating a user window of the same name must succeed and be safe to
        // query. Pre-fix this is exactly the path that segfaulted: the orphaned dock
        // was found and pW->widget()->size() dereferenced a null widget.
        auto [recreated, recreateMsg] = mpHost->openWindow(name, /*loadLayout=*/false, /*autoDock=*/true, qsl("l"));
        QVERIFY2(recreated, qPrintable(recreateMsg));
        QVERIFY2(console->mDockWidgetMap.contains(name), "user window dock not registered after recreation");
        const QSize sizeAfterRecreate = console->getUserWindowSize(name);
        QVERIFY(sizeAfterRecreate.isValid());

        // Tidy up.
        console->deleteMiniConsole(name);
        QTest::qWait(50);
        QCoreApplication::processEvents();
    }
};

void initializeQRCResourcesForUserWindowTest()
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

#include "TUserWindowTest.moc"
QTEST_MAIN(TUserWindowTest)
