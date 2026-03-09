/***************************************************************************
 *   Copyright (C) 2024 by Mudlet contributors                            *
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

#include "TLuaInterpreter.h"

#include "Host.h"
#include "TCanvas.h"
#include "TDockWidget.h"
#include "TLabel.h"
#include "TMainConsole.h"
#include "TScrollBox.h"

// createCanvas(parentName, canvasName, x, y, width, height)
// parentName: name of parent label, dock widget, or "main" for main window
// Returns true on success, false + error message on failure
int TLuaInterpreter::createCanvas(lua_State* L)
{
    const QString parentName = getVerifiedString(L, __func__, 1, "parent name");
    const QString canvasName = getVerifiedString(L, __func__, 2, "canvas name");
    const int x = getVerifiedInt(L, __func__, 3, "x-coordinate");
    const int y = getVerifiedInt(L, __func__, 4, "y-coordinate");
    const int width = getVerifiedInt(L, __func__, 5, "width");
    const int height = getVerifiedInt(L, __func__, 6, "height");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    // Check if canvas already exists
    if (host.mpConsole->mCanvasMap.contains(canvasName)) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' already exists").arg(canvasName), true);
    }

    // Resolve parent widget — match createLabel's resolution order:
    // dock widget → scroll box → mpMainFrame (labels also end up on mpMainFrame)
    QWidget* parentWidget = nullptr;

    if (parentName == QLatin1String("main") || parentName.isEmpty()) {
        parentWidget = host.mpConsole->mpMainFrame;
    } else {
        auto pW = host.mpConsole->mDockWidgetMap.value(parentName);
        if (pW) {
            parentWidget = pW->widget();
        } else {
            auto pS = host.mpConsole->mScrollBoxMap.value(parentName);
            if (pS) {
                parentWidget = pS->widget();
            } else {
                // Labels created with a label parent also land on mpMainFrame
                // (createLabel doesn't check mLabelMap for parent resolution)
                // so we do the same to ensure canvas and labels are siblings
                parentWidget = host.mpConsole->mpMainFrame;
            }
        }
    }

    auto* canvas = new TCanvas(parentWidget);
    canvas->mName = canvasName;
    canvas->move(x, y);
    canvas->resize(width, height);
    canvas->show();

    host.mpConsole->mCanvasMap[canvasName] = canvas;

    lua_pushboolean(L, true);
    return 1;
}

// deleteCanvas(canvasName)
int TLuaInterpreter::deleteCanvas(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.take(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->deleteLater();
    lua_pushboolean(L, true);
    return 1;
}

// canvasDrawLine(canvasName, x1, y1, x2, y2, r, g, b[, a[, lineWidth]])
int TLuaInterpreter::canvasDrawLine(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");
    const double x1 = getVerifiedDouble(L, __func__, 2, "x1");
    const double y1 = getVerifiedDouble(L, __func__, 3, "y1");
    const double x2 = getVerifiedDouble(L, __func__, 4, "x2");
    const double y2 = getVerifiedDouble(L, __func__, 5, "y2");
    const int r = getVerifiedInt(L, __func__, 6, "red");
    const int g = getVerifiedInt(L, __func__, 7, "green");
    const int b = getVerifiedInt(L, __func__, 8, "blue");

    int a = 255;
    if (lua_gettop(L) >= 9 && lua_isnumber(L, 9)) {
        a = static_cast<int>(lua_tointeger(L, 9));
    }

    double lineWidth = 1.0;
    if (lua_gettop(L) >= 10 && lua_isnumber(L, 10)) {
        lineWidth = lua_tonumber(L, 10);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->addLine(x1, y1, x2, y2, QColor(r, g, b, a), lineWidth);

    lua_pushboolean(L, true);
    return 1;
}

// canvasDrawRect(canvasName, x, y, w, h, r, g, b[, a[, lineWidth[, fillR, fillG, fillB[, fillA]]]])
int TLuaInterpreter::canvasDrawRect(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");
    const double x = getVerifiedDouble(L, __func__, 2, "x");
    const double y = getVerifiedDouble(L, __func__, 3, "y");
    const double w = getVerifiedDouble(L, __func__, 4, "width");
    const double h = getVerifiedDouble(L, __func__, 5, "height");
    const int r = getVerifiedInt(L, __func__, 6, "red");
    const int g = getVerifiedInt(L, __func__, 7, "green");
    const int b = getVerifiedInt(L, __func__, 8, "blue");

    int a = 255;
    if (lua_gettop(L) >= 9 && lua_isnumber(L, 9)) {
        a = static_cast<int>(lua_tointeger(L, 9));
    }

    double lineWidth = 1.0;
    if (lua_gettop(L) >= 10 && lua_isnumber(L, 10)) {
        lineWidth = lua_tonumber(L, 10);
    }

    QColor fillColor = Qt::transparent;
    if (lua_gettop(L) >= 13) {
        int fr = getVerifiedInt(L, __func__, 11, "fill red");
        int fg = getVerifiedInt(L, __func__, 12, "fill green");
        int fb = getVerifiedInt(L, __func__, 13, "fill blue");
        int fa = 255;
        if (lua_gettop(L) >= 14 && lua_isnumber(L, 14)) {
            fa = static_cast<int>(lua_tointeger(L, 14));
        }
        fillColor = QColor(fr, fg, fb, fa);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->addRect(x, y, w, h, QColor(r, g, b, a), lineWidth, fillColor);

    lua_pushboolean(L, true);
    return 1;
}

// canvasDrawEllipse(canvasName, cx, cy, rx, ry, r, g, b[, a[, lineWidth[, fillR, fillG, fillB[, fillA]]]])
int TLuaInterpreter::canvasDrawEllipse(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");
    const double cx = getVerifiedDouble(L, __func__, 2, "center x");
    const double cy = getVerifiedDouble(L, __func__, 3, "center y");
    const double rx = getVerifiedDouble(L, __func__, 4, "radius x");
    const double ry = getVerifiedDouble(L, __func__, 5, "radius y");
    const int r = getVerifiedInt(L, __func__, 6, "red");
    const int g = getVerifiedInt(L, __func__, 7, "green");
    const int b = getVerifiedInt(L, __func__, 8, "blue");

    int a = 255;
    if (lua_gettop(L) >= 9 && lua_isnumber(L, 9)) {
        a = static_cast<int>(lua_tointeger(L, 9));
    }

    double lineWidth = 1.0;
    if (lua_gettop(L) >= 10 && lua_isnumber(L, 10)) {
        lineWidth = lua_tonumber(L, 10);
    }

    QColor fillColor = Qt::transparent;
    if (lua_gettop(L) >= 13) {
        int fr = getVerifiedInt(L, __func__, 11, "fill red");
        int fg = getVerifiedInt(L, __func__, 12, "fill green");
        int fb = getVerifiedInt(L, __func__, 13, "fill blue");
        int fa = 255;
        if (lua_gettop(L) >= 14 && lua_isnumber(L, 14)) {
            fa = static_cast<int>(lua_tointeger(L, 14));
        }
        fillColor = QColor(fr, fg, fb, fa);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->addEllipse(cx, cy, rx, ry, QColor(r, g, b, a), lineWidth, fillColor);

    lua_pushboolean(L, true);
    return 1;
}

// canvasClear(canvasName)
int TLuaInterpreter::canvasClear(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->clearDrawings();

    lua_pushboolean(L, true);
    return 1;
}

// canvasSetClickThrough(canvasName, clickThrough)
int TLuaInterpreter::canvasSetClickThrough(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");
    const bool clickThrough = getVerifiedBool(L, __func__, 2, "click through");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->setClickThrough(clickThrough);

    lua_pushboolean(L, true);
    return 1;
}

// moveCanvas(canvasName, x, y)
int TLuaInterpreter::moveCanvas(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");
    const int x = getVerifiedInt(L, __func__, 2, "x-coordinate");
    const int y = getVerifiedInt(L, __func__, 3, "y-coordinate");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->move(x, y);

    lua_pushboolean(L, true);
    return 1;
}

// resizeCanvas(canvasName, width, height)
int TLuaInterpreter::resizeCanvas(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");
    const int width = getVerifiedInt(L, __func__, 2, "width");
    const int height = getVerifiedInt(L, __func__, 3, "height");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->resize(width, height);

    lua_pushboolean(L, true);
    return 1;
}

// showCanvas(canvasName)
int TLuaInterpreter::showCanvas(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->show();

    lua_pushboolean(L, true);
    return 1;
}

// hideCanvas(canvasName)
int TLuaInterpreter::hideCanvas(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->hide();

    lua_pushboolean(L, true);
    return 1;
}

// raiseCanvas(canvasName)
int TLuaInterpreter::raiseCanvas(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->raise();

    lua_pushboolean(L, true);
    return 1;
}

// lowerCanvas(canvasName)
int TLuaInterpreter::lowerCanvas(lua_State* L)
{
    const QString canvasName = getVerifiedString(L, __func__, 1, "canvas name");

    Host& host = getHostFromLua(L);
    if (!host.mpConsole) {
        return warnArgumentValue(L, __func__, "main console not available", true);
    }

    auto* canvas = host.mpConsole->mCanvasMap.value(canvasName);
    if (!canvas) {
        return warnArgumentValue(L, __func__, qsl("canvas '%1' not found").arg(canvasName), true);
    }

    canvas->lower();

    lua_pushboolean(L, true);
    return 1;
}
