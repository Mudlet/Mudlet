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

#include "VoskRecognizer.h"

#include "mudlet.h"

#include <QAudioDevice>
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QPointer>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QtMath>

#if defined(Q_OS_MACOS)
#include "MacMicrophonePermission.h"
#endif

// Static member initialization
QLibrary VoskRecognizer::sVoskLibrary;
bool VoskRecognizer::sLibraryLoaded = false;
bool VoskRecognizer::sLibraryLoadAttempted = false;

VoskRecognizer::vosk_model_new_fn VoskRecognizer::s_vosk_model_new = nullptr;
VoskRecognizer::vosk_model_free_fn VoskRecognizer::s_vosk_model_free = nullptr;
VoskRecognizer::vosk_recognizer_new_fn VoskRecognizer::s_vosk_recognizer_new = nullptr;
VoskRecognizer::vosk_recognizer_free_fn VoskRecognizer::s_vosk_recognizer_free = nullptr;
VoskRecognizer::vosk_recognizer_accept_waveform_fn VoskRecognizer::s_vosk_recognizer_accept_waveform = nullptr;
VoskRecognizer::vosk_recognizer_result_fn VoskRecognizer::s_vosk_recognizer_result = nullptr;
VoskRecognizer::vosk_recognizer_partial_result_fn VoskRecognizer::s_vosk_recognizer_partial_result = nullptr;
VoskRecognizer::vosk_recognizer_final_result_fn VoskRecognizer::s_vosk_recognizer_final_result = nullptr;
VoskRecognizer::vosk_recognizer_reset_fn VoskRecognizer::s_vosk_recognizer_reset = nullptr;
VoskRecognizer::vosk_set_log_level_fn VoskRecognizer::s_vosk_set_log_level = nullptr;
VoskRecognizer::vosk_recognizer_set_endpointer_mode_fn VoskRecognizer::s_vosk_recognizer_set_endpointer_mode = nullptr;
VoskRecognizer::vosk_recognizer_set_words_fn VoskRecognizer::s_vosk_recognizer_set_words = nullptr;

// Sample rate for Vosk - most models expect 16kHz
static constexpr int VOSK_SAMPLE_RATE = 16000;

// Common hallucination words that Vosk models generate during silence or speech onset
Q_GLOBAL_STATIC_WITH_ARGS(QStringList,
                          kHallucinationWords,
                          ({qsl("the"), qsl("a"), qsl("an"), qsl("to"), qsl("of"), qsl("and"), qsl("in"), qsl("is"), qsl("it"), qsl("i"), qsl("that"), qsl("for"), qsl("you"), qsl("on"), qsl("be")}))

// Regex to strip leading hallucination words from multi-word results
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, kLeadingHallucinationRx, (qsl("^(the|a|an|to)\\s+"), QRegularExpression::CaseInsensitiveOption))

// A leading filler word this long was not spoken: the decoder assigns the silence
// preceding an utterance to its first word, so a phantom "the" carries the whole
// pause with it. Measured durations were 2.2-4.3s for phantoms against 0.12-0.48s
// for every genuinely spoken word, so the boundary sits in an empty gap.
static constexpr double MIN_PHANTOM_LEADING_WORD_SECONDS = 1.0;

// Whether the leading word of a result spans enough silence to be a decoder
// artifact rather than speech. Without word timings there is nothing to judge by,
// and the historical behaviour - strip it - is kept.
static bool leadingWordIsPhantom(const QJsonArray& words)
{
    if (words.isEmpty()) {
        return true;
    }

    const QJsonObject first = words.first().toObject();
    if (!first.contains(QLatin1String("start")) || !first.contains(QLatin1String("end"))) {
        return true;
    }

    const double duration = first.value(QLatin1String("end")).toDouble() - first.value(QLatin1String("start")).toDouble();
    return duration >= MIN_PHANTOM_LEADING_WORD_SECONDS;
}

VoskRecognizer::VoskRecognizer(QObject* parent)
: SpeechRecognizer(parent)
{
    // Set up audio format for Vosk (16kHz, 16-bit, mono PCM)
    mAudioFormat.setSampleRate(VOSK_SAMPLE_RATE);
    mAudioFormat.setChannelCount(1);
    mAudioFormat.setSampleFormat(QAudioFormat::Int16);

    // Connect the process timer to audio processing slot
    connect(&mProcessTimer, &QTimer::timeout, this, &VoskRecognizer::processAudioData);
}

// Human-readable description of a QAudio::Error, for messages a player sees.
// Each of these is substituted into "Audio input error occurred: %1", so they
// read as the tail of that sentence rather than as standalone messages.
static QString audioErrorText(const QAudio::Error error)
{
    switch (error) {
    case QAudio::NoError:
        //: Completes "Audio input error occurred: %1" - no fault was reported
        return QCoreApplication::translate("VoskRecognizer", "no error");
    case QAudio::OpenError:
        //: Completes "Audio input error occurred: %1" - the microphone could not be opened
        return QCoreApplication::translate("VoskRecognizer", "the microphone could not be opened");
    case QAudio::IOError:
        //: Completes "Audio input error occurred: %1" - reading from the microphone failed
        return QCoreApplication::translate("VoskRecognizer", "reading from the microphone failed");
    case QAudio::UnderrunError:
        //: Completes "Audio input error occurred: %1" - the microphone went silent mid-capture
        return QCoreApplication::translate("VoskRecognizer", "the microphone stopped supplying audio");
    case QAudio::FatalError:
        //: Completes "Audio input error occurred: %1" - the microphone can no longer be used
        return QCoreApplication::translate("VoskRecognizer", "the microphone became unusable");
    }
    //: Completes "Audio input error occurred: %1" - the fault could not be identified
    return QCoreApplication::translate("VoskRecognizer", "unknown error");
}

VoskRecognizer::~VoskRecognizer()
{
    // cancel() ends with setState(), which emits stateChanged(). Connections are
    // still live until ~QObject runs, so that would deliver a state change from a
    // half-destroyed object to slots that go on to query it.
    blockSignals(true);
    cancel();
    releaseVoskResources();
}

