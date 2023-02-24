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

// mapper-specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

#include "TLuaInterpreter.h"

// TODO - optimise these includes
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomLines
int TLuaInterpreter::getCustomLines(lua_State* L)
{
    int roomID = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (!pR) { //if the room doesn't exist return nil
        return warnArgumentValue(L, __func__, qsl("room %1 doesn't exist").arg(roomID));
    }
    lua_newtable(L); //return table customLines[]
    QStringList exits = pR->customLines.keys();
    for (int i = 0, iTotal = exits.size(); i < iTotal; ++i) {
        lua_pushstring(L, exits.at(i).toUtf8().constData());
        lua_newtable(L); //customLines[direction]
        lua_pushstring(L, "attributes");
        lua_newtable(L); //customLines[direction]["attributes"]
        lua_pushstring(L, "style");
        switch (pR->customLinesStyle.value(exits.at(i))) {
        case Qt::DotLine:
            lua_pushstring(L, "dot line");
            break;
        case Qt::DashLine:
            lua_pushstring(L, "dash line");
            break;
        case Qt::DashDotLine:
            lua_pushstring(L, "dash dot line");
            break;
        case Qt::DashDotDotLine:
            lua_pushstring(L, "dash dot dot line");
            break;
        case Qt::SolidLine:
            [[fallthrough]];
        default:
            lua_pushstring(L, "solid line");
        }
        lua_settable(L, -3); //customLines[direction]["attributes"]["style"]
        lua_pushstring(L, "arrow");
        lua_pushboolean(L, pR->customLinesArrow.value(exits.at(i)));
        lua_settable(L, -3); //customLines[direction]["attributes"]["arrow"]
        lua_pushstring(L, "color");
        lua_newtable(L);
        lua_pushstring(L, "r");
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).red());
        lua_settable(L, -3);
        lua_pushstring(L, "g");
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).green());
        lua_settable(L, -3);
        lua_pushstring(L, "b");
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).blue());
        lua_settable(L, -3);
        lua_settable(L, -3); //customLines[direction]["attributes"]["color"]
        lua_settable(L, -3); //customLines[direction]["attributes"]
        lua_pushstring(L, "points");
        lua_newtable(L); //customLines[direction][points]
        QList<QPointF> pointL = pR->customLines.value(exits.at(i));
        for (int k = 0, kTotal = pointL.size(); k < kTotal; ++k) {
            lua_pushnumber(L, k);
            lua_newtable(L);
            lua_pushstring(L, "x");
            lua_pushnumber(L, pointL.at(k).x());
            lua_settable(L, -3);
            lua_pushstring(L, "y");
            lua_pushnumber(L, pointL.at(k).y());
            lua_settable(L, -3);
            lua_settable(L, -3);
        }
        lua_settable(L, -3); //customLines[direction]["points"]
        lua_settable(L, -3); //customLines[direction]
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomLines1
int TLuaInterpreter::getCustomLines1(lua_State* L)
{
    int roomID = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (!pR) { //if the room doesn't exist return nil
        return warnArgumentValue(L, __func__, qsl("room %1 doesn't exist").arg(roomID));
    }
    lua_newtable(L); //return table customLines[]
    QStringList exits = pR->customLines.keys();
    for (int i = 0, iTotal = exits.size(); i < iTotal; ++i) {
        lua_pushstring(L, exits.at(i).toUtf8().constData());
        lua_newtable(L); //customLines[direction]
        lua_pushstring(L, "attributes");
        lua_newtable(L); //customLines[direction]["attributes"]
        lua_pushstring(L, "style");
        switch (pR->customLinesStyle.value(exits.at(i))) {
        case Qt::DotLine:
            lua_pushstring(L, "dot line");
            break;
        case Qt::DashLine:
            lua_pushstring(L, "dash line");
            break;
        case Qt::DashDotLine:
            lua_pushstring(L, "dash dot line");
            break;
        case Qt::DashDotDotLine:
            lua_pushstring(L, "dash dot dot line");
            break;
        case Qt::SolidLine:
            [[fallthrough]];
        default:
            lua_pushstring(L, "solid line");
        }
        lua_settable(L, -3); //customLines[direction]["attributes"]["style"]
        lua_pushstring(L, "arrow");
        lua_pushboolean(L, pR->customLinesArrow.value(exits.at(i)));
        lua_settable(L, -3); //customLines[direction]["attributes"]["arrow"]
        lua_pushstring(L, "color");
        lua_newtable(L);
        lua_pushinteger(L, 1);
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).red());
        lua_settable(L, -3);
        lua_pushinteger(L, 2);
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).green());
        lua_settable(L, -3);
        lua_pushinteger(L, 3);
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).blue());
        lua_settable(L, -3);
        lua_settable(L, -3); //customLines[direction]["attributes"]["color"]
        lua_settable(L, -3); //customLines[direction]["attributes"]
        lua_pushstring(L, "points");
        lua_newtable(L); //customLines[direction]["points"]
        QList<QPointF> pointL = pR->customLines.value(exits.at(i));
        for (int k = 0, kTotal = pointL.size(); k < kTotal; ++k) {
            // To allow the output from here to be fed back into addCustomLine
            // we need to start the numbering from the Lua standard of 1 and
            // NOT the C/C++ standard of 0 - otherwise the end-user has to
            // fiddle with the zero-th entry to keep the points in order:
            lua_pushinteger(L, k+1);
            lua_newtable(L); //customLines[direction]["points"][3 x coordinates]
            lua_pushinteger(L, 1);
            lua_pushnumber(L, pointL.at(k).x());
            lua_settable(L, -3);
            lua_pushinteger(L, 2);
            lua_pushnumber(L, pointL.at(k).y());
            lua_settable(L, -3);
            lua_pushinteger(L, 3);
            lua_pushnumber(L, pR->z);
            lua_settable(L, -3);
            lua_settable(L, -3); //customLines[direction]["points"][3 x coordinates]
        }
        lua_settable(L, -3); //customLines[direction]["points"]
        lua_settable(L, -3); //customLines[direction]
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitWeights
int TLuaInterpreter::getExitWeights(lua_State* L)
{
    int roomID = getVerifiedInt(L, __func__, 1, "roomID");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    lua_newtable(L);
    if (pR) {
        QStringList keys = pR->getExitWeights().keys();
        for (int i = 0; i < keys.size(); i++) {
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_pushnumber(L, pR->getExitWeight(keys.at(i)));
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteMapLabel
int TLuaInterpreter::deleteMapLabel(lua_State* L)
{
    int area = getVerifiedInt(L, __func__, 1, "areaID");
    int labelID = getVerifiedInt(L, __func__, 2, "labelID");
    Host& host = getHostFromLua(L);
    host.mpMap->deleteMapLabel(area, labelID);
    return 0;
}
