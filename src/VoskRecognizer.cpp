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

#include "SpeechAudioCapture.h"
#include "mudlet.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSettings>
#include <QtMath>

#include <optional>

#if defined(Q_OS_MACOS)
#include "MacMicrophonePermission.h"
#endif

QLibrary VoskRecognizer::sVoskLibrary;
bool VoskRecognizer::sLibraryLoaded = false;
bool VoskRecognizer::sLibraryLoadAttempted = false;
bool VoskRecognizer::sLibraryUnloadedByRequest = false;

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
// artifact rather than speech. Without word timings there is nothing to judge
// by, so the word stands: a libvosk that cannot report timings would otherwise
// lose the first word of every utterance to a guess, silently - "the dragon
// attacks" arriving as "dragon attacks" with nothing said about it.
static bool leadingWordIsPhantom(const QJsonArray& words)
{
    if (words.isEmpty()) {
        return false;
    }

    const QJsonObject first = words.first().toObject();
    if (!first.contains(QLatin1String("start")) || !first.contains(QLatin1String("end"))) {
        return false;
    }

    const double duration = first.value(QLatin1String("end")).toDouble() - first.value(QLatin1String("start")).toDouble();
    return duration >= MIN_PHANTOM_LEADING_WORD_SECONDS;
}

// Below this, the decoder's own confidence is not enough to keep a result that
// is nothing but a single filler word
static constexpr double MIN_TRUSTED_SINGLE_WORD_CONFIDENCE = 0.8;

// Confidence Vosk reported for a result consisting of exactly one word, or
// nothing when the result is not one word or carries no per-word detail to
// judge it by.
static std::optional<double> singleWordConfidence(const QJsonObject& resultObject)
{
    const QJsonArray words = resultObject.value(QLatin1String("result")).toArray();
    if (words.size() != 1) {
        return std::nullopt;
    }
    return words.first().toObject().value(QLatin1String("conf")).toDouble();
}

// Word detail for a final result, as the sysSTTWords schema describes it.
// skipLeading drops the first word for a result whose leading word was struck
// from the text: the two are emitted together and describing different
// sequences would make the word list evidence for a phrase nobody was told
// about.
static QVariantList wordsFromResult(const QJsonArray& words, const bool skipLeading)
{
    QVariantList wordsList;
    for (int index = skipLeading ? 1 : 0; index < words.size(); ++index) {
        const QJsonObject wordObject = words.at(index).toObject();
        QVariantMap wordData;
        wordData[qsl("word")] = wordObject.value(QLatin1String("word")).toString();
        wordData[qsl("conf")] = wordObject.value(QLatin1String("conf")).toDouble();
        wordData[qsl("start")] = wordObject.value(QLatin1String("start")).toDouble();
        wordData[qsl("end")] = wordObject.value(QLatin1String("end")).toDouble();
        wordsList.append(wordData);
    }
    return wordsList;
}

VoskRecognizer::VoskRecognizer(QObject* parent)
: SpeechRecognizer(parent)
, mpCapture(new SpeechAudioCapture(this))
{
    connect(mpCapture, &SpeechAudioCapture::pcm, this, &VoskRecognizer::slot_pcmReady);
    connect(mpCapture, &SpeechAudioCapture::captureError, this, &VoskRecognizer::slot_captureError);
    // A silence timeout ends the utterance the way the user stopping would:
    // finalise and report, never discard
    connect(mpCapture, &SpeechAudioCapture::silenceTimedOut, this, &VoskRecognizer::stopListening);
}

void VoskRecognizer::setSilenceTimeout(int msec)
{
    mpCapture->setSilenceTimeout(msec);
}