bool VoskRecognizer::loadVoskLibrary()
{
    if (sLibraryLoadAttempted) {
        return sLibraryLoaded;
    }

    sLibraryLoadAttempted = true;

    // Determine library name based on platform
    // QLibrary automatically adds platform-specific prefix/suffix:
    // - On macOS: adds "lib" prefix and ".dylib" suffix -> "vosk" becomes "libvosk.dylib"
    // - On Windows: adds ".dll" suffix -> "vosk" becomes "vosk.dll"
    // - On Linux: adds "lib" prefix and ".so" suffix -> "vosk" becomes "libvosk.so"
    const QString libName = QStringLiteral("vosk");

    sVoskLibrary.setFileName(libName);

    if (!sVoskLibrary.load()) {
        // Try common installation paths
        for (const QString& path : librarySearchPaths()) {
            sVoskLibrary.setFileName(path);
            if (sVoskLibrary.load()) {
                break;
            }
        }
    }

    if (!sVoskLibrary.isLoaded()) {
        qWarning() << "VoskRecognizer: Failed to load Vosk library:" << sVoskLibrary.errorString();
        return false;
    }

    // Resolve function pointers
    s_vosk_model_new = reinterpret_cast<vosk_model_new_fn>(sVoskLibrary.resolve("vosk_model_new"));
    s_vosk_model_free = reinterpret_cast<vosk_model_free_fn>(sVoskLibrary.resolve("vosk_model_free"));
    s_vosk_recognizer_new = reinterpret_cast<vosk_recognizer_new_fn>(sVoskLibrary.resolve("vosk_recognizer_new"));
    s_vosk_recognizer_free = reinterpret_cast<vosk_recognizer_free_fn>(sVoskLibrary.resolve("vosk_recognizer_free"));
    s_vosk_recognizer_accept_waveform = reinterpret_cast<vosk_recognizer_accept_waveform_fn>(sVoskLibrary.resolve("vosk_recognizer_accept_waveform"));
    s_vosk_recognizer_result = reinterpret_cast<vosk_recognizer_result_fn>(sVoskLibrary.resolve("vosk_recognizer_result"));
    s_vosk_recognizer_partial_result = reinterpret_cast<vosk_recognizer_partial_result_fn>(sVoskLibrary.resolve("vosk_recognizer_partial_result"));
    s_vosk_recognizer_final_result = reinterpret_cast<vosk_recognizer_final_result_fn>(sVoskLibrary.resolve("vosk_recognizer_final_result"));
    s_vosk_recognizer_reset = reinterpret_cast<vosk_recognizer_reset_fn>(sVoskLibrary.resolve("vosk_recognizer_reset"));
    s_vosk_set_log_level = reinterpret_cast<vosk_set_log_level_fn>(sVoskLibrary.resolve("vosk_set_log_level"));

    // Optional newer API functions (may not be available in all Vosk versions)
    s_vosk_recognizer_set_endpointer_mode = reinterpret_cast<vosk_recognizer_set_endpointer_mode_fn>(sVoskLibrary.resolve("vosk_recognizer_set_endpointer_mode"));
    s_vosk_recognizer_set_words = reinterpret_cast<vosk_recognizer_set_words_fn>(sVoskLibrary.resolve("vosk_recognizer_set_words"));

    // Check that essential functions were resolved
    if (!s_vosk_model_new || !s_vosk_model_free || !s_vosk_recognizer_new || !s_vosk_recognizer_free || !s_vosk_recognizer_accept_waveform || !s_vosk_recognizer_result
        || !s_vosk_recognizer_partial_result || !s_vosk_recognizer_final_result) {
        qWarning() << "VoskRecognizer: Failed to resolve required Vosk functions";
        // Unloading on its own would leave the pointers that did resolve aiming
        // into a library that is no longer mapped
        resetLibraryLoadState();
        // resetLibraryLoadState() clears this to allow a fresh probe; a library
        // whose symbols are missing is not worth re-probing on every call
        sLibraryLoadAttempted = true;
        return false;
    }

    sLibraryLoaded = true;

    // Set Vosk log level to quiet (only errors)
    if (s_vosk_set_log_level) {
        s_vosk_set_log_level(-1);
    }

    return true;
}

bool VoskRecognizer::isVoskAvailable()
{
    if (!sLibraryLoadAttempted) {
        loadVoskLibrary();
    }
    return sLibraryLoaded;
}

bool VoskRecognizer::isLibraryAvailable()
{
    return isVoskAvailable();
}

void VoskRecognizer::resetLibraryLoadState()
{
    // Unload library if it was loaded
    if (sVoskLibrary.isLoaded()) {
        sVoskLibrary.unload();
    }

    // Reset state flags to allow fresh detection
    sLibraryLoaded = false;
    sLibraryLoadAttempted = false;

    // Clear function pointers
    s_vosk_model_new = nullptr;
    s_vosk_model_free = nullptr;
    s_vosk_recognizer_new = nullptr;
    s_vosk_recognizer_free = nullptr;
    s_vosk_recognizer_accept_waveform = nullptr;
    s_vosk_recognizer_result = nullptr;
    s_vosk_recognizer_partial_result = nullptr;
    s_vosk_recognizer_final_result = nullptr;
    s_vosk_recognizer_reset = nullptr;
    s_vosk_set_log_level = nullptr;
    s_vosk_recognizer_set_endpointer_mode = nullptr;
    s_vosk_recognizer_set_words = nullptr;
}

QString VoskRecognizer::userLibraryPath()
{
    return mudlet::getMudletPath(enums::mainDataItemPath, qsl("vosk-lib"));
}

