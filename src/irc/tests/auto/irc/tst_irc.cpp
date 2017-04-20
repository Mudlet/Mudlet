/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irc.h"
#include "irccore.h"
#include "ircmodel.h"
#include "ircutil.h"
#include <QtTest/QtTest>
#include <QtCore/QRegExp>

class tst_Irc : public QObject
{
    Q_OBJECT

private slots:
    void testCreation();
    void testVersion();

    void testCodeToString_data();
    void testCodeToString();

    void testMetaObject();
    void testMetaTypes();

    void testPrefix_data();
    void testPrefix();

    void testDebug();
};

void tst_Irc::testCreation()
{
    Irc ircStatic;
    Q_UNUSED(ircStatic);

    QScopedPointer<Irc> ircDynamic(new Irc);
    Q_UNUSED(ircDynamic);
}

void tst_Irc::testVersion()
{
    QVERIFY(!Irc::version().isEmpty());
}

void tst_Irc::testCodeToString_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("str");

    QTest::newRow("RPL_WELCOME") << 1 << QString("RPL_WELCOME");
    QTest::newRow("RPL_ISUPPORT") << 5 << QString("RPL_ISUPPORT");
    QTest::newRow("RPL_TOPIC") << 332 << QString("RPL_TOPIC");
    QTest::newRow("RPL_NAMREPLY") << 353 << QString("RPL_NAMREPLY");
    QTest::newRow("RPL_ENDOFNAMES") << 366 << QString("RPL_ENDOFNAMES");

    QTest::newRow("ERR_NOSUCHNICK") << 401 << QString("ERR_NOSUCHNICK");
    QTest::newRow("ERR_NOSUCHCHANNEL") << 403 << QString("ERR_NOSUCHCHANNEL");
    QTest::newRow("ERR_NICKNAMEINUSE") << 433 << QString("ERR_NICKNAMEINUSE");
    QTest::newRow("ERR_OPERONLY") << 520 << QString("ERR_OPERONLY");
}

void tst_Irc::testCodeToString()
{
    QFETCH(int, code);
    QFETCH(QString, str);

    QCOMPARE(Irc::codeToString(code), str);
}

void tst_Irc::testMetaObject()
{
    Irc irc;

    QVERIFY(Irc::staticMetaObject.indexOfEnumerator("Code") != -1);
    QVERIFY(Irc::staticMetaObject.indexOfEnumerator("Color") != -1);
    QVERIFY(Irc::staticMetaObject.indexOfEnumerator("DataRole") != -1);

    QString ver;
    QVERIFY(QMetaObject::invokeMethod(&irc, "version", Q_RETURN_ARG(QString, ver)));
    QCOMPARE(ver, Irc::version());

    QString str;
    QVERIFY(QMetaObject::invokeMethod(&irc, "codeToString", Q_RETURN_ARG(QString, str), Q_ARG(int, Irc::RPL_ISUPPORT)));
    QCOMPARE(str, Irc::codeToString(Irc::RPL_ISUPPORT));
}

void tst_Irc::testMetaTypes()
{
    IrcCore::registerMetaTypes();
    QVERIFY(qMetaTypeId<Irc*>());
    QVERIFY(qMetaTypeId<IrcConnection*>());
    QVERIFY(qMetaTypeId<IrcCommand*>());
    QVERIFY(qMetaTypeId<IrcMessage*>());
    QVERIFY(qMetaTypeId<IrcNetwork*>());

    IrcModel::registerMetaTypes();
    QVERIFY(qMetaTypeId<IrcBuffer*>());
    QVERIFY(qMetaTypeId<IrcBufferModel*>());
    QVERIFY(qMetaTypeId<IrcChannel*>());
    QVERIFY(qMetaTypeId<IrcUser*>());
    QVERIFY(qMetaTypeId<IrcUserModel*>());

    IrcUtil::registerMetaTypes();
    QVERIFY(qMetaTypeId<IrcCommandParser*>());
    QVERIFY(qMetaTypeId<IrcCommandQueue*>());
    QVERIFY(qMetaTypeId<IrcCompleter*>());
    QVERIFY(qMetaTypeId<IrcLagTimer*>());
    QVERIFY(qMetaTypeId<IrcPalette*>());
    QVERIFY(qMetaTypeId<IrcTextFormat*>());
}

void tst_Irc::testPrefix_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("expectedNick");
    QTest::addColumn<QString>("expectedIdent");
    QTest::addColumn<QString>("expectedHost");

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

void tst_Irc::testPrefix()
{
    QFETCH(bool, valid);
    QFETCH(QString, prefix);
    QFETCH(QString, expectedNick);
    QFETCH(QString, expectedIdent);
    QFETCH(QString, expectedHost);

    QString actualNick = Irc::nickFromPrefix(prefix);
    QString actualIdent = Irc::identFromPrefix(prefix);
    QString actualHost = Irc::hostFromPrefix(prefix);

    Q_UNUSED(valid);
    QCOMPARE(expectedNick, actualNick);
    QCOMPARE(expectedIdent, actualIdent);
    QCOMPARE(expectedHost, actualHost);
}

void tst_Irc::testDebug()
{
    QString str;
    QDebug dbg(&str);

    dbg << Irc::RPL_AWAY;
    QCOMPARE(str.trimmed(), QString::fromLatin1("RPL_AWAY"));
    str.clear();

    dbg << Irc::NameRole;
    QCOMPARE(str.trimmed(), QString::fromLatin1("NameRole"));
    str.clear();

    dbg << Irc::Brown;
    QCOMPARE(str.trimmed(), QString::fromLatin1("Brown"));
    str.clear();

    dbg << Irc::SortByActivity;
    QCOMPARE(str.trimmed(), QString::fromLatin1("SortByActivity"));
    str.clear();
}

QTEST_MAIN(tst_Irc)

#include "tst_irc.moc"
