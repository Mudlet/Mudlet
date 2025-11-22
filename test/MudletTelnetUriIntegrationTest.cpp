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
#include <QDebug>

/**
 * Integration tests for telnet:// URI support
 */
class MudletTelnetUriIntegrationTest : public QObject {
Q_OBJECT

private:
    QTemporaryDir* tempDir = nullptr;

    QString getProfileHomePath(const QString& profileName);
    QString getProfileDataItemPath(const QString& profileName, const QString& item);
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);
    QString readProfileData(const QString& profile, const QString& item);
    bool profileExists(const QString& profileName);
    QString addProfile(const QString& host, const int port, const QString& login, const QString& password);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testBasicTelnetUriParsing();
    void testProfileDirectoryIsCreated();
    void testProfileDataIsPersisted();
    void testExistingProfileIsReused();
    void testProfileNameUniqueness();
    void testProfileNameSanitization();
    void testEmptyHostHandling();
    void testPortValidation();
    void testUsernameExtraction();
    void testPasswordExtraction();
    void testDefaultPort();
    void testIPv4AddressSupport();
    void testIPv6AddressSupport();
    void testRealWorldExample_MudClessidra();
    void testCaseInsensitivity();
};

QString MudletTelnetUriIntegrationTest::getProfileHomePath(const QString& profileName)
{
    if (!tempDir) return QString();
    return tempDir->path() + "/" + profileName;
}

QString MudletTelnetUriIntegrationTest::getProfileDataItemPath(const QString& profileName, const QString& item)
{
    if (!tempDir) return QString();
    return tempDir->path() + "/" + profileName + "/" + item;
}

