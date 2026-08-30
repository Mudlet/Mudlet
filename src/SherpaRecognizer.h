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

#ifndef MUDLET_SHERPARECOGNIZER_H
#define MUDLET_SHERPARECOGNIZER_H

#include "SpeechRecognizer.h"
#include "utils.h"

#include <QLibrary>
#include <QString>

class SpeechAudioCapture;

// Forward declarations for sherpa-onnx C-API types. The recognizer and stream
// handles are opaque everywhere; the config and result structs are populated
// only inside the implementation file, which vendors their layout from the
// pinned sherpa-onnx release.
struct SherpaOnnxOnlineRecognizer;
struct SherpaOnnxOnlineStream;
struct SherpaOnnxOnlineRecognizerConfig;
struct SherpaOnnxOnlineRecognizerResult;

// sherpa-onnx based implementation of SpeechRecognizer
// (https://github.com/k2-fsa/sherpa-onnx). Runs streaming transducer models
// such as NVIDIA's nemotron-speech-streaming and the Zipformer family. The
// library is loaded dynamically at runtime to allow graceful fallback if it
// is not installed.

class SherpaRecognizer : public SpeechRecognizer
{
    Q_OBJECT

public:
    explicit SherpaRecognizer(QObject* parent = nullptr);
    ~SherpaRecognizer() override;

    // SpeechRecognizer interface implementation
    bool initialize(const QString& modelPath) override;
    // Read from the model rather than fixed for the engine: biasing needs the
    // model's own sub-word vocabulary, so the same engine can bias one model
    // and not the next. Gated on mRecognizer, not just on mSupportsBiasing/
    // mBpeVocabPath: those are set once a model directory is found to carry
    // bpe.vocab, before the recognizer is actually created, and releasing the
    // recognizer does not clear them - so without the gate this would go on
    // claiming biasing for a model that is no longer loaded. Announced through
    // capabilitiesChanged() when this answer changes, because a consumer that
    // read them once would otherwise not know.
    Capabilities capabilities() const override
    {
        Capabilities answer;
        answer.biasing = mRecognizer && mSupportsBiasing;
        answer.onDevice = true;
        return answer;
    }
    void doStartListening() override;
    void doStopListening() override;
    void doCancel() override;
    void setSilenceTimeout(int msec) override;
    int silenceTimeout() const override;

    float audioLevel() const override { return listening() ? mRecentAudioLevel : 0.0f; }
    bool hasLiveNativeResources() const override { return mRecognizer || mStream; }
    void releaseResources() override;
    // Gated on the live handle rather than answering from a remembered
    // string, the way VoskRecognizer::modelPath() is and for the same reason
    // (see its comment): a package reads this to decide whether setup already
    // happened, so it must not name a model that failed to load or has just
    // been freed.
    QString modelPath() const override { return mRecognizer ? mModelPath : QString(); }

    QString currentLanguage() const override { return mRecognizer ? mCurrentLanguage : QString(); }
    bool setLanguage(const QString& languageCode) override;

    QString backendName() const override { return qsl("sherpa-onnx"); }
    QString backendVersion() const override;

    // Maps to the endpoint rules baked into the recognizer at model load, so
    // changing it with a model loaded reloads that model
    bool setSensitivity(Sensitivity sensitivity) override;
    Sensitivity sensitivity() const override { return mSensitivity; }

    // Static method to check if the sherpa-onnx library is available on this system
    static bool sherpaAvailable();

    // Reset library load state to allow re-checking (e.g., after installation)
    static bool resetLibraryLoadState();
    static void unloadLibraryByRequest(bool unloaded) { sLibraryUnloadedByRequest = unloaded; }

    // Get the list of paths where the sherpa-onnx library is searched
    static QStringList librarySearchPaths();

    // Get the user-writable directory the sherpa-onnx library can be installed into
    static QString userLibraryPath();

    // Get the base directory where models are stored
    static QString modelsDirectoryPath();

    // Get list of installed models (directory names)
    static QStringList getInstalledModels();

    // Get the default model path for the current platform
    static QString defaultModelPath();

    // Whether the directory holds a sherpa-onnx streaming transducer model
    // (tokens.txt plus encoder/decoder/joiner ONNX files)
    static bool looksLikeModelDir(const QString& modelPath);

protected:
    // Take the retained vocabulary and rebuild the decoder toward it. See the
    // .cpp definition for why this goes through loadModel() rather than
    // initialize().
    VocabularyResult applyVocabulary(const QStringList& words) override;

private slots:
    // Consumes 16kHz mono Int16 PCM from the shared capture component
    void slot_pcmReady(const QByteArray& pcmData);
    void slot_captureError(const QString& message);

private:
    // Load the sherpa-onnx library dynamically. Static: it touches only the
    // shared library handle and function pointers, so no instance is needed to probe.
    static bool loadSherpaLibrary();

    // The entire job of loading (or reloading) a model, including baking in
    // vocabulary() for whatever bias support the model turns out to have.
    // initialize() wraps this with a vocabulary-applied bookkeeping fix that
    // applyVocabulary() would only have to redo (its own caller,
    // setVocabulary(), does that from applyVocabulary()'s return value), so
    // applyVocabulary() calls this directly instead of initialize().
    bool loadModel(const QString& modelPath);

