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

#include "SherpaRecognizer.h"

#include "SpeechAudioCapture.h"
#include "mudlet.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QTextStream>
#include <QVarLengthArray>
#include <QtMath>

#if defined(Q_OS_MACOS)
#include "MacMicrophonePermission.h"
#endif

// Vendored from sherpa-onnx v1.13.5 c-api.h. These layouts are the ABI the
// loaded library reads, so field order and types must not change; sherpa-onnx
// itself only ever appends fields, which initialize() defends against by
// passing the struct inside a larger zeroed block.

struct SherpaOnnxOnlineTransducerModelConfig
{
    const char* encoder;
    const char* decoder;
    const char* joiner;
};

struct SherpaOnnxOnlineParaformerModelConfig
{
    const char* encoder;
    const char* decoder;
};

struct SherpaOnnxOnlineZipformer2CtcModelConfig
{
    const char* model;
};

struct SherpaOnnxOnlineNemoCtcModelConfig
{
    const char* model;
};

struct SherpaOnnxOnlineToneCtcModelConfig
{
    const char* model;
};

struct SherpaOnnxOnlineModelConfig
{
    SherpaOnnxOnlineTransducerModelConfig transducer;
    SherpaOnnxOnlineParaformerModelConfig paraformer;
    SherpaOnnxOnlineZipformer2CtcModelConfig zipformer2_ctc;
    const char* tokens;
    qint32 num_threads;
    const char* provider;
    qint32 debug;
    const char* model_type;
    const char* modeling_unit;
    const char* bpe_vocab;
    const char* tokens_buf;
    qint32 tokens_buf_size;
    SherpaOnnxOnlineNemoCtcModelConfig nemo_ctc;
    SherpaOnnxOnlineToneCtcModelConfig t_one_ctc;
};

struct SherpaOnnxFeatureConfig
{
    qint32 sample_rate;
    qint32 feature_dim;
};

struct SherpaOnnxOnlineCtcFstDecoderConfig
{
    const char* graph;
    qint32 max_active;
};

struct SherpaOnnxHomophoneReplacerConfig
{
    const char* dict_dir;
    const char* lexicon;
    const char* rule_fsts;
};

struct SherpaOnnxOnlineRecognizerConfig
{
    SherpaOnnxFeatureConfig feat_config;
    SherpaOnnxOnlineModelConfig model_config;
    const char* decoding_method;
    qint32 max_active_paths;
    qint32 enable_endpoint;
    float rule1_min_trailing_silence;
    float rule2_min_trailing_silence;
    float rule3_min_utterance_length;
    const char* hotwords_file;
    float hotwords_score;
    SherpaOnnxOnlineCtcFstDecoderConfig ctc_fst_decoder_config;
    const char* rule_fsts;
    const char* rule_fars;
    float blank_penalty;
    const char* hotwords_buf;
    qint32 hotwords_buf_size;
    SherpaOnnxHomophoneReplacerConfig hr;
};

struct SherpaOnnxOnlineRecognizerResult
{
    const char* text;
    const char* tokens;
    const char* const* tokens_arr;
    float* timestamps;
    qint32 count;
    const char* json;
};

// Static member initialization
QLibrary SherpaRecognizer::sSherpaLibrary;
bool SherpaRecognizer::sLibraryLoaded = false;
bool SherpaRecognizer::sLibraryLoadAttempted = false;
bool SherpaRecognizer::sLibraryUnloadedByRequest = false;

SherpaRecognizer::create_recognizer_fn SherpaRecognizer::s_createOnlineRecognizer = nullptr;
SherpaRecognizer::destroy_recognizer_fn SherpaRecognizer::s_destroyOnlineRecognizer = nullptr;
SherpaRecognizer::create_stream_fn SherpaRecognizer::s_createOnlineStream = nullptr;
SherpaRecognizer::destroy_stream_fn SherpaRecognizer::s_destroyOnlineStream = nullptr;
SherpaRecognizer::accept_waveform_fn SherpaRecognizer::s_onlineStreamAcceptWaveform = nullptr;
SherpaRecognizer::is_ready_fn SherpaRecognizer::s_isOnlineStreamReady = nullptr;
SherpaRecognizer::decode_stream_fn SherpaRecognizer::s_decodeOnlineStream = nullptr;
SherpaRecognizer::get_result_fn SherpaRecognizer::s_getOnlineStreamResult = nullptr;
SherpaRecognizer::destroy_result_fn SherpaRecognizer::s_destroyOnlineRecognizerResult = nullptr;
SherpaRecognizer::is_endpoint_fn SherpaRecognizer::s_onlineStreamIsEndpoint = nullptr;
SherpaRecognizer::stream_reset_fn SherpaRecognizer::s_onlineStreamReset = nullptr;
SherpaRecognizer::input_finished_fn SherpaRecognizer::s_onlineStreamInputFinished = nullptr;
SherpaRecognizer::get_version_fn SherpaRecognizer::s_getVersionStr = nullptr;

