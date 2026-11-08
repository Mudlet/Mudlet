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

#include "FlowLayout.h"

#include <QGuiApplication>
#include <QStyle>
#include <QWidget>

#include <algorithm>

namespace uiDesign {

FlowLayout::FlowLayout(QWidget* pParent, const int horizontalSpacing, const int verticalSpacing)
: QLayout(pParent)
, mHorizontalSpacing(horizontalSpacing)
, mVerticalSpacing(verticalSpacing)
{
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem* pItem = takeAt(0)) {
        delete pItem;
    }
}

void FlowLayout::addItem(QLayoutItem* pItem)
{
    mItems.append(pItem);
    invalidate();
}

void FlowLayout::invalidate()
{
    mCachedWidth = -1;
    mCachedHeight = 0;
    QLayout::invalidate();
}

void FlowLayout::setWidthPolicy(const Width policy)
{
    if (mWidthPolicy == policy) {
        return;
    }
    mWidthPolicy = policy;
    invalidate();
}

void FlowLayout::setLineAlignment(const LineAlignment alignment)
{
    if (mLineAlignment == alignment) {
        return;
    }
    mLineAlignment = alignment;
    invalidate();
}

void FlowLayout::setStretchWidget(QWidget* pWidget)
{
    mpStretchWidget = pWidget;
    invalidate();
}

int FlowLayout::count() const
{
    return mItems.count();
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    return mItems.value(index);
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= mItems.count()) {
        return nullptr;
    }
    QLayoutItem* pItem = mItems.takeAt(index);
    invalidate();
    return pItem;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    if (width == mCachedWidth) {
        return mCachedHeight;
    }
    mCachedWidth = width;
    mCachedHeight = runItems(QRect(0, 0, width, 0), false);
    return mCachedHeight;
}

QSize FlowLayout::sizeHint() const
{
    if (mWidthPolicy == Width::WidestItem) {
        return minimumSize();
    }

    const QMargins margins = contentsMargins();
    int width = 0;
    int height = 0;
    bool first = true;
    for (const QLayoutItem* pItem : mItems) {
        if (pItem->isEmpty()) {
            continue;
        }
        const QSize hint = pItem->sizeHint();
        width += hint.width() + (first ? 0 : horizontalSpacing());
        height = std::max(height, hint.height());
        first = false;
    }
    return QSize(width + margins.left() + margins.right(), height + margins.top() + margins.bottom());
}

// One item wide: any wider would stop the column holding the row from being dragged smaller
QSize FlowLayout::minimumSize() const
{
    const QMargins margins = contentsMargins();
    QSize smallest;
    for (const QLayoutItem* pItem : mItems) {
        if (pItem->isEmpty()) {
            continue;
        }
        // Under WidestItem the hint, which is what runItems() gives each item: a smaller
        // minimum would promise a width the row is then drawn wider than
        smallest = smallest.expandedTo(mWidthPolicy == Width::WidestItem ? pItem->sizeHint() : pItem->minimumSize());
    }
    return QSize(smallest.width() + margins.left() + margins.right(), smallest.height() + margins.top() + margins.bottom());
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    runItems(rect, true);
}

int FlowLayout::horizontalSpacing() const
{
    return mHorizontalSpacing >= 0 ? mHorizontalSpacing : styleSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    return mVerticalSpacing >= 0 ? mVerticalSpacing : styleSpacing(QStyle::PM_LayoutVerticalSpacing);
}

int FlowLayout::runItems(const QRect& rect, const bool place) const
{
    const QMargins margins = contentsMargins();
    const QRect content = rect.marginsRemoved(margins);
    const int gapAcross = horizontalSpacing();
    const int gapDown = verticalSpacing();
    int x = content.x();
    int y = content.y();
    int lineHeight = 0;
    // Placing waits for a full line: centring needs the line's final height
    QList<QPair<QLayoutItem*, QRect>> line;
    const QWidget* pParent = parentWidget();
    const Qt::LayoutDirection direction = pParent ? pParent->layoutDirection() : QGuiApplication::layoutDirection();
    // Placed left to right, then mirrored inside the margins as QBoxLayout and QGridLayout mirror
    const auto placeLine = [this, &line, &y, &lineHeight, &content, direction]() {
        const int spare = line.isEmpty() ? 0 : std::max(0, content.right() - line.constLast().second.right());
        int shift = 0;
        for (auto& [pItem, geometry] : line) {
            geometry.translate(shift, 0);
            if (spare && mpStretchWidget && pItem->widget() == mpStretchWidget) {
                geometry.setWidth(geometry.width() + spare);
                shift = spare;
            }
            if (mLineAlignment == LineAlignment::Centre) {
                geometry.moveTop(y + (lineHeight - geometry.height()) / 2);
            }
            pItem->setGeometry(QStyle::visualRect(direction, content, geometry));
        }
        line.clear();
    };

    for (QLayoutItem* pItem : mItems) {
        if (pItem->isEmpty()) {
            continue;
        }
        const QSize hint = pItem->sizeHint();
        // An empty line takes an item of any width, so one wider than the row overflows its edge
        if (lineHeight > 0 && x + hint.width() > content.right() + 1) {
            if (place) {
                placeLine();
            }
            x = content.x();
            y += lineHeight + gapDown;
            lineHeight = 0;
        }
        if (place) {
            line.append({pItem, QRect(QPoint(x, y), hint)});
        }
        x += hint.width() + gapAcross;
        lineHeight = std::max(lineHeight, hint.height());
    }
    if (place) {
        placeLine();
    }

    return y + lineHeight - rect.y() + margins.bottom();
}

int FlowLayout::styleSpacing(const QStyle::PixelMetric metric) const
{
    const QWidget* pParent = parentWidget();
    if (!pParent) {
        return 0;
    }
    // QCommonStyle answers -1 ("ask the layout") for these, and this is the layout
    return std::max(0, pParent->style()->pixelMetric(metric, nullptr, pParent));
}

} // namespace uiDesign
