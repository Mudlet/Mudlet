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
#include "SherpaRecognizer.h"
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

// docs/stt-api.md's "refusals speak": a consumer driving the bridge from events
// has to hear a refusal too, and with no engine installed there is no recognizer
// to emit errorOccurred through - so this raises the event on mudlet itself.
// Without it "no engine" and "nothing said yet" look identical from Lua.
static void reportSpeechRefusal(const QString& message)
{
    if (auto* pMudlet = mudlet::self()) {
        pMudlet->raiseSpeechEvent(qsl("sysSTTError"), message);
    }
}

// Every directory a dynamically-loaded engine could have been installed into,
// across all of them rather than Vosk's alone. A reader told where Mudlet
// looked has to be told where it looked for the engine they installed, and on
// a sherpa-only machine that was never any of the paths this used to name.
static QStringList speechLibrarySearchPaths()
{
    QStringList paths = SherpaRecognizer::librarySearchPaths();
    for (const QString& path : VoskRecognizer::librarySearchPaths()) {
        if (!paths.contains(path)) {
            paths.append(path);
        }
    }
    return paths;
}

// The message for a call that needs an engine when there is none. "failed to
// create speech recognizer" described the symptom of a missing library rather
// than the library, and sent people looking for a fault in Mudlet.
static QString noEngineMessage()
{
    if (VoskRecognizer::libraryUnloadedByRequest()) {
        return qsl("the speech engine library was unloaded on request - call stt.reloadLibrary() before using speech recognition again");
    }
    return qsl("the speech engine library is not installed, so speech recognition cannot be used - looked in: %1").arg(speechLibrarySearchPaths().join(qsl(", ")));
}

// words follows a symbol resolved from the library, so unloading or reloading it
// changes what the backend can do without anything else happening. Announced
// here because docs/stt-api.md tells consumers to re-read capabilities on a
// change rather than cache them, which needs the change to be announced at all.
static void announceSpeechCapabilities(mudlet* pMudlet)
{
    auto* pRecognizer = pMudlet ? qobject_cast<VoskRecognizer*>(pMudlet->speechRecognizer()) : nullptr;
    if (pRecognizer) {
        pRecognizer->announceCapabilitiesIfChanged();
    }
}

// Whether any speech engine at all is present and loadable: a model-based
// one (Vosk, sherpa-onnx), or - since stt.init() can now reach it with no
// model at all - the built-in macOS backend. availableBackends() deliberately
// excludes the latter (see its own comment), so it is asked about separately
// here rather than trusting that list alone.
static bool speechEngineAvailable()
{
    return !SpeechRecognizerFactory::availableBackends().isEmpty() || SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform);
}

// Which backend the on-demand initSpeechRecognition() calls below should ask
// for, so that what stt.available() counts and what those calls can actually
// build are the same set. Auto resolves through availableBackends(), which
// deliberately omits the model-less macOS backend - so on a Mac with neither
// Vosk nor sherpa installed an eager Auto built nothing and stt.start()
// refused with "the speech engine library is not installed" while
// stt.available() answered true. Naming Platform in exactly that case is what
// closes that gap; the backend genuinely works there once stt.init() has run.
static SpeechRecognizerFactory::Backend onDemandSpeechBackend()
{
    if (!SpeechRecognizerFactory::availableBackends().isEmpty()) {
        return SpeechRecognizerFactory::Backend::Auto;
    }
    if (SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform)) {
        return SpeechRecognizerFactory::Backend::Platform;
    }
    return SpeechRecognizerFactory::Backend::Auto;
}

// Which model-based backend's on-disk install paths answer stt.getModelPath(),
// stt.getLibraryPath() and getInfo().searchPaths: whichever is actually loaded
// when it is one of these two, the auto-preferred installed backend otherwise,
// and Vosk's own paths as the last resort - so these platform-tier reads
// always name a real, checkable directory, exactly as they did before sherpa
// or the built-in macOS backend existed. The macOS backend never answers for
// these: it installs no library and needs no model, so it has no paths of its
// own to report.
static SpeechRecognizerFactory::Backend modelBasedBackendForPaths(mudlet* pMudlet)
{
    auto* pRecognizer = pMudlet ? pMudlet->speechRecognizer() : nullptr;
    if (qobject_cast<SherpaRecognizer*>(pRecognizer)) {
        return SpeechRecognizerFactory::Backend::Sherpa;
    }
    if (qobject_cast<VoskRecognizer*>(pRecognizer)) {
        return SpeechRecognizerFactory::Backend::Vosk;
    }

    const auto backends = SpeechRecognizerFactory::availableBackends();
    return backends.isEmpty() ? SpeechRecognizerFactory::Backend::Vosk : backends.first();
}

