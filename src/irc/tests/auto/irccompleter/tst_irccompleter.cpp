/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irccompleter.h"
#include "ircbuffermodel.h"
#include "irccommandparser.h"
#include "ircchannel.h"
#include "ircbuffer.h"
#include <QtTest/QtTest>
#include "tst_ircclientserver.h"
#include "tst_ircdata.h"

class tst_IrcCompleter : public tst_IrcClientServer
{
    Q_OBJECT

private slots:
    void testSuffix();
    void testBuffer();
    void testParser();

    void testCompletion_data();
    void testCompletion();

    void testReset();
};

void tst_IrcCompleter::testSuffix()
{
    IrcCompleter completer;
    QCOMPARE(completer.suffix(), QString(":"));
    QCOMPARE(completer.property("suffix").toString(), QString(":"));

    QSignalSpy spy(&completer, SIGNAL(suffixChanged(QString)));
    QVERIFY(spy.isValid());

    completer.setSuffix(",");
    QCOMPARE(completer.suffix(), QString(","));
    QCOMPARE(completer.property("suffix").toString(), QString(","));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).toString(), QString(","));
}

void tst_IrcCompleter::testBuffer()
{
    qRegisterMetaType<IrcBuffer*>("IrcBuffer*");

    IrcCompleter completer;
    QVERIFY(!completer.buffer());

    QSignalSpy spy(&completer, SIGNAL(bufferChanged(IrcBuffer*)));
    QVERIFY(spy.isValid());

    IrcBuffer* buffer = new IrcBuffer(&completer);
    completer.setBuffer(buffer);
    QCOMPARE(completer.buffer(), buffer);
    QCOMPARE(completer.property("buffer").value<IrcBuffer*>(), buffer);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).value<IrcBuffer*>(), buffer);

    completer.setBuffer(0);
    QVERIFY(!completer.buffer());
    QCOMPARE(spy.count(), 2);
    QVERIFY(!spy.last().at(0).value<IrcBuffer*>());
}

void tst_IrcCompleter::testParser()
{
    qRegisterMetaType<IrcCommandParser*>("IrcCommandParser*");

    IrcCompleter completer;
    QVERIFY(!completer.parser());

    QSignalSpy spy(&completer, SIGNAL(parserChanged(IrcCommandParser*)));
    QVERIFY(spy.isValid());

    IrcCommandParser* parser = new IrcCommandParser(&completer);
    completer.setParser(parser);
    QCOMPARE(completer.parser(), parser);
    QCOMPARE(completer.property("parser").value<IrcCommandParser*>(), parser);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).value<IrcCommandParser*>(), parser);

    completer.setParser(0);
    QVERIFY(!completer.parser());
    QCOMPARE(spy.count(), 2);
    QVERIFY(!spy.last().at(0).value<IrcCommandParser*>());
}

Q_DECLARE_METATYPE(QList<int>)
void tst_IrcCompleter::testCompletion_data()
{
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("cursor");
    QTest::addColumn<QStringList>("completions");
    QTest::addColumn<QList<int> >("positions");

    for (int i = -1; i <= 3; ++i)
        QTest::newRow("/j @ " + QByteArray::number(i)) << QString() << "/j" << i << QStringList("/JOIN ") << (QList<int>() << QString("/JOIN ").length());

    QTest::newRow("/q #2") << QString() << "/q" << QString("/q").length()
                       << (QStringList() << "/QUERY " << "/QUIT ")
                       << (QList<int>() << QString("/QUERY ").length() << QString("/QUIT ").length());

    QTest::newRow("/QUERY q") << QString() << "/quer q " << QString("/quer").length()
                       << (QStringList("/QUERY q "))
                       << (QList<int>() << QString("/QUERY ").length());

    QTest::newRow("/query q") << QString() << "/query q" << QString("/query q").length()
                       << (QStringList() << "/query quackgyver " << "/query quelx ")
                       << (QList<int>() << QString("/query quackgyver ").length() << QString("/query quelx ").length());

    QTest::newRow("buffers") << QString() << "q" << QString("q").length()
                       << (QStringList() << "quackgyver " << "quelx ")
                       << (QList<int>() << QString("quackgyver ").length() << QString("quelx ").length());

    QTest::newRow("repeat") << QString() << "#freenode " << QString("#freenode ").length()
                       << (QStringList() << "#freenode " << "#freenode ")
                       << (QList<int>() << QString("#freenode ").length() << QString("#freenode ").length());

    QStringList names1;
    QStringList names2;
    QList<int> positions;
    foreach (const QString& name, tst_IrcData::names()) {
        if (name.startsWith("je", Qt::CaseInsensitive)) {
            names1 += name + ": ";
            names2 += name + ", ";
            positions += name.length() + 2;
        }
    }
    QTest::newRow("je...:") << ":" << "je" << 1 << names1 << positions;
    QTest::newRow("je...,") << "," << "je" << 1 << names2 << positions;

    names1.clear();
    names2.clear();
    positions.clear();
    foreach (const QString& name, tst_IrcData::names()) {
        if (name.startsWith("sa", Qt::CaseInsensitive)) {
            names1 += "... " + name + " ";
            positions += QString("... ").length() + name.length() + QString(" ").length();
        }
    }
    QTest::newRow("... sa") << QString() << "... sa" << QString("... ").length() << names1 << positions;

    QTest::newRow("spaces") << QString() << "/quit  foo  #free  rest... " << QString("/quit  foo  #free ").length()
                       << QStringList("/quit  foo  #freenode  rest... ")
                       << (QList<int>() << QString("/quit  foo  #freenode ").length());
}

