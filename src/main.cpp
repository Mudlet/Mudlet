/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2016-2020 by Stephen Lyons                   *
 *                                            - slysven@virginmedia.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
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

#include "pre_guard.h"
#include <chrono>
#include <QDesktopWidget>
#include <QDir>
#if defined(Q_OS_WIN32) && ! defined(INCLUDE_UPDATER)
#include <QMessageBox>
#endif // defined(Q_OS_WIN32) && ! defined(INCLUDE_UPDATER)
#include <QPainter>
#include <QSplashScreen>
#include "post_guard.h"

using namespace std::chrono_literals;

#if defined(_MSC_VER) && defined(_DEBUG)
// Enable leak detection for MSVC debug builds. _DEBUG is MSVC specific and
// leak detection does not work when it is not defined.
#include <Windows.h>
#include <pcre.h>
#endif // _MSC_VER && _DEBUG

TConsole* spDebugConsole = nullptr;

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

QCoreApplication* createApplication(int& argc, char* argv[], unsigned int& action)
{
    action = 0;

// A crude and simplistic commandline options processor - note that Qt deals
// with its options automagically!
#if !(defined(Q_OS_LINUX) || defined(Q_OS_WIN32) || defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD))
    // Handle other currently unconsidered OSs - what are they - by returning the
    // normal GUI type application handle.
    return new QApplication(argc, argv);
#endif

    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--") == 0) {
            break; // Bail out on end of option type arguments
        }

        char argument = 0;
        bool isOption = false;
        if (strlen(argv[i]) > 2 && strncmp(argv[i], "--", 2) == 0) {
            argument = argv[i][2];
            isOption = true;
        } else if (strlen(argv[i]) > 1 && strncmp(argv[i], "-", 1) == 0) {
            argument = argv[i][1];
            isOption = true;
        }

        if (isOption) {
            if (tolower(argument) == 'v') {
                action = 2; // Make this the only action to do and do it directly
                break;
            }

            if (tolower(argument) == 'h' || argument == '?') {
                action = 1; // Make this the only action to do and do it directly
                break;
            }

            if (tolower(argument) == 'q') {
                action |= 4;
            }
        }
    }

    if ((action) & (1 | 2)) {
        return new QCoreApplication(argc, argv);
    } else {
#if defined(Q_OS_MACOS) && (QT_VERSION < QT_VERSION_CHECK(5, 12, 0))
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
        return new QApplication(argc, argv); // Normal course of events - (GUI), so: game on!
    }
}

#if defined(INCLUDE_FONTS)
void copyFont(const QString& externalPathName, const QString& resourcePathName, const QString& fileName)
{
    if (!QFile::exists(QStringLiteral("%1/%2").arg(externalPathName, fileName))) {
        QFile fileToCopy(QStringLiteral(":/%1/%2").arg(resourcePathName,fileName));
        fileToCopy.copy(QStringLiteral("%1/%2").arg(externalPathName, fileName));
    }
}
#endif

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
    spDebugConsole = nullptr;
    unsigned int startupAction = 0;

    // due to a Qt bug, this only safely works for both non- and HiDPI displays on 5.12+
    // 5.6 - 5.11 make the application blow up in size on non-HiDPI displays
