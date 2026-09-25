/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

// The policy behind the hidden-input box, with no box in sight: whether the
// game's request for hidden input (IAC WILL ECHO) should be answered with one
// is decided on the Host from five inputs - the ECHO state, the profile's
// preference, a suppression that lasts until the game releases ECHO, a
// dismissal that lasts until the player's next line goes to the game, and
// whether the auto-login still intends to send the stored password - and
// answered by Host::passwordEntryWanted() and one signal that fires only when
// that answer changes. Host::sendPasswordEntry() is the one path text takes out
// of the box, straight to the wire past every hook a command line's line runs.
//
// Budget: cTelnet::checkEchoAnomalyPattern() counts every WILL and every WONT
// ECHO that cTelnet acts on, and 5 toggles inside a 5 second window latch an
// anomaly that makes the process refuse ECHO for good. init() clears that window
// before every case, and each case may make at most 4 toggles of its own,
// counting the WONT cleanup() sends for a case that left ECHO up.

#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "TMainConsole.h"
#include "ctelnet.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class PasswordEntryPolicyTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Password-Entry-Policy");
    const QString mLocalhost = qsl("localhost");
    QByteArray mOriginalEncoding;

    // Feeds one IAC <command> <option> as if the server had sent it, through the
    // loopback so that the whole sequence is processed before the call returns.
    void serverSaysOption(const char command, const char option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(option);
        mpHost->mTelnet.loopbackTest(data);
    }

    void serverSaysEcho(const char command) { serverSaysOption(command, OPT_ECHO); }

    bool echoNegotiatedByServer() const { return mpHost->mTelnet.hisOptionState.test(static_cast<size_t>(OPT_ECHO)); }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    QString luaGlobal(const char* name) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    QByteArray asSent(const QString& line) const { return line.toUtf8() + (mpHost->mUSE_UNIX_EOL ? QByteArrayLiteral("\n") : QByteArrayLiteral("\r\n")); }

    bool waitForServerToReceive(const QByteArray& text) const
    {
        return QTest::qWaitFor(
                [this, &text]() {
                    return mpServer->received().contains(text);
                },
                5000);
    }

    QString consoleText() const { return mpHost->mpConsole->buffer.lineBuffer.join(QChar::LineFeed); }

    // Puts back everything a case can change about the profile, its cTelnet and
    // the process-wide ECHO state, so that a case failing partway does not fail
    // the ones after it as well.
    void resetState()
    {
        auto& telnet = mpHost->mTelnet;
        telnet.mEchoToggleCount = 0;
        telnet.mEchoAnomalyDetected = false;
        telnet.mEchoToggleTimer.invalidate();
        for (QTimer* timer : {telnet.mTimerPasswordModeTimeout, telnet.mTimerCharacterModeDetect, telnet.mTimerLogin, telnet.mTimerPass}) {
            if (timer) {
                timer->stop();
            }
        }
        telnet.mServerRequestedSGA = false;
        telnet.mCharacterModeDetected = false;
        telnet.mAutoLoginPasswordOutstanding = false;
        telnet.mEncodingWarningIssued = false;
        mpHost->setLogin(QString());
        mpHost->setPass(QString());
        mpHost->mSecuredPasswordPending = false;
        mpHost->setDisablePasswordMasking(false);
        telnet.setAutoLoginPending(false);
        // Through the negotiation, never setRemoteEchoingActive(false) alone,
        // which would leave cTelnet believing ECHO is still on and the next
        // case's WILL ignored as a repeat. The Host flags are cleared anyway:
        // a case that never negotiated ECHO leaves them as it set them.
        if (mpHost->isRemoteEchoingActive() || echoNegotiatedByServer()) {
            serverSaysEcho(TN_WONT);
        }
        mpHost->setRemoteEchoingActive(false);
        mpHost->mAllowToSendCommand = true;
        mpServer->forgetReceived();
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

        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (!mpHost->mTelnet.mConnectionTimer.isValid()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
        }
        mOriginalEncoding = mpHost->mTelnet.getEncoding();
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        resetState();
        QVERIFY2(!mpHost->passwordEntryWanted(), "a box was wanted before this case asked for anything");
    }

    void cleanup()
    {
        // The last case takes the view down and the connection with it, so
        // there is nothing left to put back.
        if (mpHost && mpHost->mpConsole) {
            resetState();
        }
    }

    // Toggles spent: 2.
    void test_willEchoWantsABoxAndWontEchoTakesItDown()
    {
        QSignalSpy wanted(mpHost, &Host::signal_passwordEntryWantedChanged);

        serverSaysEcho(TN_WILL);
        QVERIFY2(mpHost->passwordEntryWanted(), "WILL ECHO did not ask for a hidden-input box");
        QCOMPARE(wanted.count(), 1);
        QCOMPARE(wanted.last().first().toBool(), true);

        // Nothing changed, so nothing is announced again
        mpHost->recomputePasswordEntryWanted();
        mpHost->setRemoteEchoingActive(true);
        QCOMPARE(wanted.count(), 1);

        serverSaysEcho(TN_WONT);
        QVERIFY2(!mpHost->passwordEntryWanted(), "WONT ECHO did not take the hidden-input box down");
        QCOMPARE(wanted.count(), 2);
        QCOMPARE(wanted.last().first().toBool(), false);
    }

    // Toggles spent: 2.
    void test_thePreferenceTurnsTheBoxOffAndOnMidPrompt()
    {
        QSignalSpy wanted(mpHost, &Host::signal_passwordEntryWantedChanged);
        serverSaysEcho(TN_WILL);
        QVERIFY(mpHost->passwordEntryWanted());

        mpHost->setDisablePasswordMasking(true);
        QVERIFY2(!mpHost->passwordEntryWanted(), "turning the preference on mid-prompt did not close the box");
        QCOMPARE(wanted.count(), 2);

        mpHost->setDisablePasswordMasking(false);
        QVERIFY2(mpHost->passwordEntryWanted(), "turning the preference off mid-prompt did not open the box");
        QCOMPARE(wanted.count(), 3);

        // With the preference on, a prompt asks for nothing and nothing is announced
        serverSaysEcho(TN_WONT);
        mpHost->setDisablePasswordMasking(true);
        QCOMPARE(wanted.count(), 4);
        serverSaysEcho(TN_WILL);
        QVERIFY(!mpHost->passwordEntryWanted());
        QCOMPARE(wanted.count(), 4);
    }

    // The first Esc on an empty box hides it until the player's next line goes
    // to the game; the second within the same hold hides it until the game
    // releases ECHO. A script's send() is not the player's line. Toggles spent: 4.
    void test_escStepsPastOneLineThenLastsUntilTheGameReleasesEcho()
    {
        QSignalSpy wanted(mpHost, &Host::signal_passwordEntryWantedChanged);
        serverSaysEcho(TN_WILL);
        QVERIFY(mpHost->passwordEntryWanted());
        QVERIFY(!mpHost->passwordEntryReopened());

        mpHost->dismissPasswordEntry();
        QVERIFY2(!mpHost->passwordEntryWanted(), "the first Esc did not hide the box");
        QVERIFY2(mpHost->passwordEntryReopened(), "the first Esc was not remembered for the box that follows it");
        QCOMPARE(wanted.count(), 2);

        // Not the player's line: a script sending at 10 Hz must not keep bringing the box back
        QVERIFY(runLua(qsl("send('look')")));
        QVERIFY(waitForServerToReceive(asSent(qsl("look"))));
        QTest::qWait(50);
        QVERIFY2(!mpHost->passwordEntryWanted(), "a script's send() ended the dismissal");
        QCOMPARE(wanted.count(), 2);

        mpHost->clearPasswordEntryDismissal();
        QVERIFY2(!mpHost->passwordEntryWanted(), "the re-open happened inside the call that reported the player's line rather than one event-loop turn later");
        QTRY_VERIFY2(mpHost->passwordEntryWanted(), "the player's line did not bring the box back");
        QVERIFY2(mpHost->passwordEntryReopened(), "the box that came back does not know it follows an Esc");
        QCOMPARE(wanted.count(), 3);

        mpHost->dismissPasswordEntry();
        QVERIFY2(!mpHost->passwordEntryWanted(), "the second Esc did not hide the box");
        QCOMPARE(wanted.count(), 4);
        mpHost->clearPasswordEntryDismissal();
        QTest::qWait(50);
        QVERIFY2(!mpHost->passwordEntryWanted(), "after the second Esc a line brought the box back before the game released ECHO");
        QCOMPARE(wanted.count(), 4);

        serverSaysEcho(TN_WONT);
        QVERIFY2(!mpHost->mPasswordEntrySuppressed && !mpHost->mPasswordEntryDismissed && !mpHost->mPasswordEntryDismissedOnce, "WONT ECHO did not clear the hold's flags");
        QCOMPARE(wanted.count(), 4);

        serverSaysEcho(TN_WILL);
        QVERIFY2(mpHost->passwordEntryWanted(), "the next prompt's box was held back by the previous hold's Escs");
        QVERIFY2(!mpHost->passwordEntryReopened(), "the next hold's box thinks it follows an Esc");
        QCOMPARE(wanted.count(), 5);
    }

    // cTelnet::reset() releases ECHO while it is already off, and that must still
    // end whatever the hold left behind, or it would outlive the connection.
    // Toggles spent: 0.
    void test_releasingEchoWhileItIsOffStillClearsTheHoldFlags()
    {
        mpHost->mPasswordEntrySuppressed = true;
        mpHost->mPasswordEntryDismissed = true;
        mpHost->mPasswordEntryDismissedOnce = true;
        QVERIFY(!mpHost->isRemoteEchoingActive());

        mpHost->setRemoteEchoingActive(false);
        mpHost->setRemoteEchoingActive(false);

        QVERIFY2(!mpHost->mPasswordEntrySuppressed, "the suppression survived a release made while ECHO was already off");
        QVERIFY2(!mpHost->mPasswordEntryDismissed, "the dismissal survived a release made while ECHO was already off");
        QVERIFY2(!mpHost->mPasswordEntryDismissedOnce, "the Esc count survived a release made while ECHO was already off");
    }

    // Character-at-a-time recognition is advisory and fires on any server that
    // holds ECHO past a line with SGA offered - GoMud's rejected-password path
    // does exactly that - so it has no say over the box. Toggles spent: 2.
    void test_characterModeRecognitionChangesNothing()
    {
        QSignalSpy wanted(mpHost, &Host::signal_passwordEntryWantedChanged);
        serverSaysOption(TN_WILL, OPT_SUPPRESS_GO_AHEAD);
        QVERIFY(mpHost->mTelnet.mServerRequestedSGA);
        serverSaysEcho(TN_WILL);
        QVERIFY(mpHost->passwordEntryWanted());

        QVERIFY(mpHost->sendPasswordEntry(qsl("hunter2")));
        QVERIFY(waitForServerToReceive(asSent(qsl("hunter2"))));
        QTimer* detect = mpHost->mTelnet.mTimerCharacterModeDetect;
        QVERIFY2(detect && detect->isActive(), "the line sent from the box with ECHO and SGA up did not arm character-at-a-time detection as a command line's line would");
        QTimer* timeout = mpHost->mTelnet.mTimerPasswordModeTimeout;
        QVERIFY2(timeout && timeout->isActive(), "the line sent from the box did not start the mask safety timeout as a command line's line would");

        detect->start(0ms);
        QTRY_VERIFY(mpHost->mTelnet.mCharacterModeDetected);

        QVERIFY2(mpHost->passwordEntryWanted(), "recognition took the box down");
        QCOMPARE(wanted.count(), 1);
    }

    // While the auto-login still intends to send the stored password no box
    // opens, so a command typed ahead stays in the command line; once it has sent
    // it under the game's mask, none opens until the game releases ECHO.
    // Toggles spent: 3.
    void test_autoLoginHoldsTheBoxBackAndItsSendSuppressesTheHold()
    {
        QSignalSpy wanted(mpHost, &Host::signal_passwordEntryWantedChanged);
        mpHost->setLogin(qsl("morquin"));
        mpHost->setPass(qsl("hunter2"));
        mpHost->mTelnet.setAutoLoginPending(true);

        serverSaysEcho(TN_WILL);
        QVERIFY2(!mpHost->passwordEntryWanted(), "a box opened while the auto-login still intended to send the password");
        QCOMPARE(wanted.count(), 0);

        // The timer, not slot_send_pass() directly, so that the case leaves no
        // live timer behind for the next one
        mpHost->mTelnet.mTimerPass->start(0ms);
        QVERIFY(waitForServerToReceive(asSent(qsl("hunter2"))));
        QTRY_VERIFY2(!mpHost->mTelnet.autoLoginPending(), "the auto-login sent the password but still says it intends to");
        QVERIFY2(mpHost->mPasswordEntrySuppressed, "the password sent under the game's mask did not suppress the box for the rest of the hold");
        QVERIFY2(!mpHost->passwordEntryWanted(), "a box opened after the auto-login had answered under the mask");
        QCOMPARE(wanted.count(), 0);

        serverSaysEcho(TN_WONT);
        QVERIFY(!mpHost->mPasswordEntrySuppressed);
        serverSaysEcho(TN_WILL);
        QVERIFY2(mpHost->passwordEntryWanted(), "the suppression outlived the hold it was set in");
        QCOMPARE(wanted.count(), 1);
    }

    // With ECHO off at the send there is no WONT to end a suppression, so none is
    // set: a prompt an hour later still gets its box. Toggles spent: 2.
    void test_autoLoginSentWithEchoOffSuppressesNothing()
    {
        mpHost->setLogin(qsl("morquin"));
        mpHost->setPass(qsl("hunter2"));
        mpHost->mTelnet.setAutoLoginPending(true);

        mpHost->mTelnet.mTimerPass->start(0ms);
        QVERIFY(waitForServerToReceive(asSent(qsl("hunter2"))));
        QTRY_VERIFY(!mpHost->mTelnet.autoLoginPending());
        QVERIFY2(!mpHost->mPasswordEntrySuppressed, "a password sent with ECHO off suppressed a box nothing will ever release");

        serverSaysEcho(TN_WILL);
        QVERIFY2(mpHost->passwordEntryWanted(), "a later prompt got no box after an auto-login the game never masked");
    }

    // A keychain lookup that timed out keeps the password step armed and the box
    // held back; one that was refused ends both. Toggles spent: 2.
    void test_keychainRefusalEndsThePendingWindow()
    {
        mpHost->setLogin(qsl("morquin"));
        mpHost->mSecuredPasswordPending = true;
        QVERIFY(mpHost->hasAutoLoginCredentials());
        mpHost->mTelnet.mTimerPass->start(60000ms);
        mpHost->mTelnet.setAutoLoginPending(true);

        serverSaysEcho(TN_WILL);
        QVERIFY(!mpHost->passwordEntryWanted());

        mpHost->securedPasswordAnswered(false, QString(), qsl("Operation timed out"), true);
        QVERIFY2(mpHost->mTelnet.autoLoginPending(), "a lookup that only timed out stopped the auto-login intending to send the password it still owes");
        QVERIFY(!mpHost->passwordEntryWanted());

        mpHost->securedPasswordAnswered(false, QString(), qsl("Could not read the keychain: access denied"), false);
        QVERIFY2(!mpHost->mTelnet.autoLoginPending(), "a refused keychain left the auto-login intending to send a password it will never have");
        QVERIFY2(mpHost->passwordEntryWanted(), "a refused keychain went on holding the box back");
    }

    // The auto-login's name answers a prompt the player may have stepped past, so
    // it ends a dismissal; and having a password to send is what arms the
    // pending window. Toggles spent: 2.
    void test_theAutoLoginNameEndsADismissalAndAPasswordArmsThePendingWindow()
    {
        mpHost->setLogin(qsl("morquin"));
        serverSaysEcho(TN_WILL);
        mpHost->dismissPasswordEntry();
        QVERIFY(!mpHost->passwordEntryWanted());

        mpHost->mTelnet.slot_send_login();
        QVERIFY(waitForServerToReceive(asSent(qsl("morquin"))));
        QVERIFY2(!mpHost->mTelnet.autoLoginPending(), "with no password to send, the login step armed the password step anyway");
        QTRY_VERIFY2(mpHost->passwordEntryWanted(), "the auto-login's name did not end the dismissal");

        mpHost->setPass(qsl("hunter2"));
        mpHost->mTelnet.slot_send_login();
        QVERIFY2(mpHost->mTelnet.autoLoginPending(), "with a password to send, the login step did not arm the password step");
        QVERIFY2(mpHost->mTelnet.mTimerPass->isActive(), "the login step said the password step was armed without starting its timer");
        QVERIFY2(!mpHost->passwordEntryWanted(), "a box stayed open while the auto-login intended to send the password");
    }

    // The box's line goes straight to the wire: no sysDataSendRequest, no alias
    // pass, no `command` global, no split on the command separator, and a stale
    // denyCurrentSend() cannot drop it. Toggles spent: 0.
    void test_sendPasswordEntryGoesStraightToTheWire()
    {
        QVERIFY(runLua(qsl("sawSendRequest = ''\n"
                           "aliasFired = ''\n"
                           "command = 'sentinel'\n"
                           "policyHandlerId = registerAnonymousEventHandler('sysDataSendRequest', function(_, cmd) sawSendRequest = cmd end)\n"
                           "policyAliasId = tempAlias('^x', [[aliasFired = 'yes']])\n"
                           "denyCurrentSend()\n")));
        const auto tidy = qScopeGuard([this]() {
            runLua(qsl("killAnonymousEventHandler(policyHandlerId)\nkillAlias(policyAliasId)\n"));
        });
        mpHost->mTelnet.mTimerLogin->start(60000ms);

        QVERIFY2(mpHost->sendPasswordEntry(qsl("x;;y")), "the box's line was not written to the server");
        QVERIFY2(waitForServerToReceive(asSent(qsl("x;;y"))), "the box's line did not reach the server whole");
        QCOMPARE(luaGlobal("sawSendRequest"), QString());
        QCOMPARE(luaGlobal("aliasFired"), QString());
        QCOMPARE(luaGlobal("command"), qsl("sentinel"));
        QVERIFY2(!mpHost->mTelnet.mTimerLogin->isActive(), "the player's answer did not cancel the auto-login");

        // The hooks the box's line skipped do fire for a command line's line -
        // the separator splits it, the alias swallows the "x" half and the event
        // sees the "y" half - so their silence above was not for want of being
        // wired up
        mpHost->send(qsl("x;;y"));
        QVERIFY(waitForServerToReceive(asSent(qsl("y"))));
        QCOMPARE(luaGlobal("sawSendRequest"), qsl("y"));
        QCOMPARE(luaGlobal("aliasFired"), qsl("yes"));

        mpServer->forgetReceived();
        QVERIFY(mpHost->sendPasswordEntry(QString()));
        QVERIFY2(waitForServerToReceive(asSent(QString())), "an empty box did not send an empty line");
    }

    // A line the game is unlikely to understand is quoted in the warning - unless
    // it was hidden input, from the box or the auto-login. Toggles spent: 0.
    void test_theEncodingWarningNeverQuotesHiddenInput()
    {
        mpHost->mTelnet.setEncoding(QByteArrayLiteral("ASCII"), false);
        const auto restoreEncoding = qScopeGuard([this]() {
            mpHost->mTelnet.setEncoding(mOriginalEncoding, false);
        });
        const QString before = consoleText();

        mpHost->mTelnet.mEncodingWarningIssued = false;
        QVERIFY(mpHost->sendPasswordEntry(qsl("pässwörd")));
        QTRY_VERIFY2(consoleText().mid(before.length()).contains(qsl("hidden input")), "no warning was posted for a line the box sent that ASCII cannot carry");
        QVERIFY2(!consoleText().contains(qsl("pässwörd")), "the warning quoted the password typed into the box");

        mpHost->mTelnet.mEncodingWarningIssued = false;
        mpHost->setLogin(qsl("morquin"));
        mpHost->setPass(qsl("gëheim"));
        mpHost->mTelnet.setAutoLoginPending(true);
        mpHost->mTelnet.mTimerPass->start(0ms);
        QVERIFY(waitForServerToReceive(QByteArrayLiteral("g")));
        QTRY_COMPARE(consoleText().mid(before.length()).count(qsl("hidden input")), 2);
        QVERIFY2(!consoleText().contains(qsl("gëheim")), "the warning quoted the auto-login password");

        // The quote is still there for a command line's line, so the two above
        // were withheld rather than never made
        mpHost->mTelnet.mEncodingWarningIssued = false;
        mpHost->send(qsl("ünsafe"));
        QTRY_VERIFY2(consoleText().contains(qsl("ünsafe")), "the warning for an ordinary line no longer quotes it");
    }

    // The policy is the core's contract for hidden input: it has to answer, and
    // announce, with no view at all. Last, because taking the view down also
    // takes the connection with it. Toggles spent: 0.
    void test_answersWithNoView()
    {
        QPointer<TMainConsole> console = mpHost->mpConsole;
        mpHost->forceClose();
        QVERIFY2(mpHost->requestClose(), "Closing the profile was refused");
        QTRY_VERIFY2(console.isNull(), "The main console view was not destroyed by closing the profile");
        QVERIFY(mpHost->mpConsole.isNull());
        QVERIFY2(!mpHost->passwordEntryWanted(), "closing the connection left a box wanted");

        QSignalSpy wanted(mpHost, &Host::signal_passwordEntryWantedChanged);
        mpHost->setRemoteEchoingActive(true);
        QVERIFY2(mpHost->passwordEntryWanted(), "with no view, ECHO no longer asks for a box");
        QCOMPARE(wanted.count(), 1);

        mpHost->dismissPasswordEntry();
        QVERIFY(!mpHost->passwordEntryWanted());
        mpHost->clearPasswordEntryDismissal();
        QTRY_VERIFY2(mpHost->passwordEntryWanted(), "the deferred recompute did not run into no receiver");
        QCOMPARE(wanted.count(), 3);

        // Nothing to write to, so false - but never a crash
        QVERIFY(!mpHost->sendPasswordEntry(qsl("x")));
        mpHost->send(qsl("y"));

        mpHost->setRemoteEchoingActive(false);
        QVERIFY(!mpHost->passwordEntryWanted());
        QCOMPARE(wanted.count(), 4);
    }
};

#include "PasswordEntryPolicyTest.moc"
MUDLET_GROUPED_TEST_MAIN(PasswordEntryPolicyTest)