SherpaRecognizer::SherpaRecognizer(QObject* parent)
: SpeechRecognizer(parent)
, mpCapture(new SpeechAudioCapture(this))
{
    connect(mpCapture, &SpeechAudioCapture::pcm, this, &SherpaRecognizer::slot_pcmReady);
    connect(mpCapture, &SpeechAudioCapture::captureError, this, &SherpaRecognizer::slot_captureError);
    // A silence timeout ends the utterance the way the user stopping would:
    // finalise and report, never discard
    connect(mpCapture, &SpeechAudioCapture::silenceTimedOut, this, &SherpaRecognizer::stopListening);
}

SherpaRecognizer::~SherpaRecognizer()
{
    // cancel() ends with setState(), which emits stateChanged(). Connections are
    // still live until ~QObject runs, so that would deliver a state change from a
    // half-destroyed object to slots that go on to query it.
    blockSignals(true);
    cancel();
    releaseSherpaResources();
}

void SherpaRecognizer::setSilenceTimeout(int msec)
{
    mpCapture->setSilenceTimeout(msec);
}

int SherpaRecognizer::silenceTimeout() const
{
    return mpCapture->silenceTimeout();
}

// The sherpa-onnx C-API library depends on onnxruntime, which release
// bundles place beside it. Loading those first lets the dynamic linker
// satisfy the dependency from a directory it would not search on its own.
static void preloadBundledDependencies(const QString& directory)
{
    const QDir dir(directory);
    const QStringList dependencies = dir.entryList({qsl("libonnxruntime*"), qsl("onnxruntime*")}, QDir::Files);
    for (const QString& dependency : dependencies) {
        QLibrary library(dir.filePath(dependency));
        // Exported globally so the c-api library's own linkage resolves
        // against the preloaded image; ~QLibrary does not unload, so the
        // mapping outlives this scope
        library.setLoadHints(QLibrary::ExportExternalSymbolsHint);
        library.load();
    }
}

bool SherpaRecognizer::loadSherpaLibrary()
{
    if (sLibraryLoadAttempted) {
        return sLibraryLoaded;
    }

    sLibraryLoadAttempted = true;

    // QLibrary automatically adds platform-specific prefix/suffix:
    // "sherpa-onnx-c-api" becomes libsherpa-onnx-c-api.dylib / .so, or
    // sherpa-onnx-c-api.dll on Windows
    sSherpaLibrary.setFileName(qsl("sherpa-onnx-c-api"));

    if (!sSherpaLibrary.load()) {
        // Try common installation paths
        for (const QString& path : librarySearchPaths()) {
            preloadBundledDependencies(QFileInfo(path).absolutePath());
            sSherpaLibrary.setFileName(path);
            if (sSherpaLibrary.load()) {
                break;
            }
        }
    }

    if (!sSherpaLibrary.isLoaded()) {
        qWarning() << "SherpaRecognizer: Failed to load sherpa-onnx library:" << sSherpaLibrary.errorString();
        return false;
    }

    // Resolve function pointers
    s_createOnlineRecognizer = reinterpret_cast<create_recognizer_fn>(sSherpaLibrary.resolve("SherpaOnnxCreateOnlineRecognizer"));
    s_destroyOnlineRecognizer = reinterpret_cast<destroy_recognizer_fn>(sSherpaLibrary.resolve("SherpaOnnxDestroyOnlineRecognizer"));
    s_createOnlineStream = reinterpret_cast<create_stream_fn>(sSherpaLibrary.resolve("SherpaOnnxCreateOnlineStream"));
    s_destroyOnlineStream = reinterpret_cast<destroy_stream_fn>(sSherpaLibrary.resolve("SherpaOnnxDestroyOnlineStream"));
    s_onlineStreamAcceptWaveform = reinterpret_cast<accept_waveform_fn>(sSherpaLibrary.resolve("SherpaOnnxOnlineStreamAcceptWaveform"));
    s_isOnlineStreamReady = reinterpret_cast<is_ready_fn>(sSherpaLibrary.resolve("SherpaOnnxIsOnlineStreamReady"));
    s_decodeOnlineStream = reinterpret_cast<decode_stream_fn>(sSherpaLibrary.resolve("SherpaOnnxDecodeOnlineStream"));
    s_getOnlineStreamResult = reinterpret_cast<get_result_fn>(sSherpaLibrary.resolve("SherpaOnnxGetOnlineStreamResult"));
    s_destroyOnlineRecognizerResult = reinterpret_cast<destroy_result_fn>(sSherpaLibrary.resolve("SherpaOnnxDestroyOnlineRecognizerResult"));
    s_onlineStreamIsEndpoint = reinterpret_cast<is_endpoint_fn>(sSherpaLibrary.resolve("SherpaOnnxOnlineStreamIsEndpoint"));
    s_onlineStreamReset = reinterpret_cast<stream_reset_fn>(sSherpaLibrary.resolve("SherpaOnnxOnlineStreamReset"));
    s_onlineStreamInputFinished = reinterpret_cast<input_finished_fn>(sSherpaLibrary.resolve("SherpaOnnxOnlineStreamInputFinished"));

    // Optional: only used for diagnostics
    s_getVersionStr = reinterpret_cast<get_version_fn>(sSherpaLibrary.resolve("SherpaOnnxGetVersionStr"));

    if (!s_createOnlineRecognizer || !s_destroyOnlineRecognizer || !s_createOnlineStream || !s_destroyOnlineStream || !s_onlineStreamAcceptWaveform || !s_isOnlineStreamReady || !s_decodeOnlineStream
        || !s_getOnlineStreamResult || !s_destroyOnlineRecognizerResult || !s_onlineStreamIsEndpoint || !s_onlineStreamReset || !s_onlineStreamInputFinished) {
        qWarning() << "SherpaRecognizer: Failed to resolve required sherpa-onnx functions";
        // Unloading on its own would leave the pointers that did resolve aiming
        // into a library that is no longer mapped
        resetLibraryLoadState();
        // resetLibraryLoadState() clears this to allow a fresh probe; a library
        // whose symbols are missing is not worth re-probing on every call
        sLibraryLoadAttempted = true;
        return false;
    }

    sLibraryLoaded = true;

    if (s_getVersionStr) {
        qInfo().noquote() << "SherpaRecognizer: Loaded sherpa-onnx" << QString::fromUtf8(s_getVersionStr());
    }

    return true;
}

