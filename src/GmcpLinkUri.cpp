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

#include "GmcpLinkUri.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

#include "utils.h"

namespace {

constexpr qsizetype MAX_MESSAGE_NAME_LENGTH = 128;

std::optional<QJsonValue> singleJsonValue(const QString& text)
{
    // QJsonDocument takes only an object or an array at the top
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray("[") + text.toUtf8() + QByteArray("]"), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray() || document.array().size() != 1) {
        return std::nullopt;
    }
    return document.array().first();
}

} // anonymous namespace

bool GmcpLinkUri::dataAllowsSelection() const
{
    if (json.isEmpty()) {
        return true;
    }
    const auto value = singleJsonValue(json);
    return value && value->isObject();
}

QString GmcpLinkUri::message() const
{
    return json.isEmpty() ? name : qsl("%1 %2").arg(name, json);
}

std::optional<GmcpLinkUri> parseGmcpLinkUri(const QString& uri)
{
    const qsizetype queryStart = uri.indexOf(QLatin1Char('?'));
    const QString encoded = uri.mid(qstrlen("gmcp:"), queryStart < 0 ? -1 : queryStart - qstrlen("gmcp:"));
    const QString decoded = QUrl::fromPercentEncoding(encoded.toUtf8());

    // Qt's JSON parser lets these through inside a string, and a NUL would cut the message short on its way through Lua
    for (const QChar character : decoded) {
        if (character.unicode() < 0x20 || character.unicode() == 0x7F) {
            return std::nullopt;
        }
    }

    const qsizetype nameEnd = decoded.indexOf(QLatin1Char(' '));
    GmcpLinkUri link;
    link.name = decoded.left(nameEnd);

    static const QRegularExpression validName(qsl(R"(\A[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+\z)"));
    if (link.name.size() > MAX_MESSAGE_NAME_LENGTH || !validName.match(link.name).hasMatch()) {
        return std::nullopt;
    }

    // Core is the connection's own negotiation and Char.Login carries passwords, and GMCP names are not case sensitive
    const QString lowerName = link.name.toLower();
    if (lowerName.startsWith(qsl("core.")) || lowerName == qsl("char.login") || lowerName.startsWith(qsl("char.login."))) {
        return std::nullopt;
    }

    if (nameEnd >= 0) {
        link.json = decoded.mid(nameEnd + 1).trimmed();
        if (!singleJsonValue(link.json)) {
            return std::nullopt;
        }
    }
    return link;
}

QString gmcpMessageWithSelected(const QString& message, const bool selected)
{
    const qsizetype nameEnd = message.indexOf(QLatin1Char(' '));
    const QString name = message.left(nameEnd);
    QJsonObject data;
    if (nameEnd >= 0) {
        data = singleJsonValue(message.mid(nameEnd + 1)).value_or(QJsonValue()).toObject();
    }
    data[qsl("selected")] = selected;
    return qsl("%1 %2").arg(name, QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
}
