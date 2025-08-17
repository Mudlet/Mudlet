/***************************************************************************
 *   Copyright (C) 2025 by Rishi Mondal - mavrickrishi@gmail.com          *
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

#include "mudlet_libmudlet_integration.h"
#include "Host.h"

// Feature flags for gradual migration
namespace MudletFeatureFlags {
    bool UseLibMudletTriggers = false;
    bool UseLibMudletAliases = false;
    bool UseLibMudletTimers = false;
    bool UseLibMudletTextBuffer = false;
    bool UseLibMudletNetworking = false;
}

HostLibMudletBridge::HostLibMudletBridge(Host* host)
    : mHost(host) {
}

HostLibMudletBridge::~HostLibMudletBridge() {
    shutdownLibMudlet();
}

bool HostLibMudletBridge::initializeLibMudlet() {
    if (mInitialized) {
        return true;
    }
    
    // Create core adapter
    mCoreAdapter = std::make_unique<LibMudletQtAdapter>();
    if (!mCoreAdapter->initialize()) {
        return false;
    }
    
    // Create session for this host
    QString sessionName = QString("host_%1").arg(reinterpret_cast<quintptr>(mHost));
    QString result = mCoreAdapter->createSession(sessionName);
    if (result.isEmpty()) {
        return false;
    }
    
    // Get the session from libmudlet core
    auto core = mudlet::Core::create();
    mSession = core->getSession(sessionName.toStdString());
    if (!mSession) {
        return false;
    }
    
    // Create session adapter
    mSessionAdapter = std::make_unique<LibMudletSessionAdapter>(mSession);
    
    // Connect signals for integration
    QObject::connect(mSessionAdapter.get(), &LibMudletSessionAdapter::triggerFired,
                    [this](int triggerId, const QString& match) {
        // Forward to existing Host trigger processing if needed
        // This allows gradual migration
    });
    
    QObject::connect(mSessionAdapter.get(), &LibMudletSessionAdapter::aliasExecuted,
                    [this](int aliasId, const QString& command, const QString& expansion) {
        // Forward to existing Host alias processing if needed
    });
    
    QObject::connect(mSessionAdapter.get(), &LibMudletSessionAdapter::timerFired,
                    [this](int timerId) {
        // Forward to existing Host timer processing if needed
    });
    
    mInitialized = true;
    return true;
}

void HostLibMudletBridge::shutdownLibMudlet() {
    if (!mInitialized) {
        return;
    }
    
    mSessionAdapter.reset();
    if (mCoreAdapter) {
        QString sessionName = QString("host_%1").arg(reinterpret_cast<quintptr>(mHost));
        mCoreAdapter->removeSession(sessionName);
        mCoreAdapter->shutdown();
        mCoreAdapter.reset();
    }
    mSession.reset();
    
    mInitialized = false;
}

LibMudletSessionAdapter* HostLibMudletBridge::getSessionAdapter() const {
    return mSessionAdapter.get();
}

void HostLibMudletBridge::migrateTriggers() {
    if (!mInitialized || !mHost) {
        return;
    }
    
    // TODO: Implement trigger migration
    // This would copy existing triggers from Host to libmudlet
    // For now, this is a placeholder for future implementation
}

void HostLibMudletBridge::migrateAliases() {
    if (!mInitialized || !mHost) {
        return;
    }
    
    // TODO: Implement alias migration
    // This would copy existing aliases from Host to libmudlet
}

void HostLibMudletBridge::migrateTimers() {
    if (!mInitialized || !mHost) {
        return;
    }
    
    // TODO: Implement timer migration
    // This would copy existing timers from Host to libmudlet
}

void HostLibMudletBridge::processLibMudletEvents() {
    if (mCoreAdapter) {
        mCoreAdapter->processEvents();
    }
}
