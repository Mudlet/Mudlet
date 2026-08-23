#ifndef MUDLET_TMCPLUABRIDGE_H
#define MUDLET_TMCPLUABRIDGE_H

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

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>

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

class Host;

// What one tool call produced. success == false is a tool execution error, which the
// model is shown so it can correct itself - it is not a protocol error.
struct MCPToolResult
{
    bool success = false;
    QString text;
};

// Exposes Mudlet's Lua interpreter to the MCP server as a single "lua" tool.
//
// One tool rather than one per Lua API function: the API runs to hundreds of functions,
// which would swamp a model's context, and any of them can be reached from a snippet.
//
// Not bound to a profile: there is one MCP server for the whole application, so which
// profile a snippet runs in is decided per call, from the tool's "profile" argument.
class TMCPLuaBridge : public QObject
{
    Q_OBJECT

public:
    explicit TMCPLuaBridge(QObject* parent = nullptr);

    QJsonArray getAvailableTools() const;
    bool hasTool(const QString& toolName) const;
    MCPToolResult callTool(const QString& toolName, const QJsonObject& arguments);

    // Runs one snippet on an already-chosen interpreter. Separate from callTool() so that
    // deciding which profile to run in stays apart from running, and so the runner can be
    // exercised against a bare lua_State.
    static MCPToolResult runLua(lua_State* L, const QString& luaCode);

    static constexpr const char* MCP_LUA_TOOL = "lua";

private:
    static Host* targetHost(const QString& profileName, QString& failure);
    static QJsonValue luaToJson(lua_State* L, int index, int depth);
    static QJsonValue luaTableToJson(lua_State* L, int index, int depth);
    static QString numberKey(double value);
    static QString jsonToText(const QJsonValue& value);
};

#endif // MUDLET_TMCPLUABRIDGE_H
