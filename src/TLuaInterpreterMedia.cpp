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

// media-specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

#include "TLuaInterpreter.h"

#include "Host.h"
#include "TEvent.h"
#include "TMedia.h"
#include "mudlet.h"

// The argument parsers below hold QStrings and TMediaData while they run, so
// they type-check with TLuaInterpreter::check...Arg() and leave the raise until
// the parsing scope has been left - see checkStringArg()

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#receiveMSP
int TLuaInterpreter::receiveMSP(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!host.mTelnet.isMSPEnabled()) {
        return warnArgumentValue(L, __func__, "MSP is not currently enabled");
    }

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "receiveMSP: bad argument #1 type (message as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    const std::string msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));
    host.mTelnet.setMSPVariables(QByteArray(msg.c_str(), msg.length()));

    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::loadMediaFileAsOrderedArguments(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);
    const int numArgs = lua_gettop(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};
        QString stringValue;

        // name[,url])
        for (int i = 1; i <= numArgs && !errorPushed; i++) {
            if (lua_isnil(L, i)) {
                continue;
            }

            switch (i) {
            case 1:
                if (!checkStringArg(L, func, i, "name")) {
                    errorPushed = true;
                    break;
                }

                stringValue = lua_tostring(L, i);

                if (QDir::homePath().contains('\\')) {
                    stringValue.replace('/', R"(\)");
                } else {
                    stringValue.replace('\\', "/");
                }

                mediaData.setMediaFileName(stringValue);
                break;
            case 2:
                if (!checkStringArg(L, func, i, "url")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaUrl(QString{lua_tostring(L, i)});
                break;
            }
        }

        if (!errorPushed) {
            if (mediaData.mediaFileName().isEmpty()) {
                return warnArgumentValue(L, func, QLatin1String("missing argument 1 (file to play)"));
            }

            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

            host.mpMedia->playMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::loadMediaFileAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("url")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : "value for url")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("url") && !value.isEmpty()) {
                    mediaData.setMediaUrl(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            if (mediaData.mediaFileName().isEmpty()) {
                lua_pushstring(L, R"(loadMusicFile: missing name (add name = "file to play"))");
                errorPushed = true;
            } else {
                mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
                mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

                host.mpMedia->playMedia(mediaData);
            }
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadMusicFile
int TLuaInterpreter::loadMusicFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return loadMediaFileAsTableArgument(L, __func__);
    }

    return loadMediaFileAsOrderedArguments(L, __func__);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadSoundFile
int TLuaInterpreter::loadSoundFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return loadMediaFileAsTableArgument(L, __func__);
    }

    return loadMediaFileAsOrderedArguments(L, __func__);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadVideoFile
int TLuaInterpreter::loadVideoFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (!lua_istable(L, 1)) {
        lua_pushfstring(L, "%s: needs to be a table", __func__);
        return lua_error(L);
    }

    return loadMediaFileAsTableArgument(L, __func__);
}

