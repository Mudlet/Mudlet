/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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

#include "dlgNotepad.h"
#include <QDir>
#include "TDebug.h"
#include "Host.h"


dlgNotepad::dlgNotepad( Host * pH )
: mpHost( pH )

{
    setupUi(this);
}


void dlgNotepad::save()
{
    QString directoryFile = QDir::homePath()+"/.config/mudlet/profiles/"+mpHost->getName();
    QString fileName = directoryFile + "/notes.txt";
    QDir dirFile;
    if( ! dirFile.exists( directoryFile ) )
    {
        dirFile.mkpath( directoryFile );
    }
    QFile file;
    file.setFileName( fileName );
    file.open( QIODevice::WriteOnly );
    QTextStream fileStream;
    fileStream.setDevice( &file );
    fileStream << notesEdit->toPlainText();
    file.close();
}

void dlgNotepad::restore()
{
    QString directoryFile = QDir::homePath()+"/.config/mudlet/profiles/"+mpHost->getName();
    QString fileName = directoryFile + "/notes.txt";
    QDir dirFile;
    QFile file;
    file.setFileName( fileName );
    file.open( QIODevice::ReadOnly );
    QTextStream fileStream;
    fileStream.setDevice( &file );
    QString txt = fileStream.readAll();
    notesEdit->setPlainText(txt);
}
