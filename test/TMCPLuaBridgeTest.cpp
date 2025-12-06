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

#include <TMCPLuaBridge.h>
#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

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

class TMCPLuaBridgeTest : public QObject
{
    Q_OBJECT

private:
    lua_State* L = nullptr;
    TMCPLuaBridge* bridge = nullptr;

private slots:
    void init()
    {
        L = luaL_newstate();
        luaL_openlibs(L);
        bridge = new TMCPLuaBridge(nullptr, nullptr);
    }

    void cleanup()
    {
        delete bridge;
        bridge = nullptr;
        if (L) {
            lua_close(L);
            L = nullptr;
        }
    }

    // ========== luaStackToJson tests ==========

    void testLuaStackToJson_nil()
    {
        lua_pushnil(L);
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isNull());
        lua_pop(L, 1);
    }

    void testLuaStackToJson_boolean_true()
    {
        lua_pushboolean(L, 1);
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isBool());
        QCOMPARE(result.toBool(), true);
        lua_pop(L, 1);
    }

    void testLuaStackToJson_boolean_false()
    {
        lua_pushboolean(L, 0);
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isBool());
        QCOMPARE(result.toBool(), false);
        lua_pop(L, 1);
    }

    void testLuaStackToJson_number_integer()
    {
        lua_pushinteger(L, 42);
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isDouble());
        QCOMPARE(result.toInt(), 42);
        lua_pop(L, 1);
    }

    void testLuaStackToJson_number_float()
    {
        lua_pushnumber(L, 3.14159);
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isDouble());
        QCOMPARE(result.toDouble(), 3.14159);
        lua_pop(L, 1);
    }

    void testLuaStackToJson_string()
    {
        lua_pushstring(L, "hello world");
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isString());
        QCOMPARE(result.toString(), QString("hello world"));
        lua_pop(L, 1);
    }

    void testLuaStackToJson_string_empty()
    {
        lua_pushstring(L, "");
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isString());
        QCOMPARE(result.toString(), QString(""));
        lua_pop(L, 1);
    }

    void testLuaStackToJson_string_unicode()
    {
        lua_pushstring(L, "こんにちは");
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isString());
        QCOMPARE(result.toString(), QString("こんにちは"));
        lua_pop(L, 1);
    }

    void testLuaStackToJson_emptyTable()
    {
        lua_newtable(L);
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isObject());
        QVERIFY(result.toObject().isEmpty());
        lua_pop(L, 1);
    }

    void testLuaStackToJson_arrayTable()
    {
        lua_newtable(L);
        lua_pushinteger(L, 1);
        lua_pushstring(L, "first");
        lua_settable(L, -3);
        lua_pushinteger(L, 2);
        lua_pushstring(L, "second");
        lua_settable(L, -3);

        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isObject());
        QJsonObject obj = result.toObject();
        QCOMPARE(obj["1"].toString(), QString("first"));
        QCOMPARE(obj["2"].toString(), QString("second"));
        lua_pop(L, 1);
    }

    void testLuaStackToJson_objectTable()
    {
        lua_newtable(L);
        lua_pushstring(L, "name");
        lua_pushstring(L, "Mudlet");
        lua_settable(L, -3);
        lua_pushstring(L, "version");
        lua_pushnumber(L, 4.0);
        lua_settable(L, -3);

        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isObject());
        QJsonObject obj = result.toObject();
        QCOMPARE(obj["name"].toString(), QString("Mudlet"));
        QCOMPARE(obj["version"].toDouble(), 4.0);
        lua_pop(L, 1);
    }

    void testLuaStackToJson_nestedTable()
    {
        lua_newtable(L);
        lua_pushstring(L, "outer");
        lua_newtable(L);
        lua_pushstring(L, "inner");
        lua_pushstring(L, "value");
        lua_settable(L, -3);
        lua_settable(L, -3);

        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isObject());
        QJsonObject obj = result.toObject();
        QVERIFY(obj["outer"].isObject());
        QCOMPARE(obj["outer"].toObject()["inner"].toString(), QString("value"));
        lua_pop(L, 1);
    }

    void testLuaStackToJson_function()
    {
        luaL_dostring(L, "return function() end");
        QJsonValue result = bridge->luaStackToJson(L, -1);
        QVERIFY(result.isString());
        QVERIFY(result.toString().contains("Unsupported Lua type"));
        lua_pop(L, 1);
    }

    // ========== jsonToLuaStack tests ==========

    void testJsonToLuaStack_null()
    {
        QJsonValue value;
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_isnil(L, -1));
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_boolean_true()
    {
        QJsonValue value(true);
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_isboolean(L, -1));
        QCOMPARE(lua_toboolean(L, -1), 1);
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_boolean_false()
    {
        QJsonValue value(false);
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_isboolean(L, -1));
        QCOMPARE(lua_toboolean(L, -1), 0);
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_number_integer()
    {
        QJsonValue value(42);
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_isnumber(L, -1));
        QCOMPARE(lua_tointeger(L, -1), 42);
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_number_float()
    {
        QJsonValue value(3.14159);
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_isnumber(L, -1));
        QCOMPARE(lua_tonumber(L, -1), 3.14159);
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_string()
    {
        QJsonValue value(QString("hello world"));
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_isstring(L, -1));
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), QString("hello world"));
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_string_empty()
    {
        QJsonValue value(QString(""));
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_isstring(L, -1));
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), QString(""));
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_emptyArray()
    {
        QJsonArray arr;
        QJsonValue value(arr);
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_istable(L, -1));
        lua_pushnil(L);
        QVERIFY(lua_next(L, -2) == 0);
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_array()
    {
        QJsonArray arr;
        arr.append("first");
        arr.append("second");
        arr.append("third");
        QJsonValue value(arr);

        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_istable(L, -1));

        lua_rawgeti(L, -1, 1);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), QString("first"));
        lua_pop(L, 1);

        lua_rawgeti(L, -1, 2);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), QString("second"));
        lua_pop(L, 1);

        lua_rawgeti(L, -1, 3);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), QString("third"));
        lua_pop(L, 1);

        lua_pop(L, 1);
    }

    void testJsonToLuaStack_emptyObject()
    {
        QJsonObject obj;
        QJsonValue value(obj);
        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_istable(L, -1));
        lua_pushnil(L);
        QVERIFY(lua_next(L, -2) == 0);
        lua_pop(L, 1);
    }

    void testJsonToLuaStack_object()
    {
        QJsonObject obj;
        obj["name"] = "Mudlet";
        obj["version"] = 4.0;
        obj["active"] = true;
        QJsonValue value(obj);

        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_istable(L, -1));

        lua_getfield(L, -1, "name");
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), QString("Mudlet"));
        lua_pop(L, 1);

        lua_getfield(L, -1, "version");
        QCOMPARE(lua_tonumber(L, -1), 4.0);
        lua_pop(L, 1);

        lua_getfield(L, -1, "active");
        QCOMPARE(lua_toboolean(L, -1), 1);
        lua_pop(L, 1);

        lua_pop(L, 1);
    }

    void testJsonToLuaStack_nestedObject()
    {
        QJsonObject inner;
        inner["value"] = "nested";
        QJsonObject outer;
        outer["inner"] = inner;
        QJsonValue value(outer);

        bridge->jsonToLuaStack(L, value);
        QVERIFY(lua_istable(L, -1));

        lua_getfield(L, -1, "inner");
        QVERIFY(lua_istable(L, -1));

        lua_getfield(L, -1, "value");
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), QString("nested"));
        lua_pop(L, 3);
    }

    // ========== Round-trip conversion tests ==========

    void testConversionRoundtrip_primitives()
    {
        QJsonObject original;
        original["string"] = "test";
        original["number"] = 42.5;
        original["bool"] = true;

        bridge->jsonToLuaStack(L, QJsonValue(original));
        QJsonValue result = bridge->luaStackToJson(L, -1);

        QVERIFY(result.isObject());
        QJsonObject resultObj = result.toObject();
        QCOMPARE(resultObj["string"].toString(), QString("test"));
        QCOMPARE(resultObj["number"].toDouble(), 42.5);
        QCOMPARE(resultObj["bool"].toBool(), true);
        lua_pop(L, 1);
    }

    void testConversionRoundtrip_array()
    {
        QJsonArray original;
        original.append(1);
        original.append(2);
        original.append(3);

        bridge->jsonToLuaStack(L, QJsonValue(original));
        QJsonValue result = bridge->luaStackToJson(L, -1);

        QVERIFY(result.isObject());
        QJsonObject resultObj = result.toObject();
        QCOMPARE(resultObj["1"].toInt(), 1);
        QCOMPARE(resultObj["2"].toInt(), 2);
        QCOMPARE(resultObj["3"].toInt(), 3);
        lua_pop(L, 1);
    }

    void testConversionRoundtrip_complex()
    {
        QJsonObject inner;
        inner["data"] = "value";
        QJsonArray arr;
        arr.append(1);
        arr.append(2);
        QJsonObject original;
        original["nested"] = inner;
        original["array"] = arr;
        original["flag"] = false;

        bridge->jsonToLuaStack(L, QJsonValue(original));
        QJsonValue result = bridge->luaStackToJson(L, -1);

        QVERIFY(result.isObject());
        QJsonObject resultObj = result.toObject();
        QVERIFY(resultObj["nested"].isObject());
        QCOMPARE(resultObj["nested"].toObject()["data"].toString(), QString("value"));
        QCOMPARE(resultObj["flag"].toBool(), false);
        lua_pop(L, 1);
    }

    // ========== getAvailableTools tests ==========

    void testGetAvailableTools_afterLoad()
    {
        bridge->loadLuaFunctions();
        QJsonArray tools = bridge->getAvailableTools();

        QVERIFY(tools.size() >= 1);

        bool foundLuaTool = false;
        for (const auto& tool : tools) {
            QJsonObject toolObj = tool.toObject();
            if (toolObj["name"].toString() == "lua") {
                foundLuaTool = true;
                QVERIFY(toolObj.contains("description"));
                QVERIFY(toolObj.contains("inputSchema"));

                QJsonObject schema = toolObj["inputSchema"].toObject();
                QCOMPARE(schema["type"].toString(), QString("object"));
                QVERIFY(schema.contains("properties"));
                QVERIFY(schema["properties"].toObject().contains("code"));
            }
        }
        QVERIFY(foundLuaTool);
    }

    void testGetAvailableTools_schemaStructure()
    {
        bridge->loadLuaFunctions();
        QJsonArray tools = bridge->getAvailableTools();

        for (const auto& tool : tools) {
            QJsonObject toolObj = tool.toObject();
            QVERIFY(toolObj.contains("name"));
            QVERIFY(toolObj.contains("description"));
            QVERIFY(toolObj.contains("inputSchema"));
            QVERIFY(!toolObj["name"].toString().isEmpty());
        }
    }

    // ========== callTool tests ==========

    void testCallTool_unknownTool()
    {
        bridge->loadLuaFunctions();
        QJsonObject args;
        MCPToolResult result = bridge->callTool("nonexistent_tool", args);

        QVERIFY(!result.success);
        QCOMPARE(result.errorCode, -32601);
        QVERIFY(result.errorMessage.contains("not found"));
    }

    void testCallTool_missingCode()
    {
        bridge->loadLuaFunctions();
        QJsonObject args;
        MCPToolResult result = bridge->callTool("lua", args);

        QVERIFY(!result.success);
        QCOMPARE(result.errorCode, -32602);
        QVERIFY(result.errorMessage.contains("code"));
    }

    void testCallTool_emptyCode()
    {
        bridge->loadLuaFunctions();
        QJsonObject args;
        args["code"] = "";
        MCPToolResult result = bridge->callTool("lua", args);

        QVERIFY(!result.success);
        QCOMPARE(result.errorCode, -32602);
    }

    // ========== loadLuaFunctions tests ==========

    void testLoadLuaFunctions_idempotent()
    {
        QVERIFY(bridge->loadLuaFunctions());
        QJsonArray tools1 = bridge->getAvailableTools();

        QVERIFY(bridge->loadLuaFunctions());
        QJsonArray tools2 = bridge->getAvailableTools();

        QCOMPARE(tools1.size(), tools2.size());
    }
};

#include "TMCPLuaBridgeTest.moc"
QTEST_MAIN(TMCPLuaBridgeTest)
