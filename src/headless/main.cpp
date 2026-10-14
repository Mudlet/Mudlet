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

#include <cerrno>
#include <cstring>
#include <csignal>
#include <cstdio>
#include <memory>

// A Qt Resource Collection in a static library only reaches the executable if something here names it
extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

namespace {

enum ExitCode { exitOk = 0, exitProfileFailed = 1, exitUsage = 2, exitOutputFailed = 3 };

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

// 0, or the errno the write failed with
int writeLine(FILE* stream, const QString& text)
{
    const QByteArray bytes = text.toUtf8() + '\n';
    const auto length = static_cast<size_t>(bytes.size());
    errno = 0;
    if (fwrite(bytes.constData(), 1, length, stream) == length && fflush(stream) == 0) {
        return 0;
    }
    return errno ? errno : EIO;
}

void reportOutputError(const int error)
{
    //: %1 is the name of the executable, %2 is the system's reason, such as "No space left on device"
    writeLine(stderr, QCoreApplication::translate("headless", "%1: cannot write to standard output: %2").arg(QLatin1String(APP_TARGET), QString::fromLocal8Bit(std::strerror(error))));
}

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
                                       "Usage: %1 -p <profile>\n"
                                       "Runs a Mudlet profile with no window, writing the lines the game sends to\n"
                                       "standard output.\n"
                                       "\n"
                                       "Options:\n"
                                       "  -p, --profile <profile>  the profile to open and connect.\n"
                                       "  -h, --help               displays this message.\n"
                                       "  -v, --version            displays version information.\n"
                                       "\n"
                                       "It exits with 0 once the profile closes, as closeMudlet() or closeProfile() do,\n"
                                       "or standard output's reader goes away; 1 if the profile cannot be opened or\n"
                                       "closed; 2 for a usage error; and 3 if standard output cannot be written\n"
                                       "otherwise.\n"
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
    initializeQRCResources();

    QCommandLineParser parser;
    const QCommandLineOption profileOption(QStringList{qsl("p"), qsl("profile")}, qsl("Profile to open"), qsl("profile"));
    parser.addOption(profileOption);
    const QCommandLineOption helpOption(QStringList{qsl("h"), qsl("help")}, qsl("Display help and exit"));
    parser.addOption(helpOption);
    const QCommandLineOption versionOption(QStringList{qsl("v"), qsl("version")}, qsl("Display version and exit"));
    parser.addOption(versionOption);

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
        if (const int error = writeLine(stdout, parser.isSet(helpOption) ? helpText() : versionText()); error && error != EPIPE) {
            reportOutputError(error);
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
    } else if (parser.value(profileOption).isEmpty()) {
        //: Command line error of the headless runner; -p is the option that names the profile to run
        exitCode = usageError(QCoreApplication::translate("headless", "No profile was given; name one with -p."));
    } else if (parser.values(profileOption).size() > 1) {
        // Last-wins would quietly drop one, where the GUI opens every -p it is given
        //: Command line error of the headless runner; %1 is an option, such as --profile
        exitCode = usageError(QCoreApplication::translate("headless", "%1 was given more than once.").arg(qsl("--profile")));
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
    int outputError = 0;
    QObject::connect(session.get(), &THeadlessSession::outputLine, [quit, &outputError](const QString&, const QString& text, bool) {
        if (outputError) {
            return;
        }
        outputError = writeLine(stdout, text);
        if (!outputError) {
            return;
        }
        // Either way the run is over and the profile closes and saves; a reader that went away is no error
        if (outputError != EPIPE) {
            reportOutputError(outputError);
        }
        quit();
    });
    QObject::connect(session.get(), &THeadlessSession::quitRequested, app, quit);

    const THeadlessSession::StartResult started = session->start({.profile = parser.value(profileOption), .connect = true});
    if (started == THeadlessSession::StartResult::started) {
        if (!quitAsked) {
            exitCode = app->exec();
        }
        if (!session->shutdown(true)) {
            writeLine(stderr, qsl("%1: %2").arg(QLatin1String(APP_TARGET), session->errorString()));
            exitCode = exitProfileFailed;
        } else if (outputError && outputError != EPIPE) {
            exitCode = exitOutputFailed;
        }
    } else {
        writeLine(stderr, qsl("%1: %2").arg(QLatin1String(APP_TARGET), session->errorString()));
        // One whose save did not load keeps its last good save; any other its scripts opened is saved
        if (!session->shutdown(true)) {
            writeLine(stderr, qsl("%1: %2").arg(QLatin1String(APP_TARGET), session->errorString()));
        }
        exitCode = exitProfileFailed;
    }
    session.reset();

    // Joins the match helpers while the application object still exists; see TriggerMatchPool::shutdown()
    TriggerMatchPool::shutdown();
    delete app;
    return exitCode;
}
