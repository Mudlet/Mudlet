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

class ProxyPasswordCompatibilityTest : public QObject {
Q_OBJECT

private slots:
    void initTestCase();
    void testPlaintextToEncryptedMigration();
    void testProfileIsolation();
    void testRegressionFromPlaintext();
    void cleanupTestCase();
};

void ProxyPasswordCompatibilityTest::initTestCase()
{
}

void ProxyPasswordCompatibilityTest::testPlaintextToEncryptedMigration()
{
    QString plainPassword = "legacy_password";
    QString profileName = "MigrationTestProfile";
    
    // Simulate reading a plaintext password from settings (legacy behavior)
    QVERIFY(!SecureStringUtils::isEncryptedFormat(plainPassword));
    
    // "Migrate" by encrypting the plaintext password
    QString encryptedPassword = SecureStringUtils::encryptStringForProfile(plainPassword, profileName);
    QVERIFY(SecureStringUtils::isEncryptedFormat(encryptedPassword));
    
    // Verify it can be decrypted back to original
    QString decryptedPassword = SecureStringUtils::decryptStringForProfile(encryptedPassword, profileName);
    QCOMPARE(decryptedPassword, plainPassword);
}

void ProxyPasswordCompatibilityTest::testProfileIsolation()
{
    // This test verifies that legacy migration preserves profile isolation
    QString password = "shared_password";
    QString profile1 = "Profile_A";
    QString profile2 = "Profile_B";
    
    // Simulate legacy migration: plaintext -> encrypted for different profiles
    QString encrypted1 = SecureStringUtils::encryptStringForProfile(password, profile1);
    QString encrypted2 = SecureStringUtils::encryptStringForProfile(password, profile2);
    
    // Verify migration preserves profile-specific encryption
    QVERIFY(encrypted1 != encrypted2);
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted1));
    QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted2));
    
    // Verify migration maintains decryption isolation
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted1, profile1), password);
    QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted2, profile2), password);
    
    // Verify cross-profile decryption still fails after migration
    QVERIFY(SecureStringUtils::decryptStringForProfile(encrypted1, profile2) != password);
    QVERIFY(SecureStringUtils::decryptStringForProfile(encrypted2, profile1) != password);
}

void ProxyPasswordCompatibilityTest::testRegressionFromPlaintext()
{
    // Test edge cases: various plaintext passwords that should NOT be detected as encrypted
    // This prevents false positives during legacy migration
    QStringList plaintextPasswords = {
        "password",
        "admin123", 
        "P@ssw0rd!",
        "user@domain.com",
        "very_long_password_with_underscores_and_numbers_12345",
        "短密码",  // Short Unicode password
        "" // Empty passwords should also not be detected as encrypted
    };
    
    for (const QString& password : plaintextPasswords) {
        // Critical: ensure these are NOT detected as encrypted during migration
        QVERIFY(!SecureStringUtils::isEncryptedFormat(password));
        
        if (!password.isEmpty()) {
            // Verify migration path works correctly
            QString encrypted = SecureStringUtils::encryptStringForProfile(password, "TestProfile");
            QVERIFY(SecureStringUtils::isEncryptedFormat(encrypted));
            QCOMPARE(SecureStringUtils::decryptStringForProfile(encrypted, "TestProfile"), password);
        }
    }
}

void ProxyPasswordCompatibilityTest::cleanupTestCase()
{
}

#include "ProxyPasswordCompatibilityTest.moc"
QTEST_MAIN(ProxyPasswordCompatibilityTest)
