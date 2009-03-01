/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

#undef QT_NO_DEBUG_OUTPUT

#include <QFontDatabase>
#include <QApplication>
#include "mudlet.h"
#include "TConsole.h"
#include "FontManager.h"
#include <QSplashScreen>
#include <QFontDatabase>

#define MUDLET_HOME "/usr/local/share/mudlet/"

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
    
    /*QFile file_doc( "/usr/local/share/mudlet/mudlet_documentation.html" );
    QFile file_doc_old( directory+"/mudlet_documentation.html" );
    if( file_doc.exists() )
    {
        if( file_doc_old.exists() )
        {
            file_doc_old.remove();
        }
        file_doc.copy( directory+"/mudlet_documentation.html" );
    }
    QFile file_lua( "/usr/local/share/mudlet/LuaGlobal.lua" );
    if( file_lua.exists() )
    {
        QFile file_lua_old( directory+"/LuaGlobal.lua" );
        if( file_lua_old.exists() )
        {
            file_lua_old.remove();
        }
        file_lua.copy( directory+"/LuaGlobal.lua" );
    } */
    
    /*QString fonts = directory+"/fonts/ttf-bitstream-vera-1.10";
    QDir fontDir( "/usr/local/share/mudlet/" );
    fontDir.mkpath( fonts );
    QStringList fontDirList = fontDir.entryList(QDir::Files);
    for( int i=0; i<fontDirList.size();i++ )
    {
        QFile::copy( "fonts/ttf-bitstream-vera-1.10/"+fontDirList[i], fonts+"/"+fontDirList[i] );    
    } */       
    
    mudlet::self();
    
    mudlet::debugMode = false;
    HostManager::self();
    //HostManager::self()->restore();  
    FontManager fm;
    fm.addFonts();
    mudlet::self()->show();
    
    splash.showMessage("All data has been loaded successfully.\n\nHave fun!");
    QTime t;
    t.start();
    while( t.elapsed() < 1500 ){}
    splash.finish( mudlet::self() );
    app.exec();
    //HostManager::self()->serialize();
}



