/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2016 by Stephen Lyons                        *
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


#include "FontManager.h"
#include "HostManager.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QSplashScreen>
#include <QStringBuilder>
#include <QTextLayout>
#include "post_guard.h"

#if defined(_MSC_VER) && defined(_DEBUG)
// Enable leak detection for MSVC debug builds. _DEBUG is MSVC specific and
// leak detection does not work when it is not defined.
#include <Windows.h>
#include <pcre.h>
#endif // _MSC_VER && _DEBUG

#include <iostream>


using namespace std;

TConsole *  spDebugConsole = 0;

extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;

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

QCoreApplication * createApplication(int &argc, char *argv[], unsigned int &action)
{
    action = 0;

// A crude and simplistic commandline options processor - note that Qt deals
// with its options automagically!
#if ! ( defined(Q_OS_LINUX) || defined(Q_OS_WIN32) || defined(Q_OS_MAC) )
// Handle other currently unconsidered OSs - what are they - by returning the
// normal GUI type application handle.
    return new QApplication(argc, argv);
#endif

    for(int i = 1; i < argc; ++i)
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        if( qstrcmp(argv[i], "--") == 0 )
            break; // Bail out on end of option type arguments
#endif

        char argument = 0;
        bool isOption = false;
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        if( strlen(argv[i]) > 2 && strncmp(argv[i], "--", 2) == 0 )
        {
            argument = argv[i][2];
            isOption = true;
        }
        else if( strlen(argv[i]) > 1 && strncmp(argv[i], "-", 1) == 0 )
        {
            argument = argv[i][1];
            isOption = true;
        }
#elif defined(Q_OS_WIN32)
// TODO: Do Qt builds for Windows use Unix '-' as option prefix or is the normal (for them) '/' used - as assumed here and in the help text
        if( strlen(argv[i]) > 1 && strncmp(argv[i], "/", 1) == 0 )
        {
            argument = argv[i][1];
            isOption = true;
        }
#endif

        if( isOption )
        {
            if(tolower(argument) == 'v')
            {
                action = 2; // Make this the only action to do and do it directly
                break;
            }

            if(tolower(argument) == 'h' || argument == '?')
            {
                action = 1; // Make this the only action to do and do it directly
                break;
            }

            if(tolower(argument) == 'q')
            {
                action |= 4;
            }

        }
    }

    if( (action) & ( 1 | 2) )
        return new QCoreApplication(argc, argv);  // Ah, we're gonna bail out early, just need a command-line application
    else {
#if defined (Q_OS_MACOS)
        // Workaround for horrible mac rendering issues once the mapper widget is open
        // see https://bugreports.qt.io/browse/QTBUG-41257
        QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif
#if defined (Q_OS_WIN32)        
        // Force OpenGL use as we use some functions that aren't provided by Qt's OpenGL layer on Windows (QOpenGLFunctions)
        QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#endif        
        return new QApplication(argc, argv); // Normal course of events - (GUI), so: game on!
    }
}

