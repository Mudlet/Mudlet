/*
 * Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
 *
 * This test is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "ircdecoder_p.h"
#include <QtTest/QtTest>
#include <QtCore/QTextCodec>

class tst_IrcDecoder : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();
};

void tst_IrcDecoder::testDefaults()
{
    IrcDecoder decoder;
    QCOMPARE(decoder.encoding(), QByteArray("UTF-8"));
}

QTEST_MAIN(tst_IrcDecoder)

#include "tst_ircdecoder.moc"
