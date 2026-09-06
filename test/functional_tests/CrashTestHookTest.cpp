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

// MUDLET_CRASH_TEST=1 has to kill Mudlet the moment server text reaches a buffer, since that is the
// only way we have of exercising crash reporting end to end. The crash it fakes is a write through a
// null pointer, which is undefined behaviour the compiler may delete outright - clang did, leaving
// the hook a no-op in every optimised clang build while gcc kept it and Linux CI stayed green.
// Nothing but a real process shows the difference, so this runs one: the child is this same binary,
// running crashesOnRequest() alone. It lives with the telnet tests because the hook is called from
// TBuffer::translateToPlainText, on text the server sent.
//
// The test tree is built unoptimised, where the store survives either way; what would catch a
// regression is Windows, which builds Release with clang on every pull request, and the optimised
// tag builds.

#include <QProcess>
#include <QtTest/QtTest>

#include "GroupedTest.h"
#include "SentryWrapper.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>

// Announced before the hook is touched, so that a child which died on the way there - a failed
// spawn, no display - cannot be mistaken for one the hook killed.
static constexpr const char* reachedHookMarker = "reached-the-crash-hook";
static constexpr const char* childEnvironmentVariable = "MUDLET_CRASH_TEST_CHILD";
static constexpr int survivedExitCode = 66;

class CrashTestHookTest : public QObject
{
    Q_OBJECT

private slots:
    void crashHookKillsTheProcess()
    {
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(qsl("MUDLET_CRASH_TEST"), qsl("1"));
        environment.insert(QString::fromUtf8(childEnvironmentVariable), qsl("1"));
        // Qt Test answers a crash by launching gdb or lldb for a backtrace this test has no use for
        // - 2.4s against 0.08s here, and an attach on a loaded runner can take far longer than that.
        environment.insert(qsl("QTEST_DISABLE_STACK_DUMP"), qsl("1"));

        QProcess child;
        child.setProcessEnvironment(environment);
        child.start(QCoreApplication::applicationFilePath(), {qsl("CrashTestHookTest"), qsl("crashesOnRequest")});
        // Both waits have to expire well inside the 60s ctest gives the case, or ctest kills this
        // process first and the assertions below never get to say what went wrong.
        QVERIFY(child.waitForStarted(15000));
        QVERIFY2(child.waitForFinished(30000), "the child neither crashed nor returned");

        const QString output = QString::fromUtf8(child.readAllStandardOutput());
        QVERIFY2(output.contains(QString::fromUtf8(reachedHookMarker)),
                 qPrintable(qsl("the child never reached crashIfRequested()\nstdout: %1\nstderr: %2").arg(output, QString::fromUtf8(child.readAllStandardError()))));
        // A sanitizer intercepts the segmentation fault and exits normally with a code of its own,
        // so what says the hook worked is the child never coming back from it. Exiting 0 counts as
        // coming back just as much as the marker code does, in case the hook is ever rewritten to
        // end the process politely - that would report no crash to Sentry at all.
        const bool cameBack = child.exitStatus() == QProcess::NormalExit && (child.exitCode() == survivedExitCode || child.exitCode() == 0);
        QVERIFY2(!cameBack, "the child came back from crashIfRequested() with MUDLET_CRASH_TEST=1 set - the crash was optimised away");
    }

    void crashesOnRequest()
    {
        if (qEnvironmentVariable(childEnvironmentVariable) != qsl("1")) {
            QSKIP("runs only as the child of crashHookKillsTheProcess()");
        }

#ifdef Q_OS_WIN
        // Otherwise the access violation raises Windows' error reporting dialog and a CI runner sits
        // on it until the test times out.
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif

        std::fputs(reachedHookMarker, stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);

        crashIfRequested();

        // _Exit rather than exit: the Linux CI leg runs this under LeakSanitizer, whose atexit
        // handler would report the abandoned QApplication and swap this code for its own - which
        // the parent would read as a crash, passing the test on a hook that did nothing.
        std::_Exit(survivedExitCode);
    }
};

#include "CrashTestHookTest.moc"
MUDLET_GROUPED_TEST_MAIN(CrashTestHookTest)
