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
#include <SecureStringUtils.h>
#include <utils.h>

#include <QtTest/QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>

// Hermetic unit tests: MUDLET_TEST_MODE forces encrypted file storage so these run
// deterministically on every platform without touching a system keychain. The real
// keychain paths - including the qtkeychain 0.17 Windows naming migrations - are covered
// by CredentialManagerKeychainTest, kept as a separate executable so its Windows-only,
// environment-dependent tests cannot affect this suite.
//
// Two APIs are covered here. The static one is file storage by construction. The
// async one is what dlgConnectionProfiles and dlgProfilePreferences actually call,
// and MUDLET_TEST_MODE routes it down the same file storage, so its callbacks are
// delivered synchronously and need no event loop.

namespace {
// Where a credential sits for a path component spelled out in full - which is how a Mudlet
// that shortened a long name by truncation alone filed one, and where such a Mudlet still
// looks today.
QString credentialPath(const QString& profileComponent, const QString& key)
{
    return qsl("%1/profiles/%2/passwords/%3").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), profileComponent, key);
}

// Writes a credential straight to one path, encrypted for that profile: what an older
// Mudlet's own store leaves behind.
bool plantCredential(const QString& path, const QString& profileName, const QString& credential)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return false;
    }

    const QString encrypted = SecureStringUtils::encryptStringForProfile(credential, profileName);
    QFile file(path);

    if (encrypted.isEmpty() || !file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    return file.write(encrypted.toUtf8()) != -1;
}

// What such a file holds, read the way the profile it was encrypted for reads it
QString plantedCredential(const QString& path, const QString& profileName)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    return SecureStringUtils::decryptStringForProfile(QString::fromUtf8(file.readAll()), profileName);
}

// The 50 characters two long names share is the path component a Mudlet that shortened
// names by truncation alone filed its credential under, so each case below needs both
// halves of its profile name - and cleanupTestCase() needs the whole name again. Named
// once here so that the two cannot drift apart, and so that the shape of a colliding
// pair is stated where the names are rather than in each case.
const QString scmKeptPrefix(50, QChar('k'));
const QString scmKeptProfile = scmKeptPrefix + qsl("KeptForAnOlderMudlet");
const QString scmNewerCopyPrefix(50, QChar('n'));
const QString scmNewerCopyProfile = scmNewerCopyPrefix + qsl("NewerCopyIsTheOneHandedBack");
const QString scmWrittenThroughPrefix(50, QChar('w'));
const QString scmWrittenThroughProfile = scmWrittenThroughPrefix + qsl("WrittenThroughToTheOlderPath");
const QString scmEmptyCredentialPrefix(50, QChar('e'));
const QString scmEmptyCredentialProfile = scmEmptyCredentialPrefix + qsl("EmptyCredentialSavedHere");
const QString scmCollidingPrefix(50, QChar('c'));
const QString scmFirstCollidingProfile = scmCollidingPrefix + qsl("FirstOfTwoCollidingNames");
const QString scmSecondCollidingProfile = scmCollidingPrefix + qsl("SecondOfTwoCollidingNames");
const QString scmNeverFiledPrefix(50, QChar('f'));
const QString scmNeverFiledProfile = scmNeverFiledPrefix + qsl("NeverFiledUnderTheTruncatedPath");

