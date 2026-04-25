/***************************************************************************
 *   Copyright (C) 2018, 2020-2021 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

/***************************************************************************
 *   This class is entirely concerned with overcoming the inability to     *
 *   modify the text format for individual tabs in a QTabBar widget        *
 *   via stylesheets on an index or tab text basis.                        *
 ***************************************************************************/

#include "TTabBar.h"

#include "utils.h"

#include <QApplication>
#include <QStyleOption>
#include <QStyleOptionTab>
#include <QPainter>
#include <QVariant>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QScreen>
#include <QDateTime>

// Constants for improved drag detection
static const int DETACH_DISTANCE_THRESHOLD = 80;         // Pixels to drag before tab detaches
static const int DETACH_PIXEL_BUFFER = 20;               // Drag tolerance buffer
static const int VERTICAL_MOVEMENT_RATIO_THRESHOLD = 60; // Percentage of movement that must be vertical
static const int TAB_REORDER_DELAY_MS = 150;             // Delay before allowing tab detachment

void TStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    // Delegate to the application style (DarkTheme) rather than our proxy base
    // style, which resolves to a separate Fusion instance that doesn't have
    // dark palette colors on Windows 10
    auto* appStyle = qApp->style();

    if (element == QStyle::CE_TabBarTab && widget) {
        QString tabName = mpTabBar->tabData(mpTabBar->tabAt(option->rect.center())).toString();
        QFont font = widget->font();
        bool isStyleChanged = false;
        if (mBoldTabsSet.contains(tabName) || mItalicTabsSet.contains(tabName) || mUnderlineTabsSet.contains(tabName)) {
            painter->save();
            font.setBold(mBoldTabsSet.contains(tabName));
            font.setItalic(mItalicTabsSet.contains(tabName));
            font.setUnderline(mUnderlineTabsSet.contains(tabName));
            isStyleChanged = true;
            painter->setFont(font);
        }

        appStyle->drawControl(element, option, painter, widget);

        if (isStyleChanged) {
            painter->restore();
        }

    } else {
        appStyle->drawControl(element, option, painter, widget);
    }

    // Paint the connection-status indicator on top of the rendered tab.
    // We hook CE_TabBarTab (rather than CE_TabBarTabLabel) because the macOS
    // native style draws the whole tab in CE_TabBarTab without sub-dispatching
    // to CE_TabBarTabLabel - see issue #9213.
    // tabAt() may return -1 (e.g. style previewing); tabConnectionIndicator()
    // guards that case and returns None, which is then a no-op.
    if (element == QStyle::CE_TabBarTab) {
        const auto* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
        if (tabOption) {
            const TabConnectionIndicator state = tabConnectionIndicator(mpTabBar->tabAt(tabOption->rect.center()));
            if (state != TabConnectionIndicator::None) {
                paintConnectionIndicator(painter, tabOption, state);
            }
        }
    }
}

QRect TStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
    QRect rect = QProxyStyle::subElementRect(element, option, widget);

    // Reserve space at the leading edge of the tab for the connection
    // indicator so that long tab text is not painted underneath it.
    // tabAt() may return -1; tabConnectionIndicator() guards that case.
    if (element == SE_TabBarTabText) {
        const auto* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
        if (tabOption && mpTabBar) {
            const TabConnectionIndicator state = tabConnectionIndicator(mpTabBar->tabAt(tabOption->rect.center()));
            if (state != TabConnectionIndicator::None) {
                // Place reserved space after the leading close button (if any).
                const QRect leftBtn = QProxyStyle::subElementRect(SE_TabBarTabLeftButton, option, widget);
                const int reservationStart = leftBtn.isValid() ? leftBtn.right() + 1 : rect.left();
                rect.setLeft(qMax(rect.left(), reservationStart) + sIndicatorReservedWidth);
            }
        }
    }

    return rect;
}

void TStyle::paintConnectionIndicator(QPainter* painter, const QStyleOptionTab* tabOption, TabConnectionIndicator state) const
{
    constexpr int diameter = 8;
    constexpr int leftMargin = 6;
    const QRect& tabRect = tabOption->rect;
    // On macOS the close button sits at the leading edge of the tab; place the
    // indicator just to the right of it so they do not overlap. On platforms
    // without a leading close button the indicator falls back to the tab's
    // leading edge.
    const QRect leftBtn = QProxyStyle::subElementRect(SE_TabBarTabLeftButton, tabOption, mpTabBar);
    const int startX = leftBtn.isValid() ? leftBtn.right() + 1 : tabRect.left();
    const int x = startX + leftMargin;
    const int y = tabRect.center().y() - diameter / 2 + 1;
    const QRect dotRect(x, y, diameter, diameter);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    switch (state) {
    case TabConnectionIndicator::Connected:
        painter->setBrush(QColor(50, 180, 50));
        painter->setPen(QPen(QColor(40, 150, 40), 1));
        painter->drawEllipse(dotRect);
        break;
    case TabConnectionIndicator::Connecting:
        painter->setBrush(QColor(220, 180, 50));
        painter->setPen(QPen(QColor(180, 150, 40), 1));
        painter->drawEllipse(dotRect);
        break;
    case TabConnectionIndicator::Error: {
        painter->setBrush(QColor(220, 50, 50));
        painter->setPen(QPen(QColor(180, 40, 40), 1));
        QPolygon triangle;
        triangle << QPoint(dotRect.center().x(), dotRect.top()) << QPoint(dotRect.left(), dotRect.bottom()) << QPoint(dotRect.right(), dotRect.bottom());
        painter->drawPolygon(triangle);
        break;
    }
    case TabConnectionIndicator::Disconnected:
        painter->setBrush(Qt::transparent);
        painter->setPen(QPen(QColor(120, 120, 120), 2));
        painter->drawEllipse(dotRect);
        break;
    case TabConnectionIndicator::None:
        break;
    }

    painter->restore();
}

