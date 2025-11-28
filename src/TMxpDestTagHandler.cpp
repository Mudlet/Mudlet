/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TMxpDestTagHandler.h"
#include "TMxpClient.h"

bool TMxpDestTagHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    Q_UNUSED(ctx)
    Q_UNUSED(client)
    return tag->isNamed(qsl("DEST"));
}

TMxpTagHandlerResult TMxpDestTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)
    
    QString frameName = extractFrameName(tag);
    
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpDestTagHandler::handleStartTag: DEST tag received"
             << "frameName:" << frameName
             << "EOL:" << hasEOL(tag)
             << "EOF:" << hasEOF(tag);
#endif
    
    if (frameName.isEmpty()) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpDestTagHandler::handleStartTag: Empty frame name, not handled";
#endif
        return MXP_TAG_NOT_HANDLED;
    }
    
    bool eol = hasEOL(tag);
    bool eof = hasEOF(tag);
    
    if (client.setMxpDestination(frameName, eol, eof)) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpDestTagHandler::handleStartTag: Destination set to:" << frameName;
#endif
        return MXP_TAG_HANDLED;
    }
    
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpDestTagHandler::handleStartTag: Failed to set destination:" << frameName;
#endif
    return MXP_TAG_NOT_HANDLED;
}

TMxpTagHandlerResult TMxpDestTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    Q_UNUSED(ctx)
    Q_UNUSED(tag)
    
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpDestTagHandler::handleEndTag: Clearing destination";
#endif
    client.clearMxpDestination();
    
    return MXP_TAG_HANDLED;
}

QString TMxpDestTagHandler::extractFrameName(MxpStartTag* tag)
{
    // Frame name can be first positional argument or NAME attribute
    QString frameName = tag->getAttributeByNameOrIndex(qsl("NAME"), 0);
    
    // If empty, check if any attribute without a value (flag-style)
    if (frameName.isEmpty()) {
        const auto& attrNames = tag->getAttributesNames();

        for (const auto& attrName : attrNames) {
            // Skip known flags
            if (attrName.compare(qsl("EOL"), Qt::CaseInsensitive) != 0 &&
                attrName.compare(qsl("EOF"), Qt::CaseInsensitive) != 0 &&
                attrName.compare(qsl("NAME"), Qt::CaseInsensitive) != 0) {
                const auto& attr = tag->getAttribute(attrName);
                // Assume this is the frame name
                if (!attr.hasValue()) {
                    frameName = attrName;
                    break;
                }
            }
        }
    }
    
    return frameName;
}

bool TMxpDestTagHandler::hasEOL(MxpStartTag* tag)
{
    return tag->hasAttribute(qsl("EOL"));
}

bool TMxpDestTagHandler::hasEOF(MxpStartTag* tag)
{
    return tag->hasAttribute(qsl("EOF"));
}
