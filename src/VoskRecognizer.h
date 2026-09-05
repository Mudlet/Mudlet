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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef MUDLET_VOSKRECOGNIZER_H
#define MUDLET_VOSKRECOGNIZER_H

#include "SpeechRecognizer.h"
#include "utils.h"

#include <QLibrary>
#include <QPointer>
#include <QString>
#include <QVariantList>

#include <optional>

class QJsonArray;
class QJsonObject;
class SpeechAudioCapture;

// Forward declarations for Vosk types (opaque pointers). The recognizer handle is
// not called VoskRecognizer because that is this file's Qt class, which would hide
// the struct inside the class scope and force a ::-qualification on every use.
struct VoskModel;
struct VoskRecognizerHandle;

// Vosk-based implementation of SpeechRecognizer.
// Uses the Vosk offline speech recognition library (https://alphacephei.com/vosk/).
// The library is loaded dynamically at runtime to allow graceful fallback
// if Vosk is not available.

class VoskRecognizer : public SpeechRecognizer
{
    Q_OBJECT

public:
    // Endpointer mode controls how quickly Vosk detects end of speech
    enum class EndpointerMode {
        Default = 0,   // Balanced (default, good for most use)
        VeryShort = 1, // Very fast detection (may cut off words)
        Short = 2,     // Faster detection (good for commands)
        Long = 3,      // Slower detection (good for dictation)
        VeryLong = 4   // Very slow detection (continuous speech)
    };

    explicit VoskRecognizer(QObject* parent = nullptr);
    ~VoskRecognizer() override;

    // SpeechRecognizer interface implementation
    bool initialize(const QString& modelPath) override;
    void setSilenceTimeout(int msec) override;
    int silenceTimeout() const override;
    // Vosk delivers per-word confidence and timing; it has no biasing, and
    // grammar constraint (vosk_recognizer_new_grm) is not wired up yet, so
    // only word results are claimed
    // Word detail is the one thing this engine offers beyond plain text: it
    // cannot be biased, takes no grammar, and decodes on this machine.
    Capabilities capabilities() const override
    {
        Capabilities answer;
        // Only claimed when the library can actually be asked for timings:
        // docs/stt-api.md is explicit that an implementation which cannot
        // supply real ones must not claim this, and a package that waits for
        // sysSTTWords before acting on a result would wait forever.
        answer.wordResults = (s_vosk_recognizer_set_words != nullptr);
        // Same reasoning as wordResults above: an older libvosk without the
        // endpointer symbol cannot be tuned at all, and setEndpointerMode()
        // refuses rather than remembering a request the engine never got. That
        // is a property of the installed library, not of the attempt.
        answer.sensitivityTuning = (s_vosk_recognizer_set_endpointer_mode != nullptr);
        answer.onDevice = true;
        return answer;
    }

    float audioLevel() const override { return listening() ? mRecentAudioLevel : 0.0f; }
    bool hasLiveNativeResources() const override { return mVoskModel || mVoskRecognizer; }
    void releaseResources() override;
    // Both read through the live model handle rather than answering from a
    // remembered string. getInfo() documents modelPath as "the model actually
    // loaded (empty when none)" and a package reads it to decide whether setup
    // has already happened, so where the assignment sits must not be able to
    // make it name a model that failed to load or has just been freed.
    QString modelPath() const override { return mVoskModel ? mModelPath : QString(); }

    QString currentLanguage() const override { return mVoskModel ? mCurrentLanguage : QString(); }
    bool setLanguage(const QString& languageCode) override;

    QString backendName() const override { return qsl("Vosk"); }
    QString backendVersion() const override;

    // SpeechRecognizer sensitivity interface (maps to EndpointerMode)
    bool setSensitivity(Sensitivity sensitivity) override;
    Sensitivity sensitivity() const override;

    // Vosk-specific: Finer-grained control over end-of-speech detection
    bool setEndpointerMode(EndpointerMode mode);
    EndpointerMode endpointerMode() const { return mEndpointerMode; }

    // Whether the Vosk library can be used, loading it on the first ask
    static bool libraryAvailable();

