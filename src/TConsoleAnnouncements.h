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
class QWidget;
class TBuffer;

namespace consoleAnnouncements {
// Whether a view's new text is read out: a screen reader is on, the console is the main one, the profile asks
// for it and is the active one
bool wanted(const Host& host, bool isMainConsole);
// Reads out lines previousLastLine up to but not including lastLine, both lines of the buffer as it is now, so the
// caller takes lines trimmed off the top since off previousLastLine. When lastLine is the smaller, lines were removed,
// so it raises a removal on view's accessible text instead.
void announceNewLines(QWidget* view, const TBuffer& buffer, int previousLastLine, int lastLine);
} // namespace consoleAnnouncements

#endif // MUDLET_TCONSOLEANNOUNCEMENTS_H
