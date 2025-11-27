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
#if defined(INCLUDE_3DMAPPER)
#include "glwidget_integration.h"
#endif

#include <limits>
#include <math.h>

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

static const char *bad_window_type = "%s: bad argument #%d type (window name as string expected, got %s)!";
static const char *bad_cmdline_type = "%s: bad argument #%d type (command line name as string expected, got %s)!";
static const char *bad_window_value = "window \"%s\" not found";
static const char *bad_cmdline_value = "command line \"%s\" not found";
static const char *bad_label_value = "label \"%s\" not found";

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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCommandLineMenuEvent
int TLuaInterpreter::addCommandLineMenuEvent(lua_State * L)
{
    int args = 1;
    const int argsCount = lua_gettop(L);

    QString commandLineName;
    if (argsCount >= 3) {
        commandLineName = getVerifiedString(L, __func__, args++, "command line name");
    } else {
        commandLineName = qsl("main");
    }
    auto menuLabel = getVerifiedString(L, __func__, args++, "menu label");
    auto eventName = getVerifiedString(L, __func__, args++, "event name");

    const auto& commandline = COMMANDLINE(L, commandLineName);
    commandline->contextMenuItems.insert(menuLabel, eventName);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMouseEvent
int TLuaInterpreter::addMouseEvent(lua_State * L)
{
    Host& host = getHostFromLua(L);
    QStringList actionInfo;
    const QString uniqueName = getVerifiedString(L, __func__, 1, "uniquename");
    if (host.mConsoleActions.contains(uniqueName)) {
        return warnArgumentValue(L, __func__, qsl("mouse event '%1' already exists").arg(uniqueName));
    }

    actionInfo << getVerifiedString(L, __func__, 2, "event name", false);

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
    const QString windowName {WINDOW_NAME(L, 1)};
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
        auto font = QFont(getVerifiedString(L, __func__, 2, "font name"),
                          getVerifiedInt(L, __func__, 1, "font size"), QFont::Normal);
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
    QString windowName = QLatin1String("main");
    const int n = lua_gettop(L);
    int counter = 1;

    if (n > 5) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "createCommandLine: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        windowName = lua_tostring(L, 1);
        counter++;
        if (isMain(windowName)) {
            // createCommandLine only accepts the empty name as the main window
            windowName.clear();
        }
    }

    if (lua_type(L, counter) != LUA_TSTRING) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandLine name as string expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    }
    const QString commandLineName{lua_tostring(L, counter)};
    counter++;
    const int x = getVerifiedInt(L, __func__, counter, "commandline x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "commandline y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "commandline width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "commandline height");
    counter++;

    const Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->createCommandLine(windowName, commandLineName, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createLabel
int TLuaInterpreter::createLabel(lua_State* L)
{
    QString labelName;
    QString windowName = QLatin1String("main");

    if (lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "createLabel: bad argument #1 type (label or parent window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if ((lua_type(L, 1) == LUA_TSTRING) && (lua_type(L, 2) == LUA_TSTRING)) {
        windowName = lua_tostring(L, 1);
        labelName = lua_tostring(L, 2);
        createLabelUserWindow(L, windowName, labelName);
    } else if ((lua_type(L, 1) == LUA_TSTRING) && (lua_type(L, 2) == LUA_TNUMBER)) {
        labelName = lua_tostring(L, 1);
        createLabelMainWindow(L, labelName);
    } else {
        lua_pushfstring(L, "createLabel: bad argument #2 type (label name as string or label x-coordinate as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMiniConsole
int TLuaInterpreter::createMiniConsole(lua_State* L)
{
    QString name = "";
    int counter = 3;
    //make the windowname optional by using counter. If windowname "main" add to main console

    QString windowName = getVerifiedString(L, __func__, 1, "miniconsole name");
    if (isMain(windowName)) {
        // createMiniConsole only accepts the empty name as the main window
        windowName.clear();
    }

    if (!lua_isnumber(L, 2) && lua_gettop(L) >= 2) {
        name = getVerifiedString(L, __func__, 2, "miniconsole name");
    } else {
        name = windowName;
        windowName.clear();
        counter = 2;
    }

    const int x = getVerifiedInt(L, __func__, counter, "miniconsole x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "miniconsole y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "miniconsole width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "miniconsole height");

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
    QString name = "";
    int counter = 3;
    // make the windowname optional by using counter. If windowname "main" - add to main console

    QString windowName = getVerifiedString(L, __func__, 1, "scrollBox name");
    if (isMain(windowName)) {
        // createScrollBox only accepts the empty name as the main window
        windowName.clear();
    }

    if (!lua_isnumber(L, 2) && lua_gettop(L) >= 2) {
        name = getVerifiedString(L, __func__, 2, "scrollBox name");
    } else {
        name = windowName;
        windowName.clear();
        counter = 2;
    }

    const int x = getVerifiedInt(L, __func__, counter, "scrollBox x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "scrollBox y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "scrollBox width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "scrollBox height");

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

    if (auto [success, message] = host.mpConsole->deleteCommandLine(commandLineName); !success) {
        lua_pushboolean(L, false);
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteScrollBox
int TLuaInterpreter::deleteScrollBox(lua_State* L)
{
    const QString scrollBoxName = getVerifiedString(L, __func__, 1, "scrollbox name");
    const Host& host = getHostFromLua(L);

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
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->skipLine();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deselect
int TLuaInterpreter::deselect(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->deselect();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableClickthrough
int TLuaInterpreter::disableClickthrough(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

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
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setHorizontalScrollBar(false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScrollBar
int TLuaInterpreter::disableScrollBar(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setScrollBarVisible(false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTimeStamps
int TLuaInterpreter::disableTimeStamps(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
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
    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    const int n = lua_gettop(L);
    int s = 0;
    int luaReference = 0;
    bool useCurrentFormat = false;
    QString windowName = qsl("main");
    QString hint;
    QString command;
    QString text;

    if (n < 4) {
        // (string) text, (string) command/function, (string) hint
        text = getVerifiedString(L, __func__, ++s, "text");
        parseCommandOrFunction(L, __func__, ++s, command, luaReference);
        hint = getVerifiedString(L, __func__, ++s, "hint");

    } else {
        if (n == 4) {
            // EITHER: (string) text, (string) command/function, (string) hint, (bool) standard/NotDefaultFormat
            //     OR: (string) windowName, (string) text, (string) command/function, (string) hint
            if (!lua_isboolean(L, 4)) {
                windowName = getVerifiedString(L, __func__, ++s, "window name");
            }
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandOrFunction(L, __func__, ++s, command, luaReference);
            hint = getVerifiedString(L, __func__, ++s, "hint");
            if (lua_isboolean(L, 4)) {
                useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
            }
        } else {
            // n > 4:
            // (string) windowName, (string) text, (string) command/function, (string) hint, (bool) standard/NotDefaultFormat
            windowName = getVerifiedString(L, __func__, ++s, "window name");
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandOrFunction(L, __func__, ++s, command, luaReference);
            hint = getVerifiedString(L, __func__, ++s, "hint");
            useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
        }
    }

    commandList << command;
    luaReferences << luaReference;
    hintList << hint;

    auto console = CONSOLE(L, windowName);
    console->echoLink(text, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoUserWindow
int TLuaInterpreter::echoUserWindow(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    const QString text = getVerifiedString(L, __func__, 2, "text");
    Host& host = getHostFromLua(L);
    host.echoWindow(windowName, text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoPopup
int TLuaInterpreter::echoPopup(lua_State* L)
{
    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    const int n = lua_gettop(L);
    int s = 0;
    bool useCurrentFormat = false;
    QString windowName = qsl("main");
    QString text;

    if (n < 4) {
        // (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}
        text = getVerifiedString(L, __func__, ++s, "text");
        parseCommandsOrFunctionsTable(L, __func__, ++s, commandList, luaReferences);
        parseHintsTable(L, __func__, ++s, hintList);

    } else {
        if (n == 4) {
            // EITHER: (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}, (bool) standard/NotDefaultFormat
            //     OR: (string) windowName, (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}
            if (!lua_isboolean(L, 4)) {
                windowName = getVerifiedString(L, __func__, ++s, "window name");
            }
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandsOrFunctionsTable(L, __func__, ++s, commandList, luaReferences);
            parseHintsTable(L, __func__, ++s, hintList);
            if (lua_isboolean(L, 4)) {
                useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
            }
        } else {
            // n > 4:
            // (string) windowName, (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}, (bool) standard/NotDefaultFormat
            windowName = getVerifiedString(L, __func__, ++s, "window name");
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandsOrFunctionsTable(L, __func__, ++s, commandList, luaReferences);
            parseHintsTable(L, __func__, ++s, hintList);
            useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
        }
    }

    if ((hintList.size() - commandList.size()) < 0 || (hintList.size() - commandList.size()) > 1) {
        lua_pushnil(L);
        lua_pushfstring(L, "command table and hint table sizes do not match up (%d and %d, either they must be the same or there should be one extra hint) - cannot create popup", commandList.size(), hintList.size());
        return 2;
    }

    auto console = CONSOLE(L, windowName);
    console->echoLink(text, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableClickthrough
int TLuaInterpreter::enableClickthrough(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);

    host.setClickthrough(windowName, true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLinkStyle
int TLuaInterpreter::setLinkStyle(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    const QString linkColor = getVerifiedString(L, __func__, 2, "link color", true);
    const QString linkVisitedColor = getVerifiedString(L, __func__, 3, "link visited color", true);
    const bool underline = (lua_gettop(L) >= 4) ? getVerifiedBool(L, __func__, 4, "underline", true) : true;

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
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setHorizontalScrollBar(true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScrollBar
int TLuaInterpreter::enableScrollBar(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setScrollBarVisible(true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableTimeStamps
int TLuaInterpreter::enableTimeStamps(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
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
    const Host& host = getHostFromLua(L);
    QColor color;

    QString windowName = qsl("main");
    const int n = lua_gettop(L);
    if (n > 0) {
        windowName = getVerifiedString(L, __func__, 1, "window name");
    }

    if (isMain(windowName)) {
        color = host.mpConsole->getConsoleBgColor();
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
    std::list<int> const result = host.mpConsole->getBgColor(windowName);
    for (const int pos : result) {
        lua_pushnumber(L, pos);
    }
    return result.size();
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderBottom
int TLuaInterpreter::getBorderBottom(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.borders().bottom());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderLeft
int TLuaInterpreter::getBorderLeft(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.borders().left());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderRight
int TLuaInterpreter::getBorderRight(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.borders().right());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderSizes
int TLuaInterpreter::getBorderSizes(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    auto sizes = host.borders();
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
    lua_pushnumber(L, host.borders().top());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getClipboardText
int TLuaInterpreter::getClipboardText(lua_State* L)
{
    QClipboard* clipboard = QApplication::clipboard();
    lua_pushstring(L, clipboard->text().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getColumnCount
int TLuaInterpreter::getColumnCount(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

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
    const QString windowName {WINDOW_NAME(L, 1)};
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
    std::list<int> const result = host.mpConsole->getFgColor(windowName);
    for (const int pos : result) {
        lua_pushnumber(L, pos);
    }
    return result.size();
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFont
int TLuaInterpreter::getFont(lua_State* L)
{
    QString windowName = qsl("main");
    windowName = WINDOW_NAME(L, 1);
    auto console = CONSOLE(L, windowName);
    Host& host = getHostFromLua(L);

    auto actualFontFamily = [](const QFont& font) -> QString {
        return QFontInfo(font).family();
    };

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
    const QString windowName {WINDOW_NAME(L, 1)};
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

    auto size = host.mpConsole->getLabelSizeHint(labelName);
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
    if (auto stylesheet = host.mpConsole->getLabelStyleSheet(label)) {
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
    const QString windowName {WINDOW_NAME(L, 1)};
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
    QString windowName;
    if (n > 2) {
        windowName = getVerifiedString(L, __func__, s++, "mini console, user window or buffer name {may be omitted for the \"main\" console}", true);
    }
    const int lineFrom = getVerifiedInt(L, __func__, s++, "start line");
    const int lineTo = getVerifiedInt(L, __func__, s, "end line");

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
int TLuaInterpreter::getMouseEvents(lua_State * L)
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
    const QSize mainWindowSize = host.mpConsole->getMainWindowSize();

    lua_pushnumber(L, mainWindowSize.width());
    lua_pushnumber(L, mainWindowSize.height());

    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRowCount
int TLuaInterpreter::getRowCount(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

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
    const QString windowName {WINDOW_NAME(L, 1)};
    auto pConsole = CONSOLE(L, windowName);
    // *pConsole can be the main console as well as any user one
    lua_pushboolean(L, pConsole->showTimeStamps());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getUserWindowSize
int TLuaInterpreter::getUserWindowSize(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

    const Host& host = getHostFromLua(L);
    const QSize userWindowSize = host.mpConsole->getUserWindowSize(windowName);
    lua_pushnumber(L, userWindowSize.width());
    lua_pushnumber(L, userWindowSize.height());

    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getWindowWrap
int TLuaInterpreter::getWindowWrap(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getWrapAt());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasFocus
int TLuaInterpreter::hasFocus(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->hasFocus()); //FIXME
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hideToolBar
int TLuaInterpreter::hideToolBar(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);
    host.getActionUnit()->hideToolBar(windowName);
    return 0;
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
    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    const int n = lua_gettop(L);
    int s = 0;
    int luaReference = 0;
    bool useCurrentFormat = false;
    QString windowName = qsl("main");
    QString hint;
    QString command;
    QString text;

    if (n < 4) {
        // (string) text, (string) command/function, (string) hint
        text = getVerifiedString(L, __func__, ++s, "text");
        parseCommandOrFunction(L, __func__, ++s, command, luaReference);
        hint = getVerifiedString(L, __func__, ++s, "hint");

    } else {
        if (n == 4) {
            // EITHER: (string) text, (string) command/function, (string) hint, (bool) standard/NotDefaultFormat
            //     OR: (string) windowName, (string) text, (string) command/function, (string) hint
            if (!lua_isboolean(L, 4)) {
                windowName = getVerifiedString(L, __func__, ++s, "window name");
            }
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandOrFunction(L, __func__, ++s, command, luaReference);
            hint = getVerifiedString(L, __func__, ++s, "hint");
            if (lua_isboolean(L, 4)) {
                useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
            }
        } else {
            // n > 4:
            // (string) windowName, (string) text, (string) command/function, (string) hint, (bool) standard/NotDefaultFormat
            windowName = getVerifiedString(L, __func__, ++s, "window name");
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandOrFunction(L, __func__, ++s, command, luaReference);
            hint = getVerifiedString(L, __func__, ++s, "hint");
            useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
        }
    }

    commandList << command;
    luaReferences << luaReference;
    hintList << hint;

    auto console = CONSOLE(L, windowName);
    console->insertLink(text, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertPopup
int TLuaInterpreter::insertPopup(lua_State* L)
{
    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    const int n = lua_gettop(L);
    int s = 0;
    bool useCurrentFormat = false;
    QString windowName = qsl("main");
    QString text;

    if (n < 4) {
        // (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}
        text = getVerifiedString(L, __func__, ++s, "text");
        parseCommandsOrFunctionsTable(L, __func__, ++s, commandList, luaReferences);
        parseHintsTable(L, __func__, ++s, hintList);

    } else {
        if (n == 4) {
            // EITHER: (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}, (bool) standard/NotDefaultFormat
            //     OR: (string) windowName, (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}
            if (!lua_isboolean(L, 4)) {
                windowName = getVerifiedString(L, __func__, ++s, "window name");
            }
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandsOrFunctionsTable(L, __func__, ++s, commandList, luaReferences);
            parseHintsTable(L, __func__, ++s, hintList);
            if (lua_isboolean(L, 4)) {
                useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
            }
        } else {
            // n > 4:
            // (string) windowName, (string) text, {table of (string) / (functions) commands}, {table of (strings) hints}, (bool) standard/NotDefaultFormat
            windowName = getVerifiedString(L, __func__, ++s, "window name");
            text = getVerifiedString(L, __func__, ++s, "text");
            parseCommandsOrFunctionsTable(L, __func__, ++s, commandList, luaReferences);
            parseHintsTable(L, __func__, ++s, hintList);
            useCurrentFormat = getVerifiedBool(L, __func__, ++s, "useCurrentFormat");
        }
    }

    if ((hintList.size() - commandList.size()) < 0 || (hintList.size() - commandList.size()) > 1) {
        lua_pushnil(L);
        lua_pushfstring(L, "command table and hint table sizes do not match up (%d and %d, either they must be the same or there should be one extra hint) - cannot create popup", commandList.size(), hintList.size());
        return 2;
    }

    auto console = CONSOLE(L, windowName);
    console->insertLink(text, commandList, hintList, useCurrentFormat, luaReferences);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertText
int TLuaInterpreter::insertText(lua_State* L)
{
    QString windowName;
    int s = 0;

    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, ++s);
    }
    const QString text = getVerifiedString(L, __func__, ++s, "text");

    auto console = CONSOLE(L, windowName);
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
    result = host.mpConsole->getBgColor(windowName);
    auto it = result.begin();
    if (result.size() < 3) {
        return 0;
    }
    if (ansiBg < 0) {
        return 0;
    }
    if (ansiBg > 16) {
        return 0;
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
    QString windowName = "main";
    const int ansiFg = getVerifiedInt(L, __func__, 1, "ANSI color");

    std::list<int> result;
    const Host& host = getHostFromLua(L);
    result = host.mpConsole->getFgColor(windowName);
    auto it = result.begin();
    if (result.size() < 3) {
        return 0;
    }
    if (ansiFg < 0) {
        return 0;
    }
    if (ansiFg > 16) {
        return 0;
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
    const QString windowName {WINDOW_NAME(L, 1)};
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->lowerWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveCursor
int TLuaInterpreter::moveCursor(lua_State* L)
{
    int s = 1;
    const int n = lua_gettop(L);
    QString windowName;
    if (n > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    const int luaFrom = getVerifiedInt(L, __func__, s++, "x");
    const int luaTo = getVerifiedInt(L, __func__, s, "y");

    auto console = CONSOLE(L, windowName);
    lua_pushboolean(L, console->moveCursor(luaFrom, luaTo));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveCursorEnd
int TLuaInterpreter::moveCursorEnd(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->moveCursorEnd();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveWindow
int TLuaInterpreter::moveWindow(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    const double x1 = getVerifiedDouble(L, __func__, 2, "x");
    const double y1 = getVerifiedDouble(L, __func__, 3, "y");
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
    const QString name{lua_tostring(L, 1)};

    bool loadLayout = true;
    if (n > 1) {
        loadLayout = getVerifiedBool(L, __func__, 2, "loadLayout", true);
    }
    bool autoDock = true;
    if (n > 2) {
        autoDock = getVerifiedBool(L, __func__, 3, "autoDock", true);
    }
    QString area = QString();
    if (n > 3) {
        if (lua_type(L, 4) != LUA_TSTRING) {
            lua_pushfstring(L, "openUserWindow: bad argument #4 type (area as string expected, got %s!)", luaL_typename(L, 4));
            return lua_error(L);
        }
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
    return movieFunc(L, qsl("pauseMovie"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseWindow
int TLuaInterpreter::raiseWindow(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->raiseWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCommandLineMenuEvent
int TLuaInterpreter::removeCommandLineMenuEvent(lua_State * L)
{
    int args = 1;
    const int argsCount = lua_gettop(L);

    QString commandLineName;
    if (argsCount >= 2) {
        commandLineName = getVerifiedString(L, __func__, args++, "command line name");
    } else {
        commandLineName = qsl("main");
    }
    auto menuLabel = getVerifiedString(L, __func__, args++, "menu label");

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
int TLuaInterpreter::removeMouseEvent(lua_State * L)
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
    QString windowName;

    if (n > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    const QString text = getVerifiedString(L, __func__, s, "with");

    auto console = CONSOLE(L, windowName);
    console->replace(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetCmdLineAction
int TLuaInterpreter::resetCmdLineAction(lua_State* L){
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
    const int n = lua_gettop(L);
    if (n > 0) {
        windowName = getVerifiedString(L, __func__, 1, "console name");
    }

    Host* host = &getHostFromLua(L);
    if (!host->resetBackgroundImage(windowName)) {
        return warnArgumentValue(L, __func__, qsl("console '%1' not found").arg(windowName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetFormat
int TLuaInterpreter::resetFormat(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->reset();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resizeWindow
int TLuaInterpreter::resizeWindow(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "windowName");
    const double x1 = getVerifiedDouble(L, __func__, 2, "width");
    const double y1 = getVerifiedDouble(L, __func__, 3, "height");
    Host& host = getHostFromLua(L);
    host.resizeWindow(text, static_cast<int>(x1), static_cast<int>(y1));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveWindowLayout
int TLuaInterpreter::saveWindowLayout(lua_State* L)
{
    mudlet::self()->mHasSavedLayout = false;
    lua_pushboolean(L, mudlet::self()->saveWindowLayout());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#scaleMovie
int TLuaInterpreter::scaleMovie(lua_State* L)
{
    return movieFunc(L, qsl("scaleMovie"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCaptureGroup
int TLuaInterpreter::selectCaptureGroup(lua_State *L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L,
                        "selectCaptureGroup: bad argument #1 type (capture group as number or capture group name as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }

    Host &host = getHostFromLua(L);
    TLuaInterpreter *pL = host.getLuaInterpreter();
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
            std::string &s = *its;

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
            if (mudlet::smDebugMode) {
                TDebug(Qt::white, Qt::red) << "selectCaptureGroup(" << begin << ", " << length << ")\n" >> &host;
            }
        }
    } else if (lua_isstring(L, 1)) {
        auto name = lua_tostring(L, 1);
        if (pL->mCapturedNameGroupsPosList.contains(name)) {
            begin = pL->mCapturedNameGroupsPosList.value(name).first;
            length = pL->mCapturedNameGroupsPosList.value(name).second;
        }
    }
    if (length > 0) {
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
    QString name = "main";
    if (n >= 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto commandline = COMMANDLINE(L, name);
    commandline->selectAll();
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
    QString windowName;

    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, s++);
    }
    const int from = getVerifiedInt(L, __func__, s++, "from position");
    const int to = getVerifiedInt(L, __func__, s, "length");

    auto console = CONSOLE(L, windowName);
    const int ret = console->selectSection(from, to);
    lua_pushboolean(L, ret != -1);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectString
int TLuaInterpreter::selectString(lua_State* L)
{
    int s = 1;
    QString windowName;
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    const QString searchText = getVerifiedString(L, __func__, s++, "text to select");
    // CHECK: Do we need to qualify this for a non-blank string?

    qint64 const numOfMatch = static_cast <qint64> (getVerifiedInt(L, __func__, s, "match count {1 for first}"));

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->select(searchText, numOfMatch));
    return 1;
}

int TLuaInterpreter::setActiveProfile(lua_State* L)
{
    auto& hostManager = mudlet::self()->getHostManager();
    const QString profileName = getVerifiedString(L, __func__, 1, "profile name");

    if (profileName.isEmpty()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, "setActiveProfile: profile name cannot be empty");
        return 2;
    }

    if (!mudlet::self()->profileExists(profileName)) {
        lua_pushboolean(L, false);
        lua_pushfstring(L, "setActiveProfile: profile '%s' does not exist", profileName.toUtf8().constData());
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
    QString styleSheet;
    QString tag;
    const int n = lua_gettop(L);
    styleSheet = getVerifiedString(L, __func__, 1, "style sheet");
    if (n > 1) {
        tag = getVerifiedString(L, __func__, 2, "tag");
    }

    Host& host = getHostFromLua(L);
    TEvent event {};
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
    QString windowName;
    int r, alpha;
    int s = 1;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    if (lua_type(L, s) == LUA_TSTRING) {
        windowName = WINDOW_NAME(L, s++);
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

    if (isMain(windowName)) {
        host.mBgColor.setRgb(r, g, b, alpha);
        host.mpConsole->setConsoleBgColor(r, g, b, alpha);
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
    QString imgPath;
    int mode = 1;
    int counter = 1;
    const int n = lua_gettop(L);
    if (n > 1 && lua_type(L, 2) == LUA_TSTRING) {
        windowName = getVerifiedString(L, __func__, 1, "console or label name");
        counter++;
    }

    imgPath = getVerifiedString(L, __func__, counter, "image path");
    counter++;

    if (n > 2 || (counter == 2 && n > 1)) {
        mode = getVerifiedInt(L, __func__, counter, "mode");
    }

    if (mode < 1 || mode > 4) {
        return warnArgumentValue(L, __func__, qsl(
            "%1 is not a valid mode! Valid modes are 1 'border', 2 'center', 3 'tile', 4 'style'").arg(mode));
    }

    Host* host = &getHostFromLua(L);
    if (!host->setBackgroundImage(windowName, imgPath, mode)) {
        return warnArgumentValue(L, __func__, qsl("console or label '%1' not found").arg(windowName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBgColor
int TLuaInterpreter::setBgColor(lua_State* L)
{
    QString windowName;
    int r, g, b, alpha;

    auto validRange = [](int number) { return number >= 0 && number <= 255; };

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

    auto console = CONSOLE(L, windowName);
    console->setBgColor(r, g, b, alpha);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBold
int TLuaInterpreter::setBold(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable bold attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Bold, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderBottom
int TLuaInterpreter::setBorderBottom(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.borders();
    sizes.setBottom(getVerifiedInt(L, __func__, 1, "new size"));
    host.setBorders(sizes);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderColor
int TLuaInterpreter::setBorderColor(lua_State* L)
{
    const int luaRed = getVerifiedInt(L, __func__, 1, "red");
    const int luaGreen = getVerifiedInt(L, __func__, 2, "green");
    const int luaBlue = getVerifiedInt(L, __func__, 3, "blue");
    const Host& host = getHostFromLua(L);
    QPalette framePalette;
    framePalette.setColor(QPalette::Text, QColor(Qt::black));
    framePalette.setColor(QPalette::Highlight, QColor(55, 55, 255));
    framePalette.setColor(QPalette::Window, QColor(luaRed, luaGreen, luaBlue, 255));
    host.mpConsole->mpMainFrame->setPalette(framePalette);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderLeft
int TLuaInterpreter::setBorderLeft(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.borders();
    sizes.setLeft(getVerifiedInt(L, __func__, 1, "new size"));
    host.setBorders(sizes);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderRight
int TLuaInterpreter::setBorderRight(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.borders();
    sizes.setRight(getVerifiedInt(L, __func__, 1, "new size"));
    host.setBorders(sizes);
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
        host.setBorders({value, value, value, value});
        break;
    }
    case 2: {
        auto height = getVerifiedInt(L, __func__, 1, "new height");
        auto width = getVerifiedInt(L, __func__, 2, "new width");
        host.setBorders({width, height, width, height});
        break;
    }
    case 3: {
        auto top = getVerifiedInt(L, __func__, 1, "new top size");
        auto width = getVerifiedInt(L, __func__, 2, "new width");
        auto bottom = getVerifiedInt(L, __func__, 3, "new bottom size");
        host.setBorders({width, top, width, bottom});
        break;
        }
    default: {
        auto top = getVerifiedInt(L, __func__, 1, "new top size");
        auto right = getVerifiedInt(L, __func__, 2, "new right size");
        auto bottom = getVerifiedInt(L, __func__, 3, "new bottom size");
        auto left = getVerifiedInt(L, __func__, 4, "new left size");
        host.setBorders({left, top, right, bottom});
        break;
    }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderTop
int TLuaInterpreter::setBorderTop(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto sizes = host.borders();
    sizes.setTop(getVerifiedInt(L, __func__, 1, "new size"));
    host.setBorders(sizes);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFgColor
int TLuaInterpreter::setFgColor(lua_State* L)
{
    int s = 0;
    const int n = lua_gettop(L);
    auto validRange = [](int number) { return number >= 0 && number <= 255; };
    QString windowName;
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

    auto console = CONSOLE(L, windowName);
    console->setFgColor(luaRed, luaGreen, luaBlue);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setButtonStyleSheet
int TLuaInterpreter::setButtonStyleSheet(lua_State* L)
{
    //args: name, css text
    const QString name = getVerifiedString(L, __func__, 1, "name");
    const QString css = getVerifiedString(L, __func__, 2, "css");
    Host& host = getHostFromLua(L);
    auto actionIds = host.getActionUnit()->findItems(name);
    if (actionIds.empty()) {
        return warnArgumentValue(L, __func__, qsl("no button named '%1' found").arg(name));
    }
    for (auto actionId : actionIds) {
        auto action = host.getActionUnit()->getAction(actionId);
        action->css = css;
    }
    host.getActionUnit()->updateToolbar();
    lua_pushboolean(L, 1);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setClipboardText
int TLuaInterpreter::setClipboardText(lua_State* L)
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(getVerifiedString(L, __func__, 1, "text"));
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCmdLineAction
int TLuaInterpreter::setCmdLineAction(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString name = getVerifiedString(L, __func__, 1, "command line name");
    if (name.isEmpty()) {
        return warnArgumentValue(L, __func__, "command line name cannot be an empty string");
    }
    lua_remove(L, 1);

    if (!lua_isfunction(L, 1)) {
        lua_pushfstring(L, "setCmdLineAction: bad argument #2 type (function expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    const int func = luaL_ref(L, LUA_REGISTRYINDEX);

    if (!host.setCmdLineAction(name, func)) {
        return warnArgumentValue(L, __func__, qsl("command line name '%1' not found").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCmdLineStyleSheet
int TLuaInterpreter::setCmdLineStyleSheet(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = getVerifiedString(L, __func__, 1, "command line name", true);
    }
    const QString styleSheet = getVerifiedString(L, __func__, n, "StyleSheet");
    const Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setCmdLineStyleSheet(name, styleSheet); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFont
int TLuaInterpreter::setFont(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }

    const QString fontName = getVerifiedString(L, __func__, s, "name");

    if (fontName.trimmed().isEmpty()) {
        return warnArgumentValue(L, __func__, "font must not be empty");
    }

    if (!mudlet::self()->getAvailableFonts().contains(fontName, Qt::CaseInsensitive)) {
        return warnArgumentValue(L, __func__, qsl("font '%1' is not available").arg(fontName));
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
    // On GNU/Linux or FreeBSD ensure that emojis are displayed in colour even
    // if this font doesn't support it:
    QFont::insertSubstitution(fontName, qsl("Noto Color Emoji"));
    // TODO issue #4159: a nonexisting font breaks the console
#endif
    // For Qt 6.9+, emoji font support is handled globally in FontManager::addEmojiFont()
#endif

    auto console = CONSOLE(L, windowName);
    if (console == host.mpConsole) {
        // apply changes to main console and its while-scrolling component too.
        auto result = host.setDisplayFont(QFont(fontName, host.getDisplayFont().pointSize()));
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }
        console->refreshView();
    } else {
        auto font = QFont(fontName, console->font().pointSize());
        console->setFont(font);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFontSize
int TLuaInterpreter::setFontSize(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }

    const int size = getVerifiedInt(L, __func__, s, "size");
    if (size <= 0) {
        // just throw an error, no default needed.
        return warnArgumentValue(L, __func__, "size cannot be 0 or negative");
    }

    auto console = CONSOLE(L, windowName);
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
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable italic attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Italic, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelToolTip
int TLuaInterpreter::setLabelToolTip(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    const QString labelToolTip = getVerifiedString(L, __func__, 2, "text");
    double duration = 0;
    if (lua_gettop(L) > 2) {
        duration = getVerifiedDouble(L, __func__, 3, "duration");
    }

    const Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelToolTip(labelName, labelToolTip, duration); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelClickCallback
int TLuaInterpreter::setLabelClickCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelClickCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelDoubleClickCallback
int TLuaInterpreter::setLabelDoubleClickCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelDoubleClickCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelMoveCallback
int TLuaInterpreter::setLabelMoveCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelMoveCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnEnter
int TLuaInterpreter::setLabelOnEnter(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelOnEnter"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnLeave
int TLuaInterpreter::setLabelOnLeave(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelOnLeave"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelReleaseCallback
int TLuaInterpreter::setLabelReleaseCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelReleaseCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelStyleSheet
int TLuaInterpreter::setLabelStyleSheet(lua_State* L)
{
    std::string label = getVerifiedString(L, __func__, 1, "label").toStdString();
    std::string markup = getVerifiedString(L, __func__, 2, "markup").toStdString();
    const Host& host = getHostFromLua(L);
    host.mpConsole->setLabelStyleSheet(label, markup);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelCursor
int TLuaInterpreter::setLabelCursor(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    const int labelCursor = getVerifiedInt(L, __func__, 2, "cursortype");
    const Host& host = getHostFromLua(L);

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
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    const QString pixmapLocation = getVerifiedString(L, __func__, 2, "custom cursor location");

    if (n > 2) {
        hotX = getVerifiedInt(L, __func__, 3, "hot spot x-coordinate");
        hotY = getVerifiedInt(L, __func__, 4, "hot spot y-coordinate");
    }

    const Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelCustomCursor(labelName, pixmapLocation, hotX, hotY); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelWheelCallback
int TLuaInterpreter::setLabelWheelCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelWheelCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLink
int TLuaInterpreter::setLink(lua_State* L)
{
    QString windowName = qsl("main");
    int s = 0;
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, ++s);
    }

    QString command;
    int luaReference = 0;
    parseCommandOrFunction(L, __func__, ++s, command, luaReference);
    const QString hint = getVerifiedString(L, __func__, ++s, "tooltip");

    const Host& host = getHostFromLua(L);
    QStringList commandList;
    QStringList hintList;
    QVector<int> luaReferences;
    commandList << command;
    hintList << hint;
    luaReferences << luaReference;

    auto console = CONSOLE(L, windowName);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMiniConsoleFontSize
int TLuaInterpreter::setMiniConsoleFontSize(lua_State* L)
{
    const QString windowName = getVerifiedString(L, __func__, 1, "miniconsole name");
    const int size = getVerifiedInt(L, __func__, 2, "font size");
    auto console = CONSOLE(L, windowName);
    if (size < 1) {
        return warnArgumentValue(L, __func__, qsl("setting font size of '%1' failed").arg(windowName));
    }
    console->setFontSize(size);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovie
int TLuaInterpreter::setMovie(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }
    const QString moviePath = getVerifiedString(L, __func__, 2, "movie (gif) path");

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
    return movieFunc(L, qsl("setMovieFrame"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovieSpeed
int TLuaInterpreter::setMovieSpeed(lua_State* L)
{
    return movieFunc(L, qsl("setMovieSpeed"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setOverline
int TLuaInterpreter::setOverline(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable overline attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Overline, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setPopup
int TLuaInterpreter::setPopup(lua_State* L)
{
    QString windowName = qsl("main");
    int s = 0;
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, ++s);
    }

    QStringList commandList;
    QVector<int> luaReferences;
    parseCommandsOrFunctionsTable(L, __func__, ++s, commandList, luaReferences);

    QStringList hintList;
    parseHintsTable(L, __func__, ++s, hintList);

    if ((hintList.size() - commandList.size()) < 0 || (hintList.size() - commandList.size()) > 1) {
        lua_pushnil(L);
        lua_pushfstring(L, "command table and hint table sizes do not match up (%d and %d, either they must be the same or there should be one extra hint) - cannot create popup", commandList.size(), hintList.size());
        return 2;
    }

    const Host& host = getHostFromLua(L);
    auto console = CONSOLE(L, windowName);
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
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable reverse attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Reverse, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStrikeOut
int TLuaInterpreter::setStrikeOut(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable strikeout attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::StrikeOut, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextFormat
int TLuaInterpreter::setTextFormat(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    const int n = lua_gettop(L);

    const QString windowName {WINDOW_NAME(L, 1)};

    QVector<int> colorComponents(6); // 0-2 RGB background, 3-5 RGB foreground
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
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (bold format as boolean or number {true/non-zero to enable} expected, got %s!)",
                        s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool underline;
    if (lua_isboolean(L, ++s)) {
        underline = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        underline = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (underline format as boolean or number {true/non-zero to enable} expected, got %s!)",
                        s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool italics;
    if (lua_isboolean(L, ++s)) {
        italics = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        italics = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (italic format as boolean or number {true/non-zero to enable} expected, got %s!)",
                        s, luaL_typename(L, s));
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
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (strikeout format as boolean or number {true/non-zero to enable} is optional, got %s!)",
                            s, luaL_typename(L, s));
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
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (overline format as boolean or number {true/non-zero to enable} is optional, got %s!)",
                            s, luaL_typename(L, s));
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
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (reverse format as boolean or number {true/non-zero to enable} is optional, got %s!)",
                            s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    TChar::AttributeFlags const flags = (bold ? TChar::Bold : TChar::None)
            | (italics ? TChar::Italic : TChar::None)
            | (overline ? TChar::Overline : TChar::None)
            | (reverse ? TChar::Reverse : TChar::None)
            | (strikeout ? TChar::StrikeOut : TChar::None)
            | (underline ? TChar::Underline : TChar::None);

    if (!host.mpConsole->setTextFormat(windowName,
                                      QColor(colorComponents.at(3), colorComponents.at(4), colorComponents.at(5)),
                                      QColor(colorComponents.at(0), colorComponents.at(1), colorComponents.at(2)),
                                      flags)) {
        return warnArgumentValue(L, __func__, qsl("window '%1' does not exist").arg(windowName), true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUnderline
int TLuaInterpreter::setUnderline(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    const bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable underline attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Underline, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUserWindowTitle
int TLuaInterpreter::setUserWindowTitle(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "name");
    QString title;
    if (lua_gettop(L) > 1) {
        title = getVerifiedString(L, __func__, 2, "title", true);
    }

    const Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->setUserWindowTitle(name, title); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUserWindowStyleSheet
int TLuaInterpreter::setUserWindowStyleSheet(lua_State* L)
{
    const QString userWindowName = getVerifiedString(L, __func__, 1, "userwindow name");
    const QString userWindowStyleSheet = getVerifiedString(L, __func__, 2, "StyleSheet");
    const Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setUserWindowStyleSheet(userWindowName, userWindowStyleSheet); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindow
int TLuaInterpreter::setWindow(lua_State* L)
{
    const int n = lua_gettop(L);
    int x = 0, y = 0;
    bool show = true;

    const QString windowname {WINDOW_NAME(L, 1)};

    if (lua_type(L, 2) != LUA_TSTRING) {
        lua_pushfstring(L, "setWindow: bad argument #2 type (element name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    const QString name{lua_tostring(L, 2)};

    if (n > 2) {
        x = getVerifiedInt(L, __func__, 3, "x-coordinate");
        y = getVerifiedInt(L, __func__, 4, "y-coordinate");
        show = getVerifiedBool(L, __func__, 5, "show element");
    }

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
    QString windowName;
    if (lua_gettop(L) > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    const int luaFrom = getVerifiedInt(L, __func__, s, "wrapAt");
    auto console = CONSOLE(L, windowName);
    console->setWrapAt(luaFrom);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindowWrapIndent
int TLuaInterpreter::setWindowWrapIndent(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    const int luaFrom = getVerifiedInt(L, __func__, 2, "wrapTo");
    auto console = CONSOLE(L, windowName);
    console->setIndentCount(luaFrom);
    return 0;
}

//Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindowWrapHangingIndent
int TLuaInterpreter::setWindowWrapHangingIndent(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    const int luaFrom = getVerifiedInt(L, __func__, 2, "wrapTo");
    auto console = CONSOLE(L, windowName);
    console->setHangingIndentCount(luaFrom);
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
    return movieFunc(L, qsl("startMovie"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showToolBar
int TLuaInterpreter::showToolBar(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);
    host.getActionUnit()->showToolBar(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCommandBackgroundColor
int TLuaInterpreter::setCommandBackgroundColor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QString windowName;
    int r, alpha;
    int s = 1;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    if (lua_type(L, s) == LUA_TSTRING) {
        windowName = WINDOW_NAME(L, s++);
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

    if (isMain(windowName)) {
        host.mCommandBgColor.setRgb(r, g, b, alpha);
        host.mpConsole->setCommandBgColor(r, g, b, alpha);
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
    QString windowName;
    int r, alpha;
    int s = 1;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    if (lua_type(L, s) == LUA_TSTRING) {
        windowName = WINDOW_NAME(L, s++);
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

    if (isMain(windowName)) {
        host.mCommandFgColor.setRgb(r, g, b, alpha);
        host.mpConsole->setCommandFgColor(r, g, b, alpha);
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
        windowName = getVerifiedString(L, __func__, 1, "window name", true);
        targetLine = getVerifiedInt(L, __func__, 2, "line to scroll to");
    } else if (n == 1) {
        if (lua_isnumber(L, 1)) {
            windowName = QLatin1String("main");
            targetLine = getVerifiedInt(L, __func__, 1, "line to scroll to");
        } else {
            windowName = getVerifiedString(L, __func__, 1, "window name", true);
            stopScrolling = true;
        }
    } else if (n == 0) {
        windowName = QLatin1String("main");
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
    lua_pushfstring(L, "'%s' is not a known label, any type of console, nor command line", windowName.toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#wrapLine
int TLuaInterpreter::wrapLine(lua_State* L)
{
    int s = 1;
    QString windowName = qsl("main");
    if (lua_gettop(L)) {
        windowName = getVerifiedString(L, __func__, s++, "window name");
    }
    const int lineNumber = getVerifiedInt(L, __func__, s, "line");

    const Host& host = getHostFromLua(L);
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
    const QString windowName {WINDOW_NAME(L, 1)};
    Host& host = getHostFromLua(L);
    host.pasteWindow(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScrolling
int TLuaInterpreter::enableScrolling(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    if (windowName.compare(qsl("main"), Qt::CaseSensitive) == 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "scrolling cannot be enabled/disabled for the 'main' window", windowName.toUtf8().constData());
        return 2;
    }

    auto console = CONSOLE(L, windowName);
    console->setScrolling(true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScrolling
int TLuaInterpreter::disableScrolling(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
    if (windowName.compare(qsl("main"), Qt::CaseSensitive) == 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "scrolling cannot be enabled/disabled for the 'main' window", windowName.toUtf8().constData());
        return 2;
    }

    auto console = CONSOLE(L, windowName);
    console->setScrolling(false);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#scrollingActive
int TLuaInterpreter::scrollingActive(lua_State* L)
{
    const QString windowName {WINDOW_NAME(L, 1)};
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
int TLuaInterpreter::movieFunc(lua_State* L, const QString& funcName)
{
    const QString labelName = getVerifiedString(L, funcName.toUtf8().constData(), 1, "label name");
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }
    auto pN = LABEL(L, labelName);
    auto movie = pN->movie();
    if (!movie) {
        return warnArgumentValue(L, __func__, qsl("no movie found at label '%1'").arg(labelName));
    }

    if (funcName == qsl("startMovie")) {
        movie->start();
    } else if (funcName == qsl("pauseMovie")) {
        movie->setPaused(true);
    } else if (funcName == qsl("setMovieFrame")) {
        const int frame = getVerifiedInt(L, funcName.toUtf8().constData(), 2, "movie frame number");
        lua_pushboolean(L, movie->jumpToFrame(frame));
        return 1;
    } else if (funcName == qsl("setMovieSpeed")) {
        const int speed = getVerifiedInt(L, funcName.toUtf8().constData(), 2, "movie playback speed in %");
        movie->setSpeed(speed);
    } else if (funcName == qsl("scaleMovie")) {
        bool autoScale{true};
        const int n = lua_gettop(L);
        if (n > 1) {
            autoScale = getVerifiedBool(L, funcName.toUtf8().constData(), 2, "activate/deactivate scaling movie", true);
        }
        movie->setScaledSize(pN->size());
        if (autoScale) {
            connect(pN, &TLabel::resized, pN, [=] { movie->setScaledSize(pN->size()); });
        } else {
            pN->disconnect(SIGNAL(resized()));
        }
    } else {
        return warnArgumentValue(L, __func__, qsl("'%1' is not a known function name - bug in Mudlet, please report it").arg(funcName));
    }

    lua_pushboolean(L, true);
    return 1;
}