    // Re-read capabilities and emit capabilitiesChanged() if they moved. Called
    // when a model loads, and again when the library is unloaded or reloaded
    // underneath this instance - wordResults follows a symbol from it, so that
    // is the other moment what this backend can do genuinely changes.
    void announceCapabilitiesIfChanged();

    // Whether stt.unloadLibrary() has latched the library out, so a refusal can
    // say that rather than "not installed"
    static bool libraryUnloadedByRequest() { return sLibraryUnloadedByRequest; }

    // Unload the library and forget everything resolved from it, so a later
    // probe starts fresh. False when the module would not unload, which means
    // its file is still mapped and cannot be replaced yet.
    // unloadLibrary() latches "stay unloaded"; reloadLibrary() lifts it. Both
    // go through resetLibraryLoadState(), which does the unmapping itself.
    static bool resetLibraryLoadState();
    static void unloadLibraryByRequest(bool unloaded) { sLibraryUnloadedByRequest = unloaded; }

    static QStringList librarySearchPaths();

    // The user-writable directory the Vosk library can be installed into
    static QString userLibraryPath();

    static QString defaultModelPath();


    // Model selection and management
    // Get the path to the currently selected model (from settings, or auto-detect best available)
    static QString getSelectedModelPath();

    // The model name settings ask for when that model is not installed, empty
    // otherwise. getSelectedModelPath() answers with whatever else is on disk
    // in that case, which keeps speech working but hands a package configured
    // for one language a decoder for another - so the substitution is reported
    // rather than made quietly.
    static QString missingSelectedModel();


    // The directory models are installed into
    static QString modelsDirectoryPath();

    // Directory names, relative to modelsDirectoryPath(), of everything
    // installed there that looks like a Vosk model
    static QStringList getInstalledModels();

    // Get the "best" available model (prefers larger models over smaller ones)
    static QString getBestAvailableModel();

    // Whether the leading word of a result spans enough silence to be a decoder
    // artifact rather than speech, and the word detail for a result whose
    // leading word was struck from the text. Pure functions over what the
    // decoder returned, and public because what they decide is whether a word
    // the player actually said survives - there is no other way to hold them to
    // that without a microphone and a live model.
    static bool leadingWordIsPhantom(const QJsonArray& words);
    static QVariantList wordsFromResult(const QJsonArray& words, bool skipLeading);

    // Reads one of the decoder's result strings into an object. Answers false
    // with a reason when there is nothing to read - a null pointer, or bytes
    // that do not parse into an object. Both used to leave the transcription
    // empty, which is the same path "nothing was said" takes, so an utterance
    // the engine had already accepted vanished with no event and no log line.
    static bool parseEngineResult(const char* json, QJsonObject& result, QString& failureReason);

    // Whether a lone filler word was an artifact rather than something said.
    // Public for the same reason as the two above: it decides whether a word
    // the player said survives, and "i" is a MUD player's inventory command.
    static bool loneFillerWordWasNotSpoken(const QString& text, const std::optional<double>& confidence);

protected:
    // SpeechRecognizer declares these protected: a holder of a concrete
    // VoskRecognizer* must go through startListening()/stopListening()/
    // cancel() like every other caller, not reach around the state machine.
    void doStartListening() override;
    void doStopListening() override;
    void doCancel() override;

private slots:
    // Consumes 16kHz mono Int16 PCM from the shared capture component
    void slot_pcmReady(const QByteArray& pcmData);
    void slot_captureError(const QString& message);

private:
    // Load the Vosk library dynamically. Static: it touches only the shared
    // library handle and function pointers, so no instance is needed to probe.
    static bool loadVoskLibrary();

    void releaseVoskResources();

    // Starts listening once permission to use the microphone is settled
    void startListeningInternal();

    // Audio level for visual feedback
    float calculateAudioLevel(const QByteArray& data) const;


    QString findModelPathForLanguage(const QString& languageCode) const;

