/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2022, 2025 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

// any call that that modifies the map visually needs to call host.mpMap->update();

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
            lua_pushnumber(L, pR->z());
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
    host.mpMap->update();
    return 0;
}
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addAreaName
int TLuaInterpreter::addAreaName(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "area name").trimmed();

    const Host& host = getHostFromLua(L);
    if ((!host.mpMap) || (!host.mpMap->mpRoomDB)) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (name.isEmpty()) {
        // Empty names now not allowed
        return warnArgumentValue(L, __func__, "area names may not be empty strings (and spaces are trimmed from the ends)");
    } else if (host.mpMap->mpRoomDB->getAreaNamesMap().values().count(name) > 0) {
        // That name is already IN the areaNamesMap
        return warnArgumentValue(L, __func__, qsl("area names may not be duplicated and areaID %1 already has the name '%2'")
            .arg(QString::number(host.mpMap->mpRoomDB->getAreaNamesMap().key(name)), name));
    }

    // Note that adding an area name implicitly creates an underlying TArea instance
    lua_pushnumber(L, host.mpMap->mpRoomDB->addArea(name));

    if (host.mpMap->mpMapper) {
        host.mpMap->mpMapper->updateAreaComboBox();
    }

    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCustomLine
int TLuaInterpreter::addCustomLine(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    //args: id_from, id_to, direction, style, line color, arrow (bool)
    int id_to = 0;
    int r = 255;
    int g = 0;
    int b = 0;
    Qt::PenStyle line_style(Qt::SolidLine);
    QString direction;
    QList<qreal> x;
    QList<qreal> y;
    QList<int> z;
    const int id_from = getVerifiedInt(L, __func__, 1, "roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id_from));
    }

    if (!lua_isnumber(L, 2) && !lua_istable(L, 2)) {
        lua_pushfstring(L, "addCustomLine: bad argument #2 type (target roomID as number or coordinate list as table expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (lua_isnumber(L, 2)) {
        id_to = static_cast<int>(lua_tointeger(L, 2));
        TRoom* pR_to = host.mpMap->mpRoomDB->getRoom(id_to);
        if (!pR_to) {
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid target roomID").arg(id_to));
        }
        const int area = pR->getArea();
        const int area_to = pR_to->getArea();
        if (area != area_to) {
            return warnArgumentValue(L, __func__, qsl(
                "target room is in area '%1' (ID: %2) which is not the one '%3' (ID: %4) in which this custom line is to be drawn")
                .arg((host.mpMap->mpRoomDB->getAreaNamesMap()).value(area_to), QString::number(area_to),
                        (host.mpMap->mpRoomDB->getAreaNamesMap()).value(area), QString::number(area)));
        }

        x.append(static_cast<qreal>(pR_to->x()));
        y.append(static_cast<qreal>(pR_to->y()));
        z.append(pR->z());
    } else if (lua_istable(L, 2)) {
        lua_pushnil(L);
        int i = 0; // Indexes groups of coordinates in the table
        while (lua_next(L, 2) != 0) {
            ++i;
            if (lua_type(L, -1) != LUA_TTABLE) {
                lua_pushfstring(L,
                                "addCustomLine: bad argument #2 table item index #%d type (coordinate list must be a table containing tables of three coordinates, got %s as indicated item!)",
                                i,
                                luaL_typename(L, -1));
                return lua_error(L);
            }
            lua_pushnil(L);
            int j = 0; // Indexes items (individual coordinates) in current inner table:
            while (lua_next(L, -2) != 0) {
                ++j;
                if (j <= 3) {
                    if (lua_type(L, -1) != LUA_TNUMBER) {
                        char coordinate = '\0';
                        switch (j) {
                        case 1:
                            coordinate = 'x';
                            break;
                        case 2:
                            coordinate = 'y';
                            break;
                        case 3:
                            coordinate = 'z';
                            break;
                        default:
                            Q_UNREACHABLE();
                        }
                        lua_pushfstring(L,
                                        "addCustomLine: bad argument #2 table item index #%d inner table item #%d type (coordinates list as table containing tables of three numbers (x, y and z "
                                        "coordinates} expected, but got a %s as the %c-coordinate at that index!)",
                                        i,
                                        j,
                                        luaL_typename(L, -1),
                                        coordinate);
                        return lua_error(L);
                    }
                    switch (j) {
                    case 1:
                        x.append(lua_tonumber(L, -1));
                        break;
                    case 2:
                        y.append(lua_tonumber(L, -1));
                        break;
                    case 3:
                        z.append(static_cast<int>(lua_tonumber(L, -1)));
                        break;
                    default:; // No-op
                    }
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        if (!i || !x.count()) {
            // If there is only an empty sub-table inside the table then i is
            // one but there is nothing in any of the QLists and things will
            // still blow up as per Issue #5272 - so also check for at least one
            // x-coordinate value:
            return warnArgumentValue(L, __func__, "missing coordinates to create the line to");
        }
        if (x.count() != y.count() || x.count() != z.count()) {
            return warnArgumentValue(L, __func__, "mismatch in numbers of coordinates for the points for the custom line given in table as second argument; each must contain three coordinates, i.e. x, y AND z numeric values as a sub-table");
        }
    }

    direction = dirToString(L, 3);
    if (direction.isEmpty()) {
        lua_pushfstring(L, "addCustomLine: bad argument #3 type (direction as string or number (between 1 and 12 inclusive) expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }
    if (!pR->hasExitOrSpecialExit(direction)) {
        return warnArgumentValue(L, __func__, qsl("roomID %1 does not have an exit in a direction that can be identified from '%2'")
            .arg(QString::number(id_from), lua_tostring(L, 3)));
    }

    const QString lineStyleString = getVerifiedString(L, __func__, 4, "line style");
    if (!lineStyleString.compare(QLatin1String("solid line"))) {
        line_style = Qt::SolidLine;
    } else if (!lineStyleString.compare(QLatin1String("dot line"))) {
        line_style = Qt::DotLine;
    } else if (!lineStyleString.compare(QLatin1String("dash line"))) {
        line_style = Qt::DashLine;
    } else if (!lineStyleString.compare(QLatin1String("dash dot line"))) {
        line_style = Qt::DashDotLine;
    } else if (!lineStyleString.compare(QLatin1String("dash dot dot line"))) {
        line_style = Qt::DashDotDotLine;
    } else {
        return warnArgumentValue(L, __func__, qsl(
            "invalid line style '%1', only use one of: 'solid line', 'dot line', 'dash line', 'dash dot line' or 'dash dot dot line'")
            .arg(lineStyleString));
    }

    if (!lua_istable(L, 5)) {
        lua_pushfstring(L, "addCustomLine: bad argument #5 type (RGB color components as a table expected, got %s!)", luaL_typename(L, 5));
        return lua_error(L);
    } else {
        lua_pushnil(L);
        int tind = 0;
        while (lua_next(L, 5) != 0) {
            if (++tind <= 3) {
                if (lua_type(L, -1) != LUA_TNUMBER) {
                    lua_pushfstring(L,
                                    "addCustomLine: bad argument #5 table item #%d type (%s color component as a number between 0 and 255 expected, got %s!)",
                                    tind,
                                    (tind == 1 ? "red" : (tind == 2 ? "green" : "blue")),
                                    luaL_typename(L, -1));
                    return lua_error(L);
                }

                qint64 const component = lua_tointeger(L, -1);
                if (component < 0 || component > 255) {
                    return warnArgumentValue(L, __func__, qsl(
                        "%1 color component in the table of the fifth argument is %2 which is out of the valid range (0 to 255)")
                        .arg((tind == 1 ? "red" : (tind == 2 ? "green" : "blue")), QString::number(component)));
                }
                switch (tind) {
                case 1:
                    r = static_cast<int>(component);
                    break;
                case 2:
                    g = static_cast<int>(component);
                    break;
                case 3:
                    b = static_cast<int>(component);
                    break;
                default:
                    Q_UNREACHABLE();
                }
            }
            lua_pop(L, 1);
        }
    }

    const bool arrow = getVerifiedBool(L, __func__, 6, "end with arrow");
    const int lz = z.at(0);
    QList<QPointF> points;
    points.append(QPointF(x.at(0), y.at(0)));
    for (int i = 1, total = z.size(); i < total; ++i) {
        if (lz != z.at(i)) {
            return warnArgumentValue(L, __func__, qsl(
                "the z values are not all on the same level (first wrong value is %1 at index %2)")
                .arg(QString::number(z.at(i)), QString::number(i + 1)));
        }
        points.append(QPointF(x.at(i), y.at(i)));
    }

    //Heiko: direction/line relationship must be unique
    pR->customLines[direction] = points;
    pR->customLinesArrow[direction] = arrow;
    pR->customLinesStyle[direction] = line_style;
    pR->customLinesColor[direction] = QColor(r, g, b);

    // Need to update the TRoom {min|max}_{x|y} settings as they are used during
    // the painting process - and not doing that here causes the new line to not
    // show up properly:
    pR->calcRoomDimensions();

    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMapEvent
int TLuaInterpreter::addMapEvent(lua_State* L)
{
    QStringList actionInfo;
    const QString uniqueName = getVerifiedString(L, __func__, 1, "uniquename");
    actionInfo << getVerifiedString(L, __func__, 2, "event name");

    if (!lua_isstring(L, 3)) {
        actionInfo << QString();
    } else {
        actionInfo << lua_tostring(L, 3);
    }
    if (!lua_isstring(L, 4)) {
        actionInfo << uniqueName;
    } else {
        actionInfo << lua_tostring(L, 4);
    }
    //variable number of arguments
    for (int i = 5; i <= lua_gettop(L); i++) {
        actionInfo << lua_tostring(L, i);
    }
    const Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserActions.insert(uniqueName, actionInfo);
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMapMenu
int TLuaInterpreter::addMapMenu(lua_State* L)
{
    //    first arg = unique name, second arg= parent name, third arg = display name (=unique name if not provided)
    QStringList menuList;
    const QString uniqueName = getVerifiedString(L, __func__, 1, "uniquename");

    if (!lua_isstring(L, 2)) {
        menuList << "";
    } else {
        menuList << lua_tostring(L, 2);
    }
    if (!lua_isstring(L, 3)) {
        menuList << uniqueName;
    } else {
        menuList << lua_tostring(L, 3);
    }
    const Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserMenus.insert(uniqueName, menuList);
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addRoom
int TLuaInterpreter::addRoom(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    const bool added = host.mpMap->addRoom(id);
    bool issueBadAreaWarning = false;
    if (added) {
        int areaID = -1;
        if (lua_gettop(L) > 1) {
            areaID = getVerifiedInt(L, __func__, 2, "areaID");
        }
        // defer area calculations as all new rooms are initialised at 0,0,0 anyway
        if (!host.mpMap->setRoomArea(id, areaID, true)) {
            // The above will fail if the areaID does not exist (given that
            // "added" is true then the room now exists - so that isn't the
            // failure reason) so stuff the room in the "Default Area" instead
            host.mpMap->setRoomArea(id, -1, true);
            issueBadAreaWarning = true;
        }
        host.mpMap->update();

        if (issueBadAreaWarning) {
            lua_pushnil(L);
            lua_pushfstring(L, "addRoom: created roomID %d but failed to place it in areaID %d, does that area actually exist? (Room has been placed in areaID -1 instead.)", id, areaID);
            return 2;
        }
    }

    lua_pushboolean(L, added);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addSpecialExit
int TLuaInterpreter::addSpecialExit(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const int fromRoomID = getVerifiedInt(L, __func__, 1, "exit roomID");
    TRoom* pR_from = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR_from) {
        return warnArgumentValue(L, __func__, csmInvalidExitRoomID.arg(fromRoomID));
    }

    const int toRoomID = getVerifiedInt(L, __func__, 2, "entrance roomID");
    TRoom* pR_to = host.mpMap->mpRoomDB->getRoom(toRoomID);
    if (!pR_to) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid entrance roomID").arg(toRoomID));
    }

    const QString dir = getVerifiedString(L, __func__, 3, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the special exit name/command cannot be empty");
    }

    pR_from->setSpecialExit(toRoomID, dir);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#auditAreas
int TLuaInterpreter::auditAreas(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    host.mpMap->audit();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#centerview
int TLuaInterpreter::centerview(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!host.mpMap || !host.mpMap->mpRoomDB || !host.mpMap->mpMapper) {
        return warnArgumentValue(L, __func__, "you haven't opened a map yet");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (pR) {
        host.mpMap->mRoomIdHash[host.getName()] = roomId;
        host.mpMap->mNewMove = true;
#if defined(INCLUDE_3DMAPPER)
        if (host.mpMap->mpM) {
            host.mpMap->mpM->update();
        }
#endif

        if (host.mpMap->mpMapper->mp2dMap) {
            host.mpMap->mpMapper->mp2dMap->isCenterViewCall = true;
            host.mpMap->mpMapper->mp2dMap->update();
            host.mpMap->mpMapper->mp2dMap->isCenterViewCall = false;
            host.mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
        }
        lua_pushboolean(L, true);
        return 1;
    } else {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearAreaUserData
int TLuaInterpreter::clearAreaUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int areaId = getVerifiedInt(L, __func__, 1, "areaID");
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(areaId));
    }
    if (!pA->mUserData.isEmpty()) {
        pA->mUserData.clear();
        host.mpMap->setUnsaved(__func__);
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearAreaUserDataItem
int TLuaInterpreter::clearAreaUserDataItem(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int areaId = getVerifiedInt(L, __func__, 1, "areaID");
    const QString key = getVerifiedString(L, __func__, 2, "key");
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(areaId));
    }
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key can not be an empty string");
    }
    lua_pushboolean(L, (pA->mUserData.remove(key) > 0));
    host.mpMap->setUnsaved(__func__);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearMapSelection
int TLuaInterpreter::clearMapSelection(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpMapper || !host.mpMap->mpMapper->mp2dMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    if (host.mpMap->mpMapper->mp2dMap->mMultiSelection) {
        return warnArgumentValue(L, __func__, "rooms are being selected right now and cannot be stopped at this point");
    }
    if (host.mpMap->mpMapper->mp2dMap->mMultiSelectionSet.isEmpty()) {
        lua_pushboolean(L, false);
    } else {
        host.mpMap->mpMapper->mp2dMap->clearSelection();
        lua_pushboolean(L, true);
    }
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearMapUserData
int TLuaInterpreter::clearMapUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (!host.mpMap->mUserData.isEmpty()) {
        host.mpMap->mUserData.clear();
        host.mpMap->setUnsaved(__func__);
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearMapUserDataItem
int TLuaInterpreter::clearMapUserDataItem(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const QString key = getVerifiedString(L, __func__, 1, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key can not be an empty string");
    }
    lua_pushboolean(L, (host.mpMap->mUserData.remove(key) > 0));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearRoomUserData
int TLuaInterpreter::clearRoomUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    if (!pR->userData.isEmpty()) {
        pR->userData.clear();
        host.mpMap->setUnsaved(__func__);
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearRoomUserDataItem
int TLuaInterpreter::clearRoomUserDataItem(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    const QString key = getVerifiedString(L, __func__, 2, "key");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    // Turns out that an empty key IS possible, but if this changes this should be uncommented
    //        if (key.isEmpty()) {
    //            // If the user accidentally supplied an white-space only or empty key
    //            // string we don't do anything, but we, successfully, fail to do it... 8-)
    //            lua_pushboolean( L, false );
    //        }
    /*      else */ if (pR->userData.contains(key)) {
        pR->userData.remove(key);
        host.mpMap->setUnsaved(__func__);
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearSpecialExits
int TLuaInterpreter::clearSpecialExits(lua_State* L)
{
    const int id_from = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (pR) {
        pR->clearSpecialExits();
    }
    host.mpMap->update();
    return 0;
}
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#closeMapWidget
int TLuaInterpreter::closeMapWidget(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.closeMapWidget(); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#connectExitStub
int TLuaInterpreter::connectExitStub(lua_State* L)
{
    int toRoom = 0;
    bool hasDirection = false;
    bool hasToRoomId = false;
    const int fromRoom = getVerifiedInt(L, __func__, 1, "fromID");

    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    if (lua_gettop(L) < 2) {
        lua_pushfstring(L, "connectExitStub: missing argument #2 (toID as number or direction as number or string expected)");
        return lua_error(L); // lua_error() doesn't return to here!
    }

    if (lua_gettop(L) > 2) {
        // Both toRoomID AND direction given
        hasDirection = true;
        hasToRoomId = true;
    } else {
        // Only have one of toRoomID or direction given - we need to examine the
        // argument more closely
        if (lua_type(L, 2) == LUA_TSTRING) {
            // It is a string so it is (we will assume) a direction
            hasDirection = true;
        } else if (lua_type(L, 2) == LUA_TNUMBER) {
            const int value = qRound(lua_tonumber(L, 2));
            if (value >= DIR_OUT || value <= DIR_NORTH) {
                // Ambiguous - look in more detail and check whether there is a
                // a room with the given number and/or an exit stub:
                const bool hasRoomWithNumberAsId = static_cast<bool>(host.mpMap->mpRoomDB->getRoom(value));
                auto pR = host.mpMap->mpRoomDB->getRoom(fromRoom);
                const bool hasExitStubWithNumberAsDirection = (pR && pR->exitStubs.contains(value));
                if (hasRoomWithNumberAsId) {
                    if (hasExitStubWithNumberAsDirection) {
                        return warnArgumentValue(
                                L, __func__, qsl("%1 is too ambiguous a number to parse into a toID or a direction code as both are valid in this case. If this is a direction, try providing it as a string").arg(lua_tonumber(L, 2)));
                    }
                    // else - usable as only one of the two flags is set:
                    hasToRoomId = true;
                } else {
                    if (!hasExitStubWithNumberAsDirection) {
                        // not usable, as neither flag is set:
                        return warnArgumentValue(L, __func__, qsl("%1 is not valid as a toID nor a direction code").arg(lua_tonumber(L, 2)));
                    }
                    // else - usable as only one of the two flags is set:
                    hasDirection = true;
                }
            } else {
                // it is a number greater than 12 so it is (we will assume) a
                // toRoomID - or it is zero or a negative number and will never
                // work as a roomID but treat it as such so that it will trigger
                // an invalid roomID run-time error message:
                hasToRoomId = true;
            }

        } else {
            errorArgumentType(L, __func__, 2, "toID or direction", "number or string");
            return lua_error(L); // lua_error() doesn't return to here!
        }
    }

    // dirType will be 1 to 12 if it was parsed as one of that range as a NUMBER
    // or an (English) STRING of one of the directions
    int dirType = 0;
    if (hasDirection) {
        const int argNumber = hasToRoomId ? 3 : 2;
        dirType = dirToNumber(L, argNumber);
        if (!dirType) {
            return warnArgumentValue(L, __func__, qsl("argument %1 as '%2' cannot be parsed as a valid direction").arg(QString::number(argNumber), QString::fromUtf8(lua_tostring(L, argNumber))));
        }
    }

    if (hasToRoomId) {
        toRoom = getVerifiedInt(L, __func__, 2, "toID");
    }

    QString errMsg;
    if (hasDirection) {
        if (hasToRoomId) {
            errMsg = host.mpMap->connectExitStubByDirectionAndToId(fromRoom, dirType, toRoom);
        } else {
            errMsg = host.mpMap->connectExitStubByDirection(fromRoom, dirType);
        }

    } else /* effectively: if (!hasDirection && hasToRoomId) */ {
        errMsg = host.mpMap->connectExitStubByToId(fromRoom, toRoom);
    }

    if (!errMsg.isEmpty()) {
        lua_pushnil(L);
        lua_pushstring(L, errMsg.toUtf8().constData());
        return 2;
    }

    host.mpMap->mMapGraphNeedsUpdate = true;

    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMapLabel
int TLuaInterpreter::createMapLabel(lua_State* L)
{
    int fontSize = 50;
    float zoom = 30.0;
    bool showOnTop = true;
    bool noScaling = true;
    bool temporary = false;
    QString fontName;
    int foregroundTransparency = 255;
    int backgroundTransparency = 50;

    const int args = lua_gettop(L);
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    const QString text = getVerifiedString(L, __func__, 2, "text");
    const float posx = getVerifiedFloat(L, __func__, 3, "posX");
    const float posy = getVerifiedFloat(L, __func__, 4, "posY");
    const float posz = getVerifiedFloat(L, __func__, 5, "posZ");
    const int fgr = getVerifiedInt(L, __func__, 6, "fgRed");
    const int fgg = getVerifiedInt(L, __func__, 7, "fgGreen");
    const int fgb = getVerifiedInt(L, __func__, 8, "fgBlue");
    const int bgr = getVerifiedInt(L, __func__, 9, "bgRed");
    const int bgg = getVerifiedInt(L, __func__, 10, "bgGreen");
    const int bgb = getVerifiedInt(L, __func__, 11, "bgBlue");
    int olr = fgr;
    int olg = fgg;
    int olb = fgb;

    if (args > 11) {
        zoom = getVerifiedFloat(L, __func__, 12, "zoom", true);
        fontSize = getVerifiedInt(L, __func__, 13, "fontSize", true);
        if (args > 13) {
            showOnTop = getVerifiedBool(L, __func__, 14, "showOnTop", true);
            if (args > 14) {
                noScaling = getVerifiedBool(L, __func__, 15, "noScaling", true);
            }
        }
    }
    if (args > 15) {
        fontName = getVerifiedString(L, __func__, 16, "fontName", true);
    }
    if (args > 16) {
        foregroundTransparency = getVerifiedInt(L, __func__, 17, "foregroundTransparency", true);
    }
    if (args > 17) {
        backgroundTransparency = getVerifiedInt(L, __func__, 18, "backgroundTransparency", true);
    }
    if (args > 18) {
        temporary = getVerifiedBool(L, __func__, 19, "temporary", true);
    }
    if (args > 19) {
        olr = getVerifiedInt(L, __func__, 20, "outlineRed");
        olg = getVerifiedInt(L, __func__, 21, "outlineGreen");
        olb = getVerifiedInt(L, __func__, 22, "outlineBlue");
    }

    const Host& host = getHostFromLua(L);
    lua_pushinteger(L, host.mpMap->createMapLabel(area, text, posx, posy, posz, QColor(fgr, fgg, fgb, foregroundTransparency), QColor(bgr, bgg, bgb, backgroundTransparency), showOnTop, noScaling, temporary, zoom, fontSize, fontName, QColor(olr, olg, olb, foregroundTransparency)));
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMapImageLabel
int TLuaInterpreter::createMapImageLabel(lua_State* L)
{
    const int args = lua_gettop(L);
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    const QString imagePathFileName = getVerifiedString(L, __func__, 2, "imagePathFileName");
    const float posx = getVerifiedFloat(L, __func__, 3, "posX");
    const float posy = getVerifiedFloat(L, __func__, 4, "posY");
    const float posz = getVerifiedFloat(L, __func__, 5, "posZ");
    const float width = getVerifiedFloat(L, __func__, 6, "width");
    const float height = getVerifiedFloat(L, __func__, 7, "height");
    const float zoom = getVerifiedFloat(L, __func__, 8, "zoom");
    const bool showOnTop = getVerifiedBool(L, __func__, 9, "showOnTop");
    bool temporary = false;
    if (args > 9) {
        temporary = getVerifiedBool(L, __func__, 10, "showOnTop", true);
    }

    const Host& host = getHostFromLua(L);
    lua_pushinteger(L, host.mpMap->createMapImageLabel(area, imagePathFileName, posx, posy, posz, width, height, zoom, showOnTop, temporary));
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMapper
int TLuaInterpreter::createMapper(lua_State* L)
{
    const int n = lua_gettop(L);
    QString windowName = "";
    int counter = 1;

    if (n > 4) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "createMapper: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        windowName = lua_tostring(L, 1);
        counter++;
        if (isMain(windowName)) {
            // createMapper only accepts the empty name as the main window
            windowName.clear();
        }
    }

    const int x = getVerifiedInt(L, __func__, counter, "mapper x-coordinate");
    counter++;
    const int y = getVerifiedInt(L, __func__, counter, "mapper y-coordinate");
    counter++;
    const int width = getVerifiedInt(L, __func__, counter, "mapper width");
    counter++;
    const int height = getVerifiedInt(L, __func__, counter, "mapper height");

    const Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->createMapper(windowName, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createRoomID
int TLuaInterpreter::createRoomID(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (lua_gettop(L) > 0) {
        const int minId = getVerifiedInt(L, __func__, 1, "minimum room Id", true);
        if (minId < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "minimum roomID %1 is an optional value but if provided it must be greater than zero").arg(minId));
        }
        lua_pushnumber(L, host.mpMap->createNewRoomID(lua_tointeger(L, 1)));
    } else {
        lua_pushnumber(L, host.mpMap->createNewRoomID());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteArea
int TLuaInterpreter::deleteArea(lua_State* L)
{
    int id = 0;
    QString name;

    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (lua_isnumber(L, 1)) {
        id = lua_tonumber(L, 1);
        if (id < 1) {
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid areaID greater than zero").arg(id));
        }
        if (!host.mpMap->mpRoomDB->getAreaIDList().contains(id) && !host.mpMap->mpRoomDB->getAreaNamesMap().contains(id)) {
            return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(id));
        }
    } else if (lua_isstring(L, 1)) {
        name = lua_tostring(L, 1);
        if (name.isEmpty()) {
            return warnArgumentValue(L, __func__, "an empty string is not a valid area name");
        } else if (!host.mpMap->mpRoomDB->getAreaNamesMap().values().contains(name)) {
            return warnArgumentValue(L, __func__, qsl("string '%1' is not a valid area name").arg(name));
        } else if (name == host.mpMap->getDefaultAreaName()) {
            return warnArgumentValue(L, __func__, "you can't delete the default area");
        }
    } else {
        lua_pushfstring(L,
                        "deleteArea: bad argument #1 type (area Id as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }

    bool result = false;
    if (!id) {
        result = host.mpMap->mpRoomDB->removeArea(name);
    } else {
        result = host.mpMap->mpRoomDB->removeArea(id);
    }

    if (result) {
        if (host.mpMap->mpMapper) {
            host.mpMap->mpMapper->updateAreaComboBox();
        }
        host.mpMap->setUnsaved(__func__);
        host.mpMap->update();
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteMap
int TLuaInterpreter::deleteMap(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        // These tests pass even if the map is empty, however there can still
        // be deleteable data present even in that case - and this test will
        // still succeed immediately after this function has been used!
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    host.mpMap->mapClear();

    host.mpMap->update();

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteRoom
int TLuaInterpreter::deleteRoom(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    if (id <= 0) {
        return 0;
    }
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->mpRoomDB->removeRoom(id));
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableMapInfo
int TLuaInterpreter::disableMapInfo(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "label");
    auto& host = getHostFromLua(L);
    if (!host.mpMap->mMapInfoContributorManager->disableContributor(name)) {
        return warnArgumentValue(L, __func__, qsl("map info '%1' does not exist").arg(name));
    }

    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableMapInfo
int TLuaInterpreter::enableMapInfo(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "label");
    auto& host = getHostFromLua(L);
    if (!host.mpMap->mMapInfoContributorManager->enableContributor(name)) {
        return warnArgumentValue(L, __func__, qsl("map info '%1' does not exist").arg(name));
    }

    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllAreaUserData
int TLuaInterpreter::getAllAreaUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int areaId = getVerifiedInt(L, __func__, 1, "areaID");

    QStringList keys;
    QStringList values;
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(areaId));
    }
    keys = pA->mUserData.keys();
    values = pA->mUserData.values();
    lua_newtable(L);
    for (int i = 0; i < keys.size(); i++) {
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_pushstring(L, values.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllMapUserData
int TLuaInterpreter::getAllMapUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QStringList keys;
    QStringList values;
    keys = host.mpMap->mUserData.keys();
    values = host.mpMap->mUserData.values();
    lua_newtable(L);
    for (int i = 0; i < keys.size(); i++) {
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_pushstring(L, values.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllRoomEntrances
int TLuaInterpreter::getAllRoomEntrances(lua_State* L)
{
    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    lua_newtable(L);
    QList<int> entrances = host.mpMap->mpRoomDB->getEntranceHash().values(roomId);
    // Could use a .toSet().toList() to remove duplicates values
    if (entrances.count() > 1) {
        std::sort(entrances.begin(), entrances.end());
    }
    for (int i = 0; i < entrances.size(); i++) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, entrances.at(i));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllRoomUserData
int TLuaInterpreter::getAllRoomUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    QStringList keys;
    QStringList values;
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    keys = pR->userData.keys();
    values = pR->userData.values();
    lua_newtable(L);
    for (int i = 0; i < keys.size(); i++) {
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_pushstring(L, values.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaExits
int TLuaInterpreter::getAreaExits(lua_State* L)
{
    const int n = lua_gettop(L);
    bool isFullDataRequired = false;
    const int area = getVerifiedInt(L, __func__, 1, "areaID");

    if (n > 1) {
        isFullDataRequired = getVerifiedBool(L, __func__, 2, "full data wanted", true);
    }

    const Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(area));
    }

    lua_newtable(L);
    if (n < 2 || (n > 1 && !isFullDataRequired)) {
        // Replicate original implementation
        QList<int> areaExits = pA->getAreaExitRoomIds();
        if (areaExits.size() > 1) {
            std::sort(areaExits.begin(), areaExits.end());
        }
        for (int i = 0; i < areaExits.size(); i++) {
            lua_pushnumber(L, i + 1); // Lua lists/arrays begin at 1 not 0!
            lua_pushnumber(L, areaExits.at(i));
            lua_settable(L, -3);
        }
    } else {
        QMultiMap<int, QPair<QString, int>> const areaExits = pA->getAreaExitRoomData();
        QList<int> const fromRooms = areaExits.uniqueKeys();
        for (const int fromRoom : fromRooms) {
            lua_pushnumber(L, fromRoom);
            lua_newtable(L);
            QList<QPair<QString, int>> const toRoomsData = areaExits.values(fromRoom);
            for (const auto& j : toRoomsData) {
                lua_pushstring(L, j.first.toUtf8().constData());
                lua_pushnumber(L, j.second);
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaRooms
int TLuaInterpreter::getAreaRooms(lua_State* L)
{
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    const Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    QSetIterator<int> itAreaRoom(pA->getAreaRooms());
    int i = -1;
    while (itAreaRoom.hasNext()) {
        lua_pushnumber(L, ++i);
        // We should have started at 1 but past code had incorrectly started
        // with a zero index and we must maintain compatibility with code written
        // for that
        lua_pushnumber(L, itAreaRoom.next());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaRooms1
int TLuaInterpreter::getAreaRooms1(lua_State* L)
{
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    const Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    int i = 0;
    for (const int room : std::as_const(pA->getAreaRooms())) {
        lua_pushnumber(L, ++i);
        lua_pushnumber(L, room);
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaTable
int TLuaInterpreter::getAreaTable(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QMapIterator<int, QString> it(host.mpMap->mpRoomDB->getAreaNamesMap());
    lua_newtable(L);
    while (it.hasNext()) {
        it.next();
        const int areaId = it.key();
        const QString name = it.value();
        lua_pushstring(L, name.toUtf8().constData());
        lua_pushnumber(L, areaId);
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaTableSwap
int TLuaInterpreter::getAreaTableSwap(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QMapIterator<int, QString> it(host.mpMap->mpRoomDB->getAreaNamesMap());
    lua_newtable(L);
    while (it.hasNext()) {
        it.next();
        const int areaId = it.key();
        const QString name = it.value();
        lua_pushnumber(L, areaId);
        lua_pushstring(L, name.toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaUserData
int TLuaInterpreter::getAreaUserData(lua_State* L)
{
    const int areaId = getVerifiedInt(L, __func__, 1, "areaID");
    const QString key = getVerifiedString(L, __func__, 2, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key is not allowed to be an empty string");
    }

    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(areaId));
    }
    if (!pA->mUserData.contains(key)) {
        return warnArgumentValue(L, __func__, qsl("no user data with key '%1' in areaID %2").arg(key, QString::number(areaId)));
    }
    lua_pushstring(L, pA->mUserData.value(key).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomEnvColorTable
int TLuaInterpreter::getCustomEnvColorTable(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap->mCustomEnvColors.empty()) {
        lua_newtable(L);
        QList<int> const colorList = host.mpMap->mCustomEnvColors.keys();
        for (auto idx : colorList) {
            lua_pushnumber(L, idx);
            lua_newtable(L);
            // red component
            {
                lua_pushnumber(L, 1);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).red());
                lua_settable(L, -3);
            }
            // green component
            {
                lua_pushnumber(L, 2);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).green());
                lua_settable(L, -3);
            }
            // blue component
            {
                lua_pushnumber(L, 3);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).blue());
                lua_settable(L, -3);
            }
            // alpha component
            {
                lua_pushnumber(L, 4);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).alpha());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }
    } else {
        lua_newtable(L);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDoors
int TLuaInterpreter::getDoors(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }

    lua_newtable(L);
    const QStringList keys = pR->doors.keys();
    for (unsigned int i = 0, total = keys.size(); i < total; ++i) {
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_pushnumber(L, pR->doors.value(keys.at(i)));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitStubs
int TLuaInterpreter::getExitStubs(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    QList<int> const stubs = pR->exitStubs;
    lua_newtable(L);
    for (int i = 0, total = stubs.size(); i < total; ++i) {
        lua_pushnumber(L, i);
        lua_pushnumber(L, stubs.at(i));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitStubs1
int TLuaInterpreter::getExitStubs1(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    QList<int> const stubs = pR->exitStubs;
    lua_newtable(L);
    for (int i = 0, total = stubs.size(); i < total; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, stubs.at(i));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitStubsNames
int TLuaInterpreter::getExitStubsNames(lua_State* L)
{
    const QStringList stubmap = { "north", "northeast", "northwest", "east", "west",
                                  "south", "southeast", "southwest", "up", "down", "in",
                                  "out", "other"
    };

    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    QList<int> const stubs = pR->exitStubs;
    lua_newtable(L);
    for (int i = 0, total = stubs.size(); i < total; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, stubmap[stubs.at(i) - 1].toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getGridMode
int TLuaInterpreter::getGridMode(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int id = getVerifiedInt(L, __func__, 1, "areaID");

    TArea* area = host.mpMap->mpRoomDB->getArea(id);
    if (!area) {
        return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(id));
    } else {
        lua_pushboolean(L, area->gridMode);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapEvents
int TLuaInterpreter::getMapEvents(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                // create the result table
                lua_newtable(L);
                QMapIterator<QString, QStringList> it(host.mpMap->mpMapper->mp2dMap->mUserActions);
                while (it.hasNext()) {
                    it.next();
                    const QStringList eventInfo = it.value();
                    lua_createtable(L, 0, 4);
                    lua_pushstring(L, eventInfo.at(0).toUtf8().constData());
                    lua_setfield(L, -2, "event name");
                    lua_pushstring(L, eventInfo.at(1).toUtf8().constData());
                    lua_setfield(L, -2, "parent");
                    lua_pushstring(L, eventInfo.at(2).toUtf8().constData());
                    lua_setfield(L, -2, "display name");
                    lua_createtable(L, eventInfo.length() - 3, 0);
                    for (int i = 3; i < eventInfo.length(); i++) {
                        lua_pushinteger(L, i - 2); //lua indexes are 1 based!
                        lua_pushstring(L, eventInfo.at(i).toUtf8().constData());
                        lua_settable(L, -3);
                    }
                    lua_setfield(L, -2, "arguments");

                    // Add the mapEvent object to the result table
                    lua_setfield(L, -2, it.key().toUtf8().constData());
                }
            }
            return 1;
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapLabel
int TLuaInterpreter::getMapLabel(lua_State* L)
{
    const int areaId = getVerifiedInt(L, __func__, 1, "areaID");

    if (!lua_isstring(L, 2) && !lua_isnumber(L, 2)) {
        lua_pushfstring(L, "getMapLabel: bad argument #2 type (labelID as number or labelText as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString labelText;
    int labelId = -1;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        labelId = lua_tointeger(L, 2);
        if (labelId < 0) {
            return warnArgumentValue(L, __func__, qsl("labelID %1 is invalid, it must be zero or greater").arg(labelId));
        }
    } else {
        labelText = lua_tostring(L, 2);
        // Can be an empty string as image labels have no text!
    }

    const Host& host = getHostFromLua(L);
    auto pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("areaID %1 does not exist").arg(areaId));
    }
    if (pA->mMapLabels.isEmpty()) {
        // Return an empty table:
        lua_newtable(L);
        return 1;
    }

    if (labelId >= 0) {
        if (!pA->mMapLabels.contains(labelId)) {
            return warnArgumentValue(L, __func__, qsl("labelID %1 does not exist in area with areaID %2")
                .arg(QString::number(labelId), QString::number(areaId)));
        }
        lua_newtable(L);
        auto label = pA->mMapLabels.value(labelId);
        pushMapLabelPropertiesToLua(L, label);
        return 1;
    }

    lua_newtable(L);
    QMapIterator<int, TMapLabel> it(pA->mMapLabels);
    while (it.hasNext()) {
        it.next();
        if (it.value().text == labelText) {
            lua_newtable(L);
            pushMapLabelPropertiesToLua(L, it.value());
            lua_pushnumber(L, it.key());
            lua_insert(L, -2);
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapLabels
int TLuaInterpreter::getMapLabels(lua_State* L)
{
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    const Host& host = getHostFromLua(L);
    lua_newtable(L);
    auto pA = host.mpMap->mpRoomDB->getArea(area);
    if (pA && !pA->mMapLabels.isEmpty()) {
        QMapIterator<int, TMapLabel> it(pA->mMapLabels);
        while (it.hasNext()) {
            it.next();
            lua_pushnumber(L, it.key());
            lua_pushstring(L, it.value().text.toUtf8().constData());
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapMenus
int TLuaInterpreter::getMapMenus(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!(host.mpMap && host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap)) {
        return warnArgumentValue(L, __func__, "you haven't opened a map yet");
    }

    lua_newtable(L);
    QMapIterator<QString, QStringList> it(host.mpMap->mpMapper->mp2dMap->mUserMenus);
    while (it.hasNext()) {
        it.next();
        QString parent, display;
        QStringList menuInfo = it.value();
        parent = menuInfo[0];
        display = menuInfo[1];
        qDebug() << it.key() << parent << display;
        lua_pushstring(L, display.toUtf8().constData());
        lua_pushstring(L, parent.isEmpty() ? "top-level" :parent.toUtf8().constData());
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapSelection
int TLuaInterpreter::getMapSelection(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost || !pHost->mpMap || !pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    lua_newtable(L);
    QList<int> selectionRoomsList{pHost->mpMap->mpMapper->mp2dMap->mMultiSelectionSet.begin(),
                                  pHost->mpMap->mpMapper->mp2dMap->mMultiSelectionSet.end()};
    if (!selectionRoomsList.isEmpty()) {
        if (selectionRoomsList.count() > 1) {
            std::sort(selectionRoomsList.begin(), selectionRoomsList.end());
        }

        lua_pushstring(L, "center");
        lua_pushnumber(L, pHost->mpMap->mpMapper->mp2dMap->getCenterSelectedRoomId());
        lua_settable(L, -3);

        lua_pushstring(L, "rooms");
        lua_newtable(L);
        for (int i = 0, total = selectionRoomsList.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushnumber(L, selectionRoomsList.at(i));
            lua_settable(L, -3);
        }

        lua_settable(L, -3);

    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapUserData
int TLuaInterpreter::getMapUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const QString key = getVerifiedString(L, __func__, 1, "key");
    if (!host.mpMap->mUserData.contains(key)) {
        return warnArgumentValue(L, __func__, qsl("no user data with key '%1' in map").arg(key));
    }
    lua_pushstring(L, host.mpMap->mUserData.value(key).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapZoom
int TLuaInterpreter::getMapZoom(lua_State* L)
{
    std::optional<int> areaID;
    if (lua_gettop(L)) {
        areaID = getVerifiedInt(L, __func__, 1, "area id", true);
    }
    const Host& host = getHostFromLua(L);
    if (host.mpMap.isNull() || host.mpMap->mpMapper.isNull()) {
        return warnArgumentValue(L, __func__, "no map loaded or no active mapper");
    }

    if (areaID.has_value()) {
        if (!host.mpMap->mpRoomDB->getArea(areaID.value())) {
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid areaID").arg(QString::number(areaID.value())));
        }
        lua_pushnumber(L, host.mpMap->mpRoomDB->get2DMapZoom(areaID.value()));
        return 1;
    }

    areaID = host.mpMap->mpMapper->mp2dMap->mAreaID;
    lua_pushnumber(L, host.mpMap->mpRoomDB->get2DMapZoom(areaID.value()));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPath
int TLuaInterpreter::getPath(lua_State* L)
{
    const int originRoomId = getVerifiedInt(L, __func__, 1, "starting roomID");
    const int targetRoomId = getVerifiedInt(L, __func__, 2, "target roomID");

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (!host.mpMap->mpRoomDB->getRoom(originRoomId)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid source roomID").arg(originRoomId));
    } else if (!host.mpMap->mpRoomDB->getRoom(targetRoomId)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid target roomID").arg(targetRoomId));
    }

    const bool ret = host.mpMap->gotoRoom(originRoomId, targetRoomId);
    const int totalWeight = host.assemblePath(); // Needed even if unsuccessful, to clear lua tables then
    if (ret) {
        lua_pushboolean(L, true);
        lua_pushnumber(L, totalWeight);
        return 2;
    } else {
        lua_pushboolean(L, false);
        lua_pushnumber(L, -1);
        lua_pushfstring(L, "getPath: no path found from the roomID %d to roomID %d!", originRoomId, targetRoomId);
        return 3;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayerRoom
int TLuaInterpreter::getPlayerRoom(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!host.mpMap || !host.mpMap->mpRoomDB || !host.mpMap->mpMapper) {
        return warnArgumentValue(L, __func__, "you haven't opened a map yet");
    }

    auto roomID = host.mpMap->mRoomIdHash.value(host.getName(), -1);
    if (roomID == -1) {
        return warnArgumentValue(L, __func__, "the player does not have a valid roomID set");
    }
    lua_pushnumber(L, roomID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomArea
int TLuaInterpreter::getRoomArea(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, pR->getArea());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomAreaName
int TLuaInterpreter::getRoomAreaName(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int id = -1;
    QString name;
    if (!lua_isnumber(L, 1)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L,
                            "getRoomAreaName: bad argument #1 type (area id as number or area name as string\n"
                            "expected, got %s!)",
                            luaL_typename(L, 1));
            return lua_error(L);
        }
        name = lua_tostring(L, 1);
    } else {
        id = lua_tonumber(L, 1);
    }

    if (!name.isNull()) {
        const int result = host.mpMap->mpRoomDB->getAreaNamesMap().key(name, -1);
        lua_pushnumber(L, result);
        if (result != -1) {
            return 1;
        } else {
            lua_pushfstring(L, "getRoomAreaName: string '%s' is not a valid area name", name.toUtf8().constData());
            return 2;
        }
    } else {
        if (host.mpMap->mpRoomDB->getAreaNamesMap().contains(id)) {
            lua_pushstring(L, host.mpMap->mpRoomDB->getAreaNamesMap().value(id).toUtf8().constData());
            return 1;
        } else {
            lua_pushnumber(L, -1);
            lua_pushfstring(L, "getRoomAreaName: number %d is not a valid area id", id);
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomChar
int TLuaInterpreter::getRoomChar(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }

    lua_pushstring(L, pR->mSymbol.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomCharColor
int TLuaInterpreter::getRoomCharColor(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }

    lua_pushnumber(L, pR->mSymbolColor.red());
    lua_pushnumber(L, pR->mSymbolColor.green());
    lua_pushnumber(L, pR->mSymbolColor.blue());
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomCoordinates
int TLuaInterpreter::getRoomCoordinates(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    } else {
        lua_pushnumber(L, pR->x());
        lua_pushnumber(L, pR->y());
        lua_pushnumber(L, pR->z());
        return 3;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomEnv
int TLuaInterpreter::getRoomEnv(lua_State* L)
{
    const int roomID = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (pR) {
        lua_pushnumber(L, pR->environment);
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomExits
int TLuaInterpreter::getRoomExits(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        lua_newtable(L);
        if (pR->getNorth() != -1) {
            lua_pushstring(L, "north");
            lua_pushnumber(L, pR->getNorth());
            lua_settable(L, -3);
        }
        if (pR->getNorthwest() != -1) {
            lua_pushstring(L, "northwest");
            lua_pushnumber(L, pR->getNorthwest());
            lua_settable(L, -3);
        }
        if (pR->getNortheast() != -1) {
            lua_pushstring(L, "northeast");
            lua_pushnumber(L, pR->getNortheast());
            lua_settable(L, -3);
        }
        if (pR->getSouth() != -1) {
            lua_pushstring(L, "south");
            lua_pushnumber(L, pR->getSouth());
            lua_settable(L, -3);
        }
        if (pR->getSouthwest() != -1) {
            lua_pushstring(L, "southwest");
            lua_pushnumber(L, pR->getSouthwest());
            lua_settable(L, -3);
        }
        if (pR->getSoutheast() != -1) {
            lua_pushstring(L, "southeast");
            lua_pushnumber(L, pR->getSoutheast());
            lua_settable(L, -3);
        }
        if (pR->getWest() != -1) {
            lua_pushstring(L, "west");
            lua_pushnumber(L, pR->getWest());
            lua_settable(L, -3);
        }
        if (pR->getEast() != -1) {
            lua_pushstring(L, "east");
            lua_pushnumber(L, pR->getEast());
            lua_settable(L, -3);
        }
        if (pR->getUp() != -1) {
            lua_pushstring(L, "up");
            lua_pushnumber(L, pR->getUp());
            lua_settable(L, -3);
        }
        if (pR->getDown() != -1) {
            lua_pushstring(L, "down");
            lua_pushnumber(L, pR->getDown());
            lua_settable(L, -3);
        }
        if (pR->getIn() != -1) {
            lua_pushstring(L, "in");
            lua_pushnumber(L, pR->getIn());
            lua_settable(L, -3);
        }
        if (pR->getOut() != -1) {
            lua_pushstring(L, "out");
            lua_pushnumber(L, pR->getOut());
            lua_settable(L, -3);
        }
        return 1;
    } else {
        return 0;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomHashByID
int TLuaInterpreter::getRoomHashByID(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    if (!host.mpMap->mpRoomDB->roomIDToHash.contains(id)) {
        return warnArgumentValue(L, __func__, qsl("no hash for room %1").arg(id));
    }
    const QString retHash = host.mpMap->mpRoomDB->roomIDToHash[id];
    lua_pushstring(L, retHash.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomIDbyHash
int TLuaInterpreter::getRoomIDbyHash(lua_State* L)
{
    const QString hash = getVerifiedString(L, __func__, 1, "hash");
    const Host& host = getHostFromLua(L);
    const int retID = host.mpMap->mpRoomDB->hashToRoomID.value(hash, -1);
    lua_pushnumber(L, retID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomName
int TLuaInterpreter::getRoomName(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int id = getVerifiedInt(L, __func__, 1, "roomID");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }
    lua_pushstring(L, pR->name.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRooms
int TLuaInterpreter::getRooms(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_newtable(L);
    QHashIterator<int, TRoom*> it(host.mpMap->mpRoomDB->getRoomMap());
    while (it.hasNext()) {
        it.next();
        lua_pushnumber(L, it.key());
        lua_pushstring(L, it.value()->name.toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomsByPosition
int TLuaInterpreter::getRoomsByPosition(lua_State* L)
{
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    const int x = getVerifiedInt(L, __func__, 2, "x");
    const int y = getVerifiedInt(L, __func__, 3, "y");
    const int z = getVerifiedInt(L, __func__, 4, "z");

    const Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushnil(L);
        return 1;
    }

    QList<int> rL = pA->getRoomsByPosition(x, y, z);
    lua_newtable(L);
    for (int i = 0; i < rL.size(); i++) {
        lua_pushnumber(L, i);
        lua_pushnumber(L, rL[i]);
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomUserData
int TLuaInterpreter::getRoomUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    const QString key = getVerifiedString(L, __func__, 2, "key");
    bool isBackwardCompatibilityRequired = true;
    if (lua_gettop(L) > 2) {
        isBackwardCompatibilityRequired = !getVerifiedBool(L, __func__, 3, "enableFullErrorReporting {default = false}", true);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        if (!isBackwardCompatibilityRequired) {
            return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
        }
        lua_pushstring(L, "");
        return 1;
    }
    if (!pR->userData.contains(key)) {
        if (!isBackwardCompatibilityRequired) {
            return warnArgumentValue(L, __func__, qsl(
                "no user data with key '%1' in room with ID %2").arg(key, QString::number(roomId)));
        }
        lua_pushstring(L, "");
        return 1;
    }
    lua_pushstring(L, pR->userData.value(key).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomUserDataKeys
int TLuaInterpreter::getRoomUserDataKeys(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    QStringList keys;
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    keys = pR->userData.keys();
    lua_newtable(L);
    for (int i = 0; i < keys.size(); i++) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomWeight
int TLuaInterpreter::getRoomWeight(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int roomId;
    if (lua_gettop(L) > 0) {
        roomId = getVerifiedInt(L, __func__, 1, "roomID");
    } else {
        roomId = host.mpMap->mRoomIdHash.value(host.getName());
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (pR) {
        lua_pushnumber(L, pR->getWeight());
        return 1;
    } else {
        return 0;
    }
}

// documented in the wiki!
int TLuaInterpreter::getSpecialExits(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const int id_from = getVerifiedInt(L, __func__, 1, "exit roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id_from));
    }

    bool showAllExits = false;
    if (lua_gettop(L) > 1) {
        showAllExits = getVerifiedBool(L, __func__, 2, "show every exit to same entrance roomID", true);
    }

    QMapIterator<QString, int> itSpecialExit(pR->getSpecialExits());
    QMultiMap<int, QString> specialExitsByExitId;
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        specialExitsByExitId.insert(itSpecialExit.value(), itSpecialExit.key());
    }

    QList<int> const exitRoomIdList = specialExitsByExitId.keys();
    lua_newtable(L);
    for (int i = 0, exitRoomIdCount = exitRoomIdList.count(); i < exitRoomIdCount; ++i) {
        lua_pushnumber(L, exitRoomIdList.at(i));
        lua_newtable(L);
        {
            const QStringList exitCommandsToThisRoomId = specialExitsByExitId.values(exitRoomIdList.at(i));
            int bestUnlockedExitIndex = -1;
            int bestUnlockedExitWeight = -1;
            int bestLockedExitIndex = -1;
            int bestLockedExitWeight = -1;
            const int exitCommandsCount = exitCommandsToThisRoomId.count();
            for (int j = 0; j < exitCommandsCount; ++j) {
                if (showAllExits || exitCommandsCount == 1) {
                    // The simpler case - show all exits (or the only exit) to
                    // this room:
                    lua_pushstring(L, exitCommandsToThisRoomId.at(j).toUtf8().constData());
                    lua_pushstring(L, pR->hasSpecialExitLock(exitCommandsToThisRoomId.at(j)) ? "1" : "0");
                    lua_settable(L, -3);
                    // Go on to next exit to this room:
                    continue;
                }

                // The more complex (but highly unlikely in most MUDs) case
                // - find the best exit to this room when there are more than
                // one:
                const int thisExitWeight = pR->getExitWeight(exitCommandsToThisRoomId.at(j));
                if (pR->hasSpecialExitLock(exitCommandsToThisRoomId.at(j))) {
                    if (bestLockedExitIndex == -1) {
                        bestLockedExitIndex = j;
                        bestLockedExitWeight = thisExitWeight;
                    } else if (thisExitWeight < bestLockedExitWeight) {
                        bestLockedExitIndex = j;
                        bestLockedExitWeight = thisExitWeight;
                    }

                } else {
                    if (bestUnlockedExitIndex == -1) {
                        bestUnlockedExitIndex = j;
                        bestUnlockedExitWeight = thisExitWeight;
                    } else if (thisExitWeight < bestUnlockedExitWeight) {
                        bestUnlockedExitIndex = j;
                        bestUnlockedExitWeight = thisExitWeight;
                    }

                }
            }

            if (!showAllExits && (exitCommandsCount > 1)) {
                // Produce the best exit to this room given that there IS more
                // than one and we haven't been asked to show them all:
                const int bestExitIndex = (bestUnlockedExitIndex != -1) ? bestUnlockedExitIndex : bestLockedExitIndex;
                lua_pushstring(L, exitCommandsToThisRoomId.at(bestExitIndex).toUtf8().constData());
                lua_pushstring(L, pR->hasSpecialExitLock(exitCommandsToThisRoomId.at(bestExitIndex)) ? "1" : "0");
                lua_settable(L, -3);
            }
        }
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSpecialExitsSwap
int TLuaInterpreter::getSpecialExitsSwap(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getSpecialExitsSwap: bad argument #1 type (exit roomID as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    const int id_from = lua_tointeger(L, 1);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id_from));
    }

    QMapIterator<QString, int> it(pR->getSpecialExits());
    lua_newtable(L);
    while (it.hasNext()) {
        it.next();
        lua_pushstring(L, it.key().toUtf8().constData());
        lua_pushnumber(L, it.value());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#gotoRoom
int TLuaInterpreter::gotoRoom(lua_State* L)
{
    const int targetRoomId = getVerifiedInt(L, __func__, 1, "target roomID");

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (!host.mpMap->mpRoomDB->getRoom(targetRoomId)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid target roomID").arg(targetRoomId));
    }

    if (!host.mpMap->gotoRoom(targetRoomId)) {
        const int totalWeight = host.assemblePath(); // Needed if unsuccessful to clear lua speedwalk tables
        Q_UNUSED(totalWeight)
        return warnArgumentValue(L, __func__, qsl("no path found from current room to room with id %1").arg(targetRoomId), true);
    }
    host.startSpeedWalk();
    lua_pushboolean(L, true);
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasExitLock
int TLuaInterpreter::hasExitLock(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");

    const int dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushfstring(L, "hasExitLock: bad argument #2 type (direction as number or string expected, got %s!)");
        return lua_error(L);
    }

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        lua_pushboolean(L, pR->hasExitLock(dir));
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasSpecialExitLock
int TLuaInterpreter::hasSpecialExitLock(lua_State* L)
{
    const int fromRoomID = getVerifiedInt(L, __func__, 1, "exit roomID");
    // Second argument was the entrance roomID but it is not needed any more and is ignored
    const QString dir = getVerifiedString(L, __func__, 3, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the special exit name/command cannot be empty");
    }

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidExitRoomID.arg(fromRoomID));
    }
    if (!pR->getSpecialExits().contains(dir)) {
        return warnArgumentValue(L, __func__, qsl("the special exit name/command '%1' does not exist in roomID %2")
            .arg(dir, QString::number(fromRoomID)));
    }

    lua_pushboolean(L, pR->hasSpecialExitLock(dir));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#highlightRoom
int TLuaInterpreter::highlightRoom(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const int fgr = getVerifiedInt(L, __func__, 2, "color1Red");
    const int fgg = getVerifiedInt(L, __func__, 3, "color1Green");
    const int fgb = getVerifiedInt(L, __func__, 4, "color1Blue");
    const int bgr = getVerifiedInt(L, __func__, 5, "color2Red");
    const int bgg = getVerifiedInt(L, __func__, 6, "color2Green");
    const int bgb = getVerifiedInt(L, __func__, 7, "color2Blue");
    const float radius = getVerifiedFloat(L, __func__, 8, "highlightRadius");
    const int alpha1 = getVerifiedInt(L, __func__, 9, "color1Alpha");
    const int alpha2 = getVerifiedInt(L, __func__, 10, "color2Alpha");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        auto fg = QColor(fgr, fgg, fgb, alpha1);
        auto bg = QColor(bgr, bgg, bgb, alpha2);
        pR->highlight = true;
        pR->highlightColor = fg;
        pR->highlightColor2 = bg;
        pR->highlightRadius = radius;

        host.mpMap->update();
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killMapInfo
int TLuaInterpreter::killMapInfo(lua_State* L)
{
    auto& host = getHostFromLua(L);
    auto name = getVerifiedString(L, __func__, 1, "label");
    if (!host.mpMap->mMapInfoContributorManager->removeContributor(name)) {
        return warnArgumentValue(L, __func__, qsl("map info '%1' does not exist").arg(name));
    }

    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadJsonMap
int TLuaInterpreter::loadJsonMap(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost || !pHost->mpMap || !pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    auto source = getVerifiedString(L, __func__, 1, "import pathFileName");
    if (source.isEmpty()) {
        return warnArgumentValue(L, __func__, "a non-empty path and file name to read to must be provided");
    }

    if (auto [result, message] = pHost->mpMap->readJsonMapFile(source); !result) {
        return warnArgumentValue(L, __func__, message);
    }

    // Must run the audit() process now - as it is no longer done within
    // TMap::readJsonMapFile(...) as that can now be used elsewhere:
    pHost->mpMap->audit();
    pHost->mpMap->mpMapper->mp2dMap->init();
    pHost->mpMap->mpMapper->updateAreaComboBox();
    pHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
    pHost->mpMap->mpMapper->show();

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadMap
int TLuaInterpreter::loadMap(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    QString location;
    if (lua_gettop(L)) {
        location = getVerifiedString(L, __func__, 1, "Map pathFile {loads last stored map if omitted}", true);
    }

    bool isOk = false;
    if (!location.isEmpty() && location.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
        QString errMsg;
        isOk = host.mpConsole->importMap(location, &errMsg);
        if (!isOk) {
            // A false was returned which indicates an error, convert it to a nil
            lua_pushnil(L);
            // And add the expected error message, is to be structured in a
            // compatible manner
            if (!errMsg.isEmpty()) {
                lua_pushstring(L, errMsg.toUtf8().constData());
                return 2;
            } else {
                return 1;
            }
        }
    } else {
        isOk = host.mpConsole->loadMap(location);
    }
    lua_pushboolean(L, isOk);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lockExit
int TLuaInterpreter::lockExit(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");

    const int dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushfstring(L, "lockExit: bad argument #2 type (direction as number or string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    const bool b = getVerifiedBool(L, __func__, 3, "lockIfTrue");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->setExitLock(dir, b);
        host.mpMap->setUnsaved(__func__);
        host.mpMap->update();
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lockRoom
int TLuaInterpreter::lockRoom(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const bool b = getVerifiedBool(L, __func__, 2, "lockIfTrue");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->isLocked = b;
        host.mpMap->setUnsaved(__func__);
        host.mpMap->update();
        host.mpMap->mMapGraphNeedsUpdate = true;
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lockSpecialExit
int TLuaInterpreter::lockSpecialExit(lua_State* L)
{
    const int fromRoomID = getVerifiedInt(L, __func__, 1, "exit roomID");
    // The second argument (was the toRoomID) is now ignored as it is not required/considered in any way
    const QString dir = getVerifiedString(L, __func__, 3, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the special exit name/command cannot be empty");
    }
    const bool b = getVerifiedBool(L, __func__, 4, "special exit lock state");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidExitRoomID.arg(fromRoomID));
    }
    if (!pR->setSpecialExitLock(dir, b)) {
        return warnArgumentValue(L, __func__, qsl("the special exit name/command %1 does not exist in roomID %2")
            .arg(dir, QString::number(fromRoomID)));
    }

    lua_pushboolean(L, true);
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    host.mpMap->mMapGraphNeedsUpdate = true;
    return 1;
}


int TLuaInterpreter::openMapWidget(lua_State* L)
{
    const int n = lua_gettop(L);
    QString area = QString();
    int x = -1, y = -1, width = -1, height = -1;
    if (n == 1) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "openMapWidget: bad argument #1 type (area as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        area = lua_tostring(L, 1);
    }

    if (n > 1) {
        area = qsl("f");
        x = getVerifiedInt(L, __func__, 1, "x-coordinate");
        y = getVerifiedInt(L, __func__, 2, "y-coordinate");
    }
    if (n > 2) {
        width = getVerifiedInt(L, __func__, 3, "width");
        height = getVerifiedInt(L, __func__, 4, "height");
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.openMapWidget(area.toLower(), x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMainWindowSize
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerMapInfo
int TLuaInterpreter::registerMapInfo(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "label");

    if (!lua_isfunction(L, 2)) {
        lua_pushfstring(L, "registerMapInfo: bad argument #2 type (callback as function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    const int callback = luaL_ref(L, LUA_REGISTRYINDEX);

    auto& host = getHostFromLua(L);
    host.mpMap->mMapInfoContributorManager->registerContributor(name, [=](int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor) {
        Q_UNUSED(infoColor)
        lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
        if (roomID > 0) {
            lua_pushinteger(L, roomID);
        } else {
            lua_pushnil(L);
        }
        lua_pushinteger(L, selectionSize);
        lua_pushinteger(L, areaId);
        lua_pushinteger(L, displayAreaId);

        const int error = lua_pcall(L, 4, 6, 0);
        if (error) {
            const int errorCount = lua_gettop(L);
            if (mudlet::smDebugMode) {
                for (int i = 1; i <= errorCount; i++) {
                    if (lua_isstring(L, i)) {
                        auto errorMessage = lua_tostring(L, i);
                        TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA ERROR: when running map info callback for '" << name << "\nreason: " << errorMessage << "\n" >> 0;
                    }
                }
            }
            lua_pop(L, errorCount);
            return MapInfoProperties{};
        }

        auto nResult = lua_gettop(L);
        auto index = -nResult;
        const QString text = lua_tostring(L, index);
        const bool isBold = lua_toboolean(L, ++index);
        const bool isItalic = lua_toboolean(L, ++index);
        int r = -1;
        int g = -1;
        int b = -1;
        if (!lua_isnil(L, ++index)) {
            r = lua_tonumber(L, index);
        }
        if (!lua_isnil(L, ++index)) {
            g = lua_tonumber(L, index);
        }
        if (!lua_isnil(L, ++index)) {
            b = lua_tonumber(L, index);
        }
        QColor color;
        if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
            color = QColor(r, g, b);
        }
        lua_pop(L, nResult);
        return MapInfoProperties{ isBold, isItalic, text, color };
    });

    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCustomLine
int TLuaInterpreter::removeCustomLine(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    //args: room_id, direction
    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }

    const QString direction = dirToString(L, 2);
    if (direction.isEmpty()) {
        lua_pushfstring(L, "removeCustomLine: bad argument #2 type (direction as string or number (between 1 and 12 inclusive) expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (!pR->hasExitOrSpecialExit(direction)) {
        return warnArgumentValue(L, __func__, qsl(
            "roomID %1 does not have an exit that can be identified from '%2'").arg(QString::number(roomId), lua_tostring(L, 2)));
    }

    if (0 >= (pR->customLines.remove(direction) + pR->customLinesArrow.remove(direction)
        + pR->customLinesStyle.remove(direction) + pR->customLinesColor.remove(direction))) {
        return warnArgumentValue(L, __func__, qsl(
            "roomID %1 does not appear to have a custom exit line for the exit indentifed from '%2'")
            .arg(QString::number(roomId), lua_tostring(L, 2)));
    }
    // Need to update the TRoom {min|max}_{x|y} settings as they are used during
    // the painting process:
    pR->calcRoomDimensions();
    host.mpMap->update();

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeMapEvent
int TLuaInterpreter::removeMapEvent(lua_State* L)
{
    const QString displayName = getVerifiedString(L, __func__, 1, "event name");
    const Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserActions.remove(displayName);
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeMapMenu
int TLuaInterpreter::removeMapMenu(lua_State* L)
{
    const QString uniqueName = getVerifiedString(L, __func__, 1, "Menu name");
    if (uniqueName.isEmpty()) {
        return 0;
    }
    const Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserMenus.remove(uniqueName);
                //remove all entries with this as parent
                QStringList removeList;
                removeList.append(uniqueName);
                bool newElement = true;
                while (newElement) {
                    newElement = false;
                    QMapIterator<QString, QStringList> it(host.mpMap->mpMapper->mp2dMap->mUserMenus);
                    while (it.hasNext()) {
                        it.next();
                        QStringList menuInfo = it.value();
                        const QString parent = menuInfo[0];
                        if (removeList.contains(parent)) {
                            host.mpMap->mpMapper->mp2dMap->mUserMenus.remove(it.key());
                            if (it.key() != "" && !removeList.contains(it.key())) {
                                host.mpMap->mpMapper->mp2dMap->mUserMenus.remove(it.key());
                                removeList.append(it.key());
                                newElement = true;
                            }
                        }
                    }
                }
                QMapIterator<QString, QStringList> it2(host.mpMap->mpMapper->mp2dMap->mUserActions);
                while (it2.hasNext()) {
                    it2.next();
                    const QString actParent = it2.value()[1];
                    if (removeList.contains(actParent)) {
                        host.mpMap->mpMapper->mp2dMap->mUserActions.remove(it2.key());
                    }
                }
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeSpecialExit
int TLuaInterpreter::removeSpecialExit(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const int fromRoomID = getVerifiedInt(L, __func__, 1, "exit roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidExitRoomID.arg(fromRoomID));
    }

    const QString dir = getVerifiedString(L, __func__, 2, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the exit command cannot be empty");
    }

    if (!pR->getSpecialExits().contains(dir)) {
        return warnArgumentValue(L, __func__, qsl(
            "the special exit name/command '%1' does not exist in exit roomID %2").arg(dir, QString::number(fromRoomID)));
    }
    pR->setSpecialExit(-1, dir);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetRoomArea
int TLuaInterpreter::resetRoomArea(lua_State* L)
{
    //will reset the room area to our void area
    const int id = getVerifiedInt(L, __func__, 1, "roomID");

    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (!host.mpMap->mpRoomDB->getRoomIDList().contains(id)) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }
    const bool result = host.mpMap->setRoomArea(id, -1, false);
    if (result) {
        host.mpMap->update();
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#roomExists
int TLuaInterpreter::roomExists(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#roomLocked
int TLuaInterpreter::roomLocked(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        const bool r = pR->isLocked;
        lua_pushboolean(L, r);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveJsonMap
int TLuaInterpreter::saveJsonMap(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost || !pHost->mpMap || !pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString destination;

    if (lua_gettop(L) > 0) {
        destination = getVerifiedString(L, __func__, 1, "export pathFileName");
    }

    if (auto [result, message] = pHost->mpMap->writeJsonMapFile(destination); !result) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveMap
int TLuaInterpreter::saveMap(lua_State* L)
{
    QString location;
    int saveVersion = 0;

    if (lua_gettop(L) > 0) {
        location = getVerifiedString(L, __func__, 1, "save location path and file name", true);
        if (lua_gettop(L) > 1) {
            saveVersion = getVerifiedInt(L, __func__, 2, "map format version", true);
        }
    }

    const Host& host = getHostFromLua(L);
    const bool error = host.mpConsole->saveMap(location, saveVersion);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#searchAreaUserData
int TLuaInterpreter::searchAreaUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString key = QString();
    QString value = QString(); //both of these assigns a null value which is detectably different from the empty value

    if (lua_gettop(L)) {
        key = getVerifiedString(L, __func__, 1, "key", true);
        if (lua_gettop(L) > 1) {
            value = getVerifiedString(L, __func__, 2, "value", true);
        }
    }

    lua_newtable(L);

    QMapIterator<int, TArea*> itArea(host.mpMap->mpRoomDB->getAreaMap());
    // For best performance do the three different types of action in three
    // different branches each with a loop - rather than choosing a branch
    // within a loop for each room

    lua_newtable(L);
    if (key.isNull()) { // Find all keys everywhere
        QSet<QString> keysSet;
        while (itArea.hasNext()) {
            itArea.next();
            QList<QString> areaDataKeysList{itArea.value()->mUserData.keys()};
            keysSet.unite(QSet<QString>{areaDataKeysList.begin(), areaDataKeysList.end()});
        }

        QStringList keys{keysSet.begin(), keysSet.end()};
        if (keys.size() > 1) {
            std::sort(keys.begin(), keys.end());
        }

        for (unsigned int i = 0, total = keys.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else if (value.isNull()) { // Find all values for a particular key in every room
        QSet<QString> valuesSet; // Use a set as it automatically eliminates duplicates
        while (itArea.hasNext()) {
            itArea.next();
            const QString areaValueForKey = itArea.value()->mUserData.value(key, QString());
            // If the key is NOT present, will return second argument which is a
            // null QString which is NOT the same as an empty QString.
            if (!areaValueForKey.isNull()) {
                valuesSet.insert(areaValueForKey);
            }
        }

        QStringList values{valuesSet.begin(), valuesSet.end()};
        if (values.size() > 1) {
            std::sort(values.begin(), values.end());
        }

        for (unsigned int i = 0, total = values.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, values.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else {
        QSet<int> areaIdsSet;
        while (itArea.hasNext()) { // Find all areas with a particular key AND value
            itArea.next();

            const QString areaDataValue = itArea.value()->mUserData.value(key, QString());
            if ((!areaDataValue.isNull()) && (!value.compare(areaDataValue, Qt::CaseSensitive))) {
                areaIdsSet.insert(itArea.key());
            }
        }

        QList<int> areaIds{areaIdsSet.begin(), areaIdsSet.end()};
        if (areaIds.size() > 1) {
            std::sort(areaIds.begin(), areaIds.end());
        }

        for (unsigned int i = 0, total = areaIds.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushnumber(L, areaIds.at(i));
            lua_settable(L, -3);
        }
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#searchRoom
int TLuaInterpreter::searchRoom(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int room_id = 0;
    const int n = lua_gettop(L);
    bool gotRoomID = false;
    bool caseSensitive = false;
    bool exactMatch = false;
    QString room;

    if (lua_isnumber(L, 1)) {
        room_id = lua_tointeger(L, 1);
        gotRoomID = true;
    } else if (lua_isstring(L, 1)) {
        if (n > 1) {
            if (lua_isboolean(L, 2)) {
                caseSensitive = lua_toboolean(L, 2);
                if (n > 2) {
                    if (lua_isboolean(L, 3)) {
                        exactMatch = lua_toboolean(L, 3);
                    } else {
                        lua_pushfstring(L, R"(searchRoom: bad argument #3 type ("exact match" as boolean is optional, got %s!))", luaL_typename(L, 3));
                        return lua_error(L);
                    }
                }
            } else {
                lua_pushfstring(L, R"(searchRoom: bad argument #2 type ("case sensitive" as boolean is optional, got %s!))", luaL_typename(L, 2));
                return lua_error(L);
            }
        }
        room = lua_tostring(L, 1);
    } else {
        lua_pushfstring(L, R"(searchRoom: bad argument #1 ("room name" as string expected, got %s!))", luaL_typename(L, 1));
        return lua_error(L);
    }

    if (gotRoomID) {
        TRoom* pR = host.mpMap->mpRoomDB->getRoom(room_id);
        if (pR) {
            lua_pushstring(L, pR->name.toUtf8().constData());
            return 1;
        } else {
            lua_pushfstring(L, "searchRoom: bad argument #1 value (roomID %d does not exist!)", room_id);
            // Should've been a nil with this as an second returned string!
            return 1;
        }
    } else {
        QList<TRoom*> const roomList = host.mpMap->mpRoomDB->getRoomPtrList();
        lua_newtable(L);
        QList<int> roomIdsFound;
        for (auto pR : roomList) {
            if (!pR) {
                continue;
            }
            if (exactMatch) {
                if (pR->name.compare(room, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) == 0) {
                    roomIdsFound.append(pR->getId());
                }
            } else {
                if (pR->name.contains(room, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                    roomIdsFound.append(pR->getId());
                }
            }
        }
        if (!roomIdsFound.isEmpty()) {
            for (const int i : roomIdsFound) {
                TRoom* pR = host.mpMap->mpRoomDB->getRoom(i);
                // This test is to keep Coverity happy as it thinks pR could be
                // a nullptr in some odd situation {CID 1415023}:
                if (pR) {
                    const QString name = pR->name;
                    const int roomID = pR->getId();
                    lua_pushnumber(L, roomID);
                    lua_pushstring(L, name.toUtf8().constData());
                    lua_settable(L, -3);
                }
            }
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#searchRoomUserData
int TLuaInterpreter::searchRoomUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString key = QString();
    QString value = QString(); //both of these assigns a null value which is detectably different from the empty value

    if (lua_gettop(L)) {
        key = getVerifiedString(L, __func__, 1, "key", true);
        if (lua_gettop(L) > 1) {
            value = getVerifiedString(L, __func__, 2, "value", true);
        }
    }

    lua_newtable(L);

    QHashIterator<int, TRoom*> itRoom(host.mpMap->mpRoomDB->getRoomMap());
    // For best performance do the three different types of action in three
    // different branches each with a loop - rather than choosing a branch
    // within a loop for each room

    lua_newtable(L);
    if (key.isNull()) { // Find all keys everywhere
        QSet<QString> keysSet;
        while (itRoom.hasNext()) {
            itRoom.next();
            // In the brave new world of range based initializers one must use
            // a pair of iterators that point to the SAME thing that lasts
            // long enough - using the output of a Qt method that returns a
            // QList twice is not good enough and causes seg. faults...
            QList<QString> roomDataKeysList{itRoom.value()->userData.keys()};
            keysSet.unite(QSet<QString>{roomDataKeysList.begin(), roomDataKeysList.end()});
        }

        QStringList keys{keysSet.begin(), keysSet.end()};
        if (keys.size() > 1) {
            std::sort(keys.begin(), keys.end());
        }

        for (unsigned int i = 0, total = keys.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else if (value.isNull()) { // Find all values for a particular key in every room
        QSet<QString> valuesSet; // Use a set as it automatically eliminates duplicates
        while (itRoom.hasNext()) {
            itRoom.next();
            const QString roomValueForKey = itRoom.value()->userData.value(key, QString());
            // If the key is NOT present, will return second argument which is a
            // null QString which is NOT the same as an empty QString.
            if (!roomValueForKey.isNull()) {
                valuesSet.insert(roomValueForKey);
            }
        }

        QStringList values{valuesSet.begin(), valuesSet.end()};
        if (values.size() > 1) {
            std::sort(values.begin(), values.end());
        }

        for (unsigned int i = 0, total = values.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, values.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else { // Find all rooms where key and value match
        QSet<int> roomIdsSet;
        while (itRoom.hasNext()) {
            itRoom.next();

            const QString roomDataValue = itRoom.value()->userData.value(key, QString());
            if ((!roomDataValue.isNull()) && (!value.compare(roomDataValue, Qt::CaseSensitive))) {
                roomIdsSet.insert(itRoom.key());
            }
        }

        QList<int> roomIds{roomIdsSet.begin(), roomIdsSet.end()};
        if (roomIds.size() > 1) {
            std::sort(roomIds.begin(), roomIds.end());
        }

        for (unsigned int i = 0, total = roomIds.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushnumber(L, roomIds.at(i));
            lua_settable(L, -3);
        }
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAreaName
int TLuaInterpreter::setAreaName(lua_State* L)
{
    int id = -1;
    QString existingName;
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (lua_isnumber(L, 1)) {
        id = lua_tonumber(L, 1);
        if (id < 1) {
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid areaID greater than zero").arg(id));
        }
        // Strangely, previous code allowed this command to create a NEW area's name
        // with this ID, but without a TArea instance to accompany it (the latter was/is
        // instantiated as needed when a room is moved to the relevant area...) and we
        // need to continue to allow this - Slysven
        //        else if (!host.mpMap->mpRoomDB->getAreaIDList().contains(id)) {
        //            return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(id));
        //        }
    } else if (lua_isstring(L, 1)) {
        existingName = lua_tostring(L, 1);
        id = host.mpMap->mpRoomDB->getAreaNamesMap().key(existingName, 0);
        if (existingName.isEmpty()) {
            return warnArgumentValue(L, __func__, "area name cannot be empty");
        } else if (!host.mpMap->mpRoomDB->getAreaNamesMap().values().contains(existingName)) {
            return warnArgumentValue(L, __func__, csmInvalidAreaName.arg(existingName));
        } else if (host.mpMap->mpRoomDB->getAreaNamesMap().value(-1).contains(existingName)) {
            return warnArgumentValue(L, __func__, qsl(
                "area name '%1' is reserved and protected - it cannot be changed").arg(existingName));
        }
    } else {
        lua_pushfstring(L,
                        "setAreaName: bad argument #1 type (areaID as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }

    const QString newName = getVerifiedString(L, __func__, 2, "area name").trimmed();
    // Now allow non-Ascii names but eliminate any leading or trailing spaces

    if (newName.isEmpty()) {
        // Empty name not allowed (any more)
        return warnArgumentValue(L, __func__, "area names may not be empty strings (and spaces are trimmed from the ends)");
    } else if (host.mpMap->mpRoomDB->getAreaNamesMap().values().count(newName) > 0) {
        // That name is already IN the areaNamesMap, and since we now enforce
        // uniqueness there can be only one of it - so we can check if this is a
        // problem or just pointless quite easily...!
        if (host.mpMap->mpRoomDB->getAreaNamesMap().value(id) != newName) {
            // And it isn't the trivial case, where the given areaID already IS that name
            return warnArgumentValue(L, __func__, qsl(
                "area names may not be duplicated and areaID %1 already has the name '%2'")
                .arg(QString::number(host.mpMap->mpRoomDB->getAreaNamesMap().key(newName)), newName));
        }
        // Renaming an area to the same name is pointlessly successful!
        lua_pushboolean(L, true);
        return 1;
    }

    bool isCurrentAreaRenamed = false;
    if (host.mpMap->mpMapper) {
        if (id > 0 && host.mpMap->mpRoomDB->getAreaNamesMap().value(id) == host.mpMap->mpMapper->comboBox_showArea->currentText()) {
            isCurrentAreaRenamed = true;
        }
    }

    const bool result = host.mpMap->mpRoomDB->setAreaName(id, newName);
    if (result) {
        host.mpMap->setUnsaved(__func__);
        host.mpMap->update();
        if (host.mpMap->mpMapper) {
            host.mpMap->mpMapper->updateAreaComboBox();
            if (isCurrentAreaRenamed) {
                host.mpMap->mpMapper->comboBox_showArea->setCurrentText(newName);
            }
        }
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAreaUserData
int TLuaInterpreter::setAreaUserData(lua_State* L)
{
    const int areaId = getVerifiedInt(L, __func__, 1, "areaID");
    const QString key = getVerifiedString(L, __func__, 2, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key is not allowed to be an empty string");
    }
    const QString value = getVerifiedString(L, __func__, 3, "value");

    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(areaId));
    }
    pA->mUserData[key] = value;
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCustomEnvColor
int TLuaInterpreter::setCustomEnvColor(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "environmentID");
    const int r = getVerifiedInt(L, __func__, 2, "red color component");
    const int g = getVerifiedInt(L, __func__, 3, "green color component");
    const int b = getVerifiedInt(L, __func__, 4, "blue color component");
    int alpha = 255;
    if (lua_gettop(L) > 4) {
        alpha = getVerifiedInt(L, __func__, 5, "alpha color component", true);
    }

    if ((r < 0) || (r > 255)) {
        lua_pushnil(L);
        lua_pushfstring(L, "red color component %d out of range {0 to 255}", r);
        return 2;
    }
    if ((g < 0) || (g > 255)) {
        lua_pushnil(L);
        lua_pushfstring(L, "green color component %d out of range {0 to 255}", g);
        return 2;
    }
    if ((b < 0) || (b > 255)) {
        lua_pushnil(L);
        lua_pushfstring(L, "blue color component %d out of range {0 to 255}", b);
        return 2;
    }
    if ((alpha < 0) || (alpha > 255)) {
        lua_pushnil(L);
        lua_pushfstring(L, "alpha color component %d out of range {0 to 255}", alpha);
        return 2;
    }
    const QColor& newColor = QColor(r, g, b, alpha);
    Host& host = getHostFromLua(L);
    host.mpMap->mCustomEnvColors[id] = newColor;
    switch (id) { // See TMap::restore16ColorSet() for mapping of indexes:
    case 257:   host.mRed_2 = newColor;             break;
    case 258:   host.mGreen_2 = newColor;           break;
    case 259:   host.mYellow_2 = newColor;          break;
    case 260:   host.mBlue_2 = newColor;            break;
    case 261:   host.mMagenta_2 = newColor;         break;
    case 262:   host.mCyan_2 = newColor;            break;
    case 263:   host.mWhite_2 = newColor;           break;
    case 264:   host.mBlack_2 = newColor;           break;
    case 265:   host.mLightRed_2 = newColor;        break;
    case 266:   host.mLightGreen_2 = newColor;      break;
    case 267:   host.mLightYellow_2 = newColor;     break;
    case 268:   host.mLightBlue_2 = newColor;       break;
    case 269:   host.mLightMagenta_2 = newColor;    break;
    case 270:   host.mLightCyan_2 = newColor;       break;
    case 271:   host.mLightWhite_2 = newColor;      break;
    case 272:   host.mLightBlack_2 = newColor;      break;
    default: {} // No-op
    }

    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDoor
int TLuaInterpreter::setDoor(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    const QString exitCmd = getVerifiedString(L, __func__, 2, "door command");

    if (exitCmd.compare(qsl("n")) && exitCmd.compare(qsl("e")) && exitCmd.compare(qsl("s")) && exitCmd.compare(qsl("w"))
        && exitCmd.compare(qsl("ne"))
        && exitCmd.compare(qsl("se"))
        && exitCmd.compare(qsl("sw"))
        && exitCmd.compare(qsl("nw"))
        && exitCmd.compare(qsl("up"))
        && exitCmd.compare(qsl("down"))
        && exitCmd.compare(qsl("in"))
        && exitCmd.compare(qsl("out"))) {
        // One of the above WILL BE ZERO if the exitCmd is ONE of the above qsls
        // So the above will be TRUE if NONE of above strings match - which
        // means we must treat the exitCmd as a SPECIAL exit
        if (!(pR->getSpecialExits().contains(exitCmd))) {
            // And NOT a special one either
            return warnArgumentValue(L, __func__, qsl(
                "roomID %1 does not have a special exit in direction '%2'")
                .arg(QString::number(roomId), exitCmd));
        }
        // else IS a valid special exit - so fall out of if and continue
    } else {
        // Is a normal exit so see if it is valid
        if (!(((!exitCmd.compare(qsl("n"))) && (pR->getExit(DIR_NORTH) > 0 || pR->exitStubs.contains(DIR_NORTH)))
                || ((!exitCmd.compare(qsl("e"))) && (pR->getExit(DIR_EAST) > 0 || pR->exitStubs.contains(DIR_EAST)))
                || ((!exitCmd.compare(qsl("s"))) && (pR->getExit(DIR_SOUTH) > 0 || pR->exitStubs.contains(DIR_SOUTH)))
                || ((!exitCmd.compare(qsl("w"))) && (pR->getExit(DIR_WEST) > 0 || pR->exitStubs.contains(DIR_WEST)))
                || ((!exitCmd.compare(qsl("ne"))) && (pR->getExit(DIR_NORTHEAST) > 0 || pR->exitStubs.contains(DIR_NORTHEAST)))
                || ((!exitCmd.compare(qsl("se"))) && (pR->getExit(DIR_SOUTHEAST) > 0 || pR->exitStubs.contains(DIR_SOUTHEAST)))
                || ((!exitCmd.compare(qsl("sw"))) && (pR->getExit(DIR_SOUTHWEST) > 0 || pR->exitStubs.contains(DIR_SOUTHWEST)))
                || ((!exitCmd.compare(qsl("nw"))) && (pR->getExit(DIR_NORTHWEST) > 0 || pR->exitStubs.contains(DIR_NORTHWEST)))
                || ((!exitCmd.compare(qsl("up"))) && (pR->getExit(DIR_UP) > 0 || pR->exitStubs.contains(DIR_UP)))
                || ((!exitCmd.compare(qsl("down"))) && (pR->getExit(DIR_DOWN) > 0 || pR->exitStubs.contains(DIR_DOWN)))
                || ((!exitCmd.compare(qsl("in"))) && (pR->getExit(DIR_IN) > 0 || pR->exitStubs.contains(DIR_IN)))
                || ((!exitCmd.compare(qsl("out"))) && (pR->getExit(DIR_OUT) > 0 || pR->exitStubs.contains(DIR_OUT))))) {
            // No there IS NOT a stub or real exit in the exitCmd direction
            return warnArgumentValue(L, __func__, qsl(
                "roomID %1 does not have a normal exit or a stub exit in direction '%2'")
                .arg(QString::number(roomId), exitCmd));
        }
        // else IS a valid stub or real normal exit -fall through to continue
    }

    const int doorStatus = getVerifiedInt(L, __func__, 3, "door type  {0='none', 1='open', 2='closed' or 3='locked'}");
    if (doorStatus < 0 || doorStatus > 3) {
        return warnArgumentValue(L, __func__, qsl(
            "door type %1 is not one of 0='none', 1='open', 2='closed' or 3='locked'").arg(doorStatus));
    }

    const bool result = pR->setDoor(exitCmd, doorStatus);
    if (result) {
        host.mpMap->setUnsaved(__func__);
        host.mpMap->update();
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setExit
int TLuaInterpreter::setExit(lua_State* L)
{
    const int from = getVerifiedInt(L, __func__, 1, "from roomID");
    const int to = getVerifiedInt(L, __func__, 2, "to roomID");

    const int dir = dirToNumber(L, 3);
    if (!dir) {
        lua_pushfstring(L, "setExit: bad argument #3 type (direction as number or string expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->setExit(from, to, dir));
    host.mpMap->mMapGraphNeedsUpdate = true;
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setExitStub
int TLuaInterpreter::setExitStub(lua_State* L)
{
    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    const int dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushfstring(L, "setExitStub: bad argument #2 type (direction as number or string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    const bool status = getVerifiedBool(L, __func__, 3, "set/unset");

    const Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return 0;
    }
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushstring(L, "setExitStub: roomId doesn't exist");
        return lua_error(L);
    }
    if (dir > 12 || dir < 1) {
        lua_pushstring(L, "setExitStub: direction must be between 1 and 12");
        return lua_error(L);
    }
    pR->setExitStub(dir, status);
    host.mpMap->update();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setExitWeight
int TLuaInterpreter::setExitWeight(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const int roomID = getVerifiedInt(L, __func__, 1, "roomID");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomID));
    }

    const QString direction(dirToString(L, 2));
    if (direction.isEmpty()) {
        lua_pushfstring(L, "setExitWeight: bad argument #2 type (direction as string or number {between 1 and 12 inclusive} expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (!pR->hasExitOrSpecialExit(direction)) {
        return warnArgumentValue(L, __func__, qsl("roomID %1 does not have an exit that can be identified from '%2'")
            .arg(QString::number(roomID), lua_tostring(L, 2)));
    }

    qint64 const weight = getVerifiedInt(L, __func__, 3, "exit weight");
    if (weight < 0 || weight > std::numeric_limits<int>::max()) {
        return warnArgumentValue(L, __func__, qsl(
            "weight %1 is outside of the usable range of 0 (which resets the weight back to that of the destination room) to %2")
            .arg(QString::number(weight), QString::number(std::numeric_limits<int>::max())));
    }

    pR->setExitWeight(direction, weight);
    lua_pushboolean(L, true);
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setGridMode
int TLuaInterpreter::setGridMode(lua_State* L)
{
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    const bool gridMode = getVerifiedBool(L, __func__, 2, "true/false");
    const Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushboolean(L, false);
        return 1;
    } else {
        pA->gridMode = gridMode;
        pA->calcSpan();
        host.mpMap->update();
    }
    host.mpMap->setUnsaved(__func__);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapUserData
int TLuaInterpreter::setMapUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const QString key = getVerifiedString(L, __func__, 1, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key is not allowed to be an empty string");
    }
    const QString value = getVerifiedString(L, __func__, 2, "value");

    host.mpMap->mUserData[key] = value;
    host.mpMap->setUnsaved(__func__);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapZoom
int TLuaInterpreter::setMapZoom(lua_State* L)
{
    const qreal zoom = getVerifiedDouble(L, __func__, 1, "zoom");
    int areaID = 0;
    if (lua_gettop(L) > 1) {
        areaID = getVerifiedInt(L, __func__, 2, "area id", true);
    }
    const Host& host = getHostFromLua(L);
    if (host.mpMap.isNull() || host.mpMap->mpMapper.isNull()) {
        return warnArgumentValue(L, __func__, "no map loaded or no active mapper");
    }

    auto [success, errMsg] = host.mpMap->mpMapper->mp2dMap->setMapZoom(zoom, areaID);
    if (!success) {
        return warnArgumentValue(L, __func__, errMsg.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomArea
int TLuaInterpreter::setRoomArea(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QVector<int> roomIds;
    if (lua_isnumber(L, 1)) {
        const int id = getVerifiedInt(L, __func__, 1, "roomID");
        if (!host.mpMap->mpRoomDB->getRoomIDList().contains(id)) {
            return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
        }
        roomIds.append(id);
    } else if (lua_istable(L, 1)) {
        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            const int id = getVerifiedInt(L, __func__, -1, "roomID");
            if (!host.mpMap->mpRoomDB->getRoomIDList().contains(id)) {
                return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
            }
            roomIds.append(id);
            lua_pop(L, 1);
        }
    } else {
        lua_pushfstring(L,
                        "setRoomArea: bad argument #1 type (roomID as number or table of roomIDs\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }

    int areaId = -1;
    QString areaName;
    if (lua_isnumber(L, 2)) {
        areaId = lua_tonumber(L, 2);
        if (areaId < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "number %1 is not a valid areaID greater than zero. "
                "To remove a room's area, use resetRoomArea(roomID)").arg(areaId));
        }
        if (!host.mpMap->mpRoomDB->getAreaNamesMap().contains(areaId)) {
            return warnArgumentValue(L, __func__, csmInvalidAreaID.arg(areaId));
        }
    } else if (lua_isstring(L, 2)) {
        areaName = lua_tostring(L, 2);
        // areaId will be zero if not found!
        if (areaName.isEmpty()) {
            return warnArgumentValue(L, __func__, "area name cannot be empty");
        }
        areaId = host.mpMap->mpRoomDB->getAreaNamesMap().key(areaName, 0);
        if (!areaId) {
            return warnArgumentValue(L, __func__, qsl("area name '%1' does not exist").arg(areaName));
        }
    } else {
        lua_pushfstring(L,
                        "setRoomArea: bad argument #2 type (areaID as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }

    const bool result = std::all_of(roomIds.begin(), roomIds.end(), [&](int id) {
        // defer area recalculation on all rooms until the last room (.back())
        return host.mpMap->setRoomArea(id, areaId, id != roomIds.back());
    });

    if (result) {
        host.mpMap->update();
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomChar
int TLuaInterpreter::setRoomChar(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const QString symbol = getVerifiedString(L, __func__, 2, "room symbol");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }

    if (symbol.isEmpty()) {
        // Allow an empty string to be used to clear the symbol:
        pR->mSymbol.clear();
    } else {
        // 10.0 is the maximum supported by the Qt versions (5.14+) we
        // handle/use/allow:
        pR->mSymbol = symbol.normalized(QString::NormalizationForm_C, QChar::Unicode_10_0);
    }
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomCharColor
int TLuaInterpreter::setRoomCharColor(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const int r = getVerifiedInt(L, __func__, 2, "red component");
    if (r < 0 || r > 255) {
        lua_pushfstring(L, "setRoomCharColor: bad argument #2 type (red component value %d out of range (0 to 255)", r);
        return lua_error(L);
    }
    const int g = getVerifiedInt(L, __func__, 3, "green component");
    if (g < 0 || g > 255) {
        lua_pushfstring(L, "setRoomCharColor: bad argument #3 type (red component value %d out of range (0 to 255)", r);
        return lua_error(L);
    }
    const int b = getVerifiedInt(L, __func__, 4, "blue component");
    if (b < 0 || b > 255) {
        lua_pushfstring(L, "setRoomCharColor: bad argument #4 type (blue component value %d out of range (0 to 255)", r);
        return lua_error(L);
    }

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }

    pR->mSymbolColor = QColor(r, g, b);
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomCoordinates
int TLuaInterpreter::setRoomCoordinates(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const int x = getVerifiedInt(L, __func__, 2, "x");
    const int y = getVerifiedInt(L, __func__, 3, "y");
    const int z = getVerifiedInt(L, __func__, 4, "z");
    const Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->setRoomCoordinates(id, x, y, z));
    host.mpMap->update();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomEnv
int TLuaInterpreter::setRoomEnv(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const int env = getVerifiedInt(L, __func__, 2, "environmentID");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }
    pR->environment = env;
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomIDbyHash
int TLuaInterpreter::setRoomIDbyHash(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const QString hash = getVerifiedString(L, __func__, 2, "hash");
    const Host& host = getHostFromLua(L);
    if (host.mpMap->mpRoomDB->roomIDToHash.contains(id)) {
        host.mpMap->mpRoomDB->hashToRoomID.remove(host.mpMap->mpRoomDB->roomIDToHash[id]);
    }
    if (host.mpMap->mpRoomDB->hashToRoomID.contains(hash)) {
        host.mpMap->mpRoomDB->roomIDToHash.remove(host.mpMap->mpRoomDB->hashToRoomID[hash]);
    }
    host.mpMap->mpRoomDB->hashToRoomID[hash] = id;
    host.mpMap->mpRoomDB->roomIDToHash[id] = hash;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomName
int TLuaInterpreter::setRoomName(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const QString name = getVerifiedString(L, __func__, 2, "room name", true);

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }
    pR->name = name;
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomUserData
int TLuaInterpreter::setRoomUserData(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const int roomId = getVerifiedInt(L, __func__, 1, "roomID");
    const QString key = getVerifiedString(L, __func__, 2, "key");
    // Ideally should reject empty keys but this could break existing scripts so we can't
    const QString value = getVerifiedString(L, __func__, 3, "value");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(roomId));
    }
    pR->userData[key] = value;
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomWeight
int TLuaInterpreter::setRoomWeight(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");
    const int w = getVerifiedInt(L, __func__, 2, "weight");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }

    pR->setWeight(w);
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    host.mpMap->mMapGraphNeedsUpdate = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#unHighlightRoom
int TLuaInterpreter::unHighlightRoom(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->highlight = false;
        host.mpMap->update();
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#unsetRoomCharColor
int TLuaInterpreter::unsetRoomCharColor(lua_State* L)
{
    const int id = getVerifiedInt(L, __func__, 1, "roomID");

    const Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, csmInvalidRoomID.arg(id));
    }

    // Reset it to the default (and invalid) QColor:
    pR->mSymbolColor = {};
    host.mpMap->setUnsaved(__func__);
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#updateMap
int TLuaInterpreter::updateMap(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (host.mpMap) {
        host.mpMap->update();
    }
    return 0;
}

int TLuaInterpreter::getCollisionLocationsInArea(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    const int area = getVerifiedInt(L, __func__, 1, "areaID");
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("areaID %1 not found").arg(QString::number(area)));
    }

    lua_newtable(L);
    int i = 0;
    for (const auto& coordinateSet : pA->getCollisionNodes()) {
        lua_pushnumber(L, ++i);
        {
            lua_newtable(L);

            // x:
            lua_pushnumber(L, 1);
            lua_pushnumber(L, std::get<0>(coordinateSet));
            lua_settable(L, -3);

            // y:
            lua_pushnumber(L, 2);
            lua_pushnumber(L, std::get<1>(coordinateSet));
            lua_settable(L, -3);

            // z:
            lua_pushnumber(L, 3);
            lua_pushnumber(L, std::get<2>(coordinateSet));
            lua_settable(L, -3);
        }
        lua_settable(L, -3);
    }
    return 1;

}
