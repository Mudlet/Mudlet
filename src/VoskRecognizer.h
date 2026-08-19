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

#include <QLibrary>
#include <QPointer>
#include <QString>
#include <QVariantList>

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
    void startListening() override;
    void stopListening() override;
    void cancel() override;
    void resetUtterance() override;
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
        answer.wordResults = true;
        answer.onDevice = true;
        return answer;
    }

    float audioLevel() const override { return listening() ? mRecentAudioLevel : 0.0f; }
    bool hasLiveNativeResources() const override { return mVoskModel || mVoskRecognizer; }
    void releaseResources() override;
    QString modelPath() const override { return mModelPath; }

    QStringList availableLanguages() const override;
    QString currentLanguage() const override { return mCurrentLanguage; }
    bool setLanguage(const QString& languageCode) override;

    QString backendName() const override { return QStringLiteral("Vosk"); }
    QString backendVersion() const override;
    bool backendAvailable() const override;

    // SpeechRecognizer sensitivity interface (maps to EndpointerMode)
    void setSensitivity(Sensitivity sensitivity) override;
    Sensitivity sensitivity() const override;

    // Vosk-specific: Finer-grained control over end-of-speech detection
    void setEndpointerMode(EndpointerMode mode);
    EndpointerMode endpointerMode() const { return mEndpointerMode; }

    // Whether the Vosk library can be used, loading it on the first ask
    static bool libraryAvailable();

    // Unload the library and forget everything resolved from it, so a later
    // probe starts fresh. False when the module would not unload, which means
    // its file is still mapped and cannot be replaced yet.
    static bool resetLibraryLoadState();

    static QStringList librarySearchPaths();

    // The user-writable directory the Vosk library can be installed into
    static QString userLibraryPath();

    static QString defaultModelPath();

    // Where the model for a language can be downloaded from
    static QString modelDownloadUrl(const QString& languageCode);

    // Model selection and management
    // Get the path to the currently selected model (from settings, or auto-detect best available)
    static QString getSelectedModelPath();

    // Set the selected model path (saves to settings)
    static void setSelectedModelPath(const QString& modelPath);

    // The directory models are installed into
    static QString modelsDirectoryPath();

    // Directory names, relative to modelsDirectoryPath(), of everything
    // installed there that looks like a Vosk model
    static QStringList getInstalledModels();

    // Get the "best" available model (prefers larger models over smaller ones)
    static QString getBestAvailableModel();

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
