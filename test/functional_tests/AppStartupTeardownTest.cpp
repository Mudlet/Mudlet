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
 * Functional test for the way out of main() (#10460).
 *
 * A test that links mudlet_core builds its own application object and never
 * reaches src/main.cpp, so the shipped binary is run as a child here - as
 * HeadlessVersionTest does for the options main() answers. A GUI start hands the
 * TLS warm-up to a thread, and a second Mudlet - opened for a telnet:// link or
 * a package while one is already running - forwards that to the running one and
 * leaves main() moments later, with the warm-up still in flight. Unjoined, leaving
 * is where static destruction takes Qt's TLS backend mutex and its library store
 * out from under it: "QMutex: destroying locked mutex", then sometimes a crash
 * inside freed plugin machinery. This test plays the running instance and has the
 * child forward a telnet:// link to it. Nothing on that path writes to the config
 * root; the sandbox below is for what it reads.
 *
 * It is a race, so one clean exit proves nothing; hence the repeats. With the
 * warm-up left unjoined, a Linux debug build warned in 28 of 30 runs and crashed
 * in 15; an ASan build warned in all 30 and crashed in none, so the warning is
 * what is checked for, not only the crash.
 *
 * Run with: ctest -R AppStartupTeardownTest -V
 */

#include <QtTest/QtTest>
#include <QLocalServer>
#include <QLocalSocket>

#include "GroupedTest.h"

#ifndef MUDLET_APP_BINARY
// Set by test/functional_tests/CMakeLists.txt. Without it there is no binary to
// drive and every case below would pass having run nothing, so fail the build
// rather than go quietly green if this file is ever moved out of that group.
#error "AppStartupTeardownTest needs MUDLET_APP_BINARY - see test/functional_tests/CMakeLists.txt"
#endif

class AppStartupTeardownTest : public QObject
{
    Q_OBJECT

private:
    // Enough runs to catch a warm-up that only sometimes loses the race, few
    // enough that the whole case stays well inside its ctest timeout
    static constexpr int scmRunCount = 8;
    static constexpr int scmStartTimeoutMs = 10000;
    static constexpr int scmFinishTimeoutMs = 20000;
    // Must match the name main() gives MudletInstanceCoordinator
    static constexpr QLatin1StringView scmInstanceServerName{"MudletInstanceCoordinator"};
    static constexpr QLatin1StringView scmTelnetUri{"telnet://localhost:1"};

    static QString appBinary() { return QString::fromUtf8(MUDLET_APP_BINARY); }

    // QTest cuts a failure message off at a few KB, and QT_DEBUG_PLUGINS puts
    // that much plugin scanning ahead of what a crash leaves at the end
    static QString tail(const QString& text) { return text.right(1500); }

    // A portable.txt beside the shipped binary, or in $HOME/.config/mudlet,
    // outranks XDG_CONFIG_HOME (MudletApp::resolveConfigRoot()), so the child
    // would read the real install's config instead of the sandbox's. Note the
    // directory checked is the application's, not this test binary's.
    // DialogTeardownTest and HeadlessVersionTest skip for the same reason.
    static bool portableMarkerWouldWin()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QFileInfo(appBinary()).absolutePath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void exitsCleanlyAfterForwardingToARunningInstance()
    {
        QVERIFY2(QFileInfo::exists(appBinary()), qPrintable(qsl("no application binary at %1").arg(appBinary())));
        if (portableMarkerWouldWin()) {
            QSKIP("a portable.txt would send the child at the real config instead of the sandbox");
        }

        for (int run = 1; run <= scmRunCount; ++run) {
            // A root of its own per run, so the child reads none of the
            // developer's profiles and leaves nothing behind. Creating
            // mudlet/profiles is what makes XDG_CONFIG_HOME outrank the legacy
            // ~/.config/mudlet, see MudletApp::xdgConfigDir(). Under /tmp on
            // Unix: the instance socket lives in it, and macOS's per-user temp
            // directory leaves too little of sockaddr_un's 104 bytes.
#if defined(Q_OS_WINDOWS)
            QTemporaryDir sandbox;
#else
            QTemporaryDir sandbox(qsl("/tmp/mudlet-teardown-XXXXXX"));
#endif
            QVERIFY2(sandbox.isValid(), qPrintable(sandbox.errorString()));
            QVERIFY(QDir().mkpath(qsl("%1/config/mudlet/profiles").arg(sandbox.path())));
            QVERIFY(QDir().mkpath(qsl("%1/tmp").arg(sandbox.path())));

            // Before the stand-in, so it outlives the sockets whose handlers append to it
            QByteArray forwarded;
            // Stands in for the running Mudlet the child forwards to
            QLocalServer runningInstance;
            connect(&runningInstance, &QLocalServer::newConnection, &runningInstance, [&runningInstance, &forwarded]() {
                while (QLocalSocket* socket = runningInstance.nextPendingConnection()) {
                    connect(socket, &QLocalSocket::readyRead, socket, [socket, &forwarded]() {
                        forwarded += socket->readAll();
                    });
                }
            });
#if defined(Q_OS_WINDOWS)
            // Pipe names are machine-wide on Windows, so a Mudlet the developer
            // has open would take the child's forward instead of this stand-in
            QLocalSocket probe;
            probe.connectToServer(scmInstanceServerName);
            if (probe.waitForConnected(500)) {
                QSKIP("a running Mudlet holds the instance pipe this test has to stand in for");
            }
            const QString serverName = scmInstanceServerName;
#else
            // Where QLocalSocket looks for the name, given the TMPDIR below
            const QString serverName = qsl("%1/tmp/%2").arg(sandbox.path(), scmInstanceServerName);
#endif
            QVERIFY2(runningInstance.listen(serverName), qPrintable(runningInstance.errorString()));

            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            environment.insert(qsl("XDG_CONFIG_HOME"), qsl("%1/config").arg(sandbox.path()));
            environment.insert(qsl("XDG_DATA_HOME"), qsl("%1/data").arg(sandbox.path()));
            // Keeps a WITH_SENTRY build's crashpad database out of the real cache
            environment.insert(qsl("XDG_CACHE_HOME"), qsl("%1/cache").arg(sandbox.path()));
            // TMPDIR is what Qt reads on Unix, TMP and TEMP on Windows, where
            // runUpdate() would otherwise launch a pending installer from the real one
            for (const QString& variable : {qsl("TMPDIR"), qsl("TMP"), qsl("TEMP")}) {
                environment.insert(variable, qsl("%1/tmp").arg(sandbox.path()));
            }
            environment.insert(qsl("QT_QPA_PLATFORM"), qsl("offscreen"));
            environment.insert(qsl("MUDLET_TEST_MODE"), qsl("1"));
            // Should the forward fail, the child carries on as an instance of its
            // own, and this keeps it from registering itself as the telnet handler
            environment.insert(qsl("CI"), qsl("1"));
            // Has Qt log every plugin directory it searches, so the run below
            // can tell whether the warm-up looked for a TLS backend at all. Forced
            // to stderr, as macOS would send it to the system log instead.
            environment.insert(qsl("QT_DEBUG_PLUGINS"), qsl("1"));
            environment.insert(qsl("QT_FORCE_STDERR_LOGGING"), qsl("1"));
            // The warm-up leaves Qt's CA store loaded for the process lifetime,
            // and an early return leaves the application object to the OS, so a
            // leak check here would only ever report those. Appended rather than
            // replacing what ctest set, since the runtime takes the last setting
            // of a flag and the earlier ones stay.
            const QString inheritedSanitizerOptions = environment.value(qsl("ASAN_OPTIONS"));
            environment.insert(qsl("ASAN_OPTIONS"), inheritedSanitizerOptions.isEmpty() ? qsl("detect_leaks=0") : qsl("%1:detect_leaks=0").arg(inheritedSanitizerOptions));

            QProcess mudlet;
            mudlet.setProcessEnvironment(environment);
            mudlet.setProgram(appBinary());
            mudlet.setArguments({scmTelnetUri});
            mudlet.start();

            const QString where = qsl("run %1 of %2").arg(QString::number(run), QString::number(scmRunCount));
            QVERIFY2(mudlet.waitForStarted(scmStartTimeoutMs), qPrintable(qsl("%1: %2").arg(where, mudlet.errorString())));
            const QByteArray expected = qsl("TELNET_URI:%1").arg(scmTelnetUri).toUtf8();
            // Not waitForFinished(): the stand-in only accepts the child's connections
            // while events are processed. Nor QTRY_VERIFY, which on a late finish
            // reports its timeout as too short instead of the checks below.
            QDeadlineTimer deadline(scmFinishTimeoutMs);
            while (mudlet.state() != QProcess::NotRunning && !deadline.hasExpired()) {
                QTest::qWait(20);
            }
            const bool finished = mudlet.state() == QProcess::NotRunning;
            // The child can be gone before the stand-in has read what it wrote
            const bool reachedReturn = QTest::qWaitFor(
                    [&forwarded, &expected]() {
                        return forwarded.contains(expected);
                    },
                    5000);
            if (!finished) {
                mudlet.kill();
                mudlet.waitForFinished(scmStartTimeoutMs);
            }

            const QString output = QString::fromUtf8(mudlet.readAllStandardOutput());
            const QString standardError = QString::fromUtf8(mudlet.readAllStandardError());
            // Both streams: under MSYS2 main() sends debug output to stdout
            const QString diagnostics = qsl("%1, end of stdout:\n%2\nend of stderr:\n%3").arg(where, tail(output), tail(standardError));
            QVERIFY2(finished,
                     qPrintable(qsl("%1; %2").arg(reachedReturn ? qsl("mudlet forwarded the link but never exited, so it hung on the way out of main()")
                                                                : qsl("mudlet never finished, so it did not leave through the forwarding return and is running as an instance of its own"),
                                                  diagnostics)));
            QVERIFY2(reachedReturn, qPrintable(qsl("the link never reached the stand-in, so main() did not leave through the forwarding return; %1").arg(diagnostics)));
            QVERIFY2(mudlet.exitStatus() == QProcess::NormalExit, qPrintable(qsl("mudlet crashed on leaving main(); %1").arg(diagnostics)));
            QVERIFY2(mudlet.exitCode() == 0, qPrintable(qsl("mudlet exited %1; %2").arg(QString::number(mudlet.exitCode()), diagnostics)));
            const QString lockedMutex = qsl("destroying locked mutex");
            QVERIFY2(!standardError.contains(lockedMutex) && !output.contains(lockedMutex),
                     qPrintable(qsl("a locked mutex was destroyed on the way out of main(), as happens with the TLS warm-up still in flight; %1").arg(diagnostics)));
            // Without a warm-up in flight at the return the case would pass with
            // the #10460 fix reverted, and cover nothing. Nothing else on this
            // path loads TLS, so a search of a tls directory is the warm-up's.
            QVERIFY2(standardError.contains(qsl("/tls\"")) || output.contains(qsl("/tls\"")),
                     qPrintable(qsl("Qt never searched for a TLS backend, so the TLS warm-up never ran, or main() returned without waiting for it; %1").arg(diagnostics)));
        }
    }
};

#include "AppStartupTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(AppStartupTeardownTest)
