/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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
#include "TStringUtils.h"

#include <QUrl>


bool TStringUtils::isQuote(QChar ch)
{
    return isOneOf(ch, qsl("\'\""));
}

bool TStringUtils::isOneOf(QChar inputCharacter, const QString& characterSet)
{
    for (const auto& setCharacter : characterSet) {
        if (setCharacter == inputCharacter) {
            return true;
        }
    }

    return false;
}

std::optional<TStringUtils::TelnetUrl> TStringUtils::parseTelnetUrl(const QString& input)
{
    if (input.isEmpty()) {
        return std::nullopt;
    }

    const QUrl url(input, QUrl::TolerantMode);
    if (!url.isValid()) {
        return std::nullopt;
    }

    if (url.scheme().compare(qsl("telnet"), Qt::CaseInsensitive) != 0) {
        return std::nullopt;
    }

    if (!url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment()) {
        return std::nullopt;
    }

    const QString host = url.host();
    if (host.isEmpty()) {
        return std::nullopt;
    }

    const QString path = url.path();
    if (!path.isEmpty() && path != qsl("/")) {
        return std::nullopt;
    }

    int port = url.port();
    if (port == -1) {
        port = 23;
    }

    if (port < 1 || port > 65535) {
        return std::nullopt;
    }

    return TelnetUrl{host, static_cast<quint16>(port)};
}
