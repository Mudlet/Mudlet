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

// mudlet-headless: one profile with no window, its main console on standard output. It links mudlet_core
// alone, so it is also the check that the core runs without Qt Widgets.

#include "Host.h"
#include "MudletApp.h"
#include "SecureStringUtils.h"
#include "THeadlessSession.h"
#include "TriggerMatchPool.h"
#include "utils.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QMutex>
#include <QPointer>
#include <QWaitCondition>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstring>
#include <csignal>
#include <cstdio>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

#if defined(Q_OS_WINDOWS)
#include <QVarLengthArray>

#include <cstdlib>
#include <windows.h>
#else
#include <QSocketNotifier>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif

// A Qt Resource Collection in a static library only reaches the executable if something here names it
extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

namespace {

enum ExitCode { exitOk = 0, exitFailed = 1, exitUsage = 2, exitOutputFailed = 3, exitDisconnected = 4, exitSignalBase = 128 };

void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#if !defined(Q_OS_WINDOWS)
int gSignalPipe[2] = {-1, -1};

// How long the writes may still wait, all told, once a stop signal comes: a slow reader still gets the last lines,
// and one that has stopped reading, as `| less` at its prompt has, cannot hold up the save. The rest is dropped.
constexpr qint64 scmStopWaitMs = 500;
// When that time is up, on the steady clock; 0 until a stop signal comes. Read by the writes, from Qt's threads too.
std::atomic<qint64> gStopDeadlineMs = 0;

qint64 steadyMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void beginStop()
{
    qint64 notYet = 0;
    gStopDeadlineMs.compare_exchange_strong(notYet, steadyMs() + scmStopWaitMs);
}

// What poll() answers for the descriptor once it is ready, or 0 with errno set; EINTR for a write the stop's
// time ran out on
short waitFor(const int descriptor, const short events)
{
    const bool writing = events & POLLOUT;
    pollfd ready[2] = {{descriptor, events, 0}, {gSignalPipe[0], POLLIN, 0}};
    while (true) {
        const qint64 deadline = writing ? gStopDeadlineMs.load() : 0;
        const nfds_t count = writing && !deadline && gSignalPipe[0] >= 0 ? 2 : 1;
        // Not for ever while a signal may come: the main thread can take it off the pipe before this one wakes
        const int timeoutMs = deadline ? static_cast<int>(std::max<qint64>(deadline - steadyMs(), 0)) : (count == 2 ? 100 : -1);
        if (::poll(ready, count, timeoutMs) < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        if (ready[0].revents) {
            return ready[0].revents;
        }
        if (count == 2 && (ready[1].revents & POLLIN)) {
            beginStop();
        } else if (deadline && !timeoutMs) {
            errno = EINTR;
            return 0;
        }
    }
}

// Before each write: one to a full pipe blocks with nothing to interrupt it, and POLLOUT promises room for the
// PIPE_BUF bytes written at a time. A POLLNVAL, which macOS answers at once for a terminal, is left to the write.
bool waitToWrite(const int descriptor)
{
    return waitFor(descriptor, POLLOUT) != 0;
}

// Waits out a descriptor inherited non-blocking, as from a parent that set it on a terminal it shares
bool retryAfterWaiting(const int descriptor, const short events)
{
    if (errno == EINTR) {
        return true;
    }
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        return false;
    }
    const int waited = errno;
    const short answered = waitFor(descriptor, events);
    if (!answered) {
        if (errno != EINTR) {
            errno = waited;
        }
        return false;
    }
    // POLLERR and POLLHUP are left to the retried call, which says what ended the stream (EPIPE, or 0 at the end).
    // macOS answers POLLNVAL at once for a terminal, so that is waited out in pauses rather than spun on.
    thread_local int pauseMs = 1;
    if (!(answered & POLLNVAL)) {
        pauseMs = 1;
        return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));
    pauseMs = std::min(pauseMs * 2, 50);
    return true;
}
#endif

// 0, or the errno the write failed with
int writeBytes(FILE* stream, const QByteArray& bytes)
{
#if defined(Q_OS_WINDOWS)
    const auto length = static_cast<size_t>(bytes.size());
    errno = 0;
    _doserrno = 0;
    if (fwrite(bytes.constData(), 1, length, stream) == length && fflush(stream) == 0) {
        return 0;
    }
    // The CRT reports a pipe whose reader has gone as EINVAL; what happened is in _doserrno
    if (_doserrno == ERROR_NO_DATA || _doserrno == ERROR_BROKEN_PIPE) {
        return EPIPE;
    }
    return errno ? errno : EIO;
#else
    // Not through stdio, which drops what a non-blocking descriptor cannot take at once. Both streams
    // are unbuffered, so stdio holds nothing to write first.
    const int descriptor = fileno(stream);
    qsizetype done = 0;
    while (done < bytes.size()) {
        if (!waitToWrite(descriptor)) {
            return errno;
        }
        const ssize_t written = ::write(descriptor, bytes.constData() + done, static_cast<size_t>(std::min<qsizetype>(bytes.size() - done, PIPE_BUF)));
        if (written >= 0) {
            done += written;
        } else if (!retryAfterWaiting(descriptor, POLLOUT)) {
            return errno;
        }
    }
    return 0;
#endif
}

int writeLine(FILE* stream, const QString& text)
{
    return writeBytes(stream, text.toUtf8() + '\n');
}

