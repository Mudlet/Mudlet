/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020 by Stephem Lyons - slysven@virginmedia.com         *
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
    Q_UNUSED(client)
    if (tag->getAttributesCount() < 2) { // UNEXPECTED: element without definition nor attributes
        return MXP_TAG_NOT_HANDLED;
    }

    TMxpElement element;
    element.name = tag->getAttrName(0); // element-name

    if (tag->hasAttribute("DELETE")) {
        ctx.getElementRegistry().unregisterElement(element.name);
        return MXP_TAG_HANDLED;
    }

    static const QStringList boolAttrs({"OPEN", "DELETE", "EMPTY"});
    if (!tag->getAttribute(1).hasValue()) {
        const QString& secondAttr = tag->getAttrName(1);
        if (!boolAttrs.contains(secondAttr, Qt::CaseInsensitive)) { // it is a definition and not  {OPEN, DELETE, EMPTY}
            element.definition = secondAttr;
        }
    }

    if (tag->hasAttribute("ATT")) {
        // Split attribute list into list of attributes and loop over each definiten
        element.attrs = tag->getAttributeValue("ATT").split(' ', Qt::SkipEmptyParts);

        for (int i = 0, numattr = element.attrs.size(); i < numattr; i++) {
            QString attr = element.attrs.at(i);
            // Find an argument after a = (if any), like in ATT="NAME=someone"
            QString arg = attr.section("=", 1);

            // The attribute name is the first part before any = . We have them all lowercase internally
            element.attrs[i] = attr = attr.section("=", 0, 0).toLower();
            if (!arg.isEmpty()) {
                // If a default argument was specified, store it.
                element.defaultValues[attr] = arg;
            }
        }
    }

    if (tag->hasAttribute("TAG")) {
        element.tag = tag->getAttributeValue("TAG");
    }

    if (tag->hasAttribute("FLAG")) {
        element.flags = tag->getAttributeValue("FLAG");
    }

    element.open = tag->hasAttribute("OPEN");
    element.empty = tag->hasAttribute("EMPTY");

    if (!element.definition.isEmpty()) {
        element.parsedDefinition = TMxpTagParser::parseToMxpNodeList(element.definition);
    }

    ctx.getElementRegistry().registerElement(element);

    return MXP_TAG_HANDLED;
}
