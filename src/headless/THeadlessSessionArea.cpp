/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

// The main window's area an app gives, and where in it the main console's text goes

#include "THeadlessSession.h"

#include "Host.h"

#include <QFontMetrics>
#include <QMetaMethod>

#include <algorithm>
#include <utility>

bool THeadlessSession::setMainWindowSize(const QSize& pixels)
{
    if (!usable()) {
        return false;
    }
    const QFontMetrics metrics(consoleFont());
    const qint64 columns = pixels.width() / std::max(1, metrics.averageCharWidth());
    const qint64 rows = pixels.height() / std::max(1, metrics.height());
    if (columns < 1 || columns > 65535 || rows < 1 || rows > 65535) {
        //: Error from a frontend with no window given a main window size that cannot be sent to the game; %1 is the width asked for and %2 the height, in pixels, %3 how many columns of the main console's font that holds and %4 how many rows
        mErrorString = tr("A main window of %1 by %2 pixels holds %3 columns by %4 rows of its font, which cannot be sent to the game: each must be from 1 to 65535.")
                               .arg(pixels.width())
                               .arg(pixels.height())
                               .arg(columns)
                               .arg(rows);
        return false;
    }
    if (!mpHost->noteMainAreaGiven(mAnsweringMainWindowRequest)) {
        return true;
    }
    // As a resize of the GUI's main window, which tells scripts nothing for the size they have
    if (mpHost->mainAreaInPixels() && mpHost->mainWindowSize() == pixels) {
        return true;
    }
    mpHost->setMainAreaPixels(pixels);
    // The first handler may close the profile, and a function the frontend bound into Lua delete the session
    mpHost->layOutMainAreaWithNoView(Host::MainAreaChange::area);
    return true;
}

QSize THeadlessSession::mainConsoleGrid() const
{
    return mpHost ? QSize(mpHost->mScreenWidth, mpHost->mScreenHeight) : QSize();
}

QRect THeadlessSession::mainConsoleTextRect() const
{
    return mpHost ? mpHost->mainTextRectWithNoView() : QRect();
}

void THeadlessSession::connectAreaRelays(Host* pHost, const unsigned int run)
{
    mDeliveredMainConsoleArea.reset();
    // Read at delivery, not as sent, so what a turn changed more than once is sent once
    connect(
            pHost,
            &Host::signal_mainAreaLaidOut,
            this,
            [this, run]() {
                if (run != mRun || !mpHost || !mDeliveredMainConsoleArea) {
                    return;
                }
                const std::pair area{mainConsoleGrid(), mainConsoleTextRect()};
                if (area == *mDeliveredMainConsoleArea) {
                    return;
                }
                mDeliveredMainConsoleArea = area;
                const QString profile = mProfileName;
                emit mainConsoleAreaChanged(profile, area.first, area.second);
            },
            Qt::QueuedConnection);
    connect(
            pHost,
            &Host::signal_mainWindowResizeRequested,
            this,
            [this, run](const QSize& pixels, const quint64 request) {
                if (run != mRun || !mpHost) {
                    return;
                }
                // With no app to answer, no size it gives later is to be taken as an answer
                if (!isSignalConnected(QMetaMethod::fromSignal(&THeadlessSession::mainWindowResizeRequested))) {
                    mpHost->noteMainWindowRequestUnheard(request);
                    return;
                }
                const QString profile = mProfileName;
                // The handler may delete the session
                const QPointer<THeadlessSession> self(this);
                const auto outer = std::exchange(mAnsweringMainWindowRequest, std::pair{pixels, request});
                emit mainWindowResizeRequested(profile, pixels);
                if (self) {
                    mAnsweringMainWindowRequest = outer;
                }
            },
            Qt::QueuedConnection);
}

void THeadlessSession::noteAreaAtLoad()
{
    mDeliveredMainConsoleArea = {mainConsoleGrid(), mainConsoleTextRect()};
}