// Which of two copies of one credential was written last is what decides between them, so a
// case that needs one to be the newer says so outright rather than leaving a filesystem's
// timestamp resolution to separate two writes made a moment apart.
bool setModificationTime(const QString& path, const int secondsFromNow)
{
    QFile file(path);
    return file.open(QIODevice::ReadWrite) && file.setFileTime(QDateTime::currentDateTime().addSecs(secondsFromNow), QFileDevice::FileModificationTime);
}
} // namespace

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
    void testOverlongKeyNamesAreRejected();
    void testLongProfileNamesDoNotShareOneCredential();
    void testLongKeyNamesDoNotShareOneCredential();
    void testCredentialsFiledUnderTheTruncatedPathSurvive();
    void testLegacySharedKeyPathOwnershipIsUndecidable();
    void testLegacyKeyAtTheLengthCapClaimsNoLongerKeysCredential();
    void testTheTruncatedPathKeepsItsCopyForAnOlderMudlet();
    void testTheNewerOfTheTwoCopiesIsTheOneHandedBack();
    void testAPasswordSavedHereReachesTheTruncatedPathToo();
    void testAnEmptyCredentialTakesTheTruncatedPathAwayRatherThanEmptyingIt();
    void testTheTruncatedPathIsNeverCreatedNorTakenOver();
    void testPathTraversalPrevention();
    void testConcurrentAccess();
    void testAsyncStoreAndRetrieve();
    void testAsyncApiSharesTheStoreWithTheStaticOne();
    void testAsyncRemovePassword();
    void testCredentialExistsWithoutHandingOverTheSecret();
    void testAsyncEmptyArgumentsAreReportedThroughTheCallback();
    void testAsyncApiRefusesTheKeysTheStaticApiRefuses();
    void cleanupTestCase();

private:
    QTemporaryDir mConfigDir;
};

void CredentialManagerTest::initTestCase()
{
    // Set environment variable to indicate we're in test mode
    // This prevents keychain access that would require user password input
    qputenv("MUDLET_TEST_MODE", "1");

    // Credentials are filed under QStandardPaths::AppConfigLocation, so without
    // this the suite writes into the home directory of whoever runs it. Only
    // takes effect where QStandardPaths honours XDG, which is where ctest runs
    // several of these binaries at once.
    QVERIFY(mConfigDir.isValid());
    qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
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

// 100 characters is the longest key name the API accepts and one more is refused
// outright. The accepted key is stored first so that the refusal has something it
// could damage: the two differ only in that last character, so anything that
// shortened them to a common path component would put them on one credential.
void CredentialManagerTest::testOverlongKeyNamesAreRejected()
{
    const QString profile = "OverlongKeyProfile";
    const QString password = "test_password";
    const QString longestAccepted(100, QChar('k'));
    const QString tooLong(101, QChar('k'));

    QVERIFY(CredentialManager::storeCredential(profile, longestAccepted, password));
    QCOMPARE(CredentialManager::retrieveCredential(profile, longestAccepted), password);

    QVERIFY(!CredentialManager::storeCredential(profile, tooLong, password));
    QVERIFY(CredentialManager::retrieveCredential(profile, tooLong).isEmpty());
    QVERIFY(!CredentialManager::removeCredential(profile, tooLong));

    // and the rejection left the accepted key's credential alone
    QCOMPARE(CredentialManager::retrieveCredential(profile, longestAccepted), password);

    CredentialManager::removeCredential(profile, longestAccepted);
}

// A profile name is a path component too, and one long enough to be shortened to fit
// used to become the same component as any other name starting the same way. Both
// profiles then wrote to one file, so the one that saved first read back whatever the
// other had encrypted with its own key - nothing it could decrypt.
void CredentialManagerTest::testLongProfileNamesDoNotShareOneCredential()
{
    const QString sharedPrefix(50, QChar('p'));
    const QString first = sharedPrefix + "FirstProfile";
    const QString second = sharedPrefix + "SecondProfile";
    const QString key = "character";

    QVERIFY(CredentialManager::storeCredential(first, key, "first_secret"));
    QVERIFY(CredentialManager::storeCredential(second, key, "second_secret"));

    QCOMPARE(CredentialManager::retrieveCredential(first, key), QString("first_secret"));
    QCOMPARE(CredentialManager::retrieveCredential(second, key), QString("second_secret"));

    CredentialManager::removeCredential(first, key);
    CredentialManager::removeCredential(second, key);
}

// Two long keys under one profile share the profile's encryption key, so a shared path
// component does not even fail closed the way two profiles do: whoever asks for the
// first key is handed the second key's secret in full
void CredentialManagerTest::testLongKeyNamesDoNotShareOneCredential()
{
    const QString profile = "LongKeyProfile";
    const QString sharedPrefix(50, QChar('k'));
    const QString first = sharedPrefix + "first";
    const QString second = sharedPrefix + "second";

    QVERIFY(CredentialManager::storeCredential(profile, first, "first_secret"));
    QVERIFY(CredentialManager::storeCredential(profile, second, "second_secret"));

    QCOMPARE(CredentialManager::retrieveCredential(profile, first), QString("first_secret"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, second), QString("second_secret"));

    CredentialManager::removeCredential(profile, first);
    CredentialManager::removeCredential(profile, second);
}

// An installed Mudlet has credentials on disk under the shortened path, so the file is
// written here the way that Mudlet wrote it rather than through the API. It has to come
// back, and it has to stop being reachable by the name it used to collide with - which
// is what storing for the neighbouring profile afterwards checks.
void CredentialManagerTest::testCredentialsFiledUnderTheTruncatedPathSurvive()
{
    const QString sharedPrefix(50, QChar('t'));
    const QString profile = sharedPrefix + "FiledBeforeTheRename";
    const QString neighbour = sharedPrefix + "StoredAfterwards";
    const QString key = "character";

    const QString legacyDir = qsl("%1/profiles/%2/passwords").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), sharedPrefix);
    QVERIFY(QDir().mkpath(legacyDir));

    QFile legacyFile(qsl("%1/%2").arg(legacyDir, key));
    QVERIFY(legacyFile.open(QIODevice::WriteOnly | QIODevice::Text));
    const QString encrypted = SecureStringUtils::encryptStringForProfile("saved_long_ago", profile);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(legacyFile.write(encrypted.toUtf8()) != -1);
    legacyFile.close();

    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("saved_long_ago"));

    QVERIFY(CredentialManager::storeCredential(neighbour, key, "the_neighbour"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("saved_long_ago"));
    QCOMPARE(CredentialManager::retrieveCredential(neighbour, key), QString("the_neighbour"));

    // and removing it takes the shortened file with it, so the next read cannot bring
    // the removed password back
    QVERIFY(CredentialManager::removeCredential(profile, key));
    QVERIFY(CredentialManager::retrieveCredential(profile, key).isEmpty());

    CredentialManager::removeCredential(neighbour, key);
}

