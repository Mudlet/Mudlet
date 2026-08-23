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
#include "TMap.h"
#include "mudlet.h"
#include "utils.h"

#include <QElapsedTimer>
#include <QJsonDocument>
#include <QLocale>
#include <QScopeGuard>
#include <QSet>
#include <QStringList>

#include <cmath>

// Runs the caller's code with print() diverted into a table.
//
// The capture is installed in the chunk's own environment rather than over _G.print,
// because Mudlet dispatches from nested event loops - installPackage()'s unpacking
// progress, a profile save, a modal dialog - and any trigger, timer or second MCP
// request that runs in that window would otherwise have its output captured into this
// call's result and never reach the user's console. Only the snippet, and functions it
// defines, see the capture. The flag turns it off once the call is over, so a timer the
// snippet created prints to the console when it fires later rather than into a table
// nobody will read.
//
// print is seeded in the table constructor, not assigned afterwards: assigning would go
// through __newindex and land in _G, which is the very thing being avoided.
static const char* csmLuaRunner = R"LUA(
local code = ...
local lines = {}
local capturing = true

local chunk, syntaxError = loadstring(code, 'mcp')
if not chunk then
    return false, '', syntaxError
end

local env = setmetatable({
    print = function(...)
        if not capturing then
            return _G.print(...)
        end
        local parts = {}
        for i = 1, select('#', ...) do
            parts[i] = tostring((select(i, ...)))
        end
        lines[#lines + 1] = table.concat(parts, '\t')
    end
}, {__index = _G, __newindex = _G})
setfenv(chunk, env)

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
capturing = false
return returned[1], table.concat(lines, '\n'), unpack(returned, 2, count)
)LUA";

// A table that refers to itself would otherwise recurse until the C stack gives out, and
// the code being rendered here is written by a model, so _G is a plausible return value.
static constexpr int csmMaxTableDepth = 12;

// Depth alone does not bound the walk. A table holding twenty references to itself is
// only one level deeper each step but has 20^12 nodes to render, so the depth cap never
// trips: measured, five self-references reached 20GB resident and had to be killed.
// Cap how many values may be converted in total as well.
static constexpr int csmMaxNodes = 200000;

// A count hook gives the VM the wall-clock deadline declared in the header. A snippet that
// swallows the error inside its own loop can still spin - no cooperative limit can prevent
// that - but it can no longer do so by accident.
static constexpr int csmHookInstructionCount = 20000;

// Shared rather than per-call because a nested MCP request may start while an outer one is
// still running; the inner call takes the earlier of the two deadlines and puts the outer
// one back on the way out, so nesting cannot extend a deadline that has already been set.
static QElapsedTimer smExecutionTimer;
static qint64 smExecutionDeadline = 0;
static int smDeadlineMs = TMCPLuaBridge::csmDefaultDeadlineMs;

static void executionDeadlineHook(lua_State* L, lua_Debug*)
{
    if (smExecutionTimer.isValid() && smExecutionTimer.elapsed() >= smExecutionDeadline) {
        //: Error shown to an AI model when the Lua code it sent ran for too long and was stopped. %1 is a whole number of seconds.
        luaL_error(L, "%s", TMCPLuaBridge::tr("stopped after running for more than %1 seconds").arg((smDeadlineMs + 999) / 1000).toUtf8().constData());
    }
}

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
        //: Error shown to an AI model that asked for a tool this server does not have. %1 is the name it asked for.
        return {false, tr("Unknown tool: %1").arg(toolName)};
    }

    // QJsonValue::toString() answers an empty string for a number, an object or a boolean,
    // so an argument of the wrong type would otherwise read as one that was never sent: a
    // numeric "profile" would quietly run the code in whichever profile is in the
    // foreground rather than the one that was asked for.
    const QJsonValue codeArgument = arguments.value(qsl("code"));
    if (!codeArgument.isUndefined() && !codeArgument.isString()) {
        //: Error shown to an AI model that sent the wrong kind of value for the "code" argument. Keep 'code' as-is, it names the argument.
        return {false, tr("The 'code' argument must be a string.")};
    }
    const QString luaCode = codeArgument.toString();
    if (luaCode.isEmpty()) {
        //: Error shown to an AI model that left out the Lua code to run. Keep 'code' as-is, it names the argument.
        return {false, tr("The 'code' argument is required and cannot be empty.")};
    }

    const QJsonValue profileArgument = arguments.value(qsl("profile"));
    if (!profileArgument.isUndefined() && !profileArgument.isNull() && !profileArgument.isString()) {
        //: Error shown to an AI model that sent the wrong kind of value for the "profile" argument. Keep 'profile' as-is, it names the argument.
        return {false, tr("The 'profile' argument must be a string.")};
    }

    QString failure;
    Host* pTarget = targetHost(profileArgument.toString(), failure);
    if (!pTarget) {
        return {false, failure};
    }

    lua_State* L = pTarget->getLuaInterpreter()->getLuaGlobalState();
    if (!L) {
        //: Error shown to an AI model when the profile it named cannot run Lua.
        return {false, tr("That profile has no Lua interpreter running.")};
    }

    return runLua(L, luaCode);
}

