/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
  Parts of this code come from Konversation and are copyrighted to:
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
  Copyright (C) 2004-2009 Eli Mackenzie <argonel@gmail.com>
*/

#include "irctextformat.h"
#include "ircpalette.h"
#if QT_VERSION >= 0x050000
#include <QRegularExpression>
#endif
#include <QStringList>
#include <QRegExp>
#include <QUrl>
#include "irc.h"

IRC_BEGIN_NAMESPACE

/*!
    \file irctextformat.h
    \brief \#include &lt;IrcTextFormat&gt;
 */

/*!
    \class IrcTextFormat irctextformat.h <IrcTextFormat>
    \ingroup util
    \brief Provides methods for text formatting.

    IrcTextFormat is used to convert IRC-style formatted messages to either
    plain text or HTML. When converting to plain text, the IRC-style formatting
    (colors, bold, underline etc.) are simply stripped away. When converting
    to HTML, the IRC-style formatting is converted to the corresponding HTML
    formatting.

    \code
    IrcTextFormat format;
    QString text = format.toPlainText(message);

    format.palette()->setColorName(Irc::Red, "#ff3333");
    format.palette()->setColorName(Irc::Green, "#33ff33");
    format.palette()->setColorName(Irc::Blue, "#3333ff");
    // ...
    QString html = format.toHtml(message);
    \endcode

    \sa IrcPalette
 */

/*!
    \enum IrcTextFormat::SpanFormat
    This enum describes the supported formats for HTML span-elements.
 */

/*!
    \var IrcTextFormat::SpanStyle
    \brief HTML span-elements with style-attributes.
 */

/*!
    \var IrcTextFormat::SpanClass
    \brief HTML span-elements with class-attributes.
 */

class IrcTextFormatPrivate
{
public:
    void parse(const QString& str, QString* text, QString* html, QList<QUrl>* urls) const;

    QString plainText;
    QString html;
    QList<QUrl> urls;
    QString urlPattern;
    IrcPalette* palette;
    IrcTextFormat::SpanFormat spanFormat;
};

static bool parseColors(const QString& message, int pos, int* len, int* fg = 0, int* bg = 0)
{
    // fg(,bg)
    *len = 0;
    if (fg)
        *fg = -1;
    if (bg)
        *bg = -1;
    QRegExp rx(QLatin1String("(\\d{1,2})(?:,(\\d{1,2}))?"));
    int idx = rx.indexIn(message, pos);
    if (idx == pos) {
        *len = rx.matchedLength();
        if (fg)
            *fg = rx.cap(1).toInt();
        if (bg) {
            bool ok = false;
            int tmp = rx.cap(2).toInt(&ok);
            if (ok)
                *bg = tmp;
        }
    }
    return *len > 0;
}

static QString generateLink(const QString& protocol, const QString& href)
{
    const char* exclude = ":/?@%#=+&,";
    const QByteArray url = QUrl::toPercentEncoding(href, exclude);
    return QString(QLatin1String("<a href='%1%2'>%3</a>")).arg(protocol, url, href);
}

static QString parseLinks(const QString& message, const QString& pattern, QList<QUrl>* urls)
{
    QString processed = message;
#if QT_VERSION >= 0x050000
    int offset = 0;
    QRegularExpression rx(pattern);
    QRegularExpressionMatchIterator it = rx.globalMatch(message);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString protocol;
        if (match.capturedRef(2).isEmpty()) {
            QStringRef link = match.capturedRef(1);
            if (link.startsWith(QStringLiteral("ftp."), Qt::CaseInsensitive))
                protocol = QStringLiteral("ftp://");
            else if (link.contains(QStringLiteral("@")))
                protocol = QStringLiteral("mailto:");
            else
                protocol = QStringLiteral("http://");
        }

        const int start = match.capturedStart();
        const int len = match.capturedEnd() - start;
        const QString href = match.captured();
        const QString link = generateLink(protocol, href);
        processed.replace(start + offset, len, link);
        offset += link.length() - len;
        if (urls)
            urls->append(QUrl(protocol + href));
    }
