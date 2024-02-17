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
#if defined(INCLUDE_3DMAPPER)
#include "glwidget.h"
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
