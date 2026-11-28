#ifndef MUDLET_TPAINTPACER_H
#define MUDLET_TPAINTPACER_H

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

#include <QElapsedTimer>
#include <QObject>
#include <QRegion>

#include <functional>

class TBuffer;
class QTimer;

// Caps the repaints that new output drives at one per frame. Those paints run inside the receive loop, so one per
// network packet delays the next packet; holding them to a frame lets a flood put several packets' worth of text into
// one paint, and a longer frame while more than a screen of lines arrives between two.
class TPaintPacer : public QObject
{
    Q_OBJECT

public:
    TPaintPacer(const TBuffer& buffer, QObject* parent);

    // An area of the view to repaint; wholeView is the view's rect
    void schedule(const QRect& area, const QRect& wholeView);
    // Called as the view paints: what it paints needs no deferred repaint
    void paintStarted(const QRegion& painted);
    // How many rows the view shows, read when a frame's pace is worked out
    void setScreenRows(std::function<int()> rows) { mScreenRows = std::move(rows); }
    // The scroll bar repaints on every range change, so new output moves it with the frame rather than every packet
    void setScrollBarUpdatePending(bool pending) { mScrollBarUpdatePending = pending; }
    bool scrollBarUpdatePending() const { return mScrollBarUpdatePending; }
    int paceMs() const;

    static constexpr int csmPaintPaceMs = 16;
    // The frame while more than a screen of lines arrives between two
    static constexpr int csmFloodPaintPaceMs = 33;

signals:
    void paintWanted(const QRegion& region);
    void scrollBarWanted();

private:
    friend class BlinkRowRepaintTest;
    friend class FramePacingTest;
    friend class PaintPacerTest;
    friend class ScrollLostOnPartialRepaintTest;

    void handOver(const QRegion& region);

    const TBuffer& mBuffer;
    std::function<int()> mScreenRows;
    QTimer* mpPaintPacer = nullptr;
    bool mPaintPacerStretched = false;
    bool mPaintPacerMoved = false;
    // Restarted by each paint, so schedule() can tell how much of the frame's window is left
    QElapsedTimer mSincePaint;
    qint64 mLinesAddedAtPaint = 0;
    // What the deferred repaint has to cover once the pacer fires
    QRegion mPendingPaintRegion;
    // The whole view was asked for since the last paint, so asking again only costs time
    bool mWholePaneRequested = false;
    bool mScrollBarUpdatePending = false;
};

#endif // MUDLET_TPAINTPACER_H
