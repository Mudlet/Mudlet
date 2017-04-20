/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irc.h"
#include "irccommand.h"
#include "ircprotocol.h"
#include "ircconnection.h"
#include "ircmessage.h"
#include "ircfilter.h"
#include <QtTest/QtTest>
#include <QtCore/QRegExp>
#include <QtCore/QTextCodec>
#include <QtCore/QScopedPointer>
#ifndef QT_NO_SSL
#include <QtNetwork/QSslSocket>
#endif

#include "tst_ircdata.h"
#include "tst_ircclientserver.h"

class FriendlyConnection : public IrcConnection
{
    friend class tst_IrcConnection;
};

class TestProtocol : public IrcProtocol
{
public:
    TestProtocol(IrcConnection* connection) : IrcProtocol(connection)
    {
    }

    virtual bool write(const QByteArray& data)
    {
        written = data;
        return IrcProtocol::write(data);
    }

    QByteArray written;
};

class tst_IrcConnection : public tst_IrcClientServer
{
    Q_OBJECT

private slots:
    void testDefaults();

    void testHost_data();
    void testHost();

    void testPort_data();
    void testPort();

    void testUserName_data();
    void testUserName();

    void testNickName_data();
    void testNickName();

    void testRealName_data();
    void testRealName();

    void testPassword_data();
    void testPassword();

    void testDisplayName_data();
    void testDisplayName();

    void testEncoding_data();
    void testEncoding();

    void testSocket_data();
    void testSocket();

    void testSecure();
    void testSasl();
    void testNoSasl();
    void testSsl();

    void testOpen();
    void testEnabled();

    void testStatus();
    void testConnection();
    void testMessages();
    void testMessageFlags();
    void testStatusPrefixes();
    void testMessageComposer();
    void testMessageComposerCrash_data();
    void testMessageComposerCrash();
    void testBatch();
    void testServerTime();

    void testSendCommand();
    void testSendData();

    void testMessageFilter();
    void testCommandFilter();

    void testDebug();
    void testWarnings();

    void testCtcp();
    void testClone();
    void testSaveRestore();
    void testSignals();
    void testServers();
};

void tst_IrcConnection::testDefaults()
{
    IrcConnection connection;
    QVERIFY(connection.host().isNull());
    QCOMPARE(connection.port(), 6667);
    QVERIFY(connection.userName().isNull());
    QVERIFY(connection.nickName().isNull());
    QVERIFY(connection.realName().isNull());
    QVERIFY(connection.password().isNull());
    QVERIFY(connection.displayName().isNull());
    QCOMPARE(connection.encoding(), QByteArray("ISO-8859-15"));
    QCOMPARE(connection.status(), IrcConnection::Inactive);
    QVERIFY(!connection.isActive());
    QVERIFY(!connection.isConnected());
    QVERIFY(connection.isEnabled());
    QCOMPARE(connection.reconnectDelay(), 0);
    QVERIFY(connection.socket());
    QVERIFY(!connection.isSecure());
    QVERIFY(connection.saslMechanism().isNull());
    QVERIFY(!IrcConnection::supportedSaslMechanisms().isEmpty());
    QVERIFY(connection.network());
}

void tst_IrcConnection::testHost_data()
{
    QTest::addColumn<QString>("host");

    QTest::newRow("null") << QString();
    QTest::newRow("empty") << QString("");
    QTest::newRow("space") << QString(" ");
    QTest::newRow("invalid") << QString("invalid");
    QTest::newRow("local") << QString("127.0.0.1");
}

void tst_IrcConnection::testHost()
{
    QFETCH(QString, host);

    IrcConnection connection;
    QSignalSpy spy(&connection, SIGNAL(hostChanged(QString)));
    QVERIFY(spy.isValid());
    connection.setHost(host);
    QCOMPARE(connection.host(), host);
    QCOMPARE(spy.count(), !host.isEmpty() ? 1 : 0);
    if (!spy.isEmpty())
        QCOMPARE(spy.first().first().toString(), host);

    IrcConnection another(host);
    QCOMPARE(another.host(), host);
}

void tst_IrcConnection::testPort_data()
{
    QTest::addColumn<int>("port");

    QTest::newRow("-1") << -1;
    QTest::newRow("0") << 0;
    QTest::newRow("6666") << 6666;
    QTest::newRow("6667") << 6667;
    QTest::newRow("6668") << 6668;
}

void tst_IrcConnection::testPort()
{
    QFETCH(int, port);

    IrcConnection connection;
    QSignalSpy spy(&connection, SIGNAL(portChanged(int)));
    QVERIFY(spy.isValid());
    connection.setPort(port);
    QCOMPARE(connection.port(), port);
    QCOMPARE(spy.count(), port != 6667 ? 1 : 0);
    if (!spy.isEmpty())
        QCOMPARE(spy.first().first().toInt(), port);
}

void tst_IrcConnection::testUserName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("space") << QString(" ") << QString("");
    QTest::newRow("spaces") << QString(" foo bar ") << QString("foo");
}

void tst_IrcConnection::testUserName()
{
    QFETCH(QString, name);
    QFETCH(QString, result);

    IrcConnection connection;
    QSignalSpy spy(&connection, SIGNAL(userNameChanged(QString)));
    QVERIFY(spy.isValid());
    connection.setUserName(name);
    QCOMPARE(connection.userName(), result);
    QCOMPARE(spy.count(), !result.isEmpty() ? 1 : 0);
    if (!spy.isEmpty())
        QCOMPARE(spy.first().first().toString(), result);
}

void tst_IrcConnection::testNickName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("space") << QString(" ") << QString("");
    QTest::newRow("spaces") << QString(" foo bar ") << QString("foo");
}

void tst_IrcConnection::testNickName()
{
    QFETCH(QString, name);
    QFETCH(QString, result);

    IrcConnection connection;
    QSignalSpy spy(&connection, SIGNAL(nickNameChanged(QString)));
    QVERIFY(spy.isValid());
    connection.setNickName(name);
    QCOMPARE(connection.nickName(), result);
    QCOMPARE(spy.count(), !result.isEmpty() ? 1 : 0);
    if (!spy.isEmpty())
        QCOMPARE(spy.first().first().toString(), result);
}

void tst_IrcConnection::testRealName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("space") << QString(" ") << QString(" ");
    QTest::newRow("spaces") << QString(" foo bar ") << QString(" foo bar ");
}

void tst_IrcConnection::testRealName()
{
    QFETCH(QString, name);
    QFETCH(QString, result);

    IrcConnection connection;
    QSignalSpy spy(&connection, SIGNAL(realNameChanged(QString)));
    QVERIFY(spy.isValid());
    connection.setRealName(name);
    QCOMPARE(connection.realName(), result);
    QCOMPARE(spy.count(), !result.isEmpty() ? 1 : 0);
    if (!spy.isEmpty())
        QCOMPARE(spy.first().first().toString(), result);
}

void tst_IrcConnection::testPassword_data()
{
    QTest::addColumn<QString>("passwd");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("space") << QString(" ") << QString(" ");
    QTest::newRow("spaces") << QString(" foo bar ") << QString(" foo bar ");
}

void tst_IrcConnection::testPassword()
{
    QFETCH(QString, passwd);
    QFETCH(QString, result);

    IrcConnection connection;
    QSignalSpy spy(&connection, SIGNAL(passwordChanged(QString)));
    QVERIFY(spy.isValid());
    connection.setPassword(passwd);
    QCOMPARE(connection.password(), result);
    QCOMPARE(spy.count(), !result.isEmpty() ? 1 : 0);
    if (!spy.isEmpty())
        QCOMPARE(spy.first().first().toString(), result);
}

void tst_IrcConnection::testDisplayName_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString() << QString();
    QTest::newRow("empty") << QString() << QString("") << QString("");
    QTest::newRow("space") << QString() << QString(" ") << QString(" ");

    QTest::newRow("host") << QString("host") << QString() << QString("host");
    QTest::newRow("name") << QString() << QString("name") << QString("name");
    QTest::newRow("explicit") << QString("host") << QString("name") << QString("name");
}

void tst_IrcConnection::testDisplayName()
{
    QFETCH(QString, host);
    QFETCH(QString, name);
    QFETCH(QString, result);

    IrcConnection connection;
    connection.setHost(host);
    connection.setDisplayName(name);
    QCOMPARE(connection.displayName(), result);
}

void tst_IrcConnection::testEncoding_data()
{
    QTest::addColumn<QByteArray>("encoding");
    QTest::addColumn<QByteArray>("actual");
    QTest::addColumn<bool>("supported");

    QTest::newRow("null") << QByteArray() << QByteArray("ISO-8859-15") << false;
    QTest::newRow("empty") << QByteArray("") << QByteArray("ISO-8859-15") << false;
    QTest::newRow("space") << QByteArray(" ") << QByteArray("ISO-8859-15") << false;
    QTest::newRow("invalid") << QByteArray("invalid") << QByteArray("ISO-8859-15") << false;
    foreach (const QByteArray& codec, QTextCodec::availableCodecs())
        QTest::newRow(codec) << codec << codec << true;
}

void tst_IrcConnection::testEncoding()
{
    QFETCH(QByteArray, encoding);
    QFETCH(QByteArray, actual);
    QFETCH(bool, supported);

    if (!supported)
        QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setEncoding(): unsupported encoding \"" + encoding + "\" ");

    IrcConnection connection;
    connection.setEncoding(encoding);
    QCOMPARE(connection.encoding(), actual);
}

Q_DECLARE_METATYPE(QAbstractSocket*)
void tst_IrcConnection::testSocket_data()
{
    QTest::addColumn<QAbstractSocket*>("socket");

    QTest::newRow("null") << static_cast<QAbstractSocket*>(0);
    QTest::newRow("tcp") << static_cast<QAbstractSocket*>(new QTcpSocket(this));
#ifndef QT_NO_SSL
    QTest::newRow("ssl") << static_cast<QAbstractSocket*>(new QSslSocket(this));
#endif
}

void tst_IrcConnection::testSocket()
{
    QFETCH(QAbstractSocket*, socket);

    IrcConnection connection;
    connection.setSocket(socket);
    QCOMPARE(connection.socket(), socket);
    QCOMPARE(connection.isSecure(), socket && socket->inherits("QSslSocket"));
}

