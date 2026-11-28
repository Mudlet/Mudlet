/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TPaintPacer.h"

#include "TBuffer.h"

#include <QTimer>

TPaintPacer::TPaintPacer(const TBuffer& buffer, QObject* parent)
: QObject(parent)
, mBuffer(buffer)
, mpPaintPacer(new QTimer(this))
{
    mpPaintPacer->setSingleShot(true);
    connect(mpPaintPacer, &QTimer::timeout, this, [this]() {
        // Armed at the normal pace by the line that began a flood, so stretched to the flood pace -
        // once and by no more than the paces differ, as a paint under an overlay restarts mSincePaint.
        const qint64 sincePaint = mSincePaint.isValid() ? mSincePaint.elapsed() : csmFloodPaintPaceMs;
        if (!mPaintPacerStretched && !mPendingPaintRegion.isEmpty() && sincePaint < csmFloodPaintPaceMs && paceMs() == csmFloodPaintPaceMs) {
            mPaintPacerStretched = true;
            mpPaintPacer->start(std::min(csmFloodPaintPaceMs - csmPaintPaceMs, csmFloodPaintPaceMs - static_cast<int>(sincePaint)));
            return;
        }
        handOver(std::exchange(mPendingPaintRegion, QRegion()));
    });
}

// The scroll bar first, as the view moves it before it paints
void TPaintPacer::handOver(const QRegion& region)
{
    if (mScrollBarUpdatePending) {
        emit scrollBarWanted();
    }
    if (!region.isEmpty()) {
        emit paintWanted(region);
    }
}

void TPaintPacer::schedule(const QRect& area, const QRect& wholeView)
{
    if (mWholePaneRequested) {
        // A script echoing in a loop lands here on every line; the scroll bar
        // catches up once the event loop runs, as the paint itself will.
        if (mScrollBarUpdatePending && !mpPaintPacer->isActive()) {
            mpPaintPacer->start(0);
        }
        return;
    }
    const bool nothingHeldBack = mPendingPaintRegion.isEmpty();
    mPendingPaintRegion += area;
    const int pace = paceMs();
    const qint64 sincePaint = mSincePaint.isValid() ? mSincePaint.elapsed() : pace;

    // Nothing painted recently, so this frame's window is open: Qt still merges
    // whatever else arrives before the event loop gets around to painting.
    if (sincePaint >= pace) {
        handOver(std::exchange(mPendingPaintRegion, QRegion()));
        mWholePaneRequested = area.contains(wholeView);
        mPaintPacerMoved = false;
        return;
    }

    // A timer still running with nothing held back is due on the last frame's clock. Moved once a
    // frame, as paints covering the pane can outpace it and only its firing moves the scrollbar.
    const bool leftRunning = mpPaintPacer->isActive();
    if (!leftRunning || (nothingHeldBack && !mPaintPacerMoved)) {
        mPaintPacerMoved = leftRunning;
        mPaintPacerStretched = pace == csmFloodPaintPaceMs;
        mpPaintPacer->start(pace - static_cast<int>(sincePaint));
    }
}

void TPaintPacer::paintStarted(const QRegion& painted)
{
    mSincePaint.restart();
    mLinesAddedAtPaint = mBuffer.linesAddedTotal();
    mWholePaneRequested = false;
    if (!mPendingPaintRegion.isEmpty()) {
        // Whatever this paint covers is current now, so a deferred repaint of it
        // would be redundant. Only the remainder - if a partial expose left one -
        // and a pending scrollbar update still need the pacer.
        mPendingPaintRegion -= painted;
        if (mPendingPaintRegion.isEmpty() && !mScrollBarUpdatePending) {
            mpPaintPacer->stop();
        }
    }
}

// Nobody reads lines that scroll past between two frames, and a frame costs as much as
// taking in hundreds of them, so a flood gets fewer frames and finishes sooner.
int TPaintPacer::paceMs() const
{
    const int rows = mScreenRows ? mScreenRows() : 0;
    return mBuffer.linesAddedTotal() - mLinesAddedAtPaint > rows ? csmFloodPaintPaceMs : csmPaintPaceMs;
}
