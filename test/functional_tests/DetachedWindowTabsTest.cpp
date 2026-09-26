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
 * A detached window can hold several profiles as tabs, and profiles move
 * between it, the main window and other detached windows. None of that has a
 * Lua entry point, so it is driven here the way the UI drives it: tab drops,
 * the tab bar's own signals, and the window's menu actions.
 *
 * Four live profiles, because the main window always keeps one tab and several
 * cases need two or three tabs in a detached window beside that.
 *
 * Run with: ctest -R DetachedWindowTabsTest -V
 */

#include <QDockWidget>
#include <QFileInfo>
#include <QMimeData>
#include <QPointer>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TDetachedWindow.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class DetachedWindowTabsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QStringList mProfiles{qsl("DetachedWindowTabs-1"), qsl("DetachedWindowTabs-2"), qsl("DetachedWindowTabs-3"), qsl("DetachedWindowTabs-4")};
    // Far enough apart that two 800x600 windows cannot overlap, since two that
    // do merge on their own a moment after one of them moves
    const QPoint mFirstWindowPos{200, 200};
    const QPoint mSecondWindowPos{2200, 200};

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

        for (const QString& profileName : mProfiles) {
            startProfile(profileName);
            if (QTest::currentTestFailed()) {
                return;
            }
        }
        QCOMPARE(mudlet::self()->mpTabBar->count(), mProfiles.size());
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Every case starts from all four profiles in the main window, whatever the
    // one before it left behind
    void cleanup()
    {
        const QStringList detachedProfiles = mudlet::self()->getDetachedWindows().keys();
        for (const QString& profileName : detachedProfiles) {
            mudlet::self()->slot_tabReattachRequested(profileName, -1);
        }
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->getDetachedWindows().isEmpty();
                },
                2000ms));
        // Let the deferred deletes of the windows just emptied run now rather
        // than in the middle of the next case
        QTest::qWait(150ms);
    }

    void test_aDroppedTabMovesIntoTheWindow()
    {
        TDetachedWindow* pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        QVERIFY(pWindow);
        QCOMPARE(pWindow->getProfileCount(), 1);

        QMimeData foreignMime;
        foreignMime.setText(mProfiles.at(2));
        QVERIFY2(!dragEnter(pWindow, &foreignMime), "the window accepted a drag that was not a Mudlet tab");

        QVERIFY(drop(pWindow, mProfiles.at(2)));

        QCOMPARE(pWindow->getProfileCount(), 2);
        QCOMPARE(tabBarOf(pWindow)->tabNames(), QStringList({mProfiles.at(3), mProfiles.at(2)}));
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(2));
        QCOMPARE(shownConsole(pWindow), consoleOf(mProfiles.at(2)));
        QCOMPARE(mudlet::self()->getDetachedWindows().value(mProfiles.at(2)).data(), pWindow);
        QVERIFY(!mudlet::self()->mpTabBar->tabNames().contains(mProfiles.at(2)));

        // Dropping a tab the window already holds changes nothing
        QVERIFY(drop(pWindow, mProfiles.at(2)));
        QCOMPARE(pWindow->getProfileCount(), 2);
        QCOMPARE(tabBarOf(pWindow)->count(), 2);
    }

    // Taking a tab out of a detached window has to leave the neighbour a user
    // would expect selected - and the console shown has to be that tab's
    void test_removingATabSelectsTheRightNeighbour()
    {
        TDetachedWindow* pWindow = windowWithThreeTabs();
        QVERIFY(pWindow);
        // [4, 3, 2], with 2 current as the last one to arrive
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(1));

        // The current tab, when it is the last one: the one before it takes over
        reattachAndWait(mProfiles.at(1));
        QCOMPARE(tabBarOf(pWindow)->tabNames(), QStringList({mProfiles.at(3), mProfiles.at(2)}));
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(2));
        QCOMPARE(tabBarOf(pWindow)->currentIndex(), 1);
        QCOMPARE(shownConsole(pWindow), consoleOf(mProfiles.at(2)));

        // A tab ahead of the current one: the current one stays current
        QVERIFY(drop(pWindow, mProfiles.at(1)));
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(1));
        reattachAndWait(mProfiles.at(3));
        QCOMPARE(tabBarOf(pWindow)->tabNames(), QStringList({mProfiles.at(2), mProfiles.at(1)}));
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(1));
        QCOMPARE(tabBarOf(pWindow)->currentIndex(), 1);
        QCOMPARE(shownConsole(pWindow), consoleOf(mProfiles.at(1)));

        // The current tab with one after it: the next one moves into its place
        QVERIFY(drop(pWindow, mProfiles.at(3)));
        pWindow->switchToProfile(mProfiles.at(1));
        QCOMPARE(tabBarOf(pWindow)->currentIndex(), 1);
        reattachAndWait(mProfiles.at(1));
        QCOMPARE(tabBarOf(pWindow)->tabNames(), QStringList({mProfiles.at(2), mProfiles.at(3)}));
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(3));
        QCOMPARE(shownConsole(pWindow), consoleOf(mProfiles.at(3)));
    }

    // Dragging tabs around within the window rebuilds the stack of consoles
    // behind them, which must not change which console is on screen
    void test_reorderingTabsKeepsTheShownConsole()
    {
        TDetachedWindow* pWindow = windowWithThreeTabs();
        QVERIFY(pWindow);
        pWindow->switchToProfile(mProfiles.at(2));
        TTabBar* pTabBar = tabBarOf(pWindow);
        QCOMPARE(pTabBar->tabNames(), QStringList({mProfiles.at(3), mProfiles.at(2), mProfiles.at(1)}));
        QCOMPARE(shownConsole(pWindow), consoleOf(mProfiles.at(2)));

        pTabBar->moveTab(1, 2);

        QCOMPARE(pTabBar->tabNames(), QStringList({mProfiles.at(3), mProfiles.at(1), mProfiles.at(2)}));
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(2));
        QCOMPARE(shownConsole(pWindow), consoleOf(mProfiles.at(2)));
        QStackedWidget* pStack = stackOf(pWindow);
        QCOMPARE(pStack->count(), 3);
        for (int i = 0; i < pTabBar->count(); ++i) {
            QCOMPARE(pStack->widget(i), consoleOf(pTabBar->tabName(i)));
        }
    }

    void test_movingTheLastProfileOutOfAWindowClosesIt()
    {
        TDetachedWindow* pTarget = detach(mProfiles.at(3), mFirstWindowPos);
        QPointer<TDetachedWindow> pSource = detach(mProfiles.at(2), mSecondWindowPos);
        QVERIFY(pTarget);
        QVERIFY(pSource);
        QVERIFY(pSource != pTarget);

        QVERIFY(drop(pTarget, mProfiles.at(2)));

        QCOMPARE(pTarget->getProfileNames().size(), 2);
        QVERIFY(pTarget->getProfileNames().contains(mProfiles.at(2)));
        QCOMPARE(mudlet::self()->getDetachedWindows().value(mProfiles.at(2)).data(), pTarget);
        QVERIFY2(QTest::qWaitFor(
                         [&pSource]() {
                             return pSource.isNull();
                         },
                         2000ms),
                 "the window the last profile left was not destroyed");
    }

    // Moving one detached window over most of another merges the two, taking
    // the moved window's profiles into the one it landed on
    void test_overlappingWindowsMerge()
    {
        QPointer<TDetachedWindow> pStill = detach(mProfiles.at(3), mFirstWindowPos);
        QPointer<TDetachedWindow> pMoved = detach(mProfiles.at(2), mSecondWindowPos);
        QVERIFY(pStill);
        QVERIFY(pMoved);
        // Apart, they have to stay two windows - or the merge below proves nothing
        QTest::qWait(300ms);
        QVERIFY(pStill && pMoved);
        QCOMPARE(pStill->getProfileCount(), 1);
        QCOMPARE(pMoved->getProfileCount(), 1);

        pMoved->activateWindow();
        QVERIFY(QTest::qWaitFor(
                [&pMoved]() {
                    return pMoved->isActiveWindow();
                },
                2000ms));
        pMoved->move(pStill->pos() + QPoint(20, 20));

        QVERIFY2(QTest::qWaitFor(
                         [this, &pStill]() {
                             return mudlet::self()->getDetachedWindows().value(mProfiles.at(2)).data() == pStill.data();
                         },
                         2000ms),
                 "dragging one detached window over another did not merge them");
        QVERIFY(pStill);
        QCOMPARE(pStill->getProfileCount(), 2);
        QVERIFY(QTest::qWaitFor(
                [&pMoved]() {
                    return pMoved.isNull();
                },
                2000ms));
        // Its profile moved rather than closed
        QVERIFY(HostManager::self()->getHost(mProfiles.at(2)));
    }

    // Dragging a tab out of a detached window's tab bar takes the current
    // profile home and leaves the others where they are
    void test_draggingATabOutReattachesOnlyThatProfile()
    {
        TDetachedWindow* pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        QVERIFY(pWindow);
        QVERIFY(drop(pWindow, mProfiles.at(2)));
        QCOMPARE(pWindow->getCurrentProfileName(), mProfiles.at(2));

        emit tabBarOf(pWindow)->tabDetachRequested(1, QPoint(50, 50));

        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return !mudlet::self()->getDetachedWindows().contains(mProfiles.at(2));
                },
                2000ms));
        QVERIFY(mudlet::self()->mpTabBar->tabNames().contains(mProfiles.at(2)));
        QCOMPARE(pWindow->getProfileNames(), QStringList({mProfiles.at(3)}));
        QVERIFY(pWindow->isVisible());
    }

    void test_draggingTheLastTabOutClosesTheWindow()
    {
        QPointer<TDetachedWindow> pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        QVERIFY(pWindow);

        emit tabBarOf(pWindow)->tabDetachRequested(0, QPoint(50, 50));

        QVERIFY(QTest::qWaitFor(
                [&pWindow]() {
                    return pWindow.isNull();
                },
                2000ms));
        QVERIFY(mudlet::self()->mpTabBar->tabNames().contains(mProfiles.at(3)));
        // Reattached, not closed along with its window
        QVERIFY(HostManager::self()->getHost(mProfiles.at(3)));
    }

    // The preference is global, so a detached window's tabs have to follow it
    // as well as the main window's
    void test_connectionIndicatorsFollowTheSetting()
    {
        TDetachedWindow* pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        QVERIFY(pWindow);
        QVERIFY(drop(pWindow, mProfiles.at(2)));
        TTabBar* pTabBar = tabBarOf(pWindow);
        const bool originalSetting = mudlet::self()->showTabConnectionIndicators();

        mudlet::self()->setShowTabConnectionIndicators(false);
        for (int i = 0; i < pTabBar->count(); ++i) {
            QCOMPARE(pTabBar->tabConnectionIndicator(i), TabConnectionIndicator::None);
        }

        mudlet::self()->setShowTabConnectionIndicators(true);
        for (int i = 0; i < pTabBar->count(); ++i) {
            QVERIFY2(pTabBar->tabConnectionIndicator(i) == TabConnectionIndicator::Connected, qPrintable(qsl("tab %1 ('%2') shows no connected indicator").arg(i).arg(pTabBar->tabName(i))));
        }

        mudlet::self()->setShowTabConnectionIndicators(false);
        for (int i = 0; i < pTabBar->count(); ++i) {
            QCOMPARE(pTabBar->tabConnectionIndicator(i), TabConnectionIndicator::None);
        }

        mudlet::self()->setShowTabConnectionIndicators(originalSetting);
    }

    // The toolbar and menus of a detached window act on its own profile, even
    // while the main window's active host is another one - which it gets back
    // afterwards
    void test_windowActionsActOnTheWindowsOwnProfile()
    {
        TDetachedWindow* pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        QVERIFY(pWindow);
        Host* pDetachedHost = HostManager::self()->getHost(mProfiles.at(3));
        Host* pMainHost = HostManager::self()->getHost(mProfiles.at(0));
        QVERIFY(pDetachedHost && pMainHost);
        mudlet::self()->mpCurrentActiveHost = pMainHost;
        const bool detachedBefore = pDetachedHost->mpConsole->timeStampButton->isChecked();
        const bool mainBefore = pMainHost->mpConsole->timeStampButton->isChecked();

        QVERIFY(QMetaObject::invokeMethod(pWindow, "slot_toggleTimeStamp"));

        QCOMPARE(pDetachedHost->mpConsole->timeStampButton->isChecked(), !detachedBefore);
        QCOMPARE(pMainHost->mpConsole->timeStampButton->isChecked(), mainBefore);
        QCOMPARE(mudlet::self()->mpCurrentActiveHost.data(), pMainHost);

        QVERIFY(QMetaObject::invokeMethod(pWindow, "slot_toggleTimeStamp"));
        QCOMPARE(pDetachedHost->mpConsole->timeStampButton->isChecked(), detachedBefore);
    }

    void test_windowMenuSwitchesToAProfileInAnotherDetachedWindow()
    {
        TDetachedWindow* pFirst = detach(mProfiles.at(3), mFirstWindowPos);
        TDetachedWindow* pSecond = detach(mProfiles.at(2), mSecondWindowPos);
        QVERIFY(pFirst && pSecond);
        QVERIFY(drop(pSecond, mProfiles.at(1)));
        QCOMPARE(pSecond->getCurrentProfileName(), mProfiles.at(1));

        QAction* pAction = windowMenuAction(pFirst, mProfiles.at(2), qsl("(Detached)"));
        QVERIFY2(pAction, "the first window's Window menu does not list the profile held by the second one");
        pAction->trigger();

        QCOMPARE(pSecond->getCurrentProfileName(), mProfiles.at(2));
        QCOMPARE(shownConsole(pSecond), consoleOf(mProfiles.at(2)));
    }

    void test_windowMenuSwitchesToAProfileInTheMainWindow()
    {
        TDetachedWindow* pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        QVERIFY(pWindow);
        TTabBar* pMainTabBar = mudlet::self()->mpTabBar;
        const int firstIndex = pMainTabBar->tabNames().indexOf(mProfiles.at(0));
        const int secondIndex = pMainTabBar->tabNames().indexOf(mProfiles.at(1));
        QVERIFY(firstIndex >= 0 && secondIndex >= 0);
        pMainTabBar->setCurrentIndex(firstIndex);
        QCOMPARE(pMainTabBar->currentIndex(), firstIndex);

        QAction* pAction = windowMenuAction(pWindow, mProfiles.at(1), qsl("(Main Window)"));
        QVERIFY2(pAction, "the detached window's Window menu does not list a profile held by the main window");
        pAction->trigger();

        QCOMPARE(pMainTabBar->currentIndex(), secondIndex);
        QVERIFY(mudlet::self()->getActiveHost());
        QCOMPARE(mudlet::self()->getActiveHost()->getName(), mProfiles.at(1));
    }

    // The toolbar's map button docks a map into the detached window itself.
    // One window can hold several profiles, so their maps take turns: only the
    // current tab's is shown, and only if the user left it open. Last, since
    // the reattach in cleanup() takes the dock into the main window with it.
    void test_theMapButtonDocksAMapThatFollowsItsTab()
    {
        TDetachedWindow* pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        QVERIFY(pWindow);
        QVERIFY(drop(pWindow, mProfiles.at(2)));
        const QString mapKey = qsl("map_%1").arg(mProfiles.at(2));
        Host* pHost = HostManager::self()->getHost(mProfiles.at(2));
        QVERIFY(pHost && pHost->mpMap);
        QVERIFY(!pWindow->getDockWidget(mapKey));

        QVERIFY(QMetaObject::invokeMethod(pWindow, "slot_showMapperDialog"));

        QPointer<QDockWidget> pDock = pWindow->getDockWidget(mapKey);
        QVERIFY2(pDock, "the map button made no map dock in the detached window");
        QVERIFY(pDock->isVisible());
        auto pMapper = qobject_cast<dlgMapper*>(pDock->widget());
        QVERIFY(pMapper);
        QCOMPARE(pHost->mpMap->mpMapper.data(), pMapper);

        pWindow->switchToProfile(mProfiles.at(3));
        QVERIFY2(!pDock->isVisible(), "another profile's map stayed on screen after its tab was left");

        pWindow->switchToProfile(mProfiles.at(2));
        QVERIFY2(pDock->isVisible(), "the map did not come back with its tab");

        // Closed by the user, it stays closed when its tab comes round again
        QVERIFY(QMetaObject::invokeMethod(pWindow, "slot_showMapperDialog"));
        QVERIFY(!pDock->isVisible());
        pWindow->switchToProfile(mProfiles.at(3));
        pWindow->switchToProfile(mProfiles.at(2));
        QVERIFY2(!pDock->isVisible(), "a map the user closed reopened when its tab was selected again");
    }

