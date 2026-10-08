/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet development team                     *
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
 * A profile that gets new output while another tab is the one on show has its
 * tab marked, and the mark goes when the player switches to it. A detached
 * window has a tab bar of its own, and only the main window's used to be told -
 * so a profile behind another tab of a detached window never showed that it had
 * text waiting.
 *
 * Three profiles, because it takes two to hide one behind the other in a
 * detached window and the main window keeps a tab of its own.
 *
 * What a mark looks like, and which of two marks wins, is TabActivityMarkTest's
 * to cover - this is about the right bar being told.
 *
 * Run with: ctest -R DetachedWindowTabActivityTest -V
 */

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>
#include <string>

#include "MudletApp.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TDetachedWindow.h"
#include "TMainConsole.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class DetachedWindowTabActivityTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpFirstHost = nullptr;
    Host* mpSecondHost = nullptr;
    Host* mpThirdHost = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowTabActivity-First");
    const QString mSecondHostname = qsl("DetachedWindowTabActivity-Second");
    const QString mThirdHostname = qsl("DetachedWindowTabActivity-Third");

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

        mpFirstHost = startProfile(mFirstHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        mpSecondHost = startProfile(mSecondHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        mpThirdHost = startProfile(mThirdHostname);
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

    void cleanup()
    {
        // Whichever profiles a case left outside, so that each one starts from
        // three tabs in the main window
        const QStringList detachedProfiles = mudlet::self()->getDetachedWindows().keys();
        for (const QString& profileName : detachedProfiles) {
            mudlet::self()->slot_tabReattachRequested(profileName);
        }
        QVERIFY(mudlet::self()->getDetachedWindows().isEmpty());
        QCOMPARE(mudlet::self()->mpTabBar->count(), 3);
    }

    void test_gameTextForAProfileBehindAnotherTabMarksItsTab()
    {
        TDetachedWindow* pWindow = gatherSecondAndThirdIntoOneWindow();
        QVERIFY(pWindow);
        TTabBar* pTabBar = tabBarOf(pWindow);
        QVERIFY(pTabBar);
        QVERIFY2(!pTabBar->tabBold(mSecondHostname), "the tab is marked before any text has arrived, so finding it marked afterwards would prove nothing");

        sendGameText(mpSecondHost);

        QVERIFY2(pTabBar->tabBold(mSecondHostname), "the game sent text to a profile behind another tab of a detached window, and its tab does not say so");
        QVERIFY(!pTabBar->tabItalic(mSecondHostname));
    }

    // cTelnet posts whatever followed the server's last newline a moment after
    // the line itself, and that is usually nothing at all
    void test_anEmptyPostLeavesAHiddenProfilesTabAlone()
    {
        TDetachedWindow* pWindow = gatherSecondAndThirdIntoOneWindow();
        QVERIFY(pWindow);
        TTabBar* pTabBar = tabBarOf(pWindow);
        QVERIFY(pTabBar);
        QVERIFY(!pTabBar->tabBold(mSecondHostname));

        // What cTelnet::slot_timerPosting() posts when nothing was held back
        std::string nothing{"\r"};
        mpSecondHost->printOnDisplay(nothing, true);

        QVERIFY2(!pTabBar->tabBold(mSecondHostname), "a post that added no text marked the tab of a profile behind another one");
    }

    // A chat capture that moves a line elsewhere and gags it leaves the main
    // buffer looking as it did, but the player still has text to read
    void test_aGaggedLineStillMarksAHiddenProfilesTab()
    {
        TDetachedWindow* pWindow = gatherSecondAndThirdIntoOneWindow();
        QVERIFY(pWindow);
        TTabBar* pTabBar = tabBarOf(pWindow);
        QVERIFY(pTabBar);
        QVERIFY(!pTabBar->tabBold(mSecondHostname));
        QVERIFY(mpSecondHost->getLuaInterpreter()->compileAndExecuteScript(qsl("gagTriggerId = tempTrigger('The wind howls.', function() deleteLine() end)")));

        sendGameText(mpSecondHost);
        const bool marked = pTabBar->tabBold(mSecondHostname);
        // Removed ahead of the assertion, which returns on failure
        QVERIFY(mpSecondHost->getLuaInterpreter()->compileAndExecuteScript(qsl("killTrigger(gagTriggerId)")));

        QVERIFY2(marked, "a line a trigger gagged left the hidden profile's tab unmarked");
    }

    void test_gameTextForTheProfileOnShowLeavesItsTabAlone()
    {
        TDetachedWindow* pWindow = gatherSecondAndThirdIntoOneWindow();
        QVERIFY(pWindow);
        TTabBar* pTabBar = tabBarOf(pWindow);
        QVERIFY(pTabBar);

        sendGameText(mpThirdHost);

        QVERIFY2(!pTabBar->tabBold(mThirdHostname), "the profile the detached window is showing was marked as having text the player has not seen");
    }

    void test_switchingToAMarkedTabClearsIt()
    {
        TDetachedWindow* pWindow = gatherSecondAndThirdIntoOneWindow();
        QVERIFY(pWindow);
        TTabBar* pTabBar = tabBarOf(pWindow);
        QVERIFY(pTabBar);
        sendGameText(mpSecondHost);
        QVERIFY(pTabBar->tabBold(mSecondHostname));

        // What a click on the tab comes to
        pTabBar->setCurrentIndex(pTabBar->tabIndex(mSecondHostname));

        QCOMPARE(pWindow->getCurrentProfileName(), mSecondHostname);
        QVERIFY2(!pTabBar->tabBold(mSecondHostname), "the player switched to the profile and its tab still claims text they have not seen");

        // The switch has to move which profile counts as hidden as well, or the
        // tab left behind could never be marked
        sendGameText(mpThirdHost);
        QVERIFY2(pTabBar->tabBold(mThirdHostname), "the profile the player switched away from got text and its tab does not say so");
    }

    // Multiview puts every profile of the main window on show at once, which is
    // why it turns the marks off there. It does nothing of the kind for a
    // detached window, which goes on showing one profile at a time.
    void test_multiviewDoesNotSilenceADetachedWindow()
    {
        TDetachedWindow* pWindow = gatherSecondAndThirdIntoOneWindow();
        QVERIFY(pWindow);
        TTabBar* pTabBar = tabBarOf(pWindow);
        QVERIFY(pTabBar);
        QVERIFY(!pTabBar->tabBold(mSecondHostname));

        mudlet::self()->slot_multiView(true);
        sendGameText(mpSecondHost);
        const bool marked = pTabBar->tabBold(mSecondHostname);
        // Put back ahead of the assertion, which returns on failure
        mudlet::self()->slot_multiView(false);

        QVERIFY2(marked, "with multiview on, text for a profile hidden in a detached window went unmarked");
    }

    // The main window's marks now come from the same two functions as a detached
    // window's, so this is here to see that sharing them cost it nothing
    void test_theMainWindowStillMarksAndClearsItsOwnTabs()
    {
        TTabBar* pTabBar = mudlet::self()->mpTabBar;
        QCOMPARE(pTabBar->count(), 3);
        // Visiting a tab is what clears it, so visit both of the ones under test,
        // which leaves neither carrying a mark from an earlier case
        mudlet::self()->activateProfile(mpSecondHost);
        mudlet::self()->activateProfile(mpFirstHost);
        QVERIFY(!pTabBar->tabBold(mFirstHostname));
        QVERIFY(!pTabBar->tabBold(mSecondHostname));

        sendGameText(mpSecondHost);
        QVERIFY2(pTabBar->tabBold(mSecondHostname), "the game sent text to a profile behind another tab of the main window, and its tab does not say so");

        sendGameText(mpFirstHost);
        QVERIFY2(!pTabBar->tabBold(mFirstHostname), "the profile the main window is showing was marked as having text the player has not seen");

        pTabBar->setCurrentIndex(pTabBar->tabIndex(mSecondHostname));
        QVERIFY2(!pTabBar->tabBold(mSecondHostname), "the player switched to the profile and its tab still claims text they have not seen");
    }

    // mudlet runs an orphan check from a timer, which reattaches any profile it
    // finds in neither the main window nor a detached one. A move that yields to
    // the event loop part way through must not leave the profile in that state,
    // or the check gives it a main window tab as well as its new window.
    void test_aProfileMovingIntoAWindowIsNeverOrphaned()
    {
        mudlet::self()->slot_tabDetachRequested(mudlet::self()->mpTabBar->tabIndex(mSecondHostname), QPoint(200, 200));
        TDetachedWindow* pWindow = mudlet::self()->getDetachedWindows().value(mSecondHostname);
        QVERIFY(pWindow);

        bool checked = false;
        QStringList orphansMidMove;
        QTimer::singleShot(0, mudlet::self(), [&checked, &orphansMidMove]() {
            checked = true;
            orphansMidMove = mudlet::self()->getOrphanedProfiles();
            mudlet::self()->reattachOrphanedProfiles();
        });
        mudlet::self()->slot_profileDetachToWindow(mThirdHostname, pWindow);
        QCoreApplication::processEvents();

        QVERIFY(checked);
        QVERIFY2(orphansMidMove.isEmpty(), qPrintable(qsl("orphaned while moving into a detached window: %1").arg(orphansMidMove.join(qsl(", ")))));
        QCOMPARE(mudlet::self()->getDetachedWindows().value(mThirdHostname), pWindow);
        QCOMPARE(mudlet::self()->mpTabBar->count(), 1);
    }

private:
    // Leaves the first profile in the main window and puts the other two in one
    // detached window. That window ends up showing the third, since a profile
    // joining a window is switched to - so the second is the one out of sight.
    TDetachedWindow* gatherSecondAndThirdIntoOneWindow()
    {
        mudlet::self()->slot_tabDetachRequested(mudlet::self()->mpTabBar->tabIndex(mSecondHostname), QPoint(200, 200));
        TDetachedWindow* pWindow = mudlet::self()->getDetachedWindows().value(mSecondHostname);
        if (!pWindow) {
            QTest::qFail(qPrintable(qsl("detaching '%1' produced no window for it").arg(mSecondHostname)), __FILE__, __LINE__);
            return nullptr;
        }

        mudlet::self()->slot_profileDetachToWindow(mThirdHostname, pWindow);
        if (mudlet::self()->getDetachedWindows().value(mThirdHostname) != pWindow) {
            QTest::qFail(qPrintable(qsl("'%1' did not join the window '%2' was detached into").arg(mThirdHostname, mSecondHostname)), __FILE__, __LINE__);
            return nullptr;
        }
        if (pWindow->getCurrentProfileName() != mThirdHostname) {
            QTest::qFail(qPrintable(qsl("the detached window is showing '%1', and every case here assumes '%2'").arg(pWindow->getCurrentProfileName(), mThirdHostname)), __FILE__, __LINE__);
            return nullptr;
        }
        return pWindow;
    }

    // The bar is private to the window. The consoles that move in with their
    // profiles bring no tab bars of their own, which is what the count checks.
    TTabBar* tabBarOf(TDetachedWindow* pWindow)
    {
        const QList<TTabBar*> tabBars = pWindow->findChildren<TTabBar*>();
        if (tabBars.size() != 1) {
            QTest::qFail(qPrintable(qsl("expected the detached window to hold one tab bar and found %1").arg(tabBars.size())), __FILE__, __LINE__);
            return nullptr;
        }
        return tabBars.first();
    }

    // The call cTelnet::postData() makes with each batch of text from the game
    void sendGameText(Host* pHost)
    {
        std::string text{"The wind howls.\n"};
        pHost->printOnDisplay(text, true);
    }

    Host* startProfile(const QString& hostname)
    {
        Host* pHost = TestProfile::create(hostname, mLocalhost, mPort);
        if (!pHost) {
            QTest::qFail("No active host available for the test.", __FILE__, __LINE__);
            return nullptr;
        }

        QSignalSpy connectionSpy(&(pHost->mTelnet), &cTelnet::signal_connected);
        QSignalSpy greetingSpy(pHost->mpConsole.data(), &TMainConsole::signal_newDataAlert);
        if (!connectionSpy.wait(2000)) {
            QTest::qFail("Could not connect with the host.", __FILE__, __LINE__);
            return nullptr;
        }
        // The stub greets each client a moment after it connects, and that is
        // new text like any other. Left to arrive in its own time it would mark
        // a tab partway through whichever case was running by then.
        if (greetingSpy.isEmpty() && !greetingSpy.wait(2000)) {
            QTest::qFail("The server's greeting never arrived.", __FILE__, __LINE__);
            return nullptr;
        }
        return pHost;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "DetachedWindowTabActivityTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowTabActivityTest)
