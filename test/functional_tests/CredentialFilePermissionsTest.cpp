/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

/*
 * Where there is no keychain, a saved password is kept in an encrypted file under
 * profiles/<name>/passwords/, and the key that decrypts it one directory up in
 * profiles/<name>/. Written with the default umask they are readable by every account
 * on the machine, which is what these cases guard against.
 *
 * Two things have to hold for that guard to mean anything, and both are checked rather
 * than assumed. The umask is pinned to the usual 022 in initTestCase(), and a scratch
 * file and directory are created to prove the pin took effect - a default ACL or a mount
 * option can override a umask, and then "owner-only" would prove nothing. The config
 * root is redirected with XDG_CONFIG_HOME, which QStandardPaths reads on Linux only:
 * on macOS and Windows the root persists between runs, so each case removes the profile
 * directory before it starts and every case asserts the key is absent before it saves.
 *
 * A third group of cases covers the log line that names where a password was found, in
 * both directions: that it appears when the encrypted file answered, and that it does
 * not appear when nothing could be decrypted.
 *
 * Run with: ctest -R CredentialFilePermissionsTest -V
 */

#include <QtTest/QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "CredentialManager.h"
#include "MudletPaths.h"
#include "SecureStringUtils.h"
#include "utils.h"

#if defined(Q_OS_UNIX)
#include <sys/stat.h>
#endif

#include "GroupedTest.h"
#include "PortableModeTestHelper.h"

// QTest::ignoreMessage can only assert that a message was printed, so counting the
// diagnostic is the only way to say it was not.
static QtMessageHandler previousMessageHandler = nullptr;
static int sourceClaims = 0;

static void countSourceClaims(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (message.contains(QLatin1String("in the encrypted file"))) {
        ++sourceClaims;
    }
    if (previousMessageHandler) {
        previousMessageHandler(type, context, message);
    }
}

class CredentialFilePermissionsTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfile = qsl("PermissionsProfile");
    // Longer than the 50 characters the earlier naming scheme truncated to, so that this
    // profile has a second, legacy-named copy of its credential
    const QString mLongProfile = qsl("PermissionsProfileWithAVeryLongNameThatTheOlderMudletHadToCutShort");
    const QString mKey = qsl("character");
    const QString mPassword = qsl("correct horse battery staple");

    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;
    QByteArray mSavedLoggingRules;
#if defined(Q_OS_UNIX)
    mode_t mSavedUmask = 0;
    bool mUmaskPinned = false;
#endif

    QString configRoot() const { return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation); }

    QString profileDirectory(const QString& profile) const { return qsl("%1/profiles/%2").arg(configRoot(), MudletPaths::sanitizeForPath(profile)); }

    // SecureStringUtils builds its paths from the raw profile name where CredentialManager
    // sanitizes, so for a long name the key and the credential are in different directories
    QString rawProfileDirectory(const QString& profile) const { return qsl("%1/profiles/%2").arg(configRoot(), profile); }

    QString credentialFile(const QString& profile) const { return qsl("%1/passwords/%2").arg(profileDirectory(profile), MudletPaths::sanitizeForPath(mKey)); }

    QString credentialDirectory(const QString& profile) const { return QFileInfo(credentialFile(profile)).absolutePath(); }

    QString encryptionKeyFile(const QString& profile) const { return qsl("%1/encryption_key").arg(rawProfileDirectory(profile)); }

    // The standalone SecureStringUtils store, which keeps its own file beside the credential
    QString standalonePasswordFile(const QString& profile) const { return qsl("%1/passwords/%2.dat").arg(rawProfileDirectory(profile), mKey); }

    // Where a Mudlet that predates the digest-bearing name filed the same credential: both
    // components simply cut to 50 characters
    QString legacyCredentialFile(const QString& profile) const { return qsl("%1/profiles/%2/passwords/%3").arg(configRoot(), profile.left(50), mKey); }

    static bool reachableByOthers(const QString& path)
    {
        const QFileDevice::Permissions permissions = QFileInfo(path).permissions();
        return permissions.testAnyFlags(QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther);
    }

    // What the default umask leaves behind, so that a case can put the files back the way an
    // earlier Mudlet wrote them
    static bool openToEveryone(const QString& path)
    {
        QFileDevice::Permissions permissions = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther;

        if (QFileInfo(path).isDir()) {
            permissions |= QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;
        }

        return QFile::setPermissions(path, permissions);
    }

    void saveAPassword(const QString& profile)
    {
        // The config root persists between runs on the platforms that ignore
        // XDG_CONFIG_HOME, and a key left owner-only by an earlier run would satisfy these
        // cases on its own
        QVERIFY2(!QFileInfo::exists(encryptionKeyFile(profile)), "this case started on a config root an earlier run left behind, so it would prove nothing");
        QVERIFY(CredentialManager::storeCredential(profile, mKey, mPassword));
        QVERIFY(QFileInfo::exists(credentialFile(profile)));
        QVERIFY(QFileInfo::exists(encryptionKeyFile(profile)));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config root cannot be redirected away from the real one");
        }

        QVERIFY(mConfigDir.isValid());
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        // The log cases read a qDebug() line, which a rule in the environment would filter
        // out before it ever reached them
        mSavedLoggingRules = qgetenv("QT_LOGGING_RULES");
        qputenv("QT_LOGGING_RULES", QByteArray());
