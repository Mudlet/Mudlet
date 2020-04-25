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

#include "TMxpTagParser.h"
#include "TMxpNodeBuilder.h"
#include "TStringUtils.h"
#include <QDebug>

static QStringRef stripTagAndTrim(const QString& tagText)
{
    return TStringUtils::trimmedRef(TStringUtils::stripRef(tagText, '<', '>'));
}

static bool isEndTag(const QString& tagText)
{
    return stripTagAndTrim(tagText).startsWith("/");
}

static bool isTag(const QString& tagString)
{
    return TStringUtils::isBetween(TStringUtils::trimmedRef(tagString), '<', '>');
}

QList<QSharedPointer<MxpNode>> TMxpTagParser::parseToMxpNodeList(const QString& tagText, bool ignoreText) const
{
    TMxpNodeBuilder nodeBuilder(ignoreText);

    QList<QSharedPointer<MxpNode>> result;
    for (int i = 0; i < tagText.length(); i++) {
        if (nodeBuilder.accept(tagText[i].toLatin1())) {
            result.append(QSharedPointer<MxpNode>(nodeBuilder.buildNode()));
            i--;
        }
    }

    if (nodeBuilder.hasNode()) {
        result.append(QSharedPointer<MxpNode>(nodeBuilder.buildNode()));
    }

    return result;
}

MxpTag* TMxpTagParser::parseTag(const QString& tagText) const
{
    return isEndTag(tagText) ? static_cast<MxpTag*>(parseEndTag(tagText)) : static_cast<MxpTag*>(parseStartTag(tagText));
}

MxpEndTag* TMxpTagParser::parseEndTag(const QString& tagText) const
{
    QStringRef tagContent = TStringUtils::trimmedRef(stripTagAndTrim(tagText)).mid(1);
    const QStringList& parts = parseToList(tagContent);

    if (parts.size() > 1) {
        qDebug() << "WARN: end tag " << tagText << " has attributes";
    }

    return new MxpEndTag(parts.first());
}

MxpStartTag* TMxpTagParser::parseStartTag(const QString& tagText) const
{
    QStringRef tagContent = TStringUtils::stripRef(tagText, '<', '>');

    const QStringList& parts = parseToList(tagContent);

    const QString& name = parts.first();

    bool isClosed = parts.back() == "/";
    size_t attrsEndIndex = isClosed ? parts.size() - 1 : parts.size();

    QList<MxpTagAttribute> attrs;
    for (int i = 1; i < attrsEndIndex; i++) {
        attrs.append(parseAttribute(parts[i]));
    }

    return new MxpStartTag(name, attrs, isClosed);
}

MxpTagAttribute TMxpTagParser::parseAttribute(const QString& attr) const
{
    if (isTag(attr)) {
        return MxpTagAttribute(attr, QString());
    }

    int sepIdx = attr.indexOf('=');
    if (sepIdx == -1) {
        return MxpTagAttribute(attr, QString());
    }

    const QString& name = attr.left(sepIdx);
    const QString& value = TStringUtils::unquoteRef(attr.midRef(sepIdx + 1)).toString();

    return MxpTagAttribute(name, value);
}
QStringList TMxpTagParser::parseToList(const QString& tagText)
{
    return parseToList(QStringRef(&tagText));
}

QStringList TMxpTagParser::parseToList(const QStringRef& tagText)
{
    QStringList result;

    int start = 0;

    while (start < tagText.length()) {
        if (TStringUtils::isQuote(tagText[start])) {
            auto end = readTextBlock(tagText, start + 1, tagText.length(), tagText[start]);
            if (start != end) {
                result.append(tagText.mid(start + 1, end - start - 1).toString());
            }

            start = end + 1;
            continue;
        }

        if (!tagText[start].isSpace()) {
            auto end = readTextBlock(tagText, start, tagText.length(), ' ');
            if (start != end) {
                result.append(tagText.mid(start, end - start).toString());
            }

            start = end;
            continue;
        }

        start++;
    }

    return result;
}

int TMxpTagParser::readTextBlock(const QStringRef& str, int start, int end, QChar terminatingChar)
{
    bool inQuote = false;
    QChar curQuote = 0;

    int pos = start;
    while (pos < end) {
        if (!inQuote && str[pos] == terminatingChar) {
            break;
        }

        if (TStringUtils::isQuote(str[pos])) {
            if (!inQuote) {
                inQuote = true;
                curQuote = str[pos];
            } else {
                if (str[pos] == curQuote) {
                    inQuote = false;
                }
            }
        }

        pos++;
    }

    return pos;
}
