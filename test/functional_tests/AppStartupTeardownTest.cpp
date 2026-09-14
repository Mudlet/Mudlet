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
 * Every other functional test links mudlet_core and builds its own
 * QApplication, so nothing in the tree runs src/main.cpp and nothing sees what
 * happens as it returns. main() hands the SSL warm-up to a thread and then
 * leaves, and leaving is where static destruction takes Qt's TLS backend mutex
 * and its library store out from under whatever is still running: the process
 * reports "QMutex: destroying locked mutex" and dies inside freed plugin
 * machinery.
 *
 * --version is the shortest path through main() that still starts the warm-up -
 * initSentry(), the QApplication, mudlet::start(), setupConfig() and the
 * translators, then an immediate return from the version branch - so it leaves
 * a background task the least room to finish and fails first. It stops short of
 * init(), so nothing here registers a telnet handler or writes a desktop file;
 * the sandbox below is for what setupConfig() reads, not for writes.
 *
 * It is a race, so one clean exit proves nothing; hence the repeats. The
 * regression this covers failed all six of six runs on a Linux debug build.
 *
 * Run with: ctest -R AppStartupTeardownTest -V
 */

#include <QtTest/QtTest>

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

    static QString appBinary() { return QString::fromUtf8(MUDLET_APP_BINARY); }

    // setupConfig() consults portable.txt beside the executable, and
    // $HOME/.config/mudlet/portable.txt, before it ever looks at
    // XDG_CONFIG_HOME - so with either one present the child runs against the
    // real portable config, and an invalid path there makes it qFatal() and
    // read as exactly the crash this test hunts. DialogTeardownTest skips for
    // the same reason; the executable here is the application, not this binary.
    static bool portableMarkerWouldWin()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QFileInfo(appBinary()).absolutePath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void exitsCleanlyAfterEveryStartup()
    {
        QVERIFY2(QFileInfo::exists(appBinary()), qPrintable(qsl("no application binary at %1").arg(appBinary())));
        if (portableMarkerWouldWin()) {
            QSKIP("a portable.txt would send the child at the real config instead of the sandbox");
        }

        for (int run = 1; run <= scmRunCount; ++run) {
            // A root of its own per run, so the child reads none of the
            // developer's profiles and leaves nothing behind. Creating
            // mudlet/profiles is what makes XDG_CONFIG_HOME outrank the legacy
            // ~/.config/mudlet, see utils::xdgConfigDir()
            QTemporaryDir sandbox;
            QVERIFY2(sandbox.isValid(), qPrintable(sandbox.errorString()));
            QVERIFY(QDir().mkpath(qsl("%1/config/mudlet/profiles").arg(sandbox.path())));

            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            environment.insert(qsl("XDG_CONFIG_HOME"), qsl("%1/config").arg(sandbox.path()));
            environment.insert(qsl("XDG_DATA_HOME"), qsl("%1/data").arg(sandbox.path()));
            // Keeps a WITH_SENTRY build's crashpad database out of the real cache
            environment.insert(qsl("XDG_CACHE_HOME"), qsl("%1/cache").arg(sandbox.path()));
            environment.insert(qsl("QT_QPA_PLATFORM"), qsl("offscreen"));
            environment.insert(qsl("MUDLET_TEST_MODE"), qsl("1"));
            // The application deliberately never deletes its QApplication on
            // this path, and Qt's CA store stays loaded for the process
            // lifetime, so a leak check here would only ever report those.
            // Appended rather than replacing what ctest set, since the runtime
            // takes the last setting of a flag and the earlier ones stay.
            const QString inheritedSanitizerOptions = environment.value(qsl("ASAN_OPTIONS"));
            environment.insert(qsl("ASAN_OPTIONS"), inheritedSanitizerOptions.isEmpty() ? qsl("detect_leaks=0") : qsl("%1:detect_leaks=0").arg(inheritedSanitizerOptions));

            QProcess mudlet;
            mudlet.setProcessEnvironment(environment);
            mudlet.setProgram(appBinary());
            mudlet.setArguments({qsl("--version")});
            mudlet.start();

            const QString where = qsl("run %1 of %2").arg(QString::number(run), QString::number(scmRunCount));
            QVERIFY2(mudlet.waitForStarted(scmStartTimeoutMs), qPrintable(qsl("%1: %2").arg(where, mudlet.errorString())));
            QVERIFY2(mudlet.waitForFinished(scmFinishTimeoutMs), qPrintable(qsl("%1: --version never finished").arg(where)));

            const QString output = QString::fromUtf8(mudlet.readAllStandardOutput());
            const QString diagnostics = qsl("%1, stderr:\n%2").arg(where, QString::fromUtf8(mudlet.readAllStandardError()));
            QVERIFY2(mudlet.exitStatus() == QProcess::NormalExit, qPrintable(qsl("mudlet --version crashed on leaving main(); %1").arg(diagnostics)));
            QVERIFY2(mudlet.exitCode() == 0, qPrintable(qsl("mudlet --version exited %1; %2").arg(QString::number(mudlet.exitCode()), diagnostics)));
            // Without this the case would still pass against a main() that
            // returned before ever reaching the warm-up, and cover nothing. The
            // application name is the one part of that banner no translation
            // touches - mudlet, mudlet.exe or Mudlet, depending on the platform
            QVERIFY2(output.contains(qsl("mudlet"), Qt::CaseInsensitive), qPrintable(qsl("no version banner, so main() never reached the version branch; %1").arg(diagnostics)));
        }
    }
};

#include "AppStartupTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(AppStartupTeardownTest)
