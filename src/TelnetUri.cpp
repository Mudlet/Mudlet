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

#include "TelnetUri.h"

#include "utils.h"

#include <QDebug>
#include <QUrl>

std::optional<TelnetUriData> parseTelnetUri(const QString& uri)
{
    QUrl url(uri);

    if (!url.isValid()) {
        qWarning() << "parseTelnetUri() - Malformed URI";
        return std::nullopt;
    }

    const QString scheme = url.scheme();
    const bool isTelnets = scheme.compare(qsl("telnets"), Qt::CaseInsensitive) == 0;
    if (scheme.compare(qsl("telnet"), Qt::CaseInsensitive) != 0 && !isTelnets) {
        qWarning() << "parseTelnetUri() - Invalid URI scheme:" << scheme;
        return std::nullopt;
    }

    TelnetUriData data;
    data.useTls = isTelnets;
    data.host = url.host();
    if (data.host.isEmpty()) {
        qWarning() << "parseTelnetUri() - URI missing host";
        return std::nullopt;
    }

    data.port = url.port(data.useTls ? 992 : 23);
    if (data.port < 1 || data.port > 65535) {
        qWarning() << "parseTelnetUri() - Port out of range:" << data.port;
        return std::nullopt;
    }

    if (!url.userName().isEmpty()) {
        data.username = url.userName();
    }

    qDebug() << "parseTelnetUri() - Parsed URI:" << url.toDisplayString(QUrl::RemoveUserInfo);
    return data;
}
