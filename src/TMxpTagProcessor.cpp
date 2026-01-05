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
#include "TMxpDestTagHandler.h"
#include "TMxpElementDefinitionHandler.h"
#include "TMxpEntityTagHandler.h"
#include "TMxpExpireTagHandler.h"
#include "TMxpFontTagHandler.h"
#include "TMxpFormattingTagsHandler.h"
#include "TMxpFrameTagHandler.h"
#include "TMxpHRTagHandler.h"
#include "TMxpImageTagHandler.h"
#include "TMxpLinkTagHandler.h"
#include "TMxpMusicTagHandler.h"
#include "TMxpSendTagHandler.h"
#include "TMxpSoundTagHandler.h"
#include "TMxpSupportTagHandler.h"
#include "TMxpTagHandlerResult.h"
#include "TMxpTagParser.h"
#include "TMxpVarTagHandler.h"
#include "TMxpVersionTagHandler.h"

#ifdef DEBUG_MXP_PROCESSING
#include <QDebug>
#endif

TMxpTagHandlerResult TMxpTagProcessor::handleTag(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpTagProcessor::handleTag() processing tag:" << tag->getName();
#endif
    
    if (!client.tagReceived(tag)) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "  client.tagReceived() returned false, not handling";
#endif
        return MXP_TAG_NOT_HANDLED;
    }

    for (const auto& handler : mRegisteredHandlers) {
        TMxpTagHandlerResult result = handler->handleTag(ctx, client, tag);

        if (result != MXP_TAG_NOT_HANDLED) {
#ifdef DEBUG_MXP_PROCESSING
            qDebug() << "  Handler handled tag, result:" << result;
#endif
            result = client.tagHandled(tag, result);
            if (result != MXP_TAG_NOT_HANDLED) {
                return result;
            }
        }
    }

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "  No handler handled tag:" << tag->getName();
#endif
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
    // Version control tags
    registerHandler(TMxpFeatureOptions({"version", {}}), new TMxpVersionTagHandler());
    registerHandler(TMxpFeatureOptions({"support", {}}), new TMxpSupportTagHandler());

    // Variable and entity tags
    registerHandler(TMxpFeatureOptions({"var", {"publish"}}), new TMxpVarTagHandler());
    registerHandler(TMxpFeatureOptions({"entity", {"name", "value", "desc", "private", "publish", "delete", "add", "remove"}}), new TMxpEntityTagHandler());
    
    // Line spacing tags
    registerHandler(TMxpFeatureOptions({"br", {}}), new TMxpBRTagHandler());
    registerHandler(TMxpFeatureOptions({"hr", {}}), new TMxpHRTagHandler());
    
    // Link tags
    registerHandler(TMxpFeatureOptions({"send", {"href", "hint", "prompt", "expire"}}), new TMxpSendTagHandler());
    registerHandler(TMxpFeatureOptions({"a", {"href", "hint", "expire"}}), new TMxpLinkTagHandler());
    registerHandler(TMxpFeatureOptions({"expire", {"name"}}), new TMxpExpireTagHandler());
    
    // Color and font tags
    registerHandler(TMxpFeatureOptions({"color", {"fore", "back"}}), new TMxpColorTagHandler());
    registerHandler(TMxpFeatureOptions({"font", {"color", "back"}}), new TMxpFontTagHandler());
    
    // Media tags (MSP compatibility)
    registerHandler(TMxpFeatureOptions({"sound", {"fname", "v", "l", "p", "t", "u"}}), new TMxpSoundTagHandler());
    registerHandler(TMxpFeatureOptions({"music", {"fname", "v", "l", "p", "c", "t", "u"}}), new TMxpMusicTagHandler());

    // Image tag (placeholder - not yet implemented, but swallow it)
    // Register without advertising support since IMAGE is not fully implemented
    registerHandler(new TMxpImageTagHandler());

    // Frame and destination tags
    registerHandler(TMxpFeatureOptions({"frame", {"name", "action", "internal", "external", "align", "left", "right", "top", "bottom", "width", "height", "scrolling", "floating", "title"}}), new TMxpFrameTagHandler());
    registerHandler(TMxpFeatureOptions({"dest", {"name", "eol", "eof"}}), new TMxpDestTagHandler());

    // Formatting tags (text style)
    mSupportedMxpElements["b"] = QVector<QString>();
    mSupportedMxpElements["bold"] = QVector<QString>();
    mSupportedMxpElements["strong"] = QVector<QString>();
    mSupportedMxpElements["h"] = QVector<QString>();
    mSupportedMxpElements["high"] = QVector<QString>();

    mSupportedMxpElements["i"] = QVector<QString>();
    mSupportedMxpElements["italic"] = QVector<QString>();
    mSupportedMxpElements["em"] = QVector<QString>();

    mSupportedMxpElements["u"] = QVector<QString>();
    mSupportedMxpElements["underline"] = QVector<QString>();

    mSupportedMxpElements["s"] = QVector<QString>();
    mSupportedMxpElements["strikeout"] = QVector<QString>();

    // Additional HTML tags - recognized but not styled differently
    mSupportedMxpElements["h1"] = QVector<QString>();
    mSupportedMxpElements["h2"] = QVector<QString>();
    mSupportedMxpElements["h3"] = QVector<QString>();
    mSupportedMxpElements["h4"] = QVector<QString>();
    mSupportedMxpElements["h5"] = QVector<QString>();
    mSupportedMxpElements["h6"] = QVector<QString>();
    mSupportedMxpElements["small"] = QVector<QString>();
    mSupportedMxpElements["tt"] = QVector<QString>();

    mRegisteredHandlers.append(QSharedPointer<TMxpTagHandler>(new TMxpFormattingTagsHandler()));

    // Custom element support
    registerHandler(TMxpFeatureOptions({"element", {"name", "definition", "att", "tag", "flag", "open", "delete", "empty"}}), new TMxpElementDefinitionHandler());
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
