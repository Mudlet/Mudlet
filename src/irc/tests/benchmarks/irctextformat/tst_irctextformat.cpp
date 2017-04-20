/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irctextformat.h"
#include <QtTest/QtTest>

class tst_IrcTextFormat : public QObject
{
    Q_OBJECT

private slots:
    void testToHtml_data();
    void testToHtml();
};

void tst_IrcTextFormat::testToHtml_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("info") << QString("[freenode-info] if you're at a conference and other people are having trouble connecting, please mention it to staff: http://freenode.net/faq.shtml#gettinghelp");
    QTest::newRow("topic") << QString("Communi 1.2.2 - IRC framework || Home: https://communi.github.io || Docs: https://communi.github.io/doc || MeeGo: http://store.ovi.com/content/219150");
    QTest::newRow("commit") << QString("[communi-desktop] jpnurmi pushed 2 new commits to master: https://github.com/communi/communi-desktop/compare/257ca915a490...8832bfe8d0b8");
    QTest::newRow("welcome") << QString("Welcome to the Communi development lounge. Communi for MeeGo/Symbian users are kindly asked to submit a review in Nokia Store.");
}

void tst_IrcTextFormat::testToHtml()
{
    QFETCH(QString, text);

    IrcTextFormat format;
    QBENCHMARK {
        format.toHtml(text);
    }
}

QTEST_MAIN(tst_IrcTextFormat)

#include "tst_irctextformat.moc"