QStringList VoskRecognizer::librarySearchPaths()
{
    QStringList paths;

#if defined(Q_OS_MACOS)
    paths << QDir(userLibraryPath()).filePath(qsl("libvosk.dylib"));
    paths << QStringLiteral("/usr/local/lib/libvosk.dylib") << QStringLiteral("/opt/homebrew/lib/libvosk.dylib")
          << QCoreApplication::applicationDirPath() + QStringLiteral("/../Frameworks/libvosk.dylib");
#elif defined(Q_OS_WIN)
    paths << QDir(userLibraryPath()).filePath(qsl("libvosk.dll"));
    // Vosk releases include libvosk.dll (with "lib" prefix)
    paths << QCoreApplication::applicationDirPath() + QStringLiteral("/libvosk.dll");
#else
    paths << QDir(userLibraryPath()).filePath(qsl("libvosk.so"));
    paths << QStringLiteral("/usr/lib/libvosk.so") << QStringLiteral("/usr/local/lib/libvosk.so") << QStringLiteral("/usr/lib/x86_64-linux-gnu/libvosk.so");
#endif

    return paths;
}

bool VoskRecognizer::isBackendAvailable() const
{
    return isVoskAvailable();
}

QString VoskRecognizer::backendVersion() const
{
    // Vosk doesn't provide a version API, return a placeholder
    return sLibraryLoaded ? QStringLiteral("0.3.x") : QString();
}

bool VoskRecognizer::initialize(const QString& modelPath)
{
    if (!loadVoskLibrary()) {
        emit errorOccurred(tr("Vosk library not available"));
        setState(State::Error);
        return false;
    }

    // Release any existing resources
    releaseVoskResources();

    // Check that the model path exists
    QDir modelDir(modelPath);
    if (!modelDir.exists()) {
        emit errorOccurred(tr("Model path does not exist: %1").arg(modelPath));
        setState(State::Error);
        return false;
    }

    mModelPath = modelPath;
    qInfo().noquote() << "VoskRecognizer: Loading model from:" << modelPath;

    // Load the Vosk model
    mVoskModel = s_vosk_model_new(modelPath.toUtf8().constData());
    if (!mVoskModel) {
        emit errorOccurred(tr("Failed to load Vosk model from: %1").arg(modelPath));
        setState(State::Error);
        return false;
    }

    // Create the recognizer
    mVoskRecognizer = s_vosk_recognizer_new(mVoskModel, static_cast<float>(VOSK_SAMPLE_RATE));
    if (!mVoskRecognizer) {
        s_vosk_model_free(mVoskModel);
        mVoskModel = nullptr;
        emit errorOccurred(tr("Failed to create Vosk recognizer"));
        setState(State::Error);
        return false;
    }

    // Apply optional settings if available
    if (s_vosk_recognizer_set_endpointer_mode && mEndpointerMode != EndpointerMode::Default) {
        s_vosk_recognizer_set_endpointer_mode(mVoskRecognizer, static_cast<int>(mEndpointerMode));
#ifdef DEBUG_STT
        qDebug() << "VoskRecognizer: Applied endpointer mode" << static_cast<int>(mEndpointerMode);
#endif
    }
    // Not conditional on mWordsEnabled: the word timings are needed to tell a
    // phantom leading word from a spoken one, whatever the display preference is
    if (s_vosk_recognizer_set_words) {
        s_vosk_recognizer_set_words(mVoskRecognizer, 1);
#ifdef DEBUG_STT
        qDebug() << "VoskRecognizer: Enabled word-level results";
#endif
    }

    // Try to determine language from model path (convention: vosk-model-small-en-us-0.15)
    const QString dirName = modelDir.dirName();
    if (dirName.contains(QLatin1String("-en-"))) {
        mCurrentLanguage = QStringLiteral("en-US");
    } else if (dirName.contains(QLatin1String("-de-"))) {
        mCurrentLanguage = QStringLiteral("de-DE");
    } else if (dirName.contains(QLatin1String("-fr-"))) {
        mCurrentLanguage = QStringLiteral("fr-FR");
    } else if (dirName.contains(QLatin1String("-es-"))) {
        mCurrentLanguage = QStringLiteral("es-ES");
    } else {
        mCurrentLanguage = QStringLiteral("unknown");
    }

    setState(State::Ready);
    return true;
}

bool VoskRecognizer::setupAudioInput()
{
    // Get the default audio input device
    const QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull()) {
        emit errorOccurred(tr("No microphone available"));
        return false;
    }

    // Always use the device's preferred format for reliable capture on macOS
    // We'll resample/convert to Vosk's format (16kHz mono Int16) later
    QAudioFormat formatToUse = inputDevice.preferredFormat();

    // ...but only Float and Int16 samples can be converted; anything else would
    // capture happily and then be discarded frame by frame, leaving the
    // recognizer Listening and permanently deaf. Ask for Int16 at the device's
    // own rate and channel count, and failing that at Vosk's 16kHz mono.
    if (formatToUse.sampleFormat() != QAudioFormat::Float && formatToUse.sampleFormat() != QAudioFormat::Int16) {
        QAudioFormat fallbackFormat = formatToUse;
        fallbackFormat.setSampleFormat(QAudioFormat::Int16);
        if (!inputDevice.isFormatSupported(fallbackFormat)) {
            fallbackFormat = mAudioFormat; // Vosk's own 16kHz mono Int16
        }
        if (!inputDevice.isFormatSupported(fallbackFormat)) {
            //: Shown when the microphone offers no audio format speech recognition can convert
            emit errorOccurred(tr("The microphone does not offer an audio format speech recognition can use"));
            return false;
        }
        formatToUse = fallbackFormat;
    }

    // Store the format we're actually using for later conversion
    mActualAudioFormat = formatToUse;

    // A source left behind by an attempt that failed after allocating one would
    // otherwise linger, holding the capture device and still connected to
    // handleAudioStateChanged(), which reads the member rather than sender()
    if (mAudioSource) {
        mAudioSource->stop();
        delete mAudioSource;
        mAudioSource = nullptr;
    }

    // Create the audio source with the device's preferred format
    mAudioSource = new QAudioSource(inputDevice, formatToUse, this);

    // Connect to state changes
    connect(mAudioSource.data(), &QAudioSource::stateChanged, this, &VoskRecognizer::handleAudioStateChanged);

    return true;
}

