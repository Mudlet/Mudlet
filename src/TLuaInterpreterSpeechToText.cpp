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

// Lowercase, script-friendly name for a recognizer state. Kept separate from
// the Q_ENUM name so that Lua sees a stable identifier regardless of how the
// enumerators are spelled in C++.
static const char* speechRecognizerStateName(const SpeechRecognizer::State state)
{
    switch (state) {
    case SpeechRecognizer::State::Ready:
        return "ready";
    case SpeechRecognizer::State::Listening:
        return "listening";
    case SpeechRecognizer::State::Processing:
        return "processing";
    case SpeechRecognizer::State::Error:
        return "error";
    case SpeechRecognizer::State::Uninitialized:
        break;
    }
    return "uninitialized";
}

// Lowercase, script-friendly name for an end-of-speech sensitivity, matching
// the words stt.setSensitivity() accepts.
static const char* speechSensitivityName(const SpeechRecognizer::Sensitivity sensitivity)
{
    switch (sensitivity) {
    case SpeechRecognizer::Sensitivity::Short:
        return "short";
    case SpeechRecognizer::Sensitivity::Long:
        return "long";
    case SpeechRecognizer::Sensitivity::Default:
        break;
    }
    return "default";
}

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
        lua_pushboolean(L, false);
    } else {
        pRecognizer->startListening();
        lua_pushboolean(L, true);
    }

    return 1;
}

// stt.isListening()
// Check if speech recognition is currently active.
// Returns true if listening, false otherwise.
// Reports the recognizer's own state rather than mudlet's active flag: that flag
// tracks what the user asked for, and nothing clears it when the recognizer
// leaves Listening on its own. Reading the recognizer keeps this in step with
// stt.getInfo().listening.
int TLuaInterpreter::sttIsListening(lua_State* L)
{
    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        lua_pushboolean(L, false);
        return 1;
    }

    auto* pRecognizer = pMudlet->speechRecognizer();
    lua_pushboolean(L, pRecognizer && pRecognizer->isListening());
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
// Returns a table with: backend, version, available, initialized, listening,
// state, language, modelPath, searchPaths
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

            // Engine state, which distinguishes Error from Uninitialized -
            // both of which report initialized == false
            lua_pushstring(L, "state");
            lua_pushstring(L, speechRecognizerStateName(pRecognizer->state()));
            lua_settable(L, -3);

            // Path of the model actually in use, as opposed to the directory
            // models are installed into that stt.getModelPath() reports
            lua_pushstring(L, "modelPath");
            lua_pushstring(L, pRecognizer->modelPath().toUtf8().constData());
            lua_settable(L, -3);

            // Milliseconds of continuous silence before listening stops
            // automatically; 0 while the timeout is disabled
            lua_pushstring(L, "silenceTimeout");
            lua_pushinteger(L, pRecognizer->silenceTimeout());
            lua_settable(L, -3);

            // Level last heard from the microphone, so a caller can tell a
            // misheard phrase from one that barely arrived
            lua_pushstring(L, "audioLevel");
            lua_pushnumber(L, pRecognizer->audioLevel());
            lua_settable(L, -3);

            // How quickly the engine calls an utterance finished
            lua_pushstring(L, "sensitivity");
            lua_pushstring(L, speechSensitivityName(pRecognizer->sensitivity()));
            lua_settable(L, -3);

            // What this backend can do, so packages adapt rather than guess:
            // biasing/grammar govern whether setVocabulary reaches the
            // engine, words whether sysSTTWords fires, onDevice whether audio
            // stays on this machine
            lua_pushstring(L, "capabilities");
            lua_newtable(L);
            lua_pushstring(L, "biasing");
            lua_pushboolean(L, pRecognizer->supportsBiasing());
            lua_settable(L, -3);
            lua_pushstring(L, "grammar");
            lua_pushboolean(L, pRecognizer->supportsGrammar());
            lua_settable(L, -3);
            lua_pushstring(L, "words");
            lua_pushboolean(L, pRecognizer->supportsWordResults());
            lua_settable(L, -3);
            lua_pushstring(L, "onDevice");
            lua_pushboolean(L, pRecognizer->onDevice());
            lua_settable(L, -3);
            lua_settable(L, -3);
        } else {
            lua_pushstring(L, "initialized");
            lua_pushboolean(L, false);
            lua_settable(L, -3);

            lua_pushstring(L, "listening");
            lua_pushboolean(L, false);
            lua_settable(L, -3);

            lua_pushstring(L, "state");
            lua_pushstring(L, speechRecognizerStateName(SpeechRecognizer::State::Uninitialized));
            lua_settable(L, -3);

            lua_pushstring(L, "modelPath");
            lua_pushstring(L, "");
            lua_settable(L, -3);
        }
    }

    lua_pushstring(L, "searchPaths");
    lua_newtable(L);
    int pathIndex = 1;
    for (const QString& path : VoskRecognizer::librarySearchPaths()) {
        lua_pushinteger(L, pathIndex++);
        lua_pushstring(L, path.toUtf8().constData());
        lua_settable(L, -3);
    }
    lua_settable(L, -3);

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

