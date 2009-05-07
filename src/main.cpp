/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
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

//#undef QT_NO_DEBUG_OUTPUT

#include <QFontDatabase>
#include <QApplication>
#include "mudlet.h"
#include "TConsole.h"
#include "FontManager.h"
#include <QSplashScreen>
#include <QFontDatabase>

#define MUDLET_HOME "/usr/local/share/mudlet/"

using namespace std;

TConsole *  spDebugConsole = 0;

void debugOutput(QtMsgType type, const char *msg)
{
    switch (type) 
    {
    case QtDebugMsg:
        cout << msg << endl;
        if( mudlet::mpDebugConsole )
        {
            //mudlet::mpDebugConsole->print( msg );
        }
        else
        {
            fprintf(stderr, "Debug: %s\n", msg);
        }
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        abort();
    }
}


int main(int argc, char *argv[])
{
    spDebugConsole = 0;
    qInstallMsgHandler( debugOutput );
    
    Q_INIT_RESOURCE(mudlet_alpha);
    QApplication app(argc, argv);
    QPixmap pixmap(":/Mudlet_splashscreen_main");
    QSplashScreen splash(pixmap);
    splash.show();
    
    splash.showMessage("Loading profiles ...");
    
    app.processEvents();
    
    QString directory = QDir::homePath()+"/.config/mudlet";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory ); 
    }
    
    QFile file_doc(":/mudlet_documentation.html");
    QFile file_doc_old;
    file_doc_old.setFileName( directory+"/mudlet_documentation.html" );
    qDebug()<<"deleting old manual<"<<directory+"/mudlet_documentation.html"<<">";
    bool ok=file_doc_old.remove();
    if( ok ) qDebug()<<"OK deleted file";
    else qDebug()<<"ERROR: could not remove file";
    file_doc.copy( directory+"/mudlet_documentation.html" );

    QFile file_lua(":/LuaGlobal.lua");
    QFile file_lua_old( directory+"/LuaGlobal.lua" );
    file_lua_old.remove();
    file_lua.copy( directory+"/LuaGlobal.lua" );

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

    /*QFile file_f(":/fonts/ttf-bitstream-vera-1.10/");
    file_f.copy( directory+"/" );

    QFile file_f(":/fonts/ttf-bitstream-vera-1.10/");
    file_f.copy( directory+"/" );

    QFile file_f(":/fonts/ttf-bitstream-vera-1.10/");
    file_f.copy( directory+"/" );  */


    mudlet::self();
    
    mudlet::debugMode = false;
    HostManager::self();
    FontManager fm;
    fm.addFonts();
    mudlet::self()->show();
    
    splash.showMessage("All data has been loaded successfully.\n\nHave fun!");
    QTime t;
    t.start();
    while( t.elapsed() < 1500 ){}
    splash.finish( mudlet::self() );
    app.exec();
    qDebug()<<"*** Have a nice day! ***";
}



