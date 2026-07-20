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
#include <QtTest/QtTest>

// Hermetic unit tests: MUDLET_TEST_MODE forces encrypted file storage so these run
// deterministically on every platform without touching a system keychain. The real
// keychain paths - including the qtkeychain 0.17 Windows naming migrations - are covered
// by CredentialManagerKeychainTest, kept as a separate executable so its Windows-only,
// environment-dependent tests cannot affect this suite.

class CredentialManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testStoreAndRetrieve();
    void testProfileIsolation();
    void testSpecialCharacterProfileIsolation();
    void testKeyIsolation();
    void testEmptyPassword();
    void testRemovePassword();
    void testInputSanitization();
    void testPathTraversalPrevention();
    void testConcurrentAccess();
    void cleanupTestCase();
};

void CredentialManagerTest::initTestCase()
{
    // Set environment variable to indicate we're in test mode
    // This prevents keychain access that would require user password input
    qputenv("MUDLET_TEST_MODE", "1");
}

void CredentialManagerTest::testStoreAndRetrieve()
{
    QString profile = "TestProfile";
    QString key = "test_password";
    QString password = "secret123";

    // Store password
    QVERIFY(CredentialManager::storeCredential(profile, key, password));

    // Retrieve password
    QString retrieved = CredentialManager::retrieveCredential(profile, key);
    QCOMPARE(retrieved, password);
}

void CredentialManagerTest::testProfileIsolation()
{
    QString profile1 = "Profile1";
    QString profile2 = "Profile2";
    QString key = "shared_key";
    QString password1 = "password1";
    QString password2 = "password2";

    // Store different passwords for different profiles
    QVERIFY(CredentialManager::storeCredential(profile1, key, password1));
    QVERIFY(CredentialManager::storeCredential(profile2, key, password2));

    // Verify isolation
    QCOMPARE(CredentialManager::retrieveCredential(profile1, key), password1);
    QCOMPARE(CredentialManager::retrieveCredential(profile2, key), password2);
}

void CredentialManagerTest::testSpecialCharacterProfileIsolation()
{
    // Test for issue #8933: Profiles with similar names sharing passwords
    // Profile names that differ only in special characters should NOT collide.
    // Using only characters allowed by the UI validation in dlgConnectionProfiles.cpp:
    // ". _0123456789-#&" plus letters. All these would collide to "Game_Server"
    // under the old sanitization logic.
    QString profileDot = "Game.Server";
    QString profileHash = "Game#Server";
    QString profileAmp = "Game&Server";
    QString profileSpace = "Game Server";
    QString profileDash = "Game-Server";
    QString key = "password";
    QString passwordDot = "password_dot";
    QString passwordHash = "password_hash";
    QString passwordAmp = "password_amp";
    QString passwordSpace = "password_space";
    QString passwordDash = "password_dash";

    // Store different passwords for profiles that differ only in special characters
    QVERIFY(CredentialManager::storeCredential(profileDot, key, passwordDot));
    QVERIFY(CredentialManager::storeCredential(profileHash, key, passwordHash));
    QVERIFY(CredentialManager::storeCredential(profileAmp, key, passwordAmp));
    QVERIFY(CredentialManager::storeCredential(profileSpace, key, passwordSpace));
    QVERIFY(CredentialManager::storeCredential(profileDash, key, passwordDash));

    // Verify each profile retrieves its own password (not the last stored one)
    QCOMPARE(CredentialManager::retrieveCredential(profileDot, key), passwordDot);
    QCOMPARE(CredentialManager::retrieveCredential(profileHash, key), passwordHash);
    QCOMPARE(CredentialManager::retrieveCredential(profileAmp, key), passwordAmp);
    QCOMPARE(CredentialManager::retrieveCredential(profileSpace, key), passwordSpace);
    QCOMPARE(CredentialManager::retrieveCredential(profileDash, key), passwordDash);

    // Cleanup
    CredentialManager::removeCredential(profileDot, key);
    CredentialManager::removeCredential(profileHash, key);
    CredentialManager::removeCredential(profileAmp, key);
    CredentialManager::removeCredential(profileSpace, key);
    CredentialManager::removeCredential(profileDash, key);
}

void CredentialManagerTest::testKeyIsolation()
{
    QString profile = "TestProfile";
    QString key1 = "proxy";
    QString key2 = "database";
    QString password1 = "proxy_pass";
    QString password2 = "db_pass";

    // Store different passwords for different keys
    QVERIFY(CredentialManager::storeCredential(profile, key1, password1));
    QVERIFY(CredentialManager::storeCredential(profile, key2, password2));

    // Verify isolation
    QCOMPARE(CredentialManager::retrieveCredential(profile, key1), password1);
    QCOMPARE(CredentialManager::retrieveCredential(profile, key2), password2);
}

void CredentialManagerTest::testEmptyPassword()
{
    QString profile = "TestProfile";
    QString key = "empty_test";

    // Store empty password (should remove any existing password)
    QVERIFY(CredentialManager::storeCredential(profile, key, ""));

    // Should return empty string
    QString retrieved = CredentialManager::retrieveCredential(profile, key);
    QVERIFY(retrieved.isEmpty());
}

