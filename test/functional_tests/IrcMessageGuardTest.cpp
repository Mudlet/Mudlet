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
 * What one sendIrc() may put on the wire - issues #10770 and #10771. The Lua
 * specs can only reach the refusals: a sendIrc() that is going to be sent
 * creates the profile's IRC dialog, which the spec suite avoids because nothing
 * in the Lua API closes it again. So the half that says a legitimate message
 * still goes out unchanged lives here, along with the bytes that reach a
 * connected server, which no spec can see at all.
 *
 * Run with: ctest -R IrcMessageGuardTest -V
 */

#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "Host.h"
#include "TLuaInterpreter.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgIRC.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Accepts the IRC client's connection and keeps every byte of it. It answers
// nothing: IrcConnection writes as soon as open() has put it in the connecting
// state, so no registration handshake is needed to see what it sends.
class IrcRecordingServer : public QTcpServer
{
    Q_OBJECT

public:
    using QTcpServer::QTcpServer;

    QByteArray received() const { return mReceived; }
    void clearReceived() { mReceived.clear(); }

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        auto* socket = new QTcpSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            delete socket;
            return;
        }
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            mReceived.append(socket->readAll());
        });
    }

private:
    QByteArray mReceived;
};

class IrcMessageGuardTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpGameServer = nullptr;
    IrcRecordingServer* mpIrcServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("IrcMessageGuard");

    // The dialog reads the profile's IRC settings in its constructor, so they
    // have to be stored before it is built.
    dlgIRC* openClient()
    {
        QPair<bool, QString> stored = dlgIRC::writeIrcHostName(mpHost, qsl("127.0.0.1"));
        if (!stored.first) {
            return nullptr;
        }
        stored = dlgIRC::writeIrcHostPort(mpHost, mpIrcServer->serverPort());
        if (!stored.first) {
            return nullptr;
        }
        stored = dlgIRC::writeIrcNickName(mpHost, qsl("guardbot"));
        if (!stored.first) {
            return nullptr;
        }
        stored = dlgIRC::writeIrcChannels(mpHost, QStringList() << qsl("#guard"));
        if (!stored.first) {
            return nullptr;
        }

        mpIrcServer->clearReceived();
        // built directly rather than through openIRC(), which is a Lua function
        mpHost->mpDlgIRC = new dlgIRC(mpHost);
        mpHost->mpDlgIRC->show();
        return mpHost->mpDlgIRC;
    }

    // Every complete line the client has sent, with the CR LF taken off - so an
    // injected second command is a second entry rather than something hidden
    // inside the first.
    QList<QByteArray> wireLines() const
    {
        QList<QByteArray> lines;
        for (const QByteArray& line : mpIrcServer->received().split('\n')) {
            const QByteArray trimmed = QByteArray(line).replace('\r', QByteArray());
            if (!trimmed.isEmpty()) {
                lines.append(trimmed);
            }
        }
        return lines;
    }

    bool waitForWireLine(const QByteArray& line)
    {
        return QTest::qWaitFor(
                [this, line]() {
                    return wireLines().contains(line);
                },
                5000);
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

        mpGameServer = new TelnetServerStub(qApp);
        mpGameServer->start(qsl("localhost"), 0);
        QVERIFY2(mpGameServer->serverPort() != 0, "The telnet stub did not start listening");

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpIrcServer = new IrcRecordingServer(qApp);
        QVERIFY2(mpIrcServer->listen(QHostAddress::LocalHost, 0), "The IRC stub did not start listening");

        mpHost = TestProfile::create(mProfileName, qsl("localhost"), QString::number(mpGameServer->serverPort()));
        QVERIFY2(mpHost, "Profile took too long to load");
    }

    void cleanup()
    {
        if (mpHost && mpHost->mpDlgIRC) {
            delete mpHost->mpDlgIRC;
            // ~dlgIRC() sends a QUIT, and the next test's wire must not be
            // holding it when it looks for a command of its own
            QTest::qWait(100);
        }
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            const QString profilePath = MudletPaths::getMudletPath(enums::profileHomePath, mProfileName);
            delete mudlet::self();
            QDir(profilePath).removeRecursively();
        }
        delete mpGameServer;
        mpGameServer = nullptr;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The half no Lua spec can reach: a guard that refused everything would pass
    // every one of them.
    void test_ordinaryMessagesAreAccepted()
    {
        const QList<QPair<QString, QString>> accepted = {
                {qsl("#mudlet"), qsl("hello")},
                // the mIRC formatting codes and the CTCP delimiter, which an IRC
                // message may legitimately carry
                {qsl("#mudlet"), QString(QChar(0x02)) + qsl("bold") + QChar(0x03) + qsl("04red") + QChar(0x0f) + qsl(" and ") + QChar(0x01) + qsl("ACTION waves") + QChar(0x01)},
                {qsl("#mudlet"), qsl("a message with spaces in it")},
                {qsl("SomeNick"), qsl("a private message")},
                // a comma-separated target list is one PRIVMSG in the protocol
                {qsl("#mudlet,#mudlet-dev"), qsl("hello both")},
                // a colon anywhere but the front of the target, and a message
                // that starts with one
                {qsl("#mud:let"), qsl(":not a prefix")},
        };

        for (const auto& [target, message] : accepted) {
            const auto result = dlgIRC::validateMsgArguments(target, message);
            QVERIFY2(result.first, qPrintable(qsl("sending \"%1\" to \"%2\" was refused: %3").arg(message, target, result.second)));
            QVERIFY(result.second.isEmpty());
        }
    }

    void test_lineProtocolBreakersAreRefused()
    {
        const QList<QPair<QString, QString>> refused = {
                // #10770, from the report
                {qsl("#mudlet"), qsl("MARK before\r\nQUIT :injected-quit")},
                {qsl("#mudlet"), qsl("MARK before\nPRIVMSG #evil :injected-lf")},
                {qsl("#mudlet"), qsl("MARK before\rinjected-cr")},
                {qsl("#mudlet"), qsl("MARK before") + QChar(QChar::Null) + qsl("injected-nul")},
                {qsl("#mudlet\r\nJOIN #evil"), qsl("MARK target injection")},
                {qsl("#mudlet") + QChar(QChar::Null) + qsl("#evil"), qsl("MARK target nul")},
                // the other ways a target can confuse the line protocol
                {qsl("#mudlet #evil"), qsl("MARK spaced target")},
                {qsl("#mudlet\t#evil"), qsl("MARK tabbed target")},
                {qsl(":#mudlet"), qsl("MARK colon target")},
                {qsl("#mudlet,"), qsl("MARK empty name in the list")},
                {QString(), qsl("MARK empty target")},
                // nothing to send is not a send that succeeded
                {qsl("#mudlet"), QString()},
        };

        for (const auto& [target, message] : refused) {
            const auto result = dlgIRC::validateMsgArguments(target, message);
            QVERIFY2(!result.first, qPrintable(qsl("sending \"%1\" to \"%2\" was accepted").arg(message, target)));
            QVERIFY2(!result.second.isEmpty(), "a refusal has to say why");
            // the reason quotes the offending text, so it must not break its own
            // line doing it
            QVERIFY2(!result.second.contains(QChar::CarriageReturn) && !result.second.contains(QChar::LineFeed) && !result.second.contains(QChar::Null),
                     qPrintable(qsl("the refusal carried the very characters it refused: %1").arg(result.second)));
        }
    }

    // #10771: the message the Lua API is given is text, whatever it starts with.
    // The command parser sendMsg() uses would turn each of these into a protocol
    // command of the game server's choosing.
    void test_theLuaPathSendsTextRatherThanCommands()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !wireLines().isEmpty();
                         },
                         5000),
                 "the IRC client never reached the stub server");

        const QStringList commandsThatAreNotCommandsHere = {
                qsl("/PRIVMSG #evil :owned-by-an-unknown-verb"), // tolerant parser: any verb at all
                qsl("/QUOTE PRIVMSG #evil :owned-by-quote"),     // and a registered one that does the same
                qsl("/join #evil"),
                qsl("/nick pwned"),
                qsl("/quit :injected-quit"),
        };
        for (const QString& message : commandsThatAreNotCommandsHere) {
            const auto result = client->sendText(qsl("#guard"), message);
            QVERIFY2(result.first, qPrintable(qsl("sending \"%1\" was refused: %2").arg(message, result.second)));
            QVERIFY2(waitForWireLine(qsl("PRIVMSG #guard :%1").arg(message).toUtf8()), qPrintable(qsl("\"%1\" did not reach the channel as text").arg(message)));
        }

        for (const QByteArray& line : wireLines()) {
            QVERIFY2(!line.startsWith("JOIN #evil") && !line.startsWith("NICK pwned") && !line.startsWith("QUIT") && !line.startsWith("PRIVMSG #evil"),
                     qPrintable(qsl("a message put \"%1\" on the wire as a command of its own").arg(QString::fromUtf8(line))));
        }
        QVERIFY2(!mpHost->mpDlgIRC.isNull(), "a message closed the IRC client");
    }

    // The same thing through the API a package actually calls, which is what
    // says sendIrc() reaches the path above rather than the parsing one. No Lua
    // spec can get this far: a sendIrc() that is going to be sent needs a client
    // that has joined a channel.
    void test_sendIrcItselfSendsTextRatherThanCommands()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !wireLines().isEmpty();
                         },
                         5000),
                 "the IRC client never reached the stub server");
        // what a joined channel leaves behind, without needing the server to say so
        client->mReadyForSending = true;

        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("ircResult, ircError = sendIrc(\"#guard\", \"/join #evil\")")));
        QVERIFY2(waitForWireLine("PRIVMSG #guard :/join #evil"), "sendIrc did not send the leading-slash message as text");

        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("ircResult, ircError = sendIrc(\"#guard\", \"MARK\\r\\nQUIT :injected-quit\")")));

        for (const QByteArray& line : wireLines()) {
            QVERIFY2(!line.startsWith("JOIN #evil") && !line.startsWith("QUIT"), qPrintable(qsl("sendIrc put \"%1\" on the wire as a command of its own").arg(QString::fromUtf8(line))));
        }
    }

    // The other entry point, which the Lua specs cannot reach at all: a refusal
    // has to stop the bytes, not merely return false.
    void test_aRefusedMessageReachesNoSocket()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !wireLines().isEmpty();
                         },
                         5000),
                 "the IRC client never reached the stub server");

        QVERIFY(!client->sendText(qsl("#guard"), qsl("MARK\r\nQUIT :injected-quit")).first);
        QVERIFY(!client->sendMsg(qsl("#guard"), qsl("MARK\r\nQUIT :injected-quit")).first);
        QVERIFY(!client->sendMsg(qsl("#guard\r\nJOIN #evil"), qsl("MARK")).first);

        // the one message that is allowed through is what proves the wait above
        // was long enough for a refused one to have arrived
        QVERIFY(client->sendText(qsl("#guard"), qsl("MARK allowed")).first);
        QVERIFY2(waitForWireLine("PRIVMSG #guard :MARK allowed"), "the allowed message never reached the stub server");

        for (const QByteArray& line : wireLines()) {
            QVERIFY2(!line.contains("injected-quit") && !line.startsWith("JOIN #evil"), qPrintable(qsl("a refused message still reached the wire: %1").arg(QString::fromUtf8(line))));
        }
    }

    // The IRC window clears its input line whatever sendMsg() returns, so a
    // refusal it did not report would take the typed message away in silence.
    void test_theIrcWindowReportsARefusedMessage()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !wireLines().isEmpty();
                         },
                         5000),
                 "the IRC client never reached the stub server");

        // a comma-separated target list is legal in the protocol and has always
        // worked here, so it must still
        client->lineEdit->setText(qsl("/msg #guard,#guard-two hello both"));
        QVERIFY(QMetaObject::invokeMethod(client, "slot_onTextEntered"));
        QVERIFY2(waitForWireLine("PRIVMSG #guard,#guard-two :hello both"), "a /msg to a target list never reached the wire");

        // a target that starts with a colon: parsed as a /msg, then refused by
        // the guard, which is the path that used to swallow the message
        client->lineEdit->setText(qsl("/msg :#guard hello"));
        QVERIFY(QMetaObject::invokeMethod(client, "slot_onTextEntered"));
        QVERIFY2(client->ircBrowser->toPlainText().contains(qsl("Could not send that message")),
                 qPrintable(qsl("the refusal was not reported, the window says: %1").arg(client->ircBrowser->toPlainText())));
    }

    // The nick and the server password are put on the wire by the connection
    // itself at registration, without passing through validateMsgArguments().
    void test_theNickAndThePasswordRefuseALineBreak()
    {
        QVERIFY(!dlgIRC::writeIrcNickName(mpHost, qsl("bob\r\nQUIT :injected")).first);
        QVERIFY(!dlgIRC::writeIrcNickName(mpHost, qsl("bob\nJOIN #evil")).first);
        QVERIFY(!dlgIRC::writeIrcNickName(mpHost, qsl("bob and jane")).first);
        QVERIFY(dlgIRC::writeIrcNickName(mpHost, qsl("bob")).first);

        QVERIFY(!dlgIRC::writeIrcPassword(mpHost, qsl("hunter2\r\nPRIVMSG #evil :injected")).first);
        // a password is a trailing parameter, so a space in one is its own business
        QVERIFY(dlgIRC::writeIrcPassword(mpHost, qsl("hunter2 with spaces")).first);

        // and the refusal does not hand the password back to whoever is reading
        const auto refusal = dlgIRC::writeIrcPassword(mpHost, qsl("hunter2\r\nPRIVMSG #evil :injected"));
        QVERIFY(!refusal.second.contains(qsl("hunter2")));
    }
};

#include "IrcMessageGuardTest.moc"
MUDLET_GROUPED_TEST_MAIN(IrcMessageGuardTest)
