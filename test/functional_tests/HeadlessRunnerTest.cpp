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
 * mudlet-headless is run as a child, as HeadlessVersionTest runs the GUI, since
 * its command line, its standard streams and its exit code are what a script
 * that drives it sees. Each run gets a config root of its own and no display.
 *
 * The child is waited on with QTRY_* rather than waitForFinished(), as the
 * game it connects to is a TelnetServerStub served by this process's event loop.
 *
 * Run with: ctest -R HeadlessRunnerTest -V
 */

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "TelnetServerStub.h"

#if defined(Q_OS_UNIX)
#include <csignal>
#include <poll.h>
#include <unistd.h>
#endif

#include "GroupedTest.h"

#ifndef MUDLET_HEADLESS_BINARY
#error "HeadlessRunnerTest needs MUDLET_HEADLESS_BINARY - see test/functional_tests/CMakeLists.txt"
#endif

class HeadlessRunnerTest : public QObject
{
    Q_OBJECT

private:
    static constexpr int scmFinishTimeoutMs = 30000;

    QTemporaryDir mSandbox;

    static QString headlessBinary() { return QString::fromUtf8(MUDLET_HEADLESS_BINARY); }

    // A portable.txt beside the binary, or in $HOME/.config/mudlet, outranks XDG_CONFIG_HOME
    // (MudletApp::resolveConfigRoot()), so the child would run the real install's profiles
    static bool portableMarkerWouldWin()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QFileInfo(headlessBinary()).absolutePath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    QString profileFolder(const QString& name) const { return qsl("%1/config/mudlet/profiles/%2").arg(mSandbox.path(), name); }

    static QProcessEnvironment childEnvironment(const QString& root)
    {
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        // So the runner's own default of offscreen is what is tested, not ctest's
        environment.remove(qsl("QT_QPA_PLATFORM"));
        environment.remove(qsl("DISPLAY"));
        environment.remove(qsl("WAYLAND_DISPLAY"));
        // The texts checked for below are translated, and only an interfaceLanguage in Mudlet.ini says otherwise
        environment.insert(qsl("LC_ALL"), qsl("C"));
        environment.insert(qsl("LANG"), qsl("C"));
        environment.insert(qsl("XDG_CONFIG_HOME"), qsl("%1/config").arg(root));
        environment.insert(qsl("XDG_DATA_HOME"), qsl("%1/data").arg(root));
        environment.insert(qsl("XDG_CACHE_HOME"), qsl("%1/cache").arg(root));
        environment.insert(qsl("MUDLET_TEST_MODE"), qsl("1"));
        return environment;
    }

    bool writeProfile(const QString& name, const QString& script) const
    {
        const QString saveFolder = qsl("%1/current").arg(profileFolder(name));
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessRunner</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped())
                               .toUtf8());
        return true;
    }

    // As MudletApp::writeProfileData() writes them, which the child's Host reads for its game
    bool writeProfileData(const QString& name, const QString& item, const QString& value) const
    {
        QFile file(qsl("%1/%2").arg(profileFolder(name), item));
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_12);
        out << value;
        return out.status() == QDataStream::Ok;
    }

    qsizetype saveCount(const QString& name) const { return QDir(qsl("%1/current").arg(profileFolder(name))).entryList(QStringList{qsl("*.xml")}, QDir::Files).size(); }

    static QString readFile(const QString& path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    void startChild(QProcess& child, const QStringList& arguments, const QString& root = QString(), const QString& platform = QString()) const
    {
        QProcessEnvironment environment = childEnvironment(root.isEmpty() ? mSandbox.path() : root);
        if (!platform.isEmpty()) {
            environment.insert(qsl("QT_QPA_PLATFORM"), platform);
        }
        child.setProcessEnvironment(environment);
        child.setProgram(headlessBinary());
        child.setArguments(arguments);
        child.start();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerWouldWin()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mSandbox.isValid());
        // Creating mudlet/profiles is what makes XDG_CONFIG_HOME outrank the legacy ~/.config/mudlet
        QVERIFY(QDir().mkpath(qsl("%1/config/mudlet/profiles").arg(mSandbox.path())));
    }

    void test_exitCodes_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::addColumn<int>("exitCode");
        QTest::addColumn<QString>("standardOutputHas");
        QTest::addColumn<QString>("standardErrorHas");

        QTest::newRow("--version") << QStringList{qsl("--version")} << 0 << qsl("GPLv3") << QString();
        QTest::newRow("--help") << QStringList{qsl("--help")} << 0 << qsl("--profile") << QString();
        QTest::newRow("unknown option") << QStringList{qsl("--no-such-option")} << 2 << QString() << qsl("--help");
        QTest::newRow("no profile") << QStringList() << 2 << QString() << qsl("-p");
        QTest::newRow("stray argument") << QStringList{qsl("-p"), qsl("a"), qsl("b")} << 2 << QString() << qsl("'b'");
        QTest::newRow("unknown profile") << QStringList{qsl("-p"), qsl("No Such Runner Profile")} << 1 << QString() << qsl("No Such Runner Profile");
    }

    void test_exitCodes()
    {
        QFETCH(QStringList, arguments);
        QFETCH(int, exitCode);
        QFETCH(QString, standardOutputHas);
        QFETCH(QString, standardErrorHas);

        QProcess child;
        startChild(child, arguments);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardOutput = QString::fromUtf8(child.readAllStandardOutput());
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == exitCode, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardOutput.contains(standardOutputHas), qPrintable(standardOutput));
        QVERIFY2(standardError.contains(standardErrorHas), qPrintable(standardError));
    }

