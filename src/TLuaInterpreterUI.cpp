/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2022 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2016 by Eric Wallace - eewallace@gmail.com              *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2022-2023 by Lecker Kebap - Leris@mudlet.org            *
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

// UI-specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

#include "TLuaInterpreter.h"

#include <QClipboard>
#include <QGuiApplication>

#include "EAction.h"
#include "Host.h"
#include "TArea.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TTabBar.h"
#include "TTextBox.h"
#include "TTextEdit.h"
#include "TTimer.h"
#include "dlgComposer.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgTriggerEditor.h"
#include "mapInfoContributorManager.h"
#include "mudlet.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget_integration.h"
#endif

#include <array>
#include <limits>
#include <math.h>

#include <QCollator>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMovie>
#include <QVector>
#ifdef QT_TEXTTOSPEECH_LIB
#include <QTextToSpeech>
#endif // QT_TEXTTOSPEECH_LIB

static const char* bad_window_type = "%s: bad argument #%d type (window name as string expected, got %s)!";
static const char* bad_cmdline_type = "%s: bad argument #%d type (command line name as string expected, got %s)!";
static const char* bad_window_value = "window \"%s\" not found";
static const char* bad_cmdline_value = "command line \"%s\" not found";
static const char* bad_label_value = "label \"%s\" not found";
// A Host outlives its main console: closing a profile's window destroys the view
// while triggers, the buffer, logging and Lua all keep running. Whatever only a
// widget can answer has to report this rather than dereference what is gone.
static const char* no_main_window_value = "the profile has no main window";

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

#define WINDOW_NAME(ARG_L, ARG_pos)                                                                                                                                                                    \
    ({                                                                                                                                                                                                 \
        int pos_ = (ARG_pos);                                                                                                                                                                          \
        const char* res_;                                                                                                                                                                              \
        if ((lua_gettop(ARG_L) < pos_) || lua_isnil(ARG_L, pos_)) {                                                                                                                                    \
            res_ = "";                                                                                                                                                                                 \
        } else {                                                                                                                                                                                       \
            if (!lua_isstring(ARG_L, pos_)) {                                                                                                                                                          \
                lua_pushfstring(ARG_L, bad_window_type, __FUNCTION__, pos_, luaL_typename(ARG_L, pos_));                                                                                               \
                return lua_error(ARG_L);                                                                                                                                                               \
            }                                                                                                                                                                                          \
            res_ = lua_tostring(ARG_L, pos_);                                                                                                                                                          \
        }                                                                                                                                                                                              \
        res_;                                                                                                                                                                                          \
    })

#define CMDLINE_NAME(ARG_L, ARG_pos)                                                                                                                                                                   \
    ({                                                                                                                                                                                                 \
        int pos_ = (ARG_pos);                                                                                                                                                                          \
        if (!lua_isstring(ARG_L, pos_)) {                                                                                                                                                              \
            lua_pushfstring(ARG_L, bad_cmdline_type, __FUNCTION__, pos_, luaL_typename(ARG_L, pos_));                                                                                                  \
            return lua_error(ARG_L);                                                                                                                                                                   \
        }                                                                                                                                                                                              \
        lua_tostring(ARG_L, pos_);                                                                                                                                                                     \
    })

#define CONSOLE_NIL(ARG_L, ARG_name)                                                                                                                                                                   \
    ({                                                                                                                                                                                                 \
        auto name_ = (ARG_name);                                                                                                                                                                       \
        auto console_ = getHostFromLua(ARG_L).findConsole(name_);                                                                                                                                      \
        console_;                                                                                                                                                                                      \
    })

#define CONSOLE(ARG_L, ARG_name)                                                                                                                                                                       \
    ({                                                                                                                                                                                                 \
        auto name_ = (ARG_name);                                                                                                                                                                       \
        auto console_ = getHostFromLua(ARG_L).findConsole(name_);                                                                                                                                      \
        if (!console_) {                                                                                                                                                                               \
            lua_pushnil(ARG_L);                                                                                                                                                                        \
            lua_pushfstring(ARG_L, bad_window_value, name_.toUtf8().constData());                                                                                                                      \
            return 2;                                                                                                                                                                                  \
        }                                                                                                                                                                                              \
        console_;                                                                                                                                                                                      \
    })

#define COMMANDLINE(ARG_L, ARG_name)                                                                                                                                                                   \
    ({                                                                                                                                                                                                 \
        const QString& name_ = (ARG_name);                                                                                                                                                             \
        auto console_ = getHostFromLua(ARG_L).mpConsole;                                                                                                                                               \
        auto cmdLine_ = !console_ ? nullptr : (isMain(name_) ? &*console_->mpCommandLine : console_->mSubCommandLineMap.value(name_));                                                                 \
        if (!cmdLine_) {                                                                                                                                                                               \
            lua_pushnil(ARG_L);                                                                                                                                                                        \
            lua_pushfstring(ARG_L, bad_cmdline_value, name_.toUtf8().constData());                                                                                                                     \
            return 2;                                                                                                                                                                                  \
        }                                                                                                                                                                                              \
        cmdLine_;                                                                                                                                                                                      \
    })

#define LABEL(ARG_L, ARG_name)                                                                                                                                                                         \
    ({                                                                                                                                                                                                 \
        const QString& name_ = (ARG_name);                                                                                                                                                             \
        auto console_ = getHostFromLua(ARG_L).mpConsole;                                                                                                                                               \
        auto label_ = console_ ? console_->mLabelMap.value(name_) : nullptr;                                                                                                                           \
        if (!label_) {                                                                                                                                                                                 \
            lua_pushnil(ARG_L);                                                                                                                                                                        \
            lua_pushfstring(ARG_L, bad_label_value, name_.toUtf8().constData());                                                                                                                       \
            return 2;                                                                                                                                                                                  \
        }                                                                                                                                                                                              \
        label_;                                                                                                                                                                                        \
    })

