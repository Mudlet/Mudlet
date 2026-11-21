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
 * A detached profile window must not be tied to the main window: a top-level
 * window that has a QWidget parent gets a QWindow transient parent, which
 * Windows implements as native window ownership, and an owned window is pinned
 * above its owner permanently. The user then cannot bring the main window to the
 * front at all - clicking it focuses it while it stays behind the detached one.
 *
 * Qt's xcb plugin sets no WM_TRANSIENT_FOR for such a window, so none of this
 * reproduces on Linux - which is what this test is here to cover. It asserts the
 * parentage rather than a stacking order, since offscreen has no window manager
 * to ask.
 *
 * It also owns the only fixture with two live profiles in one mudlet instance,
 * so the cases that need two real profiles - such as the rule that a detach has
 * to leave a tab behind - live here as well.
 *
 * Run with: ctest -R DetachedWindowOwnershipTest -V
 */

#include <QAction>
#include <QDockWidget>
#include <QFileInfo>
#include <QMimeData>
#include <QSignalSpy>
#include <QTabBar>
#include <QTemporaryDir>
#include <QPointer>
#include <QScopeGuard>
#include <QWindow>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletApp.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostDialogs.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TCommandLine.h"
#include "TDetachedWindow.h"
#include "TMainConsole.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "dlgTriggerEditor.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class DetachedWindowOwnershipTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QPointer<TDetachedWindow> mpDetachedWindow;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowOwnership-First");
    const QString mSecondHostname = qsl("DetachedWindowOwnership-Second");
    // Started by the cases that need three tabs; cleanup() closes it again so
    // that every case begins with the same two
    const QString mThirdHostname = qsl("DetachedWindowOwnership-Third");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
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
        // already in use and never gets an enabled Connect button. The
        // opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);
        deleteProfileDirectory(mThirdHostname);

        // Two of them, because a tab only detaches while another one is left
        // behind in the main window
        startProfile(mFirstHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mSecondHostname);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            deleteProfileDirectory(mFirstHostname);
            deleteProfileDirectory(mSecondHostname);
            deleteProfileDirectory(mThirdHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Each test detaches for itself so that a failure cannot leave a window open
    // for the next one - and so that the reattach is a case of its own below
    void init() { mpDetachedWindow = detachSecondProfile(); }

    void cleanup()
    {
        // Whichever profiles a failed case left outside, so that none of them
        // outlives the test that detached it
        const QStringList detachedProfiles = mudlet::self()->getDetachedWindows().keys();
        for (const QString& profileName : detachedProfiles) {
            mudlet::self()->slot_tabReattachRequested(profileName);
        }
        closeThirdProfile();
        // init() detaches tab 1 and expects that to be the second profile
        const int firstTab = mudlet::self()->mpTabBar->tabIndex(mFirstHostname);
        if (firstTab > 0) {
            mudlet::self()->slot_tabDetachRequested(firstTab, QPoint(200, 200));
            reattachAndWait(mFirstHostname, 0);
        }
    }

    void test_detachedWindowIsNotOwnedByTheMainWindow()
    {
        QVERIFY(mpDetachedWindow);
        QVERIFY2(!mpDetachedWindow->parentWidget(), "the detached window has a parent widget, which makes it an owned window on Windows and keeps it above the main window forever");

        // Checked as well as the parent widget, since setTransientParent() would
        // reintroduce the same bug without a parent widget being involved
        QWindow* pHandle = mpDetachedWindow->windowHandle();
        QVERIFY2(pHandle, "the detached window was never shown, so its native window cannot be inspected");
        QVERIFY2(!pHandle->transientParent(), "the detached window has a transient parent, which keeps it above the main window on Windows");

        // The icon used to come from the parent widget and now comes from the main
        // window directly, so it is the one thing losing the parent could have
        // quietly dropped
        QVERIFY2(!mpDetachedWindow->windowIcon().isNull(), "the detached window did not inherit the main window's icon");
    }

    // detachTab() activates the detached window, but the tab change it does first
    // leaves Host::setFocusOnHostActiveCommandLine()'s zero-timer queued, and that
    // activates the main window - so without a second, later-queued activation the
    // profile the user dragged out is left behind the window they dragged it from.
    // Note this only fails without the fix while that timer actually runs: it
    // early-returns when one is already in flight (Host::mFocusHandOffPending), so
    // the A/B that CLAUDE.md asks for is what proves the assertion has teeth.
    void test_theDetachedWindowIsTheOneLeftActive()
    {
        QVERIFY(mpDetachedWindow);
        // An unconditional wait rather than qWaitFor, which returns the moment the
        // predicate first holds: this has to outlast the 10ms and 50ms follow-ups
        // setFocusOnHostActiveCommandLine() queues, so that it measures where
        // activation settles instead of a moment those could still move it from
        QTest::qWait(200ms);
        QCOMPARE(QApplication::activeWindow(), mpDetachedWindow.data());
    }

    // The player carries on typing straight after the drag, so the profile they
    // dragged out has to have the keyboard in its new window
    void test_theDetachedProfilesCommandLineHasTheKeyboard()
    {
        QVERIFY(mpDetachedWindow);
        Host* pHost = HostManager::self()->getHost(mSecondHostname);
        QVERIFY(pHost && pHost->mainConsoleView());
        reattachAndWait(mSecondHostname);
        QTest::qWait(200ms); // the reattach's own deferred focus changes
        // Away from the command line, so that only the detach can hand it the keyboard:
        // left alone, the new window gives it to its own tab bar
        mudlet::self()->mpTabBar->setFocus(Qt::MouseFocusReason);
        mpDetachedWindow = detachSecondProfile();
        QVERIFY(mpDetachedWindow);
        // Unconditional for the same reason as the case above
        QTest::qWait(200ms);
        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pHost->mainConsoleView()->mpCommandLine.data()));
    }

    // A user window is not moved with its profile, docked or floating, so the
    // command line last used in one cannot be the one given the keyboard
    void test_theDetachedProfilesCommandLineHasTheKeyboardAfterAUserWindowsWasUsed()
    {
        QVERIFY(mpDetachedWindow);
        Host* pHost = HostManager::self()->getHost(mSecondHostname);
        QVERIFY(pHost && pHost->mainConsoleView());
        reattachAndWait(mSecondHostname);
        QTest::qWait(200ms);
        // Ahead of the open, so that a check failing after it still closes the window
        auto restore = qScopeGuard([pHost]() {
            if (TConsole* pWindow = pHost->mainConsoleView()->subConsoleWidget(qsl("qaKeyboardWindow"))) {
                pHost->mainConsoleView()->forgetCommandLine(pWindow->mpCommandLine);
            }
            pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("hideWindow('qaKeyboardWindow')"));
            pHost->mainConsoleView()->recordActiveCommandLine(pHost->mainConsoleView()->mpCommandLine);
        });
        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('qaKeyboardWindow') enableCommandLine('qaKeyboardWindow')")));
        TConsole* pUserWindow = pHost->mainConsoleView()->subConsoleWidget(qsl("qaKeyboardWindow"));
        QVERIFY(pUserWindow && pUserWindow->mpCommandLine);
        const QPointer<TCommandLine> pUserCommandLine = pUserWindow->mpCommandLine.data();
        pHost->mainConsoleView()->recordActiveCommandLine(pUserCommandLine);
        mudlet::self()->mpTabBar->setFocus(Qt::MouseFocusReason);

        mpDetachedWindow = detachSecondProfile();
        QVERIFY(mpDetachedWindow);
        QVERIFY2(pUserCommandLine->window() != mpDetachedWindow.data(), "the user window moved with its profile, so this case no longer covers a command line left behind");
        QTest::qWait(200ms);
        QVERIFY2(QApplication::focusWidget() != pUserCommandLine.data(), "the keyboard went to the user window's command line, left behind in the main window");
        QCOMPARE(QApplication::activeWindow(), mpDetachedWindow.data());
        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pHost->mainConsoleView()->mpCommandLine.data()));

        // As a toolbar button, or a click on a console without a command line of its
        // own, does: the command line given the keyboard has to be the one on record from now on
        pHost->setFocusOnHostActiveCommandLine();
        QTest::qWait(200ms);
        QCOMPARE(QApplication::activeWindow(), mpDetachedWindow.data());
        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pHost->mainConsoleView()->mpCommandLine.data()));
    }

    // A command line made in the profile's own console moves with it, so when it
    // was the last one used it keeps the keyboard
    void test_aCommandLineThatMovedWithItsProfileKeepsTheKeyboard()
    {
        QVERIFY(mpDetachedWindow);
        Host* pHost = HostManager::self()->getHost(mSecondHostname);
        QVERIFY(pHost && pHost->mainConsoleView());
        reattachAndWait(mSecondHostname);
        QTest::qWait(200ms);
        auto restore = qScopeGuard([pHost]() {
            pHost->mainConsoleView()->forgetCommandLine(pHost->mainConsoleView()->subCommandLineWidget(qsl("qaKeyboardLine")));
            pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("deleteCommandLine('qaKeyboardLine')"));
            pHost->mainConsoleView()->recordActiveCommandLine(pHost->mainConsoleView()->mpCommandLine);
        });
        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createCommandLine('qaKeyboardLine', 0, 0, 200, 30)")));
        const QPointer<TCommandLine> pSubCommandLine = pHost->mainConsoleView()->subCommandLineWidget(qsl("qaKeyboardLine"));
        QVERIFY(pSubCommandLine);
        pHost->mainConsoleView()->recordActiveCommandLine(pSubCommandLine);
        mudlet::self()->mpTabBar->setFocus(Qt::MouseFocusReason);

        mpDetachedWindow = detachSecondProfile();
        QVERIFY(mpDetachedWindow);
        QVERIFY2(pSubCommandLine->window() == mpDetachedWindow.data(), "the profile's own command line did not move with it, so this case no longer covers one that did");
        QTest::qWait(200ms);
        QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(pSubCommandLine.data()));
    }

    void test_reattachingDestroysTheDetachedWindow()
    {
        QPointer<TDetachedWindow> pWindow = mpDetachedWindow;
        QVERIFY(pWindow);

        mudlet::self()->slot_tabReattachRequested(mSecondHostname);

        QVERIFY(mudlet::self()->getDetachedWindows().isEmpty());
        // Nothing owns the window now that it has no parent, so the reattach has
        // to be what deletes it
        QVERIFY(QTest::qWaitFor(
                [&pWindow]() {
                    return pWindow.isNull();
                },
                2000ms));
    }

    // The editor puts focus back on the window it was opened from once it closes,
    // and that window can be gone by then
    void test_closingAnEditorOpenedFromAGoneDetachedWindowIsSafe()
    {
        QPointer<TDetachedWindow> pWindow = mpDetachedWindow;
        QVERIFY(pWindow);
        Host* pHost = HostManager::self()->getHost(mSecondHostname);
        QVERIFY(pHost);

        QVERIFY(QMetaObject::invokeMethod(pWindow.data(), "slot_showTriggerDialog"));
        QPointer<dlgTriggerEditor> pEditor = HostDialogs::of(pHost).mpEditorDialog.data();
        QVERIFY2(pEditor, "the detached window opened no editor");

        reattachAndWait(mSecondHostname);
        QVERIFY(QTest::qWaitFor(
                [&pWindow]() {
                    return pWindow.isNull();
                },
                2000ms));

        pEditor->close();
        // Outlasts the 50ms the focus hand-back waits before it runs
        QTest::qWait(300ms);
        QVERIFY(mudlet::self());
    }

    // Which tab is being dragged out has no say in whether it may be - only how
    // many tabs are left behind does. The left-most tab used to be refused
    // outright, however many tabs sat beside it, while the tab bar's context
    // menu offered the same profile its "Detach Tab" entry.
    void test_theLeftMostTabDetachesWhileAnotherTabIsLeftBehind()
    {
        QVERIFY(mpDetachedWindow);
        // init() detached the second profile, so put it back first: with one tab
        // to its name the main window has nothing to keep and refuses either way
        reattachAndWait(mSecondHostname);
        QCOMPARE(mudlet::self()->mpTabBar->count(), 2);

        const QString leftMostProfile = mudlet::self()->mpTabBar->tabName(0);
        const QString survivingProfile = mudlet::self()->mpTabBar->tabName(1);
        // Taking out the tab that is current is the arrangement the old guard
        // put out of reach, so it is the one to detach from
        mudlet::self()->mpTabBar->setCurrentIndex(0);
        mudlet::self()->slot_tabDetachRequested(0, QPoint(200, 200));

        QVERIFY2(mudlet::self()->getDetachedWindows().contains(leftMostProfile), qPrintable(qsl("dragging the left-most tab ('%1') out of the window produced no window for it").arg(leftMostProfile)));
        QCOMPARE(mudlet::self()->mpTabBar->count(), 1);
        // The tab left behind has to be the one the main window now shows,
        // rather than the chrome of two tabs around a console that has gone
        QCOMPARE(mudlet::self()->mpTabBar->currentIndex(), 0);
        QCOMPARE(mudlet::self()->mpTabBar->tabName(0), survivingProfile);
        QVERIFY(mudlet::self()->getActiveHost());
        QCOMPARE(mudlet::self()->getActiveHost()->getName(), survivingProfile);

        // The one tab that is left has nothing to leave behind, so it stays
        mudlet::self()->slot_tabDetachRequested(0, QPoint(200, 200));
        QCOMPARE(mudlet::self()->getDetachedWindows().count(), 1);
        QCOMPARE(mudlet::self()->mpTabBar->count(), 1);

        // Back where the fixture expects it, rather than appended at the end
        reattachAndWait(leftMostProfile, 0);
    }

    // The threshold decision and the detach itself sit either side of one
    // connect(), and each is covered on its own side of it: with the signal
    // unwired a drag would do nothing at all while both of those still passed
    void test_theTabBarsDetachSignalReachesTheMainWindow()
    {
        QVERIFY(mpDetachedWindow);
        reattachAndWait(mSecondHostname);
        QCOMPARE(mudlet::self()->mpTabBar->count(), 2);

        const QString leftMostProfile = mudlet::self()->mpTabBar->tabName(0);
        emit mudlet::self() -> mpTabBar->tabDetachRequested(0, QPoint(200, 200));

        QVERIFY2(mudlet::self()->getDetachedWindows().contains(leftMostProfile),
                 qPrintable(qsl("the tab bar asked for '%1' to be detached and no window appeared, so the signal never reaches mudlet").arg(leftMostProfile)));

        reattachAndWait(leftMostProfile, 0);
    }

    // The window's own Reattach puts the profile back in the slot it was
    // dragged out of
    void test_reattachingFromTheWindowPutsTheTabBackWhereItWas()
    {
        QVERIFY(mpDetachedWindow);
        reattachAndWait(mSecondHostname);
        QCOMPARE(mudlet::self()->mpTabBar->count(), 2);

        const QString leftMostProfile = mudlet::self()->mpTabBar->tabName(0);
        mudlet::self()->slot_tabDetachRequested(0, QPoint(200, 200));
        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(leftMostProfile);
        QVERIFY2(pDetachedWindow, qPrintable(qsl("detaching the left-most tab ('%1') produced no window for it").arg(leftMostProfile)));
        QAction* pReattach = pDetachedWindow->findChild<QAction*>(qsl("reattach_action"));
        QVERIFY2(pReattach, "the detached window has no reattach action");

        pReattach->trigger();
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->getDetachedWindows().isEmpty();
                },
                2000ms));
        QCOMPARE(mudlet::self()->mpTabBar->count(), 2);
        QCOMPARE(mudlet::self()->mpTabBar->tabName(0), leftMostProfile);
    }

    // A tab from the middle of the bar, where neither "first" nor "last" would
    // pass for the slot it left
    void test_reattachingFromTheMiddleSlotPutsTheTabBetweenItsNeighbours()
    {
        startThirdProfile();
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList before = mudlet::self()->mpTabBar->tabNames();

        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));
        triggerReattach(before.at(1));

        QCOMPARE(mudlet::self()->mpTabBar->tabNames(), before);
    }

    // With two profiles out at once, each one's slot has to allow for the other
    // coming back first
    void test_profilesDetachedOneAfterAnotherComeBackInTheirOldOrder()
    {
        startThirdProfile();
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList before = mudlet::self()->mpTabBar->tabNames();

        mudlet::self()->slot_tabDetachRequested(0, QPoint(200, 200));
        mudlet::self()->slot_tabDetachRequested(0, QPoint(400, 200));
        triggerReattach(before.at(0));
        triggerReattach(before.at(1));
        QCOMPARE(mudlet::self()->mpTabBar->tabNames(), before);

        mudlet::self()->slot_tabDetachRequested(0, QPoint(200, 200));
        mudlet::self()->slot_tabDetachRequested(0, QPoint(400, 200));
        triggerReattach(before.at(1));
        triggerReattach(before.at(0));
        QCOMPARE(mudlet::self()->mpTabBar->tabNames(), before);
    }

    // The Window menu's "Reattach detached windows" goes through the profiles
    // by name rather than by where they were
    void test_reattachingAllDetachedWindowsPutsEveryTabBack()
    {
        startThirdProfile();
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList before = mudlet::self()->mpTabBar->tabNames();

        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));
        mudlet::self()->slot_tabDetachRequested(0, QPoint(400, 200));
        mudlet::self()->slot_reattachAllDetachedWindows();
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->getDetachedWindows().isEmpty();
                },
                2000ms));

        QCOMPARE(mudlet::self()->mpTabBar->tabNames(), before);
    }

    // A tab dropped on the main window's body, away from its tab bar, says
    // nothing about where it should go
    void test_droppingADetachedTabOnTheMainWindowPutsItBackWhereItWas()
    {
        startThirdProfile();
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList before = mudlet::self()->mpTabBar->tabNames();

        mudlet::self()->slot_tabDetachRequested(0, QPoint(200, 200));
        QVERIFY(mudlet::self()->getDetachedWindows().contains(before.at(0)));
        dropTab(mudlet::self(), before.at(0));
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->getDetachedWindows().isEmpty();
                },
                2000ms));

        QCOMPARE(mudlet::self()->mpTabBar->tabNames(), before);
    }

    // Double-clicking the tab in the detached window is the same Reattach
    void test_doubleClickingADetachedTabPutsItBackWhereItWas()
    {
        startThirdProfile();
        if (QTest::currentTestFailed()) {
            return;
        }
        const QStringList before = mudlet::self()->mpTabBar->tabNames();

        mudlet::self()->slot_tabDetachRequested(0, QPoint(200, 200));
        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(before.at(0));
        QVERIFY(pDetachedWindow);
        auto* pTabBar = pDetachedWindow->findChild<QTabBar*>();
        QVERIFY(pTabBar);
        QTest::mouseDClick(pTabBar, Qt::LeftButton, Qt::NoModifier, pTabBar->tabRect(0).center());
        QVERIFY(QTest::qWaitFor(
                [&before]() {
                    return !mudlet::self()->getDetachedWindows().contains(before.at(0));
                },
                2000ms));

        QCOMPARE(mudlet::self()->mpTabBar->tabNames(), before);
    }

    // The slot belongs to the profile, not to the window it was first put in
    void test_aProfileMovedToAnotherDetachedWindowKeepsItsSlot()
    {
        QVERIFY(mpDetachedWindow);
        const QStringList before = mudlet::self()->mpTabBar->tabNames();
        QCOMPARE(before.size(), 1);
        startProfile(mThirdHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        const QString movedProfile = before.at(0);

        mudlet::self()->slot_tabDetachRequested(0, QPoint(400, 200));
        QVERIFY(mudlet::self()->getDetachedWindows().value(movedProfile) != mpDetachedWindow);
        dropTab(mpDetachedWindow, movedProfile);
        QVERIFY2(mudlet::self()->getDetachedWindows().value(movedProfile) == mpDetachedWindow, qPrintable(qsl("'%1' did not move into the other detached window").arg(movedProfile)));
        mpDetachedWindow->switchToProfile(movedProfile);
        triggerReattach(movedProfile);

        QCOMPARE(mudlet::self()->mpTabBar->tabNames(), QStringList({movedProfile, mThirdHostname}));
    }

    // A main tab dropped into a window that is already open leaves its slot
    // too, and that window's Reattach puts it back there
    void test_reattachingATabDroppedIntoAnOpenWindowPutsItBackWhereItWas()
    {
        QVERIFY(mpDetachedWindow);
        startProfile(mThirdHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        QCOMPARE(mudlet::self()->mpTabBar->count(), 2);
        const QString leftMostProfile = mudlet::self()->mpTabBar->tabName(0);
        const QString rightMostProfile = mudlet::self()->mpTabBar->tabName(1);

        mudlet::self()->slot_profileDetachToWindow(leftMostProfile, mpDetachedWindow);
        QVERIFY2(mudlet::self()->getDetachedWindows().value(leftMostProfile) == mpDetachedWindow, qPrintable(qsl("'%1' did not move into the open detached window").arg(leftMostProfile)));
        mpDetachedWindow->switchToProfile(leftMostProfile);
        QCOMPARE(mpDetachedWindow->getCurrentProfileName(), leftMostProfile);
        QAction* pReattach = mpDetachedWindow->findChild<QAction*>(qsl("reattach_action"));
        QVERIFY2(pReattach, "the detached window has no reattach action");

        pReattach->trigger();
        QVERIFY(QTest::qWaitFor(
                [leftMostProfile]() {
                    return !mudlet::self()->getDetachedWindows().contains(leftMostProfile);
                },
                2000ms));
        QCOMPARE(mudlet::self()->mpTabBar->count(), 2);
        QCOMPARE(mudlet::self()->mpTabBar->tabName(0), leftMostProfile);
        QCOMPARE(mudlet::self()->mpTabBar->tabName(1), rightMostProfile);
    }

    // Detaching takes the map dock's show hook with it, so the reattach has to put one back. Hidden while
    // detached, so that the reattach's own check cannot size it first.
    void test_aReattachedMapDockShownWhereOtherDocksWereOpensAtAUsableWidth()
    {
        QPointer<QDockWidget> pDock = showSecondProfilesMapDock();
        QVERIFY(pDock);
        auto hide = qScopeGuard([&pDock]() {
            if (pDock) {
                pDock->hide();
            }
        });
        mpDetachedWindow = detachSecondProfile();
        QVERIFY(mpDetachedWindow);
        QVERIFY(pDock);
        pDock->hide();
        QTest::qWait(50ms);
        reattachAndWait(mSecondHostname);
        QVERIFY(pDock);
        QVERIFY(!pDock->isVisible());
        useAndLeaveTheRightDockArea();
        pDock->show();
        QTRY_VERIFY2(pDock->width() > pDock->minimumSizeHint().width(), qPrintable(qsl("the map is only %1 pixels wide, its minimum").arg(pDock->width())));
    }

    // The reattach shows the dock with its signals blocked, so no show hook sees it
    void test_aMapDockReattachedWhereOtherDocksWereOpensAtAUsableWidth()
    {
        QPointer<QDockWidget> pDock = showSecondProfilesMapDock();
        QVERIFY(pDock);
        auto hide = qScopeGuard([&pDock]() {
            if (pDock) {
                pDock->hide();
            }
        });
        mpDetachedWindow = detachSecondProfile();
        QVERIFY(mpDetachedWindow);
        useAndLeaveTheRightDockArea();
        reattachAndWait(mSecondHostname);
        QVERIFY(pDock);
        QVERIFY(pDock->isVisible());
        QTRY_VERIFY2(pDock->width() > pDock->minimumSizeHint().width(), qPrintable(qsl("the map is only %1 pixels wide, its minimum").arg(pDock->width())));
    }

    // Moved and then hidden before the move was laid out, the dock waits to be shown again, which a closing profile never does
    void test_aClosedProfileLeavesNoMapDockGrowthRetryBehind()
    {
        startThirdProfile();
        if (QTest::currentTestFailed()) {
            return;
        }
        Host* pHost = HostManager::self()->getHost(mThirdHostname);
        QVERIFY(pHost);
        QVERIFY(pHost->openMapWidget(qsl("right"), -1, -1, -1, -1).first);
        QTest::qWait(50ms);
        QPointer<QDockWidget> pDock = mudlet::self()->findChild<QDockWidget*>(qsl("dockMap_%1").arg(mThirdHostname));
        QVERIFY(pDock);
        QVERIFY(pHost->openMapWidget(qsl("left"), -1, -1, -1, -1).first);
        QVERIFY(pHost->closeMapWidget().first);
        QTest::qWait(50ms);
        QPointer<QObject> pRetry = pDock->findChild<QObject*>(qsl("dockGrowthRetry"), Qt::FindDirectChildrenOnly);
        QVERIFY2(pRetry, "the hidden map dock holds no retry of its own to grow once shown");

        closeThirdProfile();
        QTRY_VERIFY(!pDock);
        QVERIFY(!pRetry);
    }

private:
    // init() detached the second profile, so this puts it back to open its map in the main window
    QDockWidget* showSecondProfilesMapDock()
    {
        reattachAndWait(mSecondHostname);
        mudlet::self()->resize(1280, 900);
        mudlet::self()->mpTabBar->setCurrentIndex(mudlet::self()->mpTabBar->tabIndex(mSecondHostname));
        Host* pHost = mudlet::self()->getActiveHost();
        if (!pHost || pHost->getName() != mSecondHostname) {
            QTest::qFail("the second profile did not become the active one", __FILE__, __LINE__);
            return nullptr;
        }
        QDockWidget* pDock = mudlet::self()->getMainWindowDockWidget(qsl("map_%1").arg(mSecondHostname));
        if (!pDock || !pDock->isVisible()) {
            mudlet::self()->slot_showMapperDialog();
            pDock = mudlet::self()->getMainWindowDockWidget(qsl("map_%1").arg(mSecondHostname));
        }
        QTest::qWait(50ms);
        if (!pDock || !pDock->isVisible() || mudlet::self()->dockWidgetArea(pDock) != Qt::RightDockWidgetArea) {
            QTest::qFail("the map did not open docked on the right of the main window", __FILE__, __LINE__);
            return nullptr;
        }
        return pDock;
    }

    // Docks that shared an area and left it leave Qt nothing but its minimum to give the next one
    static void useAndLeaveTheRightDockArea()
    {
        QList<QDockWidget*> docks;
        for (int i = 0; i < 3; ++i) {
            auto pDock = new QDockWidget(mudlet::self());
            pDock->setObjectName(qsl("dockOwnershipTest_%1").arg(i));
            pDock->setWidget(new QWidget(pDock));
            mudlet::self()->addDockWidget(Qt::RightDockWidgetArea, pDock);
            docks.append(pDock);
        }
        QTest::qWait(50ms);
        qDeleteAll(docks);
        QTest::qWait(50ms);
    }

    // init() detached the second profile, so put it back and add a third
    void startThirdProfile()
    {
        QVERIFY(mpDetachedWindow);
        reattachAndWait(mSecondHostname);
        startProfile(mThirdHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        QCOMPARE(mudlet::self()->mpTabBar->count(), 3);
    }

    void closeThirdProfile()
    {
        const int thirdTab = mudlet::self()->mpTabBar->tabIndex(mThirdHostname);
        if (thirdTab < 0) {
            return;
        }
        mudlet::self()->slot_closeProfileRequested(thirdTab);
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return HostManager::self()->getHost(mThirdHostname) == nullptr;
                },
                10s));
        // Or the next case to start it is told the name is already in use
        deleteProfileDirectory(mThirdHostname);
    }

    void triggerReattach(const QString& profileName)
    {
        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(profileName);
        QVERIFY2(pDetachedWindow, qPrintable(qsl("'%1' has no detached window").arg(profileName)));
        QAction* pReattach = pDetachedWindow->findChild<QAction*>(qsl("reattach_action"));
        QVERIFY2(pReattach, "the detached window has no reattach action");
        pReattach->trigger();
        QVERIFY(QTest::qWaitFor(
                [profileName]() {
                    return !mudlet::self()->getDetachedWindows().contains(profileName);
                },
                2000ms));
    }

    // Through event() itself: QApplication::sendEvent() does not hand a
    // synthesised drop on to the widget
    static void dropTab(QWidget* pTarget, const QString& profileName)
    {
        QMimeData mimeData;
        mimeData.setData(qsl("application/x-mudlet-tab"), profileName.toUtf8());
        QDropEvent drop(QPointF(50, 50), Qt::MoveAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
        static_cast<QObject*>(pTarget)->event(&drop);
    }

    void reattachAndWait(const QString& profileName, int insertIndex = -1)
    {
        mudlet::self()->slot_tabReattachRequested(profileName, insertIndex);
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->getDetachedWindows().isEmpty();
                },
                2000ms));
    }

    TDetachedWindow* detachSecondProfile()
    {
        if (!mudlet::self()->getDetachedWindows().isEmpty()) {
            QTest::qFail("a detached window was left over from an earlier test", __FILE__, __LINE__);
            return nullptr;
        }

        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));

        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(mSecondHostname);
        if (!pDetachedWindow) {
            // Whichever profile sits at tab 1 is the one that detaches, so say
            // which one was expected rather than only that nothing appeared
            QTest::qFail(qPrintable(qsl("detaching tab 1 produced no window for '%1' - the tab order is not what this test assumes").arg(mSecondHostname)), __FILE__, __LINE__);
        }
        return pDetachedWindow;
    }

    void startProfile(const QString& hostname)
    {
        auto host = TestProfile::create(hostname, mLocalhost, mPort);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "DetachedWindowOwnershipTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowOwnershipTest)
