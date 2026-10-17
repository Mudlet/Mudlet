/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TLabelModel.h"

#include "Host.h"

#include <QFile>
#include <QImage>
#include <QRegularExpression>
#include <QSvgRenderer>

TLabelModel::TLabelModel(Host* pHost, const QString& name)
: mpHost(pHost)
, mName(name)
{
}

TLabelModel::~TLabelModel()
{
    if (!mpHost) {
        return;
    }

    auto* interpreter = mpHost->getLuaInterpreter();
    for (const int funcRef : {mClickFunction, mDoubleClickFunction, mReleaseFunction, mMoveFunction, mWheelFunction, mEnterFunction, mLeaveFunction}) {
        if (funcRef) {
            interpreter->freeLuaRegistryIndex(funcRef);
        }
    }
}

void TLabelModel::setClick(const int func)
{
    releaseFunc(mClickFunction, func);
    mClickFunction = func;
}

void TLabelModel::setDoubleClick(const int func)
{
    releaseFunc(mDoubleClickFunction, func);
    mDoubleClickFunction = func;
}

void TLabelModel::setRelease(const int func)
{
    releaseFunc(mReleaseFunction, func);
    mReleaseFunction = func;
}

void TLabelModel::setMove(const int func)
{
    releaseFunc(mMoveFunction, func);
    mMoveFunction = func;
}

void TLabelModel::setWheel(const int func)
{
    releaseFunc(mWheelFunction, func);
    mWheelFunction = func;
}

void TLabelModel::setEnter(const int func)
{
    releaseFunc(mEnterFunction, func);
    mEnterFunction = func;
}

void TLabelModel::setLeave(const int func)
{
    releaseFunc(mLeaveFunction, func);
    mLeaveFunction = func;
}

void TLabelModel::releaseFunc(const int existingFunction, const int newFunction)
{
    if (!mpHost) {
        return;
    }

    if (newFunction != existingFunction) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(existingFunction);
    }
}

// the lookbehind keeps selection-background-color and friends out of it
QString TLabelModel::styleSheetWithBackgroundColor(const QString& styleSheet, const QColor& color)
{
    static const QRegularExpression declaration(qsl("(?<![-\\w])background-color\\s*:[^;]*;"));
    const QString newColor = qsl("background-color: rgba(%1, %2, %3, %4);").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    QString sheet = styleSheet;
    if (sheet.contains(declaration)) {
        sheet.replace(declaration, newColor);
    } else {
        if (!sheet.isEmpty() && !sheet.endsWith(QChar::LineFeed)) {
            sheet.append(QChar::LineFeed);
        }
        sheet.append(newColor);
    }
    return sheet;
}

// Index just past the quoted string or comment starting at i, or i when none starts there.
static qsizetype pastQuoteOrComment(const QString& sheet, qsizetype i)
{
    const QChar c = sheet.at(i);
    if (c == u'"' || c == u'\'') {
        qsizetype j = i + 1;
        while (j < sheet.size() && sheet.at(j) != c) {
            j += sheet.at(j) == u'\\' ? 2 : 1;
        }
        return qMin(j + 1, sheet.size());
    }
    if (c == u'/' && i + 1 < sheet.size() && sheet.at(i + 1) == u'*') {
        const qsizetype end = sheet.indexOf(qsl("*/"), i + 2);
        return end < 0 ? sheet.size() : end + 2;
    }
    return i;
}

// The end of the background-image declaration starting at i, or i when none starts there;
// a url() may hold a ';' of its own, as a data: URI does. Qt reads property names in any case.
static qsizetype pastBackgroundImageDeclaration(const QString& sheet, qsizetype i)
{
    static const QString property = qsl("background-image");
    if (!QStringView(sheet).sliced(i).startsWith(property, Qt::CaseInsensitive)) {
        return i;
    }
    qsizetype j = i + property.size();
    while (j < sheet.size() && sheet.at(j).isSpace()) {
        ++j;
    }
    if (j == sheet.size() || sheet.at(j) != u':') {
        return i;
    }
    int parentheses = 0;
    while (++j < sheet.size()) {
        if (const qsizetype skipped = pastQuoteOrComment(sheet, j); skipped != j) {
            j = skipped - 1;
            continue;
        }
        const QChar c = sheet.at(j);
        if (c == u'(') {
            ++parentheses;
        } else if (c == u')' && parentheses > 0) {
            --parentheses;
        } else if (parentheses == 0 && c == u';') {
            return j + 1;
        } else if (parentheses == 0 && c == u'}') {
            return j;
        }
    }
    return j;
}

