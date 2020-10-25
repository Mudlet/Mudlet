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
TMxpTagHandlerResult TMxpEntityTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    if (tag->getAttributesCount() < 2) {
        return MXP_TAG_NOT_HANDLED;
    }

    static const QStringList boolOptions({"PRIVATE", "PUBLISH", "DELETE", "ADD", "REMOVE"});
    TEntityResolver& resolver = ctx.getEntityResolver();

    const QString& name = tag->getAttrName(1);

    if (tag->hasAttribute("DELETE")) {
        resolver.unregisterEntity(name);
    } else if (!boolOptions.contains(tag->getAttribute(1).getName(), Qt::CaseInsensitive)) { // 2nd attribute is actually the value
        const QString& value = tag->getAttrName(1);
        if (tag->hasAttribute("ADD")) {
            QString newDefinition = resolver.getResolution(name);
            newDefinition.append("|");
            newDefinition.append(value);

            resolver.registerEntity(name, newDefinition);
        } else if (tag->hasAttribute("REMOVE")) {
            QString currentValue = resolver.getResolution(name);
            QString toDelete = currentValue.contains("|") ? "|" + value : value;
            resolver.registerEntity(name, currentValue.replace(toDelete, ""));
        } else { // PUBLISH
            resolver.registerEntity(name, value);
            client.publishEntity(name, value);
        }
    }

    return MXP_TAG_HANDLED;
}
