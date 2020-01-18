/***************************************************************************
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2019-2020 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "mudlet.h"

TDockWidget::TDockWidget(Host* pH, const QString& consoleName)
: QDockWidget()
, widgetConsoleName(consoleName)
, hasLayoutAlready(false)
, mpHost(pH)
, mpConsole(nullptr)
{
    if (titleBarWidget()) {
        mudlet::self()->recordAFontUse(titleBarWidget()->font());
    } else {
        mudlet::self()->recordAFontUse(this->QDockWidget::font());
    }
    slot_handleEmojiFontSubstitutionChanges();

    connect(this, &QDockWidget::topLevelChanged, this, &TDockWidget::slot_floatingStateChanged);
    connect(mudlet::self(), &mudlet::signal_fontSubstitutionIndexChanged, this, &TDockWidget::slot_handleEmojiFontSubstitutionChanges);
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

void TDockWidget::slot_floatingStateChanged(const bool isFloating)
{
    Q_UNUSED(isFloating);
    if (titleBarWidget()) {
        mudlet::self()->recordAFontUse(titleBarWidget()->font());
    }
}

void TDockWidget::slot_handleEmojiFontSubstitutionChanges()
{
    if (titleBarWidget()) {
        mudlet::self()->recordAFontUse(titleBarWidget()->font());
        QFont newFont(QFont(titleBarWidget()->font().family(), titleBarWidget()->font().pointSize(), titleBarWidget()->font().weight()));
        titleBarWidget()->setFont(QFont("Bitstream Vera Sans", 6, QFont::Light));
        titleBarWidget()->setFont(newFont);
        update();
        return;
    }

    // If there is no custom titlebar widget then, if floating, the titlebar
    // likely uses the font of the main widget (possibly being the application
    // font) which we can, anyhow, source from the base class's font:
    mudlet::self()->recordAFontUse(this->QDockWidget::font());
    QFont newFont(QFont(this->QDockWidget::font().family(), this->QDockWidget::font().pointSize(), this->QDockWidget::font().weight()));
    this->QDockWidget::setFont(QFont("Bitstream Vera Sans", 6, QFont::Light));
    this->QDockWidget::setFont(newFont);
    update();
}