void tst_IrcCompleter::testCompletion()
{
    QFETCH(QString, suffix);
    QFETCH(QString, text);
    QFETCH(int, cursor);
    QFETCH(QStringList, completions);
    QFETCH(QList<int>, positions);

    IrcBufferModel model(connection);

    connection->open();
    waitForOpened();
    waitForWritten(tst_IrcData::welcome());
    waitForWritten(tst_IrcData::join());

    model.add("qout");
    model.add("qtassistant");

    IrcCommandParser parser;
    parser.setTriggers(QStringList("/"));
    parser.addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
    parser.addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
    parser.addCommand(IrcCommand::Kick, "KICK (<#channel>) <nick> (<reason...>)");
    parser.addCommand(IrcCommand::CtcpAction, "ME [target] <message...>");
    parser.addCommand(IrcCommand::CtcpAction, "ACTION <target> <message...>");
    parser.addCommand(IrcCommand::Custom, "QUERY <user>");
    parser.addCommand(IrcCommand::Quit, "QUIT (<message...>)");

    IrcCompleter completer;
    completer.setSuffix(suffix);
    completer.setBuffer(model.get(0));
    completer.setParser(&parser);

    QSignalSpy spy(&completer, SIGNAL(completed(QString,int)));
    QVERIFY(spy.isValid());

    for (int i = 0; i < completions.count(); ++i) {
        completer.complete(text, cursor);
        QCOMPARE(spy.count(), i + 1);
        QCOMPARE(spy.last().at(0).toString(), completions.at(i));
        QCOMPARE(spy.last().at(1).toInt(), positions.at(i));
    }
}

void tst_IrcCompleter::testReset()
{
    IrcBufferModel model(connection);
    connection->open();
    waitForOpened();
    waitForWritten(tst_IrcData::welcome());
    waitForWritten(tst_IrcData::join());
    IrcChannel* channel = model.get(0)->toChannel();
    QVERIFY(channel);

    IrcCompleter completer;
    completer.setBuffer(channel);

    QSignalSpy spy(&completer, SIGNAL(completed(QString,int)));
    QVERIFY(spy.isValid());

    completer.complete("Guest", 5);
    QCOMPARE(spy.count(), 1);
    QString guest1 = spy.last().at(0).toString();
    QVERIFY(guest1.startsWith("Guest"));

    completer.complete("Guest", 5);
    QCOMPARE(spy.count(), 2);
    QString guest2 = spy.last().at(0).toString();
    QVERIFY(guest2.startsWith("Guest"));
    QVERIFY(guest2 != guest1);

    completer.reset();
    completer.complete("Guest", 5);
    QCOMPARE(spy.count(), 3);
    QString guest3 = spy.last().at(0).toString();
    QCOMPARE(guest3, guest1);
}

QTEST_MAIN(tst_IrcCompleter)

#include "tst_irccompleter.moc"
