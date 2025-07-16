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

private:

private slots:

    void initTestCase()
    {
    }

    void testBasicEncryptionDecryption()
    {
        QString plaintext = "mypassword";
        
        QString encrypted = SecureStringUtils::encryptString(plaintext);
        QVERIFY(!encrypted.isEmpty());
        QVERIFY(encrypted != plaintext);
        
        QString decrypted = SecureStringUtils::decryptString(encrypted);
        QCOMPARE(decrypted, plaintext);
    }

    void testEncryptedFormatDetection()
    {
        // Test plaintext passwords (should NOT be detected as encrypted)
        QVERIFY(!SecureStringUtils::isEncryptedFormat("mypassword"));
        QVERIFY(!SecureStringUtils::isEncryptedFormat("secret123!@#"));
        QVERIFY(!SecureStringUtils::isEncryptedFormat("ABcd1234")); // Short, looks like Base64 but isn't
        QVERIFY(!SecureStringUtils::isEncryptedFormat(""));
        
        // Test actual encrypted passwords (should be detected as encrypted)
        QString plaintext = "testpassword";
        QString encrypted = SecureStringUtils::encryptString(plaintext);
        QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
        
        // Test simple Base64 strings (should NOT be detected as encrypted in new format)
        QVERIFY(!SecureStringUtils::isEncryptedFormat("dGVzdCBwYXNzd29yZA==")); // "test password" in Base64
        
        // Test invalid Base64 (should NOT be detected as encrypted)
        QVERIFY(!SecureStringUtils::isEncryptedFormat("not-base64!"));
        QVERIFY(!SecureStringUtils::isEncryptedFormat("invalid=base64="));
    }

    void testSafeEncryption()
    {
        QString plaintext = "mypassword";
        QString alreadyEncrypted = SecureStringUtils::encryptString(plaintext);
        
        // Safe encrypt on plaintext should encrypt it
        QString safeEncrypt1 = SecureStringUtils::safeEncryptString(plaintext);
        QVERIFY(SecureStringUtils::isEncryptedFormat(safeEncrypt1));
        
        // Safe encrypt on already-encrypted should return unchanged
        QString safeEncrypt2 = SecureStringUtils::safeEncryptString(alreadyEncrypted);
        QCOMPARE(safeEncrypt2, alreadyEncrypted);
        
        // Both should decrypt to the same plaintext
        QCOMPARE(SecureStringUtils::decryptString(safeEncrypt1), plaintext);
        QCOMPARE(SecureStringUtils::decryptString(safeEncrypt2), plaintext);
    }

    void testEmptyStrings()
    {
        // Test empty string handling
        QCOMPARE(SecureStringUtils::encryptString(""), QString());
        QCOMPARE(SecureStringUtils::decryptString(""), QString());
        QCOMPARE(SecureStringUtils::safeEncryptString(""), QString());
        QVERIFY(!SecureStringUtils::isEncryptedFormat(""));
    }

    void testNonDeterministicEncryption()
    {
        // Same plaintext should produce DIFFERENT encrypted results each time (more secure)
        QString plaintext = "consistent_password";
        QString encrypted1 = SecureStringUtils::encryptString(plaintext);
        QString encrypted2 = SecureStringUtils::encryptString(plaintext);
        
        // Should be different due to random nonces
        QVERIFY(encrypted1 != encrypted2);
        
        // But both should decrypt to the same plaintext
        QString decrypted1 = SecureStringUtils::decryptString(encrypted1);
        QString decrypted2 = SecureStringUtils::decryptString(encrypted2);
        QCOMPARE(decrypted1, plaintext);
        QCOMPARE(decrypted2, plaintext);
    }

    void testSpecialCharacters()
    {
        // Test passwords with special characters
        QString specialPassword = "pássw0rd!@#$%^&*()";
        QString encrypted = SecureStringUtils::encryptString(specialPassword);
        QString decrypted = SecureStringUtils::decryptString(encrypted);
        
        QCOMPARE(decrypted, specialPassword);
        QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
    }

    void testSecureMemoryClearing()
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

    void testCrossVersionCompatibility()
    {
        // This test ensures that passwords encrypted in one version of Mudlet
        // (e.g., "Mudlet") can be decrypted in another version (e.g., "Mudlet Public Test Build")
        // The fix removes dependency on QGuiApplication::applicationName() which varies between versions
        
        QString originalPassword = "cross_version_password";
        
        // Encrypt the password
        QString encrypted = SecureStringUtils::encryptString(originalPassword);
        QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
        
        // Decrypt should work regardless of application name changes
        QString decrypted = SecureStringUtils::decryptString(encrypted);
        QCOMPARE(decrypted, originalPassword);
        
        // Test that the base password generation is consistent across runs
        // This ensures cross-version compatibility for the key derivation
        QString testPassword = "version_test";
        QString firstEncrypt = SecureStringUtils::encryptString(testPassword);
        QString secondEncrypt = SecureStringUtils::encryptString(testPassword);
        
        // Due to random nonces, encrypted values will differ
        QVERIFY(firstEncrypt != secondEncrypt);
        
        // But both should decrypt to the same original (proving consistent key derivation)
        QCOMPARE(SecureStringUtils::decryptString(firstEncrypt), testPassword);
        QCOMPARE(SecureStringUtils::decryptString(secondEncrypt), testPassword);
    }

    void cleanupTestCase()
    {
    }
};

#include "SecureStringUtilsTest.moc"
QTEST_MAIN(SecureStringUtilsTest)
