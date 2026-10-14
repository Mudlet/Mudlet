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

#include "TNullAppFrontend.h"

#include "Host.h"
#include "HostManager.h"
#include "TLabelModel.h"
#include "TLuaInterpreter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>

using namespace std::chrono_literals;

TAppFrontend* TAppFrontend::nullView()
{
    // Function-local, so it exists before any static initialiser can ask for it
    static TNullAppFrontend view;
    return &view;
}

std::optional<QSize> TNullAppFrontend::getImageSize(const QString& imageLocation)
{
    return TLabelModel::imageSize(imageLocation);
}

bool TNullAppFrontend::openProfile(const QString& profileName, const bool connect)
{
    HostManager* pool = HostManager::self();
    return pool && pool->openProfile(profileName, connect);
}

bool TNullAppFrontend::requestProfileClose(const QString& profileName)
{
    HostManager* pool = HostManager::self();
    return pool && pool->requestProfileClose(profileName);
}

namespace {
// Process-wide, as the timers below outlive any one view
bool sForceClosePending = false;
bool sQuitPending = false;

// quit() ends only a running exec() and does nothing from an event loop pump before one, which is
// where an embedder loading profiles meets this. A nested loop before exec() still loses it.
void quitOnceRunning()
{
    if (QThread::currentThread()->loopLevel() == 0) {
        QTimer::singleShot(50ms, quitOnceRunning);
        return;
    }
    sQuitPending = false;
    QCoreApplication::quit();
}

void closeAllAndQuit();

// Nothing would delete a profile taken out of the pool after the quit, and its script may open another
void closeAllOnceReleased()
{
    if (HostManager::releasingProfile()) {
        QTimer::singleShot(50ms, closeAllOnceReleased);
        return;
    }
    closeAllAndQuit();
}

void closeAllAndQuit()
{
    if (HostManager* pool = HostManager::self()) {
        // As the GUI's, until a load returns or a profile leaves a nested event loop; a profile still
        // running a script or a map operation closes, and leaves the pool once that ends. A reset asked
        // for first that can run now ends first, as the GUI's alert runs the event loop once before its close.
        bool mustWait = pool->loadingProfile();
        for (const auto& pHost : pool->hostList()) {
            if (pHost->getLuaInterpreter()->pumpingEvents()) {
                qWarning() << "TNullAppFrontend::armForceClose() - a nested event loop is running, waiting for it to finish";
                mustWait = true;
                break;
            }
            mustWait |= pHost->profileResetInProgress() && !pHost->isClosingDown() && !pHost->getLuaInterpreter()->luaOnStack();
        }
        if (mustWait) {
            QTimer::singleShot(50ms, closeAllAndQuit);
            return;
        }
        pool->closeAllProfiles();
    }
    if (HostManager::releasingProfile()) {
        QTimer::singleShot(50ms, closeAllOnceReleased);
        return;
    }
    sForceClosePending = false;
    if (!sQuitPending) {
        sQuitPending = true;
        if (QThread::currentThread()->loopLevel() == 0) {
            qDebug() << "TNullAppFrontend::armForceClose() INFO - no event loop is running yet, so the quit waits for one";
        }
        quitOnceRunning();
    }
}
} // namespace

void TNullAppFrontend::armForceClose()
{
    if (sForceClosePending) {
        return;
    }
    sForceClosePending = true;
    // Deferred, as closing deletes the Lua state the caller runs on
    QTimer::singleShot(0ms, closeAllAndQuit);
}