    // Report a decoded utterance: strips a leading word the timings say was
    // never spoken, then emits finalResult() and the wordsResult() describing
    // the text as emitted - which is not the text Vosk returned when a word was
    // struck from it. resultObject is the whole JSON object Vosk returned, text
    // its already trimmed "text" field.
    void emitFinalResult(const QJsonObject& resultObject, QString text);
    // parseEngineResult(), with the refusal reported through errorOccurred():
    // docs/stt-api.md allows dropping only what was not spoken, so anything
    // else the caller loses has to speak.
    bool decodedResult(const char* json, QJsonObject& result);

    QString mModelPath;
    QString mCurrentLanguage;

    // Vosk handles (opaque pointers)
    VoskModel* mVoskModel = nullptr;
    VoskRecognizerHandle* mVoskRecognizer = nullptr;

    // Shared microphone capture and resampling; delivers ready-to-decode PCM
    SpeechAudioCapture* mpCapture = nullptr;

    // Track audio level for silence detection (filter hallucinations)
    float mRecentAudioLevel = 0.0f;
    static constexpr float SILENCE_THRESHOLD = 0.01f; // Below this is considered silence

    // Track speech onset to filter initial hallucinations
    int mSpeechOnsetFrames = 0;                   // Frames since speech started
    static constexpr int SPEECH_ONSET_FRAMES = 5; // ~250ms at 50ms timer = require sustained speech
    QString mLastPartialResult;                   // Track last partial to detect stuck hallucinations

    // Vosk library and function pointers (for dynamic loading)
    static QLibrary sVoskLibrary;
    static bool sLibraryLoaded;
    static bool sLibraryLoadAttempted;
    // Set by unloadLibrary(), cleared by reloadLibrary(): while it stands, no
    // read-shaped call may map the library back in behind the caller's back
    static bool sLibraryUnloadedByRequest;

    // What capabilities() last reported, so a change is announced once rather
    // than on every read. wordResults follows a symbol that only resolves when
    // the library loads, so it genuinely changes during initialize().
    Capabilities mAnnouncedCapabilities;

    // Vosk API function pointers
    using vosk_model_new_fn = VoskModel* (*)(const char*);
    using vosk_model_free_fn = void (*)(VoskModel*);
    using vosk_recognizer_new_fn = VoskRecognizerHandle* (*)(VoskModel*, float);
    using vosk_recognizer_free_fn = void (*)(VoskRecognizerHandle*);
    using vosk_recognizer_accept_waveform_fn = int (*)(VoskRecognizerHandle*, const char*, int);
    using vosk_recognizer_result_fn = const char* (*)(VoskRecognizerHandle*);
    using vosk_recognizer_partial_result_fn = const char* (*)(VoskRecognizerHandle*);
    using vosk_recognizer_final_result_fn = const char* (*)(VoskRecognizerHandle*);
    using vosk_recognizer_reset_fn = void (*)(VoskRecognizerHandle*);
    using vosk_set_log_level_fn = void (*)(int);
    using vosk_recognizer_set_endpointer_mode_fn = void (*)(VoskRecognizerHandle*, int);
    using vosk_recognizer_set_words_fn = void (*)(VoskRecognizerHandle*, int);

    static vosk_model_new_fn s_vosk_model_new;
    static vosk_model_free_fn s_vosk_model_free;
    static vosk_recognizer_new_fn s_vosk_recognizer_new;
    static vosk_recognizer_free_fn s_vosk_recognizer_free;
    static vosk_recognizer_accept_waveform_fn s_vosk_recognizer_accept_waveform;
    static vosk_recognizer_result_fn s_vosk_recognizer_result;
    static vosk_recognizer_partial_result_fn s_vosk_recognizer_partial_result;
    static vosk_recognizer_final_result_fn s_vosk_recognizer_final_result;
    static vosk_recognizer_reset_fn s_vosk_recognizer_reset;
    static vosk_set_log_level_fn s_vosk_set_log_level;
    static vosk_recognizer_set_endpointer_mode_fn s_vosk_recognizer_set_endpointer_mode;
    static vosk_recognizer_set_words_fn s_vosk_recognizer_set_words;

    EndpointerMode mEndpointerMode = EndpointerMode::Default;
};

#endif // MUDLET_VOSKRECOGNIZER_H
