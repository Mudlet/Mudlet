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

#include <SecureStringUtils.h>
#include <QtTest/QtTest>

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
    void cleanupTestCase();
};

void SecureStringUtilsTest::initTestCase()
{
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
    
    // Clear the string
    SecureStringUtils::secureStringClear(testString);
    
    // String should be empty after clearing
    QVERIFY(testString.isEmpty());
    QVERIFY(testString != originalContent);
    
    // Test QByteArray clearing
    QByteArray testArray = "sensitive_bytes";
    QByteArray originalArray = testArray;
    
    SecureStringUtils::secureByteArrayClear(testArray);
    QVERIFY(testArray.isEmpty());
    QVERIFY(testArray != originalArray);
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
    
    // Test with corrupted encrypted data (modify a character in the middle)
    if (encrypted.length() > 10) {
        QString corrupted = encrypted;
        corrupted[encrypted.length() / 2] = 'X'; // Corrupt middle character
        QString decrypted2 = SecureStringUtils::decryptStringForProfile(corrupted, profile);
        QVERIFY(decrypted2.isEmpty() || decrypted2 != password); // Should fail due to corruption
    }
    
    // Test with truncated encrypted data
    if (encrypted.length() > 5) {
        QString truncated = encrypted.left(encrypted.length() - 5);
        QString decrypted3 = SecureStringUtils::decryptStringForProfile(truncated, profile);
        QVERIFY(decrypted3.isEmpty() || decrypted3 != password); // Should fail due to truncation
    }
}

void SecureStringUtilsTest::cleanupTestCase()
{
}

#include "SecureStringUtilsTest.moc"
QTEST_MAIN(SecureStringUtilsTest)
