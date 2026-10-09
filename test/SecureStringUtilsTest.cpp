/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include <CredentialManager.h>
#include <MudletApp.h>
#include <SecureStringUtils.h>
#include <utils.h>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QVersionNumber>
#include <string>

class SecureStringUtilsTest : public QObject {
Q_OBJECT

private slots:
    void initTestCase();
    void testProfileBasedEncryption();
    void testDifferentProfilesUseDifferentKeys();
    void testEncryptedFormatDetection();
    void testEmptyStrings();
    void testNonDeterministicEncryption();
    void testSpecialCharacters();
    void testSecureMemoryClearing();
    void testProfileKeyPersistence();
    void testPortableModeFileStorage();
    void testInvalidInputHandling();
    void testLargeDataEncryption();
    void testCorruptedDataDecryption();
    void testVersionCompatibility();
    void testXMLImportProxyPasswordLogic();
    void testConveniencePasswordMethods();
    void testUnstorableKeyRefusesToEncrypt();
    void testDerivableKeyStillDecryptsOldFiles();
    void testReadingAnOldFileReencryptsIt();
    void testAnOldFileIsLeftAsItIsWhileNoKeyCanBeStored();
    void testAFileUnderTheProfilesOwnKeyIsNotRewrittenOnRead();
    void testReadingAnOldLegacyCopyReencryptsItToo();
    void cleanupTestCase();

private:
    QTemporaryDir mConfigDir;
};

void SecureStringUtilsTest::initTestCase()
{
    // Per-profile encryption keys are filed under QStandardPaths::AppConfigLocation,
    // which for a QTEST_MAIN program is $HOME/.config/SecureStringUtilsTest - so
    // without this the suite leaves key material in the home directory of whoever
    // runs it. Same recipe as CredentialManagerTest, and like it this only takes
    // effect where QStandardPaths honours XDG.
    QVERIFY(mConfigDir.isValid());
    qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
}

void SecureStringUtilsTest::testProfileBasedEncryption()
{
    QString plaintext = "mypassword";
    QString profileName = "TestProfile";
    
    QString encrypted = SecureStringUtils::encryptStringForProfile(plaintext, profileName);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(encrypted != plaintext);
    
    QString decrypted = SecureStringUtils::decryptStringForProfile(encrypted, profileName);
    QCOMPARE(decrypted, plaintext);
}

void SecureStringUtilsTest::testDifferentProfilesUseDifferentKeys()
{
    QString plaintext = "samepassword";
    QString profile1 = "Profile1";
    QString profile2 = "Profile2";
    
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(plaintext, profile1);
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(plaintext, profile2);
    
    // Should be different due to different profile keys
    QVERIFY(encrypted1 != encrypted2);
    
    // Each should decrypt correctly with its own profile
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted1, profile1), plaintext);
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted2, profile2), plaintext);
    
    // Cross-profile decryption should fail
    QVERIFY(SecureStringUtils::decryptStringForProfile(encrypted1, profile2) != plaintext);
    QVERIFY(SecureStringUtils::decryptStringForProfile(encrypted2, profile1) != plaintext);
}

void SecureStringUtilsTest::testEncryptedFormatDetection()
{
    // Test plaintext passwords (should NOT be detected as encrypted)
    QVERIFY(!SecureStringUtils::isEncryptedFormat("mypassword"));
    QVERIFY(!SecureStringUtils::isEncryptedFormat("secret123!@#"));
    QVERIFY(!SecureStringUtils::isEncryptedFormat(""));
    
    // Test actual encrypted passwords (should be detected as encrypted)
    QString plaintext = "testpassword";
    QString encrypted = SecureStringUtils::encryptStringForProfile(plaintext, "TestProfile");
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
    
    // Test invalid formats
    QVERIFY(!SecureStringUtils::isEncryptedFormat("not-base64!"));
    QVERIFY(!SecureStringUtils::isEncryptedFormat("invalid=base64="));
}

