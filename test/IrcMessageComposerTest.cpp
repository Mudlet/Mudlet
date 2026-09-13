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

#include <IrcConnection>
#include <IrcMessage>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

/*
 * The multi-line numerics communi assembles into one message - the MOTD, the
 * names list, a WHOIS - are built up on a stack inside IrcMessageComposer, and
 * which numeric arrives next is entirely the server's choice. Nothing obliges a
 * server to send the opening numeric of a block before a continuation of it, so
 * every branch that appends to the message under construction has to cope with
 * there being none, or with it being a different kind of message than the one
 * the numeric belongs to.
 *
 * These cases drive a real IrcConnection against a loopback server that says
 * exactly what the case wants said, which is the path in #10769's backtrace:
 * IrcProtocol::read() -> readLines() -> composeMessage().
 *
 * The composer only depends on communi, so this needs neither a Host nor the
 * Mudlet application stack.
 */

// The other end of the socket: answers registration, and otherwise says only
// what a case tells it to.
class FakeIrcServer : public QObject
{
    Q_OBJECT

public:
    explicit FakeIrcServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &FakeIrcServer::slot_accept);
        mListening = mServer.listen(QHostAddress::LocalHost, 0);
    }

    bool listening() const { return mListening; }

    quint16 port() const { return mServer.serverPort(); }

    bool clientPresent() const { return mpClient != nullptr; }

    void send(const QByteArray& line)
    {
        if (!mpClient) {
            return;
        }
        mpClient->write(line + "\r\n");
        mpClient->flush();
    }

private slots:
    void slot_accept()
    {
        mpClient = mServer.nextPendingConnection();
        connect(mpClient, &QTcpSocket::readyRead, this, &FakeIrcServer::slot_read);
    }

    // The registration burst goes out once the client has named itself, which is
    // what makes IrcConnection report itself connected.
    void slot_read()
    {
        mBuffer += mpClient->readAll();
        int end = -1;
        while ((end = mBuffer.indexOf('\n')) != -1) {
            const QByteArray line = mBuffer.left(end).trimmed();
            mBuffer = mBuffer.mid(end + 1);
            if (line.startsWith("USER ")) {
                send(":qa.irc.test 001 QAtester :Welcome to the QA network");
                send(":qa.irc.test 002 QAtester :Your host is qa.irc.test");
                send(":qa.irc.test 003 QAtester :This server was created today");
                send(":qa.irc.test 004 QAtester qa.irc.test qa-1.0 o o");
            }
        }
    }

private:
    QTcpServer mServer;
    QTcpSocket* mpClient = nullptr;
    QByteArray mBuffer;
    bool mListening = false;
};

// What a case needs to know about a message, kept by value: communi deletes
// every message once it has been delivered.
struct SeenMessage
{
    IrcMessage::Type type = IrcMessage::Unknown;
    int code = 0;
    QStringList parameters;
};

class IrcMessageComposerTest : public QObject
{
    Q_OBJECT

private:
    FakeIrcServer* mpServer = nullptr;
    IrcConnection* mpConnection = nullptr;
    QList<SeenMessage> mSeen;
    int mMarker = 0;

    // Says the lines and waits until a marker sent after them has arrived, so a
    // case never has to guess how long the round trip takes. An uncomposed
    // numeric of its own makes the marker, so it cannot disturb what is on the
    // compose stack.
    void say(const QByteArrayList& lines)
    {
        QVERIFY2(mpServer->clientPresent(), "the loopback server never accepted a connection");
        for (const QByteArray& line : lines) {
            mpServer->send(line);
        }
        const QByteArray marker = QByteArray::number(++mMarker);
        mpServer->send(":qa.irc.test 999 QAtester :marker " + marker);
        const QString expected = QStringLiteral("marker %1").arg(QString::fromUtf8(marker));
        // Anywhere in what has been seen, not only last: a case that composes a
        // message would otherwise spend the whole timeout and report nothing.
        QTRY_VERIFY_WITH_TIMEOUT(markerIndex(expected) != -1, 5000);
        mSeen.removeAt(markerIndex(expected));
    }

    int markerIndex(const QString& expected) const
    {
        for (int i = 0; i < mSeen.count(); ++i) {
            if (mSeen.at(i).code == 999 && mSeen.at(i).parameters.contains(expected)) {
                return i;
            }
        }
        return -1;
    }

