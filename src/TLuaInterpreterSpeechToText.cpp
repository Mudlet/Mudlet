/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

// Speech-to-text Lua API functions for TLuaInterpreter
// These functions provide a minimal bridge between the Vosk-based
// speech recognition engine and Lua scripts.

#include "TLuaInterpreter.h"

#include "Host.h"
#include "mudlet.h"
#include "SpeechRecognizer.h"
#include "SpeechRecognizerFactory.h"
#include "VoskRecognizer.h"

#include <QDir>

// stt.init([modelPath])
// Initialize speech recognition with a language model.
// modelPath is optional - falls back to SpeechRecognizerFactory::defaultModelPath().
// Returns true on success, or nil + error message on failure.
int TLuaInterpreter::sttInit(lua_State* L)
{
    const QString funcName = qsl("stt.init");
    QString modelPath;
    if (lua_gettop(L) >= 1 && !lua_isnoneornil(L, 1)) {
        modelPath = getVerifiedString(L, funcName.toUtf8().constData(), 1, "model path");
    } else {
        modelPath = SpeechRecognizerFactory::defaultModelPath();
        if (modelPath.isEmpty()) {
            return warnArgumentValue(L, funcName.toUtf8().constData(), "no model path provided and no default model is installed - please install a language model via the speech-to-text setup");
        }
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "mudlet instance not available");
    }

    // Check if model path exists
    QDir modelDir(modelPath);
    if (!modelDir.exists()) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), qsl("model path does not exist: %1").arg(modelPath).toUtf8().constData());
    }

    // Initialize through mudlet singleton
    pMudlet->initSpeechRecognition();

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "failed to create speech recognizer");
    }

    if (!pRecognizer->initialize(modelPath)) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), qsl("failed to initialize model from: %1").arg(modelPath).toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

// stt.start()
// Start listening for speech input.
// Returns true on success, or nil + error message on failure.
int TLuaInterpreter::sttStart(lua_State* L)
{
    const QString funcName = qsl("stt.start");

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "mudlet instance not available");
    }

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "speech recognizer not initialized - call stt.init() first");
    }

    if (!pRecognizer->isInitialized()) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "speech recognizer not initialized with a model - call stt.init() first");
    }

    if (pRecognizer->isListening()) {
        // Already listening, nothing to do
        lua_pushboolean(L, true);
        return 1;
    }

    pRecognizer->startListening();
    pMudlet->setSpeechRecognitionActive(true);

    lua_pushboolean(L, true);
    return 1;
}

// stt.stop()
// Stop listening and process any remaining audio.
// Returns true on success, or nil + error message on failure.
int TLuaInterpreter::sttStop(lua_State* L)
{
    const QString funcName = qsl("stt.stop");

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "mudlet instance not available");
    }

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        // Not initialized is fine - just return true
        lua_pushboolean(L, true);
        return 1;
    }

    if (pRecognizer->isListening()) {
        pRecognizer->stopListening();
    }
    pMudlet->setSpeechRecognitionActive(false);

    lua_pushboolean(L, true);
    return 1;
}

// stt.toggle()
// Toggle speech recognition on/off.
// Returns true if now listening, false if stopped, or nil + error on failure.
int TLuaInterpreter::sttToggle(lua_State* L)
{
    const QString funcName = qsl("stt.toggle");

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "mudlet instance not available");
    }

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer || !pRecognizer->isInitialized()) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "speech recognizer not initialized - call stt.init() first");
    }

    if (pRecognizer->isListening()) {
        pRecognizer->stopListening();
        pMudlet->setSpeechRecognitionActive(false);
        lua_pushboolean(L, false);
    } else {
        pRecognizer->startListening();
        pMudlet->setSpeechRecognitionActive(true);
        lua_pushboolean(L, true);
    }

    return 1;
}

// stt.isListening()
// Check if speech recognition is currently active.
// Returns true if listening, false otherwise.
int TLuaInterpreter::sttIsListening(lua_State* L)
{
    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, pMudlet->isSpeechRecognitionActive());
    return 1;
}

// stt.isAvailable()
// Check if speech recognition is available (Vosk library loaded).
// Returns true if available, false otherwise.
int TLuaInterpreter::sttIsAvailable(lua_State* L)
{
    lua_pushboolean(L, VoskRecognizer::isLibraryAvailable());
    return 1;
}

