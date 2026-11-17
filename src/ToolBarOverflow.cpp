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

#include "ToolBarOverflow.h"

#include "uiDesign.h"
#include "utils.h"

#include <QAction>
#include <QEvent>
#include <QFontMetrics>
#include <QLayout>
#include <QMenu>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

namespace uiDesign {

void ToolBarOverflow::restyle(QToolBar* pBar, const ThemeTokens& tokens)
{
    if (!pBar) {
        return;
    }
    auto* pOverflow = pBar->findChild<ToolBarOverflow*>(QString(), Qt::FindDirectChildrenOnly);
    if (!pOverflow) {
        auto* pButton = pBar->findChild<QToolButton*>(qsl("qt_toolbar_ext_button"), Qt::FindDirectChildrenOnly);
        if (!pButton) {
            return;
        }
        pOverflow = new ToolBarOverflow(pBar, pButton);
    }
    pOverflow->applyGlyph(tokens);
    pOverflow->queueRefresh();
}

QString ToolBarOverflow::word(const int hiddenCount)
{
    //: On a toolbar too narrow for all its buttons: the button that shows the rest, with the number hidden behind it. Keep it short, it has to fit under an icon
    return tr("%n more", nullptr, hiddenCount);
}

ToolBarOverflow::ToolBarOverflow(QToolBar* pBar, QToolButton* pButton)
: QObject(pBar)
, mpBar(pBar)
, mpButton(pButton)
{
    pBar->installEventFilter(this);
    pButton->installEventFilter(this);
    pButton->setProperty(scmProp_toolBarOverflowOwned, true);
    connect(pBar, &QToolBar::iconSizeChanged, this, &ToolBarOverflow::queueRefresh);
    connect(pBar, &QToolBar::toolButtonStyleChanged, this, &ToolBarOverflow::queueRefresh);
    connect(pButton, &QToolButton::toggled, this, &ToolBarOverflow::queueRefresh);
    connect(pBar, &QToolBar::orientationChanged, this, [this]() {
        mGlyphStale = true;
        queueRefresh();
    });
}

void ToolBarOverflow::applyGlyph(const ThemeTokens& tokens)
{
    // The row opens below a bar across the window, and the column beside one down its edge
    const bool across = mpBar->orientation() == Qt::Horizontal;
    const QString closed = across ? qsl(":/icons/toolbar-more-down.svg") : qsl(":/icons/toolbar-more-right.svg");
    const QString opened = across ? qsl(":/icons/toolbar-more-up.svg") : qsl(":/icons/toolbar-more-left.svg");
    mpButton->setIcon(tintedIcon(closed, opened, tokens));
    mGlyphStale = false;
}

bool ToolBarOverflow::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    switch (pEvent->type()) {
    case QEvent::LayoutDirectionChange:
        // QToolBarExtension puts the style's own arrow back on this event, after this filter
        mGlyphStale = true;
        queueRefresh();
        break;
    case QEvent::StyleChange:
    case QEvent::FontChange:
        mFitFor.clear();
        queueRefresh();
        break;
    case QEvent::Resize:
        if (pWatched == mpButton) {
            widenToTheWord();
        }
        queueRefresh();
        break;
    case QEvent::LanguageChange:
    case QEvent::LayoutRequest:
    case QEvent::Show:
    case QEvent::ActionAdded:
    case QEvent::ActionChanged:
    case QEvent::ActionRemoved:
        queueRefresh();
        break;
    default:
        break;
    }
    return QObject::eventFilter(pWatched, pEvent);
}

// Queued: Qt shows and hides the bar's buttons while it lays them out, and the count is only true afterwards
void ToolBarOverflow::queueRefresh()
{
    if (mRefreshQueued) {
        return;
    }
    mRefreshQueued = true;
    QTimer::singleShot(0, this, &ToolBarOverflow::refresh);
}

void ToolBarOverflow::refresh()
{
    mRefreshQueued = false;
    if (!mpBar || !mpButton) {
        return;
    }
    if (mGlyphStale) {
        applyGlyph(themeTokens());
    }
    fitToBar();
    showHiddenCount();
    styleItsMenu();
}

// A separator or a spacer folded away is not a button anyone is missing; a bare QWidget with a layout holds something
QList<QAction*> ToolBarOverflow::countedActions() const
{
    QList<QAction*> counted;
    for (QAction* pAction : mpBar->actions()) {
        const QWidget* pWidget = mpBar->widgetForAction(pAction);
        if (pWidget && pAction->isVisible() && !pAction->isSeparator() && (pWidget->metaObject() != &QWidget::staticMetaObject || pWidget->layout())) {
            counted.append(pAction);
        }
    }
    return counted;
}

