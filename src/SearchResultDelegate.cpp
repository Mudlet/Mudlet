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

#include "SearchResultDelegate.h"

#include "EditorCommand.h"
#include "uiDesign.h"
#include "utils.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTreeWidget>

#include <algorithm>

namespace uiDesign {

namespace {
constexpr int scmGlyphSize = 16;
constexpr int scmGlyphGap = 8;
constexpr int scmRowPaddingH = 4;
constexpr int scmItemRowPaddingV = 5;
constexpr int scmMatchRowPaddingV = 3;
constexpr int scmChipGap = 8;
constexpr int scmChipPaddingH = 6;
constexpr int scmChipPaddingV = 1;
constexpr int scmMarkerRadius = 3;
// So the marked query does not read as ending flush against the next letter
constexpr int scmMarkerBleed = 1;
// A fixed column, so the short "where" labels on screen line up
constexpr int scmWhereColumnChars = 11;
constexpr int scmWhereGap = 8;
constexpr int scmSnippetLeadContext = 12;
constexpr qreal scmItemFontScale = 1.05;
constexpr qreal scmChipFontScale = 0.82;
// The marker is light on a light page and dark on a dark one, so its text leans
// the other way
constexpr qreal scmMarkerTextStrength = 0.9;

QString glyphFileFor(const EditorViewType viewType)
{
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        return qsl(":/icons/editor-triggers.svg");
    case EditorViewType::cmAliasView:
        return qsl(":/icons/editor-aliases.svg");
    case EditorViewType::cmScriptView:
        return qsl(":/icons/editor-scripts.svg");
    case EditorViewType::cmTimerView:
        return qsl(":/icons/editor-timers.svg");
    case EditorViewType::cmKeysView:
        return qsl(":/icons/editor-keys.svg");
    case EditorViewType::cmActionView:
        return qsl(":/icons/editor-buttons.svg");
    case EditorViewType::cmVarsView:
        return qsl(":/icons/editor-variables.svg");
    default:
        return {};
    }
}
} // namespace

SearchResultDelegate::SearchResultDelegate(QTreeWidget* pTree)
: QStyledItemDelegate(pTree)
, mpTree(pTree)
{
    restyle();
}

void SearchResultDelegate::restyle()
{
    const ThemeTokens tokens = themeTokens();
    // The editor's chrome has one ink; weight and size set the name apart
    mTitleColor = tokens.mutedText;
    mMutedColor = tokens.mutedText;
    mChipBorder = tokens.border;
    mSelectedText = tokens.accentText;
    mMarkerColor = tokens.marker;
    mMarkerText = blend(mMarkerColor, tokens.darkPage ? QColor(Qt::white) : QColor(Qt::black), scmMarkerTextStrength);

    // Cached here: every row asks for its height, which only moves with the tree's font
    const QFont baseFont = mpTree ? mpTree->font() : QApplication::font();
    mNameFont = scaledFont(baseFont, scmItemFontScale);
    mNameFont.setBold(true);
    mChipFont = scaledFont(baseFont, scmChipFontScale);
    mItemRowHeight = std::max(QFontMetrics(mNameFont).height(), scmGlyphSize) + 2 * scmItemRowPaddingV;
    mMatchRowHeight = QFontMetrics(baseFont).height() + 2 * scmMatchRowPaddingV;

    mTypeGlyphs.clear();
}

QFont SearchResultDelegate::scaledFont(const QFont& base, const qreal factor)
{
    QFont scaled(base);
    // Whether a font carries a point or a pixel size is up to the platform
    if (base.pointSizeF() > 0.0) {
        scaled.setPointSizeF(base.pointSizeF() * factor);
    } else if (base.pixelSize() > 0) {
        scaled.setPixelSize(std::max(1, qRound(base.pixelSize() * factor)));
    }
    return scaled;
}

