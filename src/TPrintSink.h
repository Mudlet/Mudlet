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
    virtual ~TPrintSink() = default;

    // Appends text carrying its own per-character formatting. sourceLinkStore
    // holds the links that formatting's link indices refer to; the sink remaps
    // them into its own store, so the two must be passed together.
    virtual void printFormatted(const QString& text, const std::vector<TChar>& formatting, const TLinkStore& sourceLinkStore) = 0;

    // <DEST ... EOF> empties the destination, <DEST ... EOL> discards the
    // part-written line it would otherwise have continued.
    virtual void clearSink() = 0;
    virtual void clearSinkLastLine() = 0;
};

#endif // MUDLET_TPRINTSINK_H
