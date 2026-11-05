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

#include "THeadlessSession.h"

#include "Host.h"
#include "TConsoleModel.h"

THeadlessSession::ConsoleWrap THeadlessSession::consoleWrap() const
{
    if (!mpHost) {
        return {};
    }
    const TConsoleModel& model = mpHost->mainConsoleModel();
    return {model.mWrapAt, model.mIndentCount, model.mHangingIndentCount};
}

QColor THeadlessSession::consoleForeground() const
{
    return mpHost ? mpHost->mainConsoleModel().mFgColor : QColor();
}

QColor THeadlessSession::consoleBackground() const
{
    return mpHost ? mpHost->mainConsoleModel().mBgColor : QColor();
}

void THeadlessSession::connectOutputRelays(Host* pHost, const unsigned int run)
{
    mDeliveredConsoleWrap.reset();
    mDeliveredConsoleColors.reset();
    // Read at delivery, not as sent: the load sets each of the three in turn, and consoleWrap() gave the last already
    connect(
            &pHost->mainConsoleModel().mNotifier,
            &TConsoleModelNotifier::wrapChanged,
            this,
            [this, run]() {
                if (run != mRun || !mpHost || !mDeliveredConsoleWrap) {
                    return;
                }
                const ConsoleWrap wrap = consoleWrap();
                if (wrap == *mDeliveredConsoleWrap) {
                    return;
                }
                mDeliveredConsoleWrap = wrap;
                const QString profile = mProfileName;
                emit consoleWrapChanged(profile, wrap.wrapAt, wrap.indent, wrap.hangingIndent);
            },
            Qt::QueuedConnection);
    // A change of the game's palette sends this too, and leaves both colours as they were
    connect(
            &pHost->mainConsoleModel().mNotifier,
            &TConsoleModelNotifier::colorsChanged,
            this,
            [this, run]() {
                if (run != mRun || !mpHost || !mDeliveredConsoleColors) {
                    return;
                }
                const std::pair colors{consoleForeground(), consoleBackground()};
                if (colors == *mDeliveredConsoleColors) {
                    return;
                }
                mDeliveredConsoleColors = colors;
                const QString profile = mProfileName;
                emit consoleColorsChanged(profile, colors.first, colors.second);
            },
            Qt::QueuedConnection);
}

void THeadlessSession::noteOutputStateAtLoad()
{
    mDeliveredConsoleWrap = consoleWrap();
    mDeliveredConsoleColors = {consoleForeground(), consoleBackground()};
}
