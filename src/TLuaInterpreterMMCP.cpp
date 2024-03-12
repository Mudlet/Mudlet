/***************************************************************************
 *   Copyright (C) 2024 by John McKisson - john.mckisson@gmail.com         *
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

// mmcp-specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

#include "Host.h"
#include "MMCPServer.h"
#include "TLuaInterpreter.h"

int TLuaInterpreter::chat(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");
    const QString msg = getVerifiedString(L, __func__, 2, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->chat(target, msg);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatAll(lua_State* L)
{
    const QString msg = getVerifiedString(L, __func__, 1, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->chatAll(msg);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatAllowSnoop(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->allowSnoop(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatCall(lua_State* L)
{
    const QString host = getVerifiedString(L, __func__, 1, "host");
    int port = MMCPServer::MMCPDefaultHostPort;

    const int n = lua_gettop(L);
    if (n > 1) {
        port = getVerifiedInt(L, __func__, 2, "port number {default = 4050}", true);
        if (port > 65535 || port < 1) {
            return warnArgumentValue(L, __func__, qsl("invalid port number %1 given, if supplied it must be in range 1 to 65535").arg(port));
        }
    }

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->call(host, port);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatDoNotDisturb(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    pHost->mmcpServer->toggleDoNotDisturb();

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatEmoteAll(lua_State* L)
{
    const QString msg = getVerifiedString(L, __func__, 1, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->emoteAll(msg);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatGroup(lua_State* L)
{
    const QString group = getVerifiedString(L, __func__, 1, "group");
    const QString msg = getVerifiedString(L, __func__, 2, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->chatGroup(group, msg);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatIgnore(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->ignore(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatList(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->chatList();
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatName(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const int n = lua_gettop(L);
    QString name;
    if (n > 0) {
        name = getVerifiedString(L, __func__, 1, "name");
        const auto result = pHost->mmcpServer->chatName(name);
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
        }

        lua_pushboolean(L, true);
    } else {
        name = pHost->mmcpServer->getChatName();
        lua_pushstring(L, name.toUtf8().constData());
    }

    return 1;
}

int TLuaInterpreter::chatPing(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->ping(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatPeek(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->peek(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatPrivate(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->chatPrivate(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatServe(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->serve(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatSetGroup(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");
    const QString group = getVerifiedString(L, __func__, 2, "group");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->chatSetGroup(target, group);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatSideChannel(lua_State* L)
{
    const QString channel = getVerifiedString(L, __func__, 1, "channel");
    const QString message = getVerifiedString(L, __func__, 2, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->chatSideChannel(channel, message);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatSnoop(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->snoop(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatStartServer(lua_State* L)
{
    int port = 4050;
    if (lua_gettop(L) > 0) {
        port = getVerifiedInt(L, __func__, 1, "port number {default = 4050}", true);
        if (port > 65535 || port < 1) {
            return warnArgumentValue(L, __func__, qsl("invalid port number %1 given, if supplied it must be in range 1 to 65535").arg(port));
        }
    }

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->startServer(port);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatStopServer(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (pHost->mmcpServer) {
        const auto result = pHost->mmcpServer->stopServer();
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
        }
    } 
    
    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::chatUnChat(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mmcpServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mmcpServer->unChat(target);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}