// The directory stt.getModelPath() answers with: the same engine choice, so a
// refusal telling a package where to install a model never names a directory
// other than the one the API told it to use.
static QString speechModelsDirectory(mudlet* pMudlet)
{
    return modelBasedBackendForPaths(pMudlet) == SpeechRecognizerFactory::Backend::Sherpa ? SherpaRecognizer::modelsDirectoryPath() : VoskRecognizer::modelsDirectoryPath();
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
    const char* funcName = "stt.init";
    QString modelPath;
    bool usedDefaultModel = false;
    // Set only when no path was given and the backend this call is about to
    // use needs none - the built-in macOS one today. Skips both the "no
    // model installed" refusal below and the model-path existence check
    // further down, since there is no path to check.
    bool useModelLessBackend = false;
    SpeechRecognizerFactory::Backend backend = SpeechRecognizerFactory::Backend::Auto;

    if (lua_gettop(L) >= 1 && !lua_isnoneornil(L, 1)) {
        modelPath = getVerifiedString(L, funcName, 1, "model path");
        // An empty path is a bad argument, not a model. QDir("") is Qt's
        // spelling for the working directory, so the existence check below
        // would pass and the engine would be handed wherever Mudlet happens
        // to have been started from.
        if (modelPath.trimmed().isEmpty()) {
            return warnArgumentValue(L, funcName, "the model path is empty - give the folder a model was installed into, or call stt.init() with no argument to use the default");
        }
        // The model directory says which engine it belongs to, so a package
        // that only ever installs one engine's models never has to name it
        // separately. A layout that matches nothing falls back to Auto rather
        // than a guess - guessing wrong hands a model to the wrong decoder and
        // fails deep inside the library instead of here.
        backend = SpeechRecognizerFactory::backendForModelDir(modelPath);
    } else {
        usedDefaultModel = true;
        // Asked of every installed engine rather than only the preferred one.
        // Auto resolves through availableBackends().first(), where sherpa now
        // sorts ahead of Vosk, so a machine carrying the sherpa library but
        // only Vosk models on disk got an empty answer and a refusal - having
        // loaded that same Vosk model quite happily before sherpa existed. The
        // engine is pinned to whichever one's model was actually found, so a
        // model and a decoder from different engines can never be paired.
        for (const SpeechRecognizerFactory::Backend candidate : SpeechRecognizerFactory::availableBackends()) {
            const QString candidateModelPath = SpeechRecognizerFactory::defaultModelPath(candidate);
            if (!candidateModelPath.isEmpty()) {
                modelPath = candidateModelPath;
                backend = candidate;
                break;
            }
        }
        if (modelPath.isEmpty()) {
            // Two different problems wore one message: defaultModelPath() is
            // empty whenever no backend is available, which is the case when
            // the engine *library* is missing however many models are
            // installed. Telling someone to install what they already have
            // sends them looking in the wrong place.
            if (SpeechRecognizerFactory::availableBackends().isEmpty()) {
                // No model-based engine is installed. A model-less backend -
                // the built-in macOS one today - still works with nothing to
                // install, and hiding it here would repeat the mistake
                // availableBackends() deliberately does not make.
                if (SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform)) {
                    useModelLessBackend = true;
                    backend = SpeechRecognizerFactory::Backend::Platform;
                } else {
                    const QString message = VoskRecognizer::libraryUnloadedByRequest()
                                                    ? qsl("the speech engine library was unloaded on request - call stt.reloadLibrary() before loading a model")
                                                    : qsl("the speech engine library is not installed, so no model can be loaded - looked in: %1").arg(speechLibrarySearchPaths().join(qsl(", ")));
                    reportSpeechRefusal(message);
                    return warnArgumentValue(L, funcName, message);
                }
            } else {
                const QString message = qsl("no model path provided and no language model is installed - install one into %1").arg(speechModelsDirectory(mudlet::self()));
                reportSpeechRefusal(message);
                return warnArgumentValue(L, funcName, message);
            }
        }
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName, "mudlet instance not available");
    }

    if (!useModelLessBackend && !QDir(modelPath).exists()) {
        const QString message = qsl("model path does not exist: %1").arg(modelPath);
        reportSpeechRefusal(message);
        return warnArgumentValue(L, funcName, message);
    }

    pMudlet->initSpeechRecognition(backend);

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        const QString message = noEngineMessage();
        reportSpeechRefusal(message);
        return warnArgumentValue(L, funcName, message);
    }

    if (!pRecognizer->initialize(modelPath)) {
        // initialize() has already said what went wrong through sysSTTError
        return warnArgumentValue(L, funcName, qsl("failed to initialize model from: %1").arg(modelPath));
    }

    // Settings can name a model that is no longer installed, and
    // getSelectedModelPath() then loads whatever else is on disk. That keeps
    // speech working, which is the right call, but a package configured for one
    // language would otherwise be handed a decoder for another with nothing
    // said - init true, no event, and a language key it never asked about.
    if (usedDefaultModel && !useModelLessBackend) {
        if (const QString missing = VoskRecognizer::missingSelectedModel(); !missing.isEmpty()) {
            reportSpeechRefusal(qsl("the selected speech model %1 is not installed; loaded %2 instead").arg(missing, QDir(modelPath).dirName()));
        }
    }

    lua_pushboolean(L, true);
    return 1;
}

