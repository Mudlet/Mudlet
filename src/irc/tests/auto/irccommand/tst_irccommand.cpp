/*
 * Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
 *
 * This test is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "irccommand.h"
#include <QtTest/QtTest>

class tst_IrcCommand : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();

    void testAway();
    void testCtcpAction();
    void testCtcpReply();
    void testCtcpRequest();
    void testInvite();
    void testJoin();
    void testKick();
    void testList();
    void testMessage();
    void testMode();
    void testNames();
    void testNick();
    void testNotice();
    void testPart();
    void testPing();
    void testPong();
    void testQuit();
    void testQuote();
    void testTopic();
    void testWho();
    void testWhois();
    void testWhowas();
};

void tst_IrcCommand::testDefaults()
{
    IrcCommand cmd;
    QVERIFY(cmd.type() == IrcCommand::Custom);
    QVERIFY(cmd.parameters().isEmpty());
}

void tst_IrcCommand::testAway()
{
}

void tst_IrcCommand::testCtcpAction()
{
}

void tst_IrcCommand::testCtcpReply()
{
}

void tst_IrcCommand::testCtcpRequest()
{
}

void tst_IrcCommand::testInvite()
{
}

void tst_IrcCommand::testJoin()
{
}

void tst_IrcCommand::testKick()
{
}

void tst_IrcCommand::testList()
{
}

void tst_IrcCommand::testMessage()
{
}

void tst_IrcCommand::testMode()
{
}

void tst_IrcCommand::testNames()
{
}

void tst_IrcCommand::testNick()
{
}

void tst_IrcCommand::testNotice()
{
}

void tst_IrcCommand::testPart()
{
}

void tst_IrcCommand::testPing()
{
}

void tst_IrcCommand::testPong()
{
}

void tst_IrcCommand::testQuit()
{
}

void tst_IrcCommand::testQuote()
{
}

void tst_IrcCommand::testTopic()
{
}

void tst_IrcCommand::testWho()
{
}

void tst_IrcCommand::testWhois()
{
}

void tst_IrcCommand::testWhowas()
{
}

QTEST_MAIN(tst_IrcCommand)

#include "tst_irccommand.moc"