QString outputErrorMessage(const int error)
{
    //: %1 is the name of the executable, %2 is the system's reason, such as "No space left on device"
    return QCoreApplication::translate("headless", "%1: cannot write to standard output: %2").arg(QLatin1String(APP_TARGET), QString::fromLocal8Bit(std::strerror(error)));
}

// Standard error carries the runner's messages, Qt's log, and the profile's local text and error console in pieces,
// one left mid-line ended before another source writes. Its own write failures have nowhere to be reported.
class ErrorStream
{
public:
    enum class Source { runner, local, errorConsole };

    void write(const Source source, const QString& text)
    {
        if (text.isEmpty()) {
            return;
        }
        QByteArray bytes = text.toUtf8();
        const QMutexLocker lock(&mMutex);
        if (mMidLine && source != mSource) {
            bytes.prepend('\n');
        }
        writeBytes(stderr, bytes);
        mSource = source;
        mMidLine = !text.endsWith(QChar::LineFeed);
    }
    void report(const QString& message) { write(Source::runner, qsl("%1: %2\n").arg(QLatin1String(APP_TARGET), message)); }
    void endLine()
    {
        const QMutexLocker lock(&mMutex);
        endLineLocked();
    }
    // From any thread, as Qt logs from its own. Not through the handler that was there before, which writes
    // with nothing to end a wait on a reader that has stopped reading.
    void log(const QtMsgType type, const QMessageLogContext& context, const QString& message)
    {
        const QByteArray bytes = qFormatLogMessage(type, context, message).toUtf8() + '\n';
        const QMutexLocker lock(&mMutex);
        endLineLocked();
        writeBytes(stderr, bytes);
    }

private:
    void endLineLocked()
    {
        if (mMidLine) {
            writeBytes(stderr, QByteArrayLiteral("\n"));
            mMidLine = false;
        }
    }

    QMutex mMutex;
    Source mSource = Source::runner;
    bool mMidLine = false;
};

// Never destroyed while a Qt thread may still be logging through it
ErrorStream& errorStream()
{
    static ErrorStream* const errors = new ErrorStream;
    return *errors;
}

void logThroughErrorStream(const QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    errorStream().log(type, context, message);
}

// The reader blocks in a read that nothing portable interrupts, so it is never joined; a null receiver,
// set under the lock, stops it posting once the run is over
struct InputChannel
{
    QMutex mutex;
    QObject* receiver = nullptr;
    // Lines posted and not yet taken: the reader waits at the bound, so a file piped in is not all read into
    // memory ahead of the aliases and the game taking it
    int queued = 0;
    QWaitCondition drained;
    std::function<void(const QString&)> onLine;
    // With why the read failed, if it did
    std::function<void(const QString&)> onEnd;
};

constexpr int scmMaxQueuedLines = 1024;

// Hands a line, or the end of the input with the reason a failed read gives, to the main thread; false once
// nothing takes it
bool postInput(const std::shared_ptr<InputChannel>& channel, const QString& text, const bool ended)
{
    const QMutexLocker lock(&channel->mutex);
    while (channel->receiver && channel->queued >= scmMaxQueuedLines) {
        channel->drained.wait(&channel->mutex);
    }
    if (!channel->receiver) {
        return false;
    }
    ++channel->queued;
    QMetaObject::invokeMethod(
            channel->receiver,
            [channel, text, ended]() {
                {
                    const QMutexLocker taken(&channel->mutex);
                    --channel->queued;
                    channel->drained.wakeOne();
                }
                if (ended) {
                    channel->onEnd(text);
                } else {
                    channel->onLine(text);
                }
            },
            Qt::QueuedConnection);
    return true;
}

// Not through stdio: exit() flushes every stream under its lock, which a reader blocked in stdin holds. What
// is read is UTF-8, as a redirected input is taken to be; a console hands over text in its own code page, so
// it is read as UTF-16 instead and converted.
qint64 readInput(char* buffer, const qint64 size)
{
#if defined(Q_OS_WINDOWS)
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    const bool console = GetConsoleMode(input, &mode);
    // A UTF-16 unit takes up to three bytes of UTF-8, and a surrogate pair four for two
    static QVarLengthArray<wchar_t, 1366> wide;
    // A high surrogate the last read ended on, waiting for its pair
    static wchar_t heldSurrogate = 0;
    const int held = heldSurrogate ? 1 : 0;
    DWORD got = 0;
    BOOL read = FALSE;
    // A console read that Ctrl+C cuts short, with or without failing, is not the end of the input; the stop
    // comes as a console event
    do {
        SetLastError(ERROR_SUCCESS);
        if (console) {
            wide.resize(static_cast<int>(size / 3));
            wide[0] = heldSurrogate;
            read = ReadConsoleW(input, wide.data() + held, static_cast<DWORD>(wide.size() - held), &got, nullptr);
        } else {
            read = ReadFile(input, buffer, static_cast<DWORD>(size), &got, nullptr);
        }
    } while ((!read || got == 0) && GetLastError() == ERROR_OPERATION_ABORTED);
    // What a pipe whose writer has closed it answers with
    if (!read && GetLastError() == ERROR_BROKEN_PIPE) {
        return 0;
    }
    if (!read || !console) {
        return read ? static_cast<qint64>(got) : -1;
    }
    got += held;
    // The end of the input, which ReadFile() answers with no bytes, is a line beginning with Ctrl+Z here
    if (got > static_cast<DWORD>(held) && wide[held] == 0x1A) {
        return 0;
    }
    heldSurrogate = got && QChar::isHighSurrogate(wide[static_cast<int>(got) - 1]) ? wide[static_cast<int>(got) - 1] : 0;
    const QByteArray utf8 = QString::fromWCharArray(wide.constData(), static_cast<qsizetype>(got) - (heldSurrogate ? 1 : 0)).toUtf8();
    std::memcpy(buffer, utf8.constData(), static_cast<size_t>(utf8.size()));
    return utf8.size();
#else
    ssize_t got = 0;
    do {
        got = ::read(STDIN_FILENO, buffer, static_cast<size_t>(size));
    } while (got < 0 && retryAfterWaiting(STDIN_FILENO, POLLIN));
    return got;
#endif
}