#else
    int pos = 0;
    QRegExp rx(pattern);
    while ((pos = rx.indexIn(processed, pos)) >= 0) {
        int len = rx.matchedLength();
        QString href = processed.mid(pos, len);

        QString protocol;
        if (rx.cap(2).isEmpty()) {
            if (rx.cap(1).contains(QLatin1Char('@')))
                protocol = QLatin1String("mailto:");
            else if (rx.cap(1).startsWith(QLatin1String("ftp."), Qt::CaseInsensitive))
                protocol = QLatin1String("ftp://");
            else
                protocol = QLatin1String("http://");
        }

        QString link = generateLink(protocol, href);
        processed.replace(pos, len, link);
        pos += link.length();
        if (urls)
            urls->append(QUrl(protocol + href));
    }
#endif
    return processed;
}

void IrcTextFormatPrivate::parse(const QString& str, QString* text, QString* html, QList<QUrl>* urls) const
{
    QString processed = str;

    // TODO:
    //processed.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    processed.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    //processed.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    //processed.replace(QLatin1Char('"'), QLatin1String("&quot;"));
    //processed.replace(QLatin1Char('\''), QLatin1String("&apos;"));
    //processed.replace(QLatin1Char('\t'), QLatin1String("&nbsp;"));

    enum {
        None            = 0x0,
        Bold            = 0x1,
        Italic          = 0x4,
        LineThrough     = 0x8,
        Underline       = 0x10,
        Inverse         = 0x20
    };
    int state = None;

    int pos = 0;
    int len = 0;
    int fg = -1;
    int bg = -1;
    int depth = 0;
    bool potentialUrl = false;
    while (pos < processed.size()) {
        QString replacement;
        switch (processed.at(pos).unicode()) {
            case '\x02': // bold
                if (state & Bold) {
                    depth--;
                    replacement = QLatin1String("</span>");
                } else {
                    depth++;
                    if (spanFormat == IrcTextFormat::SpanStyle)
                        replacement = QLatin1String("<span style='font-weight: bold'>");
                    else
                        replacement = QLatin1String("<span class='bold'>");
                }
                state ^= Bold;
                break;

            case '\x03': // color
                if (parseColors(processed, pos + 1, &len, &fg, &bg)) {
                    depth++;
                    if (spanFormat == IrcTextFormat::SpanStyle) {
                        QStringList styles;
                        styles += QString(QLatin1String("color: %1")).arg(palette->colorName(fg, QLatin1String("black")));
                        if (bg != -1)
                            styles += QString(QLatin1String("background-color: %1")).arg(palette->colorName(bg, QLatin1String("transparent")));
                        replacement = QString(QLatin1String("<span style='%1'>")).arg(styles.join(QLatin1String("; ")));
                    } else {
                        QStringList classes;
                        classes += palette->colorName(fg, QLatin1String("black"));
                        if (bg != -1)
                            classes += palette->colorName(bg, QLatin1String("transparent")) + QLatin1String("-background");
                        replacement = QString(QLatin1String("<span class='%1'>")).arg(classes.join(QLatin1String(" ")));
                    }
                    // \x03FF(,BB)
                    processed.replace(pos, len + 1, replacement);
                    pos += replacement.length();
                    continue;
                } else {
                    depth--;
                    replacement = QLatin1String("</span>");
                }
                break;

                //case '\x09': // italic
            case '\x1d': // italic
                if (state & Italic) {
                    depth--;
                    replacement = QLatin1String("</span>");
                } else {
                    depth++;
                    if (spanFormat == IrcTextFormat::SpanStyle)
                        replacement = QLatin1String("<span style='font-style: italic'>");
                    else
                        replacement = QLatin1String("<span class='italic'>");
                }
                state ^= Italic;
                break;

            case '\x13': // line-through
                if (state & LineThrough) {
                    depth--;
                    replacement = QLatin1String("</span>");
                } else {
                    depth++;
                    if (spanFormat == IrcTextFormat::SpanStyle)
                        replacement = QLatin1String("<span style='text-decoration: line-through'>");
                    else
                        replacement = QLatin1String("<span class='line-through'>");
                }
                state ^= LineThrough;
                break;

            case '\x15': // underline
            case '\x1f': // underline
                if (state & Underline) {
                    depth--;
                    replacement = QLatin1String("</span>");
                } else {
                    depth++;
                    if (spanFormat == IrcTextFormat::SpanStyle)
                        replacement = QLatin1String("<span style='text-decoration: underline'>");
                    else
                        replacement = QLatin1String("<span class='underline'>");
                }
                state ^= Underline;
                break;

            case '\x16': // inverse
                if (state & Inverse) {
                    depth--;
                    replacement = QLatin1String("</span>");
                } else {
                    depth++;
                    if (spanFormat == IrcTextFormat::SpanStyle)
                        replacement = QLatin1String("<span style='text-decoration: inverse'>");
                    else
                        replacement = QLatin1String("<span class='inverse'>");
                }
                state ^= Inverse;
                break;

            case '\x0f': // none
                if (depth > 0)
                    replacement = QString(QLatin1String("</span>")).repeated(depth);
                else
                    processed.remove(pos--, 1); // must rewind back for ++pos below...
                state = None;
                depth = 0;
                break;

            case '.':
            case '/':
            case ':':
                // a dot, slash or colon NOT surrounded by a space indicates a potential URL
                if (!potentialUrl && pos > 0 && !processed.at(pos - 1).isSpace()
                        && pos < processed.length() - 1 && !processed.at(pos + 1).isSpace())
                    potentialUrl = true;
                // flow through
            default:
                if (text)
                    *text += processed.at(pos);
                break;
        }

        if (!replacement.isEmpty()) {
            processed.replace(pos, 1, replacement);
            pos += replacement.length();
        } else {
            ++pos;
        }
    }

    if ((html || urls) && potentialUrl && !urlPattern.isEmpty())
        processed = parseLinks(processed, urlPattern, urls);
    if (html)
        *html = processed;
}

