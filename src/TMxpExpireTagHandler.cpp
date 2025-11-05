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

#include "TMxpExpireTagHandler.h"
#include "TMxpClient.h"
#include "TMxpContext.h"

#ifdef DEBUG_MXP_PROCESSING
#include <QDebug>
#endif

// Override supports() to add debug output
bool TMxpExpireTagHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpExpireTagHandler::supports() called for tag:" << tag->getName();
#endif
    // Call parent class supports() which checks if tag->isNamed("EXPIRE")
    bool result = TMxpSingleTagHandler::supports(ctx, client, tag);
#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpExpireTagHandler::supports() returning:" << result;
#endif
    return result;
}

// <EXPIRE name>
// Removes all links tagged with the given expire name
TMxpTagHandlerResult TMxpExpireTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)

    QString expireName;

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpExpireTagHandler: Processing EXPIRE tag";
    qDebug() << "  Attribute count:" << tag->getAttributesCount();
    for (int i = 0; i < tag->getAttributesCount(); i++) {
        auto attr = tag->getAttribute(i);
        qDebug() << "  Attribute" << i << ":" << attr.getName() << "=" << attr.getValue();
    }
#endif

    // The expire name can be specified in different ways:
    // <EXPIRE name> or <EXPIRE name="name">
    if (tag->getAttributesCount() > 0) {
        if (tag->hasAttribute(qsl("name"))) {
            expireName = tag->getAttributeValue(qsl("name"));
        } else {
            // First attribute without a value is the name
            expireName = tag->getAttribute(0).getName();
        }
    }

    if (expireName.isEmpty()) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpExpireTagHandler: No expire name found, not handling";
#endif
        return MXP_TAG_NOT_HANDLED;
    }

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpExpireTagHandler: Expiring links with name:" << expireName;
#endif

    client.expireLinks(expireName);

    return MXP_TAG_HANDLED;
}