void readStandardInput(const std::shared_ptr<InputChannel>& channel)
{
    QByteArray pending;
    char buffer[4096];
    qint64 got = 0;
    while ((got = readInput(buffer, sizeof(buffer))) > 0) {
        pending.append(buffer, got);
        qsizetype end = 0;
        while ((end = pending.indexOf('\n')) >= 0) {
            if (!postInput(channel, QString::fromUtf8(pending.first(end)), false)) {
                return;
            }
            pending.remove(0, end + 1);
        }
    }
    // Taken before anything else can change errno, or the last error on Windows
    const QString failure = got < 0 ? qt_error_string() : QString();
    // Only the end of the input ends a line; a failed read may have cut this one short
    if (!got && !pending.isEmpty() && !postInput(channel, QString::fromUtf8(pending), false)) {
        return;
    }
    postInput(channel, failure, true);
}

QString uncaughtSignalsMessage(const QString& reason)
{
    //: Warning from the headless runner; %1 is the system's reason
    return QCoreApplication::translate("headless", "cannot catch SIGINT, SIGTERM and SIGHUP, which will end the run without a save: %1").arg(reason);
}

#if defined(Q_OS_WINDOWS)
std::function<void(int)> gOnStopSignal;

std::atomic_bool gStopSignalled = false;
// The first one's number, for the main thread to act on before the event the handler posts is delivered
std::atomic_int gFirstStopSignal = 0;

BOOL WINAPI forwardConsoleEvent(const DWORD type)
{
    // The default handler ends the process, for a second Ctrl+C while the first one's save runs
    if (gStopSignalled.exchange(true) && (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT)) {
        return FALSE;
    }
    // As the POSIX signals the exit code names: Ctrl+C, Ctrl+Break, and a closed console, logoff or shutdown
    const int number = type == CTRL_C_EVENT ? SIGINT : (type == CTRL_BREAK_EVENT ? SIGTERM : 1);
    int none = 0;
    gFirstStopSignal.compare_exchange_strong(none, number);
    QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [number]() {
                gOnStopSignal(number);
            },
            Qt::QueuedConnection);
    // Windows ends the process once this returns from the last three, so the save gets their timeout instead
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) {
        Sleep(INFINITE);
    }
    return TRUE;
}
#else
void forwardSignal(const int number)
{
    const int savedErrno = errno;
    // Here, not once the main thread reads the pipe, as that can be busy in a script that never returns. Not
    // SIGHUP: a closed terminal can send it twice, and the second must not cut the save short.
    for (const int caught : {SIGINT, SIGTERM}) {
        struct sigaction current = {};
        if (sigaction(caught, nullptr, &current) == 0 && current.sa_handler == forwardSignal) {
            signal(caught, SIG_DFL);
        }
    }
    const char byte = static_cast<char>(number);
    // A full pipe already holds a signal to act on
    [[maybe_unused]] const auto written = ::write(gSignalPipe[1], &byte, 1);
    errno = savedErrno;
}

QString systemReason()
{
    return QString::fromLocal8Bit(std::strerror(errno));
}

// Leaves alone one that was ignored when the run began, as under nohup. Returns what could not be done.
QStringList catchStopSignals()
{
    if (::pipe(gSignalPipe) != 0) {
        return {uncaughtSignalsMessage(systemReason())};
    }
    // POSIX promises only "not -1" for success here. A blocking pipe would stall a handler writing to a full one,
    // and the main thread reading an empty one.
    if (::fcntl(gSignalPipe[0], F_SETFD, FD_CLOEXEC) == -1 || ::fcntl(gSignalPipe[1], F_SETFD, FD_CLOEXEC) == -1 || ::fcntl(gSignalPipe[0], F_SETFL, O_NONBLOCK) == -1
        || ::fcntl(gSignalPipe[1], F_SETFL, O_NONBLOCK) == -1) {
        const QString reason = systemReason();
        ::close(gSignalPipe[0]);
        ::close(gSignalPipe[1]);
        gSignalPipe[0] = gSignalPipe[1] = -1;
        return {uncaughtSignalsMessage(reason)};
    }
    QStringList problems;
    struct sigaction action = {};
    action.sa_handler = forwardSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    for (const auto& [number, name] : {std::pair{SIGINT, "SIGINT"}, std::pair{SIGTERM, "SIGTERM"}, std::pair{SIGHUP, "SIGHUP"}}) {
        struct sigaction current = {};
        const bool queried = sigaction(number, nullptr, &current) == 0;
        if (queried && current.sa_handler == SIG_IGN) {
            continue;
        }
        if (!queried || sigaction(number, &action, nullptr) != 0) {
            //: Warning from the headless runner; %1 is the signal's name, such as SIGHUP, %2 is the system's reason
            problems << QCoreApplication::translate("headless", "cannot catch %1, which will end the run without a save: %2").arg(QLatin1String(name), systemReason());
        }
    }
    return problems;
}
#endif