#if defined (Q_OS_UNIX) && (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QScopedPointer<QCoreApplication> initApp(createApplication(argc, argv, startupAction));
    auto * app = qobject_cast<QApplication*>(initApp.data());

    // Non-GUI actions --help and --version as suggested by GNU coding standards,
    // section 4.7: http://www.gnu.org/prep/standards/standards.html#Command_002dLine-Interfaces
    QStringList texts;
    if (startupAction & 2) {
        // Do "version" action - wording and format is quite tightly specified by the coding standards
#if defined(QT_DEBUG)
        texts << QCoreApplication::translate("main", "%1 %2%3 (with debug symbols, without optimisations)\n",
         "%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev")
                 .arg(QLatin1String(APP_TARGET), QLatin1String(APP_VERSION), QLatin1String(APP_BUILD));
#else // ! defined(QT_DEBUG)
        texts << QLatin1String(APP_TARGET " " APP_VERSION APP_BUILD " \n");
#endif // ! defined(QT_DEBUG)
        texts << QCoreApplication::translate("main", "Qt libraries %1 (compilation) %2 (runtime)\n",
             "%1 and %2 are version numbers").arg(QLatin1String(QT_VERSION_STR), qVersion());
        texts << QCoreApplication::translate("main", "Copyright © 2008-%1  Mudlet developers\n").arg(QStringLiteral(__DATE__).mid(7, 4));
        texts << QCoreApplication::translate("main", "Licence GPLv2+: GNU GPL version 2 or later - http://gnu.org/licenses/gpl.html\n");
        texts << QCoreApplication::translate("main", "This is free software: you are free to change and redistribute it.\n"
                                                     "There is NO WARRANTY, to the extent permitted by law.\n");
        std::cout << texts.join(QString()).toStdString();
        return 0;
    } else if (startupAction & 1) {
        // Do "help" action
        texts << QCoreApplication::translate("main", "Usage: %1 [OPTION...]\n"
                                                     "       -h, --help      displays this message.\n"
                                                     "       -v, --version   displays version information.\n"
                                                     "       -q, --quiet     no splash screen on startup.\n\n"
                                                     "There are other inherited options that arise from the Qt Libraries which are\n"
                                                     "less likely to be useful for normal use of this application:\n")
                 .arg(QLatin1String(APP_TARGET));
        // From documentation and from http://qt-project.org/doc/qt-5/qapplication.html:
        texts << QStringLiteral("       --dograb        ignore any implicit or explicit -nograb.\n"
                                "                       --dograb wins over --nograb even when --nograb is last on\n"
                                "                       the command line.\n");
#if defined(Q_OS_LINUX)
        texts << QStringLiteral("       --nograb        the application should never grab the mouse or the\n"
                                "                       keyboard. This option is set by default when Mudlet is\n"
                                "                       running in the gdb debugger under Linux.\n");
#else // ! defined(Q_OS_LINUX)
        texts << QStringLiteral("       --nograb        the application should never grab the mouse or the\n"
                                "                       keyboard.\n");
#endif // ! defined(Q_OS_LINUX)
        texts << QStringLiteral("       --reverse       sets the application's layout direction to right to left.\n"
                                "       --style= style  sets the application GUI style. Possible values depend on\n"
                                "                       your system configuration. If Qt was compiled with\n"
                                "                       additional styles or has additional styles as plugins\n"
                                "                       these will be available to the -style command line\n"
                                "                       option. You can also set the style for all Qt\n"
                                "                       applications by setting the QT_STYLE_OVERRIDE environment\n"
                                "                       variable.\n"
                                "       --style style   is the same as listed above.\n"
                                "       --stylesheet= stylesheet  sets the application styleSheet.\n"
                                "                       The value must be a path to a file that contains the\n"
                                "                       Style Sheet. Note: Relative URLs in the Style Sheet file\n"
                                "                       are relative to the Style Sheet file's path.\n"
                                "       --stylesheet stylesheet  is the same as listed above.\n");
// Not sure about MacOS case as that does not use X
#if defined(Q_OS_UNIX) && (! defined(Q_OS_MACOS))
        texts << QStringLiteral("       --sync          forces the X server to perform each X client request\n"
                                "                       immediately and not use buffer optimization. It makes the\n"
                                "                       program easier to debug and often much slower. The --sync\n"
                                "                       option is only valid for the X11 version of Qt.\n");
#endif // defined(Q_OS_UNIX) and not defined(Q_OS_MACOS)
        texts << QStringLiteral("       --widgetcount   prints debug message at the end about number of widgets\n"
                                "                       left undestroyed and maximum number of widgets existing\n"
                                "                       at the same time.\n"
                                "       --qmljsdebugger=1234[,block]  activates the QML/JS debugger with a\n"
                                "                       specified port. The number is the port value and block is\n"
                                "                       optional and will make the application wait until a\n"
                                "                       debugger connects to it.\n\n");
        texts << QCoreApplication::translate("main", "Report bugs to: https://github.com/Mudlet/Mudlet/issues\n");
        texts << QCoreApplication::translate("main", "Project home page: http://www.mudlet.org/\n");
        std::cout << texts.join(QString()).toStdString();
        return 0;
    }

    /*******************************************************************
     * If we get to HERE then we are going to run a GUI application... *
     *******************************************************************/

