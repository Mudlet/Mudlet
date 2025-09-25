/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2022, 2024 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2016 by Eric Wallace - eewallace@gmail.com              *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2022-2025 by Lecker Kebap - Leris@mudlet.org            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// mudlet-object specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

#include "TLuaInterpreter.h"

#include "EAction.h"
#include "Host.h"
#include "TAlias.h"
#include "TArea.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TFlipButton.h"
#include "TForkedProcess.h"
#include "TLabel.h"
#include "TMapLabel.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TTimer.h"
#include "dlgComposer.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgTriggerEditor.h"
#include "mapInfoContributorManager.h"
#include "mudlet.h"
#include "TGameDetails.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget_integration.h"
#endif

#include <limits>
#include <math.h>

#include "pre_guard.h"
#include <QtConcurrent>
#include <QCollator>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTableWidget>
#include <QToolTip>
#include <QFileInfo>
#include <QMovie>
#include <QVector>
#ifdef QT_TEXTTOSPEECH_LIB
#include <QTextToSpeech>
#endif // QT_TEXTTOSPEECH_LIB
#include "post_guard.h"

static const char *bad_window_type = "%s: bad argument #%d type (window name as string expected, got %s)!";
static const char *bad_cmdline_type = "%s: bad argument #%d type (command line name as string expected, got %s)!";
static const char *bad_window_value = "window \"%s\" not found";
static const char *bad_cmdline_value = "command line \"%s\" not found";
// Not used: static const char *bad_label_value = "label \"%s\" not found";

// No documentation available in wiki - internal function
static bool isMain(const QString& name)
{
    if (name.isEmpty()) {
        return true;
    }
    if (!name.compare(qsl("main"))) {
        return true;
    }
    return false;
}

#define WINDOW_NAME(ARG_L, ARG_pos)                                                                      \
    ({                                                                                                   \
        int pos_ = (ARG_pos);                                                                            \
        const char *res_;                                                                                \
        if ((lua_gettop(ARG_L) < pos_) || lua_isnil(ARG_L, pos_)) {                                      \
            res_ = "";                                                                                   \
        } else {                                                                                         \
            if (!lua_isstring(ARG_L, pos_)) {                                                            \
                lua_pushfstring(ARG_L, bad_window_type, __FUNCTION__, pos_, luaL_typename(ARG_L, pos_)); \
                return lua_error(ARG_L);                                                                 \
            }                                                                                            \
            res_ = lua_tostring(ARG_L, pos_);                                                            \
        }                                                                                                \
        res_;                                                                                            \
    })

#define CMDLINE_NAME(ARG_L, ARG_pos)                                                                 \
    ({                                                                                               \
        int pos_ = (ARG_pos);                                                                        \
        if (!lua_isstring(ARG_L, pos_)) {                                                            \
            lua_pushfstring(ARG_L, bad_cmdline_type, __FUNCTION__, pos_, luaL_typename(ARG_L, pos_));\
            return lua_error(ARG_L);                                                                 \
        }                                                                                            \
        lua_tostring(ARG_L, pos_);                                                                   \
    })

#define CONSOLE_NIL(ARG_L, ARG_name)                                                           \
    ({                                                                                         \
        auto name_ = (ARG_name);                                                               \
        auto console_ = getHostFromLua(ARG_L).findConsole(name_);                              \
        console_;                                                                              \
    })

#define CONSOLE(ARG_L, ARG_name)                                                               \
    ({                                                                                         \
        auto name_ = (ARG_name);                                                               \
        auto console_ = getHostFromLua(ARG_L).findConsole(name_);                              \
        if (!console_) {                                                                       \
            lua_pushnil(ARG_L);                                                                \
            lua_pushfstring(ARG_L, bad_window_value, name_.toUtf8().constData());              \
            return 2;                                                                          \
        }                                                                                      \
        console_;                                                                              \
    })

#define COMMANDLINE(ARG_L, ARG_name)                                                           \
    ({                                                                                         \
        const QString& name_ = (ARG_name);                                                     \
        auto console_ = getHostFromLua(ARG_L).mpConsole;                                       \
        auto cmdLine_ = isMain(name_) ? &*console_->mpCommandLine                              \
                                    : console_->mSubCommandLineMap.value(name_);               \
        if (!cmdLine_) {                                                                       \
            lua_pushnil(ARG_L);                                                                \
            lua_pushfstring(ARG_L, bad_cmdline_value, name_.toUtf8().constData());             \
            return 2;                                                                          \
        }                                                                                      \
        cmdLine_;                                                                              \
    })

