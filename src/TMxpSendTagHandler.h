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

#ifndef MUDLET_TMXPSENDTAGHANDLER_H
#define MUDLET_TMXPSENDTAGHANDLER_H
#include "TEntityResolver.h"
#include "TMxpTagHandler.h"

// <SEND [href=command] [hint=text] [prompt] [expire=name]>
class TMxpSendTagHandler : public TMxpSingleTagHandler
{
    inline static const QString ATTR_HREF = QStringLiteral("href");
    inline static const QString ATTR_HINT = QStringLiteral("hint");
    inline static const QString ATTR_PROMPT = QStringLiteral("prompt");
    inline static const QString ATTR_EXPIRE = QStringLiteral("expire");
    inline static const QString TAG_CONTENT_PLACEHOLDER = QStringLiteral("&text;");

    bool mIsHrefInContent;
    QString mCurrentTagContent;
    int mLinkId;

    void updateHrefInLinks(TMxpClient& client) const;
    void resetCurrentTagContent(TMxpClient& client);
public:
    static QString extractHref(MxpStartTag* tag);
    static QString extractHint(MxpStartTag* tag);

    TMxpSendTagHandler();
    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
    TMxpTagHandlerResult handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag) override;

    void handleContent(char ch) override;

};
#include "TMxpTagHandler.h"
#endif //MUDLET_TMXPSENDTAGHANDLER_H
