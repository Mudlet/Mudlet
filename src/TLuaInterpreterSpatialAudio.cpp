/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

// Spatial Audio Lua API functions - Table-based API
// This file provides Lua bindings for the TSpatialAudio system

#include "TLuaInterpreter.h"
#include "TSpatialAudio.h"
#include "Host.h"
#include "mudlet.h"
#include <QDir>

// Helper to get spatial audio system (now automatically created in Host constructor)
static TSpatialAudio* getSpatialAudio(Host& host)
{
    if (!host.mpSpatialAudio->isInitialized()) {
        host.mpSpatialAudio->initialize();
    }

    return host.mpSpatialAudio.get();
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playSpatialSound
int TLuaInterpreter::playSpatialSound(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const char* func = "playSpatialSound";
    
    if (!lua_istable(L, 1)) {
        lua_pushfstring(L, "%s: bad argument #1 type (table expected, got %s)", func, luaL_typename(L, 1));
        return lua_error(L);
    }
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    
    // Required: key (unique identifier)
    QString key;
    lua_getfield(L, 1, "key");

    if (lua_isstring(L, -1)) {
        key = QString::fromUtf8(lua_tostring(L, -1));
    }

    lua_pop(L, 1);
    
    if (key.isEmpty()) {
        lua_pushstring(L, "playSpatialSound: missing required field 'key'");
        return lua_error(L);
    }
    
    // Optional: name (file path)
    QString fileName;
    lua_getfield(L, 1, "name");

    if (lua_isstring(L, -1)) {
        fileName = QString::fromUtf8(lua_tostring(L, -1));
        // Normalize path separators
        if (QDir::homePath().contains('\\')) {
            fileName.replace('/', R"(\)");
        } else {
            fileName.replace('\\', "/");
        }
    }

    lua_pop(L, 1);
    
    // Optional: url (download path)
    QString url;
    lua_getfield(L, 1, "url");

    if (lua_isstring(L, -1)) {
        url = QString::fromUtf8(lua_tostring(L, -1));
    }

    lua_pop(L, 1);
    
    // 'name' is required (either for local file or as filename to download)
    if (fileName.isEmpty()) {
        lua_pushstring(L, "playSpatialSound: 'name' field is required");
        return lua_error(L);
    }
    
    // Create or get existing source
    TSpatialAudioSource* source = spatial->getSource(key);
    if (!source) {
        source = spatial->createSource(key);

        if (!source) {
            return warnArgumentValue(L, func, qsl("failed to create source '%1'").arg(key));
        }
    }
    
    // Resolve file path and handle downloading if needed
    QString resolvedPath = spatial->resolveFilePath(key, fileName, url);

    if (!resolvedPath.isEmpty()) {
        // File is available - load it
        source->setSource(resolvedPath);
    } else if (!url.isEmpty()) {
        // File is being downloaded - source will be set when download completes
        qDebug() << "playSpatialSound: File download initiated for key:" << key;
    } else {
        return warnArgumentValue(L, func, qsl("file not found: '%1'").arg(fileName));
    }
    
    // Optional: position {azimuth, elevation, distance}
    lua_getfield(L, 1, "position");

    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1);  // azimuth
        lua_rawgeti(L, -2, 2);  // elevation
        lua_rawgeti(L, -3, 3);  // distance
        
        if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
            float azimuth = lua_tonumber(L, -3);
            float elevation = lua_tonumber(L, -2);
            float distance = lua_tonumber(L, -1);
            source->setPosition(azimuth, elevation, distance);
        }
        
        lua_pop(L, 3);  // pop azimuth, elevation, distance
    }

    lua_pop(L, 1);  // pop position table
    
    // Optional: volume (0-100)
    lua_getfield(L, 1, "volume");

    if (lua_isnumber(L, -1)) {
        float volume = lua_tonumber(L, -1);
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        source->setVolume(volume / 100.0f);
    }

    lua_pop(L, 1);
    
    // Optional: occlusion
    lua_getfield(L, 1, "occlusion");

    if (lua_isnumber(L, -1)) {
        float occlusion = lua_tonumber(L, -1);
        source->setOcclusion(occlusion);
    }

    lua_pop(L, 1);
    
    // Optional: loops (-1 for infinite)
    lua_getfield(L, 1, "loops");

    if (lua_isnumber(L, -1)) {
        int loops = lua_tointeger(L, -1);
        source->setLoops(loops);
    }

    lua_pop(L, 1);
    
    // Optional: room acoustics {width, height, depth, reverb, reflection, material}
    lua_getfield(L, 1, "room");
    if (lua_istable(L, -1)) {
        TSpatialAudioRoom* room = spatial->getRoom();

        if (!room) {
            room = spatial->createRoom();
        }
        
        if (room) {
            // Get room dimensions {width, height, depth}
            lua_getfield(L, -1, "dimensions");

            if (lua_istable(L, -1)) {
                lua_rawgeti(L, -1, 1);  // width
                lua_rawgeti(L, -2, 2);  // height
                lua_rawgeti(L, -3, 3);  // depth
                
                if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                    float width = lua_tonumber(L, -3);
                    float height = lua_tonumber(L, -2);
                    float depth = lua_tonumber(L, -1);
                    room->setDimensions(width, height, depth);
                }
                
                lua_pop(L, 3);  // pop width, height, depth
            }

            lua_pop(L, 1);  // pop dimensions table
            
            // Get reverb gain
            lua_getfield(L, -1, "reverb");

            if (lua_isnumber(L, -1)) {
                float reverb = lua_tonumber(L, -1);
                room->setReverbGain(reverb);
            }

            lua_pop(L, 1);
            
            // Get reflection gain
            lua_getfield(L, -1, "reflection");

            if (lua_isnumber(L, -1)) {
                float reflection = lua_tonumber(L, -1);
                room->setReflectionGain(reflection);
            }

            lua_pop(L, 1);
            
            // Get wall material (string name)
            // Note: setWallMaterial requires specifying which wall
            // For simplicity, apply to all walls
            lua_getfield(L, -1, "material");

            if (lua_isstring(L, -1)) {
                QString material = QString::fromUtf8(lua_tostring(L, -1));
                QAudioRoom::Material mat = QAudioRoom::Material::BrickBare;
                
                // Map material name to enum
                if (material == "brick") {
                    mat = QAudioRoom::Material::BrickBare;
                } else if (material == "concrete") {
                    mat = QAudioRoom::Material::ConcreteBlockCoarse;
                } else if (material == "wood") {
                    mat = QAudioRoom::Material::WoodPanel;
                } else if (material == "carpet") {
                    mat = QAudioRoom::Material::FiberGlassInsulation;
                }
                
                // Apply material to all walls
                room->setWallMaterial(QAudioRoom::Wall::LeftWall, mat);
                room->setWallMaterial(QAudioRoom::Wall::RightWall, mat);
                room->setWallMaterial(QAudioRoom::Wall::FrontWall, mat);
                room->setWallMaterial(QAudioRoom::Wall::BackWall, mat);
                room->setWallMaterial(QAudioRoom::Wall::Floor, mat);
                room->setWallMaterial(QAudioRoom::Wall::Ceiling, mat);
            }

            lua_pop(L, 1);
        }
    }

    lua_pop(L, 1);  // pop room table
    
    // Check mute state before playing
    if (mudlet* pMudlet = mudlet::self()) {
        if (pMudlet->muteAPI()) {
#ifdef DEBUG_SPATIAL_AUDIO
            qDebug() << "playSpatialSound - Not playing due to API mute state:" << key;
#endif
            // Remove the source so it doesn't play automatically
            spatial->removeSource(key);
            lua_pushboolean(L, true);
            return 1;
        }
    }
    
    // Play the sound
    source->play();
    
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopSpatialSound
int TLuaInterpreter::stopSpatialSound(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString key = getVerifiedString(L, __func__, 1, "source key");
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    TSpatialAudioSource* source = spatial->getSource(key);
    
    if (!source) {
        return warnArgumentValue(L, __func__, qsl("source '%1' not found").arg(key));
    }
    
    source->stop();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#pauseSpatialSound
int TLuaInterpreter::pauseSpatialSound(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString key = getVerifiedString(L, __func__, 1, "source key");
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    TSpatialAudioSource* source = spatial->getSource(key);
    
    if (!source) {
        return warnArgumentValue(L, __func__, qsl("source '%1' not found").arg(key));
    }
    
    source->pause();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#updateSpatialSound
int TLuaInterpreter::updateSpatialSound(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const char* func = "updateSpatialSound";
    
    const QString key = getVerifiedString(L, func, 1, "source key");
    
    if (!lua_istable(L, 2)) {
        lua_pushfstring(L, "%s: bad argument #2 type (table expected, got %s)", func, luaL_typename(L, 2));
        return lua_error(L);
    }
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    TSpatialAudioSource* source = spatial->getSource(key);
    
    if (!source) {
        return warnArgumentValue(L, func, qsl("source '%1' not found").arg(key));
    }
    
    // Optional: position {azimuth, elevation, distance}
    lua_getfield(L, 2, "position");
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1);  // azimuth
        lua_rawgeti(L, -2, 2);  // elevation
        lua_rawgeti(L, -3, 3);  // distance
        
        if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
            float azimuth = lua_tonumber(L, -3);
            float elevation = lua_tonumber(L, -2);
            float distance = lua_tonumber(L, -1);
            source->setPosition(azimuth, elevation, distance);
        }
        
        lua_pop(L, 3);  // pop azimuth, elevation, distance
    }

    lua_pop(L, 1);  // pop position table
    
    // Optional: volume (0-100)
    lua_getfield(L, 2, "volume");

    if (lua_isnumber(L, -1)) {
        float volume = lua_tonumber(L, -1);
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        source->setVolume(volume / 100.0f);
    }

    lua_pop(L, 1);
    
    // Optional: occlusion
    lua_getfield(L, 2, "occlusion");

    if (lua_isnumber(L, -1)) {
        float occlusion = lua_tonumber(L, -1);
        source->setOcclusion(occlusion);
    }

    lua_pop(L, 1);
    
    // If source has loops configured (meaning it should be playing) but isn't currently playing,
    // check if we're no longer muted and start it
    if (source->loops() != 0 && !source->isPlaying() && !source->isPaused()) {
        if (mudlet* pMudlet = mudlet::self()) {
            if (!pMudlet->muteAPI()) {
#ifdef DEBUG_SPATIAL_AUDIO
                qDebug() << "updateSpatialSound - Starting playback for previously muted source:" << key;
#endif
                source->play();
            }
        }
    }
    
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSpatialSounds
int TLuaInterpreter::getSpatialSounds(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TSpatialAudio* spatial = getSpatialAudio(host);
    
    QStringList sources = spatial->listSources();
    
    lua_createtable(L, sources.size(), 0);

    for (int i = 0; i < sources.size(); ++i) {
        lua_pushstring(L, sources[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1);
    }
    
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeSpatialSound
int TLuaInterpreter::removeSpatialSound(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString key = getVerifiedString(L, __func__, 1, "source key");
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    
    if (!spatial->removeSource(key)) {
        return warnArgumentValue(L, __func__, qsl("source '%1' not found").arg(key));
    }
    
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setSpatialListener
int TLuaInterpreter::setSpatialListener(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const char* func = "setSpatialListener";
    
    if (!lua_istable(L, 1)) {
        lua_pushfstring(L, "%s: bad argument #1 type (table expected, got %s)", func, luaL_typename(L, 1));
        return lua_error(L);
    }
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    
    // Optional: position {x, y, z}
    lua_getfield(L, 1, "position");

    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1);  // x
        lua_rawgeti(L, -2, 2);  // y
        lua_rawgeti(L, -3, 3);  // z
        
        if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
            float x = lua_tonumber(L, -3);
            float y = lua_tonumber(L, -2);
            float z = lua_tonumber(L, -1);
            spatial->setListenerPosition(x, y, z);
        }
        
        lua_pop(L, 3);  // pop x, y, z
    }

    lua_pop(L, 1);  // pop position table
    
    // Optional: rotation {yaw, pitch, roll}
    lua_getfield(L, 1, "rotation");

    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1);  // yaw
        lua_rawgeti(L, -2, 2);  // pitch
        lua_rawgeti(L, -3, 3);  // roll
        
        if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
            float yaw = lua_tonumber(L, -3);
            float pitch = lua_tonumber(L, -2);
            float roll = lua_tonumber(L, -1);
            spatial->setListenerRotation(yaw, pitch, roll);
        }
        
        lua_pop(L, 3);  // pop yaw, pitch, roll
    }

    lua_pop(L, 1);  // pop rotation table
    
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setSpatialMasterVolume
int TLuaInterpreter::setSpatialMasterVolume(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int volume = getVerifiedInt(L, __func__, 1, "volume");
    
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    spatial->setMasterVolume(volume / 100.0f);
    
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playSpatialTestTone
int TLuaInterpreter::playSpatialTestTone(lua_State* L)
{
    Host& host = getHostFromLua(L);
    
    if (!lua_istable(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: options must be a table");
        return 2;
    }
    
    TSpatialAudio* spatial = getSpatialAudio(host);
    
    // Extract key
    lua_getfield(L, 1, "key");
    if (!lua_isstring(L, -1)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: missing required field 'key'");
        return 2;
    }

    const QString key = lua_tostring(L, -1);
    lua_pop(L, 1);
    
    // Extract type
    lua_getfield(L, 1, "type");
    if (!lua_isstring(L, -1)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: missing required field 'type'");
        return 2;
    }

    const QString typeStr = QString(lua_tostring(L, -1)).toLower();
    lua_pop(L, 1);
    
    TSpatialAudio::ToneType toneType;

    if (typeStr == "white") {
        toneType = TSpatialAudio::WhiteNoise;
    } else if (typeStr == "pink") {
        toneType = TSpatialAudio::PinkNoise;
    } else if (typeStr == "sine") {
        toneType = TSpatialAudio::SineWave;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: type must be 'white', 'pink', or 'sine'");
        return 2;
    }
    
    // Extract duration
    lua_getfield(L, 1, "duration");

    if (!lua_isnumber(L, -1)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: missing required field 'duration'");
        return 2;
    }

    const float duration = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    // Extract frequency (optional, default 440 Hz)
    lua_getfield(L, 1, "frequency");
    float frequency = 440.0f;

    if (lua_isnumber(L, -1)) {
        frequency = lua_tonumber(L, -1);
    }

    lua_pop(L, 1);
    
    // Extract position
    lua_getfield(L, 1, "azimuth");

    if (!lua_isnumber(L, -1)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: missing required field 'azimuth'");
        return 2;
    }

    const float azimuth = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "elevation");

    if (!lua_isnumber(L, -1)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: missing required field 'elevation'");
        return 2;
    }

    const float elevation = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "distance");

    if (!lua_isnumber(L, -1)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: missing required field 'distance'");
        return 2;
    }

    const float distance = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    // Create the test tone source
    if (!spatial->createTestToneSource(key, toneType, frequency, duration)) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: failed to create test tone");
        return 2;
    }
    
    // Get the source and configure it
    TSpatialAudioSource* source = spatial->getSource(key, TSpatialAudio::ProtocolAPI);

    if (!source) {
        lua_pushnil(L);
        lua_pushstring(L, "playSpatialTestTone: failed to get source");
        return 2;
    }
    
    // Set position
    source->setPosition(azimuth, elevation, distance);
    
    // Set optional parameters
    lua_getfield(L, 1, "volume");

    if (lua_isnumber(L, -1)) {
        int volume = lua_tonumber(L, -1);
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        source->setVolume(volume / 100.0f);
    } else {
        source->setVolume(0.5f);  // Default 50%
    }

    lua_pop(L, 1);
    
    lua_getfield(L, 1, "loops");

    if (lua_isnumber(L, -1)) {
        source->setLoops(lua_tointeger(L, -1));
    }

    lua_pop(L, 1);
    
    // Play the sound
    source->play();
    
    lua_pushboolean(L, true);
    return 1;
}