void tst_IrcConnection::testSecure()
{
    IrcConnection connection;
    QSignalSpy spy(&connection, SIGNAL(secureChanged(bool)));
    QVERIFY(spy.isValid());
    QVERIFY(!connection.isSecure());

#ifdef QT_NO_SSL
    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setSecure(): the Qt build does not support SSL");
#endif

    connection.setSecure(true);

#ifndef QT_NO_SSL
    QVERIFY(connection.isSecure());
    QVERIFY(connection.socket()->inherits("QSslSocket"));
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toBool());
#else
    QVERIFY(!connection.isSecure());
    QVERIFY(!connection.socket()->inherits("QSslSocket"));
    QCOMPARE(spy.count(), 0);
#endif

    connection.setSecure(false);
    QVERIFY(!connection.isSecure());
    QVERIFY(!connection.socket()->inherits("QSslSocket"));
#ifndef QT_NO_SSL
    QCOMPARE(spy.count(), 2);
    QVERIFY(!spy.last().last().toBool());
#else
    QCOMPARE(spy.count(), 0);
#endif
}

void tst_IrcConnection::testSasl()
{
    QVERIFY(!IrcConnection::supportedSaslMechanisms().contains("UNKNOWN"));
    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setSaslMechanism(): unsupported mechanism: 'UNKNOWN'");
    connection->setSaslMechanism("UNKNOWN");
    QVERIFY(connection->saslMechanism().isEmpty());

    IrcProtocol* protocol = static_cast<FriendlyConnection*>(connection.data())->protocol();
    QVERIFY(protocol);

    QVERIFY(IrcConnection::supportedSaslMechanisms().contains("PLAIN"));
    connection->setSaslMechanism("PLAIN");
    QCOMPARE(connection->saslMechanism(), QString("PLAIN"));

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));
    QByteArray written = serverSocket->readAll();
    QVERIFY(written.contains("CAP LS"));
    QVERIFY(written.contains("NICK nick"));
    QVERIFY(!written.contains("PASS secret"));
    QVERIFY(!written.contains("CAP REQ :sasl"));

    QVERIFY(waitForWritten(":irc.freenode.net CAP * LS :sasl"));
    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));
    written = serverSocket->readAll();
    QVERIFY(!written.contains("CAP LS"));
    QVERIFY(!written.contains("NICK nick"));
    QVERIFY(!written.contains("PASS secret"));
    QVERIFY(written.contains("CAP REQ :sasl"));

    // do not resume handshake too early
    QCoreApplication::sendPostedEvents(protocol, QEvent::MetaCall);
    QVERIFY(!clientSocket->waitForBytesWritten(1000));

    QVERIFY(waitForWritten(":irc.freenode.net CAP user ACK :sasl"));
    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));
    QVERIFY(serverSocket->readAll().contains("AUTHENTICATE PLAIN"));

    QVERIFY(waitForWritten("AUTHENTICATE +"));
    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));

    QByteArray response = serverSocket->readAll();
    int index = response.indexOf("AUTHENTICATE");
    QVERIFY(index != -1);
    QByteArray secret = response.mid(index + 13);
    index = secret.indexOf("\r\n");
    QVERIFY(index != -1);
    secret.truncate(index + 1);
    secret = QByteArray::fromBase64(secret);
    QByteArray expected = connection->userName().toUtf8() + '\0' +
                          connection->userName().toUtf8() + '\0' +
                          connection->password().toUtf8();
    QCOMPARE(secret, expected);

    // resume handshake
    QCoreApplication::sendPostedEvents(protocol, QEvent::MetaCall);

    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));
    QVERIFY(serverSocket->readAll().contains("CAP END"));

    // TODO:
    QVERIFY(waitForWritten(":irc.freenode.net 900 user nick!user@host nick :You are now logged in as user."));
    QVERIFY(waitForWritten(":irc.freenode.net 903 user :SASL authentication successful"));
    QVERIFY(waitForWritten(":irc.freenode.net 001 user :Welcome to the freenode Internet Relay Chat Network user"));
}

void tst_IrcConnection::testNoSasl()
{
    QVERIFY(!IrcConnection::supportedSaslMechanisms().contains("UNKNOWN"));
    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setSaslMechanism(): unsupported mechanism: 'UNKNOWN'");
    connection->setSaslMechanism("UNKNOWN");
    QVERIFY(connection->saslMechanism().isEmpty());

    IrcProtocol* protocol = static_cast<FriendlyConnection*>(connection.data())->protocol();
    QVERIFY(protocol);

    QVERIFY(IrcConnection::supportedSaslMechanisms().contains("PLAIN"));
    connection->setSaslMechanism("PLAIN");
    QCOMPARE(connection->saslMechanism(), QString("PLAIN"));

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));
    QByteArray written = serverSocket->readAll();
    QVERIFY(written.contains("CAP LS"));
    QVERIFY(written.contains("NICK nick"));
    QVERIFY(!written.contains("PASS secret"));
    QVERIFY(!written.contains("CAP REQ :sasl"));

    QVERIFY(waitForWritten(":irc.freenode.net CAP * LS :no s-a-s-l here"));
    QVERIFY(!clientSocket->waitForBytesWritten(1000));
    QVERIFY(!serverSocket->waitForReadyRead(1000));
    QVERIFY(serverSocket->readAll().isEmpty());

    // resume handshake
    QCoreApplication::sendPostedEvents(protocol, QEvent::MetaCall);
    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));
    written = serverSocket->readAll();
    QVERIFY(written.contains("PASS secret"));
    QVERIFY(written.contains("CAP END"));
}

#ifndef QT_NO_SSL
class SslSocket : public QSslSocket
{
    Q_OBJECT

public:
    SslSocket(QObject* parent) : QSslSocket(parent), clientEncryptionStarted(false) { }
    bool clientEncryptionStarted;

public slots:
    void startClientEncryption()
    {
        clientEncryptionStarted = true;
        QSslSocket::startClientEncryption();
    }
};
#endif // !QT_NO_SSL

void tst_IrcConnection::testSsl()
{
#ifndef QT_NO_SSL
    SslSocket* socket = new SslSocket(connection);
    connection->setSocket(socket);
    QCOMPARE(connection->socket(), socket);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(socket->clientEncryptionStarted);
#endif // !QT_NO_SSL
}

void tst_IrcConnection::testOpen()
{
    IrcConnection connection;
    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::open(): host is empty!");
    connection.open();
    QCOMPARE(connection.status(), IrcConnection::Inactive);

    connection.setHost("irc.ser.ver");
    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::open(): userName is empty!");
    connection.open();
    QCOMPARE(connection.status(), IrcConnection::Inactive);

    connection.setUserName("user");
    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::open(): nickNames is empty!");
    connection.open();
    QCOMPARE(connection.status(), IrcConnection::Inactive);

    connection.setNickName("nick");
    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::open(): realName is empty!");
    connection.open();
    QCOMPARE(connection.status(), IrcConnection::Inactive);

    connection.setRealName("real");
    connection.open();
    QVERIFY(connection.status() != IrcConnection::Inactive);

    connection.close();
    QCOMPARE(connection.status(), IrcConnection::Closed);

    connection.setEnabled(false);
    connection.open();
    QCOMPARE(connection.status(), IrcConnection::Closed);
}

void tst_IrcConnection::testEnabled()
{
    IrcConnection connection;
    QVERIFY(connection.isEnabled());

    QSignalSpy spy(&connection, SIGNAL(enabledChanged(bool)));
    QVERIFY(spy.isValid());

    connection.setEnabled(false);
    QVERIFY(!connection.isEnabled());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).toBool(), false);

    connection.setDisabled(true);
    QVERIFY(!connection.isEnabled());
    QCOMPARE(spy.count(), 1);

    connection.setDisabled(false);
    QVERIFY(connection.isEnabled());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.last().at(0).toBool(), true);

    connection.setEnabled(true);
    QVERIFY(connection.isEnabled());
    QCOMPARE(spy.count(), 2);
}

void tst_IrcConnection::testStatus()
{
    Irc::registerMetaTypes();

    QSignalSpy statusSpy(connection, SIGNAL(statusChanged(IrcConnection::Status)));
    QSignalSpy connectingSpy(connection, SIGNAL(connecting()));
    QSignalSpy connectedSpy(connection, SIGNAL(connected()));
    QSignalSpy disconnectedSpy(connection, SIGNAL(disconnected()));

    QVERIFY(statusSpy.isValid());
    QVERIFY(connectingSpy.isValid());
    QVERIFY(connectedSpy.isValid());
    QVERIFY(disconnectedSpy.isValid());

    int statusCount = 0;
    int connectingCount = 0;
    int connectedCount = 0;
    int disconnectedCount = 0;

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connecting);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Connecting);
    QCOMPARE(connectingSpy.count(), ++connectingCount);

    QVERIFY(waitForWritten(tst_IrcData::welcome()));
    QVERIFY(connection->isActive());
    QVERIFY(connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connected);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Connected);
    QCOMPARE(connectedSpy.count(), ++connectedCount);

    clientSocket->close();
    QVERIFY(connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Closing);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Closing);

    connection->close();
    QVERIFY(!connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Closed);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Closed);
    QCOMPARE(disconnectedSpy.count(), ++disconnectedCount);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connecting);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Connecting);
    QCOMPARE(connectingSpy.count(), ++connectingCount);

    QVERIFY(waitForWritten(tst_IrcData::welcome()));
    QVERIFY(connection->isActive());
    QVERIFY(connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connected);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Connected);
    QCOMPARE(connectedSpy.count(), ++connectedCount);

    // trigger an error
    serverSocket->close();
    QVERIFY(clientSocket->waitForDisconnected(100));
    QVERIFY(!connection->isConnected());
    QVERIFY(!connection->isActive());

    QCOMPARE(statusSpy.at(statusCount++).at(0).value<IrcConnection::Status>(), IrcConnection::Error);
    QCOMPARE(statusSpy.count(), statusCount);

    connection->close();
    QVERIFY(!connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Closed);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Closed);
    QCOMPARE(disconnectedSpy.count(), ++disconnectedCount);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connecting);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Connecting);
    QCOMPARE(connectingSpy.count(), ++connectingCount);

    QVERIFY(waitForWritten(tst_IrcData::welcome()));
    QVERIFY(connection->isActive());
    QVERIFY(connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connected);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Connected);
    QCOMPARE(connectedSpy.count(), ++connectedCount);

    // trigger an error - automatic reconnect
    connection->setReconnectDelay(1);
    serverSocket->close();
    QVERIFY(clientSocket->waitForDisconnected(100));
    QVERIFY(!connection->isConnected());
    QVERIFY(!connection->isActive());

    QCOMPARE(statusSpy.at(statusCount++).at(0).value<IrcConnection::Status>(), IrcConnection::Error);
    QCOMPARE(statusSpy.at(statusCount++).at(0).value<IrcConnection::Status>(), IrcConnection::Waiting);
    QCOMPARE(statusSpy.count(), statusCount);

    QEventLoop reconnectLoop;
    QTimer::singleShot(2000, &reconnectLoop, SLOT(quit()));
    connect(connection, SIGNAL(statusChanged(IrcConnection::Status)), &reconnectLoop, SLOT(quit()));
    reconnectLoop.exec();

    QVERIFY(connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connecting);
    QCOMPARE(statusSpy.count(), ++statusCount);
    QCOMPARE(statusSpy.last().at(0).value<IrcConnection::Status>(), IrcConnection::Connecting);

    QVERIFY(waitForOpened());
    QCOMPARE(connectingSpy.count(), ++connectingCount);

    // trigger an error _after_ quit -> no automatic reconnect
    connection->quit();
    serverSocket->close();
    QVERIFY(!connection->isConnected());
    QVERIFY(!connection->isActive());
    QCOMPARE(statusSpy.at(statusCount++).at(0).value<IrcConnection::Status>(), IrcConnection::Closed);
    QCOMPARE(statusSpy.count(), statusCount);
}

