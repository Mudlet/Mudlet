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

// NEW_ENVIRON (RFC 1572) and its MNES profile
// (https://tintin.mudhalla.net/protocols/mnes/) are the out-of-band protocol
// where nearly all the behaviour is in what Mudlet *sends*: a game asks for variables
// and every answer, every "I do not maintain that one", and every later INFO
// update goes out on the wire, where nothing inside the client can see it. So
// these tests read the bytes off a recording server rather than off Mudlet, and
// assert on the reply a game would actually parse.
//
// Requests arrive by loopback so they are processed before the call returns; the
// replies are real socket writes, waited out by asking for a DO TIMING_MARK
// afterwards. Mudlet always answers that with WONT TIMING_MARK, so a test that
// expects no reply at all has a deterministic marker to wait for rather than a
// fixed sleep that would pass on a slow runner by accident.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// One NEW_ENVIRON variable as it appears in an IS or INFO reply. RFC 1572 makes
// "defined but empty" and "not defined at all" different things - a VAL byte
// with nothing after it against no VAL byte - so hasValue is not the same
// question as value.isEmpty().
struct EnvironVariable
{
    char type = NEW_ENVIRON_VAR;
    QByteArray name;
    bool hasValue = false;
    QByteArray value;
};

// The variables in an IS or INFO payload, the leading command byte already
// dropped by the caller. NEW_ENVIRON_ESC quotes the byte that follows it, which
// is how a name or value gets to carry one of the protocol's own delimiters.
static QList<EnvironVariable> variablesIn(const QByteArray& payload)
{
    QList<EnvironVariable> found;
    int i = 0;
    while (i < payload.size()) {
        const char type = payload.at(i);
        if (type != NEW_ENVIRON_VAR && type != NEW_ENVIRON_USERVAR) {
            ++i;
            continue;
        }
        EnvironVariable variable;
        variable.type = type;
        ++i;
        bool readingValue = false;
        while (i < payload.size()) {
            const char byte = payload.at(i);
            if (byte == NEW_ENVIRON_ESC && i + 1 < payload.size()) {
                (readingValue ? variable.value : variable.name).append(payload.at(i + 1));
                i += 2;
                continue;
            }
            if (byte == NEW_ENVIRON_VAR || byte == NEW_ENVIRON_USERVAR) {
                break;
            }
            if (byte == NEW_ENVIRON_VAL && !readingValue) {
                readingValue = true;
                variable.hasValue = true;
                ++i;
                continue;
            }
            (readingValue ? variable.value : variable.name).append(byte);
            ++i;
        }
        found.append(variable);
    }
    return found;
}

class TelnetNewEnvironTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-New-Environ");
    const QString mLocalhost = qsl("localhost");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    static QByteArray subnegotiation(const char option, const QByteArray& payload)
    {
        QByteArray data;
        data.append(TN_IAC).append(TN_SB).append(option).append(payload).append(TN_IAC).append(TN_SE);
        return data;
    }

    void feed(const QByteArray& data)
    {
        QByteArray copy = data;
        mpHost->mTelnet.loopbackTest(copy);
    }

    void announce(const char command, const char option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(option);
        feed(data);
    }

    // Waits out whatever the request under test produced. Mudlet answers a DO
    // TIMING_MARK with WONT TIMING_MARK whatever else is going on (RFC 860), so
    // that answer arriving means every earlier reply is already recorded -
    // including a reply of nothing at all, which is why no test here sleeps.
    QByteArray sentBytes()
    {
        announce(TN_DO, OPT_TIMING_MARK);
        QByteArray marker;
        marker.append(TN_IAC).append(TN_WONT).append(OPT_TIMING_MARK);
        const bool arrived = QTest::qWaitFor(
                [this, &marker]() {
                    return mpServer->received().contains(marker);
                },
                10000);
        if (!arrived) {
            // Mudlet answers a DO TIMING_MARK whatever else is going on, so this
            // is never a slow reply - the connection is gone and every "nothing
            // was sent" below would pass for the wrong reason.
            QTest::qFail("the telnet marker never came back, so the capture is worthless", __FILE__, __LINE__);
            return {};
        }
        const QByteArray stream = mpServer->received();
        // Cleared here rather than by the caller: a second call would otherwise
        // match the first call's marker, skip its wait and hand back the earlier
        // request's bytes.
        mpServer->forgetReceived();
        // The marker belongs to the wait, not to the request under test.
        const int markerAt = stream.indexOf(marker);
        return markerAt >= 0 ? stream.left(markerAt) : stream;
    }

    // The NEW_ENVIRON replies in what Mudlet has sent since the last clear.
    QList<Subnegotiation> newEnvironReplies() { return subnegotiationsFor(sentBytes(), static_cast<unsigned char>(OPT_NEW_ENVIRON)); }


    // Turns the protocol on the way a game does, and clears the recorder. The DO
    // that answers it can still land afterwards, which is why the assertions
    // below filter to NEW_ENVIRON subnegotiations rather than reading the raw
    // stream.
    void enableNewEnviron()
    {
        announce(TN_WILL, OPT_NEW_ENVIRON);
        QVERIFY2(mpHost->mTelnet.isNewEnvironEnabled(), "NEW_ENVIRON did not turn on, so nothing below would be answered");
        mpServer->forgetReceived();
    }

    void requestSend(const QByteArray& body) { feed(subnegotiation(OPT_NEW_ENVIRON, QByteArray(1, NEW_ENVIRON_SEND) + body)); }

    // A fresh connection, which is the one thing that clears the record of what
    // this game has asked for so far - it outlives a test method otherwise.
    void reconnect()
    {
        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        mpHost->mTelnet.disconnectIt();
        mpHost->mTelnet.reconnect();
        QVERIFY2(connected.wait(15s), "the profile did not come back after a reconnect");
        mpServer->forgetReceived();
    }

    static QByteArray named(const char type, const QByteArray& name) { return QByteArray(1, type) + name; }

    // The variables of the one reply a test expects, or an empty list and a
    // description of what was captured instead - QCOMPARE cannot be used in a
    // helper that returns a value, so the caller checks 'problem' first and gets
    // a failure message that says what went wrong rather than "size 0 != 1".
    QList<EnvironVariable> soleReplyVariables(const char expectedCommand, QString& problem)
    {
        problem.clear();
        const QList<Subnegotiation> replies = newEnvironReplies();
        if (replies.size() != 1) {
            problem = qsl("expected exactly one NEW_ENVIRON reply, captured %1").arg(replies.size());
            return {};
        }
        const QByteArray payload = replies.first().payload;
        if (payload.isEmpty()) {
            problem = qsl("the reply carried no command byte at all");
            return {};
        }
        if (payload.at(0) != expectedCommand) {
            problem = qsl("the reply's command byte was %1, expected %2").arg(QString::number(payload.at(0)), QString::number(expectedCommand));
            return {};
        }
        return variablesIn(payload.mid(1));
    }

    // How many hyperlink capabilities are on offer, counted off the advertised
    // set rather than hard-coded, so one added later does not need this updating.
    int advertisedHyperlinkVariables() const
    {
        const QMap<QString, QPair<bool, QString>> newEnvironDataMap = mpHost->mTelnet.getNewEnvironDataMap();
        int advertised = 0;
        for (auto it = newEnvironDataMap.cbegin(); it != newEnvironDataMap.cend(); ++it) {
            if (it.key().startsWith(qsl("OSC_HYPERLINKS"))) {
                ++advertised;
            }
        }
        return advertised;
    }

    static EnvironVariable variableNamed(const QList<EnvironVariable>& variables, const QByteArray& name)
    {
        for (const EnvironVariable& variable : variables) {
            if (variable.name == name) {
                return variable;
            }
        }
        return EnvironVariable{};
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

        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
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

    // Every test starts from the preferences a fresh profile has, so one that
    // turns MNES (or the protocol itself) off cannot decide what the next sees.
    void init()
    {
        QVERIFY(mpHost);
        mpHost->mEnableNEWENVIRON = true;
        mpHost->mEnableMNES = false;
        mpHost->mAdvertiseScreenReader = false;
        mpHost->mEnableOSC8Hyperlinks = true;
        mpServer->forgetReceived();
    }

    // The negotiation itself: an offer is taken up, and taken up only if the
    // player left the protocol on.
    void test_serverOfferIsAnsweredAccordingToThePreference()
    {
        announce(TN_WILL, OPT_NEW_ENVIRON);
        QVERIFY2(mpHost->mTelnet.isNewEnvironEnabled(), "an offered NEW_ENVIRON was not turned on");
        QByteArray expected;
        expected.append(TN_IAC).append(TN_DO).append(OPT_NEW_ENVIRON);
        QVERIFY2(sentBytes().contains(expected), "Mudlet did not answer WILL NEW-ENVIRON with DO");

        mpServer->forgetReceived();
        mpHost->mEnableNEWENVIRON = false;
        announce(TN_WILL, OPT_NEW_ENVIRON);
        QVERIFY2(!mpHost->mTelnet.isNewEnvironEnabled(), "NEW_ENVIRON stayed on after the player turned it off");
        expected.clear();
        expected.append(TN_IAC).append(TN_DONT).append(OPT_NEW_ENVIRON);
        QVERIFY2(sentBytes().contains(expected), "Mudlet did not refuse NEW-ENVIRON with DONT when the preference was off");
    }

    // A bare SEND asks for everything Mudlet is prepared to say. All of it is
    // sent as USERVAR, because the two RFC 1572 well-known names (USER and
    // SYSTEMTYPE) are deliberately not advertised without an opt-in.
    void test_bareSendReturnsEveryVariable()
    {
        enableNewEnviron();
        requestSend({});

        const QList<Subnegotiation> replies = newEnvironReplies();
        QCOMPARE(replies.size(), 1);
        QCOMPARE(replies.first().payload.at(0), NEW_ENVIRON_IS);

        const QList<EnvironVariable> variables = variablesIn(replies.first().payload.mid(1));
        // Exactly the advertised set, so a variable dropped from the reply is a
        // failure rather than a number that still clears a loose floor.
        QCOMPARE(variables.size(), mpHost->mTelnet.getNewEnvironDataMap().size());
        QVERIFY2(variables.size() > 20, qPrintable(qsl("only %1 variables are advertised at all, so the comparison above proves little").arg(variables.size())));

        for (const EnvironVariable& variable : variables) {
            QVERIFY2(variable.type == NEW_ENVIRON_USERVAR, qPrintable(qsl("%1 was sent as a well-known VAR, which needs an opt-in").arg(QString::fromLatin1(variable.name))));
        }

        QCOMPARE(variableNamed(variables, "CLIENT_NAME").value, QByteArray("MUDLET"));
        QCOMPARE(variableNamed(variables, "CHARSET").value, QByteArray("UTF-8"));
        QCOMPARE(variableNamed(variables, "TERMINAL_TYPE").value, QByteArray("ANSI-TRUECOLOR"));
        QCOMPARE(variableNamed(variables, "ANSI").value, QByteArray("1"));
        QCOMPARE(variableNamed(variables, "VT100").value, QByteArray("0"));
        QCOMPARE(variableNamed(variables, "WORD_WRAP").value, QByteArray::number(mpHost->mWrapAt));
        QVERIFY2(!variableNamed(variables, "CLIENT_VERSION").value.isEmpty(), "CLIENT_VERSION came back without a version");
    }

    // Naming one variable gets that one and nothing else, which is what a game
    // polling a single value relies on.
    void test_namedUservarIsAnsweredOnItsOwn()
    {
        enableNewEnviron();
        requestSend(named(NEW_ENVIRON_USERVAR, "CHARSET"));

        QString problem;
        const QList<EnvironVariable> variables = soleReplyVariables(NEW_ENVIRON_IS, problem);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        QCOMPARE(variables.size(), 1);
        const EnvironVariable variable = variables.first();
        QCOMPARE(variable.type, NEW_ENVIRON_USERVAR);
        QCOMPARE(variable.name, QByteArray("CHARSET"));
        QVERIFY(variable.hasValue);
        QCOMPARE(variable.value, QByteArray("UTF-8"));
    }

    // RFC 1572 keeps the two namespaces apart: asked for a value Mudlet keeps as
    // a USERVAR under the well-known VAR type, the answer has to be "not defined
    // there" - a name with no VALUE after it - rather than the value.
    void test_uservarAskedForAsAWellKnownVariableComesBackUndefined()
    {
        enableNewEnviron();
        requestSend(named(NEW_ENVIRON_VAR, "CHARSET"));

        QString problem;
        const QList<EnvironVariable> variables = soleReplyVariables(NEW_ENVIRON_IS, problem);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        QCOMPARE(variables.size(), 1);
        const EnvironVariable variable = variables.first();
        QCOMPARE(variable.type, NEW_ENVIRON_VAR);
        QCOMPARE(variable.name, QByteArray("CHARSET"));
        QVERIFY2(!variable.hasValue, "a USERVAR was answered under the VAR type instead of being reported undefined there");
    }

    // A name Mudlet has never heard of is answered, not ignored: RFC 1572 wants
    // the variable echoed with no VALUE so the game stops waiting on it.
    void test_unknownVariableComesBackUndefined()
    {
        enableNewEnviron();
        requestSend(named(NEW_ENVIRON_USERVAR, "NOT_A_MUDLET_VARIABLE"));

        QString problem;
        const QList<EnvironVariable> variables = soleReplyVariables(NEW_ENVIRON_IS, problem);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        QCOMPARE(variables.size(), 1);
        const EnvironVariable variable = variables.first();
        QCOMPARE(variable.type, NEW_ENVIRON_USERVAR);
        QCOMPARE(variable.name, QByteArray("NOT_A_MUDLET_VARIABLE"));
        QVERIFY2(!variable.hasValue, "a variable Mudlet does not maintain was answered with a value");
    }

    // A type byte with no name after it means "everything of this type", so the
    // two types select different halves of the advertised set - and Mudlet's
    // well-known half is deliberately empty.
    void test_bareTypeByteSelectsThatTypeOnly()
    {
        enableNewEnviron();
        requestSend(QByteArray(1, NEW_ENVIRON_USERVAR));
        const QList<Subnegotiation> uservarReplies = newEnvironReplies();
        QCOMPARE(uservarReplies.size(), 1);
        const QList<EnvironVariable> uservars = variablesIn(uservarReplies.first().payload.mid(1));
        QCOMPARE(uservars.size(), mpHost->mTelnet.getNewEnvironDataMap().size());

        mpServer->forgetReceived();
        requestSend(QByteArray(1, NEW_ENVIRON_VAR));
        const QList<Subnegotiation> varReplies = newEnvironReplies();
        QCOMPARE(varReplies.size(), 1);
        QCOMPARE(varReplies.first().payload.at(0), NEW_ENVIRON_IS);
        QVERIFY2(variablesIn(varReplies.first().payload.mid(1)).isEmpty(), "well-known variables were advertised even though none are opted in to");
    }

    // A game may put any byte in a variable name, the protocol's own delimiters
    // included, as long as it quotes them - and the echo has to quote them right
    // back or the reply reads as several variables at the far end.
    //
    // The quoting is asserted on the raw capture rather than on the decoded name:
    // an IAC that lost its doubling decodes to the same name, so only the bytes
    // themselves can say whether it was quoted. NEW_ENVIRON_ESC quoting is not
    // undone on the receive side, so the name that comes back carries the ESC
    // bytes the game sent - that is Mudlet's behaviour today rather than what
    // RFC 1572 asks for, and this pins it so a conformance fix is a deliberate
    // change to this test and not a surprise.
    void test_specialBytesInARequestedNameAreQuotedInTheReply()
    {
        enableNewEnviron();

        // IAC arrives doubled and is un-doubled by the telnet reader, so what
        // reaches NEW_ENVIRON is one 0xFF; VAL and ESC are not telnet-special
        // and arrive as themselves.
        QByteArray rawName;
        rawName.append('A').append(static_cast<char>(TN_IAC)).append(static_cast<char>(TN_IAC)).append('B').append(NEW_ENVIRON_ESC).append('C').append(NEW_ENVIRON_VAL).append('D');
        requestSend(named(NEW_ENVIRON_USERVAR, rawName));

        const QByteArray wire = sentBytes();

        // What a game reads off the socket: IAC doubled again for telnet, ESC
        // and VAL each quoted with an ESC for NEW_ENVIRON.
        QByteArray expectedOnTheWire;
        expectedOnTheWire.append('A')
                .append(static_cast<char>(TN_IAC))
                .append(static_cast<char>(TN_IAC))
                .append('B')
                .append(NEW_ENVIRON_ESC)
                .append(NEW_ENVIRON_ESC)
                .append('C')
                .append(NEW_ENVIRON_ESC)
                .append(NEW_ENVIRON_VAL)
                .append('D');
        QVERIFY2(wire.contains(expectedOnTheWire), qPrintable(qsl("the reply did not quote the name it echoed; it reads %1").arg(QString::fromLatin1(wire.toHex(' ')))));

        const QList<Subnegotiation> replies = subnegotiationsFor(wire, static_cast<unsigned char>(OPT_NEW_ENVIRON));
        QCOMPARE(replies.size(), 1);
        const QList<EnvironVariable> variables = variablesIn(replies.first().payload.mid(1));
        QCOMPARE(variables.size(), 1);

        QByteArray expectedName;
        expectedName.append('A').append(static_cast<char>(TN_IAC)).append('B').append(NEW_ENVIRON_ESC).append('C').append(NEW_ENVIRON_VAL).append('D');
        QCOMPARE(variables.first().name, expectedName);
        QVERIFY2(!variables.first().hasValue, "a name full of delimiters was matched against a variable Mudlet maintains");
    }

    // A game asks for several variables in one SEND far more often than for one,
    // and the parser carries state between them - the name it has collected is
    // only flushed when the next type byte arrives. Asking for one name at a time
    // never runs that flush.
    void test_severalNamesInOneRequestAreAllAnswered()
    {
        enableNewEnviron();
        requestSend(named(NEW_ENVIRON_USERVAR, "CHARSET") + named(NEW_ENVIRON_USERVAR, "ANSI") + named(NEW_ENVIRON_USERVAR, "TRUECOLOR"));

        QString problem;
        const QList<EnvironVariable> variables = soleReplyVariables(NEW_ENVIRON_IS, problem);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        QCOMPARE(variables.size(), 3);
        QCOMPARE(variableNamed(variables, "CHARSET").value, QByteArray("UTF-8"));
        QCOMPARE(variableNamed(variables, "ANSI").value, QByteArray("1"));
        QCOMPARE(variableNamed(variables, "TRUECOLOR").value, QByteArray("1"));
    }

    // The two namespaces in one request, which is where the parser has to keep
    // track of which type the name it is collecting belongs to. The USERVAR comes
    // first so the name before the switch is flushed by the well-known arm - the
    // mirror of the arm the all-USERVAR request above runs.
    void test_aRequestMixingBothNamespacesKeepsThemApart()
    {
        enableNewEnviron();
        requestSend(named(NEW_ENVIRON_USERVAR, "ANSI") + named(NEW_ENVIRON_VAR, "CHARSET"));

        QString problem;
        const QList<EnvironVariable> variables = soleReplyVariables(NEW_ENVIRON_IS, problem);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        QCOMPARE(variables.size(), 2);

        const EnvironVariable wellKnown = variableNamed(variables, "CHARSET");
        QCOMPARE(wellKnown.type, NEW_ENVIRON_VAR);
        QVERIFY2(!wellKnown.hasValue, "a name asked for under the well-known type came back with the USERVAR value");

        const EnvironVariable userVar = variableNamed(variables, "ANSI");
        QCOMPARE(userVar.type, NEW_ENVIRON_USERVAR);
        QCOMPARE(userVar.value, QByteArray("1"));
    }

    // The other half of negotiation: a game that asks Mudlet to turn NEW_ENVIRON
    // on, rather than offering to, and then withdraws its request.
    void test_aRequestToEnableIsAnsweredAndCanBeWithdrawn()
    {
        announce(TN_DO, OPT_NEW_ENVIRON);
        QVERIFY2(mpHost->mTelnet.isNewEnvironEnabled(), "a DO NEW-ENVIRON did not turn the protocol on");
        QByteArray expected;
        expected.append(TN_IAC).append(TN_WILL).append(OPT_NEW_ENVIRON);
        QVERIFY2(sentBytes().contains(expected), "Mudlet did not answer DO NEW-ENVIRON with WILL");

        mpServer->forgetReceived();
        announce(TN_DONT, OPT_NEW_ENVIRON);
        QVERIFY2(!mpHost->mTelnet.isNewEnvironEnabled(), "a DONT NEW-ENVIRON left the protocol on");

        mpServer->forgetReceived();
        requestSend({});
        QVERIFY2(newEnvironReplies().isEmpty(), "Mudlet kept answering NEW_ENVIRON after the game withdrew its request");
    }

    // Two malformed requests that must produce nothing rather than a reply built
    // from whatever the bytes happened to say: one too short to hold a command,
    // and one whose command is not SEND.
    void test_malformedRequestsAreIgnored()
    {
        enableNewEnviron();

        feed(subnegotiation(OPT_NEW_ENVIRON, {}));
        QVERIFY2(newEnvironReplies().isEmpty(), "a NEW_ENVIRON subnegotiation with no command byte was answered");

        mpServer->forgetReceived();
        feed(subnegotiation(OPT_NEW_ENVIRON, QByteArray(1, NEW_ENVIRON_IS) + named(NEW_ENVIRON_VAR, "CHARSET")));
        QVERIFY2(newEnvironReplies().isEmpty(), "a NEW_ENVIRON request that was not a SEND was answered");

        // The control: a well-formed request over the same connection is still
        // answered, so neither silence above can be the capture having died.
        mpServer->forgetReceived();
        requestSend(named(NEW_ENVIRON_USERVAR, "CHARSET"));
        QCOMPARE(newEnvironReplies().size(), 1);
    }

    // Nothing at all goes out once the game has withdrawn the option, neither an
    // answer to a request nor an unsolicited update.
    void test_nothingIsSentOnceTheGameWithdrawsTheOption()
    {
        enableNewEnviron();
        requestSend({});
        QCOMPARE(newEnvironReplies().size(), 1);

        announce(TN_WONT, OPT_NEW_ENVIRON);
        QVERIFY2(!mpHost->mTelnet.isNewEnvironEnabled(), "a withdrawn NEW_ENVIRON was still marked enabled");

        mpServer->forgetReceived();
        requestSend({});
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("CHARSET"));
        QVERIFY2(newEnvironReplies().isEmpty(), "Mudlet kept answering NEW_ENVIRON after the game withdrew it");
    }

    // A preference the game has already been told about is worth an INFO when it
    // changes, carrying the new value under the same type the original answer
    // used - anything else and the game files the update under a second name.
    void test_infoUpdateCarriesTheNewValue()
    {
        enableNewEnviron();
        requestSend({});
        QCOMPARE(newEnvironReplies().size(), 1);

        mpServer->forgetReceived();
        mpHost->mAdvertiseScreenReader = true;
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("SCREEN_READER"));

        QString problem;
        const QList<EnvironVariable> updates = soleReplyVariables(NEW_ENVIRON_INFO, problem);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        QCOMPARE(updates.size(), 1);
        const EnvironVariable updated = updates.first();
        QCOMPARE(updated.type, NEW_ENVIRON_USERVAR);
        QCOMPARE(updated.name, QByteArray("SCREEN_READER"));
        QCOMPARE(updated.value, QByteArray("1"));
    }

    // An INFO is only owed to a game that asked for the variable in the first
    // place: one it never requested arrives as a name it has no idea what to do
    // with. The reconnect is what makes that testable, since the record of what
    // has been asked for is only cleared by a fresh connection.
    void test_noInfoIsSentForAVariableTheGameNeverAskedFor()
    {
        reconnect();
        enableNewEnviron();
        requestSend(named(NEW_ENVIRON_USERVAR, "CHARSET"));
        QCOMPARE(newEnvironReplies().size(), 1);

        // In the advertised set, and its value has changed, but this game has
        // only ever asked about CHARSET.
        mpServer->forgetReceived();
        mpHost->mAdvertiseScreenReader = true;
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("SCREEN_READER"));
        QVERIFY2(newEnvironReplies().isEmpty(), "an INFO went out for a variable the game never asked for");

        // The control: the one variable it did ask about is still updated.
        mpServer->forgetReceived();
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("CHARSET"));
        QString problem;
        const QList<EnvironVariable> updates = soleReplyVariables(NEW_ENVIRON_INFO, problem);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        QCOMPARE(updates.size(), 1);
        QCOMPARE(updates.first().name, QByteArray("CHARSET"));
    }

    // The hyperlink capabilities change together, so they are announced together
    // - one subnegotiation the game sees as a single consistent state, not a run
    // of partial ones.
    void test_hyperlinkCapabilitiesAreAnnouncedInOneUpdate()
    {
        enableNewEnviron();
        requestSend({});
        QCOMPARE(newEnvironReplies().size(), 1);

        mpServer->forgetReceived();
        mpHost->mEnableOSC8Hyperlinks = false;
        mpHost->mTelnet.sendInfoNewEnvironOSCHyperlinks();

        const QList<Subnegotiation> replies = newEnvironReplies();
        QCOMPARE(replies.size(), 1);
        QCOMPARE(replies.first().payload.at(0), NEW_ENVIRON_INFO);

        const QList<EnvironVariable> variables = variablesIn(replies.first().payload.mid(1));
        QCOMPARE(variables.size(), advertisedHyperlinkVariables());
        QVERIFY2(advertisedHyperlinkVariables() > 10, "too few hyperlink capabilities are advertised for the comparison above to prove anything");
        for (const EnvironVariable& variable : variables) {
            QVERIFY2(variable.name.startsWith("OSC_HYPERLINKS"), qPrintable(qsl("%1 rode along with the hyperlink update").arg(QString::fromLatin1(variable.name))));
            QCOMPARE(variable.value, QByteArray("0"));
        }
    }

    // MNES narrows the advertised set to five names and moves them into the
    // well-known namespace.
    void test_mnesAnswersItsFiveVariablesAsWellKnownOnes()
    {
        mpHost->mEnableMNES = true;
        enableNewEnviron();
        requestSend({});

        const QList<Subnegotiation> replies = newEnvironReplies();
        QCOMPARE(replies.size(), 1);
        QCOMPARE(replies.first().payload.at(0), NEW_ENVIRON_IS);

        const QList<EnvironVariable> variables = variablesIn(replies.first().payload.mid(1));
        QCOMPARE(variables.size(), 5);
        QStringList names;
        for (const EnvironVariable& variable : variables) {
            QCOMPARE(variable.type, NEW_ENVIRON_VAR);
            names << QString::fromLatin1(variable.name);
        }
        names.sort();
        QCOMPARE(names, QStringList({qsl("CHARSET"), qsl("CLIENT_NAME"), qsl("CLIENT_VERSION"), qsl("MTTS"), qsl("TERMINAL_TYPE")}));
    }

    // Under MNES a name outside that set is not Mudlet's to answer at all, while
    // one inside it that Mudlet chooses not to supply is answered as maintained
    // but empty. IPADDRESS reaches the second path by being in the MNES name list
    // and absent from the data map.
    void test_mnesAnswersOnlyMnesNames()
    {
        mpHost->mEnableMNES = true;
        enableNewEnviron();

        requestSend(named(NEW_ENVIRON_VAR, "ANSI"));
        QVERIFY2(newEnvironReplies().isEmpty(), "MNES answered a variable that is not part of MNES");

        mpServer->forgetReceived();
        requestSend(named(NEW_ENVIRON_VAR, "IPADDRESS"));
        QString addressProblem;
        const QList<EnvironVariable> addressReply = soleReplyVariables(NEW_ENVIRON_IS, addressProblem);
        QVERIFY2(addressProblem.isEmpty(), qPrintable(addressProblem));
        QCOMPARE(addressReply.size(), 1);
        const EnvironVariable address = addressReply.first();
        QCOMPARE(address.name, QByteArray("IPADDRESS"));
        QVERIFY2(address.hasValue, "IPADDRESS was reported as a name MNES does not know rather than one Mudlet does not fill in");
        QVERIFY2(address.value.isEmpty(), "Mudlet supplied an IP address, which it deliberately does not do");

        mpServer->forgetReceived();
        requestSend(named(NEW_ENVIRON_VAR, "CLIENT_NAME"));
        QString clientNameProblem;
        const QList<EnvironVariable> clientNameReply = soleReplyVariables(NEW_ENVIRON_IS, clientNameProblem);
        QVERIFY2(clientNameProblem.isEmpty(), qPrintable(clientNameProblem));
        QCOMPARE(clientNameReply.size(), 1);
        const EnvironVariable clientName = clientNameReply.first();
        QCOMPARE(clientName.name, QByteArray("CLIENT_NAME"));
        QCOMPARE(clientName.value, QByteArray("MUDLET"));
    }

    // MNES has its own request parser, with the same flush-on-the-next-name shape
    // and the same hole if only one name is ever asked for.
    void test_severalMnesNamesInOneRequestAreAllAnswered()
    {
        mpHost->mEnableMNES = true;
        enableNewEnviron();
        requestSend(named(NEW_ENVIRON_VAR, "CLIENT_NAME") + named(NEW_ENVIRON_VAR, "CHARSET") + named(NEW_ENVIRON_VAR, "TERMINAL_TYPE"));

        // MNES answers a name at a time rather than gathering them into one IS.
        const QList<Subnegotiation> replies = newEnvironReplies();
        QCOMPARE(replies.size(), 3);

        QList<EnvironVariable> answered;
        for (const Subnegotiation& reply : replies) {
            QCOMPARE(reply.payload.at(0), NEW_ENVIRON_IS);
            answered.append(variablesIn(reply.payload.mid(1)));
        }
        QCOMPARE(answered.size(), 3);
        QCOMPARE(variableNamed(answered, "CLIENT_NAME").value, QByteArray("MUDLET"));
        QCOMPARE(variableNamed(answered, "CHARSET").value, QByteArray("UTF-8"));
        QCOMPARE(variableNamed(answered, "TERMINAL_TYPE").value, QByteArray("ANSI-TRUECOLOR"));
    }

    // The MTTS bitvector is how a game learns MNES is on offer before it asks
    // for anything, so the bit has to follow the preference.
    void test_mnesIsAdvertisedInTheMttsBitvector()
    {
        QVERIFY2(!(mpHost->mTelnet.getNewEnvironDataMap().value(qsl("MTTS")).second.toInt() & MTTS_STD_MNES), "MNES was advertised in MTTS while it was turned off");

        mpHost->mEnableMNES = true;
        QVERIFY2(mpHost->mTelnet.getNewEnvironDataMap().value(qsl("MTTS")).second.toInt() & MTTS_STD_MNES, "MNES was not advertised in MTTS while it was turned on");

        // MNES rides on NEW_ENVIRON, so it cannot be advertised without it.
        mpHost->mEnableNEWENVIRON = false;
        QVERIFY2(!(mpHost->mTelnet.getNewEnvironDataMap().value(qsl("MTTS")).second.toInt() & MTTS_STD_MNES), "MNES was advertised in MTTS with NEW_ENVIRON turned off");
    }
};

#include "TelnetNewEnvironTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetNewEnvironTest)
