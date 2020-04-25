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

#include "MxpTag.h"
#include "TMxpTagParser.h"

const MxpTagAttribute& MxpStartTag::getAttribute(int attrIndex) const
{
    return getAttribute(mAttrsNames[attrIndex]);
}

const MxpTagAttribute& MxpStartTag::getAttribute(const QString& attrName) const
{
    const auto ptr = mAttrsMap.find(attrName.toUpper());
    return *ptr;
}

const QString& MxpStartTag::getAttributeValue(int attrIndex) const
{
    return getAttribute(attrIndex).getValue();
}

const QString& MxpStartTag::getAttributeValue(const QString& attrName) const
{
    return getAttribute(attrName).getValue();
}

bool MxpStartTag::hasAttribute(const QString& attrName) const
{
    return mAttrsMap.contains(attrName.toUpper());
}

bool MxpStartTag::isAttributeAt(const char* attrName, int attrIndex)
{
    return mAttrsNames[attrIndex].compare(attrName, Qt::CaseInsensitive) == 0;
}

bool MxpTag::isNamed(const QString& tagName) const
{
    return name.compare(tagName, Qt::CaseInsensitive) == 0;
}

QString MxpEndTag::toString() const
{
    QString result;
    result.append("</");
    result.append(name);
    result.append(">");
    return result;
}

QString MxpStartTag::toString() const
{
    QString result;
    result.append('<');
    result.append(name);
    for (const auto& attrName : mAttrsNames) {
        result.append(' ');
        if (attrName.contains(" ") || attrName.contains("<")) {
            result.append('"');
            result.append(attrName);
            result.append('"');
        } else {
            result.append(attrName);
        }

        const auto& attr = getAttribute(attrName);
        if (attr.hasValue()) {
            result.append('=');

            const auto& val = attr.getValue();
            result.append('"');
            result.append(val);
            result.append('"');
        }
    }

    if (mIsEmpty) {
        result.append(" /");
    }

    result.append('>');

    return result;
}

MxpStartTag MxpStartTag::transform(const MxpTagAttribute::Transformation& transformation) const
{
    QList<MxpTagAttribute> newAttrs;
    for (const auto& attr : mAttrsNames) {
        newAttrs.append(transformation(mAttrsMap[attr.toUpper()]));
    }

    return MxpStartTag(name, newAttrs, mIsEmpty);
}

const QString& MxpStartTag::getAttributeByNameOrIndex(const QString& attrName, int attrIndex, const QString& defaultValue) const
{
    if (hasAttribute(attrName))
        return getAttributeValue(attrName);
    if (getAttributesCount() > attrIndex && !getAttribute(attrIndex).hasValue())
        return getAttribute(attrIndex).getName();

    return defaultValue;
}

const QString& MxpStartTag::getAttrName(int attrIndex) const
{
    return mAttrsNames[attrIndex];
}