bool SherpaRecognizer::sherpaAvailable()
{
    // Not probed again while a caller has deliberately unloaded it - the same
    // rule the Vosk backend follows, and for the same reason: otherwise
    // stt.unloadLibrary() is undone by the next call that only looks like a
    // read, and on Windows the delete it existed to permit fails on a mapped
    // module. reloadLibrary() is what clears this.
    if (!sLibraryLoadAttempted && !sLibraryUnloadedByRequest) {
        loadSherpaLibrary();
    }
    return sLibraryLoaded;
}

bool SherpaRecognizer::resetLibraryLoadState()
{
    // Whether the module actually went. QLibrary::unload() refuses while
    // anything still holds it mapped, and reporting success then is how a
    // caller comes to delete a file the loader has not let go of.
    if (sSherpaLibrary.isLoaded() && !sSherpaLibrary.unload()) {
        return false;
    }

    sLibraryLoaded = false;
    sLibraryLoadAttempted = false;

    s_createOnlineRecognizer = nullptr;
    s_destroyOnlineRecognizer = nullptr;
    s_createOnlineStream = nullptr;
    s_destroyOnlineStream = nullptr;
    s_onlineStreamAcceptWaveform = nullptr;
    s_isOnlineStreamReady = nullptr;
    s_decodeOnlineStream = nullptr;
    s_getOnlineStreamResult = nullptr;
    s_destroyOnlineRecognizerResult = nullptr;
    s_onlineStreamIsEndpoint = nullptr;
    s_onlineStreamReset = nullptr;
    s_onlineStreamInputFinished = nullptr;
    s_getVersionStr = nullptr;
    return true;
}

QString SherpaRecognizer::userLibraryPath()
{
    return mudlet::getMudletPath(enums::mainDataItemPath, qsl("sherpa-onnx-lib"));
}

QStringList SherpaRecognizer::librarySearchPaths()
{
    QStringList paths;

#if defined(Q_OS_MACOS)
    paths << QDir(userLibraryPath()).filePath(qsl("libsherpa-onnx-c-api.dylib"));
    paths << qsl("/usr/local/lib/libsherpa-onnx-c-api.dylib") << qsl("/opt/homebrew/lib/libsherpa-onnx-c-api.dylib")
          << QCoreApplication::applicationDirPath() + qsl("/../Frameworks/libsherpa-onnx-c-api.dylib");
#elif defined(Q_OS_WIN)
    paths << QDir(userLibraryPath()).filePath(qsl("sherpa-onnx-c-api.dll"));
    paths << QCoreApplication::applicationDirPath() + qsl("/sherpa-onnx-c-api.dll");
#else
    paths << QDir(userLibraryPath()).filePath(qsl("libsherpa-onnx-c-api.so"));
    paths << qsl("/usr/lib/libsherpa-onnx-c-api.so") << qsl("/usr/local/lib/libsherpa-onnx-c-api.so") << qsl("/usr/lib/x86_64-linux-gnu/libsherpa-onnx-c-api.so");
#endif

    return paths;
}

QString SherpaRecognizer::backendVersion() const
{
    if (sLibraryLoaded && s_getVersionStr) {
        return QString::fromUtf8(s_getVersionStr());
    }
    return QString();
}