void setApplicationNames()
{
    // As the GUI's, so that both share the config and the keychain entries
    QCoreApplication::setOrganizationName(qsl("Mudlet"));
    QCoreApplication::setApplicationName(MudletApp::publicTest() ? qsl("Mudlet Public Test Build") : qsl("Mudlet"));
    QCoreApplication::setApplicationVersion(MudletApp::release() ? QString(APP_VERSION) : QString(APP_VERSION) + MudletApp::buildSuffix());
}

QString helpText()
{
    //: Command line help of the headless runner; %1 is the name of the executable
    return QCoreApplication::translate("headless",
                                       "Usage: %1 -p <profile> [options]\n"
                                       "Runs a Mudlet profile with no window. The lines the game sends go to standard\n"
                                       "output, and each line read from standard input is sent as if typed on the\n"
                                       "command line. What the profile shows only on its own screen, such as echo(),\n"
                                       "the commands sent and the error console, goes to standard error.\n"
                                       "\n"
                                       "Options:\n"
                                       "  -p, --profile <profile>  the profile to open, and connect unless --offline is\n"
                                       "                           given; with --host, it is made if there is none.\n"
                                       "  --host <host>            the game server to connect to, stored in the profile\n"
                                       "                           as the connection dialog stores it; a profile that\n"
                                       "                           does not exist yet is made, and set up as the dialog\n"
                                       "                           sets one up.\n"
                                       "  --port <port>            the game server's port, stored likewise.\n"
                                       "  --tls, --no-tls          connects with TLS, or without it, stored likewise.\n"
                                       "  --login <name>           the character name to log in with, stored likewise.\n"
                                       "  --password-env <var>     the environment variable holding the character's\n"
                                       "                           password, for this run only; it is not stored. One\n"
                                       "                           that is not set or is empty is a command line error.\n"
                                       "  --offline                opens the profile without connecting, as the dialog's\n"
                                       "                           Offline button does.\n"
                                       "  --package <file>         installs a package once the profile has loaded; give\n"
                                       "                           it more than once to install several. One already\n"
                                       "                           installed under the same name is left as it is,\n"
                                       "                           working or not, and said so on standard error.\n"
                                       "  -e, --exec <lua>         runs Lua code once the profile has loaded; give it\n"
                                       "                           more than once to run several pieces in order.\n"
                                       "  --no-stdin               does not read standard input, as for a background\n"
                                       "                           run; other than on Windows, one closed when the\n"
                                       "                           run begins is taken for this.\n"
                                       "  --exit-on-disconnect     exits once the connection to the game ends or cannot\n"
                                       "                           be made.\n"
                                       "  -h, --help               displays this message.\n"
                                       "  -v, --version            displays version information.\n"
                                       "\n"
                                       "Standard input is read once the connection is made, so that no line of it goes\n"
                                       "before the game can take it; one read while there is no connection, as after it\n"
                                       "failed or ended, runs the profile's aliases but is not sent. With --offline it\n"
                                       "is read at once.\n"
                                       "\n"
                                       "Packages and -e Lua usually run before the connection is made, so a command\n"
                                       "they send the game may be lost; send it from a sysConnectionEvent handler\n"
                                       "instead.\n"
                                       "\n"
                                       "The profile is saved as it closes; when closeProfile() closes it, only if its\n"
                                       "\"Auto save on exit\" setting is on. Exit status:\n"
                                       "  0      the profile closed, as closeMudlet() or closeProfile() do, standard\n"
                                       "         input ended, or standard output's reader went away.\n"
                                       "  1      the profile could not be opened, it, or a profile one of its scripts\n"
                                       "         opened, could not be saved, a package could not be installed or not\n"
                                       "         all of it works, Lua given with -e failed, without --offline a\n"
                                       "         package or -e Lua sent the game something before the connection was\n"
                                       "         made, standard input could not be read, or a line of it was refused\n"
                                       "         while another profile loaded or, without --offline, sent the game\n"
                                       "         something while there was no connection.\n"
                                       "  2      the command line was wrong, such as a new profile's name, or\n"
                                       "         connection details, the connection dialog would not take.\n"
                                       "  3      standard output was closed when the run began, or could not be\n"
                                       "         written, other than by its reader going away, as `| head` does.\n"
                                       "  4      the connection ended or could not be made, with --exit-on-disconnect;\n"
                                       "         reconnect() ends it too.\n"
                                       "  128+n  signal n (SIGHUP, SIGINT or SIGTERM) ended the run.\n"
                                       "Where more than one applies, the first of these is given: 1 for a profile that\n"
                                       "could not be saved, then 128+n, 3, 4, and 1 for the rest. A profile that could\n"
                                       "not be opened always gives 1, or 2 as above.\n"
                                       "Report bugs to: https://github.com/Mudlet/Mudlet/issues")
            .arg(QLatin1String(APP_TARGET));
}

// The GUI's --version text, under this executable's name
QString versionText()
{
    QStringList texts;
#if defined(QT_DEBUG)
    texts << QCoreApplication::translate("main",
                                         "%1 %2%3 (with debug symbols, without optimisations)",
                                         "%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev")
                     .arg(QLatin1String(APP_TARGET), QLatin1String(APP_VERSION), MudletApp::buildSuffix());
#else
    texts << qsl("%1 %2%3").arg(QLatin1String(APP_TARGET), QLatin1String(APP_VERSION), MudletApp::buildSuffix());
#endif
    texts << QCoreApplication::translate("main", "Qt libraries %1 (compilation) %2 (runtime)", "%1 and %2 are version numbers").arg(QLatin1String(QT_VERSION_STR), qVersion());
    // PLACEMARKER: Date-stamp needing annual update
    //: Part of the --version text
    texts << QCoreApplication::translate("main", "Copyright © 2008-2026  Mudlet developers");
    //: Part of the --version text
    texts << QCoreApplication::translate("main", "Licence GPLv3: GNU GPL version 3 - http://gnu.org/licenses/gpl.html");
    //: Part of the --version text
    texts << QCoreApplication::translate("main",
                                         "This is free software: you are free to change and redistribute it.\n"
                                         "There is NO WARRANTY, to the extent permitted by law.");
    return texts.join(QLatin1Char('\n'));
}

