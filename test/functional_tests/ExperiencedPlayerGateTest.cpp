/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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
 * Locks in who Mudlet considers an experienced player. That decision gates the
 * first-time guidance - the interface tour and the one-line hints - so getting
 * it wrong either buries a newcomer's onboarding or drops a beginner tour on
 * top of a ten-year veteran's session.
 *
 * mudlet::rememberFirstLaunch() and mudlet::evaluateExperiencedPlayer() take
 * their settings, profiles path and "now" as arguments, so most cases run
 * without a mudlet instance. The last two drive a real init() instead, to pin
 * the production wiring.
 *
 * Run with: ctest -R ExperiencedPlayerGateTest -V
 */

#include <QtTest/QtTest>
#include <QTimeZone>

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ExperiencedPlayerGateTest : public QObject
{
    Q_OBJECT

private:
    QByteArray mSavedXdg;
    // Outlives the two live-singleton cases, which share one mudlet instance
    QTemporaryDir mLiveConfig;
    // Fixed, so the six month arithmetic does not depend on the day the suite runs
    const QDateTime mNow = QDateTime(QDate(2026, 8, 5), QTime(12, 0), QTimeZone::UTC);
    const QString mKey = qsl("firstLaunchDate");

    QString profilesPathIn(const QString& configDir) const { return qsl("%1/profiles").arg(configDir); }

    QString iniIn(const QString& configDir) const { return qsl("%1/Mudlet.ini").arg(configDir); }

    QString makeProfile(const QString& configDir, const QString& name) const
    {
        const QString path = qsl("%1/%2").arg(profilesPathIn(configDir), name);
        return QDir().mkpath(path) ? path : QString();
    }

    void setFirstLaunch(QSettings& settings, const QDateTime& when) const { settings.setValue(mKey, when.toUTC().toString(Qt::ISODate)); }

private slots:
    void initTestCase() { mSavedXdg = qgetenv("XDG_CONFIG_HOME"); }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // --- a brand new installation ---

    void test_freshInstallRecordsTodayAndIsNew()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        QVERIFY(!QDir(profilesPathIn(config.path())).exists());

        mudlet::rememberFirstLaunch(settings, profilesPathIn(config.path()), mNow);

        QCOMPARE(settings.value(mKey).toString(), mNow.toString(Qt::ISODate));
        QVERIFY2(!mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow), "a first-ever launch must be treated as a new player");
    }

    void test_emptyProfilesDirectoryIsStillAFirstRun()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QVERIFY(QDir().mkpath(profilesPathIn(config.path())));
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);

        mudlet::rememberFirstLaunch(settings, profilesPathIn(config.path()), mNow);

        QVERIFY(settings.contains(mKey));
        QVERIFY(!mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
    }

    // --- the recorded date, once there is one ---

    void test_recentlyRecordedFirstLaunchIsNew()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        setFirstLaunch(settings, mNow.addMonths(-1));
        QVERIFY(!makeProfile(config.path(), qsl("Achaea")).isEmpty());

        QVERIFY2(!mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow), "a month of use is not enough to be experienced, even with a profile in hand");
    }

    void test_oldRecordedFirstLaunchIsExperienced()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        setFirstLaunch(settings, mNow.addYears(-3));

        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
    }

    void test_sixMonthBoundary()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        const QString profiles = profilesPathIn(config.path());

        setFirstLaunch(settings, mNow.addMonths(-6).addDays(1));
        QVERIFY2(!mudlet::evaluateExperiencedPlayer(settings, profiles, mNow), "one day short of six months is not yet experienced");

        setFirstLaunch(settings, mNow.addMonths(-6));
        QVERIFY2(mudlet::evaluateExperiencedPlayer(settings, profiles, mNow), "exactly six months of use is experienced");

        setFirstLaunch(settings, mNow.addMonths(-6).addDays(-1));
        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profiles, mNow));
    }

    void test_futureDatedFirstLaunchIsNotExperienced()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        setFirstLaunch(settings, mNow.addYears(1));

        QVERIFY(!mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
    }

    // --- upgrading users, who have no recorded first launch ---

    void test_upgraderWithFreshlyWrittenProfilesIsExperienced()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        for (const auto& name : {qsl("Achaea"), qsl("StickMUD"), qsl("Legends of the Jedi")}) {
            const QString profile = makeProfile(config.path(), name);
            QVERIFY(!profile.isEmpty());
            QFile url(qsl("%1/url").arg(profile));
            QVERIFY(url.open(QIODevice::WriteOnly));
            url.write("achaea.com");
            url.close();
            QVERIFY2(QFileInfo(profile).lastModified() > mNow.addMonths(-6), "the fixture is only meaningful while the profile directory looks brand new");
        }

        QVERIFY2(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow), "an installation with profiles but no recorded first launch predates the key, so it is experienced");
    }

    // A restored profile may or may not keep its modification times, and never
    // keeps its birth time, so no timestamp is consulted
    void test_restoredFromBackupIsExperienced()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        QVERIFY(!makeProfile(config.path(), qsl("Restored")).isEmpty());

        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
    }

    void test_settingsWithoutProfilesStillCountAsEarlierUse()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        settings.setValue(qsl("pos"), QPoint(120, 80));
        QVERIFY(!QDir(profilesPathIn(config.path())).exists());

        mudlet::rememberFirstLaunch(settings, profilesPathIn(config.path()), mNow);

        QVERIFY2(!settings.contains(mKey), "an installation with settings on file is not on its first run");
        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
    }

    void test_unreadableProfilesDirectoryIsTakenAsPopulated()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        const QString profiles = profilesPathIn(config.path());
        QVERIFY(!makeProfile(config.path(), qsl("Achaea")).isEmpty());
        QVERIFY(QFile::setPermissions(profiles, QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        if (QFileInfo(profiles).isReadable()) {
            QVERIFY(QFile::setPermissions(profiles, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
            QSKIP("the profiles directory is readable despite the permissions - running as root?");
        }

        const bool experienced = mudlet::evaluateExperiencedPlayer(settings, profiles, mNow);
        // Restore before asserting, or a failure leaves QTemporaryDir unable to clean up
        QVERIFY(QFile::setPermissions(profiles, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        QVERIFY(experienced);
    }

    void test_upgradeDoesNotRecordAFirstLaunch()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        QVERIFY(!makeProfile(config.path(), qsl("Achaea")).isEmpty());

        mudlet::rememberFirstLaunch(settings, profilesPathIn(config.path()), mNow);

        QVERIFY(!settings.contains(mKey));
        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow.addYears(1)));
    }

    void test_existingRecordIsNeverOverwritten()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        const QDateTime original = mNow.addMonths(-3);
        setFirstLaunch(settings, original);

        mudlet::rememberFirstLaunch(settings, profilesPathIn(config.path()), mNow);

        QCOMPARE(settings.value(mKey).toString(), original.toString(Qt::ISODate));
    }

    void test_unparseableRecordFallsBackToTheEarlierUseCheck()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        settings.setValue(mKey, qsl("not a date"));

        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));

        mudlet::rememberFirstLaunch(settings, profilesPathIn(config.path()), mNow);
        QCOMPARE(settings.value(mKey).toString(), qsl("not a date"));
    }

    void test_recordedValueSurvivesAQSettingsRoundTrip()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        {
            QSettings writer(iniIn(config.path()), QSettings::IniFormat);
            mudlet::rememberFirstLaunch(writer, profilesPathIn(config.path()), mNow.addYears(-2));
        }

        QSettings reader(iniIn(config.path()), QSettings::IniFormat);
        QVERIFY2(mudlet::evaluateExperiencedPlayer(reader, profilesPathIn(config.path()), mNow), "the recorded date must be readable back out of Mudlet.ini");

        QFile ini(iniIn(config.path()));
        QVERIFY(ini.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY2(QString::fromUtf8(ini.readAll()).contains(qsl("firstLaunchDate=2024-08-05T12:00:00Z")), "the date is stored as plain ISO 8601, so it can be read and edited by hand");
    }

    // --- the live singleton ---

    // Nothing else in the suite notices the init() call being moved or dropped,
    // which would make every fresh install take the "used before" fallback
    void test_initRecordsTheFirstLaunchOnAFreshInstall()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - setupConfig() takes the portable branch");
        }
        QVERIFY(mLiveConfig.isValid());
        // $XDG_CONFIG_HOME/mudlet/profiles is the opt-in marker, without which
        // setupConfig() keeps using a legacy ~/.config/mudlet
        const QString configDir = qsl("%1/mudlet").arg(mLiveConfig.path());
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(configDir)));
        qputenv("XDG_CONFIG_HOME", mLiveConfig.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), configDir);
        QVERIFY2(mudlet::getQSettings()->allKeys().isEmpty(), "a fresh config dir must start out with an empty Mudlet.ini - something wrote settings before init()");
        QVERIFY2(QDir(mudlet::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty(), "the opt-in profiles/ dir has to be empty, or this is not a fresh install");
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));

        mudlet::self()->init();

        QVERIFY2(mudlet::getQSettings()->contains(mKey), "init() must record the first launch date");
        QCOMPARE(QDateTime::fromString(mudlet::getQSettings()->value(mKey).toString(), Qt::ISODate).isValid(), true);
    }

    // Pins the key and profiles path experiencedMudletPlayer() picks for itself,
    // which the case above cannot - there both branches would answer "new".
    // Runs last: experiencedMudletPlayer() memoises for the life of the process.
    void test_experiencedThroughTheRealSettings()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - setupConfig() takes the portable branch");
        }
        QVERIFY(mudlet::self());
        auto* settings = mudlet::getQSettings();
        QVERIFY(settings);
        settings->remove(mKey);
        QVERIFY(!makeProfile(mudlet::getMudletPath(enums::mainPath), qsl("Achaea")).isEmpty());

        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "a profile with no recorded first launch must read as an experienced player");
    }
};

#include "ExperiencedPlayerGateTest.moc"
MUDLET_GROUPED_TEST_MAIN(ExperiencedPlayerGateTest)
