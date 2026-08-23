/***************************************************************************
 *   Copyright (C) 2025-2026 by Vadim Peretokin - vadim.peretokin@mudlet.org *
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

#include "TMCPLuaBridge.h"

#include "Host.h"
#include "HostManager.h"
#include "TLuaInterpreter.h"
#include "mudlet.h"
#include "utils.h"

#include <QJsonDocument>
#include <QStringList>

// Runs the caller's code with print() diverted into a table, then puts print back.
// Keeping the whole dance inside one chunk keeps the saved print and the collected lines
// as locals; an earlier version parked them in _G, where they outlived the call and
// collided with any script that happened to use those names.
static const char* csmLuaRunner = R"LUA(
local code = ...
local lines = {}
local realPrint = print
print = function(...)
    local parts = {}
    for i = 1, select('#', ...) do
        parts[i] = tostring((select(i, ...)))
    end
    lines[#lines + 1] = table.concat(parts, '\t')
end

local chunk, syntaxError = loadstring(code, 'mcp')
if not chunk then
    print = realPrint
    return false, table.concat(lines, '\n'), syntaxError
end

local returned = { pcall(chunk) }
print = realPrint
local ok = table.remove(returned, 1)
return ok, table.concat(lines, '\n'), unpack(returned)
)LUA";

// A table that refers to itself would otherwise recurse until the C stack gives out, and
// the code being rendered here is written by a model, so _G is a plausible return value.
static constexpr int csmMaxTableDepth = 12;

TMCPLuaBridge::TMCPLuaBridge(Host* pHost, QObject* parent)
: QObject(parent)
, mpHost(pHost)
{
}

bool TMCPLuaBridge::hasTool(const QString& toolName) const
{
    return toolName == QLatin1String(MCP_LUA_TOOL);
}

QJsonArray TMCPLuaBridge::getAvailableTools() const
{
    QJsonObject codeArgument;
    codeArgument[qsl("type")] = qsl("string");
    //: Describes an argument of the MCP "lua" tool to an AI model. Keep print() as-is, it is Lua code.
    codeArgument[qsl("description")] = tr("Lua code to run. Report values back with print(), or return them.");

    QJsonObject profileArgument;
    profileArgument[qsl("type")] = qsl("string");
    //: Describes an argument of the MCP "lua" tool to an AI model.
    profileArgument[qsl("description")] = tr("Name of the profile to run the code in. Defaults to the active profile.");

    QJsonObject properties;
    properties[qsl("code")] = codeArgument;
    properties[qsl("profile")] = profileArgument;

    QJsonObject inputSchema;
    inputSchema[qsl("type")] = qsl("object");
    inputSchema[qsl("properties")] = properties;
    inputSchema[qsl("required")] = QJsonArray{qsl("code")};

    QJsonObject tool;
    tool[qsl("name")] = QString::fromLatin1(MCP_LUA_TOOL);
    //: Human-readable name of the MCP "lua" tool.
    tool[qsl("title")] = tr("Run Lua in Mudlet");
    //: Describes the MCP "lua" tool to an AI model. "Lua" and "MUD" are proper nouns.
    tool[qsl("description")] = tr("Runs Lua code inside the Mudlet MUD client and returns whatever it printed or returned.");
    tool[qsl("inputSchema")] = inputSchema;

    return QJsonArray{tool};
}

MCPToolResult TMCPLuaBridge::callTool(const QString& toolName, const QJsonObject& arguments)
{
    if (!hasTool(toolName)) {
        return {false, tr("Unknown tool: %1").arg(toolName)};
    }

    const QString luaCode = arguments.value(qsl("code")).toString();
    if (luaCode.isEmpty()) {
        return {false, tr("The 'code' argument is required and cannot be empty.")};
    }

    return runLua(luaCode, arguments.value(qsl("profile")).toString());
}

MCPToolResult TMCPLuaBridge::runLua(const QString& luaCode, const QString& profileName)
{
    Host* pTarget = mpHost;
    if (!profileName.isEmpty()) {
        pTarget = mudlet::self() ? mudlet::self()->getHostManager().getHost(profileName) : nullptr;
        if (!pTarget) {
            return {false, tr("No profile named '%1' is open.").arg(profileName)};
        }
    } else if (!pTarget) {
        pTarget = mudlet::self() ? mudlet::self()->getActiveHost() : nullptr;
        if (!pTarget) {
            return {false, tr("No profile is open to run Lua in. Open one, or name a profile in the 'profile' argument.")};
        }
    }

    lua_State* L = pTarget->getLuaInterpreter()->pGlobalLua;
    if (!L) {
        return {false, tr("That profile has no Lua interpreter running.")};
    }

    const int stackBefore = lua_gettop(L);

    if (luaL_loadstring(L, csmLuaRunner) != 0) {
        const QString message = QString::fromUtf8(lua_tostring(L, -1));
        lua_settop(L, stackBefore);
        return {false, tr("Could not compile Mudlet's Lua runner: %1").arg(message)};
    }

    lua_pushstring(L, luaCode.toUtf8().constData());
    if (lua_pcall(L, 1, LUA_MULTRET, 0) != 0) {
        const QString message = QString::fromUtf8(lua_tostring(L, -1));
        lua_settop(L, stackBefore);
        return {false, tr("Lua error: %1").arg(message)};
    }

    // The runner always answers with ok, printed output, then whatever the code returned.
    if (lua_gettop(L) - stackBefore < 2) {
        lua_settop(L, stackBefore);
        return {false, tr("Mudlet's Lua runner returned nothing.")};
    }

    const bool ok = lua_toboolean(L, stackBefore + 1) != 0;
    const QString printed = QString::fromUtf8(lua_tostring(L, stackBefore + 2));
    const int firstValue = stackBefore + 3;
    const int lastValue = lua_gettop(L);

    QStringList pieces;
    if (!printed.isEmpty()) {
        pieces << printed;
    }

    if (!ok) {
        const QString message = firstValue <= lastValue ? QString::fromUtf8(lua_tostring(L, firstValue)) : tr("unknown error");
        pieces << tr("Lua error: %1").arg(message);
        lua_settop(L, stackBefore);
        return {false, pieces.join(QChar::LineFeed)};
    }

    QStringList values;
    for (int i = firstValue; i <= lastValue; ++i) {
        values << jsonToText(luaToJson(L, i, 0));
    }
    lua_settop(L, stackBefore);

    if (!values.isEmpty()) {
        pieces << values.join(QChar::Tabulation);
    }

    if (pieces.isEmpty()) {
        return {true, tr("The code ran and produced no output.")};
    }
    return {true, pieces.join(QChar::LineFeed)};
}

QJsonValue TMCPLuaBridge::luaToJson(lua_State* L, int index, int depth)
{
    // Work in absolute indices so that pushing onto the stack while walking a table does
    // not shift the slot being read out from under us.
    if (index < 0) {
        index = lua_gettop(L) + 1 + index;
    }

    switch (lua_type(L, index)) {
    case LUA_TNIL:
        return QJsonValue();
    case LUA_TBOOLEAN:
        return QJsonValue(lua_toboolean(L, index) != 0);
    case LUA_TNUMBER:
        return QJsonValue(lua_tonumber(L, index));
    case LUA_TSTRING:
        return QJsonValue(QString::fromUtf8(lua_tostring(L, index)));
    case LUA_TTABLE:
        return luaTableToJson(L, index, depth);
    default:
        return QJsonValue(qsl("<%1>").arg(QString::fromUtf8(lua_typename(L, lua_type(L, index)))));
    }
}

QJsonValue TMCPLuaBridge::luaTableToJson(lua_State* L, int index, int depth)
{
    if (depth >= csmMaxTableDepth) {
        return QJsonValue(qsl("<table nested too deeply>"));
    }

    // A Lua table is a list and a map at once. Render a plain 1..n sequence as a JSON
    // array so that a model reading the result sees a list, not {"1":..., "2":...}.
    const int length = static_cast<int>(lua_objlen(L, index));
    int keyCount = 0;
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        ++keyCount;
        lua_pop(L, 1);
    }

    if (length > 0 && keyCount == length) {
        QJsonArray array;
        for (int i = 1; i <= length; ++i) {
            lua_rawgeti(L, index, i);
            array.append(luaToJson(L, lua_gettop(L), depth + 1));
            lua_pop(L, 1);
        }
        return array;
    }

    QJsonObject object;
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        // lua_tostring() on a number key would rewrite the key in place, and lua_next()
        // then loses its place in the table - so read numbers without converting them.
        QString key;
        if (lua_type(L, -2) == LUA_TSTRING) {
            key = QString::fromUtf8(lua_tostring(L, -2));
        } else if (lua_type(L, -2) == LUA_TNUMBER) {
            key = QString::number(lua_tonumber(L, -2));
        } else {
            key = qsl("<%1>").arg(QString::fromUtf8(lua_typename(L, lua_type(L, -2))));
        }
        object[key] = luaToJson(L, lua_gettop(L), depth + 1);
        lua_pop(L, 1);
    }
    return object;
}

QString TMCPLuaBridge::jsonToText(const QJsonValue& value)
{
    switch (value.type()) {
    case QJsonValue::String:
        return value.toString();
    case QJsonValue::Bool:
        return value.toBool() ? qsl("true") : qsl("false");
    case QJsonValue::Double:
        return QString::number(value.toDouble());
    case QJsonValue::Array:
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    case QJsonValue::Object:
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    default:
        return qsl("nil");
    }
}
