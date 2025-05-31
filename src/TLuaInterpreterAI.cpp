/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// AI-specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

#include "TLuaInterpreter.h"

#include "Host.h"
#include "LlamaFileManager.h"
#include "mudlet.h"
#include "TEvent.h"

#include "pre_guard.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "post_guard.h"

// No documentation available in wiki - internal function
std::pair<bool, QString> TLuaInterpreter::aiEnabled(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    
    if (!pMudlet->isAIAvailable()) {
        return {false, qsl("AI is not available")};
    }
    
    if (!pMudlet->isAIRunning()) {
        return {false, qsl("AI is not currently running")};
    }
    
    return {true, QString()};
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAIAvailable
int TLuaInterpreter::isAIAvailable(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    lua_pushboolean(L, pMudlet->isAIAvailable());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAIRunning
int TLuaInterpreter::isAIRunning(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    lua_pushboolean(L, pMudlet->isAIRunning());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAIModelPath
int TLuaInterpreter::getAIModelPath(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    lua_pushstring(L, pMudlet->getAIModelPath().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#aiChat
int TLuaInterpreter::aiChat(lua_State* L)
{
    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    
    auto result = aiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }
    
    const QString prompt = getVerifiedString(L, __func__, 1, "prompt");
    if (prompt.isEmpty()) {
        return warnArgumentValue(L, __func__, "prompt cannot be empty");
    }
    
    // Optional parameters
    double temperature = 0.7;
    int maxTokens = 150;
    bool stream = false;
    QString eventName;
    
    if (lua_gettop(L) >= 2) {
        if (lua_istable(L, 2)) {
            lua_pushstring(L, "temperature");
            lua_gettable(L, 2);
            if (lua_isnumber(L, -1)) {
                temperature = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            lua_pushstring(L, "max_tokens");
            lua_gettable(L, 2);
            if (lua_isnumber(L, -1)) {
                maxTokens = static_cast<int>(lua_tointeger(L, -1));
            }
            lua_pop(L, 1);
            
            lua_pushstring(L, "stream");
            lua_gettable(L, 2);
            if (lua_isboolean(L, -1)) {
                stream = lua_toboolean(L, -1);
            }
            lua_pop(L, 1);
            
            lua_pushstring(L, "event");
            lua_gettable(L, 2);
            if (lua_isstring(L, -1)) {
                eventName = QString::fromUtf8(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    
    // Build messages array for chat completion
    QJsonArray messages;
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);
    
    LlamafileManager::ApiRequest request;
    request.messages = QJsonObject{{"messages", messages}};
    request.temperature = temperature;
    request.maxTokens = maxTokens;
    request.stream = stream;
    
    auto* aiManager = pMudlet->getAIManager();
    
    if (stream && !eventName.isEmpty()) {
        // Streaming mode with events
        aiManager->chatCompletion(request, [&host, eventName](const LlamafileManager::ApiResponse& response) {
            QStringList eventData;
            eventData << (response.success ? "true" : "false");
            eventData << response.error;
            
            if (response.success && response.data.contains("choices")) {
                const QJsonArray choices = response.data["choices"].toArray();
                if (!choices.isEmpty()) {
                    const QJsonObject choice = choices[0].toObject();
                    const QJsonObject message = choice["message"].toObject();
                    eventData << message["content"].toString();
                } else {
                    eventData << "";
                }
            } else {
                eventData << "";
            }
            
            auto pEvent = new TEvent(eventName, eventData);
            host.raiseEvent(pEvent);
        });
        
        lua_pushboolean(L, true);
        return 1;
    } else {
        // Synchronous mode - use coroutine
        auto L_coroutine = lua_newthread(L);
        lua_pushvalue(L, lua_gettop(L)); // Copy the thread to the top
        int threadRef = luaL_ref(L, LUA_REGISTRYINDEX); // Store thread reference
        
        aiManager->chatCompletion(request, [L_coroutine, threadRef](const LlamafileManager::ApiResponse& response) {
            if (response.success) {
                if (response.data.contains("choices")) {
                    const QJsonArray choices = response.data["choices"].toArray();
                    if (!choices.isEmpty()) {
                        const QJsonObject choice = choices[0].toObject();
                        const QJsonObject message = choice["message"].toObject();
                        const QString content = message["content"].toString();
                        
                        lua_pushboolean(L_coroutine, true);
                        lua_pushstring(L_coroutine, content.toUtf8().constData());
                    } else {
                        lua_pushboolean(L_coroutine, false);
                        lua_pushstring(L_coroutine, "No response content");
                    }
                } else {
                    lua_pushboolean(L_coroutine, false);
                    lua_pushstring(L_coroutine, "Invalid response format");
                }
            } else {
                lua_pushboolean(L_coroutine, false);
                lua_pushstring(L_coroutine, response.error.toUtf8().constData());
            }
            
            // Resume the coroutine
            int nresults = 0;
            int status = lua_resume(L_coroutine, nullptr, 0, &nresults);
            
            // Clean up the thread reference
            luaL_unref(L_coroutine, LUA_REGISTRYINDEX, threadRef);
        });
        
        // Yield the coroutine
        return lua_yield(L_coroutine, 0);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#aiComplete
int TLuaInterpreter::aiComplete(lua_State* L)
{
    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    
    auto result = aiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }
    
    const QString prompt = getVerifiedString(L, __func__, 1, "prompt");
    if (prompt.isEmpty()) {
        return warnArgumentValue(L, __func__, "prompt cannot be empty");
    }
    
    // Optional parameters
    double temperature = 0.7;
    int maxTokens = 150;
    bool stream = false;
    QString eventName;
    
    if (lua_gettop(L) >= 2) {
        if (lua_istable(L, 2)) {
            lua_pushstring(L, "temperature");
            lua_gettable(L, 2);
            if (lua_isnumber(L, -1)) {
                temperature = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            lua_pushstring(L, "max_tokens");
            lua_gettable(L, 2);
            if (lua_isnumber(L, -1)) {
                maxTokens = static_cast<int>(lua_tointeger(L, -1));
            }
            lua_pop(L, 1);
            
            lua_pushstring(L, "stream");
            lua_gettable(L, 2);
            if (lua_isboolean(L, -1)) {
                stream = lua_toboolean(L, -1);
            }
            lua_pop(L, 1);
            
            lua_pushstring(L, "event");
            lua_gettable(L, 2);
            if (lua_isstring(L, -1)) {
                eventName = QString::fromUtf8(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    
    LlamafileManager::ApiRequest request;
    request.prompt = prompt;
    request.temperature = temperature;
    request.maxTokens = maxTokens;
    request.stream = stream;
    
    auto* aiManager = pMudlet->getAIManager();
    
    if (stream && !eventName.isEmpty()) {
        // Streaming mode with events
        aiManager->textCompletion(request, [&host, eventName](const LlamafileManager::ApiResponse& response) {
            QStringList eventData;
            eventData << (response.success ? "true" : "false");
            eventData << response.error;
            
            if (response.success && response.data.contains("content")) {
                eventData << response.data["content"].toString();
            } else {
                eventData << "";
            }
            
            auto pEvent = new TEvent(&host, eventName, eventData);
            host.raiseEvent(pEvent);
        });
        
        lua_pushboolean(L, true);
        return 1;
    } else {
        // Synchronous mode - use coroutine
        auto L_coroutine = lua_newthread(L);
        lua_pushvalue(L, lua_gettop(L)); // Copy the thread to the top
        int threadRef = luaL_ref(L, LUA_REGISTRYINDEX); // Store thread reference
        
        aiManager->textCompletion(request, [L_coroutine, threadRef](const LlamafileManager::ApiResponse& response) {
            if (response.success) {
                if (response.data.contains("content")) {
                    const QString content = response.data["content"].toString();
                    lua_pushboolean(L_coroutine, true);
                    lua_pushstring(L_coroutine, content.toUtf8().constData());
                } else {
                    lua_pushboolean(L_coroutine, false);
                    lua_pushstring(L_coroutine, "No response content");
                }
            } else {
                lua_pushboolean(L_coroutine, false);
                lua_pushstring(L_coroutine, response.error.toUtf8().constData());
            }
            
            // Resume the coroutine
            int nresults = 0;
            int status = lua_resume(L_coroutine, nullptr, 0, &nresults);
            
            // Clean up the thread reference
            luaL_unref(L_coroutine, LUA_REGISTRYINDEX, threadRef);
        });
        
        // Yield the coroutine
        return lua_yield(L_coroutine, 0);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#aiEmbeddings
int TLuaInterpreter::aiEmbeddings(lua_State* L)
{
    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    
    auto result = aiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }
    
    QStringList input;
    
    if (lua_isstring(L, 1)) {
        // Single string input
        input << getVerifiedString(L, __func__, 1, "text");
    } else if (lua_istable(L, 1)) {
        // Array of strings
        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            if (lua_isstring(L, -1)) {
                input << QString::fromUtf8(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    } else {
        return warnArgumentValue(L, __func__, "input must be a string or array of strings");
    }
    
    if (input.isEmpty()) {
        return warnArgumentValue(L, __func__, "input cannot be empty");
    }
    
    // Optional event name for streaming
    QString eventName;
    if (lua_gettop(L) >= 2) {
        eventName = getVerifiedString(L, __func__, 2, "event name", true);
    }
    
    LlamafileManager::ApiRequest request;
    request.input = input;
    
    auto* aiManager = pMudlet->getAIManager();
    
    if (!eventName.isEmpty()) {
        // Event-based mode
        aiManager->embeddings(request, [&host, eventName](const LlamafileManager::ApiResponse& response) {
            QStringList eventData;
            eventData << (response.success ? "true" : "false");
            eventData << response.error;
            
            if (response.success && response.data.contains("data")) {
                const QJsonArray data = response.data["data"].toArray();
                QStringList embeddings;
                for (const auto& item : data) {
                    const QJsonObject obj = item.toObject();
                    const QJsonArray embedding = obj["embedding"].toArray();
                    QStringList values;
                    for (const auto& val : embedding) {
                        values << QString::number(val.toDouble());
                    }
                    embeddings << values.join(",");
                }
                eventData << embeddings.join(";");
            } else {
                eventData << "";
            }
            
            auto pEvent = new TEvent(&host, eventName, eventData);
            host.raiseEvent(pEvent);
        });
        
        lua_pushboolean(L, true);
        return 1;
    } else {
        // Synchronous mode - use coroutine
        auto L_coroutine = lua_newthread(L);
        lua_pushvalue(L, lua_gettop(L)); // Copy the thread to the top
        int threadRef = luaL_ref(L, LUA_REGISTRYINDEX); // Store thread reference
        
        aiManager->embeddings(request, [L_coroutine, threadRef](const LlamafileManager::ApiResponse& response) {
            if (response.success) {
                if (response.data.contains("data")) {
                    const QJsonArray data = response.data["data"].toArray();
                    
                    lua_pushboolean(L_coroutine, true);
                    lua_newtable(L_coroutine); // Create table for embeddings
                    
                    int index = 1;
                    for (const auto& item : data) {
                        const QJsonObject obj = item.toObject();
                        const QJsonArray embedding = obj["embedding"].toArray();
                        
                        lua_newtable(L_coroutine); // Create table for this embedding
                        int embIndex = 1;
                        for (const auto& val : embedding) {
                            lua_pushnumber(L_coroutine, val.toDouble());
                            lua_rawseti(L_coroutine, -2, embIndex++);
                        }
                        lua_rawseti(L_coroutine, -2, index++);
                    }
                } else {
                    lua_pushboolean(L_coroutine, false);
                    lua_pushstring(L_coroutine, "No embedding data in response");
                }
            } else {
                lua_pushboolean(L_coroutine, false);
                lua_pushstring(L_coroutine, response.error.toUtf8().constData());
            }
            
            // Resume the coroutine
            int nresults = 0;
            int status = lua_resume(L_coroutine, nullptr, 0, &nresults);
            
            // Clean up the thread reference
            luaL_unref(L_coroutine, LUA_REGISTRYINDEX, threadRef);
        });
        
        // Yield the coroutine
        return lua_yield(L_coroutine, 0);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAIModels
int TLuaInterpreter::getAIModels(lua_State* L)
{
    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    
    auto result = aiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }
    
    // Optional event name for async mode
    QString eventName;
    if (lua_gettop(L) >= 1) {
        eventName = getVerifiedString(L, __func__, 1, "event name", true);
    }
    
    auto* aiManager = pMudlet->getAIManager();
    
    if (!eventName.isEmpty()) {
        // Event-based mode
        aiManager->getModels([&host, eventName](const LlamafileManager::ApiResponse& response) {
            QStringList eventData;
            eventData << (response.success ? "true" : "false");
            eventData << response.error;
            
            if (response.success && response.data.contains("data")) {
                const QJsonArray data = response.data["data"].toArray();
                QStringList models;
                for (const auto& item : data) {
                    const QJsonObject obj = item.toObject();
                    models << obj["id"].toString();
                }
                eventData << models.join(",");
            } else {
                eventData << "";
            }
            
            auto pEvent = new TEvent(&host, eventName, eventData);
            host.raiseEvent(pEvent);
        });
        
        lua_pushboolean(L, true);
        return 1;
    } else {
        // Synchronous mode - use coroutine
        auto L_coroutine = lua_newthread(L);
        lua_pushvalue(L, lua_gettop(L)); // Copy the thread to the top
        int threadRef = luaL_ref(L, LUA_REGISTRYINDEX); // Store thread reference
        
        aiManager->getModels([L_coroutine, threadRef](const LlamafileManager::ApiResponse& response) {
            if (response.success) {
                if (response.data.contains("data")) {
                    const QJsonArray data = response.data["data"].toArray();
                    
                    lua_pushboolean(L_coroutine, true);
                    lua_newtable(L_coroutine); // Create table for models
                    
                    int index = 1;
                    for (const auto& item : data) {
                        const QJsonObject obj = item.toObject();
                        lua_pushstring(L_coroutine, obj["id"].toString().toUtf8().constData());
                        lua_rawseti(L_coroutine, -2, index++);
                    }
                } else {
                    lua_pushboolean(L_coroutine, false);
                    lua_pushstring(L_coroutine, "No model data in response");
                }
            } else {
                lua_pushboolean(L_coroutine, false);
                lua_pushstring(L_coroutine, response.error.toUtf8().constData());
            }
            
            // Resume the coroutine
            int nresults = 0;
            int status = lua_resume(L_coroutine, nullptr, 0, &nresults);
            
            // Clean up the thread reference
            luaL_unref(L_coroutine, LUA_REGISTRYINDEX, threadRef);
        });
        
        // Yield the coroutine
        return lua_yield(L_coroutine, 0);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#aiChatAsync
int TLuaInterpreter::aiChatAsync(lua_State* L)
{
    auto& host = getHostFromLua(L);
    
    auto result = aiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }
    
    const QString prompt = getVerifiedString(L, __func__, 1, "prompt");
    const QString eventName = getVerifiedString(L, __func__, 2, "event name");
    
    if (prompt.isEmpty()) {
        return warnArgumentValue(L, __func__, "prompt cannot be empty");
    }
    
    if (eventName.isEmpty()) {
        return warnArgumentValue(L, __func__, "event name cannot be empty");
    }
    
    // Optional parameters table
    double temperature = 0.7;
    int maxTokens = 150;
    
    if (lua_gettop(L) >= 3 && lua_istable(L, 3)) {
        lua_pushstring(L, "temperature");
        lua_gettable(L, 3);
        if (lua_isnumber(L, -1)) {
            temperature = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        
        lua_pushstring(L, "max_tokens");
        lua_gettable(L, 3);
        if (lua_isnumber(L, -1)) {
            maxTokens = static_cast<int>(lua_tointeger(L, -1));
        }
        lua_pop(L, 1);
    }
    
    // Build messages array for chat completion
    QJsonArray messages;
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);
    
    LlamafileManager::ApiRequest request;
    request.messages = QJsonObject{{"messages", messages}};
    request.temperature = temperature;
    request.maxTokens = maxTokens;
    request.stream = false;
    
    mudlet* pMudlet = mudlet::self();
    auto* aiManager = pMudlet->getAIManager();
    
    aiManager->chatCompletion(request, [&host, eventName](const LlamafileManager::ApiResponse& response) {
        QStringList eventData;
        eventData << (response.success ? "true" : "false");
        eventData << response.error;
        
        if (response.success && response.data.contains("choices")) {
            const QJsonArray choices = response.data["choices"].toArray();
            if (!choices.isEmpty()) {
                const QJsonObject choice = choices[0].toObject();
                const QJsonObject message = choice["message"].toObject();
                eventData << message["content"].toString();
            } else {
                eventData << "";
            }
        } else {
            eventData << "";
        }
        
        auto pEvent = new TEvent(&host, eventName, eventData);
        host.raiseEvent(pEvent);
    });
    
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#aiCompleteAsync
int TLuaInterpreter::aiCompleteAsync(lua_State* L)
{
    auto& host = getHostFromLua(L);
    
    auto result = aiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }
    
    const QString prompt = getVerifiedString(L, __func__, 1, "prompt");
    const QString eventName = getVerifiedString(L, __func__, 2, "event name");
    
    if (prompt.isEmpty()) {
        return warnArgumentValue(L, __func__, "prompt cannot be empty");
    }
    
    if (eventName.isEmpty()) {
        return warnArgumentValue(L, __func__, "event name cannot be empty");
    }
    
    // Optional parameters table
    double temperature = 0.7;
    int maxTokens = 150;
    
    if (lua_gettop(L) >= 3 && lua_istable(L, 3)) {
        lua_pushstring(L, "temperature");
        lua_gettable(L, 3);
        if (lua_isnumber(L, -1)) {
            temperature = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        
        lua_pushstring(L, "max_tokens");
        lua_gettable(L, 3);
        if (lua_isnumber(L, -1)) {
            maxTokens = static_cast<int>(lua_tointeger(L, -1));
        }
        lua_pop(L, 1);
    }
    
    LlamafileManager::ApiRequest request;
    request.prompt = prompt;
    request.temperature = temperature;
    request.maxTokens = maxTokens;
    request.stream = false;
    
    mudlet* pMudlet = mudlet::self();
    auto* aiManager = pMudlet->getAIManager();
    
    aiManager->textCompletion(request, [&host, eventName](const LlamafileManager::ApiResponse& response) {
        QStringList eventData;
        eventData << (response.success ? "true" : "false");
        eventData << response.error;
        
        if (response.success && response.data.contains("content")) {
            eventData << response.data["content"].toString();
        } else {
            eventData << "";
        }
        
        auto pEvent = new TEvent(&host, eventName, eventData);
        host.raiseEvent(pEvent);
    });
    
    lua_pushboolean(L, true);
    return 1;
}

