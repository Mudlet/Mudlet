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

#include "AppleSpeechRecognizer.h"

#include "MacMicrophonePermission.h"
#include "SpeechAudioCapture.h"

#include <QPointer>
#include <QSysInfo>
#include <QTimer>
#include <QVariantMap>
#include <QtMath>

#import <AVFoundation/AVFoundation.h>
#import <Speech/Speech.h>

// ARC owns each of these; the struct is created and destroyed from this file
// alone, so the header never has to know what an SFSpeechRecognizer is.
struct AppleSpeechSession
{
    SFSpeechRecognizer* recognizer = nil;
    SFSpeechAudioBufferRecognitionRequest* request = nil;
    SFSpeechRecognitionTask* task = nil;
    // What appendAudioPCMBuffer: is handed: mono 32-bit float at the rate
    // SpeechAudioCapture delivers. Built once - it never varies.
    AVAudioFormat* format = nil;
};

// Build a recognizer for a locale, or nil when the system has none for it.
static SFSpeechRecognizer* recognizerForLanguage(const QString& languageCode)
{
    NSLocale* locale = languageCode.isEmpty() ? [NSLocale currentLocale] : [NSLocale localeWithLocaleIdentifier:languageCode.toNSString()];
    if (!locale) {
        return nil;
    }
    return [[SFSpeechRecognizer alloc] initWithLocale:locale];
}

/*static*/ bool AppleSpeechRecognizer::speechAvailable()
{
    SFSpeechRecognizer* probe = recognizerForLanguage(QString());
    return probe != nil && probe.available;
}

AppleSpeechRecognizer::AppleSpeechRecognizer(QObject* parent)
: SpeechRecognizer(parent)
, mpSession(new AppleSpeechSession)
, mpCapture(new SpeechAudioCapture(this))
{
    mpSession->format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                         sampleRate:static_cast<double>(SpeechAudioCapture::scmSampleRate)
                                                           channels:1
                                                        interleaved:NO];

    connect(mpCapture, &SpeechAudioCapture::pcm, this, &AppleSpeechRecognizer::slot_pcmReady);
    connect(mpCapture, &SpeechAudioCapture::captureError, this, &AppleSpeechRecognizer::slot_captureError);
    // A silence timeout ends the utterance the way the user stopping would:
    // finalise and report, never discard
    connect(mpCapture, &SpeechAudioCapture::silenceTimedOut, this, &AppleSpeechRecognizer::stopListening);
}

AppleSpeechRecognizer::~AppleSpeechRecognizer()
{
    // cancel() ends with setState(), which emits stateChanged(). Connections
    // are still live until ~QObject runs, so that would deliver a state change
    // from a half-destroyed object to slots that go on to query it.
    blockSignals(true);
    cancel();
    mpCapture->stop();
    cancelTask();
}

bool AppleSpeechRecognizer::initialize(const QString& modelPath)
{
    Q_UNUSED(modelPath)

    // The device has to go before the engine it was feeding, or a caller
    // re-initialising mid-session is left with a live microphone and no way to
    // close it
    mpCapture->stop();
    cancelTask();

    SFSpeechRecognizer* recognizer = recognizerForLanguage(mCurrentLanguage);
    if (!recognizer) {
        //: Shown when macOS has no speech recognition for the chosen language; %1 is a language code such as en-US
        emit errorOccurred(tr("macOS has no speech recognition for the language '%1'.").arg(mCurrentLanguage.isEmpty() ? QString::fromNSString([NSLocale currentLocale].localeIdentifier) : mCurrentLanguage));
        setState(State::Error);
        return false;
    }
    if (!recognizer.available) {
        //: Shown when the macOS speech recognizer exists but the system reports it as unusable right now
        emit errorOccurred(tr("The macOS speech recognizer is not available at the moment. It becomes available once macOS has finished preparing the language it needs."));
        setState(State::Error);
        return false;
    }

    mpSession->recognizer = recognizer;
    mCurrentLanguage = QString::fromNSString(recognizer.locale.localeIdentifier);
    mLastPartialResult.clear();
    mRecentAudioLevel = 0.0f;

    // Nothing to bake in the way a model load does: contextualStrings belongs
    // to the request, which is built per utterance from vocabulary(), so any
    // words already offered are picked up by the first task without this
    // needing to touch the applied-vocabulary bookkeeping.
    setState(State::Ready);
    return true;
}

void AppleSpeechRecognizer::setSilenceTimeout(int msec)
{
    mpCapture->setSilenceTimeout(msec);
}