int usageError(const QString& message)
{
    //: %1 is the name of the executable, %2 says what was wrong with the command line
    writeLine(stderr, QCoreApplication::translate("headless", "%1: %2\nTry '%1 --help' for more information.").arg(QLatin1String(APP_TARGET), message));
    return exitUsage;
}

} // namespace

int main(int argc, char* argv[])
{
#if !defined(Q_OS_WINDOWS)
    // One of these the run began without would be the next descriptor opened, such as Qt's event descriptor,
    // which the game's lines would then be written into, or standard input read from
    bool standardInputClosed = false;
    bool standardOutputClosed = false;
    for (const int descriptor : {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO}) {
        if (::fcntl(descriptor, F_GETFD) != -1 || errno != EBADF) {
            continue;
        }
        standardInputClosed = standardInputClosed || descriptor == STDIN_FILENO;
        standardOutputClosed = standardOutputClosed || descriptor == STDOUT_FILENO;
        // The lowest free descriptor, so this one; without it the run cannot be kept off whatever takes its place
        if (::open("/dev/null", descriptor == STDIN_FILENO ? O_RDONLY : O_WRONLY) != descriptor) {
            return exitFailed;
        }
    }
    // Lua's io.write() shares stdout with writeBytes(); buffered, what it wrote would come out only at stdio's
    // next flush, behind game lines written since
    setvbuf(stdout, nullptr, _IONBF, 0);
#else
    const bool standardInputClosed = false;
    const bool standardOutputClosed = false;
#endif
    initializeQRCResources();

    QCommandLineParser parser;
    const QCommandLineOption profileOption(QStringList{qsl("p"), qsl("profile")}, qsl("Profile to open"), qsl("profile"));
    parser.addOption(profileOption);
    const QCommandLineOption helpOption(QStringList{qsl("h"), qsl("help")}, qsl("Display help and exit"));
    parser.addOption(helpOption);
    const QCommandLineOption versionOption(QStringList{qsl("v"), qsl("version")}, qsl("Display version and exit"));
    parser.addOption(versionOption);
    const QCommandLineOption execOption(QStringList{qsl("e"), qsl("exec")}, qsl("Lua to run once the profile has loaded"), qsl("lua"));
    parser.addOption(execOption);
    const QCommandLineOption noStdinOption(qsl("no-stdin"), qsl("Do not read standard input"));
    parser.addOption(noStdinOption);
    const QCommandLineOption exitOnDisconnectOption(qsl("exit-on-disconnect"), qsl("Exit once the connection ends"));
    parser.addOption(exitOnDisconnectOption);
    const QCommandLineOption hostOption(qsl("host"), qsl("Game server to connect to"), qsl("host"));
    parser.addOption(hostOption);
    const QCommandLineOption portOption(qsl("port"), qsl("Port of the game server"), qsl("port"));
    parser.addOption(portOption);
    const QCommandLineOption tlsOption(qsl("tls"), qsl("Connect with TLS"));
    parser.addOption(tlsOption);
    const QCommandLineOption noTlsOption(qsl("no-tls"), qsl("Connect without TLS"));
    parser.addOption(noTlsOption);
    const QCommandLineOption loginOption(qsl("login"), qsl("Character name to log in with"), qsl("name"));
    parser.addOption(loginOption);
    const QCommandLineOption passwordEnvOption(qsl("password-env"), qsl("Environment variable holding the character's password"), qsl("variable"));
    parser.addOption(passwordEnvOption);
    const QCommandLineOption offlineOption(qsl("offline"), qsl("Open the profile without connecting"));
    parser.addOption(offlineOption);
    const QCommandLineOption packageOption(qsl("package"), qsl("Package to install once the profile has loaded"), qsl("file"));
    parser.addOption(packageOption);

    // Help and version need no platform plugin, so they are answered before there is one. Qt's own options
    // are still in this list, so it is read leniently, and single-dash words whole, as the GUI's src/main.cpp
    // does.
    QStringList rawArguments;
    rawArguments.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        rawArguments << QString::fromLocal8Bit(argv[i]);
    }
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.parse(rawArguments);
#if !defined(Q_OS_WINDOWS)
    // A reader that goes away (`mudlet-headless -p x | head`) would otherwise kill the profile mid-session.
    // Caught, not ignored: an ignored signal stays ignored in every program a script starts.
    signal(SIGPIPE, [](int) {});
