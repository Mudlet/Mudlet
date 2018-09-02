/***************************************************************************
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
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

TDockWidget::TDockWidget(Host* pH, const QString& consoleName) : QDockWidget()
{
    mpHost = pH;
    hasLayoutAlready = false;
    widgetConsoleName = consoleName;
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
