/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "THeadlessSession.h"

#include "Host.h"
#include "TLuaInterpreter.h"

namespace {

// What getConfig() left on the top of the stack, or nothing for a value a frontend has no use for
QVariant settingValue(lua_State* L)
{
    switch (lua_type(L, -1)) {
    case LUA_TBOOLEAN:
        return QVariant(static_cast<bool>(lua_toboolean(L, -1)));
    case LUA_TNUMBER:
        return QVariant(static_cast<double>(lua_tonumber(L, -1)));
    case LUA_TSTRING: {
        size_t length = 0;
        const char* text = lua_tolstring(L, -1, &length);
        return QVariant(QString::fromUtf8(text, static_cast<qsizetype>(length)));
    }
    default:
        return {};
    }
}

void pushString(lua_State* L, const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    lua_pushlstring(L, utf8.constData(), static_cast<size_t>(utf8.size()));
}

} // namespace

bool THeadlessSession::settings(const QStringList& keys, QVariantMap& values)
{
    if (!usable()) {
        return false;
    }
    lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
    for (const QString& key : keys) {
        const int top = lua_gettop(L);
        lua_pushcfunction(L, TLuaInterpreter::getConfig);
        pushString(L, key);
        // The names of the options, as showSentText has, rather than the booleans kept for older scripts
        lua_pushboolean(L, true);
        if (lua_pcall(L, 2, 1, 0) == 0) {
            if (const QVariant value = settingValue(L); value.isValid()) {
                values.insert(key, value);
            }
        }
        lua_settop(L, top);
    }
    return true;
}

bool THeadlessSession::setSettings(const QVariantMap& values, QMap<QString, QString>& refused)
{
    if (!usable()) {
        return false;
    }
    lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        const QVariant& value = it.value();
        const int top = lua_gettop(L);
        lua_pushcfunction(L, TLuaInterpreter::setConfig);
        pushString(L, it.key());
        switch (value.typeId()) {
        case QMetaType::Bool:
            lua_pushboolean(L, value.toBool());
            break;
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Double:
            lua_pushnumber(L, value.toDouble());
            break;
        case QMetaType::QString:
            pushString(L, value.toString());
            break;
        default:
            lua_settop(L, top);
            //: Why a frontend with no window could not change one of a profile's settings
            refused.insert(it.key(), tr("A setting takes a true or false, a number or some text."));
            continue;
        }
        if (lua_pcall(L, 2, 2, 0) != 0) {
            refused.insert(it.key(), QString::fromUtf8(lua_tostring(L, -1)));
        } else if (!lua_toboolean(L, -2)) {
            refused.insert(it.key(), lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString());
        }
        lua_settop(L, top);
    }
    return true;
}
