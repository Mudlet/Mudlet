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
 * The IRC window is a rich-text browser, so every piece of text dlgIRC puts in
 * it that it did not write itself - the profile's IRC settings, a nick a server
 * says is reserved, what the user typed - has to be escaped, or markup in it is
 * rendered rather than shown. And a link in it opens only if it is a web page:
 * communi turns anything that looks like "scheme:" into one, and the browser
 * must not load or hand any of the others to the desktop. None of this has a
 * Lua entry point: the Lua IRC surface never reads the window's text or clicks
 * in it.
 *
 * Run with: ctest -R IrcWindowMarkupTest -V
 */

#include <QDesktopServices>
#include <QPointer>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgIRC.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Accepts the IRC client's connection and says what a test tells it to.
// IrcConnection acts on what arrives whatever state it is in, so no
// registration handshake is needed.
class IrcMarkupServer : public QTcpServer
{
    Q_OBJECT

public:
    using QTcpServer::QTcpServer;

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
        connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

private:
    QPointer<QTcpSocket> mSocket;
    int mConnectionCount = 0;
};

class IrcWindowMarkupTest : public QObject
{
    Q_OBJECT

public slots:
    // Stands in for the desktop for every scheme a test clicks, so nothing is
    // ever really opened and what would have been is on record
    void recordOpenedUrl(const QUrl& url) { mOpenedUrls << url; }

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpGameServer = nullptr;
    IrcMarkupServer* mpIrcServer = nullptr;
    Host* mpHost = nullptr;
    QList<QUrl> mOpenedUrls;
    const QString mProfileName = qsl("IrcWindowMarkup");
    const QString mMarkupNick = qsl("<i>nick</i>");
    const QString mMarkupChannel = qsl("#<b>chan</b>");
    const QStringList mSchemes{qsl("http"), qsl("https"), qsl("file"), qsl("javascript"), qsl("mailto"), qsl("ftp"), qsl("smb"), qsl("data")};

    // The dialog reads the profile's IRC settings in its constructor, and
    // showing it is what starts the client
    dlgIRC* openClient(const QString& host, const QString& nick, const QString& channel)
    {
        if (!dlgIRC::writeIrcHostName(mpHost, host).first || !dlgIRC::writeIrcHostPort(mpHost, mpIrcServer->serverPort()).first || !dlgIRC::writeIrcNickName(mpHost, nick).first
            || !dlgIRC::writeIrcChannels(mpHost, QStringList() << channel).first) {
            return nullptr;
        }
        mpHost->mpDlgIRC = new dlgIRC(mpHost);
        mpHost->mpDlgIRC->show();
        return mpHost->mpDlgIRC;
    }

    dlgIRC* openConnectedClient()
    {
        const int connectionsBefore = mpIrcServer->connectionCount();
        dlgIRC* client = openClient(qsl("127.0.0.1"), mMarkupNick, mMarkupChannel);
        if (!client
            || !QTest::qWaitFor(
                    [this, connectionsBefore]() {
                        return mpIrcServer->connectionCount() > connectionsBefore;
                    },
                    5000)) {
            return nullptr;
        }
        return client;
    }

    QString shownText(dlgIRC* client) const { return client->ircBrowser->toPlainText(); }

    // Markup that was rendered rather than escaped leaves only its inner text
    // behind, so finding the tags themselves in the plain text is the proof
    bool waitForShownText(dlgIRC* client, const QString& needle)
    {
        return QTest::qWaitFor(
                [this, client, needle]() {
                    return shownText(client).contains(needle);
                },
                5000);
    }

    void enterText(dlgIRC* client, const QString& text)
    {
        client->lineEdit->setText(text);
        QTest::keyClick(client->lineEdit, Qt::Key_Return);
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

        mpIrcServer = new IrcMarkupServer(qApp);
        QVERIFY2(mpIrcServer->listen(QHostAddress::LocalHost, 0), "The IRC stub did not start listening");

        mpHost = TestProfile::create(mProfileName, qsl("localhost"), QString::number(mpGameServer->serverPort()));
        QVERIFY2(mpHost, "Profile took too long to load");

        for (const QString& scheme : mSchemes) {
            QDesktopServices::setUrlHandler(scheme, this, "recordOpenedUrl");
        }
    }