// Parsing a command or a commands table anchors each function it holds in the
// Lua registry, so a call that goes on to fail has to let those references go
// again. CONSOLE() returns straight out of its caller, which is why every one of
// these functions resolves its window before it parses.
static void releaseLuaReferences(lua_State* L, const QVector<int>& luaReferences)
{
    for (const int luaReference : luaReferences) {
        // a string command takes no reference and is recorded as a zero, which
        // luaL_unref() would mistake for a registry slot of its own
        if (luaReference > 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, luaReference);
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCommandLineMenuEvent
int TLuaInterpreter::addCommandLineMenuEvent(lua_State* L)
{
    const int argsCount = lua_gettop(L);
    const bool hasCommandLineName = (argsCount >= 3);
    const int menuLabelPos = hasCommandLineName ? 2 : 1;

    if (hasCommandLineName && !checkStringArg(L, __func__, 1, "command line name")) {
        return lua_error(L);
    }
    if (!checkStringArg(L, __func__, menuLabelPos, "menu label") || !checkStringArg(L, __func__, menuLabelPos + 1, "event name")) {
        return lua_error(L);
    }

    const QString commandLineName = hasCommandLineName ? QString{lua_tostring(L, 1)} : qsl("main");
    const QString menuLabel{lua_tostring(L, menuLabelPos)};
    const QString eventName{lua_tostring(L, menuLabelPos + 1)};

    const auto& commandline = COMMANDLINE(L, commandLineName);
    commandline->contextMenuItems.insert(menuLabel, eventName);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMouseEvent
int TLuaInterpreter::addMouseEvent(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!checkStringArg(L, __func__, 1, "uniquename")) {
        return lua_error(L);
    }
    if (const QString uniqueName{lua_tostring(L, 1)}; host.mConsoleActions.contains(uniqueName)) {
        return warnArgumentValue(L, __func__, qsl("mouse event '%1' already exists").arg(uniqueName));
    }
    if (!checkStringArg(L, __func__, 2, "event name", false)) {
        return lua_error(L);
    }

    const QString uniqueName{lua_tostring(L, 1)};
    QStringList actionInfo;
    actionInfo << QString{lua_tostring(L, 2)};

    // Display name
    if (!lua_isstring(L, 3)) {
        actionInfo << uniqueName;
    } else {
        actionInfo << lua_tostring(L, 3);
    }

    // tooltip text
    if (!lua_isstring(L, 4)) {
        actionInfo << QString();
    } else {
        actionInfo << lua_tostring(L, 4);
    }

    host.mConsoleActions.insert(uniqueName, actionInfo);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#appendBuffer
int TLuaInterpreter::appendBuffer(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->appendBuffer();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#calcFontSize
int TLuaInterpreter::calcFontSize(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName = qsl("main");
    QSize size;

    // font name and size are passed in as arguments
    if (lua_gettop(L) == 2) {
        // hoisted because the order the two QFont arguments were evaluated in is
        // unspecified, so which failure got reported was up to the compiler
        if (!checkIntArg(L, __func__, 1, "font size") || !checkStringArg(L, __func__, 2, "font name")) {
            return lua_error(L);
        }
        auto font = QFont(QString{lua_tostring(L, 2)}, static_cast<int>(lua_tointeger(L, 1)), QFont::Normal);
        auto fontMetrics = QFontMetrics(font);
        size = QSize(fontMetrics.averageCharWidth(), fontMetrics.height());

        lua_pushnumber(L, size.width());
        lua_pushnumber(L, size.height());
        return 2;
    }

    // otherwise either window name or font size is passed in
    if (lua_gettop(L) == 1 && lua_isnumber(L, 1)) {
        auto fontSize = lua_tonumber(L, 1);
        auto font = QFont(qsl("Bitstream Vera Sans Mono"), fontSize, QFont::Normal);

        auto fontMetrics = QFontMetrics(font);
        size = QSize(fontMetrics.averageCharWidth(), fontMetrics.height());
    } else {
        windowName = WINDOW_NAME(L, 1);
        size = host.calcFontSize(windowName);
    }

    if (size.width() <= -1) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, size.width());
    lua_pushnumber(L, size.height());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearUserWindow
// Note that this is registered as both clearUserWindow(...) AND clearWindow(...)
int TLuaInterpreter::clearUserWindow(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L)) {
        windowName = getVerifiedString(L, __func__, 1, "window name", true);
    }

    Host& host = getHostFromLua(L);
    host.clearWindow(windowName);
    // Note that exceptionally THIS function does not return a true/nil+error
    // message on failure - because on success this could plonk a "true" on the
    // main screen if run from the command line - which sort of messes with the
    // idea of clearing it of text!
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#copy
int TLuaInterpreter::copy(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    console->copy();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createBuffer
int TLuaInterpreter::createBuffer(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    host.createBuffer(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createCommandLine
int TLuaInterpreter::createCommandLine(lua_State* L)
{
    const int n = lua_gettop(L);
    int counter = 1;
    const bool hasParentWindow = (n > 5);

    if (hasParentWindow) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "createCommandLine: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        counter++;
    }

    const int commandLineNamePos = counter++;
    if (lua_type(L, commandLineNamePos) != LUA_TSTRING) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandLine name as string expected, got %s!)", commandLineNamePos, luaL_typename(L, commandLineNamePos));
        return lua_error(L);
    }
    const int x = getVerifiedInt(L, __func__, counter, "commandline x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "commandline y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "commandline width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "commandline height");
    counter++;

    QString windowName = qsl("main");
    if (hasParentWindow) {
        windowName = lua_tostring(L, 1);
        if (isMain(windowName)) {
            // createCommandLine only accepts the empty name as the main window
            windowName.clear();
        }
    }
    const QString commandLineName{lua_tostring(L, commandLineNamePos)};

    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->createCommandLine(windowName, commandLineName, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createLabel
int TLuaInterpreter::createLabel(lua_State* L)
{
    if (lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "createLabel: bad argument #1 type (label or parent window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (lua_type(L, 2) == LUA_TSTRING) {
        return createLabelUserWindow(L, lua_tostring(L, 1), lua_tostring(L, 2));
    }
    if (lua_type(L, 2) == LUA_TNUMBER) {
        return createLabelMainWindow(L, lua_tostring(L, 1));
    }

    lua_pushfstring(L, "createLabel: bad argument #2 type (label name as string or label x-coordinate as number expected, got %s!)", luaL_typename(L, 2));
    return lua_error(L);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMiniConsole
int TLuaInterpreter::createMiniConsole(lua_State* L)
{
    int counter = 3;
    //make the windowname optional by using counter. If windowname "main" add to main console

    if (!checkStringArg(L, __func__, 1, "miniconsole name")) {
        return lua_error(L);
    }

    const bool hasParentWindow = (!lua_isnumber(L, 2) && lua_gettop(L) >= 2);
    if (hasParentWindow) {
        if (!checkStringArg(L, __func__, 2, "miniconsole name")) {
            return lua_error(L);
        }
    } else {
        counter = 2;
    }

    const int x = getVerifiedInt(L, __func__, counter, "miniconsole x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "miniconsole y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "miniconsole width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "miniconsole height");

    QString windowName;
    QString name;
    if (hasParentWindow) {
        windowName = lua_tostring(L, 1);
        if (isMain(windowName)) {
            // createMiniConsole only accepts the empty name as the main window
            windowName.clear();
        }
        name = lua_tostring(L, 2);
    } else {
        name = lua_tostring(L, 1);
        if (isMain(name)) {
            name.clear();
        }
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.createMiniConsole(windowName, name, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message, true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createScrollBox
int TLuaInterpreter::createScrollBox(lua_State* L)
{
    int counter = 3;
    // make the windowname optional by using counter. If windowname "main" - add to main console

    if (!checkStringArg(L, __func__, 1, "scrollBox name")) {
        return lua_error(L);
    }

    const bool hasParentWindow = (!lua_isnumber(L, 2) && lua_gettop(L) >= 2);
    if (hasParentWindow) {
        if (!checkStringArg(L, __func__, 2, "scrollBox name")) {
            return lua_error(L);
        }
    } else {
        counter = 2;
    }

    const int x = getVerifiedInt(L, __func__, counter, "scrollBox x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "scrollBox y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "scrollBox width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "scrollBox height");

    QString windowName;
    QString name;
    if (hasParentWindow) {
        windowName = lua_tostring(L, 1);
        if (isMain(windowName)) {
            // createScrollBox only accepts the empty name as the main window
            windowName.clear();
        }
        name = lua_tostring(L, 2);
    } else {
        name = lua_tostring(L, 1);
        if (isMain(name)) {
            name.clear();
        }
    }

    const Host& host = getHostFromLua(L);
    if (auto [success, message] = host.createScrollBox(windowName, name, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message, true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteLabel
int TLuaInterpreter::deleteLabel(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value, true);
    }
    if (auto [success, message] = host.mpConsole->deleteLabel(labelName); !success) {
        lua_pushboolean(L, false);
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteMiniConsole
int TLuaInterpreter::deleteMiniConsole(lua_State* L)
{
    const QString miniConsoleName = getVerifiedString(L, __func__, 1, "miniconsole name");
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value, true);
    }
    if (auto [success, message] = host.mpConsole->deleteMiniConsole(miniConsoleName); !success) {
        lua_pushboolean(L, false);
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteCommandLine
int TLuaInterpreter::deleteCommandLine(lua_State* L)
{
    const QString commandLineName = getVerifiedString(L, __func__, 1, "command line name");

    if (isMain(commandLineName)) {
        lua_pushboolean(L, false);
        lua_pushstring(L, "the main command line cannot be deleted");
        return 2;
    }

    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value, true);
    }
    if (auto [success, message] = host.mpConsole->deleteCommandLine(commandLineName); !success) {
        lua_pushboolean(L, false);
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createTextEdit
int TLuaInterpreter::createTextEdit(lua_State* L)
{
    const int n = lua_gettop(L);
    int counter = 1;
    const bool hasParentWindow = (n > 5);

    if (hasParentWindow) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "createTextEdit: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        counter++;
    }

    const int textEditNamePos = counter++;
    if (lua_type(L, textEditNamePos) != LUA_TSTRING) {
        lua_pushfstring(L, "createTextEdit: bad argument #%d type (text edit name as string expected, got %s!)", textEditNamePos, luaL_typename(L, textEditNamePos));
        return lua_error(L);
    }
    const int x = getVerifiedInt(L, __func__, counter, "text edit x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "text edit y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "text edit width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "text edit height");
    counter++;

    QString windowName = qsl("main");
    if (hasParentWindow) {
        windowName = lua_tostring(L, 1);
        if (isMain(windowName)) {
            // createTextEdit only accepts the empty name as the main window
            windowName.clear();
        }
    }
    const QString textEditName{lua_tostring(L, textEditNamePos)};

    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->createTextBox(windowName, textEditName, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteTextEdit
int TLuaInterpreter::deleteTextEdit(lua_State* L)
{
    const QString textEditName = getVerifiedString(L, __func__, 1, "text edit name");

    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value, true);
    }
    if (auto [success, message] = host.mpConsole->deleteTextBox(textEditName); !success) {
        lua_pushboolean(L, false);
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTextEditText
int TLuaInterpreter::getTextEditText(lua_State* L)
{
    const QString textEditName = getVerifiedString(L, __func__, 1, "text edit name");

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    lua_pushstring(L, pT->toPlainText().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextEditText
int TLuaInterpreter::setTextEditText(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "text edit name")) {
        return lua_error(L);
    }
    const QString text = getVerifiedString(L, __func__, 2, "text");
    const QString textEditName{lua_tostring(L, 1)};

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    pT->setPlainText(text);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearTextEdit
int TLuaInterpreter::clearTextEdit(lua_State* L)
{
    const QString textEditName = getVerifiedString(L, __func__, 1, "text edit name");

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    pT->clear();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextEditReadOnly
int TLuaInterpreter::setTextEditReadOnly(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "text edit name")) {
        return lua_error(L);
    }
    const bool readOnly = getVerifiedBool(L, __func__, 2, "read only state");
    const QString textEditName{lua_tostring(L, 1)};

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    pT->setReadOnly(readOnly);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextEditPlaceholder
int TLuaInterpreter::setTextEditPlaceholder(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "text edit name")) {
        return lua_error(L);
    }
    const QString placeholder = getVerifiedString(L, __func__, 2, "placeholder text");
    const QString textEditName{lua_tostring(L, 1)};

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    pT->setPlaceholderText(placeholder);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextEditStyleSheet
int TLuaInterpreter::setTextEditStyleSheet(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "text edit name")) {
        return lua_error(L);
    }
    const QString css = getVerifiedString(L, __func__, 2, "stylesheet");
    const QString textEditName{lua_tostring(L, 1)};

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    pT->setStyleSheet(css);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextEditFont
int TLuaInterpreter::setTextEditFont(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "text edit name")) {
        return lua_error(L);
    }
    const QString fontName = getVerifiedString(L, __func__, 2, "font name");
    const QString textEditName{lua_tostring(L, 1)};

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    QFont font = pT->font();
    // An unlisted name comes back from the resolution as it was given, and goes
    // through: the font database leaves out families the platform still resolves,
    // such as the fontconfig alias "Helvetica". The weight comes from the
    // resolution either way, so the bold of an earlier "Family Style" name is not
    // left behind on the next family.
    const auto resolved = host.resolveFontFamily(fontName);
    font.setFamily(resolved.family);
    font.setWeight(resolved.weight);
    pT->setFont(font);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextEditFontSize
int TLuaInterpreter::setTextEditFontSize(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "text edit name")) {
        return lua_error(L);
    }
    const int size = getVerifiedInt(L, __func__, 2, "font size");
    const QString textEditName{lua_tostring(L, 1)};

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    QFont font = pT->font();
    font.setPointSize(size);
    pT->setFont(font);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextEditTabMovesFocus
int TLuaInterpreter::setTextEditTabMovesFocus(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "text edit name")) {
        return lua_error(L);
    }
    const bool tabMovesFocus = getVerifiedBool(L, __func__, 2, "tab moves focus state");
    const QString textEditName{lua_tostring(L, 1)};

    const Host& host = getHostFromLua(L);
    auto pT = host.mpConsole ? host.mpConsole->mTextBoxMap.value(textEditName) : nullptr;
    if (!pT) {
        return warnArgumentValue(L, __func__, qsl("text edit name '%1' not found").arg(textEditName));
    }

    pT->setTabChangesFocus(tabMovesFocus);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteScrollBox
int TLuaInterpreter::deleteScrollBox(lua_State* L)
{
    const QString scrollBoxName = getVerifiedString(L, __func__, 1, "scrollbox name");
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value, true);
    }
    if (auto [success, message] = host.mpConsole->deleteScrollBox(scrollBoxName); !success) {
        lua_pushboolean(L, false);
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteLine
int TLuaInterpreter::deleteLine(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->skipLine();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deselect
int TLuaInterpreter::deselect(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->deselect();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableClickthrough
int TLuaInterpreter::disableClickthrough(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);

    host.setClickthrough(windowName, false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableCommandLine
int TLuaInterpreter::disableCommandLine(lua_State* L)
{
    const QString commandLineName{CMDLINE_NAME(L, 1)};
    if (isMain(commandLineName)) {
        return warnArgumentValue(L, __func__, "this function is not permitted on the main command line");
    }
    auto console = CONSOLE_NIL(L, commandLineName);
    if (console) {
        // This name matches a TConsole instance so we are referring to a
        // TCommandLine at the bottom of it - so need to call the original
        // function:
        console->setCmdVisible(false);
        lua_pushboolean(L, true);
        return 1;
    }

    // Else this might refer to an additional command line which must exist
    // for it to be shown by this function - the following macro will fail
    // (and return with a nil and an error message) if it doesn't:
    auto commandLine = COMMANDLINE(L, commandLineName);
    commandLine->setVisible(false);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableHorizontalScrollBar
int TLuaInterpreter::disableHorizontalScrollBar(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setHorizontalScrollBar(false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScrollBar
int TLuaInterpreter::disableScrollBar(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setScrollBarVisible(false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTimeStamps
int TLuaInterpreter::disableTimeStamps(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto pConsole = CONSOLE(L, windowName);
    // *pConsole can be the main console as well as any user one
    if (!pConsole->showTimeStamps()) {
        lua_pushnil(L);
        if (windowName.isEmpty()) {
            lua_pushstring(L, qsl("timestamps were not enabled for the main console").toUtf8().constData());
        } else {
            lua_pushstring(L, qsl("timestamps were not enabled for the \"%1\" console").arg(windowName).toUtf8().constData());
        }
        return 2;
    }

    pConsole->slot_toggleTimeStamps(false);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoLink
int TLuaInterpreter::echoLink(lua_State* L)
{
    const int n = lua_gettop(L);
    // with exactly four arguments the last one is either the format flag - and
    // then there is no window name - or the hint
    const bool hasWindowName = (n > 4) || (n == 4 && !lua_isboolean(L, 4));
    const bool hasFormatFlag = (n > 4) || (n == 4 && lua_isboolean(L, 4));

    int s = 0;
    const int windowNamePos = hasWindowName ? ++s : 0;
    if (hasWindowName && !checkStringArg(L, __func__, windowNamePos, "window name")) {
        return lua_error(L);
    }
    const int textPos = ++s;
    if (!checkStringArg(L, __func__, textPos, "text")) {
        return lua_error(L);
    }
    int commandPos = ++s;
    if (!checkCommandOrFunctionArg(L, __func__, commandPos)) {
        return lua_error(L);
    }
    const int hintPos = ++s;
    if (!checkStringArg(L, __func__, hintPos, "hint")) {
        return lua_error(L);
    }
    const int formatPos = hasFormatFlag ? ++s : 0;
    if (hasFormatFlag && !checkBoolArg(L, __func__, formatPos, "useCurrentFormat")) {
        return lua_error(L);
    }

    // resolved before the parse, so a miss strands nothing - see releaseLuaReferences()
    const QString windowName = hasWindowName ? QString{lua_tostring(L, windowNamePos)} : qsl("main");
    auto console = CONSOLE(L, windowName);

    QString command;
    int luaReference = 0;
    parseCommandOrFunction(L, __func__, commandPos, command, luaReference);

    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    commandList << command;
    luaReferences << luaReference;
    hintList << QString{lua_tostring(L, hintPos)};

    const bool useCurrentFormat = hasFormatFlag && lua_toboolean(L, formatPos);
    console->echoLink(QString{lua_tostring(L, textPos)}, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoUserWindow
int TLuaInterpreter::echoUserWindow(lua_State* L)
{
    const char* windowName = WINDOW_NAME(L, 1);
    const QString text = getVerifiedString(L, __func__, 2, "text");
    Host& host = getHostFromLua(L);
    host.echoWindow(QString{windowName}, text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoPopup
int TLuaInterpreter::echoPopup(lua_State* L)
{
    const int n = lua_gettop(L);
    // with exactly four arguments the last one is either the format flag - and
    // then there is no window name - or the hints table
    const bool hasWindowName = (n > 4) || (n == 4 && !lua_isboolean(L, 4));
    const bool hasFormatFlag = (n > 4) || (n == 4 && lua_isboolean(L, 4));

    int s = 0;
    const int windowNamePos = hasWindowName ? ++s : 0;
    if (hasWindowName && !checkStringArg(L, __func__, windowNamePos, "window name")) {
        return lua_error(L);
    }
    const int textPos = ++s;
    if (!checkStringArg(L, __func__, textPos, "text")) {
        return lua_error(L);
    }
    int commandPos = ++s;
    int hintPos = ++s;
    const int formatPos = hasFormatFlag ? ++s : 0;

    if (!checkCommandsOrFunctionsTable(L, __func__, commandPos) || !checkHintsTable(L, __func__, hintPos) || (hasFormatFlag && !checkBoolArg(L, __func__, formatPos, "useCurrentFormat"))) {
        return lua_error(L);
    }

    // resolved before the parse, so a miss strands nothing - see releaseLuaReferences()
    const QString windowName = hasWindowName ? QString{lua_tostring(L, windowNamePos)} : qsl("main");
    auto console = CONSOLE(L, windowName);

    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    parseCommandsOrFunctionsTable(L, __func__, commandPos, commandList, luaReferences);
    parseHintsTable(L, __func__, hintPos, hintList);

    if ((hintList.size() - commandList.size()) < 0 || (hintList.size() - commandList.size()) > 1) {
        releaseLuaReferences(L, luaReferences);
        lua_pushnil(L);
        lua_pushfstring(L,
                        "command table and hint table sizes do not match up (%d and %d, either they must be the same or there should be one extra hint) - cannot create popup",
                        commandList.size(),
                        hintList.size());
        return 2;
    }

    const bool useCurrentFormat = hasFormatFlag && lua_toboolean(L, formatPos);
    console->echoLink(QString{lua_tostring(L, textPos)}, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableClickthrough
int TLuaInterpreter::enableClickthrough(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);

    host.setClickthrough(windowName, true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLinkStyle
int TLuaInterpreter::setLinkStyle(lua_State* L)
{
    const bool hasUnderline = (lua_gettop(L) >= 4);
    if (!checkStringArg(L, __func__, 1, "label name") || !checkStringArg(L, __func__, 2, "link color", true) || !checkStringArg(L, __func__, 3, "link visited color", true)) {
        return lua_error(L);
    }
    if (hasUnderline && !checkBoolArg(L, __func__, 4, "underline", true)) {
        return lua_error(L);
    }

    const QString labelName{lua_tostring(L, 1)};
    const QString linkColor{lua_tostring(L, 2)};
    const QString linkVisitedColor{lua_tostring(L, 3)};
    const bool underline = hasUnderline ? static_cast<bool>(lua_toboolean(L, 4)) : true;

    Host& host = getHostFromLua(L);

    if (!host.setLinkStyle(labelName, linkColor, linkVisitedColor, underline)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetLinkStyle
int TLuaInterpreter::resetLinkStyle(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");

    Host& host = getHostFromLua(L);

    if (!host.resetLinkStyle(labelName)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearVisitedLinks
int TLuaInterpreter::clearVisitedLinks(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");

    Host& host = getHostFromLua(L);

    if (!host.clearVisitedLinks(labelName)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// commandlines inserted by the createCommandLine(...) function:
int TLuaInterpreter::enableCommandLine(lua_State* L)
{
    const QString commandLineName{CMDLINE_NAME(L, 1)};
    if (isMain(commandLineName)) {
        return warnArgumentValue(L, __func__, "this function is not permitted on the main command line");
    }
    auto console = CONSOLE_NIL(L, commandLineName);
    if (console) {
        // This name matches a TConsole instance so we are referring to a
        // TCommandLine at the bottom of it - so need to call the original
        // function that creates the latter if needed:
        console->setCmdVisible(true);
        lua_pushboolean(L, true);
        return 1;
    }

    // Else this might refer to an additional command line which must exist
    // for it to be shown by this function - the following macro will fail
    // (and return with a nil and an error message) if it doesn't:
    auto commandLine = COMMANDLINE(L, commandLineName);
    commandLine->setVisible(true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableHorizontalScrollBar
int TLuaInterpreter::enableHorizontalScrollBar(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setHorizontalScrollBar(true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScrollBar
int TLuaInterpreter::enableScrollBar(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setScrollBarVisible(true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getScrollBarVisible
int TLuaInterpreter::getScrollBarVisible(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    lua_pushboolean(L, console->getScrollBarVisible());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableTimeStamps
int TLuaInterpreter::enableTimeStamps(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto pConsole = CONSOLE(L, windowName);
    // *pConsole can be the main console as well as any user one
    if (pConsole->showTimeStamps()) {
        lua_pushnil(L);
        if (windowName.isEmpty()) {
            lua_pushstring(L, qsl("timestamps were not enabled for the main console").toUtf8().constData());
        } else {
            lua_pushstring(L, qsl("timestamps were not enabled for the \"%1\" console").arg(windowName).toUtf8().constData());
        }
        return 2;
    }

    pConsole->slot_toggleTimeStamps(true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAvailableFonts
int TLuaInterpreter::getAvailableFonts(lua_State* L)
{
    auto fontList = mudlet::self()->getAvailableFonts();

    lua_newtable(L);
    for (auto& font : fontList) {
        lua_pushstring(L, font.toUtf8().constData());
        lua_pushboolean(L, true);
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBackgroundColor
int TLuaInterpreter::getBackgroundColor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QColor color;

    QString windowName = qsl("main");
    const int n = lua_gettop(L);
    if (n > 0) {
        windowName = getVerifiedString(L, __func__, 1, "window name");
    }

    if (isMain(windowName)) {
        // the view's colour is a reference to this one, so read it straight from
        // the model and the answer is the same with or without a window
        color = host.mainConsoleModel().mBgColor;
    } else if (auto optionalColor = host.getBackgroundColor(windowName)) {
        color = optionalColor.value();
    } else {
        return warnArgumentValue(L, __func__, qsl("window '%1' does not exist").arg(windowName));
    }

    lua_pushnumber(L, color.red());
    lua_pushnumber(L, color.green());
    lua_pushnumber(L, color.blue());
    lua_pushnumber(L, color.alpha());
    return 4;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBgColor
int TLuaInterpreter::getBgColor(lua_State* L)
{
    QString windowName = qsl("main");
    if (lua_gettop(L) > 0) {
        windowName = getVerifiedString(L, __func__, 1, "window name", true);
    }

    const Host& host = getHostFromLua(L);
    std::list<int> const result = host.mpConsole ? host.mpConsole->getBgColor(windowName) : std::list<int>{};
    for (const int pos : result) {
        lua_pushnumber(L, pos);
    }
    return result.size();
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderBottom
int TLuaInterpreter::getBorderBottom(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.userBorders().bottom());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderLeft
int TLuaInterpreter::getBorderLeft(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.userBorders().left());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderRight
int TLuaInterpreter::getBorderRight(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.userBorders().right());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderSizes
int TLuaInterpreter::getBorderSizes(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    auto sizes = host.userBorders();
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, sizes.top());
    lua_setfield(L, -2, "top");
    lua_pushinteger(L, sizes.right());
    lua_setfield(L, -2, "right");
    lua_pushinteger(L, sizes.bottom());
    lua_setfield(L, -2, "bottom");
    lua_pushinteger(L, sizes.left());
    lua_setfield(L, -2, "left");
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderTop
int TLuaInterpreter::getBorderTop(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.userBorders().top());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderColor
int TLuaInterpreter::getBorderColor(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    const QColor color = host.mpConsole->borderColor();
    lua_pushnumber(L, color.red());
    lua_pushnumber(L, color.green());
    lua_pushnumber(L, color.blue());
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getClipboardText
int TLuaInterpreter::getClipboardText(lua_State* L)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    lua_pushstring(L, clipboard->text().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getColumnCount
int TLuaInterpreter::getColumnCount(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    int columns;
    auto console = CONSOLE(L, windowName);
    columns = console->mUpperPane->getColumnCount();
    lua_pushnumber(L, columns);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getColumnNumber
int TLuaInterpreter::getColumnNumber(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getColumnNumber());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCurrentLine
int TLuaInterpreter::getCurrentLine(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = getHostFromLua(L).findConsole(windowName);
    if (!console) {
        // the next line should be "pushnil"; compatibility with old bugs and all that
        lua_pushstring(L, "ERROR: mini console does not exist");
        lua_pushfstring(L, bad_window_value, windowName.toUtf8().constData());
        return 2;
    }
    const QString line = console->getCurrentLine();
    lua_pushstring(L, line.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFgColor
int TLuaInterpreter::getFgColor(lua_State* L)
{
    QString windowName = qsl("main");
    if (lua_gettop(L) > 0) {
        windowName = getVerifiedString(L, __func__, 1, "window name", true);
    }

    const Host& host = getHostFromLua(L);
    std::list<int> const result = host.mpConsole ? host.mpConsole->getFgColor(windowName) : std::list<int>{};
    for (const int pos : result) {
        lua_pushnumber(L, pos);
    }
    return result.size();
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFont
int TLuaInterpreter::getFont(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    Host& host = getHostFromLua(L);

    auto actualFontFamily = [](const QFont& font) -> QString {
        return QFontInfo(font).family();
    };

    // A console wins a name a label also carries, the way it does for every other
    // window function. Labels are not in the map CONSOLE() searches, so a name no
    // console answers to is tried as a label before that macro gets to refuse it:
    auto console = CONSOLE_NIL(L, windowName);
    if (!console) {
        if (host.mpConsole) {
            if (TLabel* pLabel = host.mpConsole->mLabelMap.value(windowName)) {
                lua_pushstring(L, actualFontFamily(pLabel->font()).toUtf8().constData());
                return 1;
            }
        }
        console = CONSOLE(L, windowName);
    }

    QString fontName;

    if (console == host.mpConsole) {
        fontName = actualFontFamily(host.getDisplayFont());
    } else if (console->mUpperPane) {
        fontName = actualFontFamily(console->mUpperPane->font());
    } else {
        fontName = actualFontFamily(console->font());
    }

    lua_pushstring(L, fontName.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFontSize
int TLuaInterpreter::getFontSize(lua_State* L)
{
    int rval = -1;
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    rval = console->mUpperPane->font().pointSize();

    if (rval <= -1) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, rval);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getImageSize
int TLuaInterpreter::getImageSize(lua_State* L)
{
    const QString imageLocation = getVerifiedString(L, __func__, 1, "image location");
    if (imageLocation.isEmpty()) {
        return warnArgumentValue(L, __func__, "image location cannot be an empty string");
    }

    auto size = mudlet::self()->getImageSize(imageLocation);
    if (!size) {
        return warnArgumentValue(L, __func__, qsl("couldn't retrieve image size, is the location '%1' correct?").arg(imageLocation));
    }
    lua_pushnumber(L, size->width());
    lua_pushnumber(L, size->height());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLabelSizeHint
int TLuaInterpreter::getLabelSizeHint(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    const Host& host = getHostFromLua(L);
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }

    auto size = host.mpConsole ? host.mpConsole->getLabelSizeHint(labelName) : std::nullopt;
    if (!size) {
        return warnArgumentValue(L, __func__, qsl("label '%1' does not exist").arg(labelName));
    }
    lua_pushnumber(L, size->width());
    lua_pushnumber(L, size->height());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLabelStyleSheet
int TLuaInterpreter::getLabelStyleSheet(lua_State* L)
{
    const QString label = getVerifiedString(L, __func__, 1, "label");
    const Host& host = getHostFromLua(L);
    if (auto stylesheet = host.mpConsole ? host.mpConsole->getLabelStyleSheet(label) : std::nullopt) {
        lua_pushstring(L, stylesheet->toUtf8().constData());
        return 1;
    }

    lua_pushnil(L);
    lua_pushfstring(L, "label '%s' does not exist", label.toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLastLineNumber
int TLuaInterpreter::getLastLineNumber(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE_NIL(L, windowName);
    const int number = console ? console->getLastLineNumber() : -1;
    lua_pushnumber(L, number);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLineCount
int TLuaInterpreter::getLineCount(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getLineCount());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLines
int TLuaInterpreter::getLines(lua_State* L)
{
    const int n = lua_gettop(L);
    int s = 1;
    const int windowNamePos = (n > 2) ? s++ : 0;
    if (windowNamePos && !checkStringArg(L, __func__, windowNamePos, "mini console, user window or buffer name {may be omitted for the \"main\" console}", true)) {
        return lua_error(L);
    }
    const int lineFrom = getVerifiedInt(L, __func__, s++, "start line");
    const int lineTo = getVerifiedInt(L, __func__, s, "end line");

    QString windowName;
    if (windowNamePos) {
        windowName = lua_tostring(L, windowNamePos);
    }

    Host& host = getHostFromLua(L);
    QPair<bool, QStringList> const result = host.getLines(windowName, lineFrom, lineTo);
    if (!result.first) {
        // Only one QString in .second - the error message
        return warnArgumentValue(L, __func__, result.second.at(0));
    }
    lua_newtable(L);
    for (int i = 0, total = result.second.size(); i < total; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, result.second.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLineNumber
int TLuaInterpreter::getLineNumber(lua_State* L)
{
    QString windowName;
    int s = 0;

    if (lua_gettop(L) > 0) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, ++s);
    }

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getLineNumber());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMainConsoleWidth
int TLuaInterpreter::getMainConsoleWidth(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int fw = QFontMetrics(host.getDisplayFont()).averageCharWidth();
    fw *= host.mWrapAt + 1;
    lua_pushnumber(L, fw);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMouseEvents
int TLuaInterpreter::getMouseEvents(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    // create the result table
    lua_newtable(L);
    QMapIterator<QString, QStringList> it(host.mConsoleActions);
    while (it.hasNext()) {
        it.next();
        const QStringList eventInfo = it.value();
        lua_createtable(L, 0, 3);
        lua_pushstring(L, eventInfo.at(0).toUtf8().constData());
        lua_setfield(L, -2, "event name");
        lua_pushstring(L, eventInfo.at(1).toUtf8().constData());
        lua_setfield(L, -2, "display name");
        lua_pushstring(L, eventInfo.at(2).toUtf8().constData());
        lua_setfield(L, -2, "tooltip text");

        // Add the mapEvent object to the result table
        lua_setfield(L, -2, it.key().toUtf8().constData());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMousePosition
int TLuaInterpreter::getMousePosition(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }

    const QPoint pos = host.mpConsole->mapFromGlobal(QCursor::pos());

    lua_pushnumber(L, pos.x());
    lua_pushnumber(L, pos.y());

    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getProfileTabNumber
int TLuaInterpreter::getProfileTabNumber(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto profileIndex = mudlet::self()->mpTabBar->tabIndex(host.getName());
    if (profileIndex != -1) {
        lua_pushnumber(L, profileIndex + 1);
        return 1;
    }

    return warnArgumentValue(L, __func__, "could not retrieve the tab number");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMainWindowSize
int TLuaInterpreter::getMainWindowSize(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    const QSize mainWindowSize = host.mpConsole->getMainWindowSize();

    lua_pushnumber(L, mainWindowSize.width());
    lua_pushnumber(L, mainWindowSize.height());

    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRowCount
int TLuaInterpreter::getRowCount(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    int rows;
    auto console = CONSOLE(L, windowName);
    rows = console->mUpperPane->getRowCount();
    lua_pushnumber(L, rows);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getScroll
int TLuaInterpreter::getScroll(lua_State* L)
{
    QString windowName;

    const int n = lua_gettop(L);
    if (n == 1) {
        windowName = getVerifiedString(L, __func__, 1, "window name", true);
    } else {
        windowName = QLatin1String("main");
    }

    auto console = getHostFromLua(L).findConsole(windowName);
    if (!console) {
        lua_pushnil(L);
        lua_pushfstring(L, bad_window_value, windowName.toUtf8().constData());
        return 2;
    }

    int result = console->mUpperPane->mCursorY;
    result = std::min(result, console->getLastLineNumber());
    result = std::max(result, 0);
    lua_pushnumber(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSelection
int TLuaInterpreter::getSelection(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }
    auto console = CONSOLE(L, windowName);

    auto [valid, text, start, length] = console->getSelection();

    if (!valid) {
        return warnArgumentValue(L, __func__, text);
    }

    lua_pushstring(L, text.toUtf8().constData());
    lua_pushnumber(L, start);
    lua_pushnumber(L, length);
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTextFormat
int TLuaInterpreter::getTextFormat(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L)) {
        windowName = getVerifiedString(L, __func__, 1, "window name", true);
    }

    auto console = getHostFromLua(L).findConsole(windowName);

    if (!console) {
        return warnArgumentValue(L, __func__, qsl("window '%1' not found").arg(windowName));
    }

    QPair<quint8, TChar> const result = console->getTextAttributes();

    if (result.first == 2) {
        return warnArgumentValue(L, __func__, qsl("current selection invalid in window '%1'").arg(windowName));
    }

    if (result.first != 0) {
        return warnArgumentValue(L, __func__, qsl("no character under cursor or selection in window '%1'").arg(windowName));
    }

    lua_newtable(L);

    lua_pushstring(L, "bold");
    lua_pushboolean(L, result.second.isBold());
    lua_settable(L, -3);

    lua_pushstring(L, "italic");
    lua_pushboolean(L, result.second.isItalic());
    lua_settable(L, -3);

    lua_pushstring(L, "overline");
    lua_pushboolean(L, result.second.isOverlined());
    lua_settable(L, -3);

    lua_pushstring(L, "reverse");
    lua_pushboolean(L, result.second.isReversed());
    lua_settable(L, -3);

    lua_pushstring(L, "strikeout");
    lua_pushboolean(L, result.second.isStruckOut());
    lua_settable(L, -3);

    lua_pushstring(L, "underline");
    lua_pushboolean(L, result.second.isUnderlined());
    lua_settable(L, -3);

    lua_pushstring(L, "underlineStyle");
    // This says what is drawn, not what the cell remembers - a hyperlink's own
    // styling can OR a second style flag onto a cell that SGR already styled,
    // and setUnderline(false) leaves a style flag behind. So resolve in the
    // order the screen does: TTextEdit::drawCustomDecorations tries wavy, then
    // dotted, then dashed, and Qt draws the plain underline when none is set.
    if (!result.second.isUnderlined()) {
        lua_pushstring(L, "none");
    } else if (result.second.isUnderlineWavy()) {
        lua_pushstring(L, "wavy");
    } else if (result.second.isUnderlineDotted()) {
        lua_pushstring(L, "dotted");
    } else if (result.second.isUnderlineDashed()) {
        lua_pushstring(L, "dashed");
    } else {
        lua_pushstring(L, "solid");
    }
    lua_settable(L, -3);

    lua_pushstring(L, "blinking");
    if (Q_UNLIKELY(result.second.isFastBlinking())) {
        lua_pushstring(L, "fast");
    } else {
        if (Q_UNLIKELY(result.second.isBlinking())) {
            lua_pushstring(L, "slow");
        } else {
            lua_pushstring(L, "none");
        }
    }
    lua_settable(L, -3);

    lua_pushstring(L, "concealed");
    lua_pushboolean(L, result.second.isConcealed());
    lua_settable(L, -3);

    lua_pushstring(L, "alternateFont");
    lua_pushinteger(L, result.second.alternateFont());
    lua_settable(L, -3);

    const QColor foreground(result.second.foreground());
    lua_pushstring(L, "foreground");
    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, foreground.red());
    lua_settable(L, -3);

    lua_pushnumber(L, 2);
    lua_pushnumber(L, foreground.green());
    lua_settable(L, -3);

    lua_pushnumber(L, 3);
    lua_pushnumber(L, foreground.blue());
    lua_settable(L, -3);
    lua_settable(L, -3);

    const QColor background(result.second.background());
    lua_pushstring(L, "background");
    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, background.red());
    lua_settable(L, -3);

    lua_pushnumber(L, 2);
    lua_pushnumber(L, background.green());
    lua_settable(L, -3);

    lua_pushnumber(L, 3);
    lua_pushnumber(L, background.blue());
    lua_settable(L, -3);

    lua_settable(L, -3);

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#timeStampsEnabled
int TLuaInterpreter::timeStampsEnabled(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto pConsole = CONSOLE(L, windowName);
    // *pConsole can be the main console as well as any user one
    lua_pushboolean(L, pConsole->showTimeStamps());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getUserWindowSize
int TLuaInterpreter::getUserWindowSize(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    const QSize userWindowSize = host.mpConsole->getUserWindowSize(windowName);
    lua_pushnumber(L, userWindowSize.width());
    lua_pushnumber(L, userWindowSize.height());

    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getWindowGeometry
int TLuaInterpreter::getWindowGeometry(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const QString windowName = getVerifiedString(L, __func__, 1, "window name");

    if (auto geometry = host.windowGeometry(windowName)) {
        lua_pushnumber(L, geometry->x());
        lua_pushnumber(L, geometry->y());
        lua_pushnumber(L, geometry->width());
        lua_pushnumber(L, geometry->height());
        return 4;
    }

    lua_pushnil(L);
    lua_pushfstring(L, bad_window_value, windowName.toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#windowVisible
int TLuaInterpreter::windowVisible(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const QString windowName = getVerifiedString(L, __func__, 1, "window name");

    if (auto visible = host.windowVisible(windowName)) {
        lua_pushboolean(L, *visible);
        return 1;
    }

    lua_pushnil(L);
    lua_pushfstring(L, bad_window_value, windowName.toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLabelText
int TLuaInterpreter::getLabelText(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    auto label = LABEL(L, labelName);
    lua_pushstring(L, label->text().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getWindowWrap
int TLuaInterpreter::getWindowWrap(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getWrapAt());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasFocus
int TLuaInterpreter::hasFocus(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole && host.mpConsole->hasFocus()); //FIXME
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hideToolBar
int TLuaInterpreter::hideToolBar(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);
    if (auto [moved, message] = host.getActionUnit()->hideToolBar(windowName); !moved) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hideWindow
int TLuaInterpreter::hideWindow(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");

    Host& host = getHostFromLua(L);
    host.hideWindow(text);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertLink
int TLuaInterpreter::insertLink(lua_State* L)
{
    const int n = lua_gettop(L);
    // with exactly four arguments the last one is either the format flag - and
    // then there is no window name - or the hint
    const bool hasWindowName = (n > 4) || (n == 4 && !lua_isboolean(L, 4));
    const bool hasFormatFlag = (n > 4) || (n == 4 && lua_isboolean(L, 4));

    int s = 0;
    const int windowNamePos = hasWindowName ? ++s : 0;
    if (hasWindowName && !checkStringArg(L, __func__, windowNamePos, "window name")) {
        return lua_error(L);
    }
    const int textPos = ++s;
    if (!checkStringArg(L, __func__, textPos, "text")) {
        return lua_error(L);
    }
    int commandPos = ++s;
    if (!checkCommandOrFunctionArg(L, __func__, commandPos)) {
        return lua_error(L);
    }
    const int hintPos = ++s;
    if (!checkStringArg(L, __func__, hintPos, "hint")) {
        return lua_error(L);
    }
    const int formatPos = hasFormatFlag ? ++s : 0;
    if (hasFormatFlag && !checkBoolArg(L, __func__, formatPos, "useCurrentFormat")) {
        return lua_error(L);
    }

    // resolved before the parse, so a miss strands nothing - see releaseLuaReferences()
    const QString windowName = hasWindowName ? QString{lua_tostring(L, windowNamePos)} : qsl("main");
    auto console = CONSOLE(L, windowName);

    QString command;
    int luaReference = 0;
    parseCommandOrFunction(L, __func__, commandPos, command, luaReference);

    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    commandList << command;
    luaReferences << luaReference;
    hintList << QString{lua_tostring(L, hintPos)};

    const bool useCurrentFormat = hasFormatFlag && lua_toboolean(L, formatPos);
    console->insertLink(QString{lua_tostring(L, textPos)}, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertPopup
int TLuaInterpreter::insertPopup(lua_State* L)
{
    const int n = lua_gettop(L);
    // with exactly four arguments the last one is either the format flag - and
    // then there is no window name - or the hints table
    const bool hasWindowName = (n > 4) || (n == 4 && !lua_isboolean(L, 4));
    const bool hasFormatFlag = (n > 4) || (n == 4 && lua_isboolean(L, 4));

    int s = 0;
    const int windowNamePos = hasWindowName ? ++s : 0;
    if (hasWindowName && !checkStringArg(L, __func__, windowNamePos, "window name")) {
        return lua_error(L);
    }
    const int textPos = ++s;
    if (!checkStringArg(L, __func__, textPos, "text")) {
        return lua_error(L);
    }
    int commandPos = ++s;
    int hintPos = ++s;
    const int formatPos = hasFormatFlag ? ++s : 0;

    if (!checkCommandsOrFunctionsTable(L, __func__, commandPos) || !checkHintsTable(L, __func__, hintPos) || (hasFormatFlag && !checkBoolArg(L, __func__, formatPos, "useCurrentFormat"))) {
        return lua_error(L);
    }

    // resolved before the parse, so a miss strands nothing - see releaseLuaReferences()
    const QString windowName = hasWindowName ? QString{lua_tostring(L, windowNamePos)} : qsl("main");
    auto console = CONSOLE(L, windowName);

    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    parseCommandsOrFunctionsTable(L, __func__, commandPos, commandList, luaReferences);
    parseHintsTable(L, __func__, hintPos, hintList);

    if ((hintList.size() - commandList.size()) < 0 || (hintList.size() - commandList.size()) > 1) {
        releaseLuaReferences(L, luaReferences);
        lua_pushnil(L);
        lua_pushfstring(L,
                        "command table and hint table sizes do not match up (%d and %d, either they must be the same or there should be one extra hint) - cannot create popup",
                        commandList.size(),
                        hintList.size());
        return 2;
    }

    const bool useCurrentFormat = hasFormatFlag && lua_toboolean(L, formatPos);
    console->insertLink(QString{lua_tostring(L, textPos)}, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertText
int TLuaInterpreter::insertText(lua_State* L)
{
    const char* windowName = "";
    int s = 0;

    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, ++s);
    }
    const QString text = getVerifiedString(L, __func__, ++s, "text");

    auto console = CONSOLE(L, QString{windowName});
    console->insertText(text);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAnsiBgColor
int TLuaInterpreter::isAnsiBgColor(lua_State* L)
{
    QString windowName = qsl("main");
    const int ansiBg = getVerifiedInt(L, __func__, 1, "ANSI color");

    std::list<int> result;
    const Host& host = getHostFromLua(L);
    result = host.mpConsole ? host.mpConsole->getBgColor(windowName) : std::list<int>{};
    auto it = result.begin();
    if (result.size() < 3) {
        return warnArgumentValue(L, __func__, qsl("current selection invalid in window '%1'").arg(windowName));
    }
    if (ansiBg < 0 || ansiBg > 16) {
        return warnArgumentValue(L, __func__, qsl("ANSI color %1 out of range (0 to 16)").arg(ansiBg));
    }


    QColor c;
    switch (ansiBg) {
    case 0:
        c = host.mBgColor;
        break;
    case 1:
        c = host.mLightBlack;
        break;
    case 2:
        c = host.mBlack;
        break;
    case 3:
        c = host.mLightRed;
        break;
    case 4:
        c = host.mRed;
        break;
    case 5:
        c = host.mLightGreen;
        break;
    case 6:
        c = host.mGreen;
        break;
    case 7:
        c = host.mLightYellow;
        break;
    case 8:
        c = host.mYellow;
        break;
    case 9:
        c = host.mLightBlue;
        break;
    case 10:
        c = host.mBlue;
        break;
    case 11:
        c = host.mLightMagenta;
        break;
    case 12:
        c = host.mMagenta;
        break;
    case 13:
        c = host.mLightCyan;
        break;
    case 14:
        c = host.mCyan;
        break;
    case 15:
        c = host.mLightWhite;
        break;
    case 16:
        c = host.mWhite;
        break;
    }

    int val = *it;
    if (val == c.red()) {
        it++;
        val = *it;
        if (val == c.green()) {
            it++;
            val = *it;
            if (val == c.blue()) {
                lua_pushboolean(L, true);
                return 1;
            }
        }
    }

    lua_pushboolean(L, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAnsiFgColor
int TLuaInterpreter::isAnsiFgColor(lua_State* L)
{
    QString windowName = qsl("main");
    const int ansiFg = getVerifiedInt(L, __func__, 1, "ANSI color");

    std::list<int> result;
    const Host& host = getHostFromLua(L);
    result = host.mpConsole ? host.mpConsole->getFgColor(windowName) : std::list<int>{};
    auto it = result.begin();
    if (result.size() < 3) {
        return warnArgumentValue(L, __func__, qsl("current selection invalid in window '%1'").arg(windowName));
    }
    if (ansiFg < 0 || ansiFg > 16) {
        return warnArgumentValue(L, __func__, qsl("ANSI color %1 out of range (0 to 16)").arg(ansiFg));
    }


    QColor c;
    switch (ansiFg) {
    case 0:
        c = host.mFgColor;
        break;
    case 1:
        c = host.mLightBlack;
        break;
    case 2:
        c = host.mBlack;
        break;
    case 3:
        c = host.mLightRed;
        break;
    case 4:
        c = host.mRed;
        break;
    case 5:
        c = host.mLightGreen;
        break;
    case 6:
        c = host.mGreen;
        break;
    case 7:
        c = host.mLightYellow;
        break;
    case 8:
        c = host.mYellow;
        break;
    case 9:
        c = host.mLightBlue;
        break;
    case 10:
        c = host.mBlue;
        break;
    case 11:
        c = host.mLightMagenta;
        break;
    case 12:
        c = host.mMagenta;
        break;
    case 13:
        c = host.mLightCyan;
        break;
    case 14:
        c = host.mCyan;
        break;
    case 15:
        c = host.mLightWhite;
        break;
    case 16:
        c = host.mWhite;
        break;
    }

    int val = *it;
    if (val == c.red()) {
        it++;
        val = *it;
        if (val == c.green()) {
            it++;
            val = *it;
            if (val == c.blue()) {
                lua_pushboolean(L, true);
                return 1;
            }
        }
    }

    lua_pushboolean(L, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadWindowLayout
int TLuaInterpreter::loadWindowLayout(lua_State* L)
{
    lua_pushboolean(L, mudlet::self()->loadWindowLayout());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lowerWindow
int TLuaInterpreter::lowerWindow(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole && host.mpConsole->lowerWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveCursor
int TLuaInterpreter::moveCursor(lua_State* L)
{
    int s = 1;
    const int n = lua_gettop(L);
    const char* windowName = "";
    if (n > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    const int luaFrom = getVerifiedInt(L, __func__, s++, "x");
    const int luaTo = getVerifiedInt(L, __func__, s, "y");

    auto console = CONSOLE(L, QString{windowName});
    lua_pushboolean(L, console->moveCursor(luaFrom, luaTo));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveCursorEnd
int TLuaInterpreter::moveCursorEnd(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->moveCursorEnd();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveWindow
int TLuaInterpreter::moveWindow(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "name")) {
        return lua_error(L);
    }
    const double x1 = getVerifiedDouble(L, __func__, 2, "x");
    const double y1 = getVerifiedDouble(L, __func__, 3, "y");
    const QString text{lua_tostring(L, 1)};
    Host& host = getHostFromLua(L);
    host.moveWindow(text, static_cast<int>(x1), static_cast<int>(y1));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#openUserWindow
int TLuaInterpreter::openUserWindow(lua_State* L)
{
    const int n = lua_gettop(L);
    if (lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "openUserWindow:  bad argument #1 type (name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (n > 1 && !checkBoolArg(L, __func__, 2, "loadLayout", true)) {
        return lua_error(L);
    }
    if (n > 2 && !checkBoolArg(L, __func__, 3, "autoDock", true)) {
        return lua_error(L);
    }
    if (n > 3 && lua_type(L, 4) != LUA_TSTRING) {
        lua_pushfstring(L, "openUserWindow: bad argument #4 type (area as string expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    }

    const QString name{lua_tostring(L, 1)};
    const bool loadLayout = (n > 1) ? static_cast<bool>(lua_toboolean(L, 2)) : true;
    const bool autoDock = (n > 2) ? static_cast<bool>(lua_toboolean(L, 3)) : true;
    QString area;
    if (n > 3) {
        area = lua_tostring(L, 4);
    }

    Host& host = getHostFromLua(L);
    //Don't create Userwindow if there is a Label with the same name already. It breaks the UserWindow

    if (auto [success, message] = host.openWindow(name, loadLayout, autoDock, area.toLower()); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#paste
int TLuaInterpreter::paste(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    console->paste();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#pauseMovie
int TLuaInterpreter::pauseMovie(lua_State* L)
{
    return movieFunc(L, "pauseMovie");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseWindow
int TLuaInterpreter::raiseWindow(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole && host.mpConsole->raiseWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCommandLineMenuEvent
int TLuaInterpreter::removeCommandLineMenuEvent(lua_State* L)
{
    const int argsCount = lua_gettop(L);
    const bool hasCommandLineName = (argsCount >= 2);
    const int menuLabelPos = hasCommandLineName ? 2 : 1;

    if (hasCommandLineName && !checkStringArg(L, __func__, 1, "command line name")) {
        return lua_error(L);
    }
    if (!checkStringArg(L, __func__, menuLabelPos, "menu label")) {
        return lua_error(L);
    }

    const QString commandLineName = hasCommandLineName ? QString{lua_tostring(L, 1)} : qsl("main");
    const QString menuLabel{lua_tostring(L, menuLabelPos)};

    const auto& commandline = COMMANDLINE(L, commandLineName);

    if (commandline->contextMenuItems.remove(menuLabel) == 0) {
        lua_pushboolean(L, false);
        lua_pushfstring(L, "removeCommandLineMenuEvent: cannot remove '%s', menu item does not exist", menuLabel.toUtf8().constData());
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeMouseEvent
int TLuaInterpreter::removeMouseEvent(lua_State* L)
{
    const QString uniqueName = getVerifiedString(L, __func__, 1, "event name");
    Host& host = getHostFromLua(L);
    if (host.mConsoleActions.remove(uniqueName) == 0) {
        return warnArgumentValue(L, __func__, qsl("mouse event '%1' does not exist").arg(uniqueName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#replace
int TLuaInterpreter::replace(lua_State* L)
{
    const int n = lua_gettop(L);
    int s = 1;
    const char* windowName = "";

    if (n > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    const QString text = getVerifiedString(L, __func__, s, "with");

    auto console = CONSOLE(L, QString{windowName});
    console->replace(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetCmdLineAction
int TLuaInterpreter::resetCmdLineAction(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString name = getVerifiedString(L, __func__, 1, "command line name");
    if (name.isEmpty()) {
        return warnArgumentValue(L, __func__, "command line name cannot be an empty string");
    }

    bool lua_result = false;
    lua_result = host.resetCmdLineAction(name);
    if (lua_result) {
        lua_pushboolean(L, true);
        return 1;
    }
    return warnArgumentValue(L, __func__, qsl("command line name '%1' not found").arg(name));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetBackgroundImage
int TLuaInterpreter::resetBackgroundImage(lua_State* L)
{
    QString windowName = qsl("main");
    bool fullWindow = false;
    const int n = lua_gettop(L);
    int counter = 1;
    const bool hasWindowName = (n > 0 && lua_type(L, 1) == LUA_TSTRING);
    if (hasWindowName) {
        counter++;
    }

    if (counter <= n) {
        fullWindow = getVerifiedBool(L, __func__, counter, "fullWindow");
        counter++;
    }

    if (hasWindowName) {
        windowName = lua_tostring(L, 1);
    }

    if (fullWindow && !(windowName.isEmpty() || windowName.compare(qsl("main"), Qt::CaseSensitive) == 0)) {
        return warnArgumentValue(L, __func__, qsl("the full window background can only be reset on the main console"));
    }

    Host* host = &getHostFromLua(L);
    if (!host->resetBackgroundImage(windowName, fullWindow)) {
        return warnArgumentValue(L, __func__, qsl("console '%1' not found").arg(windowName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetFormat
int TLuaInterpreter::resetFormat(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->reset();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resizeWindow
int TLuaInterpreter::resizeWindow(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "windowName")) {
        return lua_error(L);
    }
    const double x1 = getVerifiedDouble(L, __func__, 2, "width");
    const double y1 = getVerifiedDouble(L, __func__, 3, "height");
    const QString text{lua_tostring(L, 1)};
    Host& host = getHostFromLua(L);
    host.resizeWindow(text, static_cast<int>(x1), static_cast<int>(y1));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveWindowLayout
int TLuaInterpreter::saveWindowLayout(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    // the flag is what makes the save on the way out a no-op, and a save asked
    // for from a script is no substitute for that one, so it goes back up only
    // if this call really saved
    const bool hadSavedLayout = pMudlet->mHasSavedLayout;
    pMudlet->mHasSavedLayout = false;
    const bool saved = pMudlet->saveWindowLayout();
    pMudlet->mHasSavedLayout = hadSavedLayout && saved;
    lua_pushboolean(L, saved);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#scaleMovie
int TLuaInterpreter::scaleMovie(lua_State* L)
{
    return movieFunc(L, "scaleMovie");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCaptureGroup
// Note: numeric argument uses matches[] indexing, i.e. selectCaptureGroup(1)
// selects matches[1] (full match), selectCaptureGroup(2) selects matches[2]
// (first capture group), etc. Named arguments select the named group directly.
int TLuaInterpreter::selectCaptureGroup(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "selectCaptureGroup: bad argument #1 type (capture group as number or capture group name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pL = host.getLuaInterpreter();
    int begin = 0;
    int length = 0;

    if (lua_isnumber(L, 1)) {
        auto captureGroup = lua_tonumber(L, 1);
        if (captureGroup < 1) {
            lua_pushnumber(L, -1);
            return 1;
        }
        // We want capture groups to start with 1 instead of 0 so predecrement
        // luaNumOfMatch :
        if (--captureGroup < static_cast<int>(host.getLuaInterpreter()->mCaptureGroupList.size())) {
            auto iti = pL->mCaptureGroupPosList.begin();
            auto its = pL->mCaptureGroupList.begin();
            begin = *iti;
            std::string& s = *its;

            for (int i = 0; iti != pL->mCaptureGroupPosList.end(); ++iti, ++i) {
                begin = *iti;
                if (i >= captureGroup) {
                    break;
                }
            }
            for (int i = 0; its != pL->mCaptureGroupList.end(); ++its, ++i) {
                s = *its;
                if (i >= captureGroup) {
                    break;
                }
            }

            length = QString::fromStdString(s).size();
            if (TDebug::wants(TDebug::Category::Selection)) {
                TDebug(Qt::white, Qt::red, TDebug::Category::Selection) << "selectCaptureGroup(" << begin << ", " << length << ")\n" >> &host;
            }
        }
    } else if (lua_isstring(L, 1)) {
        auto name = lua_tostring(L, 1);
        if (pL->mCapturedNameGroupsPosList.contains(name)) {
            begin = pL->mCapturedNameGroupsPosList.value(name).first;
            length = pL->mCapturedNameGroupsPosList.value(name).second;
        }
    }
    if (length > 0 && host.mpConsole) {
        const int pos = host.mpConsole->selectSection(begin, length);
        lua_pushnumber(L, pos);
    } else {
        lua_pushnumber(L, -1);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCmdLineText
int TLuaInterpreter::selectCmdLineText(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = qsl("main");
    if (n >= 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto commandline = COMMANDLINE(L, name);
    commandline->selectAll();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCurrentLine
int TLuaInterpreter::selectCurrentLine(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    console->selectCurrentLine();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectSection
int TLuaInterpreter::selectSection(lua_State* L)
{
    int s = 1;
    const char* windowName = "";

    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, s++);
    }
    const int from = getVerifiedInt(L, __func__, s++, "from position");
    const int to = getVerifiedInt(L, __func__, s, "length");

    auto console = CONSOLE(L, QString{windowName});
    lua_pushboolean(L, console->selectSection(from, to));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectString
int TLuaInterpreter::selectString(lua_State* L)
{
    int s = 1;
    const char* windowName = "";
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    const int searchTextPos = s++;
    if (!checkStringArg(L, __func__, searchTextPos, "text to select")) {
        return lua_error(L);
    }
    // CHECK: Do we need to qualify this for a non-blank string?

    const auto numOfMatch = getVerifiedInt(L, __func__, s, "match count {1 for first}");
    const QString searchText{lua_tostring(L, searchTextPos)};

    auto console = CONSOLE(L, QString{windowName});
    lua_pushnumber(L, console->select(searchText, numOfMatch));
    return 1;
}

int TLuaInterpreter::setActiveProfile(lua_State* L)
{
    auto& hostManager = mudlet::self()->getHostManager();
    const QString requestedName = getVerifiedString(L, __func__, 1, "profile name");

    if (requestedName.isEmpty()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, "setActiveProfile: profile name cannot be empty");
        return 2;
    }

    const QString profileName = mudlet::self()->getCanonicalProfileName(requestedName);
    if (profileName.isEmpty()) {
        lua_pushboolean(L, false);
        lua_pushfstring(L, "setActiveProfile: profile '%s' does not exist", requestedName.toUtf8().constData());
        return 2;
    }

    if (!hostManager.hostLoaded(profileName)) {
        lua_pushboolean(L, false);
        lua_pushfstring(L, "setActiveProfile: profile '%s' is not loaded", profileName.toUtf8().constData());
        return 2;
    }

    mudlet::self()->mpTabBar->setCurrentIndex(mudlet::self()->mpTabBar->tabIndex(profileName));
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAppStyleSheet
int TLuaInterpreter::setAppStyleSheet(lua_State* L)
{
    const int n = lua_gettop(L);
    if (!checkStringArg(L, __func__, 1, "style sheet")) {
        return lua_error(L);
    }
    if (n > 1 && !checkStringArg(L, __func__, 2, "tag")) {
        return lua_error(L);
    }

    const QString styleSheet{lua_tostring(L, 1)};
    QString tag;
    if (n > 1) {
        tag = lua_tostring(L, 2);
    }

    Host& host = getHostFromLua(L);
    TEvent event{};
    event.mArgumentList.append(QLatin1String("sysAppStyleSheetChange"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(tag);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(host.getName());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    qApp->setStyleSheet(styleSheet);
    mudlet::self()->getHostManager().postInterHostEvent(nullptr, event, true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBackgroundColor
int TLuaInterpreter::setBackgroundColor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const char* windowNameArg = "";
    int r, alpha;
    int s = 1;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    if (lua_type(L, s) == LUA_TSTRING) {
        windowNameArg = WINDOW_NAME(L, s++);
        r = getVerifiedInt(L, __func__, s, "red value 0-255");
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else if (lua_isnumber(L, s)) {
        r = static_cast<int>(lua_tonumber(L, s));
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (window name as string, or red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    const int g = getVerifiedInt(L, __func__, ++s, "green value 0-255");
    if (!validRange(g)) {
        return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
    }

    const int b = getVerifiedInt(L, __func__, ++s, "blue value 0-255");
    if (!validRange(b)) {
        return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
    }

    // if we get nothing for the alpha value, assume it is 255. If we get a non-number value, complain.
    alpha = 255;
    if (lua_gettop(L) > s) {
        alpha = getVerifiedInt(L, __func__, ++s, "alpha value 0-255", true);
        if (!validRange(alpha)) {
            return warnArgumentValue(L, __func__, csmInvalidAlphaValue.arg(alpha));
        }
    }

    const QString windowName{windowNameArg};
    if (isMain(windowName)) {
        host.mBgColor.setRgb(r, g, b, alpha);
        // Host outlives its main console, so there may be no view to restyle -
        // the buffer's copy of the colours still has to follow:
        if (host.mpConsole) {
            host.mpConsole->setConsoleBgColor(r, g, b, alpha);
        } else {
            host.refreshMainConsoleColors();
        }
    } else if (!host.setBackgroundColor(windowName, r, g, b, alpha)) {
        return warnArgumentValue(L, __func__, qsl("window/label '%1' not found").arg(windowName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBackgroundImage
int TLuaInterpreter::setBackgroundImage(lua_State* L)
{
    QString windowName = qsl("main");
    int mode = 1;
    bool fullWindow = false;
    int counter = 1;
    const int n = lua_gettop(L);
    const bool hasWindowName = (n > 1 && lua_type(L, 2) == LUA_TSTRING);
    if (hasWindowName) {
        if (!checkStringArg(L, __func__, 1, "console or label name")) {
            return lua_error(L);
        }
        counter++;
    }

    const int imgPathPos = counter++;
    if (!checkStringArg(L, __func__, imgPathPos, "image path")) {
        return lua_error(L);
    }

    if (counter <= n) {
        mode = getVerifiedInt(L, __func__, counter, "mode");
        counter++;
    }

    if (counter <= n) {
        fullWindow = getVerifiedBool(L, __func__, counter, "fullWindow");
        counter++;
    }

    if (hasWindowName) {
        windowName = lua_tostring(L, 1);
    }
    QString imgPath{lua_tostring(L, imgPathPos)};

    if (mode < 1 || mode > 5) {
        return warnArgumentValue(L, __func__, qsl("%1 is not a valid mode! Valid modes are 1 'border', 2 'center', 3 'tile', 4 'style', 5 'cover'").arg(mode));
    }

    if (mode == 5 && !fullWindow) {
        return warnArgumentValue(L, __func__, qsl("mode 'cover' is not supported for the main display - pass true as the 4th argument to apply it to the full window background instead"));
    }

    if (fullWindow && !(windowName.isEmpty() || windowName.compare(qsl("main"), Qt::CaseSensitive) == 0)) {
        return warnArgumentValue(L, __func__, qsl("the full window background can only be used with the main console"));
    }

    Host* host = &getHostFromLua(L);
    if (!host->setBackgroundImage(windowName, imgPath, mode, fullWindow)) {
        if (fullWindow) {
            // the console name is already validated above, so this is about the image
            return warnArgumentValue(L, __func__, qsl("could not use '%1' as a full window background image").arg(imgPath));
        }
        return warnArgumentValue(L, __func__, qsl("console or label '%1' not found").arg(windowName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBgColor
int TLuaInterpreter::setBgColor(lua_State* L)
{
    const char* windowName = "";
    int r, g, b, alpha;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    int s = 1;
    if (lua_isstring(L, s) && !lua_isnumber(L, s)) {
        windowName = WINDOW_NAME(L, s);

        if (!lua_isnumber(L, ++s)) {
            lua_pushfstring(L, "setBgColor: bad argument #%d type (red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
        r = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else if (lua_isnumber(L, s)) {
        r = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else {
        lua_pushfstring(L, "setBgColor: bad argument #%d type (window name as string, or red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    g = getVerifiedInt(L, __func__, ++s, "green value 0-255");
    if (!validRange(g)) {
        return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
    }

    b = getVerifiedInt(L, __func__, ++s, "blue value 0-255");
    if (!validRange(b)) {
        return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
    }

    // if we get nothing for the alpha value, assume it is 255. If we get a non-number value, complain.
    alpha = 255;
    if (lua_gettop(L) > s) {
        alpha = getVerifiedInt(L, __func__, ++s, "alpha value 0-255", true);
        if (!validRange(alpha)) {
            return warnArgumentValue(L, __func__, csmInvalidAlphaValue.arg(alpha));
        }
    }

    auto console = CONSOLE(L, QString{windowName});
    console->setBgColor(r, g, b, alpha);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBold
int TLuaInterpreter::setBold(lua_State* L)
{
    const char* windowName = "";
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable bold attribute");
    auto console = CONSOLE(L, QString{windowName});
    console->setDisplayAttributes(TChar::Bold, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderBottom
int TLuaInterpreter::setBorderBottom(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.userBorders();
    sizes.setBottom(getVerifiedInt(L, __func__, 1, "new size"));
    host.setUserBorders(sizes);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderColor
int TLuaInterpreter::setBorderColor(lua_State* L)
{
    const int luaRed = getVerifiedInt(L, __func__, 1, "red");
    const int luaGreen = getVerifiedInt(L, __func__, 2, "green");
    const int luaBlue = getVerifiedInt(L, __func__, 3, "blue");
    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    host.mpConsole->setBorderColor(QColor(luaRed, luaGreen, luaBlue));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderLeft
int TLuaInterpreter::setBorderLeft(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.userBorders();
    sizes.setLeft(getVerifiedInt(L, __func__, 1, "new size"));
    host.setUserBorders(sizes);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderRight
int TLuaInterpreter::setBorderRight(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.userBorders();
    sizes.setRight(getVerifiedInt(L, __func__, 1, "new size"));
    host.setUserBorders(sizes);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderSizes
int TLuaInterpreter::setBorderSizes(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const int numberOfArguments = lua_gettop(L);
    switch (numberOfArguments) {
    case 0:
        break;
    case 1: {
        auto value = getVerifiedInt(L, __func__, 1, "new size");
        host.setUserBorders({value, value, value, value});
        break;
    }
    case 2: {
        auto height = getVerifiedInt(L, __func__, 1, "new height");
        auto width = getVerifiedInt(L, __func__, 2, "new width");
        host.setUserBorders({width, height, width, height});
        break;
    }
    case 3: {
        auto top = getVerifiedInt(L, __func__, 1, "new top size");
        auto width = getVerifiedInt(L, __func__, 2, "new width");
        auto bottom = getVerifiedInt(L, __func__, 3, "new bottom size");
        host.setUserBorders({width, top, width, bottom});
        break;
    }
    default: {
        auto top = getVerifiedInt(L, __func__, 1, "new top size");
        auto right = getVerifiedInt(L, __func__, 2, "new right size");
        auto bottom = getVerifiedInt(L, __func__, 3, "new bottom size");
        auto left = getVerifiedInt(L, __func__, 4, "new left size");
        host.setUserBorders({left, top, right, bottom});
        break;
    }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderTop
int TLuaInterpreter::setBorderTop(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.userBorders();
    sizes.setTop(getVerifiedInt(L, __func__, 1, "new size"));
    host.setUserBorders(sizes);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFgColor
int TLuaInterpreter::setFgColor(lua_State* L)
{
    int s = 0;
    const int n = lua_gettop(L);
    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };
    const char* windowName = "";
    if (n > 3) {
        windowName = WINDOW_NAME(L, ++s);
    }
    const int luaRed = getVerifiedInt(L, __func__, ++s, "red component value");
    if (!validRange(luaRed)) {
        return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(luaRed));
    }
    const int luaGreen = getVerifiedInt(L, __func__, ++s, "green component value");
    if (!validRange(luaGreen)) {
        return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(luaGreen));
    }
    const int luaBlue = getVerifiedInt(L, __func__, ++s, "blue component value");
    if (!validRange(luaBlue)) {
        return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(luaBlue));
    }

    auto console = CONSOLE(L, QString{windowName});
    console->setFgColor(luaRed, luaGreen, luaBlue);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setButtonStyleSheet
int TLuaInterpreter::setButtonStyleSheet(lua_State* L)
{
    //args: name, css text
    if (!checkStringArg(L, __func__, 1, "name") || !checkStringArg(L, __func__, 2, "css")) {
        return lua_error(L);
    }
    const QString name{lua_tostring(L, 1)};
    const QString css{lua_tostring(L, 2)};
    Host& host = getHostFromLua(L);
    auto actionIds = host.getActionUnit()->findItems(name);
    if (actionIds.empty()) {
        return warnArgumentValue(L, __func__, qsl("no button named '%1' found").arg(name));
    }
    for (auto actionId : actionIds) {
        auto action = host.getActionUnit()->getAction(actionId);
        if (!action) {
            continue;
        }
        action->css = css;
    }
    host.getActionUnit()->updateAllToolbars();
    lua_pushboolean(L, 1);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setClipboardText
int TLuaInterpreter::setClipboardText(lua_State* L)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(getVerifiedString(L, __func__, 1, "text"));
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCmdLineAction
int TLuaInterpreter::setCmdLineAction(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!checkStringArg(L, __func__, 1, "command line name")) {
        return lua_error(L);
    }
    if (const QString name{lua_tostring(L, 1)}; name.isEmpty()) {
        return warnArgumentValue(L, __func__, "command line name cannot be an empty string");
    }
    if (!lua_isfunction(L, 2)) {
        lua_pushfstring(L, "setCmdLineAction: bad argument #2 type (function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    const QString name{lua_tostring(L, 1)};
    lua_remove(L, 1);
    const int func = luaL_ref(L, LUA_REGISTRYINDEX);

    if (!host.setCmdLineAction(name, func)) {
        luaL_unref(L, LUA_REGISTRYINDEX, func);
        return warnArgumentValue(L, __func__, qsl("command line name '%1' not found").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCmdLineStyleSheet
int TLuaInterpreter::setCmdLineStyleSheet(lua_State* L)
{
    const int n = lua_gettop(L);
    // The mandatory stylesheet is last, but with no arguments at all that would
    // be index 0 - not a valid Lua stack index, and Lua 5.1 hands back the first
    // free slot for it rather than complaining:
    const int styleSheetIndex = qMax(n, 1);
    if (n > 1 && !checkStringArg(L, __func__, 1, "command line name", true)) {
        return lua_error(L);
    }
    if (!checkStringArg(L, __func__, styleSheetIndex, "StyleSheet")) {
        return lua_error(L);
    }

    const QString name = (n > 1) ? QString{lua_tostring(L, 1)} : qsl("main");
    const QString styleSheet{lua_tostring(L, styleSheetIndex)};
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->setCmdLineStyleSheet(name, styleSheet); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCmdLineStyleSheet
int TLuaInterpreter::getCmdLineStyleSheet(lua_State* L)
{
    // an explicit nil means "the main command line", as it does for the window
    // name of every other getter that takes an optional one
    const bool hasName = lua_gettop(L) > 0 && !lua_isnil(L, 1);
    if (hasName && !checkStringArg(L, __func__, 1, "command line name")) {
        return lua_error(L);
    }

    const QString name = hasName ? QString{lua_tostring(L, 1)} : qsl("main");
    const Host& host = getHostFromLua(L);

    if (auto styleSheet = host.mpConsole ? host.mpConsole->getCmdLineStyleSheet(name) : std::nullopt) {
        lua_pushstring(L, styleSheet->toUtf8().constData());
        return 1;
    }

    return warnArgumentValue(L, __func__, qsl("command-line name '%1' not found").arg(name));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFont
int TLuaInterpreter::setFont(lua_State* L)
{
    Host& host = getHostFromLua(L);

    const char* windowName = "";
    int s = 1;

    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }

    const QString fontName = getVerifiedString(L, __func__, s, "name");

    if (fontName.trimmed().isEmpty()) {
        return warnArgumentValue(L, __func__, "font must not be empty");
    }

    const auto resolved = host.resolveFontFamily(fontName);
    if (!resolved.available) {
        return warnArgumentValue(L, __func__, qsl("font '%1' is not available").arg(fontName));
    }

    const QString effectiveFontName = resolved.family;
    const QFont::Weight fontWeight = resolved.weight;

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
    // On GNU/Linux or FreeBSD ensure that emojis are displayed in colour even
    // if this font doesn't support it:
    QFont::insertSubstitution(effectiveFontName, qsl("Noto Color Emoji"));
#endif
    // For Qt 6.9+, emoji font support is handled globally in FontManager::addEmojiFont()
#endif

    // A console wins a name a label also carries - nothing stops a label being
    // called "main". Labels are not in the map CONSOLE() searches, so a name no
    // console answers to is tried as a label before that macro gets to refuse it:
    const QString targetName{windowName};
    auto console = CONSOLE_NIL(L, targetName);
    if (!console) {
        if (host.mpConsole) {
            if (TLabel* pLabel = host.mpConsole->mLabelMap.value(targetName)) {
                QFont labelFont = host.createFontWithSettings(effectiveFontName, pLabel->font().pointSize());
                if (fontWeight != QFont::Normal) {
                    labelFont.setWeight(fontWeight);
                }
                pLabel->setFont(labelFont);
                lua_pushboolean(L, true);
                return 1;
            }
        }
        console = CONSOLE(L, targetName);
    }

    if (console == host.mpConsole) {
        // apply changes to main console and its while-scrolling component too.
        QFont newFont = host.createFontWithSettings(effectiveFontName, host.getDisplayFont().pointSize());

        if (fontWeight != QFont::Normal) {
            newFont.setWeight(fontWeight);
        }

        auto result = host.setDisplayFont(newFont, Host::DisplayFontChange::UserChoice);

        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }

        console->refreshView();
    } else {
        QFont newFont = host.createFontWithSettings(effectiveFontName, console->font().pointSize());

        if (fontWeight != QFont::Normal) {
            newFont.setWeight(fontWeight);
        }

        console->setFont(newFont);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFontSize
int TLuaInterpreter::setFontSize(lua_State* L)
{
    Host& host = getHostFromLua(L);

    const char* windowName = "";
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }

    const int size = getVerifiedInt(L, __func__, s, "size");
    if (size <= 0) {
        // just throw an error, no default needed.
        return warnArgumentValue(L, __func__, "size cannot be 0 or negative");
    }

    auto console = CONSOLE(L, QString{windowName});
    if (console == host.mpConsole) {
        // get host profile display font and alter it, since that is how it's done in Settings.
        host.setDisplayFontSize(size);
    } else {
        console->setFontSize(size);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setItalics
int TLuaInterpreter::setItalics(lua_State* L)
{
    const char* windowName = "";
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable italic attribute");
    auto console = CONSOLE(L, QString{windowName});
    console->setDisplayAttributes(TChar::Italic, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelToolTip
int TLuaInterpreter::setLabelToolTip(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "label name") || !checkStringArg(L, __func__, 2, "text")) {
        return lua_error(L);
    }
    double duration = 0;
    if (lua_gettop(L) > 2) {
        duration = getVerifiedDouble(L, __func__, 3, "duration");
    }
    const QString labelName{lua_tostring(L, 1)};
    const QString labelToolTip{lua_tostring(L, 2)};

    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->setLabelToolTip(labelName, labelToolTip, duration); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLabelToolTip
int TLuaInterpreter::getLabelToolTip(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "a label cannot have an empty string as its name");
    }

    const Host& host = getHostFromLua(L);
    if (auto toolTip = host.mpConsole ? host.mpConsole->getLabelToolTip(labelName) : std::nullopt) {
        lua_pushstring(L, toolTip->toUtf8().constData());
        return 1;
    }

    return warnArgumentValue(L, __func__, qsl("label name '%1' not found").arg(labelName));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelClickCallback
int TLuaInterpreter::setLabelClickCallback(lua_State* L)
{
    return setLabelCallback(L, "setLabelClickCallback");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelDoubleClickCallback
int TLuaInterpreter::setLabelDoubleClickCallback(lua_State* L)
{
    return setLabelCallback(L, "setLabelDoubleClickCallback");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelMoveCallback
int TLuaInterpreter::setLabelMoveCallback(lua_State* L)
{
    return setLabelCallback(L, "setLabelMoveCallback");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnEnter
int TLuaInterpreter::setLabelOnEnter(lua_State* L)
{
    return setLabelCallback(L, "setLabelOnEnter");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnLeave
int TLuaInterpreter::setLabelOnLeave(lua_State* L)
{
    return setLabelCallback(L, "setLabelOnLeave");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelReleaseCallback
int TLuaInterpreter::setLabelReleaseCallback(lua_State* L)
{
    return setLabelCallback(L, "setLabelReleaseCallback");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelStyleSheet
int TLuaInterpreter::setLabelStyleSheet(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "label name") || !checkStringArg(L, __func__, 2, "stylesheet")) {
        return lua_error(L);
    }
    const QString labelName{lua_tostring(L, 1)};
    const QString stylesheet{lua_tostring(L, 2)};
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->setLabelStyleSheet(labelName, stylesheet); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setSvgTint
int TLuaInterpreter::setSvgTint(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "label name")) {
        return lua_error(L);
    }

    QColor color;
    if (lua_type(L, 2) == LUA_TSTRING) {
        const QString colorStr = getVerifiedString(L, __func__, 2, "color string");
        color = QColor(colorStr);
        if (!color.isValid()) {
            return warnArgumentValue(L, __func__, qsl("'%1' is not a valid color").arg(colorStr));
        }
    } else {
        const int r = getVerifiedInt(L, __func__, 2, "red value 0-255");
        const int g = getVerifiedInt(L, __func__, 3, "green value 0-255");
        const int b = getVerifiedInt(L, __func__, 4, "blue value 0-255");

        auto validRange = [](int number) {
            return number >= 0 && number <= 255;
        };

        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
        if (!validRange(g)) {
            return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
        }
        if (!validRange(b)) {
            return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
        }
        color = QColor(r, g, b);
    }

    const QString labelName{lua_tostring(L, 1)};

    Host& host = getHostFromLua(L);
    if (!host.setSvgTint(labelName, color)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetSvgTint
int TLuaInterpreter::resetSvgTint(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    Host& host = getHostFromLua(L);

    if (!host.resetSvgTint(labelName)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setSvgRotation
int TLuaInterpreter::setSvgRotation(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "label name")) {
        return lua_error(L);
    }
    const double angle = getVerifiedDouble(L, __func__, 2, "angle");
    const QString labelName{lua_tostring(L, 1)};
    Host& host = getHostFromLua(L);

    if (!host.setSvgRotation(labelName, angle)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetSvgRotation
int TLuaInterpreter::resetSvgRotation(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    Host& host = getHostFromLua(L);

    if (!host.resetSvgRotation(labelName)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setSvgShear
int TLuaInterpreter::setSvgShear(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "label name")) {
        return lua_error(L);
    }
    const double shearX = getVerifiedDouble(L, __func__, 2, "shearX");
    const double shearY = getVerifiedDouble(L, __func__, 3, "shearY");
    const QString labelName{lua_tostring(L, 1)};
    Host& host = getHostFromLua(L);

    if (!host.setSvgShear(labelName, shearX, shearY)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetSvgShear
int TLuaInterpreter::resetSvgShear(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    Host& host = getHostFromLua(L);

    if (!host.resetSvgShear(labelName)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetSvgTransform
int TLuaInterpreter::resetSvgTransform(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    Host& host = getHostFromLua(L);

    if (!host.resetSvgTransform(labelName)) {
        return warnArgumentValue(L, __func__, qsl("label '%1' not found").arg(labelName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelCursor
int TLuaInterpreter::setLabelCursor(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "label name")) {
        return lua_error(L);
    }
    const int labelCursor = getVerifiedInt(L, __func__, 2, "cursortype");
    const QString labelName{lua_tostring(L, 1)};
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->setLabelCursor(labelName, labelCursor); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelCustomCursor
int TLuaInterpreter::setLabelCustomCursor(lua_State* L)
{
    const int n = lua_gettop(L);
    int hotX = -1, hotY = -1;
    if (!checkStringArg(L, __func__, 1, "label name") || !checkStringArg(L, __func__, 2, "custom cursor location")) {
        return lua_error(L);
    }

    if (n > 2) {
        hotX = getVerifiedInt(L, __func__, 3, "hot spot x-coordinate");
        hotY = getVerifiedInt(L, __func__, 4, "hot spot y-coordinate");
    }

    const QString labelName{lua_tostring(L, 1)};
    const QString pixmapLocation{lua_tostring(L, 2)};

    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->setLabelCustomCursor(labelName, pixmapLocation, hotX, hotY); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelWheelCallback
int TLuaInterpreter::setLabelWheelCallback(lua_State* L)
{
    return setLabelCallback(L, "setLabelWheelCallback");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLink
int TLuaInterpreter::setLink(lua_State* L)
{
    const char* windowName = "main";
    int s = 0;
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, ++s);
    }

    int commandPos = ++s;
    if (!checkCommandOrFunctionArg(L, __func__, commandPos)) {
        return lua_error(L);
    }
    const int hintPos = ++s;
    if (!checkStringArg(L, __func__, hintPos, "tooltip")) {
        return lua_error(L);
    }

    // resolved before the parse, so a miss strands nothing - see releaseLuaReferences()
    const Host& host = getHostFromLua(L);
    auto console = CONSOLE(L, QString{windowName});

    QString command;
    int luaReference = 0;
    parseCommandOrFunction(L, __func__, commandPos, command, luaReference);

    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    commandList << command;
    hintList << QString{lua_tostring(L, hintPos)};
    luaReferences << luaReference;

    console->setLink(commandList, hintList, luaReferences);
    if (console != host.mpConsole) {
        console->mUpperPane->forceUpdate();
        console->mLowerPane->forceUpdate();
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMainWindowSize
int TLuaInterpreter::setMainWindowSize(lua_State* L)
{
    const int x1 = getVerifiedInt(L, __func__, 1, "mainWidth");
    const int y1 = getVerifiedInt(L, __func__, 2, "mainHeight");
    mudlet::self()->resize(x1, y1);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapWindowTitle
int TLuaInterpreter::setMapWindowTitle(lua_State* L)
{
    QString title;
    if (lua_gettop(L)) {
        title = getVerifiedString(L, __func__, 1, "title", true);
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.setMapperTitle(title); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapWindowTitle
int TLuaInterpreter::getMapWindowTitle(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (auto title = host.getMapperTitle()) {
        lua_pushstring(L, title->toUtf8().constData());
        return 1;
    }

    return warnArgumentValue(L, __func__, "no floating/dockable type map window found");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovie
int TLuaInterpreter::setMovie(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "label name")) {
        return lua_error(L);
    }
    if (const QString labelName{lua_tostring(L, 1)}; labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }
    if (!checkStringArg(L, __func__, 2, "movie (gif) path")) {
        return lua_error(L);
    }
    const QString labelName{lua_tostring(L, 1)};
    const QString moviePath{lua_tostring(L, 2)};

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.setMovie(labelName, moviePath); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovieFrame
int TLuaInterpreter::setMovieFrame(lua_State* L)
{
    return movieFunc(L, "setMovieFrame");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovieSpeed
int TLuaInterpreter::setMovieSpeed(lua_State* L)
{
    return movieFunc(L, "setMovieSpeed");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setOverline
int TLuaInterpreter::setOverline(lua_State* L)
{
    const char* windowName = "";
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable overline attribute");
    auto console = CONSOLE(L, QString{windowName});
    console->setDisplayAttributes(TChar::Overline, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setPopup
int TLuaInterpreter::setPopup(lua_State* L)
{
    const char* windowName = "main";
    int s = 0;
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, ++s);
    }

    int commandPos = ++s;
    int hintPos = ++s;
    if (!checkCommandsOrFunctionsTable(L, __func__, commandPos) || !checkHintsTable(L, __func__, hintPos)) {
        return lua_error(L);
    }

    // resolved before the parse, so a miss strands nothing - see releaseLuaReferences()
    const Host& host = getHostFromLua(L);
    auto console = CONSOLE(L, QString{windowName});

    QStringList commandList;
    QVector<int> luaReferences;
    parseCommandsOrFunctionsTable(L, __func__, commandPos, commandList, luaReferences);

    QStringList hintList;
    parseHintsTable(L, __func__, hintPos, hintList);

    if ((hintList.size() - commandList.size()) < 0 || (hintList.size() - commandList.size()) > 1) {
        releaseLuaReferences(L, luaReferences);
        lua_pushnil(L);
        lua_pushfstring(L,
                        "command table and hint table sizes do not match up (%d and %d, either they must be the same or there should be one extra hint) - cannot create popup",
                        commandList.size(),
                        hintList.size());
        return 2;
    }

    console->setLink(commandList, hintList, luaReferences);
    if (console != host.mpConsole) {
        console->mUpperPane->forceUpdate();
        console->mLowerPane->forceUpdate();
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAppStyleSheet
int TLuaInterpreter::setProfileStyleSheet(lua_State* L)
{
    const QString styleSheet = getVerifiedString(L, __func__, 1, "style sheet");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.setProfileStyleSheet(styleSheet));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setReverse
int TLuaInterpreter::setReverse(lua_State* L)
{
    const char* windowName = "";
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable reverse attribute");
    auto console = CONSOLE(L, QString{windowName});
    console->setDisplayAttributes(TChar::Reverse, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStrikeOut
int TLuaInterpreter::setStrikeOut(lua_State* L)
{
    const char* windowName = "";
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable strikeout attribute");
    auto console = CONSOLE(L, QString{windowName});
    console->setDisplayAttributes(TChar::StrikeOut, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextFormat
int TLuaInterpreter::setTextFormat(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    const int n = lua_gettop(L);

    // Every argument check below can raise a Lua error, and lua_error() longjmps
    // past C++ destructors - so nothing holding heap memory may be alive while they
    // run: the window name stays the Lua-owned string anchored at stack index 1 and
    // the colour components a plain array until the last check has passed. The
    // blinkMode QString further down is exempt only because it holds a
    // QStringLiteral until after the last raise; give it a computed default and the
    // leak comes back
    const char* windowNameCString = WINDOW_NAME(L, 1);

    std::array<int, 6> colorComponents{}; // 0-2 RGB background, 3-5 RGB foreground
    colorComponents[0] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 2, "red background color component"), 255.0));
    colorComponents[1] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 3, "green background color component"), 255.0));
    colorComponents[2] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 4, "blue background color component"), 255.0));
    colorComponents[3] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 5, "red foreground color component"), 255.0));
    colorComponents[4] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 6, "green foreground color component"), 255.0));
    colorComponents[5] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 7, "blue foreground color component"), 255.0));

    int s = 7;
    bool bold;
    if (lua_isboolean(L, ++s)) {
        bold = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        bold = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (bold format as boolean or number {true/non-zero to enable} expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool underline;
    if (lua_isboolean(L, ++s)) {
        underline = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        underline = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (underline format as boolean or number {true/non-zero to enable} expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool italics;
    if (lua_isboolean(L, ++s)) {
        italics = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        italics = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (italic format as boolean or number {true/non-zero to enable} expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool strikeout = false;
    if (s < n) {
        // s has not been incremented yet so this means we still have another argument!

        if (lua_isboolean(L, ++s)) {
            strikeout = lua_toboolean(L, s);
        } else if (lua_isnumber(L, s)) {
            strikeout = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
        } else {
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (strikeout format as boolean or number {true/non-zero to enable} is optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    bool overline = false;
    if (s < n) {
        // s has not been incremented yet so this means we still have another argument!
        if (lua_isboolean(L, ++s)) {
            overline = lua_toboolean(L, s);
        } else if (lua_isnumber(L, s)) {
            overline = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
        } else {
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (overline format as boolean or number {true/non-zero to enable} is optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    bool reverse = false;
    if (s < n) {
        // s has not been incremented yet so this means we still have another argument!
        if (lua_isboolean(L, ++s)) {
            reverse = lua_toboolean(L, s);
        } else if (lua_isnumber(L, s)) {
            reverse = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
        } else {
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (reverse format as boolean or number {true/non-zero to enable} is optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    QString blinkMode = qsl("none");

    if (s < n) {
        // s has not been incremented yet so this means we still have another argument!
        if (lua_isstring(L, ++s)) {
            blinkMode = lua_tostring(L, s);
            if (blinkMode != qsl("none") && blinkMode != qsl("slow") && blinkMode != qsl("fast")) {
                return warnArgumentValue(L, __func__, qsl("blink mode must be \"none\", \"slow\", or \"fast\", got \"%1\"").arg(blinkMode));
            }
        } else {
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (blink mode as string {\"none\"/\"slow\"/\"fast\"} is optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    const bool slowBlink = (blinkMode == qsl("slow"));
    const bool fastBlink = (blinkMode == qsl("fast"));

    TChar::AttributeFlags const flags = (bold ? TChar::Bold : TChar::None) | (italics ? TChar::Italic : TChar::None) | (overline ? TChar::Overline : TChar::None)
                                        | (reverse ? TChar::Reverse : TChar::None) | (strikeout ? TChar::StrikeOut : TChar::None) | (underline ? TChar::Underline : TChar::None)
                                        | (fastBlink ? TChar::FastBlink : (slowBlink ? TChar::Blink : TChar::None));

    const QString windowName{windowNameCString};
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value, true);
    }
    if (!host.mpConsole->setTextFormat(windowName, QColor(colorComponents[3], colorComponents[4], colorComponents[5]), QColor(colorComponents[0], colorComponents[1], colorComponents[2]), flags)) {
        return warnArgumentValue(L, __func__, qsl("window '%1' does not exist").arg(windowName), true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUnderline
int TLuaInterpreter::setUnderline(lua_State* L)
{
    const char* windowName = "";
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable underline attribute");
    auto console = CONSOLE(L, QString{windowName});
    console->setDisplayAttributes(TChar::Underline, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUserWindowTitle
int TLuaInterpreter::setUserWindowTitle(lua_State* L)
{
    const int n = lua_gettop(L);
    if (!checkStringArg(L, __func__, 1, "name")) {
        return lua_error(L);
    }
    if (n > 1 && !checkStringArg(L, __func__, 2, "title", true)) {
        return lua_error(L);
    }

    const QString name{lua_tostring(L, 1)};
    QString title;
    if (n > 1) {
        title = lua_tostring(L, 2);
    }

    const Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->setUserWindowTitle(name, title); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getUserWindowTitle
int TLuaInterpreter::getUserWindowTitle(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "name");
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }

    auto [success, result] = host.mpConsole->getUserWindowTitle(name);
    if (!success) {
        return warnArgumentValue(L, __func__, result);
    }

    lua_pushstring(L, result.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUserWindowStyleSheet
int TLuaInterpreter::setUserWindowStyleSheet(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "userwindow name") || !checkStringArg(L, __func__, 2, "StyleSheet")) {
        return lua_error(L);
    }
    const QString userWindowName{lua_tostring(L, 1)};
    const QString userWindowStyleSheet{lua_tostring(L, 2)};
    const Host& host = getHostFromLua(L);

    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, no_main_window_value);
    }
    if (auto [success, message] = host.mpConsole->setUserWindowStyleSheet(userWindowName, userWindowStyleSheet); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getUserWindowStyleSheet
int TLuaInterpreter::getUserWindowStyleSheet(lua_State* L)
{
    const QString userWindowName = getVerifiedString(L, __func__, 1, "userwindow name");
    if (userWindowName.isEmpty()) {
        return warnArgumentValue(L, __func__, "a userwindow cannot have an empty string as its name");
    }

    const Host& host = getHostFromLua(L);
    if (auto styleSheet = host.mpConsole ? host.mpConsole->getUserWindowStyleSheet(userWindowName) : std::nullopt) {
        lua_pushstring(L, styleSheet->toUtf8().constData());
        return 1;
    }

    return warnArgumentValue(L, __func__, qsl("userwindow name '%1' not found").arg(userWindowName));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindow
int TLuaInterpreter::setWindow(lua_State* L)
{
    const int n = lua_gettop(L);
    int x = 0, y = 0;
    bool show = true;

    const char* windownameArg = WINDOW_NAME(L, 1);

    if (lua_type(L, 2) != LUA_TSTRING) {
        lua_pushfstring(L, "setWindow: bad argument #2 type (element name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    if (n > 2) {
        x = getVerifiedInt(L, __func__, 3, "x-coordinate");
        y = getVerifiedInt(L, __func__, 4, "y-coordinate");
        show = getVerifiedBool(L, __func__, 5, "show element");
    }

    const QString windowname{windownameArg};
    const QString name{lua_tostring(L, 2)};

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.setWindow(windowname, name, x, y, show); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindowWrap
int TLuaInterpreter::setWindowWrap(lua_State* L)
{
    int s = 1;
    const char* windowName = "";
    if (lua_gettop(L) > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    const int luaFrom = getVerifiedInt(L, __func__, s, "wrapAt");
    auto console = CONSOLE(L, QString{windowName});
    if (luaFrom < 1) {
        // a width of zero or less cannot hold a single character, so nothing
        // could be displayed in such a window - the preferences dialog does not
        // offer these values either
        return warnArgumentValue(L, __func__, qsl("wrapAt must be greater than zero, got %1").arg(luaFrom));
    }
    console->setWrapAt(luaFrom);
    // only the main console's width belongs to the profile - it is what the
    // preferences dialog shows, what NEW-ENVIRON reports as WORD_WRAP and what
    // caps the width NAWS reports to the game
    if (console->getType() == TConsole::MainConsole) {
        Host& host = getHostFromLua(L);
        const int priorWrapAt = host.mWrapAt;
        host.mWrapAt = luaFrom;
        if (priorWrapAt != luaFrom) {
            host.mTelnet.sendInfoNewEnvironValue(qsl("WORD_WRAP"));
        }
        host.updateDisplayDimensions();
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindowWrapIndent
int TLuaInterpreter::setWindowWrapIndent(lua_State* L)
{
    const char* windowName = WINDOW_NAME(L, 1);
    const int luaFrom = getVerifiedInt(L, __func__, 2, "wrapTo");
    auto console = CONSOLE(L, QString{windowName});
    console->setIndentCount(luaFrom);
    if (luaFrom >= 0 && console->getType() == TConsole::MainConsole) {
        Host& host = getHostFromLua(L);
        host.mWrapIndentCount = luaFrom;
    }
    return 0;
}

//Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindowWrapHangingIndent
int TLuaInterpreter::setWindowWrapHangingIndent(lua_State* L)
{
    const char* windowName = WINDOW_NAME(L, 1);
    const int luaFrom = getVerifiedInt(L, __func__, 2, "wrapTo");
    auto console = CONSOLE(L, QString{windowName});
    console->setHangingIndentCount(luaFrom);
    if (luaFrom >= 0 && console->getType() == TConsole::MainConsole) {
        Host& host = getHostFromLua(L);
        host.mWrapHangingIndentCount = luaFrom;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showWindow
int TLuaInterpreter::showWindow(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.showWindow(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#startMovie
int TLuaInterpreter::startMovie(lua_State* L)
{
    return movieFunc(L, "startMovie");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showToolBar
int TLuaInterpreter::showToolBar(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);
    if (auto [moved, message] = host.getActionUnit()->showToolBar(windowName); !moved) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCommandBackgroundColor
int TLuaInterpreter::setCommandBackgroundColor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const char* windowNameArg = "";
    int r, alpha;
    int s = 1;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    if (lua_type(L, s) == LUA_TSTRING) {
        windowNameArg = WINDOW_NAME(L, s++);
        r = getVerifiedInt(L, __func__, s, "red value 0-255");
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else if (lua_isnumber(L, s)) {
        r = static_cast<int>(lua_tonumber(L, s));
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (window name as string, or red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    const int g = getVerifiedInt(L, __func__, ++s, "green value 0-255");
    if (!validRange(g)) {
        return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
    }

    const int b = getVerifiedInt(L, __func__, ++s, "blue value 0-255");
    if (!validRange(b)) {
        return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
    }

    // if we get nothing for the alpha value, assume it is 255. If we get a non-number value, complain.
    alpha = 255;
    if (lua_gettop(L) > s) {
        alpha = getVerifiedInt(L, __func__, ++s, "alpha value 0-255", true);
        if (!validRange(alpha)) {
            return warnArgumentValue(L, __func__, csmInvalidAlphaValue.arg(alpha));
        }
    }

    const QString windowName{windowNameArg};
    if (isMain(windowName)) {
        host.mCommandBgColor.setRgb(r, g, b, alpha);
        if (host.mpConsole) {
            host.mpConsole->setCommandBgColor(r, g, b, alpha);
        }
    } else if (!host.setCommandBackgroundColor(windowName, r, g, b, alpha)) {
        return warnArgumentValue(L, __func__, qsl("window/label '%1' not found").arg(windowName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCommandForegroundColor
int TLuaInterpreter::setCommandForegroundColor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const char* windowNameArg = "";
    int r, alpha;
    int s = 1;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    if (lua_type(L, s) == LUA_TSTRING) {
        windowNameArg = WINDOW_NAME(L, s++);
        r = getVerifiedInt(L, __func__, s, "red value 0-255");
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else if (lua_isnumber(L, s)) {
        r = static_cast<int>(lua_tonumber(L, s));
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
        }
    } else {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (window name as string, or red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    const int g = getVerifiedInt(L, __func__, ++s, "green value 0-255");
    if (!validRange(g)) {
        return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
    }

    const int b = getVerifiedInt(L, __func__, ++s, "blue value 0-255");
    if (!validRange(b)) {
        return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
    }

    // if we get nothing for the alpha value, assume it is 255. If we get a non-number value, complain.
    alpha = 255;
    if (lua_gettop(L) > s) {
        alpha = getVerifiedInt(L, __func__, ++s, "alpha value 0-255", true);
        if (!validRange(alpha)) {
            return warnArgumentValue(L, __func__, csmInvalidAlphaValue.arg(alpha));
        }
    }

    const QString windowName{windowNameArg};
    if (isMain(windowName)) {
        host.mCommandFgColor.setRgb(r, g, b, alpha);
        if (host.mpConsole) {
            host.mpConsole->setCommandFgColor(r, g, b, alpha);
        }
    } else if (!host.setCommandForegroundColor(windowName, r, g, b, alpha)) {
        return warnArgumentValue(L, __func__, qsl("window/label '%1' not found").arg(windowName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#scrollTo
int TLuaInterpreter::scrollTo(lua_State* L)
{
    QString windowName;
    int targetLine = -1;
    bool stopScrolling = false;

    const int n = lua_gettop(L);
    if (n == 2) {
        if (!checkStringArg(L, __func__, 1, "window name", true)) {
            return lua_error(L);
        }
        targetLine = getVerifiedInt(L, __func__, 2, "line to scroll to");
        windowName = lua_tostring(L, 1);
    } else if (n == 1) {
        if (lua_isnumber(L, 1)) {
            targetLine = getVerifiedInt(L, __func__, 1, "line to scroll to");
            windowName = qsl("main");
        } else {
            windowName = getVerifiedString(L, __func__, 1, "window name", true);
            stopScrolling = true;
        }
    } else if (n == 0) {
        windowName = qsl("main");
        stopScrolling = true;
    }

    auto console = getHostFromLua(L).findConsole(windowName);
    if (!console) {
        lua_pushnil(L);
        lua_pushfstring(L, bad_window_value, windowName.toUtf8().constData());
        return 2;
    }

    const int numLines = console->getLastLineNumber();
    if (targetLine >= numLines) { // larger than buffer or at end
        stopScrolling = true;
    } else if (targetLine < 0) { // negative, count from end of buffer
        targetLine = std::max((numLines + targetLine), 0);
    }

    if (stopScrolling) {
        if (!console->mUpperPane->mIsTailMode) {
            console->mLowerPane->mCursorY = console->buffer.size();
            console->mLowerPane->hide();
            console->buffer.mCursorY = console->buffer.size();
            console->mUpperPane->mCursorY = console->buffer.size();
            console->mUpperPane->mCursorX = 0;
            console->mUpperPane->mIsTailMode = true;
            console->mUpperPane->updateScreenView();
            console->mUpperPane->forceUpdate();
        }
    } else {
        console->scrollUp(console->mUpperPane->mCursorY - targetLine);
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#windowType
int TLuaInterpreter::windowType(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const QString windowName = getVerifiedString(L, __func__, 1, "window name");

    if (auto kind = host.windowType(windowName)) {
        lua_pushstring(L, kind->toUtf8().constData());
        return 1;
    }

    lua_pushnil(L);
    lua_pushfstring(L, "'%s' is not a known label, any type of console, command line, text edit, nor scroll box", windowName.toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#wrapLine
int TLuaInterpreter::wrapLine(lua_State* L)
{
    const bool hasWindowName = (lua_gettop(L) != 0);
    if (hasWindowName && !checkStringArg(L, __func__, 1, "window name")) {
        return lua_error(L);
    }
    const int lineNumber = getVerifiedInt(L, __func__, hasWindowName ? 2 : 1, "line");
    QString windowName = hasWindowName ? QString{lua_tostring(L, 1)} : qsl("main");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        // sub-windows die with the view, but the main window's buffer is the
        // model's and still holds the wrap settings the view was using
        if (isMain(windowName)) {
            TBuffer& buffer = host.mainConsoleModel().buffer;
            buffer.wrapLine(lineNumber, buffer.mWrapAt, buffer.mWrapIndent, buffer.mWrapHangingIndent);
        }
        return 0;
    }
    host.mpConsole->luaWrapLine(windowName, lineNumber);
    return 0;
}

// No Documentation - public function but should stay undocumented -- compare https://github.com/Mudlet/Mudlet/issues/1149
int TLuaInterpreter::pasteWindow(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "pasteWindow: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    const QString windowName{WINDOW_NAME(L, 1)};
    Host& host = getHostFromLua(L);
    host.pasteWindow(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScrolling
int TLuaInterpreter::enableScrolling(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    if (windowName.compare(qsl("main"), Qt::CaseSensitive) == 0) {
        return warnArgumentValue(L, __func__, "scrolling cannot be enabled/disabled for the 'main' window");
    }

    auto console = CONSOLE(L, windowName);
    console->setScrolling(true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScrolling
int TLuaInterpreter::disableScrolling(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    if (windowName.compare(qsl("main"), Qt::CaseSensitive) == 0) {
        return warnArgumentValue(L, __func__, "scrolling cannot be enabled/disabled for the 'main' window");
    }

    auto console = CONSOLE(L, windowName);
    console->setScrolling(false);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#scrollingActive
int TLuaInterpreter::scrollingActive(lua_State* L)
{
    const QString windowName{WINDOW_NAME(L, 1)};
    if (windowName.compare(qsl("main"), Qt::CaseSensitive) == 0) {
        // Handle the main console case:
        lua_pushboolean(L, true);
        return 1;
    }

    auto console = CONSOLE(L, windowName);
    lua_pushboolean(L, console->getScrolling());
    return 1;
}

// No documentation available in wiki - internal function
// funcName is not a QString because a QByteArray made from one would still be
// alive inside the raising checks below - see checkStringArg()
int TLuaInterpreter::movieFunc(lua_State* L, const char* funcName)
{
    if (!checkStringArg(L, funcName, 1, "label name")) {
        return lua_error(L);
    }
    const QLatin1StringView func{funcName};

    TLabel* pN = nullptr;
    QMovie* movie = nullptr;
    {
        const QString labelName{lua_tostring(L, 1)};
        if (labelName.isEmpty()) {
            return warnArgumentValue(L, __func__, "label name cannot be an empty string");
        }
        pN = LABEL(L, labelName);
        movie = pN->movie();
        if (!movie) {
            return warnArgumentValue(L, __func__, qsl("no movie found at label '%1'").arg(labelName));
        }
    }

    if (func == qsl("startMovie")) {
        movie->start();
    } else if (func == qsl("pauseMovie")) {
        movie->setPaused(true);
    } else if (func == qsl("setMovieFrame")) {
        if (!checkIntArg(L, funcName, 2, "movie frame number")) {
            return lua_error(L);
        }
        lua_pushboolean(L, movie->jumpToFrame(static_cast<int>(lua_tointeger(L, 2))));
        return 1;
    } else if (func == qsl("setMovieSpeed")) {
        if (!checkIntArg(L, funcName, 2, "movie playback speed in %")) {
            return lua_error(L);
        }
        movie->setSpeed(static_cast<int>(lua_tointeger(L, 2)));
    } else if (func == qsl("scaleMovie")) {
        bool autoScale{true};
        const int n = lua_gettop(L);
        if (n > 1) {
            if (!checkBoolArg(L, funcName, 2, "activate/deactivate scaling movie", true)) {
                return lua_error(L);
            }
            autoScale = lua_toboolean(L, 2);
        }
        movie->setScaledSize(pN->size());
        if (autoScale) {
            connect(pN, &TLabel::resized, movie, [=] {
                movie->setScaledSize(pN->size());
            });
        } else {
            // only drop the movie-scaling connection(s); other consumers of
            // the label's resized signal must stay connected
            QObject::disconnect(pN, &TLabel::resized, movie, nullptr);
        }
    } else {
        return warnArgumentValue(L, __func__, qsl("'%1' is not a known function name - bug in Mudlet, please report it").arg(funcName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCommand
int TLuaInterpreter::addCommand(lua_State* L)
{
    if (!lua_istable(L, 1)) {
        return warnArgumentValue(L, __func__, "addCommand needs a table, e.g. addCommand{name = 'Speech', menuPath = 'Speech'}");
    }

    mudlet::CommandRequest request;
    // Leaving a field out and giving it the wrong type are different mistakes:
    // the first asks for nothing, the second asks for something and is ignored.
    // menuPath is the one that bites, because the path is conceptually a list
    // and the sibling surfaces field really does take one, so menuPath =
    // {"Speech", "Voices"} is an easy thing to write and used to place the
    // command at the top of Extensions without a word. Numbers are left alone:
    // lua_isstring() accepts them, so shortcut = 12345 still reaches the
    // sequence parser and is refused there for what it actually is.
    QString wrongField;
    QString wrongType;
    auto stringField = [&](const char* field) -> QString {
        lua_getfield(L, 1, field);
        QString value;
        if (lua_isstring(L, -1)) {
            value = QString::fromUtf8(lua_tostring(L, -1));
        } else if (!lua_isnoneornil(L, -1) && wrongField.isEmpty()) {
            wrongField = QString::fromUtf8(field);
            wrongType = QString::fromUtf8(luaL_typename(L, -1));
        }
        lua_pop(L, 1);
        return value;
    };

    request.name = stringField("name");
    request.icon = stringField("icon");
    request.tooltip = stringField("tooltip");
    request.menuPath = stringField("menuPath");
    request.shortcut = stringField("shortcut");
    if (!wrongField.isEmpty()) {
        return warnArgumentValue(L, __func__, qsl("%1 has to be a string and this one is a %2").arg(wrongField, wrongType));
    }
    if (request.name.isEmpty()) {
        return warnArgumentValue(L, __func__, "a command needs a name to show");
    }

    // surfaces is a list rather than a single word, so a client that grows
    // another surface takes another entry rather than a new spelling of "both".
    // A bare string is accepted for the one-surface case.
    bool wantsMenu = false;
    bool wantsToolbar = false;
    bool named = false;
    auto nameSurface = [&](const QString& surface) -> bool {
        named = true;
        if (surface == qsl("menu")) {
            wantsMenu = true;
        } else if (surface == qsl("toolbar")) {
            wantsToolbar = true;
        } else {
            return false;
        }
        return true;
    };

    lua_getfield(L, 1, "surfaces");
    if (lua_isstring(L, -1)) {
        const QString surface = QString::fromUtf8(lua_tostring(L, -1));
        if (!nameSurface(surface)) {
            lua_pop(L, 1);
            return warnArgumentValue(L, __func__, qsl("'%1' is not a surface this client has - use 'menu' or 'toolbar'").arg(surface));
        }
    } else if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            // A list of names, so a key/value table such as {menu = true} is a
            // mistake worth naming by type: a boolean has no string form, so
            // quoting the value would send the package looking for a surface
            // named by the empty string
            if (!lua_isstring(L, -1)) {
                const QString type = QString::fromUtf8(luaL_typename(L, -1));
                lua_pop(L, 3);
                return warnArgumentValue(L, __func__, qsl("surfaces has to be a list of surface names and this one holds a %1 - use surfaces = {'menu', 'toolbar'}").arg(type));
            }
            const QString surface = QString::fromUtf8(lua_tostring(L, -1));
            if (!nameSurface(surface)) {
                lua_pop(L, 3);
                return warnArgumentValue(L, __func__, qsl("'%1' is not a surface this client has - use 'menu' or 'toolbar'").arg(surface));
            }
            lua_pop(L, 1);
        }
        // An empty list asks for the command to go nowhere, which no package
        // can have meant - and silently treating it as "both" would place a
        // command in the two places it just said it did not want
        if (!named) {
            lua_pop(L, 1);
            return warnArgumentValue(L, __func__, "surfaces is empty, so there is nowhere to put the command - name 'menu', 'toolbar', or leave surfaces out for both");
        }
    } else if (!lua_isnoneornil(L, -1)) {
        const QString type = QString::fromUtf8(luaL_typename(L, -1));
        lua_pop(L, 1);
        return warnArgumentValue(L, __func__, qsl("surfaces has to be a surface name or a list of them, not a %1 - use 'menu' or 'toolbar'").arg(type));
    }
    lua_pop(L, 1);

    // Leaving surfaces out means "wherever this client puts commands", which is
    // both. Naming it and naming nothing in it is refused above.
    if (!named) {
        request.surfaces = mudlet::CommandSurface::Both;
    } else if (wantsMenu && wantsToolbar) {
        request.surfaces = mudlet::CommandSurface::Both;
    } else if (wantsToolbar) {
        request.surfaces = mudlet::CommandSurface::Toolbar;
    } else {
        request.surfaces = mudlet::CommandSurface::Menu;
    }

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    QString error;
    const int commandId = pMudlet->addAddonCommand(request, &host, error);
    if (commandId < 0) {
        return warnArgumentValue(L, __func__, error.isEmpty() ? qsl("the command could not be placed") : error);
    }

    lua_pushinteger(L, commandId);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCommand
int TLuaInterpreter::removeCommand(lua_State* L)
{
    const int commandId = getVerifiedInt(L, __func__, 1, "commandId");

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    lua_pushboolean(L, pMudlet->removeAddonCommand(commandId, &host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableCommand
int TLuaInterpreter::enableCommand(lua_State* L)
{
    const int commandId = getVerifiedInt(L, __func__, 1, "commandId");

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    lua_pushboolean(L, pMudlet->setAddonCommandEnabled(commandId, true, &host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableCommand
int TLuaInterpreter::disableCommand(lua_State* L)
{
    const int commandId = getVerifiedInt(L, __func__, 1, "commandId");

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    lua_pushboolean(L, pMudlet->setAddonCommandEnabled(commandId, false, &host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCommandChecked
int TLuaInterpreter::setCommandChecked(lua_State* L)
{
    const int commandId = getVerifiedInt(L, __func__, 1, "commandId");
    const bool checked = getVerifiedBool(L, __func__, 2, "checked");

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    lua_pushboolean(L, pMudlet->setAddonCommandChecked(commandId, checked, &host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCommandIcon
int TLuaInterpreter::setCommandIcon(lua_State* L)
{
    const int commandId = getVerifiedInt(L, __func__, 1, "commandId");
    const QString icon = getVerifiedString(L, __func__, 2, "icon");

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    lua_pushboolean(L, pMudlet->setAddonCommandIcon(commandId, icon, &host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCommandTooltip
int TLuaInterpreter::setCommandTooltip(lua_State* L)
{
    const int commandId = getVerifiedInt(L, __func__, 1, "commandId");
    const QString tooltip = getVerifiedString(L, __func__, 2, "tooltip");

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    lua_pushboolean(L, pMudlet->setAddonCommandTooltip(commandId, tooltip, &host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCommandPulse
int TLuaInterpreter::setCommandPulse(lua_State* L)
{
    const int commandId = getVerifiedInt(L, __func__, 1, "commandId");
    const bool enabled = getVerifiedBool(L, __func__, 2, "enabled");

    QString color1 = qsl("#ff4444");
    QString color2 = qsl("#cc0000");
    int interval = 500;

    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
        color1 = getVerifiedString(L, __func__, 3, "color1");
    }
    if (lua_gettop(L) >= 4 && !lua_isnil(L, 4)) {
        color2 = getVerifiedString(L, __func__, 4, "color2");
    }
    if (lua_gettop(L) >= 5 && !lua_isnil(L, 5)) {
        interval = getVerifiedInt(L, __func__, 5, "interval");
        if (interval < 1) {
            return warnArgumentValue(L, __func__, qsl("interval must be greater than zero, got %1").arg(interval));
        }
    }

    auto& host = getHostFromLua(L);
    mudlet* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    QString error;
    const bool success = pMudlet->setAddonCommandPulse(commandId, enabled, color1, color2, interval, &host, error);
    if (!success && !error.isEmpty()) {
        return warnArgumentValue(L, __func__, error);
    }
    lua_pushboolean(L, success);
    return 1;
}