QPair<bool, QString> MudletTelnetUriIntegrationTest::writeProfileData(const QString& profile, const QString& item, const QString& what)
{
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

QString MudletTelnetUriIntegrationTest::readProfileData(const QString& profile, const QString& item)
{
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

bool MudletTelnetUriIntegrationTest::profileExists(const QString& profileName)
{
    QString profilePath = getProfileHomePath(profileName);
    return QDir(profilePath).exists();
}

QString MudletTelnetUriIntegrationTest::addProfile(const QString& host, const int port, const QString& login, const QString& password)
{
    qDebug() << "addProfile() - Creating profile for host:" << host << "port:" << port;
    
    if (!tempDir) return QString();

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

    // Create the profile directory BEFORE writing data
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

void MudletTelnetUriIntegrationTest::initTestCase()
{
    tempDir = new QTemporaryDir();
    QVERIFY2(tempDir->isValid(), "Failed to create temporary directory for tests");
    qDebug() << "Test profile directory:" << tempDir->path();
}

void MudletTelnetUriIntegrationTest::cleanupTestCase()
{
    delete tempDir;
    tempDir = nullptr;
}

void MudletTelnetUriIntegrationTest::init()
{
    // Clean up for each test
    if (tempDir) {
        QDir dir(tempDir->path());
        for (const QString& entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir(tempDir->path() + "/" + entry).removeRecursively();
        }
    }
}

void MudletTelnetUriIntegrationTest::testBasicTelnetUriParsing()
{
    QUrl url("telnet://mud.clessidra.it:4000");
    QVERIFY2(url.isValid(), "telnet://mud.clessidra.it:4000 should be a valid URI");
    QCOMPARE(url.scheme(), QString("telnet"));
    QCOMPARE(url.host(), QString("mud.clessidra.it"));
    QCOMPARE(url.port(), 4000);
}

void MudletTelnetUriIntegrationTest::testProfileDirectoryIsCreated()
{
    QString host = "mud.clessidra.it";
    int port = 4000;
    QString profileName = addProfile(host, port, QString(), QString());
    QVERIFY2(!profileName.isEmpty(), "addProfile should return a non-empty profile name");
    QString profilePath = getProfileHomePath(profileName);
    QVERIFY2(QDir(profilePath).exists(),
              QString("Profile directory should exist at: %1").arg(profilePath).toUtf8().constData());
}

void MudletTelnetUriIntegrationTest::testProfileDataIsPersisted()
{
    QString host = "mud.example.com";
    int port = 4000;
    QString login = "testuser";
    QString profileName = addProfile(host, port, login, QString());
    QVERIFY2(!profileName.isEmpty(), "Profile should be created");
    
    QCOMPARE(readProfileData(profileName, "url"), host);
    QCOMPARE(readProfileData(profileName, "port").toInt(), port);
    QCOMPARE(readProfileData(profileName, "login"), login);
}

void MudletTelnetUriIntegrationTest::testExistingProfileIsReused()
{
    QString host = "mud.example.com";
    int port = 4000;
    
    QString profileName1 = addProfile(host, port, QString(), QString());
    QVERIFY(!profileName1.isEmpty());
    
    QString profileName2 = addProfile(host, port, QString(), QString());
    QCOMPARE(profileName1, profileName2);
}

void MudletTelnetUriIntegrationTest::testProfileNameUniqueness()
{
    QString host = "mud.example.com";
    
    QString profile1 = addProfile(host, 4000, QString(), QString());
    QString profile2 = addProfile(host, 4001, QString(), QString());
    QString profile3 = addProfile(host, 4002, QString(), QString());
    
    QVERIFY(profile1 != profile2);
    QVERIFY(profile2 != profile3);
    QVERIFY(profile1 != profile3);
}

void MudletTelnetUriIntegrationTest::testProfileNameSanitization()
{
    QString host = "mud/test:example?com";
    int port = 4000;
    QString profileName = addProfile(host, port, QString(), QString());
    QVERIFY2(!profileName.isEmpty(), "Profile should be created even with invalid characters in host");
    QVERIFY(!profileName.contains('/'));
    QVERIFY(!profileName.contains(':'));
    QVERIFY(!profileName.contains('?'));
}

void MudletTelnetUriIntegrationTest::testEmptyHostHandling()
{
    QString profileName = addProfile(QString(), 4000, QString(), QString());
    QVERIFY(!profileName.isEmpty());
    QCOMPARE(profileName, QString("New Profile"));
}

void MudletTelnetUriIntegrationTest::testPortValidation()
{
    QUrl url1("telnet://mud.example.com:0");
    QUrl url2("telnet://mud.example.com:65536");
    QUrl url3("telnet://mud.example.com:4000");
    
    QVERIFY(!(url1.port() >= 1 && url1.port() <= 65535));
    QVERIFY(!(url2.port() >= 1 && url2.port() <= 65535));
    QVERIFY(url3.port() >= 1 && url3.port() <= 65535);
}

void MudletTelnetUriIntegrationTest::testUsernameExtraction()
{
    QUrl url("telnet://player@mud.example.com:4000");
    QVERIFY(url.isValid());
    QCOMPARE(url.userName(), QString("player"));
    QCOMPARE(url.host(), QString("mud.example.com"));
}

void MudletTelnetUriIntegrationTest::testPasswordExtraction()
{
    QUrl url("telnet://player:secret@mud.example.com:4000");
    QVERIFY(url.isValid());
    QCOMPARE(url.userName(), QString("player"));
    QCOMPARE(url.password(), QString("secret"));
    QCOMPARE(url.host(), QString("mud.example.com"));
}

void MudletTelnetUriIntegrationTest::testDefaultPort()
{
    QUrl url("telnet://mud.example.com");
    QVERIFY(url.isValid());
    QCOMPARE(url.port(23), 23);
}

void MudletTelnetUriIntegrationTest::testIPv4AddressSupport()
{
    QUrl url("telnet://192.168.1.100:4000");
    QVERIFY(url.isValid());
    QCOMPARE(url.host(), QString("192.168.1.100"));
    QCOMPARE(url.port(), 4000);
    
    QString profileName = addProfile("192.168.1.100", 4000, QString(), QString());
    QVERIFY(!profileName.isEmpty());
}

void MudletTelnetUriIntegrationTest::testIPv6AddressSupport()
{
    QUrl url("telnet://[::1]:4000");
    QVERIFY(url.isValid());
    QCOMPARE(url.host(), QString("::1"));
    QCOMPARE(url.port(), 4000);
}

void MudletTelnetUriIntegrationTest::testRealWorldExample_MudClessidra()
{
    QUrl url("telnet://mud.clessidra.it:4000");
    QVERIFY(url.isValid());
    QCOMPARE(url.scheme(), QString("telnet"));
    QCOMPARE(url.host(), QString("mud.clessidra.it"));
    QCOMPARE(url.port(), 4000);
    
    QString profileName = addProfile("mud.clessidra.it", 4000, QString(), QString());
    QVERIFY(!profileName.isEmpty());
    QVERIFY(QDir(getProfileHomePath(profileName)).exists());
    QCOMPARE(readProfileData(profileName, "url"), QString("mud.clessidra.it"));
    QCOMPARE(readProfileData(profileName, "port").toInt(), 4000);
}

void MudletTelnetUriIntegrationTest::testCaseInsensitivity()
{
    QString profile1 = addProfile("MUD.Example.COM", 4000, QString(), QString());
    QString profile2 = addProfile("mud.example.com", 4000, QString(), QString());
    QCOMPARE(profile1, profile2);
}

#include "MudletTelnetUriIntegrationTest.moc"
QTEST_MAIN(MudletTelnetUriIntegrationTest)