// Private
int TLuaInterpreter::playMusicFileAsOrderedArguments(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);
    const int numArgs = lua_gettop(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};
        QString stringValue;
        int intValue = 0;

        // name[,volume][,fadein][,fadeout][,start][,loops][,key][,tag][,continue][,url][,finish]
        for (int i = 1; i <= numArgs && !errorPushed; i++) {
            if (lua_isnil(L, i)) {
                continue;
            }

            switch (i) {
            case 1:
                if (!checkStringArg(L, func, i, "name")) {
                    errorPushed = true;
                    break;
                }

                stringValue = lua_tostring(L, i);

                if (QDir::homePath().contains('\\')) {
                    stringValue.replace('/', R"(\)");
                } else {
                    stringValue.replace('\\', "/");
                }

                mediaData.setMediaFileName(stringValue);
                break;
            case 2:
                if (!checkIntArg(L, func, i, "volume")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue == TMediaData::MediaVolumePreload) {
                    {
                    } // Volume of 0 supports preloading
                } else if (intValue > TMediaData::MediaVolumeMax) {
                    intValue = TMediaData::MediaVolumeMax;
                } else if (intValue < TMediaData::MediaVolumeMin) {
                    intValue = TMediaData::MediaVolumeMin;
                }

                mediaData.setMediaVolume(intValue);
                break;
            case 3:
                if (!checkIntArg(L, func, i, "fadein")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeIn(intValue);
                break;
            case 4:
                if (!checkIntArg(L, func, i, "fadeout")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeOut(intValue);
                break;
            case 5:
                if (!checkIntArg(L, func, i, "start")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaStart(intValue);
                break;
            case 6:
                if (!checkIntArg(L, func, i, "loops")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < TMediaData::MediaLoopsRepeat || intValue == 0) {
                    intValue = TMediaData::MediaLoopsDefault;
                }

                mediaData.setMediaLoops(intValue);
                break;
            case 7:
                if (!checkStringArg(L, func, i, "key")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaKey(QString{lua_tostring(L, i)});
                break;
            case 8:
                if (!checkStringArg(L, func, i, "tag")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaTag(QString{lua_tostring(L, i)});
                break;
            case 9:
                if (!checkBoolArg(L, func, i, "continue")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaContinue(lua_toboolean(L, i));
                break;
            case 10:
                if (!checkStringArg(L, func, i, "url")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaUrl(QString{lua_tostring(L, i)});
                break;
            case 11:
                if (!checkIntArg(L, func, i, "finish")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFinish(intValue);
                break;
            }
        }

        if (!errorPushed) {
            if (mediaData.mediaFileName().isEmpty()) {
                return warnArgumentValue(L, func, QLatin1String("missing argument 1 (file to play)"));
            }

            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeMusic);
            host.mpMedia->playMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::playMusicFileAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("url") || key == QLatin1String("key") || key == QLatin1String("tag") || key == QLatin1String("caption")) {
                if (!checkStringArg(L,
                                    func,
                                    -1,
                                    key == QLatin1String("name")      ? "value for name"
                                    : key == QLatin1String("key")     ? "value for key"
                                    : key == QLatin1String("tag")     ? "value for tag"
                                    : key == QLatin1String("caption") ? "value for caption"
                                                                      : "value for url")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("url") && !value.isEmpty()) {
                    mediaData.setMediaUrl(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                } else if (key == QLatin1String("caption") && !value.isEmpty()) {
                    mediaData.setMediaCaption(value);
                }
            } else if (key == QLatin1String("volume") || key == QLatin1String("fadein") || key == QLatin1String("fadeout") || key == QLatin1String("start") || key == QLatin1String("finish")
                       || key == QLatin1String("loops")) {
                if (!checkIntArg(L,
                                 func,
                                 -1,
                                 key == QLatin1String("volume")    ? "value for volume"
                                 : key == QLatin1String("fadein")  ? "value for fadein"
                                 : key == QLatin1String("fadeout") ? "value for fadeout"
                                 : key == QLatin1String("start")   ? "value for start"
                                 : key == QLatin1String("finish")  ? "value for finish"
                                                                   : "value for loops")) {
                    errorPushed = true;
                    break;
                }

                int value = static_cast<int>(lua_tointeger(L, -1));

                if (key == QLatin1String("volume")) {
                    if (value == TMediaData::MediaVolumePreload) {
                        {
                        } // Volume of 0 supports preloading
                    } else if (value > TMediaData::MediaVolumeMax) {
                        value = TMediaData::MediaVolumeMax;
                    } else if (value < TMediaData::MediaVolumeMin) {
                        value = TMediaData::MediaVolumeMin;
                    }

                    mediaData.setMediaVolume(value);
                } else if (key == QLatin1String("fadein")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaFadeIn(value);
                } else if (key == QLatin1String("fadeout")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaFadeOut(value);
                } else if (key == QLatin1String("start")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaStart(value);
                } else if (key == QLatin1String("finish")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaFinish(value);
                } else if (key == QLatin1String("loops")) {
                    if (value < TMediaData::MediaLoopsRepeat || value == 0) {
                        value = TMediaData::MediaLoopsDefault;
                    }

                    mediaData.setMediaLoops(value);
                }
            } else if (key == QLatin1String("continue")) {
                if (!checkBoolArg(L, func, -1, "value for continue")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaContinue(lua_toboolean(L, -1));
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            if (mediaData.mediaFileName().isEmpty()) {
                lua_pushstring(L, R"(playMusicFile: missing name (add name = "file to play"))");
                errorPushed = true;
            } else {
                mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
                mediaData.setMediaType(TMediaData::MediaTypeMusic);
                host.mpMedia->playMedia(mediaData);
            }
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playMusicFile
int TLuaInterpreter::playMusicFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return playMusicFileAsTableArgument(L, __func__);
    }

    return playMusicFileAsOrderedArguments(L, __func__);
}

// Private
int TLuaInterpreter::playSoundFileAsOrderedArguments(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);
    const int numArgs = lua_gettop(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};
        QString stringValue;
        int intValue = 0;

        // name[,volume][,fadein][,fadeout][,start][,loops][,key][,tag][,priority][,url][,finish]
        for (int i = 1; i <= numArgs && !errorPushed; i++) {
            if (lua_isnil(L, i)) {
                continue;
            }

            switch (i) {
            case 1:
                if (!checkStringArg(L, func, i, "name")) {
                    errorPushed = true;
                    break;
                }

                stringValue = lua_tostring(L, i);

                if (QDir::homePath().contains('\\')) {
                    stringValue.replace('/', R"(\)");
                } else {
                    stringValue.replace('\\', "/");
                }

                mediaData.setMediaFileName(stringValue);
                break;
            case 2:
                if (!checkIntArg(L, func, i, "volume")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue == TMediaData::MediaVolumePreload) {
                    {
                    } // Volume of 0 supports preloading
                } else if (intValue > TMediaData::MediaVolumeMax) {
                    intValue = TMediaData::MediaVolumeMax;
                } else if (intValue < TMediaData::MediaVolumeMin) {
                    intValue = TMediaData::MediaVolumeMin;
                }

                mediaData.setMediaVolume(intValue);
                break;
            case 3:
                if (!checkIntArg(L, func, i, "fadein")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeIn(intValue);
                break;
            case 4:
                if (!checkIntArg(L, func, i, "fadeout")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeOut(intValue);
                break;
            case 5:
                if (!checkIntArg(L, func, i, "start")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaStart(intValue);
                break;
            case 6:
                if (!checkIntArg(L, func, i, "loops")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < TMediaData::MediaLoopsRepeat || intValue == 0) {
                    intValue = TMediaData::MediaLoopsDefault;
                }

                mediaData.setMediaLoops(intValue);
                break;
            case 7:
                if (!checkStringArg(L, func, i, "key")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaKey(QString{lua_tostring(L, i)});
                break;
            case 8:
                if (!checkStringArg(L, func, i, "tag")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaTag(QString{lua_tostring(L, i)});
                break;
            case 9:
                if (!checkIntArg(L, func, i, "priority")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue > TMediaData::MediaPriorityMax) {
                    intValue = TMediaData::MediaPriorityMax;
                } else if (intValue < TMediaData::MediaPriorityMin) {
                    intValue = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(intValue);
                break;
            case 10:
                if (!checkStringArg(L, func, i, "url")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaUrl(QString{lua_tostring(L, i)});
                break;
            case 11:
                if (!checkIntArg(L, func, i, "finish")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFinish(intValue);
                break;
            }
        }

        if (!errorPushed) {
            if (mediaData.mediaFileName().isEmpty()) {
                return warnArgumentValue(L, func, QLatin1String("missing argument 1 (file to play)"));
            }

            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeSound);

            host.mpMedia->playMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::playSoundFileAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("url") || key == QLatin1String("key") || key == QLatin1String("tag") || key == QLatin1String("caption")) {
                if (!checkStringArg(L,
                                    func,
                                    -1,
                                    key == QLatin1String("name")      ? "value for name"
                                    : key == QLatin1String("key")     ? "value for key"
                                    : key == QLatin1String("tag")     ? "value for tag"
                                    : key == QLatin1String("caption") ? "value for caption"
                                                                      : "value for url")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("url") && !value.isEmpty()) {
                    mediaData.setMediaUrl(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                } else if (key == QLatin1String("caption") && !value.isEmpty()) {
                    mediaData.setMediaCaption(value);
                }
            } else if (key == QLatin1String("volume") || key == QLatin1String("fadein") || key == QLatin1String("fadeout") || key == QLatin1String("start") || key == QLatin1String("finish")
                       || key == QLatin1String("loops") || key == QLatin1String("priority")) {
                if (!checkIntArg(L,
                                 func,
                                 -1,
                                 key == QLatin1String("volume")    ? "value for volume"
                                 : key == QLatin1String("fadein")  ? "value for fadein"
                                 : key == QLatin1String("fadeout") ? "value for fadeout"
                                 : key == QLatin1String("start")   ? "value for start"
                                 : key == QLatin1String("finish")  ? "value for finish"
                                 : key == QLatin1String("loops")   ? "value for loops"
                                                                   : "value for priority")) {
                    errorPushed = true;
                    break;
                }

                int value = static_cast<int>(lua_tointeger(L, -1));

                if (key == QLatin1String("volume")) {
                    if (value == TMediaData::MediaVolumePreload) {
                        {
                        } // Volume of 0 supports preloading
                    } else if (value > TMediaData::MediaVolumeMax) {
                        value = TMediaData::MediaVolumeMax;
                    } else if (value < TMediaData::MediaVolumeMin) {
                        value = TMediaData::MediaVolumeMin;
                    }

                    mediaData.setMediaVolume(value);
                } else if (key == QLatin1String("fadein")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaFadeIn(value);
                } else if (key == QLatin1String("fadeout")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaFadeOut(value);
                } else if (key == QLatin1String("start")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaStart(value);
                } else if (key == QLatin1String("finish")) {
                    if (value < 0) {
                        lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaFinish(value);
                } else if (key == QLatin1String("loops")) {
                    if (value < TMediaData::MediaLoopsRepeat || value == 0) {
                        value = TMediaData::MediaLoopsDefault;
                    }

                    mediaData.setMediaLoops(value);
                } else if (key == QLatin1String("priority")) {
                    if (value > TMediaData::MediaPriorityMax) {
                        value = TMediaData::MediaPriorityMax;
                    } else if (value < TMediaData::MediaPriorityMin) {
                        value = TMediaData::MediaPriorityMin;
                    }

                    mediaData.setMediaPriority(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            if (mediaData.mediaFileName().isEmpty()) {
                lua_pushstring(L, R"(playSoundFile: missing name (add name = "file to play"))");
                errorPushed = true;
            } else {
                mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
                mediaData.setMediaType(TMediaData::MediaTypeSound);

                host.mpMedia->playMedia(mediaData);
            }
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playSoundFile
int TLuaInterpreter::playSoundFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return playSoundFileAsTableArgument(L, __func__);
    }

    return playSoundFileAsOrderedArguments(L, __func__);
}

// Private
int TLuaInterpreter::playVideoFileAsTableArgument(lua_State* L, const char* func)
{
    Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key{lua_tostring(L, -1)};
            lua_pop(L, 1);

            if (!key.compare(QLatin1String("name"), Qt::CaseInsensitive) || !key.compare(QLatin1String("url"), Qt::CaseInsensitive) || !key.compare(QLatin1String("key"), Qt::CaseInsensitive)
                || !key.compare(QLatin1String("tag"), Qt::CaseInsensitive)) {
                if (!checkStringArg(L,
                                    func,
                                    -1,
                                    !key.compare(QLatin1String("name"), Qt::CaseInsensitive)  ? "value for name"
                                    : !key.compare(QLatin1String("key"), Qt::CaseInsensitive) ? "value for key"
                                    : !key.compare(QLatin1String("tag"), Qt::CaseInsensitive) ? "value for tag"
                                                                                              : "value for url")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (!key.compare(QLatin1String("name"), Qt::CaseInsensitive) && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (!key.compare(QLatin1String("url"), Qt::CaseInsensitive) && !value.isEmpty()) {
                    mediaData.setMediaUrl(value);
                } else if (!key.compare(QLatin1String("key"), Qt::CaseInsensitive) && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (!key.compare(QLatin1String("tag"), Qt::CaseInsensitive) && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            } else if (!key.compare(QLatin1String("volume"), Qt::CaseInsensitive) || !key.compare(QLatin1String("start"), Qt::CaseInsensitive)
                       || !key.compare(QLatin1String("finish"), Qt::CaseInsensitive) || !key.compare(QLatin1String("loops"), Qt::CaseInsensitive)) {
                if (!checkIntArg(L,
                                 func,
                                 -1,
                                 !key.compare(QLatin1String("volume"), Qt::CaseInsensitive)   ? "value for volume"
                                 : !key.compare(QLatin1String("start"), Qt::CaseInsensitive)  ? "value for start"
                                 : !key.compare(QLatin1String("finish"), Qt::CaseInsensitive) ? "value for finish"
                                                                                              : "value for loops")) {
                    errorPushed = true;
                    break;
                }

                int value = static_cast<int>(lua_tointeger(L, -1));

                if (!key.compare(QLatin1String("volume"), Qt::CaseInsensitive)) {
                    if (value != TMediaData::MediaVolumePreload) {
                        value = qBound(static_cast<int>(TMediaData::MediaVolumeMin), value, static_cast<int>(TMediaData::MediaVolumeMax));
                    }

                    mediaData.setMediaVolume(value);
                } else if (!key.compare(QLatin1String("start"), Qt::CaseInsensitive)) {
                    if (value < 0) {
                        lua_pushfstring(L, "playVideoFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaStart(value);
                } else if (!key.compare(QLatin1String("finish"), Qt::CaseInsensitive)) {
                    if (value < 0) {
                        lua_pushfstring(L, "playVideoFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", value);
                        errorPushed = true;
                        break;
                    }

                    mediaData.setMediaFinish(value);
                } else if (!key.compare(QLatin1String("loops"), Qt::CaseInsensitive)) {
                    if (value < TMediaData::MediaLoopsRepeat || value == 0) {
                        value = TMediaData::MediaLoopsDefault;
                    }

                    mediaData.setMediaLoops(value);
                }
            } else if (!key.compare(QLatin1String("continue"), Qt::CaseInsensitive)) {
                if (!checkBoolArg(L, func, -1, "value for continue")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaContinue(lua_toboolean(L, -1));
            } else if (!key.compare(QLatin1String("stream"), Qt::CaseInsensitive)) {
                if (!checkBoolArg(L, func, -1, "value for stream")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaInput(lua_toboolean(L, -1) ? TMediaData::MediaInputStream : TMediaData::MediaInputNotSet);
            } else if (!key.compare(QLatin1String("close"), Qt::CaseInsensitive)) {
                if (!checkBoolArg(L, func, -1, "value for close")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaClose(lua_toboolean(L, -1) ? TMediaData::MediaCloseEnabled : TMediaData::MediaCloseDefault);
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            if (mediaData.mediaFileName().isEmpty()) {
                lua_pushstring(L, R"(playVideoFile: missing name (add name = "file to play"))");
                errorPushed = true;
            } else {
                mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
                mediaData.setMediaType(TMediaData::MediaTypeVideo);
                host.mpMedia->playMedia(mediaData);
            }
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playVideoFile
int TLuaInterpreter::playVideoFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (!lua_istable(L, 1)) {
        lua_pushfstring(L, "%s: needs to be a table", __func__);
        return lua_error(L);
    }

    return playVideoFileAsTableArgument(L, __func__);
}

// Private
void TLuaInterpreter::processPlayingMediaTable(lua_State* L, TMediaData& mediaData)
{
    const Host& host = getHostFromLua(L);
    QList<TMediaData> const matchingMediaDataList = host.mpMedia->playingMedia(mediaData);

    int index = 1;
    lua_newtable(L);

    QListIterator<TMediaData> itTMediaData(matchingMediaDataList);

    while (itTMediaData.hasNext()) {
        TMediaData const matchedMediaData = itTMediaData.next();
        lua_pushinteger(L, index++);
        lua_newtable(L);

        if (!matchedMediaData.mediaFileName().isEmpty()) {
            lua_pushstring(L, "name");
            lua_pushstring(L, matchedMediaData.mediaFileName().toUtf8().constData());
            lua_settable(L, -3);
        }

        if (matchedMediaData.mediaVolume() != TMediaData::MediaVolumePreload) {
            lua_pushstring(L, "volume");
            lua_pushinteger(L, matchedMediaData.mediaVolume());
            lua_settable(L, -3);
        }

        if (matchedMediaData.mediaPriority() != TMediaData::MediaPriorityNotSet) {
            lua_pushstring(L, "priority");
            lua_pushinteger(L, matchedMediaData.mediaPriority());
            lua_settable(L, -3);
        }

        if (!matchedMediaData.mediaTag().isEmpty()) {
            lua_pushstring(L, "tag");
            lua_pushstring(L, matchedMediaData.mediaTag().toUtf8().constData());
            lua_settable(L, -3);
        }

        if (!matchedMediaData.mediaKey().isEmpty()) {
            lua_pushstring(L, "key");
            lua_pushstring(L, matchedMediaData.mediaKey().toUtf8().constData());
            lua_settable(L, -3);
        }

        lua_settable(L, -3);
    }
}

// Private
int TLuaInterpreter::getPlayingMusicAsOrderedArguments(lua_State* L, const char* func)
{
    const int numArgs = lua_gettop(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};
        QString stringValue;

        // values as ordered args: name[,key][,tag]
        for (int i = 1; i <= numArgs && !errorPushed; i++) {
            if (lua_isnil(L, i)) {
                continue;
            }

            switch (i) {
            case 1:
                if (!checkStringArg(L, func, i, "name")) {
                    errorPushed = true;
                    break;
                }

                stringValue = lua_tostring(L, i);

                if (QDir::homePath().contains('\\')) {
                    stringValue.replace('/', R"(\)");
                } else {
                    stringValue.replace('\\', "/");
                }

                mediaData.setMediaFileName(stringValue);
                break;
            case 2:
                if (!checkStringArg(L, func, i, "key")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaKey(QString{lua_tostring(L, i)});
                break;
            case 3:
                if (!checkStringArg(L, func, i, "tag")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaTag(QString{lua_tostring(L, i)});
                break;
            }
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeMusic);

            processPlayingMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Private
int TLuaInterpreter::getPlayingMusicAsTableArgument(lua_State* L, const char* func)
{
    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeMusic);

            processPlayingMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayingMusic
int TLuaInterpreter::getPlayingMusic(lua_State* L)
{
    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return getPlayingMusicAsTableArgument(L, __func__);
        }

        return getPlayingMusicAsOrderedArguments(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::getPlayingSoundsAsOrderedArguments(lua_State* L, const char* func)
{
    const int numArgs = lua_gettop(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};
        QString stringValue;
        int intValue = 0;

        // values as ordered args: name[,key][,tag][,priority])
        for (int i = 1; i <= numArgs && !errorPushed; i++) {
            if (lua_isnil(L, i)) {
                continue;
            }

            switch (i) {
            case 1:
                if (!checkStringArg(L, func, i, "name")) {
                    errorPushed = true;
                    break;
                }

                stringValue = lua_tostring(L, i);

                if (QDir::homePath().contains('\\')) {
                    stringValue.replace('/', R"(\)");
                } else {
                    stringValue.replace('\\', "/");
                }

                mediaData.setMediaFileName(stringValue);
                break;
            case 2:
                if (!checkStringArg(L, func, i, "key")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaKey(QString{lua_tostring(L, i)});
                break;
            case 3:
                if (!checkStringArg(L, func, i, "tag")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaTag(QString{lua_tostring(L, i)});
                break;
            case 4:
                if (!checkIntArg(L, func, i, "priority")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue > TMediaData::MediaPriorityMax) {
                    intValue = TMediaData::MediaPriorityMax;
                } else if (intValue < TMediaData::MediaPriorityMin) {
                    intValue = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(intValue);
                break;
            }
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeSound);

            processPlayingMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Private
int TLuaInterpreter::getPlayingSoundsAsTableArgument(lua_State* L, const char* func)
{
    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            } else if (key == QLatin1String("priority")) {
                if (!checkIntArg(L, func, -1, "value for priority")) {
                    errorPushed = true;
                    break;
                }

                int value = static_cast<int>(lua_tointeger(L, -1));

                if (value > TMediaData::MediaPriorityMax) {
                    value = TMediaData::MediaPriorityMax;
                } else if (value < TMediaData::MediaPriorityMin) {
                    value = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(value);
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeSound);

            processPlayingMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayingSounds
int TLuaInterpreter::getPlayingSounds(lua_State* L)
{
    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return getPlayingSoundsAsTableArgument(L, __func__);
        }

        return getPlayingSoundsAsOrderedArguments(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::getPlayingVideosAsTableArgument(lua_State* L, const char* func)
{
    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeVideo);

            processPlayingMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayingVideos
int TLuaInterpreter::getPlayingVideos(lua_State* L)
{
    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return getPlayingVideosAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeVideo);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Private
void TLuaInterpreter::processPausedMediaTable(lua_State* L, TMediaData& mediaData)
{
    const Host& host = getHostFromLua(L);
    QList<TMediaData> const matchingMediaDataList = host.mpMedia->pausedMedia(mediaData);

    int index = 1;
    lua_newtable(L);

    QListIterator<TMediaData> itTMediaData(matchingMediaDataList);

    while (itTMediaData.hasNext()) {
        TMediaData const matchedMediaData = itTMediaData.next();
        lua_pushinteger(L, index++);
        lua_newtable(L);

        if (!matchedMediaData.mediaFileName().isEmpty()) {
            lua_pushstring(L, "name");
            lua_pushstring(L, matchedMediaData.mediaFileName().toUtf8().constData());
            lua_settable(L, -3);
        }

        if (matchedMediaData.mediaVolume() != TMediaData::MediaVolumePreload) {
            lua_pushstring(L, "volume");
            lua_pushinteger(L, matchedMediaData.mediaVolume());
            lua_settable(L, -3);
        }

        if (matchedMediaData.mediaPriority() != TMediaData::MediaPriorityNotSet) {
            lua_pushstring(L, "priority");
            lua_pushinteger(L, matchedMediaData.mediaPriority());
            lua_settable(L, -3);
        }

        if (!matchedMediaData.mediaTag().isEmpty()) {
            lua_pushstring(L, "tag");
            lua_pushstring(L, matchedMediaData.mediaTag().toUtf8().constData());
            lua_settable(L, -3);
        }

        if (!matchedMediaData.mediaKey().isEmpty()) {
            lua_pushstring(L, "key");
            lua_pushstring(L, matchedMediaData.mediaKey().toUtf8().constData());
            lua_settable(L, -3);
        }

        lua_settable(L, -3);
    }
}

// Private
int TLuaInterpreter::getPausedSoundsAsTableArgument(lua_State* L, const char* func)
{
    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeSound);

            processPausedMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPausedSounds
int TLuaInterpreter::getPausedSounds(lua_State* L)
{
    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return getPausedSoundsAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    processPausedMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::getPausedMusicAsTableArgument(lua_State* L, const char* func)
{
    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeMusic);

            processPausedMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPausedMusic
int TLuaInterpreter::getPausedMusic(lua_State* L)
{
    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return getPausedMusicAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    processPausedMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::getPausedVideosAsTableArgument(lua_State* L, const char* func)
{
    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeVideo);

            processPausedMediaTable(L, mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPausedVideos
int TLuaInterpreter::getPausedVideos(lua_State* L)
{
    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return getPausedVideosAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeVideo);

    processPausedMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::stopMusicAsOrderedArguments(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);
    const int numArgs = lua_gettop(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};
        QString stringValue;
        int intValue = 0;

        // values as ordered args: name[,key][,tag][,fadeaway][,fadeout]
        for (int i = 1; i <= numArgs && !errorPushed; i++) {
            if (lua_isnil(L, i)) {
                continue;
            }

            switch (i) {
            case 1:
                if (!checkStringArg(L, func, i, "name")) {
                    errorPushed = true;
                    break;
                }

                stringValue = lua_tostring(L, i);

                if (QDir::homePath().contains('\\')) {
                    stringValue.replace('/', R"(\)");
                } else {
                    stringValue.replace('\\', "/");
                }

                mediaData.setMediaFileName(stringValue);
                break;
            case 2:
                if (!checkStringArg(L, func, i, "key")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaKey(QString{lua_tostring(L, i)});
                break;
            case 3:
                if (!checkStringArg(L, func, i, "tag")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaTag(QString{lua_tostring(L, i)});
                break;
            case 4:
                if (!checkBoolArg(L, func, i, "fadeaway")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeAway(lua_toboolean(L, i));
                break;
            case 5:
                if (!checkIntArg(L, func, i, "fadeout")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "stopMusic: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeOut(intValue);
                break;
            }
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeMusic);

            host.mpMedia->stopMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopMusicAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            } else if (key == QLatin1String("fadeaway")) {
                if (!checkBoolArg(L, func, -1, "value for fadeaway")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeAway(lua_toboolean(L, -1));
            } else if (key == QLatin1String("fadeout")) {
                if (!checkIntArg(L, func, -1, "value for fadeout")) {
                    errorPushed = true;
                    break;
                }

                const int value = static_cast<int>(lua_tointeger(L, -1));

                if (value < 0) {
                    lua_pushfstring(L, "stopMusic: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeOut(value);
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeMusic);

            host.mpMedia->stopMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopMusic
int TLuaInterpreter::stopMusic(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return stopMusicAsTableArgument(L, __func__);
        }

        return stopMusicAsOrderedArguments(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopSoundsAsOrderedArguments(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);
    const int numArgs = lua_gettop(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};
        QString stringValue;
        int intValue = 0;

        // values as ordered args: name[,key][,tag][,priority][,fadeaway][,fadeout])
        for (int i = 1; i <= numArgs && !errorPushed; i++) {
            if (lua_isnil(L, i)) {
                continue;
            }

            switch (i) {
            case 1:
                if (!checkStringArg(L, func, i, "name")) {
                    errorPushed = true;
                    break;
                }

                stringValue = lua_tostring(L, i);

                if (QDir::homePath().contains('\\')) {
                    stringValue.replace('/', R"(\)");
                } else {
                    stringValue.replace('\\', "/");
                }

                mediaData.setMediaFileName(stringValue);
                break;
            case 2:
                if (!checkStringArg(L, func, i, "key")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaKey(QString{lua_tostring(L, i)});
                break;
            case 3:
                if (!checkStringArg(L, func, i, "tag")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaTag(QString{lua_tostring(L, i)});
                break;
            case 4:
                if (!checkIntArg(L, func, i, "priority")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue > TMediaData::MediaPriorityMax) {
                    intValue = TMediaData::MediaPriorityMax;
                } else if (intValue < TMediaData::MediaPriorityMin) {
                    intValue = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(intValue);
                break;
            case 5:
                if (!checkBoolArg(L, func, i, "fadeaway")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeAway(lua_toboolean(L, i));
                break;
            case 6:
                if (!checkIntArg(L, func, i, "fadeout")) {
                    errorPushed = true;
                    break;
                }

                intValue = static_cast<int>(lua_tointeger(L, i));

                if (intValue < 0) {
                    lua_pushfstring(L, "stopSounds: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeOut(intValue);
                break;
            }
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeSound);

            host.mpMedia->stopMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopSoundsAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            } else if (key == QLatin1String("priority")) {
                if (!checkIntArg(L, func, -1, "value for priority")) {
                    errorPushed = true;
                    break;
                }

                int value = static_cast<int>(lua_tointeger(L, -1));

                if (value > TMediaData::MediaPriorityMax) {
                    value = TMediaData::MediaPriorityMax;
                } else if (value < TMediaData::MediaPriorityMin) {
                    value = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(value);
            } else if (key == QLatin1String("fadeaway")) {
                if (!checkBoolArg(L, func, -1, "value for fadeaway")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeAway(lua_toboolean(L, -1));
            } else if (key == QLatin1String("fadeout")) {
                if (!checkIntArg(L, func, -1, "value for fadeout")) {
                    errorPushed = true;
                    break;
                }

                const int value = static_cast<int>(lua_tointeger(L, -1));

                if (value < 0) {
                    lua_pushfstring(L, "stopSounds: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeOut(value);
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeSound);

            host.mpMedia->stopMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopSounds
int TLuaInterpreter::stopSounds(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return stopSoundsAsTableArgument(L, __func__);
        }

        return stopSoundsAsOrderedArguments(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopVideosAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            } else if (key == QLatin1String("fadeaway")) {
                if (!checkBoolArg(L, func, -1, "value for fadeaway")) {
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeAway(lua_toboolean(L, -1));
            } else if (key == QLatin1String("fadeout")) {
                if (!checkIntArg(L, func, -1, "value for fadeout")) {
                    errorPushed = true;
                    break;
                }

                const int value = static_cast<int>(lua_tointeger(L, -1));

                if (value < 0) {
                    lua_pushfstring(L, "stopVideos: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                    errorPushed = true;
                    break;
                }

                mediaData.setMediaFadeOut(value);
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeVideo);

            host.mpMedia->stopMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopVideos
int TLuaInterpreter::stopVideos(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return stopVideosAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeVideo);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::pauseSoundsAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeSound);

            host.mpMedia->pauseMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#pauseSounds
int TLuaInterpreter::pauseSounds(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return pauseSoundsAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->pauseMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::pauseMusicAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeMusic);

            host.mpMedia->pauseMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#pauseMusic
int TLuaInterpreter::pauseMusic(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return pauseMusicAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->pauseMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::pauseVideosAsTableArgument(lua_State* L, const char* func)
{
    const Host& host = getHostFromLua(L);

    bool errorPushed = false;
    {
        TMediaData mediaData{};

        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (!checkStringArg(L, func, -2, "table keys")) {
                errorPushed = true;
                break;
            }

            // read the key from a copy: lua_tostring() on the slot itself converts a
            // numeric key in place, which makes the next lua_next() fail
            lua_pushvalue(L, -2);
            const QString key = QString{lua_tostring(L, -1)}.toLower();
            lua_pop(L, 1);

            if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
                if (!checkStringArg(L, func, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag")) {
                    errorPushed = true;
                    break;
                }

                QString value{lua_tostring(L, -1)};

                if (key == QLatin1String("name") && !value.isEmpty()) {
                    if (QDir::homePath().contains('\\')) {
                        value.replace('/', R"(\)");
                    } else {
                        value.replace('\\', "/");
                    }

                    mediaData.setMediaFileName(value);
                } else if (key == QLatin1String("key") && !value.isEmpty()) {
                    mediaData.setMediaKey(value);
                } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                    mediaData.setMediaTag(value);
                }
            }

            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }

        if (!errorPushed) {
            mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
            mediaData.setMediaType(TMediaData::MediaTypeVideo);

            host.mpMedia->pauseMedia(mediaData);
        }
    }

    if (errorPushed) {
        return lua_error(L);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#pauseVideos
int TLuaInterpreter::pauseVideos(lua_State* L)
{
    const Host& host = getHostFromLua(L);

    if (lua_gettop(L)) {
        if (!lua_istable(L, 1)) {
            lua_pushfstring(L, "%s: needs to be a table", __func__);
            return lua_error(L);
        }

        return pauseVideosAsTableArgument(L, __func__);
    }

    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeVideo);

    host.mpMedia->pauseMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#purgeMediaCache
int TLuaInterpreter::purgeMediaCache(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mTelnet.purgeMediaCache());
    return 1;
}
