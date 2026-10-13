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
 * Five guards on a profile that is still opening or closing, each of which used
 * to take the application apart underneath work that was still running: a close
 * shortcut held down while a profile loads (PR #8301, issue #7478), a startup
 * autologin for a profile that is already open (PR #8475, issue #1195), a
 * close that ran inside the keystroke that asked for it rather than once
 * everything else had finished (PR #7461), a second close of the profile,
 * or of the whole application, asked for while the first one's save question
 * is still open, and a close or reset delivered by the event loop that
 * waiting for a save pumps.
 *
 * Run with: ctest -R ProfileCloseGuardTest -V
 */

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
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

    // Holds the profile's save running for 300ms and waits for it; true if the wait ran a timer
    bool holdASaveAndWaitForIt(Host* pHost)
    {
        QObject scope;
        bool pumped = false;
        pHost->mWritingHostAndModules = true;
        QTimer::singleShot(10ms, &scope, [&pumped]() {
            pumped = true;
        });
        QTimer::singleShot(300ms, &scope, [host = QPointer<Host>(pHost)]() {
            if (host) {
                host->mWritingHostAndModules = false;
            }
        });
        pHost->waitForProfileSave();
        return pumped;
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
        QVERIFY2(connected.wait(2s), "could not connect the profile to the stub server");
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

    // The save question runs the event loop, so a close of the same profile
    // reached from a script meanwhile asked again and then deleted the console
    // the first question was still running over.
    void test_aCloseAskedForWhileTheSaveQuestionIsUpIsHeldOff_data()
    {
        QTest::addColumn<bool>("closeTheApplication");
        QTest::newRow("profile") << false;
        QTest::newRow("application") << true;
    }

    void test_aCloseAskedForWhileTheSaveQuestionIsUpIsHeldOff()
    {
        QFETCH(bool, closeTheApplication);
        mpHost->mFORCE_SAVE_ON_EXIT = false;
        const auto restoreForcedSave = qScopeGuard([this]() {
            mpHost->mFORCE_SAVE_ON_EXIT = true;
        });
        QPointer<QMessageBox> firstQuestion;
        int questionsSeen = 0;
        bool secondCloseReturned = false;
        QElapsedTimer waited;
        QTimer driver;
        driver.setInterval(10ms);
        connect(&driver, &QTimer::timeout, this, [&]() {
            auto* question = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!question) {
                return;
            }
            if (!firstQuestion) {
                firstQuestion = question;
                ++questionsSeen;
                waited.start();
                QTimer::singleShot(0ms, this, [this, closeTheApplication, &secondCloseReturned]() {
                    if (closeTheApplication) {
                        mudlet::self()->close();
                    } else {
                        mudlet::self()->slot_closeProfileByName(mProfileName);
                    }
                    secondCloseReturned = true;
                });
                return;
            }
            if (question != firstQuestion) {
                ++questionsSeen;
                question->button(QMessageBox::No)->click();
                return;
            }
            // Cancel keeps the profile for the test after this one
            if (secondCloseReturned || waited.hasExpired(5000)) {
                driver.stop();
                question->button(QMessageBox::Cancel)->click();
            }
        });
        driver.start();

        mudlet::self()->slot_closeProfileByName(mProfileName);

        QVERIFY(firstQuestion.isNull() || !firstQuestion->isVisible());
        QVERIFY(secondCloseReturned);
        QCOMPARE(questionsSeen, 1);
        QTest::qWait(100ms);
        QVERIFY2(profileIsStillOpen(), "the profile closed although its save question was cancelled");
        QCOMPARE(HostManager::self()->getHost(mProfileName), mpHost);
    }

    // waitForProfileSave() pumps the event loop with the Host, and whoever called the wait, still
    // using it, so a close already posted when the wait began waits for it to end. A profile of
    // its own, as the close goes through once the wait is over.
    void test_aCloseDeliveredDuringASaveWaitWaitsForTheWaitToEnd()
    {
        const QString otherProfileName = qsl("ProfileCloseGuard-SaveWait-Profile");
        const auto removeOtherProfile = qScopeGuard([otherProfileName]() {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, otherProfileName)).removeRecursively();
        });
        QDir(MudletApp::getMudletPath(enums::profileHomePath, otherProfileName)).removeRecursively();
        const QPointer<Host> other(TestProfile::create(otherProfileName, mLocalhost, mPort));
        QVERIFY2(other, "no active host after creating the second profile");
        QCOMPARE(HostManager::self()->getHost(otherProfileName), other.data());

        mudlet::self()->slot_closeProfileByName(otherProfileName);
        QVERIFY2(other, "the profile was closed inside the call that asked for it");

        const bool pumped = holdASaveAndWaitForIt(other);

        QVERIFY2(pumped, "the wait did not run the event loop, so the posted close never reached the guard");
        QVERIFY2(other, "the profile was destroyed while its save was still being waited for");
        QVERIFY2(QTest::qWaitFor(
                         [otherProfileName]() {
                             return HostManager::self()->getHost(otherProfileName) == nullptr;
                         },
                         10s),
                 "the close never went through once the wait was over");
        QVERIFY(profileIsStillOpen());
    }

    // A wait started from a timer in another wait's pump ends first; the outer one's caller still
    // uses the profile, so a close asked for between the two ends still waits for the outer one.
    void test_aCloseBetweenTheEndsOfTwoNestedSaveWaitsWaitsForTheOuterOne()
    {
        const QString otherProfileName = qsl("ProfileCloseGuard-NestedWait-Profile");
        const auto removeOtherProfile = qScopeGuard([otherProfileName]() {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, otherProfileName)).removeRecursively();
        });
        QDir(MudletApp::getMudletPath(enums::profileHomePath, otherProfileName)).removeRecursively();
        const QPointer<Host> other(TestProfile::create(otherProfileName, mLocalhost, mPort));
        QVERIFY2(other, "no active host after creating the second profile");

        QObject scope;
        bool innerWaitReturned = false;
        bool aliveBetweenTheEnds = false;
        QTimer::singleShot(50ms, &scope, [&]() {
            other->waitForProfileSave();
            innerWaitReturned = true;
            mudlet::self()->slot_closeProfileByName(otherProfileName);
            QTest::qWait(200ms);
            aliveBetweenTheEnds = !other.isNull();
        });
        holdASaveAndWaitForIt(other);

        QVERIFY2(innerWaitReturned, "the outer wait ended before the inner one was started");
        QVERIFY2(aliveBetweenTheEnds && other, "the profile was destroyed while the outer save wait was still running");
        QVERIFY2(QTest::qWaitFor(
                         [otherProfileName]() {
                             return HostManager::self()->getHost(otherProfileName) == nullptr;
                         },
                         10s),
                 "the close never went through once the outer wait was over");
        QVERIFY(profileIsStillOpen());
    }

    // The wait ends on its own, so a close of the profile asked for during it (here the Lua
    // closeProfile(), which has already answered true) goes through once it is over
    void test_aCloseAskedForDuringASaveWaitGoesThroughOnceTheWaitIsOver()
    {
        const QString otherProfileName = qsl("ProfileCloseGuard-CloseInWait-Profile");
        const auto removeOtherProfile = qScopeGuard([otherProfileName]() {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, otherProfileName)).removeRecursively();
        });
        QDir(MudletApp::getMudletPath(enums::profileHomePath, otherProfileName)).removeRecursively();
        const QPointer<Host> other(TestProfile::create(otherProfileName, mLocalhost, mPort));
        QVERIFY2(other, "no active host after creating the second profile");

        QObject scope;
        bool asked = false;
        bool closeAccepted = false;
        QTimer::singleShot(50ms, &scope, [&]() {
            asked = true;
            closeAccepted = other->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(closeProfile() == true)"));
        });
        holdASaveAndWaitForIt(other);

        QVERIFY2(asked, "the wait ended before the close was asked for");
        QVERIFY2(closeAccepted, "closeProfile() did not answer true");
        QVERIFY2(other && !other->isClosingDown(), "the close began while a save was being waited for");
        QVERIFY2(QTest::qWaitFor(
                         [otherProfileName]() {
                             return HostManager::self()->getHost(otherProfileName) == nullptr;
                         },
                         10s),
                 "the close asked for during the wait never went through once it was over");
        QVERIFY(profileIsStillOpen());
    }

    // Likewise a reset: the Lua resetProfile() during the wait answers true, and the reset runs
    // once the wait is over rather than under its caller
    void test_aResetAskedForDuringASaveWaitRunsOnceTheWaitIsOver()
    {
        QSignalSpy resetting(mpHost, &Host::signal_profileResetting);
        QObject scope;
        bool asked = false;
        bool resetAccepted = false;
        QTimer::singleShot(50ms, &scope, [&]() {
            asked = true;
            resetAccepted = mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(resetProfile() == true)"));
        });
        holdASaveAndWaitForIt(mpHost);

        QVERIFY2(asked, "the wait ended before the reset was asked for");
        QVERIFY2(resetAccepted, "resetProfile() did not answer true");
        QVERIFY2(resetting.isEmpty(), "the reset ran while a save was being waited for");
        QVERIFY2(QTest::qWaitFor(
                         [&resetting]() {
                             return !resetting.isEmpty();
                         },
                         10s),
                 "the reset asked for during the wait never ran once it was over");
        QVERIFY(profileIsStillOpen());
    }

    // A reset asked for before the wait began has its second phase delivered by the wait's pump
    void test_aResetQueuedBeforeASaveWaitRunsOnceTheWaitIsOver()
    {
        QSignalSpy resetting(mpHost, &Host::signal_profileResetting);
        QVERIFY(mpHost->resetProfile_phase1());
        const bool pumped = holdASaveAndWaitForIt(mpHost);

        QVERIFY2(pumped, "the wait did not run the event loop, so the queued reset never reached the guard");
        QVERIFY2(resetting.isEmpty(), "the reset ran while a save was being waited for");
        QVERIFY2(QTest::qWaitFor(
                         [&resetting]() {
                             return !resetting.isEmpty();
                         },
                         10s),
                 "the reset never ran once the wait was over");
        QVERIFY(profileIsStillOpen());
    }

    // An application close (the window manager's, say) during the wait is asked again once the
    // wait is over: the profile's save question coming up shows it was, and cancelling it keeps
    // the profile for the test after this one
    void test_anApplicationCloseDuringASaveWaitIsAskedAgainOnceTheWaitIsOver()
    {
        mpHost->mFORCE_SAVE_ON_EXIT = false;
        const auto restoreForcedSave = qScopeGuard([this]() {
            mpHost->mFORCE_SAVE_ON_EXIT = true;
        });
        QObject scope;
        bool asked = false;
        bool acceptedDuringTheWait = true;
        QTimer::singleShot(50ms, &scope, [&]() {
            asked = true;
            QCloseEvent close;
            QCoreApplication::sendEvent(mudlet::self(), &close);
            acceptedDuringTheWait = close.isAccepted();
        });
        holdASaveAndWaitForIt(mpHost);

        QVERIFY2(asked, "the wait ended before the application close was asked for");
        QVERIFY2(!acceptedDuringTheWait, "the application closed while a save was being waited for");
        bool questionSeen = false;
        QTimer driver;
        driver.setInterval(10ms);
        connect(&driver, &QTimer::timeout, &scope, [&]() {
            if (auto* question = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
                questionSeen = true;
                driver.stop();
                question->button(QMessageBox::Cancel)->click();
            }
        });
        driver.start();
        QVERIFY2(QTest::qWaitFor(
                         [&questionSeen]() {
                             return questionSeen;
                         },
                         5s),
                 "the application close asked for during the wait was never asked again once it was over");
        QTest::qWait(100ms);
        QVERIFY2(profileIsStillOpen(), "the profile closed although its save question was cancelled");
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
                         10s),
                 "the profile never closed once the event loop got a turn");
    }
};

#include "ProfileCloseGuardTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileCloseGuardTest)
