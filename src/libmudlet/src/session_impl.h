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

#include "mudlet/session_simple.h"
#include "engines/trigger_engine_impl.h"
#include "engines/alias_engine_impl.h"
#include "engines/timer_engine_impl.h"
#include <memory>
#include <mutex>

namespace mudlet {

/**
 * @brief Session implementation for individual MUD connections
 */
class SessionImpl : public Session {
public:
    explicit SessionImpl(const std::string& name);
    ~SessionImpl() override;
    
    // Session interface implementation
    std::string getName() const override;
    ConnectionState getConnectionState() const override;
    
    bool connect(const ConnectionInfo& info, ConnectionCallback callback) override;
    void disconnect() override;
    bool isConnected() const override;
    
    void send(const std::string& text, bool echo = true) override;
    
    TriggerEngine& getTriggerEngine() override;
    AliasEngine& getAliasEngine() override;
    TimerEngine& getTimerEngine() override;
    ScriptEngine& getScriptEngine() override;
    TextBuffer& getTextBuffer() override;
    NetworkEngine& getNetworkEngine() override;
    
    void processEvents() override;
    
    void setTextReceivedCallback(TextReceivedCallback callback) override;
    void setConnectionStatusCallback(ConnectionStatusCallback callback) override;

private:
    std::string mName;
    ConnectionState mConnectionState = ConnectionState::Disconnected;
    
    // Engines
    std::unique_ptr<TriggerEngineImpl> mTriggerEngine;
    std::unique_ptr<AliasEngineImpl> mAliasEngine;
    std::unique_ptr<TimerEngineImpl> mTimerEngine;
    
    // Callbacks
    TextReceivedCallback mTextReceivedCallback;
    ConnectionStatusCallback mConnectionStatusCallback;
    
    mutable std::mutex mMutex;
};

} // namespace mudlet

#endif // LIBMUDLET_SESSION_IMPL_H