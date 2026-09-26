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

#include <QMetaMethod>
#include <QObject>
#include <QString>

// What a console model's buffer tells the view showing it. The buffer only
// emits; a model with no view has nobody listening, so the buffer needs no
// view check of its own.
class TConsoleModelNotifier : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    bool hasLineMirror() const { return isSignalConnected(QMetaMethod::fromSignal(&TConsoleModelNotifier::lineCommitted)); }

signals:
    // The count is of every line in the buffer, not just the new ones.
    void linesAppended(int lineCount);
    // A game line as the game sent it, before a trigger can gag or rewrite it.
    // Only emitted while --mirror is on.
    void lineCommitted(const QString& line);
    // A line that may have been wrapped by the game is being held back for its
    // continuation, which may never come.
    void serverWrapLineHeld();
    void linkCharactersChanged();
    void spoilerRevealed();
};

#endif // MUDLET_TCONSOLEMODELNOTIFIER_H
