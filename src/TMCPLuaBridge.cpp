/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vperetokin@gmail.com          *
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
#include "TLuaInterpreter.h"
#include "mudlet.h"

#include <QDebug>
#include <QFile>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStandardPaths>

TMCPLuaBridge::TMCPLuaBridge(Host* pHost, QObject* parent)
: QObject(parent)
, mpHost(pHost)
, mFunctionsLoaded(false)
{
}

bool TMCPLuaBridge::loadLuaFunctions()
{
    if (mFunctionsLoaded) {
        return true;
    }

    // Create a single Lua execution tool instead of exposing all individual functions
    createLuaExecutionTool();

    mFunctionsLoaded = true;
    qDebug() << "TMCPLuaBridge: Loaded" << mTools.size() << "MCP tools";

    emit toolsChanged();
    return true;
}


void TMCPLuaBridge::createLuaExecutionTool()
{
    MCPTool tool;
    tool.name = qsl("lua");
    tool.description = tr("Run Lua code in the Mudlet client");

    // Create input schema for the Lua execution tool
    QJsonObject schema;
    schema[qsl("type")] = qsl("object");

    QJsonObject properties;
    QJsonArray required;

    // Required 'code' parameter
    QJsonObject codeParam;
    codeParam[qsl("type")] = qsl("string");
    codeParam[qsl("description")] = tr("Lua code to run");
    properties[qsl("code")] = codeParam;
    required.append(qsl("code"));

    // Optional 'profile' parameter
    QJsonObject profileParam;
    profileParam[qsl("type")] = qsl("string");
    profileParam[qsl("description")] = tr("Profile name to run the code in. If not specified, uses the currently active profile");
    properties[qsl("profile")] = profileParam;

    schema[qsl("properties")] = properties;
    schema[qsl("required")] = required;

    tool.inputSchema = schema;

    // Store a placeholder lua function info (not used for execution)
    tool.luaFunction.name = qsl("lua");
    tool.luaFunction.description = tool.description;

    mTools[tool.name] = tool;
    qDebug() << "TMCPLuaBridge: Created Lua tool";
}

QJsonValue TMCPLuaBridge::executeLuaCode(const QString& luaCode, const QString& profileName)
{
    Host* targetHost = mpHost;

    // If a profile name is specified, try to get that profile's host
    if (!profileName.isEmpty()) {
        targetHost = mudlet::self()->getHostManager().getHost(profileName);
        if (!targetHost) {
            return QJsonValue(tr("Profile '%1' not found").arg(profileName));
        }
    } else if (!targetHost) {
        // If no profile specified and mpHost is null, try to get the active host
        targetHost = mudlet::self()->getActiveHost();
        if (!targetHost) {
            return QJsonValue(tr("No active profile available for Lua execution"));
        }
    }

    lua_State* L = targetHost->getLuaInterpreter()->pGlobalLua;
    if (!L) {
        return QJsonValue(tr("Lua interpreter not available"));
    }

    // Capture output by redirecting print function temporarily
    QString output;
    QString originalPrintCode = qsl(R"(
        local original_print = print
        local captured_output = {}
        print = function(...)
            local args = {...}
            local str_args = {}
            for i, v in ipairs(args) do
                str_args[i] = tostring(v)
            end
            table.insert(captured_output, table.concat(str_args, '\t'))
        end
    )");

    // Execute setup code
    int setupResult = luaL_dostring(L, originalPrintCode.toUtf8().constData());
    if (setupResult) {
        return QJsonValue(tr("Failed to setup Lua output capture"));
    }

    // Execute the user's code using luaL_loadstring + lua_pcall to capture return values
    // (luaL_dostring uses lua_pcall with 0 return values, so we can't capture them)
    int stackBefore = lua_gettop(L);
    int loadResult = luaL_loadstring(L, luaCode.toUtf8().constData());

    QString resultStr;
    if (loadResult != 0) {
        // Syntax error in Lua code
        if (lua_gettop(L) > 0) {
            resultStr = tr("Lua Syntax Error: %1").arg(QString::fromUtf8(lua_tostring(L, -1)));
            lua_pop(L, 1);
        } else {
            resultStr = tr("Unknown Lua syntax error occurred");
        }
    } else {
        // Execute the compiled chunk, requesting all return values
        int execResult = lua_pcall(L, 0, LUA_MULTRET, 0);

        if (execResult != 0) {
            // Runtime error
            if (lua_gettop(L) > 0) {
                resultStr = tr("Lua Error: %1").arg(QString::fromUtf8(lua_tostring(L, -1)));
                lua_pop(L, 1);
            } else {
                resultStr = tr("Unknown Lua error occurred");
            }
        } else {
            // Get all return values
            int returnCount = lua_gettop(L) - stackBefore;
            if (returnCount > 0) {
                if (returnCount == 1) {
                    QJsonValue returnValue = luaStackToJson(L, -1);
                    if (!returnValue.isNull()) {
                        if (returnValue.isString()) {
                            resultStr = returnValue.toString();
                        } else {
                            resultStr = QJsonDocument(QJsonArray{returnValue}).toJson(QJsonDocument::Compact);
                        }
                    }
                } else {
                    // Multiple return values - return as array
                    QJsonArray returnArray;
                    for (int i = -returnCount; i <= -1; ++i) {
                        returnArray.append(luaStackToJson(L, i));
                    }
                    resultStr = QJsonDocument(returnArray).toJson(QJsonDocument::Compact);
                }
                lua_pop(L, returnCount);
            }
        }
    }

    // Get captured output
    QString getCapturedCode = qsl(R"(
        local result = ""
        if captured_output and type(captured_output) == "table" then
            result = table.concat(captured_output, '\n')
        end
        print = original_print
        return result
    )");

    if (luaL_dostring(L, getCapturedCode.toUtf8().constData()) == 0) {
        if (lua_gettop(L) > 0) {
            QString capturedOutput = QString::fromUtf8(lua_tostring(L, -1));
            if (!capturedOutput.isEmpty()) {
                resultStr = capturedOutput + (resultStr.isEmpty() ? "" : "\n" + resultStr);
            }
            lua_pop(L, 1);
        }
    }

    // Return null if no output or return value
    if (resultStr.isEmpty()) {
        return QJsonValue();
    }

    return QJsonValue(resultStr);
}

