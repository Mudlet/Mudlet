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
    Q_UNUSED(L)
    mudlet* pMudlet = mudlet::self();

    if (!pMudlet->aiModelAvailable()) {
        return {false, qsl("AI is not available")};
    }

    if (!pMudlet->aiRunning()) {
        return {false, qsl("AI is not currently running")};
    }

    return {true, QString()};
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
    QString eventName = "aiChatResponse"; // Default event name

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

    // Always use event-based approach (async)
    aiManager->chatCompletion(request, [&host, eventName](const LlamafileManager::ApiResponse& response) {
        TEvent event {};

        // Add event name as first argument
        event.mArgumentList.append(eventName);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        // Add success status
        event.mArgumentList.append(response.success ? QLatin1String("1") : QLatin1String("0"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);

        // Add error message
        event.mArgumentList.append(response.error);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        // Add response content
        QString content;
        if (response.success && response.data.contains("content")) {
            content = response.data["content"].toString();
            // Strip leading "\n " and trailing "</s>"
            if (content.startsWith("\n ")) {
                content = content.mid(2);
            }
            if (content.endsWith("</s>")) {
                content.chop(4);
            }
        }
        event.mArgumentList.append(content);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        host.raiseEvent(event);
    });

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#aiPrompt
int TLuaInterpreter::aiPrompt(lua_State* L)
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
    QString eventName = "aiPromptResponse"; // Default event name

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

            lua_pushstring(L, "event");
            lua_gettable(L, 2);
            if (lua_isstring(L, -1)) {
                eventName = QString::fromUtf8(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }

    LlamafileManager::ApiRequest request;
    request.prompt = prompt; // Use prompt field for text completion
    request.temperature = temperature;
    if (maxTokens > 0) {
        request.maxTokens = maxTokens;
    }
    request.stream = stream;

    auto* aiManager = pMudlet->getAIManager();

    aiManager->textCompletion(request, [&host, eventName](const LlamafileManager::ApiResponse& response) {
        TEvent event {};

        // Add event name as first argument
        event.mArgumentList.append(eventName);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        // Add success status
        event.mArgumentList.append(response.success ? QLatin1String("1") : QLatin1String("0"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);

        // Add error message
        event.mArgumentList.append(response.error);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        // Add response content
        QString content;
        if (response.success && response.data.contains("content")) {
            content = response.data["content"].toString();
            // Strip leading "\n " and trailing "</s>"
            if (content.startsWith("\n ")) {
                content = content.mid(2);
            }
            if (content.endsWith("</s>")) {
                content.chop(4);
            }
        }
        event.mArgumentList.append(content);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        qDebug() << "event for aiPrompt:" << event;
        host.raiseEvent(event);
    });

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#aiPromptStream
int TLuaInterpreter::aiPromptStream(lua_State* L)
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
    int maxTokens = 0; // 0 means no limit
    QString eventName = "aiPromptStreamResponse"; // Default event name

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
    if (maxTokens > 0) {
        request.maxTokens = maxTokens;
    }
    request.stream = true;

    auto* aiManager = pMudlet->getAIManager();

    // For streaming, we need to handle the response differently
    // This will require modifications to LlamaFileManager to support streaming callbacks
    aiManager->textCompletionStream(request,
        // Chunk callback - fired for each streaming chunk
        [&host, eventName](const QString& chunk, bool isComplete) {
            TEvent event {};

            // Add event name as first argument
            event.mArgumentList.append(eventName);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            // Add chunk type
            event.mArgumentList.append(isComplete ? QLatin1String("complete") : QLatin1String("partial"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            // Add success status (always true for chunks, errors handled separately)
            event.mArgumentList.append(QLatin1String("1"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);

            // Add empty error message for chunks
            event.mArgumentList.append(QString());
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            // Add chunk content
            QString content = chunk;
            // Strip leading "\n " and trailing "</s>" for final chunk
            if (isComplete) {
                if (content.startsWith("\n ")) {
                    content = content.mid(2);
                }
                if (content.endsWith("</s>")) {
                    content.chop(4);
                }
            }
            event.mArgumentList.append(content);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            host.raiseEvent(event);
        },
        // Error callback - fired on error
        [&host, eventName](const QString& error) {
            TEvent event {};

            // Add event name as first argument
            event.mArgumentList.append(eventName);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            // Add chunk type
            event.mArgumentList.append(QLatin1String("error"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            // Add success status (false for errors)
            event.mArgumentList.append(QLatin1String("0"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);

            // Add error message
            event.mArgumentList.append(error);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            // Add empty content
            event.mArgumentList.append(QString());
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            host.raiseEvent(event);
        }
    );

    lua_pushboolean(L, true);
    return 1;
}
