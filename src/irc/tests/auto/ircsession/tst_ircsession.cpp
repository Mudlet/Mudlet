/*
 * Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
 *
 * This test is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "irc.h"
#include "irccommand.h"
#include "ircsession.h"
#include "ircmessage.h"
#include <QtTest/QtTest>
#include <QtCore/QTextCodec>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#ifndef QT_NO_OPENSSL
#include <QtNetwork/QSslSocket>
#endif

class tst_IrcSession : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

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

    void testEncoding_data();
    void testEncoding();

    void testSocket_data();
    void testSocket();

    void testSession();
    void testSendCommand();
    void testSendData();

private:
    QTcpServer server;
};

void tst_IrcSession::initTestCase()
{
    server.listen();
}

void tst_IrcSession::cleanupTestCase()
{
    server.close();
}

void tst_IrcSession::testDefaults()
{
    IrcSession session;
    QVERIFY(session.host().isNull());
    QCOMPARE(session.port(), 6667);
    QVERIFY(session.userName().isNull());
    QVERIFY(session.nickName().isNull());
    QVERIFY(session.realName().isNull());
    QCOMPARE(session.encoding(), QByteArray("UTF-8"));
    QVERIFY(session.socket());
    QVERIFY(session.socket()->inherits("QAbstractSocket"));
}

void tst_IrcSession::testHost_data()
{
    QTest::addColumn<QString>("host");

    QTest::newRow("null") << QString();
    QTest::newRow("empty") << QString("");
    QTest::newRow("space") << QString(" ");
    QTest::newRow("invalid") << QString("invalid");
    QTest::newRow(qPrintable(server.serverAddress().toString())) << server.serverAddress().toString();
}

void tst_IrcSession::testHost()
{
    QFETCH(QString, host);

    IrcSession session;
    session.setHost(host);
    QCOMPARE(session.host(), host);
}

void tst_IrcSession::testPort_data()
{
    QTest::addColumn<int>("port");

    QTest::newRow("-1") << -1;
    QTest::newRow("0") << 0;
    QTest::newRow("6666") << 6666;
    QTest::newRow("6667") << 6667;
    QTest::newRow("6668") << 6668;
    QTest::newRow(qPrintable(QString::number(server.serverPort()))) << static_cast<int>(server.serverPort());
}

void tst_IrcSession::testPort()
{
    QFETCH(int, port);

    IrcSession session;
    session.setPort(port);
    QCOMPARE(session.port(), port);
}

void tst_IrcSession::testUserName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("space") << QString(" ") << QString("");
    QTest::newRow("spaces") << QString(" foo bar ") << QString("foo");
}

void tst_IrcSession::testUserName()
{
    QFETCH(QString, name);
    QFETCH(QString, result);

    IrcSession session;
    session.setUserName(name);
    QCOMPARE(session.userName(), result);
}

void tst_IrcSession::testNickName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("space") << QString(" ") << QString("");
    QTest::newRow("spaces") << QString(" foo bar ") << QString("foo");
}

void tst_IrcSession::testNickName()
{
    QFETCH(QString, name);
    QFETCH(QString, result);

    IrcSession session;
    session.setNickName(name);
    QCOMPARE(session.nickName(), result);
}

void tst_IrcSession::testRealName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("space") << QString(" ") << QString(" ");
    QTest::newRow("spaces") << QString(" foo bar ") << QString(" foo bar ");
}

void tst_IrcSession::testRealName()
{
    QFETCH(QString, name);
    QFETCH(QString, result);

    IrcSession session;
    session.setRealName(name);
    QCOMPARE(session.realName(), result);
}

void tst_IrcSession::testEncoding_data()
{
    QTest::addColumn<QByteArray>("encoding");
    QTest::addColumn<QByteArray>("actual");

    QTest::newRow("null") << QByteArray() << QByteArray("UTF-8");
    QTest::newRow("empty") << QByteArray("") << QByteArray("UTF-8");
    QTest::newRow("space") << QByteArray(" ") << QByteArray("UTF-8");
    QTest::newRow("invalid") << QByteArray("invalid") << QByteArray("UTF-8");
    foreach (const QByteArray& codec, QTextCodec::availableCodecs())
        QTest::newRow(codec) << codec << codec;
}

void tst_IrcSession::testEncoding()
{
    QFETCH(QByteArray, encoding);
    QFETCH(QByteArray, actual);

    IrcSession session;
    session.setEncoding(encoding);
    QCOMPARE(session.encoding(), actual);
}

Q_DECLARE_METATYPE(QAbstractSocket*)
void tst_IrcSession::testSocket_data()
{
    QTest::addColumn<QAbstractSocket*>("socket");

    QTest::newRow("null") << static_cast<QAbstractSocket*>(0);
    QTest::newRow("tcp") << static_cast<QAbstractSocket*>(new QTcpSocket(this));
#ifndef QT_NO_OPENSSL
    QTest::newRow("ssl") << static_cast<QAbstractSocket*>(new QSslSocket(this));
#endif
}

void tst_IrcSession::testSocket()
{
    QFETCH(QAbstractSocket*, socket);

    IrcSession session;
    session.setSocket(socket);
    QCOMPARE(session.socket(), socket);
}

Q_DECLARE_METATYPE(QString*)
Q_DECLARE_METATYPE(IrcMessage*)
void tst_IrcSession::testSession()
{
    qRegisterMetaType<QString*>();
    qRegisterMetaType<IrcMessage*>();

    IrcSession session;
    QSignalSpy connectingSpy(&session, SIGNAL(connecting()));
    QSignalSpy passwordSpy(&session, SIGNAL(password(QString*)));
    QSignalSpy connectedSpy(&session, SIGNAL(connected()));
    QSignalSpy disconnectedSpy(&session, SIGNAL(disconnected()));
    QSignalSpy messageReceivedSpy(&session, SIGNAL(messageReceived(IrcMessage*)));

    QVERIFY(connectingSpy.isValid());
    QVERIFY(passwordSpy.isValid());
    QVERIFY(connectedSpy.isValid());
    QVERIFY(disconnectedSpy.isValid());
    QVERIFY(messageReceivedSpy.isValid());

    session.setUserName("user");
    session.setNickName("nick");
    session.setRealName("real");
    session.setHost(server.serverAddress().toString());
    session.setPort(server.serverPort());

    session.open();
    QVERIFY(server.waitForNewConnection(2000));
    QTcpSocket* serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);

    QVERIFY(session.socket()->waitForConnected());
    QCOMPARE(connectingSpy.count(), 1);
    QCOMPARE(passwordSpy.count(), 1);

    QVERIFY(serverSocket->write(":irc.ser.ver 001 nick :Welcome to the Internet Relay Chat Network nick\r\n"));
    QVERIFY(serverSocket->waitForBytesWritten());
    QVERIFY(session.socket()->waitForReadyRead());
    QCOMPARE(connectedSpy.count(), 1);

    session.close();
    session.socket()->waitForDisconnected();
    QCOMPARE(disconnectedSpy.count(), 1);
}

void tst_IrcSession::testSendCommand()
{
    IrcSession session;
    QVERIFY(!session.sendCommand(0));
    QVERIFY(!session.sendCommand(IrcCommand::createQuit()));

    session.setUserName("user");
    session.setNickName("nick");
    session.setRealName("real");
    session.setHost(server.serverAddress().toString());
    session.setPort(server.serverPort());
    session.open();
    QVERIFY(session.socket()->waitForConnected());

    QVERIFY(!session.sendCommand(0));
    QVERIFY(session.sendCommand(IrcCommand::createQuit()));
    session.close();
}

void tst_IrcSession::testSendData()
{
    IrcSession session;
    QVERIFY(!session.sendData("QUIT"));

    session.setUserName("user");
    session.setNickName("nick");
    session.setRealName("real");
    session.setHost(server.serverAddress().toString());
    session.setPort(server.serverPort());
    session.open();
    QVERIFY(session.socket()->waitForConnected());

    QVERIFY(session.sendData("QUIT"));
    session.close();
}

QTEST_MAIN(tst_IrcSession)

#include "tst_ircsession.moc"