// Whether a model writes its units in upper case. English sub-word models
// trained on upper-cased text do, and a biasing word given in the other case
// tokenises into something the decoder never scores, so the bias silently
// does nothing. Decided from the model's own token list rather than assumed.
static bool tokensAreUppercase(const QString& tokensPath)
{
    QFile tokensFile(tokensPath);
    if (!tokensFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    int upper = 0;
    int lower = 0;
    QTextStream stream(&tokensFile);
    while (!stream.atEnd() && (upper + lower) < 500) {
        const QString line = stream.readLine();
        for (const QChar character : line) {
            if (character.isUpper()) {
                ++upper;
            } else if (character.isLower()) {
                ++lower;
            }
        }
    }

    return upper > lower;
}

bool SherpaRecognizer::loadModel(const QString& modelPath)
{
    if (!loadSherpaLibrary()) {
        //: Shown when speech recognition is asked to load a model but the recognition library itself is not installed
        emit errorOccurred(tr("sherpa-onnx library not available"));
        setState(State::Error);
        return false;
    }

    releaseSherpaResources();

    const QDir modelDir(modelPath);
    if (!modelDir.exists()) {
        //: Shown when a speech model cannot be found; %1 is the folder that was looked for
        emit errorOccurred(tr("Model path does not exist: %1").arg(modelPath));
        setState(State::Error);
        return false;
    }

    // Locate the transducer files. Quantised weights are preferred when a
    // model ships both, matching what the published packages recommend.
    auto pickModelFile = [&modelDir](const QString& stem) -> QString {
        const QStringList candidates = modelDir.entryList({stem + qsl("*.onnx")}, QDir::Files, QDir::Name);
        for (const QString& candidate : candidates) {
            if (candidate.contains(QLatin1String("int8"))) {
                return modelDir.filePath(candidate);
            }
        }
        return candidates.isEmpty() ? QString() : modelDir.filePath(candidates.first());
    };

    const QString encoderPath = pickModelFile(qsl("encoder"));
    const QString decoderPath = pickModelFile(qsl("decoder"));
    const QString joinerPath = pickModelFile(qsl("joiner"));
    const QString tokensPath = modelDir.exists(qsl("tokens.txt")) ? modelDir.filePath(qsl("tokens.txt")) : QString();

    if (encoderPath.isEmpty() || decoderPath.isEmpty() || joinerPath.isEmpty() || tokensPath.isEmpty()) {
        //: Shown when a model directory exists but does not contain the files a sherpa-onnx streaming model needs; %1 is that directory
        emit errorOccurred(tr("Not a sherpa-onnx streaming model (needs tokens.txt and encoder/decoder/joiner .onnx files): %1").arg(modelPath));
        setState(State::Error);
        return false;
    }

    mModelPath = modelPath;
    // Hotword biasing works by scoring the model's own sub-word units, so it
    // needs the vocabulary those units come from - specifically bpe.vocab,
    // the scored text listing, not the bpe.model the tokeniser itself uses.
    // Published model packages carry the latter and not the former, so a pack
    // that wants biasing has to derive it; without it the capability is
    // reported as absent rather than accepting words that would be ignored.
    mBpeVocabPath = modelDir.exists(qsl("bpe.vocab")) ? modelDir.filePath(qsl("bpe.vocab")) : QString();
    mSupportsBiasing = !mBpeVocabPath.isEmpty();
    if (!mSupportsBiasing && modelDir.exists(qsl("bpe.model"))) {
        qInfo().noquote() << "SherpaRecognizer: model has bpe.model but no bpe.vocab, so recognition cannot be biased toward a vocabulary;"
                          << "generate bpe.vocab from bpe.model (piece and score per line) to enable it";
    }
    mUppercaseTokens = tokensAreUppercase(tokensPath);

    qInfo().noquote() << "SherpaRecognizer: Loading model from:" << modelPath;

    // The config crosses the ABI boundary by pointer. A library newer than the
    // vendored 1.13.5 layout may read fields appended after it, so the struct
    // sits at the front of a larger zeroed block and any such field reads as
    // zero, which sherpa-onnx replaces with its own default.
    alignas(std::max_align_t) char configBlock[4096] = {};
    static_assert(sizeof(SherpaOnnxOnlineRecognizerConfig) <= sizeof(configBlock));
    auto* config = reinterpret_cast<SherpaOnnxOnlineRecognizerConfig*>(configBlock);

    const QByteArray encoderUtf8 = encoderPath.toUtf8();
    const QByteArray decoderUtf8 = decoderPath.toUtf8();
    const QByteArray joinerUtf8 = joinerPath.toUtf8();
    const QByteArray tokensUtf8 = tokensPath.toUtf8();

    config->model_config.transducer.encoder = encoderUtf8.constData();
    config->model_config.transducer.decoder = decoderUtf8.constData();
    config->model_config.transducer.joiner = joinerUtf8.constData();
    config->model_config.tokens = tokensUtf8.constData();
    config->model_config.num_threads = 2;
    config->enable_endpoint = 1;

    // Endpoint rules by sensitivity; Default leaves the zeroes in place so the
    // library defaults (2.4s / 1.2s / 20s) apply. Rule 1 ends an utterance
    // after silence with no speech decoded, rule 2 after silence following
    // speech, rule 3 caps utterance length.
    //
    // Only the timing is varied. A blank penalty was tried here to stop quiet
    // onsets being lost, and measurement went against it: the phrase that
    // exposed it came back with an added substitution ("where" for "wear")
    // that the unpenalised decoder got right. Discouraging blanks buys
    // insertions, so it stays off until something measures otherwise.
    switch (mSensitivity) {
    case Sensitivity::Short:
        config->rule1_min_trailing_silence = 1.0f;
        config->rule2_min_trailing_silence = 0.6f;
        config->rule3_min_utterance_length = 15.0f;
        break;
    case Sensitivity::Long:
        config->rule1_min_trailing_silence = 3.6f;
        config->rule2_min_trailing_silence = 2.0f;
        config->rule3_min_utterance_length = 30.0f;
        break;
    case Sensitivity::Default:
        break;
    }

    // Everything left zeroed takes the library default: 16kHz 80-dim
    // features, greedy_search decoding, CPU provider

    // Biasing, when there is both a model that can do it and words to bias
    // toward. Hotwords are only honoured by modified beam search - greedy
    // decoding has no alternative paths to reweight - so asking for one means
    // asking for the other. The buffers must outlive the create call below,
    // which copies out of them.
    const QByteArray bpeVocabUtf8 = mBpeVocabPath.toUtf8();
    QStringList biasWords = vocabulary();
    if (mUppercaseTokens) {
        for (QString& word : biasWords) {
            word = word.toUpper();
        }
    }
    const QByteArray hotwordsUtf8 = biasWords.join(QLatin1Char('\n')).toUtf8();

    if (mSupportsBiasing) {
        config->model_config.modeling_unit = "bpe";
        config->model_config.bpe_vocab = bpeVocabUtf8.constData();

        if (!vocabulary().isEmpty()) {
            config->decoding_method = "modified_beam_search";
            config->hotwords_buf = hotwordsUtf8.constData();
            config->hotwords_buf_size = hotwordsUtf8.size();
            // Zero is a real score, not an unset one. The config block is
            // zeroed so that a newer library's appended fields read as zero and
            // it substitutes its own defaults - which works for the pointers,
            // where null means "unset", and not for this float, where the
            // library honours the zero and boosts every hotword by nothing.
            // Measured: 300 words applied, and recognition byte-identical to
            // no biasing at all, down to the same mishearings.
            //
            // 1.5 is the value sherpa-onnx uses throughout its own examples and
            // command line, so it is their number rather than one invented here.
            config->hotwords_score = 1.5f;
            qInfo().noquote() << "SherpaRecognizer: biasing toward" << vocabulary().size() << "words";
        }
    }

    mRecognizer = s_createOnlineRecognizer(config);
    if (!mRecognizer) {
        //: Shown when a speech model folder exists but could not be loaded; %1 is that folder
        emit errorOccurred(tr("Failed to load sherpa-onnx model from: %1").arg(modelPath));
        setState(State::Error);
        return false;
    }

    // Try to determine language from model path (convention: sherpa-onnx-nemotron-speech-streaming-en-0.6b-...)
    const QString dirName = modelDir.dirName();
    if (dirName.contains(QLatin1String("-en-")) || dirName.contains(QLatin1String("-en_"))) {
        mCurrentLanguage = qsl("en-US");
    } else if (dirName.contains(QLatin1String("-de-"))) {
        mCurrentLanguage = qsl("de-DE");
    } else if (dirName.contains(QLatin1String("-fr-"))) {
        mCurrentLanguage = qsl("fr-FR");
    } else if (dirName.contains(QLatin1String("-es-"))) {
        mCurrentLanguage = qsl("es-ES");
    } else {
        mCurrentLanguage = qsl("unknown");
    }

    // Whether this engine can bias was just decided by the model that loaded,
    // so anyone who read the capabilities before now may be holding a stale
    // answer - announced only if this one actually differs from it
    announceCapabilitiesIfChanged();

    setState(State::Ready);
    return true;
}

void SherpaRecognizer::announceCapabilitiesIfChanged()
{
    if (const Capabilities current = capabilities(); !(current == mAnnouncedCapabilities)) {
        mAnnouncedCapabilities = current;
        emit capabilitiesChanged(current);
    }
}

bool SherpaRecognizer::initialize(const QString& modelPath)
{
    if (!loadModel(modelPath)) {
        return false;
    }

    // vocabulary() may hold words offered before this model could bias them,
    // or while a different model was loaded. loadModel() above already baked
    // them into this model's config using whatever it found - so there is
    // nothing left to apply, only mVocabularyApplied's bookkeeping to correct.
    // noteVocabularyApplied() records that directly rather than going through
    // setVocabulary(vocabulary()): that call would reach applyVocabulary(),
    // which reloads the model to bias it - a second, byte-identical load of
    // what loadModel() just built, for a multi-hundred-MB model paid on every
    // stt.init() that holds a vocabulary. clearAppliedVocabulary() otherwise:
    // this model cannot bias, so whatever the flag said about the previous one
    // no longer applies, and a later identical offer must not be told Applied
    // against a model that never received it.
    if (mSupportsBiasing) {
        noteVocabularyApplied();
    } else {
        clearAppliedVocabulary();
    }

    return state() == State::Ready;
}

void SherpaRecognizer::doStartListening()
{
    // Check microphone permission on macOS using native API
    // Qt's permission API requires proper app signing with entitlements,
    // which development builds don't have, so we use AVFoundation directly.
#if defined(Q_OS_MACOS)
    auto status = MacMicrophonePermission::checkStatus();

    switch (status) {
    case MacMicrophonePermission::AuthorizationStatus::NotDetermined: {
        // requestAccess() dispatches its callback to the main queue, so this runs
        // on the main thread already. Use QPointer to safely handle the case where
        // SherpaRecognizer is destroyed before the permission callback arrives.
        //
        // Starting first, so SpeechRecognizer::startListening() refuses a
        // second request while the player is still looking at the first one -
        // two dialogs, then two callbacks, the later of which would rebuild
        // the recognizer and restart capture underneath the earlier.
        setState(State::Starting);
        QPointer<SherpaRecognizer> weakThis = this;
        MacMicrophonePermission::requestAccess([weakThis](bool granted) {
            if (!weakThis) {
                return; // SherpaRecognizer was destroyed
            }
            // The player may be looking at the permission dialog for a long
            // time, and a script can close or re-initialise the recognizer
            // while they are. Either leaves this request answering for a
            // session nobody is waiting on any more, so granting it would open
            // the microphone with no start behind it. Only a recognizer still
            // waiting on this very request is still Starting - cancel() called
            // while Starting is handled entirely by the base class and never
            // reaches doCancel(), so this is the only place that finds out.
            if (weakThis->state() != State::Starting) {
                return;
            }
            if (granted) {
                weakThis->startListeningInternal();
            } else {
                qWarning() << "SherpaRecognizer: Microphone permission denied by user";
                // SherpaRecognizer::tr, not QObject::tr: the lambda is not a member, and
                // the default context would file this identical string a second
                // time for translators to translate twice
                //: Shown when the player refuses Mudlet access to the microphone; the path names the macOS setting that grants it
                emit weakThis->errorOccurred(SherpaRecognizer::tr("Microphone permission denied. Please grant microphone access in System Settings > Privacy & Security > Microphone."));
                weakThis->setState(State::Error);
            }
        });
        return;
    }
    case MacMicrophonePermission::AuthorizationStatus::Denied:
    case MacMicrophonePermission::AuthorizationStatus::Restricted:
        qWarning() << "SherpaRecognizer: Microphone permission denied or restricted";
        //: Shown when microphone access was refused earlier and has to be granted in system settings before speech will work
        emit errorOccurred(tr("Microphone permission denied. Please grant microphone access in System Settings > Privacy & Security > Microphone."));
        // The same state a denial reaches when the dialog is answered now, as
        // docs/stt-api.md requires: a package driving its controls from state
        // would otherwise keep offering to listen on a machine that cannot
        setState(State::Error);
        return;
    case MacMicrophonePermission::AuthorizationStatus::Authorized:
        break;
    }
#endif

    startListeningInternal();
}

void SherpaRecognizer::startListeningInternal()
{
    if (!mRecognizer) {
        //: Shown when speech recognition could not be prepared for listening
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        setState(State::Error);
        return;
    }

    // A fresh stream per session: the stream is the decoding state, so this is
    // what guarantees no residue from the previous session
    destroyStream();
    mStream = s_createOnlineStream(mRecognizer);
    if (!mStream) {
        qWarning() << "SherpaRecognizer: Failed to create recognition stream";
        //: Shown when speech recognition could not be prepared for listening
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        setState(State::Error);
        return;
    }

    mLastPartialResult.clear();
    mSilentChunks = 0;
    mRecentAudioLevel = 0.0f;

    // mpCapture emits its own translated captureError before returning false,
    // which slot_captureError() has already turned into errorOccurred - only
    // the state transition is left to do here
    if (!mpCapture->start()) {
        destroyStream();
        setState(State::Error);
        return;
    }

    setState(State::Listening);
}

void SherpaRecognizer::doStopListening()
{
    setState(State::Processing);

    mpCapture->stop();

    if (mRecognizer && mStream) {
        // Flush: signal end-of-input, then decode whatever is buffered
        s_onlineStreamInputFinished(mStream);
        while (s_isOnlineStreamReady(mRecognizer, mStream)) {
            s_decodeOnlineStream(mRecognizer, mStream);
        }

        const SherpaOnnxOnlineRecognizerResult* result = s_getOnlineStreamResult(mRecognizer, mStream);
        if (result) {
            const QString text = result->text ? QString::fromUtf8(result->text).trimmed() : QString();
            if (!text.isEmpty()) {
                emit finalResult(text);
            }
            s_destroyOnlineRecognizerResult(result);
        }
    }

    destroyStream();

    // Only if this call still owns the session. setState(Processing) above
    // reaches Lua synchronously, so a handler for that state change can have
    // closed the recognizer while this call was inside the decoder; saying
    // Ready on top of that would claim a loaded model that has been freed.
    if (state() == State::Processing) {
        setState(State::Ready);
    }
}

void SherpaRecognizer::doCancel()
{
    // Stop audio capture and drop the stream without processing the remainder
    mpCapture->stop();
    destroyStream();

    setState(State::Ready);
}

void SherpaRecognizer::slot_pcmReady(const QByteArray& pcmData)
{
    if (!mRecognizer || !mStream || pcmData.isEmpty()) {
        return;
    }

    const float level = calculateAudioLevel(pcmData);

    // Smoothed the way the silence detection is, so the reported level
    // reflects the phrase rather than whichever 50ms chunk was last seen
    mRecentAudioLevel = mRecentAudioLevel * 0.7f + level * 0.3f;

    if (level < SILENCE_LEVEL) {
        ++mSilentChunks;
    } else {
        mSilentChunks = 0;
    }

    // sherpa-onnx consumes mono float samples in [-1, 1]
    const auto* samples = reinterpret_cast<const qint16*>(pcmData.constData());
    const int numSamples = pcmData.size() / static_cast<int>(sizeof(qint16));
    if (numSamples == 0) {
        return;
    }

    QVarLengthArray<float, 4096> floatSamples(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        floatSamples[i] = static_cast<float>(samples[i]) / 32768.0f;
    }

    s_onlineStreamAcceptWaveform(mStream, SpeechAudioCapture::scmSampleRate, floatSamples.constData(), numSamples);
    while (s_isOnlineStreamReady(mRecognizer, mStream)) {
        s_decodeOnlineStream(mRecognizer, mStream);
    }

    const SherpaOnnxOnlineRecognizerResult* result = s_getOnlineStreamResult(mRecognizer, mStream);
    QString text;
    if (result) {
        text = result->text ? QString::fromUtf8(result->text).trimmed() : QString();
        s_destroyOnlineRecognizerResult(result);
    }

    // The endpointer trips on any silence rule, including trailing silence
    // that decoded nothing - which happens over and over while nobody is
    // speaking. Resetting on one of those discards the encoder state that has
    // just begun consuming the next phrase, and that is heard as the first
    // word going missing: for a command, the word carrying the whole meaning.
    const bool atEndpoint = s_onlineStreamIsEndpoint(mRecognizer, mStream);

    if (atEndpoint && !text.isEmpty()) {
#ifdef DEBUG_STT
        qDebug() << "SherpaRecognizer: Final result:" << text;
#endif
        emit finalResult(text);
        mLastPartialResult.clear();

        // Only if this callback still owns the stream, for the reason
        // doStopListening() re-checks state() after its own setState(): the
        // emit above reaches Lua synchronously, and "stop after the first
        // phrase" is the ordinary push-to-talk shape for a sysSTTResult
        // handler. stt.stop() runs doStopListening() -> destroyStream(), and
        // stt.close() frees the recognizer as well, so returning here to reset
        // would hand the C ABI null handles.
        if (!mRecognizer || !mStream) {
            return;
        }
        s_onlineStreamReset(mRecognizer, mStream);
    } else if (atEndpoint && mSilentChunks >= SILENT_CHUNKS_BEFORE_IDLE_RESET) {
        // Housekeeping during a real lull: without it the utterance clock runs
        // on through the silence until the maximum-length rule is permanently
        // met, which would cut the next phrase short at its first word. Safe
        // here precisely because nothing has been heard for a while.
        s_onlineStreamReset(mRecognizer, mStream);
        mLastPartialResult.clear();
    } else if (!text.isEmpty() && text != mLastPartialResult) {
        emit partialResult(text);
        mLastPartialResult = text;
    }
}

void SherpaRecognizer::slot_captureError(const QString& message)
{
    // The capture component has already torn its device down; the recognizer
    // just surfaces the fault and leaves Listening
    emit errorOccurred(message);
    setState(State::Error);
}

float SherpaRecognizer::calculateAudioLevel(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return 0.0f;
    }

    // Calculate RMS of 16-bit PCM samples
    const auto* samples = reinterpret_cast<const qint16*>(data.constData());
    const int numSamples = data.size() / sizeof(qint16);

    if (numSamples == 0) {
        return 0.0f;
    }

    qint64 sum = 0;
    for (int i = 0; i < numSamples; ++i) {
        sum += static_cast<qint64>(samples[i]) * samples[i];
    }

    const double rms = qSqrt(static_cast<double>(sum) / numSamples);

    // Normalize to 0.0-1.0 range (32767 is max for 16-bit signed)
    return static_cast<float>(qMin(rms / 32767.0, 1.0));
}

