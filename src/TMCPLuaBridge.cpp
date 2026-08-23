/***************************************************************************
 *   Copyright (C) 2025-2026 Vadim Peretokin - vadim.peretokin@mudlet.org  *
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
#include <QLocale>
#include <QSet>
#include <QStringList>

// Runs the caller's code with print() diverted into a table, then puts print back.
// The whole dance stays inside one chunk so that the saved print and the collected lines
// are locals: anything kept in _G would outlive the call and collide with a script that
// happened to use the same name.
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

-- Count the results rather than measuring them afterwards. A nil anywhere in the list
-- leaves a hole, and both # and unpack() stop at the first one, so the Lua idiom
-- "return nil, reason" would otherwise reach the caller as no values at all.
local returned, count = {}, 0
local function collect(...)
    count = select('#', ...)
    for i = 1, count do
        returned[i] = (select(i, ...))
    end
end
collect(pcall(chunk))
print = realPrint
return returned[1], table.concat(lines, '\n'), unpack(returned, 2, count)
)LUA";

// A table that refers to itself would otherwise recurse until the C stack gives out, and
// the code being rendered here is written by a model, so _G is a plausible return value.
static constexpr int csmMaxTableDepth = 12;

TMCPLuaBridge::TMCPLuaBridge(QObject* parent)
: QObject(parent)
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
    profileArgument[qsl("description")] = tr("Name of the profile to run the code in. Defaults to the profile the user is currently looking at.");

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

    // QJsonValue::toString() answers an empty string for a number, an object or a boolean,
    // so an argument of the wrong type would otherwise read as one that was never sent: a
    // numeric "profile" would quietly run the code in whichever profile is in the
    // foreground rather than the one that was asked for.
    const QJsonValue codeArgument = arguments.value(qsl("code"));
    if (!codeArgument.isUndefined() && !codeArgument.isString()) {
        return {false, tr("The 'code' argument must be a string.")};
    }
    const QString luaCode = codeArgument.toString();
    if (luaCode.isEmpty()) {
        return {false, tr("The 'code' argument is required and cannot be empty.")};
    }

    const QJsonValue profileArgument = arguments.value(qsl("profile"));
    if (!profileArgument.isUndefined() && !profileArgument.isNull() && !profileArgument.isString()) {
        return {false, tr("The 'profile' argument must be a string.")};
    }

    QString failure;
    Host* pTarget = targetHost(profileArgument.toString(), failure);
    if (!pTarget) {
        return {false, failure};
    }

    lua_State* L = pTarget->getLuaInterpreter()->getLuaGlobalState();
    if (!L) {
        return {false, tr("That profile has no Lua interpreter running.")};
    }

    return runLua(L, luaCode);
}

Host* TMCPLuaBridge::targetHost(const QString& profileName, QString& failure)
{
    if (!profileName.isEmpty()) {
        Host* pNamed = mudlet::self() ? mudlet::self()->getHostManager().getHost(profileName) : nullptr;
        if (!pNamed) {
            failure = tr("No profile named '%1' is open.").arg(profileName);
        }
        return pNamed;
    }

    // The foreground profile, which is the one the user would mean by "here". A model that
    // needs to be sure of which profile it is in should name it instead.
    Host* pActive = mudlet::self() ? mudlet::self()->getActiveHost() : nullptr;
    if (!pActive) {
        failure = tr("No profile is open to run Lua in. Open one, or name a profile in the 'profile' argument.");
    }
    return pActive;
}

MCPToolResult TMCPLuaBridge::runLua(lua_State* L, const QString& luaCode)
{
    const int stackBefore = lua_gettop(L);

    if (luaL_loadstring(L, csmLuaRunner) != 0) {
        const QString message = QString::fromUtf8(lua_tostring(L, -1));
        lua_settop(L, stackBefore);
        return {false, tr("Could not compile Mudlet's Lua runner: %1").arg(message)};
    }

    // Push the length too: lua_pushstring() would stop at the first embedded NUL and
    // silently run only the leading fragment of the snippet.
    const QByteArray code = luaCode.toUtf8();
    lua_pushlstring(L, code.constData(), code.size());
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
    const QString printed = readString(L, stackBefore + 2);
    const int firstValue = stackBefore + 3;
    const int lastValue = lua_gettop(L);

    QStringList pieces;
    if (!printed.isEmpty()) {
        pieces << printed;
    }

    if (!ok) {
        // error() can be handed a table rather than a string, and lua_tostring() answers
        // null for one of those, which would drop the message entirely.
        const QString message = firstValue <= lastValue ? jsonToText(luaToJson(L, firstValue, 0)) : tr("unknown error");
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
        return QJsonValue(readString(L, index));
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

    // Each level holds a key and a value from lua_next while it recurses, and the C API
    // promises only LUA_MINSTACK free slots, so ask for room rather than assume it.
    if (!lua_checkstack(L, 4)) {
        return QJsonValue(qsl("<table too large to read>"));
    }

    // A Lua table is a list and a map at once. Render a plain 1..n sequence as a JSON
    // array so that a model reading the result sees a list, not {"1":..., "2":...}.
    const int length = static_cast<int>(lua_objlen(L, index));
    int keyCount = 0;
    int sequenceKeyCount = 0;
    QSet<QString> stringKeys;
    QSet<QString> numberKeys;
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        ++keyCount;
        // lua_tostring() on a number key would rewrite the key in place, and lua_next()
        // then loses its place in the table - so read numbers without converting them.
        if (lua_type(L, -2) == LUA_TSTRING) {
            stringKeys.insert(readString(L, -2));
        } else if (lua_type(L, -2) == LUA_TNUMBER) {
            const double number = lua_tonumber(L, -2);
            numberKeys.insert(numberKey(number));
            if (number >= 1 && number <= length && static_cast<int>(number) == number) {
                ++sequenceKeyCount;
            }
        }
        lua_pop(L, 1);
    }

    // Counting keys is not enough on its own: lua_objlen() may answer any border of a table
    // that has a hole, so { "a", nil, "c", x = "d" } has three keys and a border of three,
    // and the array branch would render ["a",null,"c"] and drop x without a trace.
    if (length > 0 && keyCount == length && sequenceKeyCount == length) {
        QJsonArray array;
        for (int i = 1; i <= length; ++i) {
            lua_rawgeti(L, index, i);
            array.append(luaToJson(L, lua_gettop(L), depth + 1));
            lua_pop(L, 1);
        }
        return array;
    }

    // t[1] and t["1"] are separate keys in Lua but the same key in JSON. Where a table
    // carries both, bracket the numeric ones so neither entry silently overwrites the
    // other. Decided before the walk because lua_next's order is unspecified, so picking
    // a loser mid-iteration would make the same table render differently between calls.
    const bool bracketNumbers = stringKeys.intersects(numberKeys);

    QJsonObject object;
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        QString key;
        if (lua_type(L, -2) == LUA_TSTRING) {
            key = readString(L, -2);
        } else if (lua_type(L, -2) == LUA_TNUMBER) {
            key = numberKey(lua_tonumber(L, -2));
            if (bracketNumbers) {
                key = qsl("[%1]").arg(key);
            }
        } else {
            key = qsl("<%1>").arg(QString::fromUtf8(lua_typename(L, lua_type(L, -2))));
        }
        object[key] = luaToJson(L, lua_gettop(L), depth + 1);
        lua_pop(L, 1);
    }
    return object;
}

QString TMCPLuaBridge::readString(lua_State* L, int index)
{
    // Read the length rather than treating it as a C string: Lua strings may hold NULs, and
    // stopping at the first one truncates a value - or, for a table key, collides two
    // distinct keys into one and drops whichever the walk reaches first. Callers must have
    // established that the slot really holds a string, because lua_tolstring() rewrites a
    // number in place and lua_next() then loses its place in the table.
    size_t length = 0;
    const char* text = lua_tolstring(L, index, &length);
    return QString::fromUtf8(text, static_cast<int>(length));
}

QString TMCPLuaBridge::numberKey(double value)
{
    return QString::number(value, 'g', QLocale::FloatingPointShortest);
}

QString TMCPLuaBridge::jsonToText(const QJsonValue& value)
{
    switch (value.type()) {
    case QJsonValue::String:
        return value.toString();
    case QJsonValue::Bool:
        return value.toBool() ? qsl("true") : qsl("false");
    case QJsonValue::Double:
        // Not plain QString::number(): that renders six significant digits, so a room id
        // or a timestamp comes back as 1.23457e+06. FloatingPointShortest produces the
        // same text QJsonDocument would, so a number reads alike bare and inside a table.
        return QString::number(value.toDouble(), 'g', QLocale::FloatingPointShortest);
    case QJsonValue::Array:
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    case QJsonValue::Object:
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    default:
        return qsl("nil");
    }
}
