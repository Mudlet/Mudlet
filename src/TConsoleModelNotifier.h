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

// What a console model's buffer tells the view showing it, and for the main
// console's model, Host. The buffer only emits, so it needs no view check of
// its own.
class TConsoleModelNotifier : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

signals:
    // The count is of every line in the buffer, not just the new ones.
    void linesAppended(int lineCount);
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
    // The buffer was emptied, so view state indexing its old lines is stale.
    void bufferCleared();
    void spoilerRevealed();
};

#endif // MUDLET_TCONSOLEMODELNOTIFIER_H