// stt.getLibraryPath()
// Get the user-writable directory the speech recognition library is installed into.
// Returns the path as a string.
int TLuaInterpreter::sttGetLibraryPath(lua_State* L)
{
    // Use the same vosk-lib directory VoskRecognizer searches
    lua_pushstring(L, VoskRecognizer::userLibraryPath().toUtf8().constData());
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
        if (pRecognizer) {
            if (pRecognizer->isListening()) {
                pRecognizer->cancel();
            }
            pRecognizer->releaseResources();
        }
    }

    lua_pushboolean(L, true);
    return 1;
}


// stt.setSilenceTimeout(milliseconds)
// Stop listening automatically after this long of continuous silence, with
// the utterance finalised exactly as stt.stop() would. 0 disables the
// timeout. The setting persists across listening sessions.
// Returns true, or nil + error message on failure.
int TLuaInterpreter::sttSetSilenceTimeout(lua_State* L)
{
    const int msec = getVerifiedInt(L, __func__, 1, "milliseconds");
    if (msec < 0) {
        return warnArgumentValue(L, __func__, qsl("milliseconds must be 0 (disabled) or greater, got %1").arg(msec));
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    pMudlet->initSpeechRecognition();
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, __func__, "failed to create speech recognizer");
    }

    pRecognizer->setSilenceTimeout(msec);
    lua_pushboolean(L, true);
    return 1;
}

// stt.setSensitivity(mode)
// How quickly the engine decides an utterance has ended: "short" for
// commands, "default" for balanced use, "long" for dictation. Engines apply
// this to their own end-of-speech detection, so the effect is comparable
// rather than identical between them.
// Returns true, or nil + error message on failure.
int TLuaInterpreter::sttSetSensitivity(lua_State* L)
{
    const QString mode = getVerifiedString(L, __func__, 1, "sensitivity").toLower();

    SpeechRecognizer::Sensitivity sensitivity;
    if (mode == QLatin1String("short")) {
        sensitivity = SpeechRecognizer::Sensitivity::Short;
    } else if (mode == QLatin1String("default")) {
        sensitivity = SpeechRecognizer::Sensitivity::Default;
    } else if (mode == QLatin1String("long")) {
        sensitivity = SpeechRecognizer::Sensitivity::Long;
    } else {
        return warnArgumentValue(L, __func__, qsl(R"(sensitivity must be "short", "default" or "long", got "%1")").arg(mode));
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    pMudlet->initSpeechRecognition();
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, __func__, "failed to create speech recognizer");
    }

    pRecognizer->setSensitivity(sensitivity);
    lua_pushboolean(L, true);
    return 1;
}