int AppleSpeechRecognizer::silenceTimeout() const
{
    return mpCapture->silenceTimeout();
}

bool AppleSpeechRecognizer::hasLiveNativeResources() const
{
    return mpSession->recognizer != nil || mpSession->task != nil;
}

void AppleSpeechRecognizer::releaseResources()
{
    mpCapture->stop();
    cancelTask();
    mpSession->recognizer = nil;
    // mCurrentLanguage is left alone on purpose: it is the caller's setting
    // rather than a resource, and the next initialize() should honour it.
    mLastPartialResult.clear();
    mRecentAudioLevel = 0.0f;
    setState(State::Uninitialized);
}

QString AppleSpeechRecognizer::backendVersion() const
{
    return QSysInfo::productVersion();
}

bool AppleSpeechRecognizer::setLanguage(const QString& languageCode)
{
    if (languageCode.isEmpty()) {
        return false;
    }

    if (!mpSession->recognizer) {
        // Nothing is loaded, so this is simply the locale initialize() will
        // ask for when it runs
        mCurrentLanguage = languageCode;
        return true;
    }

    // Built before anything is torn down: a locale the system cannot serve
    // must leave the working recognizer where it was
    SFSpeechRecognizer* replacement = recognizerForLanguage(languageCode);
    if (!replacement || !replacement.available) {
        //: Shown when a script asks for a speech language macOS cannot recognise; %1 is a language code such as en-US
        emit errorOccurred(tr("macOS has no speech recognition for the language '%1'.").arg(languageCode));
        return false;
    }

    // Whatever is in flight belongs to the old locale, so it goes rather than
    // being finalised into the new one
    cancel();
    mpCapture->stop();
    cancelTask();

    mpSession->recognizer = replacement;
    mCurrentLanguage = QString::fromNSString(replacement.locale.localeIdentifier);
    return true;
}

bool AppleSpeechRecognizer::setSensitivity(Sensitivity sensitivity)
{
    Q_UNUSED(sensitivity)
    return false;
}

SpeechRecognizer::VocabularyResult AppleSpeechRecognizer::applyVocabulary(const QStringList& words)
{
    Q_UNUSED(words)
    // The base has already stored these, and beginRecognitionTask() reads
    // vocabulary() when it builds each request - so there is nothing to
    // rebuild here the way sherpa has to rebuild a decoder.
    //
    // "Applied" here means "will be used", not "in effect this instant":
    // contextualStrings belongs to a request, and a request already being
    // decoded keeps the words it was created with. The next one - the next
    // utterance while listening, or the next startListening() - is the first
    // biased by these.
    return VocabularyResult::Applied;
}

void AppleSpeechRecognizer::doStartListening()
{
    // Two permissions stand between here and a microphone, and either can put
    // a dialog in front of the player. The state rules in SpeechRecognizer.h
    // are what make that safe: Starting is entered before the first
    // asynchronous hop, so a second startListening() is refused rather than
    // raising a second dialog, and every continuation re-checks Starting
    // because a cancel() from there is handled by the base and never reaches
    // doCancel().
    switch ([SFSpeechRecognizer authorizationStatus]) {
    case SFSpeechRecognizerAuthorizationStatusNotDetermined: {
        setState(State::Starting);
        QPointer<AppleSpeechRecognizer> weakThis = this;
        [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {
            // Apple calls this on an arbitrary queue; everything below touches
            // Qt objects, so it has to happen on the main thread
            dispatch_async(dispatch_get_main_queue(), ^{
                if (!weakThis) {
                    return; // the recognizer was destroyed while the dialog was up
                }
                // The player can sit on the dialog indefinitely, and a script
                // can cancel or close the recognizer while they do. Only a
                // recognizer still waiting on this very request is still
                // Starting, and this is the only place that can find out -
                // cancel() from Starting never reaches doCancel(). Nothing has
                // been acquired yet, so there is nothing to release here.
                if (weakThis->state() != State::Starting) {
                    return;
                }
                if (status != SFSpeechRecognizerAuthorizationStatusAuthorized) {
                    qWarning() << "AppleSpeechRecognizer: speech recognition authorization denied";
                    // AppleSpeechRecognizer::tr, not QObject::tr: the block is
                    // not a member, and the default context would file this
                    // identical string a second time for translators
                    //: Shown when the player refuses Mudlet permission to use macOS speech recognition; the path names the setting that grants it
                    emit weakThis->errorOccurred(AppleSpeechRecognizer::tr("Speech recognition permission denied. Please allow it in System Settings > Privacy & Security > Speech Recognition."));
                    weakThis->setState(State::Error);
                    return;
                }
                weakThis->requestMicrophoneThenStart();
            });
        }];
        return;
    }
    case SFSpeechRecognizerAuthorizationStatusDenied:
    case SFSpeechRecognizerAuthorizationStatusRestricted:
        qWarning() << "AppleSpeechRecognizer: speech recognition authorization denied or restricted";
        //: Shown when macOS speech recognition was refused earlier and has to be allowed in system settings before speech will work
        emit errorOccurred(tr("Speech recognition permission denied. Please allow it in System Settings > Privacy & Security > Speech Recognition."));
        // The same state the denial reaches when the dialog is answered now,
        // as docs/stt-api.md requires: a package driving its controls from
        // state would otherwise keep offering to listen on a machine that cannot
        setState(State::Error);
        return;
    case SFSpeechRecognizerAuthorizationStatusAuthorized:
        break;
    }

    requestMicrophoneThenStart();
}

