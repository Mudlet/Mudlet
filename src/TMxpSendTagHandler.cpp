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
#include "TMxpSendTagHandler.h"
#include "TMxpClient.h"
#include "TStringUtils.h"

TMxpSendTagHandler::TMxpSendTagHandler() : TMxpSingleTagHandler("SEND"), mLinkId(0), mIsHrefInContent(false) {}
TMxpTagHandlerResult TMxpSendTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    //    if (tag->hasAttr("EXPIRE") && tag->getAttr(0).isNamed("EXPIRE"))
    //        return MXP_TAG_NOT_HANDLED;

    QString href = extractHref(tag);
    QString hint = extractHint(tag);

    if (href.contains(TAG_CONTENT_PLACEHOLDER, Qt::CaseInsensitive) || hint.contains(TAG_CONTENT_PLACEHOLDER, Qt::CaseInsensitive)) {
        mIsHrefInContent = true;
    }

    QStringList hrefs = href.split('|');
    QStringList hints = hint.isEmpty() ? hrefs : hint.split('|');

    while (hints.size() > hrefs.size()) {
        hints.removeFirst();
    }

    // handle print to prompt feature PROMPT
    // <SEND "tell Zugg " PROMPT>Zugg</SEND>
    QString command = tag->hasAttribute(ATTR_PROMPT) ? QStringLiteral("printCmdLine") : QStringLiteral("send");

    for (int i = 0; i < hrefs.size(); i++) {
        hrefs[i] = ctx.getEntityResolver().interpolate(hrefs[i]);
        hrefs[i] = QStringLiteral("%1([[%2]])").arg(command, hrefs[i]);

        hints[i] = ctx.getEntityResolver().interpolate(hints[i]);
    }

    mLinkId = client.setLink(hrefs, hints);

    client.setLinkMode(true);

    return MXP_TAG_HANDLED;
}
QString TMxpSendTagHandler::extractHref(MxpStartTag* tag)
{
    if (tag->getAttributesCount() == 0) {
        // <send>buy bread</send>
        return TAG_CONTENT_PLACEHOLDER;
    }

    if (tag->hasAttribute(ATTR_HREF)) {
        // <!ELEMENT Item '<send href="buy &text;">'>
        // <Item>bread</Item>
        // <!EL shop "<send href='shop identify &name;|shop buy &name;' hint='Right mouse click to act on items from this shop|Identify &desc;|Buy &desc;' expire=shop" ATT='name desc'>
        // <shop name="sword" desc="A shining sword">A shining sword of Lewshire</shop>
        return tag->getAttributeValue(ATTR_HREF);
    }

    if (!tag->getAttribute(0).isNamed(ATTR_PROMPT) && !tag->getAttribute(0).isNamed(ATTR_HINT) && !tag->getAttribute(0).isNamed(ATTR_EXPIRE)) {
        // has one attribute, but not called href
        // <SEND "tell Zugg " PROMPT>Zugg</SEND>
        // <send "drink &text;">fountain</send>
        return tag->getAttrName(0);
    }

    return TAG_CONTENT_PLACEHOLDER;
}

QString TMxpSendTagHandler::extractHint(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_HINT)) {
        return tag->getAttributeValue(ATTR_HINT);
    } else if (tag->getAttributesCount() > 1 && !tag->getAttribute(1).isNamed(ATTR_PROMPT) && !tag->getAttribute(1).isNamed(ATTR_EXPIRE)) {
        return tag->getAttrName(1);
    }

    return QString();
}

TMxpTagHandlerResult TMxpSendTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    if (mIsHrefInContent) {
        updateHrefInLinks(client);
    }

    resetCurrentTagContent(client);
    return MXP_TAG_HANDLED;
}

void TMxpSendTagHandler::resetCurrentTagContent(TMxpClient& client)
{
    mIsHrefInContent = false;
    mCurrentTagContent.clear();
    client.setLinkMode(false);
}

void TMxpSendTagHandler::updateHrefInLinks(TMxpClient& client) const
{
    QStringList *hrefs, *hints;
    if (client.getLink(mLinkId, &hrefs, &hints)) {
        if (hrefs != nullptr) {
            hrefs->replaceInStrings(TAG_CONTENT_PLACEHOLDER, mCurrentTagContent, Qt::CaseInsensitive);
        }

        if (hints != nullptr) {
            hints->replaceInStrings(TAG_CONTENT_PLACEHOLDER, mCurrentTagContent, Qt::CaseInsensitive);
        }
    }
}
void TMxpSendTagHandler::handleContent(char ch)
{
    if (mIsHrefInContent) {
        mCurrentTagContent.append(ch);
    }
}
