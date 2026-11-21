#ifndef MUDLET_THREADPOOLHOLD_H
#define MUDLET_THREADPOOLHOLD_H


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

#include <QSemaphore>
#include <QThreadPool>

#include <chrono>
#include <thread>

// Holds every pool thread, so background work such as a save's writes cannot end, until a thread of its own lets
// them go: a wait for that work on the main thread then ends late, failing a test, rather than hangs it
class PoolHold
{
public:
    explicit PoolHold(const std::chrono::milliseconds releaseAfter)
    : mThreads(QThreadPool::globalInstance()->maxThreadCount())
    {
        for (int i = 0; i < mThreads; ++i) {
            QThreadPool::globalInstance()->start([this]() {
                mHeld.release();
                mRelease.acquire();
            });
        }
        mHeldAll = mHeld.tryAcquire(mThreads, 10000);
        mReleaser = std::thread([this, releaseAfter]() {
            std::this_thread::sleep_for(releaseAfter);
            mRelease.release(mThreads);
        });
    }
    ~PoolHold()
    {
        mReleaser.join();
        QThreadPool::globalInstance()->waitForDone(10000);
    }
    PoolHold(const PoolHold&) = delete;
    PoolHold& operator=(const PoolHold&) = delete;
    bool heldAll() const { return mHeldAll; }

private:
    int mThreads = 0;
    bool mHeldAll = false;
    QSemaphore mHeld;
    QSemaphore mRelease;
    std::thread mReleaser;
};

#endif // MUDLET_THREADPOOLHOLD_H