void AppleSpeechRecognizer::requestMicrophoneThenStart()
{
    // Reached either straight from doStartListening() while still Ready, or
    // from the speech-authorisation continuation while Starting. Speech
    // recognition being allowed says nothing about the microphone, so this is
    // a second, independent dialog - and the gap between the two is exactly
    // where a cancel() lands, which is why the callback below re-checks
    // Starting just as the first one did.
    switch (MacMicrophonePermission::checkStatus()) {
    case MacMicrophonePermission::AuthorizationStatus::NotDetermined: {
        setState(State::Starting);
        QPointer<AppleSpeechRecognizer> weakThis = this;
        MacMicrophonePermission::requestAccess([weakThis](bool granted) {
            if (!weakThis) {
                return; // the recognizer was destroyed while the dialog was up
            }
            if (weakThis->state() != State::Starting) {
                return; // cancelled or closed between the two dialogs
            }
            if (!granted) {
                qWarning() << "AppleSpeechRecognizer: microphone permission denied by user";
                //: Shown when the player refuses Mudlet access to the microphone; the path names the macOS setting that grants it
                emit weakThis->errorOccurred(AppleSpeechRecognizer::tr("Microphone permission denied. Please grant microphone access in System Settings > Privacy & Security > Microphone."));
                weakThis->setState(State::Error);
                return;
            }
            weakThis->startListeningInternal();
        });
        return;
    }
    case MacMicrophonePermission::AuthorizationStatus::Denied:
    case MacMicrophonePermission::AuthorizationStatus::Restricted:
        qWarning() << "AppleSpeechRecognizer: microphone permission denied or restricted";
        //: Shown when microphone access was refused earlier and has to be granted in system settings before speech will work
        emit errorOccurred(tr("Microphone permission denied. Please grant microphone access in System Settings > Privacy & Security > Microphone."));
        setState(State::Error);
        return;
    case MacMicrophonePermission::AuthorizationStatus::Authorized:
        break;
    }

    startListeningInternal();
}

void AppleSpeechRecognizer::startListeningInternal()
{
    if (!mpSession->recognizer) {
        //: Shown when speech recognition could not be prepared for listening
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        setState(State::Error);
        return;
    }

    // Checked here rather than at initialize(): macOS fetches the on-device
    // assets for a locale in its own time, so a "no" now can be a "yes" in an
    // hour, and refusing to initialize would need a restart to undo. Refused
    // rather than worked around, because the only way around it is to let the
    // audio go to Apple - which is precisely what this backend promises not
    // to do.
    if (!mpSession->recognizer.supportsOnDeviceRecognition) {
        //: Shown when macOS cannot recognise this language without sending the audio to Apple, which Mudlet will not do; %1 is a language code such as en-US
        emit errorOccurred(tr("macOS cannot recognise '%1' on this Mac without sending the audio to Apple, so Mudlet will not listen. Add the language in System Settings > Keyboard > Dictation to have macOS download it.").arg(mCurrentLanguage));
        setState(State::Error);
        return;
    }

    mLastPartialResult.clear();
    mRecentAudioLevel = 0.0f;
    mConsecutiveImmediateFailures = 0;

    if (!beginRecognitionTask()) {
        //: Shown when speech recognition could not be prepared for listening
        emit errorOccurred(tr("Failed to initialize speech recognition"));
        setState(State::Error);
        return;
    }

    // mpCapture emits its own translated captureError before returning false,
    // which slot_captureError() has already turned into errorOccurred - only
    // the state transition is left to do here
    if (!mpCapture->start()) {
        cancelTask();
        setState(State::Error);
        return;
    }

    setState(State::Listening);
}

