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
#include "UntrustedText.h"

#include <QUrl>

// <A href=URL [hint=text] [expire=name]>
TMxpTagHandlerResult TMxpLinkTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)

    // Extract expire name if present
    QString expireName;
    if (tag->hasAttribute(qsl("expire"))) {
        expireName = tag->getAttributeValue(qsl("expire"));
    }

    mCurrentTagContent.clear();
    mHref = getHref(tag);
    if (mHref.isEmpty()) {
        mIsHrefInContent = false;
        return MXP_TAG_NOT_HANDLED;
    }
    mIsHrefInContent = mHref.contains(TAG_CONTENT_PLACEHOLDER, Qt::CaseInsensitive);
    if (!mIsHrefInContent && !opensSafely(mHref)) {
        // shown as plain text rather than as a link that opens nothing
        return MXP_TAG_HANDLED;
    }

    // Server-supplied, and lands in the same tooltip as an OSC 8 hint. An
    // explicit hint is prose written to be read; falling back to the href makes
    // this a link target the user is being asked to trust.
    const QString hint = tag->hasAttribute(qsl("hint")) ? UntrustedText::forAuthoredText(tag->getAttributeValue(qsl("hint"))) : UntrustedText::forTarget(mHref);
    const QString action = actionFor(mHref);

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
        if (opensSafely(href)) {
            links->first() = actionFor(href);
        } else {
            // The link is already drawn, so leave it with nothing to run
            links->clear();
            if (hints != nullptr) {
                hints->clear();
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
    return qsl("openUrl(%1)").arg(LuaLiteral::quote(href));
}

// openUrl() hands the address to the desktop, which opens a file: or bare path
// as a local file and passes any other scheme to whatever application claimed
// it, so a game only gets to point at a remote page or a mail address. The same
// schemes as an OSC 8 link, plus mailto.
bool TMxpLinkTagHandler::opensSafely(const QString& href)
{
    const QString scheme = QUrl(href).scheme().toLower();
    return scheme == qsl("http") || scheme == qsl("https") || scheme == qsl("ftp") || scheme == qsl("mailto");
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
void TMxpLinkTagHandler::handleContent(char ch)
{
    if (mIsHrefInContent) {
        mCurrentTagContent.append(ch);
    }
}
