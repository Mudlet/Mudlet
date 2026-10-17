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
#include <QThread>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "TelnetServerStub.h"

#if defined(Q_OS_UNIX)
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "GroupedTest.h"

#ifndef MUDLET_HEADLESS_BINARY
#error "HeadlessRunnerTest needs MUDLET_HEADLESS_BINARY - see test/functional_tests/CMakeLists.txt"
#endif

// A game that keeps what it is sent, and can hang up
class RecordingGame
{
public:
    bool start()
    {
        QObject::connect(&mServer, &QTcpServer::newConnection, [this]() {
            mpClient = mServer.nextPendingConnection();
            QObject::connect(mpClient, &QTcpSocket::readyRead, [this]() {
                mReceived += mpClient->readAll();
            });
            mpClient->write("Welcome to the recording game.\n");
        });
        return mServer.listen(QHostAddress::LocalHost);
    }
    quint16 port() const { return mServer.serverPort(); }
    bool connected() const { return mpClient && mpClient->state() == QAbstractSocket::ConnectedState; }
    QByteArray received() const { return mReceived; }
    void hangUp() { mpClient->disconnectFromHost(); }

private:
    QTcpServer mServer;
    QPointer<QTcpSocket> mpClient;
    QByteArray mReceived;
};

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

    // A profile whose sysExitEvent, and anything else the script records, lands in a file of its own
    bool writeRecordingProfile(const QString& name, const QString& script, QString& recordPath) const
    {
        recordPath = qsl("%1/%2.record").arg(mSandbox.path(), name);
        // Opened binary so Windows writes no \r
        return writeProfile(name,
                            qsl(R"lua(function record(text)
  local file = io.open([[%1]], "ab")
  file:write(text .. "\n")
  file:close()
end
registerAnonymousEventHandler("sysExitEvent", function() record("sysExitEvent") end)
%2)lua")
                                    .arg(recordPath, script));
    }

    bool connectTo(const QString& name, const RecordingGame& game) const
    {
        return writeProfileData(name, qsl("url"), qsl("127.0.0.1")) && writeProfileData(name, qsl("port"), QString::number(game.port()));
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

#if defined(Q_OS_UNIX)
    void test_aStopSignalEndsAWaitOnAStalledReader_data()
    {
        QTest::addColumn<bool>("errorsToo");
        QTest::newRow("standard output") << false;
        // As `mudlet-headless -p name 2>&1 | less`: what the close says goes to the stalled pipe too
        QTest::newRow("both streams") << true;
    }

    // A reader that holds the pipe but stops reading, as `mudlet-headless -p name | less` left at its prompt,
    // stalls the run as the GUI's --mirror would; a stop signal still ends it with a save
    void test_aStopSignalEndsAWaitOnAStalledReader()
    {
        QFETCH(bool, errorsToo);
        const QString name = errorsToo ? qsl("Headless Runner Stalled Both") : qsl("Headless Runner Stalled Reader");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempTimer(0.01, function() feedTriggers(string.rep("x", 1000) .. "\n") end, true))lua"), recordPath));

        int output[2];
        QVERIFY(::pipe(output) == 0);
        QProcess child;
        child.setChildProcessModifier([output, errorsToo]() {
            ::dup2(output[1], STDOUT_FILENO);
            if (errorsToo) {
                ::dup2(output[1], STDERR_FILENO);
            }
            ::close(output[0]);
            ::close(output[1]);
        });
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        ::close(output[1]);
        // Full enough that the writer is waiting on it; a pipe holds at least 16KB everywhere
        int buffered = 0;
        QTRY_VERIFY_WITH_TIMEOUT(::ioctl(output[0], FIONREAD, &buffered) == 0 && buffered >= 16384, scmFinishTimeoutMs);
        QTest::qWait(500);
        QVERIFY(::kill(child.processId(), SIGTERM) == 0);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        ::close(output[0]);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 128 + SIGTERM, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(!standardError.contains(qsl("cannot write to standard output")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }
#endif

#if defined(Q_OS_LINUX)
    // Lines are read only as fast as the aliases and the game take them, so a file piped in does not sit in
    // memory whole; the count is what Linux reports, hence only there
    void test_inputReadAheadIsBounded()
    {
        const QString name = qsl("Headless Runner Flood");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(looks = 0 tempAlias("^look$", [[looks = looks + 1]]))lua"), recordPath));
        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        const auto residentKb = [&child]() {
            QFile status(qsl("/proc/%1/status").arg(child.processId()));
            if (!status.open(QIODevice::ReadOnly)) {
                return 0;
            }
            for (const QByteArray& line : status.readAll().split('\n')) {
                if (line.startsWith("VmRSS:")) {
                    return line.mid(6).trimmed().split(' ').constFirst().toInt();
                }
            }
            return 0;
        };
        QTest::qWait(2000);
        const int before = residentKb();
        QVERIFY(before > 0);
        // Two million lines, which held whole would take hundreds of megabytes
        const QByteArray lines = QByteArray("look\n").repeated(2000000);
        child.write(lines);
        QTest::qWait(4000);
        const int during = residentKb();
        QVERIFY2(during < before + 100 * 1024, qPrintable(qsl("resident set grew from %1 kB to %2 kB").arg(before).arg(during)));
        QVERIFY(::kill(child.processId(), SIGTERM) == 0);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        QCOMPARE(child.exitCode(), 128 + SIGTERM);
    }
#endif

    // Every line past the bound is still typed, in order, as the reader goes on once the main thread has taken some
    void test_linesPastTheBoundAreTypedInOrder()
    {
        const QString name = qsl("Headless Runner Numbered");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(seen = {}
tempAlias("^n (\\d+)$", [[seen[#seen + 1] = tonumber(matches[2])]])
registerAnonymousEventHandler("sysExitEvent", function()
  local inOrder = true
  for i, n in ipairs(seen) do inOrder = inOrder and n == i end
  record(#seen .. (inOrder and " in order" or " out of order"))
end))lua"),
                                      recordPath));
        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        QByteArray lines;
        for (int i = 1; i <= 3000; ++i) {
            lines += "n " + QByteArray::number(i) + "\n";
        }
        child.write(lines);
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QVERIFY2(readFile(recordPath).contains(qsl("3000 in order")), qPrintable(readFile(recordPath)));
    }

    // Each line is typed on the command line, aliases and all, and the end of the input closes the run
    void test_standardInputIsTypedUntilItEnds()
    {
        const QString name = qsl("Headless Runner Input");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^greet$", [[send("sent by an alias")]]))lua"), recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_VERIFY_WITH_TIMEOUT(game.connected(), scmFinishTimeoutMs);
        child.write("typed line\ngreet\n");
        QTRY_VERIFY_WITH_TIMEOUT(game.received().contains("typed line") && game.received().contains("sent by an alias"), scmFinishTimeoutMs);
        QVERIFY(!game.received().contains("greet"));
        child.closeWriteChannel();

        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardOutput = QString::fromUtf8(child.readAllStandardOutput());
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
        // The commands echoed are the profile's own, so they are diagnostics, and the game's lines are not
        QVERIFY2(standardOutput.contains(qsl("Welcome to the recording game.")), qPrintable(standardOutput));
        QVERIFY2(!standardOutput.contains(qsl("typed line")), qPrintable(standardOutput));
        QVERIFY2(standardError.contains(qsl("typed line")), qPrintable(standardError));
    }

    // As `printf note | mudlet-headless`, whose last line has no line feed
    void test_aLastLineWithNoLineFeedIsTyped()
    {
        const QString name = qsl("Headless Runner Unended Line");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^note$", [[record("noted")]]))lua"), recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("note");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("noted\nsysExitEvent\n"));
    }

    // Lines read once a close has begun are refused, as the GUI's closing window takes no more typing, so none
    // reaches the game after sysExitEvent
    void test_inputEndsWhenACloseStarts()
    {
        const QString name = qsl("Headless Runner Input Close");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(tempAlias("^bye$", [[closeProfile()]])
registerAnonymousEventHandler("sysExitEvent", function() send("exit event", false) end))lua"),
                                      recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_VERIFY_WITH_TIMEOUT(game.connected(), scmFinishTimeoutMs);
        QByteArray lines("bye\n");
        for (int i = 0; i < 3000; ++i) {
            lines += "after " + QByteArray::number(i) + "\n";
        }
        child.write(lines);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QVERIFY2(standardError.count(qsl("The profile has closed, so it takes no more commands.")) == 1, qPrintable(standardError.left(2000)));
        const QByteArray received = game.received();
        QVERIFY2(received.contains("exit event"), received.constData());
        QVERIFY2(!received.mid(received.indexOf("exit event")).contains("after"), received.right(500).constData());
    }

    void test_noStdinOutlivesItsInput_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::addColumn<QString>("record");
        QTest::newRow("reading standard input") << QStringList() << qsl("sysExitEvent\n");
        QTest::newRow("--no-stdin") << QStringList{qsl("--no-stdin")} << qsl("timer\nsysExitEvent\n");
    }

    void test_noStdinOutlivesItsInput()
    {
        QFETCH(QStringList, arguments);
        QFETCH(QString, record);
        const QString name = qsl("Headless Runner No Input %1").arg(arguments.size());
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempTimer(1, function() record("timer") closeMudlet() end))lua"), recordPath));

        QProcess child;
        child.setStandardInputFile(QProcess::nullDevice());
        startChild(child, QStringList{qsl("-p"), name} + arguments);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), record);
    }

    // Every -e runs, in order, though one fails; the failure is on standard error and in the exit code, as is
    // what the GUI shows only on the profile's screen
    void test_execRunsEachChunk()
    {
        const QString name = qsl("Headless Runner Exec");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));

        QProcess child;
        startChild(child,
                   {qsl("-p"),
                    name,
                    qsl("-e"),
                    qsl(R"lua(record("first") echo("echoed by exec\n"))lua"),
                    qsl("--exec"),
                    // Split, so the chunk name the error quotes does not hold the message
                    qsl(R"lua(error("exec " .. "failure"))lua"),
                    qsl("-e"),
                    qsl(R"lua(record("third") closeMudlet())lua")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardOutput = QString::fromUtf8(child.readAllStandardOutput());
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("first\nthird\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
        QVERIFY2(standardError.count(qsl("exec failure")) == 1, qPrintable(standardError));
        QVERIFY2(standardError.contains(qsl("echoed by exec\n")), qPrintable(standardError));
        QVERIFY2(!standardOutput.contains(qsl("echoed by exec")), qPrintable(standardOutput));
    }

    void test_exitOnDisconnect_data()
    {
        QTest::addColumn<bool>("exitOnDisconnect");
        QTest::newRow("--exit-on-disconnect") << true;
        QTest::newRow("staying on") << false;
    }

    void test_exitOnDisconnect()
    {
        QFETCH(bool, exitOnDisconnect);
        const QString name = exitOnDisconnect ? qsl("Headless Runner Disconnect") : qsl("Headless Runner Stay");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, exitOnDisconnect ? QStringList{qsl("-p"), name, qsl("--exit-on-disconnect")} : QStringList{qsl("-p"), name});
        QTRY_VERIFY_WITH_TIMEOUT(game.connected(), scmFinishTimeoutMs);
        game.hangUp();
        if (!exitOnDisconnect) {
            QTest::qWait(1000);
            QCOMPARE(child.state(), QProcess::Running);
            child.closeWriteChannel();
        }
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == (exitOnDisconnect ? 4 : 0), qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    void test_aConnectionThatCannotBeMadeExitsFour()
    {
        const QString name = qsl("Headless Runner Refused");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        quint16 port = 0;
        {
            QTcpServer closed;
            QVERIFY(closed.listen(QHostAddress::LocalHost));
            port = closed.serverPort();
        }
        QVERIFY(writeProfileData(name, qsl("url"), qsl("127.0.0.1")) && writeProfileData(name, qsl("port"), QString::number(port)));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--exit-on-disconnect")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 4, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }

    // A close whose save fails says so, and the run fails with it
    void test_unsavableCloseExitsOne()
    {
        const QString name = qsl("Headless Runner Unsavable");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local current = getMudletHomeDir() .. "/current"
  os.rename(current, current .. ".moved")
  local file = io.open(current, "wb")
  file:write("not a folder")
  file:close()
  echo("half a line")
end)
closeMudlet())lua"),
                                      recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("The profile could not be saved.")), qPrintable(standardError));
        // What Qt logs, and the runner's own messages, start a line of their own
        QVERIFY2(standardError.contains(qsl("half a line\n")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }

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

    void test_stopSignalSavesAndExits_data()
    {
        QTest::addColumn<int>("signalNumber");
        QTest::newRow("SIGINT") << SIGINT;
        QTest::newRow("SIGTERM") << SIGTERM;
        QTest::newRow("SIGHUP") << SIGHUP;
    }

    void test_stopSignalSavesAndExits()
    {
        QFETCH(int, signalNumber);
        const QString name = qsl("Headless Runner Signal %1").arg(signalNumber);
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempTimer(0, function() feedTriggers("headless ready\n") end))lua"), recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QByteArray standardOutput;
        QTRY_VERIFY_WITH_TIMEOUT((standardOutput += child.readAllStandardOutput()).contains("headless ready"), scmFinishTimeoutMs);
        QVERIFY(::kill(static_cast<pid_t>(child.processId()), signalNumber) == 0);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 128 + signalNumber, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // A second one, of any kind, ends a run whose main thread never gets back to the event loop
    void test_secondStopSignalEndsABusyRun()
    {
        const QString name = qsl("Headless Runner Busy");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--no-stdin"), qsl("-e"), qsl(R"lua(record("busy") while true do end)lua")});
        QTRY_COMPARE_WITH_TIMEOUT(readFile(recordPath), qsl("busy\n"), scmFinishTimeoutMs);
        const auto pid = static_cast<pid_t>(child.processId());
        QVERIFY(::kill(pid, SIGTERM) == 0);
        // Apart, so the first is handled before the second comes
        QTest::qWait(500);
        QCOMPARE(child.state(), QProcess::Running);
        QVERIFY(::kill(pid, SIGINT) == 0);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 10000);
        QCOMPARE(child.exitStatus(), QProcess::CrashExit);
    }

    // A closed terminal can send its hangup twice, and the second leaves the close and its save to finish
    void test_repeatedHangupStillSaves()
    {
        const QString name = qsl("Headless Runner Hangup");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local started = os.clock()
  while os.clock() - started < 1 do end
  record("slow exit handler done")
end)
tempTimer(0, function() feedTriggers("headless ready\n") end))lua"),
                                      recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        QByteArray standardOutput;
        QTRY_VERIFY_WITH_TIMEOUT((standardOutput += child.readAllStandardOutput()).contains("headless ready"), scmFinishTimeoutMs);
        const auto pid = static_cast<pid_t>(child.processId());
        QVERIFY(::kill(pid, SIGHUP) == 0);
        QTest::qWait(200);
        QVERIFY(::kill(pid, SIGHUP) == 0);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 128 + SIGHUP);
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\nslow exit handler done\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // A read that fails is not the end of the input, so the run says so and fails, though it closes as at the end
    void test_unreadableInputExitsOne()
    {
        const QString name = qsl("Headless Runner Unreadable Input");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));

        QProcess child;
        // Opens, and every read of it fails with EISDIR
        child.setStandardInputFile(mSandbox.path());
        startChild(child, {qsl("-p"), name});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("cannot read standard input")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // A line cut short by a read that fails is not typed: only the end of the input ends a line. A terminal
    // whose other end has gone fails its reads.
    void test_aFailedReadDropsTheUnendedLine()
    {
        const QString name = qsl("Headless Runner Cut Line");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^note$", [[record("noted")]]))lua"), recordPath));
        const int terminal = ::posix_openpt(O_RDWR | O_NOCTTY);
        QVERIFY(terminal >= 0);
        QVERIFY(::grantpt(terminal) == 0 && ::unlockpt(terminal) == 0);
        const int input = ::open(::ptsname(terminal), O_RDWR | O_NOCTTY);
        QVERIFY(input >= 0);
        termios raw{};
        QVERIFY(::tcgetattr(input, &raw) == 0);
        ::cfmakeraw(&raw);
        QVERIFY(::tcsetattr(input, TCSANOW, &raw) == 0);
        // Or the child would keep the other end open itself
        QVERIFY(::fcntl(terminal, F_SETFD, FD_CLOEXEC) == 0 && ::fcntl(input, F_SETFD, FD_CLOEXEC) == 0);

        QProcess child;
        child.setChildProcessModifier([input]() {
            ::dup2(input, STDIN_FILENO);
        });
        startChild(child, {qsl("-p"), name});
        ::close(input);
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        QCOMPARE(::write(terminal, "note", 4), 4);
        QTest::qWait(500);
        ::close(terminal);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("cannot read standard input")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }

    // Standard input and output a parent left non-blocking are waited on, not given up on
    void test_nonBlockingStreamsAreWaitedOn()
    {
        const QString name = qsl("Headless Runner Non-blocking");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(tempAlias("^note$", [[record("noted")]])
tempTimer(0, function()
  for i = 1, 20000 do
    feedTriggers("flood line " .. i .. " to fill the pipe to the reader\n")
  end
end))lua"),
                                      recordPath));

        QProcess child;
        child.setChildProcessModifier([]() {
            for (const int descriptor : {STDIN_FILENO, STDOUT_FILENO}) {
                ::fcntl(descriptor, F_SETFL, ::fcntl(descriptor, F_GETFL) | O_NONBLOCK);
            }
        });
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        // Not reading, so the flood fills the pipe while standard input has nothing yet
        QThread::msleep(2000);
        child.write("note\n");
        child.closeWriteChannel();
        QByteArray standardOutput;
        QTRY_VERIFY_WITH_TIMEOUT((standardOutput += child.readAllStandardOutput(), child.state() == QProcess::NotRunning), scmFinishTimeoutMs);
        standardOutput += child.readAllStandardOutput();
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY(standardOutput.contains("flood line 20000 "));
        QCOMPARE(readFile(recordPath), qsl("noted\nsysExitEvent\n"));
    }

    // Lua's io.write() shares standard output; what stdio holds of it must not fail the runner's next line
    void test_luaOutputLeftWithStdioIsNotAFailedWrite()
    {
        const QString name = qsl("Headless Runner Lua Write");
        QVERIFY(writeProfile(name, qsl(R"lua(tempTimer(0, function()
  io.write(string.rep("x", 1000000) .. "\n")
  io.write("held\n")
  feedTriggers("after the pipe filled\n")
  closeMudlet()
end))lua")));

        QProcess child;
        child.setChildProcessModifier([]() {
            ::fcntl(STDOUT_FILENO, F_SETFL, ::fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK);
        });
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        // Not reading, so the first write fills the pipe and the rest find it full
        QThread::msleep(2000);
        QByteArray standardOutput;
        QTRY_VERIFY_WITH_TIMEOUT((standardOutput += child.readAllStandardOutput(), child.state() == QProcess::NotRunning), scmFinishTimeoutMs);
        standardOutput += child.readAllStandardOutput();
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY(standardOutput.endsWith("after the pipe filled\n"));
    }

    // As test_closedOutputSavesAndExits(), with the runner waiting on a full non-blocking pipe when its reader goes
    void test_aReaderGoneFromAFullNonBlockingOutputSavesAndExits()
    {
        const QString name = qsl("Headless Runner Non-blocking Reader");
        QVERIFY(writeProfile(name, qsl(R"lua(tempTimer(0, function()
  for i = 1, 20000 do
    feedTriggers("flood line " .. i .. " to fill the pipe to the reader\n")
  end
end))lua")));

        int output[2];
        QVERIFY(::pipe(output) == 0);
        QProcess child;
        child.setChildProcessModifier([output]() {
            ::dup2(output[1], STDOUT_FILENO);
            ::close(output[0]);
            ::close(output[1]);
            ::fcntl(STDOUT_FILENO, F_SETFL, ::fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK);
        });
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        ::close(output[1]);
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        QThread::msleep(2000);
        ::close(output[0]);

        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(!standardError.contains(qsl("cannot write")), qPrintable(standardError));
    }

    // What ended the run never hides that its save failed
    void test_aFailedSaveOutranksTheSignal()
    {
        const QString name = qsl("Headless Runner Signal Unsavable");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local current = getMudletHomeDir() .. "/current"
  os.rename(current, current .. ".moved")
  local file = io.open(current, "wb")
  file:write("not a folder")
  file:close()
end)
tempTimer(0, function() feedTriggers("headless ready\n") end))lua"),
                                      recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        QByteArray standardOutput;
        QTRY_VERIFY_WITH_TIMEOUT((standardOutput += child.readAllStandardOutput()).contains("headless ready"), scmFinishTimeoutMs);
        QVERIFY(::kill(static_cast<pid_t>(child.processId()), SIGTERM) == 0);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = QString::fromUtf8(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
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
