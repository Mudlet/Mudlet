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
 * A password that only arrives after the timer-driven auto-login has passed its
 * password step - what an unanswered keychain prompt produces, since the profile
 * now loads and connects without waiting for the answer.
 *
 * cTelnet::sendOutstandingAutoLoginPassword() types it for the player, but only
 * while the game is provably still waiting at that prompt: sent a moment too
 * late it appears unmasked on screen and reaches the game as a command. So a
 * stub game drives a real profile through the login and the cases below place
 * the password against each state the game can be in when it turns up.
 *
 * Run with: ctest -R TelnetLatePasswordTest -V
 */

#include <QPointer>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <chrono>
#include <memory>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include <qtkeychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif

#include "AutoLoginDelaysTestHelper.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "CredentialManager.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
const QByteArray csLoginLine = QByteArrayLiteral("player\r\n");
const QByteArray csPasswordLine = QByteArrayLiteral("secret\r\n");
const QByteArray csCommandLine = QByteArrayLiteral("look\r\n");
const QByteArray csPasswordPrompt = QByteArrayLiteral("Password:\r\n");
// IAC WILL ECHO / IAC WONT ECHO - a game masking input, and releasing it again.
const QByteArray csMaskOn = QByteArrayLiteral("\xff\xfb\x01");
const QByteArray csMaskOff = QByteArrayLiteral("\xff\xfc\x01");
// IAC WILL SGA - with ECHO already on, what a character-at-a-time server negotiates up front.
const QByteArray csSgaOn = QByteArrayLiteral("\xff\xfb\x03");
} // namespace

// A game that says nothing unless the test tells it to, and records what the client typed. Only
// the text matters here - the client's telnet negotiation is parsed off so that an IAC DO ECHO
// answering the mask cannot be mistaken for input.
class PasswordPromptServerStub : public QObject
{
    Q_OBJECT

public:
    explicit PasswordPromptServerStub(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &PasswordPromptServerStub::onNewConnection);
    }

    // An ephemeral port, so concurrent test runs cannot collide on a fixed one.
    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }
    QByteArray receivedText() const { return mReceivedText; }
    // Sent the moment a client connects, before it has typed anything - the way a game that
    // negotiates its options up front greets a connection.
    void setGreeting(const QByteArray& greeting) { mGreeting = greeting; }

    void sendRaw(const QByteArray& data)
    {
        if (!mClient) {
            return;
        }
        mClient->write(data);
        mClient->flush();
    }

private slots:
    void onNewConnection()
    {
        mClient = mServer.nextPendingConnection();
        if (!mClient) {
            return;
        }
        connect(mClient, &QTcpSocket::readyRead, this, &PasswordPromptServerStub::onReadyRead);
        if (!mGreeting.isEmpty()) {
            mClient->write(mGreeting);
            mClient->flush();
        }
        connect(mClient, &QTcpSocket::disconnected, mClient, &QObject::deleteLater);
    }

    void onReadyRead()
    {
        mBuffer.append(mClient->readAll());
        parseBuffer();
    }

