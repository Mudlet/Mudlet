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

#include "TTabBar.h"

#include "uiDesign.h"
#include "utils.h"

#include <QApplication>
#include <QEvent>
#include <QStyleOption>
#include <QStyleOptionTab>
#include <QPainter>
#include <QPen>
#include <QVariant>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QScreen>
#include <QDateTime>

// Constants for improved drag detection
static const int VERTICAL_MOVEMENT_RATIO_THRESHOLD = 60; // Percentage of movement that must be vertical
static const int TAB_REORDER_DELAY_MS = 150;             // Delay before allowing tab detachment

// A script's sheet that names QTabBar, through setAppStyleSheet() or setProfileStyleSheet(), keeps the last word over
// the strip
bool TStyle::scriptSheetTakesTheTabs() const
{
    return qApp->styleSheet().contains(qsl("QTabBar")) || (mpTabBar && mpTabBar->styleSheet().contains(qsl("QTabBar")));
}

// Qt paints a reordered tab's drag pixmap with the rect moved to the origin, so tabAt() on it
// names the wrong tab or none; tabIndex is set since Qt 6.8
int TStyle::tabIndexOf(const QStyleOptionTab* tabOption) const
{
    if (!tabOption) {
        return -1;
    }
    if (tabOption->tabIndex >= 0) {
        return tabOption->tabIndex;
    }
    return mpTabBar ? mpTabBar->tabAt(tabOption->rect.center()) : -1;
}

// Each tab leads with scmTabStripInset of the gap and trails the rest, so the first name stands where it always has and
// every name after it a whole gap on from the one before
TStyle::TabLayout TStyle::layoutTab(const QStyleOptionTab* tabOption) const
{
    TabLayout layout;
    if (!tabOption) {
        return layout;
    }

    const QRect bounds = tabOption->rect;
    const QRect content = bounds.adjusted(uiDesign::scmTabStripInset, 0, uiDesign::scmTabStripInset - uiDesign::scmTabGap, 0);
    // The word's own line, clear of the air and the underline below it
    const QRect line(content.left(), bounds.top() + uiDesign::scmTabPaddingTop, content.width(), qMax(0, bounds.height() - uiDesign::scmTabPaddingTop - uiDesign::scmTabPaddingBottom));

    int left = content.left();
    int right = content.right();

    QRect indicator;
    if (tabConnectionIndicator(tabIndexOf(tabOption)) != TabConnectionIndicator::None) {
        indicator = QRect(left, line.top() + (line.height() - sIndicatorDiameter) / 2, sIndicatorDiameter, sIndicatorDiameter);
        left += sIndicatorReservedWidth;
    }

    QRect close;
    if (!tabOption->rightButtonSize.isEmpty()) {
        const QSize wanted = tabOption->rightButtonSize;
        close = QRect(content.right() - wanted.width() + 1, line.top() + (line.height() - wanted.height()) / 2, wanted.width(), wanted.height());
        right = close.left() - 1 - uiDesign::scmTabCloseGap;
    }

    const QRect text(left, line.top(), qMax(0, right - left + 1), line.height());

    layout.content = QStyle::visualRect(tabOption->direction, bounds, content);
    layout.indicator = indicator.isNull() ? indicator : QStyle::visualRect(tabOption->direction, bounds, indicator);
    layout.text = QStyle::visualRect(tabOption->direction, bounds, text);
    layout.close = close.isNull() ? close : QStyle::visualRect(tabOption->direction, bounds, close);
    return layout;
}

// New output is marked in the full text ink as well as by its weight, so the mark reads louder than a quiet tab; the
// chosen tab's accent word and underline keep the two apart, and a hovered word takes the accent without the underline
QColor TStyle::wordInk(const QStyleOptionTab* tabOption, const uiDesign::ThemeTokens& tokens) const
{
    if (!(tabOption->state & State_Enabled)) {
        return tokens.disabledText;
    }
    if (tabOption->state & State_Selected) {
        return uiDesign::readableOn(tokens.page, tokens.accentText, tokens.text, uiDesign::scmTextMinimumRatio);
    }
    if (tabOption->state & State_MouseOver) {
        return tokens.accentText;
    }
    const QString tabName = mpTabBar ? mpTabBar->tabData(tabIndexOf(tabOption)).toString() : QString();
    if (mBoldTabsSet.contains(tabName) || mItalicTabsSet.contains(tabName) || mUnderlineTabsSet.contains(tabName)) {
        return tokens.text;
    }
    return tokens.mutedText;
}

void TStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    const auto* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
    if (tabOption && (element == CE_TabBarTab || element == CE_TabBarTabShape || element == CE_TabBarTabLabel)) {
        if (scriptSheetTakesTheTabs()) {
            paintThroughApplicationSheet(element, tabOption, painter, widget);
        } else {
            paintTab(painter, tabOption, widget, element != CE_TabBarTabLabel, element != CE_TabBarTabShape);
        }
        return;
    }

    // Delegate to the application style (DarkTheme) rather than our proxy base
    // style, which resolves to a separate Fusion instance that doesn't have
    // dark palette colors on Windows 10
    qApp->style()->drawControl(element, option, painter, widget);
}

void TStyle::paintThroughApplicationSheet(const ControlElement element, const QStyleOptionTab* tabOption, QPainter* painter, const QWidget* widget) const
{
    const int index = tabIndexOf(tabOption);
    const QString tabName = mpTabBar ? mpTabBar->tabData(index).toString() : QString();
    painter->save();
    // Only for a marked tab: the painter already carries the font a sheet's QTabBar::tab rule sets
    if (mBoldTabsSet.contains(tabName) || mItalicTabsSet.contains(tabName) || mUnderlineTabsSet.contains(tabName)) {
        QFont font = widget ? widget->font() : painter->font();
        font.setBold(mBoldTabsSet.contains(tabName));
        font.setItalic(mItalicTabsSet.contains(tabName));
        font.setUnderline(mUnderlineTabsSet.contains(tabName));
        painter->setFont(font);
    }
    qApp->style()->drawControl(element, tabOption, painter, widget);
    painter->restore();

    const TabConnectionIndicator state = tabConnectionIndicator(index);
    if (element != CE_TabBarTab || state == TabConnectionIndicator::None) {
        return;
    }
    const QRect bounds = tabOption->rect;
    const QRect box(bounds.left() + sIndicatorInsetUnderScriptSheet, bounds.top() + (bounds.height() - sIndicatorDiameter) / 2, sIndicatorDiameter, sIndicatorDiameter);
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    paintConnectionIndicator(painter, QStyle::visualRect(tabOption->direction, bounds, box), state, tokens, tokens.mutedText);
}

void TStyle::paintTab(QPainter* painter, const QStyleOptionTab* tabOption, const QWidget* widget, const bool shape, const bool label) const
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const TabLayout layout = layoutTab(tabOption);
    const bool chosen = tabOption->state & State_Selected;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Over the hairline along the bar's bottom row; from the keyboard it grows up into the air under the word
    if (shape && chosen) {
        const int thickness = (tabOption->state & State_HasFocus) ? uiDesign::scmTabFocusUnderlineWidth : uiDesign::scmTabUnderlineWidth;
        painter->fillRect(QRect(layout.content.left(), tabOption->rect.bottom() - thickness + 1, layout.content.width(), thickness), tokens.accent);
    }

    if (label) {
        const QString tabName = mpTabBar ? mpTabBar->tabData(tabIndexOf(tabOption)).toString() : QString();
        QFont font = widget ? widget->font() : painter->font();
        if (mBoldTabsSet.contains(tabName)) {
            font.setBold(true);
        } else if (chosen) {
            font.setWeight(uiDesign::scmTabChosenWeight);
        }
        font.setItalic(mItalicTabsSet.contains(tabName));
        font.setUnderline(mUnderlineTabsSet.contains(tabName));
        painter->setFont(font);

        const QColor ink = wordInk(tabOption, tokens);
        painter->setPen(ink);
        const int mnemonic = qApp->style()->styleHint(SH_UnderlineShortcut, tabOption, widget) ? Qt::TextShowMnemonic : Qt::TextHideMnemonic;
        // From the leading edge: every tab is measured bold, so a lighter word centred in its box would stand off the line
        painter->drawText(layout.text, QStyle::visualAlignment(tabOption->direction, Qt::AlignLeft | Qt::AlignVCenter) | mnemonic, tabOption->text);

        const TabConnectionIndicator state = tabConnectionIndicator(tabIndexOf(tabOption));
        if (state != TabConnectionIndicator::None && !layout.indicator.isNull()) {
            paintConnectionIndicator(painter, layout.indicator, state, tokens, ink);
        }
    }

    painter->restore();
}

void TStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    if (element == PE_IndicatorTabClose && option && scriptSheetTakesTheTabs()) {
        qApp->style()->drawPrimitive(element, option, painter, widget);
        return;
    }

    if (element == PE_IndicatorTabClose && option) {
        // Only the option: through QStyleSheetStyle (a profile sheet on the bar) the widget
        // is the tab bar, not the close button that asked
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const bool lit = option->state & (State_Raised | State_Sunken);
        const QColor ink = !(option->state & State_Enabled) ? tokens.disabledText : (lit ? tokens.accentText : tokens.mutedText);
        const qreal ratio = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
        const QPixmap cross = uiDesign::themedGlyphPixmap(qsl(":/icons/editor-clear.svg"), ink, uiDesign::scmTabCloseBoxSize, uiDesign::scmTabCloseGlyphSize, ratio);
        if (!cross.isNull()) {
            const QSize drawn = cross.deviceIndependentSize().toSize();
            painter->drawPixmap(QPoint(option->rect.left() + (option->rect.width() - drawn.width()) / 2, option->rect.top() + (option->rect.height() - drawn.height()) / 2), cross);
        }
        return;
    }

    if (element == PE_FrameTabBarBase) {
        // A script's sheet has always drawn its tabs with no base under them
        if (option && !scriptSheetTakesTheTabs()) {
            uiDesign::paintTabStripHairline(painter, option->rect, uiDesign::themeTokens());
        }
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

QRect TStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
    const auto* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
    if (tabOption && scriptSheetTakesTheTabs()) {
        return qApp->style()->subElementRect(element, option, widget);
    }
    if (tabOption) {
        switch (element) {
        case SE_TabBarTabText:
            return layoutTab(tabOption).text;
        case SE_TabBarTabRightButton:
            return layoutTab(tabOption).close;
        case SE_TabBarTabLeftButton:
            return QRect();
        default:
            break;
        }
    }

    return QProxyStyle::subElementRect(element, option, widget);
}

// Sizes must come from the style that paints. The rest goes to qApp->style(): our proxy base is a
// separate instance of the platform's native style and would size a look nothing here draws
QSize TStyle::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget) const
{
    if (type == CT_TabBarTab && !scriptSheetTakesTheTabs()) {
        // QTabBar has already added the text, the frame and the buttons
        return contentsSize;
    }
    return qApp->style()->sizeFromContents(type, option, contentsSize, widget);
}

int TStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    if (scriptSheetTakesTheTabs()) {
        return qApp->style()->pixelMetric(metric, option, widget);
    }

    switch (metric) {
    case PM_TabBarTabHSpace:
        return uiDesign::scmTabGap;
    case PM_TabBarTabVSpace:
        return uiDesign::scmTabPaddingTop + uiDesign::scmTabPaddingBottom;
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        return uiDesign::scmTabCloseBoxSize;
    case PM_TabBarTabOverlap:
    case PM_TabBarTabShiftHorizontal:
    case PM_TabBarTabShiftVertical:
    case PM_TabBarBaseHeight:
    case PM_TabBarBaseOverlap:
        return 0;
    default:
        return qApp->style()->pixelMetric(metric, option, widget);
    }
}

int TStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    switch (hint) {
    case SH_TabBar_CloseButtonPosition:
        // macOS alone would put the cross in front of the word
        return QTabBar::RightSide;
    case SH_TabBar_Alignment:
        return Qt::AlignLeft;
    default:
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

// Walked against the page: no tab is filled, so that is what every indicator stands on
void TStyle::paintConnectionIndicator(QPainter* painter, const QRect& box, TabConnectionIndicator state, const uiDesign::ThemeTokens& tokens, const QColor& ringInk) const
{
    const auto stateInk = [&tokens](const qreal hue) {
        return uiDesign::readableOn(tokens.page, uiDesign::stateColor(hue, tokens.darkPage), tokens.text, uiDesign::scmQuietMinimumRatio);
    };

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    switch (state) {
    case TabConnectionIndicator::Connected:
        painter->setBrush(stateInk(uiDesign::scmStateHue_ok));
        painter->drawEllipse(box);
        break;
    case TabConnectionIndicator::Connecting:
        painter->setBrush(stateInk(uiDesign::scmStateHue_warning));
        painter->drawEllipse(box);
        break;
    case TabConnectionIndicator::Error: {
        painter->setBrush(stateInk(uiDesign::scmStateHue_error));
        QPolygon triangle;
        triangle << QPoint(box.center().x(), box.top()) << QPoint(box.left(), box.bottom()) << QPoint(box.right(), box.bottom());
        painter->drawPolygon(triangle);
        break;
    }
    case TabConnectionIndicator::Disconnected:
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(ringInk, sIndicatorRingWidth));
        painter->drawEllipse(box);
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
    // Removing a stale indicator is always safe (the tab may already be gone,
    // e.g. from removeTab); for inserts, refuse unknown names so we don't
    // accumulate phantom entries.
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
    if (!mpTabBar || index < 0 || index >= mpTabBar->count()) {
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
    if (!mpTabBar || index < 0 || index >= mpTabBar->count()) {
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

    // Every tab is measured bold, as new output marks it, which is wider than the chosen tab's semibold, so the
    // strip does not shift for either. Sizing uses tabText(), which Qt may have given an accelerator, not tabData.
    const QFontMetrics fm(font());
    const int w = fm.horizontalAdvance(tabText(index));

    QFont f = font();
    f.setBold(true);
    f.setItalic(mStyle.tabItalic(index));
    f.setUnderline(mStyle.tabUnderline(index));
    const QFontMetrics bfm(f);

    const int bw = bfm.horizontalAdvance(tabText(index));

    s.setWidth(s.width() - w + bw);

    // Reserve room for the connection indicator we paint ourselves
    // (see TabConnectionIndicator declaration for why).
    if (mStyle.tabConnectionIndicator(index) != TabConnectionIndicator::None) {
        s.setWidth(s.width() + TStyle::sIndicatorReservedWidth);
    }

    return s;
}

bool TTabBar::adoptCloseButton(const int index)
{
    auto* pClose = tabButton(index, QTabBar::RightSide);
    if (!pClose || pClose->testAttribute(Qt::WA_SetStyle)) {
        return false;
    }
    pClose->setStyle(&mStyle);
    pClose->resize(pClose->sizeHint());
    return true;
}

void TTabBar::changeEvent(QEvent* event)
{
    // A script's sheet arrives or goes, on the bar or on the application, with a StyleChange
    if (event->type() == QEvent::StyleChange) {
        expandOnlyUnderAScriptSheet();
    }
    QTabBar::changeEvent(event);
}

// QTabBar::insertTab() sizes the cross from the application style and lays out before calling
// this; the refresh re-measures the tabs, which reads the adopted button's current size()
void TTabBar::tabInserted(int index)
{
    if (adoptCloseButton(index)) {
        refreshAfterApplicationStyleChange();
    }
    QTabBar::tabInserted(index);
}

// QApplication::setStyle() delivers StyleChange only to widgets without
// WA_SetStyle, and installing our TStyle sets that attribute - so the bar
// keeps tab sizes computed against the previous application style unless
// we hand it the event it was skipped for.
void TTabBar::refreshAfterApplicationStyleChange()
{
    // QTabBar::setTabsClosable() makes crosses without calling tabInserted()
    for (int i = 0, total = count(); i < total; ++i) {
        adoptCloseButton(i);
        // An application sheet may just have taken the tabs or let them go
        if (auto* pClose = tabButton(i, QTabBar::RightSide)) {
            pClose->resize(pClose->sizeHint());
        }
    }
    QEvent event(QEvent::StyleChange);
    QApplication::sendEvent(this, &event);
    updateGeometry();
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

void TTabBar::markActivity(const QString& tabName, const bool isLowerPriorityChange)
{
    const int index = tabIndex(tabName);
    if (count() < 2 || index < 0) {
        return;
    }
    if (!isLowerPriorityChange) {
        if (tabBold(index) && !tabItalic(index)) {
            return;
        }
        setTabBold(index, true);
        setTabItalic(index, false);
    } else {
        // A lower priority, local change must not replace the indication of a
        // higher priority remote one:
        if (tabBold(index) || tabItalic(index)) {
            return;
        }
        setTabItalic(index, true);
    }
    relayoutTab(index);
}

void TTabBar::clearActivity(const int index)
{
    if (!tabBold(index) && !tabItalic(index) && !tabUnderline(index)) {
        return;
    }
    setTabBold(index, false);
    setTabItalic(index, false);
    setTabUnderline(index, false);
    relayoutTab(index);
}

void TTabBar::relayoutTab(const int index)
{
    // QTabBar caches each tab's size and only asks tabSizeHint() again from its
    // private refresh(), which setTabText() triggers - without it a tab whose
    // font just became wider would clip its text:
    QTabBar::setTabText(index, tabText(index));
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
        setTabText(index, qsl("%1%2").arg(prefix, tabData(index).toString()));
    }
}

void TTabBar::applyPrefixToDisplayedText(int index, const QString& prefix)
{
    if (index > -1) {
        setTabText(index, qsl("%1%2").arg(prefix, tabData(index).toString()));
    }
}

void TTabBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mDragStartPos = event->pos();
        mDragTabName = tabData(tabAt(event->pos())).toString();
        mDragStartTime = QDateTime::currentMSecsSinceEpoch();
        mPendingDetach = false;
    }
    QTabBar::mousePressEvent(event);
}

void TTabBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mDragTabName.clear();
    }
    QTabBar::mouseReleaseEvent(event);
}

void TTabBar::mouseMoveEvent(QMouseEvent* event)
{
    // Check if we should start a drag operation
    if (!(event->buttons() & Qt::LeftButton) || mDragTabName.isEmpty()) {
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

            // totalDistance is from the press point, not the bar's centre, so the tear-off threshold doesn't
            // depend on where the tab sits or was grabbed. It is Manhattan: horizontal travel counts too.
            if (!tabBarGlobalRect.contains(globalPos) && isVerticalMovement && totalDistance > DETACH_DISTANCE_THRESHOLD) {
                const int dragIndex = tabIndex(mDragTabName);
                mDragTabName.clear();
                if (dragIndex > -1) {
                    emit tabDetachRequested(dragIndex, globalPos);
                }
                return;
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