bool TStyle::setTabConnectionIndicator(const QString& tabName, TabConnectionIndicator state)
{
    if (tabName.isEmpty()) {
        return false;
    }
    // Removing a stale indicator must always be allowed (e.g. from removeTab),
    // even if the tab no longer exists. For inserts, verify the tab name
    // corresponds to a real tab to avoid leaking phantom entries.
    if (state == TabConnectionIndicator::None) {
        return mConnectionIndicators.remove(tabName) > 0;
    }
    bool textIsInATab = false;
    for (int i = 0, total = mpTabBar->count(); i < total; ++i) {
        if (mpTabBar->tabData(i).toString() == tabName) {
            textIsInATab = true;
            break;
        }
    }
    if (!textIsInATab) {
        return false;
    }
    const auto it = mConnectionIndicators.constFind(tabName);
    if (it != mConnectionIndicators.cend() && it.value() == state) {
        return false;
    }
    mConnectionIndicators.insert(tabName, state);
    return true;
}

bool TStyle::setTabConnectionIndicator(int index, TabConnectionIndicator state)
{
    if (index < 0 || index >= mpTabBar->count()) {
        return false;
    }
    return setTabConnectionIndicator(mpTabBar->tabData(index).toString(), state);
}

TabConnectionIndicator TStyle::tabConnectionIndicator(const QString& tabName) const
{
    if (tabName.isEmpty()) {
        return TabConnectionIndicator::None;
    }
    return mConnectionIndicators.value(tabName, TabConnectionIndicator::None);
}

TabConnectionIndicator TStyle::tabConnectionIndicator(int index) const
{
    if (index < 0 || index >= mpTabBar->count()) {
        return TabConnectionIndicator::None;
    }
    return tabConnectionIndicator(mpTabBar->tabData(index).toString());
}

void TStyle::setNamedTabState(const QString& tabName, const bool state, QSet<QString>& effect)
{
    bool textIsInATab = false;
    for (int i = 0, total = mpTabBar->count(); i < total; ++i) {
        if (mpTabBar->tabData(i).toString() == tabName) {
            textIsInATab = true;
            break;
        }
    }

    if (!textIsInATab) {
        return;
    }

    if (state) {
        effect.insert(tabName);
    } else {
        effect.remove(tabName);
    }
}

void TStyle::setIndexedTabState(const int index, const bool state, QSet<QString>& effect)
{
    if (index < 0 || index >= mpTabBar->count()) {
        return;
    }

    if (state) {
        effect.insert(mpTabBar->tabData(index).toString());
    } else {
        effect.remove(mpTabBar->tabData(index).toString());
    }
}

bool TStyle::namedTabState(const QString& tabName, const QSet<QString>& effect) const
{
    bool textIsInATab = false;
    for (int i = 0, total = mpTabBar->count(); i < total; ++i) {
        if (mpTabBar->tabData(i).toString() == tabName) {
            textIsInATab = true;
            break;
        }
    }

    if (!textIsInATab) {
        return false;
    }

    return effect.contains(tabName);
}

bool TStyle::indexedTabState(const int index, const QSet<QString>& effect) const
{
    if (index < 0 || index >= mpTabBar->count()) {
        return false;
    }

    return effect.contains(mpTabBar->tabData(index).toString());
}

QSize TTabBar::tabSizeHint(int index) const
{
    QSize s = QTabBar::tabSizeHint(index);

    if (mStyle.tabBold(index) || mStyle.tabItalic(index) || mStyle.tabUnderline(index)) {
        const QFontMetrics fm(font());
        // Note that this method must use (because it is associated with sizing
        // the text to show) the (possibly Qt modified to include an
        // accelarator) actual tabText and not the profile name that we have
        // stored in the tabData:
        const int w = fm.horizontalAdvance(tabText(index));

        QFont f = font();
        f.setBold(mStyle.tabBold(index));
        f.setItalic(mStyle.tabItalic(index));
        f.setUnderline(mStyle.tabUnderline(index));
        const QFontMetrics bfm(f);

        const int bw = bfm.horizontalAdvance(tabText(index));

        s.setWidth(s.width() - w + bw);
    }

    // Reserve space at the trailing edge for our manually-painted connection
    // indicator (issue #9213). We paint it ourselves rather than via
    // setTabIcon() so the native macOS tab style does not add its own padding.
    if (mStyle.tabConnectionIndicator(index) != TabConnectionIndicator::None) {
        s.setWidth(s.width() + TStyle::sIndicatorReservedWidth);
    }

    return s;
}

