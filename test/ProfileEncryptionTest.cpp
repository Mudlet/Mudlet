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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.             *
 ***************************************************************************/

#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "SecureStringUtils.h"

class ProfileEncryptionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testProfileBasedEncryption();
    void testDifferentProfilesUseDifferentKeys();
    void testProfileKeyPersistence();
    void testFallbackBehavior();
    void cleanupTestCase();
};

void ProfileEncryptionTest::initTestCase()
{
    // Initialize the Qt application if not already done
    if (!QCoreApplication::instance()) {
        int argc = 1;
        char* argv[] = {"test"};
        new QCoreApplication(argc, argv);
    }
}

void ProfileEncryptionTest::testProfileBasedEncryption()
{
    const QString testPassword = "MySecretPassword123!";
    const QString testProfile = "TestProfile1";
    
    // Test basic encryption/decryption for a profile
    QString encrypted = SecureStringUtils::encryptStringForProfile(testPassword, testProfile);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(encrypted != testPassword);
    // Profile-aware encryption uses raw Base64, not SSU2: prefix
    QVERIFY(!encrypted.startsWith("SSU2:"));
    QVERIFY(QByteArray::fromBase64(encrypted.toUtf8()).size() > 0);
    
    QString decrypted = SecureStringUtils::decryptStringForProfile(encrypted, testProfile);
    QCOMPARE(decrypted, testPassword);
}

void ProfileEncryptionTest::testDifferentProfilesUseDifferentKeys()
{
    const QString testPassword = "MySecretPassword123!";
    const QString profile1 = "TestProfile1";
    const QString profile2 = "TestProfile2";
    
    // Encrypt the same password with different profiles
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(testPassword, profile1);
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(testPassword, profile2);
    
    // Encrypted values should be different (different keys/salts)
    QVERIFY(encrypted1 != encrypted2);
    
    // Each should decrypt correctly with its own profile
    QString decrypted1 = SecureStringUtils::decryptStringForProfile(encrypted1, profile1);
    QString decrypted2 = SecureStringUtils::decryptStringForProfile(encrypted2, profile2);
    
    QCOMPARE(decrypted1, testPassword);
    QCOMPARE(decrypted2, testPassword);
    
    // Cross-profile decryption should fail or return different results
    QString crossDecrypt1 = SecureStringUtils::decryptStringForProfile(encrypted1, profile2);
    QString crossDecrypt2 = SecureStringUtils::decryptStringForProfile(encrypted2, profile1);
    
    // These should not equal the original password
    QVERIFY(crossDecrypt1 != testPassword);
    QVERIFY(crossDecrypt2 != testPassword);
}

void ProfileEncryptionTest::testProfileKeyPersistence()
{
    const QString testPassword = "PersistenceTest!";
    const QString testProfile = "PersistenceProfile";
    
    // First encryption
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(testPassword, testProfile);
    QString decrypted1 = SecureStringUtils::decryptStringForProfile(encrypted1, testProfile);
    QCOMPARE(decrypted1, testPassword);
    
    // Second encryption - should use the same key and be decryptable
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(testPassword, testProfile);
    QString decrypted2 = SecureStringUtils::decryptStringForProfile(encrypted2, testProfile);
    QCOMPARE(decrypted2, testPassword);
    
    // The first encrypted value should still decrypt correctly
    QString redecrypted1 = SecureStringUtils::decryptStringForProfile(encrypted1, testProfile);
    QCOMPARE(redecrypted1, testPassword);
}

void ProfileEncryptionTest::testFallbackBehavior()
{
    const QString testPassword = "FallbackTest!";
    const QString testProfile = "FallbackProfile";
    
    // This should work regardless of keychain availability
    QString encrypted = SecureStringUtils::encryptStringForProfile(testPassword, testProfile);
    QVERIFY(!encrypted.isEmpty());
    // Profile-aware encryption uses raw Base64, not SSU2: prefix
    QVERIFY(!encrypted.startsWith("SSU2:"));
    QVERIFY(QByteArray::fromBase64(encrypted.toUtf8()).size() > 0);
    
    QString decrypted = SecureStringUtils::decryptStringForProfile(encrypted, testProfile);
    QCOMPARE(decrypted, testPassword);
}

void ProfileEncryptionTest::cleanupTestCase()
{
    // Any cleanup needed
}

QTEST_MAIN(ProfileEncryptionTest)
#include "ProfileEncryptionTest.moc"
