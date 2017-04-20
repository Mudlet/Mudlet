/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irclagtimer.h"
#include "ircconnection.h"
#include "tst_ircclientserver.h"
#include "tst_ircdata.h"
#include <QtTest/QtTest>

class tst_IrcLagTimer : public tst_IrcClientServer
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testInterval();
    void testConnection();
    void testLag();
};

void tst_IrcLagTimer::testDefaults()
{
    IrcLagTimer timer;
    QCOMPARE(timer.lag(), qint64(-1));
    QVERIFY(!timer.connection());
    QCOMPARE(timer.interval(), 60);
}

void tst_IrcLagTimer::testInterval()
{
    IrcLagTimer timer;
    timer.setInterval(INT_MIN);
    QCOMPARE(timer.interval(), INT_MIN);
    timer.setInterval(0);
    QCOMPARE(timer.interval(), 0);
    timer.setInterval(INT_MAX);
    QCOMPARE(timer.interval(), INT_MAX);
}

void tst_IrcLagTimer::testConnection()
{
    IrcLagTimer timer(connection);
    QCOMPARE(timer.connection(), connection.data());
    timer.setConnection(0);
    QVERIFY(!timer.connection());
    timer.setConnection(connection);
    QCOMPARE(timer.connection(), connection.data());
}

void tst_IrcLagTimer::testLag()
{
#if QT_VERSION >= 0x040700
    IrcLagTimer timer(connection);

    QSignalSpy lagSpy(&timer, SIGNAL(lagChanged(qint64)));
    QVERIFY(lagSpy.isValid());
    int lagCount = 0;

    connection->open();
    QVERIFY(waitForOpened());

    QCOMPARE(timer.lag(), -1ll);

    QVERIFY(waitForWritten(tst_IrcData::welcome()));

    // cheat a bit to avoid waiting a 1s interval at minimum...
    QMetaObject::invokeMethod(&timer, "_irc_pingServer");
    QVERIFY(clientSocket->waitForBytesWritten(1000));
    QVERIFY(serverSocket->waitForReadyRead(1000));

    QRegExp rx("PING communi/(\\d+)");
    QString written = QString::fromUtf8(serverSocket->readAll());
    QVERIFY(rx.indexIn(written) != -1);

    waitForWritten(QString(":irc.ser.ver PONG communi communi/%1").arg(QDateTime::currentMSecsSinceEpoch() - 1234ll).toUtf8());
    QVERIFY(timer.lag() >= 1234ll);
    QCOMPARE(lagSpy.count(), ++lagCount);
    QVERIFY(lagSpy.last().at(0).toLongLong() >= 1234ll);

    timer.setConnection(0);
    QCOMPARE(timer.lag(), -1ll);
    QCOMPARE(lagSpy.count(), ++lagCount);
    QCOMPARE(lagSpy.last().at(0).toLongLong(), -1ll);

    timer.setConnection(connection);
    QCOMPARE(timer.lag(), -1ll);
    QCOMPARE(lagSpy.count(), lagCount);

    waitForWritten(QString(":irc.ser.ver PONG communi communi/%1").arg(QDateTime::currentMSecsSinceEpoch() - 4321ll).toUtf8());
    QVERIFY(timer.lag() >= 4321ll);
    QCOMPARE(lagSpy.count(), ++lagCount);
    QVERIFY(lagSpy.last().at(0).toLongLong() >= 4321ll);

    connection->close();
    QCOMPARE(timer.lag(), -1ll);
    QCOMPARE(lagSpy.count(), ++lagCount);
    QCOMPARE(lagSpy.last().at(0).toLongLong(), -1ll);
#endif // QT_VERSION >= 0x040700
}

QTEST_MAIN(tst_IrcLagTimer)

#include "tst_irclagtimer.moc"
