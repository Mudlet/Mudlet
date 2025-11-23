#ifndef MUDLET_TMCPLUABRIDGE_H
#define MUDLET_TMCPLUABRIDGE_H

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

#include "Host.h"
#include "utils.h"

#include "pre_guard.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QVariant>
#include "post_guard.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

struct LuaFunctionInfo {
    QString name;
    QString signature;
    QString description;
    QStringList parameters;
    QString returnType;
};

struct MCPToolResult {
    bool success;
    QJsonValue result;
    QString errorMessage;
    int errorCode;
};

class TMCPLuaBridge : public QObject
{
    Q_OBJECT

public:
    explicit TMCPLuaBridge(Host* pHost, QObject* parent = nullptr);
    ~TMCPLuaBridge() = default;

    bool loadLuaFunctions();
    QJsonArray getAvailableTools() const;
    MCPToolResult callTool(const QString& toolName, const QJsonObject& arguments);

signals:
    void toolsChanged();

private:
    struct MCPTool {
        QString name;
        QString description;
        QJsonObject inputSchema;
        LuaFunctionInfo luaFunction;
    };

    void createLuaExecutionTool();
    QJsonValue executeLuaCode(const QString& luaCode, const QString& profileName = QString());
    QJsonValue executeLuaFunction(const LuaFunctionInfo& luaFunc, const QJsonObject& arguments);
    QJsonValue luaStackToJson(lua_State* L, int index);
    void jsonToLuaStack(lua_State* L, const QJsonValue& value);
    QString extractParameterType(const QString& signature, const QString& paramName);

    Host* mpHost;
    QMap<QString, MCPTool> mTools;
    QMap<QString, LuaFunctionInfo> mLuaFunctions;
    bool mFunctionsLoaded;
};

#endif // MUDLET_TMCPLUABRIDGE_H
