/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2016-2021, 2023, 2024 by Stephen Lyons       *
 *                                            - slysven@virginmedia.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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


#include "HostManager.h"
#include "mudlet.h"
#include "MudletInstanceCoordinator.h"
#include "pre_guard.h"
#include <chrono>
#include <QCommandLineParser>
#include <QDir>
#if defined(Q_OS_WIN32) && !defined(INCLUDE_UPDATER)
#include <QMessageBox>
#endif // defined(Q_OS_WIN32) && !defined(INCLUDE_UPDATER)
#include <QCommandLineOption>
#include <QPainter>
#include <QPointer>
#include <QScreen>
#include <QSettings>
#include <QSplashScreen>
#include <QStringList>
#include <QTranslator>
#include "post_guard.h"
#include "AltFocusMenuBarDisable.h"
#include "TAccessibleConsole.h"
#include "TAccessibleTextEdit.h"
#include "Announcer.h"
#include "FileOpenHandler.h"

using namespace std::chrono_literals;


#if defined(_MSC_VER) && defined(_DEBUG)
// Enable leak detection for MSVC debug builds. _DEBUG is MSVC specific and
// leak detection does not work when it is not defined.
#include <Windows.h>
#include <pcre.h>
#endif // _MSC_VER && _DEBUG

#if defined(Q_OS_WIN32)
bool runUpdate();
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
// Enable leak detection for MSVC debug builds.

#define PCRE_CLIENT_TYPE (_CLIENT_BLOCK | ((('P' << 8) | 'C') << 16))

static void* pcre_malloc_dbg(size_t size)
{
    return ::_malloc_dbg(size, PCRE_CLIENT_TYPE, __FILE__, __LINE__);
}

static void pcre_free_dbg(void* ptr)
{
    return ::_free_dbg(ptr, PCRE_CLIENT_TYPE);
}

#endif // _DEBUG && _MSC_VER

#if defined(INCLUDE_FONTS)
void copyFont(const QString& externalPathName, const QString& resourcePathName, const QString& fileName)
{
    if (!QFile::exists(qsl("%1/%2").arg(externalPathName, fileName))) {
        QFile fileToCopy(qsl(":/%1/%2").arg(resourcePathName, fileName));
        fileToCopy.copy(qsl("%1/%2").arg(externalPathName, fileName));
    }
}

#if defined(Q_OS_LINUX)
void removeOldNoteColorEmojiFonts()
{
    // PLACEMARKER: previous Noto Color Emoji font versions removal
    // Identify old versions so that we can remove them and later on only try
    // to load the latest (otherwise, as they all have the same family name
    // only the first one found will be loaded by the FontManager class):
    QStringList oldNotoFontDirectories;
    // The directory name format is made by Mudlet and is based upon the
    // release date of the version on upstream's Github site, currently:
    // https://github.com/googlefonts/noto-emoji/releases
    // Not all previously released versions have been carried by Mudlet only
    // the ones listed here have been.
    // When adding a later version, append the path and version comment of the
    // replaced one comment to this area:
    // Tag: "v2018-04-24-pistol-update"
    oldNotoFontDirectories << qsl("%1/notocoloremoji-unhinted-2018-04-24-pistol-update").arg(mudlet::getMudletPath(mudlet::mainFontsPath));
    // Release: "v2019-11-19-unicode12"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2019-11-19-unicode12").arg(mudlet::getMudletPath(mudlet::mainFontsPath));
    // Release: "Noto Emoji v2.0238"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2021-07-15-v2.028").arg(mudlet::getMudletPath(mudlet::mainFontsPath));
    // Release: "Unicode 14.0"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2021-11-01-v2.034").arg(mudlet::getMudletPath(mudlet::mainFontsPath));
    // Release: "Unicode 15.0"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2022-09-16-v2.038").arg(mudlet::getMudletPath(mudlet::mainFontsPath));

    QListIterator<QString> itOldNotoFontDirectory(oldNotoFontDirectories);
    while (itOldNotoFontDirectory.hasNext()) {
        auto oldNotoFontDirectory = itOldNotoFontDirectory.next();
        QDir oldDir{oldNotoFontDirectory};
        if (oldDir.exists()) {
            // This can fail but we do not worry about that too much, as long
            // as it nukes any "NotoColorEmoji.ttf" files:
            if (!oldDir.removeRecursively()) {
                qDebug().nospace().noquote() << "main::removeOldNoteColorEmojiFonts() INFO - failed to remove old Noto Color Emoji font located at: " << oldDir.absolutePath();
            }
        }
    }
}
#endif // defined(Q_OS_LINUX)
#endif // defined(INCLUDE_FONTS)

