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
#include "TStrUtils.h"

TMxpSendTagHandler::TMxpSendTagHandler() : TMxpSingleTagHandler("SEND"), mLinkId(0), mIsHrefInContent(false) {}
TMxpTagHandlerResult TMxpSendTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    //    if (tag->hasAttr("EXPIRE") && tag->getAttr(0).isNamed("EXPIRE"))
    //        return MXP_TAG_NOT_HANDLED;

    QString href = extractHref(tag);
    QString hint = extractHint(tag);

    if (href.contains("&text;", Qt::CaseInsensitive) || hint.contains("&text;", Qt::CaseInsensitive)) {
        mIsHrefInContent = true;
    }

    QStringList hrefs = href.split("|");
    QStringList hints = hint.isEmpty() ? hrefs : hint.split("|");

    while (hints.size() > hrefs.size()) {
        hints.removeFirst();
    }

    // handle print to prompt feature PROMPT
    // <SEND "tell Zugg " PROMPT>Zugg</SEND>
    bool sendToPrompt = tag->hasAttr("PROMPT");

    for (int i = 0; i < hrefs.size(); i++) {
        hrefs[i] = ctx.getEntityResolver().interpolate(hrefs[i]);
        hints[i] = ctx.getEntityResolver().interpolate(hints[i]);

        if (!sendToPrompt) {
            hrefs[i] = QStringLiteral("send([[%1]])").arg(hrefs[i]);
        } else {
            hrefs[i] = QStringLiteral("printCmdLine([[%1]])").arg(hrefs[i]);
        }
    }

    mLinkId = client.setLink(hrefs, hints);

    client.setLinkMode(true);

    return MXP_TAG_HANDLED;
}
QString TMxpSendTagHandler::extractHref(MxpStartTag* tag)
{
    if (tag->getAttrsCount() == 0) {
        // <send>buy bread</send>
        return "&text;";
    } else if (tag->hasAttr("href")) {
        // <!ELEMENT Item '<send href="buy &text;">'>
        // <Item>bread</Item>
        // <!EL shop "<send href='shop identify &name;|shop buy &name;' hint='Right mouse click to act on items from this shop|Identify &desc;|Buy &desc;' expire=shop" ATT='name desc'>
        // <shop name="sword" desc="A shining sword">A shining sword of Lewshire</shop>
        return tag->getAttrValue("href");
    } else if (!tag->getAttr(0).isNamed("PROMPT") && !tag->getAttr(0).isNamed("HINT") && !tag->getAttr(0).isNamed("EXPIRE")) {
        // has one attribute, but not called href
        // <SEND "tell Zugg " PROMPT>Zugg</SEND>
        // <send "drink &text;">fountain</send>
        return tag->getAttrName(0);
    } else {
        return "&text;";
    }
}

QString TMxpSendTagHandler::extractHint(MxpStartTag* tag)
{
    if (tag->hasAttr("hint")) {
        return tag->getAttrValue("hint");
    }

    if (tag->getAttrsCount() > 1 && !tag->getAttr(1).isNamed("PROMPT") && !tag->getAttr(1).isNamed("EXPIRE")) {
        return tag->getAttrName(1);
    }

    return QString();
}

TMxpTagHandlerResult TMxpSendTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    if (mIsHrefInContent) {
        QStringList *hrefs, *hints;
        if (client.getLink(mLinkId, &hrefs, &hints)) {
            if (hrefs != nullptr)
                hrefs->replaceInStrings("&text;", mCurrentTagContent, Qt::CaseInsensitive);

            if (hints != nullptr)
                hints->replaceInStrings("&text;", mCurrentTagContent, Qt::CaseInsensitive);
        }
        mCurrentTagContent.clear();
    }

    mIsHrefInContent = false;

    client.setLinkMode(false);
    return MXP_TAG_HANDLED;
}
void TMxpSendTagHandler::handleContent(char ch)
{
    if (mIsHrefInContent)
        mCurrentTagContent.append(ch);
}
