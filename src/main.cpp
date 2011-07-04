/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
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



#include <QFontDatabase>
#include <QApplication>
#include "mudlet.h"
#include "TConsole.h"
#include "FontManager.h"
#include <QSplashScreen>
#include <QFontDatabase>
#include <QtCore>
#include <qdir.h>
#include <QFile>
#include <QtCore>

#define MUDLET_HOME "/usr/local/share/mudlet/"

using namespace std;

TConsole *  spDebugConsole = 0;

//#ifdef QT_DEBUG
//    QFile debugStreamFile("C:\\mudletDebugStream.txt");;
//    QTextStream debugStream(&debugStreamFile);
//#endif

void debugOutput(QtMsgType type, const char *msg)
{
    //debugStream << msg << endl;
    ;
//    switch (type)
//    {
//    case QtDebugMsg:
//        cout << msg << endl;

//        /*if( mudlet::mpDebugConsole )
//        {
//            ;//mudlet::mpDebugConsole->print( msg );
//        }
//        else
//        {
//            fprintf(stderr, "Debug: %s\n", msg);
//        }*/
//        break;
//    case QtWarningMsg:
//        fprintf(stderr, "Warning: %s\n", msg);
//        break;
//    case QtCriticalMsg:
//        fprintf(stderr, "Critical: %s\n", msg);
//        break;
//    case QtFatalMsg:
//        fprintf(stderr, "Fatal: %s\n", msg);
//        abort();
//        break;
//    }
}

QStringList gSysErrors;

extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;

