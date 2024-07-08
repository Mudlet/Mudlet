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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#receiveMSP
int TLuaInterpreter::receiveMSP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    std::string msg;

    if (!host.mTelnet.isMSPEnabled()) {
        return warnArgumentValue(L, __func__, "MSP is not currently enabled");
    }

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "receiveMSP: bad argument #1 type (message as string expected, got %1!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));
    host.mTelnet.setMSPVariables(QByteArray(msg.c_str(), msg.length()));

    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::loadMediaFileAsOrderedArguments(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    const int numArgs = lua_gettop(L);
    QString stringValue;

    // name[,url])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "url");
            mediaData.setMediaUrl(stringValue);
            break;
        }
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        return warnArgumentValue(L, __func__, QLatin1String("missing argument 1 (file to play)"));
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::loadMediaFileAsTableArgument(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("url")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : "value for url");

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

    if (mediaData.getMediaFileName().isEmpty()) {
        lua_pushstring(L, R"(loadMusicFile: missing name (add name = "file to play"))");
        return lua_error(L);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

    host.mpMedia->playMedia(mediaData);
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
        return loadMediaFileAsTableArgument(L);
    }

    return loadMediaFileAsOrderedArguments(L);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadSoundFile
int TLuaInterpreter::loadSoundFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return loadMediaFileAsTableArgument(L);
    }

    return loadMediaFileAsOrderedArguments(L);
}

// Private
int TLuaInterpreter::playMusicFileAsOrderedArguments(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    const int numArgs = lua_gettop(L);
    QString stringValue;
    int intValue = 0;
    bool boolValue = 0;

    // name[,volume][,fadein][,fadeout][,start][,loops][,key][,tag][,continue][,url][,finish]
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            intValue = getVerifiedInt(L, __func__, i, "volume");

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
            intValue = getVerifiedInt(L, __func__, i, "fadein");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeIn(intValue);
            break;
        case 4:
            intValue = getVerifiedInt(L, __func__, i, "fadeout");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(intValue);
            break;
        case 5:
            intValue = getVerifiedInt(L, __func__, i, "start");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", intValue);
                return lua_error(L);
            }

            mediaData.setMediaStart(intValue);
            break;
        case 6:
            intValue = getVerifiedInt(L, __func__, i, "loops");

            if (intValue < TMediaData::MediaLoopsRepeat || intValue == 0) {
                intValue = TMediaData::MediaLoopsDefault;
            }

            mediaData.setMediaLoops(intValue);
            break;
        case 7:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 8:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 9:
            boolValue = getVerifiedBool(L, __func__, i, "continue");
            mediaData.setMediaContinue(boolValue);
            break;
        case 10:
            stringValue = getVerifiedString(L, __func__, i, "url");
            mediaData.setMediaUrl(stringValue);
            break;
        case 11:
            intValue = getVerifiedInt(L, __func__, i, "finish");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFinish(intValue);
            break;
        }
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        return warnArgumentValue(L, __func__, QLatin1String("missing argument 1 (file to play)"));
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);
    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::playMusicFileAsTableArgument(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("url") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L,
                                              __func__,
                                              -1,
                                              key == QLatin1String("name")  ? "value for name"
                                              : key == QLatin1String("key") ? "value for key"
                                              : key == QLatin1String("tag") ? "value for tag"
                                                                            : "value for url");

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
            }
        } else if (key == QLatin1String("volume") || key == QLatin1String("fadein") || key == QLatin1String("fadeout") || key == QLatin1String("start") || key == QLatin1String("finish") || key == QLatin1String("loops")) {
            int value = getVerifiedInt(L,
                                       __func__,
                                       -1,
                                       key == QLatin1String("volume")    ? "value for volume"
                                       : key == QLatin1String("fadein")  ? "value for fadein"
                                       : key == QLatin1String("fadeout") ? "value for fadeout"
                                       : key == QLatin1String("start")   ? "value for start"
                                       : key == QLatin1String("finish")  ? "value for finish"
                                                                         : "value for loops");

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
                    return lua_error(L);
                }

                mediaData.setMediaFadeIn(value);
            } else if (key == QLatin1String("fadeout")) {
                if (value < 0) {
                    lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                    return lua_error(L);
                }

                mediaData.setMediaFadeOut(value);
            } else if (key == QLatin1String("start")) {
                if (value < 0) {
                    lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", value);
                    return lua_error(L);
                }

                mediaData.setMediaStart(value);
            } else if (key == QLatin1String("finish")) {
                if (value < 0) {
                    lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", value);
                    return lua_error(L);
                }

                mediaData.setMediaFinish(value);
            } else if (key == QLatin1String("loops")) {
                if (value < TMediaData::MediaLoopsRepeat || value == 0) {
                    value = TMediaData::MediaLoopsDefault;
                }

                mediaData.setMediaLoops(value);
            }
        } else if (key == QLatin1String("continue")) {
            const bool value = getVerifiedBool(L, __func__, -1, "value for continue must be boolean");
            mediaData.setMediaContinue(value);
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        lua_pushstring(L, R"(playMusicFile: missing name (add name = "file to play"))");
        return lua_error(L);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);
    host.mpMedia->playMedia(mediaData);
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
        return playMusicFileAsTableArgument(L);
    }

    return playMusicFileAsOrderedArguments(L);
}

