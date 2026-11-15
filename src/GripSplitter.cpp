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

#include "GripSplitter.h"

#include "uiDesign.h"

#include <QApplication>
#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace uiDesign {

namespace {
constexpr int scmHeaderThickness = 32;
constexpr int scmHeaderPadding = 2;
constexpr int scmSeamThickness = 1;
constexpr int scmSeamHoveredThickness = 3;

// A stylesheet fills the panes and the palette cannot see it, so each pane
// names its tone in scmProp_paneTone
QColor paneTone(const QWidget* pPane, const QColor& fallback)
{
    if (!pPane) {
        return fallback;
    }
    const QColor tone = qvariant_cast<QColor>(pPane->property(scmProp_paneTone));
    return tone.isValid() ? tone : fallback;
}
} // namespace

GripSplitterHandle::GripSplitterHandle(const Qt::Orientation orientation, QSplitter* pParent)
: QSplitterHandle(orientation, pParent)
{
    // QSplitterHandle leaves WA_Hover to the style, and the cursor needs the
    // hover positions whatever the style
    setAttribute(Qt::WA_Hover);
}

void GripSplitterHandle::setContent(QWidget* pContent)
{
    if (mpContent == pContent) {
        return;
    }
    if (mpContent) {
        mpContent->hide();
        mpContent->setParent(nullptr);
    }
    mpContent = pContent;
    if (!mpContent) {
        updateGeometry();
        update();
        return;
    }

    mpContent->setParent(this);
    // WA_TransparentForMouseEvents is not inherited, and a child left without it
    // swallows the press that starts a drag
    mpContent->setAttribute(Qt::WA_TransparentForMouseEvents);
    for (QWidget* pChild : mpContent->findChildren<QWidget*>()) {
        pChild->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    mpContent->setGeometry(rect());
    mpContent->show();
    updateGeometry();
    update();
}

void GripSplitterHandle::setResizes(const bool resizes)
{
    if (mResizes == resizes) {
        return;
    }
    mResizes = resizes;
    // Through placeCursor(): over the clickable piece the cursor stays a link
    // even when the handle resizes nothing
    placeCursor(underMouse() ? mapFromGlobal(QCursor::pos()) : QPoint(-1, -1));
    update();
}

void GripSplitterHandle::setClickable(QWidget* pPiece)
{
    if (mpClickable == pPiece) {
        return;
    }
    if (mpClickable) {
        mpClickable->removeEventFilter(this);
    }
    mpClickable = pPiece;
    if (mpClickable) {
        mpClickable->installEventFilter(this);
    }
}

bool GripSplitterHandle::overClickable(const QPoint& point) const
{
    if (!mpClickable || !mpClickable->isVisible()) {
        return false;
    }
    return QRect(mpClickable->mapTo(this, QPoint(0, 0)), mpClickable->size()).contains(point);
}

void GripSplitterHandle::placeCursor(const QPoint& point)
{
    const Qt::CursorShape edge = mResizes ? (orientation() == Qt::Vertical ? Qt::SplitVCursor : Qt::SplitHCursor) : Qt::ArrowCursor;
    const Qt::CursorShape wanted = overClickable(point) ? Qt::PointingHandCursor : edge;
    if (cursor().shape() != wanted) {
        setCursor(wanted);
    }
}

bool GripSplitterHandle::event(QEvent* event)
{
    // A pointer that comes to rest on the piece in one movement sends a
    // HoverEnter and no HoverMove
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverMove) {
        placeCursor(static_cast<QHoverEvent*>(event)->position().toPoint());
    }
    return QSplitterHandle::event(event);
}

bool GripSplitterHandle::eventFilter(QObject* watched, QEvent* event)
{
    // The cursor is only placed as the pointer moves, so a piece shown or hidden
    // under a still pointer would leave it stale
    if (watched == mpClickable && (event->type() == QEvent::Show || event->type() == QEvent::Hide) && underMouse()) {
        placeCursor(mapFromGlobal(QCursor::pos()));
    }
    return QSplitterHandle::eventFilter(watched, event);
}

// An inert handle skips the base class: it would still move the panes and the
// fit would snap them back, a heading that fights back
void GripSplitterHandle::mousePressEvent(QMouseEvent* event)
{
    if (mResizes) {
        QSplitterHandle::mousePressEvent(event);
    }
    // Another button's press must not pair with a later left release
    if (event->button() == Qt::LeftButton) {
        mPressedAt = event->globalPosition().toPoint();
        mDragged = false;
    }
}

void GripSplitterHandle::mouseMoveEvent(QMouseEvent* event)
{
    if (mResizes) {
        QSplitterHandle::mouseMoveEvent(event);
    }
    // Global coordinates: the handle follows the pointer during a drag, so in
    // its own the pointer never moves
    if ((event->buttons() & Qt::LeftButton) && (event->globalPosition().toPoint() - mPressedAt).manhattanLength() >= QApplication::startDragDistance()) {
        mDragged = true;
    }
}

void GripSplitterHandle::mouseReleaseEvent(QMouseEvent* event)
{
    if (mResizes) {
        QSplitterHandle::mouseReleaseEvent(event);
    }
    if (event->button() != Qt::LeftButton || mDragged) {
        return;
    }
    if (overClickable(event->position().toPoint())) {
        emit clicked(mpClickable);
    }
}

