#ifndef MUDLET_GMCPLINKURI_H
#define MUDLET_GMCPLINKURI_H

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

#include <QString>

#include <optional>

struct GmcpLinkUri
{
    QString name;
    // The single JSON value the game sent, percent-decoded and trimmed, or empty when the message carries no data
    QString json;

    bool dataAllowsSelection() const;
    QString message() const;
};

// Parsed on arrival so only what was checked reaches the wire: a game or another player's relayed text wrote it.
// Takes the whole URI; a menu item's scheme may be in any letter case. From the first '?' on is the link's own parameters.
// Where it refuses one, whyNot, if given, says which check failed, for the game's developer.
std::optional<GmcpLinkUri> parseGmcpLinkUri(const QString& uri, QString* whyNot = nullptr);

// The message a selection link sends: its JSON object with "selected" set, replacing any already
// there. Takes a message from GmcpLinkUri::message() whose data allows a selection.
QString gmcpMessageWithSelected(const QString& message, bool selected);

#endif // MUDLET_GMCPLINKURI_H