#if defined(Q_OS_UNIX)
    // Often why the profile is not found, so it is reported before the lookup. Only on Unix does
    // QDir::homePath() follow HOME
    void test_unknownProfileReportsShadowedProfiles()
    {
        const QString root = qsl("%1/shadowing").arg(mSandbox.path());
        QVERIFY(QDir().mkpath(qsl("%1/config/mudlet/profiles/Shown").arg(root)));
        QVERIFY(QDir().mkpath(qsl("%1/home/.config/mudlet/profiles/Shadowed").arg(root)));
        QProcess child;
        QProcessEnvironment environment = childEnvironment(root);
        environment.insert(qsl("HOME"), qsl("%1/home").arg(root));
        child.setProcessEnvironment(environment);
        child.start(headlessBinary(), QStringList{qsl("-p"), qsl("Shadowed")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitCode(), 1);
        QVERIFY2(standardError.contains(qsl("will not be listed")), qPrintable(standardError));
        QVERIFY2(standardError.contains(qsl("There is no profile named \"Shadowed\"")), qPrintable(standardError));
    }
#endif

    // The game's lines on stdout, plain, and closeProfile() from a trigger on them ending the run cleanly
    void test_gameLinesReachStandardOutput()
    {
        const QString name = qsl("Headless Runner Game");
        QVERIFY(writeProfile(name, qsl(R"lua(tempTrigger("headless runner test", [[closeProfile()]]))lua")));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("Welcome to the headless runner test."));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(writeProfileData(name, qsl("url"), qsl("127.0.0.1")));
        QVERIFY(writeProfileData(name, qsl("port"), QString::number(server.serverPort())));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        // Windows writes standard output in text mode, so its lines end in \r\n
        const QStringList lines = QString::fromUtf8(child.readAllStandardOutput()).remove(QLatin1Char('\r')).split(QLatin1Char('\n'));
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QVERIFY2(lines.contains(qsl("Welcome to the headless runner test.")), qPrintable(lines.join(QLatin1Char('\n'))));
    }

    void test_closeMudletSavesAndExits_data()
    {
        QTest::addColumn<QString>("platform");
        QTest::newRow("platform chosen by the runner") << QString();
        QTest::newRow("platform chosen by the user") << qsl("minimal");
    }

    void test_closeMudletSavesAndExits()
    {
        QFETCH(QString, platform);
        const QString name = platform.isEmpty() ? qsl("Headless Runner Close") : qsl("Headless Runner Close Platform");
        const QString recordPath = qsl("%1/%2.record").arg(mSandbox.path(), name);
        const QString platformPath = qsl("%1/%2.platform").arg(mSandbox.path(), name);
        // Opened binary so Windows writes no \r. The platform the runner picks is not handed down to what a
        // script starts, which would come up with no window.
        QVERIFY(writeProfile(name,
                             qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local file = io.open([[%1]], "ab")
  file:write("sysExitEvent\n")
  file:close()
end)
local platform = io.open([[%2]], "wb")
platform:write(tostring(os.getenv("QT_QPA_PLATFORM")))
platform:close()
closeMudlet())lua")
                                     .arg(recordPath, platformPath)));

        QProcess child;
        startChild(child, {qsl("--profile"), name}, QString(), platform);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
        // One the user chose is theirs to hand down
        QCOMPARE(readFile(platformPath), platform.isEmpty() ? qsl("nil") : platform);
    }

    void test_versionIsTranslated()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/config/mudlet/profiles").arg(root.path())));
        QFile ini(qsl("%1/config/mudlet/Mudlet.ini").arg(root.path()));
        QVERIFY(ini.open(QIODevice::WriteOnly | QIODevice::Text));
        ini.write("[General]\ninterfaceLanguage=de_DE\n");
        ini.close();

        QProcess child;
        startChild(child, {qsl("--version")}, root.path());
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardOutput = QString::fromUtf8(child.readAllStandardOutput());
        QCOMPARE(child.exitCode(), 0);
        QVERIFY2(standardOutput.contains(qsl("Qt-Bibliotheken")), qPrintable(standardOutput));
    }

