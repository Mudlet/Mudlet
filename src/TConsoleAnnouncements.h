#ifndef MUDLET_TCONSOLEANNOUNCEMENTS_H
#define MUDLET_TCONSOLEANNOUNCEMENTS_H

/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

class Host;
class QObject;
class TBuffer;

namespace consoleAnnouncements {
// Whether a view's new text is read out: a screen reader is on, the console is the main one, the profile asks
// for it and is the active one
bool wanted(const Host& host, bool isMainConsole);
// The lines from previousLastLine up to lastLine, read out joined; fewer lines than before are a removal on
// the accessible text of view, the widget the event names, rather than anything new to read
void announceNewLines(QObject* view, const TBuffer& buffer, int previousLastLine, int lastLine);
} // namespace consoleAnnouncements

#endif // MUDLET_TCONSOLEANNOUNCEMENTS_H