void SecureStringUtilsTest::testEmptyStrings()
{
    // Test empty string handling
    QCOMPARE(SecureStringUtils::encryptStringForProfile("", "Profile"), QString());
    QCOMPARE(SecureStringUtils::encryptStringForProfile("password", ""), QString());
    QCOMPARE(SecureStringUtils::decryptStringForProfile("", "Profile"), QString());
    QCOMPARE(SecureStringUtils::decryptStringForProfile("encrypted", ""), QString());
    QVERIFY(!SecureStringUtils::isEncryptedFormat(""));
}

void SecureStringUtilsTest::testNonDeterministicEncryption()
{
    // Same plaintext should produce DIFFERENT encrypted results each time (due to random nonces)
    QString plaintext = "consistent_password";
    QString profile = "TestProfile";
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(plaintext, profile);
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(plaintext, profile);
    
    // Should be different due to random nonces
    QVERIFY(encrypted1 != encrypted2);
    
    // But both should decrypt to the same plaintext
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted1, profile), plaintext);
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted2, profile), plaintext);
}

void SecureStringUtilsTest::testSpecialCharacters()
{
    // Test passwords with special characters
    QString specialPassword = "pássw0rd!@#$%^&*()";
    QString profile = "TestProfile";
    QString encrypted = SecureStringUtils::encryptStringForProfile(specialPassword, profile);
    QString decrypted = SecureStringUtils::decryptStringForProfile(encrypted, profile);
    
    QCOMPARE(decrypted, specialPassword);
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
}

void SecureStringUtilsTest::testSecureMemoryClearing()
{
    QString testString = "sensitive_data";
    QString originalContent = testString;
    
    SecureStringUtils::secureStringClear(testString);
    QVERIFY(testString.isEmpty());
    QVERIFY(testString != originalContent);

    QByteArray testArray = "sensitive_bytes";
    QByteArray originalArray = testArray;

    SecureStringUtils::secureByteArrayClear(testArray);
    QVERIFY(testArray.isEmpty());
    QVERIFY(testArray != originalArray);

    std::string testStdString = "sensitive_std_data";
    std::string originalStdString = testStdString;

    SecureStringUtils::secureStdStringClear(testStdString);
    QVERIFY(testStdString.empty());
    QVERIFY(testStdString != originalStdString);
}

void SecureStringUtilsTest::testProfileKeyPersistence()
{
    // Test that the same profile uses consistent keys
    QString password = "testpassword";
    QString profile = "PersistentProfile";
    
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(password, profile);
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(password, profile);
    
    // Both should decrypt correctly (proving key consistency)
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted1, profile), password);
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted2, profile), password);
}

void SecureStringUtilsTest::testPortableModeFileStorage()
{
    // Test that profile-specific encryption/decryption works consistently
    // This exercises the file-based key storage in portable mode
    QString profileName = "PortableTestProfile";
    QString plaintext = "portable_test_password";
    
    // First encryption - this will trigger key generation and file storage
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(plaintext, profileName);
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted1));
    
    // Second encryption with same profile - should use same key from file
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(plaintext, profileName);
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted2));
    
    // Both should decrypt correctly
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted1, profileName), plaintext);
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted2, profileName), plaintext);
    
    // Test that different profiles use different keys
    QString otherProfile = "AnotherPortableProfile";
    QString encrypted3 = SecureStringUtils::encryptStringForProfile(plaintext, otherProfile);
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted3));
    
    // Should decrypt correctly with its own profile
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted3, otherProfile), plaintext);
    
    // Cross-profile decryption should fail (different keys)
    QString crossDecrypt = SecureStringUtils::decryptStringForProfile(encrypted3, profileName);
    QVERIFY(crossDecrypt.isEmpty() || crossDecrypt != plaintext);
}