#if defined(Q_OS_UNIX)
    // A reader that goes away, as `mudlet-headless -p name | head -1` does, ends the run as closeMudlet() does.
    // A pipe of the test's own, as QProcess keeps its end of the child's standard output open until the end.
    void test_closedOutputSavesAndExits()
    {
        const QString name = qsl("Headless Runner Reader");
        const QString recordPath = qsl("%1/reader.record").arg(mSandbox.path());
        QVERIFY(writeProfile(name,
                             qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local file = io.open([[%1]], "ab")
  file:write("sysExitEvent\n")
  file:close()
end)
tempTimer(0.1, function() feedTriggers("headless tick\n") end, true))lua")
                                     .arg(recordPath)));

        int output[2];
        QVERIFY(::pipe(output) == 0);
        QProcess child;
        child.setChildProcessModifier([output]() {
            ::dup2(output[1], STDOUT_FILENO);
            ::close(output[0]);
            ::close(output[1]);
        });
        startChild(child, {qsl("-p"), name});
        ::close(output[1]);
        QByteArray firstLine;
        char character = 0;
        pollfd readable{output[0], POLLIN, 0};
        while (::poll(&readable, 1, scmFinishTimeoutMs) == 1 && ::read(output[0], &character, 1) == 1 && character != '\n') {
            firstLine.append(character);
        }
        ::close(output[0]);
        QCOMPARE(firstLine, QByteArray("headless tick"));

        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // As above, with the reader gone before the load is over, and a load that runs the event loop (as one
    // trying a map file that will not read does) before the runner's own loop starts: the timer's line
    // comes before sysLoadEvent's
    void test_outputClosedDuringTheLoadSavesAndExits()
    {
        const QString name = qsl("Headless Runner Early Reader");
        const QString recordPath = qsl("%1/early.record").arg(mSandbox.path());
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local function record(text)
  local file = io.open([[%1]], "ab")
  file:write(text .. "\n")
  file:close()
end
registerAnonymousEventHandler("sysExitEvent", function() record("sysExitEvent") end)
registerAnonymousEventHandler("sysLoadEvent", function() record("sysLoadEvent") end)
tempTimer(0, function() record("timer") end)
feedTriggers("headless during the load\n"))lua")
                                     .arg(recordPath)));
        const QString mapFolder = qsl("%1/map").arg(profileFolder(name));
        QVERIFY(QDir().mkpath(mapFolder));
        QFile mapFile(qsl("%1/2020-01-01#00-00-00map.dat").arg(mapFolder));
        QVERIFY(mapFile.open(QIODevice::WriteOnly));
        mapFile.write(QByteArray(64, 'x'));
        mapFile.close();

        int output[2];
        QVERIFY(::pipe(output) == 0);
        ::close(output[0]);
        QProcess child;
        child.setChildProcessModifier([output]() {
            ::dup2(output[1], STDOUT_FILENO);
            ::close(output[1]);
        });
        startChild(child, {qsl("-p"), name});
        ::close(output[1]);

        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("timer\nsysLoadEvent\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }
#endif

    void test_unreadableSaveExitsOne()
    {
        const QString name = qsl("Headless Runner Corrupt");
        QVERIFY(QDir().mkpath(qsl("%1/current").arg(profileFolder(name))));
        QFile saveFile(qsl("%1/current/2020-01-01#00-00-00.xml").arg(profileFolder(name)));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        saveFile.write("<?xml version=\"1.0\"?><MudletPackage version=\"1.001\"><HostPackage><Host");
        saveFile.close();

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 1);
        QCOMPARE(saveCount(name), 1);
    }

