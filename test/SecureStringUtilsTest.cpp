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

void SecureStringUtilsTest::cleanupTestCase()
{
}

#include "SecureStringUtilsTest.moc"
QTEST_MAIN(SecureStringUtilsTest)
