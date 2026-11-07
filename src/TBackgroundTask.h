#ifndef MUDLET_TBACKGROUNDTASK_H
#define MUDLET_TBACKGROUNDTASK_H


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

#include <QFuture>
#include <QList>
#include <QString>

#include <functional>

// File work a profile hands off its main thread. What it answers reaches the main thread through a
// QFutureWatcher, never through a wait. Without threads, as in a browser build, or with runInline set,
// each task runs on the calling thread and its future is already finished when it returns.
class TBackgroundTask
{
public:
    static QFuture<bool> run(std::function<bool()> task);
    // For a task that answers why it failed, empty when it did not
    static QFuture<QString> runForReason(std::function<QString()> task);
    // Starts once every one of earlier has finished, without holding a pool thread while it waits
    static QFuture<bool> runAfter(const QList<QFuture<bool>>& earlier, std::function<bool()> task);
    static bool runsInline();
    // For an embedding app with no worker threads to spare, and for tests
    static void setRunInline(bool runInline);

private:
    static bool smRunInline;
};

#endif // MUDLET_TBACKGROUNDTASK_H