#define LABEL(ARG_L, ARG_name)                                                                 \
    ({                                                                                         \
        const QString& name_ = (ARG_name);                                                     \
        auto console_ = getHostFromLua(ARG_L).mpConsole;                                       \
        auto label_ = console_->mLabelMap.value(name_);                                        \
        if (!label_) {                                                                         \
            lua_pushnil(ARG_L);                                                                \
            lua_pushfstring(ARG_L, bad_label_value, name_.toUtf8().constData());               \
            return 2;                                                                          \
        }                                                                                      \
        label_;                                                                                \
    })

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCmdLineSuggestion
int TLuaInterpreter::addCmdLineSuggestion(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    const QString text = getVerifiedString(L, __func__, n, "suggestion text");
    auto pN = COMMANDLINE(L, name);
    pN->addSuggestion(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#adjustStopWatch
int TLuaInterpreter::adjustStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "adjustStopWatch: bad argument #1 type (stopwatchID as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    const double adjustment = getVerifiedDouble(L, __func__, 2, "modification in seconds");
    const bool result = host.adjustStopWatch(watchId, qRound(adjustment * 1000.0));
    // This is only likely to fail when a numeric first argument was given:
    if (!result) {
        return warnArgumentValue(L, __func__, csmInvalidStopWatchID.arg(watchId));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#appendCmdLine
int TLuaInterpreter::appendCmdLine(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";

    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    const QString text = getVerifiedString(L, __func__, n, "text to set on command line");
    auto pN = COMMANDLINE(L, name);

    const QString curText = pN->toPlainText();
    pN->setPlainText(curText + text);
    QTextCursor cur = pN->textCursor();
    cur.clearSelection();
    cur.movePosition(QTextCursor::EndOfLine);
    pN->setTextCursor(cur);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearCmdLine
int TLuaInterpreter::clearCmdLine(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto pN = COMMANDLINE(L, name);
    pN->clear();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearCmdLineSuggestions
int TLuaInterpreter::clearCmdLineSuggestions(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n == 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto pN = COMMANDLINE(L, name);
    pN->clearSuggestions();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createStopWatch
int TLuaInterpreter::createStopWatch(lua_State* L)
{
    QString name;
    bool autoStart = true;
    const int n = lua_gettop(L);
    int s = 1;
    if (n) {
        if (lua_type(L, s) == LUA_TBOOLEAN) {
            autoStart = lua_toboolean(L, s);
        } else if (lua_type(L, s) == LUA_TSTRING) {
            autoStart = false;
            name = lua_tostring(L, 1);
        } else if (lua_type(L, s) == LUA_TNIL) {
            ; // fallthrough for compatibility with old-style stopwatches in case createStopWatch(nil) is passed
            // note that 'nil' will still count towards the stack's gettop amount
        } else {
            lua_pushfstring(L, "createStopWatch: bad argument #%d type (name as string or autostart as boolean are optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }

        if (n > 1) {
            autoStart = getVerifiedBool(L, __func__, ++s, "autostart", true);
        }
    }


    Host& host = getHostFromLua(L);
    QPair<int, QString> const result = host.createStopWatch(name);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }

    if (autoStart) {
        host.startStopWatch(result.first);
    }

    lua_pushnumber(L, result.first);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteStopWatch
int TLuaInterpreter::deleteStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "deleteStopWatch: bad argument #1 type (stopwatchID as number or stopwatch name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    const bool result = host.destroyStopWatch(watchId);
    // This is only likely to fail when a numeric first argument was given:
    if (!result) {
        return warnArgumentValue(L, __func__, csmInvalidStopWatchID.arg(watchId));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCmdLineSuggestion
int TLuaInterpreter::removeCmdLineSuggestion(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    const QString text = getVerifiedString(L, __func__, n, "suggestion text");
    auto pN = COMMANDLINE(L, name);
    pN->removeSuggestion(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableAlias
int TLuaInterpreter::disableAlias(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    const bool error = host.getAliasUnit()->disableAlias(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableKey
int TLuaInterpreter::disableKey(lua_State* L)
{
    const QString keyName = getVerifiedString(L, __func__, 1, "key name");
    Host& host = getHostFromLua(L);
    const bool error = host.getKeyUnit()->disableKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScript
int TLuaInterpreter::disableScript(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "script name");

    Host& host = getHostFromLua(L);
    int cnt = 0;
    QMap<int, TScript*> const scripts = host.getScriptUnit()->getScriptList();
    for (auto script : scripts) {
        if (script->getName() == name) {
            cnt++;
            script->setIsActive(false);
        }
    }
    if (cnt == 0) {
        return warnArgumentValue(L, __func__, qsl("script '%1' not found").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTimer
int TLuaInterpreter::disableTimer(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    const bool error = host.getTimerUnit()->disableTimer(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTrigger
int TLuaInterpreter::disableTrigger(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    const bool error = host.getTriggerUnit()->disableTrigger(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableAlias
int TLuaInterpreter::enableAlias(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    const bool error = host.getAliasUnit()->enableAlias(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableKey
int TLuaInterpreter::enableKey(lua_State* L)
{
    const QString keyName = getVerifiedString(L, __func__, 1, "key name");
    Host& host = getHostFromLua(L);
    const bool error = host.getKeyUnit()->enableKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScript
int TLuaInterpreter::enableScript(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "script name");

    Host& host = getHostFromLua(L);
    int cnt = 0;
    QMap<int, TScript*> const scripts = host.getScriptUnit()->getScriptList();
    for (auto script : scripts) {
        if (script->getName() == name) {
            cnt++;
            script->setIsActive(true);
        }
    }
    if (cnt == 0) {
        return warnArgumentValue(L, __func__, qsl("script '%1' not found").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableTimer
int TLuaInterpreter::enableTimer(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    const bool error = host.getTimerUnit()->enableTimer(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableTrigger
int TLuaInterpreter::enableTrigger(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    const bool error = host.getTriggerUnit()->enableTrigger(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#exists
int TLuaInterpreter::exists(lua_State* L)
{
    auto [isId, nameOrId] = getVerifiedStringOrInteger(L, __func__, 1, "itemID or item name");
    // Although we only use 6 ASCII strings the user may not enter a purely
    // ASCII value which we might have to report...
    QString type = getVerifiedString(L, __func__, 2, "item type").toLower();
    bool isOk = false;
    const int id = nameOrId.toInt(&isOk);
    if (isId && (!isOk || id < 0)) {
        // Must be zero or more but doesn't seem to be, must return the
        // original supplied argument as a string (rather than the nameOrId
        // "number" as the latter will have been rounded to an integer) to
        // show what was entered:
        return warnArgumentValue(L, __func__, csmInvalidItemID.arg(lua_tostring(L, 1)));
    }

    Host& host = getHostFromLua(L);
    int count = 0;
    type = type.toLower();
    if (!type.compare(QLatin1String("timer"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getTimerUnit()->getTimer(id);
            lua_pushnumber(L, static_cast<bool>(pT) ? 1 : 0);
            return 1;
        }

        count = host.getTimerUnit()->mLookupTable.count(nameOrId);
    } else if (!type.compare(QLatin1String("trigger"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getTriggerUnit()->getTrigger(id);
            lua_pushnumber(L, static_cast<bool>(pT) ? 1 : 0);
            return 1;
        }

        count = host.getTriggerUnit()->mLookupTable.count(nameOrId);
    } else if (!type.compare(QLatin1String("alias"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getAliasUnit()->getAlias(id);
            lua_pushnumber(L, static_cast<bool>(pT) ? 1 : 0);
            return 1;
        }

        count = host.getAliasUnit()->mLookupTable.count(nameOrId);
    } else if (!type.compare(QLatin1String("keybind"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getKeyUnit()->getKey(id);
            lua_pushnumber(L, static_cast<bool>(pT) ? 1 : 0);
            return 1;
        }

        count = host.getKeyUnit()->mLookupTable.count(nameOrId);
    } else if (!type.compare(QLatin1String("button"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getActionUnit()->getAction(id);
            lua_pushnumber(L, static_cast<bool>(pT) ? 1 : 0);
            return 1;
        }

        count = host.getActionUnit()->findItems(nameOrId).size();
    } else if (!type.compare(QLatin1String("script"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getScriptUnit()->getScript(id);
            lua_pushnumber(L, static_cast<bool>(pT) ? 1 : 0);
            return 1;
        }

        count = host.getScriptUnit()->findItems(nameOrId).size();
    } else {
        return warnArgumentValue(L, __func__, qsl(
            "invalid item type '%1' given, it should be one of: 'alias', 'button', 'script', 'keybind', 'timer' or 'trigger'").arg(type));
    }
    // If we get here we have successfully identified a type and have looked for
    // the item type with a specific NAME - so now just return the count of
    // those found:
    lua_pushnumber(L, count);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getButtonState
int TLuaInterpreter::getButtonState(lua_State* L)
{
    auto& host = getHostFromLua(L);
    if (!lua_gettop(L)) {
        // The original function only works in the script for a push-down button
        // and takes no arguments so provide the backwards compatible behaviour
        // if that is the case:
        lua_pushnumber(L, host.mpConsole->getButtonState());
        return 1;
    }

    auto [retCount, pItem] = getTActionFromIdOrName(L, 1, __func__);
    if (retCount) {
        // pItem will be a nullptr if retCount is non-zero:
        return retCount;
    }

    lua_pushboolean(L, pItem->mButtonState);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCmdLine
int TLuaInterpreter::getCmdLine(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n >= 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto commandline = COMMANDLINE(L, name);
    const QString text = commandline->toPlainText();
    lua_pushstring(L, text.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getConsoleBufferSize
int TLuaInterpreter::getConsoleBufferSize(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L)) {
        windowName = WINDOW_NAME(L, 1);
    }

    // The macro will have returned with a nil + error message if the windowName
    // was not found:
    auto console = CONSOLE(L, windowName);
    // Indicate success with two numeric return values:
    lua_pushnumber(L, console->buffer.mLinesLimit);
    lua_pushnumber(L, console->buffer.mBatchDeleteSize);
    return 2;
}


int TLuaInterpreter::getProfileStats(lua_State* L)
{
    Host& host = getHostFromLua(L);

    auto [_1, triggersTotal, totalPatterns, tempTriggers, activeTriggers, activePatterns] = host.getTriggerUnit()->assembleReport();
    auto [_2, aliasesTotal, tempAliases, activeAliases] = host.getAliasUnit()->assembleReport();
    auto [_3, timersTotal, tempTimers, activeTimers] = host.getTimerUnit()->assembleReport();
    auto [_4, keysTotal, tempKeys, activeKeys] = host.getKeyUnit()->assembleReport();
    auto [_5, scriptsTotal, tempScripts, activeScripts] = host.getScriptUnit()->assembleReport();
    auto [_6, gifsTotal, activeGifs] = host.getGifTracker()->assembleReport();

    lua_newtable(L);

    // Triggers
    lua_pushstring(L, "triggers");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, triggersTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempTriggers);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeTriggers);
    lua_settable(L, -3); // active

    lua_pushstring(L, "patterns");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, totalPatterns);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activePatterns);
    lua_settable(L, -3);

    lua_settable(L, -3); // patterns
    lua_settable(L, -3); // triggers

    // Aliases
    lua_pushstring(L, "aliases");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, aliasesTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempAliases);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeAliases);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Timers
    lua_pushstring(L, "timers");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, timersTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempTimers);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeTimers);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Keys
    lua_pushstring(L, "keys");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, keysTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempKeys);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeKeys);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Scripts
    lua_pushstring(L, "scripts");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, scriptsTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempScripts);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeScripts);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Gifs
    lua_pushstring(L,"gifs");
    lua_newtable(L);

    lua_pushstring(L,"total");
    lua_pushnumber(L,gifsTotal);
    lua_settable(L,-3);

    lua_pushstring(L,"active");
    lua_pushnumber(L,activeGifs);
    lua_settable(L,-3);
    lua_settable(L,-3);

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getStopWatches
int TLuaInterpreter::getStopWatches(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const QList<int> stopWatchIds = host.getStopWatchIds();
    lua_newtable(L);
    for (const int watchId : stopWatchIds) {
        lua_pushnumber(L, watchId);
        auto pStopWatch = host.getStopWatch(watchId);
        lua_newtable(L);
        {
            lua_pushstring(L, "name");
            lua_pushstring(L, pStopWatch->name().toUtf8().constData());
            lua_settable(L, -3);

            lua_pushstring(L, "isRunning");
            lua_pushboolean(L, pStopWatch->running());
            lua_settable(L, -3);

            lua_pushstring(L, "isPersistent");
            lua_pushboolean(L, pStopWatch->persistent());
            lua_settable(L, -3);

            lua_pushstring(L, "elapsedTime");
            const QStringList splitTimeString(pStopWatch->getElapsedDayTimeString().split(QLatin1Char(':')));
            generateElapsedTimeTable(L, splitTimeString, true, pStopWatch->getElapsedMilliSeconds());
            lua_settable(L, -3);
        }
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getStopWatchTime
int TLuaInterpreter::getStopWatchTime(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "getStopWatchTime: bad argument #1 type (stopwatchID as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    int watchId = 0;
    QPair<bool, double> result;
    const Host& host = getHostFromLua(L);
    if (lua_type(L, 1) == LUA_TNUMBER) {
        watchId = static_cast<int>(lua_tointeger(L, 1));
        result = host.getStopWatchTime(watchId);
        if (!result.first) {
            return warnArgumentValue(L, __func__, csmInvalidStopWatchID.arg(watchId));
        }

    } else {
        const QString name{lua_tostring(L, 1)};
        // Using an empty string will return the first unnamed stopwatch:
        watchId = host.findStopWatchId(name);
        if (!watchId) {
            if (name.isEmpty()) {
                return warnArgumentValue(L, __func__, "no unnamed stopwatches found");
            }
            return warnArgumentValue(L, __func__, qsl("stopwatch with name '%1' not found").arg(name));
        }

        result = host.getStopWatchTime(watchId);
        // We have already validated the name to get the watchId - so for things
        // to fail now is, unlikely?
        if (Q_UNLIKELY(!result.first)) {
            return warnArgumentValue(L, __func__, qsl(
                "stopwatch with name '%1' (ID: %2) has disappeared - this should not happen, please report it to Mudlet developers")
                .arg(name, QString::number(watchId)));
        }
    }

    lua_pushnumber(L, result.second);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getStopWatchBrokenDownTime
int TLuaInterpreter::getStopWatchBrokenDownTime(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "getStopWatchBrokenDownTime: bad argument #1 type (stopwatchID as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    QPair<bool, QString> const result = host.getBrokenDownStopWatchTime(watchId);
    // This is only likely to fail when a numeric first argument was given:
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }

    const QStringList splitTimeString(result.second.split(QLatin1Char(':')));
    generateElapsedTimeTable(L, splitTimeString, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getScript
int TLuaInterpreter::getScript(lua_State* L)
{
    const int n = lua_gettop(L);
    int pos = 1;
    const QString name = getVerifiedString(L, __func__, 1, "script name");
    if (n > 1) {
        pos = getVerifiedInt(L, __func__, 2, "script position");
    }
    Host& host = getHostFromLua(L);

    auto ids = host.getScriptUnit()->findItems(name);
    if (pos >= 1 && pos <= static_cast<int>(ids.size())) {
        auto pS = host.getScriptUnit()->getScript(ids.at(pos - 1));
        if (pS) {
            lua_pushstring(L, pS->getScript().toUtf8().constData());
            lua_pushnumber(L, ids.at(pos - 1));
            return 2;
        }
    }

    lua_pushnumber(L, -1);
    lua_pushstring(L, qsl("script \"%1\" at position %2 not found").arg(name, QString::number(pos)).toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#invokeFileDialog
int TLuaInterpreter::invokeFileDialog(lua_State* L)
{
    const int n = lua_gettop(L);
    Host& host = getHostFromLua(L);
    QString location = mudlet::getMudletPath(enums::profileHomePath, host.getName());
    const bool luaDir = getVerifiedBool(L, __func__, 1, "fileOrFolder");
    const QString title = getVerifiedString(L, __func__, 2, "dialogTitle");

    if (n > 2) {
        QString target = getVerifiedString(L, __func__, 3, "dialogLocation");
        QDir dir(target);

        if (dir.exists()) {
            location = target;
        }
    }

    if (!luaDir) {
        const QString fileName = QFileDialog::getExistingDirectory(nullptr, title, location);
        lua_pushstring(L, fileName.toUtf8().constData());
        return 1;
    } else {
        const QString fileName = QFileDialog::getOpenFileName(nullptr, title, location);
        lua_pushstring(L, fileName.toUtf8().constData());
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isActive
int TLuaInterpreter::isActive(lua_State* L)
{
    auto [isId, nameOrId] = getVerifiedStringOrInteger(L, __func__, 1, "item name or ID");
    // Although we only use 4 ASCII strings the user may not enter a purely
    // ASCII value which we might have to report...
    const QString type = getVerifiedString(L, __func__, 2, "item type");
    bool isOk = false;
    const int id = nameOrId.toInt(&isOk);
    if (isId && (!isOk || id < 0)) {
        // Must be zero or more but doesn't seem to be, must return the
        // original supplied argument as a string (rather than the nameOrId
        // "number" as the latter will have been rounded to an integer) to
        // show what was entered:
        return warnArgumentValue(L, __func__, csmInvalidItemID.arg(lua_tostring(L, 1)));
    }
    bool checkAncestors = false;
    if (lua_gettop(L) > 2) {
        checkAncestors = getVerifiedBool(L, __func__, 3, "also check ancestors", true);
    }

    Host& host = getHostFromLua(L);
    int cnt = 0;
    // Remember, QString::compare(...) returns zero for a match:
    if (!type.compare(QLatin1String("timer"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getTimerUnit()->getTimer(id);
            cnt = (static_cast<bool>(pT)
                   && (pT->isOffsetTimer() ? pT->shouldBeActive() : pT->isActive())
                   && (!checkAncestors || pT->shouldAncestorsBeActive())) ? 1 : 0;
        } else {
            auto itpItem = host.getTimerUnit()->mLookupTable.constFind(nameOrId);
            while (itpItem != host.getTimerUnit()->mLookupTable.cend() && itpItem.key() == nameOrId) {
                auto pT = itpItem.value();
                // Offset timer have their active state recorded differently
                if ((pT->isOffsetTimer() ? pT->shouldBeActive() : pT->isActive())
                    && (!checkAncestors || pT->shouldAncestorsBeActive())) {

                    ++cnt;
                }
                ++itpItem;
            }
        }

    } else if (!type.compare(QLatin1String("trigger"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getTriggerUnit()->getTrigger(id);
            cnt = (static_cast<bool>(pT) && pT->isActive()) ? 1 : 0;
        } else {
            auto itpItem = host.getTriggerUnit()->mLookupTable.constFind(nameOrId);
            while (itpItem != host.getTriggerUnit()->mLookupTable.cend() && itpItem.key() == nameOrId) {
                auto pT = itpItem.value();
                if (pT->isActive() && (!checkAncestors || pT->ancestorsActive())) {
                    ++cnt;
                }
                ++itpItem;
            }
        }

    } else if (!type.compare(QLatin1String("alias"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getAliasUnit()->getAlias(id);
            cnt = (static_cast<bool>(pT) && pT->isActive()) ? 1 : 0;
        } else {
            auto itpItem = host.getAliasUnit()->mLookupTable.constFind(nameOrId);
            while (itpItem != host.getAliasUnit()->mLookupTable.cend() && itpItem.key() == nameOrId) {
                auto pT = itpItem.value();
                if (pT->isActive() && (!checkAncestors || pT->ancestorsActive())) {
                    ++cnt;
                }
                ++itpItem;
            }
        }

    } else if (!type.compare(QLatin1String("keybind"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getKeyUnit()->getKey(id);
            cnt = (static_cast<bool>(pT) && pT->isActive()) ? 1 : 0;
        } else {
            auto itpItem = host.getKeyUnit()->mLookupTable.constFind(nameOrId);
            while (itpItem != host.getKeyUnit()->mLookupTable.cend() && itpItem.key() == nameOrId) {
                auto pT = itpItem.value();
                if (pT->isActive() && (!checkAncestors || pT->ancestorsActive())) {
                    ++cnt;
                }
                ++itpItem;
            }
        }

    } else if (!type.compare(QLatin1String("button"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getActionUnit()->getAction(id);
            cnt = (static_cast<bool>(pT) && pT->isActive()) ? 1 : 0;
        } else {
            QMap<int, TAction*> const actions = host.getActionUnit()->getActionList();
            for (auto action : actions) {
                if (action->getName() == nameOrId && action->isActive() && (!checkAncestors || action->ancestorsActive())) {
                    ++cnt;
                }
            }
        }

    } else if (!type.compare(QLatin1String("script"), Qt::CaseInsensitive)) {
        if (isId) {
            auto pT = host.getScriptUnit()->getScript(id);
            cnt = (static_cast<bool>(pT) && pT->isActive()) ? 1 : 0;
        } else {
            QMap<int, TScript*> const scripts = host.getScriptUnit()->getScriptList();
            for (auto script : scripts) {
                if (script->getName() == nameOrId && script->isActive() && (!checkAncestors || script->ancestorsActive())) {
                    ++cnt;
                }
            }
        }

    } else {
        return warnArgumentValue(L, __func__, qsl(
            "invalid item type '%1' given, it should be one (case insensitive) of: 'alias', 'button', 'script', 'keybind', 'timer' or 'trigger'").arg(type));
    }
    lua_pushnumber(L, cnt);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isPrompt
int TLuaInterpreter::isPrompt(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const int userCursorY = host.mpConsole->getLineNumber();
    if (userCursorY < host.mpConsole->buffer.promptBuffer.size() && userCursorY >= 0) {
        lua_pushboolean(L, host.mpConsole->buffer.promptBuffer.at(userCursorY));
        return 1;
    } else {
        if (host.mpConsole->mTriggerEngineMode && host.mpConsole->mIsPromptLine) {
            lua_pushboolean(L, true);
        } else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killAlias
int TLuaInterpreter::killAlias(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.getAliasUnit()->killAlias(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killKey
int TLuaInterpreter::killKey(lua_State* L)
{
    QString keyName = getVerifiedString(L, __func__, 1, "key name");
    Host& host = getHostFromLua(L);
    const bool error = host.getKeyUnit()->killKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killTimer
int TLuaInterpreter::killTimer(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "ID");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.killTimer(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killTrigger
int TLuaInterpreter::killTrigger(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "ID");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.killTrigger(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permAlias
int TLuaInterpreter::permAlias(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "alias name");
    const QString parent = getVerifiedString(L, __func__, 2, "alias group/parent");
    const QString regex = getVerifiedString(L, __func__, 3, "regexp pattern");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permAlias: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    const QString script{lua_tostring(L, 4)};
    auto [aliasId, message] = pLuaInterpreter->startPermAlias(name, parent, regex, script);
    if (aliasId == -1) {
        lua_pushfstring(L, "permAlias: cannot create alias (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, aliasId);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permPromptTrigger
int TLuaInterpreter::permPromptTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    const QString triggerName = getVerifiedString(L, __func__, 1, "trigger name");
    const QString parentName = getVerifiedString(L, __func__, 2, "parent trigger name");
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(3); !validationResult) {
        lua_pushfstring(L, "permPromptTrigger: bad argument #%d (%s)", 3, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    const QString luaFunction = lua_tostring(L, 3);

    auto [triggerID, message] = pLuaInterpreter->startPermPromptTrigger(triggerName, parentName, luaFunction);
    if(triggerID == - 1) {
        lua_pushfstring(L, "permPromptTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permRegexTrigger
int TLuaInterpreter::permRegexTrigger(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "trigger name");
    const QString parent = getVerifiedString(L, __func__, 2, "trigger parent");

    QStringList regList;
    if (!lua_istable(L, 3)) {
        lua_pushfstring(L, "permRegexTrigger: bad argument #3 type (sub-strings list as table expected, got %s!)",
                        luaL_typename(L, 3));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            regList << lua_tostring(L, -1);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permRegexTrigger: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    const QString script{lua_tostring(L, 4)};
    auto [triggerId, message] = pLuaInterpreter->startPermRegexTrigger(name, parent, regList, script);
    if (triggerId == -1) {
        lua_pushfstring(L, "permRegexTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerId);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permBeginOfLineStringTrigger
int TLuaInterpreter::permBeginOfLineStringTrigger(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "trigger name");
    const QString parent = getVerifiedString(L, __func__, 2, "trigger parent");

    QStringList regList;
    if (!lua_istable(L, 3)) {
        lua_pushfstring(L, "permBeginOfLineStringTrigger: bad argument #3 type (sub-strings list as table expected, got %s!)",
                        luaL_typename(L, 3));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            regList << lua_tostring(L, -1);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permBeginOfLineStringTrigger: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    const QString script{lua_tostring(L, 4)};
    auto [triggerId, message] = pLuaInterpreter->startPermBeginOfLineStringTrigger(name, parent, regList, script);
    if (triggerId == -1) {
        lua_pushfstring(L, "permRegexTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerId);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permSubstringTrigger
int TLuaInterpreter::permSubstringTrigger(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "trigger name");
    const QString parent = getVerifiedString(L, __func__, 2, "trigger parent");
    QStringList regList;
    if (!lua_istable(L, 3)) {
        lua_pushfstring(L, "permSubstringTrigger: bad argument #3 type (sub-strings list as table expected, got %s!)",
                        luaL_typename(L, 3));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            regList << lua_tostring(L, -1);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permSubstringTrigger: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    const QString script{lua_tostring(L, 4)};
    auto [triggerID, message] = pLuaInterpreter->startPermSubstringTrigger(name, parent, regList, script);
    if(triggerID == - 1) {
        lua_pushfstring(L, "permSubstringTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permScript
int TLuaInterpreter::permScript(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "script name");
    const QString parent = getVerifiedString(L, __func__, 2, "script parent name");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(3); !validationResult) {
        lua_pushfstring(L, "permScript: bad argument #%d (%s)", 3, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    const QString luaCode{lua_tostring(L, 3)};
    auto [id, message] = pLuaInterpreter->createPermScript(name, parent, luaCode);
    if (id == -1) {
        lua_pushfstring(L, "permScript: cannot create script (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, id);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permTimer
int TLuaInterpreter::permTimer(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "timer name");
    const QString parent = getVerifiedString(L, __func__, 2, "timer parent name");
    const double time = getVerifiedDouble(L, __func__, 3, "time in seconds");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permTimer: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    const QString luaCode{lua_tostring(L, 4)};
    auto [id, message] = pLuaInterpreter->startPermTimer(name, parent, time, luaCode);
    if (id == -1) {
        lua_pushfstring(L, "permTimer: cannot create timer (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, id);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permKey
int TLuaInterpreter::permKey(lua_State* L)
{
    QString keyName = getVerifiedString(L, __func__, 1, "key name");
    QString parentGroup = getVerifiedString(L, __func__, 2, "key parent group");

    uint_fast8_t argIndex = 3;
    int keyModifier = Qt::NoModifier;
    if (lua_gettop(L) > 4) {
        keyModifier = getVerifiedInt(L, __func__, 3, "key modifier", true);
        argIndex++;
    }
    int keyCode = getVerifiedInt(L, __func__, argIndex, "key code");

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(++argIndex); !validationResult) {
        lua_pushfstring(L, "permKey: bad argument #%d (%s)", argIndex, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    QString luaFunction{lua_tostring(L, argIndex)};
    auto [keyID, message] = pLuaInterpreter->startPermKey(keyName, parentGroup, keyCode, keyModifier, luaFunction);
    if(keyID == - 1) {
        lua_pushfstring(L, "permKey: cannot create key (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, keyID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#printCmdLine
int TLuaInterpreter::printCmdLine(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    const QString text = getVerifiedString(L, __func__, n, "text to set on command line");

    auto pN = COMMANDLINE(L, name);
    pN->setPlainText(text);
    QTextCursor cur = pN->textCursor();
    cur.clearSelection();
    cur.movePosition(QTextCursor::EndOfLine);
    pN->setTextCursor(cur);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseEvent
int TLuaInterpreter::raiseEvent(lua_State* L)
{
    Host& host = getHostFromLua(L);

    TEvent event {};

    const int n = lua_gettop(L);
    // We go from the top of the stack down, because luaL_ref will
    // only reference the object at the top of the stack
    for (int i = n; i >= 1; i--) {
        switch (lua_type(L, -1)) {
        case LUA_TNUMBER:
            // https://en.wikipedia.org/wiki/Double-precision_floating-point_format#IEEE_754_double-precision_binary_floating-point_format:_binary64
            // suggests that 17 decimal digits is the most we can rely on:
            event.mArgumentList.prepend(QString::number(lua_tonumber(L, -1), 'g', 17));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_NUMBER);
            lua_pop(L, 1);
            break;
        case LUA_TSTRING:
            event.mArgumentList.prepend(lua_tostring(L, -1));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_STRING);
            lua_pop(L, 1);
            break;
        case LUA_TBOOLEAN:
            event.mArgumentList.prepend(QString::number(lua_toboolean(L, -1)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_BOOLEAN);
            lua_pop(L, 1);
            break;
        case LUA_TNIL:
            event.mArgumentList.prepend(QString());
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_NIL);
            lua_pop(L, 1);
            break;
        case LUA_TTABLE:
            event.mArgumentList.prepend(QString::number(luaL_ref(L, LUA_REGISTRYINDEX)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_TABLE);
            // luaL_ref pops the object, so we don't have to
            break;
        case LUA_TFUNCTION:
            event.mArgumentList.prepend(QString::number(luaL_ref(L, LUA_REGISTRYINDEX)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_FUNCTION);
            // luaL_ref pops the object, so we don't have to
            break;
        default:
            lua_pushfstring(L,
                            "raiseEvent: bad argument #%d type (string, number, boolean, table,\n"
                            "function, or nil expected, got a %s!)",
                            i,
                            luaL_typename(L, -1));
            return lua_error(L);
        }
    }

    host.raiseEvent(event);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseGlobalEvent
int TLuaInterpreter::raiseGlobalEvent(lua_State* L)
{
    Host& host = getHostFromLua(L);

    const int n = lua_gettop(L);
    if (!n) {
        lua_pushstring(L, "raiseGlobalEvent: missing argument #1 (eventName as, probably, a string expected!)");
        return lua_error(L);
    }

    TEvent event {};

    for (int i = 1; i <= n; ++i) {
        // The sending profile of the event does not receive the event if
        // sent via this command but if the same eventName is to be used for
        // an event within a profile and to other profiles it is safest to
        // insert a string like "local" or "self" or the profile name from
        // getProfileName() as an (last) additional argument after all the
        // other so the handler can tell it is handling a local event from
        // raiseEvent(...) and not one from another profile! - Slysven
        switch (lua_type(L, i)) {
        case LUA_TNUMBER:
            event.mArgumentList.append(QString::number(lua_tonumber(L, i), 'g', 17));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            break;
        case LUA_TSTRING:
            event.mArgumentList.append(lua_tostring(L, i));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            break;
        case LUA_TBOOLEAN:
            event.mArgumentList.append(QString::number(lua_toboolean(L, i)));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);
            break;
        case LUA_TNIL:
            event.mArgumentList.append(QString());
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NIL);
            break;
        default:
            lua_pushfstring(L,
                            "raiseGlobalEvent: bad argument type #%d (boolean, number, string or nil\n"
                            "expected, got a %s!)",
                            i,
                            luaL_typename(L, i));
            return lua_error(L);
        }
    }

    event.mArgumentList.append(host.getName());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    mudlet::self()->getHostManager().postInterHostEvent(&host, event);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#remainingTime
int TLuaInterpreter::remainingTime(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "remainingTime: bad argument #1 (timerID as number or timer name as string expected, got %s!", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    int result = -2;
    QString timerName;
    qint64 timerId = 0;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        // Is definitely a number and not a string that can be coerced into a number
        timerId = lua_tointeger(L, 1);
        result = host.getTimerUnit()->remainingTime(static_cast<int>(timerId));
    } else {
        timerName = lua_tostring(L, 1);
        result = host.getTimerUnit()->remainingTime(timerName);
    }

    if (result == -1) {
        return warnArgumentValue(L, __func__, "timer is inactive or expired");
    }

    if (result == -2) {
        if (timerName.isNull()) {
            // timerName was never set so we must have used the number
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid timerID").arg(timerId));
        }
        return warnArgumentValue(L, __func__, qsl("timer named '%1' not found").arg(timerName));
    }

    lua_pushnumber(L, result / 1000.0);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetProfileIcon
int TLuaInterpreter::resetProfileIcon(lua_State* L)
{
    Host& host = getHostFromLua(L);

    auto [success, message] = mudlet::self()->resetProfileIcon(host.getName());
    if (!success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetStopWatch
int TLuaInterpreter::resetStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "resetStopWatch: bad argument #1 type (stopwatchID as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    if (lua_type(L, 1) == LUA_TNUMBER) {
        QPair<bool, QString> const result = host.resetStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }

        lua_pushboolean(L, true);
        return 1;
    }

    QPair<bool, QString> const result = host.resetStopWatch(lua_tostring(L, 1));
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setButtonState
int TLuaInterpreter::setButtonState(lua_State* L)
{
    auto [retCount, pItem] = getTActionFromIdOrName(L, 1, __func__);
    if (retCount) {
        // pItem will be a nullptr if retCount is non-zero:
        return retCount;
    }

    auto checked = getVerifiedBool(L, __func__, 2, "checked");

    if (pItem->mButtonState != checked) {
        pItem->mButtonState = checked;
        if (pItem->mpEButton) {
            pItem->mpEButton->setChecked(checked);
        }
        if (pItem->mpFButton) {
            pItem->mpFButton->setChecked(checked);
        }
        lua_pushboolean(L, true);
        return 1;
    }

    // We only returned in the above (with a true value) if we changed the state:
    lua_pushboolean(L, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setConsoleBufferSize
int TLuaInterpreter::setConsoleBufferSize(lua_State* L)
{
    int s = 1;
    const int n = lua_gettop(L);
    QString windowName;
    if (n > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    auto linesLimit = getVerifiedInt(L, __func__, s++, "linesLimit");
    auto sizeOfBatchDeletion = getVerifiedInt(L, __func__, s++, "sizeOfBatchDeletion");
    
    // Optional fourth parameter: useMaximum (boolean)
    bool useMaximum = false;
    if (s <= n) {
        useMaximum = lua_toboolean(L, s);
    }

    // The macro will have returned with a nil + error message if the windowName
    // was not found:
    auto console = CONSOLE(L, windowName);
    Host& host = getHostFromLua(L);

    if (useMaximum) {
        // Maximum buffer size is only supported for the main console
        if (console != host.mpConsole) {
            return warnArgumentValue(L, __func__, "useMaximum parameter is only supported for the main console");
        }

        // Use system maximum buffer size instead of the provided linesLimit
        const int maxBufferSize = console->buffer.getMaxBufferSize();
        console->buffer.setBufferSize(maxBufferSize, sizeOfBatchDeletion);

        // Update Host settings
        host.setConsoleBufferSize(maxBufferSize);
        host.setUseMaxConsoleBufferSize(true);
    } else {
        console->buffer.setBufferSize(linesLimit, sizeOfBatchDeletion);

        // Update Host settings if this is the main console
        if (console == host.mpConsole) {
            host.setConsoleBufferSize(linesLimit);
            host.setUseMaxConsoleBufferSize(false);
        }
    }
    
    // Indicate success with a true return value:
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setProfileIcon
int TLuaInterpreter::setProfileIcon(lua_State* L)
{
    const QString iconPath = getVerifiedString(L, __func__, 1, "icon file path");
    if (iconPath.isEmpty()) {
        return warnArgumentValue(L, __func__, "a blank string is not a valid icon file path");
    }
    if (!QFileInfo::exists(iconPath)) {
        return warnArgumentValue(L, __func__, qsl("path '%1' doesn't exist").arg(iconPath));
    }

    Host& host = getHostFromLua(L);

    auto [success, message] = mudlet::self()->setProfileIcon(host.getName(), iconPath);
    if (!success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setScript
int TLuaInterpreter::setScript(lua_State* L)
{
    const int n = lua_gettop(L);
    int pos = 1;
    QString name = getVerifiedString(L, __func__, 1, "script name");

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(2); !validationResult) {
        lua_pushfstring(L, "setScript: bad argument #%d (%s)", 2, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    const QString luaCode{lua_tostring(L, 2)};

    if (n > 2) {
        pos = getVerifiedInt(L, __func__, 3, "script position");
    }

    auto [id, message] = pLuaInterpreter->setScriptCode(name, luaCode, --pos);
    if (id == -1) {
        lua_pushfstring(L, "setScript: cannot set script (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, id);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStopWatchName
int TLuaInterpreter::setStopWatchName(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "setStopWatchName: bad argument #1 type (stopwatchID as number or current name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    int watchId = 0;
    Host& host = getHostFromLua(L);
    QString currentName;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        watchId = static_cast<int>(lua_tointeger(L, 1));
    } else {
        // Using an empty string will return the first unnamed stopwatch:
        currentName = lua_tostring(L, 1);
    }

    const QString newName = getVerifiedString(L, __func__, 2, "stopwatch new name");

    QPair<bool, QString> result;
    if (currentName.isNull()) {
        // Will be null if no value was assigned to it - so use the id form:
        result = host.setStopWatchName(watchId, newName);
    } else {
        result = host.setStopWatchName(currentName, newName);
    }

    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStopWatchPersistence
int TLuaInterpreter::setStopWatchPersistence(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "setStopWatchPersistence: bad argument #1 type (stopwatchID as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    const bool isPersistent = getVerifiedBool(L, __func__, 2, "persistence");

    // This is only likely to fail when a numeric first argument was given:
    if (!host.makeStopWatchPersistent(watchId, isPersistent)) {
        return warnArgumentValue(L, __func__, csmInvalidStopWatchID.arg(watchId));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTriggerStayOpen
int TLuaInterpreter::setTriggerStayOpen(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    const double b = getVerifiedDouble(L, __func__, s, "number of lines");
    Host& host = getHostFromLua(L);
    host.getTriggerUnit()->setTriggerStayOpen(windowName, static_cast<int>(b));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#startStopWatch
int TLuaInterpreter::startStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "startStopWatch: bad argument #1 type (stopwatchID as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    if (lua_type(L, 1) == LUA_TNUMBER) {
        // Flag (if true) to replicate previous (reset and start again from zero
        // if call is repeated without any other actions being carried out on
        // stopwatch) behaviour if only a single NUMERIC argument (ID) supplied:
        bool autoResetAndRestart = true;
        if (lua_gettop(L) > 1) {
            autoResetAndRestart = getVerifiedBool(L, __func__, 2, "automatic reset and restart with a numeric stopwatchID", true);
        }

        QPair<bool, QString> result;
        if (autoResetAndRestart) {
            result = host.resetAndRestartStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        } else {
            result = host.startStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        }
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }

        lua_pushboolean(L, true);
        return 1;
    }

    QPair<bool, QString> const result = host.startStopWatch(lua_tostring(L, 1));
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopStopWatch
int TLuaInterpreter::stopStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "stopStopWatch: bad argument #1 type (stopwatchID as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    int watchId = 0;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        watchId = static_cast<int>(lua_tointeger(L, 1));
        QPair<bool, QString> const result = host.stopStopWatch(watchId);
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }

    } else {
        const QString name{lua_tostring(L, 1)};
        QPair<bool, QString> const result = host.stopStopWatch(name);
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }

        watchId = host.findStopWatchId(name);
        // We have already validated the name to get the watchId - so for things
        // to fail now is, unlikely?
        if (Q_UNLIKELY(!watchId)) {
            return warnArgumentValue(L, __func__, qsl(
                "stopwatch with name '%1' (ID: %2) has disappeared - this should not happen, please report it to Mudlet developers")
                .arg(name, QString::number(watchId)));
        }
    }

    // We know that this watchId is valid so can use the return value directly
    // as we want to emulate the past behaviour where stopping the stopWatch
    // returned the elapsed time ONCE:
    lua_pushnumber(L, host.getStopWatchTime(watchId).second);
    return 1;
}

// Note that this function has four arguments, of which the *second* may be omitted. :-/
int TLuaInterpreter::tempAnsiColorTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    QString code;
    int ansiFgColor = TTrigger::scmIgnored;
    int ansiBgColor = TTrigger::scmIgnored;
    int s = 0;

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d type (foreground color as ANSI Color number {%d = ignore foreground color, %d = default color, 0 to 255 ANSI color} expected, got %s!)",
                        s, TTrigger::scmIgnored, TTrigger::scmDefault, luaL_typename(L, s));
        return lua_error(L);
    }
    {   // separate block so that "value" is not scoped to the whole function
        const int value = lua_tointeger(L, s);
        if (value == TTrigger::scmIgnored && lua_gettop(L) < 2) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, it cannot be used (to ignore the foreground color) if the background color is omitted")
                .arg(value));
        }
        // At present we limit the range to (Trigger::scmIgnored),
        // (Trigger::scmDefault) and 0-255 ANSI colors - in the future we could
        // extend it to other "coded" values for locally generated textual
        // content
        if (!(value == TTrigger::scmIgnored || value == TTrigger::scmDefault || (value >= 0 && value <= 255))) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, only %2 (ignore foreground color), %3 (default foregroud color) or 0 to 255 recognised")
                .arg(QString::number(value), QString::number(TTrigger::scmIgnored), QString::number(TTrigger::scmDefault)));
        }
        if (value == TTrigger::scmIgnored && lua_gettop(L) < 4) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, you cannot ignore both foreground and background color (omitted)").arg(value));
        }
        ansiFgColor = value;
    }

    // s=1 at this point. If top=4 the next argument must be the BG color number,
    // otherwise it may have been omitted.
    if (lua_gettop(L) < s+3 && !lua_isnumber(L, s+1)) {
        // BG color omitted, skip this part
    } else if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d type (background color as ANSI Color number {%d = ignore foreground color, %d = default color, 0 to 255 ANSI color} expected, got %s!)",
                        s, TTrigger::scmIgnored, TTrigger::scmDefault, luaL_typename(L, s));
        return lua_error(L);
    } else {
        const int value = lua_tointeger(L, s);
        if (!(value == TTrigger::scmIgnored || value == TTrigger::scmDefault || (value >= 0 && value <= 255))) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, only %2 (ignore background color), %3 (default background color) or 0 to 255 recognised")
                .arg(QString::number(value), QString::number(TTrigger::scmIgnored), QString::number(TTrigger::scmDefault)));
        } else if (value == TTrigger::scmIgnored && ansiFgColor == TTrigger::scmIgnored) {
                return warnArgumentValue(L, __func__, qsl(
                    "invalid ANSI color number %1, you cannot ignore both foreground and background color")
                    .arg(value));
        } else {
            ansiBgColor = value;
        }
    }

    if (lua_isstring(L, ++s)) {
        code = QString::fromUtf8(lua_tostring(L, s));
    } else if (lua_isfunction(L, s)) {
        // leave code as a null QString(), see below
    } else {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d type (code to run as a string or a function expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    int expiryCount = -1;
    if (lua_isnumber(L, ++s)) {
        expiryCount = lua_tonumber(L, s);
        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, ++s)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d value (trigger expiration count must be a number, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    const int triggerID = pLuaInterpreter->startTempColorTrigger(ansiFgColor, ansiBgColor, code, expiryCount);
    if (code.isNull()) {
        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, s-1);
        lua_settable(L, LUA_REGISTRYINDEX);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempAlias
int TLuaInterpreter::tempAlias(lua_State* L)
{
    const QString regex = getVerifiedString(L, __func__, 1, "regex-type pattern");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    if (lua_isfunction(L, 2)) {

        const int result = pLuaInterpreter->startTempAlias(regex, QString());
        if (result == -1) {
            lua_pushnumber(L, -1);
            return 2;
        }

        TAlias* alias = host.getAliasUnit()->getAlias(result);
        Q_ASSERT_X(alias,
                   "TLuaInterpreter::tempAlias(...)",
                   "Got a positive result from LuaInterpreter::startTempAlias(...) but that failed to produce pointer to it from Host::mAliasUnit::getAlias(...)");
        alias->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, alias);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
        lua_pushnumber(L, result);
        return 1;
    }

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "tempAlias: bad argument #2 type (lua script as string or function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    const QString script{lua_tostring(L, 2)};

    lua_pushnumber(L, pLuaInterpreter->startTempAlias(regex, script));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempBeginOfLineTrigger
int TLuaInterpreter::tempBeginOfLineTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    int expiryCount = -1;
    const QString pattern = getVerifiedString(L, __func__, 1, "pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempBeginOfLineTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempBeginOfLineTrigger(pattern, QString(lua_tostring(L, 2)), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempBeginOfLineTrigger(pattern, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempBeginOfLineTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempButton
int TLuaInterpreter::tempButton(lua_State* L)
{
    //args: parent, name, orientation
    const QString cmdButtonUp = "";
    const QString cmdButtonDown = "";
    const QString script = "";
    QString toolbar;
    QStringList nameL;
    nameL << toolbar;

    toolbar = getVerifiedString(L, __func__, 1, "toolbar name");
    const QString name = getVerifiedString(L, __func__, 2, "button text");
    const int orientation = getVerifiedInt(L, __func__, 3, "orientation");

    Host& host = getHostFromLua(L);
    TAction* pP = host.getActionUnit()->findAction(toolbar);
    if (!pP) {
        return 0;
    }
    TAction* pT = host.getActionUnit()->findAction(name);
    if (pT) {
        return 0;
    }
    pT = new TAction(pP, &host);
    pT->setName(name);
    pT->setCommandButtonUp(cmdButtonUp);
    pT->setCommandButtonDown(cmdButtonDown);
    pT->setIsPushDownButton(false);
    pT->mLocation = pP->mLocation;
    pT->mOrientation = orientation;
    pT->setScript(script);
    pT->setIsFolder(false);
    pT->setIsActive(true);


    //    pT->setIsPushDownButton( isChecked );
    //    pT->mLocation = location;
    //    pT->mOrientation = orientation;
    //    pT->setIsActive( pT->shouldBeActive() );
    //    pT->setButtonColor( color );
    //    pT->setButtonRotation( rotation );
    //    pT->setButtonColumns( columns );
    ////      pT->setButtonFlat( flatButton );
    //    pT->mUseCustomLayout = useCustomLayout;
    //    pT->mPosX = posX;
    //    pT->mPosY = posY;
    //    pT->mSizeX = sizeX;
    //    pT->mSizeY = sizeY;
    //    pT->css = mpActionsMainArea->css->toPlainText();


    pT->registerAction();
    // N/U:     int childID = pT->getID();
    host.getActionUnit()->updateToolbar();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempButtonToolbar
int TLuaInterpreter::tempButtonToolbar(lua_State* L)
{
    QString name;
    const QString cmdButtonUp = "";
    const QString cmdButtonDown = "";
    const QString script = "";
    QStringList nameL;
    nameL << name;

    name = getVerifiedString(L, __func__, 1, "name");
    int location = getVerifiedInt(L, __func__, 2, "location");
    const int orientation = getVerifiedInt(L, __func__, 3, "orientation");

    if (location > 0) {
        location++;
    }
    Host& host = getHostFromLua(L);
    TAction* pT = host.getActionUnit()->findAction(name);
    if (pT) {
        return 0;
    }

    //insert a new root item
    //ROOT_ACTION:

    pT = new TAction(name, &host);
    pT->setCommandButtonUp(cmdButtonUp);
    QStringList nl;
    nl << name;

    pT->setName(name);
    pT->setCommandButtonUp(cmdButtonUp);
    pT->setCommandButtonDown(cmdButtonDown);
    pT->setIsPushDownButton(false);
    pT->mLocation = location;
    pT->mOrientation = orientation;
    pT->setScript(script);
    pT->setIsFolder(true);
    pT->setIsActive(true);
    pT->registerAction();
    // N/U:     int childID = pT->getID();
    host.getActionUnit()->updateToolbar();


    return 1;
}

// match ANSI numbering and fixing that would break existing scripts.
int TLuaInterpreter::tempColorTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int value = getVerifiedInt(L, __func__, 1, "foreground color");

    // match ANSI numbering and fixing that would break existing scripts so it has
    // to be tweaked here (and in the Mudlet XML save file format!)
    int foregroundColor = TTrigger::scmIgnored;
    // clang-format off
    switch (value) {
    case 0:     foregroundColor = TTrigger::scmDefault;  break; // Default foreground colour
    case 1:     foregroundColor =      8;   break; // light black (dark gray)
    case 2:     foregroundColor =      0;   break; // black
    case 3:     foregroundColor =      9;   break; // light red
    case 4:     foregroundColor =      1;   break; // red
    case 5:     foregroundColor =     10;   break; // light green
    case 6:     foregroundColor =      2;   break; // green
    case 7:     foregroundColor =     11;   break; // light yellow
    case 8:     foregroundColor =      3;   break; // yellow
    case 9:     foregroundColor =     12;   break; // light blue
    case 10:    foregroundColor =      4;   break; // blue
    case 11:    foregroundColor =     13;   break; // light magenta
    case 12:    foregroundColor =      5;   break; // magenta
    case 13:    foregroundColor =     14;   break; // light cyan
    case 14:    foregroundColor =      6;   break; // cyan
    case 15:    foregroundColor =     15;   break; // light white
    case 16:    foregroundColor =      7;   break; // white (light gray)
    // The default includes case -1:    foregroundColor = TTrigger::scmIgnored
    // which means only consider the background color now (and that cannot be
    // set to this value) - NOTE: TTrigger::scmIgnored has been set to BE -1
    // when it was added after Mudlet 3.7.1 but if that is subsequently changed
    // it will break the API for this lua function
    // other colours in ANSI 256 colours handled but not mentioned in Wiki
    default:    foregroundColor =  value;   break;
    // clang-format on
    }

    value = getVerifiedInt(L, __func__, 2, "background color");
    int backgroundColor = TTrigger::scmIgnored;
    // clang-format off
    switch (value) {
    case 0:     backgroundColor = TTrigger::scmDefault;  break; // Default background colour
    case 1:     backgroundColor =      8;   break; // light black (dark gray)
    case 2:     backgroundColor =      0;   break; // black
    case 3:     backgroundColor =      9;   break; // light red
    case 4:     backgroundColor =      1;   break; // red
    case 5:     backgroundColor =     10;   break; // light green
    case 6:     backgroundColor =      2;   break; // green
    case 7:     backgroundColor =     11;   break; // light yellow
    case 8:     backgroundColor =      3;   break; // yellow
    case 9:     backgroundColor =     12;   break; // light blue
    case 10:    backgroundColor =      4;   break; // blue
    case 11:    backgroundColor =     13;   break; // light magenta
    case 12:    backgroundColor =      5;   break; // magenta
    case 13:    backgroundColor =     14;   break; // light cyan
    case 14:    backgroundColor =      6;   break; // cyan
    case 15:    backgroundColor =     15;   break; // light white
    case 16:    backgroundColor =      7;   break; // white (light gray)
    // The default includes case -1:    backgroundColor = TTrigger::scmIgnored
    // but this cannot be used for the foreground case at the same time:
    default:    backgroundColor =  value;   break;
    // clang-format on
    }

    if (foregroundColor == TTrigger::scmIgnored && backgroundColor == TTrigger::scmIgnored) {
        return warnArgumentValue(L, __func__, "only one of foreground and background colors can be -1 (ignored)");
    }

    int triggerID;
    int expiryCount = -1;

    if (lua_isnumber(L, 4)) {
        expiryCount = lua_tonumber(L, 4);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 4)) {
        lua_pushfstring(L, "tempColorTrigger: bad argument #4 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    }

    if (lua_isstring(L, 3)) {
        triggerID = pLuaInterpreter->startTempColorTrigger(foregroundColor, backgroundColor, QString(lua_tostring(L, 3)), expiryCount);
    } else if (lua_isfunction(L, 3)) {
        triggerID = pLuaInterpreter->startTempColorTrigger(foregroundColor, backgroundColor, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempColorTrigger: bad argument #3 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempComplexRegexTrigger
int TLuaInterpreter::tempComplexRegexTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString triggerName = getVerifiedString(L, __func__, 1, "trigger name create or add to");
    const QString pattern = getVerifiedString(L, __func__, 2, "regex pattern to match");

    if (!lua_isstring(L, 3) && !lua_isfunction(L, 3)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #3 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (!lua_isnumber(L, 4)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #4 type (multiline flag as number expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    }
    const bool multiLine = lua_tonumber(L, 4);

    if (!lua_isnumber(L, 7)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #7 type (filter flag as number expected, got %s!)", luaL_typename(L, 7));
        return lua_error(L);
    }
    const bool filter = lua_tonumber(L, 7);

    if (!lua_isnumber(L, 8)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #8 type (match all flag as number expected, got %s!)", luaL_typename(L, 8));
        return lua_error(L);
    }
    const bool matchAll = lua_tonumber(L, 8);

    const int fireLength = getVerifiedInt(L, __func__, 12, "fire length");
    const int lineDelta = getVerifiedInt(L, __func__, 13, "line delta");

    bool colorTrigger;
    QString fgColor;
    if (lua_isnumber(L, 5)) {
        colorTrigger = false;
    } else {
        colorTrigger = true;
        fgColor = lua_tostring(L, 5);
    }

    QString bgColor;
    if (lua_isnumber(L, 6)) {
        colorTrigger = false;
    } else {
        bgColor = lua_tostring(L, 6);
    }

    bool highlight;
    QColor hlFgColor;
    if (lua_isnumber(L, 9)) {
        highlight = false;
    } else {
        highlight = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        hlFgColor.setNamedColor(lua_tostring(L, 9));
#else
        hlFgColor = QColor::fromString(lua_tostring(L, 9));
#endif
    }
    QColor hlBgColor;
    if (lua_isnumber(L, 10)) {
        highlight = false;
    } else {
        highlight = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        hlBgColor.setNamedColor(lua_tostring(L, 10));
#else
        hlBgColor = QColor::fromString(lua_tostring(L, 10));
#endif
    }

    QString soundFile;
    bool playSound;
    if (lua_type(L, 11) == LUA_TSTRING) {
        playSound = true;
        soundFile = lua_tostring(L, 11);
    } else {
        playSound = false;
    }

    int expiryCount = -1;

    if (lua_isnumber(L, 14)) {
        expiryCount = lua_tonumber(L, 14);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 14)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #14 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 14));
        return lua_error(L);
    }

    QStringList patterns;
    QList<int> propertyList;
    TTrigger* pP = host.getTriggerUnit()->findTrigger(triggerName);
    if (pP) {
        patterns = pP->getPatternsList();
        propertyList = pP->getRegexCodePropertyList();
    }
    patterns << pattern;
    if (colorTrigger) {
        propertyList << REGEX_COLOR_PATTERN;
    } else {
        propertyList << REGEX_PERL;
    }

    auto pT = new TTrigger("a", patterns, propertyList, multiLine, &host);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setName(triggerName);
    pT->mPerlSlashGOption = matchAll; //match all
    pT->mFilterTrigger = filter;
    pT->setConditionLineDelta(lineDelta); //line delta
    pT->mStayOpen = fireLength;           //fire length
    pT->mSoundTrigger = playSound;        //sound trigger, need to set sound file if true
    if (playSound) {
        pT->setSound(soundFile);
    }
    pT->setIsColorizerTrigger(highlight); //highlight
    pT->setExpiryCount(expiryCount);
    if (highlight) {
        pT->setColorizerFgColor(hlFgColor);
        pT->setColorizerBgColor(hlBgColor);
    }

    if (lua_isstring(L, 3)) {
        pT->setScript(lua_tostring(L, 3));
    } else if (lua_isfunction(L, 3)) {
        pT->setScript(QString());

        pT->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, pT);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    }

    lua_pushnumber(L, pT->getID());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempExactMatchTrigger
int TLuaInterpreter::tempExactMatchTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    int expiryCount = -1;
    const QString exactMatchPattern = getVerifiedString(L, __func__, 1, "exact match pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempExactMatchTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempExactMatchTrigger(exactMatchPattern, QString(lua_tostring(L, 2)), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempExactMatchTrigger(exactMatchPattern, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempExactMatchTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempKey
int TLuaInterpreter::tempKey(lua_State* L)
{
    uint_fast8_t argIndex = 1;
    int keyModifier = Qt::NoModifier;
    if (lua_gettop(L) > 2) {
        keyModifier = getVerifiedInt(L, __func__, 1, "key modifier", true);
        argIndex++;
    }
    int keyCode = getVerifiedInt(L, __func__, argIndex, "key code");

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    if (lua_isfunction(L, ++argIndex)) {

        const int result = pLuaInterpreter->startTempKey(keyModifier, keyCode, QString());
        if (result == -1) {
            lua_pushnumber(L, -1);
            return 2;
        }

        TKey* key = host.getKeyUnit()->getKey(result);
        Q_ASSERT_X(key,
                   "TLuaInterpreter::tempKey(...)",
                   "Got a positive result from LuaInterpreter::startTempKey(...) but that failed to produce pointer to it from Host::mKeyUnit::getKey(...)");
        key->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, key);
        lua_pushvalue(L, argIndex);
        lua_settable(L, LUA_REGISTRYINDEX);
        lua_pushnumber(L, result);
        return 1;
    }

    if (!lua_isstring(L, argIndex)) {
        lua_pushfstring(L, "tempKey: bad argument #%d type (lua script as string or function expected, got %s!)", argIndex, luaL_typename(L, argIndex));
        return lua_error(L);
    }
    const QString luaFunction{lua_tostring(L, argIndex)};

    const int timerID = pLuaInterpreter->startTempKey(keyModifier, keyCode, luaFunction);
    lua_pushnumber(L, timerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempLineTrigger
int TLuaInterpreter::tempLineTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    const int from = getVerifiedInt(L, __func__, 1, "line to start matching from");
    const int howMany  = getVerifiedInt(L, __func__, 2, "how many lines to match for");
    int triggerID;
    // temp line triggers expire naturally on their own, thus don't need the expiry mechanism applicable to all other triggers
    const int dontExpire = -1;

    if (lua_isstring(L, 3)) {
        triggerID = pLuaInterpreter->startTempLineTrigger(from, howMany, QString(lua_tostring(L, 3)), dontExpire);
    } else if (lua_isfunction(L, 3)) {
        triggerID = pLuaInterpreter->startTempLineTrigger(from, howMany, QString(), dontExpire);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempLineTrigger: bad argument #3 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempPromptTrigger
int TLuaInterpreter::tempPromptTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    int triggerID;
    int expiryCount = -1;

    if (lua_isnumber(L, 2)) {
        expiryCount = lua_tonumber(L, 2);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 2)) {
        lua_pushfstring(L, "tempPromptTrigger: bad argument #2 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    if (lua_isstring(L, 1)) {
        triggerID = pLuaInterpreter->startTempPromptTrigger(QString(lua_tostring(L, 1)), expiryCount);
    } else if (lua_isfunction(L, 1)) {
        triggerID = pLuaInterpreter->startTempPromptTrigger(QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 1);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempPromptTrigger: bad argument #1 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempRegexTrigger
int TLuaInterpreter::tempRegexTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    int expiryCount = -1;
    const QString regexPattern = getVerifiedString(L, __func__, 1, "regex pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempRegexTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempRegexTrigger(regexPattern, lua_tostring(L, 2), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempRegexTrigger(regexPattern, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempRegexTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempTimer
int TLuaInterpreter::tempTimer(lua_State* L)
{
    bool repeating{};
    const double time = getVerifiedDouble(L, __func__, 1, "time in seconds {maybe decimal}");
    const int n = lua_gettop(L);

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (lua_isfunction(L, 2)) {
        if (n > 2) {
            repeating = getVerifiedBool(L, __func__, 3, "repeating", true);
        }
        QPair<int, QString> const result = pLuaInterpreter->startTempTimer(time, QString(), repeating);
        if (result.first == -1) {
            lua_pushnumber(L, -1);
            lua_pushstring(L, result.second.toUtf8().constData());
            return 2;
        }

        TTimer* timer = host.getTimerUnit()->getTimer(result.first);
        Q_ASSERT_X(timer,
                   "TLuaInterpreter::tempTimer(...)",
                   "Got a positive result from LuaInterpreter::startTempTimer(...) but that failed to produce pointer to it from Host::mTimerUnit::getTimer(...)");
        timer->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, timer);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
        lua_pushnumber(L, result.first);
        return 1;
    }

    const QString luaCode = getVerifiedString(L, __func__, 2, "script or function name");
    if (n > 2) {
        repeating = getVerifiedBool(L, __func__, 3, "repeating", true);
    }
    QPair<int, QString> const result = pLuaInterpreter->startTempTimer(time, luaCode, repeating);
    lua_pushnumber(L, result.first);
    if (result.first == -1) {
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempTrigger
int TLuaInterpreter::tempTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    int expiryCount = -1;
    const QString substringPattern = getVerifiedString(L, __func__, 1, "substring pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempTrigger(substringPattern, QString(lua_tostring(L, 2)), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempTrigger(substringPattern, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getProfiles
int TLuaInterpreter::getProfiles(lua_State* L)
{
    auto& hostManager = mudlet::self()->getHostManager();
    const QStringList profiles = QDir(mudlet::getMudletPath(enums::profilesPath))
                                   .entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    lua_newtable(L);

    for (const auto& profile : profiles) {
        lua_pushstring(L, profile.toUtf8().constData());
        lua_newtable(L);

        QString url = mudlet::self()->readProfileData(profile, qsl("url"));
        QString port = mudlet::self()->readProfileData(profile, qsl("port"));
        QString description = mudlet::self()->readProfileData(profile, qsl("description"));

        // if url/port haven't been written to disk yet (which is what happens
        // when a default profile is opened for the first time), fetch this data from game details
        if (url.isEmpty()) {
            auto it = TGameDetails::findGame(profile);
            if (it != TGameDetails::scmDefaultGames.end()) {
                url = (*it).hostUrl;
            }
        }
        if (port.isEmpty()) {
            auto it = TGameDetails::findGame(profile);
            if (it != TGameDetails::scmDefaultGames.end()) {
                port = QString::number((*it).port);
            }
        }

        if (!url.isEmpty()) {
            lua_pushstring(L, "host");
            lua_pushstring(L, url.toUtf8().constData());
            lua_settable(L, -3);
        }

        if (!port.isEmpty()) {
            lua_pushstring(L, "port");
            lua_pushstring(L, port.toUtf8().constData());
            lua_settable(L, -3);
        }

        lua_pushstring(L, "description");
        lua_pushstring(L, description.toUtf8().constData());
        lua_settable(L, -3);


        auto host = hostManager.getHost(profile);
        const auto loaded = static_cast<bool>(host);
        lua_pushstring(L, "loaded");
        lua_pushboolean(L, loaded);
        lua_settable(L, -3);

        if (loaded) {
            auto [hostName, hostPort, connected] = host->mTelnet.getConnectionInfo();

            lua_pushstring(L, "connected");
            lua_pushboolean(L, connected);
            lua_settable(L, -3);
        }

        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadProfile
int TLuaInterpreter::loadProfile(lua_State* L)
{
    auto& hostManager = mudlet::self()->getHostManager();
    const QString profileName = getVerifiedString(L, __func__, 1, "profile name");
    bool offline = false;

    if (lua_gettop(L) > 1) {
        offline = getVerifiedBool(L, __func__, 2, "offline mode", true);
    }

    if (profileName.isEmpty()) {
        lua_pushnil(L);
        lua_pushstring(L, "loadProfile: profile name cannot be empty");
        return 2;
    }

    if (!mudlet::self()->profileExists(profileName)) {
        lua_pushnil(L);
        lua_pushfstring(L, "loadProfile: profile '%s' does not exist", profileName.toUtf8().constData());
        return 2;
    }

    if (hostManager.hostLoaded(profileName)) {
        lua_pushnil(L);
        lua_pushfstring(L, "loadProfile: profile '%s' is already loaded", profileName.toUtf8().constData());
        return 2;
    }

    bool success = mudlet::self()->loadProfile(profileName, !offline);
    mudlet::self()->slot_connectionDialogueFinished(profileName, !offline);
    mudlet::self()->enableToolbarButtons();

    if (!success) {
        lua_pushnil(L);
        lua_pushfstring(L, "loadProfile: failed to load profile '%s'", profileName.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#closeProfile
int TLuaInterpreter::closeProfile(lua_State* L)
{
    auto& hostManager = mudlet::self()->getHostManager();
    QString profileName;

    if (lua_gettop(L) == 0) {
        Host& host = getHostFromLua(L);
        profileName = host.getName();
    } else {
        profileName = getVerifiedString(L, __func__, 1, "profile name");
    }

    if (!mudlet::self()->profileExists(profileName)) {
        lua_pushnil(L);
        lua_pushfstring(L, "closeProfile: profile '%s' does not exist", profileName.toUtf8().constData());
        return 2;
    }

    if (!hostManager.hostLoaded(profileName)) {
        lua_pushnil(L);
        lua_pushfstring(L, "closeProfile: profile '%s' is not loaded", profileName.toUtf8().constData());
        return 2;
    }

    auto profileIndex = mudlet::self()->mpTabBar->tabIndex(profileName);
    if (profileIndex != -1) {
        emit mudlet::self()->mpTabBar->tabCloseRequested(profileIndex);
        lua_pushboolean(L, true);
        return 1;
    }
    return 0;
}