QJsonArray TMCPLuaBridge::getAvailableTools() const
{
    QJsonArray tools;

    qDebug() << "TMCPLuaBridge: Returning" << mTools.size() << "tools";

    for (const auto& tool : mTools) {
        QJsonObject toolDef;
        toolDef[qsl("name")] = tool.name;
        toolDef[qsl("title")] = tool.name; // Add optional title field
        toolDef[qsl("description")] = tool.description;
        toolDef[qsl("inputSchema")] = tool.inputSchema;

        qDebug() << "TMCPLuaBridge: Tool:" << tool.name << "schema:" << tool.inputSchema;
        tools.append(toolDef);
    }

    return tools;
}

MCPToolResult TMCPLuaBridge::callTool(const QString& toolName, const QJsonObject& arguments)
{
    MCPToolResult result;
    result.success = false;
    result.errorCode = -32603;

    if (!mTools.contains(toolName)) {
        result.errorMessage = tr("Tool not found: %1").arg(toolName);
        result.errorCode = -32601;
        return result;
    }

    const MCPTool& tool = mTools[toolName];

    try {
        if (toolName == qsl("lua")) {
            // Special handling for the Lua tool
            QString luaCode = arguments[qsl("code")].toString();
            if (luaCode.isEmpty()) {
                result.errorMessage = tr("Missing required parameter: code");
                result.errorCode = -32602;
                return result;
            }
            QString profileName = arguments[qsl("profile")].toString();
            result.result = executeLuaCode(luaCode, profileName);
        } else {
            // Handle regular Lua function tools (if any remain)
            result.result = executeLuaFunction(tool.luaFunction, arguments);
        }
        result.success = true;
        result.errorMessage.clear();
        result.errorCode = 0;
    } catch (const std::exception& e) {
        result.errorMessage = tr("Error executing tool %1: %2").arg(toolName, QString::fromStdString(e.what()));
    } catch (...) {
        result.errorMessage = tr("Unknown error executing tool %1").arg(toolName);
    }

    return result;
}

