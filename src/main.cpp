/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2016-2021, 2023-2025 by Stephen Lyons        *
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
#include <chrono>
#include <QCommandLineParser>
#include <QDir>
#if defined(Q_OS_WINDOWS) && !defined(INCLUDE_UPDATER)
#include <QMessageBox>
#endif // defined(Q_OS_WINDOWS) && !defined(INCLUDE_UPDATER)
#include <QCommandLineOption>
#include <QPainter>
#include <QPointer>
#include <QScreen>
#include <QSettings>
#include <QSplashScreen>
#include <QStringList>
#include <QTranslator>
#include "AltFocusMenuBarDisable.h"
#include "TAccessibleConsole.h"
#include "TAccessibleTextEdit.h"
#include "FileOpenHandler.h"

#if defined(Q_OS_WINDOWS) && defined(INCLUDE_UPDATER)
#include <windows.h>
#include <QThread>
#endif

using namespace std::chrono_literals;

#if defined(Q_OS_WINDOWS)
bool runUpdate();
#endif

#if defined(INCLUDE_FONTS)
void copyFont(const QString& externalPathName, const QString& resourcePathName, const QString& fileName)
{
    if (!QFile::exists(qsl("%1/%2").arg(externalPathName, fileName))) {
        QFile fileToCopy(qsl(":/%1/%2").arg(resourcePathName, fileName));
        fileToCopy.copy(qsl("%1/%2").arg(externalPathName, fileName));
    }
}

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
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
    oldNotoFontDirectories << qsl("%1/notocoloremoji-unhinted-2018-04-24-pistol-update").arg(mudlet::getMudletPath(enums::mainFontsPath));
    // Release: "v2019-11-19-unicode12"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2019-11-19-unicode12").arg(mudlet::getMudletPath(enums::mainFontsPath));
    // Release: "Noto Emoji v2.0238"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2021-07-15-v2.028").arg(mudlet::getMudletPath(enums::mainFontsPath));
    // Release: "Unicode 14.0"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2021-11-01-v2.034").arg(mudlet::getMudletPath(enums::mainFontsPath));
    // Release: "Unicode 15.0"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2022-09-16-v2.038").arg(mudlet::getMudletPath(enums::mainFontsPath));
    // Release: "Unicode 15.1, take 3"
    oldNotoFontDirectories << qsl("%1/noto-color-emoji-2023-11-30-v2.042").arg(mudlet::getMudletPath(enums::mainFontsPath));

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
    QSettings* pSettings = mudlet::getQSettings();
    auto interfaceLanguage = pSettings->value(QLatin1String("interfaceLanguage")).toString();
    auto userLocale = interfaceLanguage.isEmpty() ? QLocale::system() : QLocale(interfaceLanguage);
    if (userLocale == QLocale::c()) {
        // nothing found
        return nullptr;
    }
    // We only need the Mudlet translations for the Command Line texts, no need
    // for any Qt ones:
    QTranslator* pMudletTranslator = new QTranslator(qApp);
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

#ifdef Q_OS_WINDOWS
void msys2QtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context)
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
        std::cout << msg.toUtf8().constData() << std::endl;
        break;
    case QtWarningMsg:
    case QtCriticalMsg:
    case QtFatalMsg:
        std::cerr << msg.toUtf8().constData() << std::endl;
    }
}
#endif

int main(int argc, char* argv[])
{
#ifdef Q_OS_WINDOWS
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (qgetenv("MSYSTEM").isNull()) {
            // print stdout to console if Mudlet is started in a console in Windows
            // credit to https://stackoverflow.com/a/41701133 for the workaround
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        } else {
            // simply print qt logs into stdout and stderr if it's MSYS2
            qInstallMessageHandler(msys2QtMessageHandler);
        }
    }
#endif

#if defined(Q_OS_MACOS)
    // Workaround for horrible mac rendering issues once the mapper widget
    // is open - see https://bugreports.qt.io/browse/QTBUG-41257
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#elif defined(Q_OS_FREEBSD)
#if defined(INCLUDE_3DMAPPER)
    // Cure for diagnostic:
    // "Qt WebEngine seems to be initialized from a plugin. Please set
    // Qt::AA_ShareOpenGLContexts using QCoreApplication::setAttribute
    // before constructing QGuiApplication."
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif // INCLUDE_3DMAPPER
#endif

    auto app = qobject_cast<QApplication*>(new QApplication(argc, argv));

    QAccessible::installFactory(TAccessibleConsole::consoleFactory);
    QAccessible::installFactory(TAccessibleTextEdit::textEditFactory);

