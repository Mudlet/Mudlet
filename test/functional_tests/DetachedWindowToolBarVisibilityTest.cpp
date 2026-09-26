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
 * The "Show main toolbar" setting has to reach a detached profile window's
 * toolbar as well as the main window's. A detached window is handed that state
 * once, in its constructor; before the fix the settings path never pushed a
 * later change, so a window already open when the setting changed is the case
 * that went missing. The toolbar's own toggle did reach those windows, through
 * mudlet::synchronizeToolBarVisibility().
 *
 * Two cases pass without the fix and are there as regression guards rather
 * than as tests of it: ...aFreshlyDetachedWindowStartsWithTheToolBarTheSettingAsksFor
 * covers the constructor path, and ...theToolBarToggleStillReachesAnOpenDetachedWindow
 * covers the toggle path, which used to reach the detached windows through a
 * loop of its own inside synchronizeToolBarVisibility(). The rest fail
 * without the fix.
 *
 * Run with: ctest -R DetachedWindowToolBarVisibilityTest -V
 */

#include <QAction>
#include <QFileInfo>
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
        // The constructor shows the toolbar without going through
        // TDetachedWindow::setToolBarVisibility(), so the menu item starts out
        // unchecked next to a visible toolbar - put the two in step here, or the
        // assertion below would hold whatever the settings path does
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        QVERIFY2(pToggleAction->isChecked(), "the setting did not check the detached window's Show Toolbar menu item, so the assertion below would test nothing");

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

private:
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
        if (!connectionSpy.wait(2000)) {
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