private:
    // The IAC that begins an IAC SE, honouring an escaped IAC IAC inside the subnegotiation.
    int findSubnegotiationEnd(int from) const
    {
        int j = from;
        while (j + 1 < mBuffer.size()) {
            if (static_cast<unsigned char>(mBuffer.at(j)) == static_cast<unsigned char>(TN_IAC)) {
                const unsigned char next = static_cast<unsigned char>(mBuffer.at(j + 1));
                if (next == static_cast<unsigned char>(TN_IAC)) {
                    j += 2;
                    continue;
                }
                if (next == static_cast<unsigned char>(TN_SE)) {
                    return j;
                }
            }
            ++j;
        }
        return -1;
    }

    void parseBuffer()
    {
        int i = 0;
        while (i < mBuffer.size()) {
            if (static_cast<unsigned char>(mBuffer.at(i)) != static_cast<unsigned char>(TN_IAC)) {
                mReceivedText.append(mBuffer.at(i));
                ++i;
                continue;
            }
            if (i + 1 >= mBuffer.size()) {
                break; // incomplete
            }
            const unsigned char cmd = static_cast<unsigned char>(mBuffer.at(i + 1));
            if (cmd == static_cast<unsigned char>(TN_IAC)) {
                i += 2; // escaped literal 255 in the main stream
                continue;
            }
            if (cmd == static_cast<unsigned char>(TN_WILL) || cmd == static_cast<unsigned char>(TN_WONT) || cmd == static_cast<unsigned char>(TN_DO) || cmd == static_cast<unsigned char>(TN_DONT)) {
                if (i + 2 >= mBuffer.size()) {
                    break; // incomplete
                }
                i += 3;
                continue;
            }
            if (cmd == static_cast<unsigned char>(TN_SB)) {
                const int end = findSubnegotiationEnd(i + 2);
                if (end == -1) {
                    break; // incomplete subnegotiation
                }
                i = end + 2;
                continue;
            }
            i += 2; // any other 2-byte IAC command
        }
        mBuffer = mBuffer.mid(i);
    }

    QTcpServer mServer;
    QPointer<QTcpSocket> mClient;
    QByteArray mBuffer;
    QByteArray mReceivedText;
    QByteArray mGreeting;
};

class TelnetLatePasswordTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    PasswordPromptServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-LatePassword");
    QString mPort; // assigned the stub's actual loopback port in init()

    // Long enough that the login is set on the loaded profile before the username timer reads it,
    // short enough that four cases do not dominate the group's run time.
    static constexpr int csUsernameDelayMs = 800;
    static constexpr int csPasswordDelayMs = 1200;

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own: sharing the developer's ~/.config/mudlet means
        // sharing a profile list, so a second copy of this test running at the same time is told
        // the name it types is already in use and never gets an enabled Connect button.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        // Belt and braces alongside setStorePasswordsSecurely(false) below: no path this test
        // reaches may put a keychain prompt on the developer's screen.
        qputenv("MUDLET_TEST_MODE", "1");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new PasswordPromptServerStub(qApp);
        QVERIFY2(mpServer->start(), "PasswordPromptServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // ---- The game is still at its password prompt --------------------------

    // P1: the server is still masking input, so the prompt it printed is still the one waiting.
    void testLatePasswordIsSentWhileTheServerStillMasksInput()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        deliverLatePassword(host);
        QVERIFY2(waitForReceivedText(csLoginLine + csPasswordLine), "the late password was not sent to the still-masked prompt");
        QVERIFY2(host->mTelnet.mTimerPasswordModeTimeout && host->mTelnet.mTimerPasswordModeTimeout->isActive(),
                 "the late password did not start the safety timeout against a game that never releases the mask, as the auto-login password does");
    }

    // P2: the mask is still on, and a command the player's own package turned down never reached
    // the prompt - so nothing has taken it over and the password is still what the game wants.
    void testADeniedCommandLeavesTheLatePasswordArmed()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        QVERIFY2(host->mLuaInterpreter.compileAndExecuteScript(qsl("registerAnonymousEventHandler(\"sysDataSendRequest\", function() denyCurrentSend() end)")),
                 "the denying event handler could not be registered, so this test cannot cover a denied command");
        host->send(qsl("look"));
        // Nothing to wait for, so give a send that should not happen the same margin as one that
        // should before concluding it did not:
        QTest::qWait(500);
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        deliverLatePassword(host);
        QVERIFY2(waitForReceivedText(csLoginLine + csPasswordLine), "a command the game never saw disarmed the late password");
    }

    // P3: a line-mode game that masks its login prompt as well as its password prompt has ECHO
    // and SGA on from its first byte, which is also what a character-at-a-time server looks like.
    // The login line going out under that mask must not start the character-at-a-time verdict:
    // nothing withdrew the mask, and the game is sitting at its password prompt.
    void testLatePasswordIsSentToAGameThatMasksItsLoginPromptToo()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        mpServer->setGreeting(csMaskOn + csSgaOn + QByteArrayLiteral("Name: "));
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);
        QVERIFY2(host->isRemoteEchoingActive(), "the greeting's WILL ECHO did not put the client into masking mode");
        QVERIFY2(host->mTelnet.mServerRequestedSGA, "the greeting's WILL SGA was not recorded");

        mpServer->sendRaw(csPasswordPrompt);
        QVERIFY2(waitForConsoleContains(host, qsl("Password:")), "the prompt never reached the client");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        // Past the detection window (CHARACTER_MODE_DETECT, 3 s), so a verdict the login line had
        // started would have latched by now and refused the password below
        QTest::qWait(3500);

        deliverLatePassword(host);
        QVERIFY2(waitForReceivedText(csLoginLine + csPasswordLine), "the late password was not sent to the still-masked password prompt");
    }

    // ---- Nothing proves the prompt is still waiting ------------------------

    // N0: no ECHO negotiation at all, so there is no proof the prompt on screen is still the
    // password one - "the server has said nothing since" would fit any question it had asked.
    void testLatePasswordIsNotSentAtAQuietPrompt()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt);
        QVERIFY2(waitForConsoleContains(host, qsl("Password:")), "the prompt never reached the client");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);
        QVERIFY(!host->isRemoteEchoingActive());

        deliverLatePassword(host);
        QVERIFY2(waitForConsoleContains(host, qsl("too late for the automatic login")), "the player was not told why the password was not sent");
        QCOMPARE(mpServer->receivedText(), csLoginLine);
    }

    // N1: the mask was released and the game printed something else, so whatever is reading input
    // now is not the password prompt.
    void testLatePasswordIsNotSentAfterTheGameMovedOn()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        mpServer->sendRaw(csMaskOff + QByteArrayLiteral("Timed out.\r\n"));
        QVERIFY2(waitForMasking(host, false), "the client never left password-masking mode");

        deliverLatePassword(host);
        QVERIFY2(waitForConsoleContains(host, qsl("too late for the automatic login")), "the player was not told why the password was not sent");
        QCOMPARE(mpServer->receivedText(), csLoginLine);
    }

    // N6: the mask was released and later put up again. The prompt the password step found is
    // over, and whatever masked question the game is asking now is a different one - a mask is
    // only proof of the prompt it was on for.
    void testLatePasswordIsNotSentToAMaskTheGamePutUpAgain()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        mpServer->sendRaw(csMaskOff + QByteArrayLiteral("Timed out.\r\n"));
        QVERIFY2(waitForMasking(host, false), "the client never left password-masking mode");
        mpServer->sendRaw(QByteArrayLiteral("Delete this character? Enter its password to confirm:\r\n") + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client did not enter password-masking mode for the second prompt");

        deliverLatePassword(host);
        QVERIFY2(waitForConsoleContains(host, qsl("too late for the automatic login")), "the player was not told why the password was not sent");
        QCOMPARE(mpServer->receivedText(), csLoginLine);
    }

    // N2: the player typed at the prompt themselves, so it is their input the game is answering
    // and not a password Mudlet still owes it.
    void testLatePasswordIsNotSentAfterTheUserTookOver()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        host->send(qsl("look"));
        QVERIFY2(waitForReceivedText(csLoginLine + csCommandLine), "the typed command never reached the game");

        deliverLatePassword(host);
        // Nothing to wait for, so give a send that should not happen the same margin as one that
        // should before concluding it did not:
        QTest::qWait(500);
        QCOMPARE(mpServer->receivedText(), csLoginLine + csCommandLine);
    }

    // N3: GMCP Char.Login took the login over, which cancels the login timers - the auto-login has
    // no prompt of its own left, so the password goes nowhere and there is nothing to explain.
    void testLatePasswordIsNotSentAfterTheLoginTimersWereCancelled()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword();
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        QVERIFY2(waitForThePasswordStep(host), "the auto-login never reached its password step");
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        host->mTelnet.cancelLoginTimers();

        deliverLatePassword(host);
        QTest::qWait(500);
        QCOMPARE(mpServer->receivedText(), csLoginLine);
        QVERIFY2(!consoleContains(host, qsl("too late for the automatic login")), "a login Mudlet is not driving was told about a password it never owed");
    }

    // N4: a profile with a login and no password at all - nothing is on its way, so the password
    // step is never armed and a password set by hand later has no claim on the prompt.
    void testALoginOnlyProfileNeverArmsThePasswordStep()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword(KeychainAnswer::NothingStored);
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        waitOutThePasswordStep();
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        deliverLatePassword(host);
        QTest::qWait(500);
        QCOMPARE(mpServer->receivedText(), csLoginLine);
        QVERIFY2(!consoleContains(host, qsl("too late for the automatic login")), "a profile that never had a password waiting was told one arrived too late");
    }

    // N7: the keychain did answer after the lookup gave up, but without the password - the player
    // denied the prompt. That answer is as final as any other, so the password step is not armed
    // for a password that is no longer on its way.
    void testALateRefusalLeavesThePasswordStepUnarmed()
    {
        const ScopedAutoLoginDelays delays(csUsernameDelayMs, csPasswordDelayMs);
        Host* host = connectWithLoginAndNoPassword(KeychainAnswer::TimedOutThenRefused);
        QVERIFY(host);

        mpServer->sendRaw(csPasswordPrompt + csMaskOn);
        QVERIFY2(waitForMasking(host, true), "the client never entered password-masking mode");
        waitOutThePasswordStep();
        QCOMPARE(mpServer->receivedText(), csLoginLine);

        deliverLatePassword(host);
        QTest::qWait(500);
        QCOMPARE(mpServer->receivedText(), csLoginLine);
    }

    // ---- The profile's own keychain lookup ---------------------------------

    // K1: Host::loadSecuredPassword()'s lookup, from its start to its late answer, against a
    // keychain that answers only when the test says so. A password is on its way - so the login
    // step arms the password step - while the lookup is out and after its deadline, until the
    // keychain answers; a refusal then is final.
    void testTheProfilesLookupKeepsAPasswordOnItsWayUntilTheKeychainAnswers()
    {
        Host* host = TestProfile::create(mHostname, qsl("localhost"), mPort, 20s);
        QVERIFY(host);
        host->setLogin(qsl("player"));
        host->setPass(QString());

        auto reads = std::make_shared<QList<QPointer<QKeychain::Job>>>();
        QPointer<CredentialManager> manager = new CredentialManager(host);
        manager->mOperationTimeoutMs = 300;
        // Never started, so nothing reaches the real keychain: the test answers each read itself
        manager->mJobStartHook = [reads](QKeychain::Job* job) {
            reads->append(job);
            return false;
        };
        // The keychain path is the one under test, and two things route CredentialManager away from
        // it: MUDLET_TEST_MODE, and - since saved sign-ins began following the "Store passwords in"
        // preference (#11029) - the preference this class turns off in init() so that no case can
        // reach the real keychain by accident. Both are read as the lookup starts, so both go back
        // immediately afterwards; the job hook above is what actually keeps the reads away from the
        // machine's own store.
        qunsetenv("MUDLET_TEST_MODE");
        mudlet::self()->setStorePasswordsSecurely(true);
        host->lookUpSecuredPassword(manager);
        mudlet::self()->setStorePasswordsSecurely(false);
        qputenv("MUDLET_TEST_MODE", "1");

        QVERIFY2(!reads->isEmpty(), "the lookup never read the keychain, so this test cannot cover it");
        QVERIFY2(host->hasAutoLoginCredentials(), "a password on its way from the keychain does not arm the password step");

        // The lookup gives up at its deadline and the profile deletes its manager
        QVERIFY2(QTest::qWaitFor(
                         [&manager]() {
                             return manager.isNull();
                         },
                         5000),
                 "the lookup never answered at its deadline");
        QVERIFY2(host->hasAutoLoginCredentials(), "a lookup that timed out stopped the password step waiting for the answer it still owes");

        QPointer<QKeychain::Job> read = reads->constFirst();
        QVERIFY2(read, "the read the lookup gave up on was deleted before the keychain answered it");
        read->emitFinishedWithError(QKeychain::AccessDenied, qsl("synthetic: the prompt was denied"));
        QVERIFY2(!host->hasAutoLoginCredentials(), "the keychain refusing the password late left the password step waiting for it");
    }

