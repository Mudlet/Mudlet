/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "EditorLineMarginDelegate.h"

#include "uiDesign.h"
#include "utils.h"

#include "edbee/models/textdocument.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/texttheme.h"

#include <QFontMetrics>
#include <QPainter>
#include <algorithm>

namespace uiDesign {

// edbee's own room before the widest number, which the mark is put in front of
static constexpr int scmEdbeeRoomBeforeNumber = 5;
// edbee's LineNumberRightPadding and MarginPaddingRight, file-local to textmargincomponent.cpp: the number is
// redrawn in the box edbee draws it in
static constexpr int scmEdbeeRoomAfterNumber = 10 + 5;
// edbee draws a number this far down its line
static constexpr int scmEdbeeNumberDrop = 2;
static constexpr int scmMarkGap = 3;
// Of the code font's height: the glyph's circle then stands about as tall as the digits beside it
static constexpr qreal scmMarkScale = 0.9;

void EditorLineMarginDelegate::setErrorLine(const int line)
{
    const int wanted = std::max(0, line);
    if (wanted == mErrorLine) {
        return;
    }
    mErrorLine = wanted;
    if (marginComponent()) {
        marginComponent()->update();
    }
}

size_t EditorLineMarginDelegate::markedLine()
{
    edbee::TextMarginComponent* pMargin = marginComponent();
    if (mErrorLine <= 0 || !pMargin) {
        return std::string::npos;
    }
    const size_t lineCount = pMargin->renderer()->textDocument()->lineCount();
    if (lineCount == 0) {
        return std::string::npos;
    }
    return std::min(static_cast<size_t>(mErrorLine - 1), lineCount - 1);
}

// The code pane is on the edbee theme's background, not the window's page, so the ink is walked against that
QColor EditorLineMarginDelegate::errorInk()
{
    edbee::TextMarginComponent* pMargin = marginComponent();
    if (!pMargin) {
        return {};
    }
    edbee::TextTheme* pTheme = pMargin->renderer()->theme();
    const QColor background = pTheme->backgroundColor();
    const bool darkBackground = background.lightness() < 128;
    return readableOn(background, stateColor(scmStateHue_error, darkBackground), pTheme->foregroundColor(), scmTextMinimumRatio);
}

// The marked number is left out here and drawn in renderAfter(), as edbee draws every number in one ink
QString EditorLineMarginDelegate::lineText(const size_t line)
{
    if (line == markedLine()) {
        return QString();
    }
    return TextMarginComponentDelegate::lineText(line);
}

// From the editor's font, not the painter's: edbee asks for the width outside a paint. Its margin font is the
// family and size of that same font.
int EditorLineMarginDelegate::markSize()
{
    edbee::TextMarginComponent* pMargin = marginComponent();
    if (!pMargin) {
        return 0;
    }
    return qRound(QFontMetrics(pMargin->editorWidget()->config()->font()).height() * scmMarkScale);
}

QPixmap EditorLineMarginDelegate::markGlyph(const QColor& ink, const int size, const qreal ratio)
{
    const QString key = qsl("%1/%2/%3").arg(ink.name(QColor::HexArgb), QString::number(size), QString::number(ratio));
    if (key != mMarkGlyphKey) {
        mMarkGlyph = tintedGlyph(glyphPixmap(qsl(":/icons/editor-errors.svg")), ink).scaled(QSize(size, size) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        mMarkGlyph.setDevicePixelRatio(ratio);
        mMarkGlyphKey = key;
    }
    return mMarkGlyph;
}

int EditorLineMarginDelegate::widthBeforeLineNumber()
{
    return scmEdbeeRoomBeforeNumber + markSize() + scmMarkGap;
}

void EditorLineMarginDelegate::renderAfter(QPainter* painter, const size_t startLine, const size_t endLine, const int width)
{
    const size_t line = markedLine();
    if (line == std::string::npos || line < startLine || line > endLine) {
        return;
    }
    edbee::TextRenderer* pRenderer = marginComponent()->renderer();
    const int lineHeight = pRenderer->lineHeight();
    const int top = pRenderer->yPosForLine(line);
    const QColor ink = errorInk();

    const QString number = TextMarginComponentDelegate::lineText(line);
    const int numberLeft = widthBeforeLineNumber();
    const int numberRight = width - scmEdbeeRoomAfterNumber;
    // The painter still carries the margin's font: edbee set it for the numbers just before this.
    // Numbers are right-aligned, so the mark follows this one's own start rather than the column's.
    const int numberStart = numberRight - painter->fontMetrics().horizontalAdvance(number);
    const int size = markSize();
    mMarkRect = QRectF(numberStart - scmMarkGap - size, top + scmEdbeeNumberDrop + (lineHeight - size) / 2.0, size, size);

    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawPixmap(mMarkRect.topLeft(), markGlyph(ink, size, marginComponent()->devicePixelRatioF()));
    painter->setPen(ink);
    painter->drawText(numberLeft, top + scmEdbeeNumberDrop, numberRight - numberLeft, lineHeight, Qt::AlignRight, number);
    painter->restore();
}

} // namespace uiDesign