/*!
    Constructs a new text format with \a parent.
 */
IrcTextFormat::IrcTextFormat(QObject* parent) : QObject(parent), d_ptr(new IrcTextFormatPrivate)
{
    Q_D(IrcTextFormat);
    d->palette = new IrcPalette(this);
    d->urlPattern = QString("\\b((?:(?:([a-z][\\w\\.-]+:/{1,3})|www|ftp\\d{0,3}[.]|[a-z0-9.\\-]+[.][a-z]{2,4}/)(?:[^\\s()<>]+|\\(([^\\s()<>]+|(\\([^\\s()<>]+\\)))*\\))+(?:\\(([^\\s()<>]+|(\\([^\\s()<>]+\\)))*\\)|\\}\\]|[^\\s`!()\\[\\]{};:'\".,<>?%1%2%3%4%5%6])|[a-z0-9.\\-+_]+@[a-z0-9.\\-]+[.][a-z]{1,5}[^\\s/`!()\\[\\]{};:'\".,<>?%1%2%3%4%5%6]))").arg(QChar(0x00AB)).arg(QChar(0x00BB)).arg(QChar(0x201C)).arg(QChar(0x201D)).arg(QChar(0x2018)).arg(QChar(0x2019));
    d->spanFormat = SpanStyle;
}

/*!
    Destructs the text format.
 */
IrcTextFormat::~IrcTextFormat()
{
}

/*!
    This property holds the palette used for color formatting.

    \par Access function:
    \li \ref IrcPalette* <b>palette</b>() const
 */
IrcPalette* IrcTextFormat::palette() const
{
    Q_D(const IrcTextFormat);
    return d->palette;
}

/*!
    This property holds the regular expression pattern used for matching URLs.

    \par Access functions:
    \li QString <b>urlPattern</b>() const
    \li void <b>setUrlPattern</b>(const QString& pattern)
 */
QString IrcTextFormat::urlPattern() const
{
    Q_D(const IrcTextFormat);
    return d->urlPattern;
}

void IrcTextFormat::setUrlPattern(const QString& pattern)
{
    Q_D(IrcTextFormat);
    d->urlPattern = pattern;
}