Host* TMCPLuaBridge::targetHost(const QString& profileName, QString& failure)
{
    if (!mudlet::self()) {
        //: Error shown to an AI model when the application is not available to run Lua in.
        failure = tr("Mudlet is not running.");
        return nullptr;
    }

    Host* pTarget = nullptr;
    if (!profileName.isEmpty()) {
        // Match the profile name the way the rest of Mudlet does. getHost() is an exact
        // lookup, so "achaea" would not find the profile the user knows as "Achaea".
        const QString canonical = mudlet::self()->getCanonicalProfileName(profileName);
        pTarget = mudlet::self()->getHostManager().getHost(canonical.isEmpty() ? profileName : canonical);
        if (!pTarget) {
            //: Error shown to an AI model that named a profile which is not open. %1 is the name it gave.
            failure = tr("No profile named '%1' is open.").arg(profileName);
            return nullptr;
        }
    } else {
        // The foreground profile, which is the one the user would mean by "here". A model
        // that needs to be sure of which profile it is in should name it instead.
        pTarget = mudlet::self()->getActiveHost();
        if (!pTarget) {
            //: Error shown to an AI model when no game profile is open. Keep 'profile' as-is, it names an argument.
            failure = tr("No profile is open to run Lua in. Open one, or name a profile in the 'profile' argument.");
            return nullptr;
        }
    }

    // A profile stays in the host pool until the very end of its teardown, and an MCP
    // request is delivered from whatever nested event loop happens to be pumping - a
    // profile save, an unpacking package, a modal dialog - so without this the snippet
    // can run on a profile whose triggers are already stopped, or on a lua_State that a
    // queued phase-2 reset is about to close underneath it. waitForEvent() and
    // pumpEvents() refuse for the same reason; see TLuaInterpreterMudletObjects.cpp.
    if (pTarget->isClosingDown() || pTarget->profileResetInProgress()) {
        //: Error shown to an AI model when the profile it named is shutting down. %1 is the profile name.
        failure = tr("The profile '%1' is closing or being reset, so Lua cannot run in it right now.").arg(pTarget->getName());
        return nullptr;
    }
    if (pTarget->mpMap && pTarget->mpMap->mapOperationInProgress()) {
        //: Error shown to an AI model when the profile it named is redrawing its map. %1 is the profile name.
        failure = tr("The profile '%1' is busy with a map operation, so Lua cannot run in it right now.").arg(pTarget->getName());
        return nullptr;
    }

    return pTarget;
}

MCPToolResult TMCPLuaBridge::runLua(lua_State* L, const QString& luaCode, int deadlineMs)
{
    const int stackBefore = lua_gettop(L);

    if (luaL_loadstring(L, csmLuaRunner) != 0) {
        const QString message = QString::fromUtf8(lua_tostring(L, -1));
        lua_settop(L, stackBefore);
        //: Error shown to an AI model when Mudlet's own wrapper code failed to compile, which means a bug in Mudlet. %1 is the compiler message.
        return {false, tr("Could not compile Mudlet's Lua runner: %1").arg(message)};
    }

    // Put back whatever was hooked before, and on every exit: leaving a count hook armed
    // would charge the deadline to the next thing this interpreter runs.
    const lua_Hook previousHook = lua_gethook(L);
    const int previousMask = lua_gethookmask(L);
    const int previousCount = lua_gethookcount(L);
    const bool timerWasRunning = smExecutionTimer.isValid();
    const qint64 previousDeadline = smExecutionDeadline;
    const int previousDeadlineMs = smDeadlineMs;
    if (!timerWasRunning) {
        smExecutionTimer.start();
    }
    const qint64 ownDeadline = smExecutionTimer.elapsed() + deadlineMs;
    const bool ownDeadlineIsSooner = !timerWasRunning || ownDeadline < previousDeadline;
    smExecutionDeadline = ownDeadlineIsSooner ? ownDeadline : previousDeadline;
    smDeadlineMs = ownDeadlineIsSooner ? deadlineMs : previousDeadlineMs;
    lua_sethook(L, executionDeadlineHook, LUA_MASKCOUNT, csmHookInstructionCount);
    const auto deadlineGuard = qScopeGuard([=]() {
        lua_sethook(L, previousHook, previousMask, previousCount);
        smExecutionDeadline = previousDeadline;
        smDeadlineMs = previousDeadlineMs;
        if (!timerWasRunning) {
            smExecutionTimer.invalidate();
        }
    });

    // Push the length too: lua_pushstring() would stop at the first embedded NUL and
    // silently run only the leading fragment of the snippet.
    const QByteArray code = luaCode.toUtf8();
    lua_pushlstring(L, code.constData(), code.size());
    if (lua_pcall(L, 1, LUA_MULTRET, 0) != 0) {
        const QString message = QString::fromUtf8(lua_tostring(L, -1));
        lua_settop(L, stackBefore);
        //: Error shown to an AI model when the Lua code it sent failed. %1 is the message from Lua.
        return {false, tr("Lua error: %1").arg(message)};
    }

    // The runner always answers with ok, printed output, then whatever the code returned.
    if (lua_gettop(L) - stackBefore < 2) {
        lua_settop(L, stackBefore);
        //: Error shown to an AI model when Mudlet's own wrapper code misbehaved, which means a bug in Mudlet.
        return {false, tr("Mudlet's Lua runner returned nothing.")};
    }

    const bool ok = lua_toboolean(L, stackBefore + 1) != 0;
    const QString printed = readString(L, stackBefore + 2);
    const int firstValue = stackBefore + 3;
    const int lastValue = lua_gettop(L);
    // One budget for the whole reply rather than one per returned value, so that
    // "return _G, _G, _G" cannot multiply the ceiling by the number of values.
    int nodeBudget = csmMaxNodes;

    QStringList pieces;
    if (!printed.isEmpty()) {
        pieces << printed;
    }

    if (!ok) {
        // error() can be handed a table rather than a string, and lua_tostring() answers
        // null for one of those, which would drop the message entirely.
        //: Stands in for a Lua error message that could not be read.
        const QString message = firstValue <= lastValue ? jsonToText(luaToJson(L, firstValue, 0, nodeBudget)) : tr("unknown error");
        //: Error shown to an AI model when the Lua code it sent failed. %1 is the message from Lua.
        pieces << tr("Lua error: %1").arg(message);
        lua_settop(L, stackBefore);
        return {false, pieces.join(QChar::LineFeed)};
    }

    QStringList values;
    for (int i = firstValue; i <= lastValue; ++i) {
        values << jsonToText(luaToJson(L, i, 0, nodeBudget));
    }
    lua_settop(L, stackBefore);

    if (!values.isEmpty()) {
        pieces << values.join(QChar::Tabulation);
    }

    if (pieces.isEmpty()) {
        //: Shown to an AI model when its Lua code succeeded but reported nothing back. Keep print(), echo(), cecho() and display() as-is, they are Lua function names.
        return {true,
                tr("The code ran. It returned nothing and printed nothing - note that only print() and returned values are reported back, not echo(), cecho() or display(), which write to the "
                   "profile's window.")};
    }
    return {true, pieces.join(QChar::LineFeed)};
}

