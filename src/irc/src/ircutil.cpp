/*
* Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/

#include "../include/ircutil.h"
#include <QString>
#include <QRegExp>

/*!
    \class Irc::Util ircutil.h
    \brief The Irc::Util class provides IRC related utility functions.
 */

static QRegExp URL_PATTERN(QLatin1String("((www\\.(?!\\.)|(ssh|fish|irc|(f|sf|ht)tp(|s))://)(\\.?[\\d\\w/,\\':~\\^\\?=;#@\\-\\+\\%\\*\\{\\}\\!\\(\\)]|&)+)|([-.\\d\\w]+@[-.\\d\\w]{2,}\\.[\\w]{2,})"), Qt::CaseInsensitive);

namespace Irc
{
    /*!
        Parses and returns the nick part from \a target.
    */
    QString Util::nickFromTarget(const QString& target)
    {
        int index = target.indexOf(QLatin1Char('!'));
        return target.left(index);
    }

    /*!
        Parses and returns the host part from \a target.
    */
    QString Util::hostFromTarget(const QString& target)
    {
        int index = target.indexOf(QLatin1Char('!'));
        return target.mid(index + 1);
    }

    /*!
        Converts \a message to HTML. This function parses the
        message and replaces IRC-style formatting like colors,
        bold and underline to the corresponding HTML formatting.
        Furthermore, this function detects URLs and replaces
        them with appropriate HTML hyperlinks.
    */
    QString Util::messageToHtml(const QString& message)
    {
        QString processed = message;
        processed.replace(QLatin1Char('&'), QLatin1String("&amp;"));
        processed.replace(QLatin1Char('<'), QLatin1String("&lt;"));
        processed.replace(QLatin1Char('>'), QLatin1String("&gt;"));
        processed.replace(QLatin1Char('\t'), QLatin1String("&nbsp;"));

        enum
        {
            None            = 0x0,
            Bold            = 0x1,
            Color           = 0x2,
            Italic          = 0x4,
            StrikeThrough   = 0x8,
            Underline       = 0x10,
            Inverse         = 0x20
        };
        int state = None;

        int pos = 0;
        while (pos < processed.size())
        {
            if (state & Color)
            {
                QString tmp = processed.mid(pos, 2);
                processed.remove(pos, 2);
                processed = processed.arg(colorNameFromCode(tmp.toInt()));
                state &= ~Color;
                pos += 2;
                continue;
            }

            QString replacement;
            switch (processed.at(pos).unicode())
            {
            case '\x02': // bold
                if (state & Bold)
                    replacement = QLatin1String("</span>");
                else
                    replacement = QLatin1String("<span style='font-weight: bold'>");
                state ^= Bold;
                break;

            case '\x03': // color
                if (state & Color)
                    replacement = QLatin1String("</span>");
                else
                    replacement = QLatin1String("<span style='color: %1'>");
                state ^= Color;
                break;

            /*case '\x09': // italic
                if (state & Italic)
                    replacement = QLatin1String("</span>");
                else
                    replacement = QLatin1String("<span style='text-decoration: underline'>");
                state ^= Italic;
                break;*/

            case '\x13': // strike-through
                if (state & StrikeThrough)
                    replacement = QLatin1String("</span>");
                else
                    replacement = QLatin1String("<span style='text-decoration: line-through'>");
                state ^= StrikeThrough;
                break;

            case '\x15': // underline
            case '\x1f': // underline
                if (state & Underline)
                    replacement = QLatin1String("</span>");
                else
                    replacement = QLatin1String("<span style='text-decoration: underline'>");
                state ^= Underline;
                break;

            case '\x16': // inverse
                state ^= Inverse;
                break;

            case '\x0f': // none
                replacement = QLatin1String("</span>");
                state = None;
                break;

            default:
                break;
            }

            if (!replacement.isNull())
            {
                processed.replace(pos, 1, replacement);
                pos += replacement.length();
            }
            else
            {
                ++pos;
            }
        }

        pos = 0;
        while ((pos = URL_PATTERN.indexIn(processed, pos)) >= 0)
        {
            int len = URL_PATTERN.matchedLength();
            QString href = processed.mid(pos, len);

            // Don't consider trailing &gt; as part of the link.
            QString append;
            if (href.endsWith(QLatin1String("&gt;")))
            {
                append.append(href.right(4));
                href.chop(4);
            }

            // Don't consider trailing comma or semi-colon as part of the link.
            if (href.endsWith(QLatin1Char(',')) || href.endsWith(QLatin1Char(';')))
            {
                append.append(href.right(1));
                href.chop(1);
            }

            // Don't consider trailing closing parenthesis part of the link when
            // there's an opening parenthesis preceding in the beginning of the
            // URL or there is no opening parenthesis in the URL at all.
            if (pos > 0 && href.endsWith(QLatin1Char(')')) 
                && (processed.at(pos-1) == QLatin1Char('(') 
                || !href.contains(QLatin1Char('('))))
            {
                append.prepend(href.right(1));
                href.chop(1);
            }

            // Qt doesn't support (?<=pattern) so we do it here
            if (pos > 0 && processed.at(pos-1).isLetterOrNumber())
            {
                pos++;
                continue;
            }

            QString protocol;
            if (URL_PATTERN.cap(1).startsWith(QLatin1String("www."), Qt::CaseInsensitive))
                protocol = QLatin1String("http://");
            else if (URL_PATTERN.cap(1).isEmpty())
                protocol = QLatin1String("mailto:");

            QString source = href;
            source.replace(QLatin1String("&amp;"), QLatin1String("&"));

            QString link = QString(QLatin1String("<a href='%1%2'>%3</a>")).arg(protocol, source, href) + append;
            processed.replace(pos, len, link);
            pos += link.length();
        }

        return processed;
    }

    /*!
        Converts \a code to a color name.
    */
    QString Util::colorNameFromCode(int code)
    {
        switch (code)
        {
        case 0:  return QLatin1String("white");
        case 1:  return QLatin1String("black");
        case 2:  return QLatin1String("navy");
        case 3:  return QLatin1String("green");
        case 4:  return QLatin1String("red");
        case 5:  return QLatin1String("maroon");
        case 6:  return QLatin1String("purple");
        case 7:  return QLatin1String("orange");
        case 8:  return QLatin1String("yellow");
        case 9:  return QLatin1String("lime");
        case 10: return QLatin1String("darkcyan");
        case 11: return QLatin1String("cyan");
        case 12: return QLatin1String("blue");
        case 13: return QLatin1String("magenta");
        case 14: return QLatin1String("gray");
        case 15: return QLatin1String("lightgray");
        default: return QLatin1String("black");
        }
    }
}
