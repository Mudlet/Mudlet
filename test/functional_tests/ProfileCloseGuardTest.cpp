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
 * Three guards on a profile that is still opening or closing, each of which used
 * to take the application apart underneath work that was still running: a close
 * shortcut held down while a profile loads (PR #8301, issue #7478), a startup
 * autologin for a profile that is already open (PR #8475, issue #1195), and a
 * close that ran inside the keystroke that asked for it rather than once
 * everything else had finished (PR #7461).
 *
 * Run with: ctest -R ProfileCloseGuardTest -V
 */

#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ProfileCloseGuardTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ProfileCloseGuard-Test-Profile");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    bool profileIsStillOpen() const { return HostManager::self()->getHost(mProfileName) != nullptr; }

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
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)).removeRecursively();
        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "no active host after creating the profile");
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2000), "could not connect the profile to the stub server");
        // otherwise closing the profile asks whether to save it, and the modal
        // question would hang the test
        QVERIFY2(mpHost->mFORCE_SAVE_ON_EXIT, "profiles must save without asking, or a close puts up a modal question");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mProfileName);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // PR #8301: the close shortcut only asked whether a profile was there, not
    // whether it had finished opening, so holding the keys down while picking a
    // game tore the profile down from under its own load.
    void test_theCloseShortcutLetsAProfileFinishLoading()
    {
        QVERIFY(profileIsStillOpen());
        // the shortcut closes the profile of the current tab, so a fixture where
        // either is something else would go green without reaching the guard
        QCOMPARE(mudlet::self()->getActiveHost(), mpHost);
        QCOMPARE(mudlet::self()->mpTabBar->currentIndex(), mudlet::self()->mpTabBar->tabIndex(mProfileName));

        mpHost->mIsProfileLoadingSequence = true;
        const auto clearLoadingFlag = qScopeGuard([this]() {
            mpHost->mIsProfileLoadingSequence = false;
        });

        mudlet::self()->slot_closeCurrentProfile();
        QTest::qWait(100ms);

        QVERIFY2(profileIsStillOpen(), "the close shortcut closed a profile that was still loading");
    }

    // PR #8475: autologin did not ask whether the profile was already loaded, so
    // a profile named twice at startup was loaded on top of itself.
    void test_autoLoginSkipsAProfileThatIsAlreadyOpen()
    {
        QVERIFY(profileIsStillOpen());
        QSignalSpy loadedAgain(mudlet::self(), &mudlet::signal_profileLoaded);

        mudlet::self()->doAutoLogin(mProfileName, true);
        QTest::qWait(100ms);

        QCOMPARE(loadedAgain.count(), 0);
        QCOMPARE(HostManager::self()->getHost(mProfileName), mpHost);
    }

    // PR #7461: the close used to run inside the keystroke that asked for it, so
    // a held-down shortcut stacked one close on top of the last. Runs last: it
    // is the one that takes the profile away.
    void test_aCloseWaitsForTheEventLoopBeforeItHappens()
    {
        const int tab = mudlet::self()->mpTabBar->tabIndex(mProfileName);
        QVERIFY2(tab >= 0, "the profile has no tab to close");

        mudlet::self()->slot_closeProfileRequested(tab);

        QVERIFY2(profileIsStillOpen(), "the profile was closed inside the call that asked for it");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !profileIsStillOpen();
                         },
                         10000),
                 "the profile never closed once the event loop got a turn");
    }
};

#include "ProfileCloseGuardTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileCloseGuardTest)
