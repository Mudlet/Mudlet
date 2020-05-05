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

#ifndef MUDLET_TMXPTAGHANDLER_H
#define MUDLET_TMXPTAGHANDLER_H

#include "MxpTag.h"
#include "TMxpTagHandlerResult.h"

#include "pre_guard.h"
#include <QString>
#include "post_guard.h"

class TMxpClient;
class TMxpContext;

class TMxpTagHandler
{
public:
    virtual TMxpTagHandlerResult handleTag(TMxpContext& ctx, TMxpClient& client, MxpTag* tag);

    virtual void handleContent(char ch) {}

    void handleContent(const QString& text)
    {
        for (auto& ch : text) {
            handleContent(ch.toLatin1());
        }
    }

    virtual void handleTextNode(TMxpContext& ctx, TMxpClient& client, MxpTextNode* tag) {
        handleContent(tag->getContent());
    }

    virtual TMxpTagHandlerResult handleNode(TMxpContext& ctx, TMxpClient& client, MxpNode* node) {
        if (node->isTag()) {
            return handleTag(ctx, client, node->asTag());
        } else {
            handleTextNode(ctx, client, node->asText());
            return MXP_TAG_HANDLED;
        }
    }

    virtual bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) { return true; }

    virtual TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) { return MXP_TAG_NOT_HANDLED; }

    virtual TMxpTagHandlerResult handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag) { return MXP_TAG_NOT_HANDLED; }

    virtual ~TMxpTagHandler() = default;
};

class TMxpSingleTagHandler : public TMxpTagHandler
{
    QString tagName;

public:
    explicit TMxpSingleTagHandler(QString tagName) : tagName(std::move(tagName)) {}

    virtual bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) { return tag->isNamed(tagName); }
};

#endif //MUDLET_TMXPTAGHANDLER_H
