
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

#include <QObject>
#include <QtGlobal>
#if QT_CONFIG(future)
#include <QFutureWatcher>
#include <QPromise>
#include <QtConcurrentRun>
#endif

#include <algorithm>
#include <optional>

namespace {
QString reasonFor(const bool succeeded)
{
    return succeeded ? QString() : QStringLiteral("failed");
}
} // namespace

struct TBackgroundResult::State
{
#if QT_CONFIG(future)
    // Unset for a task that ran inline
    std::optional<QFuture<QString>> future;
#endif
    bool finished = true;
    QString reason = QStringLiteral("never ran");
};

TBackgroundResult::TBackgroundResult()
: mpState(std::make_shared<State>())
{
}

TBackgroundResult::TBackgroundResult(std::shared_ptr<State> state)
: mpState(std::move(state))
{
}

bool TBackgroundResult::isFinished() const
{
#if QT_CONFIG(future)
    if (mpState->future) {
        return mpState->future->isFinished();
    }
#endif
    return mpState->finished;
}

bool TBackgroundResult::isRunning() const
{
#if QT_CONFIG(future)
    if (mpState->future) {
        return mpState->future->isRunning();
    }
#endif
    return false;
}

QString TBackgroundResult::reason() const
{
#if QT_CONFIG(future)
    if (mpState->future) {
        return mpState->future->isFinished() ? mpState->future->result() : QString();
    }
#endif
    return mpState->reason;
}

bool TBackgroundResult::succeeded() const
{
    return isFinished() && reason().isEmpty();
}

void TBackgroundResult::waitForFinished() const
{
#if QT_CONFIG(future)
    if (mpState->future) {
        mpState->future->waitForFinished();
    }
#endif
}

void TBackgroundResult::whenFinished(QObject* context, const QString& notifierName, std::function<void()> then) const
{
#if QT_CONFIG(future)
    if (mpState->future) {
        auto* watcher = new QFutureWatcher<QString>(context);
        watcher->setObjectName(notifierName);
        QObject::connect(watcher, &QFutureWatcher<QString>::finished, context, std::move(then));
        QObject::connect(watcher, &QFutureWatcher<QString>::finished, watcher, &QObject::deleteLater);
        watcher->setFuture(*mpState->future);
        return;
    }
#endif
    // Posted, as a watcher's finished is, so it never runs during the call
    auto* notifier = new QObject(context);
    notifier->setObjectName(notifierName);
    QMetaObject::invokeMethod(
            notifier,
            [notifier, then = std::move(then)]() {
                notifier->deleteLater();
                then();
            },
            Qt::QueuedConnection);
}

bool TBackgroundTask::smRunInline = false;

bool TBackgroundTask::runsInline()
{
#if QT_CONFIG(future)
    return smRunInline;
#else
    return true;
#endif
}

void TBackgroundTask::setRunInline(const bool runInline)
{
    smRunInline = runInline;
}

TBackgroundResult TBackgroundTask::run(std::function<bool()> task)
{
    return runForReason([task = std::move(task)]() {
        return reasonFor(task());
    });
}

TBackgroundResult TBackgroundTask::runForReason(std::function<QString()> task)
{
    auto state = std::make_shared<TBackgroundResult::State>();
#if QT_CONFIG(future)
    if (!runsInline()) {
        state->future = QtConcurrent::run(std::move(task));
        return TBackgroundResult(state);
    }
#endif
    state->reason = task();
    return TBackgroundResult(state);
}

TBackgroundResult TBackgroundTask::runAfter(const QList<TBackgroundResult>& earlier, std::function<bool()> task)
{
#if QT_CONFIG(future)
    const bool earlierDone = std::all_of(earlier.cbegin(), earlier.cend(), [](const TBackgroundResult& result) {
        return result.isFinished();
    });
    // Inline only once the earlier ones have finished, as ones started before inline running was asked for may not have
    if (!runsInline() || !earlierDone) {
        // Started now, so the future reads as running while it waits: a continuation's own would read as neither
        // running nor finished until a pool thread picks it up, and a waiter would take that for done
        auto promise = std::make_shared<QPromise<QString>>();
        promise->start();
        auto state = std::make_shared<TBackgroundResult::State>();
        state->future = promise->future();
        QList<QFuture<QString>> futures;
        for (const auto& result : earlier) {
            if (result.mpState->future) {
                futures.append(*result.mpState->future);
            }
        }
        // Async, not the default Sync: a whenAll() already finished would run the task on this thread
        QtFuture::whenAll(futures.begin(), futures.end()).then(QtFuture::Launch::Async, [promise, task = std::move(task)](const QList<QFuture<QString>>&) {
            promise->addResult(reasonFor(task()));
            promise->finish();
        });
        return TBackgroundResult(state);
    }
#endif
    return run(std::move(task));
}
