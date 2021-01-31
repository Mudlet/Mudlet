/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"

static QStringView stripTagAndTrim(const QString& tagText)
{
    return TStringUtils::strip(QStringView(tagText).trimmed(), '<', '>').trimmed();
}

static bool isEndTag(const QString& tagText)
{
    return stripTagAndTrim(tagText).startsWith('/');
}

static bool isTag(const QString& tagString)
{
    return TStringUtils::isBetween(QStringView(tagString).trimmed(), '<', '>');
}

QList<QSharedPointer<MxpNode>> TMxpTagParser::parseToMxpNodeList(const QString& tagText, bool ignoreText) const
{
    TMxpNodeBuilder nodeBuilder(ignoreText);
    std::string tagStdStr = tagText.toStdString();

    QList<QSharedPointer<MxpNode>> result;
    for (int i = 0; i < tagText.length(); ++i) {
        if (nodeBuilder.accept(tagStdStr[i])) {
            result.append(QSharedPointer<MxpNode>(nodeBuilder.buildNode()));
            --i;
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
    QStringView tagContent = stripTagAndTrim(tagText).mid(1);
    const QStringList& parts = parseToList(tagContent);

    if (parts.size() > 1) {
        qWarning().noquote().nospace() << "TMxpTagParser::parseEndTag(\"" << tagText << "\") WARNING - this end tag has attributes.";
    }

    return new MxpEndTag(parts.first());
}

MxpStartTag* TMxpTagParser::parseStartTag(const QString& tagText) const
{
    QStringView tagContent = TStringUtils::strip(tagText, '<', '>');

    const QStringList& parts = parseToList(tagContent);

    const QString& name = parts.first();

    bool isClosed = parts.back() == "/";
    int attrsEndIndex = isClosed ? parts.size() - 1 : parts.size();

    QList<MxpTagAttribute> attrs;
    for (int i = 1; i < attrsEndIndex; ++i) {
        attrs.append(parseAttribute(parts.at(i)));
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
    const QString& value = TStringUtils::unquote(QStringView(attr).mid(sepIdx + 1)).toString();

    return MxpTagAttribute(name, value);
}

QStringList TMxpTagParser::parseToList(const QString& tagText)
{
    return parseToList(QStringView(tagText));
}

QStringList TMxpTagParser::parseToList(const QStringView tagText)
{
    QStringList result;

    int start = 0;

    while (start < tagText.length()) {
        if (TStringUtils::isQuote(tagText.at(start))) {
            auto end = readTextBlock(tagText, start + 1, tagText.length(), tagText.at(start));
            if (start != end) {
                result.append(tagText.mid(start + 1, end - start - 1).toString());
            }

            start = end + 1;
            continue;
        }

        if (!tagText.at(start).isSpace()) {
            auto end = readTextBlock(tagText, start, tagText.length(), ' ');
            if (start != end) {
                result.append(tagText.mid(start, end - start).toString());
            }

            start = end;
            continue;
        }

        ++start;
    }

    return result;
}

int TMxpTagParser::readTextBlock(QStringView str, const int start, const int end, const QChar terminatingChar)
{
    bool inQuote = false;
    QChar curQuote = QChar::Null;

    int pos = start;
    while (pos < end) {
        if (!inQuote && str.at(pos) == terminatingChar) {
            break;
        }

        if (TStringUtils::isQuote(str.at(pos))) {
            if (!inQuote) {
                inQuote = true;
                curQuote = str.at(pos);
            } else {
                if (str.at(pos) == curQuote) {
                    inQuote = false;
                }
            }
        }

        ++pos;
    }

    return pos;
}