void VoskRecognizer::startListening()
{
    if (mState != State::Ready) {
        // Every refusal but "already listening" reports why: startListening()
        // returns void, so silence here reads to the caller as a successful start
        if (mState == State::Uninitialized) {
            //: Shown when speech recognition is asked to listen before a language model is loaded
            emit errorOccurred(tr("Recognizer not initialized. Call initialize() first."));
        } else if (mState == State::Error) {
            //: Shown when speech recognition is asked to listen while it is in an error state
            emit errorOccurred(tr("Speech recognition is in an error state - reload the model before listening again."));
        } else if (mState == State::Processing) {
            //: Shown when speech recognition is asked to listen while still transcribing the previous phrase
            emit errorOccurred(tr("Speech recognition is still processing the previous phrase."));
        }
        return;
    }

    // Check microphone permission on macOS using native API
    // Qt's permission API requires proper app signing with entitlements,
    // which development builds don't have, so we use AVFoundation directly.
#if defined(Q_OS_MACOS)
    auto status = MacMicrophonePermission::checkStatus();

    switch (status) {
    case MacMicrophonePermission::AuthorizationStatus::NotDetermined: {
        // requestAccess() dispatches its callback to the main queue, so this runs
        // on the main thread already. Use QPointer to safely handle the case where
        // VoskRecognizer is destroyed before the permission callback arrives.
        QPointer<VoskRecognizer> weakThis = this;
        MacMicrophonePermission::requestAccess([weakThis](bool granted) {
            if (!weakThis) {
                return; // VoskRecognizer was destroyed
            }
            if (granted) {
                weakThis->startListeningInternal();
            } else {
                qWarning() << "VoskRecognizer: Microphone permission denied by user";
                emit weakThis->errorOccurred(QObject::tr("Microphone permission denied. Please grant microphone access in System Settings > Privacy & Security > Microphone."));
                weakThis->setState(State::Error);
            }
        });
        return;
    }
    case MacMicrophonePermission::AuthorizationStatus::Denied:
    case MacMicrophonePermission::AuthorizationStatus::Restricted:
        qWarning() << "VoskRecognizer: Microphone permission denied or restricted";
        emit errorOccurred(tr("Microphone permission denied. Please grant microphone access in System Settings > Privacy & Security > Microphone."));
        setState(State::Error);
        return;
    case MacMicrophonePermission::AuthorizationStatus::Authorized:
        break;
    }
#endif

    startListeningInternal();
}

void VoskRecognizer::startListeningInternal()
{
    // The recognizer is rebuilt before any audio device is opened, so the two
    // failures below cannot leave a QAudioSource allocated for a session that
    // never starts. setupAudioInput() and the start() check after it can still
    // fail with one allocated, which is why both clean up after themselves.
    //
    // Recreate the recognizer for a new session to ensure clean state
    // Note: vosk_recognizer_reset() can leave the decoder in an inconsistent state
    // after vosk_recognizer_final_result() has been called, causing crashes
    // when accept_waveform() triggers internal CleanUp(). Recreating is safer.
    if (mVoskRecognizer && s_vosk_recognizer_free) {
        s_vosk_recognizer_free(mVoskRecognizer);
        mVoskRecognizer = nullptr;
    }

    if (!mVoskModel) {
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        setState(State::Error);
        return;
    }

    mVoskRecognizer = s_vosk_recognizer_new(mVoskModel, static_cast<float>(VOSK_SAMPLE_RATE));
    if (!mVoskRecognizer) {
        qWarning() << "VoskRecognizer: Failed to recreate recognizer";
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        setState(State::Error);
        return;
    }

    if (!setupAudioInput()) {
        setState(State::Error);
        return;
    }

    // Reapply settings to the new recognizer
    if (s_vosk_recognizer_set_endpointer_mode && mEndpointerMode != EndpointerMode::Default) {
        s_vosk_recognizer_set_endpointer_mode(mVoskRecognizer, static_cast<int>(mEndpointerMode));
    }

    // Always on, whatever the user's confidence-highlighting preference: the word
    // timings are what tell a phantom leading word from a spoken one. The
    // preference governs whether that detail is shown, not whether it is asked for.
    if (s_vosk_recognizer_set_words) {
        s_vosk_recognizer_set_words(mVoskRecognizer, 1);
    }

    // Clear any buffered audio
    mAudioBuffer.clear();
    mResamplePhase = 0.0;

    // Set volume to maximum
    mAudioSource->setVolume(1.0);

    // Set a larger buffer size for more reliable capture
    mAudioSource->setBufferSize(32000);

    // Try PULL mode - start() returns a QIODevice we read from
    mAudioDevice = mAudioSource->start();

    if (!mAudioDevice) {
        qWarning() << "VoskRecognizer: Failed to start audio source - start() returned null";
        // Released here rather than left for the next attempt to trip over: it
        // may still hold the capture device open
        mAudioSource->stop();
        mAudioSource->deleteLater();
        mAudioSource = nullptr;
        //: Shown when the microphone could not be opened for speech recognition
        emit errorOccurred(tr("Failed to start audio capture"));
        setState(State::Error);
        return;
    }

    // Poll every 50ms for audio data
    mProcessTimer.start(50);

    setState(State::Listening);
}

void VoskRecognizer::stopListening()
{
    if (mState != State::Listening) {
        return;
    }

    setState(State::Processing);

    // Stop the timer
    mProcessTimer.stop();

    // Stop audio capture
    if (mAudioSource) {
        mAudioSource->stop();
        mAudioSource->deleteLater();
        mAudioSource = nullptr;
    }
    mAudioDevice = nullptr;

    // Get final result from Vosk
    if (mVoskRecognizer) {
        const char* resultJson = s_vosk_recognizer_final_result(mVoskRecognizer);
        if (resultJson) {
            const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(resultJson));
            const QJsonObject obj = doc.object();
            QString text = obj.value(QLatin1String("text")).toString().trimmed();
            if (!text.isEmpty()) {
                // Apply same hallucination filtering as in processAudioData
                const bool isSingleWord = !text.contains(QLatin1Char(' '));
                const bool isHallucinationWord = kHallucinationWords->contains(text.toLower());

                // Check confidence if word-level results are available
                bool hasHighConfidence = false;

                if (mWordsEnabled && obj.contains(QLatin1String("result"))) {
                    const QJsonArray wordsArray = obj.value(QLatin1String("result")).toArray();
                    if (wordsArray.size() == 1) {
                        const double conf = wordsArray.first().toObject().value(QLatin1String("conf")).toDouble();
                        hasHighConfidence = conf >= 0.8;
                    }
                }

                // Filter single-word hallucinations on stop, unless confidence is high
                const bool shouldFilter = isSingleWord && isHallucinationWord && !hasHighConfidence;

                if (shouldFilter) {
#ifdef DEBUG_STT
                    qDebug() << "VoskRecognizer: Filtered hallucination on stop:" << text;
#endif
                } else {
                    // Strip a leading hallucination word only when its timing says it
                    // was never spoken - otherwise a phrase genuinely beginning "a",
                    // "an" or "to" loses its first word
                    if (leadingWordIsPhantom(obj.value(QLatin1String("result")).toArray())) {
                        text.replace(*kLeadingHallucinationRx, QString());
                        text = text.trimmed();
                    }

                    if (!text.isEmpty()) {
                        emit finalResult(text);
                    }
                }
            }
        }
    }

    mAudioBuffer.clear();
    mResamplePhase = 0.0;
    setState(State::Ready);
}

