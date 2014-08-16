/*
 * Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
 *
 * This test is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "ircsender.h"
#include <QtTest/QtTest>

class tst_IrcSender : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();

    void testSender_data();
    void testSender();
};

void tst_IrcSender::testDefaults()
{
    IrcSender sender;
    QVERIFY(!sender.isValid());
    QVERIFY(sender.prefix().isNull());
    QVERIFY(sender.name().isNull());
    QVERIFY(sender.user().isNull());
    QVERIFY(sender.host().isNull());
}

void tst_IrcSender::testSender_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("host");

    QTest::newRow("null") << false << QString() << QString() << QString() << QString();
    QTest::newRow("empty") << false << QString("") << QString("") << QString("") << QString("");
    QTest::newRow("trimmed") << true << QString(" n!u@h ") << QString("n") << QString("u") << QString("h");
    QTest::newRow("n!u@h") << true << QString("n!u@h") << QString("n") << QString("u") << QString("h");

    QTest::newRow("n@h") << true << QString("n@h") << QString("n") << QString() << QString("h");
    QTest::newRow("n!u") << true << QString("n!u") << QString("n") << QString("u") << QString();
    QTest::newRow("!u@h") << false << QString("!u@h") << QString() << QString() << QString();
    QTest::newRow("n!@h") << false << QString("n!@h") << QString() << QString() << QString();
    QTest::newRow("n!u@") << false << QString("n!u@") << QString() << QString() << QString();

    QTest::newRow("n !u@h") << false << QString("n !u@h") << QString() << QString() << QString();
    QTest::newRow("n! u@h") << false << QString("n! u@h") << QString() << QString() << QString();
    QTest::newRow("n!u @h") << false << QString("n!u @h") << QString() << QString() << QString();
    QTest::newRow("n!u@ h") << false << QString("n!u@ h") << QString() << QString() << QString();
    QTest::newRow("n ! u @ h") << false << QString("n ! u @ h") << QString() << QString() << QString();
}

void tst_IrcSender::testSender()
{
    QFETCH(bool, valid);
    QFETCH(QString, prefix);
    QFETCH(QString, name);
    QFETCH(QString, user);
    QFETCH(QString, host);

    IrcSender sender(prefix);
    QCOMPARE(sender.isValid(), valid);
    QCOMPARE(sender.name(), name);
    QCOMPARE(sender.user(), user);
    QCOMPARE(sender.host(), host);
}

QTEST_MAIN(tst_IrcSender)

#include "tst_ircsender.moc"
