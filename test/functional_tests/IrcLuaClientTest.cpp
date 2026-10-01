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
 * What the Lua IRC functions see and do while a profile's IRC client is
 * running: the getters read the live session rather than the stored settings,
 * restartIrc() reconnects with the stored ones, and the session ends with the
 * client's window. The Lua specs cannot reach any of it, because opening a
 * client is the one thing the Lua API has no way to undo.
 *
 * Run with: ctest -R IrcLuaClientTest -V
 */

#include <IrcConnection>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TIrcClient.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "dlgIRC.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Keeps every line each connection sends, and speaks to the newest one.
// IrcConnection acts on what arrives whatever state it is in, so a test
// scripts only the server lines it needs.
class IrcSessionServer : public QTcpServer
{
    Q_OBJECT

public:
    using QTcpServer::QTcpServer;

    int connectionCount() const { return mSockets.size(); }

    QList<QByteArray> lines(int connection) const
    {
        QList<QByteArray> result;
        for (const QByteArray& line : mReceived.value(connection).split('\n')) {
            const QByteArray trimmed = QByteArray(line).replace('\r', QByteArray());
            if (!trimmed.isEmpty()) {
                result.append(trimmed);
            }
        }
        return result;
    }

    bool sendLine(const QByteArray& line)
    {
        if (mSockets.isEmpty()) {
            return false;
        }
        QTcpSocket* socket = mSockets.last();
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
            return false;
        }
        socket->write(line + "\r\n");
        return socket->flush();
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        auto* socket = new QTcpSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            delete socket;
            return;
        }
        const int index = mSockets.size();
        mSockets.append(socket);
        mReceived.append(QByteArray());
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, index]() {
            mReceived[index].append(socket->readAll());
        });
    }

private:
    QList<QPointer<QTcpSocket>> mSockets;
    QList<QByteArray> mReceived;
};

class IrcLuaClientTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpGameServer = nullptr;
    IrcSessionServer* mpIrcServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("IrcLuaClient");
    const QString mNick = qsl("luabot");
    const QString mChannel = qsl("#lua");
    const QString mServerHost = qsl("127.0.0.1");
    const QString mServerName = qsl("irc.stub.test");

    bool runLua(const QString& code) { return mpHost->mLuaInterpreter.compileAndExecuteScript(code); }

    // What the Lua expression evaluates to, each value put through tostring()
    // and joined with "|", so nil and a second return value are visible too
    QString luaValues(const QString& expression)
    {
        const QString code = qsl("local function pack(...) return {n = select('#', ...), ...} end\n"
                                 "local values = pack(%1)\n"
                                 "local parts = {}\n"
                                 "for i = 1, values.n do parts[i] = tostring(values[i]) end\n"
                                 "ircProbe = table.concat(parts, '|')")
                                     .arg(expression);
        if (!runLua(code)) {
            return qsl("<Lua error>");
        }
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "ircProbe");
        const char* value = lua_tostring(L, -1);
        const QString result = value ? QString::fromUtf8(value) : QString();
        lua_pop(L, 1);
        return result;
    }

    bool storeSettings(const QString& nick, const QString& channel)
    {
        return luaValues(qsl("setIrcNick('%1')").arg(nick)) == qsl("true") && luaValues(qsl("setIrcServer('%1', %2)").arg(mServerHost).arg(mpIrcServer->serverPort())) == qsl("true|nil")
               && luaValues(qsl("setIrcChannels({'%1'})").arg(channel)) == qsl("true");
    }

    bool waitForConnection(int count)
    {
        return QTest::qWaitFor(
                [this, count]() {
                    return mpIrcServer->connectionCount() >= count;
                },
                5000);
    }

    bool waitForLine(int connection, const QByteArray& line)
    {
        return QTest::qWaitFor(
                [this, connection, line]() {
                    return mpIrcServer->lines(connection).contains(line);
                },
                5000);
    }

    bool waitForLua(const QString& expression, const QString& expected)
    {
        return QTest::qWaitFor(
                [this, expression, expected]() {
                    return luaValues(expression) == expected;
                },
                5000);
    }

    // Opened through Lua, connected to the stub and registered on it, which is
    // what makes the client send a QUIT rather than merely drop the socket
    bool openRegisteredClient()
    {
        if (!storeSettings(mNick, mChannel)) {
            return false;
        }
        const int connectionsBefore = mpIrcServer->connectionCount();
        if (luaValues(qsl("openIRC()")) != qsl("true") || !waitForConnection(connectionsBefore + 1)) {
            return false;
        }
        return welcome(mNick)
               && QTest::qWaitFor(
                       [this]() {
                           return mpHost->mpDlgIRC && mpHost->mpDlgIRC->ircBrowser->toPlainText().contains(qsl("! Connected to"));
                       },
                       5000);
    }

    // The auto-join is queued until the server welcomes the client
    bool welcome(const QString& nick) { return mpIrcServer->sendLine(qsl(":%1 001 %2 :Welcome").arg(mServerName, nick).toUtf8()); }

    // The list holds the server buffer and one per channel, in no promised
    // order, so a row is found by the buffer it carries rather than by its index
    bool showBuffer(const QString& title)
    {
        QAbstractItemModel* model = mpHost->mpDlgIRC->bufferList->model();
        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, 0);
            auto* buffer = index.data(Irc::BufferRole).value<IrcBuffer*>();
            if (buffer && !buffer->title().compare(title, Qt::CaseInsensitive)) {
                mpHost->mpDlgIRC->bufferList->setCurrentIndex(index);
                return true;
            }
        }
        return false;
    }

    QString shownText() const { return mpHost->mpDlgIRC ? mpHost->mpDlgIRC->ircBrowser->toPlainText() : QString(); }

    QString windowTitle() const { return mpHost->mpDlgIRC ? mpHost->mpDlgIRC->windowTitle() : QString(); }

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

        mpIrcServer = new IrcSessionServer(qApp);
        QVERIFY2(mpIrcServer->listen(QHostAddress::LocalHost, 0), "The IRC stub did not start listening");

        mpHost = TestProfile::create(mProfileName, qsl("localhost"), QString::number(mpGameServer->serverPort()));
        QVERIFY2(mpHost, "Profile took too long to load");

        QVERIFY(runLua(qsl("ircEvents = {}\n"
                           "registerAnonymousEventHandler('sysIrcMessage', function(_, from, to, text) ircEvents[#ircEvents + 1] = from .. '>' .. to .. ':' .. text end)")));
    }

    void cleanup()
    {
        if (mpHost && mpHost->mpDlgIRC) {
            delete mpHost->mpDlgIRC;
            // lets the stub read the QUIT before the next test looks at its lines
            QTest::qWait(100);
        }
        if (mpHost) {
            runLua(qsl("ircEvents = {}"));
        }
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mProfileName);
            delete mudlet::self();
            QDir(profilePath).removeRecursively();
        }
        delete mpGameServer;
        mpGameServer = nullptr;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_withoutAClientTheStoredSettingsAreReported()
    {
        QVERIFY(storeSettings(qsl("storedNick"), qsl("#stored")));
        QVERIFY(!mpHost->mpDlgIRC);

        QCOMPARE(luaValues(qsl("getIrcNick()")), qsl("storedNick"));
        QCOMPARE(luaValues(qsl("getIrcServer()")), qsl("%1|%2|false").arg(mServerHost).arg(mpIrcServer->serverPort()));
        QCOMPARE(luaValues(qsl("table.concat(getIrcChannels(), ',')")), qsl("#stored"));
        QCOMPARE(luaValues(qsl("getIrcConnectedHost()")), qsl("false|no client active"));
        QCOMPARE(luaValues(qsl("restartIrc()")), qsl("false"));
    }

    void test_openIrcShowsAClientWithTheStoredSettings()
    {
        QVERIFY(storeSettings(mNick, mChannel));
        const int connectionsBefore = mpIrcServer->connectionCount();

        QCOMPARE(luaValues(qsl("openIRC()")), qsl("true"));
        QVERIFY2(mpHost->mpDlgIRC, "openIRC() did not open the IRC window");
        QVERIFY(mpHost->mpDlgIRC->isVisible());
        QCOMPARE(windowTitle(), qsl("Mudlet IRC Client - %1 - %2 on %3").arg(mProfileName, mNick, mServerHost));

        QVERIFY2(waitForConnection(connectionsBefore + 1), "the client never reached the stub server");
        QVERIFY2(waitForLine(connectionsBefore, qsl("NICK %1").arg(mNick).toUtf8()), "the stored nick was not the one registered");
        QVERIFY(welcome(mNick));
        QVERIFY2(waitForLine(connectionsBefore, qsl("JOIN %1").arg(mChannel).toUtf8()), "the stored channel was not joined");
        QVERIFY(shownText().contains(qsl("$ Nick: %1").arg(mNick)));

        // a second call shows the same client rather than opening another
        dlgIRC* first = mpHost->mpDlgIRC;
        QCOMPARE(luaValues(qsl("openIRC()")), qsl("true"));
        QCOMPARE(mpHost->mpDlgIRC.data(), first);
        QTest::qWait(100);
        QCOMPARE(mpIrcServer->connectionCount(), connectionsBefore + 1);
    }

    // What the client is using, not what has been stored since
    void test_theGettersReadTheRunningClient()
    {
        QVERIFY(openRegisteredClient());

        QVERIFY(storeSettings(qsl("laterNick"), qsl("#later")));
        QVERIFY(luaValues(qsl("setIrcServer('irc.later.invalid', 7000, true)")) == qsl("true|nil"));

        QCOMPARE(luaValues(qsl("getIrcNick()")), mNick);
        QCOMPARE(luaValues(qsl("getIrcServer()")), qsl("%1|%2|false").arg(mServerHost).arg(mpIrcServer->serverPort()));
        QCOMPARE(luaValues(qsl("table.concat(getIrcChannels(), ',')")), mChannel);
        QCOMPARE(luaValues(qsl("getIrcConnectedHost()")), qsl("false|not yet connected"));

        QVERIFY(mpIrcServer->sendLine(qsl(":%1 002 %2 :Your host is %1").arg(mServerName, mNick).toUtf8()));
        QVERIFY2(waitForLua(qsl("getIrcConnectedHost()"), qsl("true|%1").arg(mServerName)), qPrintable(luaValues(qsl("getIrcConnectedHost()"))));
    }

    void test_joinsAndPartsAreTrackedAndReported()
    {
        QVERIFY(openRegisteredClient());
        QCOMPARE(luaValues(qsl("sendIrc('%1', 'too soon')").arg(mChannel)), qsl("nil|not ready to send just yet"));

        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h JOIN %2").arg(mNick, mChannel).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h JOIN #extra").arg(mNick).toUtf8()));
        QVERIFY2(waitForLua(qsl("table.concat(getIrcChannels(), ',')"), qsl("%1,#extra").arg(mChannel)), qPrintable(luaValues(qsl("table.concat(getIrcChannels(), ',')"))));
        QVERIFY2(luaValues(qsl("table.concat(ircEvents, '\\n')")).contains(qsl("%1>#extra:").arg(mNick)), qPrintable(luaValues(qsl("table.concat(ircEvents, '\\n')"))));

        const int connection = mpIrcServer->connectionCount() - 1;
        QCOMPARE(luaValues(qsl("sendIrc('%1', 'now then')").arg(mChannel)), qsl("true"));
        QVERIFY(waitForLine(connection, qsl("PRIVMSG %1 :now then").arg(mChannel).toUtf8()));
        // the server does not echo it, so the client shows it itself
        QVERIFY2(shownText().contains(qsl("now then")), qPrintable(shownText()));

        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h PART #extra").arg(mNick).toUtf8()));
        QVERIFY2(waitForLua(qsl("table.concat(getIrcChannels(), ',')"), mChannel), qPrintable(luaValues(qsl("table.concat(getIrcChannels(), ',')"))));
    }

    void test_aNickChangeIsTrackedAndReported()
    {
        QVERIFY(openRegisteredClient());

        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h NICK :renamedbot").arg(mNick).toUtf8()));
        QVERIFY2(waitForLua(qsl("getIrcNick()"), qsl("renamedbot")), qPrintable(luaValues(qsl("getIrcNick()"))));
        QCOMPARE(windowTitle(), qsl("Mudlet IRC Client - %1 - renamedbot on %2").arg(mProfileName, mServerHost));
        QVERIFY2(luaValues(qsl("table.concat(ircEvents, '\\n')")).contains(qsl("%1>renamedbot:Your nick has changed.").arg(mNick)), qPrintable(luaValues(qsl("table.concat(ircEvents, '\\n')"))));
    }

    void test_aReservedNickIsReplaced()
    {
        QVERIFY(storeSettings(mNick, mChannel));
        const int connection = mpIrcServer->connectionCount();
        QCOMPARE(luaValues(qsl("openIRC()")), qsl("true"));
        QVERIFY(waitForConnection(connection + 1));

        QVERIFY(mpIrcServer->sendLine(qsl(":%1 433 * %2 :Nickname is already in use").arg(mServerName, mNick).toUtf8()));
        QVERIFY2(QTest::qWaitFor(
                         [this, connection]() {
                             for (const QByteArray& line : mpIrcServer->lines(connection)) {
                                 if (line.startsWith(qsl("NICK %1_").arg(mNick).toUtf8())) {
                                     return true;
                                 }
                             }
                             return false;
                         },
                         5000),
                 "no replacement nick was sent");
        QVERIFY(shownText().contains(qsl("! The Nickname %1 is reserved. Automatically changing Nickname to: %1_").arg(mNick)));
    }

    void test_restartIrcReconnectsWithTheStoredSettings()
    {
        QVERIFY(openRegisteredClient());
        const int oldConnection = mpIrcServer->connectionCount() - 1;
        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h JOIN %2").arg(mNick, mChannel).toUtf8()));
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return mpHost->mpDlgIRC && mpHost->mpDlgIRC->bufferList->model()->rowCount() == 2;
                },
                5000));

        // otherwise the line lands in the channel's document, which the restart takes away
        QVERIFY(showBuffer(mServerHost));

        QVERIFY(storeSettings(qsl("restartedbot"), qsl("#restarted")));
        QCOMPARE(luaValues(qsl("restartIrc()")), qsl("true"));

        QVERIFY2(waitForLine(oldConnection, "QUIT :Restarting IRC Client"), "the old connection was not quit");
        QVERIFY(shownText().contains(qsl("! Restarting IRC Client.")));
        // the channel's buffer goes; the server's stays
        QCOMPARE(mpHost->mpDlgIRC->bufferList->model()->rowCount(), 1);
        QCOMPARE(windowTitle(), qsl("Mudlet IRC Client - %1 - restartedbot on %2").arg(mProfileName, mServerHost));
        QCOMPARE(luaValues(qsl("getIrcNick()")), qsl("restartedbot"));
        QCOMPARE(luaValues(qsl("table.concat(getIrcChannels(), ',')")), qsl("#restarted"));

        QVERIFY2(waitForConnection(oldConnection + 2), "the client did not reconnect");
        QVERIFY(waitForLine(oldConnection + 1, "NICK restartedbot"));
        QVERIFY(welcome(qsl("restartedbot")));
        QVERIFY(waitForLine(oldConnection + 1, "JOIN #restarted"));
    }

    void test_sendIrcOpensAClientThatIsNotReadyYet()
    {
        QVERIFY(storeSettings(mNick, mChannel));
        QVERIFY(!mpHost->mpDlgIRC);
        const int connectionsBefore = mpIrcServer->connectionCount();

        QCOMPARE(luaValues(qsl("sendIrc('%1', 'first')").arg(mChannel)), qsl("nil|not ready to send just yet"));
        QVERIFY2(mpHost->mpDlgIRC, "sendIrc() did not open the IRC window");
        QVERIFY(mpHost->mpDlgIRC->isVisible());
        QVERIFY2(waitForConnection(connectionsBefore + 1), "the client never reached the stub server");
    }

    // The session belongs to the window: once that is gone, Lua sees no client
    void test_closingTheWindowEndsTheSession()
    {
        QVERIFY(openRegisteredClient());
        const int connection = mpIrcServer->connectionCount() - 1;
        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h NICK :closingbot").arg(mNick).toUtf8()));
        QVERIFY(waitForLua(qsl("getIrcNick()"), qsl("closingbot")));

        delete mpHost->mpDlgIRC;
        QVERIFY(!mpHost->mpDlgIRC);

        QVERIFY2(waitForLine(connection, "QUIT :closingbot closed their client."), "closing the window did not quit the server");
        QCOMPARE(luaValues(qsl("getIrcConnectedHost()")), qsl("false|no client active"));
        QCOMPARE(luaValues(qsl("restartIrc()")), qsl("false"));
        QCOMPARE(luaValues(qsl("getIrcNick()")), mNick);
    }

    // Ending the session disconnects it, which the window must not hear about
    // while it is being destroyed
    void test_closingTheWindowRunsNoneOfItsSlots()
    {
        QVERIFY(openRegisteredClient());
        dlgIRC* window = mpHost->mpDlgIRC;
        QVERIFY2(window->isVisible(), "SETUP: the window was never shown");
        QPointer<QTextDocument> shownDocument = window->ircBrowser->document();
        int changes = 0;
        QObject probe;
        connect(shownDocument, &QTextDocument::contentsChanged, &probe, [&changes]() {
            ++changes;
        });
        // Connected after the window's own, so each runs after the window's slot would
        TIrcClient* session = mpHost->getOrCreateIrcClient();
        bool sessionDisconnected = false;
        QString textAtDisconnect;
        connect(session->connection(), &IrcConnection::disconnected, &probe, [&]() {
            sessionDisconnected = true;
            textAtDisconnect = window->ircBrowser->toPlainText();
        });
        bool sessionEnded = false;
        bool visibleAtSessionEnd = false;
        connect(session, &QObject::destroyed, &probe, [&]() {
            sessionEnded = true;
            visibleAtSessionEnd = window->isVisible();
        });

        delete window;

        QVERIFY2(sessionDisconnected, "SETUP: closing the window did not disconnect the session");
        QVERIFY2(sessionEnded, "SETUP: closing the window did not end the session");
        QVERIFY2(!shownDocument, "SETUP: the shown document outlived its session");
        QCOMPARE(changes, 0);
        QVERIFY2(!textAtDisconnect.contains(qsl("! Disconnected")), "the window heard its session disconnect");
        QVERIFY2(visibleAtSessionEnd, "the window heard its session go");
    }

    // With no frontend to start it, openIRC() leaves a session that never
    // connected, and restarting it must not connect it behind no window. Last,
    // since it takes the frontend away for good.
    void test_restartIrcLeavesASessionWithNoFrontendClosed()
    {
        QVERIFY(storeSettings(mNick, mChannel));
        QVERIFY(!mpHost->mpDlgIRC);
        QObject::disconnect(mpHost, &Host::signal_showIrcClient, nullptr, nullptr);
        const int connectionsBefore = mpIrcServer->connectionCount();

        QCOMPARE(luaValues(qsl("openIRC()")), qsl("true"));
        QVERIFY2(!mpHost->mpDlgIRC, "SETUP: something still opened the IRC window");
        QVERIFY2(mpHost->mpIrcClient, "SETUP: openIRC() made no session");

        QCOMPARE(luaValues(qsl("restartIrc()")), qsl("false"));
        QTest::qWait(500);
        QCOMPARE(mpIrcServer->connectionCount(), connectionsBefore);
    }
};

#include "IrcLuaClientTest.moc"
MUDLET_GROUPED_TEST_MAIN(IrcLuaClientTest)
