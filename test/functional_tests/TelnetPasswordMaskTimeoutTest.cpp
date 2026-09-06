/***************************************************************************
 *   Copyright (C) 2026 by Morquin - morquin@morquin.dk                    *
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

// A server that sends IAC WILL ECHO puts the profile into password masking and
// is trusted to send WONT ECHO once the password line is done. A server that
// never does would leave the player typing invisibly for the rest of the
// session, so during the login phase cTelnet keeps a 60 second single-shot
// timer that clears the masking itself. The prompt does not start that timer:
// each masked line the player sends starts it, and restarts it, so it can only
// fire a minute after the last line went out and never while one is still being
// typed - which would clear the input mid-password.
//
// When it fires it sends DONT ECHO as well as dropping Mudlet's masking flag,
// so a later WILL ECHO from the same game is a fresh request rather than a
// repeat of one cTelnet still has on its books. Without that, the second
// password prompt of a session is typed in the clear.
//
// History: the arming used to compare QElapsedTimer::elapsed(), which is in
// milliseconds, against .count() of a 5 minute constant, which is the number 5,
// so the safety net only existed inside the first 5 milliseconds of a
// connection. Hence the wait below until the connection is at least 50 ms old,
// which is past that window and far inside the intended 5 minutes, and the
// interval being pinned at exactly 60000 rather than merely non-zero.
//
// Budget: cTelnet::checkEchoAnomalyPattern() counts every WILL and every WONT
// ECHO, and 5 toggles inside a 5 second window latch an anomaly that makes the
// process refuse ECHO for good. A grouped ctest case runs this whole class in
// one process, so init() clears that window before every test function and each
// function may make at most 4 toggles of its own. The two below spend 2 each.

#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <chrono>

#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TelnetPasswordMaskTimeoutTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-Password-Mask-Timeout");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    // Feeds one IAC <command> ECHO as if the server had sent it. Loopback rather
    // than the socket, so the whole sequence is processed before the call returns
    // and no wait is needed for the state it leaves behind.
    void serverSaysEcho(const char command)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(OPT_ECHO);
        mpHost->mTelnet.loopbackTest(data);
    }

    // cTelnet's record of the server half of the ECHO negotiation, which gates
    // whether a WILL ECHO is acted on or dropped as a repeat.
    bool echoNegotiatedByServer() const { return mpHost->mTelnet.hisOptionState.test(static_cast<size_t>(OPT_ECHO)); }

    // A line the player submits, sent the way Host::send() sends it.
    void playerSends(const QString& text)
    {
        QString line = text;
        QVERIFY2(mpHost->mTelnet.sendData(line, true, true), "the line was not written to the stub server");
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
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        // The connection clock starts just before signal_connected is emitted, so
        // it says whether the connection already completed inside create(), which
        // a spy constructed only now could not have seen.
        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (!mpHost->mTelnet.mConnectionTimer.isValid()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the stub server.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        // Each case gets a clean ECHO anomaly window (see the file comment), so
        // the toggles one case spends do not count against the next.
        mpHost->mTelnet.mEchoToggleCount = 0;
        mpHost->mTelnet.mEchoAnomalyDetected = false;
        mpHost->mTelnet.mEchoToggleTimer.invalidate();
    }

    // The well-behaved server: the prompt only turns masking on, the password
    // line starts the safety net, and WONT ECHO takes it back down again before
    // it can fire.
    void test_promptDoesNotStartTheClockAndWontEchoCancelsIt()
    {
        QVERIFY(mpHost);

        QVERIFY2(!mpHost->mDisablePasswordMasking, "the profile has password masking switched off, so no timeout would ever be armed");
        QVERIFY2(!mpHost->isRemoteEchoingActive(), "masking was already on before this test announced any ECHO");
        QVERIFY2(!echoNegotiatedByServer(), "ECHO was already negotiated, so the WILL below would be ignored as a repeat");
        QTimer* timer = mpHost->mTelnet.mTimerPasswordModeTimeout;
        QVERIFY2(!timer || !timer->isActive(), "something armed the password mode timeout before this test did");

        QVERIFY(mpHost->mTelnet.mConnectionTimer.isValid());
        // The assertions below only bite once the connection is old enough to be
        // outside the 5 millisecond window the unfixed comparison left open.
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return mpHost->mTelnet.mConnectionTimer.elapsed() >= 50;
                },
                5s));

        serverSaysEcho(TN_WILL);

        QVERIFY2(mpHost->isRemoteEchoingActive(), "WILL ECHO did not turn password masking on");
        timer = mpHost->mTelnet.mTimerPasswordModeTimeout;
        QVERIFY2(!timer || !timer->isActive(), "the prompt alone started the clock, so it can fire while the player is still typing the password");

        playerSends(qsl("hunter2"));

        timer = mpHost->mTelnet.mTimerPasswordModeTimeout;
        QVERIFY2(timer, "the masked line did not create the password mode timeout at all");
        QVERIFY2(timer->isActive(), "the password mode timeout exists but the masked line did not start it");
        QVERIFY2(timer->isSingleShot(), "the password mode timeout would repeat");
        QCOMPARE(timer->interval(), 60000);

        // Every masked line starts the clock afresh. Reading the remaining time
        // back cannot show that, since a coarse QTimer may shift a minute-long
        // deadline by a few seconds either way, so stop it by hand instead and
        // require the next line to bring it back.
        timer->stop();

        playerSends(qsl("hunter2"));

        QVERIFY2(timer->isActive(), "the second masked line did not restart the password mode timeout");
        QCOMPARE(timer->interval(), 60000);

        serverSaysEcho(TN_WONT);

        QVERIFY2(!mpHost->isRemoteEchoingActive(), "WONT ECHO did not turn password masking off");
        QVERIFY2(!timer->isActive(), "WONT ECHO left the password mode timeout running");
        QVERIFY2(!echoNegotiatedByServer(), "WONT ECHO did not clear the negotiated ECHO state");
    }

    // The buggy server: WILL ECHO, the password line, and then silence. The timer
    // is what ends the masking, so it is fired early here rather than waiting out
    // its minute, and the game has to be able to mask a later prompt afterwards.
    void test_timeoutClearsMaskingAndLetsTheNextPromptMaskAgain()
    {
        QVERIFY(mpHost);

        QVERIFY2(!mpHost->isRemoteEchoingActive(), "masking was still on when this test started");
        QVERIFY2(!echoNegotiatedByServer(), "ECHO was still negotiated, so the WILL below would be ignored as a repeat");

        serverSaysEcho(TN_WILL);

        QVERIFY2(mpHost->isRemoteEchoingActive(), "WILL ECHO did not turn password masking on");

        playerSends(qsl("hunter2"));

        QTimer* timer = mpHost->mTelnet.mTimerPasswordModeTimeout;
        QVERIFY2(timer, "the masked line did not create the password mode timeout at all");
        QVERIFY2(timer->isActive(), "the masked line during the login phase did not start the password mode timeout");
        QCOMPARE(timer->interval(), 60000);

        // Same timer, same connected slot: only the wait is cut short.
        timer->start(0ms);
        QTRY_VERIFY_WITH_TIMEOUT(!mpHost->isRemoteEchoingActive(), 5000);
        QVERIFY2(!timer->isActive(), "the single-shot timeout was still running after it fired");
        QVERIFY2(!echoNegotiatedByServer(), "the timeout did not send DONT ECHO, so the game's next WILL ECHO is dropped as a repeat");

        serverSaysEcho(TN_WILL);

        QVERIFY2(mpHost->isRemoteEchoingActive(), "a password prompt after the timeout no longer masks anything");
        QVERIFY2(!timer->isActive(), "the prompt alone started the clock again");
    }
};

#include "TelnetPasswordMaskTimeoutTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetPasswordMaskTimeoutTest)
