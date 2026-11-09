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

#include "PairGridLayout.h"

#include <QGuiApplication>
#include <QStyle>
#include <QWidget>

#include <algorithm>
#include <tuple>
#include <utility>

namespace uiDesign {

namespace {

bool shown(const QLayoutItem* pItem)
{
    return pItem && !pItem->isEmpty();
}

int hintWidth(const QLayoutItem* pItem)
{
    return shown(pItem) ? pItem->sizeHint().width() : 0;
}

int heightAt(const QLayoutItem* pItem, const int width)
{
    if (!shown(pItem)) {
        return 0;
    }
    return pItem->hasHeightForWidth() ? pItem->heightForWidth(width) : pItem->sizeHint().height();
}

} // namespace

PairGridLayout::PairGridLayout(QWidget* pParent, const int horizontalSpacing, const int verticalSpacing)
: QLayout(pParent)
, mHorizontalSpacing(horizontalSpacing)
, mVerticalSpacing(verticalSpacing)
{
}

PairGridLayout::~PairGridLayout()
{
    while (QLayoutItem* pItem = takeAt(0)) {
        delete pItem;
    }
}

void PairGridLayout::addRow(const RowKind kind, QWidget* pFirst, QWidget* pSecond)
{
    Row row;
    row.kind = kind;
    for (auto [pWidget, ppItem] : {std::pair{pFirst, &row.pFirst}, std::pair{pSecond, &row.pSecond}}) {
        if (!pWidget) {
            continue;
        }
        addChildWidget(pWidget);
        *ppItem = new QWidgetItem(pWidget);
        mItems.append(*ppItem);
    }
    mRows.append(row);
    invalidate();
}

void PairGridLayout::addPair(QWidget* pLabel, QWidget* pControl)
{
    addRow(RowKind::Pair, pLabel, pControl);
}

void PairGridLayout::addSpanningWidget(QWidget* pWidget)
{
    addRow(RowKind::Spanning, pWidget, nullptr);
}

void PairGridLayout::addFooter(QWidget* pLeading, QWidget* pTrailing)
{
    addRow(RowKind::Footer, pLeading, pTrailing);
}

void PairGridLayout::setReadingOrder(const ReadingOrder order)
{
    mReadingOrder = order;
    invalidate();
}

void PairGridLayout::setPairsPerRow(const PairsPerRow pairsPerRow)
{
    if (pairsPerRow == mPairsPerRow) {
        return;
    }
    mPairsPerRow = pairsPerRow;
    invalidate();
}

void PairGridLayout::setPairSpacing(const int spacing)
{
    mPairSpacing = spacing;
    invalidate();
}

void PairGridLayout::addItem(QLayoutItem* pItem)
{
    mItems.append(pItem);
    mRows.append({RowKind::Spanning, pItem, nullptr});
    invalidate();
}

int PairGridLayout::count() const
{
    return mItems.count();
}

QLayoutItem* PairGridLayout::itemAt(int index) const
{
    return mItems.value(index);
}

QLayoutItem* PairGridLayout::takeAt(int index)
{
    if (index < 0 || index >= mItems.count()) {
        return nullptr;
    }
    QLayoutItem* pItem = mItems.takeAt(index);
    // The row stays, a place in the grid whatever else is taken out of it
    for (Row& row : mRows) {
        if (row.pFirst == pItem) {
            row.pFirst = nullptr;
        }
        if (row.pSecond == pItem) {
            row.pSecond = nullptr;
        }
    }
    invalidate();
    return pItem;
}

void PairGridLayout::invalidate()
{
    mCachedWidth = -1;
    mCachedHeight = 0;
    QLayout::invalidate();
}

// The controls take whatever width the row has beyond the labels
Qt::Orientations PairGridLayout::expandingDirections() const
{
    return Qt::Horizontal;
}

bool PairGridLayout::hasHeightForWidth() const
{
    return true;
}

int PairGridLayout::heightForWidth(int width) const
{
    if (width == mCachedWidth) {
        return mCachedHeight;
    }
    mCachedWidth = width;
    mCachedHeight = runRows(QRect(0, 0, width, 0), false);
    return mCachedHeight;
}

// Two pairs to a row, at the narrowest that takes
QSize PairGridLayout::sizeHint() const
{
    const QMargins margins = contentsMargins();
    const int width = narrowestContentWidth(true) + margins.left() + margins.right();
    return QSize(width, heightForWidth(width));
}

// One pair to a row is as narrow as this goes; the height asked for is the
// shortest it comes to, two pairs to a row, since whatever holds it asks for the
// height at the width it has through heightForWidth()
QSize PairGridLayout::minimumSize() const
{
    const QMargins margins = contentsMargins();
    return QSize(narrowestContentWidth(false) + margins.left() + margins.right(), sizeHint().height());
}

void PairGridLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    runRows(rect, true);
}