QString TTabBar::tabName(const int index) const
{
    QString tabName{tabData(index).toString()};
    return tabName;
}

int TTabBar::tabIndex(const QString& tabName) const
{
    int index = -1;
    if (tabName.isEmpty()) {
        return index;
    }
    const int total = count();
    while (++index < total) {
        if (!tabData(index).toString().compare(tabName)) {
            return index;
        }
    }
    return -1;
}

void TTabBar::removeTab(int index)
{
    if (index >= 0 && index < count()) {
        setTabBold(index, false);
        setTabItalic(index, false);
        setTabUnderline(index, false);
        setTabConnectionIndicator(index, TabConnectionIndicator::None);
        QTabBar::removeTab(index);
    }
}

void TTabBar::removeTab(const QString& tabName)
{
    const int index = tabIndex(tabName);
    if (index > -1) {
        setTabBold(index, false);
        setTabItalic(index, false);
        setTabUnderline(index, false);
        setTabConnectionIndicator(index, TabConnectionIndicator::None);
        QTabBar::removeTab(index);
    }
}

QStringList TTabBar::tabNames() const
{
    QStringList results;
    for (int i = 0, total = count(); i < total; ++i) {
        results << tabData(i).toString();
    }

    return results;
}

void TTabBar::applyPrefixToDisplayedText(const QString& tabName, const QString& prefix)
{
    const int index = tabIndex(tabName);
    if (index > -1) {
        QTabBar::setTabText(index, qsl("%1%2").arg(prefix, tabData(index).toString()));
    }
}

void TTabBar::applyPrefixToDisplayedText(int index, const QString& prefix)
{
    if (index > -1) {
        QTabBar::setTabText(index, qsl("%1%2").arg(prefix, tabData(index).toString()));
    }
}

void TTabBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mDragStartPos = event->pos();
        mDragIndex = tabAt(event->pos());
        mDragStartTime = QDateTime::currentMSecsSinceEpoch();
        mPendingDetach = false;
    }
    QTabBar::mousePressEvent(event);
}

void TTabBar::mouseMoveEvent(QMouseEvent* event)
{
    // Check if we should start a drag operation
    if (!(event->buttons() & Qt::LeftButton) || mDragIndex == -1) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    // Calculate movement vectors
    const QPoint movement = event->pos() - mDragStartPos;
    const int totalDistance = movement.manhattanLength();

    // Only proceed if we've moved enough to start considering detachment
    if (totalDistance >= QApplication::startDragDistance()) {
        // Calculate directional components
        const int horizontalDistance = qAbs(movement.x());
        const int verticalDistance = qAbs(movement.y());

        // Ensure we have enough time for Qt's tab reordering to be attempted first
        const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        const qint64 timeSincePress = currentTime - mDragStartTime;

        // Only consider detachment after the reorder delay has passed
        if (timeSincePress >= TAB_REORDER_DELAY_MS) {
            // Improved directional detection: require predominantly vertical movement
            // and sufficient distance from the tab bar
            bool isVerticalMovement = false;

            if (verticalDistance > 0) {
                const int verticalPercentage = (verticalDistance * 100) / (horizontalDistance + verticalDistance);
                isVerticalMovement = verticalPercentage >= VERTICAL_MOVEMENT_RATIO_THRESHOLD;
            }

            // Check if we're significantly outside the tab bar area
            const QPoint globalPos = mapToGlobal(event->pos());
            const QRect tabBarGlobalRect = QRect(mapToGlobal(rect().topLeft()), rect().size());

            // Calculate distance from tab bar with enhanced threshold
            if (!tabBarGlobalRect.contains(globalPos) && isVerticalMovement) {
                const QPoint distanceFromBar = globalPos - tabBarGlobalRect.center();
                const int distanceFromBarManhattan = distanceFromBar.manhattanLength();

                // Use the improved threshold and ensure it's primarily vertical movement
                if (distanceFromBarManhattan > DETACH_DISTANCE_THRESHOLD) {
                    emit tabDetachRequested(mDragIndex, globalPos);
                    mDragIndex = -1; // Reset drag state
                    return;
                }
            }
        }
    }

    // Always call the parent implementation to allow normal tab reordering
    QTabBar::mouseMoveEvent(event);
}

void TTabBar::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasFormat("application/x-mudlet-tab")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void TTabBar::dragMoveEvent(QDragMoveEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasFormat("application/x-mudlet-tab")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void TTabBar::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasFormat("application/x-mudlet-tab")) {
        const QString tabName = QString::fromUtf8(mimeData->data("application/x-mudlet-tab"));
        const int dropIndex = tabAt(event->position().toPoint());
        emit tabReattachRequested(tabName, dropIndex);
        event->acceptProposedAction();
    }
}

void TTabBar::onDetachedTabReattach(const QString& tabName)
{
    // This slot can be connected to detached windows for reattachment
    const int insertIndex = count(); // Insert at end by default
    emit tabReattachRequested(tabName, insertIndex);
}
