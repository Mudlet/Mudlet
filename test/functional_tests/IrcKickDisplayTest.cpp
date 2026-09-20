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
 * Where a kick ends up in the IRC window - issue #10877. Being kicked yourself
 * makes libcommuni destroy the channel's buffer, and the document the kick line
 * was just written into goes with it, so the line has to be copied into the
 * server buffer to survive. No Lua spec can reach any of this: the Lua IRC
 * surface is the getIrc, setIrc, sendIrc and restartIrc functions, none of
 * which reads the window's text or closes the dialog again.
 *
 * Run with: ctest -R IrcKickDisplayTest -V
 */

#include <QPointer>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgIRC.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Accepts the IRC client's connection and can script the server side of it.
// IrcConnection parses and filters whatever arrives whatever state it is in, so
// no registration handshake is needed to make it act on a JOIN or a KICK.
class IrcScriptedServer : public QTcpServer
{
    Q_OBJECT

public:
    using QTcpServer::QTcpServer;

    // Each test opens a client of its own and the one before it left a socket
    // behind, so a test waits for a connection newer than the count it saw
    // rather than for a socket to merely exist
    int connectionCount() const { return mConnectionCount; }

    bool sendLine(const QByteArray& line)
    {
        if (mSocket.isNull() || mSocket->state() != QAbstractSocket::ConnectedState) {
            return false;
        }
        mSocket->write(line + "\r\n");
        return mSocket->flush();
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        auto* socket = new QTcpSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            delete socket;
            return;
        }
        mSocket = socket;
        ++mConnectionCount;
        // read and drop, so the client is never blocked on a full buffer
        connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

private:
    QPointer<QTcpSocket> mSocket;
    int mConnectionCount = 0;
};

class IrcKickDisplayTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpGameServer = nullptr;
    IrcScriptedServer* mpIrcServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("IrcKickDisplay");
    const QString mNick = qsl("kickbot");
    const QString mChannel = qsl("#kick");
    const QString mServerHost = qsl("127.0.0.1");

    // The dialog reads the profile's IRC settings in its constructor, so they
    // have to be stored before it is built.
    dlgIRC* openClient()
    {
        if (!dlgIRC::writeIrcHostName(mpHost, mServerHost).first || !dlgIRC::writeIrcHostPort(mpHost, mpIrcServer->serverPort()).first || !dlgIRC::writeIrcNickName(mpHost, mNick).first
            || !dlgIRC::writeIrcChannels(mpHost, QStringList() << mChannel).first) {
            return nullptr;
        }

        const int connectionsBefore = mpIrcServer->connectionCount();
        // built directly rather than through openIRC(), which is a Lua function
        mpHost->mpDlgIRC = new dlgIRC(mpHost);
        mpHost->mpDlgIRC->show();
        if (!QTest::qWaitFor(
                    [this, connectionsBefore]() {
                        return mpIrcServer->connectionCount() > connectionsBefore;
                    },
                    5000)) {
            return nullptr;
        }
        return mpHost->mpDlgIRC;
    }

    int bufferCount(dlgIRC* client) const { return client->bufferList->model()->rowCount(); }

    // The list holds the server buffer and one per channel, in no promised
    // order, so a row is found by the buffer it carries rather than by its index
    bool showBuffer(dlgIRC* client, const QString& title)
    {
        QAbstractItemModel* model = client->bufferList->model();
        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, 0);
            auto* buffer = index.data(Irc::BufferRole).value<IrcBuffer*>();
            if (buffer && !buffer->title().compare(title, Qt::CaseInsensitive)) {
                client->bufferList->setCurrentIndex(index);
                return true;
            }
        }
        return false;
    }

    QString shownText(dlgIRC* client) const { return client->ircBrowser->toPlainText(); }

    bool waitForShownText(dlgIRC* client, const QString& needle)
    {
        return QTest::qWaitFor(
                [this, client, needle]() {
                    return shownText(client).contains(needle);
                },
                5000);
    }

    bool waitForBufferCount(dlgIRC* client, int count)
    {
        return QTest::qWaitFor(
                [this, client, count]() {
                    return bufferCount(client) == count;
                },
                5000);
    }

    // Our own JOIN is what makes communi create the channel's buffer, and it is
    // only "own" when the nick matches the connection's exactly
    bool joinOwnChannel(dlgIRC* client) { return mpIrcServer->sendLine(qsl(":%1!u@h JOIN %2").arg(mNick, mChannel).toUtf8()) && waitForBufferCount(client, 2); }

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

        mpIrcServer = new IrcScriptedServer(qApp);
        QVERIFY2(mpIrcServer->listen(QHostAddress::LocalHost, 0), "The IRC stub did not start listening");

        mpHost = TestProfile::create(mProfileName, qsl("localhost"), QString::number(mpGameServer->serverPort()));
        QVERIFY2(mpHost, "Profile took too long to load");
    }

    void cleanup()
    {
        if (mpHost && mpHost->mpDlgIRC) {
            delete mpHost->mpDlgIRC;
            // ~dlgIRC() sends a QUIT and drops the socket, which the stub has to
            // notice before the next test looks for a connection of its own
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

    // The reported bug: the channel's tab goes, and with the line only ever
    // written there, nothing anywhere says what happened.
    void test_beingKickedLeavesTheLineInTheServerTab()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(joinOwnChannel(client), "the channel buffer was never created");

        // joining selects the channel's own tab, so the server document is the
        // one that is not on screen when the kick lands
        QVERIFY(mpIrcServer->sendLine(qsl(":op!u@h KICK %1 %2 :get out").arg(mChannel, mNick).toUtf8()));
        QVERIFY2(waitForBufferCount(client, 1), "the channel buffer outlived the kick, so this is no longer the case being tested");

        QVERIFY2(showBuffer(client, mServerHost), "the server buffer is not in the list");
        const QString shown = shownText(client);
        QCOMPARE(shown.count(qsl("! op kicked %1 from %2 (get out)").arg(mNick, mChannel)), 1);
    }

    // The other half of appendToDocument(): a document that is on screen is
    // written through the browser rather than directly
    void test_beingKickedWhileTheServerTabIsOnScreenShowsTheLineOnce()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(joinOwnChannel(client), "the channel buffer was never created");
        QVERIFY2(showBuffer(client, mServerHost), "the server buffer is not in the list");

        QVERIFY(mpIrcServer->sendLine(qsl(":op!u@h KICK %1 %2 :out you go").arg(mChannel, mNick).toUtf8()));
        QVERIFY2(waitForShownText(client, qsl("kicked %1 from %2 (out you go)").arg(mNick, mChannel)), qPrintable(qsl("the server tab says: %1").arg(shownText(client))));
        QCOMPARE(shownText(client).count(qsl("! op kicked %1 from %2 (out you go)").arg(mNick, mChannel)), 1);
    }

    // Somebody else being kicked belongs in the channel it happened in and
    // nowhere else - the server tab is not a log of every kick on the network
    void test_aKickOfSomebodyElseIsNotCopiedToTheServerTab()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(joinOwnChannel(client), "the channel buffer was never created");

        QVERIFY(mpIrcServer->sendLine(qsl(":bystander!u@h JOIN %1").arg(mChannel).toUtf8()));
        QVERIFY(mpIrcServer->sendLine(qsl(":op!u@h KICK %1 bystander :spam").arg(mChannel).toUtf8()));
        QVERIFY2(waitForShownText(client, qsl("kicked bystander")), qPrintable(qsl("the channel tab says: %1").arg(shownText(client))));
        QCOMPARE(shownText(client).count(qsl("! op kicked bystander from %1 (spam)").arg(mChannel)), 1);

        QVERIFY2(showBuffer(client, mServerHost), "the server buffer is not in the list");
        QVERIFY2(!shownText(client).contains(qsl("kicked bystander")), qPrintable(qsl("the server tab says: %1").arg(shownText(client))));
    }

    // A kick naming us in a channel we have no buffer for is not processed by
    // any buffer, so communi hands it to the server buffer itself. It is already
    // being shown there, and copying it would show it twice.
    void test_aKickInAChannelWeAreNotInIsShownOnce()
    {
        dlgIRC* client = openClient();
        QVERIFY2(client, "the IRC client could not be opened");

        QVERIFY(mpIrcServer->sendLine(qsl(":op!u@h KICK #never %1 :go").arg(mNick).toUtf8()));
        QVERIFY2(showBuffer(client, mServerHost), "the server buffer is not in the list");
        QVERIFY2(waitForShownText(client, qsl("kicked %1 from #never").arg(mNick)), qPrintable(qsl("the server tab says: %1").arg(shownText(client))));
        QCOMPARE(shownText(client).count(qsl("! op kicked %1 from #never (go)").arg(mNick)), 1);
    }
};

#include "IrcKickDisplayTest.moc"
MUDLET_GROUPED_TEST_MAIN(IrcKickDisplayTest)
