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

#include "EditorTreeRowDelegate.h"

#include "EditorTreeRowMetrics.h"
#include "TTreeWidget.h"
#include "uiDesign.h"

#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>

#include <algorithm>

namespace uiDesign {

EditorTreeRowDelegate::EditorTreeRowDelegate(TTreeWidget* pTree)
: QStyledItemDelegate(pTree)
, mpTree(pTree)
{
    if (mpTree) {
        mIndentStep = mpTree->indentation() > 0 ? mpTree->indentation() : scmTreeFallbackIndentStep;
        mpTree->setIndentation(0);
        // QAbstractScrollArea makes the viewport in its constructor and the trees never replace it
        mpTree->viewport()->installEventFilter(this);
    }
}

void EditorTreeRowDelegate::restyle()
{
    const ThemeTokens tokens = themeTokens();
    mChevronInk = tokens.mutedText;
    mAccentBar = tokens.accent;
    restyleInks(tokens);
    clearCaches();
}

void EditorTreeRowDelegate::clearCaches() const
{
    for (auto& cached : mChevronGlyphs) {
        cached = QPixmap();
    }
    mDecorations.clear();
    clearGlyphs();
}

void EditorTreeRowDelegate::syncGlyphRatio() const
{
    const qreal ratio = mpTree ? mpTree->devicePixelRatioF() : 1.0;
    if (qFuzzyCompare(ratio + 1.0, mGlyphRatio + 1.0)) {
        return;
    }
    mGlyphRatio = ratio;
    // Clearing on a ratio change is why the decoration key can leave the ratio out
    clearCaches();
}

int EditorTreeRowDelegate::rowLevelOf(const QModelIndex& index) const
{
    return treeRowLevelOf(index);
}

// None at the top level: that row is the tree's own heading, and the trees use an undecorated root
EditorTreeRowDelegate::ChevronState EditorTreeRowDelegate::chevronOf(const QModelIndex& index, const int level) const
{
    if (!mpTree || !index.isValid() || index.column() != 0 || level < 1) {
        return ChevronState::None;
    }
    const QAbstractItemModel* pModel = index.model();
    if (!pModel || pModel->rowCount(index) < 1) {
        return ChevronState::None;
    }
    return mpTree->isExpanded(index) ? ChevronState::Open : ChevronState::Closed;
}

QPixmap EditorTreeRowDelegate::chevronGlyph(const ChevronState state) const
{
    QPixmap& glyph = mChevronGlyphs[static_cast<int>(state)];
    if (!glyph.isNull() || state == ChevronState::None) {
        return glyph;
    }
    glyph = treeRowChevronGlyph(state == ChevronState::Open, mChevronInk, mGlyphRatio);
    return glyph;
}

EditorTreeRowDelegate::Decoration
EditorTreeRowDelegate::composeDecoration(const int key, const int level, const ChevronState chevron, const bool dotSlot, const QPixmap& dot, const QPixmap& mark) const
{
    if (mDecorations.size() >= scmTreeDecorationCacheLimit) {
        mDecorations.clear();
    }

    const int lead = level * mIndentStep;
    const int dotWidth = dotSlot ? scmTreeDotDiameter : 0;
    const int markWidth = mark.isNull() ? 0 : scmTreeMarkSize;
    const int gap = (dotWidth > 0 && markWidth > 0) ? scmTreeDotGap : 0;
    const int slotWidth = std::max(lead + dotWidth + gap + markWidth, 1);

    QPixmap composed(qRound(slotWidth * mGlyphRatio), qRound(scmTreeSlotHeight * mGlyphRatio));
    composed.setDevicePixelRatio(mGlyphRatio);
    composed.fill(Qt::transparent);
    QPainter painter(&composed);
    if (chevron != ChevronState::None) {
        painter.drawPixmap(QPointF(lead - mIndentStep + (mIndentStep - scmTreeChevronBox) / 2.0, (scmTreeSlotHeight - scmTreeChevronBox) / 2.0), chevronGlyph(chevron));
    }
    if (!dot.isNull()) {
        painter.drawPixmap(QPointF(lead, (scmTreeSlotHeight - scmTreeDotDiameter) / 2.0), dot);
    }
    if (markWidth > 0) {
        painter.drawPixmap(QPointF(lead + dotWidth + gap, (scmTreeSlotHeight - scmTreeMarkSize) / 2.0), mark);
    }
    painter.end();

    Decoration decoration;
    decoration.icon = QIcon(composed);
    decoration.size = QSize(slotWidth, scmTreeSlotHeight);
    mDecorations.insert(key, decoration);
    return decoration;
}

QRect EditorTreeRowDelegate::dotHitRect(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!mpTree || !index.isValid() || index.column() != 0 || option.rect.isEmpty() || !carriesDot(index)) {
        return {};
    }

