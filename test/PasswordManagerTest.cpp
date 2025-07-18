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

#include <PasswordManager.h>
#include <QtTest/QtTest>

class PasswordManagerTest : public QObject {
Q_OBJECT

private slots:
    void initTestCase();
    void testStoreAndRetrieve();
    void testProfileIsolation();
    void testKeyIsolation();
    void testEmptyPassword();
    void testRemovePassword();
    void cleanupTestCase();
};

void PasswordManagerTest::initTestCase()
{
}

void PasswordManagerTest::testStoreAndRetrieve()
{
    QString profile = "TestProfile";
    QString key = "test_password";
    QString password = "secret123";
    
    // Store password
    QVERIFY(PasswordManager::storePassword(profile, key, password));
    
    // Retrieve password
    QString retrieved = PasswordManager::retrievePassword(profile, key);
    QCOMPARE(retrieved, password);
}

void PasswordManagerTest::testProfileIsolation()
{
    QString profile1 = "Profile1";
    QString profile2 = "Profile2";
    QString key = "shared_key";
    QString password1 = "password1";
    QString password2 = "password2";
    
    // Store different passwords for different profiles
    QVERIFY(PasswordManager::storePassword(profile1, key, password1));
    QVERIFY(PasswordManager::storePassword(profile2, key, password2));
    
    // Verify isolation
    QCOMPARE(PasswordManager::retrievePassword(profile1, key), password1);
    QCOMPARE(PasswordManager::retrievePassword(profile2, key), password2);
}

void PasswordManagerTest::testKeyIsolation()
{
    QString profile = "TestProfile";
    QString key1 = "proxy";
    QString key2 = "database";
    QString password1 = "proxy_pass";
    QString password2 = "db_pass";
    
    // Store different passwords for different keys
    QVERIFY(PasswordManager::storePassword(profile, key1, password1));
    QVERIFY(PasswordManager::storePassword(profile, key2, password2));
    
    // Verify isolation
    QCOMPARE(PasswordManager::retrievePassword(profile, key1), password1);
    QCOMPARE(PasswordManager::retrievePassword(profile, key2), password2);
}

void PasswordManagerTest::testEmptyPassword()
{
    QString profile = "TestProfile";
    QString key = "empty_test";
    
    // Store empty password (should remove any existing password)
    QVERIFY(PasswordManager::storePassword(profile, key, ""));
    
    // Should return empty string
    QString retrieved = PasswordManager::retrievePassword(profile, key);
    QVERIFY(retrieved.isEmpty());
}

void PasswordManagerTest::testRemovePassword()
{
    QString profile = "TestProfile";
    QString key = "remove_test";
    QString password = "temp_password";
    
    // Store password
    QVERIFY(PasswordManager::storePassword(profile, key, password));
    QCOMPARE(PasswordManager::retrievePassword(profile, key), password);
    
    // Remove password
    QVERIFY(PasswordManager::removePassword(profile, key));
    
    // Should return empty string after removal
    QString retrieved = PasswordManager::retrievePassword(profile, key);
    QVERIFY(retrieved.isEmpty());
}

void PasswordManagerTest::cleanupTestCase()
{
    // Clean up test passwords
    PasswordManager::removePassword("TestProfile", "test_password");
    PasswordManager::removePassword("Profile1", "shared_key");
    PasswordManager::removePassword("Profile2", "shared_key");
    PasswordManager::removePassword("TestProfile", "proxy");
    PasswordManager::removePassword("TestProfile", "database");
    PasswordManager::removePassword("TestProfile", "empty_test");
    PasswordManager::removePassword("TestProfile", "remove_test");
}

#include "PasswordManagerTest.moc"
QTEST_MAIN(PasswordManagerTest)