// Comments, quoted strings and a url()'s contents hold no declarations and no braces, as in CSS.
// A declaration starts the sheet or follows a ';' or a rule's '}', so selector text such as
// QLabel#background-image:hover is not one.
QString TLabelModel::styleSheetWithoutBackgroundImage(const QString& sheet)
{
    QString result;
    qsizetype copiedUpTo = 0;
    int braces = 0;
    int parentheses = 0;
    bool declarationCanStart = true;
    qsizetype i = 0;
    while (i < sheet.size()) {
        if (const qsizetype skipped = pastQuoteOrComment(sheet, i); skipped != i) {
            if (sheet.at(i) != u'/') {
                declarationCanStart = false;
            }
            i = skipped;
            continue;
        }
        const QChar c = sheet.at(i);
        if (c == u'(') {
            ++parentheses;
        } else if (c == u')' && parentheses > 0) {
            --parentheses;
        }
        if (parentheses > 0) {
            ++i;
            continue;
        }
        if (braces == 0 && declarationCanStart) {
            if (const qsizetype end = pastBackgroundImageDeclaration(sheet, i); end != i) {
                result += QStringView(sheet).sliced(copiedUpTo, i - copiedUpTo);
                copiedUpTo = end;
                i = end;
                continue;
            }
        }
        if (c == u'{') {
            ++braces;
        } else if (c == u'}' && braces > 0) {
            --braces;
        }
        if (!c.isSpace()) {
            declarationCanStart = c == u';' || c == u'}';
        }
        ++i;
    }
    result += QStringView(sheet).sliced(copiedUpTo);
    return result;
}

// Not a case-insensitive contains("<a "): that can't tell a tag from prose and case-folds every character.
// Any whitespace separates, as in HTML; setText()'s styling pass knows only ASCII whitespace, so an
// anchor split by a non-breaking space is clickable but unstyled.
bool TLabelModel::containsAnchorTag(const QString& text)
{
    qsizetype close = -1;
    // Later '<'s are nearer the end still, so one too close to it for a tag name and separator ends the walk.
    for (qsizetype at = text.indexOf(QLatin1Char('<')); at >= 0 && at + 2 < text.size(); at = text.indexOf(QLatin1Char('<'), at + 1)) {
        const char16_t tagName = text.at(at + 1).unicode();
        if ((tagName != u'a' && tagName != u'A') || !text.at(at + 2).isSpace()) {
            continue;
        }
        // Only a tag if its closing '>' comes before any other '<'. That also rejects a '<' inside an
        // attribute value, which HTML wants written &lt; anyway.
        if (close < at) {
            // Re-searched only once the walk passes the last '>' found, keeping the scans linear.
            close = text.indexOf(QLatin1Char('>'), at + 3);
            if (close < 0) {
                // no tag anywhere past here can be closed either
                return false;
            }
        }
        const qsizetype nextOpen = text.indexOf(QLatin1Char('<'), at + 3);
        if (nextOpen < 0 || close < nextOpen) {
            return true;
        }
    }
    return false;
}