// stt.start()
// Start listening for speech input.
// Returns true on success, or nil + error message on failure.
int TLuaInterpreter::sttStart(lua_State* L)
{
    const char* funcName = "stt.start";

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName, "mudlet instance not available");
    }

    // The recognizer is only built on demand, so without this a first call
    // before stt.init() finds nothing and reports the library as missing on a
    // machine where stt.getInfo().available is true. Every setter already
    // builds it here for the same reason.
    pMudlet->initSpeechRecognition(onDemandSpeechBackend());

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        const QString message = noEngineMessage();
        reportSpeechRefusal(message);
        return warnArgumentValue(L, funcName, message);
    }

    if (!pRecognizer->initialized()) {
        const QString message = qsl("speech recognizer not initialized with a model - call stt.init() first");
        reportSpeechRefusal(message);
        return warnArgumentValue(L, funcName, message);
    }

    if (pRecognizer->listening()) {
        lua_pushboolean(L, true);
        return 1;
    }

    pRecognizer->startListening();
    if (!speechStartAccepted(pRecognizer)) {
        // The recognizer has already said why through sysSTTError; what
        // matters here is not telling the caller that recording began
        return warnArgumentValue(L, funcName, "could not start listening - the sysSTTError event carries the reason");
    }

    lua_pushboolean(L, true);
    return 1;
}

// stt.stop()
// Stop listening and process any remaining audio.
// Returns true on success, or nil + error message on failure.
int TLuaInterpreter::sttStop(lua_State* L)
{
    const char* funcName = "stt.stop";

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName, "mudlet instance not available");
    }

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        // Not initialized is fine - just return true
        lua_pushboolean(L, true);
        return 1;
    }

    // "Stopped" and "was never running because it failed" are different
    // answers, and returning true for both told a caller its session had ended
    // cleanly when the engine had faulted and produced nothing.
    if (pRecognizer->state() == SpeechRecognizer::State::Error) {
        return warnArgumentValue(L, funcName, "nothing was stopped - speech recognition is in an error state; the sysSTTError event carries the reason");
    }

    if (pRecognizer->listening()) {
        pRecognizer->stopListening();
    } else if (pRecognizer->starting()) {
        // A start still waiting on the permission dialog: there is no audio to
        // finalise, and leaving it pending means answering the dialog later
        // opens the microphone after the player asked to stop
        pRecognizer->cancel();
    }

    lua_pushboolean(L, true);
    return 1;
}

