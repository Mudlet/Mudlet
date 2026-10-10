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
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TCommandLine.h"
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
        TMainConsole* pConsole = mpHost->mainConsoleView();
        return {pConsole->width() - (pConsole->mpLeftToolBar->width() + pConsole->mpRightToolBar->width()),
                pConsole->height() - (pConsole->mpCommandLine->height() + pConsole->mpTopToolBar->height())};
    }

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    QSize dockSize(const QString& name) const
    {
        TDockWidget* pDock = mpHost->mainConsoleView()->dockWidget(name);
        return (pDock && pDock->widget()) ? pDock->widget()->size() : QSize();
    }

    // What the share logic in TMainConsole reads, so a dock left squeezed says why
    QString dockState(const QString& name) const
    {
        mudlet* window = mudlet::self();
        TDockWidget* pDock = mpHost->mainConsoleView()->dockWidget(name);
        if (!pDock) {
            return qsl("no dock named %1").arg(name);
        }
        const Qt::DockWidgetArea area = window->dockWidgetArea(pDock);
        int docksInArea = 0;
        for (auto* other : window->findChildren<QDockWidget*>()) {
            if (other->isVisible() && !other->isFloating() && other->parentWidget() == window && window->dockWidgetArea(other) == area) {
                ++docksInArea;
            }
        }
        return qsl("dock visible %1, floating %2, area %3, dock height %4, console visible %5, font height %6, visible docks in its area %7")
                .arg(pDock->isVisible())
                .arg(pDock->isFloating())
                .arg(static_cast<int>(area))
                .arg(pDock->height())
                .arg(pDock->widget() && pDock->widget()->isVisible())
                .arg(pDock->fontMetrics().height())
                .arg(docksInArea);
    }

    QString geom(const QString& name) const
    {
        TDockWidget* pDock = mpHost->mpConsole->dockWidget(name);
        if (!pDock) {
            return qsl("none");
        }
        QString out;
        QDebug(&out).nospace() << name << " dock=" << pDock->geometry() << " widget=" << (pDock->widget() ? pDock->widget()->geometry() : QRect()) << " vis=" << pDock->isVisible()
                               << " hidden=" << pDock->isHidden() << " area=" << mudlet::self()->dockWidgetArea(pDock) << " mw=" << mudlet::self()->geometry()
                               << " mwvis=" << mudlet::self()->isVisible() << " central=" << (mudlet::self()->centralWidget() ? mudlet::self()->centralWidget()->geometry() : QRect());
        return out;
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        // sized before the console exists, so the shrink under test is the one
        // each case performs and not one left over from however wide the platform
        // makes a main window nobody has sized
        mudlet::self()->resize(1200, 800);

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

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
        if (!spy.wait(5s)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
        settle();
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
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
            QVERIFY2(mpHost->mainWindowSize().value_or(QSize()) == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mainWindowSize().value_or(QSize()), measuredMainWindowSize())));
        }
    }

    // the same shrink arriving in one step, which is what unmaximising or moving
    // the window to a smaller screen does
    void test_aShrinkOfMoreThanHalfIsReported()
    {
        resizeWindow(2000, 1200);
        QCOMPARE(mpHost->mainWindowSize().value_or(QSize()), measuredMainWindowSize());

        resizeWindow(800, 600);

        QVERIFY2(mpHost->mainWindowSize().value_or(QSize()) == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mainWindowSize().value_or(QSize()), measuredMainWindowSize())));
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
            QVERIFY2(mpHost->mainWindowSize().value_or(QSize()) == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mainWindowSize().value_or(QSize()), measuredMainWindowSize())));
        }
    }

    // Qt gives a dock added beside visible ones nothing but its minimum height,
    // and a user window's console has none
    void test_aUserWindowDockedBesideAnotherGetsAShareOfTheArea()
    {
        const QString first = qsl("mwsrFirstDock");
        const QString second = qsl("mwsrSecondDock");
        const auto hideBoth = qScopeGuard([this, first, second]() {
            runLua(qsl("hideWindow('%1') hideWindow('%2')").arg(first, second));
            settle();
        });
        runLua(qsl("openUserWindow('%1', false)").arg(first));
        settle();
        const int alone = dockSize(first).height();
        QVERIFY2(alone > 100, qPrintable(qsl("the first user window is only %1 high on its own").arg(alone)));

        runLua(qsl("openUserWindow('%1', false)").arg(second));
        settle();
        const int firstHeight = dockSize(first).height();
        const int secondHeight = dockSize(second).height();
        QVERIFY2(secondHeight > alone / 4, qPrintable(qsl("the second user window is %1 high beside the first's %2 (%3)").arg(secondHeight).arg(firstHeight).arg(dockState(second))));
        QVERIFY2(firstHeight > alone / 4, qPrintable(qsl("the first user window was squeezed to %1").arg(firstHeight)));
        QCOMPARE(mpHost->userWindowSize(second).value_or(QSize()), dockSize(second));
    }

    // Geyser hides a window created hidden in the same call that opens it, so the
    // share has to be taken when it is first shown instead
    void test_aUserWindowHiddenAsItOpensGetsAShareOfTheAreaWhenShown()
    {
        const QString first = qsl("mwsrShownFirstDock");
        const QString second = qsl("mwsrHiddenSecondDock");
        const auto hideBoth = qScopeGuard([this, first, second]() {
            runLua(qsl("hideWindow('%1') hideWindow('%2')").arg(first, second));
            settle();
        });
        runLua(qsl("openUserWindow('%1', false)").arg(first));
        settle();
        const int alone = dockSize(first).height();
        QVERIFY2(alone > 100, qPrintable(qsl("the first user window is only %1 high on its own").arg(alone)));

        qDebug().noquote() << "SHAREDBG test: opening hidden" << geom(first);
        runLua(qsl("openUserWindow('%1', false) hideWindow('%1')").arg(second));
        qDebug().noquote() << "SHAREDBG test: opened and hidden" << geom(first) << geom(second);
        settle();
        qDebug().noquote() << "SHAREDBG test: settled, showing" << geom(first) << geom(second);
        runLua(qsl("showWindow('%1')").arg(second));
        qDebug().noquote() << "SHAREDBG test: shown" << geom(first) << geom(second);
        settle();
        qDebug().noquote() << "SHAREDBG test: settled after show" << geom(first) << geom(second);
        for (auto* d : mudlet::self()->findChildren<QDockWidget*>()) {
            qDebug().nospace() << "SHAREDBG test: dock " << d->objectName() << " " << d->geometry() << " vis=" << d->isVisible() << " floating=" << d->isFloating()
                               << " area=" << mudlet::self()->dockWidgetArea(d) << " parentIsMw=" << (d->parentWidget() == mudlet::self());
        }
        const int firstHeight = dockSize(first).height();
        const int secondHeight = dockSize(second).height();
        QVERIFY2(secondHeight > alone / 4, qPrintable(qsl("the second user window is %1 high beside the first's %2 (%3)").arg(secondHeight).arg(firstHeight).arg(dockState(second))));
        QVERIFY2(firstHeight > alone / 4, qPrintable(qsl("the first user window was squeezed to %1").arg(firstHeight)));
    }

    // user windows are reported through a cache of their own, which used to keep
    // the same "not less than half" rule and so kept the same way of getting stuck
    void test_aUserWindowShrunkByMoreThanHalfIsReported()
    {
        const QString userWindow = qsl("mwsrUserWindow");
        runLua(qsl("openUserWindow('%1', false)").arg(userWindow));
        settle();
        QVERIFY2(mpHost->windowRegistry().hasDockWidget(userWindow), "the user window was not created");

        runLua(qsl("resizeWindow('%1', 600, 400)").arg(userWindow));
        settle();
        QCOMPARE(mpHost->userWindowSize(userWindow).value_or(QSize()), dockSize(userWindow));

        runLua(qsl("resizeWindow('%1', 200, 150)").arg(userWindow));
        settle();
        QVERIFY2(mpHost->userWindowSize(userWindow).value_or(QSize()) == dockSize(userWindow), qPrintable(mismatch(mpHost->userWindowSize(userWindow).value_or(QSize()), dockSize(userWindow))));

        // a script may ask for a user window this short and Mudlet gives it one,
        // so a size below any "too small to be real" bar is still the size to
        // report - refusing it would leave the cache answering for it instead
        runLua(qsl("resizeWindow('%1', 300, 40)").arg(userWindow));
        settle();
        // how much of the 40 the dock keeps is up to the window manager, so only
        // that it ended up under the bar is pinned, not the exact height
        const QSize shortDock = dockSize(userWindow);
        QVERIFY2(shortDock.height() > 0 && shortDock.height() < 50, qPrintable(qsl("expected a positive height under 50 to test with, got %1").arg(shortDock.height())));
        QVERIFY2(mpHost->userWindowSize(userWindow).value_or(QSize()) == shortDock, qPrintable(mismatch(mpHost->userWindowSize(userWindow).value_or(QSize()), shortDock)));

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
        QVERIFY2(mpHost->mainWindowSize().value_or(QSize()) == measured, qPrintable(mismatch(mpHost->mainWindowSize().value_or(QSize()), measured)));
    }

    // the shrink a player performs rather than one the test dials in: restoring a
    // maximised window drops it to well under half the screen's width in one go
    void test_restoringAMaximisedWindowReportsTheRestoredSize()
    {
        resizeWindow(800, 600);
        mudlet::self()->showMaximized();
        QTest::qWait(200ms);
        const int maximisedWidth = mpHost->mainConsoleView()->width();

        mudlet::self()->showNormal();
        resizeWindow(800, 600);

        if (maximisedWidth < 2 * mpHost->mainConsoleView()->width()) {
            QSKIP("no window manager here to maximise against, so this is not the shrink under test");
        }
        QVERIFY2(mpHost->mainWindowSize().value_or(QSize()) == measuredMainWindowSize(), qPrintable(mismatch(mpHost->mainWindowSize().value_or(QSize()), measuredMainWindowSize())));
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
