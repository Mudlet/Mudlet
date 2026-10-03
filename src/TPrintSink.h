#ifndef MUDLET_TPRINTSINK_H
#define MUDLET_TPRINTSINK_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <vector>

class TChar;
class TLinkStore;

// A text destination core code can hold instead of a view, e.g. the MXP DEST target in TBuffer.
// Keep it write-only: a getter would be coupling a view-less TConsoleModel sink must invent answers for.
class TPrintSink
{
public:
    // formatting's link indices refer to sourceLinkStore; the sink remaps them into its own store.
    // Every implementation must match TBuffer::appendFormatted: the first segment continues a part-written
    // trailing line, QChar::LineFeed starts a new line, and non-empty text ends on a committed line
    // boundary. Empty text is a no-op. formatting must match text's length; a mismatch only warns
    // (missing entries print unformatted, extras are ignored), so callers must not rely on it.
    virtual void printFormatted(const QString& text, const std::vector<TChar>& formatting, const TLinkStore& sourceLinkStore) = 0;

    // Empties the sink of everything it holds (<DEST ... EOF>).
    virtual void discardAll() = 0;

    // Blanks the part-written trailing line that the next printFormatted
    // would otherwise continue (<DEST ... EOL>).
    virtual void discardLastLine() = 0;

protected:
    // Nothing owns a sink through this interface - consoles belong to their
    // widget parents - so deleting through it is a compile error.
    ~TPrintSink() = default;
};

#endif // MUDLET_TPRINTSINK_H