// stt.setVocabulary(words)
// Supply a table (array) of words or phrases for the backend to bias or
// constrain recognition toward. Returns true only when the engine applied
// the vocabulary; false is not an error - it means this backend cannot use
// it (see stt.getInfo().capabilities) and callers should correct results
// client-side instead.
int TLuaInterpreter::sttSetVocabulary(lua_State* L)
{
    if (!lua_istable(L, 1)) {
        lua_pushfstring(L, "stt.setVocabulary: bad argument #1 type (words as table expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    QStringList words;
    for (int i = 1;; ++i) {
        lua_rawgeti(L, 1, i);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }
        if (lua_type(L, -1) == LUA_TSTRING) {
            const QString word = QString::fromUtf8(lua_tostring(L, -1)).trimmed();
            if (!word.isEmpty()) {
                words.append(word);
            }
        }
        lua_pop(L, 1);
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, __func__, "mudlet instance not available");
    }

    pMudlet->initSpeechRecognition();
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, __func__, "failed to create speech recognizer");
    }

    lua_pushboolean(L, pRecognizer->setVocabulary(words));
    return 1;
}

// stt.getPlatformKey()
// Identify the platform and architecture for selecting a Vosk library build.
// Returns a key string, or nil if this platform has no published build.
int TLuaInterpreter::sttGetPlatformKey(lua_State* L)
{
#if defined(Q_OS_MACOS)
    lua_pushstring(L, "macos");
#elif defined(Q_OS_WIN)
#if defined(Q_PROCESSOR_ARM_64)
    lua_pushnil(L);
#elif defined(Q_PROCESSOR_X86_64)
    lua_pushstring(L, "windows-x64");
#else
    lua_pushstring(L, "windows-x86");
#endif
#elif defined(Q_OS_LINUX)
#if defined(Q_PROCESSOR_ARM_64)
    lua_pushstring(L, "linux-aarch64");
#elif defined(Q_PROCESSOR_X86_64)
    lua_pushstring(L, "linux-x86_64");
#else
    lua_pushnil(L);
#endif
#else
    lua_pushnil(L);
#endif
    return 1;
}

// stt.reloadLibrary()
// Re-run Vosk library detection, for use after installing the library.
// Returns whether the library is now available, or false plus a message if
// the recognizer is in use, or still holds live native resources, and cannot
// be safely unloaded.
int TLuaInterpreter::sttReloadLibrary(lua_State* L)
{
    auto* pMudlet = mudlet::self();
    if (pMudlet) {
        auto* pRecognizer = pMudlet->speechRecognizer();
        // isInitialized() is false in State::Error, but Error can still be
        // reached with live native handles (e.g. a failure partway through
        // startListeningInternal() after the native recognizer was already
        // allocated), so check hasLiveNativeResources() directly rather than
        // relying on state alone. A failed initialize() before any handle was
        // allocated also leaves the recognizer in Error, and that case must
        // stay reloadable since it's exactly what stt.reloadLibrary() is for.
        if (pRecognizer && (pRecognizer->isListening() || pRecognizer->isInitialized() || pRecognizer->hasLiveNativeResources())) {
            return warnArgumentValue(L, __func__, "cannot reload the speech recognition library while it is in use, close speech recognition first", true);
        }
    }

    VoskRecognizer::resetLibraryLoadState();
    lua_pushboolean(L, VoskRecognizer::isLibraryAvailable());
    return 1;
}

// stt.unloadLibrary()
// Unload the Vosk library without probing for it again, so that its file can be
// deleted. Windows refuses to delete a module that is still mapped, so removing
// the library has to go through here first.
// Returns true once unloaded, or false plus a message if the recognizer is in
// use, or still holds live native resources, and cannot be safely unloaded.
int TLuaInterpreter::sttUnloadLibrary(lua_State* L)
{
    auto* pMudlet = mudlet::self();
    if (pMudlet) {
        auto* pRecognizer = pMudlet->speechRecognizer();
        // Same guard as stt.reloadLibrary(): see the comment there for why
        // hasLiveNativeResources() is checked rather than state alone.
        if (pRecognizer && (pRecognizer->isListening() || pRecognizer->isInitialized() || pRecognizer->hasLiveNativeResources())) {
            return warnArgumentValue(L, __func__, "cannot unload the speech recognition library while it is in use, close speech recognition first", true);
        }
    }

    VoskRecognizer::resetLibraryLoadState();
    lua_pushboolean(L, true);
    return 1;
}
