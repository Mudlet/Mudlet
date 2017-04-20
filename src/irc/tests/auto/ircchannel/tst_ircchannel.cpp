/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircchannel.h"
#include <QtTest/QtTest>
#include <QtCore/QRegExp>

class tst_IrcChannel : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testSignals();
    void testDebug();
};

void tst_IrcChannel::testDefaults()
{
    IrcChannel channel;
    QVERIFY(channel.title().isEmpty());
    QVERIFY(channel.name().isEmpty());
    QVERIFY(channel.prefix().isEmpty());
    QVERIFY(channel.isChannel());
    QVERIFY(channel.toChannel());
    QVERIFY(!channel.connection());
    QVERIFY(!channel.network());
    QVERIFY(!channel.model());
    QVERIFY(!channel.isActive());
    QVERIFY(!channel.isSticky());
    QVERIFY(!channel.isPersistent());
    QVERIFY(channel.mode().isEmpty());
    QVERIFY(channel.topic().isEmpty());
}

void tst_IrcChannel::testSignals()
{
    IrcChannel channel;
    QSignalSpy modeSpy(&channel, SIGNAL(modeChanged(QString)));
    QSignalSpy topicSpy(&channel, SIGNAL(topicChanged(QString)));
    QVERIFY(modeSpy.isValid());
    QVERIFY(topicSpy.isValid());
}

void tst_IrcChannel::testDebug()
{
    QString str;
    QDebug dbg(&str);

    dbg << static_cast<IrcChannel*>(0);
    QCOMPARE(str.trimmed(), QString::fromLatin1("IrcChannel(0x0)"));
    str.clear();

    IrcChannel channel;
    dbg << &channel;
    QVERIFY(QRegExp("IrcChannel\\(0x[0-9A-Fa-f]+\\) ").exactMatch(str));
    str.clear();

    channel.setObjectName("obj");
    dbg << &channel;
    QVERIFY(QRegExp("IrcChannel\\(0x[0-9A-Fa-f]+, name=obj\\) ").exactMatch(str));
    str.clear();

    channel.setPrefix("#");
    channel.setName("communi");
    dbg << &channel;
    QVERIFY(QRegExp("IrcChannel\\(0x[0-9A-Fa-f]+, name=obj, title=#communi\\) ").exactMatch(str));
    str.clear();
}

QTEST_MAIN(tst_IrcChannel)

#include "tst_ircchannel.moc"
