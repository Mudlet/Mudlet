/***************************************************************************
 *   Copyright (C) 2025 by Mudlet makers - mudlet.org                      *
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

#ifndef TELNETURLHANDLER_H
#define TELNETURLHANDLER_H

#include <QString>
#include <QUrl>

class TelnetUrlHandler
{
public:
    struct TelnetUrl {
        QString host;
        int port = 23;
        bool isValid = false;
    };

    // Parse a telnet:// URL according to RFC 4248
    // Format: telnet://[user@]host[:port][/]
    static TelnetUrl parse(const QString& urlString);

    // Check if the given string is a telnet:// URL
    static bool isTelnetUrl(const QString& urlString);
};

#endif // TELNETURLHANDLER_H
