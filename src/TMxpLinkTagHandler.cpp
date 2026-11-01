/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "TMxpLinkTagHandler.h"
#include "LuaLiteral.h"
#include "TMxpClient.h"
#include "TelnetUri.h"
#include "UntrustedText.h"

#include <QCoreApplication>
#include <QDebug>
#include <QUrl>
#include <QUrlQuery>

namespace {
// Shown in place of the address, so the tooltip and a screen reader say the click will do nothing
QString refusedHint(const QString& href)
{
    //: Tooltip on a link from the game whose address Mudlet will not open, such as a file or a program's own scheme; %1 is that address
    return QCoreApplication::translate("TMxpLinkTagHandler", "Mudlet does not open this kind of address: %1").arg(UntrustedText::forTarget(href));
}

// Mudlet connects to these itself, after asking, rather than handing them to the system
bool connectsToGame(const QString& href)
{
    const QString scheme = QUrl(href).scheme().toLower();
    return scheme == qsl("telnet") || scheme == qsl("telnets");
}

QString connectHint(const QString& href)
{
    //: Start of the tooltip on a link from the game to a game's address, which may be the game being played, followed by a colon and that address
    return qsl("%1: %2").arg(QCoreApplication::translate("TMxpLinkTagHandler", "Connect to"), UntrustedText::forTarget(href));
}
} // namespace

// <A href=URL [hint=text] [expire=name]>
TMxpTagHandlerResult TMxpLinkTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)

    // Extract expire name if present
    QString expireName;
    if (tag->hasAttribute(qsl("expire"))) {
        expireName = tag->getAttributeValue(qsl("expire"));
    }

    // An A that is not handled leaves the one it is nested in untouched
    const QString href = getHref(tag);
    if (href.isEmpty()) {
        return MXP_TAG_NOT_HANDLED;
    }
    mCurrentTagContent.clear();
    mHref = href;
    mIsHrefInContent = mHref.contains(TAG_CONTENT_PLACEHOLDER, Qt::CaseInsensitive);

    // Server-supplied, and lands in the same tooltip as an OSC 8 hint. An
    // explicit hint is prose written to be read; falling back to the href makes
    // this a link target the user is being asked to trust.
    mHintIsHref = !tag->hasAttribute(qsl("hint"));
    // An address taken from the wrapped text is only known, and checked, at the end tag
    const QString address = mIsHrefInContent ? QString() : openableAddress(mHref);
    QString hint = mHintIsHref ? UntrustedText::forTarget(address.isEmpty() ? mHref : address) : UntrustedText::forAuthoredText(tag->getAttributeValue(qsl("hint")));
    if (!mIsHrefInContent && address.isEmpty()) {
        hint = refusedHint(mHref);
    } else if (!mIsHrefInContent && mHintIsHref && connectsToGame(address)) {
        hint = connectHint(address);
    }
    const QString action = actionFor(address);

    // Use the version of setLink that supports expire names
    if (!expireName.isEmpty()) {
        mLinkId = client.setLink(QStringList(action), QStringList(hint), expireName);
    } else {
        mLinkId = client.setLink(QStringList(action), QStringList(hint));
    }

    client.setLinkMode(true);
    return MXP_TAG_HANDLED;
}

TMxpTagHandlerResult TMxpLinkTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    Q_UNUSED(ctx)
    Q_UNUSED(tag)
    QStringList *links, *hints;
    if (!client.getLink(mLinkId, &links, &hints)) {
        return MXP_TAG_NOT_HANDLED;
    }

    // The wrapped text goes into the address before it is quoted, so text
    // written to close the Lua string stays inside it
    if (links != nullptr && mIsHrefInContent && !links->isEmpty()) {
        const QString href = QString(mHref).replace(TAG_CONTENT_PLACEHOLDER, mCurrentTagContent, Qt::CaseInsensitive);
        const QString address = openableAddress(href);
        links->first() = actionFor(address);
        if (hints != nullptr && !hints->isEmpty()) {
            if (address.isEmpty()) {
                hints->first() = refusedHint(href);
            } else if (mHintIsHref) {
                hints->first() = connectsToGame(address) ? connectHint(address) : UntrustedText::forTarget(address);
            }
        }
    }

    mIsHrefInContent = false;
    mCurrentTagContent.clear();
    client.setLinkMode(false);
    return MXP_TAG_HANDLED;
}

QString TMxpLinkTagHandler::openableAddress(const QString& href)
{
    if (connectsToGame(href)) {
        return parseTelnetUri(href) ? href : QString();
    }
    // Game servers choose MXP link addresses, so only these schemes may reach the system's URL handler
    const QUrl url(href);
    const QString scheme = url.scheme().toLower();
    if (scheme != qsl("http") && scheme != qsl("https") && scheme != qsl("ftp") && scheme != qsl("mailto")) {
        qWarning().noquote().nospace() << "TMxpLinkTagHandler::openableAddress(...) - Ignored untrusted or unsupported URI scheme: \"" << scheme << "\" in link to: \"" << href.left(80) << "\"";
        return QString();
    }
    if (scheme != qsl("mailto")) {
        return href;
    }

    // Mail clients and the desktop portal attach local files named by "attach" or "attachment"
    QUrlQuery kept;
    for (const auto& [name, value] : QUrlQuery(url).queryItems(QUrl::FullyEncoded)) {
        const QString field = QUrl::fromPercentEncoding(name.toUtf8()).toLower();
        if (field == qsl("subject") || field == qsl("body")) {
            kept.addQueryItem(name, value);
        }
    }
    const QString recipients = qsl("mailto:%1").arg(url.path(QUrl::FullyEncoded));
    return kept.isEmpty() ? recipients : qsl("%1?%2").arg(recipients, kept.query(QUrl::FullyEncoded));
}

QString TMxpLinkTagHandler::actionFor(const QString& address)
{
    if (address.isEmpty()) {
        return QString();
    }
    return qsl("%1(%2)").arg(connectsToGame(address) ? qsl("connectFromGameLink") : qsl("openUrl"), LuaLiteral::quote(address));
}

QString TMxpLinkTagHandler::getHref(const MxpStartTag* tag)
{
    if (tag->getAttributesCount() == 0) {
        // <A>http://someurl.com/<A>
        return TAG_CONTENT_PLACEHOLDER;
    }
    if (tag->hasAttribute("href")) {
        return tag->getAttributeValue("href");
    }
    if (!tag->getAttribute(0).hasValue()) {
        return tag->getAttribute(0).getName();
    }
    return "";
}
void TMxpLinkTagHandler::handleContentBytes(QByteArrayView bytes)
{
    if (mIsHrefInContent) {
        mCurrentTagContent.append(QLatin1StringView(bytes));
    }
}
