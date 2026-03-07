/***************************************************************************
 *   Copyright (C) 2024 by John McKisson - john.mckisson@gmail.com         *
 *   Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "MMCP.h"
#include "MMCPClient.h"
#include "MMCPServer.h"
#include "TLuaInterpreter.h"

int TLuaInterpreter::mmcpChatTo(lua_State* L)
{
    const char* sFunc = "mmcp.chatTo";
    const QString target = getVerifiedString(L, sFunc, 1, "target");
    const QString msg = getVerifiedString(L, sFunc, 2, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->chatTo(target, msg);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpAccept(lua_State* L)
{
    const char* sFunc = "mmcp.accept";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->chatAccept(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpChatAll(lua_State* L)
{
    const char* sFunc = "mmcp.chatAll";
    const QString msg = getVerifiedString(L, sFunc, 1, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->chatAll(msg);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpAllowSnoop(lua_State* L)
{
    const char* sFunc = "mmcp.allowSnoop";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->allowSnoop(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpCall(lua_State* L)
{
    const char* sFunc = "mmcp.call";
    const QString host = getVerifiedString(L, sFunc, 1, "host");
    int port = csDefaultMMCPHostPort;

    const int n = lua_gettop(L);
    if (n > 1) {
        port = getVerifiedInt(L, sFunc, 2, qsl("port number {default = %1}").arg(csDefaultMMCPHostPort).toUtf8().constData(), true);
        if (port > 65535 || port < 1) {
            return warnArgumentValue(L, sFunc, qsl("invalid port number %1 given, if supplied it must be in range 1 to 65535").arg(port));
        }
    }

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->call(host, port);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpDeny(lua_State* L)
{
    const char* sFunc = "mmcp.deny";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->chatDeny(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpDoNotDisturb(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    pHost->mMMCPServer->toggleDoNotDisturb();

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpEmoteAll(lua_State* L)
{
    const char* sFunc = "mmcp.emoteAll";
    const QString msg = getVerifiedString(L, sFunc, 1, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->emoteAll(msg);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpChatGroup(lua_State* L)
{
    const char* sFunc = "mmcp.chatGroup";
    const QString group = getVerifiedString(L, sFunc, 1, "group");
    const QString msg = getVerifiedString(L, sFunc, 2, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->chatGroup(group, msg);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpIgnore(lua_State* L)
{
    const char* sFunc = "mmcp.ignore";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->ignore(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpDisplayClientList(lua_State* L)
{
    const char* sFunc = "mmcp.displayClientList";
    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->displayClientList();
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpChatName(lua_State* L)
{
    const char* sFunc = "mmcp.chatName";
    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const int n = lua_gettop(L);
    QString name;
    if (n > 0) {
        name = getVerifiedString(L, sFunc, 1, "name");
        const auto result = pHost->mMMCPServer->chatName(name);
        if (!result.first) {
            return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
        }

        lua_pushboolean(L, true);
    } else {
        name = pHost->mMMCPServer->getChatName();
        lua_pushstring(L, name.toUtf8().constData());
    }

    return 1;
}


int TLuaInterpreter::mmcpPing(lua_State* L)
{
    const char* sFunc = "mmcp.ping";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->ping(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpPeekConnections(lua_State* L)
{
    const char* sFunc = "mmcp.peek";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->peek(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpPrivate(lua_State* L)
{
    const char* sFunc = "mmcp.setPrivate";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->chatPrivate(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpRequestConnections(lua_State* L)
{
    const char* sFunc = "mmcp.request";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->request(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpServe(lua_State* L)
{
    const char* sFunc = "mmcp.serve";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->serve(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpSetGroup(lua_State* L)
{
    const char* sFunc = "mmcp.setGroup";
    const QString target = getVerifiedString(L, sFunc, 1, "target");
    const QString group = getVerifiedString(L, sFunc, 2, "group");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->chatSetGroup(target, group);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpSendSideChannel(lua_State* L)
{
    const char* sFunc = "mmcp.sendSideChannel";
    const QString channel = getVerifiedString(L, sFunc, 1, "channel");
    const QString message = getVerifiedString(L, sFunc, 2, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->sendSideChannel(channel, message);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpSnoop(lua_State* L)
{
    const char* sFunc = "mmcp.snoop";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->snoop(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpStartServer(lua_State* L)
{
    const char* sFunc = "mmcp.startServer";
    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    int port = pHost->getMMCPPort();
    if (lua_gettop(L) > 0) {
        port = getVerifiedInt(L, sFunc, 1, qsl("port number {default = %1}").arg(pHost->getMMCPPort()).toUtf8().constData(), true);
        if (port > 65535 || port < 1) {
            return warnArgumentValue(L, sFunc, qsl("invalid port number %1 given, if supplied it must be in range 1 to 65535").arg(port));
        }
    }

    const auto result = pHost->mMMCPServer->startServer(port);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpStopServer(lua_State* L)
{
    const char* sFunc = "mmcp.stopServer";
    Host* pHost = &getHostFromLua(L);
    if (pHost->mMMCPServer) {
        const auto result = pHost->mMMCPServer->stopServer();
        if (!result.first) {
            return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
        }
    } 
    
    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpDisconnect(lua_State* L)
{
    const char* sFunc = "mmcp.disconnect";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->disconnect(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::mmcpGetClientList(lua_State* L) {
    Host* pHost = &getHostFromLua(L);

    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const QList<QPointer<MMCPClient>>& clients = pHost->mMMCPServer->getClients();
    if (clients.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);

    int i = 0;
    QListIterator<QPointer<MMCPClient>> it(clients);
    while (it.hasNext()) {
        MMCPClient* pClient = it.next();

        if (!pClient) {
            continue;
        }

        // Create a new inner table for the client.
        lua_newtable(L);

        lua_pushstring(L, "id");
        lua_pushnumber(L, pClient->id());
        lua_settable(L, -3);

        lua_pushstring(L, "name");
        lua_pushstring(L, pClient->chatName().toUtf8().constData());
        lua_settable(L, -3);

        lua_pushstring(L, "host");
        lua_pushstring(L, pClient->host().toUtf8().constData());
        lua_settable(L, -3);

        lua_pushstring(L, "port");
        lua_pushnumber(L, pClient->port());
        lua_settable(L, -3);

        lua_pushstring(L, "version");
        lua_pushstring(L, pClient->getVersion().toUtf8().constData());
        lua_settable(L, -3);

        
        lua_pushnumber(L, ++i); // Push outer table key (index)
        lua_insert(L, -2);  // Swap the inner table and key so that the table is on top
        lua_settable(L, -3);   // Set the inner table in the outer table.
    }

    return 1;
}

int TLuaInterpreter::mmcpGetClientFlags(lua_State* L)
{
    const char* sFunc = "mmcp.getClientFlags";
    const QString target = getVerifiedString(L, sFunc, 1, "target");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mMMCPServer) {
        pHost->initMMCPServer();
    }

    const auto result = pHost->mMMCPServer->getClientFlags(target);
    if (!result.first) {
        return warnArgumentValue(L, sFunc, result.second.toUtf8().constData());
    }

    lua_pushstring(L, result.second.toUtf8().constData());
    return 1;
}