private:
    // How the profile's keychain lookup answered, which is what decides whether the auto-login
    // arms its password step for a password still on its way
    enum class KeychainAnswer {
        // The lookup gave up waiting on the keychain, which may still answer
        TimedOut,
        // ... and the keychain then answered without the password
        TimedOutThenRefused,
        // Nothing is stored anywhere
        NothingStored,
    };

    // Drives the dialog to create and connect the profile, then hands it a login, and answers its
    // keychain lookup as answer says - before the username timer reads the result. Returns with
    // the game's stub holding the login line, which is where each case takes over.
    Host* connectWithLoginAndNoPassword(KeychainAnswer answer = KeychainAnswer::TimedOut)
    {
        Host* host = TestProfile::create(mHostname, qsl("localhost"), mPort, 20s);
        if (!host) {
            qWarning("No active host");
            return nullptr;
        }
        host->setLogin(qsl("player"));
        host->setPass(QString());
        switch (answer) {
        case KeychainAnswer::TimedOut:
            host->securedPasswordAnswered(false, QString(), qsl("Operation timed out"), true);
            break;
        case KeychainAnswer::TimedOutThenRefused:
            host->securedPasswordAnswered(false, QString(), qsl("Operation timed out"), true);
            host->securedPasswordAnswered(false, QString(), qsl("Could not read the keychain: access denied"), false);
            break;
        case KeychainAnswer::NothingStored:
            host->securedPasswordAnswered(false, QString(), qsl("No stored credentials found for profile %1").arg(mHostname), false);
            break;
        }

        if (!waitForReceivedText(csLoginLine, 15000)) {
            qWarning() << "The auto-login never sent the login line - the stub has:" << mpServer->receivedText();
            return nullptr;
        }
        return host;
    }

    // With no password in hand the password step marks one as owed, which is what the late
    // password is checked against. Waiting for the mark rather than for the timer keeps a slow run
    // from delivering the password first, where the password step would then send it the usual way.
    static bool waitForThePasswordStep(Host* host)
    {
        return QTest::qWaitFor(
                [host]() {
                    return host->mTelnet.mAutoLoginPasswordOutstanding;
                },
                8000);
    }

    // A password step that was never armed leaves no mark to wait for, so the cases about one wait
    // out its timer instead: long enough for it to have fired, had it been armed.
    static void waitOutThePasswordStep() { QTest::qWait(csPasswordDelayMs + 500); }

    bool waitForReceivedText(const QByteArray& expected, int timeoutMs = 4000)
    {
        return QTest::qWaitFor(
                [this, &expected]() {
                    return mpServer->receivedText() == expected;
                },
                timeoutMs);
    }

    // Whether the game has input masked, which is the client's own record of a password prompt
    // being open - and the state sendOutstandingAutoLoginPassword() reads.
    static bool waitForMasking(Host* host, bool masked, int timeoutMs = 8000)
    {
        return QTest::qWaitFor(
                [host, masked]() {
                    return host->isRemoteEchoingActive() == masked;
                },
                timeoutMs);
    }

    // What the keychain lookup's late answer hands the profile once the prompt is answered
    static void deliverLatePassword(Host* host) { host->securedPasswordAnswered(true, qsl("secret"), QString(), false); }

    // The console wraps a printed line at its width, so the text is stitched back together and its
    // whitespace normalised before matching - a phrase must not stop being found because the line
    // it sits on happened to be broken part way through it.
    static bool consoleContains(Host* host, const QString& substring)
    {
        if (!host || !host->mpConsole) {
            return false;
        }
        auto& buffer = host->mpConsole->buffer;
        QString all;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            all.append(buffer.line(i));
            all.append(QChar::Space);
        }
        return all.simplified().contains(substring);
    }

    static bool waitForConsoleContains(Host* host, const QString& substring, int timeoutMs = 4000)
    {
        return QTest::qWaitFor(
                [host, &substring]() {
                    return consoleContains(host, substring);
                },
                timeoutMs);
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = MudletApp::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "TelnetLatePasswordTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetLatePasswordTest)
