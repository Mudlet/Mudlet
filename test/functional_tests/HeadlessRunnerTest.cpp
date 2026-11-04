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
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "TelnetServerStub.h"

#if defined(Q_OS_WINDOWS)
#include <windows.h>
#endif
#if defined(Q_OS_UNIX)
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <termios.h>
#include <thread>
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
            ++mConnections;
            mpClient = mServer.nextPendingConnection();
            QObject::connect(mpClient, &QTcpSocket::readyRead, [this]() {
                mReceived += mpClient->readAll();
            });
            mpClient->write("Welcome to the recording game.\n");
        });
        return mServer.listen(QHostAddress::LocalHost);
    }
    quint16 port() const { return mServer.serverPort(); }
    int connections() const { return mConnections; }
    bool connected() const { return mpClient && mpClient->state() == QAbstractSocket::ConnectedState; }
    QByteArray received() const { return mReceived; }
    void hangUp() { mpClient->disconnectFromHost(); }

private:
    QTcpServer mServer;
    QPointer<QTcpSocket> mpClient;
    QByteArray mReceived;
    int mConnections = 0;
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

    QString readProfileData(const QString& name, const QString& item) const
    {
        QFile file(qsl("%1/%2").arg(profileFolder(name), item));
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_12);
        QString value;
        in >> value;
        return value;
    }

    // A package of one script, which runs as it is installed
    QString writePackage(const QString& name, const QString& script) const
    {
        const QString path = qsl("%1/%2.xml").arg(mSandbox.path(), name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return QString();
        }
        file.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>%1</name><packageName></packageName><script>%2</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                           .arg(name, script.toHtmlEscaped())
                           .toUtf8());
        return path;
    }

    bool connectTo(const QString& name, const RecordingGame& game) const
    {
        return writeProfileData(name, qsl("url"), qsl("127.0.0.1")) && writeProfileData(name, qsl("port"), QString::number(game.port()));
    }

    qsizetype saveCount(const QString& name) const { return QDir(qsl("%1/current").arg(profileFolder(name))).entryList(QStringList{qsl("*.xml")}, QDir::Files).size(); }

    // The C runtime writes a line feed to a text-mode stream, as standard output and error are, as CR LF on Windows
    static QString textOf(const QByteArray& written)
    {
        QString text = QString::fromUtf8(written);
#if defined(Q_OS_WINDOWS)
        text.replace(qsl("\r\n"), qsl("\n"));
#endif
        return text;
    }

    static QString readFile(const QString& path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    // variables are NAME=value
    void startChild(QProcess& child, const QStringList& arguments, const QString& root = QString(), const QString& platform = QString(), const QStringList& variables = QStringList()) const
    {
        QProcessEnvironment environment = childEnvironment(root.isEmpty() ? mSandbox.path() : root);
        if (!platform.isEmpty()) {
            environment.insert(qsl("QT_QPA_PLATFORM"), platform);
        }
        for (const QString& variable : variables) {
            environment.insert(variable.section(QLatin1Char('='), 0, 0), variable.section(QLatin1Char('='), 1));
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
        QTest::newRow("port out of range") << QStringList{qsl("-p"), qsl("Runner Port Too High"), qsl("--host"), qsl("game.example"), qsl("--port"), qsl("70000")} << 2 << QString()
                                           << qsl("1 to 65535");
        QTest::newRow("port not a number") << QStringList{qsl("-p"), qsl("a"), qsl("--port"), qsl("x")} << 2 << QString() << qsl("not 'x'");
        QTest::newRow("port 0") << QStringList{qsl("-p"), qsl("a"), qsl("--port"), qsl("0")} << 2 << QString() << qsl("not '0'");
        QTest::newRow("password variable unset") << QStringList{qsl("-p"), qsl("a"), qsl("--password-env"), qsl("HEADLESS_RUNNER_NO_SUCH_VARIABLE")} << 2 << QString()
                                                 << qsl("HEADLESS_RUNNER_NO_SUCH_VARIABLE");
        QTest::newRow("a name the dialog refuses") << QStringList{qsl("-p"), qsl("Runner:Refused"), qsl("--host"), qsl("127.0.0.1")} << 2 << QString() << qsl("the : character is not permitted");
        QTest::newRow("--tls and --no-tls") << QStringList{qsl("-p"), qsl("a"), qsl("--tls"), qsl("--no-tls")} << 2 << QString() << qsl("--tls and --no-tls");
        QTest::newRow("a host the dialog refuses") << QStringList{qsl("-p"), qsl("Runner Bad Host"), qsl("--host"), qsl("bad host")} << 2 << QString() << qsl("not the address of a game server");
        QTest::newRow("TLS to an IP address") << QStringList{qsl("-p"), qsl("Runner TLS IP"), qsl("--host"), qsl("127.0.0.1"), qsl("--tls")} << 2 << QString() << qsl("not an IP address");
        QTest::newRow("a blank host") << QStringList{qsl("-p"), qsl("Runner Blank Host"), qsl("--host"), qsl(" ")} << 2 << QString() << qsl("--host was given nothing");
        QTest::newRow("an empty login") << QStringList{qsl("-p"), qsl("a"), qsl("--login"), QString()} << 2 << QString() << qsl("--login was given nothing");
        QTest::newRow("a blank login") << QStringList{qsl("-p"), qsl("a"), qsl("--login"), qsl(" ")} << 2 << QString() << qsl("--login was given nothing");
        QTest::newRow("a blank profile") << QStringList{qsl("-p"), qsl(" ")} << 2 << QString() << qsl("No profile was given");
        QTest::newRow("-p twice") << QStringList{qsl("-p"), qsl("a"), qsl("--profile"), qsl("b")} << 2 << QString() << qsl("--profile was given more than once");
        QTest::newRow("--host twice") << QStringList{qsl("-p"), qsl("a"), qsl("--host"), qsl("one.example"), qsl("--host"), qsl("two.example")} << 2 << QString()
                                      << qsl("--host was given more than once");
        QTest::newRow("--port twice") << QStringList{qsl("-p"), qsl("a"), qsl("--port"), qsl("23"), qsl("--port"), qsl("4000")} << 2 << QString() << qsl("--port was given more than once");
        QTest::newRow("--login twice") << QStringList{qsl("-p"), qsl("a"), qsl("--login"), qsl("one"), qsl("--login"), qsl("two")} << 2 << QString() << qsl("--login was given more than once");
        QTest::newRow("an empty config dir") << QStringList{qsl("-p"), qsl("a"), qsl("--config-dir"), QString()} << 2 << QString() << qsl("--config-dir was given nothing");
        QTest::newRow("--config-dir twice") << QStringList{qsl("-p"), qsl("a"), qsl("--config-dir"), qsl("/one"), qsl("--config-dir"), qsl("/two")} << 2 << QString()
                                            << qsl("--config-dir was given more than once");
        QTest::newRow("an unknown --color") << QStringList{qsl("-p"), qsl("a"), qsl("--color"), qsl("sometimes")} << 2 << QString() << qsl("--color takes auto, always or never, not 'sometimes'");
        QTest::newRow("--color twice") << QStringList{qsl("-p"), qsl("a"), qsl("--color"), qsl("always"), qsl("--color"), qsl("never")} << 2 << QString() << qsl("--color was given more than once");
        QTest::newRow("--password-env twice") << QStringList{qsl("-p"), qsl("a"), qsl("--password-env"), qsl("ONE"), qsl("--password-env"), qsl("TWO")} << 2 << QString()
                                              << qsl("--password-env was given more than once");
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
        const QString standardOutput = textOf(child.readAllStandardOutput());
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == exitCode, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardOutput.contains(standardOutputHas), qPrintable(standardOutput));
        QVERIFY2(standardError.contains(standardErrorHas), qPrintable(standardError));
    }

    // Relative to where it is run from, as a path on a command line is
    void test_configDirKeepsTheProfiles()
    {
        const QString name = qsl("Headless Runner Own Folder");
        const QString chosen = qsl("%1/chosen config").arg(mSandbox.path());
        QProcess child;
        child.setWorkingDirectory(mSandbox.path());
        startChild(child, {qsl("--config-dir"), qsl("chosen config"), qsl("-p"), name, qsl("--host"), qsl("game.example"), qsl("--offline"), qsl("--no-stdin"), qsl("-e"), qsl("closeMudlet()")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QVERIFY(QDir(qsl("%1/profiles/%2").arg(chosen, name)).exists());
        QVERIFY(QFileInfo::exists(qsl("%1/Mudlet.ini").arg(chosen)));
        QVERIFY(!QDir(profileFolder(name)).exists());
        QVERIFY2(standardError.contains(qsl("loaded in offline mode")), qPrintable(standardError));

        // Its language is the one its Mudlet.ini names, not the GUI's
        {
            QSettings ini(qsl("%1/Mudlet.ini").arg(chosen), QSettings::IniFormat);
            ini.setValue(qsl("interfaceLanguage"), qsl("de_DE"));
            ini.sync();
            QCOMPARE(ini.status(), QSettings::NoError);
        }
        QProcess translated;
        translated.setWorkingDirectory(mSandbox.path());
        startChild(translated, {qsl("--config-dir"), qsl("chosen config"), qsl("-p"), name, qsl("--offline"), qsl("--no-stdin"), qsl("-e"), qsl("closeMudlet()")});
        QTRY_COMPARE_WITH_TIMEOUT(translated.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString translatedError = textOf(translated.readAllStandardError());
        QVERIFY2(translated.exitCode() == 0, qPrintable(translatedError));
        QVERIFY2(translatedError.contains(qsl("im Offline-Modus geladen")), qPrintable(translatedError));

        const QString file = qsl("%1/Mudlet.ini").arg(chosen);
        QProcess refused;
        startChild(refused, {qsl("--config-dir"), file, qsl("-p"), name, qsl("--offline"), qsl("--no-stdin")});
        QTRY_COMPARE_WITH_TIMEOUT(refused.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString refusal = textOf(refused.readAllStandardError());
        QVERIFY2(refused.exitCode() == 2, qPrintable(refusal));
        QVERIFY2(refusal.contains(qsl("is not a folder")), qPrintable(refusal));
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
        const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardError = textOf(child.readAllStandardError());
        // Windows writes standard output in text mode, so its lines end in \r\n
        const QStringList lines = textOf(child.readAllStandardOutput()).remove(QLatin1Char('\r')).split(QLatin1Char('\n'));
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QVERIFY2(lines.contains(qsl("Welcome to the headless runner test.")), qPrintable(lines.join(QLatin1Char('\n'))));
    }

    void test_colorShowsTheGamesColours_data()
    {
        QTest::addColumn<QStringList>("options");
        QTest::addColumn<QStringList>("variables");
        QTest::addColumn<QByteArray>("firstLine");

        const QByteArray plain = QByteArrayLiteral("red plain bold");
        // The profile's dark red, as 24-bit colour or the nearest of 256; the profile's own colours are left out
        QTest::newRow("always, 256 colours") << QStringList{qsl("--color"), qsl("always")} << QStringList{qsl("COLORTERM=")}
                                             << QByteArrayLiteral("\x1b[0;38;5;88mred\x1b[0m plain\x1b[0;1;4m bold\x1b[0m");
        QTest::newRow("always, 24-bit colour") << QStringList{qsl("--color"), qsl("always")} << QStringList{qsl("COLORTERM=truecolor")}
                                               << QByteArrayLiteral("\x1b[0;38;2;128;0;0mred\x1b[0m plain\x1b[0;1;4m bold\x1b[0m");
        QTest::newRow("never") << QStringList{qsl("--color"), qsl("never")} << QStringList{qsl("COLORTERM=truecolor")} << plain;
        QTest::newRow("auto, to a pipe") << QStringList() << QStringList{qsl("COLORTERM=truecolor")} << plain;
    }

    void test_colorShowsTheGamesColours()
    {
        QFETCH(QStringList, options);
        QFETCH(QStringList, variables);
        QFETCH(QByteArray, firstLine);

        const QString name = qsl("Headless Runner Colours");
        QVERIFY(writeProfile(name, QString()));
        QProcess child;
        startChild(child,
                   QStringList{qsl("-p"), name, qsl("--offline"), qsl("--no-stdin"), qsl("-e"), qsl("feedTriggers('\\27[31mred\\27[0m plain\\27[1;4m bold\\27[0m\\nplain\\n') closeMudlet()")}
                           + options,
                   QString(),
                   QString(),
                   variables);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        const QList<QByteArray> lines = child.readAllStandardOutput().replace('\r', QByteArray()).split('\n');
        QCOMPARE(lines.mid(0, 2), (QList<QByteArray>{firstLine, QByteArrayLiteral("plain")}));
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
        const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardOutput = textOf(child.readAllStandardOutput());
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
        const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("timer\nsysLoadEvent\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // As `mudlet-headless --help | head -1`, which is no error
    void test_helpToAReaderThatWentAway()
    {
        int output[2];
        QVERIFY(::pipe(output) == 0);
        ::close(output[0]);
        QProcess child;
        child.setChildProcessModifier([output]() {
            ::dup2(output[1], STDOUT_FILENO);
            ::close(output[1]);
        });
        startChild(child, {qsl("--help")});
        ::close(output[1]);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
    }

    // The reader going away ends the run, but not before the -e steps given have run, as they would have
    // with nothing to read their output
    void test_stepsRunAfterTheReaderWentAway()
    {
        const QString name = qsl("Headless Runner Steps After Reader");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        int output[2];
        QVERIFY(::pipe(output) == 0);
        ::close(output[0]);
        QProcess child;
        child.setChildProcessModifier([output]() {
            ::dup2(output[1], STDOUT_FILENO);
            ::close(output[1]);
        });
        startChild(child, {qsl("-p"), name, qsl("--no-stdin"), qsl("-e"), qsl(R"lua(feedTriggers("one\ntwo\n"))lua"), qsl("-e"), qsl(R"lua(record("second step"))lua")});
        ::close(output[1]);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("second step\nsysExitEvent\n"));
    }
#endif

#if defined(Q_OS_WINDOWS)
    // The child's standard output, a pipe whose read end is already closed, so a write to it fails with
    // ERROR_NO_DATA, which the C runtime reports as EINVAL; the returned handle is closed once it has started
    static HANDLE giveAReaderThatWentAway(QProcess& child)
    {
        SECURITY_ATTRIBUTES inheritable{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        HANDLE readEnd = nullptr;
        HANDLE writeEnd = nullptr;
        if (!CreatePipe(&readEnd, &writeEnd, &inheritable, 0)) {
            return nullptr;
        }
        CloseHandle(readEnd);
        child.setCreateProcessArgumentsModifier([writeEnd](QProcess::CreateProcessArguments* arguments) {
            arguments->inheritHandles = TRUE;
            arguments->startupInfo->dwFlags |= STARTF_USESTDHANDLES;
            arguments->startupInfo->hStdOutput = writeEnd;
        });
        return writeEnd;
    }

    void test_helpToAReaderThatWentAwayOnWindows()
    {
        QProcess child;
        const HANDLE writeEnd = giveAReaderThatWentAway(child);
        QVERIFY(writeEnd);
        startChild(child, {qsl("--help")});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        CloseHandle(writeEnd);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
    }

    void test_stepsRunAfterTheReaderWentAwayOnWindows()
    {
        const QString name = qsl("Headless Runner Steps After Reader Windows");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        QProcess child;
        const HANDLE writeEnd = giveAReaderThatWentAway(child);
        QVERIFY(writeEnd);
        startChild(child, {qsl("-p"), name, qsl("--no-stdin"), qsl("-e"), qsl(R"lua(feedTriggers("one\ntwo\n"))lua"), qsl("-e"), qsl(R"lua(record("second step"))lua")});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        CloseHandle(writeEnd);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("second step\nsysExitEvent\n"));
    }
#endif

#if defined(Q_OS_UNIX)
    // A pipe the child's standard output, and its standard error too if errorsToo, write to, with lines a page
    // long so that once full it has no room left for anything; the test holds the read end
    bool startStalledChild(QProcess& child, int& readEnd, const QString& name, const QString& script, const bool errorsToo, QString& recordPath)
    {
        if (!writeRecordingProfile(name,
                                   qsl(R"lua(stalledLines = tempTimer(0.01, function() feedTriggers(string.rep("x", 4095) .. "\n") end, true)
%1)lua")
                                           .arg(script),
                                   recordPath)) {
            return false;
        }
        int output[2];
        if (::pipe(output) != 0) {
            return false;
        }
        readEnd = output[0];
        child.setChildProcessModifier([output, errorsToo]() {
            ::dup2(output[1], STDOUT_FILENO);
            if (errorsToo) {
                ::dup2(output[1], STDERR_FILENO);
            }
            ::close(output[0]);
            ::close(output[1]);
        });
        // The close's debug messages as well, which Qt would otherwise send to the journal. With stderr on the pipe,
        // the leak check's exit report (LSan's log_threads lines) is a blocking write to it, so send it to a file.
        startChild(child,
                   {qsl("-p"), name, qsl("--no-stdin")},
                   QString(),
                   QString(),
                   errorsToo ? QStringList{qsl("QT_LOGGING_RULES=default.debug=true"),
                                           qsl("QT_FORCE_STDERR_LOGGING=1"),
                                           qsl("ASAN_OPTIONS=%1:log_path=%2/stalled-sanitizer").arg(qEnvironmentVariable("ASAN_OPTIONS"), mSandbox.path())}
                             : QStringList());
        ::close(output[1]);
        // Until it stops filling for a second, though a line comes every 10ms, so the writer is waiting on it. Not
        // until it holds some size: macOS sizes a pipe by the writes it has had, so one can be full at well under 16KB.
        int buffered = 0;
        int unchangedPolls = 0;
        for (int previous = -1, waitedMs = 0; waitedMs < scmFinishTimeoutMs && unchangedPolls < 4; waitedMs += 250) {
            QTest::qWait(250);
            if (::ioctl(readEnd, FIONREAD, &buffered) != 0) {
                return false;
            }
            unchangedPolls = buffered > 0 && buffered == previous ? unchangedPolls + 1 : 0;
            previous = buffered;
        }
        return unchangedPolls == 4;
    }

    // So that a failed check leaves no runner behind: closing the pipe first frees one waiting to write to it
    static void endStalledChild(QProcess& child, int& readEnd)
    {
        if (readEnd != -1) {
            ::close(readEnd);
            readEnd = -1;
        }
        if (child.state() != QProcess::NotRunning) {
            child.kill();
            child.waitForFinished(scmFinishTimeoutMs);
        }
    }

    static QByteArray readToTheEnd(const int descriptor)
    {
        QByteArray bytes;
        char chunk[4096];
        pollfd readable{descriptor, POLLIN, 0};
        ssize_t got = 0;
        // Bounded, as a runner that never stops would never end the pipe
        const QDeadlineTimer deadline(scmFinishTimeoutMs);
        while (!deadline.hasExpired() && ::poll(&readable, 1, static_cast<int>(deadline.remainingTime())) == 1 && (got = ::read(descriptor, chunk, sizeof(chunk))) > 0) {
            bytes.append(chunk, got);
        }
        return bytes;
    }

    // What startStalledChild() sent to files rather than the stalled pipe
    QString stalledSanitizerReports() const
    {
        QString reports;
        const QDir sandbox(mSandbox.path());
        for (const QString& fileName : sandbox.entryList({qsl("stalled-sanitizer.*")}, QDir::Files)) {
            reports.append(qsl("\n%1:\n%2").arg(fileName, readFile(sandbox.filePath(fileName))));
        }
        return reports;
    }

    void test_aStopSignalEndsAWaitOnAStalledReader_data()
    {
        QTest::addColumn<bool>("errorsToo");
        QTest::newRow("standard output") << false;
        // As `mudlet-headless -p name 2>&1 | less`: what the close logs goes to the stalled pipe too
        QTest::newRow("both streams") << true;
    }

    // A reader that holds the pipe but stops reading, as `mudlet-headless -p name | less` left at its prompt,
    // stalls the run as the GUI's --mirror would; a stop signal still ends it with a save
    void test_aStopSignalEndsAWaitOnAStalledReader()
    {
        QFETCH(bool, errorsToo);
        const QString name = errorsToo ? qsl("Headless Runner Stalled Both") : qsl("Headless Runner Stalled Reader");
        QString recordPath;
        QProcess child;
        int readEnd = -1;
        const auto cleanup = qScopeGuard([&child, &readEnd]() {
            endStalledChild(child, readEnd);
        });
        QVERIFY(startStalledChild(child, readEnd, name, QString(), errorsToo, recordPath));
        QVERIFY(::kill(child.processId(), SIGTERM) == 0);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString piped = QString::fromUtf8(readToTheEnd(readEnd));
        ::close(readEnd);
        readEnd = -1;
        const QString standardError = errorsToo ? piped : textOf(child.readAllStandardError());
        const QString sanitizerReport = stalledSanitizerReports();
        // In full, as QtTest cuts a long failure message short
        if (!sanitizerReport.isEmpty()) {
            qWarning().noquote() << "The stalled runner's sanitizer report:" << sanitizerReport;
        }
        QVERIFY2(child.exitStatus() == QProcess::NormalExit, qPrintable(qsl("the runner crashed, stderr: %1, sanitizer: %2").arg(standardError.right(2000), sanitizerReport)));
        QVERIFY2(child.exitCode() == 128 + SIGTERM, qPrintable(qsl("exit code %1, stderr: %2, sanitizer: %3").arg(child.exitCode()).arg(standardError.right(2000), sanitizerReport)));
        QVERIFY2(!standardError.contains(qsl("cannot write to standard output")), qPrintable(qsl("%1, sanitizer: %2").arg(standardError.right(2000), sanitizerReport)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // One that is only slow, as `| tee` writing to a busy disk, still gets what comes after the signal
    void test_aSlowReaderGetsTheLinesAfterAStopSignal()
    {
        const QString name = qsl("Headless Runner Slow Reader");
        QString recordPath;
        QProcess child;
        int readEnd = -1;
        const auto cleanup = qScopeGuard([&child, &readEnd]() {
            endStalledChild(child, readEnd);
        });
        QVERIFY(startStalledChild(
                child, readEnd, name, qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() killTimer(stalledLines) feedTriggers("headless last line\n") end))lua"), false, recordPath));
        // Reading from before the signal, so that however long this thread is kept waiting for the processor, the
        // reader is well inside the time the runner gives it once stopping; slower than the lines come, so the
        // runner still waits on it
        QByteArray piped;
        std::thread reader([&piped, readEnd]() {
            char chunk[4096];
            pollfd readable{readEnd, POLLIN, 0};
            ssize_t got = 0;
            const QDeadlineTimer deadline(scmFinishTimeoutMs);
            while (!deadline.hasExpired() && ::poll(&readable, 1, static_cast<int>(deadline.remainingTime())) == 1 && (got = ::read(readEnd, chunk, sizeof(chunk))) > 0) {
                piped.append(chunk, got);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });
        const bool signalled = ::kill(child.processId(), SIGTERM) == 0;
        reader.join();
        QVERIFY(signalled);
        ::close(readEnd);
        readEnd = -1;
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 128 + SIGTERM, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(piped.endsWith("\nheadless last line\n"), piped.right(200).constData());
    }
#endif

    // Lines are read only as fast as the aliases and the game take them, so a file piped in does not sit in
    // memory whole: while the first line's alias holds the main thread, the rest wait to be written to the child
    void test_inputReadAheadIsBounded()
    {
        const QString name = qsl("Headless Runner Flood");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^hold$", [[record("holding") local started = os.time() while os.time() - started < 20 do end]]))lua"), recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));
        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        // More than any pipe holds: Qt makes a child's 1MB on Windows
        const QByteArray lines = "hold\n" + QByteArray("look\n").repeated(1700000);
        child.write(lines);
        QTRY_COMPARE_WITH_TIMEOUT(readFile(recordPath), qsl("holding\n"), scmFinishTimeoutMs);
        // Time enough to read it all, were it read with nothing to stop it
        QTest::qWait(2000);
        // 1024 queued lines, the reader's 4KB and the pipe, with room to spare
        const qsizetype taken = lines.size() - child.bytesToWrite();
        QVERIFY2(taken < 2 * 1024 * 1024, qPrintable(qsl("%1 of %2 bytes read").arg(taken).arg(lines.size())));
        child.kill();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
    }

    // Every line past the bound is still typed, in order, as the reader goes on once the main thread has taken some
    void test_linesPastTheBoundAreTypedInOrder()
    {
        const QString name = qsl("Headless Runner Numbered");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(seen = {}
tempAlias("^hold$", [[local started = os.time() while os.time() - started < 2 do end]])
tempAlias("^n (\\d+)$", [[seen[#seen + 1] = tonumber(matches[2])]])
registerAnonymousEventHandler("sysExitEvent", function()
  local inOrder = true
  for i, n in ipairs(seen) do inOrder = inOrder and n == i end
  record(#seen .. (inOrder and " in order" or " out of order"))
end))lua"),
                                      recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));
        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        // Held while all 3000 come, so the reader surely waits at the bound
        QByteArray lines("hold\n");
        for (int i = 1; i <= 3000; ++i) {
            lines += "n " + QByteArray::number(i) + "\n";
        }
        child.write(lines);
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardOutput = textOf(child.readAllStandardOutput());
        const QString standardError = textOf(child.readAllStandardError());
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

        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("note");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("noted\nsysExitEvent\n"));
    }

    // Typed and shown as UTF-8 whatever the locale, which is C here
    void test_textThatIsNotAsciiRoundTrips()
    {
        const QString name = qsl("Headless Runner Accents");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^say (.+)$", [[record(matches[2]) feedTriggers(matches[2] .. "\n")]]))lua"), recordPath));

        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("say \xc3\xa9t\xc3\xa9\n");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QByteArray standardOutput = textOf(child.readAllStandardOutput()).toUtf8();
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QFile record(recordPath);
        QVERIFY(record.open(QIODevice::ReadOnly));
        QCOMPARE(record.readAll(), QByteArray("\xc3\xa9t\xc3\xa9\nsysExitEvent\n"));
        QVERIFY2(standardOutput.contains("\xc3\xa9t\xc3\xa9\n"), standardOutput.toHex(' ').constData());
    }

    // Lines read once a close has begun are refused, sooner than the GUI's command line would refuse them, so
    // none reaches the game after sysExitEvent
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
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QVERIFY2(standardError.count(qsl("The profile is closing, so it takes no more commands.")) == 1, qPrintable(standardError.left(2000)));
        // The child may be seen to end before what it sent last is read
        QTRY_VERIFY2_WITH_TIMEOUT(game.received().contains("exit event"), game.received().constData(), scmFinishTimeoutMs);
        const QByteArray received = game.received();
        QVERIFY2(!received.mid(received.indexOf("exit event")).contains("after"), received.right(500).constData());
    }

    // A line that comes while an alias loads another profile is refused, the load pumping the event loop it
    // comes on; each is lost where the GUI would have typed it, so each is said and the run fails, and the close
    // that comes after still says it refuses the rest
    void test_linesRefusedWhileAProfileLoadsFailTheRun()
    {
        const QString name = qsl("Headless Runner Loads Another");
        const QString other = qsl("Headless Runner Loaded");
        QString recordPath;
        // The load alias holds the main thread first, so the lines after it are surely read before the load runs
        // the event loop
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(tempAlias("^load$", [[local started = os.clock() while os.clock() - started < 0.2 do end loadProfile("%1") record("loaded")]])
tempAlias("^bye$", [[closeProfile()]]))lua")
                                              .arg(other),
                                      recordPath));
        QVERIFY(writeProfile(other, QString()));
        // TMap::restore() pumps the event loop as it reports a map file it cannot read
        QVERIFY(QDir().mkpath(qsl("%1/map").arg(profileFolder(other))));
        QFile map(qsl("%1/map/map.json").arg(profileFolder(other)));
        QVERIFY(map.open(QIODevice::WriteOnly) && map.write("not a map") > 0);
        map.close();

        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));
        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("load\nfirst\nsecond\n");
        QTRY_COMPARE_WITH_TIMEOUT(readFile(recordPath), qsl("loaded\n"), scmFinishTimeoutMs);
        QByteArray lines("bye\n");
        for (int i = 0; i < 3000; ++i) {
            lines += "after " + QByteArray::number(i) + "\n";
        }
        child.write(lines);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError.left(3000))));
        QVERIFY2(standardError.count(qsl("Refused while a profile loads;")) == 2, qPrintable(standardError.left(3000)));
        QVERIFY2(standardError.count(qsl("The profile is closing, so it takes no more commands.")) == 1, qPrintable(standardError.left(3000)));
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
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QCOMPARE(readFile(recordPath), record);
    }

    void test_hostLoginAndPasswordConnect_data()
    {
        QTest::addColumn<bool>("newProfile");
        QTest::newRow("a profile made for the host") << true;
        QTest::newRow("an existing profile") << false;
    }

    // The details are stored as the connection dialog stores what is typed into it, which makes the profile and
    // sets it up as the dialog does; the password is for the run alone and never shown
    void test_hostLoginAndPasswordConnect()
    {
        QFETCH(bool, newProfile);
        const QString name = newProfile ? qsl("Headless Runner Made") : qsl("Headless Runner Detailed");
        const QString packagesPath = qsl("%1/%2.packages").arg(mSandbox.path(), name);
        if (newProfile) {
            QVERIFY(!QDir(profileFolder(name)).exists());
        } else {
            QString ignored;
            QVERIFY(writeRecordingProfile(name, QString(), ignored));
            QVERIFY(writeProfileData(name, qsl("url"), qsl("game.example")) && writeProfileData(name, qsl("port"), qsl("4000")));
        }
        RecordingGame game;
        QVERIFY(game.start());
        const QString password = qsl("s3cret-headless-pw");

        QProcess child;
        QProcessEnvironment environment = childEnvironment(mSandbox.path());
        environment.insert(qsl("HEADLESS_RUNNER_PASSWORD"), password);
        child.setProcessEnvironment(environment);
        child.setProgram(headlessBinary());
        // Opened binary so Windows writes no \r
        child.setArguments({qsl("-p"),
                            name,
                            qsl("--host"),
                            qsl("127.0.0.1"),
                            qsl("--port"),
                            QString::number(game.port()),
                            qsl("--login"),
                            qsl("bob"),
                            qsl("--password-env"),
                            qsl("HEADLESS_RUNNER_PASSWORD"),
                            qsl("--exit-on-disconnect"),
                            qsl("-e"),
                            qsl(R"lua(local file = io.open([[%1]], "wb") file:write(table.concat(getPackages(), " ") .. " env:" .. tostring(os.getenv("HEADLESS_RUNNER_PASSWORD"))) file:close())lua")
                                    .arg(packagesPath)});
        child.start();
        QTRY_VERIFY_WITH_TIMEOUT(game.connected(), scmFinishTimeoutMs);
        QTRY_VERIFY_WITH_TIMEOUT(game.received().contains("bob") && game.received().contains(password.toUtf8()), scmFinishTimeoutMs);
        game.hangUp();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardOutput = textOf(child.readAllStandardOutput());
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 4, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(!standardOutput.contains(password) && !standardError.contains(password), "The password was shown.");
        QCOMPARE(readProfileData(name, qsl("url")), qsl("127.0.0.1"));
        QCOMPARE(readProfileData(name, qsl("port")), QString::number(game.port()));
        QCOMPARE(readProfileData(name, qsl("login")), qsl("bob"));
        // A new profile's TLS box is stored as it stands, an existing profile's setting is left alone
        QCOMPARE(readProfileData(name, qsl("ssl_tsl")), newProfile ? QString::number(Qt::Unchecked) : QString());
        QVERIFY2(!QFileInfo::exists(qsl("%1/password").arg(profileFolder(name))), "The password for the run was stored.");
        // Nor where CredentialManager files a profile's passwords, under the application's own folder
        for (QDirIterator stored(qsl("%1/config").arg(mSandbox.path()), QDir::Files, QDirIterator::Subdirectories); stored.hasNext();) {
            const QString path = stored.next();
            QVERIFY2(!path.contains(qsl("/profiles/%1/passwords/").arg(name)), qPrintable(path));
        }
        QCOMPARE(saveCount(name), newProfile ? 1 : 2);
        // The GUI's first load installs the game's default packages
        const QString packages = readFile(packagesPath);
        QCOMPARE(packages.contains(qsl("echo")) && packages.contains(qsl("generic_mapper")), newProfile);
        // Gone from the environment before any script could read it
        QVERIFY2(packages.endsWith(qsl(" env:nil")), qPrintable(packages));
    }

    void test_tlsAndOfflineAreStored_data()
    {
        QTest::addColumn<QString>("option");
        QTest::addColumn<int>("stored");
        QTest::newRow("--tls") << qsl("--tls") << static_cast<int>(Qt::Checked);
        QTest::newRow("--no-tls") << qsl("--no-tls") << static_cast<int>(Qt::Unchecked);
    }

    // As the dialog's Offline button: the details are stored, the game is left alone
    void test_tlsAndOfflineAreStored()
    {
        QFETCH(QString, option);
        QFETCH(int, stored);
        const QString name = qsl("Headless Runner Offline %1").arg(option.mid(2));
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        RecordingGame game;
        QVERIFY(game.start());

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--host"), qsl("localhost"), qsl("--port"), QString::number(game.port()), option, qsl("--offline"), qsl("-e"), qsl("closeMudlet()")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(standardError));
        QVERIFY2(standardError.contains(qsl("loaded in offline mode")), qPrintable(standardError));
        // A connection the child made just before it exited is only accepted as the event loop runs
        QTest::qWait(200);
        QCOMPARE(game.connections(), 0);
        QCOMPARE(readProfileData(name, qsl("url")), qsl("localhost"));
        QCOMPARE(readProfileData(name, qsl("port")), QString::number(game.port()));
        QCOMPARE(readProfileData(name, qsl("ssl_tsl")), QString::number(stored));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }

    // Packages go in before the Lua, as the GUI installs those on its command line as the profile opens; one
    // that cannot be is reported as there, and fails the run
    void test_packagesAreInstalledBeforeTheLua()
    {
        const QString name = qsl("Headless Runner Package");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        const QString packagePath = qsl("%1/runnerPackage.xml").arg(mSandbox.path());
        QFile packageFile(packagePath);
        QVERIFY(packageFile.open(QIODevice::WriteOnly));
        packageFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>fromPackage</name><packageName></packageName><script>headlessPackageRan = "yes"</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)");
        packageFile.close();
        // Its script starts a save, which the next install must not be put off behind
        const QString savingPath = qsl("%1/savingPackage.xml").arg(mSandbox.path());
        QFile savingFile(savingPath);
        QVERIFY(savingFile.open(QIODevice::WriteOnly));
        savingFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>savingPackage</name><packageName></packageName><script>saveProfile()</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)");
        savingFile.close();
        const QString missing = qsl("%1/missing.mpackage").arg(mSandbox.path());

        QProcess child;
        startChild(child,
                   {qsl("-p"),
                    name,
                    qsl("--package"),
                    savingPath,
                    qsl("--package"),
                    packagePath,
                    qsl("--package"),
                    missing,
                    qsl("-e"),
                    qsl(R"lua(record(tostring(headlessPackageRan)) closeMudlet())lua")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("yes\nsysExitEvent\n"));
        QVERIFY2(standardError.count(qsl("could not be installed")) == 1 && standardError.contains(missing), qPrintable(standardError));

        // A run that installs them can be run again
        QVERIFY(QFile::remove(recordPath));
        QProcess again;
        startChild(again, {qsl("-p"), name, qsl("--package"), savingPath, qsl("--package"), packagePath, qsl("-e"), qsl(R"lua(record(tostring(headlessPackageRan)) closeMudlet())lua")});
        QTRY_COMPARE_WITH_TIMEOUT(again.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString againError = textOf(again.readAllStandardError());
        QVERIFY2(again.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(again.exitCode()).arg(againError)));
        QCOMPARE(readFile(recordPath), qsl("yes\nsysExitEvent\n"));
        QVERIFY2(againError.contains(qsl("as \"savingPackage\" is already installed")) && againError.contains(qsl("as \"runnerPackage\" is already installed")), qPrintable(againError));
    }

    // Its install handlers have run too, as the GUI's event loop runs them before it does anything else
    void test_installHandlersRunBeforeTheLua()
    {
        const QString name = qsl("Headless Runner Install Handler");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        const QString packagePath = writePackage(qsl("runnerHandled"), qsl(R"lua(registerAnonymousEventHandler("sysInstallPackage", function(_, name) headlessHandled = name end))lua"));
        QVERIFY(!packagePath.isEmpty());

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--offline"), qsl("--package"), packagePath, qsl("-e"), qsl(R"lua(record(tostring(headlessHandled)) closeMudlet())lua")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("runnerHandled\nsysExitEvent\n"));
    }

    // Installed, as in the GUI, but a part of it that could not be read fails the run; a run given it again
    // leaves it as it is, as it cannot know how the first install went
    void test_aPackageNotAllOfWhichWorksExitsOne()
    {
        const QString name = qsl("Headless Runner Partial Package");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        const QString packagePath = qsl("%1/partialPackage.xml").arg(mSandbox.path());
        QFile packageFile(packagePath);
        QVERIFY(packageFile.open(QIODevice::WriteOnly));
        packageFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>fromPackage</name><packageName></packageName><script>headlessPartialRan = "yes"</script><eventHandlerList/></Script>
<Script isActive="yes" isFolder="no"><name>cutOff</na)");
        packageFile.close();
        const QStringList arguments{qsl("-p"), name, qsl("--package"), packagePath, qsl("-e"), qsl(R"lua(record(tostring(headlessPartialRan)) closeMudlet())lua")};

        QProcess child;
        startChild(child, arguments);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("not all of it could be read")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("yes\nsysExitEvent\n"));

        QProcess again;
        startChild(again, arguments);
        QTRY_COMPARE_WITH_TIMEOUT(again.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString againError = textOf(again.readAllStandardError());
        QVERIFY2(again.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(again.exitCode()).arg(againError)));
        QVERIFY2(againError.contains(qsl("as \"partialPackage\" is already installed")), qPrintable(againError));
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
                    qsl(R"lua(record("third") closeMudlet())lua"),
                    // As in the GUI, closeMudlet() closes once the script is done, so this still runs
                    qsl("-e"),
                    qsl(R"lua(record("after closeMudlet"))lua")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardOutput = textOf(child.readAllStandardOutput());
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("first\nthird\nafter closeMudlet\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
        QVERIFY2(standardError.count(qsl("exec failure")) == 1, qPrintable(standardError));
        QVERIFY2(standardError.contains(qsl("echoed by exec\n")), qPrintable(standardError));
        QVERIFY2(!standardOutput.contains(qsl("echoed by exec")), qPrintable(standardOutput));
    }

    // closeProfile() starts the close once its script is done, and a step after it would only be refused
    void test_aStepThatClosesTheProfileEndsTheSteps()
    {
        const QString name = qsl("Headless Runner Step Closes");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--no-stdin"), qsl("-e"), qsl(R"lua(record("one") closeProfile())lua"), qsl("-e"), qsl(R"lua(record("two"))lua")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("one\nsysExitEvent\n"));
        QVERIFY2(!standardError.contains(qsl("takes no more commands")), qPrintable(standardError));
    }

    // `printf 'look\n' | mudlet-headless`: the input ends before the connection is made, and its line still goes
    void test_inputWaitsForTheConnection()
    {
        const QString name = qsl("Headless Runner Typed Ahead");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("typed ahead\n");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QTRY_VERIFY_WITH_TIMEOUT(game.received().contains("typed ahead"), scmFinishTimeoutMs);
        QVERIFY2(standardError.contains(qsl("connection made")) && standardError.contains(qsl("typed ahead")), qPrintable(standardError));
        QVERIFY2(standardError.indexOf(qsl("connection made")) < standardError.indexOf(qsl("typed ahead")), qPrintable(standardError));
    }

    // The GUI would echo such a line and drop it with nothing said
    void test_inputBeforeAConnectionThatCannotBeMadeFailsTheRun()
    {
        const QString name = qsl("Headless Runner Typed To Nobody");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^note$", [[record("noted")]]))lua"), recordPath));
        quint16 port = 0;
        {
            QTcpServer closed;
            QVERIFY(closed.listen(QHostAddress::LocalHost));
            port = closed.serverPort();
        }
        QVERIFY(writeProfileData(name, qsl("url"), qsl("127.0.0.1")) && writeProfileData(name, qsl("port"), QString::number(port)));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("note\nlook\n");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("lines not sent: 1")), qPrintable(standardError));
        // As in the GUI, its aliases still run
        QCOMPARE(readFile(recordPath), qsl("noted\nsysExitEvent\n"));
    }

    // One an alias takes whole sends the game nothing, so nothing is lost
    void test_inputAnAliasTakesWithNoConnectionPassesTheRun()
    {
        const QString name = qsl("Headless Runner Noted To Nobody");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^note$", [[record("noted")]]))lua"), recordPath));
        quint16 port = 0;
        {
            QTcpServer closed;
            QVERIFY(closed.listen(QHostAddress::LocalHost));
            port = closed.serverPort();
        }
        QVERIFY(writeProfileData(name, qsl("url"), qsl("127.0.0.1")) && writeProfileData(name, qsl("port"), QString::number(port)));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("note\n");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(!standardError.contains(qsl("lines not sent")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("noted\nsysExitEvent\n"));
    }

    // Another line's alias may run an event loop, which takes the next line; that one is still counted once
    void test_inputTakenDuringAnAliasEventLoopIsCountedOnce()
    {
        const QString name = qsl("Headless Runner Hop");
        quint16 closedPort = 0;
        {
            QTcpServer closed;
            QVERIFY(closed.listen(QHostAddress::LocalHost));
            closedPort = closed.serverPort();
        }
        QString recordPath;
        // Long enough for the next line to be waiting when connectToServer() runs its event loop
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(tempAlias("^hop$", [[
local started = os.clock()
while os.clock() - started < 0.5 do end
connectToServer("127.0.0.1", %1)
record("hopped")
]]))lua")
                                              .arg(closedPort),
                                      recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_VERIFY_WITH_TIMEOUT(game.connected(), scmFinishTimeoutMs);
        child.write("hop\nlook\n");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("lines not sent: 1")), qPrintable(standardError));
        QVERIFY(readFile(recordPath).startsWith(qsl("hopped\n")));
        QVERIFY(!game.received().contains("look"));
    }

    // Lua given with -e runs before the connection is made, so what it sends is lost
    void test_execThatSendsBeforeTheConnectionFailsTheRun()
    {
        const QString name = qsl("Headless Runner Sent Too Soon");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("-e"), qsl(R"lua(send("early"))lua")});
        QTRY_VERIFY_WITH_TIMEOUT(game.connected(), scmFinishTimeoutMs);
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("commands not sent: 1")), qPrintable(standardError));
        QVERIFY(!game.received().contains("early"));
    }

    // With --offline there is no connection to wait for, so what -e sends is not counted
    void test_execThatSendsOfflineIsNotCounted()
    {
        const QString name = qsl("Headless Runner Sent Offline");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--offline"), qsl("-e"), qsl(R"lua(send("early"))lua")});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(!standardError.contains(qsl("commands not sent")), qPrintable(standardError));
    }

    // A package installs before the connection is made too, so what it sends is counted with what -e sends
    void test_aPackageThatSendsBeforeTheConnectionFailsTheRun()
    {
        const QString name = qsl("Headless Runner Package Sent Too Soon");
        quint16 closedPort = 0;
        {
            QTcpServer closed;
            QVERIFY(closed.listen(QHostAddress::LocalHost));
            closedPort = closed.serverPort();
        }
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        const QString packagePath = writePackage(qsl("runnerSendsEarly"), qsl(R"lua(send("frompackage"))lua"));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--host"), qsl("127.0.0.1"), qsl("--port"), QString::number(closedPort), qsl("--package"), packagePath});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("commands not sent: 1")), qPrintable(standardError));
    }

    // Lost as surely as one read before the connection was made
    void test_inputAfterTheConnectionEndedFailsTheRun()
    {
        const QString name = qsl("Headless Runner Typed After Hang Up");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(tempAlias("^note$", [[record("noted")]])
registerAnonymousEventHandler("sysDisconnectionEvent", function() record("disconnected") end))lua"),
                                      recordPath));
        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_VERIFY_WITH_TIMEOUT(game.connected(), scmFinishTimeoutMs);
        game.hangUp();
        QTRY_COMPARE_WITH_TIMEOUT(readFile(recordPath), qsl("disconnected\n"), scmFinishTimeoutMs);
        child.write("note\nlook\n");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("lines not sent: 1")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("disconnected\nnoted\nsysExitEvent\n"));
        QVERIFY(!game.received().contains("look"));
    }

    // With no game to send to, a line is lost to nothing
    void test_inputOfflinePassesTheRun()
    {
        const QString name = qsl("Headless Runner Typed Offline");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^note$", [[record("noted")]]))lua"), recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--offline")});
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        child.write("note\nlook\n");
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(!standardError.contains(qsl("lines not sent")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("noted\nsysExitEvent\n"));
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
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == (exitOnDisconnect ? 4 : 0), qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    void test_aConnectionThatCannotBeMadeExitsFour_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::newRow("alone") << QStringList();
        // Which --help says the disconnection outranks
        QTest::newRow("with a failing -e") << QStringList{qsl("-e"), qsl("error('failing chunk')")};
    }

    void test_aConnectionThatCannotBeMadeExitsFour()
    {
        QFETCH(QStringList, arguments);
        const QString name = qsl("Headless Runner Refused %1").arg(arguments.size());
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
        startChild(child, QStringList{qsl("-p"), name, qsl("--exit-on-disconnect")} + arguments);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("The profile \"%1\" could not be saved.").arg(name)), qPrintable(standardError));
        // What Qt logs, and the runner's own messages, start a line of their own
        QVERIFY2(standardError.contains(qsl("half a line\n")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }

    // The map is the other half of a profile's save
    void test_anUnsavableMapExitsOne()
    {
        const QString name = qsl("Headless Runner Unsavable Map");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name,
                                      qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local map = getMudletHomeDir() .. "/map"
  os.rename(map, map .. ".moved")
  local file = io.open(map, "wb")
  file:write("not a folder")
  file:close()
end)
closeMudlet())lua"),
                                      recordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("The profile \"%1\" could not be saved.").arg(name)), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // A profile a script opened is saved as the run ends too, and one that cannot be is named
    void test_anUnsavableProfileAScriptOpenedExitsOne()
    {
        const QString name = qsl("Headless Runner Opens Another");
        const QString other = qsl("Headless Runner Opened Unsavable");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        QString otherRecordPath;
        QVERIFY(writeRecordingProfile(other,
                                      qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local current = getMudletHomeDir() .. "/current"
  os.rename(current, current .. ".moved")
  local file = io.open(current, "wb")
  file:write("not a folder")
  file:close()
end))lua"),
                                      otherRecordPath));

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--no-stdin"), qsl("-e"), qsl(R"lua(loadProfile("%1", true) closeMudlet())lua").arg(other)});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("The profile \"%1\" could not be saved.").arg(other)), qPrintable(standardError));
        QVERIFY2(!standardError.contains(qsl("The profile \"%1\" could not be saved.").arg(name)), qPrintable(standardError));
        QCOMPARE(readFile(otherRecordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

#if defined(Q_OS_UNIX)
    // The save's file is written on a thread of its own, once the folder has been found good
    void test_aSaveThatCannotBeWrittenExitsOne()
    {
        const QString name = qsl("Headless Runner Too Big");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl("closeMudlet()"), recordPath));

        QProcess child;
        // Smaller than any save, with SIGXFSZ ignored so that the write fails rather than the run ending
        child.setChildProcessModifier([]() {
            ::signal(SIGXFSZ, SIG_IGN);
            const rlimit limit{2048, 2048};
            ::setrlimit(RLIMIT_FSIZE, &limit);
        });
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("The profile \"%1\" could not be saved.").arg(name)), qPrintable(standardError));
        QCOMPARE(saveCount(name), 1);
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

    // The scripts read before the save broke off still run, and a profile they open closes as any other does
    void test_aProfileThatAnUnreadableSaveOpenedIsSaved()
    {
        const QString name = qsl("Headless Runner Corrupt Opener");
        const QString other = qsl("Headless Runner Opened By The Corrupt");
        QVERIFY(writeProfile(other, QString()));
        QVERIFY(QDir().mkpath(qsl("%1/current").arg(profileFolder(name))));
        QFile saveFile(qsl("%1/current/2020-01-01#00-00-00.xml").arg(profileFolder(name)));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>opener</name><packageName></packageName><script>loadProfile("%1", true)</script><eventHandlerList/></Script></ScriptPackage>
<HostPackage><Host)")
                               .arg(other)
                               .toUtf8());
        saveFile.close();

        QProcess child;
        startChild(child, {qsl("-p"), name});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 1);
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(saveCount(other), 2);
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
            const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 128 + signalNumber, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 2);
    }

    // Taken before the next step, though no event loop runs between them
    void test_aStopSignalDuringAnExecChunkSkipsTheRest()
    {
        const QString name = qsl("Headless Runner Signal Between Chunks");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));

        QProcess child;
        // The shell os.execute() starts is the runner's child
        startChild(child, {qsl("-p"), name, qsl("--no-stdin"), qsl("-e"), qsl(R"lua(record("first") os.execute("kill -TERM $PPID"))lua"), qsl("-e"), qsl(R"lua(record("second"))lua")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 128 + SIGTERM, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("first\nsysExitEvent\n"));
    }

    // As between the -e steps
    void test_aStopSignalDuringAPackageInstallSkipsTheRest()
    {
        const QString name = qsl("Headless Runner Signal Between Packages");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, QString(), recordPath));
        const QString first = writePackage(qsl("runnerSignalFirst"), qsl(R"lua(record("first") os.execute("kill -TERM $PPID"))lua"));
        const QString second = writePackage(qsl("runnerSignalSecond"), qsl(R"lua(record("second"))lua"));
        QVERIFY(!first.isEmpty() && !second.isEmpty());

        QProcess child;
        startChild(child, {qsl("-p"), name, qsl("--offline"), qsl("--package"), first, qsl("--package"), second, qsl("-e"), qsl(R"lua(record("exec"))lua")});
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 128 + SIGTERM, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("first\nsysExitEvent\n"));
    }

    void test_aStandardStreamClosedAtTheStart_data()
    {
        QTest::addColumn<int>("descriptor");
        QTest::addColumn<int>("exitCode");
        QTest::addColumn<QString>("record");
        QTest::addColumn<QString>("standardErrorHas");
        QTest::addColumn<bool>("signalled");
        // Qt's event descriptor would take its place, and the game's lines would be written into it
        QTest::newRow("standard output") << int(STDOUT_FILENO) << 3 << QString() << qsl("cannot write to standard output") << false;
        // Or read from, the reader taking the wake-ups meant for the main thread, a stop signal's among them; taken for --no-stdin
        QTest::newRow("standard input") << int(STDIN_FILENO) << 128 + SIGTERM << qsl("timer\nsysExitEvent\n") << QString() << true;
    }

    void test_aStandardStreamClosedAtTheStart()
    {
        QFETCH(int, descriptor);
        QFETCH(int, exitCode);
        QFETCH(QString, record);
        QFETCH(QString, standardErrorHas);
        QFETCH(bool, signalled);
        const QString name = qsl("Headless Runner Closed %1").arg(descriptor);
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempTimer(1, function() record("timer") %1 end))lua").arg(signalled ? QString() : qsl("closeMudlet()")), recordPath));

        QProcess child;
        child.setChildProcessModifier([descriptor]() {
            ::close(descriptor);
        });
        startChild(child, {qsl("-p"), name});
        if (signalled) {
            QTRY_COMPARE_WITH_TIMEOUT(readFile(recordPath), qsl("timer\n"), scmFinishTimeoutMs);
            // Qt's event descriptor, the first one opened, takes the free number otherwise
            const QString opened = QFile::symLinkTarget(qsl("/proc/%1/fd/%2").arg(child.processId()).arg(descriptor));
            QVERIFY2(opened == qsl("/dev/null"), qPrintable(opened));
            QVERIFY(::kill(child.processId(), SIGTERM) == 0);
        }
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == exitCode, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), record);
        QVERIFY2(standardError.contains(standardErrorHas), qPrintable(standardError));
    }

    // A second SIGINT or SIGTERM ends a run whose main thread never gets back to the event loop
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

    // As under nohup, a hangup ignored from the start is left ignored, so the run goes on to the end of its input
    void test_anIgnoredHangupStaysIgnored()
    {
        const QString name = qsl("Headless Runner Nohup");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempTimer(0, function() feedTriggers("headless ready\n") end))lua"), recordPath));

        QProcess child;
        child.setChildProcessModifier([]() {
            ::signal(SIGHUP, SIG_IGN);
        });
        startChild(child, {qsl("-p"), name});
        QByteArray standardOutput;
        QTRY_VERIFY_WITH_TIMEOUT((standardOutput += child.readAllStandardOutput()).contains("headless ready"), scmFinishTimeoutMs);
        QVERIFY(::kill(static_cast<pid_t>(child.processId()), SIGHUP) == 0);
        QTest::qWait(500);
        QCOMPARE(child.state(), QProcess::Running);
        child.closeWriteChannel();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
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
        const QString standardError = textOf(child.readAllStandardError());
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
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(tempAlias("^note$", [[record("noted")]]) tempAlias("^reading$", [[record("reading")]]))lua"), recordPath));
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
        // Only a read under way as the terminal goes fails; one begun after reads as the end of the input, and the
        // runner reads nothing until its connection attempt is over
        QCOMPARE(::write(terminal, "reading\n", 8), 8);
        QTRY_COMPARE_WITH_TIMEOUT(readFile(recordPath), qsl("reading\n"), scmFinishTimeoutMs);
        QCOMPARE(::write(terminal, "note", 4), 4);
        QTest::qWait(500);
        ::close(terminal);
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY2(standardError.contains(qsl("cannot read standard input")), qPrintable(standardError));
        QCOMPARE(readFile(recordPath), qsl("reading\nsysExitEvent\n"));
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

        RecordingGame game;
        QVERIFY(game.start());
        QVERIFY(connectTo(name, game));

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
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 0, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QVERIFY(standardOutput.contains("flood line 20000 "));
        QCOMPARE(readFile(recordPath), qsl("noted\nsysExitEvent\n"));
    }

    // Lua's io.write() shares standard output; a write of its that the full pipe turned away must not fail the
    // runner's next line
    void test_aLuaWriteTheFullPipeTurnedAwayIsNotAFailedWrite()
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
        const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardError = textOf(child.readAllStandardError());
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
        const QString standardError = textOf(child.readAllStandardError());
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 1, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }

    // The terminal standard output goes to is the main console's size, as the GUI's main window is, and a resize
    // of it is passed on
    void test_theGameIsToldTheTerminalsSize()
    {
        const QString name = qsl("Headless Runner Terminal Size");
        QString recordPath;
        QVERIFY(writeRecordingProfile(name, qsl(R"lua(registerAnonymousEventHandler("sysWindowResizeEvent", function() record("resized") end))lua"), recordPath));
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(writeProfileData(name, qsl("url"), qsl("127.0.0.1")));
        QVERIFY(writeProfileData(name, qsl("port"), QString::number(server.serverPort())));
        const int terminal = ::posix_openpt(O_RDWR | O_NOCTTY);
        QVERIFY(terminal >= 0);
        QVERIFY(::grantpt(terminal) == 0 && ::unlockpt(terminal) == 0);
        const int output = ::open(::ptsname(terminal), O_RDWR | O_NOCTTY);
        QVERIFY(output >= 0);
        // Narrower than the default wrap width, which caps what the game is told
        winsize size{};
        size.ws_col = 80;
        size.ws_row = 30;
        QVERIFY(::ioctl(output, TIOCSWINSZ, &size) == 0);
        QVERIFY(::fcntl(terminal, F_SETFD, FD_CLOEXEC) == 0 && ::fcntl(output, F_SETFD, FD_CLOEXEC) == 0);
        QVERIFY(::fcntl(terminal, F_SETFL, O_NONBLOCK) == 0);
        const auto closeTerminal = qScopeGuard([terminal]() {
            ::close(terminal);
        });
        // Read as it comes, so the runner's writes to the terminal never wait
        const auto drain = [terminal]() {
            char buffer[4096];
            while (::read(terminal, buffer, sizeof(buffer)) > 0) {
            }
        };

        QProcess child;
        child.setChildProcessModifier([output]() {
            ::dup2(output, STDOUT_FILENO);
        });
        startChild(child, {qsl("-p"), name, qsl("--no-stdin")});
        ::close(output);
        QVERIFY(child.waitForStarted(scmFinishTimeoutMs));
        QTRY_VERIFY_WITH_TIMEOUT((drain(), server.clientConnected()), scmFinishTimeoutMs);
        // IAC DO NAWS
        server.sendRaw(QByteArray("\xff\xfd\x1f", 3));
        QTRY_VERIFY_WITH_TIMEOUT((drain(), server.nawsUpdates().contains(QSize(80, 30))), scmFinishTimeoutMs);

        size.ws_col = 70;
        size.ws_row = 20;
        QVERIFY(::ioctl(terminal, TIOCSWINSZ, &size) == 0);
        // The terminal is not the child's controlling one, so nothing else sends it this
        QVERIFY(::kill(static_cast<pid_t>(child.processId()), SIGWINCH) == 0);
        QTRY_VERIFY_WITH_TIMEOUT((drain(), server.nawsUpdates().contains(QSize(70, 20))), scmFinishTimeoutMs);

        QVERIFY(::kill(static_cast<pid_t>(child.processId()), SIGTERM) == 0);
        QTRY_COMPARE_WITH_TIMEOUT((drain(), child.state()), QProcess::NotRunning, scmFinishTimeoutMs);
        const QString standardError = textOf(child.readAllStandardError());
        QVERIFY2(child.exitCode() == 128 + SIGTERM, qPrintable(qsl("exit code %1, stderr: %2").arg(child.exitCode()).arg(standardError)));
        QCOMPARE(readFile(recordPath), qsl("resized\nresized\nsysExitEvent\n"));
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
