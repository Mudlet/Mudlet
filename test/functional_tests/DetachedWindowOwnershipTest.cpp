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

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QPointer>
#include <QWindow>
#include <QtTest/QtTest>
#include <chrono>

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
    // early-returns when one is already in flight (Host::mFocusTimerRunning), so
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

private:
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

#include "DetachedWindowOwnershipTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowOwnershipTest)
