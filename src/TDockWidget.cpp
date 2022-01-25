/***************************************************************************
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2019-2020, 2022 by Stephen Lyons                        *
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

#include "TDockWidget.h"

TDockWidget::TDockWidget(Host* pH, const QString& consoleName)
: QDockWidget()
, widgetConsoleName(consoleName)
, mpHost(pH)
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
        mpHost->hideWindow(widgetConsoleName);
        event->ignore();
        return;
    } else {
        event->accept();
        return;
    }
}

void TDockWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
    mpHost->setDockLayoutUpdated(widgetConsoleName);
}

void TDockWidget::moveEvent(QMoveEvent* event)
{
    Q_UNUSED(event)
    mpHost->setDockLayoutUpdated(widgetConsoleName);
}

void TDockWidget::setVisible(bool visible)
{
    if (!mpHost || !mpHost->mpConsole) {
        // During shutdown / profile closure TDockWidgets will get a hide event
        // as part of the underlying Qt class's built in handling of a close
        // event as the base class QDockWidget::setVisible(bool) method is being
        // overridden - at this point it seems there is not a Main Console left
        // to be used to look some stuff up in - so in that case - just hide
        // this widget and bail out:
        if (!visible) {
            QWidget::setVisible(false);
        }
        return;
    }
    auto pC = mpHost->mpConsole->mSubConsoleMap.value(widgetConsoleName);
    if (!pC) {
        return;
    }
    //do not change the ->show() order! Otherwise, it will automatically minimize the floating/dock window(!!)
    if (visible) {
        pC->show();
        QWidget::setVisible(true);
        mpHost->mpConsole->showWindow(widgetConsoleName);
    } else {
        QWidget::setVisible(false);
    }
}