QJsonValue TMCPLuaBridge::executeLuaFunction(const LuaFunctionInfo& luaFunc, const QJsonObject& arguments)
{
    if (!mpHost || !mpHost->mpConsole || !mpHost->mLuaInterpreter.pGlobalLua) {
        throw std::runtime_error("Lua interpreter not available");
    }

    lua_State* L = mpHost->mLuaInterpreter.pGlobalLua;

    lua_getglobal(L, luaFunc.name.toUtf8().constData());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error(tr("Function %1 not found in Lua global scope").arg(luaFunc.name).toStdString());
    }

    int argCount = 0;
    for (const QString& param : luaFunc.parameters) {
        QString cleanParam = param;
        if (cleanParam.startsWith(qsl("[")) && cleanParam.endsWith(qsl("]"))) {
            cleanParam = cleanParam.mid(1, cleanParam.length() - 2);
        }

        if (arguments.contains(cleanParam)) {
            jsonToLuaStack(L, arguments[cleanParam]);
            argCount++;
        } else if (!param.startsWith(qsl("["))) {
            lua_pushnil(L);
            argCount++;
        }
    }

    int result = lua_pcall(L, argCount, LUA_MULTRET, 0);
    if (result != 0) {
        QString errorMsg = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        throw std::runtime_error(errorMsg.toStdString());
    }

    int returnCount = lua_gettop(L);
    QJsonValue returnValue;

    if (returnCount == 0) {
        returnValue = QJsonValue();
    } else if (returnCount == 1) {
        returnValue = luaStackToJson(L, -1);
    } else {
        QJsonArray returnArray;
        for (int i = -returnCount; i <= -1; ++i) {
            returnArray.append(luaStackToJson(L, i));
        }
        returnValue = returnArray;
    }

    lua_pop(L, returnCount);
    return returnValue;
}

QJsonValue TMCPLuaBridge::luaStackToJson(lua_State* L, int index)
{
    int type = lua_type(L, index);

    switch (type) {
        case LUA_TNIL:
            return QJsonValue();
        case LUA_TBOOLEAN:
            return QJsonValue(lua_toboolean(L, index) != 0);
        case LUA_TNUMBER:
            return QJsonValue(lua_tonumber(L, index));
        case LUA_TSTRING:
            return QJsonValue(QString::fromUtf8(lua_tostring(L, index)));
        case LUA_TTABLE: {
            QJsonObject obj;
            lua_pushnil(L);
            while (lua_next(L, index < 0 ? index - 1 : index) != 0) {
                QString key;
                if (lua_type(L, -2) == LUA_TSTRING) {
                    key = QString::fromUtf8(lua_tostring(L, -2));
                } else if (lua_type(L, -2) == LUA_TNUMBER) {
                    key = QString::number(lua_tonumber(L, -2));
                } else {
                    key = qsl("unknown_key");
                }
                obj[key] = luaStackToJson(L, -1);
                lua_pop(L, 1);
            }
            return QJsonValue(obj);
        }
        default:
            return QJsonValue(tr("Unsupported Lua type: %1").arg(lua_typename(L, type)));
    }
}

void TMCPLuaBridge::jsonToLuaStack(lua_State* L, const QJsonValue& value)
{
    switch (value.type()) {
        case QJsonValue::Null:
            lua_pushnil(L);
            break;
        case QJsonValue::Bool:
            lua_pushboolean(L, value.toBool() ? 1 : 0);
            break;
        case QJsonValue::Double:
            lua_pushnumber(L, value.toDouble());
            break;
        case QJsonValue::String:
            lua_pushstring(L, value.toString().toUtf8().constData());
            break;
        case QJsonValue::Array: {
            QJsonArray arr = value.toArray();
            lua_createtable(L, arr.size(), 0);
            for (int i = 0; i < arr.size(); ++i) {
                lua_pushinteger(L, i + 1);
                jsonToLuaStack(L, arr[i]);
                lua_settable(L, -3);
            }
            break;
        }
        case QJsonValue::Object: {
            QJsonObject obj = value.toObject();
            lua_createtable(L, 0, obj.size());
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                lua_pushstring(L, it.key().toUtf8().constData());
                jsonToLuaStack(L, it.value());
                lua_settable(L, -3);
            }
            break;
        }
        default:
            lua_pushnil(L);
            break;
    }
}
