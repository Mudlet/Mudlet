/***************************************************************************
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2019 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "TDockWidget.h"

TDockWidget::TDockWidget(Host* pH, const QString& consoleName)
: QDockWidget()
, widgetConsoleName(consoleName)
, hasLayoutAlready(false)
, mpHost(pH)
, mpConsole(nullptr)
{
}

// This sets the mutual pointers that the TConsole and the TDockWidget now
// have for each other as well as assigning the TConsole to be the TDockWidget's
// widget:
void TDockWidget::setTConsole(TConsole* pC)
{
    mpConsole = pC;
    setWidget(pC);
    pC->mpDockWidget = this;
}

void TDockWidget::closeEvent(QCloseEvent* event)
{
    if (!mpHost->isClosingDown()) {
        mudlet::self()->hideWindow(mpHost, widgetConsoleName);
        event->ignore();
        return;
    } else {
        event->accept();
        return;
    }
}

void TDockWidget::resizeEvent(QResizeEvent* event)
{
    if (!mudlet::self()->mIsLoadingLayout) {
        mudlet::self()->setDockLayoutUpdated(mpHost, widgetConsoleName);
    }
}

void TDockWidget::moveEvent(QMoveEvent* event)
{
    if (!mudlet::self()->mIsLoadingLayout) {
        mudlet::self()->setDockLayoutUpdated(mpHost, widgetConsoleName);
    }
}

void TDockWidget::setVisible(bool visible)
{
    auto pC = mpHost->mpConsole->mSubConsoleMap.value(widgetConsoleName);
    auto pW = mpHost->mpConsole->mDockWidgetMap.value(widgetConsoleName);
    if (!pC || !pW) {
        return;
    }
    //do not change the ->show() order! Otherwise, it will automatically minimize the floating/dock window(!!)
    if (visible) {
        pC->show();
        pW->QWidget::setVisible(true);
        mpHost->mpConsole->showWindow(widgetConsoleName);
    } else {
        pW->QWidget::setVisible(false);
    }
}