// Migration decides whose a legacy credential is by decrypting it, which separates two
// profiles but cannot separate two keys inside one - they share that profile's encryption
// key, so the file reads back cleanly whichever key asked. Two keys long enough to have
// been cut to the same path are therefore undecidable, and the file has to be left where
// it is: migrating it hands the second key the first one's secret, and removing it deletes
// a credential that was never this key's to delete.
void CredentialManagerTest::testLegacySharedKeyPathOwnershipIsUndecidable()
{
    const QString profile = "SharedLegacyKeyProfile";
    const QString sharedPrefix(50, QChar('L'));
    const QString firstKey = sharedPrefix + "first";
    const QString secondKey = sharedPrefix + "second";

    const QString legacyDir = qsl("%1/profiles/%2/passwords").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), profile);
    QVERIFY(QDir().mkpath(legacyDir));

    // planted the way the old scheme wrote it: both keys cut to their shared first 50
    const QString legacyPath = qsl("%1/%2").arg(legacyDir, sharedPrefix);
    QFile legacyFile(legacyPath);
    QVERIFY(legacyFile.open(QIODevice::WriteOnly | QIODevice::Text));
    const QString encrypted = SecureStringUtils::encryptStringForProfile("first_secret", profile);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(legacyFile.write(encrypted.toUtf8()) != -1);
    legacyFile.close();

    QVERIFY2(CredentialManager::retrieveCredential(profile, secondKey).isEmpty(), "a second key was handed the first key's legacy secret");

    // and the removal half: the file is still there afterwards, rather than having been
    // deleted on behalf of a key that never wrote it
    QVERIFY(CredentialManager::removeCredential(profile, secondKey));
    QVERIFY2(QFile::exists(legacyPath), "removing a second key deleted the first key's legacy credential");

    QVERIFY(QFile::remove(legacyPath));
    CredentialManager::removeCredential(profile, firstKey);
}