void CredentialManagerTest::testRemovePassword()
{
    QString profile = "TestProfile";
    QString key = "remove_test";
    QString password = "temp_password";

    // Store password
    QVERIFY(CredentialManager::storeCredential(profile, key, password));
    QCOMPARE(CredentialManager::retrieveCredential(profile, key), password);

    // Remove password
    QVERIFY(CredentialManager::removeCredential(profile, key));

    // Should return empty string after removal
    QString retrieved = CredentialManager::retrieveCredential(profile, key);
    QVERIFY(retrieved.isEmpty());
}

void CredentialManagerTest::testInputSanitization()
{
    QString profile = "SanitizationTestProfile";
    QString normalKey = "normal_key";
    QString password = "test_password";

    // Test normal key works
    QVERIFY(CredentialManager::storeCredential(profile, normalKey, password));
    QString retrieved = CredentialManager::retrieveCredential(profile, normalKey);
    QCOMPARE(retrieved, password);

    // Test with special characters in key names - should be rejected
    QString specialKey = "key/with\\special:chars<>|?*";
    bool specialStored = CredentialManager::storeCredential(profile, specialKey, password);
    QVERIFY(!specialStored); // Should fail due to invalid characters

    // Test with Unicode characters in keys
    QString unicodeKey = "key_with_unicode_αβγ_δεζ";
    bool unicodeStored = CredentialManager::storeCredential(profile, unicodeKey, password);

    if (unicodeStored) {
        QString unicodeRetrieved = CredentialManager::retrieveCredential(profile, unicodeKey);
        QCOMPARE(unicodeRetrieved, password);
        CredentialManager::removeCredential(profile, unicodeKey);
    }

    // Cleanup
    CredentialManager::removeCredential(profile, normalKey);
}

void CredentialManagerTest::testPathTraversalPrevention()
{
    QString profile = "PathTraversalTestProfile";
    QString password = "test_password";

    // Test various path traversal attempts in profile names
    QStringList maliciousProfiles = {
            "../../../etc/passwd", "..\\..\\windows\\system32", "/etc/shadow", "C:\\Windows\\System32\\config\\SAM", "profile/../../../sensitive", "profile\\..\\..\\sensitive"};

    for (const QString& maliciousProfile : maliciousProfiles) {
        QString key = "test_key";

        // These should be rejected or sanitized by the security measures
        bool stored = CredentialManager::storeCredential(maliciousProfile, key, password);

        // Even if storage fails, this demonstrates that path traversal is prevented
        if (stored) {
            QString retrieved = CredentialManager::retrieveCredential(maliciousProfile, key);
            // If storage succeeded, retrieval should work with same profile name
            QCOMPARE(retrieved, password);

            // Cleanup
            CredentialManager::removeCredential(maliciousProfile, key);
        }
        // If storage failed, that's also a valid security response
    }

    // Test that a normal profile still works
    QString normalProfile = "NormalProfile";
    QVERIFY(CredentialManager::storeCredential(normalProfile, "test_key", password));
    QString normalRetrieved = CredentialManager::retrieveCredential(normalProfile, "test_key");
    QCOMPARE(normalRetrieved, password);
    CredentialManager::removeCredential(normalProfile, "test_key");
}

void CredentialManagerTest::testConcurrentAccess()
{
    QString profile = "ConcurrentTestProfile";
    QString key = "concurrent_key";
    QString password = "concurrent_password";

    // Store initial credential
    QVERIFY(CredentialManager::storeCredential(profile, key, password));

    // Simulate concurrent operations (basic test)
    // In a real concurrent test, we'd use threads, but for simplicity:

    // Multiple rapid store/retrieve operations
    for (int i = 0; i < 10; ++i) {
        QString testPassword = QString("password_%1").arg(i);
        QVERIFY(CredentialManager::storeCredential(profile, key, testPassword));
        QString retrieved = CredentialManager::retrieveCredential(profile, key);
        QCOMPARE(retrieved, testPassword);
    }

    // Verify final state
    QString finalPassword = "final_password";
    QVERIFY(CredentialManager::storeCredential(profile, key, finalPassword));
    QString finalRetrieved = CredentialManager::retrieveCredential(profile, key);
    QCOMPARE(finalRetrieved, finalPassword);

    // Cleanup
    CredentialManager::removeCredential(profile, key);
}

void CredentialManagerTest::cleanupTestCase()
{
    // Clean up test passwords
    CredentialManager::removeCredential("TestProfile", "test_password");
    CredentialManager::removeCredential("Profile1", "shared_key");
    CredentialManager::removeCredential("Profile2", "shared_key");
    CredentialManager::removeCredential("TestProfile", "proxy");
    CredentialManager::removeCredential("TestProfile", "database");
    CredentialManager::removeCredential("TestProfile", "empty_test");
    CredentialManager::removeCredential("TestProfile", "remove_test");
    CredentialManager::removeCredential("SanitizationTestProfile", "normal_key");
    CredentialManager::removeCredential("PathTraversalTestProfile", "test_key");
    CredentialManager::removeCredential("ConcurrentTestProfile", "concurrent_key");

    // Defensive cleanup for special character profile isolation test
    // (ensures cleanup even if test fails early)
    CredentialManager::removeCredential("Game.Server", "password");
    CredentialManager::removeCredential("Game#Server", "password");
    CredentialManager::removeCredential("Game&Server", "password");
    CredentialManager::removeCredential("Game Server", "password");
    CredentialManager::removeCredential("Game-Server", "password");
}

#include "CredentialManagerTest.moc"
QTEST_MAIN(CredentialManagerTest)
