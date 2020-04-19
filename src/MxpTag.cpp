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
#include <QtDebug>

const MxpTagAttribute& MxpStartTag::getAttr(int attrIndex) const
{
    return getAttr(mAttrsNames[attrIndex]);
}

const MxpTagAttribute& MxpStartTag::getAttr(const QString& attrName) const
{
    const auto ptr = mAttrsMap.find(attrName.toUpper());
    return *ptr;
}

const QString& MxpStartTag::getAttrValue(int attrIndex) const
{
    return getAttr(attrIndex).getValue();
}

const QString& MxpStartTag::getAttrValue(const QString& attrName) const
{
    return getAttr(attrName).getValue();
}

bool MxpStartTag::hasAttr(const QString& attrName) const
{
    return mAttrsMap.contains(attrName.toUpper());
}

bool MxpStartTag::isAttrAt(int pos, const char* str)
{
    return mAttrsNames[pos].compare(str, Qt::CaseInsensitive) == 0;
}

bool MxpTag::isNamed(const QString& tagName) const
{
    return name.compare(tagName, Qt::CaseInsensitive) == 0;
}

QString MxpEndTag::asString() const
{
    QString result;
    result.append("</");
    result.append(name);
    result.append(">");
    return result;
}

QString MxpStartTag::asString() const
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

        const auto& attr = getAttr(attrName);
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

const QString& MxpStartTag::getAttrByNameOrIndex(const QString& attrName, int attrIndex, const QString& defaultValue) const
{
    if (hasAttr(attrName))
        return getAttrValue(attrName);
    if (getAttrsCount() > attrIndex && !getAttr(attrIndex).hasValue())
        return getAttr(attrIndex).getName();

    return defaultValue;
}
const QString& MxpStartTag::getAttrName(int attrIndex) const
{
    return mAttrsNames[attrIndex];
}
