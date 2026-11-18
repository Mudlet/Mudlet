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

#include "ContentScrollArea.h"

#include <QEvent>
#include <QScrollBar>

namespace uiDesign {

ContentScrollArea::ContentScrollArea(QWidget* pParent)
: QScrollArea(pParent)
{
    // The room is taken from the viewport at all times; only the bar inside it comes and goes
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QScrollBar* pBar = verticalScrollBar();
    // Otherwise a hidden bar collapses its container to no height, and on showing is drawn a layout pass late
    QSizePolicy policy = pBar->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    pBar->setSizePolicy(policy);
    connect(pBar, &QAbstractSlider::rangeChanged, this, &ContentScrollArea::showTheBarWhileItScrolls);
    showTheBarWhileItScrolls();
}

QSize ContentScrollArea::sizeHint() const
{
    // QScrollArea::sizeHint() without its cache, keeping Qt's cap so a long list
    // does not ask for every row's height
    const int frame = 2 * frameWidth();
    const int fontHeight = fontMetrics().height();
    QSize hint(frame, frame);
    if (const QWidget* pContent = widget()) {
        hint += widgetResizable() ? pContent->sizeHint() : pContent->size();
    } else {
        hint += QSize(12 * fontHeight, 8 * fontHeight);
    }
    return hint.boundedTo(QSize(36 * fontHeight, 24 * fontHeight));
}

QSize ContentScrollArea::minimumSizeHint() const
{
    // Qt's width also counts scroll bars, but stays under any column of fields
    // this holds, so only the height is replaced
    return QSize(QScrollArea::minimumSizeHint().width(), 2 * frameWidth());
}

bool ContentScrollArea::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (pWatched == widget() && pEvent->type() == QEvent::LayoutRequest) {
        updateGeometry();
    }
    return QScrollArea::eventFilter(pWatched, pEvent);
}

void ContentScrollArea::showTheBarWhileItScrolls()
{
    QScrollBar* pBar = verticalScrollBar();
    pBar->setVisible(pBar->maximum() > pBar->minimum());
}

} // namespace uiDesign
