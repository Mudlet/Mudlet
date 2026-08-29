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

// A write-only text destination that core code can be handed instead of a view
// pointer. The MXP DEST redirect resolves the frame it is writing into to one
// of these, so the translation loop in TBuffer never holds a TConsole (#8681).
//
// Write-only is the whole point: nothing here reads state back, so a sink can
// be satisfied by a TConsole widget today and by a view-less TConsoleModel
// later without the callers changing. Keep it that way - a getter added here
// is a coupling that the model-backed implementation would have to invent an
// answer for.
class TPrintSink
{
public:
    // Appends text carrying its own per-character formatting. sourceLinkStore
    // holds the links that formatting's link indices refer to; the sink remaps
    // them into its own store, so the two must be passed together.
    //
    // The line semantics every implementation must reproduce (today they live
    // in TBuffer::appendFormatted, which the TConsole implementation defers
    // to): the first segment continues a part-written trailing line if the
    // sink holds one, an embedded QChar::LineFeed starts a new line, and every
    // call with non-empty text finishes on a committed line boundary - the
    // next call never continues this call's text. Empty text is a no-op, with
    // no boundary committed. formatting must be the same length as text; a
    // mismatch is tolerated (missing entries print unformatted, extras are
    // ignored) but warns, so callers must not lean on it.
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
