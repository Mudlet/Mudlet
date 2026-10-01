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
 * restartIrc() reconnects with the stored ones, what arrives from the server
 * reaches both the sysIrcMessage event and the window, and the session ends
 * with the client's window. The Lua specs cannot reach any of it, because
 * opening a client is the one thing the Lua API has no way to undo.
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

    // As openRegisteredClient(), and joined to the channel, with the events
    // raised on the way there forgotten
    bool openJoinedClient()
    {
        if (!openRegisteredClient() || !mpIrcServer->sendLine(qsl(":%1!u@h JOIN %2").arg(mNick, mChannel).toUtf8())) {
            return false;
        }
        if (!QTest::qWaitFor(
                    [this]() {
                        return bufferTitles().contains(mChannel);
                    },
                    5000)) {
            return false;
        }
        return runLua(qsl("ircEvents = {}"));
    }

    // As openJoinedClient(), with no frontend to show a window
    bool openHeadlessJoinedClient()
    {
        QObject::disconnect(mpHost, &Host::signal_showIrcClient, nullptr, nullptr);
        if (!storeSettings(mNick, mChannel)) {
            return false;
        }
        const int connection = mpIrcServer->connectionCount();
        if (luaValues(qsl("openIRC()")) != qsl("true") || mpHost->mpDlgIRC || !waitForConnection(connection + 1)) {
            return false;
        }
        if (!welcome(mNick) || !waitForLine(connection, qsl("JOIN %1").arg(mChannel).toUtf8()) || !mpIrcServer->sendLine(qsl(":%1!u@h JOIN %2").arg(mNick, mChannel).toUtf8())) {
            return false;
        }
        return waitForJoinEvent() && runLua(qsl("ircEvents = {}"));
    }

    // The channel list names the stored channels before any JOIN, so it can't
    // tell that the server's JOIN has arrived; our own join reaching Lua can
    bool waitForJoinEvent()
    {
        return QTest::qWaitFor(
                [this]() {
                    return events().contains(qsl("%1>%2:! You have joined %2 as %1").arg(mNick, mChannel));
                },
                5000);
    }

    QStringList bufferTitles() const
    {
        QStringList titles;
        if (!mpHost->mpDlgIRC) {
            return titles;
        }
        QAbstractItemModel* model = mpHost->mpDlgIRC->bufferList->model();
        for (int row = 0; model && row < model->rowCount(); ++row) {
            if (auto* buffer = model->index(row, 0).data(Irc::BufferRole).value<IrcBuffer*>()) {
                titles << buffer->title();
            }
        }
        // The list is in no promised order
        titles.sort();
        return titles;
    }

    // Every sysIrcMessage since the events were last forgotten, one
    // "from>to:text" per line
    QString events() { return luaValues(qsl("table.concat(ircEvents, '\\n')")); }

    bool waitForEvents(const QString& expected)
    {
        return QTest::qWaitFor(
                [this, expected]() {
                    return events() == expected;
                },
                5000);
    }

    // What typing a line into the window does
    bool typeLine(const QString& line)
    {
        mpHost->mpDlgIRC->lineEdit->setText(line);
        return QMetaObject::invokeMethod(mpHost->mpDlgIRC, "slot_onTextEntered");
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
        if (mpHost && mpHost->mpIrcClient) {
            delete mpHost->mpIrcClient.data();
            QTest::qWait(100);
        }
        if (mpHost) {
            runLua(qsl("ircEvents = {}\n"
                       "if ircReplyHandler then killAnonymousEventHandler(ircReplyHandler) ircReplyHandler = nil end"));
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
        // the server does not echo it, so the client shows it itself, but it is
        // not news to Lua
        QVERIFY2(shownText().contains(qsl("now then")), qPrintable(shownText()));
        QVERIFY2(!events().contains(qsl("now then")), qPrintable(events()));

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

    void test_channelAndQueryMessagesReachLuaAndTheirBuffers()
    {
        QVERIFY(openJoinedClient());

        QVERIFY(mpIrcServer->sendLine(qsl(":alice!u@h PRIVMSG %1 :hello there").arg(mChannel).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":alice!u@h NOTICE %1 :heads up").arg(mChannel).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":bob!u@h JOIN %1").arg(mChannel).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":alice!u@h PRIVMSG %1 :psst").arg(mNick).toUtf8()));

        // a private message is addressed to us, not to the query buffer it opens
        const QString expected = qsl("alice>%1:hello there\n"
                                     "alice>%1:heads up\n"
                                     "bob>%1:! bob has joined %1\n"
                                     "alice>%2:psst")
                                         .arg(mChannel, mNick);
        QVERIFY2(waitForEvents(expected), qPrintable(events()));
        QStringList expectedTitles{mServerHost, mChannel, qsl("alice")};
        expectedTitles.sort();
        QCOMPARE(bufferTitles(), expectedTitles);

        QVERIFY(showBuffer(mChannel));
        QVERIFY2(shownText().contains(qsl("<alice> hello there")), qPrintable(shownText()));
        QVERIFY2(shownText().contains(qsl("<alice> [%1] heads up").arg(mChannel)), qPrintable(shownText()));
        QVERIFY2(shownText().contains(qsl("! bob has joined %1").arg(mChannel)), qPrintable(shownText()));
        QVERIFY2(!shownText().contains(qsl("psst")), qPrintable(shownText()));
        QVERIFY(showBuffer(qsl("alice")));
        QVERIFY2(shownText().contains(qsl("<alice> psst")), qPrintable(shownText()));
    }

    // Each channel the quitter shared with us reports it
    void test_aQuitIsReportedOncePerSharedChannel()
    {
        QVERIFY(openJoinedClient());
        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h JOIN #extra").arg(mNick).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":bob!u@h JOIN %1").arg(mChannel).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(":bob!u@h JOIN #extra"));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return events().contains(qsl("bob>#extra:"));
                         },
                         5000),
                 qPrintable(events()));
        QVERIFY(runLua(qsl("ircEvents = {}")));

        QVERIFY(mpIrcServer->sendLine(":bob!u@h QUIT :gone"));
        QVERIFY2(waitForEvents(qsl("bob>%1:! bob has quit (gone)\nbob>#extra:! bob has quit (gone)").arg(mChannel)), qPrintable(events()));
    }

    // The window copies the line to the server buffer when the channel's goes,
    // but Lua hears of it once
    void test_beingKickedIsReportedOnce()
    {
        QVERIFY(openJoinedClient());

        QVERIFY(mpIrcServer->sendLine(qsl(":op!u@h KICK %1 %2 :bye").arg(mChannel, mNick).toUtf8()));
        QVERIFY2(waitForEvents(qsl("op>%1:! op kicked %2 from %1 (bye)").arg(mChannel, mNick)), qPrintable(events()));
        QTest::qWait(100);
        QCOMPARE(events(), qsl("op>%1:! op kicked %2 from %1 (bye)").arg(mChannel, mNick));
        QCOMPARE(bufferTitles(), QStringList({mServerHost}));
    }

    // The event is raised before the line is shown, so an answer a script sends
    // from its handler is on screen first
    void test_aScriptsReplyIsShownBeforeTheLineItAnswers()
    {
        QVERIFY(openJoinedClient());
        const int connection = mpIrcServer->connectionCount() - 1;
        QVERIFY(runLua(qsl("ircReplyHandler = registerAnonymousEventHandler('sysIrcMessage', function(_, from, to, text)\n"
                           "  if text == 'ping-me' then sendIrc('%1', 'pong-you') end\n"
                           "end)")
                               .arg(mChannel)));

        QVERIFY(mpIrcServer->sendLine(qsl(":alice!u@h PRIVMSG %1 :ping-me").arg(mChannel).toUtf8()));
        QVERIFY2(waitForLine(connection, qsl("PRIVMSG %1 :pong-you").arg(mChannel).toUtf8()), "the script's reply was not sent");

        QVERIFY(showBuffer(mChannel));
        const QString shown = shownText();
        const qsizetype reply = shown.indexOf(qsl("<%1> pong-you").arg(mNick));
        const qsizetype line = shown.indexOf(qsl("<alice> ping-me"));
        QVERIFY2(reply != -1 && line != -1, qPrintable(shown));
        QVERIFY2(reply < line, qPrintable(shown));
    }

    // Lines no buffer takes go to the server buffer, which is named for the
    // server once it says who it is, and for the configured host again after a
    // restart
    void test_serverMessagesAreNamedForTheServerBuffer()
    {
        QVERIFY(openRegisteredClient());
        QVERIFY(runLua(qsl("ircEvents = {}")));

        QVERIFY(mpIrcServer->sendLine(qsl(":%1 002 %2 :Your host is %1").arg(mServerName, mNick).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":%1 003 %2 :Created today").arg(mServerName, mNick).toUtf8()));
        // the 002 is reported before the client has read the name out of it
        const QString expected = qsl("%1>%2:[INFO] Your host is %1\n"
                                     "%1>%1:[INFO] Created today")
                                         .arg(mServerName, mServerHost);
        QVERIFY2(waitForEvents(expected), qPrintable(events()));
        QCOMPARE(bufferTitles(), QStringList({mServerName}));
        QVERIFY(showBuffer(mServerName));
        QVERIFY2(shownText().contains(qsl("[INFO] Created today")), qPrintable(shownText()));

        QCOMPARE(luaValues(qsl("restartIrc()")), qsl("true"));
        QCOMPARE(bufferTitles(), QStringList({mServerHost}));
    }

    // The reply is timed from when the ping was typed, not from when it arrived
    void test_aTypedPingIsTimedFromWhenItWasSent()
    {
        QVERIFY(openRegisteredClient());
        const int connection = mpIrcServer->connectionCount() - 1;
        QVERIFY(runLua(qsl("ircEvents = {}")));

        QVERIFY(typeLine(qsl("/ping %1").arg(mServerName)));
        QVERIFY(waitForLine(connection, qsl("PING %1").arg(mServerName).toUtf8()));
        QTest::qWait(300);
        QVERIFY(mpIrcServer->sendLine(qsl(":%1 PONG %1 :%1").arg(mServerName).toUtf8()));

        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return events().contains(qsl("replied in"));
                         },
                         5000),
                 qPrintable(events()));
        const QRegularExpression pong(qsl("^%1>%2:! %1 replied in (\\d+\\.\\d+) seconds$").arg(QRegularExpression::escape(mServerName), QRegularExpression::escape(mServerHost)));
        const QRegularExpressionMatch match = pong.match(events());
        QVERIFY2(match.hasMatch(), qPrintable(events()));
        QVERIFY2(match.captured(1).toDouble() >= 0.25, qPrintable(events()));
        QVERIFY2(shownText().contains(qsl("! %1 replied in ").arg(mServerName)), qPrintable(shownText()));
    }

    // Creating a session does not start it, so one nothing has started never connects
    void test_aSessionWithoutAWindowDoesNotConnect()
    {
        QVERIFY(storeSettings(mNick, mChannel));
        QVERIFY2(!mpHost->mpIrcClient, "SETUP: an earlier case left a session, which would be handed back rather than made");
        const int connectionsBefore = mpIrcServer->connectionCount();

        QPointer<TIrcClient> client = mpHost->getOrCreateIrcClient();
        QTest::qWait(300);
        QCOMPARE(mpIrcServer->connectionCount(), connectionsBefore);
        QVERIFY(!mpHost->mpDlgIRC);
        QCOMPARE(luaValues(qsl("getIrcConnectedHost()")), qsl("false|not yet connected"));
        QCOMPARE(events(), QString());

        delete client.data();
        QCOMPARE(luaValues(qsl("getIrcConnectedHost()")), qsl("false|no client active"));
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

    // Without a frontend the session runs with no window: connected, joined, and
    // reporting its channel's lines. These take the frontend away for good, so
    // they come last.
    void test_withNoFrontendOpenIrcRunsTheSession()
    {
        const int connection = mpIrcServer->connectionCount();
        QVERIFY(openHeadlessJoinedClient());

        QVERIFY(mpIrcServer->sendLine(qsl(":alice!u@h PRIVMSG %1 :hello there").arg(mChannel).toUtf8()));
        QVERIFY2(waitForEvents(qsl("alice>%1:hello there").arg(mChannel)), qPrintable(events()));

        QCOMPARE(luaValues(qsl("sendIrc('%1', 'hi back')").arg(mChannel)), qsl("true"));
        QVERIFY(waitForLine(connection, qsl("PRIVMSG %1 :hi back").arg(mChannel).toUtf8()));
    }

    void test_withNoFrontendRestartIrcReconnectsTheSession()
    {
        QVERIFY(openHeadlessJoinedClient());
        const int oldConnection = mpIrcServer->connectionCount() - 1;

        QCOMPARE(luaValues(qsl("restartIrc()")), qsl("true"));
        QVERIFY2(waitForLine(oldConnection, "QUIT :Restarting IRC Client"), "the old connection was not quit");
        QVERIFY2(waitForConnection(oldConnection + 2), "the client did not reconnect");
        QVERIFY(welcome(mNick));
        QVERIFY(waitForLine(oldConnection + 1, qsl("JOIN %1").arg(mChannel).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":%1!u@h JOIN %2").arg(mNick, mChannel).toUtf8()));
        QVERIFY2(waitForJoinEvent(), qPrintable(events()));
        QVERIFY(runLua(qsl("ircEvents = {}")));

        QVERIFY(mpIrcServer->sendLine(qsl(":alice!u@h PRIVMSG %1 :welcome back").arg(mChannel).toUtf8()));
        QVERIFY2(waitForEvents(qsl("alice>%1:welcome back").arg(mChannel)), qPrintable(events()));
    }

    void test_withNoFrontendSendIrcStartsTheSession()
    {
        QVERIFY(storeSettings(mNick, mChannel));
        QObject::disconnect(mpHost, &Host::signal_showIrcClient, nullptr, nullptr);
        const int connectionsBefore = mpIrcServer->connectionCount();

        QCOMPARE(luaValues(qsl("sendIrc('%1', 'first')").arg(mChannel)), qsl("nil|not ready to send just yet"));
        QVERIFY2(!mpHost->mpDlgIRC, "SETUP: something still opened the IRC window");
        QVERIFY2(waitForConnection(connectionsBefore + 1), "sendIrc() with no frontend did not connect");
    }

    // A listener that shows no window, such as this spy, is no frontend to start the session
    void test_withNoFrontendAnotherListenerLeavesTheSessionRunning()
    {
        QVERIFY(storeSettings(mNick, mChannel));
        QObject::disconnect(mpHost, &Host::signal_showIrcClient, nullptr, nullptr);
        QSignalSpy showRequests(mpHost, &Host::signal_showIrcClient);
        const int connectionsBefore = mpIrcServer->connectionCount();

        QCOMPARE(luaValues(qsl("openIRC()")), qsl("true"));
        QCOMPARE(showRequests.count(), 1);
        QVERIFY2(!mpHost->mpDlgIRC, "SETUP: something still opened the IRC window");
        QVERIFY2(waitForConnection(connectionsBefore + 1), "openIRC() with a listener that opens no window did not connect");
    }
};

#include "IrcLuaClientTest.moc"
MUDLET_GROUPED_TEST_MAIN(IrcLuaClientTest)