#if defined(Q_OS_WINDOWS) && defined(INCLUDE_UPDATER)
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
    if (!gitShaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "main: failed to open app-build.txt for reading:" << gitShaFile.errorString();
    }
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

    const QCommandLineOption startFullscreen(QStringList() << qsl("f") << qsl("fullscreen"), qsl("Start Mudlet in fullscreen mode"));
    parser.addOption(startFullscreen);

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
        texts << appendLF.arg(QCoreApplication::translate("main", "       -f, --fullscreen             start Mudlet in fullscreen mode."));
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
        texts << appendLF.arg(QCoreApplication::translate("main", "Copyright © 2008-2025  Mudlet developers"));
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

    // Needed for Qt6 on Windows (at least) - and does not work in mudlet class c'tor
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
#if defined(Q_OS_WINDOWS)
    if (qEnvironmentVariableIsEmpty("QT_MEDIA_BACKEND")) {
        // This variable is not set - and later versions of Qt 6.x need it for
        // sound to work:
        if (qputenv("QT_MEDIA_BACKEND", QByteArray("windows"))) {
            qDebug().noquote() << "main(...) INFO - setting QT_MEDIA_BACKEND enviromental variable to: \"windows\".";
        } else {
            qWarning().noquote() << "main(...) WARNING - failed to set QT_MEDIA_BACKEND enviromental variable to: \"windows\", sound may not work.";
        }
    } else {
        qDebug().noquote().nospace() << "main(...) INFO - QT_MEDIA_BACKEND enviromental variable is set to: \"" << qgetenv("QT_MEDIA_BACKEND") << "\".";
    }
#endif
#endif

    QStringList cliProfiles = parser.values(profileToOpen);

    if (cliProfiles.isEmpty()) {
        const QString envProfiles = QString::fromLocal8Bit(qgetenv("MUDLET_PROFILES"));
        if (!envProfiles.isEmpty()) {
            // : is not an allowed character in a profile name, so we can use it to split the list
            cliProfiles = envProfiles.split(':');
        }
    }

    const QStringList onlyProfiles = parser.values(onlyPredefinedProfileToShow);
    const bool showSplash = parser.isSet(showSplashscreen);
    QImage splashImage = mudlet::getSplashScreen(releaseVersion, publicTestVersion);

    if (showSplash) {
        QPainter painter(&splashImage);
        unsigned fontSize = 16;
        const QString sourceVersionText = QString(QCoreApplication::translate("main", "Version: %1").arg(APP_VERSION + appBuild));

        bool isWithinSpace = false;
        while (!isWithinSpace) {
            QFont font(qsl("Bitstream Vera Serif"), fontSize, 75);
            font.setStyleHint(QFont::Serif, QFont::StyleStrategy(QFont::PreferMatch | QFont::PreferAntialias));
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
        const QString sourceCopyrightText = qsl("©️ Mudlet makers 2008-2025");
        QFont font(qsl("Bitstream Vera Serif"), 16, 75);
        font.setStyleHint(QFont::Serif, QFont::StyleStrategy(QFont::PreferMatch | QFont::PreferAntialias));
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
    // Specifying the screen here seems to help to put the splash screen on the
    // same monitor that the main application window will be put upon on first
    // run, in some situations the two can otherwise get to be different which
    // is misleading unhelpful to a new user...!
    QSplashScreen splash(qApp->primaryScreen(), pixmap);

    if (showSplash) {
        splash.show();
    }
    app->processEvents();

    const QString homeDirectory = mudlet::getMudletPath(enums::mainPath);
    const QDir dir;
    bool first_launch = false;
    if (!dir.exists(homeDirectory)) {
        dir.mkpath(homeDirectory);
        first_launch = true;
    }

#if defined(INCLUDE_FONTS)
    const QString bitstreamVeraFontDirectory(qsl("%1/ttf-bitstream-vera-1.10").arg(mudlet::getMudletPath(enums::mainFontsPath)));
    if (!dir.exists(bitstreamVeraFontDirectory)) {
        dir.mkpath(bitstreamVeraFontDirectory);
    }
    const QString ubuntuFontDirectory(qsl("%1/ubuntu-font-family-0.83").arg(mudlet::getMudletPath(enums::mainFontsPath)));
    if (!dir.exists(ubuntuFontDirectory)) {
        dir.mkpath(ubuntuFontDirectory);
    }
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    // Only needed/works on GNU/Linux and FreeBSD to provide color emojis:
    removeOldNoteColorEmojiFonts();
    // PLACEMARKER: current Noto Color Emoji font directory specification:
    // Release: "Unicode 16.0"
    const QString notoFontDirectory{qsl("%1/noto-color-emoji-2024-10-03-v2.047").arg(mudlet::getMudletPath(enums::mainFontsPath))};
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

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    // PLACEMARKER: current Noto Color Emoji font version file extraction
    copyFont(notoFontDirectory, qsl("fonts/noto-color-emoji-2024-10-03-v2.047"), qsl("NotoColorEmoji.ttf"));
    copyFont(notoFontDirectory, qsl("fonts/noto-color-emoji-2024-10-03-v2.047"), qsl("LICENSE"));
#endif // defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#endif // defined(INCLUDE_FONTS)

    const QString homeLink = qsl("%1/mudlet-data").arg(QDir::homePath());
#if defined(Q_OS_WINDOWS)
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
    if (parser.isSet(startFullscreen)) {
        QTimer::singleShot(0, [=]() {
            mudlet::self()->showFullScreen();
        });
    }

    QTimer::singleShot(0, qApp, [cliProfiles]() {
        // ensure Mudlet singleton is initialised before calling profile loading
        mudlet::self()->startAutoLogin(cliProfiles);
    });

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

#if defined(Q_OS_WINDOWS) && defined(INCLUDE_UPDATER)
// Helper function to check if a file is accessible (not locked by another process)
// Returns true if file can be accessed, false if locked
static bool isFileAccessible(const QString& filePath)
{
    // Try opening file with exclusive write access
    HANDLE hFile = CreateFileW(
        reinterpret_cast<const wchar_t*>(filePath.utf16()),
        GENERIC_WRITE,
        0, // No sharing - exclusive access
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION) {
            qWarning() << "File is locked:" << filePath << "- error code:" << error;
            return false; // File is locked
        }
        // File doesn't exist or other error - consider it accessible
        return true;
    }

    CloseHandle(hFile);
    return true;
}

// Helper function to try a file operation with retry logic
// Returns true if operation succeeded, false if all retries failed
static bool tryFileOperationWithRetry(const std::function<bool()>& operation, const QString& operationName, int maxAttempts = 3)
{
    const std::chrono::milliseconds retryDelays[] = {5000ms, 15000ms, 30000ms};

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        if (attempt > 0) {
            qWarning() << operationName << "- Attempt" << (attempt + 1) << "of" << maxAttempts
                      << "after" << retryDelays[attempt - 1].count() << "ms delay";
            QThread::msleep(retryDelays[attempt - 1].count());
        }

        if (operation()) {
            if (attempt > 0) {
                qWarning() << operationName << "- Succeeded on attempt" << (attempt + 1);
            }
            return true;
        }

        qWarning() << operationName << "- Failed on attempt" << (attempt + 1);
    }

    qWarning() << operationName << "- All" << maxAttempts << "attempts failed";
    return false;
}