private:
    TDetachedWindow* detach(const QString& profileName, const QPoint& position)
    {
        const int index = mudlet::self()->mpTabBar->tabNames().indexOf(profileName);
        if (index < 0) {
            QTest::qFail(qPrintable(qsl("'%1' is not in the main window to detach").arg(profileName)), __FILE__, __LINE__);
            return nullptr;
        }
        mudlet::self()->slot_tabDetachRequested(index, position);
        return mudlet::self()->getDetachedWindows().value(profileName);
    }

    // [4, 3, 2] in one detached window, profile 1 left in the main window
    TDetachedWindow* windowWithThreeTabs()
    {
        TDetachedWindow* pWindow = detach(mProfiles.at(3), mFirstWindowPos);
        if (!pWindow || !drop(pWindow, mProfiles.at(2)) || !drop(pWindow, mProfiles.at(1))) {
            QTest::qFail("could not put three profiles into one detached window", __FILE__, __LINE__);
            return nullptr;
        }
        return pWindow;
    }

    static bool dragEnter(TDetachedWindow* pWindow, QMimeData* pMime)
    {
        const QPoint centre = pWindow->rect().center();
        QDragEnterEvent enter(centre, Qt::MoveAction, pMime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pWindow, &enter);
        return enter.isAccepted();
    }

    // What TTabBar puts on the clipboard when a tab is dragged
    static bool drop(TDetachedWindow* pWindow, const QString& profileName)
    {
        QMimeData mime;
        mime.setData(qsl("application/x-mudlet-tab"), profileName.toUtf8());
        if (!dragEnter(pWindow, &mime)) {
            return false;
        }
        QDropEvent dropEvent(QPointF(pWindow->rect().center()), Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pWindow, &dropEvent);
        return dropEvent.isAccepted();
    }

    void reattachAndWait(const QString& profileName)
    {
        mudlet::self()->slot_tabReattachRequested(profileName, -1);
        QVERIFY(QTest::qWaitFor(
                [&profileName]() {
                    return !mudlet::self()->getDetachedWindows().contains(profileName);
                },
                2000ms));
    }

    static TTabBar* tabBarOf(TDetachedWindow* pWindow) { return pWindow->centralWidget()->findChild<TTabBar*>(QString(), Qt::FindDirectChildrenOnly); }

    static QStackedWidget* stackOf(TDetachedWindow* pWindow) { return pWindow->centralWidget()->findChild<QStackedWidget*>(QString(), Qt::FindDirectChildrenOnly); }

    static QWidget* shownConsole(TDetachedWindow* pWindow) { return stackOf(pWindow)->currentWidget(); }

    static QWidget* consoleOf(const QString& profileName)
    {
        Host* pHost = HostManager::self()->getHost(profileName);
        return pHost ? pHost->mpConsole.data() : nullptr;
    }

    static QAction* windowMenuAction(TDetachedWindow* pWindow, const QString& profileName, const QString& location)
    {
        pWindow->updateWindowMenu();
        // The menu's previous entries go by deleteLater()
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        const auto actions = pWindow->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly);
        for (QAction* pAction : actions) {
            if (pAction->data().toString() == profileName && pAction->text().contains(location)) {
                return pAction;
            }
        }
        return nullptr;
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
};

#include "DetachedWindowTabsTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowTabsTest)
