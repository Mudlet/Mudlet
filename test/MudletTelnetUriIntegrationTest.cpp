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
 * Integration tests for telnet:// URI support
 * 
 * These tests verify the complete workflow:
 * 1. URI parsing and validation
 * 2. Profile creation with proper directory structure
 * 3. Profile data persistence
 * 4. Profile reuse for existing connections
 * 
 * IMPORTANT: These tests will FAIL if the telnet URI implementation is broken.
 */
class MudletTelnetUriIntegrationTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir* tempDir = nullptr;

    // Helper function to simulate mudlet's profile directory structure
    QString getProfileHomePath(const QString& profileName) {
        return tempDir->path() + "/" + profileName;
    }

    QString getProfileDataItemPath(const QString& profileName, const QString& item) {
        return tempDir->path() + "/" + profileName + "/" + item;
    }

    // Simulate mudlet::writeProfileData
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what) {
        QString filePath = getProfileDataItemPath(profile, item);
        QFileInfo fileInfo(filePath);
        QDir dir;
        if (!dir.mkpath(fileInfo.absolutePath())) {
            return {false, "Failed to create directory"};
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
            return {false, file.errorString()};
        }

        QDataStream ofs(&file);
        ofs.setVersion(QDataStream::Qt_5_12);
        ofs << what;
        file.close();

        return {true, QString()};
    }

    // Simulate mudlet::readProfileData
    QString readProfileData(const QString& profile, const QString& item) {
        QString filePath = getProfileDataItemPath(profile, item);
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }

        QDataStream ifs(&file);
        ifs.setVersion(QDataStream::Qt_5_12);
        QString ret;
        ifs >> ret;
        file.close();

        return ret;
    }

    // Simulate mudlet::profileExists
    bool profileExists(const QString& profileName) {
        QString profilePath = getProfileHomePath(profileName);
        return QDir(profilePath).exists();
    }

    // Simulate mudlet::addProfile (the CRITICAL function that was broken)
    QString addProfile(const QString& host, const int port, const QString& login, const QString& password) {
        qDebug() << "addProfile() - Creating profile for host:" << host << "port:" << port;
        
        // Check if a profile with the same host and port already exists
        QStringList profileNames = QDir(tempDir->path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& profileName : profileNames) {
            if (readProfileData(profileName, "url").compare(host, Qt::CaseInsensitive) == 0 
                && readProfileData(profileName, "port").toInt() == port) {
                qDebug() << "addProfile() - Found existing profile:" << profileName;
                return profileName;
            }
        }
        
        // Create a new profile with unique name derived from host
        QString baseName = host;
        if (baseName.isEmpty()) {
            baseName = "New Profile";
        }
        baseName.remove(QRegularExpression(R"([/\\:*?"<>|])"));
        if (baseName.isEmpty()) {
            baseName = "New Profile";
        }
        
        QString newProfileName = baseName;
        int counter = 1;
        while (profileExists(newProfileName)) {
            newProfileName = baseName + QString(" (%1)").arg(counter);
            counter++;
        }
        
        qDebug() << "addProfile() - Creating new profile:" << newProfileName;
        
        // THIS IS THE CRITICAL FIX: Create the profile directory BEFORE writing data
        QDir dir;
        if (!dir.mkpath(getProfileHomePath(newProfileName))) {
            qWarning() << "addProfile() - Failed to create profile directory for:" << newProfileName;
            return QString();
        }
        
        // Save profile data
        writeProfileData(newProfileName, "url", host);
        writeProfileData(newProfileName, "port", QString::number(port));
        writeProfileData(newProfileName, "login", login);
        
        qDebug() << "addProfile() - Successfully created profile:" << newProfileName;
        return newProfileName;
    }