QTranslator* loadTranslationsForCommandLine()
{
    const QSettings settings_new(QLatin1String("mudlet"), QLatin1String("Mudlet"));
    auto pSettings = new QSettings((settings_new.contains(QLatin1String("pos")) ? QLatin1String("mudlet") : QLatin1String("Mudlet")),
                                   (settings_new.contains(QLatin1String("pos")) ? QLatin1String("Mudlet") : QLatin1String("Mudlet 1.0")));
    auto interfaceLanguage = pSettings->value(QLatin1String("interfaceLanguage")).toString();
    auto userLocale = interfaceLanguage.isEmpty() ? QLocale::system() : QLocale(interfaceLanguage);
    if (userLocale == QLocale::c()) {
        // nothing found
        return nullptr;
    }
    // We only need the Mudlet translations for the Command Line texts, no need
    // for any Qt ones:
    QTranslator* pMudletTranslator = new QTranslator;
    // If we allow the translations to be outside of the resource file inside
    // the application executable then this will have to be revised to handle
    // it:
    const bool isOk = pMudletTranslator->load(userLocale, qsl("mudlet"), QString("_"), qsl(":/lang"), qsl(".qm"));
    if (!isOk) {
        return nullptr;
    }
    QCoreApplication::installTranslator(pMudletTranslator);
    return pMudletTranslator;
}

