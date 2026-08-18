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
 * Locks in the XDG_CONFIG_HOME resolution that the Lua busted suite (and any
 * parallel run) relies on to isolate itself from the real ~/.config/mudlet, plus
 * the migration guard that keeps existing users on their legacy config dir. The
 * failure mode guarded against is a future refactor quietly dropping XDG support
 * or the guard, which resurfaces as parallel-run sqlite flakiness or, worse,
 * users' profiles appearing to vanish on upgrade.
 *
 * Creating $XDG_CONFIG_HOME/mudlet/profiles is the opt-in; the directory above it
 * on its own is not, because that is a state other tooling creates by accident.
 *
 * The resolution logic lives in utils::xdgConfigDir(legacyDefault), which takes
 * the legacy candidate as an argument, so most cases test it directly and stay
 * platform-independent (no HOME/USERPROFILE juggling). A couple of cases drive
 * the real mudlet::setupConfig() to prove the wiring end-to-end.
 *
 * Run with: ctest -R ConfigDirOverrideTest -V
 */

#include <QtTest/QtTest>

#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

class ConfigDirOverrideTest : public QObject
{
    Q_OBJECT

private:
    QByteArray mSavedXdg;

    QString mudletUnder(const QString& dir) const { return QDir::cleanPath(qsl("%1/mudlet").arg(dir)); }

    bool makeProfile(const QString& configDir, const QString& profileName) const { return QDir().mkpath(qsl("%1/profiles/%2").arg(configDir, profileName)); }

    bool makeSettingsFile(const QString& configDir) const
    {
        QFile ini(qsl("%1/Mudlet.ini").arg(configDir));
        return ini.open(QIODevice::WriteOnly);
    }