void ToolBarOverflow::fitToBar()
{
    // The word at its longest, so that a count changing cannot move the point where the bar folds
    const QFontMetrics metrics(mpButton->font());
    QString longest = word(1);
    const int most = static_cast<int>(countedActions().size());
    for (int count = 2; count <= most; ++count) {
        if (const QString candidate = word(count); metrics.horizontalAdvance(candidate) > metrics.horizontalAdvance(longest)) {
            longest = candidate;
        }
    }

    const QString fitFor = qsl("%1|%2|%3x%4|%5|%6")
                                   .arg(longest)
                                   .arg(static_cast<int>(mpBar->toolButtonStyle()))
                                   .arg(mpBar->iconSize().width())
                                   .arg(mpBar->iconSize().height())
                                   .arg(static_cast<int>(mpBar->orientation()))
                                   .arg(mpButton->font().toString());
    if (fitFor == mFitFor) {
        return;
    }
    mFitFor = fitFor;

    // Neither follows the bar: only the buttons QToolBarLayout makes for itself are connected to it. Down a window's
    // edge the button only has the bar's width, which a word beside the glyph can be too wide for
    const bool downAnEdge = mpBar->orientation() == Qt::Vertical;
    mpButton->setToolButtonStyle(downAnEdge && mpBar->toolButtonStyle() == Qt::ToolButtonTextBesideIcon ? Qt::ToolButtonTextUnderIcon : mpBar->toolButtonStyle());
    mpButton->setIconSize(mpBar->iconSize());

    // QToolBarExtension::sizeHint() answers the metric this is working out
    const QString shown = mpButton->text();
    mpButton->setText(longest);
    const QSize needed = mpButton->QToolButton::sizeHint();
    mpButton->setText(shown);

    // Down an edge QToolBarLayout gives the button the width of the widest button in the column, which the word under
    // the glyph can be wider than; the bar is widened to hold it, and the button after each layout pass
    mWordWidthDownAnEdge = downAnEdge ? needed.width() : 0;
    const QMargins barMargins = mpBar->layout() ? mpBar->layout()->contentsMargins() : QMargins();
    mpBar->setMinimumWidth(downAnEdge ? needed.width() + barMargins.left() + barMargins.right() : 0);
    widenToTheWord();

    // The sheet takes the side padding off for the word's sake, which would leave a lone glyph too small to aim at
    const int glyphPadding = mpBar->toolButtonStyle() == Qt::ToolButtonIconOnly ? 2 * scmToolBarButtonPaddingHorizontal : 0;
    const int extent = downAnEdge ? needed.height() : needed.width() + glyphPadding;
    if (mpBar->property(scmProp_toolBarOverflowExtent).toInt() != extent) {
        mpBar->setProperty(scmProp_toolBarOverflowExtent, extent);
        if (QLayout* pLayout = mpBar->layout()) {
            pLayout->invalidate();
        }
        mpBar->updateGeometry();
    }
}

void ToolBarOverflow::widenToTheWord()
{
    if (mpButton && mpButton->width() < mWordWidthDownAnEdge) {
        mpButton->resize(mWordWidthDownAnEdge, mpButton->height());
    }
}

void ToolBarOverflow::showHiddenCount()
{
    // While the row is open every button is shown, and Qt folds the same ones again when it closes
    if (mpButton->isChecked()) {
        //: Tooltip on a toolbar's overflow button while the buttons that did not fit are on show in a row of their own
        mpButton->setToolTip(utils::richText(tr("Click to fold this toolbar's extra buttons away again.")));
        return;
    }
    int hidden = 0;
    for (QAction* pAction : countedActions()) {
        hidden += mpBar->widgetForAction(pAction)->isHidden() ? 1 : 0;
    }
    if (const QString text = word(hidden); mpButton->text() != text) {
        mpButton->setText(text);
        mpButton->setAccessibleName(text);
    }
    //: Tooltip on a toolbar's overflow button, whose label is only "3 more"
    mpButton->setToolTip(utils::richText(tr("The window is too narrow for %n of this toolbar's buttons. Click to show them.", nullptr, hidden)));
}

// Qt swaps the row for this menu when a second row would not fit in the window, and makes the menu at that moment
void ToolBarOverflow::styleItsMenu()
{
    QMenu* pMenu = mpButton->menu();
    if (!pMenu) {
        return;
    }
    if (const QString sheet = menuStyleSheet(themeTokens()); pMenu->styleSheet() != sheet) {
        pMenu->setStyleSheet(sheet);
        letPopupsTakeTheFieldsCorner(pMenu);
    }
}

} // namespace uiDesign