int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && defined(_DEBUG)
    // Enable leak detection for MSVC debug builds.
    {
        // Check for a debugger and prompt if one is not attached.
        while (!IsDebuggerPresent()
               && IDYES == MessageBox(0,
                                      "You are starting debug mudlet without a debugger attached. If you wish to attach one and verify that it worked, click yes. To continue without a debugger, click no.",
                                      "Mudlet Debug",
                                      MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2))
            ;

        // _CRTDBG_ALLOC_MEM_DF: Enable heap debugging.
        // _CRTDBG_LEAK_CHECK_DF: Check for leaks at program exit.
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

        // Create a log file for writing leaks.
        HANDLE hLogFile = CreateFile("stderr.txt", GENERIC_WRITE,
            FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
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
    spDebugConsole = 0;
    unsigned int startupAction = 0;

    Q_INIT_RESOURCE(mudlet_alpha);

    QScopedPointer<QCoreApplication> initApp(createApplication(argc, argv, startupAction));

    QApplication * app = qobject_cast<QApplication *>(initApp.data());

    // Non-GUI actions --help and --version as suggested by GNU coding standards,
    // section 4.7: http://www.gnu.org/prep/standards/standards.html#Command_002dLine-Interfaces
    if( app == 0 )
    {
        if( startupAction & 2 )
        {
            // Do "version" action - wording and format is quite tightly specified by the coding standards
            std::cout << APP_TARGET << " " << APP_VERSION << APP_BUILD << std::endl;
            std::cout << "Qt libraries " << QT_VERSION_STR << "(compilation) " << qVersion() << "(runtime)" << std::endl;
            std::cout << "Copyright (C) 2008-" << std::string(__DATE__).substr(7, 4) << " Mudlet devs." << std::endl;
            std::cout << "Licence GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>" << std::endl;
            std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
            std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
        }
        if( startupAction & 1 )
        {
            // Do "help" action -
            std::cout << "Usage: " << std::string(APP_TARGET) << "[OPTION...]" <<std::endl;
#if defined (Q_OS_WIN32)
            std::cout << "   /h, /help           displays this message." << std::endl;
            std::cout << "   /v, /version        displays version information." << std::endl;
            std::cout << "   /q, /quiet          no splash screen on startup." << std::endl;
#define OPT_PREFIX '/'
#else
            std::cout << "   -h, --help          displays this message." << std::endl;
            std::cout << "   -v, --version       displays version information." << std::endl;
            std::cout << "   -q, --quiet         no splash screen on startup." << std::endl;
#define OPT_PREFIX '-'
#endif
            std::cout << "There are other inherited options that arise from the Qt Libraries which" << std::endl;
            std::cout << "are not likely to be useful for normal use of this application:" << std::endl;
// From documentation and from http://qt-project.org/doc/qt-5/qapplication.html:
            std::cout << "       " << OPT_PREFIX << "dograb         ignore any implicit or explicit -nograb." << std::endl;
            std::cout << "                       " << OPT_PREFIX << "dograb wins over " << OPT_PREFIX <<"nograb even when" << std::endl;
            std::cout << "                       " << OPT_PREFIX << "nograb is last on the command line." << std::endl;
            std::cout << "       " << OPT_PREFIX << "nograb         the application should never grab the mouse or the"<< std::endl;
#if defined( Q_OS_LINUX )
            std::cout << "                       keyboard. This option is set by default when Mudlet is" << std::endl;
            std::cout << "                       running in the gdb debugger under Linux." << std::endl;
#else
            std::cout << "                       keyboard." << std::endl;
#endif
            std::cout << "        " << OPT_PREFIX << "reverse       sets the application's layout direction to" << std::endl;
            std::cout << "                       right to left." << std::endl;
            std::cout << "        " << OPT_PREFIX << "style= style  sets the application GUI style. Possible values depend"  << std::endl;
            std::cout << "                       on your system configuration. If Qt was compiled with" << std::endl;
            std::cout << "                       additional styles or has additional styles as plugins" << std::endl;
            std::cout << "                       these will be available to the -style command line" << std::endl;
            std::cout << "                       option. You can also set the style for all Qt" << std::endl;
            std::cout << "                       applications by setting the QT_STYLE_OVERRIDE environment" << std::endl;
            std::cout << "                       variable." << std::endl;
            std::cout << "        " << OPT_PREFIX << "style style   is the same as listed above." << std::endl;
            std::cout << "        " << OPT_PREFIX << "stylesheet= stylesheet" << std::endl;
            std::cout << "                       sets the application styleSheet." << std::endl;
            std::cout << "                       The value must be a path to a file that contains the" << std::endl;
            std::cout << "                       Style Sheet. Note: Relative URLs in the Style Sheet" << std::endl;
            std::cout << "                       file are relative to the Style Sheet file's path." << std::endl;
            std::cout << "        " << OPT_PREFIX << "stylesheet stylesheet" << std::endl;
            std::cout << "                       is the same as listed above." << std::endl;
#if defined( Q_OS_UNIX )
            std::cout << "        " << OPT_PREFIX << "sync          runs Mudlet in X synchronous mode. Synchronous mode" << std::endl;
            std::cout << "                       forces the X server to perform each X client request" << std::endl;
            std::cout << "                       immediately and not use buffer optimization. It makes" << std::endl;
            std::cout << "                       the program easier to debug and often much slower. The" << std::endl;
            std::cout << "                       -sync option is only valid for the X11 version of Qt." << std::endl;
#endif
            std::cout << "        " << OPT_PREFIX << "widgetcount   prints debug message at the end about number of widgets" << std::endl;
            std::cout << "                       left undestroyed and maximum number of widgets existing" << std::endl;
            std::cout << "                       at the same time." << std::endl;
            std::cout << "        " << OPT_PREFIX << "qmljsdebugger=1234[,block]" << std::endl;
            std::cout << "                       activates the QML/JS debugger with a specified port." << std::endl;
            std::cout << "                       The number is the port value and block is optional" << std::endl;
            std::cout << "                       and will make the application wait until a debugger"  << std::endl;
            std::cout << "                       connects to it." << std::endl;
            std::cout << std::endl;
            std::cout << "Report bugs to: <https://github.com/Mudlet/Mudlet/issues>" << std::endl;
            std::cout << "pkg home page: <http://www.mudlet.org/>" << std::endl;
        }
        return 0;
    }

// Turn the cursor into the waiting one during startup, so something shows
// activity even if the quiet, no splashscreen startup has been used
    app->setOverrideCursor(QCursor(Qt::WaitCursor));
    app->setOrganizationName("Mudlet");
    app->setApplicationName("Mudlet");
    app->setApplicationVersion(APP_VERSION);

    bool show_splash = !(startupAction & 4); // Not --quiet.

    QImage splashImage(":/Mudlet_splashscreen_main.png");
    if (show_splash) {
        QPainter painter( &splashImage );
        unsigned fontSize = 16;
        QString sourceVersionText = QString( "Version: " APP_VERSION APP_BUILD );

        bool isWithinSpace = false;
        while( ! isWithinSpace )
        {
            QFont font( "DejaVu Serif", fontSize, QFont::Bold|QFont::Serif|QFont::PreferMatch|QFont::PreferAntialias );
            QTextLayout versionTextLayout( sourceVersionText, font, painter.device() );
            versionTextLayout.beginLayout();
            // Start work in this text item
            QTextLine versionTextline = versionTextLayout.createLine();
            // First draw (one line from) the text we have put in on the layout to
            // see how wide it is..., assuming accutally that it will only take one
            // line of text
            versionTextline.setLineWidth( 280 );
            //Splashscreen bitmap is (now) 320x360 - hopefully entire line will all fit into 280
            versionTextline.setPosition( QPointF( 0, 0 ) );
            // Only pretend, so we can see how much space it will take
            QTextLine dummy = versionTextLayout.createLine();
            if( ! dummy.isValid() )
            { // No second line so have got all text in first so can do it
                isWithinSpace = true;
                qreal versionTextWidth = versionTextline.naturalTextWidth();
                // This is the ACTUAL width of the created text
                versionTextline.setPosition( QPointF( (320 - versionTextWidth) / 2.0 , 270 ) );
                // And now we can place it centred horizontally
                versionTextLayout.endLayout();
                // end the layout process and paint it out
                painter.setPen( QColor( 176, 64, 0, 255 ) ); // #b04000
                versionTextLayout.draw( &painter, QPointF( 0, 0 ) );
            }
            else
            { // Too big - text has spilled over onto a second line - so try again
                fontSize--;
                versionTextLayout.clearLayout();
                versionTextLayout.endLayout();
            }
        }

        // Repeat for other text, but we know it will fit at given size
        QString sourceCopyrightText = QChar( 169 ) % QString( " Mudlet makers 2008-" ) % QString(__DATE__).mid(7);
        QFont font( "DejaVu Serif", 16, QFont::Bold|QFont::Serif|QFont::PreferMatch|QFont::PreferAntialias );
        QTextLayout copyrightTextLayout( sourceCopyrightText, font, painter.device() );
        copyrightTextLayout.beginLayout();
        QTextLine copyrightTextline = copyrightTextLayout.createLine();
        copyrightTextline.setLineWidth( 280 );
        copyrightTextline.setPosition( QPointF( 1, 1 ) );
        qreal copyrightTextWidth = copyrightTextline.naturalTextWidth();
        copyrightTextline.setPosition( QPointF( (320 - copyrightTextWidth) / 2.0 , 340 ) );
        copyrightTextLayout.endLayout();
        painter.setPen( QColor( 112, 16, 0, 255 ) ); // #701000
        copyrightTextLayout.draw( &painter, QPointF( 0, 0 ) );
    }
    QPixmap pixmap = QPixmap::fromImage(splashImage);
    QSplashScreen splash(pixmap);
    if (show_splash) {
        splash.show();
    }
    app->processEvents();

    QString splash_message;
    if (show_splash) {
        splash_message.append("\n\nMudlet comes with\n"
                              "ABSOLUTELY NO WARRANTY!\n"
                              "This is free software, and you are\n"
                              "welcome to redistribute it under\n"
                              "certain conditions; select the\n"
                              "'About' item for details.\n\n");
        splash_message.append("Locating profiles... ");
        splash.showMessage(splash_message, Qt::AlignHCenter|Qt::AlignTop);
        app->processEvents();
    }

    QString directory = QDir::homePath()+"/.config/mudlet";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );
    }

    if (show_splash) {
        splash_message.append("Done.\n\nLoading font files... ");
        splash.showMessage(splash_message, Qt::AlignHCenter|Qt::AlignTop);
        app->processEvents();
    }

    if (!QFile::exists(directory+"/COPYRIGHT.TXT")) {
        QFile file_f1(":/fonts/ttf-bitstream-vera-1.10/COPYRIGHT.TXT");
        file_f1.copy( directory+"/COPYRIGHT.TXT" );
    }

    if (!QFile::exists(directory+"/RELEASENOTES.TXT")) {
        QFile file_f2(":/fonts/ttf-bitstream-vera-1.10/RELEASENOTES.TXT");
        file_f2.copy( directory+"/RELEASENOTES.TXT" );
    }

    if (!QFile::exists(directory+"/VeraMoIt.ttf")) {
        QFile file_f3(":/fonts/ttf-bitstream-vera-1.10/VeraMoIt.ttf");
        file_f3.copy( directory+"/VeraMoIt.ttf" );
    }

    if (!QFile::exists(directory+"/local.conf")) {
        QFile file_f4(":/fonts/ttf-bitstream-vera-1.10/local.conf");
        file_f4.copy( directory+"/local.conf" );
    }

    if (!QFile::exists(directory+"/VeraMoBd.ttf")) {
        QFile file_f5(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf");
        file_f5.copy( directory+"/VeraMoBd.ttf" );
    }

    if (!QFile::exists(directory+"/VeraMoBd.ttf")) {
        QFile file_f6(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf");
        file_f6.copy( directory+"/VeraMoBd.ttf" );
    }

    if (!QFile::exists(directory+"/README.TXT")) {
        QFile file_f7(":/fonts/ttf-bitstream-vera-1.10/README.TXT");
        file_f7.copy( directory+"/README.TXT" );
    }

    if (!QFile::exists(directory+"/VeraMoBI.ttf")) {
        QFile file_f8(":/fonts/ttf-bitstream-vera-1.10/VeraMoBI.ttf");
        file_f8.copy( directory+"/VeraMoBI.ttf" );
    }

    if (!QFile::exists(directory+"/VeraMono.ttf")) {
        QFile file_f9(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf");
        file_f9.copy( directory+"/VeraMono.ttf" );
    }

    if (show_splash) {
        splash_message.append("Done.\n\n"
                              "All data has been loaded successfully.\n\n"
                              "Starting... Have fun!\n\n");
        splash.showMessage(splash_message, Qt::AlignHCenter|Qt::AlignTop);
        app->processEvents();
    }

    mudlet::debugMode = false;
    FontManager fm;
    fm.addFonts();
    QString home = QDir::homePath()+"/.config/mudlet";
    QString homeLink = QDir::homePath()+"/mudlet-data";
    QFile::link(home, homeLink);
    mudlet::start();
    mudlet::self()->show();
    if (show_splash) {
        splash.finish(mudlet::self());
    }
    app->restoreOverrideCursor();
    // NOTE: Must restore cursor - BEWARE DEBUGGERS if you terminate application
    // without doing/reaching this restore - it can be quite hard to accurately
    // click something in a parent process to the application when you are stuck
    // with some OS's choice of wait cursor - you might wish to temparily disable
    // the earlier setOverrideCursor() line and this one.
    return app->exec();
}
