/***************************************************************************
 *   Copyright (C) 2025-2026 by Mike Conley - mike.conley@stickmud.com     *
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

// Tests for the portable password file → secure storage migration path.
//
// PR #7956 introduced a regression where the connection-profiles dialog
// stopped consulting the portable password file on disk when neither the
// system keychain nor QSettings contained a password for the selected
// profile.  These tests verify that:
//
// 1. A password stored in the old portable file format (QDataStream) can
//    be read back correctly.
// 2. The CredentialManager correctly stores and retrieves passwords that
//    have been migrated from the portable format.
// 3. If the CredentialManager has no password, the portable file is still
//    a valid source of truth (simulating the fallback path).

#include <CredentialManager.h>
#include <QDataStream>
#include <QDir>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QVersionNumber>
#include <QtTest/QtTest>

class PasswordMigrationTest : public QObject {
  Q_OBJECT

private:
  QTemporaryDir mTempDir;

  // Mirrors the portable password file write logic from
  // dlgConnectionProfiles::writeProfileData / mudlet::writeProfileData
  bool writePortablePasswordFile(const QString &profileDir, const QString &item,
                                 const QString &password) {
    const QString filePath = QStringLiteral("%1/%2").arg(profileDir, item);
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
      return false;
    }
    QDataStream ofs(&file);
    // Match the version used by Mudlet for Qt >= 5.13
    ofs.setVersion(QDataStream::Qt_5_12);
    ofs << password;
    return file.commit();
  }

  // Mirrors the portable password file read logic from
  // dlgConnectionProfiles::readProfileData / mudlet::readProfileData
  QString readPortablePasswordFile(const QString &profileDir,
                                   const QString &item) {
    const QString filePath = QStringLiteral("%1/%2").arg(profileDir, item);
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

private slots:
  void initTestCase() {
    qputenv("MUDLET_TEST_MODE", "1");
    QVERIFY(mTempDir.isValid());
  }

  // Verify the portable file round-trip: write → read is lossless
  void testPortableFileRoundTrip() {
    const QString profile = QStringLiteral("RoundTripProfile");
    const QString profileDir =
        QStringLiteral("%1/%2").arg(mTempDir.path(), profile);
    QVERIFY(QDir().mkpath(profileDir));

    const QString password = QStringLiteral("s3cret!P@ss");
    QVERIFY(writePortablePasswordFile(profileDir, QStringLiteral("password"),
                                      password));

    const QString retrieved =
        readPortablePasswordFile(profileDir, QStringLiteral("password"));
    QCOMPARE(retrieved, password);
  }

  // Verify empty file → empty string (no crash, no garbage)
  void testPortableFileEmptyPassword() {
    const QString profile = QStringLiteral("EmptyPassProfile");
    const QString profileDir =
        QStringLiteral("%1/%2").arg(mTempDir.path(), profile);
    QVERIFY(QDir().mkpath(profileDir));

    // Write an empty password
    QVERIFY(writePortablePasswordFile(profileDir, QStringLiteral("password"),
                                      QString()));

    const QString retrieved =
        readPortablePasswordFile(profileDir, QStringLiteral("password"));
    QVERIFY(retrieved.isEmpty());
  }

  // Verify no file → empty string (graceful fallback)
  void testPortableFileMissing() {
    const QString profile = QStringLiteral("MissingPassProfile");
    const QString profileDir =
        QStringLiteral("%1/%2").arg(mTempDir.path(), profile);
    QVERIFY(QDir().mkpath(profileDir));

    // Don't create a password file
    const QString retrieved =
        readPortablePasswordFile(profileDir, QStringLiteral("password"));
    QVERIFY(retrieved.isEmpty());
  }

  // Simulate the migration path: portable file exists, CredentialManager
  // is empty, migration reads portable file and stores into CredentialManager.
  void testMigrationPattern() {
    const QString profile = QStringLiteral("MigrationTestProfile");
    const QString profileDir =
        QStringLiteral("%1/%2").arg(mTempDir.path(), profile);
    QVERIFY(QDir().mkpath(profileDir));

    const QString password = QStringLiteral("migr@t1on_P@ss");

    // Step 1: CredentialManager has nothing for this profile
    const QString beforeMigration = CredentialManager::retrieveCredential(
        profile, QStringLiteral("character"));
    QVERIFY(beforeMigration.isEmpty());

    // Step 2: Write password to the portable file (simulating legacy state)
    QVERIFY(writePortablePasswordFile(profileDir, QStringLiteral("password"),
                                      password));

    // Step 3: Read from portable file (simulating what migration does)
    const QString portablePassword =
        readPortablePasswordFile(profileDir, QStringLiteral("password"));
    QCOMPARE(portablePassword, password);

    // Step 4: Store into CredentialManager (simulating
    // migratePasswordsToSecureStorage)
    QVERIFY(CredentialManager::storeCredential(
        profile, QStringLiteral("character"), portablePassword));

    // Step 5: Now CredentialManager has it
    const QString afterMigration = CredentialManager::retrieveCredential(
        profile, QStringLiteral("character"));
    QCOMPARE(afterMigration, password);

    // Cleanup
    CredentialManager::removeCredential(profile, QStringLiteral("character"));
  }

  // The core regression scenario: CredentialManager is empty, QSettings is
  // empty, but the portable file exists.  The fallback should find it.
  void testFallbackToPortableFile() {
    const QString profile = QStringLiteral("FallbackTestProfile");
    const QString profileDir =
        QStringLiteral("%1/%2").arg(mTempDir.path(), profile);
    QVERIFY(QDir().mkpath(profileDir));

    const QString password = QStringLiteral("f@llb@ck_P@ss!");

    // Credential manager has nothing
    QVERIFY(CredentialManager::retrieveCredential(profile,
                                                  QStringLiteral("character"))
                .isEmpty());

    // QSettings has nothing (we don't set anything)

    // Portable file has the password
    QVERIFY(writePortablePasswordFile(profileDir, QStringLiteral("password"),
                                      password));

    // The fallback logic should: check CredentialManager → empty,
    // check QSettings → empty, check portable file → found.
    // Here we verify each step of the chain independently.

    // CredentialManager: empty
    const QString fromCredMgr = CredentialManager::retrieveCredential(
        profile, QStringLiteral("character"));
    QVERIFY(fromCredMgr.isEmpty());

    // Portable file: has password (this is what the fix restores)
    const QString fromFile =
        readPortablePasswordFile(profileDir, QStringLiteral("password"));
    QCOMPARE(fromFile, password);

    // After finding it, the migration should store it for next time
    QVERIFY(CredentialManager::storeCredential(
        profile, QStringLiteral("character"), fromFile));
    QCOMPARE(CredentialManager::retrieveCredential(profile,
                                                   QStringLiteral("character")),
             password);

    // Cleanup
    CredentialManager::removeCredential(profile, QStringLiteral("character"));
  }

  // Verify that Unicode passwords survive the portable file round-trip
  void testPortableFileUnicodePassword() {
    const QString profile = QStringLiteral("UnicodePassProfile");
    const QString profileDir =
        QStringLiteral("%1/%2").arg(mTempDir.path(), profile);
    QVERIFY(QDir().mkpath(profileDir));

    const QString password = QStringLiteral("пароль密码パスワード🔑");
    QVERIFY(writePortablePasswordFile(profileDir, QStringLiteral("password"),
                                      password));

    const QString retrieved =
        readPortablePasswordFile(profileDir, QStringLiteral("password"));
    QCOMPARE(retrieved, password);
  }

  void cleanupTestCase() {
    // Defensive cleanup
    CredentialManager::removeCredential(QStringLiteral("MigrationTestProfile"),
                                        QStringLiteral("character"));
    CredentialManager::removeCredential(QStringLiteral("FallbackTestProfile"),
                                        QStringLiteral("character"));
  }
};

#include "PasswordMigrationTest.moc"
QTEST_MAIN(PasswordMigrationTest)