void tst_IrcConnection::testConnection()
{
    Irc::registerMetaTypes();

    TestProtocol* protocol = new TestProtocol(connection);
    FriendlyConnection* friendly = static_cast<FriendlyConnection*>(connection.data());
    friendly->setProtocol(protocol);
    QCOMPARE(friendly->protocol(), protocol);
    QCOMPARE(protocol->connection(), connection.data());

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connecting);

    QVERIFY(waitForWritten(":irc.ser.ver 001 nick :Welcome to the Internet Relay Chat Network nick"));
    QVERIFY(connection->isActive());
    QVERIFY(connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connected);

    connection->close();
    QVERIFY(!connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Closed);

    // don't open when disabled
    connection->setEnabled(false);
    connection->open();
    QVERIFY(!connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Closed);

    // re-enable
    connection->setEnabled(true);
    connection->open();
    QVERIFY(connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connecting);

    QVERIFY(waitForOpened());

    protocol->written.clear();
    connection->network()->requestCapability("identify-msg");
    QVERIFY(protocol->written.contains("CAP REQ"));
    QVERIFY(protocol->written.contains("identify-msg"));

    protocol->written.clear();
    connection->network()->requestCapabilities(QStringList() << "sasl" << "communi");
    QVERIFY(protocol->written.contains("CAP REQ"));
    QVERIFY(protocol->written.contains("sasl"));
    QVERIFY(protocol->written.contains("communi"));

    QVERIFY(waitForWritten(":irc.ser.ver 001 nick :Welcome to the Internet Relay Chat Network nick"));
    QVERIFY(connection->isActive());
    QVERIFY(connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Connected);

    protocol->written.clear();
    connection->setNickName("communi");
    QVERIFY(protocol->written.contains("NICK"));
    QVERIFY(protocol->written.contains("communi"));

    protocol->written.clear();
    connection->quit();
    QVERIFY(protocol->written.contains("QUIT"));

    connection->close();
    QVERIFY(!connection->isActive());
    QVERIFY(!connection->isConnected());
    QCOMPARE(connection->status(), IrcConnection::Closed);
}

class NickChanger : public QObject
{
    Q_OBJECT

public:
    NickChanger(IrcConnection* connection) : QObject(connection)
    {
        connect(connection, SIGNAL(nickNameReserved(QString*)), SLOT(onNickNameReserved(QString*)));
    }

    QString setAlternate;
    QString passedAlternate;

public slots:
    void onNickNameReserved(QString* alternate)
    {
        Q_ASSERT(alternate);
        passedAlternate = *alternate;
        *alternate = setAlternate;
    }
};