#if defined(Q_OS_LINUX)
    // Unlike a reader that goes away, a full disk is an error, which is said on standard error; the profile
    // still closes and saves
    void test_unwritableOutputExitsThree()
    {
        if (!QFileInfo::exists(qsl("/dev/full"))) {
            QSKIP("There is no /dev/full here");
        }
        const QString name = qsl("Headless Runner Full");
        const QString recordPath = qsl("%1/full.record").arg(mSandbox.path());
        QVERIFY(writeProfile(name,
                             qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local file = io.open([[%1]], "ab")
  file:write("sysExitEvent\n")
  file:close()
end)
tempTimer(0.1, function() feedTriggers("headless tick\n") end, true))lua")
                                     .arg(recordPath)));

        for (const QStringList& arguments : {QStringList{qsl("--version")}, QStringList{qsl("-p"), name}}) {
            QProcess child;
            child.setStandardOutputFile(qsl("/dev/full"));
            startChild(child, arguments);
            QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
            const QString standardError = QString::fromUtf8(child.readAllStandardError());
            QCOMPARE(child.exitStatus(), QProcess::NormalExit);
            QVERIFY2(child.exitCode() == 3, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
            QVERIFY2(standardError.contains(qsl("cannot write to standard output: No space left on device")), qPrintable(standardError));
        }
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // The runner survives a reader that goes away without leaving SIGPIPE ignored in what scripts start,
    // where `producer | head` would then never end
    void test_scriptsStartProgramsWithSigpipeDefault()
    {
        const QString name = qsl("Headless Runner Signals");
        const QString recordPath = qsl("%1/signals.record").arg(mSandbox.path());
        QVERIFY(writeProfile(name,
                             qsl(R"lua(os.execute([[grep SigIgn /proc/self/status > '%1']])
closeMudlet())lua")
                                     .arg(recordPath)));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        QCOMPARE(child.exitCode(), 0);
        const QString record = readFile(recordPath).trimmed();
        QVERIFY2(record.startsWith(qsl("SigIgn:")), qPrintable(record));
        bool parsed = false;
        const qulonglong ignored = record.mid(7).trimmed().toULongLong(&parsed, 16);
        QVERIFY(parsed);
        QVERIFY2(!(ignored & (1ULL << (SIGPIPE - 1))), qPrintable(record));
    }

    // What a phone app built on mudlet_core can count on: nothing the core runs needs Qt Widgets
    void test_noQtWidgets()
    {
        QProcess ldd;
        ldd.start(qsl("ldd"), {headlessBinary()});
        if (!ldd.waitForStarted()) {
            QSKIP("ldd is not available here");
        }
        QVERIFY(ldd.waitForFinished());
        const QString libraries = QString::fromUtf8(ldd.readAllStandardOutput());
        if (!libraries.contains(qsl("Qt6Core"))) {
            QSKIP("ldd lists no Qt libraries, as for a static Qt, so it cannot show whether Qt Widgets is linked in");
        }
        QVERIFY2(!libraries.contains(qsl("Qt6Widgets")), qPrintable(libraries));
    }
#endif
};

#include "HeadlessRunnerTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessRunnerTest)