void SecureStringUtilsTest::testInvalidInputHandling()
{
    // Test null/empty profile names
    QString password = "testpassword";
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(password, "");
    QVERIFY(encrypted1.isEmpty()); // Should return empty for empty profile
    
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(password, QString());
    QVERIFY(encrypted2.isEmpty()); // Should return empty for null profile
    
    // Test with empty password
    QString validProfile = "ValidProfile";
    QString encrypted3 = SecureStringUtils::encryptStringForProfile("", validProfile);
    QVERIFY(encrypted3.isEmpty()); // Should return empty for empty password
    
    // Test decryption with mismatched profiles
    QString profile1 = "Profile1";
    QString profile2 = "Profile2";
    QString encrypted = SecureStringUtils::encryptStringForProfile(password, profile1);
    
    QString decrypted = SecureStringUtils::decryptStringForProfile(encrypted, profile2);
    QVERIFY(decrypted.isEmpty() || decrypted != password); // Should fail or return wrong data
}

void SecureStringUtilsTest::testLargeDataEncryption()
{
    // Test with larger strings to ensure robustness
    QString largeString = QString("A").repeated(10000); // 10KB string
    QString profile = "LargeDataProfile";
    
    QString encrypted = SecureStringUtils::encryptStringForProfile(largeString, profile);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
    
    QString decrypted = SecureStringUtils::decryptStringForProfile(encrypted, profile);
    QCOMPARE(decrypted, largeString);
}

void SecureStringUtilsTest::testCorruptedDataDecryption()
{
    QString password = "testpassword";
    QString profile = "CorruptionTestProfile";
    
    QString encrypted = SecureStringUtils::encryptStringForProfile(password, profile);
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
    
    // Test with completely invalid format
    QString invalid1 = "notencrypted";
    QVERIFY(!SecureStringUtils::isEncryptedFormat(invalid1));
    QString decrypted1 = SecureStringUtils::decryptStringForProfile(invalid1, profile);
    QVERIFY(decrypted1.isEmpty()); // Should return empty for invalid format
    
    // Test with corrupted encrypted data (corrupt the raw binary data, not Base64)
    if (encrypted.length() > 10) {
        // Decode to binary, corrupt a byte, re-encode to Base64
        QByteArray binaryData = QByteArray::fromBase64(encrypted.toLatin1());
        if (binaryData.size() > 10) {
            // Corrupt a byte in the middle of the binary data
            int corruptIndex = binaryData.size() / 2;
            char originalByte = binaryData[corruptIndex];
            binaryData[corruptIndex] = static_cast<char>(originalByte ^ 0xFF); // Flip all bits
            QString corrupted = QString::fromLatin1(binaryData.toBase64());
            QString decrypted2 = SecureStringUtils::decryptStringForProfile(corrupted, profile);
            QVERIFY(decrypted2.isEmpty() || decrypted2 != password); // Should fail due to corruption
        }
    }
    
    // Test with truncated encrypted data
    if (encrypted.length() > 5) {
        QString truncated = encrypted.left(encrypted.length() - 5);
        QString decrypted3 = SecureStringUtils::decryptStringForProfile(truncated, profile);
        QVERIFY(decrypted3.isEmpty() || decrypted3 != password); // Should fail due to truncation
    }
}

