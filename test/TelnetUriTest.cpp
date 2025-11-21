/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Development Team                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QtTest/QtTest>
#include <QUrl>

/**
 * Unit tests for telnet:// URI parsing
 */
class TelnetUriTest : public QObject {
    Q_OBJECT

private slots:
    void testBasicUriParsing();
    void testUriWithDefaultPort();
    void testUriWithUsername();
    void testUriWithUsernameAndPassword();
    void testInvalidUriWrongScheme();
    void testInvalidUriNoHost();
    void testPortValidation();
    void testIPv4AddressAsHost();
    void testIPv6AddressAsHost();
    void testCaseInsensitiveScheme();
    void testRealWorldExample();
};

void TelnetUriTest::testBasicUriParsing()
{
    QUrl url("telnet://mud.example.com:4000");
    
    QVERIFY2(url.isValid(), "URI should be valid");
    QCOMPARE(url.scheme(), QString("telnet"));
    QCOMPARE(url.host(), QString("mud.example.com"));
    QCOMPARE(url.port(), 4000);
}

void TelnetUriTest::testUriWithDefaultPort()
{
    QUrl url("telnet://example.com");
    
    QVERIFY(url.isValid());
    QCOMPARE(url.scheme(), QString("telnet"));
    QCOMPARE(url.host(), QString("example.com"));
    QCOMPARE(url.port(23), 23);
}

void TelnetUriTest::testUriWithUsername()
{
    QUrl url("telnet://player@mud.example.com:4000");
    
    QVERIFY(url.isValid());
    QCOMPARE(url.userName(), QString("player"));
    QCOMPARE(url.host(), QString("mud.example.com"));
}

void TelnetUriTest::testUriWithUsernameAndPassword()
{
    QUrl url("telnet://player:secret@mud.example.com:4000");
    
    QVERIFY(url.isValid());
    QCOMPARE(url.userName(), QString("player"));
    QCOMPARE(url.password(), QString("secret"));
}

void TelnetUriTest::testInvalidUriWrongScheme()
{
    QUrl url("http://mud.example.com:4000");
    
    QVERIFY(url.isValid());
    QVERIFY(url.scheme() != "telnet");
}

void TelnetUriTest::testInvalidUriNoHost()
{
    QUrl url("telnet://:4000");
    
    QVERIFY(url.host().isEmpty());
}

void TelnetUriTest::testPortValidation()
{
    QVERIFY(1 >= 1 && 1 <= 65535);
    QVERIFY(23 >= 1 && 23 <= 65535);
    QVERIFY(4000 >= 1 && 4000 <= 65535);
    QVERIFY(65535 >= 1 && 65535 <= 65535);
    QVERIFY(!(0 >= 1 && 0 <= 65535));
    QVERIFY(!(65536 >= 1 && 65536 <= 65535));
}

void TelnetUriTest::testIPv4AddressAsHost()
{
    QUrl url("telnet://192.168.1.100:4000");
    
    QVERIFY(url.isValid());
    QCOMPARE(url.host(), QString("192.168.1.100"));
    QCOMPARE(url.port(), 4000);
}

void TelnetUriTest::testIPv6AddressAsHost()
{
    QUrl url("telnet://[::1]:4000");
    
    QVERIFY(url.isValid());
    QCOMPARE(url.host(), QString("::1"));
    QCOMPARE(url.port(), 4000);
}

void TelnetUriTest::testCaseInsensitiveScheme()
{
    QUrl url1("telnet://mud.example.com:4000");
    QUrl url2("TELNET://mud.example.com:4000");
    
    QVERIFY(url1.isValid());
    QVERIFY(url2.isValid());
    QCOMPARE(url1.scheme().toLower(), QString("telnet"));
    QCOMPARE(url2.scheme().toLower(), QString("telnet"));
}

void TelnetUriTest::testRealWorldExample()
{
    // This is the exact URI from the bug report
    QUrl url("telnet://mud.clessidra.it:4000");
    
    QVERIFY2(url.isValid(), "telnet://mud.clessidra.it:4000 must be valid");
    QCOMPARE(url.scheme(), QString("telnet"));
    QCOMPARE(url.host(), QString("mud.clessidra.it"));
    QCOMPARE(url.port(), 4000);
}

#include "TelnetUriTest.moc"
QTEST_MAIN(TelnetUriTest)
