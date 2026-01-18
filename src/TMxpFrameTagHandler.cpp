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

#include "TMxpFrameTagHandler.h"
#include "TMxpClient.h"

bool TMxpFrameTagHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    Q_UNUSED(ctx)
    Q_UNUSED(client)
    return tag->isNamed(qsl("FRAME"));
}

TMxpTagHandlerResult TMxpFrameTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)
    
    QMap<QString, QString> attributes = extractAttributes(tag);
    QString frameName = attributes.value(qsl("NAME"));
    
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameTagHandler::handleStartTag: FRAME tag received"
             << "name:" << frameName
             << "attributes:" << attributes;
#endif
    
    if (frameName.isEmpty()) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpFrameTagHandler::handleStartTag: Empty frame name, not handled";
#endif
        return MXP_TAG_NOT_HANDLED;
    }
    
    if (client.createMxpFrame(frameName, attributes)) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpFrameTagHandler::handleStartTag: Frame created successfully:" << frameName;
#endif
        return MXP_TAG_HANDLED;
    }
    
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameTagHandler::handleStartTag: Frame creation failed:" << frameName;
#endif
    return MXP_TAG_NOT_HANDLED;
}

QMap<QString, QString> TMxpFrameTagHandler::extractAttributes(MxpStartTag* tag)
{
    QMap<QString, QString> attributes;
    
    // Get the NAME attribute (can be first positional argument or named)
    QString name = tag->getAttributeByNameOrIndex(qsl("NAME"), 0);

    if (!name.isEmpty()) {
        attributes[qsl("NAME")] = name;
    }
    
    // Extract all other attributes
    const auto& attrNames = tag->getAttributesNames();

    for (const auto& attrName : attrNames) {
        QString upperName = attrName.toUpper();
        const auto& attr = tag->getAttribute(attrName);
        
        // Common attributes
        if (upperName == qsl("INTERNAL")) {
            attributes[qsl("INTERNAL")] = qsl("true");
        } else if (upperName == qsl("EXTERNAL")) {
            attributes[qsl("EXTERNAL")] = qsl("true");
        } else if (upperName == qsl("ALIGN")) {
            attributes[qsl("ALIGN")] = attr.getValue();
        } else if (upperName == qsl("WIDTH")) {
            attributes[qsl("WIDTH")] = attr.getValue();
        } else if (upperName == qsl("HEIGHT")) {
            attributes[qsl("HEIGHT")] = attr.getValue();
        } else if (upperName == qsl("SCROLLING")) {
            attributes[qsl("SCROLLING")] = attr.getValue();
        } else if (upperName == qsl("TITLE")) {
            attributes[qsl("TITLE")] = attr.getValue();
        } else if (upperName == qsl("DOCK")) {
            // DOCK: Add this frame as a tab in an existing frame
            // This is a CMUD extension (not in official MXP 1.0 spec)
            attributes[qsl("DOCK")] = attr.getValue();
        } else if (upperName == qsl("ACTION")) {
            attributes[qsl("ACTION")] = attr.getValue();
        } else if (upperName == qsl("LEFT")) {
            // Absolute X position (pixels or percentage)
            attributes[qsl("LEFT")] = attr.getValue();
        } else if (upperName == qsl("TOP")) {
            // Absolute Y position (pixels or percentage)
            attributes[qsl("TOP")] = attr.getValue();
        } else if (upperName == qsl("FLOATING")) {
            // FLOATING makes the frame a floating window (similar to EXTERNAL)
            attributes[qsl("FLOATING")] = qsl("true");
        }
    }
    
    return attributes;
}