#if defined(Q_OS_WIN32) && defined(INCLUDE_UPDATER)
    auto abortLaunch = runUpdate();
    if (abortLaunch) {
        return 0;
    }
#endif

    // Turn the cursor into the waiting one during startup, so something shows
    // activity even if the quiet, no splashscreen startup has been used
    app->setOverrideCursor(QCursor(Qt::WaitCursor));
    app->setOrganizationName(QStringLiteral("Mudlet"));

    if (mudlet::scmIsPublicTestVersion) {
        app->setApplicationName(QStringLiteral("Mudlet Public Test Build"));
        app->setApplicationVersion(APP_VERSION APP_BUILD);
    } else {
        app->setApplicationName(QStringLiteral("Mudlet"));
        app->setApplicationVersion(APP_VERSION);
    }


    bool show_splash = !(startupAction & 4); // Not --quiet.
#if defined(INCLUDE_VARIABLE_SPLASH_SCREEN)
    QImage splashImage(mudlet::scmIsReleaseVersion ? QStringLiteral(":/Mudlet_splashscreen_main.png")
                                                   : mudlet::scmIsPublicTestVersion ? QStringLiteral(":/Mudlet_splashscreen_ptb.png")
                                                                                    : QStringLiteral(":/Mudlet_splashscreen_development.png"));
#else
    QImage splashImage(QStringLiteral(":/Mudlet_splashscreen_main.png"));
