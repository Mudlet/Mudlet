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
 * A detached profile window's toolbar and menu bar have to follow the "Show
 * main toolbar" and "Show menu bar" settings as the main window's do: when the
 * window is detached, and when a setting changes while it is open. Its Window
 * menu holds a "Show Toolbar" item whose checkmark matches the toolbar, and its
 * menu shortcuts keep working while its menu bar is hidden.
 *
 * ...aFreshlyDetachedWindowStartsWithTheToolBarTheSettingAsksFor and
 * ...theToolBarToggleStillReachesAnOpenDetachedWindow are regression guards for
 * paths that have no fix of their own here: the constructor's toolbar state,
 * and the toolbar's own toggle through mudlet::synchronizeToolBarVisibility().
 *
 * Run with: ctest -R DetachedWindowToolBarVisibilityTest -V
 */

#include <QAction>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QToolBar>
#include <QtTest/QtTest>

#include "MudletApp.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ActionEventCounter : public QObject
{
public:
    int mAdded = 0;
    int mRemoved = 0;

    bool eventFilter(QObject*, QEvent* event) override
    {
        if (event->type() == QEvent::ActionAdded) {
            ++mAdded;
        } else if (event->type() == QEvent::ActionRemoved) {
            ++mRemoved;
        }
        return false;
    }
};

class DetachedWindowToolBarVisibilityTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowToolBarVisibility-First");
    const QString mSecondHostname = qsl("DetachedWindowToolBarVisibility-Second");
    const QString mThirdHostname = qsl("DetachedWindowToolBarVisibility-Third");

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

        // A config root of this process's own, so that a second copy of this test
        // running at the same time is not told the profile names are in use. The
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
        // mpMainToolBar->isVisible() stays false while the main window itself is
        // hidden, whatever the toolbar was told - and detachTab() seeds a new
        // window from exactly that
        // mpMainToolBar->isVisible() stays false while the main window itself is
        // hidden, whatever the toolbar was told - and detachTab() seeds a new
        // window from exactly that. Starting a profile below happens to show the
        // main window too, but this does not lean on that.
        mudlet::self()->show();
        QVERIFY(mudlet::self()->isVisible());

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);
        deleteProfileDirectory(mThirdHostname);

        // Three of them: slot_tabDetachRequested() refuses index 0, and two have
        // to be detachable at once
        startProfile(mFirstHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mSecondHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mThirdHostname);
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

    // Cases run in declaration order and each leaves the two settings where it
    // put them, so every one starts from a toolbar that is turned off and a menu
    // bar that allows it to be hidden again
    void init()
    {
        mudlet::self()->setMenuBarVisibility(enums::visibleAlways);
        mudlet::self()->setToolBarVisibility(enums::visibleNever);
        QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier case");
    }

    // Every case detaches for itself, so that the setting can change before or
    // after the detach as the case needs
    void cleanup()
    {
        const QStringList detachedProfiles = mudlet::self()->getDetachedWindows().keys();
        for (const QString& profileName : detachedProfiles) {
            mudlet::self()->slot_tabReattachRequested(profileName);
        }
    }

    void test_turningTheToolBarOnReachesAnOpenDetachedWindow()
    {
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        QAction* pToggleAction = toolBarToggleAction(pDetachedWindow);
        QVERIFY(pToggleAction);
        QVERIFY2(!pDetachedToolBar->isVisible(), "the window detached with the toolbar turned off started out showing one");

        mudlet::self()->setToolBarVisibility(enums::visibleAlways);

        QVERIFY2(mudlet::self()->mpMainToolBar->isVisible(), "the main window did not take the setting either, so this run tested nothing");
        QVERIFY2(pDetachedToolBar->isVisible(), "the already-detached window did not gain the toolbar the setting turned on");
        QVERIFY2(pToggleAction->isChecked(), "the detached window's Show Toolbar menu item is still unchecked next to a toolbar that is now showing");
    }

    void test_turningTheToolBarOffReachesAnOpenDetachedWindow()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        QAction* pToggleAction = toolBarToggleAction(pDetachedWindow);
        QVERIFY(pToggleAction);
        QVERIFY2(pDetachedToolBar->isVisible(), "the window detached with the toolbar turned on started out without one");
        QVERIFY2(pToggleAction->isChecked(), "the detached window's Show Toolbar menu item is unchecked, so the assertion below would test nothing");

        mudlet::self()->setToolBarVisibility(enums::visibleNever);

        QVERIFY2(!mudlet::self()->mpMainToolBar->isVisible(), "the main window did not take the setting either, so this run tested nothing");
        QVERIFY2(!pDetachedToolBar->isVisible(), "the already-detached window kept the toolbar the setting turned off");
        QVERIFY2(!pToggleAction->isChecked(), "the detached window's Show Toolbar menu item is still checked next to a toolbar that is now hidden");
    }

    // "Until a profile is loaded" leaves a toolbar-less main window while
    // profiles are open, and a detached window is one of those profiles
    void test_theUntilAProfileIsLoadedSettingReachesAnOpenDetachedWindow()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);

        mudlet::self()->setToolBarVisibility(enums::visibleOnlyWithoutLoadedProfile);

        QVERIFY2(!mudlet::self()->mpMainToolBar->isVisible(), "the main window did not take the setting either, so this run tested nothing");
        QVERIFY2(!pDetachedToolBar->isVisible(), "the already-detached window kept a toolbar the setting only allows without a loaded profile");
    }

    // A window detached after the setting changed is a separate path from a
    // missed live update: its toolbar is built from scratch at that point
    void test_aFreshlyDetachedWindowStartsWithTheToolBarTheSettingAsksFor()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);

        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);

        QVERIFY2(pDetachedToolBar->isVisible(), "a window detached while the setting was on has no toolbar");
    }

    // Tabs can be dragged out one after another, so the setting has to land on
    // every open window rather than on whichever one the loop reaches first
    void test_theSettingReachesEveryOpenDetachedWindow()
    {
        TDetachedWindow* pFirstDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pFirstDetachedWindow);
        TDetachedWindow* pSecondDetachedWindow = detachProfile(mThirdHostname);
        QVERIFY(pSecondDetachedWindow);
        QVERIFY2(pFirstDetachedWindow != pSecondDetachedWindow, "both profiles ended up in one window, so there is only one toolbar to reach");
        QToolBar* pFirstToolBar = detachedToolBar(pFirstDetachedWindow);
        QToolBar* pSecondToolBar = detachedToolBar(pSecondDetachedWindow);
        QVERIFY(pFirstToolBar);
        QVERIFY(pSecondToolBar);

        mudlet::self()->setToolBarVisibility(enums::visibleAlways);

        QVERIFY2(mudlet::self()->mpMainToolBar->isVisible(), "the main window did not take the setting either, so this run tested nothing");
        QVERIFY2(pFirstToolBar->isVisible(), "the first detached window did not gain the toolbar the setting turned on");
        QVERIFY2(pSecondToolBar->isVisible(), "only one of the two detached windows gained the toolbar the setting turned on");

        mudlet::self()->setToolBarVisibility(enums::visibleNever);

        QVERIFY2(!pFirstToolBar->isVisible(), "the first detached window kept the toolbar the setting turned off");
        QVERIFY2(!pSecondToolBar->isVisible(), "only one of the two detached windows lost the toolbar the setting turned off");
    }

    // The toolbar's own toggle, from the detached window's context menu or its
    // Show Toolbar menu item, resolves through synchronizeToolBarVisibility()
    // and has to keep both windows in step without a loop of its own
    void test_theToolBarToggleStillReachesAnOpenDetachedWindow()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        QAction* pToggleAction = toolBarToggleAction(pDetachedWindow);
        QVERIFY(pToggleAction);
        QVERIFY(pDetachedToolBar->isVisible());

        mudlet::self()->synchronizeToolBarVisibility(false);

        QVERIFY2(!mudlet::self()->mpMainToolBar->isVisible(), "toggling the toolbar off left the main window's toolbar showing");
        QVERIFY2(!pDetachedToolBar->isVisible(), "toggling the toolbar off left the detached window's toolbar showing");
        QVERIFY2(!pToggleAction->isChecked(), "the detached window's Show Toolbar menu item stayed checked after the toggle turned the toolbar off");

        mudlet::self()->synchronizeToolBarVisibility(true);

        QVERIFY2(mudlet::self()->mpMainToolBar->isVisible(), "toggling the toolbar back on left the main window without one");
        QVERIFY2(pDetachedToolBar->isVisible(), "toggling the toolbar back on left the detached window without one");
        QVERIFY2(pToggleAction->isChecked(), "the detached window's Show Toolbar menu item stayed unchecked after the toggle turned the toolbar on");
    }

    // synchronizeToolBarVisibility() refuses to hide a toolbar while the menu
    // bar is set to never show; the settings path has never had that guard and
    // does not gain one here, because a detached window that disagreed with the
    // main window is the fault being fixed. Change this case deliberately if the
    // guard is ever extended to the settings path - and extend it to the main
    // window's own toolbar at the same time.
    void test_withTheMenuBarNeverShownTheDetachedWindowStillMatchesTheMainWindow()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        QVERIFY(pDetachedToolBar->isVisible());

        mudlet::self()->setMenuBarVisibility(enums::visibleNever);
        mudlet::self()->setToolBarVisibility(enums::visibleNever);

        QVERIFY2(!mudlet::self()->mpMainToolBar->isVisible(), "the settings path has started guarding the main window's toolbar against a hide - see this case's comment");
        QCOMPARE(pDetachedToolBar->isVisible(), mudlet::self()->mpMainToolBar->isVisible());
    }

    // The window is built hidden, so its toolbar reads as hidden whatever it
    // was told, and the menu item cannot be seeded from that
    void test_aFreshlyDetachedWindowsShowToolbarItemMatchesItsToolBar()
    {
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        QAction* pToggleAction = toolBarToggleAction(pDetachedWindow);
        QVERIFY(pToggleAction);
        QVERIFY(!pDetachedToolBar->isVisible());
        QVERIFY2(!pToggleAction->isChecked(), "a window detached without its toolbar has its Show Toolbar menu item checked");

        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        pToggleAction = toolBarToggleAction(pDetachedWindow);
        QVERIFY(pToggleAction);
        QVERIFY(pDetachedToolBar->isVisible());
        QVERIFY2(pToggleAction->isChecked(), "a window detached with its toolbar showing has its Show Toolbar menu item unchecked");
    }

    // A menu item no menu holds is one nobody can click
    void test_theShowToolbarItemIsInTheWindowMenu()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QAction* pToggleAction = toolBarToggleAction(pDetachedWindow);
        QVERIFY(pToggleAction);

        QMenu* pHoldingMenu = nullptr;
        const QList<QAction*> menuBarActions = pDetachedWindow->menuBar()->actions();
        for (QAction* pMenuAction : menuBarActions) {
            if (pMenuAction->menu() && pMenuAction->menu()->actions().contains(pToggleAction)) {
                pHoldingMenu = pMenuAction->menu();
            }
        }
        QVERIFY2(pHoldingMenu, "no menu of the detached window holds its Show Toolbar item");

        // Two items with one mnemonic make its key only move the highlight
        const QChar toggleMnemonic = mnemonicOf(pToggleAction->text());
        QVERIFY(!toggleMnemonic.isNull());
        const QList<QAction*> siblingActions = pHoldingMenu->actions();
        for (QAction* pSibling : siblingActions) {
            if (pSibling != pToggleAction) {
                QVERIFY2(mnemonicOf(pSibling->text()) != toggleMnemonic, qPrintable(qsl("the Show Toolbar item shares its mnemonic with \"%1\"").arg(pSibling->text())));
            }
        }

        // Hiding the toolbar with the menu bar never shown would leave the
        // window with neither, so the item is not offered then
        mudlet::self()->setMenuBarVisibility(enums::visibleNever);
        emit pHoldingMenu->aboutToShow();
        QVERIFY2(!pToggleAction->isEnabled(), "the Show Toolbar item offers to hide the last of the window's controls");

        mudlet::self()->setToolBarVisibility(enums::visibleNever);
        emit pHoldingMenu->aboutToShow();
        QVERIFY2(pToggleAction->isEnabled(), "the Show Toolbar item cannot bring back a hidden toolbar while the menu bar is never shown");

        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        mudlet::self()->setMenuBarVisibility(enums::visibleAlways);
        emit pHoldingMenu->aboutToShow();
        QVERIFY2(pToggleAction->isEnabled(), "the Show Toolbar item stayed disabled after the menu bar came back");
    }

    void test_theShowToolbarItemTogglesTheToolBars()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        QAction* pToggleAction = toolBarToggleAction(pDetachedWindow);
        QVERIFY(pToggleAction);
        QVERIFY(pDetachedToolBar->isVisible());

        pToggleAction->trigger();
        QVERIFY2(!pDetachedToolBar->isVisible(), "the Show Toolbar item did not hide the detached window's toolbar");
        QVERIFY2(!mudlet::self()->mpMainToolBar->isVisible(), "the Show Toolbar item did not hide the main window's toolbar");
        QVERIFY2(!pToggleAction->isChecked(), "the Show Toolbar item stayed checked after hiding the toolbar");

        pToggleAction->trigger();
        QVERIFY2(pDetachedToolBar->isVisible(), "the Show Toolbar item did not bring back the detached window's toolbar");
        QVERIFY2(mudlet::self()->mpMainToolBar->isVisible(), "the Show Toolbar item did not bring back the main window's toolbar");
        QVERIFY2(pToggleAction->isChecked(), "the Show Toolbar item stayed unchecked after showing the toolbar");
    }

    void test_theMenuBarSettingReachesAnOpenDetachedWindow()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QVERIFY2(pDetachedWindow->menuBar()->isVisible(), "the window detached with the menu bar turned on started out without one");

        mudlet::self()->setMenuBarVisibility(enums::visibleNever);
        QVERIFY2(!mudlet::self()->menuBar()->isVisible(), "the main window did not take the setting either, so this run tested nothing");
        QVERIFY2(!pDetachedWindow->menuBar()->isVisible(), "the already-detached window kept the menu bar the setting turned off");

        mudlet::self()->setMenuBarVisibility(enums::visibleAlways);
        QVERIFY2(pDetachedWindow->menuBar()->isVisible(), "the already-detached window did not get back the menu bar the setting turned on");
    }

    // A detached window always holds a loaded profile
    void test_theUntilAProfileIsLoadedMenuBarSettingReachesAnOpenDetachedWindow()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QVERIFY(pDetachedWindow->menuBar()->isVisible());

        mudlet::self()->setMenuBarVisibility(enums::visibleOnlyWithoutLoadedProfile);
        QVERIFY2(!mudlet::self()->menuBar()->isVisible(), "the main window kept a menu bar the setting only allows without a loaded profile");
        QVERIFY2(!pDetachedWindow->menuBar()->isVisible(), "the already-detached window kept a menu bar the setting only allows without a loaded profile");
    }

    void test_aFreshlyDetachedWindowStartsWithTheMenuBarTheSettingAsksFor()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        mudlet::self()->setMenuBarVisibility(enums::visibleNever);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);

        QVERIFY2(!pDetachedWindow->menuBar()->isVisible(), "a window detached while the menu bar was turned off has one");
    }

    // Qt fires no shortcut of an action whose only container is a hidden menu
    // bar, so a window without its menu bar still has to answer them
    void test_theMenuShortcutsStillWorkWithTheMenuBarHidden()
    {
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        mudlet::self()->setMenuBarVisibility(enums::visibleNever);
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QVERIFY(pDetachedWindow->menuBar()->isHidden());

        const QKeySequence timeStampSequence(Qt::CTRL | Qt::ALT | Qt::Key_T);
        QAction* pTimeStampAction = nullptr;
        const QList<QAction*> menuBarActions = pDetachedWindow->menuBar()->actions();
        for (QAction* pMenuAction : menuBarActions) {
            const QList<QAction*> menuActions = pMenuAction->menu() ? pMenuAction->menu()->actions() : QList<QAction*>();
            for (QAction* pAction : menuActions) {
                if (pAction->shortcut() == timeStampSequence) {
                    pTimeStampAction = pAction;
                }
            }
        }
        QVERIFY2(pTimeStampAction, "no menu item of the detached window has the time stamp shortcut, so there is nothing to press");

        QVERIFY2(QTest::qWaitFor(
                         [pDetachedWindow]() {
                             pDetachedWindow->activateWindow();
                             return QApplication::activeWindow() == pDetachedWindow;
                         },
                         2000ms),
                 "the detached window never became the active window, so its window shortcuts could not be reached");
        QSignalSpy triggeredSpy(pTimeStampAction, &QAction::triggered);
        QTest::keyClick(pDetachedWindow, Qt::Key_T, Qt::ControlModifier | Qt::AltModifier);
        const qsizetype firings = triggeredSpy.count();
        // Undone ahead of the compare, which returns early when it fails
        for (qsizetype i = 0; i < firings; ++i) {
            pTimeStampAction->trigger();
        }
        QCOMPARE(firings, 1);
    }

    // The window's own copies of the menu actions are put in once: adding an
    // action a widget already holds removes it and adds it again
    void test_refreshingTheToolBarActionsLeavesTheWindowsActionsAlone()
    {
        TDetachedWindow* pDetachedWindow = detachProfile(mSecondHostname);
        QVERIFY(pDetachedWindow);
        QVERIFY(!pDetachedWindow->actions().isEmpty());
        ActionEventCounter counter;
        pDetachedWindow->installEventFilter(&counter);

        pDetachedWindow->updateToolBarActions();

        pDetachedWindow->removeEventFilter(&counter);
        QCOMPARE(counter.mRemoved, 0);
        QCOMPARE(counter.mAdded, 0);
    }