// If we have link styling configured and the text contains HTML links,
// we need to inject inline styles because QTextDocument doesn't use
// widget stylesheets or QPalette for link colors when a stylesheet exists
QString TLabelModel::linkStyledText(const QString& text) const
{
    if ((mLinkColor.isEmpty() && mLinkVisitedColor.isEmpty()) || !containsAnchorTag(text)) {
        return text;
    }
    QString styledText = text;

    // Replace all <a href="..."> tags with <a href="..." style="...">
    // Note: This regex is intentionally strict (lowercase, href first, no spacing around =)
    // because Mudlet's HTML generation (via echo(), setLabelText(), etc.) consistently
    // uses this format. User-provided HTML outside this pattern will still render as
    // clickable links (Qt handles that), but won't receive custom styling.
    static const QRegularExpression anchorRegex(qsl("<a\\s+href=([\"'][^\"']*[\"'])([^>]*)>"));
    QRegularExpressionMatchIterator it = anchorRegex.globalMatch(styledText);

    // Process matches in reverse order to avoid offset issues
    QList<QRegularExpressionMatch> matches;
    while (it.hasNext()) {
        matches.prepend(it.next());
    }

    for (const auto& match : matches) {
        QString fullMatch = match.captured(0);
        QString hrefPart = match.captured(1);   // The href="..." part
        QString otherAttrs = match.captured(2); // Other attributes

        // Extract the actual URL from hrefPart (remove quotes)
        QString url = hrefPart;
        url.remove(0, 1); // Remove opening quote
        url.chop(1);      // Remove closing quote

        const bool isVisited = mVisitedLinks.contains(url);

        QString linkStyle;
        if (isVisited && !mLinkVisitedColor.isEmpty()) {
            linkStyle += qsl("color: %1; ").arg(mLinkVisitedColor);
        } else if (!mLinkColor.isEmpty()) {
            linkStyle += qsl("color: %1; ").arg(mLinkColor);
        }

        if (!mLinkUnderline) {
            linkStyle += qsl("text-decoration: none; ");
        }

        if (!linkStyle.isEmpty()) {
            linkStyle = linkStyle.trimmed();

            QString replacement;
            if (otherAttrs.contains(qsl("style="))) {
                // Already has a style attribute - merge our styles
                // This is complex, so for now just prepend our styles
                replacement = qsl("<a href=%1 style=\"%2\"").arg(hrefPart, linkStyle);
                // Intentionally overwrites any existing style attribute rather than merging
                // to keep implementation simple for the common case (labels without pre-existing inline styles)
                otherAttrs.remove(QRegularExpression(qsl("style=([\"'][^\"']*[\"'])")));
                replacement += otherAttrs + qsl(">");
            } else {
                replacement = qsl("<a href=%1 style=\"%2\"%3>").arg(hrefPart, linkStyle, otherAttrs);
            }

            styledText.replace(match.capturedStart(), match.capturedLength(), replacement);
        }
    }
    return styledText;
}

// QPixmap and QImage read a file by its content, so a raster saved under a .svg
// name has always displayed. This only asks whether the renderer is worth trying:
// the renderer itself is the authority on what is an SVG.
bool TLabelModel::svgCandidate(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray head = file.read(64);
    if (head.startsWith(QByteArrayLiteral("\x1f\x8b"))) {
        return true;
    }

    qsizetype at = 0;
    bool utf16 = false;
    if (head.startsWith(QByteArrayLiteral("\xef\xbb\xbf"))) {
        at = 3;
    } else if (head.startsWith(QByteArrayLiteral("\xff\xfe")) || head.startsWith(QByteArrayLiteral("\xfe\xff"))) {
        at = 2;
        utf16 = true;
    }
    for (; at < head.size(); ++at) {
        const char byte = head.at(at);
        // in UTF-16 every ASCII character is half of a code unit whose other half
        // is a NUL, whichever way round the byte order mark put them
        if (utf16 && byte == '\0') {
            continue;
        }
        if (byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r' || byte == '\f' || byte == '\v') {
            continue;
        }
        return byte == '<';
    }
    return false;
}

bool TLabelModel::loadSvg(QSvgRenderer& renderer, const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // QSvgRenderer inflates a gzipped document by its content on the QByteArray
    // overload, but only by a .svgz or .svg.gz name on the path one
    if (file.peek(2) == QByteArrayLiteral("\x1f\x8b")) {
        renderer.load(file.readAll());
    } else {
        file.close();
        // the path overload resolves a relative href inside the document against
        // the directory the document sits in
        renderer.load(path);
    }
    return renderer.isValid();
}

std::optional<QSize> TLabelModel::imageSize(const QString& path)
{
    // QImage reads an SVG only where the qsvg image plugin is deployed, so the
    // document's own reader answers first; anything it cannot read - a raster
    // under a .svg name included - falls through to QImage
    if (svgCandidate(path)) {
        QSvgRenderer renderer;
        if (loadSvg(renderer, path) && !renderer.defaultSize().isEmpty()) {
            return renderer.defaultSize();
        }
    }

    const QImage image(path);

    if (image.isNull()) {
        return {};
    }

    return image.size();
}