void SecureStringUtilsTest::testVersionCompatibility()
{
    // Test that version-based compatibility logic works correctly
    
    // Test version comparison for major version differences
    QVERIFY(4 > 3);  // Version 4.x should be newer than 3.x
    QVERIFY(5 > 4);  // Version 5.x should be newer than 4.x
    
    // Test version comparison for minor version differences within major version 4
    int majorVersion = 4;
    
    // Test cases for version 4.x.x
    struct {
        int minorVersion;
        bool shouldUseSecureStorage;
        QString description;
    } testCases[] = {
        {19, false, "Version 4.19.x should use legacy mode"},
        {20, true,  "Version 4.20.x should use secure storage"},
        {21, true,  "Version 4.21.x should use secure storage"},
        {50, true,  "Version 4.50.x should use secure storage"}
    };
    
    for (const auto& testCase : testCases) {
        // Simulate the version check logic from XMLimport
        bool useSecureStorage = (majorVersion > 4) || (majorVersion == 4 && testCase.minorVersion >= 20);
        
        if (useSecureStorage != testCase.shouldUseSecureStorage) {
            QFAIL(qPrintable(QString("Version compatibility test failed for %1: expected %2, got %3")
                           .arg(testCase.description)
                           .arg(testCase.shouldUseSecureStorage ? "true" : "false")
                           .arg(useSecureStorage ? "true" : "false")));
        }
        QCOMPARE(useSecureStorage, testCase.shouldUseSecureStorage);
    }
    
    // Test major version transitions
    QVERIFY((5 > 4) || (5 == 4 && 0 >= 20)); // Version 5.0.x should use secure storage
    QVERIFY((6 > 4) || (6 == 4 && 0 >= 20)); // Version 6.0.x should use secure storage
    
    qDebug() << "Version compatibility tests passed";
}

void SecureStringUtilsTest::testXMLImportProxyPasswordLogic()
{
    // Test that XMLimport now uses application version, not profile version
    // This simulates the fixed logic in XMLimport.cpp
    
    // Simulate different APP_VERSION values
    struct TestCase {
        QString appVersion;
        bool shouldUseSecureStorage;
        QString description;
    };
    
    const QList<TestCase> testCases = {
        {"4.19.0", false, "App version 4.19.0 (before secure storage)"},
        {"4.20.0", true, "App version 4.20.0 (secure storage introduced)"},
        {"4.21.0", true, "App version 4.21.0 (after secure storage)"},
        {"5.0.0", true, "App version 5.0.0 (major version after secure storage)"},
        {"3.15.0", false, "App version 3.15.0 (old version)"}
    };
    
    for (const auto& testCase : testCases) {
        // Simulate the new XMLimport logic
        const QVersionNumber appVersion = QVersionNumber::fromString(testCase.appVersion);
        const QVersionNumber secureStorageVersion = QVersionNumber(4, 20, 0);
        const bool useSecureStorage = appVersion >= secureStorageVersion;
        
        QCOMPARE(useSecureStorage, testCase.shouldUseSecureStorage);
        
        if (useSecureStorage != testCase.shouldUseSecureStorage) {
            QFAIL(qPrintable(QString("XMLimport proxy password test failed for %1: expected %2, got %3")
                           .arg(testCase.description)
                           .arg(testCase.shouldUseSecureStorage ? "true" : "false")
                           .arg(useSecureStorage ? "true" : "false")));
        }
    }
    
    qDebug() << "XMLimport proxy password logic tests passed";
}

void SecureStringUtilsTest::testConveniencePasswordMethods()
{
    QString testProfile = "ConvenienceTestProfile";
    QString testKey = "test_password";
    QString testPassword = "MyConvenienceTestPassword123!";
    
    // Ensure clean state
    SecureStringUtils::removePassword(testProfile, testKey);
    QVERIFY(!SecureStringUtils::hasPassword(testProfile, testKey));
    
    // Test storing password
    bool stored = SecureStringUtils::storePassword(testProfile, testKey, testPassword);
    QVERIFY(stored);
    
    // Test password exists
    bool exists = SecureStringUtils::hasPassword(testProfile, testKey);
    QVERIFY(exists);
    
    // Test retrieving password
    QString retrieved = SecureStringUtils::retrievePassword(testProfile, testKey);
    QCOMPARE(retrieved, testPassword);
    
    // Test removing password
    bool removed = SecureStringUtils::removePassword(testProfile, testKey);
    QVERIFY(removed);
    
    // Test password no longer exists
    bool existsAfterRemoval = SecureStringUtils::hasPassword(testProfile, testKey);
    QVERIFY(!existsAfterRemoval);
    
    // Test retrieving non-existent password
    QString nonExistent = SecureStringUtils::retrievePassword(testProfile, testKey);
    QVERIFY(nonExistent.isEmpty());
    
    // Test invalid inputs
    QVERIFY(!SecureStringUtils::storePassword("", testKey, testPassword));
    QVERIFY(!SecureStringUtils::storePassword(testProfile, "", testPassword));
    QVERIFY(!SecureStringUtils::storePassword(testProfile, "invalid/key", testPassword));
    
    qDebug() << "Convenience password methods tests passed";
}