QSize GripSplitterHandle::sizeHint() const
{
    QSize hint = QSplitterHandle::sizeHint();
    if (!mpContent) {
        return hint;
    }

    // QSplitter sizes each handle from its own sizeHint(), so one can be
    // thicker than the rest
    const QSize contentHint = mpContent->sizeHint();
    if (orientation() == Qt::Vertical) {
        hint.setHeight(std::max(scmHeaderThickness, contentHint.height() + 2 * scmHeaderPadding));
    } else {
        hint.setWidth(std::max(scmHeaderThickness, contentHint.width() + 2 * scmHeaderPadding));
    }
    return hint;
}

void GripSplitterHandle::resizeEvent(QResizeEvent* event)
{
    QSplitterHandle::resizeEvent(event);
    if (mpContent) {
        mpContent->setGeometry(rect());
    }
}

void GripSplitterHandle::enterEvent(TEnterEvent* event)
{
    QSplitterHandle::enterEvent(event);
    mHovered = true;
    update();
}

void GripSplitterHandle::leaveEvent(QEvent* event)
{
    QSplitterHandle::leaveEvent(event);
    mHovered = false;
    update();
}

void GripSplitterHandle::paintSeam(QPainter& painter, const ThemeTokens& tokens) const
{
    // Each pane's tone is carried up to the seam, so nine pixels the mouse needs
    // read as no gap
    QSplitter* pSplitter = splitter();
    const int index = pSplitter ? pSplitter->indexOf(const_cast<GripSplitterHandle*>(this)) : -1;
    const QColor before = paneTone(index > 0 ? pSplitter->widget(index - 1) : nullptr, tokens.page);
    const QColor after = paneTone(index >= 0 ? pSplitter->widget(index) : nullptr, tokens.page);

    const QColor seamColor = mHovered ? tokens.accent : tokens.separator;
    const int thickness = mHovered ? scmSeamHoveredThickness : scmSeamThickness;
    if (orientation() == Qt::Vertical) {
        const int middle = (height() - scmSeamThickness) / 2;
        painter.fillRect(QRect(0, 0, width(), middle), before);
        painter.fillRect(QRect(0, middle + scmSeamThickness, width(), height() - middle - scmSeamThickness), after);
        painter.fillRect(QRect(0, middle - (thickness - scmSeamThickness) / 2, width(), thickness), seamColor);
        return;
    }
    const int middle = (width() - scmSeamThickness) / 2;
    painter.fillRect(QRect(0, 0, middle, height()), before);
    painter.fillRect(QRect(middle + scmSeamThickness, 0, width() - middle - scmSeamThickness, height()), after);
    painter.fillRect(QRect(middle - (thickness - scmSeamThickness) / 2, 0, thickness, height()), seamColor);
}

void GripSplitterHandle::paintEvent(QPaintEvent*)
{
    const ThemeTokens tokens = themeTokens();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!mpContent) {
        paintSeam(painter, tokens);
        return;
    }

    // Drawn a corner's worth taller than the handle so only the top corners are
    // rounded
    painter.fillRect(rect(), tokens.page);
    painter.setPen(Qt::NoPen);
    painter.setBrush(tokens.separator);
    QRectF strip(rect());
    if (orientation() == Qt::Vertical) {
        strip.setHeight(strip.height() + scmRadiusPanel);
    } else {
        strip.setWidth(strip.width() + scmRadiusPanel);
    }
    painter.drawRoundedRect(strip, scmRadiusPanel, scmRadiusPanel);

    if (!mResizes || !mHovered) {
        return;
    }
    QPainterPath stripPath;
    stripPath.addRoundedRect(strip, scmRadiusPanel, scmRadiusPanel);
    painter.setClipPath(stripPath);
    if (orientation() == Qt::Vertical) {
        painter.fillRect(QRect(0, 0, width(), scmSeamHoveredThickness), tokens.accent);
    } else {
        painter.fillRect(QRect(0, 0, scmSeamHoveredThickness, height()), tokens.accent);
    }
}

GripSplitter::GripSplitter(QWidget* pParent)
: QSplitter(pParent)
{
    setHandleWidth(scmHandleThickness);
}

GripSplitter::GripSplitter(const Qt::Orientation orientation, QWidget* pParent)
: QSplitter(orientation, pParent)
{
    setHandleWidth(scmHandleThickness);
}

QSplitterHandle* GripSplitter::createHandle()
{
    return new GripSplitterHandle(orientation(), this);
}

void GripSplitter::setHeaderHandle(const int index, QWidget* pContent)
{
    auto* pHandle = qobject_cast<GripSplitterHandle*>(handle(index));
    if (!pHandle) {
        return;
    }
    pHandle->setContent(pContent);
    // QSplitter keeps the handle thickness from its last layout, and the
    // sizeHint() has just changed
    refresh();
}

bool GripSplitter::setHandleResizes(const int index, const bool resizes)
{
    auto* pHandle = qobject_cast<GripSplitterHandle*>(handle(index));
    if (!pHandle) {
        return false;
    }
    pHandle->setResizes(resizes);
    return true;
}

bool GripSplitter::handleResizes(const int index) const
{
    auto* pHandle = qobject_cast<const GripSplitterHandle*>(handle(index));
    return pHandle && pHandle->resizes();
}

} // namespace uiDesign