private:
    // Read off the text, not QKeySequence::mnemonic(): macOS turns mnemonics off,
    // so that returns an empty sequence for every item there
    static QChar mnemonicOf(const QString& text)
    {
        for (int i = 0; i < text.size() - 1; ++i) {
            if (text.at(i) != QLatin1Char('&')) {
                continue;
            }
            if (text.at(i + 1) == QLatin1Char('&')) {
                ++i;
                continue;
            }
            return text.at(i + 1).toLower();
        }
        return {};
    }

    QToolBar* detachedToolBar(TDetachedWindow* pDetachedWindow) const { return pDetachedWindow ? pDetachedWindow->findChild<QToolBar*>(qsl("detachedMainToolBar")) : nullptr; }

    QAction* toolBarToggleAction(TDetachedWindow* pDetachedWindow) const { return pDetachedWindow ? pDetachedWindow->findChild<QAction*>(qsl("toggle_toolbar_action")) : nullptr; }

    // Reattaching appends the tab rather than putting it back where it was, so
    // ask the tab bar where the profile is now instead of assuming an index
    TDetachedWindow* detachProfile(const QString& profileName)
    {
        const int tabIndex = mudlet::self()->mpTabBar->tabIndex(profileName);
        if (tabIndex < 1) {
            QTest::qFail(qPrintable(qsl("'%1' is at tab %2, which slot_tabDetachRequested() will not detach").arg(profileName).arg(tabIndex)), __FILE__, __LINE__);
            return nullptr;
        }

        mudlet::self()->slot_tabDetachRequested(tabIndex, QPoint(200, 200));

        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(profileName);
        if (!pDetachedWindow) {
            QTest::qFail(qPrintable(qsl("detaching tab %1 produced no window for '%2'").arg(tabIndex).arg(profileName)), __FILE__, __LINE__);
            return nullptr;
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

#include "DetachedWindowToolBarVisibilityTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowToolBarVisibilityTest)