// Private
int TLuaInterpreter::playSoundFileAsOrderedArguments(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    const int numArgs = lua_gettop(L);
    QString stringValue;
    int intValue = 0;

    // name[,volume][,fadein][,fadeout][,start][,loops][,key][,tag][,priority][,url][,finish]
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            intValue = getVerifiedInt(L, __func__, i, "volume");

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
            intValue = getVerifiedInt(L, __func__, i, "fadein");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %s)", "fadein", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeIn(intValue);
            break;
        case 4:
            intValue = getVerifiedInt(L, __func__, i, "fadeout");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %s)", "fadeout", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(intValue);
            break;
        case 5:
            intValue = getVerifiedInt(L, __func__, i, "start");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %s)", "start", intValue);
                return lua_error(L);
            }

            mediaData.setMediaStart(intValue);
            break;
        case 6:
            intValue = getVerifiedInt(L, __func__, i, "loops");

            if (intValue < TMediaData::MediaLoopsRepeat || intValue == 0) {
                intValue = TMediaData::MediaLoopsDefault;
            }

            mediaData.setMediaLoops(intValue);
            break;
        case 7:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 8:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 9:
            intValue = getVerifiedInt(L, __func__, i, "priority");

            if (intValue > TMediaData::MediaPriorityMax) {
                intValue = TMediaData::MediaPriorityMax;
            } else if (intValue < TMediaData::MediaPriorityMin) {
                intValue = TMediaData::MediaPriorityMin;
            }

            mediaData.setMediaPriority(intValue);
            break;
        case 10:
            stringValue = getVerifiedString(L, __func__, i, "url");
            mediaData.setMediaUrl(stringValue);
            break;
        case 11:
            intValue = getVerifiedInt(L, __func__, i, "finish");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %s)", "finish", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFinish(intValue);
            break;
        }
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        return warnArgumentValue(L, __func__, QLatin1String("missing argument 1 (file to play)"));
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::playSoundFileAsTableArgument(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("url") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L,
                                              __func__,
                                              -1,
                                              key == QLatin1String("name")  ? "value for name"
                                              : key == QLatin1String("key") ? "value for key"
                                              : key == QLatin1String("tag") ? "value for tag"
                                                                            : "value for url");

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
            }
        } else if (key == QLatin1String("volume") || key == QLatin1String("fadein") || key == QLatin1String("fadeout") || key == QLatin1String("start") || key == QLatin1String("finish") || key == QLatin1String("loops")
                   || key == QLatin1String("priority")) {
            int value = getVerifiedInt(L,
                                       __func__,
                                       -1,
                                       key == QLatin1String("volume")    ? "value for volume"
                                       : key == QLatin1String("fadein")  ? "value for fadein"
                                       : key == QLatin1String("fadeout") ? "value for fadeout"
                                       : key == QLatin1String("start")   ? "value for start"
                                       : key == QLatin1String("finish")  ? "value for finish"
                                       : key == QLatin1String("loops")   ? "value for loops"
                                                                         : "value for priority");

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
                    return lua_error(L);
                }

                mediaData.setMediaFadeIn(value);
            } else if (key == QLatin1String("fadeout")) {
                if (value < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                    return lua_error(L);
                }

                mediaData.setMediaFadeOut(value);
            } else if (key == QLatin1String("start")) {
                if (value < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", value);
                    return lua_error(L);
                }

                mediaData.setMediaStart(value);
            } else if (key == QLatin1String("finish")) {
                if (value < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "finish", value);
                    return lua_error(L);
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

    if (mediaData.getMediaFileName().isEmpty()) {
        lua_pushstring(L, R"(playSoundFile: missing name (add name = "file to play"))");
        return lua_error(L);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->playMedia(mediaData);
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
        return playSoundFileAsTableArgument(L);
    }

    return playSoundFileAsOrderedArguments(L);
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

        if (!matchedMediaData.getMediaFileName().isEmpty()) {
            lua_pushstring(L, "name");
            lua_pushstring(L, matchedMediaData.getMediaFileName().toUtf8().constData());
            lua_settable(L, -3);
        }

        if (matchedMediaData.getMediaVolume() != TMediaData::MediaVolumePreload) {
            lua_pushstring(L, "volume");
            lua_pushinteger(L, matchedMediaData.getMediaVolume());
            lua_settable(L, -3);
        }

        if (matchedMediaData.getMediaPriority() != TMediaData::MediaPriorityNotSet) {
            lua_pushstring(L, "priority");
            lua_pushinteger(L, matchedMediaData.getMediaPriority());
            lua_settable(L, -3);
        }

        if (!matchedMediaData.getMediaTag().isEmpty()) {
            lua_pushstring(L, "tag");
            lua_pushstring(L, matchedMediaData.getMediaTag().toUtf8().constData());
            lua_settable(L, -3);
        }

        if (!matchedMediaData.getMediaKey().isEmpty()) {
            lua_pushstring(L, "key");
            lua_pushstring(L, matchedMediaData.getMediaKey().toUtf8().constData());
            lua_settable(L, -3);
        }

        lua_settable(L, -3);
    }
}

