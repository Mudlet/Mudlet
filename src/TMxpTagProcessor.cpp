/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TMxpTagProcessor.h"
#include "TMxpBRTagHandler.h"
#include "TMxpColorTagHandler.h"
#include "TMxpCustomElementTagHandler.h"
#include "TMxpElementDefinitionHandler.h"
#include "TMxpEntityTagHandler.h"
#include "TMxpFontTagHandler.h"
#include "TMxpFormattingTagsHandler.h"
#include "TMxpLinkTagHandler.h"
#include "TMxpSendTagHandler.h"
#include "TMxpSupportTagHandler.h"
#include "TMxpTagHandlerResult.h"
#include "TMxpTagParser.h"
#include "TMxpVarTagHandler.h"
#include "TMxpVersionTagHandler.h"

TMxpTagHandlerResult TMxpTagProcessor::process(TMxpContext& ctx, TMxpClient& client, const std::string& currentToken)
{
    TMxpTagParser parser;
    QScopedPointer<MxpTag> tag(parser.parseTag(currentToken.c_str()));

    return handleTag(ctx, client, tag.get());
}

TMxpTagHandlerResult TMxpTagProcessor::handleTag(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    if (!client.tagReceived(tag)) {
        return MXP_TAG_NOT_HANDLED;
    }

    for (const auto& handler : mRegisteredHandlers) {
        TMxpTagHandlerResult result = handler->handleTag(ctx, client, tag);

        if (result != MXP_TAG_NOT_HANDLED) {
            result = client.tagHandled(tag, result);
            if (result != MXP_TAG_NOT_HANDLED) {
                return result;
            }                
        }
    }

    return MXP_TAG_NOT_HANDLED;
}

void TMxpTagProcessor::handleContent(char ch)
{
    for (const auto& handler : mRegisteredHandlers) {
        handler->handleContent(ch);
    }
}

TMxpTagProcessor::TMxpTagProcessor()
{
    registerHandler(new TMxpVersionTagHandler());
    registerHandler(new TMxpSupportTagHandler());

    registerHandler(TMxpFeatureOptions({"var", {"publish"}}), new TMxpVarTagHandler());
    registerHandler(TMxpFeatureOptions({"br", {}}), new TMxpBRTagHandler());
    registerHandler(TMxpFeatureOptions({"send", {"href", "hint", "prompt"}}), new TMxpSendTagHandler());
    registerHandler(TMxpFeatureOptions({"a", {"href", "hint"}}), new TMxpLinkTagHandler());
    registerHandler(TMxpFeatureOptions({"color", {"fore", "back"}}), new TMxpColorTagHandler());
    registerHandler(TMxpFeatureOptions({"font", {"color", "back"}}), new TMxpFontTagHandler());

    mSupportedMxpElements["b"] = QVector<QString>();
    mSupportedMxpElements["i"] = QVector<QString>();
    mSupportedMxpElements["u"] = QVector<QString>();
    mRegisteredHandlers.append(QSharedPointer<TMxpTagHandler>(new TMxpFormattingTagsHandler()));

    registerHandler(new TMxpEntityTagHandler());
    registerHandler(new TMxpElementDefinitionHandler());
    registerHandler(new TMxpCustomElementTagHandler());
}

void TMxpTagProcessor::registerHandler(const TMxpFeatureOptions& supports, TMxpTagHandler* handler)
{
    mSupportedMxpElements[supports.first].append(supports.second);
    mRegisteredHandlers.append(QSharedPointer<TMxpTagHandler>(handler));
}

void TMxpTagProcessor::registerHandler(TMxpTagHandler* handler)
{
    mRegisteredHandlers.append(QSharedPointer<TMxpTagHandler>(handler));
}
TMxpElementRegistry& TMxpTagProcessor::getElementRegistry()
{
    return mMxpElementRegistry;
}
QMap<QString, QVector<QString>>& TMxpTagProcessor::getSupportedElements()
{
    return mSupportedMxpElements;
}
TMxpTagHandler& TMxpTagProcessor::getMainHandler()
{
    return *this;
}
TEntityResolver& TMxpTagProcessor::getEntityResolver()
{
    return mEntityResolver;
}
