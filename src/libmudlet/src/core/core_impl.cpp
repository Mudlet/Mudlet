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

#include "core_impl.h"
#include "utils/logger.h"
#include <algorithm>

namespace mudlet {

CoreImpl::CoreImpl() 
    : mConfigManager(std::make_unique<ConfigManager>())
{
    initializeCore();
}

CoreImpl::~CoreImpl() {
    shutdownCore();
}

void CoreImpl::initializeCore() {
    if (mInitialized) {
        return;
    }
    
    // Initialize logging
    Logger::initialize();
    Logger::info("LibMudlet Core initializing...");
    
    // Initialize configuration
    mConfigManager->loadDefaultConfig();
    
    mInitialized = true;
    Logger::info("LibMudlet Core initialized successfully");
}

void CoreImpl::shutdownCore() {
    if (!mInitialized) {
        return;
    }
    
    Logger::info("LibMudlet Core shutting down...");
    
    // Clean shutdown all sessions
    std::lock_guard<std::mutex> lock(mSessionsMutex);
    for (auto& [name, session] : mSessions) {
        if (session) {
            session->disconnect();
        }
    }
    mSessions.clear();
    
    mInitialized = false;
    Logger::info("LibMudlet Core shutdown complete");
    Logger::shutdown();
}

std::shared_ptr<Session> CoreImpl::createSession(const std::string& name) {
    if (name.empty()) {
        Logger::error("Cannot create session with empty name");
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(mSessionsMutex);
    
    // Check if session already exists
    if (mSessions.find(name) != mSessions.end()) {
        Logger::warning("Session '{}' already exists", name);
        return mSessions[name];
    }
    
    // Create new session
    auto session = std::make_shared<SessionImpl>(name, *mConfigManager);
    if (!session->initialize()) {
        Logger::error("Failed to initialize session '{}'", name);
        return nullptr;
    }
    
    mSessions[name] = session;
    Logger::info("Created session '{}'", name);
    
    return session;
}

std::shared_ptr<Session> CoreImpl::getSession(const std::string& name) {
    std::lock_guard<std::mutex> lock(mSessionsMutex);
    
    auto it = mSessions.find(name);
    if (it != mSessions.end()) {
        return it->second;
    }
    
    return nullptr;
}

bool CoreImpl::removeSession(const std::string& name) {
    std::lock_guard<std::mutex> lock(mSessionsMutex);
    
    auto it = mSessions.find(name);
    if (it == mSessions.end()) {
        return false;
    }
    
    // Disconnect and cleanup the session
    if (it->second) {
        it->second->disconnect();
    }
    
    mSessions.erase(it);
    Logger::info("Removed session '{}'", name);
    
    return true;
}

std::vector<std::string> CoreImpl::getSessionNames() const {
    std::lock_guard<std::mutex> lock(mSessionsMutex);
    
    std::vector<std::string> names;
    names.reserve(mSessions.size());
    
    for (const auto& [name, session] : mSessions) {
        names.push_back(name);
    }
    
    return names;
}

void CoreImpl::setConfig(const std::string& key, const std::string& value) {
    mConfigManager->setGlobalConfig(key, value);
}

std::string CoreImpl::getConfig(const std::string& key, const std::string& defaultValue) const {
    return mConfigManager->getGlobalConfig(key, defaultValue);
}

void CoreImpl::processEvents() {
    std::lock_guard<std::mutex> lock(mSessionsMutex);
    
    // Process events for all sessions
    for (auto& [name, session] : mSessions) {
        if (session) {
            session->processEvents();
        }
    }
}

// Factory function implementation
std::unique_ptr<Core> Core::create() {
    return std::make_unique<CoreImpl>();
}

} // namespace mudlet