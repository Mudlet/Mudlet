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
#include "TMxpElementDefinitionHandler.h"
#include "TMxpContext.h"
#include "TMxpTagParser.h"

// <!ELEMENT element-name [definition] [ATT=attribute-list] [TAG=tag] [FLAG=flags] [OPEN] [DELETE] [EMPTY]>
TMxpTagHandlerResult TMxpElementDefinitionHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    if (tag->getAttributesCount() < 2) { // UNEXPECTED: element without definition nor attributes
        return MXP_TAG_NOT_HANDLED;
    }

    TMxpElement el;
    el.name = tag->getAttrName(0); // element-name

    if (tag->hasAttribute("DELETE")) {
        ctx.getElementRegistry().unregisterElement(el.name);
        return MXP_TAG_HANDLED;
    }

    static const QStringList boolAttrs({"OPEN", "DELETE", "EMPTY"});
    if (!tag->getAttribute(1).hasValue()) {
        const QString& secondAttr = tag->getAttrName(1);
        if (!boolAttrs.contains(secondAttr, Qt::CaseInsensitive)) { // it is a definition and not  {OPEN, DELETE, EMPTY}
            el.definition = secondAttr;
        }
    }

    if (tag->hasAttribute("ATT")) {
        el.attrs = tag->getAttributeValue("ATT").toLower().split(' ', QString::SkipEmptyParts);
    }

    if (tag->hasAttribute("TAG")) {
        el.tag = tag->getAttributeValue("TAG");
    }

    if (tag->hasAttribute("FLAG")) {
        el.flags = tag->getAttributeValue("FLAG");
    }

    el.open = tag->hasAttribute("OPEN");
    el.empty = tag->hasAttribute("EMPTY");

    if (!el.definition.isEmpty()) {
        TMxpTagParser parser;
        el.parsedDefinition = parser.parseToMxpNodeList(el.definition);
    }

    ctx.getElementRegistry().registerElement(el);

    return MXP_TAG_HANDLED;
}
