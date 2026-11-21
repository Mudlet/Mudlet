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

#include <QTimer>

namespace {
// As TMainConsole paces its latency box
constexpr int csmLatencyBoxPaceMs = 16;
} // namespace

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
    mDeliveredLatency.clear();
    // Read at delivery, not as sent: the load sets each of the three in turn, and consoleWrap() gave the last already
    connect(
            &pHost->mainConsoleModel().mNotifier,
            &TConsoleModelNotifier::wrapChanged,
            this,
            [this, run]() {
                // A shutdown() runs the event loop as the profiles close
                if (mShuttingDown || run != mRun || !mpHost || !mDeliveredConsoleWrap) {
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
                if (mShuttingDown || run != mRun || !mpHost || !mDeliveredConsoleColors) {
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
    connect(&pHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::incomingTextProcessed, this, [this, run]() {
        if (!mpLatencyPacer) {
            mpLatencyPacer = new QTimer(this);
            mpLatencyPacer->setSingleShot(true);
            mpLatencyPacer->setInterval(csmLatencyBoxPaceMs);
            connect(mpLatencyPacer, &QTimer::timeout, this, &THeadlessSession::sendLatency);
        }
        if (!mpLatencyPacer->isActive()) {
            mLatencyPaceRun = run;
            mpLatencyPacer->start();
        }
    });
    connect(
            &pHost->mTelnet,
            &cTelnet::signal_connecting,
            this,
            [this, run]() {
                if (run == mRun) {
                    const QString profile = mProfileName;
                    emit connecting(profile);
                }
            },
            Qt::QueuedConnection);
    connect(
            &pHost->mTelnet,
            &cTelnet::signal_connectingCalledOff,
            this,
            [this, run]() {
                if (run == mRun) {
                    const QString profile = mProfileName;
                    emit disconnected(profile);
                }
            },
            Qt::QueuedConnection);
}

void THeadlessSession::sendLatency()
{
    // A shutdown() runs the event loop as the profiles close
    if (mShuttingDown || mLatencyPaceRun != mRun || !mpHost) {
        return;
    }
    const cTelnet& telnet = mpHost->mTelnet;
    const std::optional<double> network = telnet.mGA_Driver ? std::optional<double>(telnet.networkLatencyTime) : std::nullopt;
    const double processing = mpHost->mainConsoleModel().mIncomingTextSeconds;
    const QString shown = qsl("%1 %2").arg(network ? QString::number(*network, 'f', 3) : qsl("-"), QString::number(processing, 'f', 3));
    if (shown == mDeliveredLatency) {
        return;
    }
    mDeliveredLatency = shown;
    const QString profile = mProfileName;
    emit latencyChanged(profile, network, processing);
}

void THeadlessSession::noteOutputStateAtLoad()
{
    mDeliveredConsoleWrap = consoleWrap();
    mDeliveredConsoleColors = {consoleForeground(), consoleBackground()};
    mDeliveredTimeStampsShown = timeStampsShown();
}