void VoskRecognizer::resetUtterance()
{
    // Only while listening: vosk_recognizer_reset() after a final result has been
    // taken can leave the decoder inconsistent, which is why startListeningInternal()
    // rebuilds the recognizer rather than resetting it. Mid-phrase, as here and in
    // cancel(), there is no final result outstanding and the reset is safe.
    if (mState != State::Listening) {
        return;
    }

    if (s_vosk_recognizer_reset && mVoskRecognizer) {
        s_vosk_recognizer_reset(mVoskRecognizer);
    }

    // The phrase this was tracking is gone, so nothing should be compared against
    // it, and the next words are a fresh onset rather than a continuation
    mLastPartialResult.clear();
    mSpeechOnsetFrames = 0;
}

void VoskRecognizer::cancel()
{
    if (mState != State::Listening && mState != State::Processing) {
        return;
    }

    // Stop the timer
    mProcessTimer.stop();

    // Stop audio capture without processing
    if (mAudioSource) {
        mAudioSource->stop();
        mAudioSource->deleteLater();
        mAudioSource = nullptr;
    }
    mAudioDevice = nullptr;

    // Reset the recognizer
    if (s_vosk_recognizer_reset && mVoskRecognizer) {
        s_vosk_recognizer_reset(mVoskRecognizer);
    }

    mAudioBuffer.clear();
    mResamplePhase = 0.0;
    setState(State::Ready);
}

void VoskRecognizer::processAudioData()
{
    // Timer callback for reading audio in pull mode
    if (!mAudioDevice || !mVoskRecognizer || !mAudioSource) {
        return;
    }

    // Check available bytes
    const qint64 bytesAvailable = mAudioDevice->bytesAvailable();

    if (bytesAvailable <= 0) {
        return;
    }

    // Read all available data
    QByteArray audioData = mAudioDevice->read(bytesAvailable);
    if (audioData.isEmpty()) {
        return;
    }

    // Process the audio
    processAudioDataFromBuffer(audioData);
}

