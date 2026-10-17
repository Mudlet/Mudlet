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

#include "MudletApp.h"
#include "THeadlessSession.h"
#include "TriggerMatchPool.h"
#include "utils.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QMutex>
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

#if defined(Q_OS_WINDOWS)
#include <QVarLengthArray>

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
// Set once a stop signal is acted on; read by the writes, from Qt's threads too
std::atomic_bool gStopping = false;

// A wait to write ends with a stop signal, as EINTR, unless the descriptor can be written now: the save the
// signal asks for must not wait on a reader that has stopped reading, as `mudlet-headless | less` left at its
// prompt has. Once stopping, nothing waits, and what such a reader cannot take is dropped: the run is over.
bool stopSignalled(const pollfd* ready, const nfds_t count)
{
    const bool stop = gStopping ? !ready[0].revents : (count == 2 && (ready[1].revents & POLLIN) && !ready[0].revents);
    if (stop) {
        errno = EINTR;
    }
    return stop;
}

// Before each write: one to a full pipe blocks with nothing to interrupt it, while POLLOUT on a pipe promises
// room for PIPE_BUF bytes, which is as much as is written at a time. On a POLLNVAL the write itself is left to
// say what the descriptor is, as macOS answers it at once for a terminal.
bool waitToWrite(const int descriptor)
{
    pollfd ready[2] = {{descriptor, POLLOUT, 0}, {gSignalPipe[0], POLLIN, 0}};
    const bool stopping = gStopping;
    const nfds_t count = !stopping && gSignalPipe[0] >= 0 ? 2 : 1;
    while (::poll(ready, count, stopping ? 0 : -1) < 0) {
        if (errno != EINTR) {
            return false;
        }
    }
    return !stopSignalled(ready, count);
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
    pollfd ready[2] = {{descriptor, events, 0}, {gSignalPipe[0], POLLIN, 0}};
    const bool writing = events & POLLOUT;
    const bool stopping = writing && gStopping;
    const nfds_t count = writing && !stopping && gSignalPipe[0] >= 0 ? 2 : 1;
    if (::poll(ready, count, stopping ? 0 : -1) < 0 && errno != EINTR) {
        errno = waited;
        return false;
    }
    if (writing && stopSignalled(ready, count)) {
        return false;
    }
    // POLLERR and POLLHUP are left to the retried call, which says what ended the stream (EPIPE, or 0 at the end).
    // macOS answers POLLNVAL at once for a terminal, so that is waited out in pauses rather than spun on.
    thread_local int pauseMs = 1;
    if (!(ready[0].revents & POLLNVAL)) {
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
    if (fwrite(bytes.constData(), 1, length, stream) == length && fflush(stream) == 0) {
        return 0;
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

// Standard error carries the runner's messages, Qt's log, and the profile's local text and its error console,
// the last two in pieces; one left mid-line is ended before another source writes. Its own write failures
// are not counted: there is nowhere left to report them, and the game's output does not depend on it.
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
    // From any thread, as Qt logs from its own; the handler that was there before writes whole lines
    void log(const QtMsgType type, const QMessageLogContext& context, const QString& message)
    {
        const QMutexLocker lock(&mMutex);
        endLineLocked();
        mPreviousHandler(type, context, message);
    }

    QtMessageHandler mPreviousHandler = nullptr;

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

BOOL WINAPI forwardConsoleEvent(const DWORD type)
{
    // The default handler ends the process, for a second Ctrl+C while the first one's save runs
    if (gStopSignalled.exchange(true) && (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT)) {
        return FALSE;
    }
    // As the POSIX signals the exit code names: Ctrl+C, Ctrl+Break, and a closed console, logoff or shutdown
    const int number = type == CTRL_C_EVENT ? SIGINT : (type == CTRL_BREAK_EVENT ? SIGTERM : 1);
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
    QStringList problems;
    // POSIX promises only "not -1" for success here
    if (::fcntl(gSignalPipe[0], F_SETFD, FD_CLOEXEC) == -1 || ::fcntl(gSignalPipe[1], F_SETFD, FD_CLOEXEC) == -1 || ::fcntl(gSignalPipe[0], F_SETFL, O_NONBLOCK) == -1
        || ::fcntl(gSignalPipe[1], F_SETFL, O_NONBLOCK) == -1) {
        //: Warning from the headless runner; %1 is the system's reason
        problems << QCoreApplication::translate("headless", "SIGINT, SIGTERM and SIGHUP are caught, but the pipe that passes them on could not be set up: %1").arg(systemReason());
    }
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
                                       "  -p, --profile <profile>  the profile to open and connect.\n"
                                       "  -e, --exec <lua>         runs Lua code once the profile has loaded; give it more\n"
                                       "                           than once to run several pieces in order.\n"
                                       "  --no-stdin               does not read standard input, as for a background run.\n"
                                       "  --exit-on-disconnect     exits once the connection to the game ends.\n"
                                       "  -h, --help               displays this message.\n"
                                       "  -v, --version            displays version information.\n"
                                       "\n"
                                       "The profile is saved as it closes; when closeProfile() closes it, only if its\n"
                                       "\"Auto save on exit\" setting is on. Exit status:\n"
                                       "  0      the profile closed, as closeMudlet() or closeProfile() do, or standard\n"
                                       "         input ended.\n"
                                       "  1      the profile could not be opened or saved, Lua given with -e failed, or\n"
                                       "         standard input could not be read.\n"
                                       "  2      the command line was wrong.\n"
                                       "  3      standard output could not be written.\n"
                                       "  4      the connection ended or could not be made, with --exit-on-disconnect;\n"
                                       "         reconnect() ends it too.\n"
                                       "  128+n  signal n (SIGHUP, SIGINT or SIGTERM) ended the run.\n"
                                       "Where more than one applies, the first of these is given: 1 for a profile that\n"
                                       "could not be saved, then 128+n, 3, 4, and 1 for the rest.\n"
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
    // Lua's io.write() shares stdout with writeBytes(). What stdio held back would be lost on a full
    // non-blocking stdout, and come out of order with the game's lines.
    setvbuf(stdout, nullptr, _IONBF, 0);
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

    // Help and version need no platform plugin, so they are answered before there is one. Qt's own options
    // are still in this list, so it is read leniently, and single-dash words whole, as main.cpp does.
    QStringList rawArguments;
    rawArguments.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        rawArguments << QString::fromLocal8Bit(argv[i]);
    }
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.parse(rawArguments);
    if (parser.isSet(helpOption) || parser.isSet(versionOption)) {
        // Only for the translation, as in the GUI
        QCoreApplication printApp(argc, argv);
        setApplicationNames();
        MudletApp::loadTranslationsForCommandLine(true);
        if (const int error = writeLine(stdout, parser.isSet(helpOption) ? helpText() : versionText())) {
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
    // Deleted before main() returns, as main.cpp's is, so Qt's cleanup runs ahead of static destruction
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
    } else if (parser.value(profileOption).isEmpty()) {
        //: Command line error of the headless runner; -p is the option that names the profile to run
        exitCode = usageError(QCoreApplication::translate("headless", "No profile was given; name one with -p."));
    }
    if (exitCode != exitOk) {
        delete app;
        return exitCode;
    }

#if !defined(Q_OS_WINDOWS)
    // A reader that goes away (`mudlet-headless -p x | head`) would otherwise kill the profile mid-session.
    // Caught, not ignored: an ignored signal stays ignored in every program a script starts.
    signal(SIGPIPE, [](int) {});
#endif

    auto session = std::make_unique<THeadlessSession>();
    // quit() does nothing before exec() runs, and a load that pumps events can ask for one before then
    bool quitAsked = false;
    const auto quit = [&quitAsked]() {
        quitAsked = true;
        QCoreApplication::quit();
    };
    ErrorStream& errors = errorStream();
    // Known before it can be called: nullptr puts back Qt's own, which logs in the meantime
    errors.mPreviousHandler = qInstallMessageHandler(nullptr);
    qInstallMessageHandler(logThroughErrorStream);
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
            gStopping = true;
#endif
            quit();
        }
    };
    // The run goes on: a profile that cannot be stopped gently can still be stopped
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

    auto input = std::make_shared<InputChannel>();
    // Deleted before the profile closes, taking with it the lines not yet handed over
    auto inputReceiver = std::make_unique<QObject>();
    bool inputRefusalReported = false;
    input->onLine = [&session, &errors, &inputRefusalReported](const QString& line) {
        if (!session->submit(line) && !inputRefusalReported) {
            inputRefusalReported = true;
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

    bool luaFailed = false;
    const THeadlessSession::StartResult started = session->start({.profile = parser.value(profileOption), .connect = true});
    if (started == THeadlessSession::StartResult::started) {
        for (const QString& code : parser.values(execOption)) {
            if (quitAsked || !session->host()) {
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
        if (!quitAsked && !parser.isSet(noStdinOption)) {
            input->receiver = inputReceiver.get();
            std::thread(readStandardInput, input).detach();
        }
        if (!quitAsked) {
            exitCode = app->exec();
        }
#if !defined(Q_OS_WINDOWS)
        // One that ended a wait on standard output is still in the pipe, its notifier not run since; so may a
        // second signal be, which the writes must not take for a stop still to come
        for (char number = 0; gSignalPipe[0] >= 0 && ::read(gSignalPipe[0], &number, 1) == 1;) {
            onStopSignal(number);
        }
#endif
        {
            const QMutexLocker lock(&input->mutex);
            input->receiver = nullptr;
            input->drained.wakeAll();
        }
        inputReceiver.reset();
        const bool shutDown = session->shutdown(true);
        if (!shutDown) {
            errors.report(session->errorString());
        } else if (session->saveFailed()) {
            //: Error from the headless runner when a profile it closed could not be saved
            errors.report(QCoreApplication::translate("headless", "The profile could not be saved."));
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
        } else if (luaFailed || inputFailed) {
            exitCode = exitFailed;
        }
    } else {
        errors.report(session->errorString());
        // A profile that did not load is not saved over its last good save
        if (!session->shutdown(false)) {
            errors.report(session->errorString());
        }
        exitCode = exitFailed;
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
    qInstallMessageHandler(errors.mPreviousHandler);
    return exitCode;
}
