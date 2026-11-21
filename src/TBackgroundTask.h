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

#include <QList>
#include <QString>

#include <functional>
#include <memory>

class QObject;

// What a background task answered, shared by every copy. A Qt built without threads, as for a browser, has no
// QFuture, so this stands in for one: a QFuture behind it where the task went to a pool thread, and the answer
// itself where it ran inline.
class TBackgroundResult
{
public:
    // As for a task that never ran: finished, and not succeeded
    TBackgroundResult();
    bool isFinished() const;
    bool isRunning() const;
    // Empty once it has succeeded, and why it failed otherwise; empty too until it has finished
    QString reason() const;
    bool succeeded() const;
    void waitForFinished() const;
    // Runs then from context's event loop once the task has finished, never during the call, and not at all once
    // context is gone. What tells it is a child of context named notifierName, so context owns it even when its
    // deferred delete never arrives on the way out, and it is sent a posted QEvent::FutureCallOut, as a
    // QFutureWatcher is, so a wait that delivers only those events delivers this.
    void whenFinished(QObject* context, const QString& notifierName, std::function<void()> then) const;

private:
    friend class TBackgroundTask;
    struct State;
    explicit TBackgroundResult(std::shared_ptr<State> state);
    std::shared_ptr<State> mpState;
};

// File work a profile hands off its main thread. What it answers normally reaches the main thread through whenFinished();
// a teardown, or a caller that must have the file on disk, waits with waitForFinished(). Without threads, as in a browser
// build, or with runInline set, run() and runForReason() run on the calling thread and have finished when they return.
class TBackgroundTask
{
public:
    static TBackgroundResult run(std::function<bool()> task);
    // For a task that answers why it failed, empty when it did not
    static TBackgroundResult runForReason(std::function<QString()> task);
    // Starts once every one of earlier has finished, without holding a pool thread while it waits, so even with
    // runInline set it goes to a pool thread, and is still running on return, while an earlier one is unfinished
    static TBackgroundResult runAfter(const QList<TBackgroundResult>& earlier, std::function<bool()> task);
    // For an embedding app with no worker threads to spare, and for tests
    static void setRunInline(bool runInline);

private:
    static bool runsInline();
    static bool smRunInline;
};

#endif // MUDLET_TBACKGROUNDTASK_H