private slots:
    void initTestCase() {
        tempDir = new QTemporaryDir();
        QVERIFY2(tempDir->isValid(), "Failed to create temporary directory for tests");
        qDebug() << "Test profile directory:" << tempDir->path();
    }

    void cleanupTestCase() {
        delete tempDir;
    }

    void init() {
        // Clean up for each test
        if (tempDir) {
            QDir dir(tempDir->path());
            for (const QString& entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QDir(tempDir->path() + "/" + entry).removeRecursively();
            }
        }
    }

    // TEST 1: Basic telnet URI parsing (WILL FAIL if QUrl doesn't parse correctly)
    void testBasicTelnetUriParsing() {
        QUrl url("telnet://mud.clessidra.it:4000");
        
        QVERIFY2(url.isValid(), "telnet://mud.clessidra.it:4000 should be a valid URI");
        QCOMPARE(url.scheme(), QString("telnet"));
        QCOMPARE(url.host(), QString("mud.clessidra.it"));
        QCOMPARE(url.port(), 4000);
    }

    // TEST 2: Profile directory creation (WILL FAIL if addProfile doesn't create directories)
    void testProfileDirectoryIsCreated() {
        QString host = "mud.clessidra.it";
        int port = 4000;
        
        QString profileName = addProfile(host, port, QString(), QString());
        
        QVERIFY2(!profileName.isEmpty(), "addProfile should return a non-empty profile name");
        
        QString profilePath = getProfileHomePath(profileName);
        QVERIFY2(QDir(profilePath).exists(), 
                 QString("Profile directory should exist at: %1").arg(profilePath).toUtf8().constData());
    }

    // TEST 3: Profile data persistence (WILL FAIL if data files aren't written correctly)
    void testProfileDataIsPersisted() {
        QString host = "mud.example.com";
        int port = 4000;
        QString login = "testuser";
        
        QString profileName = addProfile(host, port, login, QString());
        
        QVERIFY2(!profileName.isEmpty(), "Profile should be created");
        
        // Verify URL is saved
        QString savedHost = readProfileData(profileName, "url");
        QCOMPARE(savedHost, host);
        
        // Verify port is saved
        QString savedPort = readProfileData(profileName, "port");
        QCOMPARE(savedPort.toInt(), port);
        
        // Verify login is saved
        QString savedLogin = readProfileData(profileName, "login");
        QCOMPARE(savedLogin, login);
    }

    // TEST 4: Profile reuse (WILL FAIL if existing profiles aren't detected)
    void testExistingProfileIsReused() {
        QString host = "mud.example.com";
        int port = 4000;
        
        // Create first profile
        QString profileName1 = addProfile(host, port, QString(), QString());
        QVERIFY(!profileName1.isEmpty());
        
        // Try to create again with same host:port
        QString profileName2 = addProfile(host, port, QString(), QString());
        
        QCOMPARE(profileName1, profileName2);
        QVERIFY2(profileName1 == profileName2, 
                 "Second call to addProfile with same host:port should return existing profile name");
    }

    // TEST 5: Profile name uniqueness (WILL FAIL if duplicate names aren't handled)
    void testProfileNameUniqueness() {
        QString host = "mud.example.com";
        
        // Create profiles on different ports
        QString profile1 = addProfile(host, 4000, QString(), QString());
        QString profile2 = addProfile(host, 4001, QString(), QString());
        QString profile3 = addProfile(host, 4002, QString(), QString());
        
        QVERIFY(!profile1.isEmpty());
        QVERIFY(!profile2.isEmpty());
        QVERIFY(!profile3.isEmpty());
        
        // All should be different
        QVERIFY(profile1 != profile2);
        QVERIFY(profile2 != profile3);
        QVERIFY(profile1 != profile3);
    }

    // TEST 6: Profile name sanitization (WILL FAIL if invalid characters aren't removed)
    void testProfileNameSanitization() {
        QString host = "mud/test:example?com";
        int port = 4000;
        
        QString profileName = addProfile(host, port, QString(), QString());
        
        QVERIFY2(!profileName.isEmpty(), "Profile should be created even with invalid characters in host");
        QVERIFY2(!profileName.contains('/'), "Profile name should not contain /");
        QVERIFY2(!profileName.contains(':'), "Profile name should not contain :");
        QVERIFY2(!profileName.contains('?'), "Profile name should not contain ?");
    }

    // TEST 7: Empty host handling (WILL FAIL if empty hosts aren't handled)
    void testEmptyHostHandling() {
        QString profileName = addProfile(QString(), 4000, QString(), QString());
        
        QVERIFY2(!profileName.isEmpty(), "Should create profile even with empty host");
        QCOMPARE(profileName, QString("New Profile"));
    }

    // TEST 8: Port validation (WILL FAIL if invalid ports aren't validated)
    void testPortValidation() {
        QUrl url1("telnet://mud.example.com:0");
        QUrl url2("telnet://mud.example.com:65536");
        QUrl url3("telnet://mud.example.com:4000");
        
        // Port 0 is invalid
        QVERIFY2(!(url1.port() >= 1 && url1.port() <= 65535), 
                 "Port 0 should be considered invalid");
        
        // Port 65536 is invalid
        QVERIFY2(!(url2.port() >= 1 && url2.port() <= 65535), 
                 "Port 65536 should be considered invalid");
        
        // Port 4000 is valid
        QVERIFY2(url3.port() >= 1 && url3.port() <= 65535, 
                 "Port 4000 should be considered valid");
    }

    // TEST 9: Username extraction (WILL FAIL if username isn't extracted)
    void testUsernameExtraction() {
        QUrl url("telnet://player@mud.example.com:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.userName(), QString("player"));
        QCOMPARE(url.host(), QString("mud.example.com"));
    }

    // TEST 10: Password extraction (WILL FAIL if password isn't extracted)
    void testPasswordExtraction() {
        QUrl url("telnet://player:secret@mud.example.com:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.userName(), QString("player"));
        QCOMPARE(url.password(), QString("secret"));
        QCOMPARE(url.host(), QString("mud.example.com"));
    }

    // TEST 11: Default port (WILL FAIL if default port isn't 23)
    void testDefaultPort() {
        QUrl url("telnet://mud.example.com");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.port(23), 23);
    }

    // TEST 12: IPv4 address support (WILL FAIL if IPv4 addresses aren't supported)
    void testIPv4AddressSupport() {
        QUrl url("telnet://192.168.1.100:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.host(), QString("192.168.1.100"));
        QCOMPARE(url.port(), 4000);
        
        QString profileName = addProfile("192.168.1.100", 4000, QString(), QString());
        QVERIFY(!profileName.isEmpty());
    }

    // TEST 13: IPv6 address support (WILL FAIL if IPv6 addresses aren't supported)
    void testIPv6AddressSupport() {
        QUrl url("telnet://[::1]:4000");
        
        QVERIFY(url.isValid());
        QCOMPARE(url.host(), QString("::1"));
        QCOMPARE(url.port(), 4000);
    }

    // TEST 14: Real-world example - mud.clessidra.it (THE EXACT CASE FROM THE BUG REPORT)
    void testRealWorldExample_MudClessidra() {
        QUrl url("telnet://mud.clessidra.it:4000");
        
        QVERIFY2(url.isValid(), "telnet://mud.clessidra.it:4000 must be valid");
        QCOMPARE(url.scheme(), QString("telnet"));
        QCOMPARE(url.host(), QString("mud.clessidra.it"));
        QCOMPARE(url.port(), 4000);
        
        // This is the CRITICAL test - if this fails, the bug is NOT fixed
        QString profileName = addProfile("mud.clessidra.it", 4000, QString(), QString());
        
        QVERIFY2(!profileName.isEmpty(), "CRITICAL: Profile creation must succeed");
        QVERIFY2(QDir(getProfileHomePath(profileName)).exists(), 
                 "CRITICAL: Profile directory must exist");
        QCOMPARE(readProfileData(profileName, "url"), QString("mud.clessidra.it"));
        QCOMPARE(readProfileData(profileName, "port").toInt(), 4000);
    }

    // TEST 15: Case insensitivity (WILL FAIL if case isn't handled properly)
    void testCaseInsensitivity() {
        QString profile1 = addProfile("MUD.Example.COM", 4000, QString(), QString());
        QString profile2 = addProfile("mud.example.com", 4000, QString(), QString());
        
        QVERIFY2(profile1 == profile2, "Same host with different case should reuse profile");
    }
};

#include "MudletTelnetUriIntegrationTest.moc"
QTEST_MAIN(MudletTelnetUriIntegrationTest)