int main(int argc, char* argv[])
{
    // print stdout to console if Mudlet is started in a console in Windows
    // credit to https://stackoverflow.com/a/41701133 for the workaround
#ifdef Q_OS_WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
    // Enable leak detection for MSVC debug builds.
    {
        // Check for a debugger and prompt if one is not attached.
        while (!IsDebuggerPresent()
               && IDYES == MessageBox(0,
                                      "You are starting debug mudlet without a debugger attached. If you wish to attach one and verify that it worked, click yes. To continue without a debugger, "
                                      "click no.",
                                      "Mudlet Debug",
                                      MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2)) {
            ;
        }

        // _CRTDBG_ALLOC_MEM_DF: Enable heap debugging.
        // _CRTDBG_LEAK_CHECK_DF: Check for leaks at program exit.
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

        // Create a log file for writing leaks.
        HANDLE hLogFile = CreateFile("stderr.txt", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, hLogFile);

        // Set this to break on the allocation number shown in the debug output above.
        // _crtBreakAlloc = 0;

        pcre_malloc = pcre_malloc_dbg;
        pcre_free = pcre_free_dbg;
        pcre_stack_malloc = pcre_malloc_dbg;
        pcre_stack_free = pcre_free_dbg;
    }
#endif // _MSC_VER && _DEBUG

#if defined(Q_OS_MACOS)
    // Workaround for horrible mac rendering issues once the mapper widget
    // is open - see https://bugreports.qt.io/browse/QTBUG-41257
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#elif defined(Q_OS_FREEBSD)
    // Cure for diagnostic:
    // "Qt WebEngine seems to be initialized from a plugin. Please set
    // Qt::AA_ShareOpenGLContexts using QCoreApplication::setAttribute
    // before constructing QGuiApplication."
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif

    auto app = qobject_cast<QApplication*>(new QApplication(argc, argv));

    QAccessible::installFactory(TAccessibleConsole::consoleFactory);
    QAccessible::installFactory(TAccessibleTextEdit::textEditFactory);

#if defined(Q_OS_LINUX)
    QAccessible::installFactory(Announcer::accessibleFactory);
#endif

#if defined(Q_OS_WIN32) && defined(INCLUDE_UPDATER)
    auto abortLaunch = runUpdate();
    if (abortLaunch) {
        return 0;
    }
#endif

    // Turn the cursor into the waiting one during startup, so something shows
    // activity even if the quiet, no splashscreen startup has been used
    app->setOverrideCursor(QCursor(Qt::WaitCursor));
    app->setOrganizationName(qsl("Mudlet"));

    QFile gitShaFile(":/app-build.txt");
    gitShaFile.open(QIODevice::ReadOnly | QIODevice::Text);
    const QString appBuild = QString::fromUtf8(gitShaFile.readAll()).trimmed();

    const bool releaseVersion = appBuild.isEmpty();
    const bool publicTestVersion = appBuild.startsWith("-ptb");

    if (publicTestVersion) {
        app->setApplicationName(qsl("Mudlet Public Test Build"));
    } else {
        app->setApplicationName(qsl("Mudlet"));
    }
    if (releaseVersion) {
        app->setApplicationVersion(APP_VERSION);
    } else {
        app->setApplicationVersion(QString(APP_VERSION) + appBuild);
    }

    mudlet::start();
    // Detect config path before any files are read
    mudlet::self()->setupConfig();

    QPointer<QTranslator> commandLineTranslator(loadTranslationsForCommandLine());
    QCommandLineParser parser;
    // The third (and fourth if provided) arguments are used to populate the
    // help text that the QCommandLineParser::showHelp(...) would produce
    // however we do the -h/--help option ourself so these texts are unused
    // other than that a non-null fourth argument maybe responsible for
    // making the option take a value that follows it - as such they do not
    // need to be passed to the translation system.
    const QCommandLineOption profileToOpen(QStringList() << qsl("p") << qsl("profile"), qsl("Profile to open automatically"), qsl("profile"));
    parser.addOption(profileToOpen);

    const QCommandLineOption showHelp(QStringList() << qsl("h") << qsl("help"), qsl("Display help and exit"));
    parser.addOption(showHelp);

    const QCommandLineOption showVersion(QStringList() << qsl("v") << qsl("version"), qsl("Display version and exit"));
    parser.addOption(showVersion);

    const QCommandLineOption showSplashscreen(QStringList() << qsl("s") << qsl("splashscreen"), qsl("Show the splash screen when starting"));
    parser.addOption(showSplashscreen);

    const QCommandLineOption mirrorToStdout(QStringList() << qsl("m") << qsl("mirror"), qsl("Mirror output of all consoles to STDOUT"));
    parser.addOption(mirrorToStdout);

    QCommandLineOption beQuiet(QStringList() << qsl("q") << qsl("quiet"), qsl("Depricated option, previously used to disable showing the splash screen"));
    beQuiet.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(beQuiet);

    const QCommandLineOption onlyPredefinedProfileToShow(QStringList() << qsl("o") << qsl("only"),
                                                   qsl("Set Mudlet to only show this predefined MUD profile and hide all other predefined ones."),
                                                   qsl("predefined_game"));
    parser.addOption(onlyPredefinedProfileToShow);

    const QCommandLineOption steamMode(QStringList() << qsl("steammode"), qsl("Adjusts Mudlet settings to match Steam's requirements."));
    parser.addOption(steamMode);

    parser.addPositionalArgument("package", "Path to .mpackage file");

    const bool parsedCommandLineOk = parser.parse(app->arguments());

    const QString appendLF{qsl("%1\n")};
    const QString append2LF{qsl("%1\n\n")};

    // Non-GUI actions --help and --version as suggested by GNU coding standards,
    // section 4.7: http://www.gnu.org/prep/standards/standards.html#Command_002dLine-Interfaces
    QStringList texts;

    if (!parsedCommandLineOk) {
        // Warn of unknown options but tolerate them.
        // We want the message to be visible for someone launching from command prompt
        // and will have standard output left on their screen, but still allow program
        // to start when launched by installer.
        // --squirrel-firstrun for example is given for launch at end of install process.
        std::cout << QCoreApplication::translate("main", "Warning: %1\n").arg(parser.errorText()).toStdString();
    }

    if (parser.isSet(showHelp)) {
        // Do "help" action
        texts << appendLF.arg(QCoreApplication::translate("main", "Usage: %1 [OPTION...] [FILE] ",
                                                          // Comment to separate arguments
                                                          "%1 is the name of the executable as it is on this OS.")
                                         .arg(QLatin1String(APP_TARGET)));
        texts << appendLF.arg(QCoreApplication::translate("main", "Options:"));
        texts << appendLF.arg(QCoreApplication::translate("main", "       -h, --help                   displays this message."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       -v, --version                displays version information."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       -s, --splashscreen           show splashscreen on startup."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       -p, --profile=<profile>      additional profile to open, may be\n"
                                                                  "                                    repeated."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       -o, --only=<predefined>      make Mudlet only show the specific\n"
                                                                  "                                    predefined game, may be repeated."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       --steammode                  adjusts Mudlet settings to match\n"
                                                                  "                                    Steam's requirements."));
        texts << appendLF.arg(QCoreApplication::translate("main", "There are other inherited options that arise from the Qt Libraries which are\n"
                                                                  "less likely to be useful for normal use of this application:"));
        // From documentation and from http://qt-project.org/doc/qt-5/qapplication.html:
        texts << appendLF.arg(QCoreApplication::translate("main", "       --dograb                     ignore any implicit or explicit -nograb.\n"
                                                                  "                                    --dograb wins over --nograb even when --nograb is last on\n"
                                                                  "                                    the command line."));
#if defined(Q_OS_LINUX)
        texts << appendLF.arg(QCoreApplication::translate("main", "       --nograb                     the application should never grab the mouse or the\n"
                                                                  "                                    keyboard. This option is set by default when Mudlet is\n"
                                                                  "                                    running in the gdb debugger under Linux."));
#else // ! defined(Q_OS_LINUX)
        texts << appendLF.arg(QCoreApplication::translate("main", "       --nograb                     the application should never grab the mouse or the\n"
                                                                  "                                    keyboard."));
#endif // ! defined(Q_OS_LINUX)
        texts << appendLF.arg(QCoreApplication::translate("main", "       --reverse                    sets the application's layout direction to right to left."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       --style=style                sets the application GUI style. Possible values depend on\n"
                                                                  "                                    your system configuration. If Qt was compiled with\n"
                                                                  "                                    additional styles or has additional styles as plugins\n"
                                                                  "                                    these will be available to the -style command line\n"
                                                                  "                                    option. You can also set the style for all Qt\n"
                                                                  "                                    applications by setting the QT_STYLE_OVERRIDE environment\n"
                                                                  "                                    variable."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       --style style                is the same as listed above."));
        texts << appendLF.arg(QCoreApplication::translate("main", "       --stylesheet=stylesheet      sets the application styleSheet.\n"
                                                                  "                                    The value must be a path to a file that contains the\n"
                                                                  "                                    Style Sheet. Note: Relative URLs in the Style Sheet file\n"
                                                                  "                                    are relative to the Style Sheet file's path."));

        texts << appendLF.arg(QCoreApplication::translate("main", "       --stylesheet stylesheet      is the same as listed above."));
// Not sure about MacOS case as that does not use X
#if defined(Q_OS_UNIX) && (! defined(Q_OS_MACOS))
        texts << appendLF.arg(QCoreApplication::translate("main", "       --sync                       forces the X server to perform each X client request\n"
                                                                  "                                    immediately and not use buffer optimization. It makes the\n"
                                                                  "                                    program easier to debug and often much slower. The --sync\n"
                                                                  "                                    option is only valid for the X11 version of Qt."));
#endif // defined(Q_OS_UNIX) and not defined(Q_OS_MACOS)
        texts << appendLF.arg(QCoreApplication::translate("main", "       --widgetcount                prints debug message at the end about number of widgets\n"
                                                                  "                                    left undestroyed and maximum number of widgets existing\n"
                                                                  "                                    at the same time."));
        texts << append2LF.arg(QCoreApplication::translate("main", "       --qmljsdebugger=1234[,block] activates the QML/JS debugger with a\n"
                                                                   "                                    specified port. The number is the port value and block is\n"
                                                                   "                                    optional and will make the application wait until a\n"
                                                                   "                                    debugger connects to it."));
        texts << appendLF.arg(QCoreApplication::translate("main", "Arguments:"));
        texts << appendLF.arg(QCoreApplication::translate("main", "        [FILE]                       File to install as a package"));
        texts << appendLF.arg(QCoreApplication::translate("main", "Report bugs to: https://github.com/Mudlet/Mudlet/issues"));
        texts << appendLF.arg(QCoreApplication::translate("main", "Project home page: http://www.mudlet.org/"));
        std::cout << texts.join(QString()).toStdString();
        return 0;
    }

    if (parser.isSet(showVersion)) {
        // Do "version" action - wording and format is quite tightly specified by the coding standards
#if defined(QT_DEBUG)
        texts << appendLF.arg(QCoreApplication::translate("main", "%1 %2%3 (with debug symbols, without optimisations)",
                                                          "%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev")
                 .arg(QLatin1String(APP_TARGET), QLatin1String(APP_VERSION), appBuild));
#else // ! defined(QT_DEBUG)
        texts << QString::fromStdString(APP_TARGET " " APP_VERSION " " + appBuild.toStdString() + " \n");
#endif // ! defined(QT_DEBUG)
        texts << appendLF.arg(QCoreApplication::translate("main", "Qt libraries %1 (compilation) %2 (runtime)",
             "%1 and %2 are version numbers").arg(QLatin1String(QT_VERSION_STR), qVersion()));
        // PLACEMARKER: Date-stamp needing annual update
        texts << appendLF.arg(QCoreApplication::translate("main", "Copyright © 2008-2024  Mudlet developers"));
        texts << appendLF.arg(QCoreApplication::translate("main", "Licence GPLv2+: GNU GPL version 2 or later - http://gnu.org/licenses/gpl.html"));
        texts << appendLF.arg(QCoreApplication::translate("main", "This is free software: you are free to change and redistribute it.\n"
                                                                  "There is NO WARRANTY, to the extent permitted by law."));
        std::cout << texts.join(QString()).toStdString();
        return 0;
    }

    // Handles installing a package from a command line argument.
    // Used when mudlet is used to open an .mpackage file on some operating systems.
    //
    // If Mudlet was already open:
    // 1. Send the package path to the other process and exit.
    // 2. The other process will take responsibility for installation.
    // 3. If a profile is open, installation will occur in currently open profile.
    // 4. If no profile is open, the package will be queued for install until a profile is selected.
    //
    // If no other mudlet process is found:
    // 1. This current process will start as normal.
    // 2. The package will be queued for install until a profile is selected.

    std::unique_ptr<MudletInstanceCoordinator> instanceCoordinator = std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator");
    const bool firstInstanceOfMudlet = instanceCoordinator->tryToStart();

    const QStringList positionalArguments = parser.positionalArguments();
    if (!positionalArguments.isEmpty()) {
        const QString absPath = QDir(positionalArguments.first()).absolutePath();
        instanceCoordinator->queuePackage(absPath);
        if (!firstInstanceOfMudlet) {
            const bool successful = instanceCoordinator->installPackagesRemotely();
            if (successful) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    /*******************************************************************
     * If we get to HERE then we are going to run a GUI application... *
     *******************************************************************/
    // Unload translator so we can use main application translation system;
    if (!commandLineTranslator.isNull()) {
        QCoreApplication::removeTranslator(commandLineTranslator);
        commandLineTranslator.clear();
    }

    const QStringList cliProfiles = parser.values(profileToOpen);
    const QStringList onlyProfiles = parser.values(onlyPredefinedProfileToShow);
    
    const bool showSplash = parser.isSet(showSplashscreen);
    QImage splashImage = mudlet::getSplashScreen(releaseVersion, publicTestVersion);

    if (showSplash) {
        QPainter painter(&splashImage);
        unsigned fontSize = 16;
        const QString sourceVersionText = QString(QCoreApplication::translate("main", "Version: %1").arg(APP_VERSION + appBuild));

        bool isWithinSpace = false;
        while (!isWithinSpace) {
            const QFont font("Bitstream Vera Serif", fontSize, QFont::Bold | QFont::Serif | QFont::PreferMatch | QFont::PreferAntialias);
            QTextLayout versionTextLayout(sourceVersionText, font, painter.device());
            versionTextLayout.beginLayout();
            // Start work in this text item
            QTextLine versionTextline = versionTextLayout.createLine();
            // First draw (one line from) the text we have put in on the layout to
            // see how wide it is..., assuming actually that it will only take one
            // line of text
            versionTextline.setLineWidth(280);
            //Splashscreen bitmap is (now) 320x360 - hopefully entire line will all fit into 280
            versionTextline.setPosition(QPointF(0, 0));
            // Only pretend, so we can see how much space it will take
            const QTextLine dummy = versionTextLayout.createLine();
            if (!dummy.isValid()) {
                // No second line so have got all text in first so can do it
                isWithinSpace = true;
                const qreal versionTextWidth = versionTextline.naturalTextWidth();
                // This is the ACTUAL width of the created text
                versionTextline.setPosition(QPointF((320 - versionTextWidth) / 2.0, 270));
                // And now we can place it centred horizontally
                versionTextLayout.endLayout();
                // end the layout process and paint it out
                painter.setPen(QColor(176, 64, 0, 255)); // #b04000
                versionTextLayout.draw(&painter, QPointF(0, 0));
            } else {
                // Too big - text has spilled over onto a second line - so try again
                fontSize--;
                versionTextLayout.clearLayout();
                versionTextLayout.endLayout();
            }
        }

        // Repeat for other text, but we know it will fit at given size
        // PLACEMARKER: Date-stamp needing annual update
        const QString sourceCopyrightText = qsl("©️ Mudlet makers 2008-2024");
        const QFont font(qsl("Bitstream Vera Serif"), 16, QFont::Bold | QFont::Serif | QFont::PreferMatch | QFont::PreferAntialias);
        QTextLayout copyrightTextLayout(sourceCopyrightText, font, painter.device());
        copyrightTextLayout.beginLayout();
        QTextLine copyrightTextline = copyrightTextLayout.createLine();
        copyrightTextline.setLineWidth(280);
        copyrightTextline.setPosition(QPointF(1, 1));
        const qreal copyrightTextWidth = copyrightTextline.naturalTextWidth();
        copyrightTextline.setPosition(QPointF((320 - copyrightTextWidth) / 2.0, 340));
        copyrightTextLayout.endLayout();
        painter.setPen(QColor(112, 16, 0, 255)); // #701000
        copyrightTextLayout.draw(&painter, QPointF(0, 0));
    }
    const QPixmap pixmap = QPixmap::fromImage(splashImage);
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
    // Specifying the screen here seems to help to put the splash screen on the
    // same monitor that the main application window will be put upon on first
    // run, in some situations the two can otherwise get to be different which
    // is misleading unhelpful to a new user...!
    QSplashScreen splash(qApp->primaryScreen(), pixmap);
#else
    QSplashScreen splash(pixmap);
#endif
    if (showSplash) {
        splash.show();
    }
    app->processEvents();

    const QString homeDirectory = mudlet::getMudletPath(mudlet::mainPath);
    const QDir dir;
    bool first_launch = false;
    if (!dir.exists(homeDirectory)) {
        dir.mkpath(homeDirectory);
        first_launch = true;
    }

#if defined(INCLUDE_FONTS)
    const QString bitstreamVeraFontDirectory(qsl("%1/ttf-bitstream-vera-1.10").arg(mudlet::getMudletPath(mudlet::mainFontsPath)));
    if (!dir.exists(bitstreamVeraFontDirectory)) {
        dir.mkpath(bitstreamVeraFontDirectory);
    }
    const QString ubuntuFontDirectory(qsl("%1/ubuntu-font-family-0.83").arg(mudlet::getMudletPath(mudlet::mainFontsPath)));
    if (!dir.exists(ubuntuFontDirectory)) {
        dir.mkpath(ubuntuFontDirectory);
    }
#if defined(Q_OS_LINUX)
    // Only needed/works on Linux to provide color emojis:
    removeOldNoteColorEmojiFonts();
    // PLACEMARKER: current Noto Color Emoji font directory specification:
    // Release: "Unicode 15.1, take 3"
    const QString notoFontDirectory{qsl("%1/noto-color-emoji-2023-11-30-v2.042").arg(mudlet::getMudletPath(mudlet::mainFontsPath))};
    if (!dir.exists(notoFontDirectory)) {
        dir.mkpath(notoFontDirectory);
    }
#endif

    // The original code plonks the fonts AND the Copyright into the MAIN mudlet
    // directory - but the Copyright statement is specifically for the fonts
    // so now they all go into "./fonts/" subdirectories - I note that
    // the Debian packager already removes these fonts anyhow as they are
    // already present in a shared form in the OS anyhow so our copy is
    // superfluous...!
    // Note that the ubuntu fonts have *just* entered the Unstable "non-free"
    // Debian version as of Dec 2017:
    // https://anonscm.debian.org/cgit/pkg-fonts/fonts-ubuntu.git/
    // but there is a term in the Ubuntu licence that makes them currently (and
    // for the prior seven years) not quite the right side of the Debian Free
    // Software Guidelines.
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("COPYRIGHT.TXT"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("local.conf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("README.TXT"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("RELEASENOTES.TXT"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("Vera.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraBd.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraBI.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraIt.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraMoBd.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraMoBI.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraMoIt.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraMono.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraSe.ttf"));
    copyFont(bitstreamVeraFontDirectory, QLatin1String("fonts/ttf-bitstream-vera-1.10"), QLatin1String("VeraSeBd.ttf"));

    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("CONTRIBUTING.txt"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("copyright.txt"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("FONTLOG.txt"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("LICENCE-FAQ.txt"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("LICENCE.txt"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("README.txt"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("TRADEMARKS.txt"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-B.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-BI.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-C.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-L.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-LI.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-M.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-MI.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-R.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("Ubuntu-RI.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("UbuntuMono-B.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("UbuntuMono-BI.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("UbuntuMono-R.ttf"));
    copyFont(ubuntuFontDirectory, QLatin1String("fonts/ubuntu-font-family-0.83"), QLatin1String("UbuntuMono-RI.ttf"));

#if defined(Q_OS_LINUX)
    // PLACEMARKER: current Noto Color Emoji font version file extraction
    copyFont(notoFontDirectory, qsl("fonts/noto-color-emoji-2023-11-30-v2.042"), qsl("NotoColorEmoji.ttf"));
    copyFont(notoFontDirectory, qsl("fonts/noto-color-emoji-2023-11-30-v2.042"), qsl("LICENSE"));
#endif // defined(Q_OS_LINUX)
#endif // defined(INCLUDE_FONTS)

    const QString homeLink = qsl("%1/mudlet-data").arg(QDir::homePath());
#if defined(Q_OS_WIN32)
    /*
     * From Qt Documentation for:
     * bool QFile::link(const QString &linkName)
     *
     * "Note: To create a valid link on Windows, linkName must have a .lnk file
     * extension."
     *
     * Whilst the static form:
     * [static] bool QFile::link(const QString &fileName, const QString &linkName)
     * does not mention this particular restriction it is not unreasonable to
     * assume the same condition applies...
     */
    QString homeLinkWindows = qsl("%1/mudlet-data.lnk").arg(QDir::homePath());
    QFile oldLinkFile(homeLink);
    if (oldLinkFile.exists()) {
        // A One-time fix up past error that did not include the ".lnk" extension
        oldLinkFile.rename(homeLinkWindows);
    } else {
        QFile linkFile(homeLinkWindows);
        if (!linkFile.exists()) {
            QFile::link(homeDirectory, homeLinkWindows);
        }
    }
#else
    const QFile linkFile(homeLink);
    if (!linkFile.exists() && first_launch) {
        QFile::link(homeDirectory, homeLink);
    }
#endif

    mudlet::self()->init();

#if defined(Q_OS_WIN)
    // Associate mudlet with .mpackage files
    QSettings settings("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
    settings.setValue(".mpackage", "MudletPackage");
    settings.setValue("MudletPackage/.", "Mudlet Package");
    settings.setValue("MudletPackage/shell/open/command/.", "mudlet %1");
#endif

    // Pass ownership of MudletInstanceCoordinator to mudlet.
    mudlet::self()->takeOwnershipOfInstanceCoordinator(std::move(instanceCoordinator));

    // Handle "QEvent::FileOpen" events.
    FileOpenHandler fileOpenHandler;

    if (first_launch) {
        // give Mudlet window decent size - most of the screen on non-HiDPI
        // displays, on which ever screen it is started up on if it is a virtual
        // multi-screen setup:
        auto pScreen = qApp->primaryScreen();
        // This is the coordinates of the WHOLE of the screen in pixels, for a
        // virtual desktop - this is likely to be a subset of the virtual
        // desktop. However it may also include parts that are used by the OS
        // for taskbars, etc.
        const QRect geometry = pScreen->geometry();
        // The available size within the above that does not include the
        // reserved parts:
        const QSize availableSize = pScreen->availableSize();
        mudlet::self()->resize(availableSize.width() * 3 / 4, availableSize.height() * 3 / 4);
        mudlet::self()->move(geometry.left() + (availableSize.width() / 8), geometry.top() + availableSize.height() / 8);
    }

    if (showSplash) {
        splash.finish(mudlet::self());
    }

    mudlet::self()->smMirrorToStdOut = parser.isSet(mirrorToStdout);
    mudlet::smSteamMode = parser.isSet(steamMode);
    if (!onlyProfiles.isEmpty()) {
        mudlet::self()->onlyShowProfiles(onlyProfiles);
    }
    mudlet::self()->show();

    mudlet::self()->startAutoLogin(cliProfiles);

#if defined(INCLUDE_UPDATER)
    mudlet::self()->checkUpdatesOnStart();
#if !defined(Q_OS_MACOS)
    // Sparkle doesn't allow us to manually show the changelog, so leave it be for dblsqd only
    mudlet::self()->showChangelogIfUpdated();
#endif // Q_OS_LINUX
#endif // INCLUDE_UPDATER

    QTimer::singleShot(2s, qApp, []() {
        if (mudlet::self()->storingPasswordsSecurely()) {
            mudlet::self()->migratePasswordsToSecureStorage();
        }
    });

    app->restoreOverrideCursor();

    // NOTE: Must restore cursor - BEWARE DEBUGGERS if you terminate application
    // without doing/reaching this restore - it can be quite hard to accurately
    // click something in a parent process to the application when you are stuck
    // with some OS's choice of wait cursor - you might wish to temporarily disable
    // the earlier setOverrideCursor() line and this one.
    return app->exec();
}

#if defined(Q_OS_WIN32) && defined(INCLUDE_UPDATER)
// small detour for Windows - check if there's an updated Mudlet
// available to install. If there is, quit and run it - Squirrel
// will update Mudlet and then launch it once it's done
// return true if we should abort the current launch since the updater got started
bool runUpdate()
{
    QFileInfo updatedInstaller(qsl("%1/new-mudlet-setup.exe").arg(QCoreApplication::applicationDirPath()));
    QFileInfo seenUpdatedInstaller(qsl("%1/new-mudlet-setup-seen.exe").arg(QCoreApplication::applicationDirPath()));
    QDir updateDir;
    if (updatedInstaller.exists() && updatedInstaller.isFile() && updatedInstaller.isExecutable()) {
        if (seenUpdatedInstaller.exists() && !updateDir.remove(seenUpdatedInstaller.absoluteFilePath())) {
            qWarning() << "Couldn't delete previous installer: " << seenUpdatedInstaller;
        }

        if (!updateDir.rename(updatedInstaller.absoluteFilePath(), seenUpdatedInstaller.absoluteFilePath())) {
            qWarning() << "Failed to prep installer: couldn't move" << updatedInstaller.absoluteFilePath() << "to" << seenUpdatedInstaller.absoluteFilePath();
        }

        QProcess::startDetached(seenUpdatedInstaller.absoluteFilePath(), QStringList());
        return true;
    } else if (seenUpdatedInstaller.exists() && !updateDir.remove(seenUpdatedInstaller.absoluteFilePath())) {
        // no new updater and only the old one? Then we're restarting from an update: delete the old installer
        qWarning() << "Couldn't delete old uninstaller: " << seenUpdatedInstaller;
    }
    return false;
}
#endif
