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
    case SpeechRecognizer::State::Starting:
        return "starting";
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

// Whether a startListening() request was accepted. The call returns nothing
// and can refuse - a phrase still being processed, a microphone that will not
// open, permission denied - so the state afterwards is what says whether
// anything is going to happen. Starting means accepted but not yet listening,
// because something outside the process has still to answer; the outcome
// arrives through sysSTTStateChanged rather than from this call.
static bool speechStartAccepted(const SpeechRecognizer* pRecognizer)
{
    return pRecognizer->listening() || pRecognizer->starting();
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

    if (!QDir(modelPath).exists()) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), qsl("model path does not exist: %1").arg(modelPath).toUtf8().constData());
    }

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

    if (!pRecognizer->initialized()) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "speech recognizer not initialized with a model - call stt.init() first");
    }

    if (pRecognizer->listening()) {
        lua_pushboolean(L, true);
        return 1;
    }

    pRecognizer->startListening();
    if (!speechStartAccepted(pRecognizer)) {
        // The recognizer has already said why through sysSTTError; what
        // matters here is not telling the caller that recording began
        return warnArgumentValue(L, funcName.toUtf8().constData(), "could not start listening - the sysSTTError event carries the reason");
    }

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

    if (pRecognizer->listening()) {
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
    if (!pRecognizer || !pRecognizer->initialized()) {
        return warnArgumentValue(L, funcName.toUtf8().constData(), "speech recognizer not initialized - call stt.init() first");
    }

    if (pRecognizer->listening()) {
        pRecognizer->stopListening();
        lua_pushboolean(L, false);
    } else {
        pRecognizer->startListening();
        if (!speechStartAccepted(pRecognizer)) {
            return warnArgumentValue(L, funcName.toUtf8().constData(), "could not start listening - the sysSTTError event carries the reason");
        }
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
    lua_pushboolean(L, pRecognizer && pRecognizer->listening());
    return 1;
}

// stt.isAvailable()
// Check if speech recognition is available (Vosk library loaded).
// Returns true if available, false otherwise.
int TLuaInterpreter::sttIsAvailable(lua_State* L)
{
    lua_pushboolean(L, VoskRecognizer::libraryAvailable());
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
    lua_pushboolean(L, pRecognizer && pRecognizer->initialized());
    return 1;
}

// stt.getInfo()
// Get information about the speech recognition backend.
// The keys and their meanings are specified in docs/stt-api.md rather than
// listed again here, because a second copy of that list has already drifted
// once behind the keys this function actually sets.
int TLuaInterpreter::sttGetInfo(lua_State* L)
{
    auto* pMudlet = mudlet::self();

    lua_newtable(L);

    lua_pushstring(L, "backend");
    lua_pushstring(L, "Vosk");
    lua_settable(L, -3);

    lua_pushstring(L, "available");
    lua_pushboolean(L, VoskRecognizer::libraryAvailable());
    lua_settable(L, -3);

    // Every key below is answered whether or not a recognizer exists. A
    // package reads these to decide what it can do, and it reads them before
    // anything is installed - which was exactly when they were absent, so the
    // documented probe getInfo().capabilities.words was a nil index on any
    // machine without an engine.
    auto* pRecognizer = pMudlet ? pMudlet->speechRecognizer() : nullptr;

    lua_pushstring(L, "initialized");
    lua_pushboolean(L, pRecognizer && pRecognizer->initialized());
    lua_settable(L, -3);

    lua_pushstring(L, "listening");
    lua_pushboolean(L, pRecognizer && pRecognizer->listening());
    lua_settable(L, -3);

    // Engine state, which distinguishes Error from Uninitialized - both of
    // which report initialized == false
    lua_pushstring(L, "state");
    lua_pushstring(L, speechRecognizerStateName(pRecognizer ? pRecognizer->state() : SpeechRecognizer::State::Uninitialized));
    lua_settable(L, -3);

    // Path of the model actually in use, as opposed to the directory models
    // are installed into that stt.getModelPath() reports
    lua_pushstring(L, "modelPath");
    lua_pushstring(L, pRecognizer ? pRecognizer->modelPath().toUtf8().constData() : "");
    lua_settable(L, -3);

    // Milliseconds of continuous silence before listening stops
    // automatically; 0 while the timeout is disabled
    lua_pushstring(L, "silenceTimeout");
    lua_pushinteger(L, pRecognizer ? pRecognizer->silenceTimeout() : 0);
    lua_settable(L, -3);

    // Smoothed level of recent input, so a caller can tell a misheard phrase
    // from one that barely arrived
    lua_pushstring(L, "audioLevel");
    lua_pushnumber(L, pRecognizer ? pRecognizer->audioLevel() : 0.0f);
    lua_settable(L, -3);

    // How quickly the engine calls an utterance finished
    lua_pushstring(L, "sensitivity");
    lua_pushstring(L, pRecognizer ? speechSensitivityName(pRecognizer->sensitivity()) : "default");
    lua_settable(L, -3);

    // What this backend can do, so packages adapt rather than guess:
    // biasing/grammar govern whether setVocabulary reaches the engine, words
    // whether sysSTTWords fires, onDevice whether audio stays on this machine.
    // With no engine every answer is false, including onDevice: there is no
    // backend to make a privacy guarantee, and claiming one nobody gave is
    // the wrong way to be wrong.
    lua_pushstring(L, "capabilities");
    lua_newtable(L);
    lua_pushstring(L, "biasing");
    lua_pushboolean(L, pRecognizer && pRecognizer->supportsBiasing());
    lua_settable(L, -3);
    lua_pushstring(L, "grammar");
    lua_pushboolean(L, pRecognizer && pRecognizer->supportsGrammar());
    lua_settable(L, -3);
    lua_pushstring(L, "words");
    lua_pushboolean(L, pRecognizer && pRecognizer->supportsWordResults());
    lua_settable(L, -3);
    lua_pushstring(L, "onDevice");
    lua_pushboolean(L, pRecognizer && pRecognizer->onDevice());
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Only meaningful once an instance exists, and documented as such
    if (pRecognizer) {
        lua_pushstring(L, "version");
        lua_pushstring(L, pRecognizer->backendVersion().toUtf8().constData());
        lua_settable(L, -3);

        lua_pushstring(L, "language");
        lua_pushstring(L, pRecognizer->currentLanguage().toUtf8().constData());
        lua_settable(L, -3);
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
    lua_pushstring(L, VoskRecognizer::modelsDirectoryPath().toUtf8().constData());
    return 1;
}

// stt.getLibraryPath()
// Get the user-writable directory the speech recognition library is installed into.
// Returns the path as a string.
int TLuaInterpreter::sttGetLibraryPath(lua_State* L)
{
    lua_pushstring(L, VoskRecognizer::userLibraryPath().toUtf8().constData());
    return 1;
}

// stt.listModels()
// List available downloaded language models.
// Returns a table of model names/paths.
int TLuaInterpreter::sttListModels(lua_State* L)
{
    const QDir modelsDir(VoskRecognizer::modelsDirectoryPath());

    lua_newtable(L);

    const QStringList installedModels = VoskRecognizer::getInstalledModels();
    int index = 1;
    for (const QString& model : installedModels) {
        lua_pushinteger(L, index++);
        lua_newtable(L);

        lua_pushstring(L, "name");
        lua_pushstring(L, model.toUtf8().constData());
        lua_settable(L, -3);

        lua_pushstring(L, "path");
        lua_pushstring(L, modelsDir.filePath(model).toUtf8().constData());
        lua_settable(L, -3);

        lua_settable(L, -3);
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
            if (pRecognizer->listening()) {
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
    const int msec = getVerifiedInt(L, "stt.setSilenceTimeout", 1, "milliseconds");
    if (msec < 0) {
        return warnArgumentValue(L, "stt.setSilenceTimeout", qsl("milliseconds must be 0 (disabled) or greater, got %1").arg(msec));
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, "stt.setSilenceTimeout", "mudlet instance not available");
    }

    pMudlet->initSpeechRecognition();
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, "stt.setSilenceTimeout", "failed to create speech recognizer");
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
    const QString mode = getVerifiedString(L, "stt.setSensitivity", 1, "sensitivity").toLower();

    SpeechRecognizer::Sensitivity sensitivity;
    if (mode == QLatin1String("short")) {
        sensitivity = SpeechRecognizer::Sensitivity::Short;
    } else if (mode == QLatin1String("default")) {
        sensitivity = SpeechRecognizer::Sensitivity::Default;
    } else if (mode == QLatin1String("long")) {
        sensitivity = SpeechRecognizer::Sensitivity::Long;
    } else {
        return warnArgumentValue(L, "stt.setSensitivity", qsl(R"(sensitivity must be "short", "default" or "long", got "%1")").arg(mode));
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, "stt.setSensitivity", "mudlet instance not available");
    }

    pMudlet->initSpeechRecognition();
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, "stt.setSensitivity", "failed to create speech recognizer");
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
        return warnArgumentValue(L, "stt.setVocabulary", "mudlet instance not available");
    }

    pMudlet->initSpeechRecognition();
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        return warnArgumentValue(L, "stt.setVocabulary", "failed to create speech recognizer");
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
        // initialized() is false in State::Error, but Error can still be
        // reached with live native handles (e.g. a failure partway through
        // startListeningInternal() after the native recognizer was already
        // allocated), so check hasLiveNativeResources() directly rather than
        // relying on state alone. A failed initialize() before any handle was
        // allocated also leaves the recognizer in Error, and that case must
        // stay reloadable since it's exactly what stt.reloadLibrary() is for.
        if (pRecognizer && (pRecognizer->listening() || pRecognizer->initialized() || pRecognizer->hasLiveNativeResources())) {
            return warnArgumentValue(L, __func__, "cannot reload the speech recognition library while it is in use, close speech recognition first", true);
        }
    }

    // The unload may be refused while something still holds the module; the
    // probe below then reports what is actually loadable either way
    VoskRecognizer::resetLibraryLoadState();
    lua_pushboolean(L, VoskRecognizer::libraryAvailable());
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
        if (pRecognizer && (pRecognizer->listening() || pRecognizer->initialized() || pRecognizer->hasLiveNativeResources())) {
            return warnArgumentValue(L, __func__, "cannot unload the speech recognition library while it is in use, close speech recognition first", true);
        }
    }

    if (!VoskRecognizer::resetLibraryLoadState()) {
        return warnArgumentValue(L, "stt.unloadLibrary", "the speech recognition library is still mapped and could not be unloaded, so its file cannot be replaced yet", true);
    }

    lua_pushboolean(L, true);
    return 1;
}