// Small detour for Windows - check if there's an updated Mudlet
// available to install. If there is, quit and run it - Squirrel
// will update Mudlet and then launch it once it's done.
//
// Return true if we should abort the current launch since the updater got started
bool runUpdate()
{
    QFileInfo updatedInstaller(qsl("%1/new-mudlet-setup.exe").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
    QFileInfo seenUpdatedInstaller(qsl("%1/new-mudlet-setup-seen.exe").arg(QCoreApplication::applicationDirPath()));
    QDir updateDir;

    if (updatedInstaller.exists() && updatedInstaller.isFile() && updatedInstaller.isExecutable()) {
        // Verify the new installer is accessible before trying to move it
        if (!isFileAccessible(updatedInstaller.absoluteFilePath())) {
            qWarning() << "New installer exists but is locked, cannot proceed with update:" << updatedInstaller.absoluteFilePath();
            qWarning() << "Update will be attempted on next Mudlet restart";
            return false;
        }

        // Try to remove old installer if it exists
        if (seenUpdatedInstaller.exists()) {
            bool removed = tryFileOperationWithRetry([&]() {
                return isFileAccessible(seenUpdatedInstaller.absoluteFilePath()) &&
                       updateDir.remove(seenUpdatedInstaller.absoluteFilePath());
            }, qsl("Delete previous installer"));

            if (!removed) {
                qWarning() << "Couldn't delete previous installer after retries:" << seenUpdatedInstaller;
                qWarning() << "Update aborted to prevent potential issues";
                return false;
            }
        }

        // Try to move the installer with retry logic
        bool moved = tryFileOperationWithRetry([&]() {
            return isFileAccessible(updatedInstaller.absoluteFilePath()) &&
                   updateDir.rename(updatedInstaller.absoluteFilePath(), seenUpdatedInstaller.absoluteFilePath());
        }, qsl("Move installer to application directory"));

        if (!moved) {
            qWarning() << "Failed to prep installer: couldn't move" << updatedInstaller.absoluteFilePath()
                      << "to" << seenUpdatedInstaller.absoluteFilePath() << "after all retries";
            qWarning() << "Update will be attempted on next Mudlet restart";
            return false;
        }

        // Verify the installer is still accessible before launching
        if (!isFileAccessible(seenUpdatedInstaller.absoluteFilePath())) {
            qWarning() << "Installer was moved but is now locked, cannot launch:" << seenUpdatedInstaller.absoluteFilePath();
            qWarning() << "Update will be attempted on next Mudlet restart";
            return false;
        }

        qWarning() << "Launching installer:" << seenUpdatedInstaller.absoluteFilePath();
        QProcess::startDetached(seenUpdatedInstaller.absoluteFilePath(), QStringList());
        return true;
    } else if (seenUpdatedInstaller.exists()) {
        // no new updater and only the old one? Then we're restarting from an update: delete the old installer
        if (!updateDir.remove(seenUpdatedInstaller.absoluteFilePath())) {
            qWarning() << "Couldn't delete old installer:" << seenUpdatedInstaller;
        } else {
            qDebug() << "Successfully cleaned up old installer after update";
        }
    }
    return false;
}
#endif