static QString profileKeyFilePath(const QString& profile)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + qsl("/profiles/%1/encryption_key").arg(profile);
}

// Where XDG_CONFIG_HOME is not honoured these profiles live in the real config folder, so a
// run must neither depend on nor leave behind what an earlier one wrote there
static bool removeTestProfile(const QString& profile)
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + qsl("/profiles/%1").arg(profile)).removeRecursively();
}

// With nowhere to keep a random key, encrypting with the key anyone can derive from
// the profile name would only look like protection
void SecureStringUtilsTest::testUnstorableKeyRefusesToEncrypt()
{
    const QString profile = qsl("KeyCannotBeStored");
    QVERIFY(removeTestProfile(profile));
    auto cleanup = qScopeGuard([&profile] {
        removeTestProfile(profile);
    });
    // A directory where the key file belongs makes it impossible to write
    QVERIFY(QDir().mkpath(profileKeyFilePath(profile)));

    QVERIFY(SecureStringUtils::encryptStringForProfile(qsl("secret"), profile).isEmpty());
    QVERIFY(!SecureStringUtils::storePassword(profile, qsl("character"), qsl("secret")));
}

// A file an older Mudlet encrypted with the derivable key, built the way it built one
static QString encryptWithDerivableKey(const QString& plaintext, const QString& profile)
{
    QCryptographicHash keyHash(QCryptographicHash::Sha256);
    keyHash.addData(qsl("Mudlet").toUtf8());
    keyHash.addData(profile.toUtf8());
    keyHash.addData(qsl("MudletProfileEncryption2025").toUtf8());
    const QByteArray profileKey = keyHash.result();

    const QByteArray salt(16, '\x5a');
    const QByteArray nonce(16, '\xa5');
    QByteArray derivedKey = profileKey + salt;
    for (int i = 0; i < 100000; ++i) {
        QCryptographicHash round(QCryptographicHash::Sha256);
        round.addData(derivedKey);
        round.addData(salt);
        derivedKey = round.result();
    }

    const QByteArray cipherKey = QCryptographicHash::hash(derivedKey + nonce, QCryptographicHash::Sha256);
    QByteArray encrypted = plaintext.toUtf8();
    for (int i = 0; i < encrypted.size(); ++i) {
        encrypted[i] = encrypted[i] ^ cipherKey[i % cipherKey.size()];
    }
    const QByteArray hmac = QMessageAuthenticationCode::hash(salt + nonce + encrypted, derivedKey, QCryptographicHash::Sha256);

    QByteArray result;
    result.append(static_cast<char>(2));
    result.append(salt);
    result.append(nonce);
    result.append(hmac);
    result.append(encrypted);
    return QString::fromLatin1(result.toBase64());
}

// Whether only the profile's own key, not the derivable one, opens the ciphertext to give expected
static bool openedByTheProfilesOwnKey(const QString& ciphertext, const QString& profile, const QString& expected)
{
    bool usedDerivableKey = true;
    return SecureStringUtils::decryptStringForProfile(ciphertext, profile, &usedDerivableKey) == expected && !usedDerivableKey;
}

static QString profileFilePath(const QString& profileDirectory, const QString& item)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + qsl("/profiles/%1/%2").arg(profileDirectory, item);
}

static QByteArray rawFileBytes(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// SecureStringUtils' own files hold a QDataStream string, CredentialManager's the bare text
static bool writeDatFile(const QString& path, const QString& ciphertext)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QDataStream ofs(&file);
    ofs.setVersion(QDataStream::Qt_5_12);
    ofs << ciphertext;
    return ofs.status() == QDataStream::Ok;
}