QPixmap SearchResultDelegate::typeGlyph(const EditorViewType viewType, const bool selected) const
{
    const qreal ratio = mpTree ? mpTree->devicePixelRatioF() : 1.0;
    if (!qFuzzyCompare(ratio + 1.0, mTypeGlyphRatio + 1.0)) {
        mTypeGlyphRatio = ratio;
        mTypeGlyphs.clear();
    }

    const int key = static_cast<int>(viewType) * 2 + (selected ? 1 : 0);
    if (const auto cached = mTypeGlyphs.constFind(key); cached != mTypeGlyphs.constEnd()) {
        return cached.value();
    }

    const QString glyphFile = glyphFileFor(viewType);
    QPixmap glyph;
    if (!glyphFile.isEmpty()) {
        glyph = tintedGlyph(uiDesign::glyphPixmap(glyphFile), selected ? mSelectedText : mMutedColor).scaled(QSize(scmGlyphSize, scmGlyphSize) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        glyph.setDevicePixelRatio(ratio);
    }
    mTypeGlyphs.insert(key, glyph);
    return glyph;
}

void SearchResultDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem viewOption = option;
    initStyleOption(&viewOption, index);

    // The stylesheet draws only the pill; everything inside it is painted here
    QStyleOptionViewItem backgroundOption = viewOption;
    backgroundOption.text.clear();
    backgroundOption.icon = QIcon();
    backgroundOption.features &= ~QStyleOptionViewItem::HasDecoration;
    const QWidget* pWidget = viewOption.widget;
    QStyle* pStyle = pWidget ? pWidget->style() : QApplication::style();
    pStyle->drawControl(QStyle::CE_ItemViewItem, &backgroundOption, pPainter, pWidget);

    const QRect contentRect = viewOption.rect.adjusted(scmRowPaddingH, 0, -scmRowPaddingH, 0);
    if (contentRect.width() <= 0) {
        return;
    }

    pPainter->save();
    switch (index.data(RowKindRole).toInt()) {
    case MatchRow:
        paintMatchRow(pPainter, viewOption, index, contentRect);
        break;
    case NoticeRow:
        paintNoticeRow(pPainter, viewOption, index, contentRect);
        break;
    default:
        paintItemRow(pPainter, viewOption, index, contentRect);
    }
    pPainter->restore();
}

void SearchResultDelegate::paintItemRow(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& contentRect) const
{
    const bool selected = option.state & QStyle::State_Selected;
    QRect rect = contentRect;

    const QPixmap glyph = typeGlyph(static_cast<EditorViewType>(index.data(ViewTypeRole).toInt()), selected);
    if (!glyph.isNull()) {
        const QRect glyphRect(rect.left(), rect.top() + (rect.height() - scmGlyphSize) / 2, scmGlyphSize, scmGlyphSize);
        pPainter->drawPixmap(glyphRect, glyph);
        rect.setLeft(glyphRect.right() + 1 + scmGlyphGap);
    }

    // Chip first, so a long name is elided before it rather than run under it
    const QString typeLabel = index.data(TypeLabelRole).toString();
    QRect chipRect;
    if (!typeLabel.isEmpty()) {
        const QFontMetrics chipMetrics(mChipFont);
        const int chipWidth = chipMetrics.horizontalAdvance(typeLabel) + 2 * scmChipPaddingH;
        const int chipHeight = chipMetrics.height() + 2 * scmChipPaddingV;
        if (chipWidth + scmChipGap < rect.width()) {
            chipRect = QRect(rect.right() - chipWidth + 1, rect.top() + (rect.height() - chipHeight) / 2, chipWidth, chipHeight);
            rect.setRight(chipRect.left() - 1 - scmChipGap);
        }
    }

    const QFontMetrics nameMetrics(mNameFont);
    pPainter->setFont(mNameFont);
    pPainter->setPen(selected ? mSelectedText : mTitleColor);
    pPainter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, nameMetrics.elidedText(index.data(TitleRole).toString(), Qt::ElideRight, rect.width()));

    if (chipRect.isNull()) {
        return;
    }
    pPainter->setRenderHint(QPainter::Antialiasing, true);
    pPainter->setPen(QPen(mChipBorder, 1));
    pPainter->setBrush(Qt::NoBrush);
    pPainter->drawRoundedRect(QRectF(chipRect).adjusted(0.5, 0.5, -0.5, -0.5), scmRadiusChip, scmRadiusChip);
    pPainter->setFont(mChipFont);
    pPainter->setPen(mMutedColor);
    pPainter->drawText(chipRect, Qt::AlignCenter, typeLabel);
}

