#ifndef MUDLET_TCOMMANDHISTORY_H
#define MUDLET_TCOMMANDHISTORY_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QString>
#include <QStringList>

// A command line's history, newest first behind the entry for the line being typed, and the profile file that
// keeps it between sessions, one entry a line; shared by the GUI's command lines and those of a profile with
// no main window, so both read and write the same files
class TCommandHistory
{
public:
    TCommandHistory() = delete;

    // As a command line takes a command once it is sent: any copy further back goes, it takes the entry for the
    // line being typed, and a new empty one goes in front of it
    static void add(QStringList& history, const QString& command);
    // Appended to history; false with the reason for a file that is there but could not be read, true when
    // there is no file
    static bool read(const QString& pathFileName, QStringList& history, QString& error);
    // The entry being typed and the newest saveSize after it
    static bool write(const QString& pathFileName, const QStringList& history, int saveSize, QString& error);
};

#endif // MUDLET_TCOMMANDHISTORY_H
