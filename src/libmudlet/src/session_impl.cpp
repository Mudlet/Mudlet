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

#include "session_impl.h"
#include "utils/logger.h"

namespace mudlet {

SessionImpl::SessionImpl(const std::string& name) 
    : mName(name)
    , mTriggerEngine(std::make_unique<TriggerEngineImpl>())
    , mAliasEngine(std::make_unique<AliasEngineImpl>())
    , mTimerEngine(std::make_unique<TimerEngineImpl>()) {
    
    Logger::debug("SessionImpl created: {}", name);
}

SessionImpl::~SessionImpl() {
    Logger::debug("SessionImpl destroyed: {}", mName);
}

std::string SessionImpl::getName() const {
    return mName;
}

ConnectionState SessionImpl::getConnectionState() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mConnectionState;
}

bool SessionImpl::connect(const ConnectionInfo& info, ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    // Stub implementation for now
    Logger::info("Session '{}': Connecting to {}:{}", mName, info.hostname, info.port);
    mConnectionState = ConnectionState::Connecting;
    
    // TODO: Implement actual network connection in Phase 4
    // For now, just simulate successful connection
    mConnectionState = ConnectionState::Connected;
    
    if (callback) {
        callback(true, "");
    }
    
    return true;
}

void SessionImpl::disconnect() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    Logger::info("Session '{}': Disconnecting", mName);
    mConnectionState = ConnectionState::Disconnected;
    
    if (mConnectionStatusCallback) {
        mConnectionStatusCallback(ConnectionState::Disconnected);
    }
}

bool SessionImpl::isConnected() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mConnectionState == ConnectionState::Connected;
}

void SessionImpl::send(const std::string& text, bool echo) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    Logger::debug("Session '{}': Sending text (echo={}): {}", mName, echo, text);
    
    // TODO: Implement actual network sending in Phase 4
    // For now, just log the send request
}

TriggerEngine& SessionImpl::getTriggerEngine() {
    return *mTriggerEngine;
}

AliasEngine& SessionImpl::getAliasEngine() {
    return *mAliasEngine;
}

TimerEngine& SessionImpl::getTimerEngine() {
    return *mTimerEngine;
}

ScriptEngine& SessionImpl::getScriptEngine() {
    // TODO: Implement ScriptEngine in Phase 5
    static ScriptEngine* stub = nullptr;
    if (!stub) {
        Logger::error("ScriptEngine not yet implemented - returning null reference");
        throw std::runtime_error("ScriptEngine not implemented in Phase 1");
    }
    return *stub;
}

TextBuffer& SessionImpl::getTextBuffer() {
    // TODO: Implement TextBuffer in Phase 3
    static TextBuffer* stub = nullptr;
    if (!stub) {
        Logger::error("TextBuffer not yet implemented - returning null reference");
        throw std::runtime_error("TextBuffer not implemented in Phase 1");
    }
    return *stub;
}

NetworkEngine& SessionImpl::getNetworkEngine() {
    // TODO: Implement NetworkEngine in Phase 4
    static NetworkEngine* stub = nullptr;
    if (!stub) {
        Logger::error("NetworkEngine not yet implemented - returning null reference");
        throw std::runtime_error("NetworkEngine not implemented in Phase 1");
    }
    return *stub;
}

void SessionImpl::processEvents() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    // Process timer events
    auto firedTimers = mTimerEngine->processTimers();
    for (int timerId : firedTimers) {
        Logger::debug("Session '{}': Timer {} fired", mName, timerId);
    }
    
    // TODO: Process network events in Phase 4
    // TODO: Process text buffer events in Phase 3
}

void SessionImpl::setTextReceivedCallback(TextReceivedCallback callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mTextReceivedCallback = callback;
}

void SessionImpl::setConnectionStatusCallback(ConnectionStatusCallback callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mConnectionStatusCallback = callback;
}

} // namespace mudlet