// stt.toggle()
// Toggle speech recognition on/off.
// Returns true if now listening, false if stopped, or nil + error on failure.
int TLuaInterpreter::sttToggle(lua_State* L)
{
    const char* funcName = "stt.toggle";

    auto* pMudlet = mudlet::self();
    if (!pMudlet) {
        return warnArgumentValue(L, funcName, "mudlet instance not available");
    }

    pMudlet->initSpeechRecognition(onDemandSpeechBackend());

    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer || !pRecognizer->initialized()) {
        const QString message = pRecognizer ? qsl("speech recognizer not initialized - call stt.init() first") : noEngineMessage();
        reportSpeechRefusal(message);
        return warnArgumentValue(L, funcName, message);
    }

    if (pRecognizer->listening()) {
        pRecognizer->stopListening();
        lua_pushboolean(L, false);
    } else {
        pRecognizer->startListening();
        if (!speechStartAccepted(pRecognizer)) {
            return warnArgumentValue(L, funcName, "could not start listening - the sysSTTError event carries the reason");
        }
        lua_pushboolean(L, true);
    }

    return 1;
}

// stt.listening()
// Check if speech recognition is currently active.
// Returns true if listening, false otherwise.
// Reads the recognizer's state, which docs/stt-api.md makes the single truth:
// there is deliberately no parallel "active" flag tracking what the user asked
// for, because nothing would clear it when the recognizer leaves Listening on
// its own. This stays in step with stt.getInfo().listening for the same reason.
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

// stt.available()
// Check if any speech engine - Vosk, sherpa-onnx, or the built-in macOS
// backend - is present and loadable.
// Returns true if available, false otherwise.
int TLuaInterpreter::sttIsAvailable(lua_State* L)
{
    lua_pushboolean(L, speechEngineAvailable());
    return 1;
}

// stt.initialized()
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

    // Every key below is answered whether or not a recognizer exists. A package
    // reads these to decide what it can do, and it reads them before anything is
    // installed - which was exactly when they were absent, so the documented
    // probe getInfo().capabilities.words was a nil index on any machine without
    // an engine.
    auto* pRecognizer = pMudlet ? pMudlet->speechRecognizer() : nullptr;

    // Asked of the recognizer once one exists, so the name cannot drift from
    // the backend actually running. Empty before that: this build can create
    // three backends now, and naming one of them on the strength of nothing
    // would be the fixed value docs/stt-api.md says this key is not.
    lua_pushstring(L, "backend");
    lua_pushstring(L, pRecognizer ? pRecognizer->backendName().toUtf8().constData() : "");
    lua_settable(L, -3);

    lua_pushstring(L, "available");
    lua_pushboolean(L, speechEngineAvailable());
    lua_settable(L, -3);

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
    const bool searchSherpaPaths = modelBasedBackendForPaths(pMudlet) == SpeechRecognizerFactory::Backend::Sherpa;
    const QStringList searchPaths = searchSherpaPaths ? SherpaRecognizer::librarySearchPaths() : VoskRecognizer::librarySearchPaths();
    for (const QString& path : searchPaths) {
        lua_pushinteger(L, pathIndex++);
        lua_pushstring(L, path.toUtf8().constData());
        lua_settable(L, -3);
    }
    lua_settable(L, -3);

    return 1;
}

// stt.getModelPath()
// Get the default path where speech models should be stored, for whichever
// model-based engine is actually loaded (falling back to the auto-preferred
// installed one, then Vosk, when none is loaded yet).
// Returns the path as a string.
int TLuaInterpreter::sttGetModelPath(lua_State* L)
{
    const QString path = speechModelsDirectory(mudlet::self());
    lua_pushstring(L, path.toUtf8().constData());
    return 1;
}

// stt.getLibraryPath()
// Get the user-writable directory the speech recognition library is
// installed into, for whichever model-based engine is actually loaded (see
// stt.getModelPath()).
// Returns the path as a string.
int TLuaInterpreter::sttGetLibraryPath(lua_State* L)
{
    const bool sherpa = modelBasedBackendForPaths(mudlet::self()) == SpeechRecognizerFactory::Backend::Sherpa;
    const QString path = sherpa ? SherpaRecognizer::userLibraryPath() : VoskRecognizer::userLibraryPath();
    lua_pushstring(L, path.toUtf8().constData());
    return 1;
}

