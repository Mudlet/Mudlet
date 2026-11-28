#ifndef MUDLET_TTEXTSELECTION_H
#define MUDLET_TTEXTSELECTION_H

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

#include <QChar>
#include <QElapsedTimer>
#include <QPoint>
#include <QPointer>
#include <QRegion>
#include <QString>

#include <utility>

class Host;
class TBuffer;

// A selection in buffer terms, lines and columns, as the panes and the accessible text count them. The view that owns
// it works out where it is drawn, and raises the clipboard and accessibility events.
class TTextSelection
{
public:
    TTextSelection(TBuffer& buffer, Host* pHost);

    // Orders mDragStart and mDragSelectionEnd so mPA is the first selected cell and mPB the last, in reading order
    void normalise();
    // Widens mPA and mPB to the words they are in
    void expandToWords();
    // A drag anchored on a word separator starts at the nearest word, past any separators beside it; the anchored end
    // must be on a line of the buffer
    void anchorEndToNearestWord(bool anchorFirst);
    void selectAll();
    // Marks the selected characters on the buffer's lines, which is how they are drawn selected; unmarkLines() clears them
    void markLines();
    void unmarkLines();
    bool hasSelection() const;
    // Moves mPA and mPB down by a trim of the buffer before reading, which the other methods only allow for
    QString text(QChar newline = QChar::LineFeed, bool showTimestamps = false);
    // Appends the selection's lines as HTML; false where a line of it is gone, as a script can clear the buffer
    bool appendHtml(QString& html, bool showTimestamps) const;
    // Lines lost off the front of a buffer at its limit shift every index down, while mPA and mPB
    // stay on the old indexes until text() catches them up
    int linesTrimmed() const;
    int accessibleLength() const;
    // Start and one past the end, as the accessible text counts them, or (0, 0) for no selection
    std::pair<int, int> accessibleRange() const;
    // The offset of a line and column in the accessible text, which ends every line with a newline
    int offsetOf(int line, int column) const;

    QPoint mDragStart;
    QPoint mDragSelectionEnd;
    // The first and last selected cells, as a line of the buffer and a QChar index into it; x == the line's length on
    // mPB is the newline after it. Expanding to words on a separator leaves them crossed, which hasSelection() reads
    // as no selection.
    QPoint mPA;
    QPoint mPB;
    // Equal to mPB while the selection includes the newline after mPB's line: an
    // accessibility range ending there leaves mPB where a whole-line selection without
    // the newline does. Meaningful only while it equals mPB; unmarkLines() resets it to (-1, -1).
    QPoint mPBTakingNewline{-1, -1};
    // 1/2/3 for single/double/triple click seen so far
    int mMouseTrackLevel = 0;
    bool mCtrlSelecting = false;
    // Tracks the Shift key for keyboard selection
    bool mShiftSelection = false;
    QElapsedTimer mLastClickTimer;
    // Where the view last drew the selection, emptied when it is dropped; a region alone is not a selection, see
    // hasSelection()
    QRegion mSelectedRegion;

private:
    TBuffer& mBuffer;
    QPointer<Host> mpHost;
};

#endif // MUDLET_TTEXTSELECTION_H
