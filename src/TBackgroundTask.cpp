
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

#include "TBackgroundTask.h"

#include <QtGlobal>
#if QT_CONFIG(thread)
#include <QPromise>
#include <QtConcurrentRun>

#include <algorithm>
#include <memory>
#endif

bool TBackgroundTask::smRunInline = false;

bool TBackgroundTask::runsInline()
{
#if QT_CONFIG(thread)
    return smRunInline;
#else
    return true;
#endif
}

void TBackgroundTask::setRunInline(const bool runInline)
{
    smRunInline = runInline;
}

QFuture<bool> TBackgroundTask::run(std::function<bool()> task)
{
#if QT_CONFIG(thread)
    if (!runsInline()) {
        return QtConcurrent::run(std::move(task));
    }
#endif
    return QtFuture::makeReadyValueFuture(task());
}

QFuture<QString> TBackgroundTask::runForReason(std::function<QString()> task)
{
#if QT_CONFIG(thread)
    if (!runsInline()) {
        return QtConcurrent::run(std::move(task));
    }
#endif
    return QtFuture::makeReadyValueFuture(task());
}

QFuture<bool> TBackgroundTask::runAfter(const QList<QFuture<bool>>& earlier, std::function<bool()> task)
{
#if QT_CONFIG(thread)
    const bool earlierDone = std::all_of(earlier.cbegin(), earlier.cend(), [](const QFuture<bool>& future) {
        return future.isFinished();
    });
    // Inline only once the earlier ones have finished, as ones started before inline running was asked for may not have
    if (!runsInline() || !earlierDone) {
        // Started now, so the future reads as running while it waits: a continuation's own would read as neither
        // running nor finished until a pool thread picks it up, and a waiter would take that for done
        auto promise = std::make_shared<QPromise<bool>>();
        promise->start();
        QFuture<bool> future = promise->future();
        // whenAll() attaches continuations, which a const future cannot take
        QList<QFuture<bool>> futures = earlier;
        // Async, not the default Sync: a whenAll() already finished would run the task on this thread
        QtFuture::whenAll(futures.begin(), futures.end()).then(QtFuture::Launch::Async, [promise, task = std::move(task)](const QList<QFuture<bool>>&) {
            promise->addResult(task());
            promise->finish();
        });
        return future;
    }
#endif
    return QtFuture::makeReadyValueFuture(task());
}
