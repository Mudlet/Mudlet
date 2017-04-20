/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "irc.h"
#include "irctextformat.h"
#include "ircpalette.h"
#include <QtTest/QtTest>

class tst_IrcTextFormat : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testPlainText_data();
    void testPlainText();
    void testHtml_data();
    void testHtml();
    void testUrls_data();
    void testUrls();
};

void tst_IrcTextFormat::testDefaults()
{
    IrcTextFormat format;
    QVERIFY(format.palette());
    QVERIFY(!format.urlPattern().isEmpty());
    QCOMPARE(format.spanFormat(), IrcTextFormat::SpanStyle);
}

void tst_IrcTextFormat::testPlainText_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    QTest::newRow("bold") << "\02bold\x0f" << "bold";
    QTest::newRow("line-through") << "\x13line-through\x0f" << "line-through";
    QTest::newRow("underline") << "\x15underline\x0f" << "underline";
    QTest::newRow("inverse") << "\x16inverse\x0f" << "inverse";
    QTest::newRow("italic") << "\x1ditalic\x0f" << "italic";
    QTest::newRow("underline") << "\x1funderline\x0f" << "underline";

    IrcTextFormat format;
    QVERIFY(format.palette());
    IrcPalette* palette = format.palette();
    for (int i = Irc::White; i <= Irc::LightGray; ++i) {
        QString color = palette->colorName(i);
        QTest::newRow(color.toUtf8()) << QString("\x03%1%2\x0f").arg(i).arg(color) << color;
    }

    QTest::newRow("dummy \\x03") << "foo\x03 \02bold\x0f bar\x03" << "foo bold bar";
    QTest::newRow("extra \\x0f") << "foo\x0f \02bold\x0f bar\x0f" << "foo bold bar";
    QTest::newRow("background") << QString("foo \x03%1,%1red\x0f on \x03%1,%1red\x03 bar").arg(Irc::Red) << "foo red on red bar";
}

void tst_IrcTextFormat::testPlainText()
{
    QFETCH(QString, input);
    QFETCH(QString, output);

    IrcTextFormat format;
    QCOMPARE(format.toPlainText(input), output);

    format.parse(input);
    QCOMPARE(format.plainText(), output);
}

void tst_IrcTextFormat::testHtml_data()
{
    qRegisterMetaType<IrcTextFormat::SpanFormat>();

    QTest::addColumn<IrcTextFormat::SpanFormat>("span");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    QTest::newRow("style=bold") << IrcTextFormat::SpanStyle << "foo \02bold\x0f and \02bold\02 bar" << "foo <span style='font-weight: bold'>bold</span> and <span style='font-weight: bold'>bold</span> bar";
    QTest::newRow("class=bold") << IrcTextFormat::SpanClass << "foo \02bold\x0f and \02bold\02 bar" << "foo <span class='bold'>bold</span> and <span class='bold'>bold</span> bar";

    QTest::newRow("style=line-through") << IrcTextFormat::SpanStyle << "foo \x13line\x0f and \x13through\x13 bar" << "foo <span style='text-decoration: line-through'>line</span> and <span style='text-decoration: line-through'>through</span> bar";
    QTest::newRow("class=line-through") << IrcTextFormat::SpanClass << "foo \x13line\x0f and \x13through\x13 bar" << "foo <span class='line-through'>line</span> and <span class='line-through'>through</span> bar";

    QTest::newRow("style=underline1") << IrcTextFormat::SpanStyle << "foo \x15under\x0f and \x15line\x15 bar" << "foo <span style='text-decoration: underline'>under</span> and <span style='text-decoration: underline'>line</span> bar";

    QTest::newRow("style=inverse") << IrcTextFormat::SpanStyle << "foo \x16inverse\x0f and \x16inverse\x16 bar" << "foo <span style='text-decoration: inverse'>inverse</span> and <span style='text-decoration: inverse'>inverse</span> bar";
    QTest::newRow("class=inverse") << IrcTextFormat::SpanClass << "foo \x16inverse\x0f and \x16inverse\x16 bar" << "foo <span class='inverse'>inverse</span> and <span class='inverse'>inverse</span> bar";

    QTest::newRow("style=italic") << IrcTextFormat::SpanStyle << "foo \x1ditalic\x0f and \x1ditalic\x1d bar" << "foo <span style='font-style: italic'>italic</span> and <span style='font-style: italic'>italic</span> bar";
    QTest::newRow("class=italic") << IrcTextFormat::SpanClass << "foo \x1ditalic\x0f and \x1ditalic\x1d bar" << "foo <span class='italic'>italic</span> and <span class='italic'>italic</span> bar";

    QTest::newRow("style=underline2") << IrcTextFormat::SpanStyle << "foo \x1funder\x0f and \x1fline\x1f bar" << "foo <span style='text-decoration: underline'>under</span> and <span style='text-decoration: underline'>line</span> bar";

    IrcTextFormat format;
    QVERIFY(format.palette());
    IrcPalette* palette = format.palette();
    for (int i = Irc::White; i <= Irc::LightGray; ++i) {
        QString color = palette->colorName(i);
        QTest::newRow(QString("style=%1").arg(color).toUtf8()) << IrcTextFormat::SpanStyle << QString("foo \x03%1%2\x0f and \x03%1%2\x03 bar").arg(i).arg(color) << QString("foo <span style='color: %1'>%1</span> and <span style='color: %1'>%1</span> bar").arg(color);
        QTest::newRow(QString("class=%1").arg(color).toUtf8()) << IrcTextFormat::SpanClass << QString("foo \x03%1%2\x0f and \x03%1%2\x03 bar").arg(i).arg(color) << QString("foo <span class='%1'>%1</span> and <span class='%1'>%1</span> bar").arg(color);
    }

    QTest::newRow("extra \\x0f") << IrcTextFormat::SpanStyle << "foo\x0f \02bold\x0f bar\x0f" << "foo <span style='font-weight: bold'>bold</span> bar";

    QTest::newRow("style=background") << IrcTextFormat::SpanStyle << QString("foo \x03%1,%1red\x0f on \x03%1,%1red\x03 bar").arg(Irc::Red) << "foo <span style='color: red; background-color: red'>red</span> on <span style='color: red; background-color: red'>red</span> bar";
    QTest::newRow("class=background") << IrcTextFormat::SpanClass << QString("foo \x03%1,%1red\x0f on \x03%1,%1red\x03 bar").arg(Irc::Red) << "foo <span class='red red-background'>red</span> on <span class='red red-background'>red</span> bar";
}

