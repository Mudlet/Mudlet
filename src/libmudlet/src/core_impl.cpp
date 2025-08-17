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

#include "mudlet/core.h"
#include "session_impl.h"
#include "utils/logger.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace mudlet {

/**
 * @brief Core implementation for libmudlet
 */
class CoreImpl : public Core {
public:
    CoreImpl() {
        Logger::initialize();
        Logger::info("LibMudlet Core initialized");
    }
    
    ~CoreImpl() override {
        Logger::info("LibMudlet Core shutting down");
        Logger::shutdown();
    }
    
    std::shared_ptr<Session> createSession(const std::string& name) override {
        std::lock_guard<std::mutex> lock(mMutex);
        
        if (mSessions.find(name) != mSessions.end()) {
            Logger::warning("Session '{}' already exists", name);
            return nullptr;
        }
        
        auto session = std::make_shared<SessionImpl>(name);
        mSessions[name] = session;
        
        Logger::info("Created session '{}'", name);
        return session;
    }
    
    std::shared_ptr<Session> getSession(const std::string& name) override {
        std::lock_guard<std::mutex> lock(mMutex);
        
        auto it = mSessions.find(name);
        if (it == mSessions.end()) {
            return nullptr;
        }
        
        return it->second.lock();
    }
    
    bool removeSession(const std::string& name) override {
        std::lock_guard<std::mutex> lock(mMutex);
        
        auto it = mSessions.find(name);
        if (it == mSessions.end()) {
            return false;
        }
        
        mSessions.erase(it);
        Logger::info("Removed session '{}'", name);
        return true;
    }
    
    std::vector<std::string> getSessionNames() const override {
        std::lock_guard<std::mutex> lock(mMutex);
        
        std::vector<std::string> names;
        names.reserve(mSessions.size());
        
        for (const auto& [name, weakSession] : mSessions) {
            if (!weakSession.expired()) {
                names.push_back(name);
            }
        }
        
        return names;
    }
    
    void processEvents() override {
        std::lock_guard<std::mutex> lock(mMutex);
        
        // Process events for all active sessions
        for (auto it = mSessions.begin(); it != mSessions.end(); ) {
            auto session = it->second.lock();
            if (session) {
                session->processEvents();
                ++it;
            } else {
                // Session expired, remove it
                it = mSessions.erase(it);
            }
        }
    }
    
    void setEventCallback(EventCallback callback) override {
        std::lock_guard<std::mutex> lock(mMutex);
        mEventCallback = callback;
    }
    
    void setConfig(const std::string& key, const std::string& value) override {
        // TODO: Implement configuration management
        Logger::debug("setConfig: {} = {}", key, value);
    }
    
    std::string getConfig(const std::string& key, const std::string& defaultValue = "") const override {
        // TODO: Implement configuration management
        Logger::debug("getConfig: {} (default: {})", key, defaultValue);
        return defaultValue;
    }

private:
    mutable std::mutex mMutex;
    std::unordered_map<std::string, std::weak_ptr<Session>> mSessions;
    EventCallback mEventCallback;
};

// Factory method implementation
std::unique_ptr<Core> Core::create() {
    return std::make_unique<CoreImpl>();
}

} // namespace mudlet
