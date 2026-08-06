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
 * Locks in who Mudlet considers an experienced player. That decision gates all
 * of the first-time guidance - the interface tour, the starter UI package and
 * the one-line hints - so getting it wrong either buries a newcomer's
 * onboarding or drops a beginner tour on top of a ten-year veteran's session.
 *
 * The heuristic this replaced read profile *directory* mtimes, which record
 * when a profile was last written, not how long it has existed: every
 * QSaveFile profile write renames a temporary file into the directory and
 * bumps its mtime, so only an abandoned profile ever looked old.
 * test_upgraderWithFreshlyWrittenProfilesIsExperienced is that exact shape and
 * is the regression this file exists for.
 *
 * mudlet::rememberFirstLaunch() and mudlet::evaluateExperiencedPlayer() take
 * their settings, profiles path and "now" as arguments, so the table below runs
 * without a mudlet instance. The last case drives the real setupConfig() to
 * prove the recorded value and the reader agree on a live singleton.
 *
 * Run with: ctest -R ExperiencedPlayerGateTest -V
 */

#include <QtTest/QtTest>
#include <QTimeZone>

#include "mudlet.h"

class ExperiencedPlayerGateTest : public QObject
{
    Q_OBJECT

private:
    QByteArray mSavedXdg;
    // Fixed, so the six month arithmetic cannot straddle a real midnight
    const QDateTime mNow = QDateTime(QDate(2026, 8, 5), QTime(12, 0), QTimeZone::UTC);
    const QString mKey = qsl("firstLaunchDate");

    QString profilesPathIn(const QString& configDir) const { return qsl("%1/profiles").arg(configDir); }

    QString iniIn(const QString& configDir) const { return qsl("%1/Mudlet.ini").arg(configDir); }

    // Returns the new profile directory, or an empty string if it could not be created
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
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        // The singleton the last case starts is intentionally not deleted: it
        // was never init()'d, so its destructor would touch members only set up
        // by init(), and the process exits right after anyway.
    }

    // --- a brand new installation --------------------------------------------

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

    // A profiles directory that exists but holds nothing is still a first run -
    // an aborted earlier launch must not cost the user their onboarding.
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

    // --- the recorded date, once there is one --------------------------------

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

    // A clock set forward and back, or settings carried between machines, can
    // leave a first launch in the future. That is not tenure.
    void test_futureDatedFirstLaunchIsNotExperienced()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        setFirstLaunch(settings, mNow.addYears(1));

        QVERIFY(!mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
    }

    // --- upgrading users, who have no recorded first launch ------------------

    // The regression: profiles written seconds ago by an installation that has
    // been in use for years. The old directory-mtime heuristic called this a
    // brand new player, which is how veterans ended up with the 5.0 tour.
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

    // Timestamps are not consulted at all, which is what makes a profile copied
    // to a new machine or restored from a backup come out right - both reset
    // every timestamp on it, including the birth time.
    void test_restoredFromBackupIsExperienced()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        QVERIFY(!makeProfile(config.path(), qsl("Restored")).isEmpty());

        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
    }

    // Upgrading must not invent a first launch date - that would start the six
    // month clock now and hand the veteran a tour six months from today.
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

    // An unreadable or hand-edited value must not silently become "new player".
    void test_unparseableRecordFallsBackToTheProfilesCheck()
    {
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QSettings settings(iniIn(config.path()), QSettings::IniFormat);
        settings.setValue(mKey, qsl("not a date"));
        QVERIFY(!mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));

        QVERIFY(!makeProfile(config.path(), qsl("Achaea")).isEmpty());
        QVERIFY(mudlet::evaluateExperiencedPlayer(settings, profilesPathIn(config.path()), mNow));
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

    // --- the live singleton --------------------------------------------------

    // Proves the writer and the reader agree about which key and which profiles
    // path they are using, through the real config resolution. Runs last
    // because experiencedMudletPlayer() memoises for the life of the process.
    void test_freshInstallIsNewThroughTheRealSettings()
    {
        if (QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()))) {
            QSKIP("portable.txt present - setupConfig() takes the portable branch");
        }
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        // An empty $XDG_CONFIG_HOME/mudlet is the opt-in marker; without it
        // setupConfig() would keep using the real ~/.config/mudlet
        const QString configDir = qsl("%1/mudlet").arg(xdg.path());
        QVERIFY(QDir().mkpath(configDir));
        qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), configDir);
        const QString profiles = mudlet::getMudletPath(enums::profilesPath);
        QVERIFY(!QDir(profiles).exists());

        mudlet::rememberFirstLaunch(*mudlet::getQSettings(), profiles, QDateTime::currentDateTime());

        QVERIFY(mudlet::getQSettings()->contains(mKey));
        QVERIFY2(!mudlet::self()->experiencedMudletPlayer(), "a fresh install must classify as a new player through the real settings");
    }
};

#include "ExperiencedPlayerGateTest.moc"
QTEST_MAIN(ExperiencedPlayerGateTest)
