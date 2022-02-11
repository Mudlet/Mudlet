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
#include "TMxpEntityTagHandler.h"
#include "TStringUtils.h"

TMxpTagHandlerResult TMxpEntityTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    if (tag->getAttributesCount() < 2) {
        return MXP_TAG_NOT_HANDLED;
    }

    static const QStringList boolOptions({"PRIVATE", "PUBLISH", "DELETE", "ADD", "REMOVE"});
    TEntityResolver& resolver = ctx.getEntityResolver();

    const QString& name = tag->getAttrName(0);
    const QString& entity = qsl("&%1;").arg(name);

    if (tag->hasAttribute("DELETE")) {
        resolver.unregisterEntity(entity);
    } else if (!boolOptions.contains(tag->getAttrName(1), Qt::CaseInsensitive)) { // 2nd attribute is actually the value
        const QString& value = tag->getAttrName(1);
        if (tag->hasAttribute("ADD")) {
            QString newDefinition = resolver.getResolution(entity);
            TStringUtils::listAddItem(newDefinition, value);
            resolver.registerEntity(entity, newDefinition);
        } else if (tag->hasAttribute("REMOVE")) {
            QString newDefinition = resolver.getResolution(entity);
            TStringUtils::listRemoveItem(newDefinition, value);
            resolver.registerEntity(entity, newDefinition);
        } else {
            resolver.registerEntity(entity, value);
        }

        if (tag->hasAttribute("PUBLISH")) {
            client.publishEntity(entity, value);
        }
    }

    return MXP_TAG_HANDLED;
}
