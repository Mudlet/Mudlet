/*
 * Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
 *
 * This test is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "ircparser_p.h"
#include <QtTest/QtTest>

class tst_IrcParser : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();

    void testParse_data();
    void testParse();
};

void tst_IrcParser::testDefaults()
{
    IrcParser parser;
    QVERIFY(parser.prefix().isNull());
    QVERIFY(parser.command().isNull());
    QVERIFY(parser.params().isEmpty());
}

typedef QList<QByteArray> QByteArrayList;
Q_DECLARE_METATYPE(QByteArrayList)
void tst_IrcParser::testParse_data()
{
    QTest::addColumn<bool>("result");
    QTest::addColumn<QByteArray>("line");
    QTest::addColumn<QByteArray>("prefix");
    QTest::addColumn<QByteArray>("command");
    QTest::addColumn<QList<QByteArray> >("params");

    QTest::newRow("null") << false << QByteArray() << QByteArray() << QByteArray() << QByteArrayList();
    QTest::newRow("empty") << false << QByteArray("") << QByteArray() << QByteArray() << QByteArrayList();
    QTest::newRow("space") << false << QByteArray(" ") << QByteArray() << QByteArray() << QByteArrayList();

    QTest::newRow(":pfx") << false << QByteArray(":pfx") << QByteArray("pfx") << QByteArray() << QByteArrayList();
    QTest::newRow("cmd") << true << QByteArray("cmd") << QByteArray() << QByteArray("cmd") << QByteArrayList();
    QTest::newRow(":pfx cmd") << true << QByteArray(":pfx cmd") << QByteArray("pfx") << QByteArray("cmd") << QByteArrayList();

    QTest::newRow(":pfx cmd 1 2 3") << true << QByteArray(":pfx cmd 1 2 3") << QByteArray("pfx") << QByteArray("cmd") << (QByteArrayList() << "1" << "2" << "3");
    QTest::newRow(":pfx cmd :1 2 3") << true << QByteArray(":pfx cmd :1 2 3") << QByteArray("pfx") << QByteArray("cmd") << (QByteArrayList() << "1 2 3");
    QTest::newRow(":pfx cmd 1 :2 3") << true << QByteArray(":pfx cmd 1 :2 3") << QByteArray("pfx") << QByteArray("cmd") << (QByteArrayList() << "1" << "2 3");
    QTest::newRow(":pfx cmd 1 2 :3") << true << QByteArray(":pfx cmd 1 2 :3") << QByteArray("pfx") << QByteArray("cmd") << (QByteArrayList() << "1" << "2" << "3");
}

void tst_IrcParser::testParse()
{
    QFETCH(bool, result);
    QFETCH(QByteArray, line);
    QFETCH(QByteArray, prefix);
    QFETCH(QByteArray, command);
    QFETCH(QByteArrayList, params);

    IrcParser parser;
    QCOMPARE(parser.parse(line), result);
    QCOMPARE(parser.prefix(), prefix);
    QCOMPARE(parser.command(), command);
    QCOMPARE(parser.params(), params);
}

QTEST_MAIN(tst_IrcParser)

#include "tst_ircparser.moc"
