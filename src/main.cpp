/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                *
 *   KoehnHeiko@googlemail.com                                             *
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

#include <QtCore>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QSplashScreen>
#include <QStringBuilder>
#include <QTextLayout>
#include "TConsole.h"
#include "FontManager.h"
#include "mudlet.h"

// N/U: #define MUDLET_HOME "/usr/local/share/mudlet/"

using namespace std;

TConsole *  spDebugConsole = 0;

QStringList gSysErrors;

extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;

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
    else
        return new QApplication(argc, argv); // Normal course of events - (GUI), so: game on!
}

int main(int argc, char *argv[])
{
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
            std::cout << "Copyright (C) 2008-" << std::string(__DATE__).substr(7, 4) << " Heiko Koehn." << std::endl;
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
            std::cout << "Report bugs to: <http://launchpad.mudlet.org/>" << std::endl;
            std::cout << "pkg home page: <http://mudlet.sourceforge.net/>, also see <http://www.mudlet.org/>"<< std::endl;
        }
        return 0;
    }

// Turn the cursor into the waiting one during startup, so something shows
// activity even if the quiet, no splashscreen startup has been used
    app->setOverrideCursor(QCursor(Qt::WaitCursor));
    app->setOrganizationName("Mudlet");
    app->setApplicationName("Mudlet");
    app->setApplicationVersion(APP_VERSION);
    int splashScreenTextAnimationInterval = 100; // mSeconds

    QImage splashImage(":/Mudlet_splashscreen_main.png");
    if( startupAction & 4 )
    { // Start quietly - no splash screen - "quiet" action
        splashScreenTextAnimationInterval = 0;
    }
    else
    {
      // We now have to generate some suitable text to paint onto splash screen
      // as we have removed the static text of Heiko's copyright (because it has
      // a date that changes each year) and the version number (which changes as
      // well!)  The upside of this means no messing around with GIMP every
      // year/version. 8-)
        QPainter painter( &splashImage );
        QFont font( "DejaVu Serif", 16, QFont::Bold|QFont::Serif|QFont::PreferMatch|QFont::PreferAntialias );
        QString sourceVersionText = QString( "Version: " APP_VERSION APP_BUILD );
        QString sourceCopyrightText = QChar( 169 ) % QString( " Heiko K" ) % QChar( 246 ) % QString( "hn 2008-" ) % QString(__DATE__).mid(7);
        QTextLayout versionTextLayout( sourceVersionText, font, painter.device() );
        QTextLayout copyrightTextLayout( sourceCopyrightText, font, painter.device() );

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
        qreal versionTextWidth = versionTextline.naturalTextWidth();
        // This is the ACTUAL width of the created text
        versionTextline.setPosition( QPointF( (320 - versionTextWidth) / 2.0 , 270 ) );
        // And now we can place it centred horizontally
        versionTextLayout.endLayout();

        copyrightTextLayout.beginLayout();
        // Repeat for other text
        QTextLine copyrightTextline = copyrightTextLayout.createLine();
        copyrightTextline.setLineWidth( 280 );
        copyrightTextline.setPosition( QPointF( 1, 1 ) );
        qreal copyrightTextWidth = copyrightTextline.naturalTextWidth();
        copyrightTextline.setPosition( QPointF( (320 - copyrightTextWidth) / 2.0 , 340 ) );
        copyrightTextLayout.endLayout();

        painter.setPen( QColor( 176, 64, 0, 255 ) ); // #b04000
        versionTextLayout.draw( &painter, QPointF( 0, 0 ) );

        painter.setPen( QColor( 112, 16, 0, 255 ) ); // #701000
        copyrightTextLayout.draw( &painter, QPointF( 0, 0 ) );
    }
    QPixmap pixmap = QPixmap::fromImage(splashImage);
    QSplashScreen splash(pixmap);
    if( splashScreenTextAnimationInterval )
        splash.show();

    // Do some animation on the other (plain) text to show - designed to put up
    // the text in a useful way and then clear it to display Thorsten's work in
    // all its glory.
    const int startupMessageSize = splash.height() / splash.fontMetrics().lineSpacing();
    QStringList startupMessage;
    QTime t;
    if( splashScreenTextAnimationInterval )
    {
//    qDebug("main(): Splashscreen is %i high and the font has a linespacing of %i giving space for %i lines.",
//           splash.height(), splash.fontMetrics().lineSpacing(), startupMessageSize);
        // This should adjust to the number of lines to fill the splashscreen
        // graphic - hope there is not too variation in the font size between
        // difference systems...
        startupMessage.reserve(startupMessageSize);
        for( int i = 0; i < startupMessageSize; i++ )
            startupMessage << " ";

        // GUI actions splash-screen messages partly as suggested by GNU coding standards,
        // Following is suggested by end of GPL 3 document: https://www.gnu.org/licenses/gpl-3.0.html#howto
        // Odd structuring of message is to make it quick to add/modify each line from top downward
        // and then be able to scroll it (up) to clear.
        startupMessage[startupMessageSize-7] = QString("select the 'About' item for details.");
        startupMessage[startupMessageSize-8] = QString("redistribute it under certain conditions;");
        startupMessage[startupMessageSize-9] = QString("This is free software, and you are welcome to");
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();
        t.restart();
        int interval = 2 * splashScreenTextAnimationInterval;
        while( t.elapsed() < interval ) {}

        startupMessage[startupMessageSize-11] = QString("ABSOLUTELY NO WARRANTY!");
        startupMessage[startupMessageSize-12] = QString("Mudlet comes with");
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();
        t.restart();
        interval = 5 * splashScreenTextAnimationInterval;
        while( t.elapsed() < interval ) {}

        startupMessage[1] = QString("Locating profiles... ");
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();
        t.restart();
        interval = 5 * splashScreenTextAnimationInterval;
        while( t.elapsed() < interval ) {}
    }

    //qt_ntfs_permission_lookup++; // turn permission checking on on NTFS file systems

    QString directory = QDir::homePath()+"/.config/mudlet";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );
    }

    if( splashScreenTextAnimationInterval )
    {
        startupMessage[1].append(QString("Done."));
        startupMessage[2] = QString("Loading font files... ");
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();
        t.restart();
        int interval = 5 * splashScreenTextAnimationInterval;
        while( t.elapsed() < interval ) {}
    }

    QFile file_f1(":/fonts/ttf-bitstream-vera-1.10/COPYRIGHT.TXT");
    file_f1.copy( directory+"/COPYRIGHT.TXT" );

    QFile file_f2(":/fonts/ttf-bitstream-vera-1.10/RELEASENOTES.TXT");
    file_f2.copy( directory+"/RELEASENOTES.TXT" );

    QFile file_f3(":/fonts/ttf-bitstream-vera-1.10/VeraMoIt.ttf");
    file_f3.copy( directory+"/VeraMoIt.ttf" );

    QFile file_f4(":/fonts/ttf-bitstream-vera-1.10/local.conf");
    file_f4.copy( directory+"/local.conf" );

    QFile file_f5(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf");
    file_f5.copy( directory+"/VeraMoBd.ttf" );

    QFile file_f6(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf");
    file_f6.copy( directory+"/VeraMoBd.ttf" );

    QFile file_f7(":/fonts/ttf-bitstream-vera-1.10/README.TXT");
    file_f7.copy( directory+"/README.TXT" );

    QFile file_f8(":/fonts/ttf-bitstream-vera-1.10/VeraMoBI.ttf");
    file_f8.copy( directory+"/VeraMoBI.ttf" );

    QFile file_f9(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf");
    file_f9.copy( directory+"/VeraMono.ttf" );

    if( splashScreenTextAnimationInterval )
    {
        startupMessage[2].append(QString("Done."));
        startupMessage[3] = QString("All data has been loaded successfully.");
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();
        t.restart();
        int interval = 1 * splashScreenTextAnimationInterval;
        while( t.elapsed() < interval ) {}

        startupMessage[4] = QString("Starting...                             " % QChar::Nbsp );
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();
        t.restart();
        interval = 1 * splashScreenTextAnimationInterval;
        while( t.elapsed() < interval ) {}

        startupMessage[4] = QString("Starting...                     Have fun!");
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();
        t.restart();
        interval = 5 * splashScreenTextAnimationInterval;
        while( t.elapsed() < interval ) {}

        interval = 1 * splashScreenTextAnimationInterval;
        for( int i = startupMessageSize - 1; i > 0 ; i-- )
        {
            QString temp = " ";
            startupMessage[0].swap(temp);
            for( int j = 1; j <= i ; j++ )
            {
                startupMessage[j-1].swap(startupMessage[j]);
            }
            splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
            app->processEvents();
            t.restart();

            while( t.elapsed() < interval ) {}
        }
        startupMessage[0] = QString(" ");
        splash.showMessage(startupMessage.join("\n"), Qt::AlignHCenter|Qt::AlignBottom);
        app->processEvents();

        splash.finish( mudlet::self() );
    }

    mudlet::debugMode = false;
    HostManager::self();
    FontManager fm;
    fm.addFonts();
    QString home = QDir::homePath()+"/.config/mudlet";
    QString homeLink = QDir::homePath()+"/mudlet-data";
    QFile::link(home, homeLink);
    mudlet::self()->show();
    app->restoreOverrideCursor();
    // NOTE: Must restore cursor - BEWARE DEBUGGERS if you terminate application
    // without doing/reaching this restore - it can be quite hard to accurately
    // click something in a parent process to the application when you are stuck
    // with some OS's choice of wait cursor - you might wish to temparily disable
    // the earlier setOverrideCursor() line and this one.  8-( !
    app->exec();
}