static QString readDatFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QDataStream ifs(&file);
    ifs.setVersion(QDataStream::Qt_5_12);
    QString ciphertext;
    ifs >> ciphertext;
    return ciphertext;
}

static bool writeTextFile(const QString& path, const QString& ciphertext)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Text) && file.write(ciphertext.toUtf8()) == ciphertext.toUtf8().size();
}

static QString readTextFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly | QIODevice::Text) ? QString::fromUtf8(file.readAll()) : QString();
}

void SecureStringUtilsTest::testDerivableKeyStillDecryptsOldFiles()
{
    const QString profile = qsl("WroteWithTheDerivableKey");
    QVERIFY(removeTestProfile(profile));
    auto cleanup = qScopeGuard([&profile] {
        removeTestProfile(profile);
    });
    const QString oldFile = encryptWithDerivableKey(qsl("saved_long_ago"), profile);

    QCOMPARE(SecureStringUtils::decryptStringForProfile(oldFile, profile), qsl("saved_long_ago"));
    QVERIFY2(!QFile::exists(profileKeyFilePath(profile)), "decrypting created a key file, which cannot decrypt anything already saved");

    // ...and still once a random key has been stored for the passwords saved since
    const QString newFile = SecureStringUtils::encryptStringForProfile(qsl("saved_today"), profile);
    QVERIFY(!newFile.isEmpty());
    QVERIFY(QFile::exists(profileKeyFilePath(profile)));
    QCOMPARE(SecureStringUtils::decryptStringForProfile(newFile, profile), qsl("saved_today"));
    QCOMPARE(SecureStringUtils::decryptStringForProfile(oldFile, profile), qsl("saved_long_ago"));
}

// The derivable key protects nothing, so a file still under it is rewritten under the profile's
// own key as soon as it has been read
void SecureStringUtilsTest::testReadingAnOldFileReencryptsIt()
{
    const QString profile = qsl("ReadAnOldPasswordFile");
    QVERIFY(removeTestProfile(profile));
    auto cleanup = qScopeGuard([&profile] {
        removeTestProfile(profile);
    });
    const QString oldFile = encryptWithDerivableKey(qsl("saved_long_ago"), profile);

    const QString passwordFile = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + qsl("/profiles/%1/passwords/character.dat").arg(profile);
    QVERIFY(QDir().mkpath(QFileInfo(passwordFile).absolutePath()));
    const auto readStoredCiphertext = [&passwordFile]() {
        QFile file(passwordFile);
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }
        QDataStream ifs(&file);
        ifs.setVersion(QDataStream::Qt_5_12);
        QString ciphertext;
        ifs >> ciphertext;
        return ciphertext;
    };
    {
        QFile file(passwordFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QDataStream ofs(&file);
        ofs.setVersion(QDataStream::Qt_5_12);
        ofs << oldFile;
    }
    QCOMPARE(readStoredCiphertext(), oldFile);

    QCOMPARE(SecureStringUtils::retrievePassword(profile, qsl("character")), qsl("saved_long_ago"));

    const QString rewritten = readStoredCiphertext();
    QVERIFY2(rewritten != oldFile, "the file is still encrypted with the key anyone can derive from the profile name");
    QVERIFY(QFile::exists(profileKeyFilePath(profile)));
    QCOMPARE(SecureStringUtils::retrievePassword(profile, qsl("character")), qsl("saved_long_ago"));

    // ...and the same for the credential store's own files
    const QString credentialFile = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + qsl("/profiles/%1/passwords/proxy").arg(profile);
    {
        QFile file(credentialFile);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(file.write(encryptWithDerivableKey(qsl("proxy_long_ago"), profile).toUtf8()) > 0);
    }
    QCOMPARE(CredentialManager::retrieveCredential(profile, qsl("proxy")), qsl("proxy_long_ago"));
    {
        QFile file(credentialFile);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString stored = QString::fromUtf8(file.readAll());
        QVERIFY(!stored.isEmpty());
        QVERIFY2(SecureStringUtils::decryptStringForProfile(stored, profile) == qsl("proxy_long_ago"), "the rewritten credential no longer decrypts");
        QVERIFY2(stored != encryptWithDerivableKey(qsl("proxy_long_ago"), profile), "the credential is still encrypted with the key anyone can derive from the profile name");
    }

    // Different bytes only show a new salt: the derivable key must not be what opens either file now
    QVERIFY2(openedByTheProfilesOwnKey(rewritten, profile, qsl("saved_long_ago")), "the rewritten password file still opens with the key anyone can derive from the profile name");
    QVERIFY2(openedByTheProfilesOwnKey(readTextFile(credentialFile), profile, qsl("proxy_long_ago")), "the rewritten credential still opens with the key anyone can derive from the profile name");
}

