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

extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;

int main(int argc, char *argv[])
{
    spDebugConsole = 0;

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