// stt.listModels()
// List available downloaded language models, across every model-based engine
// - not only whichever is currently loaded - so a model downloaded for one
// engine stays visible while another is active, or before any is.
// Returns a table of model names/paths.
int TLuaInterpreter::sttListModels(lua_State* L)
{
    lua_newtable(L);
    int index = 1;

    const QDir voskModelsDir(VoskRecognizer::modelsDirectoryPath());
    for (const QString& model : VoskRecognizer::getInstalledModels()) {
        lua_pushinteger(L, index++);
        lua_newtable(L);

        lua_pushstring(L, "name");
        lua_pushstring(L, model.toUtf8().constData());
        lua_settable(L, -3);

        lua_pushstring(L, "path");
        lua_pushstring(L, voskModelsDir.filePath(model).toUtf8().constData());
        lua_settable(L, -3);

        lua_settable(L, -3);
    }

    const QDir sherpaModelsDir(SherpaRecognizer::modelsDirectoryPath());
    for (const QString& model : SherpaRecognizer::getInstalledModels()) {
        lua_pushinteger(L, index++);
        lua_newtable(L);

        lua_pushstring(L, "name");
        lua_pushstring(L, model.toUtf8().constData());
        lua_settable(L, -3);

        lua_pushstring(L, "path");
        lua_pushstring(L, sherpaModelsDir.filePath(model).toUtf8().constData());
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

    pMudlet->initSpeechRecognition(onDemandSpeechBackend());
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        const QString message = noEngineMessage();
        reportSpeechRefusal(message);
        return warnArgumentValue(L, "stt.setSilenceTimeout", message);
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

    pMudlet->initSpeechRecognition(onDemandSpeechBackend());
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        const QString message = noEngineMessage();
        reportSpeechRefusal(message);
        return warnArgumentValue(L, "stt.setSensitivity", message);
    }

    if (!pRecognizer->setSensitivity(sensitivity)) {
        const QString message = qsl("this build of the speech engine cannot tune end-of-speech detection");
        reportSpeechRefusal(message);
        return warnArgumentValue(L, "stt.setSensitivity", message);
    }
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

    pMudlet->initSpeechRecognition(onDemandSpeechBackend());
    auto* pRecognizer = pMudlet->speechRecognizer();
    if (!pRecognizer) {
        const QString message = noEngineMessage();
        reportSpeechRefusal(message);
        return warnArgumentValue(L, "stt.setVocabulary", message);
    }

    // The Lua answer stays a boolean, as documented: applied or not. A
    // backend that could have applied these words and did not is a fault
    // rather than a limitation, so it also says so where faults are reported -
    // otherwise the caller quietly falls back to correcting results itself and
    // never learns the engine had a problem.
    const auto outcome = pRecognizer->setVocabulary(words);
    if (outcome == SpeechRecognizer::VocabularyResult::Failed) {
        //: Shown when the speech engine could have used the game's vocabulary but failed to
        emit pRecognizer->errorOccurred(tr("Speech recognition could not apply the supplied vocabulary."));
    }

    lua_pushboolean(L, outcome == SpeechRecognizer::VocabularyResult::Applied);
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

    // A refused unload leaves every pointer and both flags as they were, so
    // libraryAvailable() below would skip the probe and answer from cache -
    // reporting a successful reload of a module that was never released
    if (!VoskRecognizer::resetLibraryLoadState()) {
        return warnArgumentValue(L, "stt.reloadLibrary", "the speech recognition library is still mapped and could not be released, so detection could not be re-run", true);
    }
    // Lift the latch stt.unloadLibrary() set, since asking for a reload is
    // exactly the caller saying they are done replacing the file
    VoskRecognizer::unloadLibraryByRequest(false);
    const bool available = VoskRecognizer::libraryAvailable();
    announceSpeechCapabilities(pMudlet);
    lua_pushboolean(L, available);
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

    // Stays unloaded until stt.reloadLibrary() asks for it back: without this
    // the next read-shaped call - getInfo(), available() - maps it straight
    // back in, and the file the caller meant to replace is locked again
    VoskRecognizer::unloadLibraryByRequest(true);
    announceSpeechCapabilities(pMudlet);

    lua_pushboolean(L, true);
    return 1;
}