QJsonValue TMCPLuaBridge::luaToJson(lua_State* L, int index, int depth, int& nodeBudget)
{
    // Work in absolute indices so that pushing onto the stack while walking a table does
    // not shift the slot being read out from under us.
    if (index < 0) {
        index = lua_gettop(L) + 1 + index;
    }

    if (--nodeBudget < 0) {
        //: Stands in for the rest of a Lua value that was too big to send to an AI model.
        return QJsonValue(tr("<too much data, the rest was left out>"));
    }

    switch (lua_type(L, index)) {
    case LUA_TNIL:
        return QJsonValue();
    case LUA_TBOOLEAN:
        return QJsonValue(lua_toboolean(L, index) != 0);
    case LUA_TNUMBER: {
        // QJsonValue turns a non-finite double into null, so 0/0 and math.huge would reach
        // the model as "no value" from inside a table while reading as nan/inf at the top
        // level. Spell them out instead, so the two positions agree.
        const double number = lua_tonumber(L, index);
        if (!std::isfinite(number)) {
            return QJsonValue(numberKey(number));
        }
        return QJsonValue(number);
    }
    case LUA_TSTRING:
        return QJsonValue(readString(L, index));
    case LUA_TTABLE:
        return luaTableToJson(L, index, depth, nodeBudget);
    default:
        return QJsonValue(qsl("<%1>").arg(QString::fromUtf8(lua_typename(L, lua_type(L, index)))));
    }
}

QJsonValue TMCPLuaBridge::luaTableToJson(lua_State* L, int index, int depth, int& nodeBudget)
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
            array.append(luaToJson(L, lua_gettop(L), depth + 1, nodeBudget));
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
            // A key that is neither a string nor a number has no JSON spelling, so it is
            // named after its type - but two of them would then be the same name and one
            // entry would overwrite the other, which is the loss the numeric bracketing
            // above exists to prevent. The identity makes each one distinct.
            key = qsl("<%1: %2>").arg(QString::fromUtf8(lua_typename(L, lua_type(L, -2))), QString::number(reinterpret_cast<quintptr>(lua_topointer(L, -2)), 16));
        }

        // Backstop for the cases the rules above cannot separate, such as a string key
        // spelled exactly like the rendering of some other key. Suffixing keeps both
        // entries; lua_next's order is unspecified, so which one is suffixed may vary
        // between calls, but no value is dropped.
        QString uniqueKey = key;
        for (int collision = 2; object.contains(uniqueKey); ++collision) {
            uniqueKey = qsl("%1#%2").arg(key).arg(collision);
        }
        object[uniqueKey] = luaToJson(L, lua_gettop(L), depth + 1, nodeBudget);
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
