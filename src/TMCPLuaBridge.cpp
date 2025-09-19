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

    parseLuaFunctionList();

    for (const auto& luaFunc : mLuaFunctions) {
        MCPTool tool = createMCPToolFromLuaFunction(luaFunc);
        mTools[tool.name] = tool;
    }

    mFunctionsLoaded = true;
    qDebug() << "TMCPLuaBridge: Loaded" << mTools.size() << "Lua functions as MCP tools";

    emit toolsChanged();
    return true;
}

void TMCPLuaBridge::parseLuaFunctionList()
{
    QString functionListPath = qsl(":/lua-function-list.json");

    QFile file(functionListPath);
    if (!file.exists()) {
        functionListPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, qsl("lua-function-list.json"));
        file.setFileName(functionListPath);
    }

    if (!file.exists()) {
        file.setFileName(qsl("src/lua-function-list.json"));
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "TMCPLuaBridge: Could not open lua-function-list.json";
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "TMCPLuaBridge: Error parsing lua-function-list.json:" << parseError.errorString();
        return;
    }

    QJsonObject functionList = doc.object();
    for (auto it = functionList.begin(); it != functionList.end(); ++it) {
        LuaFunctionInfo funcInfo;
        funcInfo.name = it.key();
        funcInfo.signature = it.value().toString();

        QRegularExpression re(qsl(R"((\w+)\s*=\s*(\w+)\((.*)\))"));
        QRegularExpressionMatch match = re.match(funcInfo.signature);

        if (match.hasMatch()) {
            funcInfo.returnType = match.captured(1);
            QString params = match.captured(3);

            if (!params.isEmpty()) {
                QStringList paramList = params.split(qsl(","));
                for (QString& param : paramList) {
                    param = param.trimmed();
                    if (param.startsWith(qsl("["))) {
                        param = param.mid(1);
                        if (param.endsWith(qsl("]"))) {
                            param.chop(1);
                        }
                    }
                    if (!param.isEmpty()) {
                        funcInfo.parameters.append(param);
                    }
                }
            }
        } else {
            funcInfo.returnType = qsl("unknown");
        }

        funcInfo.description = tr("Mudlet Lua function: %1").arg(funcInfo.signature);
        mLuaFunctions[funcInfo.name] = funcInfo;
    }
}

TMCPLuaBridge::MCPTool TMCPLuaBridge::createMCPToolFromLuaFunction(const LuaFunctionInfo& luaFunc)
{
    MCPTool tool;
    tool.name = luaFunc.name;
    tool.description = luaFunc.description;
    tool.inputSchema = createInputSchemaForFunction(luaFunc);
    tool.luaFunction = luaFunc;
    return tool;
}

QJsonObject TMCPLuaBridge::createInputSchemaForFunction(const LuaFunctionInfo& luaFunc)
{
    QJsonObject schema;
    schema[qsl("type")] = qsl("object");

    QJsonObject properties;
    QJsonArray required;

    for (const QString& param : luaFunc.parameters) {
        QString cleanParam = param;
        bool optional = cleanParam.startsWith(qsl("[")) && cleanParam.endsWith(qsl("]"));
        if (optional) {
            cleanParam = cleanParam.mid(1, cleanParam.length() - 2);
        }

        QString paramType = extractParameterType(luaFunc.signature, cleanParam);

        QJsonObject paramSchema;
        if (paramType == qsl("number") || paramType == qsl("int")) {
            paramSchema[qsl("type")] = qsl("number");
        } else if (paramType == qsl("boolean") || paramType == qsl("bool")) {
            paramSchema[qsl("type")] = qsl("boolean");
        } else if (paramType == qsl("table") || paramType == qsl("array")) {
            paramSchema[qsl("type")] = qsl("array");
        } else {
            paramSchema[qsl("type")] = qsl("string");
        }

        paramSchema[qsl("description")] = tr("Parameter for %1").arg(cleanParam);
        properties[cleanParam] = paramSchema;

        if (!optional) {
            required.append(cleanParam);
        }
    }

    schema[qsl("properties")] = properties;
    if (!required.isEmpty()) {
        schema[qsl("required")] = required;
    }

    return schema;
}

QString TMCPLuaBridge::extractParameterType(const QString& signature, const QString& paramName)
{
    Q_UNUSED(signature)
    Q_UNUSED(paramName)

    return qsl("string");
}

QJsonArray TMCPLuaBridge::getAvailableTools() const
{
    QJsonArray tools;

    for (const auto& tool : mTools) {
        QJsonObject toolDef;
        toolDef[qsl("name")] = tool.name;
        toolDef[qsl("description")] = tool.description;
        toolDef[qsl("inputSchema")] = tool.inputSchema;
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
        result.result = executeLuaFunction(tool.luaFunction, arguments);
        result.success = true;
        result.errorMessage.clear();
        result.errorCode = 0;
    } catch (const std::exception& e) {
        result.errorMessage = tr("Error executing Lua function %1: %2").arg(toolName, QString::fromStdString(e.what()));
    } catch (...) {
        result.errorMessage = tr("Unknown error executing Lua function %1").arg(toolName);
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