// The cap is the boundary, not the far side of it: a key of exactly the legacy 50
// characters was cut to the same path as every longer key starting with them, because
// truncation left it unchanged. That file is no more decidably its own than a longer
// key's is, so it may neither be handed what it holds nor delete it. A profile name long
// enough to have moved is what brings a key this length past migration at all, hence one
// here.
void CredentialManagerTest::testLegacyKeyAtTheLengthCapClaimsNoLongerKeysCredential()
{
    const QString shortenedProfile(50, QChar('p'));
    const QString profile = shortenedProfile + "LongEnoughToHaveBeenShortenedItself";
    const QString sharedPrefix(50, QChar('K'));
    const QString keyAtTheCap = sharedPrefix;
    const QString longerKey = sharedPrefix + "second";

    const QString legacyDir = qsl("%1/profiles/%2/passwords").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), shortenedProfile);
    QVERIFY(QDir().mkpath(legacyDir));

    // planted the way the old scheme wrote the longer key: both keys cut to the same 50
    // characters, under the profile name cut to its own first 50
    const QString legacyPath = qsl("%1/%2").arg(legacyDir, sharedPrefix);
    QFile legacyFile(legacyPath);
    QVERIFY(legacyFile.open(QIODevice::WriteOnly | QIODevice::Text));
    const QString encrypted = SecureStringUtils::encryptStringForProfile("the_longer_keys_secret", profile);
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(legacyFile.write(encrypted.toUtf8()) != -1);
    legacyFile.close();

    QVERIFY2(CredentialManager::retrieveCredential(profile, keyAtTheCap).isEmpty(), "a key at the length cap was handed a longer key's legacy secret");

    // and the removal half, as in the case above
    QVERIFY(CredentialManager::removeCredential(profile, keyAtTheCap));
    QVERIFY2(QFile::exists(legacyPath), "removing a key at the length cap deleted a longer key's legacy credential");

    QVERIFY(QFile::remove(legacyPath));
    CredentialManager::removeCredential(profile, longerKey);
}

// The file the earlier naming scheme left is the only place a Mudlet from before the rename
// looks, so bringing a credential across has to copy it and not move it. Configuration
// directories are shared between installed Mudlets, so moving it means a user who still has
// an older one loses the password from it the moment a newer Mudlet has read it once.
void CredentialManagerTest::testTheTruncatedPathKeepsItsCopyForAnOlderMudlet()
{
    const QString profile = scmKeptProfile;
    const QString key = "character";
    const QString legacyPath = credentialPath(scmKeptPrefix, key);

    QVERIFY(plantCredential(legacyPath, profile, "saved_by_the_older_mudlet"));

    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("saved_by_the_older_mudlet"));

    QVERIFY2(QFile::exists(legacyPath), "reading the credential once took away the only copy an older Mudlet can find");
    QCOMPARE(plantedCredential(legacyPath, profile), QString("saved_by_the_older_mudlet"));

    // and it is under the current naming as well, which is what makes that copy the one
    // this Mudlet goes to from here on
    QVERIFY(QFile::exists(credentialPath(utils::sanitizeForPath(profile), key)));

    CredentialManager::removeCredential(profile, key);
}

// Two Mudlets, one credential, two files - and only the older one of the two writes the
// truncated path. Whichever file was written last therefore holds the password the user set
// last, so a password re-entered in that older Mudlet has to win over the copy this naming
// scheme already holds rather than the stale one being handed back for ever.
void CredentialManagerTest::testTheNewerOfTheTwoCopiesIsTheOneHandedBack()
{
    const QString profile = scmNewerCopyProfile;
    const QString key = "character";
    const QString legacyPath = credentialPath(scmNewerCopyPrefix, key);
    const QString currentPath = credentialPath(utils::sanitizeForPath(profile), key);

    QVERIFY(CredentialManager::storeCredential(profile, key, "the_password_this_mudlet_knows"));
    QVERIFY(QFile::exists(currentPath));

    QVERIFY(plantCredential(legacyPath, profile, "re_entered_in_the_older_mudlet"));
    QVERIFY(setModificationTime(legacyPath, 30));

    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("re_entered_in_the_older_mudlet"));

    // and it is taken over rather than only read, so this naming scheme has it too once the
    // two are no longer in that order
    QVERIFY(setModificationTime(legacyPath, -30));
    QCOMPARE(plantedCredential(currentPath, profile), QString("re_entered_in_the_older_mudlet"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("re_entered_in_the_older_mudlet"));

    CredentialManager::removeCredential(profile, key);
}