void VoskRecognizer::processAudioDataFromBuffer(const QByteArray& audioData)
{
    if (!mVoskRecognizer || audioData.isEmpty()) {
        return;
    }

    // Convert from device format to Vosk format (16kHz mono Int16)
    const int srcRate = mActualAudioFormat.sampleRate();
    const int srcChannels = mActualAudioFormat.channelCount();
    const int dstRate = VOSK_SAMPLE_RATE; // 16000
    const QAudioFormat::SampleFormat srcFormat = mActualAudioFormat.sampleFormat();

    // setupAudioInput() settles on a format this can convert, so reaching either
    // of these means the device gave QAudioSource something else. Report once and
    // stop, rather than warning 20 times a second while appearing to listen.
    if (srcFormat != QAudioFormat::Float && srcFormat != QAudioFormat::Int16) {
        qWarning() << "VoskRecognizer: Unsupported audio format:" << srcFormat;
        //: %1 is a technical audio sample format name, e.g. "Int32"
        emit errorOccurred(tr("The microphone is supplying audio in a format speech recognition cannot read (%1).").arg(QString::number(static_cast<int>(srcFormat))));
        setState(State::Error);
        return;
    }

    if (srcRate <= 0 || srcChannels <= 0) {
        qWarning() << "VoskRecognizer: capture format has no usable sample rate or channel count:" << mActualAudioFormat;
        //: Shown when the microphone reports an audio format speech recognition cannot work with
        emit errorOccurred(tr("The microphone reported an unusable audio format - try selecting a different input device."));
        setState(State::Error);
        return;
    }

    const int bytesPerSample = (srcFormat == QAudioFormat::Float) ? static_cast<int>(sizeof(float)) : static_cast<int>(sizeof(qint16));
    const int frameBytes = bytesPerSample * srcChannels;

    // Carry over what the previous call could not consume, so a frame split
    // across two reads is resampled rather than discarded
    mAudioBuffer.append(audioData);
    const int availableFrames = mAudioBuffer.size() / frameBytes;

    // Interpolation reads the frame after the one it sits on, so the last frame
    // has to wait for the next call to supply its partner
    if (availableFrames < 2) {
        return;
    }

    const double ratio = static_cast<double>(srcRate) / dstRate;
    const char* const bufferData = mAudioBuffer.constData();

    // First channel only - Vosk wants mono
    const auto frameValue = [bufferData, srcChannels, srcFormat](const int frame) -> double {
        if (srcFormat == QAudioFormat::Float) {
            return static_cast<double>(reinterpret_cast<const float*>(bufferData)[frame * srcChannels]) * 32767.0;
        }
        return static_cast<double>(reinterpret_cast<const qint16*>(bufferData)[frame * srcChannels]);
    };

    QByteArray convertedData;
    convertedData.reserve(static_cast<int>(availableFrames / ratio + 1) * static_cast<int>(sizeof(qint16)));

    double phase = mResamplePhase;
    while (phase + 1.0 < availableFrames) {
        const int frame = static_cast<int>(phase);
        const double frac = phase - frame;
        const double sample = frameValue(frame) + frac * (frameValue(frame + 1) - frameValue(frame));
        // Clamped before rounding: qRound() returns int, so an out-of-range float
        // sample would overflow the conversion before a later clamp could help
        const qint16 converted = static_cast<qint16>(qRound(qBound(-32768.0, sample, 32767.0)));
        convertedData.append(reinterpret_cast<const char*>(&converted), sizeof(qint16));
        phase += ratio;
    }

    // Drop only the frames no longer needed: the frame the phase now sits on is
    // the next interpolation's left-hand sample, and any partial frame's bytes
    // stay put for the read that completes them.
    // The phase can end up past the last whole frame when downsampling, and
    // removing more bytes than the buffer holds would take the partial frame with
    // them - which desynchronises every read that follows, since what is left no
    // longer starts on a frame boundary. Any phase beyond the buffer is carried
    // instead, and the loop condition consumes it against the next read.
    const int consumedFrames = qMin(static_cast<int>(phase), availableFrames);
    if (consumedFrames > 0) {
        mAudioBuffer.remove(0, consumedFrames * frameBytes);
    }
    mResamplePhase = phase - consumedFrames;

    if (convertedData.isEmpty()) {
        return;
    }

    // Calculate and emit audio level
    const float level = calculateAudioLevel(convertedData);
    emit audioLevelChanged(level);

    // Track recent audio level with smoothing (for silence detection)
    mRecentAudioLevel = mRecentAudioLevel * 0.7f + level * 0.3f;

    // Track speech onset frames (for filtering initial hallucinations)
    const bool isSilent = mRecentAudioLevel < SILENCE_THRESHOLD;
    if (isSilent) {
        mSpeechOnsetFrames = 0; // Reset when silent
    } else {
        mSpeechOnsetFrames++;
    }

    // Feed audio to Vosk
    const int result = s_vosk_recognizer_accept_waveform(mVoskRecognizer, convertedData.constData(), convertedData.size());

    if (result > 0) {
        // We have a complete utterance
        const char* resultJson = s_vosk_recognizer_result(mVoskRecognizer);
        if (resultJson) {
            const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(resultJson));
            const QJsonObject obj = doc.object();
            QString text = obj.value(QLatin1String("text")).toString().trimmed();
            if (!text.isEmpty()) {
                // Filter single-word hallucination results
                const bool isSingleWord = !text.contains(QLatin1Char(' '));
                const bool isHallucinationWord = kHallucinationWords->contains(text.toLower());

                // Check confidence if word-level results are available
                bool hasLowConfidence = false;
                if (mWordsEnabled && obj.contains(QLatin1String("result"))) {
                    const QJsonArray wordsArray = obj.value(QLatin1String("result")).toArray();
                    if (wordsArray.size() == 1) {
                        const double conf = wordsArray.first().toObject().value(QLatin1String("conf")).toDouble();
                        // Consider confidence below 0.8 as low for single hallucination words
                        hasLowConfidence = conf < 0.8;
                    }
                }

                // Filter if: single hallucination word AND (low audio level OR low confidence)
                const bool shouldFilter = isSingleWord && isHallucinationWord && (mRecentAudioLevel < SILENCE_THRESHOLD * 5.0f || hasLowConfidence);

                if (shouldFilter) {
#ifdef DEBUG_STT
                    qDebug() << "VoskRecognizer: Filtered hallucination (final):" << text << "(level:" << mRecentAudioLevel << ", lowConf:" << hasLowConfidence << ")";
#endif
                } else {
                    // Strip a leading hallucination word only when its timing says it
                    // was never spoken - otherwise a phrase genuinely beginning "a",
                    // "an" or "to" loses its first word
                    if (leadingWordIsPhantom(obj.value(QLatin1String("result")).toArray())) {
                        text.replace(*kLeadingHallucinationRx, QString());
                        text = text.trimmed();
                    }

                    if (!text.isEmpty()) {
#ifdef DEBUG_STT
                        qDebug() << "VoskRecognizer: Final result:" << text;
#endif
                        emit finalResult(text);

                        // Emit word-level results with confidence if available and enabled
                        if (mWordsEnabled && obj.contains(QLatin1String("result"))) {
                            const QJsonArray wordsArray = obj.value(QLatin1String("result")).toArray();
                            QVariantList wordsList;
                            for (const QJsonValue& wordVal : wordsArray) {
                                const QJsonObject wordObj = wordVal.toObject();
                                QVariantMap wordData;
                                wordData[qsl("word")] = wordObj.value(QLatin1String("word")).toString();
                                wordData[qsl("conf")] = wordObj.value(QLatin1String("conf")).toDouble();
                                wordData[qsl("start")] = wordObj.value(QLatin1String("start")).toDouble();
                                wordData[qsl("end")] = wordObj.value(QLatin1String("end")).toDouble();
                                wordsList.append(wordData);
                            }
                            if (!wordsList.isEmpty()) {
                                emit wordsResult(wordsList);
                            }
                        }
                    }
                }
            }
        }
        mLastPartialResult.clear();
    } else {
        // Partial result
        const char* partialJson = s_vosk_recognizer_partial_result(mVoskRecognizer);
        if (partialJson) {
            const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(partialJson));
            QString text = doc.object().value(QLatin1String("partial")).toString().trimmed();
            if (!text.isEmpty()) {
                // Check if this is a single-word hallucination during silence or speech onset
                const bool isOnsetPhase = mSpeechOnsetFrames < SPEECH_ONSET_FRAMES;
                const bool isSingleWord = !text.contains(QLatin1Char(' '));
                const bool isHallucinationWord = kHallucinationWords->contains(text.toLower());
                const bool shouldFilter = (isSilent || isOnsetPhase) && isSingleWord && isHallucinationWord;

                // Also filter if the result is stuck on the same hallucination word
                const bool isStuckHallucination = (text == mLastPartialResult) && isSingleWord && isHallucinationWord;

                if (shouldFilter || isStuckHallucination) {
#ifdef DEBUG_STT
                    qDebug() << "VoskRecognizer: Filtered hallucination:" << text << "(level:" << mRecentAudioLevel << ", onset:" << mSpeechOnsetFrames << ")";
#endif
                } else {
                    // Strip leading hallucination words from multi-word results
                    QString cleanText = text;
                    cleanText.replace(*kLeadingHallucinationRx, QString());
                    cleanText = cleanText.trimmed();

                    if (!cleanText.isEmpty()) {
#ifdef DEBUG_STT
                        qDebug() << "VoskRecognizer: Partial result:" << cleanText << "(level:" << mRecentAudioLevel << ", onset:" << mSpeechOnsetFrames << ")";
#endif
                        emit partialResult(cleanText);
                    }
                }
                mLastPartialResult = text;
            }
        }
    }
}

