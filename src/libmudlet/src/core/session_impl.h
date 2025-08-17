#ifndef LIBMUDLET_SESSION_IMPL_H
#define LIBMUDLET_SESSION_IMPL_H

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

#include "mudlet/session.h"
#include "config_manager.h"
#include "text/text_buffer_impl.h"
#include "engines/script_engine_impl.h"
#include "engines/trigger_engine_impl.h"
#include "engines/alias_engine_impl.h"
#include "engines/timer_engine_impl.h"
#include "engines/keybind_engine_impl.h"
#include "engines/map_engine_impl.h"
#include "network/network_engine_impl.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace mudlet {

/**
 * @brief Session implementation - represents a single MUD connection
 */
class SessionImpl : public Session {
public:
    SessionImpl(const std::string& name, ConfigManager& configManager);
    ~SessionImpl() override;
    
    // Initialize the session (called after construction)
    bool initialize();
    
    // Process events (called from Core::processEvents)
    void processEvents();
    
    // Session interface implementation
    std::string getName() const override;
    
    // Network operations
    bool connect(const ConnectionInfo& info, ConnectionCallback callback = nullptr) override;
    void disconnect() override;
    bool isConnected() const override;
    void send(const std::string& text, bool echo = true) override;
    ConnectionInfo getConnectionInfo() const override;
    
    // Text buffer operations
    TextBuffer& getMainBuffer() override;
    std::shared_ptr<TextBuffer> createBuffer(const std::string& name) override;
    std::shared_ptr<TextBuffer> getBuffer(const std::string& name) override;
    bool removeBuffer(const std::string& name) override;
    void setTextCallback(TextCallback callback) override;
    
    // Script engine
    ScriptEngine& getScriptEngine() override;
    bool executeScript(const std::string& script, ScriptCallback callback = nullptr) override;
    
    // Trigger system
    TriggerEngine& getTriggerEngine() override;
    int addTrigger(const std::string& pattern, PatternType patternType, 
                  const std::string& script, bool enabled = true) override;
    bool removeTrigger(int triggerId) override;
    bool setTriggerEnabled(int triggerId, bool enabled) override;
    
    // Alias system
    AliasEngine& getAliasEngine() override;
    int addAlias(const std::string& pattern, const std::string& replacement, bool enabled = true) override;
    bool removeAlias(int aliasId) override;
    bool setAliasEnabled(int aliasId, bool enabled) override;
    
    // Timer system
    TimerEngine& getTimerEngine() override;
    int addTimer(int interval, TimerType type, const std::string& script, bool enabled = true) override;
    bool removeTimer(int timerId) override;
    bool setTimerEnabled(int timerId, bool enabled) override;
    
    // Key binding system
    KeyBindEngine& getKeyBindEngine() override;
    int addKeyBinding(const std::string& keySequence, const std::string& script, bool enabled = true) override;
    bool removeKeyBinding(int bindingId) override;
    bool setKeyBindingEnabled(int bindingId, bool enabled) override;
    
    // Mapping system
    MapEngine& getMapEngine() override;
    
    // Configuration
    void setConfig(const std::string& key, const std::string& value) override;
    std::string getConfig(const std::string& key, const std::string& defaultValue = "") const override;
    
    // Profile management
    bool saveProfile(const std::string& filename = "") override;
    bool loadProfile(const std::string& filename) override;
    void reset(bool keepConnection = false) override;

private:
    std::string mName;
    ConfigManager& mConfigManager;
    bool mInitialized = false;
    
    // Core components
    std::unique_ptr<TextBufferImpl> mMainBuffer;
    std::unordered_map<std::string, std::shared_ptr<TextBufferImpl>> mNamedBuffers;
    
    // Engines
    std::unique_ptr<ScriptEngineImpl> mScriptEngine;
    std::unique_ptr<TriggerEngineImpl> mTriggerEngine;
    std::unique_ptr<AliasEngineImpl> mAliasEngine;
    std::unique_ptr<TimerEngineImpl> mTimerEngine;
    std::unique_ptr<KeyBindEngineImpl> mKeyBindEngine;
    std::unique_ptr<MapEngineImpl> mMapEngine;
    std::unique_ptr<NetworkEngineImpl> mNetworkEngine;
    
    // Callbacks
    TextCallback mTextCallback;
    ConnectionCallback mConnectionCallback;
    
    // Thread safety
    mutable std::mutex mMutex;
    
    // Internal methods
    void setupEngineCallbacks();
    void handleIncomingText(const std::string& text, const std::unordered_map<std::string, std::string>& formatting);
    void handleConnectionStatusChange(bool connected, const std::string& error);
    void processUserCommand(const std::string& command);
    
    // Configuration helpers
    std::string getSessionConfigPath() const;
    void loadDefaultSettings();
};

} // namespace mudlet

#endif // LIBMUDLET_SESSION_IMPL_H