int VoskRecognizer::silenceTimeout() const
{
    return mpCapture->silenceTimeout();
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
    const QString libName = qsl("vosk");

    sVoskLibrary.setFileName(libName);

    if (!sVoskLibrary.load()) {
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

bool VoskRecognizer::libraryAvailable()
{
    // Not probed again while a caller has deliberately unloaded it. Otherwise
    // stt.unloadLibrary() is undone by the next getInfo() - and on Windows the
    // delete that the unload existed to permit then fails on a mapped module,
    // with nothing to explain it. reloadLibrary() is what clears this.
    if (!sLibraryLoadAttempted && !sLibraryUnloadedByRequest) {
        loadVoskLibrary();
    }
    return sLibraryLoaded;
}

bool VoskRecognizer::resetLibraryLoadState()
{
    // Whether the module actually went. QLibrary::unload() refuses while
    // anything still holds it mapped, and reporting success then is how a
    // caller comes to delete a file the loader has not let go of - the exact
    // failure on Windows that stt.unloadLibrary() exists to avoid.
    if (sVoskLibrary.isLoaded() && !sVoskLibrary.unload()) {
        return false;
    }

    // Reset state flags to allow fresh detection
    sLibraryLoaded = false;
    sLibraryLoadAttempted = false;

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
    return true;
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
    paths << qsl("/usr/local/lib/libvosk.dylib") << qsl("/opt/homebrew/lib/libvosk.dylib") << QCoreApplication::applicationDirPath() + qsl("/../Frameworks/libvosk.dylib");
#elif defined(Q_OS_WIN)
    paths << QDir(userLibraryPath()).filePath(qsl("libvosk.dll"));
    // Vosk releases include libvosk.dll (with "lib" prefix)
    paths << QCoreApplication::applicationDirPath() + qsl("/libvosk.dll");
#else
    paths << QDir(userLibraryPath()).filePath(qsl("libvosk.so"));
    paths << qsl("/usr/lib/libvosk.so") << qsl("/usr/local/lib/libvosk.so") << qsl("/usr/lib/x86_64-linux-gnu/libvosk.so");
#endif

    return paths;
}

bool VoskRecognizer::backendAvailable() const
{
    return libraryAvailable();
}

QString VoskRecognizer::backendVersion() const
{
    // Vosk doesn't provide a version API, return a placeholder
    return sLibraryLoaded ? qsl("0.3.x") : QString();
}

bool VoskRecognizer::initialize(const QString& modelPath)
{
    if (!loadVoskLibrary()) {
        setState(State::Error);
        //: Shown when speech recognition is asked to load a model but the recognition library itself is not installed
        emit errorOccurred(tr("Vosk library not available"));
        return false;
    }

    // Loading a model over a running session would free the decoder while the
    // device stayed open: audio kept arriving with nothing to decode it, the
    // state said Ready, and no stop() or close() could reach the microphone
    // again because both check listening() first. The recording light stayed
    // on for the rest of the session.
    mpCapture->stop();

    releaseVoskResources();

    QDir modelDir(modelPath);
    if (!modelDir.exists()) {
        setState(State::Error);
        //: Shown when a speech model cannot be found; %1 is the folder that was looked for
        emit errorOccurred(tr("Model path does not exist: %1").arg(modelPath));
        return false;
    }

    qInfo().noquote() << "VoskRecognizer: Loading model from:" << modelPath;

    mVoskModel = s_vosk_model_new(modelPath.toUtf8().constData());
    if (!mVoskModel) {
        setState(State::Error);
        //: Shown when a speech model folder exists but could not be loaded; %1 is that folder
        emit errorOccurred(tr("Failed to load Vosk model from: %1").arg(modelPath));
        return false;
    }

    mVoskRecognizer = s_vosk_recognizer_new(mVoskModel, static_cast<float>(VOSK_SAMPLE_RATE));
    if (!mVoskRecognizer) {
        s_vosk_model_free(mVoskModel);
        mVoskModel = nullptr;
        setState(State::Error);
        //: Shown when a speech model loaded but the recognizer using it could not be created
        emit errorOccurred(tr("Failed to create Vosk recognizer"));
        return false;
    }

    // Only now is there a model loaded for modelPath() to name; the failure
    // paths above leave it empty, which is what getInfo() promises
    mModelPath = modelPath;

    // wordResults is derived from a symbol that resolves when the library
    // loads, so what this backend can do has just changed. The header promises
    // consumers hear about that rather than having to re-read on spec.
    if (const Capabilities current = capabilities(); !(current == mAnnouncedCapabilities)) {
        mAnnouncedCapabilities = current;
        emit capabilitiesChanged(current);
    }

    if (s_vosk_recognizer_set_endpointer_mode && mEndpointerMode != EndpointerMode::Default) {
        s_vosk_recognizer_set_endpointer_mode(mVoskRecognizer, static_cast<int>(mEndpointerMode));
#ifdef DEBUG_STT
        qDebug() << "VoskRecognizer: Applied endpointer mode" << static_cast<int>(mEndpointerMode);
#endif
    }
    // Always requested: leadingWordIsPhantom() needs the timings to tell a
    // phantom leading word from a spoken one
    if (s_vosk_recognizer_set_words) {
        s_vosk_recognizer_set_words(mVoskRecognizer, 1);
#ifdef DEBUG_STT
        qDebug() << "VoskRecognizer: Enabled word-level results";
#endif
    }

    // Try to determine language from model path (convention: vosk-model-small-en-us-0.15)
    const QString dirName = modelDir.dirName();
    if (dirName.contains(QLatin1String("-en-"))) {
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

    setState(State::Ready);
    return true;
}

void VoskRecognizer::startListening()
{
    if (state() != State::Ready) {
        // Every refusal but "already listening" reports why: startListening()
        // returns void, so silence here reads to the caller as a successful start
        if (state() == State::Uninitialized) {
            //: Shown when speech recognition is asked to listen before a language model is loaded
            setState(State::Error);
            emit errorOccurred(tr("Recognizer not initialized. Call initialize() first."));
        } else if (state() == State::Error) {
            //: Shown when speech recognition is asked to listen while it is in an error state
            emit errorOccurred(tr("Speech recognition is in an error state - reload the model before listening again."));
        } else if (state() == State::Processing) {
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
        //
        // Starting first, so the guard at the top of this function refuses a
        // second request while the player is still looking at the first one -
        // two dialogs, then two callbacks, the later of which would rebuild
        // the recognizer and restart capture underneath the earlier.
        setState(State::Starting);
        QPointer<VoskRecognizer> weakThis = this;
        MacMicrophonePermission::requestAccess([weakThis](bool granted) {
            if (!weakThis) {
                return; // VoskRecognizer was destroyed
            }
            // The player may be looking at the permission dialog for a long
            // time, and a script can close or re-initialise the recognizer
            // while they are. Either leaves this request answering for a
            // session nobody is waiting on any more, so granting it would open
            // the microphone with no start behind it. Only a recognizer still
            // waiting on this very request is still Starting.
            if (weakThis->state() != State::Starting) {
                return;
            }
            if (granted) {
                weakThis->startListeningInternal();
            } else {
                qWarning() << "VoskRecognizer: Microphone permission denied by user";
                // VoskRecognizer::tr, not QObject::tr: the lambda is not a member, and
                // the default context would file this identical string a second
                // time for translators to translate twice
                //: Shown when the player refuses Mudlet access to the microphone; the path names the macOS setting that grants it
                emit weakThis->errorOccurred(VoskRecognizer::tr("Microphone permission denied. Please grant microphone access in System Settings > Privacy & Security > Microphone."));
                weakThis->setState(State::Error);
            }
        });
        return;
    }
    case MacMicrophonePermission::AuthorizationStatus::Denied:
    case MacMicrophonePermission::AuthorizationStatus::Restricted:
        qWarning() << "VoskRecognizer: Microphone permission denied or restricted";
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

void VoskRecognizer::startListeningInternal()
{
    // The recognizer is rebuilt before any audio device is opened, so the two
    // failures below cannot leave capture running for a session that never
    // starts; the capture component cleans up after its own failures.
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
        setState(State::Error);
        //: Shown when speech recognition could not be prepared for listening
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        return;
    }

    mVoskRecognizer = s_vosk_recognizer_new(mVoskModel, static_cast<float>(VOSK_SAMPLE_RATE));
    if (!mVoskRecognizer) {
        qWarning() << "VoskRecognizer: Failed to recreate recognizer";
        setState(State::Error);
        //: Shown when speech recognition could not be prepared for listening
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        return;
    }

    // Reapply settings to the new recognizer
    if (s_vosk_recognizer_set_endpointer_mode && mEndpointerMode != EndpointerMode::Default) {
        s_vosk_recognizer_set_endpointer_mode(mVoskRecognizer, static_cast<int>(mEndpointerMode));
    }

    // Always requested, for the same reason as in initialize(): without the
    // timings a phantom leading word cannot be told from a spoken one
    if (s_vosk_recognizer_set_words) {
        s_vosk_recognizer_set_words(mVoskRecognizer, 1);
    }

    // mpCapture emits its own translated captureError before returning false,
    // which slot_captureError() has already turned into errorOccurred - only
    // the state transition is left to do here
    if (!mpCapture->start()) {
        setState(State::Error);
        return;
    }

    setState(State::Listening);
}

void VoskRecognizer::emitFinalResult(const QJsonObject& resultObject, QString text)
{
    const QJsonArray words = resultObject.value(QLatin1String("result")).toArray();

    // Strip a leading hallucination word only when its timing says it was never
    // spoken - otherwise a phrase genuinely beginning "a", "an" or "to" loses
    // its first word
    bool strippedLeadingWord = false;
    if (leadingWordIsPhantom(words)) {
        const QString beforeStripping = text;
        text.replace(*kLeadingHallucinationRx, QString());
        text = text.trimmed();
        strippedLeadingWord = (text != beforeStripping);
    }

    if (text.isEmpty()) {
        return;
    }

#ifdef DEBUG_STT
    qDebug() << "VoskRecognizer: Final result:" << text;
#endif
    emit finalResult(text);

    const QVariantList wordsList = wordsFromResult(words, strippedLeadingWord);
    if (!wordsList.isEmpty()) {
        emit wordsResult(wordsList);
    }
}

void VoskRecognizer::stopListening()
{
    if (state() != State::Listening) {
        return;
    }

    setState(State::Processing);

    mpCapture->stop();

    if (mVoskRecognizer) {
        const char* resultJson = s_vosk_recognizer_final_result(mVoskRecognizer);
        if (resultJson) {
            const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(resultJson));
            const QJsonObject obj = doc.object();
            const QString text = obj.value(QLatin1String("text")).toString().trimmed();
            if (!text.isEmpty()) {
                const bool isSingleWord = !text.contains(QLatin1Char(' '));
                const bool isHallucinationWord = kHallucinationWords->contains(text.toLower());
                const std::optional<double> confidence = singleWordConfidence(obj);
                const bool hasHighConfidence = confidence && *confidence >= MIN_TRUSTED_SINGLE_WORD_CONFIDENCE;

                // The filter slot_pcmReady() applies, without its audio-level
                // term: capture has already stopped, so there is no current
                // level to weigh and a lone filler word survives here only on
                // the decoder's own confidence
                const bool shouldFilter = isSingleWord && isHallucinationWord && !hasHighConfidence;

                if (shouldFilter) {
#ifdef DEBUG_STT
                    qDebug() << "VoskRecognizer: Filtered hallucination on stop:" << text;
#endif
                } else {
                    emitFinalResult(obj, text);
                }
            }
        }
    }

    // Only if this call still owns the session. setState(Processing) above
    // reaches Lua synchronously, so a handler for that state change can have
    // closed the recognizer while this call was inside the decoder; saying
    // Ready on top of that would claim a loaded model that has been freed.
    if (state() == State::Processing) {
        setState(State::Ready);
    }
}

void VoskRecognizer::cancel()
{
    // Starting counts: a request waiting on the macOS permission dialog has no
    // audio to abandon, but leaving it there means the callback still finds
    // Starting when the player finally answers and opens the microphone after
    // they asked to stop. Dropping to Ready is what makes that guard refuse.
    if (state() == State::Starting) {
        setState(State::Ready);
        return;
    }

    if (state() != State::Listening && state() != State::Processing) {
        return;
    }

    // Stop audio capture without processing the remainder
    mpCapture->stop();

    if (s_vosk_recognizer_reset && mVoskRecognizer) {
        s_vosk_recognizer_reset(mVoskRecognizer);
    }

    setState(State::Ready);
}

void VoskRecognizer::slot_pcmReady(const QByteArray& pcmData)
{
    if (!mVoskRecognizer || pcmData.isEmpty()) {
        return;
    }

    const float level = calculateAudioLevel(pcmData);

    // Track recent audio level with smoothing (for silence detection)
    mRecentAudioLevel = mRecentAudioLevel * 0.7f + level * 0.3f;

    // Track speech onset frames (for filtering initial hallucinations)
    const bool isSilent = mRecentAudioLevel < SILENCE_THRESHOLD;
    if (isSilent) {
        mSpeechOnsetFrames = 0;
    } else {
        mSpeechOnsetFrames++;
    }

    const int result = s_vosk_recognizer_accept_waveform(mVoskRecognizer, pcmData.constData(), pcmData.size());

    // -1 is the decoder reporting that it threw internally. Treating that as
    // "not finished yet" leaves a session that looks healthy - listening, audio
    // level moving - and never produces a result, with nothing said about why.
    if (result < 0) {
        qWarning() << "VoskRecognizer: the decoder faulted while accepting audio; stopping capture";
        mpCapture->stop();
        setState(State::Error);
        //: Shown when the speech engine's decoder fails while audio is being fed to it
        emit errorOccurred(tr("The speech engine stopped decoding unexpectedly. Try starting speech recognition again."));
        return;
    }

    if (result > 0) {
        // We have a complete utterance
        const char* resultJson = s_vosk_recognizer_result(mVoskRecognizer);
        if (resultJson) {
            const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(resultJson));
            const QJsonObject obj = doc.object();
            const QString text = obj.value(QLatin1String("text")).toString().trimmed();
            if (!text.isEmpty()) {
                const bool isSingleWord = !text.contains(QLatin1Char(' '));
                const bool isHallucinationWord = kHallucinationWords->contains(text.toLower());
                const std::optional<double> confidence = singleWordConfidence(obj);
                const bool hasLowConfidence = confidence && *confidence < MIN_TRUSTED_SINGLE_WORD_CONFIDENCE;

                const bool shouldFilter = isSingleWord && isHallucinationWord && (mRecentAudioLevel < SILENCE_THRESHOLD * 5.0f || hasLowConfidence);

                if (shouldFilter) {
#ifdef DEBUG_STT
                    qDebug() << "VoskRecognizer: Filtered hallucination (final):" << text << "(level:" << mRecentAudioLevel << ", lowConf:" << hasLowConfidence << ")";
#endif
                } else {
                    emitFinalResult(obj, text);
                }
            }
        }
        mLastPartialResult.clear();
    } else {
        const char* partialJson = s_vosk_recognizer_partial_result(mVoskRecognizer);
        if (partialJson) {
            const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(partialJson));
            QString text = doc.object().value(QLatin1String("partial")).toString().trimmed();
            if (!text.isEmpty()) {
                const bool isOnsetPhase = mSpeechOnsetFrames < SPEECH_ONSET_FRAMES;
                const bool isSingleWord = !text.contains(QLatin1Char(' '));
                const bool isHallucinationWord = kHallucinationWords->contains(text.toLower());
                const bool shouldFilter = (isSilent || isOnsetPhase) && isSingleWord && isHallucinationWord;
                const bool isStuckHallucination = (text == mLastPartialResult) && isSingleWord && isHallucinationWord;

                if (shouldFilter || isStuckHallucination) {
#ifdef DEBUG_STT
                    qDebug() << "VoskRecognizer: Filtered hallucination:" << text << "(level:" << mRecentAudioLevel << ", onset:" << mSpeechOnsetFrames << ")";
#endif
                } else {
                    // No stripping here, unlike the final result: that strips
                    // only when word timings prove the leading word spanned
                    // silence, and a partial carries no timings to prove it
                    // with. Stripping anyway made live preview text mutate on
                    // commit - "dragon attacks" while speaking, "the dragon
                    // attacks" once decided - and a package acting on the last
                    // partial when a final was slow sent the wrong command.
                    const QString cleanText = text.trimmed();

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

void VoskRecognizer::slot_captureError(const QString& message)
{
    // The capture component has already torn its device down; the recognizer
    // just surfaces the fault and leaves Listening
    setState(State::Error);
    emit errorOccurred(message);
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
    // Same reason as initialize(): the device has to go before the decoder,
    // or a caller is left with a live microphone it has no call to close
    mpCapture->stop();
    releaseVoskResources();
    mModelPath.clear();
    setState(State::Uninitialized);
}

bool VoskRecognizer::setLanguage(const QString& languageCode)
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
    const QString selected = getSelectedModelPath();
    if (!selected.isEmpty()) {
        return selected;
    }

    // Fallback to hardcoded default path for backward compatibility
    return mudlet::getMudletPath(enums::mainDataItemPath, qsl("vosk-models/vosk-model-small-en-us-0.15"));
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

    const QStringList entries = modelsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        const QDir modelDir(modelsDir.absoluteFilePath(entry));

        // Vosk models carry am/, conf/, graph/ and ivector/ subdirectories, and
        // not every model ships all four
        if (modelDir.exists(qsl("am")) || modelDir.exists(qsl("conf")) || modelDir.exists(qsl("graph")) || modelDir.exists(qsl("ivector"))) {
            models.append(entry);
        }
    }

    return models;
}

QString VoskRecognizer::getBestAvailableModel()
{
    const QStringList installed = getInstalledModels();
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
    QSettings settings;
    settings.beginGroup(qsl("SpeechRecognition"));
    const QString selectedModel = settings.value(qsl("selectedModel")).toString();
    settings.endGroup();

    if (!selectedModel.isEmpty()) {
        const QString modelPath = modelsDirectoryPath() + QDir::separator() + selectedModel;
        if (QDir(modelPath).exists()) {
            return modelPath;
        }
        // Selected model no longer exists, fall through to auto-detect
    }

    const QString bestModel = getBestAvailableModel();
    if (!bestModel.isEmpty()) {
        return modelsDirectoryPath() + QDir::separator() + bestModel;
    }

    return QString();
}

bool VoskRecognizer::setEndpointerMode(EndpointerMode mode)
{
    // A libvosk without this symbol cannot be tuned at all, so remembering the
    // request would make sensitivity() agree with the caller and disagree with
    // the engine - the hardest shape to debug, since every readback insists the
    // pauses are configured while they never happen
    if (!s_vosk_recognizer_set_endpointer_mode) {
        return false;
    }

    // Clamp to valid range: Default (0) to VeryLong (4)
    const int modeInt = qBound(0, static_cast<int>(mode), 4);
    mEndpointerMode = static_cast<EndpointerMode>(modeInt);

    if (mVoskRecognizer) {
        s_vosk_recognizer_set_endpointer_mode(mVoskRecognizer, modeInt);
#ifdef DEBUG_STT
        qDebug() << "VoskRecognizer: Set endpointer mode to" << modeInt;
#endif
    }
    return true;
}

bool VoskRecognizer::setSensitivity(Sensitivity sensitivity)
{
    // Map generic Sensitivity to Vosk-specific EndpointerMode
    switch (sensitivity) {
    case Sensitivity::Short:
        return setEndpointerMode(EndpointerMode::Short);
    case Sensitivity::Long:
        return setEndpointerMode(EndpointerMode::Long);
    case Sensitivity::Default:
    default:
        return setEndpointerMode(EndpointerMode::Default);
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