// stt.isInitialized()
// Check if speech recognition has been initialized with a model.
// Returns true if initialized, false otherwise.
int TLuaInterpreter::sttIsInitialized(lua_State* L)
{
    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        lua_pushboolean(L, false);
        return 1;
    }

    auto* pRecognizer = pMudlet->speechRecognizer();
    lua_pushboolean(L, pRecognizer && pRecognizer->isInitialized());
    return 1;
}

// stt.getInfo()
// Get information about the speech recognition backend.
// Returns a table with: backend, version, available, initialized, listening
int TLuaInterpreter::sttGetInfo(lua_State* L)
{
    auto* pMudlet = mudlet::self();

    lua_newtable(L);

    // Backend name
    lua_pushstring(L, "backend");
    lua_pushstring(L, "Vosk");
    lua_settable(L, -3);

    // Library available
    lua_pushstring(L, "available");
    lua_pushboolean(L, VoskRecognizer::isLibraryAvailable());
    lua_settable(L, -3);

    if (pMudlet) {
        auto* pRecognizer = pMudlet->speechRecognizer();
        if (pRecognizer) {
            // Version
            lua_pushstring(L, "version");
            lua_pushstring(L, pRecognizer->backendVersion().toUtf8().constData());
            lua_settable(L, -3);

            // Initialized
            lua_pushstring(L, "initialized");
            lua_pushboolean(L, pRecognizer->isInitialized());
            lua_settable(L, -3);

            // Listening
            lua_pushstring(L, "listening");
            lua_pushboolean(L, pRecognizer->isListening());
            lua_settable(L, -3);

            // Current language
            lua_pushstring(L, "language");
            lua_pushstring(L, pRecognizer->currentLanguage().toUtf8().constData());
            lua_settable(L, -3);
        } else {
            lua_pushstring(L, "initialized");
            lua_pushboolean(L, false);
            lua_settable(L, -3);

            lua_pushstring(L, "listening");
            lua_pushboolean(L, false);
            lua_settable(L, -3);
        }
    }

    return 1;
}

// stt.getModelPath()
// Get the default path where speech models should be stored.
// Returns the path as a string.
int TLuaInterpreter::sttGetModelPath(lua_State* L)
{
    // Use the same global vosk-models directory as VoskRecognizer
    const QString modelPath = mudlet::getMudletPath(enums::mainDataItemPath, qsl("vosk-models"));
    lua_pushstring(L, modelPath.toUtf8().constData());
    return 1;
}

// stt.listModels()
// List available downloaded language models.
// Returns a table of model names/paths.
int TLuaInterpreter::sttListModels(lua_State* L)
{
    // Use the same global vosk-models directory as VoskRecognizer
    const QString modelBasePath = mudlet::getMudletPath(enums::mainDataItemPath, qsl("vosk-models"));
    QDir modelDir(modelBasePath);

    lua_newtable(L);

    if (!modelDir.exists()) {
        return 1; // Return empty table
    }

    const QStringList entries = modelDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int index = 1;
    for (const QString& entry : entries) {
        const QString fullPath = modelDir.filePath(entry);
        // Check if this looks like a valid Vosk model directory
        // Vosk models typically have am/, conf/, graph/, ivector/ subdirectories
        QDir entryDir(fullPath);
        if (entryDir.exists(qsl("am")) || entryDir.exists(qsl("conf")) || entryDir.exists(qsl("graph")) || entryDir.exists(qsl("ivector"))) {
            lua_pushinteger(L, index++);
            lua_newtable(L);

            lua_pushstring(L, "name");
            lua_pushstring(L, entry.toUtf8().constData());
            lua_settable(L, -3);

            lua_pushstring(L, "path");
            lua_pushstring(L, fullPath.toUtf8().constData());
            lua_settable(L, -3);

            lua_settable(L, -3);
        }
    }

    return 1;
}

// stt.close()
// Close and cleanup speech recognition resources.
// Returns true.
int TLuaInterpreter::sttClose(lua_State* L)
{
    auto* pMudlet = mudlet::self();
    if (pMudlet) {
        auto* pRecognizer = pMudlet->speechRecognizer();
        if (pRecognizer && pRecognizer->isListening()) {
            pRecognizer->cancel();
        }
        pMudlet->setSpeechRecognitionActive(false);
    }

    lua_pushboolean(L, true);
    return 1;
}
