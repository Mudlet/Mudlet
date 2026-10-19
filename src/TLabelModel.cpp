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
#include <QTextDocument>
#include <QTextDocumentFragment>

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
    accessibilityChanged(AccessibilityChange::Role);
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
    accessibilityChanged(AccessibilityChange::Role);
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

// The closing quote has to be the one that opened the value: a Lua command link holds the other kind.
static const QRegularExpression& anchorTag()
{
    static const QRegularExpression anchor(qsl("<a\\s+href=(\"[^\"]*\"|'[^']*')([^>]*)>"));
    return anchor;
}

// One attribute with the whitespace before it, if any: Qt also reads one written straight after a quote
static const QRegularExpression& anchorAttribute()
{
    static const QRegularExpression attribute(qsl("\\s*([^\\s=>\"']+)(?:\\s*=\\s*(?:\"[^\"]*\"|'[^']*'|[^\\s>\"']*))?"));
    return attribute;
}

// The attributes are read one after another from the start, so that neither data-style= nor a value
// holding "style=" is taken for a style attribute; reading stops at the first thing that is not one.
static QString withoutStyleAttributes(const QString& attributes)
{
    QString result;
    qsizetype at = 0;
    while (at < attributes.size()) {
        const QRegularExpressionMatch attribute = anchorAttribute().match(attributes, at, QRegularExpression::NormalMatch, QRegularExpression::AnchorAtOffsetMatchOption);
        if (!attribute.hasMatch()) {
            break;
        }
        if (attribute.captured(1).compare(qsl("style"), Qt::CaseInsensitive) != 0) {
            result += attribute.captured(0);
        }
        at = attribute.capturedEnd();
    }
    result += QStringView(attributes).mid(at);
    return result;
}

// Text read back with getLabelText() carries the anchor tags the last pass rewrote; each is put back as
// it was given, so resetLinkStyle() can take the styling away again. Any other tag is the script's own.
QString TLabelModel::withoutInjectedLinkStyles(const QString& text) const
{
    // a pixmap, a movie or a style sheet's qproperty-text has taken the place of what the last pass showed
    if (mStyledAnchors.isEmpty() || mText != mStyledText) {
        return text;
    }

    QString result;
    qsizetype copied = 0;
    QList<bool> restored(mStyledAnchors.size(), false);
    QRegularExpressionMatchIterator it = anchorTag().globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        for (qsizetype i = 0; i < mStyledAnchors.size(); ++i) {
            if (!restored.at(i) && mStyledAnchors.at(i).styled == match.captured(0)) {
                result += QStringView(text).mid(copied, match.capturedStart() - copied);
                result += mStyledAnchors.at(i).original;
                copied = match.capturedEnd();
                restored[i] = true;
                break;
            }
        }
    }
    result += QStringView(text).mid(copied);
    return result;
}

QString TLabelModel::linkStyledText(const QString& text)
{
    return styleLinks(withoutInjectedLinkStyles(text));
}

// Re-styles from mUnstyledText, not mText: that already carries the styling of the last pass.
QString TLabelModel::restyledText()
{
    return styleLinks(mUnstyledText);
}