// The same interoperability from the other side: a password changed here has to reach the
// file the older Mudlet reads as well, or that Mudlet goes on offering the one it was
// changed from.
void CredentialManagerTest::testAPasswordSavedHereReachesTheTruncatedPathToo()
{
    const QString profile = scmWrittenThroughProfile;
    const QString key = "character";
    const QString legacyPath = credentialPath(scmWrittenThroughPrefix, key);
    const QString currentPath = credentialPath(utils::sanitizeForPath(profile), key);

    QVERIFY(plantCredential(legacyPath, profile, "the_password_both_started_with"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("the_password_both_started_with"));

    QVERIFY(CredentialManager::storeCredential(profile, key, "the_password_changed_here"));

    QCOMPARE(plantedCredential(legacyPath, profile), QString("the_password_changed_here"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("the_password_changed_here"));

    // Both copies, not only the one the read happened to take. Which of the two a later read
    // prefers is deliberately not pinned: a store writes both inside one millisecond, which
    // is all the resolution QFileInfo::lastModified() has, so neither is "written after" the
    // other in whichever order they go - and a tie leaves fileWrittenAfter() answering for
    // the file under the current naming.
    QCOMPARE(plantedCredential(currentPath, profile), QString("the_password_changed_here"));

    // and removing the password still takes both copies with it, so neither Mudlet is left
    // holding one that was deleted. That is what file storage does, which is what these
    // cases run on; a removal that goes to a system keychain instead leaves both files
    // where they are.
    QVERIFY(CredentialManager::removeCredential(profile, key));
    QVERIFY2(!QFile::exists(legacyPath), "the copy an older Mudlet reads outlived the removal of the password");
    QVERIFY(CredentialManager::retrieveCredential(profile, key).isEmpty());
}

// An empty credential stands for "no password", and the file the earlier naming scheme left
// cannot be made to hold that: an empty file decrypts to nothing, which is exactly how a
// file belonging to a profile this one used to collide with reads. A profile that wrote one
// could therefore never recognise that file as its own again - it could neither refresh nor
// remove it - and the older Mudlet would be left with a file it can only complain about.
void CredentialManagerTest::testAnEmptyCredentialTakesTheTruncatedPathAwayRatherThanEmptyingIt()
{
    const QString profile = scmEmptyCredentialProfile;
    const QString key = "character";
    const QString legacyPath = credentialPath(scmEmptyCredentialPrefix, key);

    QVERIFY(plantCredential(legacyPath, profile, "the_password_both_started_with"));
    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("the_password_both_started_with"));

    // what clearing the password field stores
    QVERIFY(CredentialManager::storeCredential(profile, key, QString("")));

    QVERIFY2(!QFile::exists(legacyPath), "an empty credential left a file at the truncated path that no profile can claim, refresh or remove again");

    // ...and the profile can still be given that path back, so a password set in the older
    // Mudlet afterwards is still kept in step rather than the two being locked apart
    QVERIFY(plantCredential(legacyPath, profile, "set_again_in_the_older_mudlet"));
    QVERIFY(CredentialManager::storeCredential(profile, key, "set_again_here"));
    QCOMPARE(plantedCredential(legacyPath, profile), QString("set_again_here"));

    CredentialManager::removeCredential(profile, key);
}

// Keeping the older scheme's file in step is only safe where one is already there and
// already this profile's own: two profiles whose names share their first 50 characters share
// that path, so writing it for the second would put them back on a single file - the very
// collision the digest-bearing name exists to end.
void CredentialManagerTest::testTheTruncatedPathIsNeverCreatedNorTakenOver()
{
    const QString first = scmFirstCollidingProfile;
    const QString second = scmSecondCollidingProfile;
    const QString key = "character";
    const QString legacyPath = credentialPath(scmCollidingPrefix, key);

    QVERIFY(plantCredential(legacyPath, first, "the_first_profiles_password"));
    QCOMPARE(CredentialManager::retrieveCredential(first, key), QString("the_first_profiles_password"));

    // the second profile saving a password of its own, and then changing it, leaves the
    // shared file exactly as the first profile left it
    QVERIFY(CredentialManager::storeCredential(second, key, "the_second_profiles_password"));
    QVERIFY(CredentialManager::storeCredential(second, key, "the_second_profiles_password_changed"));
    QCOMPARE(plantedCredential(legacyPath, first), QString("the_first_profiles_password"));

    QCOMPARE(CredentialManager::retrieveCredential(first, key), QString("the_first_profiles_password"));
    QCOMPARE(CredentialManager::retrieveCredential(second, key), QString("the_second_profiles_password_changed"));

    // ...and that holds when the shared file is the newer of the two as well, which is the
    // one place the two rules have to agree: modification time decides which file a read
    // prefers, but only decrypting it decides whose credential it holds
    QVERIFY(setModificationTime(legacyPath, 30));
    QCOMPARE(CredentialManager::retrieveCredential(second, key), QString("the_second_profiles_password_changed"));
    QCOMPARE(plantedCredential(legacyPath, first), QString("the_first_profiles_password"));

    // and a profile that never had a file at the truncated path is not given one, so no
    // sharing is created where there was none
    const QString neverFiledThere = scmNeverFiledProfile;
    QVERIFY(CredentialManager::storeCredential(neverFiledThere, key, "a_password_saved_only_here"));
    QVERIFY2(!QFile::exists(credentialPath(scmNeverFiledPrefix, key)), "a credential was created at the path two long names would share");

    CredentialManager::removeCredential(first, key);
    CredentialManager::removeCredential(second, key);
    CredentialManager::removeCredential(neverFiledThere, key);
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

// The async API is what the connection dialog and the preferences dialog call,
// so a credential stored through it has to come back through it
void CredentialManagerTest::testAsyncStoreAndRetrieve()
{
    CredentialManager manager;
    const QString profile = "AsyncProfile";
    const QString key = "password";

    bool stored = false;
    QString storeError = "callback never ran";
    manager.storePassword(profile, key, "async_secret", [&](bool success, const QString& error) {
        stored = success;
        storeError = error;
    });
    QVERIFY2(stored, qPrintable(storeError));
    QVERIFY(storeError.isEmpty());

    bool retrieved = false;
    QString password;
    QString retrieveError = "callback never ran";
    manager.retrievePassword(profile, key, [&](bool success, QString value, const QString& error) {
        retrieved = success;
        password = value;
        retrieveError = error;
    });
    QVERIFY2(retrieved, qPrintable(retrieveError));
    QCOMPARE(password, QString("async_secret"));
    QVERIFY(retrieveError.isEmpty());
}

// The static API is the migration and cleanup path for credentials the async API
// wrote, so the two have to be looking at the same store rather than two of them
void CredentialManagerTest::testAsyncApiSharesTheStoreWithTheStaticOne()
{
    CredentialManager manager;
    const QString profile = "AsyncSharedProfile";
    const QString key = "password";

    bool stored = false;
    manager.storePassword(profile, key, "shared_secret", [&](bool success, const QString&) {
        stored = success;
    });
    QVERIFY(stored);
    QCOMPARE(CredentialManager::retrieveCredential(profile, key), QString("shared_secret"));

    // and the other way round
    QVERIFY(CredentialManager::storeCredential(profile, key, "written_statically"));
    QString password;
    manager.retrievePassword(profile, key, [&](bool, QString value, const QString&) {
        password = value;
    });
    QCOMPARE(password, QString("written_statically"));
}

void CredentialManagerTest::testAsyncRemovePassword()
{
    CredentialManager manager;
    const QString profile = "AsyncRemovalProfile";
    const QString key = "password";

    bool stored = false;
    manager.storePassword(profile, key, "doomed_secret", [&](bool success, const QString&) {
        stored = success;
    });
    QVERIFY(stored);

    bool removed = false;
    QString removeError = "callback never ran";
    manager.removePassword(profile, key, [&](bool success, const QString& error) {
        removed = success;
        removeError = error;
    });
    QVERIFY2(removed, qPrintable(removeError));

    bool retrieved = true;
    QString password = "not overwritten";
    manager.retrievePassword(profile, key, [&](bool success, QString value, const QString&) {
        retrieved = success;
        password = value;
    });
    QVERIFY(!retrieved);
    QVERIFY(password.isEmpty());
    QVERIFY(CredentialManager::retrieveCredential(profile, key).isEmpty());
}

// The preferences dialog asks only whether a password is set, so this must answer
// without the caller ever holding the secret - and a profile with no password
// stored is not a profile with an empty password
void CredentialManagerTest::testCredentialExistsWithoutHandingOverTheSecret()
{
    CredentialManager manager;
    const QString profile = "AsyncExistsProfile";
    const QString key = "password";

    // Whether the question was answered is tracked apart from the answer, so a
    // callback that never runs cannot read as "no credential stored" - a caller
    // that re-enables itself from this reply would hang on exactly that
    bool answered = false;
    bool present = false;
    auto lookUp = [&manager, &profile, &key, &answered, &present]() {
        answered = false;
        present = false;
        manager.credentialExists(profile, key, [&answered, &present](bool value) {
            answered = true;
            present = value;
        });
    };

    lookUp();
    QVERIFY(answered);
    QVERIFY(!present);

    bool stored = false;
    manager.storePassword(profile, key, "existing_secret", [&](bool success, const QString&) {
        stored = success;
    });
    QVERIFY(stored);
    lookUp();
    QVERIFY(answered);
    QVERIFY(present);

    // An empty password is how "no password saved" is spelled, so it does not
    // count as a stored credential. It has to be an empty QString rather than a
    // null one: storeCredentialToFile rejects a null credential as a programming
    // error, so a null here would assert nothing about existence.
    manager.storePassword(profile, key, QString(""), [&](bool success, const QString&) {
        stored = success;
    });
    QVERIFY(stored);
    lookUp();
    QVERIFY(answered);
    QVERIFY(!present);

    manager.removePassword(profile, key, [](bool, const QString&) {});
    lookUp();
    QVERIFY(answered);
    QVERIFY(!present);
}

// Every entry point answers its caller rather than returning silently, which is
// what a dialog waiting on the callback to re-enable itself depends on
void CredentialManagerTest::testAsyncEmptyArgumentsAreReportedThroughTheCallback()
{
    CredentialManager manager;
    const QString empty;

    // Which complaint comes back matters as much as the failure itself. Without
    // the guard these calls still fail - they get as far as generateFilePath,
    // which refuses an empty component - so only the message distinguishes a
    // rejected argument from storage that went wrong.
    const QString rejected = "cannot be empty";

    // Either half missing is enough, so a guard narrowed to one of them is caught
    const QList<QPair<QString, QString>> incomplete = {{empty, "password"}, {"AsyncGuardProfile", empty}, {empty, empty}};

    for (const auto& [profile, key] : incomplete) {
        bool stored = true;
        QString storeError;
        manager.storePassword(profile, key, "secret", [&](bool success, const QString& error) {
            stored = success;
            storeError = error;
        });
        QVERIFY(!stored);
        QVERIFY2(storeError.contains(rejected), qPrintable(storeError));

        bool retrieved = true;
        QString retrieveError;
        manager.retrievePassword(profile, key, [&](bool success, QString, const QString& error) {
            retrieved = success;
            retrieveError = error;
        });
        QVERIFY(!retrieved);
        QVERIFY2(retrieveError.contains(rejected), qPrintable(retrieveError));

        bool removed = true;
        QString removeError;
        manager.removePassword(profile, key, [&](bool success, const QString& error) {
            removed = success;
            removeError = error;
        });
        QVERIFY(!removed);
        QVERIFY2(removeError.contains(rejected), qPrintable(removeError));

        bool answered = false;
        bool present = true;
        manager.credentialExists(profile, key, [&](bool value) {
            answered = true;
            present = value;
        });
        QVERIFY(answered);
        QVERIFY(!present);
    }
}

// The dialogs store through the async API while migration and cleanup read through the
// static one, so a key only one of them accepts is a credential that can be written and
// never reached again. Which complaint comes back matters too: without the check these
// keys either store happily or fail somewhere further down for an unrelated reason.
void CredentialManagerTest::testAsyncApiRefusesTheKeysTheStaticApiRefuses()
{
    CredentialManager manager;
    const QString profile = "AsyncKeyValidationProfile";
    const QString refusal = "not valid";

    const QStringList refused = {QString(101, QChar('k')), "keys/with/separators", "keys\\with\\separators", "..", "key_with|a_pipe"};

    for (const QString& key : refused) {
        QVERIFY2(!CredentialManager::storeCredential(profile, key, "secret"), qPrintable(key));
        QVERIFY2(CredentialManager::retrieveCredential(profile, key).isEmpty(), qPrintable(key));

        // Each of these starts at the answer the unguarded code gives, so a callback that
        // never runs reads as a failure rather than as agreement
        bool stored = true;
        QString storeError;
        manager.storePassword(profile, key, "secret", [&](bool success, const QString& error) {
            stored = success;
            storeError = error;
        });
        QVERIFY2(!stored, qPrintable(key));
        QVERIFY2(storeError.contains(refusal), qPrintable(storeError));

        bool retrieved = true;
        QString password = "not overwritten";
        QString retrieveError;
        manager.retrievePassword(profile, key, [&](bool success, QString value, const QString& error) {
            retrieved = success;
            password = value;
            retrieveError = error;
        });
        QVERIFY2(!retrieved, qPrintable(key));
        QVERIFY(password.isEmpty());
        QVERIFY2(retrieveError.contains(refusal), qPrintable(retrieveError));

        bool removed = true;
        QString removeError;
        manager.removePassword(profile, key, [&](bool success, const QString& error) {
            removed = success;
            removeError = error;
        });
        QVERIFY2(!removed, qPrintable(key));
        QVERIFY2(removeError.contains(refusal), qPrintable(removeError));

        bool answered = false;
        bool present = true;
        manager.credentialExists(profile, key, [&](bool value) {
            answered = true;
            present = value;
        });
        QVERIFY(answered);
        QVERIFY2(!present, qPrintable(key));
    }

    // a key both APIs accept still works, so the check is not simply refusing everything
    const QString accepted = "reconnect";
    bool stored = false;
    manager.storePassword(profile, accepted, "async_secret", [&](bool success, const QString&) {
        stored = success;
    });
    QVERIFY(stored);
    QCOMPARE(CredentialManager::retrieveCredential(profile, accepted), QString("async_secret"));
    CredentialManager::removeCredential(profile, accepted);
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

    CredentialManager::removeCredential("AsyncProfile", "password");
    CredentialManager::removeCredential("AsyncSharedProfile", "password");
    CredentialManager::removeCredential("AsyncRemovalProfile", "password");
    CredentialManager::removeCredential("AsyncExistsProfile", "password");
    CredentialManager::removeCredential("OverlongKeyProfile", QString(100, QChar('k')));
    CredentialManager::removeCredential("AsyncKeyValidationProfile", "reconnect");

    const QString longProfilePrefix(50, QChar('p'));
    CredentialManager::removeCredential(longProfilePrefix + "FirstProfile", "character");
    CredentialManager::removeCredential(longProfilePrefix + "SecondProfile", "character");

    const QString longKeyPrefix(50, QChar('k'));
    CredentialManager::removeCredential("LongKeyProfile", longKeyPrefix + "first");
    CredentialManager::removeCredential("LongKeyProfile", longKeyPrefix + "second");

    const QString truncatedPrefix(50, QChar('t'));
    CredentialManager::removeCredential(truncatedPrefix + "FiledBeforeTheRename", "character");
    CredentialManager::removeCredential(truncatedPrefix + "StoredAfterwards", "character");

    for (const QString& profile :
         {scmKeptProfile, scmNewerCopyProfile, scmWrittenThroughProfile, scmEmptyCredentialProfile, scmFirstCollidingProfile, scmSecondCollidingProfile, scmNeverFiledProfile}) {
        CredentialManager::removeCredential(profile, "character");
    }
}

#include "CredentialManagerTest.moc"
QTEST_MAIN(CredentialManagerTest)