bool AppleSpeechRecognizer::beginRecognitionTask()
{
    // Bumps the generation, so nothing the previous task still has in flight
    // can be mistaken for this one's
    releaseTask();

    if (!mpSession->recognizer) {
        return false;
    }

    SFSpeechAudioBufferRecognitionRequest* request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
    if (!request) {
        return false;
    }
    request.shouldReportPartialResults = YES;
    // Not configurable, and deliberately so: docs/stt-api.md promises the
    // audio never leaves this machine, and this one property is what keeps
    // that promise for this backend
    request.requiresOnDeviceRecognition = YES;

    const QStringList words = vocabulary();
    if (!words.isEmpty()) {
        NSMutableArray<NSString*>* contextual = [NSMutableArray arrayWithCapacity:words.size()];
        for (const QString& word : words) {
            [contextual addObject:word.toNSString()];
        }
        request.contextualStrings = contextual;
    }

    const quint64 generation = mTaskGeneration;
    QPointer<AppleSpeechRecognizer> weakThis = this;
    SFSpeechRecognitionTask* task = [mpSession->recognizer
            recognitionTaskWithRequest:request
                         resultHandler:^(SFSpeechRecognitionResult* result, NSError* error) {
                             // Read out of the Objective-C objects here, on
                             // whichever queue Apple used, and hand the main
                             // thread plain values - it must not be left
                             // reaching back into a result whose lifetime
                             // belongs to this block.
                             QString text;
                             QVariantList wordDetails;
                             bool finalUtterance = false;
                             if (result) {
                                 finalUtterance = result.isFinal;
                                 text = QString::fromNSString(result.bestTranscription.formattedString).trimmed();
                                 for (SFTranscriptionSegment* segment in result.bestTranscription.segments) {
                                     QVariantMap wordData;
                                     wordData[qsl("word")] = QString::fromNSString(segment.substring);
                                     wordData[qsl("conf")] = static_cast<double>(segment.confidence);
                                     wordData[qsl("start")] = segment.timestamp;
                                     wordData[qsl("end")] = segment.timestamp + segment.duration;
                                     wordDetails.append(wordData);
                                 }
                             }
                             QString failure;
                             bool failedWithoutSayingWhy = false;
                             if (error) {
                                 failure = QString::fromNSString(error.localizedDescription);
                                 failedWithoutSayingWhy = failure.isEmpty();
                             }
                             if (!result && !error) {
                                 return;
                             }
                             dispatch_async(dispatch_get_main_queue(), ^{
                                 if (!weakThis) {
                                     return;
                                 }
                                 QString reported = failure;
                                 if (failedWithoutSayingWhy) {
                                     // Translated here rather than above: tr()
                                     // belongs on the main thread, and above it
                                     // was also paid for callbacks the early
                                     // return goes on to discard
                                     //: Shown when macOS speech recognition stopped and gave no reason
                                     reported = AppleSpeechRecognizer::tr("macOS speech recognition stopped without saying why.");
                                 }
                                 weakThis->handleRecognition(generation, text, finalUtterance, wordDetails, reported);
                             });
                         }];

    if (!task) {
        return false;
    }

    mpSession->request = request;
    mpSession->task = task;
    mTaskLifetime.start();
    return true;
}

void AppleSpeechRecognizer::releaseTask()
{
    // Every drop moves the generation on, which is what makes a callback from
    // the dropped task identifiable as stale
    ++mTaskGeneration;
    mpSession->task = nil;
    mpSession->request = nil;
}

void AppleSpeechRecognizer::cancelTask()
{
    if (mpSession->task) {
        [mpSession->task cancel];
    }
    releaseTask();
}

void AppleSpeechRecognizer::handleRecognition(quint64 generation, const QString& text, bool finalUtterance, const QVariantList& words, const QString& failure)
{
    if (generation != mTaskGeneration) {
        return; // from a task this recognizer has already moved on from
    }

    if (!failure.isEmpty()) {
        handleTaskEnded(failure);
        return;
    }

    if (finalUtterance) {
        if (!text.isEmpty()) {
            emit finalResult(text);
            if (!words.isEmpty()) {
                emit wordsResult(words);
            }
        }
        mLastPartialResult.clear();
        // A final result ends the task, so the session needs another one to go
        // on hearing anything
        handleTaskEnded(QString());
        return;
    }

    if (!text.isEmpty() && text != mLastPartialResult) {
        mLastPartialResult = text;
        emit partialResult(text);
    }
}