#endif
    if (parser.isSet(helpOption) || parser.isSet(versionOption)) {
        // Only for the translation, as in the GUI
        QCoreApplication printApp(argc, argv);
        setApplicationNames();
        MudletApp::loadTranslationsForCommandLine(true);
        if (const int error = standardOutputClosed ? EBADF : writeLine(stdout, parser.isSet(helpOption) ? helpText() : versionText()); error && error != EPIPE) {
            writeLine(stderr, outputErrorMessage(error));
            return exitOutputFailed;
        }
        return exitOk;
    }

    // Nothing is drawn, so no display is needed unless one is asked for
    const bool platformChosenHere = qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM");
    if (platformChosenHere) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    // Deleted before main() returns, as the GUI's src/main.cpp does its own, so Qt's cleanup runs ahead of static
    // destruction
    auto* app = new QGuiApplication(argc, argv);
    // Read only by the constructor, and a program a script starts would come up with no window of its own
    if (platformChosenHere) {
        qunsetenv("QT_QPA_PLATFORM");
    }
    setApplicationNames();
    // Kept for the whole run: with no mudlet object, nothing loads the interface's translations later
    MudletApp::loadTranslationsForCommandLine(false);
    MudletApp::setDefaultMediaBackend();

    // Strict this time: the application object has taken out the options that were Qt's
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);
    int exitCode = exitOk;
    if (!parser.parse(QCoreApplication::arguments())) {
        exitCode = usageError(parser.errorText());
    } else if (!parser.positionalArguments().isEmpty()) {
        //: %1 is what was given on the command line that is not an option
        exitCode = usageError(QCoreApplication::translate("headless", "Unexpected argument '%1'.").arg(parser.positionalArguments().constFirst()));
    } else if (parser.value(profileOption).trimmed().isEmpty()) {
        //: Command line error of the headless runner; -p is the option that names the profile to run
        exitCode = usageError(QCoreApplication::translate("headless", "No profile was given; name one with -p."));
    } else if (parser.isSet(tlsOption) && parser.isSet(noTlsOption)) {
        //: Command line error of the headless runner
        exitCode = usageError(QCoreApplication::translate("headless", "--tls and --no-tls cannot both be given."));
    }
    // Last-wins would quietly drop one; for -p the GUI would open each
    for (const QCommandLineOption* option : {&profileOption, &hostOption, &portOption, &loginOption, &passwordEnvOption}) {
        if (exitCode == exitOk && parser.values(*option).size() > 1) {
            //: Command line error of the headless runner; %1 is an option, such as --host
            exitCode = usageError(QCoreApplication::translate("headless", "%1 was given more than once.").arg(qsl("--") + option->names().constLast()));
        }
    }
    // Empty, as from a variable that is not set, is refused rather than taken for not given, which is leaving it out
    for (const auto& [option, given] : {std::pair{&hostOption, parser.value(hostOption).trimmed()}, std::pair{&loginOption, parser.value(loginOption).trimmed()}}) {
        if (exitCode == exitOk && parser.isSet(*option) && given.isEmpty()) {
            //: Command line error of the headless runner; %1 is an option, such as --host
            exitCode = usageError(QCoreApplication::translate("headless", "%1 was given nothing; leave it out to keep what the profile has stored.").arg(qsl("--") + option->names().constFirst()));
        }
    }
    THeadlessSession::Options options{.profile = parser.value(profileOption), .connect = !parser.isSet(offlineOption), .host = parser.value(hostOption), .login = parser.value(loginOption)};
    if (parser.isSet(tlsOption) || parser.isSet(noTlsOption)) {
        options.tls = parser.isSet(tlsOption);
    }
    if (bool isNumber = false; exitCode == exitOk && parser.isSet(portOption)) {
        options.port = parser.value(portOption).toInt(&isNumber);
        // Its range is the session's to check, as for any frontend; 0 there keeps what is stored
        if (!isNumber || !options.port) {
            //: Command line error of the headless runner; %1 is what was given as the port
            exitCode = usageError(QCoreApplication::translate("headless", "The port must be a number from 1 to 65535, not '%1'.").arg(parser.value(portOption)));
        }
    }
    if (exitCode == exitOk && parser.isSet(passwordEnvOption)) {
        // Read, never shown: the variable's name is all that is ever reported
        const QByteArray variable = parser.value(passwordEnvOption).toLocal8Bit();
        options.password = qEnvironmentVariable(variable.constData());
        // Before any script runs, as os.getenv() and the programs os.execute() starts would read it
        qunsetenv(variable.constData());
        if (options.password.isEmpty()) {
            //: Command line error of the headless runner; %1 is the name of an environment variable
            exitCode = usageError(
                    QCoreApplication::translate("headless", "The environment variable %1 is not set or is empty, so there is no password to log in with.").arg(parser.value(passwordEnvOption)));
        }
    }
    if (exitCode == exitOk && standardOutputClosed) {
        writeLine(stderr, outputErrorMessage(EBADF));
        exitCode = exitOutputFailed;
    }
    if (exitCode != exitOk) {
        delete app;
        return exitCode;
    }

    auto session = std::make_unique<THeadlessSession>();
    // quit() does nothing before exec() runs, and a load that pumps events can ask for one before then
    bool quitAsked = false;
    const auto quit = [&quitAsked]() {
        quitAsked = true;
        QCoreApplication::quit();
    };
    ErrorStream& errors = errorStream();
    const QtMessageHandler previousMessageHandler = qInstallMessageHandler(logThroughErrorStream);
    int outputError = 0;
    QObject::connect(session.get(), &THeadlessSession::outputLine, [quit, &outputError, &errors](const QString&, const QString& text, bool) {
        if (outputError) {
            return;
        }
        outputError = writeLine(stdout, text);
        if (!outputError) {
            return;
        }
        // Either way the run is over and the profile closes and saves; a reader that went away is no error, and
        // a stop signal that ended the wait for one is answered as a stop signal
        if (outputError != EPIPE && outputError != EINTR) {
            errors.write(ErrorStream::Source::runner, outputErrorMessage(outputError) + QChar::LineFeed);
        }
        quit();
    });
    QObject::connect(session.get(), &THeadlessSession::quitRequested, app, quit);
    int errorConsolePieces = 0;
    QObject::connect(session.get(), &THeadlessSession::localText, [&errors](const QString&, const QString& text) {
        errors.write(ErrorStream::Source::local, text);
    });
    QObject::connect(session.get(), &THeadlessSession::errorConsoleText, [&errors, &errorConsolePieces](const QString&, const QString& text) {
        ++errorConsolePieces;
        errors.write(ErrorStream::Source::errorConsole, text);
    });
    bool disconnected = false;
    if (parser.isSet(exitOnDisconnectOption)) {
        QObject::connect(session.get(), &THeadlessSession::disconnected, app, [quit, &quitAsked, &disconnected]() {
            if (!quitAsked) {
                disconnected = true;
                quit();
            }
        });
    }

    // The first signal closes the profile with a save; a SIGINT or SIGTERM after it ends the run at once
    int stopSignal = 0;
    const auto onStopSignal = [quit, &stopSignal](const int number) {
        if (!stopSignal) {
            stopSignal = number;
#if !defined(Q_OS_WINDOWS)
            beginStop();
#endif
            quit();
        }
    };
    // Not fatal: an uncaught signal still ends the run, only without a save
