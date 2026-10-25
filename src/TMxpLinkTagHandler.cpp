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
    QString hint = mHintIsHref ? UntrustedText::forTarget(mHref) : UntrustedText::forAuthoredText(tag->getAttributeValue(qsl("hint")));
    // An address taken from the wrapped text is only known, and checked, at the end tag
    const QString action = mIsHrefInContent ? QString() : actionFor(mHref);
    if (!mIsHrefInContent && action.isEmpty()) {
        hint = refusedHint(mHref);
    } else if (!mIsHrefInContent && mHintIsHref && connectsToGame(mHref)) {
        hint = connectHint(mHref);
    }

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
        links->first() = actionFor(href);
        if (hints != nullptr && !hints->isEmpty()) {
            if (links->first().isEmpty()) {
                hints->first() = refusedHint(href);
            } else if (mHintIsHref) {
                hints->first() = connectsToGame(href) ? connectHint(href) : UntrustedText::forTarget(href);
            }
        }
    }

    mIsHrefInContent = false;
    mCurrentTagContent.clear();
    client.setLinkMode(false);
    return MXP_TAG_HANDLED;
}

QString TMxpLinkTagHandler::actionFor(const QString& href)
{
    if (connectsToGame(href)) {
        return parseTelnetUri(href) ? qsl("connectFromGameLink(%1)").arg(LuaLiteral::quote(href)) : QString();
    }
    // Game servers choose MXP link addresses, so only these schemes may reach the system's URL handler
    const QString scheme = QUrl(href).scheme().toLower();
    if (scheme != qsl("http") && scheme != qsl("https") && scheme != qsl("ftp") && scheme != qsl("mailto")) {
        qWarning().noquote().nospace() << "TMxpLinkTagHandler::actionFor(...) - Ignored untrusted or unsupported URI scheme: \"" << scheme << "\" in link to: \"" << href.left(80) << "\"";
        return QString();
    }
    return qsl("openUrl(%1)").arg(LuaLiteral::quote(href));
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