void SherpaRecognizer::destroyStream()
{
    if (mStream && s_destroyOnlineStream) {
        s_destroyOnlineStream(mStream);
        mStream = nullptr;
    }
}

void SherpaRecognizer::releaseSherpaResources()
{
    destroyStream();

    if (mRecognizer && s_destroyOnlineRecognizer) {
        s_destroyOnlineRecognizer(mRecognizer);
        mRecognizer = nullptr;
    }
}

void SherpaRecognizer::releaseResources()
{
    // Same reason as initialize()/loadModel() stopping it before reloading: the
    // device has to go before the decoder, or a caller is left with a live
    // microphone it has no way to close
    mpCapture->stop();
    releaseSherpaResources();
    // capabilities() already answers false for biasing once mRecognizer is
    // gone, but these are cleared too rather than leaning on that gate alone
    mSupportsBiasing = false;
    mBpeVocabPath.clear();
    setState(State::Uninitialized);
    announceCapabilitiesIfChanged();
}

SpeechRecognizer::VocabularyResult SherpaRecognizer::applyVocabulary(const QStringList& words)
{
    Q_UNUSED(words)
    // The base has already stored these and established that this model can be
    // biased and that they differ from what is in effect.
    //
    // The word list is built into the decoder when the recognizer is created,
    // so a model already loaded has to be rebuilt to bias toward a new one.
    // Calls loadModel() directly rather than initialize(): loadModel() is the
    // whole job (rebuild, report Applied/Failed via its return); initialize()
    // wraps that with a vocabulary-applied bookkeeping fix of its own, which
    // setVocabulary() - our caller - is about to redo anyway from the result
    // returned below. Going through initialize() would just repeat that for
    // no effect, and once did something worse: an earlier version of that
    // wrapper wrote the bookkeeping fix by calling back into setVocabulary(),
    // which reached this function again and recursed without terminating.
    if (state() == State::Ready && !mModelPath.isEmpty()) {
        return loadModel(mModelPath) ? VocabularyResult::Applied : VocabularyResult::Failed;
    }

    // Mid-session the phrase being spoken matters more than the new words, so
    // they wait for the next load. Failed rather than Applied: they are not in
    // effect yet, and a caller told otherwise would stop correcting results
    // itself while nothing was biasing them.
    return VocabularyResult::Failed;
}

