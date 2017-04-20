/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irc.h"
#include "ircpalette.h"
#include "irctextformat.h"
#include <QtTest/QtTest>

class tst_IrcPalette : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testColorNames();
    void testProperties_data();
    void testProperties();
};

void tst_IrcPalette::testDefaults()
{
    IrcTextFormat format;
    QVERIFY(format.palette());
    IrcPalette* palette = format.palette();
    QVERIFY(!palette->colorNames().isEmpty());
    for (int i = Irc::White; i <= Irc::LightGray; ++i)
        QVERIFY(!palette->colorName(i).isEmpty());
    QCOMPARE(palette->colorName(-1, "fallback"), QString("fallback"));
}

void tst_IrcPalette::testColorNames()
{
    IrcTextFormat format;
    QVERIFY(format.palette());
    IrcPalette* palette = format.palette();
    QMap<int, QString> colorNames;
    for (int i = -1; i <= 123; ++i) {
        colorNames.insert(i, QString::number(i));
        palette->setColorName(i, QString::number(i));
        QCOMPARE(palette->colorName(i), QString::number(i));
    }
    QCOMPARE(palette->colorNames(), colorNames);

    QMap<int, QString> dummies;
    for (int i = 0; i < 100; i += 3)
        dummies.insert(i, QString::number(i) + "-dummy");
    palette->setColorNames(dummies);
    QCOMPARE(palette->colorNames(), dummies);
}

void tst_IrcPalette::testProperties_data()
{
    QTest::addColumn<Irc::Color>("color");
    QTest::addColumn<QString>("prop");

    QTest::newRow("white") << Irc::White << "white";
    QTest::newRow("black") << Irc::Black << "black";
    QTest::newRow("blue") << Irc::Blue << "blue";
    QTest::newRow("green") << Irc::Green << "green";
    QTest::newRow("red") << Irc::Red << "red";
    QTest::newRow("brown") << Irc::Brown << "brown";
    QTest::newRow("purple") << Irc::Purple << "purple";
    QTest::newRow("orange") << Irc::Orange << "orange";
    QTest::newRow("yellow") << Irc::Yellow << "yellow";
    QTest::newRow("lightGreen") << Irc::LightGreen << "lightGreen";
    QTest::newRow("cyan") << Irc::Cyan << "cyan";
    QTest::newRow("lightCyan") << Irc::LightCyan << "lightCyan";
    QTest::newRow("lightBlue") << Irc::LightBlue << "lightBlue";
    QTest::newRow("pink") << Irc::Pink << "pink";
    QTest::newRow("gray") << Irc::Gray << "gray";
    QTest::newRow("lightGray") << Irc::LightGray << "lightGray";
}

void tst_IrcPalette::testProperties()
{
    QFETCH(Irc::Color, color);
    QFETCH(QString, prop);

    IrcTextFormat format;
    QVERIFY(format.palette());
    IrcPalette* palette = format.palette();

    QCOMPARE(palette->property(prop.toUtf8()).toString(), prop.toLower());

    QVERIFY(palette->setProperty(prop.toUtf8(), QString("dummy")));
    QCOMPARE(palette->colorName(color), QString("dummy"));
    QCOMPARE(palette->property(prop.toUtf8()).toString(), QString("dummy"));

    palette->setColorName(color, QString("dummier"));
    QCOMPARE(palette->colorName(color), QString("dummier"));
    QCOMPARE(palette->property(prop.toUtf8()).toString(), QString("dummier"));
}

QTEST_MAIN(tst_IrcPalette)

#include "tst_ircpalette.moc"