// Private
int TLuaInterpreter::getPlayingMusicAsOrderedArguments(lua_State* L)
{
    TMediaData mediaData{};
    const int numArgs = lua_gettop(L);
    QString stringValue;

    // values as ordered args: name[,key][,tag]
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 3:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        }
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::getPlayingMusicAsTableArgument(lua_State* L)
{
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag");

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

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    processPlayingMediaTable(L, mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayingMusic
int TLuaInterpreter::getPlayingMusic(lua_State* L)
{
    TMediaData mediaData{};

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return getPlayingMusicAsTableArgument(L);
        }

        return getPlayingMusicAsOrderedArguments(L);
    }

    // no args
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::getPlayingSoundsAsOrderedArguments(lua_State* L)
{
    TMediaData mediaData{};
    const int numArgs = lua_gettop(L);
    QString stringValue;
    int intValue = 0;

    // values as ordered args: name[,key][,tag][,priority])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 3:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 4:
            intValue = getVerifiedInt(L, __func__, i, "priority");

            if (intValue > TMediaData::MediaPriorityMax) {
                intValue = TMediaData::MediaPriorityMax;
            } else if (intValue < TMediaData::MediaPriorityMin) {
                intValue = TMediaData::MediaPriorityMin;
            }

            mediaData.setMediaPriority(intValue);
            break;
        }
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::getPlayingSoundsAsTableArgument(lua_State* L)
{
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag");

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
            int value = getVerifiedInt(L, __func__, -1, "value for priority must be integer");

            if (key == QLatin1String("priority")) {
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

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayingSounds
int TLuaInterpreter::getPlayingSounds(lua_State* L)
{
    TMediaData mediaData{};

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return getPlayingSoundsAsTableArgument(L);
        }

        return getPlayingSoundsAsOrderedArguments(L);
    }

    // no args
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    processPlayingMediaTable(L, mediaData);
    return 1;
}

// Private
int TLuaInterpreter::stopMusicAsOrderedArguments(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    const int numArgs = lua_gettop(L);
    QString stringValue;
    bool boolValue;
    int intValue;

    // values as ordered args: name[,key][,tag][,fadeaway][,fadeout]
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 3:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 4:
            boolValue = getVerifiedBool(L, __func__, i, "fadeaway");
            mediaData.setMediaFadeAway(boolValue);
            break;
        case 5:
            intValue = getVerifiedInt(L, __func__, i, "fadeout");

            if (intValue < 0) {
                lua_pushfstring(L, "stopMusic: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(intValue);
            break;
        }
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopMusicAsTableArgument(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag");

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
            const bool value = getVerifiedBool(L, __func__, -1, "value for fadeaway must be boolean");
            mediaData.setMediaFadeAway(value);
        } else if (key == QLatin1String("fadeout")) {
            int value = getVerifiedInt(L, __func__, -1, "value for fadeout");

            if (value < 0) {
                lua_pushfstring(L, "stopMusic: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(value);
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopMusic
int TLuaInterpreter::stopMusic(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return stopMusicAsTableArgument(L);
        }

        return stopMusicAsOrderedArguments(L);
    }

    // no args
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopSoundsAsOrderedArguments(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    const int numArgs = lua_gettop(L);
    QString stringValue;
    bool boolValue;
    int intValue = 0;

    // values as ordered args: name[,key][,tag][,priority][,fadeaway][,fadeout])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 3:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 4:
            intValue = getVerifiedInt(L, __func__, i, "priority");

            if (intValue > TMediaData::MediaPriorityMax) {
                intValue = TMediaData::MediaPriorityMax;
            } else if (intValue < TMediaData::MediaPriorityMin) {
                intValue = TMediaData::MediaPriorityMin;
            }

            mediaData.setMediaPriority(intValue);
            break;
        case 5:
            boolValue = getVerifiedBool(L, __func__, i, "fadeaway");
            mediaData.setMediaFadeAway(boolValue);
            break;
        case 6:
            intValue = getVerifiedInt(L, __func__, i, "fadeout");

            if (intValue < 0) {
                lua_pushfstring(L, "stopSounds: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(intValue);
            break;
        }
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopSoundsAsTableArgument(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag");

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
            int value = getVerifiedInt(L, __func__, -1, "value for priority must be integer");

            if (key == QLatin1String("priority")) {
                if (value > TMediaData::MediaPriorityMax) {
                    value = TMediaData::MediaPriorityMax;
                } else if (value < TMediaData::MediaPriorityMin) {
                    value = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(value);
            }
        } else if (key == QLatin1String("fadeaway")) {
            const bool value = getVerifiedBool(L, __func__, -1, "value for fadeaway must be boolean");
            mediaData.setMediaFadeAway(value);
        } else if (key == QLatin1String("fadeout")) {
            int value = getVerifiedInt(L, __func__, -1, "value for fadeout");

            if (value < 0) {
                lua_pushfstring(L, "stopSounds: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(value);
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopSounds
int TLuaInterpreter::stopSounds(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return stopSoundsAsTableArgument(L);
        }

        return stopSoundsAsOrderedArguments(L);
    }

    // no args
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#purgeMediaCache
int TLuaInterpreter::purgeMediaCache(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mTelnet.purgeMediaCache();
    lua_pushboolean(L, true);
    return 1;
}