void VoskRecognizer::handleAudioStateChanged(QAudio::State newState)
{
    // IdleState is included: a device that stops supplying audio reports the fault
    // there as readily as it does on StoppedState, and leaving it unhandled means
    // the timer keeps polling a dead microphone while the UI still says Listening
    if ((newState == QAudio::StoppedState || newState == QAudio::IdleState) && mAudioSource) {
        const QAudio::Error audioError = mAudioSource->error();
        if (audioError != QAudio::NoError) {
            qWarning() << "VoskRecognizer: Audio error:" << audioError;

            // cancel() returns early unless Listening or Processing, so the audio
            // device is torn down here rather than left open in any other state.
            // The member is cleared before stop() is called, so that a stateChanged
            // emitted from within stop() re-enters to a null source and returns
            // rather than reporting the same fault twice.
            mProcessTimer.stop();
            QAudioSource* pDyingSource = mAudioSource;
            mAudioSource = nullptr;
            mAudioDevice = nullptr;
            pDyingSource->stop();
            pDyingSource->deleteLater();
            mAudioBuffer.clear();
            mResamplePhase = 0.0;

            //: %1 is a description of what went wrong with the microphone
            emit errorOccurred(tr("Audio input error occurred: %1").arg(audioErrorText(audioError)));
            setState(State::Error);
        }
    }
}

float VoskRecognizer::calculateAudioLevel(const QByteArray& data) const
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

void VoskRecognizer::releaseVoskResources()
{
    if (mVoskRecognizer && s_vosk_recognizer_free) {
        s_vosk_recognizer_free(mVoskRecognizer);
        mVoskRecognizer = nullptr;
    }

    if (mVoskModel && s_vosk_model_free) {
        s_vosk_model_free(mVoskModel);
        mVoskModel = nullptr;
    }
}

void VoskRecognizer::releaseResources()
{
    releaseVoskResources();
    setState(State::Uninitialized);
}

void VoskRecognizer::setState(State newState)
{
    if (mState != newState) {
        mState = newState;
        emit stateChanged(newState);
    }
}

QStringList VoskRecognizer::availableLanguages() const
{
    // Return list of languages with available Vosk models
    // In a full implementation, this would scan for installed models
    return {QStringLiteral("en-US"),
            QStringLiteral("en-GB"),
            QStringLiteral("de-DE"),
            QStringLiteral("fr-FR"),
            QStringLiteral("es-ES"),
            QStringLiteral("it-IT"),
            QStringLiteral("pt-PT"),
            QStringLiteral("ru-RU"),
            QStringLiteral("zh-CN"),
            QStringLiteral("ja-JP")};
}

bool VoskRecognizer::setLanguage(const QString& languageCode)
{
    // If already using this language, nothing to do
    if (mCurrentLanguage == languageCode) {
        return true;
    }

    // Find a model that supports the requested language
    const QString modelPath = findModelPathForLanguage(languageCode);
    if (modelPath.isEmpty()) {
        emit errorOccurred(tr("No installed model found for language: %1").arg(languageCode));
        return false;
    }

    // Reinitialize with the new model
    if (!initialize(modelPath)) {
        // initialize() already emits errorOccurred and sets state
        return false;
    }

    // mCurrentLanguage is updated by initialize() based on the model path
    return true;
}

QString VoskRecognizer::findModelPathForLanguage(const QString& languageCode) const
{
    // Extract the language portion from the code (e.g., "en" from "en-US")
    QString langPart = languageCode.left(2).toLower();

    // Scan installed models for one matching this language
    const QStringList installed = getInstalledModels();
    QString bestMatch;
    int bestScore = -1;

    const QRegularExpression versionRx(qsl("(\\d+)\\.(\\d+)"));

    for (const QString& model : installed) {
        // Check if model name contains the language code pattern (e.g., "-en-" or "-en_")
        if (!model.contains(QLatin1Char('-') + langPart + QLatin1Char('-'), Qt::CaseInsensitive) && !model.contains(QLatin1Char('-') + langPart + QLatin1Char('_'), Qt::CaseInsensitive)) {
            continue;
        }

        // Score matching models - prefer larger models over small ones
        int score = 0;
        if (!model.contains(qsl("small"), Qt::CaseInsensitive)) {
            score += 1000;
        }

        // Extract version number if present and add to score
        const QRegularExpressionMatch match = versionRx.match(model);
        if (match.hasMatch()) {
            score += match.captured(1).toInt() * 10 + match.captured(2).toInt();
        }

        if (score > bestScore) {
            bestScore = score;
            bestMatch = model;
        }
    }

    if (bestMatch.isEmpty()) {
        return QString();
    }

    return modelsDirectoryPath() + QDir::separator() + bestMatch;
}

QString VoskRecognizer::defaultModelPath()
{
    // Use the selected model from settings, or auto-detect best available
    QString selected = getSelectedModelPath();
    if (!selected.isEmpty()) {
        return selected;
    }

    // Fallback to hardcoded default path for backward compatibility
    return mudlet::getMudletPath(enums::mainDataItemPath, qsl("vosk-models/vosk-model-small-en-us-0.15"));
}

