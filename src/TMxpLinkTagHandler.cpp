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
#include "TMxpLinkTagHandler.h"
#include "TMxpClient.h"

// <A href=URL [hint=text] [expire=name]>
TMxpTagHandlerResult TMxpLinkTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    if (tag->hasAttr("EXPIRE"))
        return MXP_TAG_NOT_HANDLED;

    QString href = getHref(tag);
    if (href.isEmpty())
        return MXP_TAG_NOT_HANDLED;

    QString hint = tag->hasAttr("hint") ? tag->getAttrValue("hint") : href;

    href = QStringLiteral("openUrl([[%1]])").arg(href);

    mLinkId = client.setLink(QStringList(href), QStringList(hint));
    client.setLinkMode(true);
    return MXP_TAG_HANDLED;
}
TMxpTagHandlerResult TMxpLinkTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    QStringList* links, * hints;
    client.getLink(mLinkId, &links, &hints);

    if (links != nullptr) {
        links->replaceInStrings("&text;", mCurrentTagContent, Qt::CaseInsensitive);
    }

    client.setLinkMode(false);
    return MXP_TAG_HANDLED;
}
QString TMxpLinkTagHandler::getHref(const MxpStartTag* tag)
{
    if (tag->getAttrsCount() == 0) {
        // <A>http://someurl.com/<A>
        mIsHrefInContent = true;
        return "&text;";
    } else if (tag->hasAttr("href")) {
        return tag->getAttrValue("href");
    } else if (!tag->getAttr(0).hasValue()) {
        return tag->getAttr(0).getName();
    } else {
        return "";
    }
}
void TMxpLinkTagHandler::handleContent(char ch)
{
    if (mIsHrefInContent)
        mCurrentTagContent.append(ch);
}