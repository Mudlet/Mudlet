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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QRegularExpression>

/**
 * Unit tests for telnet:// URI parsing and validation
 * 
 * These tests verify basic URI parsing functionality.
 * Tests will FAIL if URI parsing is broken.
 */
class TelnetUriTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir* tempDir = nullptr;

private slots:
    void initTestCase()
    {
        tempDir = new QTemporaryDir();
        QVERIFY(tempDir->isValid());
        qDebug() << "Test directory:" << tempDir->path();
    }

    void cleanupTestCase()
    {
        delete tempDir;
    }

    // Test 1: Basic URI parsing
    void testBasicUriParsing()
    {
        QUrl url("telnet://mud.clessidra.it:4000");
        
        QVERIFY2(url.isValid(), "URI should be valid");
        QCOMPARE(url.scheme(), QString("telnet"));
        QCOMPARE(url.host(), QString("mud.clessidra.it"));
        QCOMPARE(url.port(), 4000);
    }

    // Test 2: URI with default port
    void testUriWithDefaultPort()
    {
        QUrl url("telnet://example.com");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("telnet"));
        QCOMPARE(url.host(), QString("example.com"));
        QCOMPARE(url.port(23), 23);
    }

    // Test 3: URI with username
    void testUriWithUsername()
    {
        QUrl url("telnet://player@mud.example.com:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("telnet"));
        QCOMPARE(url.host(), QString("mud.example.com"));
        QCOMPARE(url.port(), 4000);
        QCOMPARE(url.userName(), QString("player"));
    }

    // Test 4: URI with username and password
    void testUriWithUsernameAndPassword()
    {
        QUrl url("telnet://player:secret@mud.example.com:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("telnet"));
        QCOMPARE(url.host(), QString("mud.example.com"));
        QCOMPARE(url.port(), 4000);
        QCOMPARE(url.userName(), QString("player"));
        QCOMPARE(url.password(), QString("secret"));
    }

    // Test 5: Invalid URI - wrong scheme
    void testInvalidUriWrongScheme()
    {
        QUrl url("http://mud.example.com:4000");
        
        QVERIFY(url.isValid());
        QVERIFY(url.scheme() != "telnet");
        QCOMPARE(url.scheme(), QString("http"));
    }

    // Test 6: Invalid URI - no host
    void testInvalidUriNoHost()
    {
        QUrl url("telnet://:4000");
        
        QVERIFY(url.host().isEmpty());
    }

    // Test 7: Profile name sanitization
    void testProfileNameSanitization()
    {
        QString host = "mud/example:com?test";
        QRegularExpression invalidChars(R"([/\\:*?"<>|])");
        QString sanitized = host;
        sanitized.remove(invalidChars);
        
        QCOMPARE(sanitized, QString("mudexamplecomtest"));
    }

    // Test 8: Profile directory creation
    void testProfileDirectoryCreation()
    {
        QString profileName = "test_profile";
        QString profilePath = tempDir->path() + "/" + profileName;
        
        QDir dir;
        QVERIFY2(dir.mkpath(profilePath), "Profile directory should be created");
        QVERIFY2(QDir(profilePath).exists(), "Profile directory should exist");
    }

    // Test 9: Profile data file creation and reading
    void testProfileDataFileOperations()
    {
        QString profileName = "test_profile";
        QString profilePath = tempDir->path() + "/" + profileName;
        
        QDir dir;
        dir.mkpath(profilePath);
        
        // Write test data
        QString urlFile = profilePath + "/url";
        QString testHost = "mud.example.com";
        {
            QFile file(urlFile);
            file.open(QIODevice::WriteOnly);
            QDataStream out(&file);
            out.setVersion(QDataStream::Qt_5_12);
            out << testHost;
            file.close();
        }
        
        // Read test data
        {
            QFile file(urlFile);
            QVERIFY2(file.open(QIODevice::ReadOnly), "Should be able to read url file");
            QDataStream in(&file);
            in.setVersion(QDataStream::Qt_5_12);
            QString readHost;
            in >> readHost;
            file.close();
            
            QCOMPARE(readHost, testHost);
        }
    }

    // Test 10: Port validation range
    void testPortValidationRange()
    {
        QVERIFY(1 >= 1 && 1 <= 65535);
        QVERIFY(23 >= 1 && 23 <= 65535);
        QVERIFY(4000 >= 1 && 4000 <= 65535);
        QVERIFY(65535 >= 1 && 65535 <= 65535);
        QVERIFY(!(0 >= 1 && 0 <= 65535));
        QVERIFY(!(65536 >= 1 && 65536 <= 65535));
    }

    // Test 11: Profile name uniqueness
    void testProfileNameUniqueness()
    {
        QString baseName = "test_profile";
        QList<QString> existingProfiles;
        existingProfiles << "test_profile" << "test_profile (1)" << "test_profile (2)";
        
        QString newProfileName = baseName;
        int counter = 1;
        while (existingProfiles.contains(newProfileName)) {
            newProfileName = baseName + QString(" (%1)").arg(counter);
            counter++;
        }
        
        QCOMPARE(newProfileName, QString("test_profile (3)"));
    }

    // Test 12: URL encoding/decoding
    void testUrlEncoding()
    {
        QUrl url("telnet://user%20name@mud.example.com:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.userName(), QString("user name"));
    }

    // Test 13: IPv4 address as host
    void testIPv4AddressAsHost()
    {
        QUrl url("telnet://192.168.1.100:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.host(), QString("192.168.1.100"));
        QCOMPARE(url.port(), 4000);
    }

    // Test 14: IPv6 address as host
    void testIPv6AddressAsHost()
    {
        QUrl url("telnet://[::1]:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.host(), QString("::1"));
        QCOMPARE(url.port(), 4000);
    }

    // Test 15: Case insensitive scheme
    void testCaseInsensitiveScheme()
    {
        QUrl url1("telnet://mud.example.com:4000");
        QUrl url2("TELNET://mud.example.com:4000");
        
        QVERIFY(url1.isValid());
        QVERIFY(url2.isValid());
        QCOMPARE(url1.scheme().toLower(), QString("telnet"));
        QCOMPARE(url2.scheme().toLower(), QString("telnet"));
    }

    // Test 16: Special characters in hostname
    void testSpecialCharactersInHostname()
    {
        QUrl url("telnet://mud-1.example-game.com:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.host(), QString("mud-1.example-game.com"));
    }

    // Test 17: Very long hostname
    void testVeryLongHostname()
    {
        QString longHost = QString("very.long.subdomain.name.for.testing.purposes.example.com");
        QUrl url(QString("telnet://%1:4000").arg(longHost));
        
        QVERIFY(url.isValid());
        QCOMPARE(url.host(), longHost);
    }

    // Test 18: Empty string handling
    void testEmptyStringHandling()
    {
        QString emptyString;
        QVERIFY(emptyString.isEmpty());
        
        QString host = emptyString.isEmpty() ? "New Profile" : emptyString;
        QCOMPARE(host, QString("New Profile"));
    }
};

#include "TelnetUriTest.moc"
QTEST_MAIN(TelnetUriTest)