int main(int argc, char *argv[])
{
//    #ifdef QT_DEBUG
//        debugStreamFile.open(QFile::WriteOnly | QFile::Truncate);
//    #endif

    //FIXME qInstallMsgHandler( debugOutput );
    spDebugConsole = 0;

    QGL::setPreferredPaintEngine(QPaintEngine::Raster);//faster map drawing on ubuntu
    Q_INIT_RESOURCE(mudlet_alpha);
    QApplication app(argc, argv);
    app.setApplicationName("Mudlet");
    QPixmap pixmap(":/Mudlet_splashscreen_main.png");
    QSplashScreen splash(pixmap);
    splash.show();

    splash.showMessage("Loading profiles ...");

    app.processEvents();
    //qt_ntfs_permission_lookup++; // turn permission checking on on NTFS file systems

    QString directory = QDir::homePath()+"/.config/mudlet";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );
    }

    // QFile file_doc(":/mudlet_documentation.html");
    // QFile file_doc_old;
    // file_doc_old.setFileName( directory+"/mudlet_documentation.html" );
    // if( file_doc_old.exists() )
    // {
        //NOTE: B. von Roeder found out that the removal of old versions may *sometimes* fail on windows 7 due permission issues
        // if( ! file_doc_old.setPermissions( QFile::WriteOwner | QFile::ReadOwner | QFile::ReadUser | QFile::WriteUser | QFile::ReadOther | QFile::WriteOther ) )
        // {
            // cout << "[ERROR] could not set file permissions of the old version of the manual" << endl;
            // gSysErrors << "[ERROR] could not set file permissions of the old version of the manual";
        // }
        // string old_man_path = directory.toLatin1().data();
        // old_man_path += "/mudlet_documentation.html";
        // bool ok=file_doc_old.remove();
        // if( ok )
        // {
            // cout << "[INFO] deleted old version of the manual: " << old_man_path << endl;
        // }
        // else
        // {
            // cout << "[ERROR] could not remove old version of the manual: " << old_man_path << endl;
            // QString _m = "[ERROR] could not remove old version of the manual: ";
            // _m.append( old_man_path.c_str() );
            // gSysErrors << _m;
        // }
    // }
    // else
    // {
        // gSysErrors << "[INFO] no old version of the manual found";
    // }
    // if( file_doc.copy( directory+"/mudlet_documentation.html" ) )
    // {
        // cout << "[OK] successfully copied new version of the manual" << endl;
        // QString _m = "[INFO] local manual: ";
        // _m.append( directory );
        // gSysErrors << _m;
    // }
    // else
    // {
        // cout << "[ERROR] copy of new version of the manual failed" << endl;
        // gSysErrors << "[ERROR] copy of new version of the manual failed";
    // }
    // QFile file_lua(":/LuaGlobal.lua");

    // QFile file_lua_old( directory+"/LuaGlobal.lua" );
    // if( ! file_lua_old.setPermissions( QFile::WriteOwner | QFile::ReadOwner | QFile::ReadUser | QFile::WriteUser | QFile::ReadOther | QFile::WriteOther ) )
    // {
        // cout << "[ERROR] failed to set file permissions for the old version of LuaGlobal.lua" << endl;
        // gSysErrors << "[ERROR] failed to set file permissions for the old version of LuaGlobal.lua";
    // }
    // else
    // {
        // cout << "[OK] successfully set file permissions for the old version of LuaGlobal.lua" << endl;
    // }
    // if( file_lua_old.remove() )
    // {
        // cout << "[OK] old LuaGlobal.lua removed successfully" << endl;
        // gSysErrors << "[INFO] old LuaGlobal.lua removed successfully";
    // }
    // else
    // {
        // cout << "[ERROR] failed to remove the old version of LuaGlobal.lua" << endl;
        // gSysErrors << "[ERROR] failed to remove the old version of LuaGlobal.lua";
    // }
    // if( file_lua.copy( directory+"/LuaGlobal.lua" ) )
    // {
        // cout << "[OK] new version of LuaGlobal.lua copied successfully" << endl;
        // gSysErrors << "[INFO] LuaGlobal.lua restored successfully";
        // QFile file_lua_new(directory+"/LuaGlobal.lua");
        // if( ! file_lua_new.setPermissions( QFile::WriteOwner | QFile::ReadOwner | QFile::ReadUser | QFile::WriteUser | QFile::ReadOther | QFile::WriteOther ) )
        // {
            // cout << "[ERROR] failed to set file permissions for the new version of LuaGlobal.lua" << endl;
            // gSysErrors << "[ERROR] failed to set file permissions for the new version of LuaGlobal.lua";
        // }
        // else
        // {
            // cout << "[OK] successfully set file permissions for the new version of LuaGlobal.lua" << endl;
        // }
    // }

    // QFile file_db(":/db.lua");

    // QFile file_db_old( directory+"/db.lua" );
    // if( ! file_db_old.setPermissions( QFile::WriteOwner | QFile::ReadOwner | QFile::ReadUser | QFile::WriteUser | QFile::ReadOther | QFile::WriteOther ) )
    // {
        // cout << "[ERROR] failed to set file permissions for the old version of db.lua" << endl;
        // gSysErrors << "[ERROR] failed to set file permissions for the old version of db.lua";
    // }
    // else
    // {
        // cout << "[OK] successfully set file permissions for the old version of db.lua" << endl;
    // }
    // if( file_db_old.remove() )
    // {
        // cout << "[OK] old db.lua removed successfully" << endl;
        // gSysErrors << "[INFO] old db.lua removed successfully";
    // }
    // else
    // {
        // cout << "[ERROR] failed to remove the old version of db.lua" << endl;
        // gSysErrors << "[ERROR] failed to remove the old version of db.lua";
    // }
    // if( file_db.copy( directory+"/db.lua" ) )
    // {
        // cout << "[OK] new version of db.lua copied successfully" << endl;
        // gSysErrors << "[INFO] db.lua restored successfully";
        // QFile file_db_new(directory+"/db.lua");
        // if( ! file_db_new.setPermissions( QFile::WriteOwner | QFile::ReadOwner | QFile::ReadUser | QFile::WriteUser | QFile::ReadOther | QFile::WriteOther ) )
        // {
            // cout << "[ERROR] failed to set file permissions for the new version of db.lua" << endl;
            // gSysErrors << "[ERROR] failed to set file permissions for the new version of db.lua";
        // }
        // else
        // {
            // cout << "[OK] successfully set file permissions for the new version of db.lua" << endl;
        // }
    // }


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
    splash.showMessage("All data has been loaded successfully.\n\nHave fun!");
    QTime t;
    t.start();
    while( t.elapsed() < 1500 );
    splash.finish( mudlet::self() );
    mudlet::debugMode = false;
    HostManager::self();
    FontManager fm;
    fm.addFonts();
    QString home = QDir::homePath()+"/.config/mudlet";
    QString homeLink = QDir::homePath()+"/mudlet-data";
    QFile::link(home, homeLink);
    mudlet::self()->show();
    app.exec();
}