bool SherpaRecognizer::setSensitivity(Sensitivity sensitivity)
{
    // This engine builds its endpoint rules itself rather than asking the
    // library for a mode, so there is no symbol that can be missing here and
    // the answer is always yes - unlike the Vosk backend, which refuses when
    // its libvosk cannot be tuned
    if (mSensitivity == sensitivity) {
        return true;
    }

    mSensitivity = sensitivity;

    // The endpoint rules are baked into the recognizer at creation, so a
    // loaded model is reloaded for the change to take effect. Only when idle:
    // a listening session keeps the rules it started with.
    if (state() == State::Ready && !mModelPath.isEmpty()) {
        initialize(mModelPath);
    }
    return true;
}

bool SherpaRecognizer::setLanguage(const QString& languageCode)
{
    if (mCurrentLanguage == languageCode) {
        return true;
    }

    const QString modelPath = findModelPathForLanguage(languageCode);
    if (modelPath.isEmpty()) {
        //: Shown when a speech language is chosen with no model installed for it; %1 is a language code such as en-US
        emit errorOccurred(tr("No installed model found for language: %1").arg(languageCode));
        return false;
    }

    // initialize() already emits errorOccurred and sets state on failure, and
    // updates mCurrentLanguage from the model path on success
    return initialize(modelPath);
}

