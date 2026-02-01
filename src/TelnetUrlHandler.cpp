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

#include "TelnetUrlHandler.h"

bool TelnetUrlHandler::isTelnetUrl(const QString& urlString)
{
    return urlString.startsWith(QLatin1String("telnet://"), Qt::CaseInsensitive);
}

TelnetUrlHandler::TelnetUrl TelnetUrlHandler::parse(const QString& urlString)
{
    TelnetUrl result;

    if (!isTelnetUrl(urlString)) {
        return result;
    }

    QUrl url(urlString);
    if (!url.isValid()) {
        return result;
    }

    result.host = url.host();
    if (result.host.isEmpty()) {
        return result;
    }

    // Use port from URL if specified, otherwise default to 23 (standard telnet port)
    result.port = url.port(23);

    // Validate port range
    if (result.port < 1 || result.port > 65535) {
        return result;
    }

    result.isValid = true;
    return result;
}