#endif

    if (show_splash) {
        QPainter painter(&splashImage);
        unsigned fontSize = 16;
        QString sourceVersionText = QString(QCoreApplication::translate("main", "Version: %1").arg(APP_VERSION APP_BUILD));

        bool isWithinSpace = false;
        while (!isWithinSpace) {
            QFont font("DejaVu Serif", fontSize, QFont::Bold | QFont::Serif | QFont::PreferMatch | QFont::PreferAntialias);
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
            QTextLine dummy = versionTextLayout.createLine();
            if (!dummy.isValid()) {
                // No second line so have got all text in first so can do it
                isWithinSpace = true;
                qreal versionTextWidth = versionTextline.naturalTextWidth();
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
        QString sourceCopyrightText = QStringLiteral("©️ Mudlet makers 2008-2020");
        QFont font(QStringLiteral("DejaVu Serif"), 16, QFont::Bold | QFont::Serif | QFont::PreferMatch | QFont::PreferAntialias);
        QTextLayout copyrightTextLayout(sourceCopyrightText, font, painter.device());
        copyrightTextLayout.beginLayout();
        QTextLine copyrightTextline = copyrightTextLayout.createLine();
        copyrightTextline.setLineWidth(280);
        copyrightTextline.setPosition(QPointF(1, 1));
        qreal copyrightTextWidth = copyrightTextline.naturalTextWidth();
        copyrightTextline.setPosition(QPointF((320 - copyrightTextWidth) / 2.0, 340));
        copyrightTextLayout.endLayout();
        painter.setPen(QColor(112, 16, 0, 255)); // #701000
        copyrightTextLayout.draw(&painter, QPointF(0, 0));
    }
    QPixmap pixmap = QPixmap::fromImage(splashImage);
    QSplashScreen splash(pixmap);
    if (show_splash) {
        splash.show();
    }
    app->processEvents();

    // seed random number generator (should be done once per lifetime)
    qsrand(static_cast<quint64>(QTime::currentTime().msecsSinceStartOfDay()));

    QString homeDirectory = mudlet::getMudletPath(mudlet::mainPath);
    QDir dir;
    bool first_launch = false;
    if (!dir.exists(homeDirectory)) {
        dir.mkpath(homeDirectory);
        first_launch = true;
    }

#if defined(INCLUDE_FONTS)
    QString bitstreamVeraFontDirectory(QStringLiteral("%1/ttf-bitstream-vera-1.10").arg(mudlet::getMudletPath(mudlet::mainFontsPath)));
    if (!dir.exists(bitstreamVeraFontDirectory)) {
        dir.mkpath(bitstreamVeraFontDirectory);
    }
    QString ubuntuFontDirectory(QStringLiteral("%1/ubuntu-font-family-0.83").arg(mudlet::getMudletPath(mudlet::mainFontsPath)));
    if (!dir.exists(ubuntuFontDirectory)) {
        dir.mkpath(ubuntuFontDirectory);
    }
#if defined(Q_OS_LINUX)
    // Only needed/works on Linux to provide color emojis:
    QString notoFontDirectory(QStringLiteral("%1/noto-color-emoji-2019-11-19-unicode12").arg(mudlet::getMudletPath(mudlet::mainFontsPath)));
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
    copyFont(notoFontDirectory, QStringLiteral("fonts/noto-color-emoji-2019-11-19-unicode12"), QStringLiteral("NotoColorEmoji.ttf"));
    copyFont(notoFontDirectory, QStringLiteral("fonts/noto-color-emoji-2019-11-19-unicode12"), QStringLiteral("LICENSE"));
    copyFont(notoFontDirectory, QStringLiteral("fonts/noto-color-emoji-2019-11-19-unicode12"), QStringLiteral("README"));
#endif // defined(Q_OS_LINUX)
#endif // defined(INCLUDE_FONTS)

    mudlet::debugMode = false;

    QString homeLink = QStringLiteral("%1/mudlet-data").arg(QDir::homePath());
#ifdef Q_OS_WIN32
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
    QString homeLinkWindows = QStringLiteral("%1/mudlet-data.lnk").arg(QDir::homePath());
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
    QFile linkFile(homeLink);
    if (!linkFile.exists() && first_launch) {
        QFile::link(homeDirectory, homeLink);
    }
#endif

    mudlet::start();

    if (first_launch) {
        // give Mudlet window decent size - most of the screen on non-HiDPI displays
        auto desktop = qApp->desktop();
        auto initialSpace = desktop->availableGeometry(desktop->screenNumber());
        mudlet::self()->resize(initialSpace.width() * 3 / 4, initialSpace.height() * 3 / 4);
        mudlet::self()->move(initialSpace.width() / 8, initialSpace.height() / 8);
    }

    if (show_splash) {
        splash.finish(mudlet::self());
    }

    mudlet::self()->show();

    mudlet::self()->startAutoLogin();

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

        mudlet::self()->updateMudletDiscordInvite();
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
    QFileInfo updatedInstaller(QCoreApplication::applicationDirPath() + QStringLiteral("/new-mudlet-setup.exe"));
    QFileInfo seenUpdatedInstaller(QCoreApplication::applicationDirPath() + QStringLiteral("/new-mudlet-setup-seen.exe"));
    QDir updateDir;
    if (updatedInstaller.exists() && updatedInstaller.isFile() && updatedInstaller.isExecutable()) {
        if (!updateDir.remove(seenUpdatedInstaller.absoluteFilePath())) {
            qWarning() << "Couldn't delete previous installer";
        }

        if (!updateDir.rename(updatedInstaller.absoluteFilePath(), seenUpdatedInstaller.absoluteFilePath())) {
            qWarning() << "Failed to prep installer: couldn't rename it";
        }

        QProcess::startDetached(seenUpdatedInstaller.absoluteFilePath());
        return true;
    } else if (seenUpdatedInstaller.exists() && !updateDir.remove(seenUpdatedInstaller.absoluteFilePath())) {
         // no new updater and only the old one? Then we're restarting from an update: delete the old installer
        qWarning() << "Couldn't delete old uninstaller";
    }
    return false;
}
#endif
