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

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TDockWidget.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// getMainWindowSize() is what Geyser, the Lua function of the same name and MXP
// frame placement all measure against, so a report that stops following the
// window puts every one of them in the wrong place. It declines to report a
// shrink of more than half - geometry mid-profile-switch is not to be trusted -
// and the danger in that is the report becoming self-referential: if what it
// declined to report is what it compares the next size against, one large shrink
// makes it decline forever.
class MainWindowSizeReportTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "MainWindowSizeReport-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";

    // resizes are answered from a zero timer
    void settle() { QTest::qWait(50ms); }

    void resizeWindow(const int width, const int height)
    {
        mudlet::self()->resize(width, height);
        settle();
    }

    // What the console really has to hand out, from its own geometry rather than
    // from the reporting path under test.
    QSize measuredMainWindowSize() const
    {
        TMainConsole* pConsole = mpHost->mpConsole;
        return {pConsole->width() - (pConsole->mpLeftToolBar->width() + pConsole->mpRightToolBar->width()),
                pConsole->height() - (pConsole->mpCommandLine->height() + pConsole->mpTopToolBar->height())};
    }

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    QSize dockSize(const QString& name) const
    {
        TDockWidget* pDock = mpHost->mpConsole->mDockWidgetMap.value(name);
        return (pDock && pDock->widget()) ? pDock->widget()->size() : QSize();
    }

    int luaInt(const QString& global) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    QString mismatch(const QSize& reported, const QSize& measured) const
    {
        return qsl("reported as %1x%2 for a widget that measures %3x%4")
                .arg(QString::number(reported.width()), QString::number(reported.height()), QString::number(measured.width()), QString::number(measured.height()));
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        // sized before the console exists, so the shrink under test is the one
        // each case performs and not one left over from however wide the platform
        // makes a main window nobody has sized
        mudlet::self()->resize(1200, 800);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

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

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
        settle();
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // leaves the window at a size no case has to shrink into to get started
    void cleanup() { resizeWindow(1200, 800); }

    // dragging an edge inwards arrives in small steps, so the report has never
    // had trouble following it
    void test_aGradualShrinkIsReported()
    {
        resizeWindow(1200, 800);

        for (int width = 1150; width >= 900; width -= 50) {
            resizeWindow(width, 800);
            QVERIFY2(mpHost->mpConsole->getMainWindowSize() == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mpConsole->getMainWindowSize(), measuredMainWindowSize())));
        }
    }

    // the same shrink arriving in one step, which is what unmaximising or moving
    // the window to a smaller screen does
    void test_aShrinkOfMoreThanHalfIsReported()
    {
        resizeWindow(2000, 1200);
        QCOMPARE(mpHost->mpConsole->getMainWindowSize(), measuredMainWindowSize());

        resizeWindow(800, 600);

        QVERIFY2(mpHost->mpConsole->getMainWindowSize() == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mpConsole->getMainWindowSize(), measuredMainWindowSize())));
    }

    // a report that declined once must not go on measuring every later size
    // against what it declined to report, or nothing the window does afterwards
    // can bring it back
    void test_theReportKeepsUpAfterAShrinkOfMoreThanHalf()
    {
        resizeWindow(2000, 1200);
        resizeWindow(800, 600);

        for (const QSize& size : {QSize(900, 650), QSize(1100, 700), QSize(1200, 800)}) {
            resizeWindow(size.width(), size.height());
            QVERIFY2(mpHost->mpConsole->getMainWindowSize() == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mpConsole->getMainWindowSize(), measuredMainWindowSize())));
        }
    }

    // user windows are reported through a cache of their own, which used to keep
    // the same "not less than half" rule and so kept the same way of getting stuck
    void test_aUserWindowShrunkByMoreThanHalfIsReported()
    {
        const QString userWindow = qsl("mwsrUserWindow");
        runLua(qsl("openUserWindow('%1', false)").arg(userWindow));
        settle();
        QVERIFY2(mpHost->mpConsole->mDockWidgetMap.contains(userWindow), "the user window was not created");

        runLua(qsl("resizeWindow('%1', 600, 400)").arg(userWindow));
        settle();
        QCOMPARE(mpHost->mpConsole->getUserWindowSize(userWindow), dockSize(userWindow));

        runLua(qsl("resizeWindow('%1', 200, 150)").arg(userWindow));
        settle();
        QVERIFY2(mpHost->mpConsole->getUserWindowSize(userWindow) == dockSize(userWindow), qPrintable(mismatch(mpHost->mpConsole->getUserWindowSize(userWindow), dockSize(userWindow))));

        // a script may ask for a user window this short and Mudlet gives it one,
        // so a size below any "too small to be real" bar is still the size to
        // report - refusing it would leave the cache answering for it instead
        runLua(qsl("resizeWindow('%1', 300, 40)").arg(userWindow));
        settle();
        // how much of the 40 the dock keeps is up to the window manager, so only
        // that it ended up under the bar is pinned, not the exact height
        const QSize shortDock = dockSize(userWindow);
        QVERIFY2(shortDock.height() > 0 && shortDock.height() < 50, qPrintable(qsl("expected a positive height under 50 to test with, got %1").arg(shortDock.height())));
        QVERIFY2(mpHost->mpConsole->getUserWindowSize(userWindow) == shortDock, qPrintable(mismatch(mpHost->mpConsole->getUserWindowSize(userWindow), shortDock)));

        runLua(qsl("hideWindow('%1')").arg(userWindow));
    }

    // the same for the main window: a player can drag Mudlet down to a window
    // with well under 50 pixels left inside it once the command line and toolbars
    // have had their share. Small is not the same as not settled yet.
    void test_aMainWindowTooShortToBeUsefulIsStillReported()
    {
        // how much of the window never reaches the console differs with the
        // platform's chrome, so the height to ask for is worked out from a window
        // that fits rather than assumed
        resizeWindow(800, 200);
        const int consumedByChrome = 200 - measuredMainWindowSize().height();
        const int wantedInside = 30;
        resizeWindow(800, consumedByChrome + wantedInside);

        const QSize measured = measuredMainWindowSize();
        if (measured.height() <= 0 || measured.height() >= 50) {
            QSKIP(qPrintable(qsl("the window would not go short enough to test with - %1 pixels inside").arg(measured.height())));
        }
        QVERIFY2(mpHost->mpConsole->getMainWindowSize() == measured, qPrintable(mismatch(mpHost->mpConsole->getMainWindowSize(), measured)));
    }

    // the shrink a player performs rather than one the test dials in: restoring a
    // maximised window drops it to well under half the screen's width in one go
    void test_restoringAMaximisedWindowReportsTheRestoredSize()
    {
        resizeWindow(800, 600);
        mudlet::self()->showMaximized();
        QTest::qWait(200ms);
        const int maximisedWidth = mpHost->mpConsole->width();

        mudlet::self()->showNormal();
        resizeWindow(800, 600);

        if (maximisedWidth < 2 * mpHost->mpConsole->width()) {
            QSKIP("no window manager here to maximise against, so this is not the shrink under test");
        }
        QVERIFY2(mpHost->mpConsole->getMainWindowSize() == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mpConsole->getMainWindowSize(), measuredMainWindowSize())));
    }

    // the number Lua hands scripts is the same one, so a stale report is what
    // every Geyser layout is built against
    void test_luaSeesTheSizeTheWindowHasAfterAShrink()
    {
        resizeWindow(2000, 1200);
        resizeWindow(800, 600);

        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("mainWindowWidth, mainWindowHeight = getMainWindowSize()")));

        const QSize measured = measuredMainWindowSize();
        QCOMPARE(luaInt(qsl("mainWindowWidth")), measured.width());
        QCOMPARE(luaInt(qsl("mainWindowHeight")), measured.height());
    }
};

#include "MainWindowSizeReportTest.moc"
MUDLET_GROUPED_TEST_MAIN(MainWindowSizeReportTest)