#if defined(Q_OS_UNIX)
        // The umask a distribution ships: without it a developer's own 077 would let these
        // cases pass on files nothing had narrowed
        mSavedUmask = ::umask(S_IWGRP | S_IWOTH);
        mUmaskPinned = true;

        // A default ACL on the parent, or a mount option such as vfat's fmask, overrides a
        // umask - and then every "owner-only" assertion below would hold with the whole fix
        // taken out. Prove the pin is what decides a new file's and a new directory's mode.
        const QString scratchDirectory = qsl("%1/umask-control").arg(mConfigDir.path());
        QVERIFY(QDir().mkpath(scratchDirectory));
        QFile scratchFile(qsl("%1/file").arg(scratchDirectory));
        QVERIFY(scratchFile.open(QIODevice::WriteOnly));
        scratchFile.close();
        QVERIFY2(reachableByOthers(scratchDirectory), "the umask this case pins is not what decides a new directory's mode here, so 'owner-only' would prove nothing");
        QVERIFY2(reachableByOthers(scratchFile.fileName()), "the umask this case pins is not what decides a new file's mode here, so 'owner-only' would prove nothing");
#endif
    }

    void cleanupTestCase()
    {
#if defined(Q_OS_UNIX)
        if (mUmaskPinned) {
            ::umask(mSavedUmask);
        }
#endif
        mSavedXdgConfigHome.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
        mSavedLoggingRules.isNull() ? qunsetenv("QT_LOGGING_RULES") : qputenv("QT_LOGGING_RULES", mSavedLoggingRules);
    }

    // XDG_CONFIG_HOME only redirects the config root on Linux, so each case clears out what
    // the one before it left rather than trusting the root to be empty
    void init()
    {
        for (const QString& profile : {mProfile, mLongProfile}) {
            QDir(profileDirectory(profile)).removeRecursively();
            QDir(rawProfileDirectory(profile)).removeRecursively();
            QDir(qsl("%1/profiles/%2").arg(configRoot(), profile.left(50))).removeRecursively();
        }
    }

    // One row per file and directory the fix narrows, so that each of them is shown to need
    // the fix on its own rather than only the first one a case happens to reach
    void test_aSavedPasswordAndItsKeyAreOwnerOnly_data()
    {
        QTest::addColumn<QString>("path");
        QTest::newRow("the saved password") << credentialFile(mProfile);
        QTest::newRow("the key that decrypts it") << encryptionKeyFile(mProfile);
        QTest::newRow("the directory holding the password") << credentialDirectory(mProfile);
        QTest::newRow("the directory holding the key") << rawProfileDirectory(mProfile);
    }

    void test_aSavedPasswordAndItsKeyAreOwnerOnly()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("there are no POSIX permission bits to check on this platform");
#endif
        QFETCH(QString, path);

        saveAPassword(mProfile);

        QVERIFY2(!reachableByOthers(path), "a saved password left something other accounts on this machine can read");
    }

    void test_passwordFilesLeftOpenByAnEarlierMudletAreNarrowedWhenRead_data() { test_aSavedPasswordAndItsKeyAreOwnerOnly_data(); }

    void test_passwordFilesLeftOpenByAnEarlierMudletAreNarrowedWhenRead()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("there are no POSIX permission bits to check on this platform");