    // Asked of the style: QStyleSheetStyle places a row's picture, and its padding cannot be guessed here
    QStyleOptionViewItem dotOption(option);
    initStyleOption(&dotOption, index);
    const QRect decorationRect = mpTree->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &dotOption, mpTree);
    if (decorationRect.isEmpty()) {
        return {};
    }

    // The mark and a little slack only, so the rest of the leading edge can still start a drag
    const int lead = rowLevelOf(index) * mIndentStep;
    const QRect dotRect(decorationRect.left() + lead, decorationRect.top() + (decorationRect.height() - scmTreeDotDiameter) / 2, scmTreeDotDiameter, scmTreeDotDiameter);
    return dotRect.adjusted(-scmTreeMarkHitSlack, -scmTreeMarkHitSlack, scmTreeMarkHitSlack, scmTreeMarkHitSlack);
}

QRect EditorTreeRowDelegate::chevronHitRect(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int level = rowLevelOf(index);
    if (!mpTree || !index.isValid() || index.column() != 0 || option.rect.isEmpty() || chevronOf(index, level) == ChevronState::None) {
        return {};
    }

    QStyleOptionViewItem chevronOption(option);
    initStyleOption(&chevronOption, index);
    const QRect decorationRect = mpTree->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &chevronOption, mpTree);
    if (decorationRect.isEmpty()) {
        return {};
    }

    // The whole indentation step, where the view's branch arrow stood and a reader will aim
    return QRect(decorationRect.left() + (level - 1) * mIndentStep, option.rect.top(), mIndentStep, option.rect.height());
}

QStyleOptionViewItem EditorTreeRowDelegate::rowOption(const QModelIndex& index) const
{
    QStyleOptionViewItem option = mpTree->viewItemOption();
    option.rect = mpTree->visualRect(index);
    if (index == mpTree->currentIndex()) {
        option.state |= QStyle::State_HasFocus;
    }
    return option;
}

bool EditorTreeRowDelegate::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    // QStyledItemDelegate filters its own editor widgets through this, and that handling has to stand
    if (!mpTree || pWatched != mpTree->viewport()) {
        return QStyledItemDelegate::eventFilter(pWatched, pEvent);
    }

    switch (pEvent->type()) {
    case QEvent::MouseButtonPress:
        // Reset on every press in case a release never arrived; a double click's second press is
        // MouseButtonDblClick, so mLastDotPressIndex outlives the first press of the pair
        mPressAnswered = false;
        mLastDotPressIndex = QPersistentModelIndex();
        break;
    case QEvent::MouseMove:
        // The view recorded the press before asking the delegate, so a move handed on would
        // drag-select out of the mark or lift the row out of the tree
        if (mPressAnswered && (static_cast<QMouseEvent*>(pEvent)->buttons() & Qt::LeftButton)) {
            return true;
        }
        break;
    case QEvent::MouseButtonRelease:
        // Handed on, the view would read it as a click on whichever row it ended over and reload
        // that row, losing whatever was typed into the editor
        if (mPressAnswered) {
            mPressAnswered = false;
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

bool EditorTreeRowDelegate::editorEvent(QEvent* pEvent, QAbstractItemModel* pModel, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    const QEvent::Type eventType = pEvent->type();
    if (eventType != QEvent::MouseButtonPress && eventType != QEvent::MouseButtonDblClick) {
        return QStyledItemDelegate::editorEvent(pEvent, pModel, option, index);
    }

    auto* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
    if (pMouseEvent->button() != Qt::LeftButton || (pMouseEvent->modifiers() & scmTreeSelectionModifiers)) {
        return QStyledItemDelegate::editorEvent(pEvent, pModel, option, index);
    }
    const QPoint pressedAt = pMouseEvent->position().toPoint();
    // Chevron first: the dot's slack overlaps the chevron's cell
    const bool onChevron = chevronHitRect(option, index).contains(pressedAt);
    const bool onDot = !onChevron && dotHitRect(option, index).contains(pressedAt);

    if (eventType == QEvent::MouseButtonDblClick) {
        // Eaten even when it drifted off the mark: a double click is allowed a few pixels of drift,
        // and handed on it reaches activated() and switches the row straight back
        if (!onChevron && !onDot && !(mLastDotPressIndex.isValid() && mLastDotPressIndex == index)) {
            return QStyledItemDelegate::editorEvent(pEvent, pModel, option, index);
        }
        mPressAnswered = true;
        return true;
    }

    if (onChevron) {
        mpTree->setExpanded(index, !mpTree->isExpanded(index));
        mPressAnswered = true;
        return true;
    }

    if (!onDot) {
        return QStyledItemDelegate::editorEvent(pEvent, pModel, option, index);
    }

    if (QTreeWidgetItem* pItem = mpTree->itemFromIndex(index)) {
        mpTree->setCurrentItem(pItem);
        dotPressed();
    }
    mPressAnswered = true;
    mLastDotPressIndex = index;
    return true;
}

} // namespace uiDesign