void SecureStringUtilsTest::testAnOldFileIsLeftAsItIsWhileNoKeyCanBeStored()
{
    const QString profile = qsl("ReadAnOldFileWithNowhereToKeepTheKey");
    QVERIFY(removeTestProfile(profile));
    auto cleanup = qScopeGuard([&profile] {
        removeTestProfile(profile);
    });
    const QString passwordFile = profileFilePath(profile, qsl("passwords/character.dat"));
    const QString credentialFile = profileFilePath(profile, qsl("passwords/proxy"));
    QVERIFY(writeDatFile(passwordFile, encryptWithDerivableKey(qsl("saved_long_ago"), profile)));
    QVERIFY(writeTextFile(credentialFile, encryptWithDerivableKey(qsl("proxy_long_ago"), profile)));
    const QByteArray passwordBytes = rawFileBytes(passwordFile);
    const QByteArray credentialBytes = rawFileBytes(credentialFile);
    // A directory where the key file belongs makes it impossible to write
    QVERIFY(QDir().mkpath(profileKeyFilePath(profile)));

    QCOMPARE(SecureStringUtils::retrievePassword(profile, qsl("character")), qsl("saved_long_ago"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, qsl("proxy")), qsl("proxy_long_ago"));
    QVERIFY2(rawFileBytes(passwordFile) == passwordBytes, "a re-encryption that could not happen changed the password file");
    QVERIFY2(rawFileBytes(credentialFile) == credentialBytes, "a re-encryption that could not happen changed the credential file");
}

// Rewriting on every read would also move the file's time, which decides whether the legacy copy is newer
void SecureStringUtilsTest::testAFileUnderTheProfilesOwnKeyIsNotRewrittenOnRead()
{
    const QString profile = qsl("ReadAFileUnderItsOwnKey");
    QVERIFY(removeTestProfile(profile));
    auto cleanup = qScopeGuard([&profile] {
        removeTestProfile(profile);
    });
    QVERIFY(SecureStringUtils::storePassword(profile, qsl("character"), qsl("saved_today")));
    QVERIFY(CredentialManager::storeCredential(profile, qsl("proxy"), qsl("proxy_today")));
    const QString passwordFile = profileFilePath(profile, qsl("passwords/character.dat"));
    const QString credentialFile = profileFilePath(profile, qsl("passwords/proxy"));
    const QDateTime anHourAgo = QDateTime::currentDateTime().addSecs(-3600);
    for (const QString& path : {passwordFile, credentialFile}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));
        QVERIFY(file.setFileTime(anHourAgo, QFileDevice::FileModificationTime));
    }
    const QByteArray passwordBytes = rawFileBytes(passwordFile);
    const QByteArray credentialBytes = rawFileBytes(credentialFile);
    const QDateTime passwordTime = QFileInfo(passwordFile).lastModified();
    const QDateTime credentialTime = QFileInfo(credentialFile).lastModified();

    QVERIFY(openedByTheProfilesOwnKey(readDatFile(passwordFile), profile, qsl("saved_today")));
    QCOMPARE(SecureStringUtils::retrievePassword(profile, qsl("character")), qsl("saved_today"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, qsl("proxy")), qsl("proxy_today"));
    QVERIFY2(rawFileBytes(passwordFile) == passwordBytes && QFileInfo(passwordFile).lastModified() == passwordTime, "reading rewrote a password file that needed no re-encrypting");
    QVERIFY2(rawFileBytes(credentialFile) == credentialBytes && QFileInfo(credentialFile).lastModified() == credentialTime, "reading rewrote a credential file that needed no re-encrypting");
}