    QList<SeenMessage> ofType(const IrcMessage::Type type) const
    {
        QList<SeenMessage> matches;
        for (const SeenMessage& seen : mSeen) {
            if (seen.type == type) {
                matches << seen;
            }
        }
        return matches;
    }

private slots:
    void init()
    {
        mSeen.clear();
        mpServer = new FakeIrcServer(this);
        QVERIFY2(mpServer->listening(), "could not listen on the loopback interface");

        mpConnection = new IrcConnection(this);
        mpConnection->setHost(QStringLiteral("127.0.0.1"));
        mpConnection->setPort(mpServer->port());
        mpConnection->setNickName(QStringLiteral("QAtester"));
        mpConnection->setUserName(QStringLiteral("qatester"));
        mpConnection->setRealName(QStringLiteral("Mudlet QA"));
        connect(mpConnection, &IrcConnection::messageReceived, this, [this](IrcMessage* message) {
            SeenMessage seen;
            seen.type = message->type();
            seen.parameters = message->parameters();
            if (message->type() == IrcMessage::Numeric) {
                seen.code = static_cast<IrcNumericMessage*>(message)->code();
            }
            mSeen << seen;
        });

        mpConnection->open();
        QTRY_VERIFY_WITH_TIMEOUT(mpConnection->isConnected(), 5000);
        // isConnected() goes true on the 001, so the rest of the registration burst
        // may still be in flight. A marker round trip puts it all behind us.
        say({});
        mSeen.clear();
    }

    void cleanup()
    {
        delete mpConnection;
        mpConnection = nullptr;
        delete mpServer;
        mpServer = nullptr;
    }

    // #10769: one RPL_MOTD with no RPL_MOTDSTART before it took the whole
    // application down, because the branch that appends the line called top() on
    // an empty stack. The connection has to survive to hear the next line, and the
    // stray one has to still reach a reader.
    void motdLine_withNothingBeingComposed_doesNotCrash()
    {
        say({":qa.irc.test 372 QAtester :- a stray MOTD line"});

        const QList<SeenMessage> numerics = ofType(IrcMessage::Numeric);
        QCOMPARE(numerics.count(), 1);
        QCOMPARE(numerics.constFirst().code, 372);
        QVERIFY(mpConnection->isConnected());

        // Every client suppresses a bare RPL_MOTD, because IrcNumericMessage::isComposed()
        // answers per code, so the line only reaches a reader as part of a MOTD message
        const QList<SeenMessage> motds = ofType(IrcMessage::Motd);
        QCOMPARE(motds.count(), 1);
        QVERIFY2(motds.constFirst().parameters.contains(QStringLiteral("- a stray MOTD line")), qPrintable(motds.constFirst().parameters.join(QLatin1Char('|'))));
    }

    // The compose stack is empty again once RPL_ENDOFMOTD has emitted the MOTD, so
    // this is the same empty top() as the case above - but it is the second variant
    // the report describes, and it is the one that shows the guard does not simply
    // fold the stray line back into the MOTD that has already been delivered.
    void motdLine_afterACompletedMotd_doesNotCrash()
    {
        say({":qa.irc.test 375 QAtester :- qa.irc.test Message of the Day -", ":qa.irc.test 372 QAtester :- first line", ":qa.irc.test 376 QAtester :End of /MOTD command."});
        QCOMPARE(ofType(IrcMessage::Motd).count(), 1);

        say({":qa.irc.test 372 QAtester :- a stray MOTD line"});

        const QList<SeenMessage> motds = ofType(IrcMessage::Motd);
        QCOMPARE(motds.count(), 2);
        QVERIFY2(motds.constLast().parameters.contains(QStringLiteral("- a stray MOTD line")), qPrintable(motds.constLast().parameters.join(QLatin1Char('|'))));
        QVERIFY2(!motds.constFirst().parameters.contains(QStringLiteral("- a stray MOTD line")), qPrintable(motds.constFirst().parameters.join(QLatin1Char('|'))));
        QVERIFY(mpConnection->isConnected());
    }

