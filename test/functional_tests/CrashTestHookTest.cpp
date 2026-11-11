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

// The two halves of crash reporting that only a second process can show.
//
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
//
// The other half is the guard that stops initSentry() from setting up crash reporting under
// MUDLET_TEST_MODE - see initSentry() in src/SentryWrapper.cpp for what that costs when it is
// missing. It needs a child too, because sentry_init() is once-per-process and the control run has
// to happen in a process this one can throw away.

#include <QDir>
#include <QProcess>
#include <QStandardPaths>
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

static constexpr const char* sentryChildEnvironmentVariable = "MUDLET_SENTRY_GUARD_CHILD";

#ifdef WITH_SENTRY
static constexpr const char* databasePathMarker = "sentry-database-path: ";
static constexpr const char* databaseCreatedMarker = "sentry-database-created: ";
static constexpr const char* sentryDatabaseSuffix = "/mudlet/sentry";

static QString markedValue(const QString& output, const char* marker)
{
    const QString prefix = QString::fromUtf8(marker);
    const QStringList lines = output.split(QChar::fromLatin1('\n'));
    for (const QString& line : lines) {
        if (line.startsWith(prefix)) {
            return line.mid(prefix.size()).trimmed();
        }
    }
    return {};
}

// Both the database and the "mudlet" directory holding it, since MUDLET_GROUPED_TEST_MAIN names the
// cache location after this test rather than after Mudlet - nothing of anyone else's lives in there.
static void removeSentryDatabase(const QString& databasePath)
{
    if (!databasePath.endsWith(QString::fromUtf8(sentryDatabaseSuffix))) {
        return;
    }
    QDir database(databasePath);
    database.removeRecursively();
    if (database.cdUp()) {
        database.removeRecursively();
    }
}
#endif

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

    // What a child can see of initSentry() from the outside is the crashpad database, which
    // sentry_init() creates before it starts a backend. That the database is missing is what says
    // the guard returned before any of it.
    void initSentryDoesNothingInTestMode()
    {
#ifndef WITH_SENTRY
        QSKIP("initSentry() only sets up anything in a -DWITH_SENTRY=ON build");
#else
        // The control run first. Without it, an initSentry() that had stopped creating a database
        // for some unrelated reason would pass the guarded run below while guarding nothing.
        QString controlPath;
        bool controlCreatedDatabase = false;
        QString diagnosis;
        QVERIFY2(runSentryChild(false, controlPath, controlCreatedDatabase, diagnosis), qPrintable(diagnosis));
        QVERIFY2(controlCreatedDatabase,
                 qPrintable(qsl("initSentry() created no crashpad database at %1 even with MUDLET_TEST_MODE unset, "
                                "so the guarded run below would prove nothing")
                                    .arg(controlPath)));

        QString guardedPath;
        bool guardedCreatedDatabase = true;
        QVERIFY2(runSentryChild(true, guardedPath, guardedCreatedDatabase, diagnosis), qPrintable(diagnosis));
        QVERIFY2(!guardedCreatedDatabase, qPrintable(qsl("initSentry() ran under MUDLET_TEST_MODE - it got as far as creating its crashpad database at %1").arg(guardedPath)));
#endif
    }

    void reportsWhetherInitSentryCreatedItsDatabase()
    {
        if (qEnvironmentVariable(sentryChildEnvironmentVariable) != qsl("1")) {
            QSKIP("runs only as the child of initSentryDoesNothingInTestMode()");
        }

#ifdef WITH_SENTRY
        // MUDLET_GROUPED_TEST_MAIN names the application after this class, so the cache location
        // initSentry() builds its database path from belongs to this test rather than to whoever
        // is running it - safe to delete, and empty of any real crash report.
        const QString databasePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QString::fromUtf8(sentryDatabaseSuffix);
        QDir(databasePath).removeRecursively();
        QVERIFY(!QDir(databasePath).exists());

        // Announced before initSentry() runs, so the parent can clean up after a child that died
        // inside it.
        std::fputs(qPrintable(QString::fromUtf8(databasePathMarker) + databasePath), stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);

        initSentry();

        std::fputs(qPrintable(QString::fromUtf8(databaseCreatedMarker) + (QDir(databasePath).exists() ? qsl("yes") : qsl("no"))), stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);
#endif

        // _Exit for crashesOnRequest()'s reason, and for one of its own: the control run leaves
        // sentry initialised on purpose, so LeakSanitizer would report what sentry_init allocated
        // and fail a child that did exactly what it was asked to.
        std::_Exit(0);
    }

private:
#ifdef WITH_SENTRY
    bool runSentryChild(const bool testMode, QString& databasePath, bool& createdDatabase, QString& diagnosis)
    {
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QString::fromUtf8(sentryChildEnvironmentVariable), qsl("1"));
        environment.insert(qsl("QTEST_DISABLE_STACK_DUMP"), qsl("1"));
        if (testMode) {
            environment.insert(qsl("MUDLET_TEST_MODE"), qsl("1"));
        } else {
            // Removed rather than left unset: ctest sets it for every functional test, so the
            // control run inherits it from this process.
            environment.remove(qsl("MUDLET_TEST_MODE"));
        }

        QProcess child;
        child.setProcessEnvironment(environment);
        child.start(QCoreApplication::applicationFilePath(), {qsl("CrashTestHookTest"), qsl("reportsWhetherInitSentryCreatedItsDatabase")});
        const bool finished = child.waitForStarted(15000) && child.waitForFinished(30000);

        const QString output = QString::fromUtf8(child.readAllStandardOutput());
        databasePath = markedValue(output, databasePathMarker);
        const QString createdAnswer = markedValue(output, databaseCreatedMarker);
        createdDatabase = (createdAnswer == qsl("yes"));
        // Ahead of any verdict, since the caller's QVERIFY2 returns on a failure rather than
        // reaching a cleanup below it - and a child that died inside initSentry() is exactly the
        // case the path was announced early for.
        removeSentryDatabase(databasePath);

        if (!finished) {
            diagnosis = qsl("the child never finished (MUDLET_TEST_MODE %1)").arg(testMode ? qsl("set") : qsl("unset"));
            return false;
        }
        if (databasePath.isEmpty() || createdAnswer.isEmpty()) {
            diagnosis = qsl("the child said nothing about the crashpad database (MUDLET_TEST_MODE %1)\nstdout: %2\nstderr: %3")
                                .arg(testMode ? qsl("set") : qsl("unset"), output, QString::fromUtf8(child.readAllStandardError()));
            return false;
        }

        return true;
    }
#endif
};

#include "CrashTestHookTest.moc"
MUDLET_GROUPED_TEST_MAIN(CrashTestHookTest)