    void cleanup()
    {
        mOpenedUrls.clear();
        if (mpHost && mpHost->mpDlgIRC) {
            delete mpHost->mpDlgIRC;
            // ~dlgIRC() sends a QUIT and drops the socket, which the stub has to
            // notice before the next test looks for a connection of its own
            QTest::qWait(100);
        }
    }

    void cleanupTestCase()
    {
        for (const QString& scheme : mSchemes) {
            QDesktopServices::unsetUrlHandler(scheme);
        }
        if (mudlet::self()) {
            const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mProfileName);
            delete mudlet::self();
            QDir(profilePath).removeRecursively();
        }
        delete mpGameServer;
        mpGameServer = nullptr;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Any profile's IRC settings can be set from a script with setIrc*()
    void test_settingsShownOnStartAreText()
    {
        dlgIRC* client = openConnectedClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(waitForShownText(client, qsl("$ Nick: %1").arg(mMarkupNick)), qPrintable(shownText(client)));
        QVERIFY2(shownText(client).contains(qsl("$ Auto-Join Channels: %1").arg(mMarkupChannel)), qPrintable(shownText(client)));

        QVERIFY(mpIrcServer->sendLine(qsl(":irc.test 001 %1 :Welcome").arg(mMarkupNick).toUtf8()));
        QVERIFY2(waitForShownText(client, qsl("! Joining %1...").arg(mMarkupChannel)), qPrintable(shownText(client)));
    }

    // The host never resolves, so this is the only line that shows it
    void test_hostShownOnStartIsText()
    {
        const QString host = qsl("<b>host</b>");
        dlgIRC* client = openClient(host, qsl("nick"), qsl("#chan"));
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY2(waitForShownText(client, qsl("$ Host: %1:").arg(host)), qPrintable(shownText(client)));
    }

    // The nick in the reply is the one the server says, not necessarily ours
    void test_reservedNickNameIsText()
    {
        dlgIRC* client = openConnectedClient();
        QVERIFY2(client, "the IRC client could not be opened");
        QVERIFY(mpIrcServer->sendLine(qsl(":irc.test 433 * %1 :Nickname is already in use").arg(mMarkupNick).toUtf8()));
        QVERIFY2(waitForShownText(client, qsl("The Nickname %1 is reserved").arg(mMarkupNick)), qPrintable(shownText(client)));
    }

    void test_refusedMessageReasonIsText()
    {
        dlgIRC* client = openConnectedClient();
        QVERIFY2(client, "the IRC client could not be opened");
        enterText(client, qsl("/msg <b>a</b>,,b hi"));
        QVERIFY2(waitForShownText(client, qsl("Could not send that message: target \"<b>a</b>,,b\"")), qPrintable(shownText(client)));
    }

    // A click must neither load the link into the window nor, unless it is a
    // web page, hand it to the desktop
    void test_onlyWebLinksOpen_data()
    {
        QTest::addColumn<QString>("link");
        QTest::addColumn<bool>("opens");

        QTest::newRow("https") << qsl("https://example.org/page") << true;
        QTest::newRow("http") << qsl("http://example.org/?a=1&b=2") << true;
        QTest::newRow("local file") << qsl("file:///etc/passwd") << false;
        QTest::newRow("network share") << qsl("smb://host/share") << false;
        QTest::newRow("script") << qsl("javascript:alert(1)") << false;
        QTest::newRow("data") << qsl("data:text/html,<b>x</b>") << false;
        QTest::newRow("e-mail") << qsl("mailto:bob@example.org") << false;
        QTest::newRow("ftp") << qsl("ftp://ftp.example.org/x") << false;
        QTest::newRow("web page without a host") << qsl("http:///x") << false;
    }

    void test_onlyWebLinksOpen()
    {
        QFETCH(QString, link);
        QFETCH(bool, opens);

        dlgIRC* client = openClient(qsl("<b>host</b>"), qsl("nick"), qsl("#chan"));
        QVERIFY2(client, "the IRC client could not be opened");
        const QUrl url(link);
        emit client->ircBrowser->anchorClicked(url);
        QCOMPARE(mOpenedUrls, opens ? QList<QUrl>{url} : QList<QUrl>{});

        // emitting the signal skips the browser's own handling of a click,
        // which with openLinks set would load the link into the window
        QVERIFY2(!client->ircBrowser->openLinks(), "the browser would load any link into the window itself");
    }
};

#include "IrcWindowMarkupTest.moc"
MUDLET_GROUPED_TEST_MAIN(IrcWindowMarkupTest)