#endif
        QFETCH(QString, path);

        saveAPassword(mProfile);

        QVERIFY(openToEveryone(credentialFile(mProfile)));
        QVERIFY(openToEveryone(encryptionKeyFile(mProfile)));
        QVERIFY(openToEveryone(credentialDirectory(mProfile)));
        QVERIFY(openToEveryone(rawProfileDirectory(mProfile)));

        QCOMPARE(CredentialManager::retrieveCredential(mProfile, mKey), mPassword);

        QVERIFY2(!reachableByOthers(path), "reading a saved password left something other accounts on this machine can read");
    }

    // The copy under the earlier naming scheme is by definition one an older Mudlet wrote,
    // so it is the likeliest of all of them to have been left open
    void test_theCopyLeftByAnEarlierNamingSchemeIsNarrowedWhenRead()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("there are no POSIX permission bits to check on this platform");
#endif
        saveAPassword(mLongProfile);

        const QString legacyPath = legacyCredentialFile(mLongProfile);
        QVERIFY(legacyPath != credentialFile(mLongProfile));
        QVERIFY(QDir().mkpath(QFileInfo(legacyPath).absolutePath()));
        QVERIFY(QFile::copy(credentialFile(mLongProfile), legacyPath));
        QVERIFY(openToEveryone(legacyPath));

        // Newer than the copy under the current naming, which is what makes it the one read
        QFile legacyFile(legacyPath);
        QVERIFY(legacyFile.open(QIODevice::ReadWrite));
        QVERIFY(legacyFile.setFileTime(QDateTime::currentDateTime().addSecs(60), QFileDevice::FileModificationTime));
        legacyFile.close();

        QCOMPARE(CredentialManager::retrieveCredential(mLongProfile, mKey), mPassword);

        QVERIFY2(!reachableByOthers(legacyPath), "reading the copy left by the earlier naming scheme left it readable by other accounts on this machine");
    }

    // The passwords/<key>.dat pair SecureStringUtils offers on its own, which has no caller
    // in Mudlet today but writes the same kind of secret
    void test_theStandaloneSecureStringUtilsStoreIsOwnerOnlyToo()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("there are no POSIX permission bits to check on this platform");
#endif
        QVERIFY(SecureStringUtils::storePassword(mProfile, mKey, mPassword));

        const QString path = standalonePasswordFile(mProfile);
        QVERIFY(QFileInfo::exists(path));
        QVERIFY2(!reachableByOthers(path), "the standalone store left the password readable by other accounts on this machine");
        QVERIFY2(!reachableByOthers(QFileInfo(path).absolutePath()), "the standalone store left its directory open to other accounts on this machine");

        QVERIFY(openToEveryone(path));
        QCOMPARE(SecureStringUtils::retrievePassword(mProfile, mKey), mPassword);
        QVERIFY2(!reachableByOthers(path), "reading from the standalone store left the password readable by other accounts on this machine");
    }

    void test_theLogSaysWhichCredentialCameFromTheEncryptedFile()
    {
        saveAPassword(mProfile);

        // Anchored to the class that prints it, and naming the key, because the same
        // function answers for the proxy password and for the reconnect token as well
        QTest::ignoreMessage(QtDebugMsg,
                             QRegularExpression(QRegularExpression::escape(qsl(R"(CredentialManager: Found the "%1" credential for profile "%2" in the encrypted file)").arg(mKey, mProfile))));

        QCOMPARE(CredentialManager::retrieveCredential(mProfile, mKey), mPassword);
    }

    // The failure #10884 is about is a log line naming a source the password did not come
    // from, which the case above cannot catch: it asserts the very statement under test
    void test_nothingClaimsASourceWhenNoPasswordWasRecovered()
    {
        saveAPassword(mProfile);

        // Ciphertext this profile's key cannot make anything of, as a credential file that
        // was corrupted or encrypted under a key that has since been regenerated would be
        QFile file(credentialFile(mProfile));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(file.write("not anything this profile can decrypt") > 0);
        file.close();

        sourceClaims = 0;
        previousMessageHandler = qInstallMessageHandler(countSourceClaims);
        const QString recovered = CredentialManager::retrieveCredential(mProfile, mKey);
        qInstallMessageHandler(previousMessageHandler);

        QVERIFY(recovered.isEmpty());
        QCOMPARE(sourceClaims, 0);
    }
};

#include "CredentialFilePermissionsTest.moc"
MUDLET_GROUPED_TEST_MAIN(CredentialFilePermissionsTest)