#if defined(Q_OS_WINDOWS)
    gOnStopSignal = onStopSignal;
    if (!SetConsoleCtrlHandler(forwardConsoleEvent, TRUE)) {
        errors.report(uncaughtSignalsMessage(qt_error_string(static_cast<int>(GetLastError()))));
    }
#else
    std::unique_ptr<QSocketNotifier> signalNotifier;
    for (const QString& problem : catchStopSignals()) {
        errors.report(problem);
    }
    if (gSignalPipe[0] >= 0) {
        signalNotifier = std::make_unique<QSocketNotifier>(gSignalPipe[0], QSocketNotifier::Read);
        QObject::connect(signalNotifier.get(), &QSocketNotifier::activated, [onStopSignal]() {
            char number = 0;
            if (::read(gSignalPipe[0], &number, 1) == 1) {
                onStopSignal(number);
            }
        });
    }
#endif
    // Between the steps run before the event loop, which is what would otherwise act on a signal
    const auto drainStopSignals = [onStopSignal]() {
#if defined(Q_OS_WINDOWS)
        if (const int number = gFirstStopSignal.load()) {
            onStopSignal(number);
        }
#else
        for (char number = 0; gSignalPipe[0] >= 0 && ::read(gSignalPipe[0], &number, 1) == 1;) {
            onStopSignal(number);
        }
#endif
    };

    auto input = std::make_shared<InputChannel>();
    // Deleted before the profile closes, taking with it the lines not yet handed over
    auto inputReceiver = std::make_unique<QObject>();
    // A line refused while another profile loads is lost where the GUI's command line would have run it, so
    // each is reported and fails the run; once the profile's close has started, the rest are refused, said once
    bool inputClosedReported = false;
    bool inputDropped = false;
    // The GUI runs the aliases of a command typed while there is no connection, then echoes it and drops it
    // with nothing said. Here a line that sent the game something so lost, read before the connection was made,
    // after it ended or when it could not be made, is counted and fails the run, unless --offline was given. A
    // timer's or event handler's send lost while the line's aliases run an event loop is counted against the line too.
    bool everConnected = false;
    int linesNotSent = 0;
    // Commands lost by lines handled inside the running one, as when its alias runs an event loop, so it is not
    // charged for them too
    int nestedNotWritten = 0;
    input->onLine = [&session, &errors, &inputClosedReported, &inputDropped, &linesNotSent, &nestedNotWritten, &options](const QString& line) {
        const QPointer<const Host> pSending = session->host();
        const int notWrittenBefore = pSending ? pSending->mTelnet.gameCommandsNotWritten() : 0;
        const int outerNested = std::exchange(nestedNotWritten, 0);
        const bool submitted = session->submit(line);
        const int lost = pSending ? pSending->mTelnet.gameCommandsNotWritten() - notWrittenBefore : 0;
        const int ownLost = lost - nestedNotWritten;
        nestedNotWritten = outerNested + lost;
        if (submitted) {
            if (options.connect && ownLost > 0) {
                ++linesNotSent;
                inputDropped = true;
            }
            return;
        }
        const Host* pHost = session->host();
        if (pHost && !pHost->closeStarted()) {
            inputDropped = true;
            errors.report(session->errorString());
        } else if (!inputClosedReported) {
            inputClosedReported = true;
            errors.report(session->errorString());
        }
    };
    bool inputFailed = false;
    input->onEnd = [quit, &errors, &inputFailed](const QString& failure) {
        if (!failure.isEmpty()) {
            inputFailed = true;
            //: Error from the headless runner; %1 is the system's reason
            errors.report(QCoreApplication::translate("headless", "cannot read standard input: %1").arg(failure));
        }
        quit();
    };

    // Started after the --package and -e steps, once the connection is made or the attempt fails, so nothing
    // typed goes before the game can take it
    bool stepsRun = false;
    bool attemptEnded = false;
    bool readerStarted = false;
    const auto startReading = [&]() {
        if (readerStarted || !stepsRun || !(everConnected || attemptEnded) || quitAsked || !session->host() || parser.isSet(noStdinOption) || standardInputClosed) {
            return;
        }
        readerStarted = true;
        input->receiver = inputReceiver.get();
        std::thread(readStandardInput, input).detach();
    };
    QObject::connect(session.get(), &THeadlessSession::connected, app, [&everConnected, &startReading]() {
        everConnected = true;
        startReading();
    });
    QObject::connect(session.get(), &THeadlessSession::disconnected, app, [&attemptEnded, &startReading]() {
        attemptEnded = true;
        startReading();
    });

    bool luaFailed = false;
    bool packageFailed = false;
    const THeadlessSession::StartResult started = session->start(options);
    SecureStringUtils::secureStringClear(options.password);
    if (started == THeadlessSession::StartResult::started) {
        // Not ended by output that cannot be written, which leaves the profile running and its steps to do
        const auto stepsEnded = [&]() {
            drainStopSignals();
            return stopSignal || disconnected || !session->host() || session->host()->closeStarted();
        };
        // They usually run before the connection is made, so what they send the game may be lost
        const QPointer<const Host> pStepsHost = session->host();
        const int notWrittenBeforeSteps = pStepsHost ? pStepsHost->mTelnet.gameCommandsNotWritten() : 0;
        // Before the Lua, which may count on them and on what their install handlers set up
        for (const QString& package : parser.values(packageOption)) {
            if (stepsEnded()) {
                break;
            }
            const THeadlessSession::PackageResult result = session->installPackage(package);
            if (result != THeadlessSession::PackageResult::installed) {
                packageFailed = packageFailed || result != THeadlessSession::PackageResult::alreadyInstalled;
                errors.report(session->errorString());
            }
        }
        for (const QString& code : parser.values(execOption)) {
            if (stepsEnded()) {
                break;
            }
            // A Lua error is already on the error console; anything else is not
            const int piecesBefore = errorConsolePieces;
            if (!session->runLua(code)) {
                luaFailed = true;
                if (errorConsolePieces == piecesBefore) {
                    errors.report(session->errorString());
                }
            }
        }
        const int stepsNotSent = options.connect && pStepsHost ? pStepsHost->mTelnet.gameCommandsNotWritten() - notWrittenBeforeSteps : 0;
        if (stepsNotSent > 0) {
            //: Error from the headless runner; %1 is how many commands packages and Lua given with -e sent the game before the connection was made, which were lost
            errors.report(QCoreApplication::translate("headless",
                                                      "a package or Lua given with -e sent the game commands before there was a connection to send them to; send from a sysConnectionEvent handler "
                                                      "instead; commands not sent: %1")
                                  .arg(stepsNotSent));
        }
        drainStopSignals();
        stepsRun = true;
        // --offline has no connection to wait for; one already made may still have its signal queued
        if (!options.connect || (session->host() && session->host()->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState)) {
            everConnected = true;
        }
        startReading();
        if (!quitAsked) {
            exitCode = app->exec();
        }
        // One that ended a wait on standard output may still be in the pipe, its notifier not run since; it still
        // decides the exit status.
        drainStopSignals();
        if (linesNotSent) {
            //: Error from the headless runner; %1 is how many lines of standard input sent the game something while there was no connection, which was lost
            errors.report(QCoreApplication::translate("headless", "standard input was read while there was no connection to the game to send it to; lines not sent: %1").arg(linesNotSent));
        }
        {
            const QMutexLocker lock(&input->mutex);
            input->receiver = nullptr;
            input->drained.wakeAll();
        }
        inputReceiver.reset();
        const bool shutDown = session->shutdown(true);
        if (!shutDown) {
            errors.report(session->errorString());
        } else {
            for (const QString& profile : session->unsavedProfiles()) {
                //: Error from the headless runner when a profile it closed could not be saved; %1 is its name
                errors.report(QCoreApplication::translate("headless", "The profile \"%1\" could not be saved.").arg(profile));
            }
        }
        // As --help gives it
        if (!shutDown || session->saveFailed()) {
            exitCode = exitFailed;
        } else if (stopSignal) {
            exitCode = exitSignalBase + stopSignal;
        } else if (outputError && outputError != EPIPE && outputError != EINTR) {
            exitCode = exitOutputFailed;
        } else if (disconnected) {
            exitCode = exitDisconnected;
        } else if (luaFailed || packageFailed || stepsNotSent > 0 || inputFailed || inputDropped) {
            exitCode = exitFailed;
        }
    } else {
        // A new profile's name, or connection details, that the connection dialog would not take came from the
        // command line
        exitCode = started == THeadlessSession::StartResult::invalidName || started == THeadlessSession::StartResult::invalidConnectionDetails ? usageError(session->errorString()) : exitFailed;
        if (exitCode == exitFailed) {
            errors.report(session->errorString());
        }
        // One whose save did not load keeps its last good save; any other its scripts opened is saved
        if (!session->shutdown(true)) {
            errors.report(session->errorString());
        }
        for (const QString& profile : session->unsavedProfiles()) {
            //: Error from the headless runner when a profile it closed could not be saved; %1 is its name
            errors.report(QCoreApplication::translate("headless", "The profile \"%1\" could not be saved.").arg(profile));
        }
    }
    errors.endLine();
    session.reset();
#if defined(Q_OS_WINDOWS)
    SetConsoleCtrlHandler(forwardConsoleEvent, FALSE);
#else
    signalNotifier.reset();
#endif

    // Joins the match helpers while the application object still exists; see TriggerMatchPool::shutdown()
    TriggerMatchPool::shutdown();
    delete app;
    qInstallMessageHandler(previousMessageHandler);
    return exitCode;
}