QString SherpaRecognizer::findModelPathForLanguage(const QString& languageCode) const
{
    const QString langPart = languageCode.left(2).toLower();

    for (const QString& model : getInstalledModels()) {
        if (model.contains(QLatin1Char('-') + langPart + QLatin1Char('-'), Qt::CaseInsensitive) || model.contains(QLatin1Char('-') + langPart + QLatin1Char('_'), Qt::CaseInsensitive)) {
            return QDir(modelsDirectoryPath()).filePath(model);
        }
    }

    return QString();
}

QString SherpaRecognizer::modelsDirectoryPath()
{
    return mudlet::getMudletPath(enums::mainDataItemPath, qsl("sherpa-models"));
}

QStringList SherpaRecognizer::getInstalledModels()
{
    const QDir modelsDir(modelsDirectoryPath());
    QStringList models;
    for (const QString& entry : modelsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        if (looksLikeModelDir(modelsDir.filePath(entry))) {
            models.append(entry);
        }
    }
    return models;
}

QString SherpaRecognizer::defaultModelPath()
{
    const QStringList models = getInstalledModels();
    if (models.isEmpty()) {
        return QString();
    }
    return QDir(modelsDirectoryPath()).filePath(models.first());
}

bool SherpaRecognizer::looksLikeModelDir(const QString& modelPath)
{
    const QDir dir(modelPath);
    if (!dir.exists() || !dir.exists(qsl("tokens.txt"))) {
        return false;
    }
    return !dir.entryList({qsl("encoder*.onnx")}, QDir::Files).isEmpty() && !dir.entryList({qsl("decoder*.onnx")}, QDir::Files).isEmpty()
           && !dir.entryList({qsl("joiner*.onnx")}, QDir::Files).isEmpty();
}