QString VoskRecognizer::modelDownloadUrl(const QString& languageCode)
{
    // Return download URL for small models based on language
    static const QHash<QString, QString> modelUrls = {{QStringLiteral("en-US"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip")},
                                                      {QStringLiteral("de-DE"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-de-0.15.zip")},
                                                      {QStringLiteral("fr-FR"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip")},
                                                      {QStringLiteral("es-ES"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-es-0.42.zip")},
                                                      {QStringLiteral("it-IT"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-it-0.22.zip")},
                                                      {QStringLiteral("ru-RU"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-ru-0.22.zip")},
                                                      {QStringLiteral("zh-CN"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-cn-0.22.zip")},
                                                      {QStringLiteral("ja-JP"), QStringLiteral("https://alphacephei.com/vosk/models/vosk-model-small-ja-0.22.zip")}};

    return modelUrls.value(languageCode, modelUrls.value(QStringLiteral("en-US")));
}

QString VoskRecognizer::modelsDirectoryPath()
{
    return mudlet::getMudletPath(enums::mainDataItemPath, qsl("vosk-models"));
}

QStringList VoskRecognizer::getInstalledModels()
{
    QStringList models;
    QDir modelsDir(modelsDirectoryPath());

    if (!modelsDir.exists()) {
        return models;
    }

    // Look for directories that contain a Vosk model (must have conf/model.conf or similar)
    const QStringList entries = modelsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QString modelPath = modelsDir.absoluteFilePath(entry);
        QDir modelDir(modelPath);

        // Check if this looks like a Vosk model directory
        // Vosk models typically have am/, conf/, graph/, ivector/ subdirectories
        // or at minimum an am/ directory
        if (modelDir.exists(qsl("am")) || modelDir.exists(qsl("conf")) || modelDir.exists(qsl("graph")) || modelDir.exists(qsl("ivector"))) {
            models.append(entry);
        }
    }

    return models;
}

QString VoskRecognizer::getBestAvailableModel()
{
    QStringList installed = getInstalledModels();
    if (installed.isEmpty()) {
        return QString();
    }

    // Score models - prefer larger models (non-"small" models) over small ones
    // Also prefer newer versions if available
    QString bestModel;
    int bestScore = -1;

    const QRegularExpression versionRx(qsl("(\\d+)\\.(\\d+)"));

    for (const QString& model : installed) {
        int score = 0;

        // Large models are preferred (don't have "small" in name)
        if (!model.contains(qsl("small"), Qt::CaseInsensitive)) {
            score += 1000;
        }

        // English models get a small boost as default language
        if (model.contains(qsl("-en-"), Qt::CaseInsensitive) || model.contains(qsl("-en_"), Qt::CaseInsensitive)) {
            score += 10;
        }

        // Extract version number if present and add to score
        const QRegularExpressionMatch match = versionRx.match(model);
        if (match.hasMatch()) {
            score += match.captured(1).toInt() * 10 + match.captured(2).toInt();
        }

        if (score > bestScore) {
            bestScore = score;
            bestModel = model;
        }
    }

    return bestModel;
}

QString VoskRecognizer::getSelectedModelPath()
{
    // First check settings for a user-selected model
    QSettings settings;
    settings.beginGroup(qsl("SpeechRecognition"));
    QString selectedModel = settings.value(qsl("selectedModel")).toString();
    settings.endGroup();

    // If a model is selected in settings, verify it still exists
    if (!selectedModel.isEmpty()) {
        QString modelPath = modelsDirectoryPath() + QDir::separator() + selectedModel;
        QDir modelDir(modelPath);
        if (modelDir.exists()) {
            return modelPath;
        }
        // Selected model no longer exists, fall through to auto-detect
    }

    // Auto-detect the best available model
    QString bestModel = getBestAvailableModel();
    if (!bestModel.isEmpty()) {
        return modelsDirectoryPath() + QDir::separator() + bestModel;
    }

    // No models installed - return empty string
    return QString();
}

void VoskRecognizer::setSelectedModelPath(const QString& modelPath)
{
    QSettings settings;
    settings.beginGroup(qsl("SpeechRecognition"));

    if (modelPath.isEmpty()) {
        settings.remove(qsl("selectedModel"));
    } else {
        // Store just the model directory name, not the full path
        QDir modelDir(modelPath);
        settings.setValue(qsl("selectedModel"), modelDir.dirName());
    }

    settings.endGroup();
}

void VoskRecognizer::setEndpointerMode(EndpointerMode mode)
{
    // Clamp to valid range: Default (0) to VeryLong (4)
    int modeInt = qBound(0, static_cast<int>(mode), 4);
    mEndpointerMode = static_cast<EndpointerMode>(modeInt);

    // Apply to recognizer if it exists
    if (mVoskRecognizer && s_vosk_recognizer_set_endpointer_mode) {
        s_vosk_recognizer_set_endpointer_mode(mVoskRecognizer, modeInt);
#ifdef DEBUG_STT
        qDebug() << "VoskRecognizer: Set endpointer mode to" << modeInt;
#endif
    }
}

void VoskRecognizer::setWordsEnabled(bool enabled)
{
    // Records whether word detail is surfaced to the user. The backend is left
    // reporting words either way, because the leading-word filter depends on
    // their timings - turning them off would silently restore the old behaviour
    // of eating a spoken "a", "an" or "to".
    mWordsEnabled = enabled;
}

void VoskRecognizer::setSensitivity(Sensitivity sensitivity)
{
    // Map generic Sensitivity to Vosk-specific EndpointerMode
    switch (sensitivity) {
    case Sensitivity::Short:
        setEndpointerMode(EndpointerMode::Short);
        break;
    case Sensitivity::Long:
        setEndpointerMode(EndpointerMode::Long);
        break;
    case Sensitivity::Default:
    default:
        setEndpointerMode(EndpointerMode::Default);
        break;
    }
}

SpeechRecognizer::Sensitivity VoskRecognizer::sensitivity() const
{
    // Map Vosk-specific EndpointerMode back to generic Sensitivity
    switch (mEndpointerMode) {
    case EndpointerMode::VeryShort:
    case EndpointerMode::Short:
        return Sensitivity::Short;
    case EndpointerMode::Long:
    case EndpointerMode::VeryLong:
        return Sensitivity::Long;
    case EndpointerMode::Default:
    default:
        return Sensitivity::Default;
    }
}