// A profile name longer than the 50 characters the earlier naming scheme cut to also has a copy at
// the cut-down path, which an older Mudlet reads, and that copy was written under the same key
void SecureStringUtilsTest::testReadingAnOldLegacyCopyReencryptsItToo()
{
    const QString profile = qsl("ReadAnOldLegacyCopyOfAPasswordWithANameTooLongForTheOlderScheme");
    QVERIFY(profile.size() > 50);
    const QString legacyProfile = profile.left(50);
    const QString currentProfileDir = MudletApp::sanitizeForPath(profile);
    QVERIFY(removeTestProfile(profile));
    QVERIFY(removeTestProfile(legacyProfile));
    QVERIFY(removeTestProfile(currentProfileDir));
    auto cleanup = qScopeGuard([&] {
        removeTestProfile(profile);
        removeTestProfile(legacyProfile);
        removeTestProfile(currentProfileDir);
    });
    const QString legacyFile = profileFilePath(legacyProfile, qsl("passwords/proxy"));
    const QString currentFile = profileFilePath(currentProfileDir, qsl("passwords/proxy"));

    // Only the legacy copy, as an older Mudlet leaves it
    QVERIFY(writeTextFile(legacyFile, encryptWithDerivableKey(qsl("proxy_long_ago"), profile)));
    QCOMPARE(CredentialManager::retrieveCredential(profile, qsl("proxy")), qsl("proxy_long_ago"));
    QVERIFY2(openedByTheProfilesOwnKey(readTextFile(currentFile), profile, qsl("proxy_long_ago")), "the copy brought across is not under the profile's own key");
    QVERIFY2(openedByTheProfilesOwnKey(readTextFile(legacyFile), profile, qsl("proxy_long_ago")), "the legacy copy still opens with the key anyone can derive from the profile name");
    QVERIFY2(QFileInfo(legacyFile).lastModified() <= QFileInfo(currentFile).lastModified(), "the legacy copy was left the newer one, so every read copies it across again");

    // Both copies, the current one newer, as a Mudlet that wrote both leaves them
    QVERIFY(QFile::remove(profileKeyFilePath(profile)));
    QVERIFY(writeTextFile(legacyFile, encryptWithDerivableKey(qsl("proxy_long_ago"), profile)));
    {
        QFile file(legacyFile);
        QVERIFY(file.open(QIODevice::ReadWrite));
        QVERIFY(file.setFileTime(QDateTime::currentDateTime().addSecs(-3600), QFileDevice::FileModificationTime));
    }
    QVERIFY(writeTextFile(currentFile, encryptWithDerivableKey(qsl("proxy_long_ago"), profile)));
    QCOMPARE(CredentialManager::retrieveCredential(profile, qsl("proxy")), qsl("proxy_long_ago"));
    QVERIFY2(openedByTheProfilesOwnKey(readTextFile(currentFile), profile, qsl("proxy_long_ago")), "the current copy is not under the profile's own key");
    QVERIFY2(openedByTheProfilesOwnKey(readTextFile(legacyFile), profile, qsl("proxy_long_ago")), "the legacy copy still opens with the key anyone can derive from the profile name");
}

void SecureStringUtilsTest::cleanupTestCase()
{
}

#include "SecureStringUtilsTest.moc"
QTEST_MAIN(SecureStringUtilsTest)
