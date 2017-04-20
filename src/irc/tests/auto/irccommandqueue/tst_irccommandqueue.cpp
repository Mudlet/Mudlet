/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irccommandqueue.h"
#include "ircconnection.h"
#include "irccommand.h"
#include "ircfilter.h"
#include <QtTest/QtTest>

#include "tst_ircclientserver.h"
#include "tst_ircdata.h"

class tst_IrcCommandQueue : public tst_IrcClientServer
{
    Q_OBJECT

private slots:
    void testBatch();
    void testInterval();
    void testConnection();
    void testSize();
    void testClear();
    void testFlush();
    void testQuit();
};

void tst_IrcCommandQueue::testBatch()
{
    IrcCommandQueue queue;
    QCOMPARE(queue.batch(), 3);
    queue.setBatch(5);
    QCOMPARE(queue.batch(), 5);
}

void tst_IrcCommandQueue::testInterval()
{
    IrcCommandQueue queue;
    QCOMPARE(queue.interval(), 2);
    queue.setInterval(5);
    QCOMPARE(queue.interval(), 5);
}

void tst_IrcCommandQueue::testConnection()
{
    IrcConnection connection1;
    IrcCommandQueue queue(&connection1);
    QCOMPARE(queue.connection(), &connection1);

    IrcConnection connection2;
    queue.setConnection(&connection2);
    QCOMPARE(queue.connection(), &connection2);
}

void tst_IrcCommandQueue::testSize()
{
    IrcCommandQueue queue(connection);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(waitForWritten(tst_IrcData::welcome()));

    for (int i = 1; i <= 10; ++i) {
        connection->sendCommand(IrcCommand::createAway());
        QCOMPARE(queue.size(), i);
    }
}

void tst_IrcCommandQueue::testClear()
{
    IrcCommandQueue queue(connection);
    QCOMPARE(queue.size(), 0);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(waitForWritten(tst_IrcData::welcome()));

    for (int i = 1; i <= 10; ++i)
        connection->sendCommand(IrcCommand::createAway());

    QCOMPARE(queue.size(), 10);
    queue.clear();
    QCOMPARE(queue.size(), 0);
}

class TestCommandFilter : public QObject, public IrcCommandFilter
{
    Q_OBJECT
    Q_INTERFACES(IrcCommandFilter)

public:
    TestCommandFilter(IrcConnection* connection) { connection->installCommandFilter(this); }
    bool commandFilter(IrcCommand *command) { commands += command; return true; }
    QList<IrcCommand*> commands;
};


void tst_IrcCommandQueue::testFlush()
{
    TestCommandFilter filter(connection);
    IrcCommandQueue queue(connection);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(waitForWritten(tst_IrcData::welcome()));

    filter.commands.clear();

    for (int i = 0; i < 10; ++i)
        connection->sendCommand(IrcCommand::createAway());

    QVERIFY(filter.commands.isEmpty());
    QCOMPARE(queue.size(), 10);

    queue.flush();
    QCOMPARE(filter.commands.size(), 10);
    QCOMPARE(queue.size(), 0);
}

void tst_IrcCommandQueue::testQuit()
{
    TestCommandFilter filter(connection);
    IrcCommandQueue queue(connection);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(waitForWritten(tst_IrcData::welcome()));

    filter.commands.clear();

    for (int i = 0; i < 3; ++i)
        connection->sendCommand(IrcCommand::createAway());

    QVERIFY(filter.commands.isEmpty());
    QCOMPARE(queue.size(), 3);

    connection->sendCommand(IrcCommand::createQuit());
    QCOMPARE(filter.commands.size(), 4);
    QCOMPARE(queue.size(), 0);
}

QTEST_MAIN(tst_IrcCommandQueue)

#include "tst_irccommandqueue.moc"