// If we have link styling configured and the text contains HTML links,
// we need to inject inline styles because QTextDocument doesn't use
// widget stylesheets or QPalette for link colors when a stylesheet exists
QString TLabelModel::styleLinks(const QString& text)
{
    mUnstyledText = text;
    // Only the tags shown now are recognised when read back, and only unchanged: text a script kept from
    // an earlier pass, or an anchor it edited, comes back as its own, as nothing tells it apart from a
    // style the script wrote that way.
    mStyledAnchors.clear();
    mStyledText = text;
    if ((mLinkColor.isEmpty() && mLinkVisitedColor.isEmpty()) || !containsAnchorTag(text)) {
        return text;
    }
    QString styledText = text;

    // Replace all <a href="..."> tags with <a href="..." style="...">
    // Note: This regex is intentionally strict (lowercase, href first, no spacing around =)
    // because Mudlet's HTML generation (via echo(), setLabelText(), etc.) consistently
    // uses this format. User-provided HTML outside this pattern will still render as
    // clickable links (Qt handles that), but won't receive custom styling.
    QRegularExpressionMatchIterator it = anchorTag().globalMatch(styledText);

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
            linkStyle += qsl("color: %1; ").arg(mLinkVisitedColor.toHtmlEscaped());
        } else if (!mLinkColor.isEmpty()) {
            linkStyle += qsl("color: %1; ").arg(mLinkColor.toHtmlEscaped());
        }

        if (!mLinkUnderline) {
            linkStyle += qsl("text-decoration: none; ");
        }

        if (!linkStyle.isEmpty()) {
            linkStyle = linkStyle.trimmed();

            // Intentionally overwrites any existing style attribute rather than merging
            // to keep implementation simple for the common case (labels without pre-existing inline styles)
            const QString replacement = qsl("<a href=%1 style=\"%2\"%3>").arg(hrefPart, linkStyle, withoutStyleAttributes(otherAttrs));

            styledText.replace(match.capturedStart(), match.capturedLength(), replacement);
            mStyledAnchors.prepend({replacement, match.captured(0)});
        }
    }
    mStyledText = styledText;
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

namespace {
QString plainName(const QString& labelText)
{
    // Labels carry markup and entities such as &nbsp;, and an image leaves an object
    // replacement character behind, so the text is rendered rather than stripped
    QString plain = QTextDocumentFragment::fromHtml(labelText).toPlainText();
    plain.remove(QChar::ObjectReplacementCharacter);
    return plain.simplified();
}
} // namespace

QString TLabelModel::effectiveAccessibleName() const
{
    if (!mAccessibleName.isEmpty()) {
        return mAccessibleName;
    }
    if (QString name = plainName(mText); !name.isEmpty()) {
        return name;
    }
    // Qt shows a tooltip that is not rich text literally, so it is read that way too
    return Qt::mightBeRichText(mToolTip) ? plainName(mToolTip) : mToolTip.simplified();
}

QString TLabelModel::spokenText() const
{
    return plainName(mText);
}

bool TLabelModel::namedByToolTip() const
{
    return mAccessibleName.isEmpty() && plainName(mText).isEmpty() && !mToolTip.isEmpty();
}

bool TLabelModel::actsAsButton() const
{
    return !mClickThrough && mAccessibleButton.value_or(mClickFunction != 0 || mReleaseFunction != 0);
}

// Each setter tells the view only of a real change, as a screen reader may say the label again on hearing of one
void TLabelModel::setAccessibleName(const QString& name)
{
    if (mAccessibleName == name) {
        return;
    }
    mAccessibleName = name;
    accessibilityChanged(AccessibilityChange::Name);
}

void TLabelModel::setAccessibleButton(std::optional<bool> button)
{
    if (mAccessibleButton == button) {
        return;
    }
    mAccessibleButton = button;
    accessibilityChanged(AccessibilityChange::Role);
}

void TLabelModel::setClickThrough(bool clickThrough)
{
    if (mClickThrough == clickThrough) {
        return;
    }
    mClickThrough = clickThrough;
    accessibilityChanged(AccessibilityChange::Role);
}

void TLabelModel::setText(const QString& text)
{
    if (mText == text) {
        return;
    }
    mText = text;
    if (mAccessibleName.isEmpty()) {
        accessibilityChanged(AccessibilityChange::Name);
    }
}

void TLabelModel::setToolTip(const QString& toolTip)
{
    if (mToolTip == toolTip) {
        return;
    }
    mToolTip = toolTip;
    if (mAccessibleName.isEmpty()) {
        accessibilityChanged(AccessibilityChange::Name);
    }
}

void TLabelModel::accessibilityChanged(AccessibilityChange change) const
{
    if (mAccessibilityChanged) {
        mAccessibilityChanged(change);
    }
}
