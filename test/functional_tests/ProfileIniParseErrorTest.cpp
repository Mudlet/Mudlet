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
 * Run with: ctest -R ProfileIniParseErrorTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSettings>
#include <QTemporaryDir>

#include <limits>
#include <utility>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "mudlet.h"

#include "GroupedTest.h"

// QTest::ignoreMessage can only assert that a message was printed, so counting the
// diagnostic is the only way to say it was not.
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

    // Damage inside a section: QSettings splits the file into sections as it is
    // constructed, so a broken section header is caught there instead
    static QByteArray unparseableIni() { return QByteArrayLiteral("[CommandLines]\nUsedIndexes=1\nthis line was truncated mid-write\n"); }

    static QString iniPathFor(const QString& profileName) { return MudletApp::getMudletPath(enums::profileDataItemPath, profileName, qsl("profile.ini")); }

    // Probes a copy at a path of its own: QSettings keeps the sections it has parsed
    // per file path and shares them between instances - and beyond the life of the one
    // that parsed them - so probing the fixture would consume the parse the Host needs.
    static bool refusedByQSettings(const QString& path, const QString& copyPath)
    {
        if (!QFile::exists(copyPath) && !QFile::copy(path, copyPath)) {
            return false;
        }
        QSettings probe(copyPath, QSettings::IniFormat);
        probe.allKeys();
        return probe.status() == QSettings::FormatError;
    }

    // Reads a copy: QSettings keeps what it holds for a path, writes that never reached
    // the file included, so a QSettings on the fixture itself would read those back
    static QString writtenValue(const QString& iniPath, const QString& key)
    {
        const QString copyPath = qsl("%1.written").arg(iniPath);
        QFile::remove(copyPath);
        if (!QFile::copy(iniPath, copyPath)) {
            return qsl("(profile.ini could not be copied)");
        }
        const QString value = QSettings(copyPath, QSettings::IniFormat).value(key).toString();
        QFile::remove(copyPath);
        return value;
    }

    static QRegularExpression damageReportFor(const QString& profileName, const QString& handedOut)
    {
        return QRegularExpression(
                qsl(R"(^Host::getCmdLineSettings\(\) WARNING - the command line index ".*" of profile "%1" cannot be used, the new command line gets "%2" instead\.$)").arg(profileName, handedOut));
    }

    // The file has to be in place before the Host first uses it: profile.ini is
    // opened once and kept, so a later write here would not be read again
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

        auto* hostManager = HostManager::self();
        if (!hostManager->addHost(profileName, QString(), QString(), QString())) {
            return nullptr;
        }
        return hostManager->getHost(profileName);
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        // Read through the file rather than readProfileIniData(), which answers from
        // the very QSettings that has just cached the write and so would say "1"
        // whatever became of the file
        HostManager::self()->deleteHost(profileName); // the Host's QSettings writes itself out as it goes
        QFile written(iniPath);
        QVERIFY(written.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString contents = QString::fromUtf8(written.readAll());
        QVERIFY2(contents.contains(qsl("UsedIndexes=1")), qPrintable(qsl("the setting written to a damaged profile.ini did not reach the file, which holds: %1").arg(contents)));
        QVERIFY2(!contents.contains(qsl("truncated mid-write")), qPrintable(qsl("the damage was left in place rather than the file being replaced, which holds: %1").arg(contents)));
    }

    void test_aWellFormedProfileIniIsNotReported()
    {
        const QString profileName = qsl("ProfileIniParseError-Good");
        parseWarnings = 0;

        Host* pHost = hostWithProfileIni(profileName, QByteArray("[CommandLines]\nUsedIndexes=3\n"));
        QVERIFY2(pHost, "the profile with a well-formed profile.ini was not created");

        QCOMPARE(pHost->readProfileIniData(qsl("CommandLines/UsedIndexes")), qsl("3"));
        QCOMPARE(parseWarnings, 0);

        HostManager::self()->deleteHost(profileName);
    }

    // A hand-edited or damaged counter must not name a history file from an
    // overflowed or negative index, nor one a command line already uses
    void test_aCommandLineIndexThatCannotBeIncrementedStartsAgain_data()
    {
        const QString filesInUse = qsl("NameMapping\\chat=command_history_01\nNameMapping\\log=command_history_03\n");
        const QString firstFileInUse = qsl("NameMapping\\chat=command_history_01\n");
        const QString largest = QString::number(std::numeric_limits<int>::max());
        QTest::addColumn<QString>("iniLines");
        QTest::addColumn<bool>("damaged");
        QTest::addColumn<QStringList>("expectedFiles");
        QTest::addColumn<QString>("expectedIndex");
        QTest::newRow("INT_MAX") << qsl("UsedIndexes=%1\n").arg(largest) << false << QStringList{qsl("command_history_01"), qsl("command_history_02")} << qsl("2");
        QTest::newRow("negative") << qsl("UsedIndexes=-5\n") << true << QStringList{qsl("command_history_01"), qsl("command_history_02")} << qsl("2");
        QTest::newRow("INT_MAX, files in use") << qsl("UsedIndexes=%1\n%2").arg(largest, filesInUse) << false << QStringList{qsl("command_history_04"), qsl("command_history_05")} << qsl("5");
        QTest::newRow("negative, files in use") << qsl("UsedIndexes=-5\n%1").arg(filesInUse) << true << QStringList{qsl("command_history_04"), qsl("command_history_05")} << qsl("5");
        QTest::newRow("absent, file in use") << firstFileInUse << true << QStringList{qsl("command_history_02"), qsl("command_history_03")} << qsl("3");
        QTest::newRow("not a number, file in use") << qsl("UsedIndexes=abc\n%1").arg(firstFileInUse) << true << QStringList{qsl("command_history_02"), qsl("command_history_03")} << qsl("3");
        QTest::newRow("zero, file in use") << qsl("UsedIndexes=0\n%1").arg(firstFileInUse) << true << QStringList{qsl("command_history_02"), qsl("command_history_03")} << qsl("3");
        QTest::newRow("next file in use") << qsl("UsedIndexes=2\nNameMapping\\log=command_history_03\n") << false << QStringList{qsl("command_history_04"), qsl("command_history_05")} << qsl("5");
        QTest::newRow("wrapped onto a file in use") << qsl("UsedIndexes=-1\nNameMapping\\a=command_history_%1\nNameMapping\\b=command_history_%2\nNameMapping\\c=command_history_01\n")
                                                               .arg(QString::number(std::numeric_limits<int>::max() - 1), largest)
                                                    << true << QStringList{qsl("command_history_02"), qsl("command_history_03")} << qsl("3");
    }

    void test_aCommandLineIndexThatCannotBeIncrementedStartsAgain()
    {
        QFETCH(QString, iniLines);
        QFETCH(bool, damaged);
        QFETCH(QStringList, expectedFiles);
        QFETCH(QString, expectedIndex);
        const QString profileName = qsl("ProfileIniParseError-Index-%1").arg(QString::fromLatin1(QTest::currentDataTag()).replace(QRegularExpression(qsl("[^A-Za-z_]")), qsl("_")));
        if (damaged) {
            QTest::ignoreMessage(QtWarningMsg, damageReportFor(profileName, expectedFiles.constFirst()));
        } else {
            QTest::failOnWarning(damageReportFor(profileName, qsl(".*")));
        }

        Host* pHost = hostWithProfileIni(profileName, qsl("[CommandLines]\n%1").arg(iniLines).toUtf8());
        QVERIFY2(pHost, "the profile with a hand-edited command line index was not created");
        auto deleteTheHost = qScopeGuard([&profileName]() {
            HostManager::self()->deleteHost(profileName);
        });

        QList<std::pair<QString, QString>> handedOut;
        for (const QString& expectedFile : expectedFiles) {
            const QString name = qsl("qaIndexCommandLine%1").arg(handedOut.size() + 1);
            const auto [fileName, saveCommands] = pHost->getCmdLineSettings(enums::SubCommandLine, name);
            Q_UNUSED(saveCommands)
            QCOMPARE(fileName, expectedFile);
            handedOut.append({name, fileName});
        }

        // readProfileIniData() answers from the Host's own cache, so check the file
        deleteTheHost.dismiss();
        HostManager::self()->deleteHost(profileName);
        QCOMPARE(writtenValue(iniPathFor(profileName), qsl("CommandLines/UsedIndexes")), expectedIndex);
        for (const auto& [name, fileName] : handedOut) {
            QCOMPARE(writtenValue(iniPathFor(profileName), qsl("CommandLines/NameMapping/%1").arg(name)), fileName);
        }
    }

    // A counter rebuilt next to the largest index has nowhere to go but back to
    // the start, and must still give every new command line a history of its own
    void test_commandLinesMadeAtTheLargestIndexGetHistoriesOfTheirOwn()
    {
        const QString profileName = qsl("ProfileIniParseError-IndexBoundary");
        const QString nearLargest = qsl("command_history_%1").arg(std::numeric_limits<int>::max() - 1);
        QTest::ignoreMessage(QtWarningMsg, damageReportFor(profileName, qsl("command_history_%1").arg(std::numeric_limits<int>::max())));
        Host* pHost = hostWithProfileIni(profileName, qsl("[CommandLines]\nUsedIndexes=-1\nNameMapping\\chat=%1\n").arg(nearLargest).toUtf8());
        QVERIFY2(pHost, "the profile with a history file next to the largest index was not created");
        const auto deleteTheHost = qScopeGuard([&profileName]() {
            HostManager::self()->deleteHost(profileName);
        });

        const QList<std::pair<QString, QString>> expected{{qsl("qaBoundaryOne"), qsl("command_history_%1").arg(std::numeric_limits<int>::max())},
                                                          {qsl("qaBoundaryTwo"), qsl("command_history_01")},
                                                          {qsl("qaBoundaryThree"), qsl("command_history_02")}};
        for (const auto& [name, expectedFile] : expected) {
            const auto [fileName, saveCommands] = pHost->getCmdLineSettings(enums::SubCommandLine, name);
            Q_UNUSED(saveCommands)
            QCOMPARE(fileName, expectedFile);
        }
    }

    // A history file can outlive the profile.ini entry naming it, and handing it out
    // again would give the new command line another one's history
    void test_aHistoryFileLeftInTheProfileIsNotHandedOutAgain_data()
    {
        QTest::addColumn<QString>("iniLines");
        QTest::addColumn<QString>("leftOverFile");
        QTest::addColumn<QString>("expectedFile");
        QTest::addColumn<QString>("expectedIndex");
        QTest::newRow("no counter") << QString() << qsl("command_history_01") << qsl("command_history_02") << qsl("2");
        QTest::newRow("counter just below it") << qsl("UsedIndexes=1\n") << qsl("command_history_02") << qsl("command_history_03") << qsl("3");
    }

    void test_aHistoryFileLeftInTheProfileIsNotHandedOutAgain()
    {
        QFETCH(QString, iniLines);
        QFETCH(QString, leftOverFile);
        QFETCH(QString, expectedFile);
        QFETCH(QString, expectedIndex);
        const QString profileName = qsl("ProfileIniParseError-IndexLeftOver-%1").arg(QString::fromLatin1(QTest::currentDataTag()).replace(QRegularExpression(qsl("[^A-Za-z_]")), qsl("_")));
        QTest::failOnWarning(damageReportFor(profileName, qsl(".*")));
        const QString profileDir = QFileInfo(iniPathFor(profileName)).absolutePath();
        QVERIFY(QDir().mkpath(profileDir));
        QFile leftOver(qsl("%1/%2").arg(profileDir, leftOverFile));
        QVERIFY(leftOver.open(QIODevice::WriteOnly));
        QVERIFY(leftOver.write("a command from a command line profile.ini has forgotten\n") > 0);
        leftOver.close();

        Host* pHost = hostWithProfileIni(profileName, qsl("[CommandLines]\n%1").arg(iniLines).toUtf8());
        QVERIFY2(pHost, "the profile with a history file left in it was not created");
        auto deleteTheHost = qScopeGuard([&profileName]() {
            HostManager::self()->deleteHost(profileName);
        });

        const auto [fileName, saveCommands] = pHost->getCmdLineSettings(enums::SubCommandLine, qsl("qaLeftOverCommandLine"));
        Q_UNUSED(saveCommands)
        QCOMPARE(fileName, expectedFile);

        deleteTheHost.dismiss();
        HostManager::self()->deleteHost(profileName);
        QCOMPARE(writtenValue(iniPathFor(profileName), qsl("CommandLines/UsedIndexes")), expectedIndex);
    }

    void test_anAbsentProfileIniIsNotReported()
    {
        const QString profileName = qsl("ProfileIniParseError-Absent");
        parseWarnings = 0;

        Host* pHost = hostWithProfileIni(profileName, QByteArray());
        QVERIFY2(pHost, "the profile with no profile.ini was not created");
        QVERIFY2(!QFile::exists(iniPathFor(profileName)), "the fixture wrote a profile.ini for a profile that is supposed to have none");

        QCOMPARE(pHost->readProfileIniData(qsl("CommandLines/UsedIndexes")), QString());
        QCOMPARE(parseWarnings, 0);

        HostManager::self()->deleteHost(profileName);
    }
};

#include "ProfileIniParseErrorTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileIniParseErrorTest)