int PairGridLayout::horizontalSpacing() const
{
    return mHorizontalSpacing >= 0 ? mHorizontalSpacing : styleSpacing(Qt::Horizontal);
}

int PairGridLayout::verticalSpacing() const
{
    return mVerticalSpacing >= 0 ? mVerticalSpacing : styleSpacing(Qt::Vertical);
}

int PairGridLayout::pairSpacing() const
{
    return mPairSpacing >= 0 ? mPairSpacing : horizontalSpacing();
}

QList<QList<const PairGridLayout::Row*>> PairGridLayout::pairBlocks() const
{
    QList<QList<const Row*>> blocks;
    bool open = false;
    for (const Row& row : mRows) {
        if (row.kind != RowKind::Pair) {
            open = false;
            continue;
        }
        if (!shown(row.pFirst) && !shown(row.pSecond)) {
            continue;
        }
        if (!open) {
            blocks.append(QList<const Row*>());
            open = true;
        }
        blocks.last().append(&row);
    }
    return blocks;
}

qsizetype PairGridLayout::leadingCount(const qsizetype pairs) const
{
    return mReadingOrder == ReadingOrder::DownColumns ? (pairs + 1) / 2 : pairs;
}

PairGridLayout::Columns PairGridLayout::pairColumns(const bool twoPairs) const
{
    Columns columns;
    for (const QList<const Row*>& block : pairBlocks()) {
        const qsizetype leading = leadingCount(block.size());
        for (qsizetype index = 0; index < block.size(); ++index) {
            const Row* pRow = block.at(index);
            const bool trailing = twoPairs && (mReadingOrder == ReadingOrder::DownColumns ? index >= leading : index % 2 == 1);
            int& label = trailing ? columns.trailingLabel : columns.leadingLabel;
            int& control = trailing ? columns.trailingControl : columns.leadingControl;
            label = std::max(label, hintWidth(pRow->pFirst));
            control = std::max(control, hintWidth(pRow->pSecond));
        }
    }
    // Read across, every control on the one width, so none is narrower than the widest
    if (mReadingOrder == ReadingOrder::AcrossRows) {
        columns.leadingControl = std::max(columns.leadingControl, columns.trailingControl);
        columns.trailingControl = columns.leadingControl;
    }
    return columns;
}

// A footer piece has the width of the pair it stands under
bool PairGridLayout::footersFit(const Columns& columns, const bool twoPairs) const
{
    const int gap = horizontalSpacing();
    const int leadingHalf = columns.leadingLabel + gap + columns.leadingControl;
    const int trailingHalf = twoPairs ? columns.trailingLabel + gap + columns.trailingControl : leadingHalf;
    for (const Row& row : mRows) {
        if (row.kind != RowKind::Footer) {
            continue;
        }
        if (hintWidth(row.pFirst) > leadingHalf || hintWidth(row.pSecond) > trailingHalf) {
            return false;
        }
    }
    return true;
}

// Two to a row, a footer piece wider than its pair makes its room in its own
// label column, since widening the controls would widen those of the other
// side as well; one to a row, the controls fill the row anyway and it is they
// that make the room
PairGridLayout::Columns PairGridLayout::narrowestColumns(const bool twoPairs) const
{
    Columns columns = pairColumns(twoPairs);
    const int gap = horizontalSpacing();
    for (const Row& row : mRows) {
        if (row.kind != RowKind::Footer) {
            continue;
        }
        if (twoPairs) {
            columns.leadingLabel = std::max(columns.leadingLabel, hintWidth(row.pFirst) - gap - columns.leadingControl);
            columns.trailingLabel = std::max(columns.trailingLabel, hintWidth(row.pSecond) - gap - columns.trailingControl);
            continue;
        }
        for (const QLayoutItem* pItem : {row.pFirst, row.pSecond}) {
            columns.leadingControl = std::max(columns.leadingControl, hintWidth(pItem) - columns.leadingLabel - gap);
        }
    }
    return columns;
}

