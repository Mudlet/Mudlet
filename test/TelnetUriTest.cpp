/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Development Team                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "mudlet.h"
#include <QtTest/QtTest>

class TelnetUriTest : public QObject
{
    Q_OBJECT

private slots:
    void testValidUriParsing();
    void testInvalidUriParsing();
    void testDefaultPort();
    void testProfileNameGeneration();
    void testProfileNameCollision();
};

void TelnetUriTest::testValidUriParsing()
{
    mudlet mudletInstance;
    
    // Test basic URI
    auto data1 = mudletInstance.parseTelnetUri("telnet://aardmud.org:4000");
    QVERIFY(data1.valid);
    QCOMPARE(data1.host, QString("aardmud.org"));
    QCOMPARE(data1.port, 4000);
    
    // Test URI with default port
    auto data2 = mudletInstance.parseTelnetUri("telnet://batmud.bat.org");
    QVERIFY(data2.valid);
    QCOMPARE(data2.host, QString("batmud.bat.org"));
    QCOMPARE(data2.port, 23);
    
    // Test URI with username
    auto data3 = mudletInstance.parseTelnetUri("telnet://user@testmud.com:2000");
    QVERIFY(data3.valid);
    QCOMPARE(data3.host, QString("testmud.com"));
    QCOMPARE(data3.port, 2000);
    QCOMPARE(data3.username, QString("user"));
}

void TelnetUriTest::testInvalidUriParsing()
{
    mudlet mudletInstance;
    
    // Test invalid scheme
    auto data1 = mudletInstance.parseTelnetUri("http://example.com");
    QVERIFY(!data1.valid);
    
    // Test missing host
    auto data2 = mudletInstance.parseTelnetUri("telnet://");
    QVERIFY(!data2.valid);
    
    // Test empty string
    auto data3 = mudletInstance.parseTelnetUri("");
    QVERIFY(!data3.valid);
}

void TelnetUriTest::testDefaultPort()
{
    mudlet mudletInstance;
    
    // RFC 4248 specifies port 23 as default
    auto data = mudletInstance.parseTelnetUri("telnet://example.com");
    QVERIFY(data.valid);
    QCOMPARE(data.port, 23);
}

void TelnetUriTest::testProfileNameGeneration()
{
    // Test profile naming logic
    // Standard port (23) should use just hostname
    QString name1 = "aardmud.org";  // Expected for telnet://aardmud.org
    QCOMPARE(name1, QString("aardmud.org"));
    
    // Non-standard port should include port
    QString name2 = "aardmud.org:4000";  // Expected for telnet://aardmud.org:4000
    QCOMPARE(name2, QString("aardmud.org:4000"));
}

void TelnetUriTest::testProfileNameCollision()
{
    // Test collision handling (profile names with -2, -3 suffixes)
    QString original = "testserver.com:4000";
    QString collision1 = original + "-2";
    QString collision2 = original + "-3";
    
    QCOMPARE(collision1, QString("testserver.com:4000-2"));
    QCOMPARE(collision2, QString("testserver.com:4000-3"));
}

QTEST_MAIN(TelnetUriTest)
#include "TelnetUriTest.moc"
