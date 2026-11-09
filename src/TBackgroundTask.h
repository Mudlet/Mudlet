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

// What a background task answers, shared by every copy. Without threads, or with
// TBackgroundTask::setRunInline(true), the task has run by the time it is handed back.
class TBackgroundResult
{
public:
    // As for a task that never ran: finished, and not succeeded
    TBackgroundResult();
    bool isFinished() const;
    bool isRunning() const;
    // Empty once it has succeeded; why it failed otherwise, and empty too until it has finished
    QString reason() const;
    bool succeeded() const;
    // Blocks the calling thread until the task has finished
    void waitForFinished() const;
    // Runs then on context's thread from the event loop, never during the call, once the task has finished, and not
    // at all once context is gone. What delivers it is a child of context named notifierName, whose posted events
    // are all that a wait for only those needs to send.
    void whenFinished(QObject* context, const QString& notifierName, std::function<void()> then) const;

private:
    friend class TBackgroundTask;
    struct State;
    explicit TBackgroundResult(std::shared_ptr<State> state);
    std::shared_ptr<State> mpState;
};

// File work a profile hands off its main thread. What it answers reaches the main thread through
// TBackgroundResult::whenFinished(), never through a wait. Without threads, as in a browser build, or with
// runInline set, each task runs on the calling thread.
class TBackgroundTask
{
public:
    static TBackgroundResult run(std::function<bool()> task);
    // For a task that answers why it failed, empty when it did not
    static TBackgroundResult runForReason(std::function<QString()> task);
    // Starts once every one of earlier has finished, without holding a pool thread while it waits
    static TBackgroundResult runAfter(const QList<TBackgroundResult>& earlier, std::function<bool()> task);
    static bool runsInline();
    // For an embedding app with no worker threads to spare, and for tests
    static void setRunInline(bool runInline);

private:
    static bool smRunInline;
};

#endif // MUDLET_TBACKGROUNDTASK_H