int PairGridLayout::narrowestContentWidth(const bool twoPairs) const
{
    const Columns columns = narrowestColumns(twoPairs);
    const int gap = horizontalSpacing();
    int width = columns.leadingLabel + gap + columns.leadingControl;
    if (twoPairs) {
        width += pairSpacing() + columns.trailingLabel + gap + columns.trailingControl;
    }
    for (const Row& row : mRows) {
        if (row.kind == RowKind::Spanning && shown(row.pFirst)) {
            width = std::max(width, row.pFirst->minimumSize().width());
        }
    }
    return width;
}

int PairGridLayout::runRows(const QRect& rect, const bool place) const
{
    const QMargins margins = contentsMargins();
    const QRect content = rect.marginsRemoved(margins);
    const int gapAcross = horizontalSpacing();
    const int gapBetweenPairs = pairSpacing();
    const int gapDown = verticalSpacing();
    const bool twoPairs = mPairsPerRow == PairsPerRow::Automatic ? content.width() >= narrowestContentWidth(true) : mPairsPerRow == PairsPerRow::Two;

    // The labels keep their widths and the controls share what is left: read
    // across, one width for all of them, and read down, half each on top of
    // what each column needs. Only where that leaves a footer piece too little
    // room do the label columns take the narrowest arrangement's widths.
    const auto stretch = [this, &content, gapAcross, gapBetweenPairs, twoPairs](Columns columns) {
        if (!twoPairs) {
            columns.leadingControl = std::max(columns.leadingControl, content.width() - columns.leadingLabel - gapAcross);
            return columns;
        }
        const int labels = columns.leadingLabel + columns.trailingLabel + 2 * gapAcross + gapBetweenPairs;
        if (mReadingOrder == ReadingOrder::AcrossRows) {
            columns.leadingControl = std::max(columns.leadingControl, (content.width() - labels) / 2);
            columns.trailingControl = columns.leadingControl;
            return columns;
        }
        const int spare = content.width() - labels - columns.leadingControl - columns.trailingControl;
        if (spare > 0) {
            columns.leadingControl += spare / 2;
            columns.trailingControl += spare - spare / 2;
        }
        return columns;
    };
    Columns columns = stretch(pairColumns(twoPairs));
    if (!footersFit(columns, twoPairs)) {
        columns = stretch(narrowestColumns(twoPairs));
    }
    // The controls share an odd pixel out by putting it between the pairs, so the trailing one still ends on the edge
    const int oddPixel =
            twoPairs ? std::max(0, content.width() - columns.leadingLabel - columns.trailingLabel - 2 * gapAcross - gapBetweenPairs - columns.leadingControl - columns.trailingControl) : 0;
    const int leadingControlX = content.x() + columns.leadingLabel + gapAcross;
    const int trailingLabelX = leadingControlX + columns.leadingControl + gapBetweenPairs + oddPixel;
    const int trailingControlX = trailingLabelX + columns.trailingLabel + gapAcross;
    const int leadingHalf = columns.leadingLabel + gapAcross + columns.leadingControl;
    const int trailingHalf = columns.trailingLabel + gapAcross + columns.trailingControl;

    const QWidget* pParent = parentWidget();
    const Qt::LayoutDirection direction = pParent ? pParent->layoutDirection() : QGuiApplication::layoutDirection();
    // Every x here runs from the leading edge; mirrored inside the margins as QGridLayout mirrors its cells
    const auto put = [&content, direction](QLayoutItem* pItem, const QRect& logical) {
        pItem->setGeometry(QStyle::visualRect(direction, content, logical));
    };

    int y = content.y();
    bool firstLine = true;
    const auto startLine = [&y, &firstLine, gapDown]() {
        if (!firstLine) {
            y += gapDown;
        }
        firstLine = false;
    };
    // Every item is handed the whole of its cell, as a grid hands it: a label
    // fills it and is drawn level with the middle, and a control that will not
    // grow taller is centred in it by its layout item
    const auto putPairs = [&](const Row* pLeading, const Row* pTrailing) {
        startLine();
        int height = 0;
        if (pLeading) {
            height = std::max({height, heightAt(pLeading->pFirst, columns.leadingLabel), heightAt(pLeading->pSecond, columns.leadingControl)});
        }
        if (pTrailing) {
            height = std::max({height, heightAt(pTrailing->pFirst, columns.trailingLabel), heightAt(pTrailing->pSecond, columns.trailingControl)});
        }
        if (place) {
            for (const auto& [pRow, labelX, labelWidth, controlX, controlWidth] : {std::tuple{pLeading, content.x(), columns.leadingLabel, leadingControlX, columns.leadingControl},
                                                                                   std::tuple{pTrailing, trailingLabelX, columns.trailingLabel, trailingControlX, columns.trailingControl}}) {
                if (!pRow) {
                    continue;
                }
                if (shown(pRow->pFirst)) {
                    put(pRow->pFirst, QRect(labelX, y, labelWidth, height));
                }
                if (shown(pRow->pSecond)) {
                    put(pRow->pSecond, QRect(controlX, y, controlWidth, height));
                }
            }
        }
        y += height;
    };
    const auto putAcross = [&](QLayoutItem* pItem, const int x, const int width) {
        startLine();
        const int height = heightAt(pItem, width);
        if (place) {
            put(pItem, QRect(x, y, width, height));
        }
        y += height;
    };
    const auto putBlock = [&](const QList<const Row*>& block) {
        if (!twoPairs) {
            for (const Row* pRow : block) {
                putPairs(pRow, nullptr);
            }
            return;
        }
        if (mReadingOrder == ReadingOrder::DownColumns) {
            const qsizetype leading = leadingCount(block.size());
            for (qsizetype index = 0; index < leading; ++index) {
                putPairs(block.at(index), index + leading < block.size() ? block.at(index + leading) : nullptr);
            }
            return;
        }
        for (qsizetype index = 0; index < block.size(); index += 2) {
            putPairs(block.at(index), index + 1 < block.size() ? block.at(index + 1) : nullptr);
        }
    };

    QList<const Row*> block;
    for (const Row& row : mRows) {
        if (row.kind == RowKind::Pair) {
            if (shown(row.pFirst) || shown(row.pSecond)) {
                block.append(&row);
            }
            continue;
        }
        putBlock(block);
        block.clear();
        switch (row.kind) {
        case RowKind::Pair:
            break;
        case RowKind::Spanning:
            if (shown(row.pFirst)) {
                putAcross(row.pFirst, content.x(), content.width());
            }
            break;
        case RowKind::Footer:
            if (!twoPairs) {
                if (shown(row.pFirst)) {
                    putAcross(row.pFirst, content.x(), content.width());
                }
                // Stretched across a whole row, a button at the foot of a
                // column reads as a bar; it keeps its own width at the
                // trailing edge, where the trailing half would have put it
                if (shown(row.pSecond)) {
                    const int width = std::min(content.width(), hintWidth(row.pSecond));
                    putAcross(row.pSecond, content.x() + content.width() - width, width);
                }
                break;
            }
            if (!shown(row.pFirst) && !shown(row.pSecond)) {
                break;
            }
            startLine();
            {
                const int height = std::max(heightAt(row.pFirst, leadingHalf), heightAt(row.pSecond, trailingHalf));
                if (place && shown(row.pFirst)) {
                    put(row.pFirst, QRect(content.x(), y, leadingHalf, height));
                }
                if (place && shown(row.pSecond)) {
                    put(row.pSecond, QRect(trailingLabelX, y, trailingHalf, height));
                }
                y += height;
            }
            break;
        }
    }
    putBlock(block);

    return y - rect.y() + margins.bottom();
}

int PairGridLayout::styleSpacing(const Qt::Orientation orientation) const
{
    const QWidget* pParent = parentWidget();
    if (!pParent) {
        return 0;
    }
    const int spacing = pParent->style()->pixelMetric(orientation == Qt::Horizontal ? QStyle::PM_LayoutHorizontalSpacing : QStyle::PM_LayoutVerticalSpacing, nullptr, pParent);
    if (spacing >= 0) {
        return spacing;
    }
    // A style answering -1 spaces each pair of controls by their kinds, as a
    // grid asks it to; a label beside a button is what this holds most of
    return std::max(0, pParent->style()->layoutSpacing(QSizePolicy::Label, QSizePolicy::PushButton, orientation, nullptr, pParent));
}

} // namespace uiDesign
