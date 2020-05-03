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

#include "TMxpCustomElementTagHandler.h"
#include "TEntityResolver.h"
#include "TMxpClient.h"
#include "TMxpTagParser.h"


TMxpTagHandlerResult TMxpCustomElementTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    TMxpElement el = ctx.getElementRegistry().getElement(tag->getName());
    if (!el.flags.isEmpty()) {
        mCurrentFlagAttributes = parseFlagAttributes(tag, el);
        if (el.empty || tag->isEmpty()) {
            setFlag(client, tag, el);
        } else {
            configFlag(client, tag, el);
        }
    }

    if (!el.definition.isEmpty()) {
        for (const QSharedPointer<MxpNode>& ptr : el.parsedDefinition) {
            if (!ptr->isTag()) {
                ctx.handleContent(ptr->asText()->getContent());
            } else {
                // transform the custom tag to the given in the definition
                MxpStartTag newTag = resolveElementDefinition(el, ptr->asStartTag(), tag);
                ctx.handleTag(ctx, client, &newTag);
            }
        }
    }

    return MXP_TAG_HANDLED;
}

TMxpTagHandlerResult TMxpCustomElementTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    TMxpElement el = ctx.getElementRegistry().getElement(tag->getName());

    if (!el.flags.isEmpty() && !mCurrentFlagName.isEmpty()) { // is closing a custom tag with flag
        client.setFlag(mCurrentFlagName, mCurrentFlagAttributes, mCurrentFlagContent);
        mCurrentFlagName.clear();
    }

    if (el.definition.isEmpty())
        return MXP_TAG_HANDLED; //NO DEFINITION


    // generates closing tags in the reverse order
    // in the example: <!ELEMENT boldtext '<COLOR &col;><B>' ATT='col=red'>
    // will generate </B></COLOR>
    for (int i = el.parsedDefinition.size(); i > 0; i--) {
        MxpNode* node = el.parsedDefinition[i - 1].get();
        if (node->isTag()) {
            MxpEndTag endTag(node->asStartTag()->getName());
            ctx.handleTag(ctx, client, &endTag);
        }
    }

    return MXP_TAG_HANDLED;
}

// Receives and element definition such as:
//              <!EL help "<send href='help &desc;' hint='Click for help on &desc;' expire=help>" ATT='desc'>
// and a custom tag such as:
//              <help desc="1024">1024</help>
// and returns a new tag interpolating the definition with the custom tag values:
//              <send href='help 1024;' hint='Click for help on 1024;' expire=help>
MxpStartTag TMxpCustomElementTagHandler::resolveElementDefinition(const TMxpElement& element, MxpStartTag* definitionTag, MxpStartTag* customTag) const
{
    auto mapping = [this, customTag, element](const MxpTagAttribute& attr) {
        if (!attr.hasValue()) {
            return MxpTagAttribute(mapAttributes(element, attr.getName(), customTag));
        } else {
            if (attr.isNamed("hint")) { // not needed according to the spec, but kept to avoid changes for the user interface
                return MxpTagAttribute(attr.getName(), mapAttributes(element, attr.getValue().toUpper(), customTag));
            } else {
                return MxpTagAttribute(attr.getName(), mapAttributes(element, attr.getValue(), customTag));
            }
        }
    };

    return definitionTag->transform(mapping);
}

QString TMxpCustomElementTagHandler::mapAttributes(const TMxpElement& element, const QString& input, MxpStartTag* tag)
{
    auto mapEntityNameToTagAttributeValue = [element, tag](const QString& input) {
        QString attrName = input.mid(1, input.size() - 2);
        // get attribute value by NAME
        // <!EL help "<send href='help &desc;' hint='Click for help on &desc;' expire=help>" ATT='desc'>
        // <help desc="1024">1024</help>
        if (tag->hasAttribute(attrName)) {
            return tag->getAttributeValue(attrName);
        }

        // get attribute value by INDEX
        // <!el i13 '<send href="look &id; on ground|get all from &id; on ground" hint="look &id; on ground|get all from &id; on ground" >' att='id'>
        //  <i13 "trash can 1">A trash can</i13>
        int attrIndex = element.attrs.indexOf(attrName.toLower());
        if (attrIndex != -1 && tag->getAttributesCount() > attrIndex) {
            return tag->getAttribute(attrIndex).getName();
        }

        return input;
    };

    return TEntityResolver::interpolate(input, mapEntityNameToTagAttributeValue);
}
void TMxpCustomElementTagHandler::configFlag(TMxpClient& client, MxpStartTag* tag, const TMxpElement& el)
{
    mCurrentFlagName = el.name;
    parseFlagAttributes(tag, el);
    mCurrentFlagContent.clear();
}

void TMxpCustomElementTagHandler::setFlag(TMxpClient& ctx, const MxpStartTag* tag, const TMxpElement& el)
{
    ctx.setFlag(el.name, parseFlagAttributes(tag, el), "");
}

const QMap<QString, QString>& TMxpCustomElementTagHandler::parseFlagAttributes(const MxpStartTag* tag, const TMxpElement& el)
{
    QMap<QString, QString>& values = mCurrentFlagAttributes;

    values.clear();
    for (int i = 0; i < el.attrs.size(); i++) {
        const QString& attrName = el.attrs[i];

        if (tag->hasAttribute(attrName)) {
            values[attrName] = tag->getAttributeValue(attrName);
        } else if (tag->getAttributesCount() > i) {
            values[attrName] = tag->getAttribute(i).getName();
        }
    }
    return values;
}

void TMxpCustomElementTagHandler::handleContent(char ch)
{
    if (!mCurrentFlagName.isEmpty()) {
        mCurrentFlagContent.append(ch);
    }
}