void tst_IrcTextFormat::testHtml()
{
    QFETCH(IrcTextFormat::SpanFormat, span);
    QFETCH(QString, input);
    QFETCH(QString, output);

    IrcTextFormat format;
    format.setSpanFormat(span);
    QCOMPARE(format.toHtml(input), output);

    format.parse(input);
    QCOMPARE(format.html(), output);
}

Q_DECLARE_METATYPE(QList<QUrl>)
void tst_IrcTextFormat::testUrls_data()
{
    qRegisterMetaType<QList<QUrl> >();

    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");
    QTest::addColumn<QList<QUrl> >("urls");

    QString defaultPattern = IrcTextFormat().urlPattern();

    QTest::newRow("www.fi") << defaultPattern << "www.fi" << "<a href='http://www.fi'>www.fi</a>" << (QList<QUrl>() << QUrl("http://www.fi"));
    QTest::newRow("ftp.funet.fi") << defaultPattern << "ftp.funet.fi" << "<a href='ftp://ftp.funet.fi'>ftp.funet.fi</a>" << (QList<QUrl>() << QUrl("ftp://ftp.funet.fi"));
    QTest::newRow("jpnurmi@gmail.com") << defaultPattern << "jpnurmi@gmail.com" << "<a href='mailto:jpnurmi@gmail.com'>jpnurmi@gmail.com</a>" << (QList<QUrl>() << QUrl("mailto:jpnurmi@gmail.com"));
    QTest::newRow("quote") << defaultPattern << "http://en.wikipedia.org/wiki/Shamir's_Secret_Sharing" << "<a href='http://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing'>http://en.wikipedia.org/wiki/Shamir's_Secret_Sharing</a>" << (QList<QUrl>() << QUrl("http://en.wikipedia.org/wiki/Shamir's_Secret_Sharing"));
    QTest::newRow("percent") << defaultPattern << "http://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing" << "<a href='http://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing'>http://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing</a>" << (QList<QUrl>() << QUrl("http://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing"));
    QTest::newRow("parentheses") << defaultPattern << "http://en.wikipedia.org/wiki/Qt_(software)" << "<a href='http://en.wikipedia.org/wiki/Qt_%28software%29'>http://en.wikipedia.org/wiki/Qt_(software)</a>" << (QList<QUrl>() << QUrl("http://en.wikipedia.org/wiki/Qt_(software)"));
    QTest::newRow("hash & comma") << defaultPattern << "https://codereview.qt-project.org/#change,1" << "<a href='https://codereview.qt-project.org/#change,1'>https://codereview.qt-project.org/#change,1</a>" << (QList<QUrl>() << QUrl("https://codereview.qt-project.org/#change,1"));
    QTest::newRow("equal & question & ampersand") << defaultPattern << "https://www.google.no/imghp?hl=en&tab=wi" << "<a href='https://www.google.no/imghp?hl=en&tab=wi'>https://www.google.no/imghp?hl=en&tab=wi</a>" << (QList<QUrl>() << QUrl("https://www.google.no/imghp?hl=en&tab=wi"));
    QTest::newRow("github commits") << defaultPattern << "https://github.com/communi/libcommuni/compare/ebf3c8ea47dc...19d66ddcb122" << "<a href='https://github.com/communi/libcommuni/compare/ebf3c8ea47dc...19d66ddcb122'>https://github.com/communi/libcommuni/compare/ebf3c8ea47dc...19d66ddcb122</a>" << (QList<QUrl>() << QUrl("https://github.com/communi/libcommuni/compare/ebf3c8ea47dc...19d66ddcb122"));
    QTest::newRow("multiple") << defaultPattern << "www.fi ftp.funet.fi jpnurmi@gmail.com" << "<a href='http://www.fi'>www.fi</a> <a href='ftp://ftp.funet.fi'>ftp.funet.fi</a> <a href='mailto:jpnurmi@gmail.com'>jpnurmi@gmail.com</a>" << (QList<QUrl>() << QUrl("http://www.fi") << QUrl("ftp://ftp.funet.fi") << QUrl("mailto:jpnurmi@gmail.com"));
    QTest::newRow("empty pattern") << QString() << "www.fi ftp.funet.fi jpnurmi@gmail.com" << "www.fi ftp.funet.fi jpnurmi@gmail.com" << QList<QUrl>();

    QTest::newRow("info") << defaultPattern
                          << QString("[freenode-info] if you're at a conference and other people are having trouble connecting, please mention it to staff: http://freenode.net/faq.shtml#gettinghelp")
                          << QString("[freenode-info] if you're at a conference and other people are having trouble connecting, please mention it to staff: <a href='http://freenode.net/faq.shtml#gettinghelp'>http://freenode.net/faq.shtml#gettinghelp</a>")
                          << (QList<QUrl>() << QUrl("http://freenode.net/faq.shtml#gettinghelp"));
    QTest::newRow("topic") << defaultPattern
                           << QString("Communi 1.2.2 - IRC framework || Home: https://communi.github.io || Docs: https://communi.github.io/doc || MeeGo: http://store.ovi.com/content/219150")
                           << QString("Communi 1.2.2 - IRC framework || Home: <a href='https://communi.github.io'>https://communi.github.io</a> || Docs: <a href='https://communi.github.io/doc'>https://communi.github.io/doc</a> || MeeGo: <a href='http://store.ovi.com/content/219150'>http://store.ovi.com/content/219150</a>")
                           << (QList<QUrl>() << QUrl("https://communi.github.io") << QUrl("https://communi.github.io/doc") << QUrl("http://store.ovi.com/content/219150"));
    QTest::newRow("commit") << defaultPattern
                            << QString("[communi-desktop] jpnurmi pushed 2 new commits to master: https://github.com/communi/communi-desktop/compare/257ca915a490...8832bfe8d0b8")
                            << QString("[communi-desktop] jpnurmi pushed 2 new commits to master: <a href='https://github.com/communi/communi-desktop/compare/257ca915a490...8832bfe8d0b8'>https://github.com/communi/communi-desktop/compare/257ca915a490...8832bfe8d0b8</a>")
                            << (QList<QUrl>() << QUrl("https://github.com/communi/communi-desktop/compare/257ca915a490...8832bfe8d0b8"));
    QTest::newRow("with protocol") << defaultPattern
                                   << QString("aa http://www.fi bb ftp://ftp.funet.fi cc")
                                   << QString("aa <a href='http://www.fi'>http://www.fi</a> bb <a href='ftp://ftp.funet.fi'>ftp://ftp.funet.fi</a> cc")
                                   << (QList<QUrl>() << QUrl("http://www.fi") << QUrl("ftp://ftp.funet.fi"));
    QTest::newRow("without protocol") << defaultPattern
                                      << QString("aa www.fi bb ftp.funet.fi cc jpnurmi@gmail.com dd")
                                      << QString("aa <a href='http://www.fi'>www.fi</a> bb <a href='ftp://ftp.funet.fi'>ftp.funet.fi</a> cc <a href='mailto:jpnurmi@gmail.com'>jpnurmi@gmail.com</a> dd")
                                      << (QList<QUrl>() << QUrl("http://www.fi") << QUrl("ftp://ftp.funet.fi") << QUrl("mailto:jpnurmi@gmail.com"));
}

void tst_IrcTextFormat::testUrls()
{
    QFETCH(QString, pattern);
    QFETCH(QString, input);
    QFETCH(QString, output);
    QFETCH(QList<QUrl>, urls);

    IrcTextFormat format;
    format.setUrlPattern(pattern);
    QCOMPARE(format.urlPattern(), pattern);
    QCOMPARE(format.toHtml(input), output);

    format.parse(input);
    QCOMPARE(format.html(), output);
    QCOMPARE(format.urls(), urls);
}

QTEST_MAIN(tst_IrcTextFormat)

#include "tst_irctextformat.moc"