    // The guard must not cost a well-formed MOTD its lines
    void motd_wellFormedBlockStillComposesEveryLine()
    {
        say({":qa.irc.test 375 QAtester :- qa.irc.test Message of the Day -",
             ":qa.irc.test 372 QAtester :- first line",
             ":qa.irc.test 372 QAtester :- second line",
             ":qa.irc.test 376 QAtester :End of /MOTD command."});

        const QList<SeenMessage> motds = ofType(IrcMessage::Motd);
        QCOMPARE(motds.count(), 1);
        QVERIFY2(motds.constFirst().parameters.contains(QStringLiteral("- first line")), qPrintable(motds.constFirst().parameters.join(QLatin1Char('|'))));
        QVERIFY2(motds.constFirst().parameters.contains(QStringLiteral("- second line")), qPrintable(motds.constFirst().parameters.join(QLatin1Char('|'))));
    }

    // A WHOIS is composed on the same stack, so a stray MOTD line arriving while
    // one is being built would append itself to the WHOIS if the branch only
    // checked that the stack was not empty.
    void motdLine_duringAWhois_leavesTheWhoisAlone()
    {
        say({":qa.irc.test 311 QAtester bob ident host.example * :Bob Example", ":qa.irc.test 372 QAtester :- a stray MOTD line", ":qa.irc.test 318 QAtester bob :End of /WHOIS list."});

        const QList<SeenMessage> whoises = ofType(IrcMessage::Whois);
        QCOMPARE(whoises.count(), 1);
        QVERIFY2(!whoises.constFirst().parameters.contains(QStringLiteral("- a stray MOTD line")), qPrintable(whoises.constFirst().parameters.join(QLatin1Char('|'))));
    }

    // The WHOIS numerics that overwrite a single parameter address the slots of a
    // WHOIS message by index. Sent while a MOTD is being composed they used to
    // overwrite one of its lines instead, so a server could rewrite the MOTD a
    // player is shown.
    void whoisServerNumeric_duringAMotd_leavesTheMotdAlone()
    {
        say({":qa.irc.test 375 QAtester :- qa.irc.test Message of the Day -",
             ":qa.irc.test 372 QAtester :- first line",
             ":qa.irc.test 312 QAtester bob other.example :Some other server",
             ":qa.irc.test 376 QAtester :End of /MOTD command."});

        const QList<SeenMessage> motds = ofType(IrcMessage::Motd);
        QCOMPARE(motds.count(), 1);
        QVERIFY2(motds.constFirst().parameters.contains(QStringLiteral("- first line")), qPrintable(motds.constFirst().parameters.join(QLatin1Char('|'))));
        QVERIFY2(!motds.constFirst().parameters.contains(QStringLiteral("other.example")), qPrintable(motds.constFirst().parameters.join(QLatin1Char('|'))));
    }

    // ...but they still have to reach the WHOIS they do belong to
    void whoisServerNumeric_duringAWhois_stillFillsItIn()
    {
        say({":qa.irc.test 311 QAtester bob ident host.example * :Bob Example",
             ":qa.irc.test 312 QAtester bob other.example :Some other server",
             ":qa.irc.test 318 QAtester bob :End of /WHOIS list."});

        const QList<SeenMessage> whoises = ofType(IrcMessage::Whois);
        QCOMPARE(whoises.count(), 1);
        QVERIFY2(whoises.constFirst().parameters.contains(QStringLiteral("other.example")), qPrintable(whoises.constFirst().parameters.join(QLatin1Char('|'))));
    }

    // A WHOWAS is composed the same way and a server sends RPL_WHOISSERVER inside
    // one too, so the guard has to admit that message type as well as a WHOIS
    void whoisServerNumeric_duringAWhowas_stillFillsItIn()
    {
        say({":qa.irc.test 314 QAtester bob ident host.example * :Bob Example", ":qa.irc.test 312 QAtester bob other.example :Some other server", ":qa.irc.test 369 QAtester bob :End of WHOWAS"});

        const QList<SeenMessage> whowases = ofType(IrcMessage::Whowas);
        QCOMPARE(whowases.count(), 1);
        QVERIFY2(whowases.constFirst().parameters.contains(QStringLiteral("other.example")), qPrintable(whowases.constFirst().parameters.join(QLatin1Char('|'))));
    }
};

QTEST_MAIN(IrcMessageComposerTest)
#include "IrcMessageComposerTest.moc"