Q_DECLARE_METATYPE(QString*)
void tst_IrcConnection::testMessages()
{
    Irc::registerMetaTypes();
    qRegisterMetaType<QString*>();

    QSignalSpy messageSpy(connection, SIGNAL(messageReceived(IrcMessage*)));
    QSignalSpy accountMessageSpy(connection, SIGNAL(accountMessageReceived(IrcAccountMessage*)));
    QSignalSpy awayMessageSpy(connection, SIGNAL(awayMessageReceived(IrcAwayMessage*)));
    QSignalSpy batchMessageSpy(connection, SIGNAL(batchMessageReceived(IrcBatchMessage*)));
    QSignalSpy capabilityMessageSpy(connection, SIGNAL(capabilityMessageReceived(IrcCapabilityMessage*)));
    QSignalSpy errorMessageSpy(connection, SIGNAL(errorMessageReceived(IrcErrorMessage*)));
    QSignalSpy hostChangeMessageSpy(connection, SIGNAL(hostChangeMessageReceived(IrcHostChangeMessage*)));
    QSignalSpy inviteMessageSpy(connection, SIGNAL(inviteMessageReceived(IrcInviteMessage*)));
    QSignalSpy joinMessageSpy(connection, SIGNAL(joinMessageReceived(IrcJoinMessage*)));
    QSignalSpy kickMessageSpy(connection, SIGNAL(kickMessageReceived(IrcKickMessage*)));
    QSignalSpy modeMessageSpy(connection, SIGNAL(modeMessageReceived(IrcModeMessage*)));
    QSignalSpy namesMessageSpy(connection, SIGNAL(namesMessageReceived(IrcNamesMessage*)));
    QSignalSpy nickMessageSpy(connection, SIGNAL(nickMessageReceived(IrcNickMessage*)));
    QSignalSpy noticeMessageSpy(connection, SIGNAL(noticeMessageReceived(IrcNoticeMessage*)));
    QSignalSpy numericMessageSpy(connection, SIGNAL(numericMessageReceived(IrcNumericMessage*)));
    QSignalSpy motdMessageSpy(connection, SIGNAL(motdMessageReceived(IrcMotdMessage*)));
    QSignalSpy partMessageSpy(connection, SIGNAL(partMessageReceived(IrcPartMessage*)));
    QSignalSpy pingMessageSpy(connection, SIGNAL(pingMessageReceived(IrcPingMessage*)));
    QSignalSpy pongMessageSpy(connection, SIGNAL(pongMessageReceived(IrcPongMessage*)));
    QSignalSpy privateMessageSpy(connection, SIGNAL(privateMessageReceived(IrcPrivateMessage*)));
    QSignalSpy quitMessageSpy(connection, SIGNAL(quitMessageReceived(IrcQuitMessage*)));
    QSignalSpy topicMessageSpy(connection, SIGNAL(topicMessageReceived(IrcTopicMessage*)));
    QSignalSpy whoisMessageSpy(connection, SIGNAL(whoisMessageReceived(IrcWhoisMessage*)));
    QSignalSpy whowasMessageSpy(connection, SIGNAL(whowasMessageReceived(IrcWhowasMessage*)));
    QSignalSpy whoReplyMessageSpy(connection, SIGNAL(whoReplyMessageReceived(IrcWhoReplyMessage*)));

    QVERIFY(messageSpy.isValid());
    QVERIFY(accountMessageSpy.isValid());
    QVERIFY(awayMessageSpy.isValid());
    QVERIFY(batchMessageSpy.isValid());
    QVERIFY(capabilityMessageSpy.isValid());
    QVERIFY(errorMessageSpy.isValid());
    QVERIFY(hostChangeMessageSpy.isValid());
    QVERIFY(inviteMessageSpy.isValid());
    QVERIFY(joinMessageSpy.isValid());
    QVERIFY(kickMessageSpy.isValid());
    QVERIFY(modeMessageSpy.isValid());
    QVERIFY(namesMessageSpy.isValid());
    QVERIFY(nickMessageSpy.isValid());
    QVERIFY(noticeMessageSpy.isValid());
    QVERIFY(numericMessageSpy.isValid());
    QVERIFY(motdMessageSpy.isValid());
    QVERIFY(partMessageSpy.isValid());
    QVERIFY(pingMessageSpy.isValid());
    QVERIFY(pongMessageSpy.isValid());
    QVERIFY(privateMessageSpy.isValid());
    QVERIFY(quitMessageSpy.isValid());
    QVERIFY(topicMessageSpy.isValid());
    QVERIFY(whoisMessageSpy.isValid());
    QVERIFY(whowasMessageSpy.isValid());
    QVERIFY(whoReplyMessageSpy.isValid());

    int messageCount = 0;
    int numericMessageCount = 0;

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(":moorcock.freenode.net CAP * LS :account-notify extended-join identify-msg multi-prefix sasl"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(capabilityMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":moorcock.freenode.net 001 communi :Welcome to the freenode Internet Relay Chat Network communi"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 005 communi CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=freenode KNOCK STATUSMSG=@+ CALLERID=g :are supported by this server"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 005 communi CASEMAPPING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 ETRACE CPRIVMSG CNOTICE DEAF=D MONITOR=100 FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: :are supported by this server"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 005 communi EXTBAN=$,arxz WHOX CLIENTVER=3.0 SAFELIST ELIST=CTU :are supported by this server"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 375 communi :- moorcock.freenode.net Message of the Day -"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 372 communi :- Welcome to moorcock.freenode.net in ..."));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 376 communi :End of /MOTD command."));
    messageCount += 2; // RPL_ENDOFMOTD + IrcMotdMessage
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(motdMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":communi!~communi@hidd.en JOIN #freenode"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(joinMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":moorcock.freenode.net 332 communi #freenode :Welcome to #freenode | Staff are voiced; some may also be on /stats p -- feel free to /msg us at any time | FAQ: http://freenode.net/faq.shtml | Unwelcome queries? Use /mode your_nick +R to block them. | Channel guidelines: http://freenode.net/poundfreenode.shtml | Blog: http://blog.freenode.net | Please don't comment on spam/trolls."));
    messageCount += 2; // RPL_TOPIC & IrcTopicMessage
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(topicMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":moorcock.freenode.net 333 communi #freenode erry 1379357591"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 353 communi = #freenode :communi straterra absk007 pefn xlys Gromit TooCool Sambler gat0rs KarneAsada danis_963 Kiryx chrismeller deefloo black_male sxlnxdx bjork Kinny phobos_anomaly T13|sleeps JuxTApose Kolega2357 rorx techhelper1 hermatize Azimi iqualfragile fwilson skasturi mwallacesd mayday Guest76549 mcjohansen MangaKaDenza ARISTIDES ketas `- claptor ylluminate Cooky Brand3n cheater_1 Kirito digitaloktay Will| Iarfen abrotman smurfy Inaunt +mist Karol RougeR_"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 353 communi = #freenode :publickeating An_Ony_Moose michagogo Guest915` davidfg4 Ragnor s1lent_1 keee GingerGeek[Away] hibari derp S_T_A_N anonymuse asantoni road|runner LLckfan neoian2 aviancarrier nipples danieldaniel Pyrus Bry8Star shadowm_desktop furtardo rdymac TTSDA seaworthy Chiyo yscc Zombiebaron redpill f4cl3y Boohbah applebloom zorael kameloso^ Zetetic XAMPP wheels_up Cuppy-Cake mindlessjohnny Kymru mquin_ Rodja babilen kirin` David Affix jshyeung_ DarkAceZ karakedi"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 366 communi #freenode :End of /NAMES list."));
    messageCount += 2; // RPL_ENDOFNAMES & IrcNamesMessage
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(namesMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":ChanServ!ChanServ@services. NOTICE communi :[#freenode] Welcome to #freenode. All network staff are voiced in here, but may not always be around - type /stats p to get a list of on call staff. Others may be hiding so do feel free to ping and /msg us at will! Also please read the channel guidelines at http://freenode.net/poundfreenode.shtml - thanks."));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(noticeMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":services. 328 communi #freenode :http://freenode.net/"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);

    QVERIFY(waitForWritten("PING :moorcock.freenode.net"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(pingMessageSpy.count(), 1);

    QVERIFY(waitForWritten("PONG :moorcock.freenode.net"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(pongMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":jpnurmi!jpnurmi@qt/jpnurmi INVITE Communi84194 :#communi"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(inviteMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":moorcock.freenode.net 341 jpnurmi Communi84194 #communi"));
    messageCount += 2; // RPL_INVITING + IrcInviteMessage
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(inviteMessageSpy.count(), 2);
    QVERIFY(inviteMessageSpy.last().last().value<IrcInviteMessage*>()->property("reply").toBool());

    QVERIFY(waitForWritten(":Communi84194!ident@host NICK :communi"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(nickMessageSpy.count(), 1);

    // own nick name changes
    QSignalSpy nickNameChangedSpy(connection, SIGNAL(nickNameChanged(QString)));
    QVERIFY(nickNameChangedSpy.isValid());
    QVERIFY(waitForWritten(":communi!user@host NICK :own"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(nickMessageSpy.count(), 2);
    QCOMPARE(connection->nickName(), QString("own"));
    QCOMPARE(nickNameChangedSpy.count(), 1);
    QCOMPARE(nickNameChangedSpy.last().at(0).toString(), QString("own"));

    // nick in use
    QString prevNick = connection->nickName();
    NickChanger changer(connection);
    changer.setAlternate = "communi_";
    QSignalSpy nickNameReservedSpy(connection, SIGNAL(nickNameReserved(QString*)));
    QVERIFY(nickNameReservedSpy.isValid());
    QVERIFY(waitForWritten(":moorcock.freenode.net 433 * communi :Nickname is already in use."));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(nickNameReservedSpy.count(), 1);
    QCOMPARE(changer.passedAlternate, prevNick);

    QVERIFY(waitForWritten(":jpnurmi!jpnurmi@qt/jpnurmi MODE #communi +v communi"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(modeMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":moorcock.freenode.net 324 communi #communi +ms"));
    messageCount += 2; // RPL_CHANNELMODEIS + IrcModeMessage
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(modeMessageSpy.count(), 2);

    QVERIFY(waitForWritten(":qtassistant!jpnurmi@qt/jpnurmi/bot/qtassistant PART #communi"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(partMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":jpnurmi!jpnurmi@qt/jpnurmi PRIVMSG #communi :hello"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(privateMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":jpnurmi!jpnurmi@qt/jpnurmi QUIT :Client Quit"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(quitMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":jpnurmi!jpnurmi@qt/jpnurmi KICK #communi communi"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(kickMessageSpy.count(), 1);

    QVERIFY(waitForWritten("ERROR :just testing..."));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(errorMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":hobana.freenode.net 352 communi #communi ChanServ services. services. ChanServ H@ :0 Channel Services" ));
    messageCount += 2; // RPL_WHOREPLY + IrcWhoReplyMessage
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(whoReplyMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":hobana.freenode.net 315 communi #communi :End of /WHO list."));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(numericMessageSpy.count(), ++numericMessageCount);
    QCOMPARE(whoReplyMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":nick!user@host CHGHOST newuser newhost"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(hostChangeMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":nick!user@host AWAY :reason"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(awayMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":nick!user@host ACCOUNT account"));
    QCOMPARE(messageSpy.count(), ++messageCount);
    QCOMPARE(accountMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":asimov.freenode.net 311 jpnurmi qtassistant jpnurmi qt/jpnurmi/bot/qtassistant * :http://doc.qt.io/qt-5"));
    QVERIFY(waitForWritten(":asimov.freenode.net 318 jpnurmi qtassistant :End of /WHOIS list."));
    messageCount += 3; // RPL_WHOISUSER + RPL_ENDOFWHOIS + IrcWhoisMessage
    numericMessageCount += 2; // RPL_WHOISUSER + RPL_ENDOFWHOIS
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), numericMessageCount);
    QCOMPARE(whoisMessageSpy.count(), 1);

    QVERIFY(waitForWritten(":asimov.freenode.net 314 jpnurmi jirssi ~jpnurmi 88.95.51.136 * :J-P Nurmi"));
    QVERIFY(waitForWritten(":asimov.freenode.net 369 jpnurmi jirssi :End of WHOWAS"));
    messageCount += 3; // RPL_WHOWASUSER + RPL_ENDOFWHOWAS + IrcWhowasMessage
    numericMessageCount += 2; // RPL_WHOWASUSER + RPL_ENDOFWHOWAS
    QCOMPARE(messageSpy.count(), messageCount);
    QCOMPARE(numericMessageSpy.count(), numericMessageCount);
    QCOMPARE(whowasMessageSpy.count(), 1);
}

class MsgFilter : public QObject, public IrcMessageFilter
{
    Q_OBJECT
    Q_INTERFACES(IrcMessageFilter)

public:
    MsgFilter() : count(0), type(IrcMessage::Unknown), flags(IrcMessage::None)
    {
    }

    void reset(const QByteArray& p = "", int c = 0)
    {
        count = c;
        properties = p;
        flags = IrcMessage::None;
        type = IrcMessage::Unknown;
        values.clear();
    }

    bool messageFilter(IrcMessage* message)
    {
        ++count;
        type = message->type();
        flags = message->flags();
        foreach (const QByteArray& property, properties.split(',')) {
            QVariant value = message->property(property);
            if (!value.isNull())
                values[property] = value;
        }
        return false;
    }

public:
    int count;
    QVariantMap values;
    QByteArray properties;
    IrcMessage::Type type;
    IrcMessage::Flags flags;
};

void tst_IrcConnection::testMessageFlags()
{
    connection->open();
    QVERIFY(waitForOpened());

    int count = 0;
    MsgFilter filter;
    connection->installMessageFilter(&filter);

    QVERIFY(waitForWritten(":server CAP * LS :identify-msg"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Capability);
    QCOMPARE(filter.flags, IrcMessage::None);

    QVERIFY(waitForWritten(":server CAP communi ACK :identify-msg"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Capability);
    QCOMPARE(filter.flags, IrcMessage::None);

    QVERIFY(waitForWritten(":server 001 communi :Welcome..."));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Numeric);
    QCOMPARE(filter.flags, IrcMessage::None);

    QVERIFY(waitForWritten(":server 005 communi CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=fake KNOCK STATUSMSG=@+ CALLERID=g :are supported by this server"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Numeric);
    QCOMPARE(filter.flags, IrcMessage::None);

    QVERIFY(waitForWritten(":server 005 communi CASEMAPPING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 ETRACE CPRIVMSG CNOTICE DEAF=D MONITOR=100 FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: :are supported by this server"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Numeric);
    QCOMPARE(filter.flags, IrcMessage::None);

    QVERIFY(waitForWritten(":server 005 communi EXTBAN=$,arxz WHOX CLIENTVER=3.0 SAFELIST ELIST=CTU :are supported by this server"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Numeric);
    QCOMPARE(filter.flags, IrcMessage::None);

    filter.reset("content", count);
    QVERIFY(waitForWritten(":communi!ident@host PRIVMSG #communi :hi all"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Private);
    QCOMPARE(filter.flags, IrcMessage::Own);
    QCOMPARE(filter.values.value("content").toString(), QString("hi all"));

    filter.reset("content", count);
    QVERIFY(waitForWritten(":jpnurmi!ident@host PRIVMSG #communi :hello there, communi"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Private);
    QCOMPARE(filter.values.value("content").toString(), QString("hello there, communi"));

    filter.reset("content", count);
    QVERIFY(waitForWritten(":Guest1234!ident@host PRIVMSG #communi :hi communi"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Private);
    QCOMPARE(filter.values.value("content").toString(), QString("hi communi"));

    filter.reset("content", count);
    QVERIFY(waitForWritten(":communi!ident@host NOTICE #communi :hi all"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Notice);
    QCOMPARE(filter.flags, IrcMessage::Own);
    QCOMPARE(filter.values.value("content").toString(), QString("hi all"));

    filter.reset("content", count);
    QVERIFY(waitForWritten(":jpnurmi!ident@host NOTICE #communi :hello there, communi"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Notice);
    QCOMPARE(filter.values.value("content").toString(), QString("hello there, communi"));

    filter.reset("content", count);
    QVERIFY(waitForWritten(":Guest1234!ident@host NOTICE #communi :hi communi"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Notice);
    QCOMPARE(filter.values.value("content").toString(), QString("hi communi"));
}

void tst_IrcConnection::testStatusPrefixes()
{
    connection->open();
    QVERIFY(waitForOpened());

    int count = 0;
    MsgFilter filter;
    connection->installMessageFilter(&filter);

    QVERIFY(waitForWritten(":server 001 communi :Welcome..."));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Numeric);
    QCOMPARE(filter.flags, IrcMessage::None);

    QVERIFY(waitForWritten(":server 005 communi STATUSMSG=@+"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Numeric);
    QCOMPARE(filter.flags, IrcMessage::None);

    QVERIFY(waitForWritten(":server 375 communi :MOTD"));
    QVERIFY(waitForWritten(":server 376 communi :End of /MOTD command."));
    count += 3; // RPL_MOTDSTART, RPL_ENDOFMOTD, IrcMotdMessage
    QCOMPARE(filter.count, count);
    QCOMPARE(filter.type, IrcMessage::Motd);
    QCOMPARE(filter.flags, IrcMessage::None);

    filter.reset("target,statusPrefix,content", count);
    QVERIFY(waitForWritten(":Guest1234!ident@host PRIVMSG +#communi :hi communi"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Private);
    QCOMPARE(filter.values.value("target").toString(), QString("#communi"));
    QCOMPARE(filter.values.value("statusPrefix").toString(), QString("+"));
    QCOMPARE(filter.values.value("content").toString(), QString("hi communi"));

    filter.reset("target,statusPrefix,content", count);
    QVERIFY(waitForWritten(":Guest1234!ident@host NOTICE +#communi :hi communi"));
    QCOMPARE(filter.count, ++count);
    QCOMPARE(filter.type, IrcMessage::Notice);
    QCOMPARE(filter.values.value("target").toString(), QString("#communi"));
    QCOMPARE(filter.values.value("statusPrefix").toString(), QString("+"));
    QCOMPARE(filter.values.value("content").toString(), QString("hi communi"));
}

void tst_IrcConnection::testMessageComposer()
{
    connection->open();
    QVERIFY(waitForOpened());

    MsgFilter filter;
    connection->installMessageFilter(&filter);

    QVERIFY(waitForWritten(":my.irc.ser.ver 001 communi :Welcome..."));
    QVERIFY(waitForWritten(":my.irc.ser.ver 005 communi CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=fake KNOCK STATUSMSG=@+ CALLERID=g :are supported by this server"));
    QVERIFY(waitForWritten(":my.irc.ser.ver 005 communi CASEMAPPING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 ETRACE CPRIVMSG CNOTICE DEAF=D MONITOR=100 FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: :are supported by this server"));
    QVERIFY(waitForWritten(":my.irc.ser.ver 005 communi EXTBAN=$,arxz WHOX CLIENTVER=3.0 SAFELIST ELIST=CTU :are supported by this server"));

    filter.reset("mask,ident,host,server,nick,away,servOp,realName,composed");
    QVERIFY(waitForWritten(":my.irc.ser.ver 352 communi #communi ~jpnurmi qt/jpnurmi his.irc.ser.ver jpnurmi G*@ :0 J-P Nurmi"));
    QCOMPARE(filter.count, 2); // RPL_WHOREPLY + IrcWhoReply
    QCOMPARE(filter.values.value("mask").toString(), QString("#communi"));
    QCOMPARE(filter.values.value("ident").toString(), QString("~jpnurmi"));
    QCOMPARE(filter.values.value("host").toString(), QString("qt/jpnurmi"));
    QCOMPARE(filter.values.value("server").toString(), QString("his.irc.ser.ver"));
    QCOMPARE(filter.values.value("nick").toString(), QString("jpnurmi"));
    QCOMPARE(filter.values.value("away").toBool(), true);
    QCOMPARE(filter.values.value("servOp").toBool(), true);
    QCOMPARE(filter.values.value("realName").toString(), QString("J-P Nurmi"));
    QCOMPARE(filter.values.value("composed").toBool(), true);

    filter.reset("realName");
    QVERIFY(waitForWritten(":my.irc.ser.ver 352 communi #communi ~jpnurmi qt/jpnurmi his.irc.ser.ver jpnurmi G*@ :0"));
    QCOMPARE(filter.values.value("realName").toString(), QString());

    filter.reset("content,nick,reply,away,composed");
    QVERIFY(waitForWritten(":my.irc.ser.ver 301 communi nick :gone far away"));
    QCOMPARE(filter.values.value("content").toString(), QString("gone far away"));
    QCOMPARE(filter.values.value("nick").toString(), QString("nick"));
    QVERIFY(filter.values.value("reply").toBool());
    QVERIFY(filter.values.value("away").toBool());
    QVERIFY(filter.values.value("composed").toBool());
    QCOMPARE(filter.type, IrcMessage::Away);

    filter.reset("content,nick,reply,away,composed");
    QVERIFY(waitForWritten(":my.irc.ser.ver 301 communi nick"));
    QCOMPARE(filter.values.value("nick").toString(), QString("nick"));
    QCOMPARE(filter.values.value("content").toString(), QString());
    QVERIFY(filter.values.value("reply").toBool());
    QVERIFY(filter.values.value("away").toBool());
    QVERIFY(filter.values.value("composed").toBool());
    QCOMPARE(filter.type, IrcMessage::Away);

    filter.reset("content,nick,reply,away,composed");
    QVERIFY(waitForWritten(":my.irc.ser.ver 305 communi :You are no longer marked as being away"));
    QCOMPARE(filter.values.value("nick").toString(), QString("communi"));
    QCOMPARE(filter.values.value("content").toString(), QString("You are no longer marked as being away"));
    QVERIFY(filter.values.value("reply").toBool());
    QVERIFY(!filter.values.value("away").toBool());
    QVERIFY(filter.values.value("composed").toBool());
    QCOMPARE(filter.type, IrcMessage::Away);

    filter.reset("content,nick,reply,away,composed");
    QVERIFY(waitForWritten(":my.irc.ser.ver 306 communi :You have been marked as being away"));
    QCOMPARE(filter.values.value("nick").toString(), QString("communi"));
    QCOMPARE(filter.values.value("content").toString(), QString("You have been marked as being away"));
    QVERIFY(filter.values.value("reply").toBool());
    QVERIFY(filter.values.value("away").toBool());
    QVERIFY(filter.values.value("composed").toBool());
    QCOMPARE(filter.type, IrcMessage::Away);

    filter.reset("realName,server,info,account,address,since,idle,secure,from,channels,awayReason,valid");
    QVERIFY(waitForWritten(":asimov.freenode.net 311 jipsu qtassistant jpnurmi qt/jpnurmi/bot/qtassistant * :http://doc.qt.io/qt-5"));
    QVERIFY(waitForWritten(":asimov.freenode.net 319 jipsu qtassistant :+#jpnurmi"));
    QVERIFY(waitForWritten(":asimov.freenode.net 312 jipsu qtassistant leguin.freenode.net :Umeå, SE, EU"));
    QVERIFY(waitForWritten(":asimov.freenode.net 671 jipsu qtassistant :is using a secure connection"));
    QVERIFY(waitForWritten(":asimov.freenode.net 301 jipsu qtassistant :gone fishing"));
    QVERIFY(waitForWritten(":asimov.freenode.net 330 jipsu qtassistant qtaccountant :is logged in as"));
    QVERIFY(waitForWritten(":asimov.freenode.net 378 jipsu qtassistant :is connecting from *@88.95.51.136 88.95.51.136"));
    QVERIFY(waitForWritten(":asimov.freenode.net 317 jipsu qtassistant 15 1440706032 :seconds idle, signon time"));
    QVERIFY(waitForWritten(":asimov.freenode.net 318 jipsu qtassistant :End of /WHOIS list."));
    QCOMPARE(filter.values.value("realName").toString(), QString("http://doc.qt.io/qt-5"));
    QCOMPARE(filter.values.value("server").toString(), QString("leguin.freenode.net"));
    QCOMPARE(filter.values.value("info").toString(), QString::fromUtf8("Umeå, SE, EU"));
    QCOMPARE(filter.values.value("account").toString(), QString("qtaccountant"));
    QEXPECT_FAIL("", "RPL_WHOISHOST :is connecting from *@88.95.51.136 88.95.51.136", Continue);
    QCOMPARE(filter.values.value("address").toString(), QString("88.95.51.136"));
    QCOMPARE(filter.values.value("since").toDateTime(), QDateTime::fromTime_t(1440706032));
    QCOMPARE(filter.values.value("idle").toInt(), 15);
    QCOMPARE(filter.values.value("secure").toBool(), true);
    QCOMPARE(filter.values.value("channels").toStringList(), QStringList() << "+#jpnurmi");
    QCOMPARE(filter.values.value("awayReason").toString(), QString("gone fishing"));
    QVERIFY(filter.values.value("valid").toBool());
    QCOMPARE(filter.type, IrcMessage::Whois);

    filter.reset("realName,server,info,account,valid");
    QVERIFY(waitForWritten(":asimov.freenode.net 314 jipsu jirssi ~jpnurmi 88.95.51.136 * :J-P Nurmi"));
    QVERIFY(waitForWritten(":asimov.freenode.net 312 jipsu jirssi wolfe.freenode.net :Wed Aug 26 22:11:42 2015"));
    QVERIFY(waitForWritten(":asimov.freenode.net 330 jipsu jirssi jaccount :is logged in as"));
    QVERIFY(waitForWritten(":asimov.freenode.net 369 jipsu jirssi :End of WHOWAS"));
    QCOMPARE(filter.values.value("realName").toString(), QString("J-P Nurmi"));
    QCOMPARE(filter.values.value("server").toString(), QString("wolfe.freenode.net"));
    QCOMPARE(filter.values.value("info").toString(), QString("Wed Aug 26 22:11:42 2015"));
    QCOMPARE(filter.values.value("account").toString(), QString("jaccount"));
    QVERIFY(filter.values.value("valid").toBool());
    QCOMPARE(filter.type, IrcMessage::Whowas);
}

void tst_IrcConnection::testMessageComposerCrash_data()
{
    QTest::addColumn<QByteArray>("data");

    // unexpected replies - don't crash
    QList<Irc::Code> codes;
    codes << Irc::RPL_WHOISSERVER << Irc::RPL_WHOISACCOUNT << Irc::RPL_WHOISHOST << Irc::RPL_WHOISIDLE << Irc::RPL_WHOISSECURE << Irc::RPL_WHOISCHANNELS;
    foreach (Irc::Code code, codes)
        QTest::newRow(qPrintable(Irc::codeToString(code))) << QByteArray(":server ") + QByteArray::number(code);
}

void tst_IrcConnection::testMessageComposerCrash()
{
    QFETCH(QByteArray, data);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(waitForWritten(data));
}

void tst_IrcConnection::testBatch()
{
    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(":my.irc.ser.ver 001 communi :Welcome..."));
    QVERIFY(waitForWritten(":my.irc.ser.ver 005 communi CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=fake KNOCK STATUSMSG=@+ CALLERID=g :are supported by this server"));
    QVERIFY(waitForWritten(":my.irc.ser.ver 005 communi CASEMAPPING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 ETRACE CPRIVMSG CNOTICE DEAF=D MONITOR=100 FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: :are supported by this server"));
    QVERIFY(waitForWritten(":my.irc.ser.ver 005 communi EXTBAN=$,arxz WHOX CLIENTVER=3.0 SAFELIST ELIST=CTU :are supported by this server"));

    QSignalSpy messageSpy(connection, SIGNAL(messageReceived(IrcMessage*)));
    QSignalSpy batchMessageSpy(connection, SIGNAL(batchMessageReceived(IrcBatchMessage*)));
    QVERIFY(messageSpy.isValid());
    QVERIFY(batchMessageSpy.isValid());

    QVERIFY(waitForWritten(":irc.host BATCH +yXNAbvnRHTRBv netsplit irc.hub other.host"));
    QVERIFY(waitForWritten("@batch=yXNAbvnRHTRBv :aji!a@a QUIT :irc.hub other.host"));
    QVERIFY(waitForWritten("@batch=yXNAbvnRHTRBv :nenolod!a@a QUIT :irc.hub other.host"));
    QVERIFY(waitForWritten(":nick!user@host PRIVMSG #channel :This is not in batch, so processed immediately"));
    QVERIFY(waitForWritten("@batch=yXNAbvnRHTRBv :jilles!a@a QUIT :irc.hub other.host"));
    QVERIFY(waitForWritten(":irc.host BATCH -yXNAbvnRHTRBv"));

    QCOMPARE(messageSpy.count(), 2); // BATCH + NICK
    QCOMPARE(batchMessageSpy.count(), 1);

    IrcBatchMessage* batch = batchMessageSpy.last().last().value<IrcBatchMessage*>();
    QVERIFY(batch);
    QVERIFY(batch->isValid());
    QCOMPARE(batch->tag(), QString("yXNAbvnRHTRBv"));
    QCOMPARE(batch->batch(), QString("netsplit"));
    QCOMPARE(batch->messages().count(), 3);

    IrcQuitMessage* q1 = qobject_cast<IrcQuitMessage*>(batch->messages().at(0));
    QVERIFY(q1);
    QCOMPARE(q1->nick(), QString("aji"));
    QCOMPARE(q1->reason(), QString("irc.hub other.host"));

    IrcQuitMessage* q2 = qobject_cast<IrcQuitMessage*>(batch->messages().at(1));
    QVERIFY(q2);
    QCOMPARE(q2->nick(), QString("nenolod"));
    QCOMPARE(q2->reason(), QString("irc.hub other.host"));

    IrcQuitMessage* q3 = qobject_cast<IrcQuitMessage*>(batch->messages().at(2));
    QVERIFY(q3);
    QCOMPARE(q3->nick(), QString("jilles"));
    QCOMPARE(q3->reason(), QString("irc.hub other.host"));
}

void tst_IrcConnection::testServerTime()
{
    connection->open();
    QVERIFY(waitForOpened());

    QSignalSpy messageSpy(connection, SIGNAL(numericMessageReceived(IrcNumericMessage*)));
    QVERIFY(messageSpy.isValid());

    QVERIFY(waitForWritten("@time=2011-10-19T16:40:51.620Z :my.irc.ser.ver 001 communi :Welcome..."));

    QCOMPARE(messageSpy.count(), 1);
    IrcNumericMessage* message = messageSpy.last().last().value<IrcNumericMessage*>();
    QVERIFY(message);
    QVERIFY(message->isValid());
    QCOMPARE(message->timeStamp(), QDateTime(QDate(2011, 10, 19), QTime(16, 40, 51, 620), Qt::UTC));
}

void tst_IrcConnection::testSendCommand()
{
    IrcConnection conn;
    QVERIFY(!conn.sendCommand(0));
    QVERIFY(!conn.sendCommand(IrcCommand::createQuit()));

    TestProtocol* protocol = new TestProtocol(connection);
    FriendlyConnection* friendly = static_cast<FriendlyConnection*>(connection.data());
    friendly->setProtocol(protocol);
    QCOMPARE(friendly->protocol(), protocol);
    QCOMPARE(protocol->connection(), connection.data());

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(connection->sendCommand(IrcCommand::createQuit()));
    QVERIFY(!connection->sendCommand(0));
    QVERIFY(protocol->written.contains("QUIT"));
}

void tst_IrcConnection::testSendData()
{
    IrcConnection conn;
    QVERIFY(!conn.sendData("QUIT"));

    TestProtocol* protocol = new TestProtocol(connection);
    FriendlyConnection* friendly = static_cast<FriendlyConnection*>(connection.data());
    friendly->setProtocol(protocol);
    QCOMPARE(friendly->protocol(), protocol);
    QCOMPARE(protocol->connection(), connection.data());

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(connection->sendData("QUIT"));
    QVERIFY(protocol->written.contains("QUIT"));
}

class TestFilter : public QObject, public IrcMessageFilter, public IrcCommandFilter
{
    Q_OBJECT
    Q_INTERFACES(IrcMessageFilter IrcCommandFilter)

public:
    void clear()
    {
        commitSuicide = false;
        messageFiltered = 0;
        commandFiltered = 0;
        messageFilterEnabled = false;
        commandFilterEnabled = false;
    }

    bool messageFilter(IrcMessage*)
    {
        ++messageFiltered;
        if (commitSuicide)
            delete this;
        return messageFilterEnabled;
    }

    bool commandFilter(IrcCommand*)
    {
        ++commandFiltered;
        if (commitSuicide)
            delete this;
        return commandFilterEnabled;
    }

    bool commitSuicide;
    int messageFiltered;
    int commandFiltered;
    bool messageFilterEnabled;
    bool commandFilterEnabled;
};

void tst_IrcConnection::testMessageFilter()
{
    Irc::registerMetaTypes();

    QSignalSpy messageSpy(connection, SIGNAL(messageReceived(IrcMessage*)));
    QVERIFY(messageSpy.isValid());
    int messageCount = 0;

    TestFilter filter1;
    QScopedPointer<TestFilter> filter2(new TestFilter);
    QScopedPointer<TestFilter> filter3(new TestFilter);

    filter1.clear(); filter2->clear(); filter3->clear();

    connection->installMessageFilter(&filter1);
    connection->installMessageFilter(filter2.data());
    connection->installMessageFilter(filter3.data());

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(":moorcock.freenode.net 001 communi :Welcome to the freenode Internet Relay Chat Network communi"));
    QCOMPARE(filter1.messageFiltered, 1);
    QCOMPARE(filter2->messageFiltered, 1);
    QCOMPARE(filter3->messageFiltered, 1);
    QCOMPARE(messageSpy.count(), ++messageCount);

    filter1.clear(); filter2->clear(); filter3->clear();
    filter3->messageFilterEnabled = true;

    QVERIFY(waitForWritten(":moorcock.freenode.net 005 communi CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=freenode KNOCK STATUSMSG=@+ CALLERID=g :are supported by this server"));
    QCOMPARE(filter1.messageFiltered, 0);
    QCOMPARE(filter2->messageFiltered, 0);
    QCOMPARE(filter3->messageFiltered, 1);
    QCOMPARE(messageSpy.count(), messageCount);

    filter1.clear(); filter2->clear(); filter3->clear();
    filter2->messageFilterEnabled = true;

    QVERIFY(waitForWritten(":moorcock.freenode.net 005 communi CASEMAPPING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 ETRACE CPRIVMSG CNOTICE DEAF=D MONITOR=100 FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: :are supported by this server"));
    QCOMPARE(filter1.messageFiltered, 0);
    QCOMPARE(filter2->messageFiltered, 1);
    QCOMPARE(filter3->messageFiltered, 1);
    QCOMPARE(messageSpy.count(), messageCount);

    filter1.clear(); filter2->clear(); filter3->clear();
    filter1.messageFilterEnabled = true;

    QVERIFY(waitForWritten(":moorcock.freenode.net 005 communi EXTBAN=$,arxz WHOX CLIENTVER=3.0 SAFELIST ELIST=CTU :are supported by this server"));
    QCOMPARE(filter1.messageFiltered, 1);
    QCOMPARE(filter2->messageFiltered, 1);
    QCOMPARE(filter3->messageFiltered, 1);
    QCOMPARE(messageSpy.count(), messageCount);

    filter1.clear(); filter2->clear(); filter3->clear();

    QVERIFY(waitForWritten(":moorcock.freenode.net 375 communi :- moorcock.freenode.net Message of the Day -"));
    QCOMPARE(filter1.messageFiltered, 1);
    QCOMPARE(filter2->messageFiltered, 1);
    QCOMPARE(filter3->messageFiltered, 1);
    QCOMPARE(messageSpy.count(), ++messageCount);

    // a deleted filter gets removed
    filter2.reset();
    filter1.clear(); filter3->clear();

    QVERIFY(waitForWritten(":moorcock.freenode.net 372 communi :- Welcome to moorcock.freenode.net in ..."));
    QCOMPARE(filter1.messageFiltered, 1);
    QCOMPARE(filter3->messageFiltered, 1);
    QCOMPARE(messageSpy.count(), ++messageCount);

    QVERIFY(waitForWritten(":moorcock.freenode.net 376 communi :End of /MOTD command."));
    messageCount += 2; // RPL_ENDOFMOTD + IrcMotdMessage
    QCOMPARE(messageSpy.count(), messageCount);

    // double filters
    connection->installMessageFilter(&filter1);
    connection->installMessageFilter(filter3.data());
    filter1.clear(); filter3->clear();

    QVERIFY(waitForWritten(":communi!~communi@hidd.en JOIN #freenode"));
    QCOMPARE(filter1.messageFiltered, 2);
    QCOMPARE(filter3->messageFiltered, 2);
    QCOMPARE(messageSpy.count(), ++messageCount);

    // remove & enable double filter
    filter1.clear(); filter3->clear();
    filter1.messageFilterEnabled = true;
    connection->removeMessageFilter(filter3.data());

    QVERIFY(waitForWritten(":communi!~communi@hidd.en JOIN #communi"));
    QCOMPARE(filter1.messageFiltered, 1);
    QCOMPARE(filter3->messageFiltered, 0);
    QCOMPARE(messageSpy.count(), messageCount);

    // remove & delete
    filter3.reset();
    filter1.clear();
    connection->removeMessageFilter(&filter1);

    QVERIFY(waitForWritten(":communi!~communi@hidd.en PART #communi"));
    QCOMPARE(filter1.messageFiltered, 0);
    QCOMPARE(messageSpy.count(), ++messageCount);

    // commit a suicide & filter
    QPointer<TestFilter> suicidal1 = new TestFilter;
    connection->installMessageFilter(suicidal1);
    suicidal1->clear();
    suicidal1->messageFilterEnabled = true;
    suicidal1->commitSuicide = true;

    QVERIFY(waitForWritten(":communi!~communi@hidd.en PART #freenode"));
    QCOMPARE(messageSpy.count(), messageCount);
    QVERIFY(!suicidal1);

    // commit a suicide & don't filter
    QPointer<TestFilter> suicidal2 = new TestFilter;
    connection->installMessageFilter(suicidal2);
    suicidal2->clear();
    suicidal2->commitSuicide = true;

    QVERIFY(waitForWritten(":communi!~communi@hidd.en JOIN #qt"));
    QVERIFY(!suicidal2);
}

void tst_IrcConnection::testCommandFilter()
{
    TestProtocol* protocol = new TestProtocol(connection);
    FriendlyConnection* friendly = static_cast<FriendlyConnection*>(connection.data());
    friendly->setProtocol(protocol);
    QCOMPARE(friendly->protocol(), protocol);
    QCOMPARE(protocol->connection(), connection.data());

    TestFilter filter1;
    QScopedPointer<TestFilter> filter2(new TestFilter);
    QScopedPointer<TestFilter> filter3(new TestFilter);

    filter1.clear(); filter2->clear(); filter3->clear();

    connection->installCommandFilter(&filter1);
    connection->installCommandFilter(filter2.data());
    connection->installCommandFilter(filter3.data());

    connection->open();
    QVERIFY(waitForOpened());

    connection->sendCommand(IrcCommand::createJoin("#freenode"));
    QCOMPARE(filter1.commandFiltered, 1);
    QCOMPARE(filter2->commandFiltered, 1);
    QCOMPARE(filter3->commandFiltered, 1);
    QVERIFY(!protocol->written.isEmpty());

    protocol->written.clear();
    filter1.clear(); filter2->clear(); filter3->clear();
    filter3->commandFilterEnabled = true;

    connection->sendCommand(IrcCommand::createJoin("#communi"));
    QCOMPARE(filter1.commandFiltered, 0);
    QCOMPARE(filter2->commandFiltered, 0);
    QCOMPARE(filter3->commandFiltered, 1);
    QVERIFY(protocol->written.isEmpty());

    protocol->written.clear();
    filter1.clear(); filter2->clear(); filter3->clear();
    filter2->commandFilterEnabled = true;

    connection->sendCommand(IrcCommand::createJoin("#qt"));
    QCOMPARE(filter1.commandFiltered, 0);
    QCOMPARE(filter2->commandFiltered, 1);
    QCOMPARE(filter3->commandFiltered, 1);
    QVERIFY(protocol->written.isEmpty());

    protocol->written.clear();
    filter1.clear(); filter2->clear(); filter3->clear();
    filter1.commandFilterEnabled = true;

    connection->sendCommand(IrcCommand::createPart("#freenode"));
    QCOMPARE(filter1.commandFiltered, 1);
    QCOMPARE(filter2->commandFiltered, 1);
    QCOMPARE(filter3->commandFiltered, 1);
    QVERIFY(protocol->written.isEmpty());

    protocol->written.clear();
    filter1.clear(); filter2->clear(); filter3->clear();

    connection->sendCommand(IrcCommand::createPart("#communi"));
    QCOMPARE(filter1.commandFiltered, 1);
    QCOMPARE(filter2->commandFiltered, 1);
    QCOMPARE(filter3->commandFiltered, 1);
    QVERIFY(!protocol->written.isEmpty());

    // a deleted filter gets removed
    filter2.reset();
    filter1.clear(); filter3->clear();
    protocol->written.clear();

    connection->sendCommand(IrcCommand::createPart("#qt"));
    QCOMPARE(filter1.commandFiltered, 1);
    QCOMPARE(filter3->commandFiltered, 1);
    QVERIFY(!protocol->written.isEmpty());

    // double filters
    connection->installCommandFilter(&filter1);
    connection->installCommandFilter(filter3.data());
    filter1.clear(); filter3->clear();
    protocol->written.clear();

    connection->sendCommand(IrcCommand::createJoin("#freenode"));
    QCOMPARE(filter1.commandFiltered, 2);
    QCOMPARE(filter3->commandFiltered, 2);
    QVERIFY(!protocol->written.isEmpty());

    // remove & enable double filter
    filter1.clear(); filter3->clear();
    filter1.commandFilterEnabled = true;
    connection->removeCommandFilter(filter3.data());
    protocol->written.clear();

    connection->sendCommand(IrcCommand::createJoin("#communi"));
    QCOMPARE(filter1.commandFiltered, 1);
    QCOMPARE(filter3->commandFiltered, 0);
    QVERIFY(protocol->written.isEmpty());

    // remove & delete
    filter3.reset();
    filter1.clear();
    connection->removeCommandFilter(&filter1);
    protocol->written.clear();

    connection->sendCommand(IrcCommand::createJoin("#qt"));
    QCOMPARE(filter1.commandFiltered, 0);
    QVERIFY(!protocol->written.isEmpty());

    // commit a suicide
    QPointer<TestFilter> suicidal = new TestFilter;
    connection->installCommandFilter(suicidal);
    suicidal->commitSuicide = true;

    connection->sendCommand(IrcCommand::createPart("#qt"));
    QVERIFY(!suicidal);
}

void tst_IrcConnection::testDebug()
{
    QString str;
    QDebug dbg(&str);

    dbg << static_cast<IrcConnection*>(0);
    QCOMPARE(str.trimmed(), QString::fromLatin1("IrcConnection(0x0)"));
    str.clear();

    IrcConnection connection;
    dbg << &connection;
    QVERIFY(QRegExp("IrcConnection\\(0x[0-9A-Fa-f]+\\) ").exactMatch(str));
    str.clear();

    connection.setHost("irc.freenode.net");
    dbg << &connection;
    QVERIFY(QRegExp("IrcConnection\\(0x[0-9A-Fa-f]+, irc.freenode.net\\) ").exactMatch(str));
    str.clear();

    connection.setDisplayName("Freenode");
    dbg << &connection;
    QVERIFY(QRegExp("IrcConnection\\(0x[0-9A-Fa-f]+, Freenode\\) ").exactMatch(str));
    str.clear();

    dbg << IrcConnection::Connected;
    QCOMPARE(str.trimmed(), QString::fromLatin1("Connected"));
    str.clear();
}

void tst_IrcConnection::testWarnings()
{
    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(connection->isActive());

    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setHost() has no effect until re-connect");
    connection->setHost("foo");

    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setPort() has no effect until re-connect");
    connection->setPort(1234);

    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setUserName() has no effect until re-connect");
    connection->setUserName("foo");

    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setRealName() has no effect until re-connect");
    connection->setRealName("foo");

    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setPassword() has no effect until re-connect");
    connection->setPassword("foo");

    QTest::ignoreMessage(QtWarningMsg, "IrcConnection::setSaslMechanism() has no effect until re-connect");
    connection->setSaslMechanism("PLAIN");
}

class FakeQmlConnection : public IrcConnection
{
    Q_OBJECT
    friend class tst_IrcConnection;

public slots:
    // -Wno-overloaded-virtual
    QVariant createCtcpReply(const QVariant& request)
    {
        return QVariant::fromValue(IrcConnection::createCtcpReply(request.value<IrcPrivateMessage*>()));
    }
};

void tst_IrcConnection::testCtcp()
{
    FriendlyConnection* friendly = static_cast<FriendlyConnection*>(connection.data());

    QVariantMap replies;
    replies.insert("FOO", "bar");
    connection->setCtcpReplies(replies);
    QCOMPARE(connection->ctcpReplies(), replies);

    // PING
    IrcMessage* msg = IrcMessage::fromData(":nick!user@host PRIVMSG communi :\1PING timestamp\1", connection);
    QScopedPointer<IrcPrivateMessage> pingRequest(qobject_cast<IrcPrivateMessage*>(msg));
    QVERIFY(pingRequest.data());

    QScopedPointer<IrcCommand> pingReply(friendly->createCtcpReply(pingRequest.data()));
    QVERIFY(pingReply.data());
    QCOMPARE(pingReply->type(), IrcCommand::CtcpReply);
    QCOMPARE(pingReply->toString(), QString("NOTICE nick :\1PING timestamp\1"));

    // TIME
    msg = IrcMessage::fromData(":nick!user@host PRIVMSG communi :\1TIME\1", connection);
    QScopedPointer<IrcPrivateMessage> timeRequest(qobject_cast<IrcPrivateMessage*>(msg));
    QVERIFY(timeRequest);

    QScopedPointer<IrcCommand> timeReply(friendly->createCtcpReply(timeRequest.data()));
    QVERIFY(timeReply.data());
    QCOMPARE(timeReply->type(), IrcCommand::CtcpReply);
    QCOMPARE(timeReply->toString(), QString("NOTICE nick :\1TIME %1\1").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));

    // VERSION
    msg = IrcMessage::fromData(":nick!user@host PRIVMSG communi :\1VERSION\1", connection);
    QScopedPointer<IrcPrivateMessage> versionRequest(qobject_cast<IrcPrivateMessage*>(msg));
    QVERIFY(versionRequest.data());

    QScopedPointer<IrcCommand> versionReply(friendly->createCtcpReply(versionRequest.data()));
    QVERIFY(versionReply.data());
    QCOMPARE(versionReply->type(), IrcCommand::CtcpReply);
    QVERIFY(versionReply->toString().startsWith("NOTICE nick :\1VERSION "));
    QVERIFY(versionReply->toString().contains(Irc::version()));
    QVERIFY(versionReply->toString().endsWith("\1"));

    // SOURCE
    msg = IrcMessage::fromData(":nick!user@host PRIVMSG communi :\1SOURCE\1", connection);
    QScopedPointer<IrcPrivateMessage> sourceRequest(qobject_cast<IrcPrivateMessage*>(msg));
    QVERIFY(sourceRequest.data());

    QScopedPointer<IrcCommand> sourceReply(friendly->createCtcpReply(sourceRequest.data()));
    QVERIFY(sourceReply.data());
    QCOMPARE(sourceReply->type(), IrcCommand::CtcpReply);
    QVERIFY(sourceReply->toString().startsWith("NOTICE nick :\1SOURCE "));
    QVERIFY(sourceReply->toString().contains("https://"));
    QVERIFY(sourceReply->toString().endsWith("\1"));

    // CLIENTINFO
    msg = IrcMessage::fromData(":nick!user@host PRIVMSG communi :\1CLIENTINFO\1", connection);
    QScopedPointer<IrcPrivateMessage> infoRequest(qobject_cast<IrcPrivateMessage*>(msg));
    QVERIFY(infoRequest.data());

    QScopedPointer<IrcCommand> infoReply(friendly->createCtcpReply(infoRequest.data()));
    QVERIFY(infoReply.data());
    QCOMPARE(infoReply->type(), IrcCommand::CtcpReply);
    QVERIFY(infoReply->toString().startsWith("NOTICE nick :\1CLIENTINFO "));
    QVERIFY(infoReply->toString().contains("PING"));
    QVERIFY(infoReply->toString().contains("TIME"));
    QVERIFY(infoReply->toString().contains("VERSION"));
    QVERIFY(infoReply->toString().contains("SOURCE"));
    QVERIFY(infoReply->toString().endsWith("\1"));

    // FOO
    msg = IrcMessage::fromData(":nick!user@host PRIVMSG communi :\1FOO\1", connection);
    QScopedPointer<IrcPrivateMessage> fooRequest(qobject_cast<IrcPrivateMessage*>(msg));
    QVERIFY(fooRequest.data());

    QScopedPointer<IrcCommand> fooReply(friendly->createCtcpReply(fooRequest.data()));
    QVERIFY(fooReply.data());
    QCOMPARE(fooReply->type(), IrcCommand::CtcpReply);
    QCOMPARE(fooReply->toString(), QString("NOTICE nick :\1FOO bar\1"));

    // override
    replies.insert("VERSION", "none");
    connection->setCtcpReplies(replies);
    QCOMPARE(connection->ctcpReplies(), replies);

    msg = IrcMessage::fromData(":nick!user@host PRIVMSG communi :\1VERSION\1", connection);
    QScopedPointer<IrcPrivateMessage> overrideRequest(qobject_cast<IrcPrivateMessage*>(msg));
    QVERIFY(overrideRequest.data());

    QScopedPointer<IrcCommand> overrideReply(friendly->createCtcpReply(overrideRequest.data()));
    QVERIFY(overrideReply.data());
    QCOMPARE(overrideReply->type(), IrcCommand::CtcpReply);
    QCOMPARE(overrideReply->toString(), QString("NOTICE nick :\1VERSION none\1"));

    connection->setCtcpReplies(QVariantMap());

    // QML compatibility
    FakeQmlConnection qmlConnection;
    qmlConnection.setUserName("user");
    qmlConnection.setNickName("nick");
    qmlConnection.setRealName("real");
    qmlConnection.setPassword("secret");
    qmlConnection.setHost("127.0.0.1");
    qmlConnection.setPort(server->serverPort());

    TestProtocol* qmlProtocol = new TestProtocol(&qmlConnection);
    qmlConnection.setProtocol(qmlProtocol);
    qmlConnection.open();

    QVERIFY(server->waitForNewConnection(200));
    QAbstractSocket* qmlServerSocket = server->nextPendingConnection();
    QVERIFY(qmlServerSocket);
    QAbstractSocket* qmlClientSocket = qmlConnection.socket();
    QVERIFY(qmlClientSocket);
    QVERIFY(qmlClientSocket->waitForConnected(200));

    qmlProtocol->written.clear();
    qmlServerSocket->write(":nick!user@host PRIVMSG communi :\1PING qml\1\r\n");
    QVERIFY(qmlServerSocket->waitForBytesWritten(1000));
    QVERIFY(qmlClientSocket->waitForReadyRead(1000));
    QCOMPARE(qmlProtocol->written, QByteArray("NOTICE nick :\1PING qml\1"));

    connection->open();
    QVERIFY(waitForOpened());

    TestProtocol* protocol = new TestProtocol(friendly);
    friendly->setProtocol(protocol);
    QCOMPARE(friendly->protocol(), protocol);
    QCOMPARE(protocol->connection(), friendly);

    // PING
    protocol->written.clear();
    QVERIFY(waitForWritten(":nick!user@host PRIVMSG communi :\1PING timestamp\1\r\n"));
    QCOMPARE(protocol->written, QByteArray("NOTICE nick :\1PING timestamp\1"));

    // TIME
    protocol->written.clear();
    QVERIFY(waitForWritten(":nick!user@host PRIVMSG communi :\1TIME\1\r\n"));
    QVERIFY(protocol->written.startsWith("NOTICE nick :\1TIME "));
    QVERIFY(protocol->written.endsWith("\1"));

    // VERSION
    protocol->written.clear();
    QVERIFY(waitForWritten(":nick!user@host PRIVMSG communi :\1VERSION\1\r\n"));
    QVERIFY(protocol->written.startsWith("NOTICE nick :\1VERSION "));
    QVERIFY(protocol->written.contains(Irc::version().toUtf8()));
    QVERIFY(protocol->written.endsWith("\1"));

    // SOURCE
    protocol->written.clear();
    QVERIFY(waitForWritten(":nick!user@host PRIVMSG communi :\1SOURCE\1\r\n"));
    QVERIFY(protocol->written.startsWith("NOTICE nick :\1SOURCE "));
    QVERIFY(protocol->written.contains("https://"));
    QVERIFY(protocol->written.endsWith("\1"));

    // CLIENTINFO
    protocol->written.clear();
    QVERIFY(waitForWritten(":nick!user@host PRIVMSG communi :\1CLIENTINFO\1\r\n"));
    QVERIFY(protocol->written.startsWith("NOTICE nick :\1CLIENTINFO "));
    QVERIFY(protocol->written.contains("PING"));
    QVERIFY(protocol->written.contains("TIME"));
    QVERIFY(protocol->written.contains("VERSION"));
    QVERIFY(protocol->written.contains("SOURCE"));
    QVERIFY(protocol->written.endsWith("\1"));
}

void tst_IrcConnection::testClone()
{
    QVariantMap ud;
    ud.insert("foo", "bar");

    IrcConnection c1;
    c1.setHost("host");
    c1.setPort(123);
    c1.setServers(QStringList() << "s1" << "s2" << "s3");
    c1.setUserName("user");
    c1.setNickName("nick");
    c1.setRealName("real");
    c1.setPassword("pass");
    c1.setNickNames(QStringList() << "n1" << "n2" << "n3");
    c1.setDisplayName("display");
    c1.setUserData(ud);
    c1.setEncoding("UTF-8");
    c1.setEnabled(false);
    c1.setReconnectDelay(10);
    c1.setSecure(true);
    c1.setSaslMechanism("PLAIN");

    IrcConnection* c2 = c1.clone(&c1);
    QCOMPARE(c2->parent(), &c1);

    QCOMPARE(c2->host(), QString("host"));
    QCOMPARE(c2->port(), 123);
    QCOMPARE(c2->servers(), QStringList() << "s1" << "s2" << "s3");
    QCOMPARE(c2->userName(), QString("user"));
    QCOMPARE(c2->nickName(), QString("nick"));
    QCOMPARE(c2->realName(), QString("real"));
    QCOMPARE(c2->password(), QString("pass"));
    QCOMPARE(c2->nickNames(), QStringList() << "n1" << "n2" << "n3");
    QCOMPARE(c2->displayName(), QString("display"));
    QCOMPARE(c2->userData(), ud);
    QCOMPARE(c2->encoding(), QByteArray("UTF-8"));
    QVERIFY(!c2->isEnabled());
    QCOMPARE(c2->reconnectDelay(), 10);
    QVERIFY(c2->isSecure());
    QCOMPARE(c2->saslMechanism(), QString("PLAIN"));
}

void tst_IrcConnection::testSaveRestore()
{
    QVariantMap ud;
    ud.insert("foo", "bar");

    IrcConnection c1;
    c1.setHost("host");
    c1.setPort(123);
    c1.setServers(QStringList() << "s1" << "s2" << "s3");
    c1.setUserName("user");
    c1.setNickName("nick");
    c1.setRealName("real");
    c1.setPassword("pass");
    c1.setNickNames(QStringList() << "n1" << "n2" << "n3");
    c1.setDisplayName("display");
    c1.setUserData(ud);
    c1.setEncoding("UTF-8");
    c1.setEnabled(false);
    c1.setReconnectDelay(10);
    c1.setSecure(true);
    c1.setSaslMechanism("PLAIN");

    IrcConnection c2;
    c2.restoreState(c1.saveState());

    QCOMPARE(c2.host(), QString("host"));
    QCOMPARE(c2.port(), 123);
    QCOMPARE(c2.servers(), QStringList() << "s1" << "s2" << "s3");
    QCOMPARE(c2.userName(), QString("user"));
    QEXPECT_FAIL("", "TODO", Continue);
    QCOMPARE(c2.nickName(), QString("nick"));
    QCOMPARE(c2.realName(), QString("real"));
    QCOMPARE(c2.password(), QString("pass"));
    QCOMPARE(c2.nickNames(), QStringList() << "n1" << "n2" << "n3");
    QCOMPARE(c2.displayName(), QString("display"));
    QCOMPARE(c2.userData(), ud);
    QCOMPARE(c2.encoding(), QByteArray("UTF-8"));
    QVERIFY(!c2.isEnabled());
    QCOMPARE(c2.reconnectDelay(), 10);
    QVERIFY(c2.isSecure());
    QCOMPARE(c2.saslMechanism(), QString("PLAIN"));
}

void tst_IrcConnection::testSignals()
{
    connection->open();
    QVERIFY(waitForOpened());

    QSignalSpy channelKeyRequiredSpy(connection, SIGNAL(channelKeyRequired(QString,QString*)));
    QVERIFY(channelKeyRequiredSpy.isValid());

    QVERIFY(waitForWritten(":hobana.freenode.net 475 jpnurmi #communi :Cannot join channel (+k) - bad key"));
    QCOMPARE(channelKeyRequiredSpy.count(), 1);
    QCOMPARE(channelKeyRequiredSpy.last().first().toString(), QString("#communi"));

    QSignalSpy nickNameRequiredSpy(connection, SIGNAL(nickNameRequired(QString,QString*)));
    QVERIFY(nickNameRequiredSpy.isValid());

    QVERIFY(waitForWritten(":sinisalo.freenode.net 433 * jpnurmi :Nickname is already in use."));
    QCOMPARE(nickNameRequiredSpy.count(), 1);
    QCOMPARE(nickNameRequiredSpy.last().first().toString(), QString("jpnurmi"));
}

void tst_IrcConnection::testServers()
{
    QVERIFY(IrcConnection::isValidServer("irc.freenode.net"));
    QVERIFY(IrcConnection::isValidServer("irc.freenode.net 6667"));
    QVERIFY(IrcConnection::isValidServer("irc.freenode.net +6697"));

    QVERIFY(!IrcConnection::isValidServer(""));
    QVERIFY(!IrcConnection::isValidServer("irc.freenode.net foobar"));
    QVERIFY(!IrcConnection::isValidServer("irc.freenode.net 6667 foobar"));
}

QTEST_MAIN(tst_IrcConnection)

#include "tst_ircconnection.moc"