/*!
    \since 3.1

    This property holds the format used for HTML span-elements.

    IrcTextFormat uses HTML span-elements for converting the IRC-style text
    formatting to the corresponding HTML formatting. The \ref SpanStyle format
    generates self contained span-elements with style-attributes, resulting to
    HTML that is ready to be used with Qt's rich text classes without additional
    styling. For more flexible styling, the \ref SpanClass generates span-elements
    with class-attributes that can be styled with additional style sheets.

    The default value is \ref SpanStyle. The following table illustrates the
    difference between \ref SpanStyle and \ref SpanClass HTML formatting:

    IRC format                              | SpanStyle                                                             | SpanClass
    --------------------------------------- | ----------------------------------------------------------------------|----------
    Bold ("\02...\0F")                      | &lt;span style='font-weight: bold'&gt;...&lt;/span&gt;                | &lt;span class='bold'&gt;...&lt;/span&gt;
    Color ("\03fg...\0F")                   | &lt;span style='color: fg;'&gt;...&lt;/span&gt;                       | &lt;span class='fg'&gt;...&lt;/span&gt;
    Background ("\03fgbg...\0F")            | &lt;span style='color: fg; background-color: bg'&gt;...&lt;/span&gt;  | &lt;span class='fg bg-background'&gt;...&lt;/span&gt;
    Italic ("\09...\0F")                    | &lt;span style='font-style: italic'&gt;...&lt;/span&gt;               | &lt;span class='italic'&gt;...&lt;/span&gt;
    Line-through ("\13...\0F")              | &lt;span style='text-decoration: line-through'&gt;...&lt;/span&gt;    | &lt;span class='line-through'&gt;...&lt;/span&gt;
    Underline ("\15...\0F" or "\1F...\0F")  | &lt;span style='text-decoration: underline'&gt;...&lt;/span&gt;       | &lt;span class='underline'&gt;...&lt;/span&gt;
    Inverse ("\16...\0F")                   | &lt;span style='text-decoration: inverse'&gt;...&lt;/span&gt;         | &lt;span class='inverse'&gt;...&lt;/span&gt;

    \par Access functions:
    \li \ref SpanFormat <b>spanFormat</b>() const
    \li void <b>setSpanFormat</b>(\ref SpanFormat format)
 */
IrcTextFormat::SpanFormat IrcTextFormat::spanFormat() const
{
    Q_D(const IrcTextFormat);
    return d->spanFormat;
}

void IrcTextFormat::setSpanFormat(IrcTextFormat::SpanFormat format)
{
    Q_D(IrcTextFormat);
    d->spanFormat = format;
}


/*!
    Converts \a text to HTML. This function parses the text and replaces
    IRC-style formatting (colors, bold, underline etc.) to the corresponding
    HTML formatting. Furthermore, this function detects URLs and replaces
    them with appropriate HTML hyperlinks.

    \note URL detection can be disabled by setting an empty
          regular expression pattern used for matching URLs.

    \sa toPlainText(), parse(), palette, urlPattern, spanFormat
*/
QString IrcTextFormat::toHtml(const QString& text) const
{
    Q_D(const IrcTextFormat);
    QString html;
    d->parse(text, 0, &html, 0);
    return html;
}

/*!
    Converts \a text to plain text. This function parses the text and
    strips away IRC-style formatting (colors, bold, underline etc.)

    \sa toHtml(), parse()
*/
QString IrcTextFormat::toPlainText(const QString& text) const
{
    Q_D(const IrcTextFormat);
    QString plain;
    d->parse(text, &plain, 0, 0);
    return plain;
}

/*!
    \since 3.2

    This property holds the current plain text content.

    \par Access function:
    \li QString <b>plainText</b>() const

    \sa parse(), html, urls
 */
QString IrcTextFormat::plainText() const
{
    Q_D(const IrcTextFormat);
    return d->plainText;
}

/*!
    \since 3.2

    This property holds the current HTML content.

    \par Access function:
    \li QString <b>html</b>() const

    \sa parse(), plainText, urls
 */
QString IrcTextFormat::html() const
{
    Q_D(const IrcTextFormat);
    return d->html;
}

/*!
    \since 3.2

    This property holds the current list of URLs.

    \par Access function:
    \li QList<QUrl> <b>urls</b>() const

    \sa parse(), plainText, html
 */
QList<QUrl> IrcTextFormat::urls() const
{
    Q_D(const IrcTextFormat);
    return d->urls;
}

/*!
    \since 3.2

    Parses \a text converting it to plain text and HTML and detects URLs.

    \sa plainText, html, urls
 */
void IrcTextFormat::parse(const QString& text)
{
    Q_D(IrcTextFormat);
    d->plainText.clear();
    d->html.clear();
    d->urls.clear();
    d->parse(text, &d->plainText, &d->html, &d->urls);
}

#include "moc_irctextformat.cpp"

IRC_END_NAMESPACE
