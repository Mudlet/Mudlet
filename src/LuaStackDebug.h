/***************************************************************************
 *   Copyright (C) 2025 by Mudlet team - see AUTHORS                       *
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

#ifndef LUASTACKDEBUG_H
#define LUASTACKDEBUG_H

// Temporary debug helper for investigating Lua stack corruption in table argument parsing
// Remove this file after debugging is complete

extern "C" {
#include <lua.h>
}

#include <QDebug>

// Enable this to get verbose stack debugging
#define DEBUG_LUA_TABLE_ARGS 1

#ifdef DEBUG_LUA_TABLE_ARGS

inline void debugPrintStack(lua_State* L, const char* location, const char* funcName)
{
    int top = lua_gettop(L);
    qDebug() << "[LuaStackDebug]" << funcName << "@" << location << "- stack size:" << top;
    for (int i = 1; i <= top; ++i) {
        int type = lua_type(L, i);
        const char* typeName = lua_typename(L, type);
        QString value;
        switch (type) {
        case LUA_TSTRING:
            value = QString::fromUtf8(lua_tostring(L, i));
            if (value.length() > 50) {
                value = value.left(50) + QStringLiteral("...");
            }
            break;
        case LUA_TNUMBER:
            value = QString::number(lua_tonumber(L, i));
            break;
        case LUA_TBOOLEAN:
            value = lua_toboolean(L, i) ? QStringLiteral("true") : QStringLiteral("false");
            break;
        case LUA_TNIL:
            value = QStringLiteral("nil");
            break;
        default:
            value = QStringLiteral("<") + QString::fromUtf8(typeName) + QStringLiteral(">");
            break;
        }
        qDebug() << "  slot" << i << "(" << (i - top - 1) << "):" << typeName << "=" << value;
    }
}

// RAII helper to track stack balance
class LuaStackGuard {
public:
    LuaStackGuard(lua_State* L, const char* funcName, const char* location, int expectedDelta = 0)
        : mL(L), mFuncName(funcName), mLocation(location), mInitialTop(lua_gettop(L)), mExpectedDelta(expectedDelta)
    {
        qDebug() << "[LuaStackGuard] ENTER" << mFuncName << "@" << mLocation
                 << "- initial stack:" << mInitialTop << ", expected delta:" << mExpectedDelta;
    }

    ~LuaStackGuard()
    {
        int finalTop = lua_gettop(mL);
        int actualDelta = finalTop - mInitialTop;
        if (actualDelta != mExpectedDelta) {
            qCritical() << "[LuaStackGuard] STACK IMBALANCE in" << mFuncName << "@" << mLocation
                        << "- initial:" << mInitialTop << ", final:" << finalTop
                        << ", expected delta:" << mExpectedDelta << ", actual delta:" << actualDelta;
            debugPrintStack(mL, "on imbalance", mFuncName);
        } else {
            qDebug() << "[LuaStackGuard] EXIT" << mFuncName << "@" << mLocation
                     << "- final stack:" << finalTop << "(OK)";
        }
    }

    void checkPoint(const char* desc)
    {
        int currentTop = lua_gettop(mL);
        qDebug() << "[LuaStackGuard] CHECKPOINT" << mFuncName << "@" << desc
                 << "- current stack:" << currentTop << "(delta from start:" << (currentTop - mInitialTop) << ")";
    }

private:
    lua_State* mL;
    const char* mFuncName;
    const char* mLocation;
    int mInitialTop;
    int mExpectedDelta;
};

// Macro to easily add stack checking
#define LUA_STACK_DEBUG(L, func, loc) debugPrintStack(L, loc, func)
#define LUA_STACK_GUARD(L, func, loc, delta) LuaStackGuard _stackGuard##__LINE__(L, func, loc, delta)
#define LUA_STACK_GUARD_NAMED(L, func, loc, delta, name) LuaStackGuard name(L, func, loc, delta)
#define LUA_STACK_CHECKPOINT(guard, desc) guard.checkPoint(desc)

// Special macro for tracking table iteration
#define LUA_TABLE_ITER_DEBUG(L, func, iteration) \
    qDebug() << "[LuaTableIter]" << func << "iteration #" << iteration \
             << "- key type:" << lua_typename(L, lua_type(L, -2)) \
             << ", value type:" << lua_typename(L, lua_type(L, -1)) \
             << ", stack size:" << lua_gettop(L)

// Debug macro to check parseCommandOrFunction specifically
#define DEBUG_PARSE_CMD_BEFORE(L, func, index) \
    qDebug() << "[ParseCmd] BEFORE" << func << "- index:" << index \
             << ", stack size:" << lua_gettop(L) \
             << ", type at index:" << lua_typename(L, lua_type(L, index))

#define DEBUG_PARSE_CMD_AFTER(L, func, index, luaRef) \
    qDebug() << "[ParseCmd] AFTER" << func << "- index:" << index \
             << ", stack size:" << lua_gettop(L) \
             << ", luaRef:" << luaRef

#else

#define LUA_STACK_DEBUG(L, func, loc) ((void)0)
#define LUA_STACK_GUARD(L, func, loc, delta) ((void)0)
#define LUA_STACK_GUARD_NAMED(L, func, loc, delta, name) ((void)0)
#define LUA_STACK_CHECKPOINT(guard, desc) ((void)0)
#define LUA_TABLE_ITER_DEBUG(L, func, iteration) ((void)0)
#define DEBUG_PARSE_CMD_BEFORE(L, func, index) ((void)0)
#define DEBUG_PARSE_CMD_AFTER(L, func, index, luaRef) ((void)0)

#endif // DEBUG_LUA_TABLE_ARGS

#endif // LUASTACKDEBUG_H