void AppleSpeechRecognizer::handleTaskEnded(const QString& failure)
{
    const bool diedImmediately = !failure.isEmpty() && mTaskLifetime.isValid() && mTaskLifetime.elapsed() < scmMinTaskLifetimeMsec;
    releaseTask();

    if (state() == State::Processing) {
        // doStopListening() asked for the last utterance and has now had it
        setState(State::Ready);
        return;
    }

    if (state() != State::Listening) {
        // Cancelled, closed, or already faulted; nothing to continue
        return;
    }

    // A task that ran for a while and then ended is the framework's ordinary
    // way of closing a quiet stretch or a finished phrase, however it worded
    // it. Only tasks dying the instant they start are a fault worth reporting
    // - anything else and a silent room would end the session.
    mConsecutiveImmediateFailures = diedImmediately ? mConsecutiveImmediateFailures + 1 : 0;
    if (mConsecutiveImmediateFailures >= scmMaxImmediateFailures) {
        mpCapture->stop();
        emit errorOccurred(failure);
        setState(State::Error);
        return;
    }

    if (!beginRecognitionTask()) {
        mpCapture->stop();
        //: Shown when macOS speech recognition stopped part-way through listening and could not be restarted
        emit errorOccurred(tr("Speech recognition stopped and could not be restarted."));
        setState(State::Error);
    }
}

void AppleSpeechRecognizer::doStopListening()
{
    setState(State::Processing);

    mpCapture->stop();

    if (!mpSession->request || !mpSession->task) {
        setState(State::Ready);
        return;
    }

    // The last result comes back through the task's handler rather than from
    // here, so this returns while still Processing and handleTaskEnded()
    // settles it
    [mpSession->request endAudio];

    const quint64 generation = mTaskGeneration;
    QTimer::singleShot(scmFinaliseTimeoutMsec, this, [this, generation]() {
        if (mTaskGeneration != generation || state() != State::Processing) {
            return; // the final result arrived, or the session moved on
        }
        // Nothing came back. Processing is not a state anything can be started
        // from, so waiting for ever would take the recognizer with it.
        cancelTask();
        setState(State::Ready);
    });
}

void AppleSpeechRecognizer::doCancel()
{
    mpCapture->stop();
    cancelTask();
    mLastPartialResult.clear();
    mRecentAudioLevel = 0.0f;

    setState(State::Ready);
}

void AppleSpeechRecognizer::slot_pcmReady(const QByteArray& pcmData)
{
    if (pcmData.isEmpty() || !mpSession->request || !mpSession->format) {
        return;
    }

    const float level = calculateAudioLevel(pcmData);
    // Smoothed so the reported level reflects the phrase rather than whichever
    // 50ms chunk was last seen
    mRecentAudioLevel = mRecentAudioLevel * 0.7f + level * 0.3f;

    const auto* samples = reinterpret_cast<const qint16*>(pcmData.constData());
    const auto frames = static_cast<AVAudioFrameCount>(pcmData.size() / static_cast<int>(sizeof(qint16)));
    if (frames == 0) {
        return;
    }

    AVAudioPCMBuffer* buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:mpSession->format frameCapacity:frames];
    if (!buffer || !buffer.floatChannelData) {
        return;
    }
    buffer.frameLength = frames;

    // The framework takes mono float samples in [-1, 1]
    float* destination = buffer.floatChannelData[0];
    for (AVAudioFrameCount i = 0; i < frames; ++i) {
        destination[i] = static_cast<float>(samples[i]) / 32768.0f;
    }

    [mpSession->request appendAudioPCMBuffer:buffer];
}

void AppleSpeechRecognizer::slot_captureError(const QString& message)
{
    // The capture component has already torn its device down; the recognizer
    // just surfaces the fault and leaves Listening
    emit errorOccurred(message);
    setState(State::Error);
}

float AppleSpeechRecognizer::calculateAudioLevel(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return 0.0f;
    }

    const auto* samples = reinterpret_cast<const qint16*>(data.constData());
    const int numSamples = data.size() / static_cast<int>(sizeof(qint16));
    if (numSamples == 0) {
        return 0.0f;
    }

    qint64 sum = 0;
    for (int i = 0; i < numSamples; ++i) {
        sum += static_cast<qint64>(samples[i]) * samples[i];
    }

    const double rms = qSqrt(static_cast<double>(sum) / numSamples);

    // Normalise to 0.0-1.0 (32767 is the maximum for 16-bit signed)
    return static_cast<float>(qMin(rms / 32767.0, 1.0));
}