void SearchResultDelegate::paintMatchRow(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& contentRect) const
{
    const bool selected = option.state & QStyle::State_Selected;
    const QFontMetrics metrics(option.font);
    pPainter->setFont(option.font);
    QRect rect = contentRect;

    const QString where = index.data(WhereRole).toString();
    if (!where.isEmpty()) {
        const int nominalWidth = metrics.averageCharWidth() * scmWhereColumnChars;
        const int whereWidth = std::min(std::max(nominalWidth, metrics.horizontalAdvance(where)), rect.width() / 2);
        pPainter->setPen(mMutedColor);
        pPainter->drawText(QRect(rect.left(), rect.top(), whereWidth, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, metrics.elidedText(where, Qt::ElideRight, whereWidth));
        rect.setLeft(rect.left() + whereWidth + scmWhereGap);
    }
    if (rect.width() <= 0) {
        return;
    }

    QString snippet = index.data(SnippetRole).toString();
    int matchStart = index.data(MatchStartRole).toInt();
    int matchLength = index.data(MatchLengthRole).toInt();

    // The match is worth more than the start of the line, so the start gives way
    if (matchStart > scmSnippetLeadContext && metrics.horizontalAdvance(snippet) > rect.width()) {
        const int trimmed = matchStart - scmSnippetLeadContext;
        snippet = QChar(0x2026) + snippet.mid(trimmed);
        matchStart = matchStart - trimmed + 1;
    }

    const QString shown = metrics.elidedText(snippet, Qt::ElideRight, rect.width());
    // Only the letters elidedText() kept can carry the marker; measuring past the
    // ellipsis would mark letters the query is not in
    int shownChars = 0;
    const int comparableChars = std::min<int>(shown.length(), snippet.length());
    while (shownChars < comparableChars && shown.at(shownChars) == snippet.at(shownChars)) {
        ++shownChars;
    }
    // A line break in the value would draw letters outside the row
    const int textFlags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine;

    QRect markerRect;
    if (matchLength > 0 && matchStart >= 0 && matchStart < shownChars) {
        const int markerLeft = metrics.horizontalAdvance(shown.left(matchStart));
        const int markerRight = metrics.horizontalAdvance(shown.left(std::min<int>(shown.length(), matchStart + matchLength)));
        markerRect = QRect(rect.left() + markerLeft - scmMarkerBleed, rect.top() + (rect.height() - metrics.height()) / 2, markerRight - markerLeft + 2 * scmMarkerBleed, metrics.height());
        markerRect &= rect;
    }
    if (!markerRect.isEmpty()) {
        pPainter->setRenderHint(QPainter::Antialiasing, true);
        pPainter->setPen(Qt::NoPen);
        pPainter->setBrush(mMarkerColor);
        pPainter->drawRoundedRect(markerRect, scmMarkerRadius, scmMarkerRadius);
    }

    pPainter->setPen(selected ? mSelectedText : mTitleColor);
    pPainter->drawText(rect, textFlags, shown);

    if (markerRect.isEmpty()) {
        return;
    }
    // The whole line again, clipped, not the matched piece alone: from the same
    // origin the letters land where the first pass kerned them
    pPainter->save();
    pPainter->setClipRect(markerRect);
    pPainter->setPen(mMarkerText);
    pPainter->drawText(rect, textFlags, shown);
    pPainter->restore();
}

void SearchResultDelegate::paintNoticeRow(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& contentRect) const
{
    const QFontMetrics metrics(option.font);
    pPainter->setFont(option.font);
    pPainter->setPen(mMutedColor);
    pPainter->drawText(contentRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, metrics.elidedText(index.data(TitleRole).toString(), Qt::ElideRight, contentRect.width()));
}

// Asked for every row, so no style option is built
QSize SearchResultDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& index) const
{
    return {scmGlyphSize, index.data(RowKindRole).toInt() == ItemRow ? mItemRowHeight : mMatchRowHeight};
}

} // namespace uiDesign