    // Release sherpa-onnx resources
    void releaseSherpaResources();
    void destroyStream();

    // Internal method that actually starts listening (called after permission check)
    void startListeningInternal();

    // Calculate audio level for visual feedback
    float calculateAudioLevel(const QByteArray& data) const;

    // Find an installed model path for a given language code
    QString findModelPathForLanguage(const QString& languageCode) const;

    // Re-read capabilities() and emit capabilitiesChanged() only when the
    // answer actually moved, the way VoskRecognizer::announceCapabilitiesIfChanged()
    // does: a consumer told docs/stt-api.md's promise to re-read rather than
    // cache capabilities should not also see a signal fire for no change.
    void announceCapabilitiesIfChanged();

    // Member variables
    QString mModelPath;
    QString mCurrentLanguage;
    Sensitivity mSensitivity = Sensitivity::Default;
    QString mLastPartialResult;

    // Words to bias recognition toward, and the model's sub-word vocabulary
    // they are tokenised with. Both are needed before biasing can be claimed.
    // Reset on release alongside mRecognizer - capabilities() also gates on
    // mRecognizer, so this and that are kept redundant on purpose rather than
    // relying on the gate alone.
    QString mBpeVocabPath;
    bool mSupportsBiasing = false;
    // Whether this model's units are written in upper case, which decides the
    // case biasing words have to be given in to match them
    bool mUppercaseTokens = false;

    // What capabilities() last reported, so announceCapabilitiesIfChanged()
    // can tell a real change from a re-read of the same answer
    Capabilities mAnnouncedCapabilities;

    // Consecutive silent audio chunks, used to tell a genuine lull from the
    // moment speech is starting. Chunks arrive every 50ms.
    int mSilentChunks = 0;
    // Smoothed microphone level, reported so a consumer can see how well the
    // speech arrived rather than only what was made of it
    float mRecentAudioLevel = 0.0f;
    static constexpr float SILENCE_LEVEL = 0.01f;
    // A second of continuous silence before the decoder may be reset outside
    // of finishing an utterance: long enough that a phrase getting under way
    // has already registered and can block it.
    static constexpr int SILENT_CHUNKS_BEFORE_IDLE_RESET = 20;

    // sherpa-onnx handles (opaque pointers)
    const SherpaOnnxOnlineRecognizer* mRecognizer = nullptr;
    const SherpaOnnxOnlineStream* mStream = nullptr;

    // Shared microphone capture and resampling; delivers ready-to-decode PCM
    SpeechAudioCapture* mpCapture = nullptr;

    // sherpa-onnx library and function pointers (for dynamic loading)
    static QLibrary sSherpaLibrary;
    static bool sLibraryLoaded;
    static bool sLibraryLoadAttempted;
    // Set by unloadLibrary(), cleared by reloadLibrary(): while it stands, no
    // read-shaped call may map the library back in behind the caller's back
    static bool sLibraryUnloadedByRequest;

    // sherpa-onnx C API function pointers
    using create_recognizer_fn = const SherpaOnnxOnlineRecognizer* (*)(const SherpaOnnxOnlineRecognizerConfig*);
    using destroy_recognizer_fn = void (*)(const SherpaOnnxOnlineRecognizer*);
    using create_stream_fn = const SherpaOnnxOnlineStream* (*)(const SherpaOnnxOnlineRecognizer*);
    using destroy_stream_fn = void (*)(const SherpaOnnxOnlineStream*);
    using accept_waveform_fn = void (*)(const SherpaOnnxOnlineStream*, qint32, const float*, qint32);
    using is_ready_fn = qint32 (*)(const SherpaOnnxOnlineRecognizer*, const SherpaOnnxOnlineStream*);
    using decode_stream_fn = void (*)(const SherpaOnnxOnlineRecognizer*, const SherpaOnnxOnlineStream*);
    using get_result_fn = const SherpaOnnxOnlineRecognizerResult* (*)(const SherpaOnnxOnlineRecognizer*, const SherpaOnnxOnlineStream*);
    using destroy_result_fn = void (*)(const SherpaOnnxOnlineRecognizerResult*);
    using is_endpoint_fn = qint32 (*)(const SherpaOnnxOnlineRecognizer*, const SherpaOnnxOnlineStream*);
    using stream_reset_fn = void (*)(const SherpaOnnxOnlineRecognizer*, const SherpaOnnxOnlineStream*);
    using input_finished_fn = void (*)(const SherpaOnnxOnlineStream*);
    using get_version_fn = const char* (*)();

    static create_recognizer_fn s_createOnlineRecognizer;
    static destroy_recognizer_fn s_destroyOnlineRecognizer;
    static create_stream_fn s_createOnlineStream;
    static destroy_stream_fn s_destroyOnlineStream;
    static accept_waveform_fn s_onlineStreamAcceptWaveform;
    static is_ready_fn s_isOnlineStreamReady;
    static decode_stream_fn s_decodeOnlineStream;
    static get_result_fn s_getOnlineStreamResult;
    static destroy_result_fn s_destroyOnlineRecognizerResult;
    static is_endpoint_fn s_onlineStreamIsEndpoint;
    static stream_reset_fn s_onlineStreamReset;
    static input_finished_fn s_onlineStreamInputFinished;
    static get_version_fn s_getVersionStr;
};

#endif // MUDLET_SHERPARECOGNIZER_H