    // setupConfig() consults portable.txt beside the executable and in the home
    // config dir before the XDG/default logic; the setupConfig() integration
    // cases skip if one is present rather than report a baffling failure.
    bool portableMarkerPresent() const
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        mudlet::start();
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        // The singleton is intentionally not deleted: it was never init()'d, so
        // its destructor would touch members only set up by init(), and the
        // process exits right after anyway.
    }

    // --- utils::xdgConfigDir() resolution table -------------------------------

    void test_unsetUsesLegacy()
    {
        qunsetenv("XDG_CONFIG_HOME");
        const QString legacy = qsl("/home/someone/.config/mudlet");
        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY(!r.migrationPending);
    }

    void test_emptyBehavesLikeUnset()
    {
        qputenv("XDG_CONFIG_HOME", QByteArray());
        const QString legacy = qsl("/home/someone/.config/mudlet");
        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY(!r.migrationPending);
    }

    // A relative XDG_CONFIG_HOME is invalid per the spec and must be ignored, not
    // turned into a cwd-relative config root.
    void test_relativeXdgIgnored()
    {
        qputenv("XDG_CONFIG_HOME", QByteArray("relative/config"));
        const QString legacy = qsl("/home/someone/.config/mudlet");
        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY(!r.migrationPending);
    }

    void test_emptyXdgDirWinsOverALegacyDirWithoutProfiles()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(target));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(QDir().mkpath(legacy));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, target);
        QVERIFY(!r.migrationPending);
        QVERIFY(r.shadowedProfilesPath.isEmpty());
    }

    // A dotfile manager, container script or aborted move leaves this directory behind.
    void test_emptyXdgDirDoesNotHideLegacyProfiles()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        QVERIFY(QDir().mkpath(mudletUnder(xdg.path())));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(makeProfile(legacy, qsl("AlphaGame")));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY(r.migrationPending);
        QVERIFY(r.shadowedProfilesPath.isEmpty());
    }

    // The state one bad launch leaves behind, since Mudlet writes its Mudlet.ini
    // into whichever dir it chose. Deleting that dir has to be enough to recover.
    void test_xdgSettingsFileDoesNotHideLegacyProfiles()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(qsl("%1/fonts").arg(target)));
        QVERIFY(makeSettingsFile(target));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(makeProfile(legacy, qsl("AlphaGame")));
        QVERIFY(makeProfile(legacy, qsl("BetaGame")));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY(r.migrationPending);
        QVERIFY(r.shadowedProfilesPath.isEmpty());
    }

    void test_xdgProfilesDirWinsAndReportsShadowedLegacyProfiles()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(target)));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(makeProfile(legacy, qsl("AlphaGame")));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, target);
        QVERIFY(!r.migrationPending);
        QCOMPARE(r.shadowedProfilesPath, legacy);
    }

    void test_noShadowReportedWhenLegacyProfilesDirIsEmpty()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(target)));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(legacy)));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, target);
        QVERIFY(r.shadowedProfilesPath.isEmpty());
    }

    void test_migratedXdgDirWinsOverLegacy()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(target))); // looks like Mudlet's
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(QDir().mkpath(legacy));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, target);
        QVERIFY(!r.migrationPending);
    }

    // Regression: a stale $XDG_CONFIG_HOME/mudlet holding only the pre-4.19
    // NativeFormat Mudlet.conf (no profiles) must NOT shadow the user's real
    // profiles in the legacy dir - it must fall back and flag a migration.
    void test_staleNativeFormatDirDoesNotShadowProfiles()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(target));
        QFile stale(qsl("%1/Mudlet.conf").arg(target)); // the leftover, no Mudlet.ini/profiles
        QVERIFY(stale.open(QIODevice::WriteOnly));
        stale.close();
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(makeProfile(legacy, qsl("AlphaGame"))); // real profiles live here
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY2(r.migrationPending, "a stale non-Mudlet XDG dir must not shadow real profiles");
    }

    // Deleting the last profile leaves an empty legacy profiles/ behind, which
    // must not pull a config root in active use back out of $XDG_CONFIG_HOME.
    void test_emptyLegacyProfilesDirDoesNotOutrankXdgSettings()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(target));
        QVERIFY(makeSettingsFile(target));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(legacy)));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, target);
        QVERIFY(!r.migrationPending);
        QVERIFY(r.shadowedProfilesPath.isEmpty());
    }

    void test_guardKeepsLegacyWhenXdgTargetMissing()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        QVERIFY(!QDir(mudletUnder(xdg.path())).exists());
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(QDir().mkpath(legacy));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY(r.migrationPending);
        QVERIFY(r.shadowedProfilesPath.isEmpty());
    }

    void test_freshInstallUsesXdgWhenNeitherExists()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(!QDir(target).exists());
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(!QDir(legacy).exists());
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, target);
        QVERIFY(!r.migrationPending);
    }

    // With XDG_CONFIG_HOME=$HOME/.config the XDG target IS the legacy dir; an
    // existing one must not be reported as a pending migration (it would
    // otherwise nag the user on every startup).
    void test_noMigrationWhenXdgTargetEqualsLegacy()
    {
        QTemporaryDir cfg; // stands in for $HOME/.config
        QVERIFY(cfg.isValid());
        const QString legacy = mudletUnder(cfg.path());
        QVERIFY(QDir().mkpath(legacy));
        qputenv("XDG_CONFIG_HOME", cfg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY2(!r.migrationPending, "no migration when the XDG target and legacy dir are the same");
    }

    // XDG_CONFIG_HOME=$HOME/.config is an ordinary export, and would otherwise
    // warn on every startup about the directory it is using.
    void test_noSelfShadowWhenXdgTargetEqualsLegacyWithProfiles()
    {
        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        const QString legacy = mudletUnder(cfg.path());
        QVERIFY(makeProfile(legacy, qsl("AlphaGame")));
        qputenv("XDG_CONFIG_HOME", cfg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY2(r.shadowedProfilesPath.isEmpty(), "a directory cannot shadow itself");
    }

    void test_noSelfShadowThroughASymlinkedConfigDir()
    {
        QTemporaryDir real;
        QTemporaryDir linkHome;
        QVERIFY(real.isValid() && linkHome.isValid());
        const QString legacy = mudletUnder(real.path());
        QVERIFY(makeProfile(legacy, qsl("AlphaGame")));
        const QString linked = qsl("%1/config-link").arg(linkHome.path());
        if (!QFile::link(real.path(), linked)) {
            QSKIP("this filesystem does not support symlinks");
        }
        qputenv("XDG_CONFIG_HOME", linked.toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QVERIFY2(r.shadowedProfilesPath.isEmpty(), "one directory under two names is still one directory");
    }

    // The only case that observes the settings tier, and losing those settings
    // drops firstLaunchDate, which re-runs onboarding.
    void test_settingsOnlyLegacyOutranksEmptyXdgDir()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        QVERIFY(QDir().mkpath(mudletUnder(xdg.path())));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(QDir().mkpath(legacy));
        QVERIFY(makeSettingsFile(legacy));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        QCOMPARE(r.path, legacy);
        QVERIFY(r.migrationPending);
    }

    void test_unreadableLegacyDirStillOutranksAnEmptyXdgDir()
    {
        QTemporaryDir xdg;
        QTemporaryDir legacyHome;
        QVERIFY(xdg.isValid() && legacyHome.isValid());
        QVERIFY(QDir().mkpath(mudletUnder(xdg.path())));
        const QString legacy = mudletUnder(legacyHome.path() + qsl("/.config"));
        QVERIFY(makeProfile(legacy, qsl("AlphaGame")));
        // No traverse bit either, or QDir::exists() on profiles/ still answers and
        // the ranking never has to fall back
        if (!QFile::setPermissions(legacy, QFileDevice::Permissions())) {
            QSKIP("cannot drop permissions on this filesystem");
        }
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        const auto r = utils::xdgConfigDir(legacy);
        const bool readableAnyway = QFileInfo(legacy).isReadable();
        QVERIFY(QFile::setPermissions(legacy, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        if (readableAnyway) {
            QSKIP("running as a user that bypasses permission bits");
        }
        QCOMPARE(r.path, legacy);
        QVERIFY(r.migrationPending);
    }

    // --- mudlet::setupConfig() end-to-end wiring ------------------------------

    void test_setupConfigUsesPreCreatedXdgTarget()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - setupConfig() takes the portable branch");
        }
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        const QString target = mudletUnder(xdg.path());
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(target)));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), target);
    }

    // The warning is all that tells an affected user where their other profiles went.
    void test_setupConfigWarnsAboutShadowedLegacyProfiles()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - setupConfig() takes the portable branch");
        }
        const QString legacy = qsl("%1/.config/mudlet").arg(QDir::homePath());
        if (!utils::configDirHoldsProfiles(legacy)) {
            QSKIP("no profiles in the real ~/.config/mudlet, so nothing can be shadowed");
        }
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(mudletUnder(xdg.path()))));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("holds profiles as well")));
        mudlet::self()->setupConfig();
    }

    // With XDG unset, the config root is the usual ~/.config/mudlet, so normal
    // users are unaffected. Uses the real home dir - no HOME override.
    void test_setupConfigUnsetUsesHomeConfigDir()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - config root is deliberately relocated");
        }
        qunsetenv("XDG_CONFIG_HOME");
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/.config/mudlet").arg(QDir::homePath()));
    }
};

#include "ConfigDirOverrideTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConfigDirOverrideTest)
