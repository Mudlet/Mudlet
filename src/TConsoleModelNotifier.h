#ifndef MUDLET_TCONSOLEMODELNOTIFIER_H
#define MUDLET_TCONSOLEMODELNOTIFIER_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QObject>

class TLineFormat;
Q_MOC_INCLUDE("TBuffer.h")

// What a console model's buffer tells the view showing it, and for the main
// console's model, Host. The buffer only emits, so it needs no view check of
// its own.
class TConsoleModelNotifier : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    // isSignalConnected() is protected. The GUI never connects textPrinted or lineShown, so this spares it the work.
    bool textPrintedConnected() const;
    bool lineShownConnected() const;
    bool linkChangedConnected() const;

signals:
    // The count is of every line in the buffer, not just the new ones.
    void linesAppended(int lineCount);
    // Before triggers run on the line; read it now, as a buffer shrink moves it. The commit is half
    // done, so a handler may neither add a line nor run the event loop.
    void lineCommitted(int lineIndex);
    // Once triggers have run on the line lineCommitted sent, before it is wrapped: where it now stands, or -1
    // if they deleted it, and the last of the lines a script's wrap split it into. One follows every
    // lineCommitted, so those of lines fed from inside the pass come first.
    void lineShown(int lineIndex, int lastSegment);
    // Client output (echo(), print(), the command echo, Mudlet's messages), in pieces that need not end
    // a line; also an echo() or command echo written into a trigger's or a prompt's line, which --mirror
    // does not copy. format has one entry for each character of text, as it was written. continuesLine
    // is for a piece the view shows on the end of a line that was already whole, a trigger's or a prompt's.
    void textPrinted(const QString& text, const TLineFormat& format, bool continuesLine);
    // A line that may have been wrapped by the game is being held back for its
    // continuation, which may never come.
    void serverWrapLineHeld();
    void linkCharactersChanged();
    // These lines were restyled or written into where they stand.
    void linesChanged(int firstLine, int lastLine);
    // Lines were written at the end, which the view brings into view as
    // echoing there does.
    void newLinesWritten();
    // mShowTimeStamps was flipped.
    void timeStampsToggled();
    // mWrapAt, mIndentCount or mHangingIndentCount changed.
    void wrapChanged();
    // Host::refreshMainConsoleColors() ran, which may have changed mFgColor or mBgColor.
    void colorsChanged();
    // The null view timed a pass of incoming text, in mIncomingTextSeconds.
    void incomingTextProcessed();
    // The buffer was emptied, so view state indexing its old lines is stale.
    void bufferCleared();
    void spoilerRevealed();
    // The null view's alertNewData(): game text came in that the profile asks to be alerted to.
    void newDataAlerted();
    // What a link offers changed: its state, its selection, its spoiler, or it was expired, swept from the store or
    // a line holding it was deleted. Not for a trim, which leaves the link to the sweep.
    void linkChanged(int linkIndex);
    // The buffer's rows changed with none of the signals above sent, as the view showing it repaints only later:
    // a script's deleteLine() or wrapLine().
    void contentEdited();
};

#endif // MUDLET_TCONSOLEMODELNOTIFIER_H
