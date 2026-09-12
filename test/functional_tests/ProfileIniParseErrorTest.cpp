/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * A profile.ini that cannot be parsed has to be reported: the file holds the
 * command lines' history settings and the notepad's window state, and all of it
 * is silently replaced when the parse fails.
 *
 * Host now keeps one QSettings open for the profile's lifetime instead of making
 * one per write, and the status check moved with it - but the constructor only
 * splits the file into sections, and each section is parsed on the first lookup
 * that needs it, so damage inside one still reads as NoError there and the
 * diagnostic could never fire for it.
 *
 * Run with: ctest -R ProfileIniParseErrorTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryDir>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Counts the parse diagnostic so that the case for a well-formed file can say it
// was not printed - QTest::ignoreMessage only ever asserts the other direction.
// Matched on the function that emits it rather than on the words "could not be
// parsed", which any other subsystem is free to say about anything.
static QtMessageHandler previousMessageHandler = nullptr;
static int parseWarnings = 0;

static void countParseWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (message.contains(QLatin1String("Host::profileIni() ERROR"))) {
        ++parseWarnings;
    }
    if (previousMessageHandler) {
        previousMessageHandler(type, context, message);
    }
}

class ProfileIniParseErrorTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    // Damage inside a section, which is where a crash or a full disk leaves it -
    // and the only kind that gets this far, since QSettings splits the file into
    // sections as it is constructed and so catches a broken section header there
    static QByteArray unparseableIni() { return QByteArrayLiteral("[CommandLines]\nUsedIndexes=1\nthis line was truncated mid-write\n"); }

    static QString iniPathFor(const QString& profileName) { return mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("profile.ini")); }

    // Whether QSettings really does refuse the fixture, which is what the whole case
    // rests on. Asked of a copy at a path of its own, never of the fixture itself:
    // QSettings keeps the sections it has parsed per file path and shares them between
    // instances - and beyond the life of the one that parsed them - so probing the
    // fixture would answer the question by consuming the parse the Host has to make.
    static bool refusedByQSettings(const QString& path, const QString& copyPath)
    {
        if (!QFile::exists(copyPath) && !QFile::copy(path, copyPath)) {
            return false;
        }
        QSettings probe(copyPath, QSettings::IniFormat);
        probe.allKeys();
        return probe.status() == QSettings::FormatError;
    }

    // The Host has to meet the file on its first use of it: profile.ini is
    // opened once and kept, so anything written here afterwards would not be
    // read again. A null 'contents' is a profile that has never written one.
    Host* hostWithProfileIni(const QString& profileName, const QByteArray& contents)
    {
        const QString iniPath = iniPathFor(profileName);
        if (!QDir().mkpath(QFileInfo(iniPath).absolutePath())) {
            return nullptr;
        }
        if (!contents.isNull()) {
            QFile ini(iniPath);
            if (!ini.open(QIODevice::WriteOnly | QIODevice::Truncate) || ini.write(contents) != contents.size()) {
                return nullptr;
            }
            ini.close();
        }

        auto& hostManager = mudlet::self()->getHostManager();
        if (!hostManager.addHost(profileName, QString(), QString(), QString())) {
            return nullptr;
        }
        return hostManager.getHost(profileName);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // Keep the test hermetic: point the config dir resolution at a
        // temporary directory instead of the user's real profiles.
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        previousMessageHandler = qInstallMessageHandler(countParseWarnings);
    }

    void cleanupTestCase()
    {
        qInstallMessageHandler(previousMessageHandler);
        previousMessageHandler = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The finding itself: garbage in profile.ini, and the user is told about it.
    void test_anUnparseableProfileIniIsReported()
    {
        const QString profileName = qsl("ProfileIniParseError-Bad");
        const QString iniPath = iniPathFor(profileName);
        parseWarnings = 0;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl(R"(^Host::profileIni\(\) ERROR - the "profile\.ini" file of profile "%1" \(.*\) could not be parsed)").arg(profileName)));

        Host* pHost = hostWithProfileIni(profileName, unparseableIni());
        QVERIFY2(pHost, "the profile with an unparseable profile.ini was not created");
        QVERIFY2(refusedByQSettings(iniPath, qsl("%1.probe").arg(iniPath)), "QSettings no longer refuses the damaged fixture, so there is nothing here for the profile to report");
        pHost->writeProfileIniData(qsl("CommandLines/UsedIndexes"), qsl("1"));

        QCOMPARE(parseWarnings, 1);
        // ...and what the warning promises: the settings the file held are replaced
        // rather than the write being dropped. Through the file rather than through
        // readProfileIniData(), which answers from the very QSettings that has just
        // cached the write and so would say "1" whatever became of the file.
        mudlet::self()->getHostManager().deleteHost(profileName); // the Host's QSettings writes itself out as it goes
        QFile written(iniPath);
        QVERIFY(written.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString contents = QString::fromUtf8(written.readAll());
        QVERIFY2(contents.contains(qsl("UsedIndexes=1")), qPrintable(qsl("the setting written to a damaged profile.ini did not reach the file, which holds: %1").arg(contents)));
        QVERIFY2(!contents.contains(qsl("truncated mid-write")), qPrintable(qsl("the damage was left in place rather than the file being replaced, which holds: %1").arg(contents)));
    }

    // ...and a file that parses is not slandered, which is the state every
    // profile that has never crashed is in.
    void test_aWellFormedProfileIniIsNotReported()
    {
        const QString profileName = qsl("ProfileIniParseError-Good");
        parseWarnings = 0;

        Host* pHost = hostWithProfileIni(profileName, QByteArray("[CommandLines]\nUsedIndexes=3\n"));
        QVERIFY2(pHost, "the profile with a well-formed profile.ini was not created");

        QCOMPARE(pHost->readProfileIniData(qsl("CommandLines/UsedIndexes")), qsl("3"));
        QCOMPARE(parseWarnings, 0);

        mudlet::self()->getHostManager().deleteHost(profileName);
    }

    // A profile that has never stored anything has no profile.ini at all, and an
    // absent file is not a broken one.
    void test_anAbsentProfileIniIsNotReported()
    {
        const QString profileName = qsl("ProfileIniParseError-Absent");
        parseWarnings = 0;

        Host* pHost = hostWithProfileIni(profileName, QByteArray());
        QVERIFY2(pHost, "the profile with no profile.ini was not created");
        QVERIFY2(!QFile::exists(iniPathFor(profileName)), "the fixture wrote a profile.ini for a profile that is supposed to have none");

        QCOMPARE(pHost->readProfileIniData(qsl("CommandLines/UsedIndexes")), QString());
        QCOMPARE(parseWarnings, 0);

        mudlet::self()->getHostManager().deleteHost(profileName);
    }
};

#include "ProfileIniParseErrorTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileIniParseErrorTest)
