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

#ifndef MUDLET_APPLESPEECHRECOGNIZER_H
#define MUDLET_APPLESPEECHRECOGNIZER_H

#include "SpeechRecognizer.h"
#include "utils.h"

#include <QElapsedTimer>
#include <QString>

class SpeechAudioCapture;

// The Objective-C objects this backend holds - the recognizer, the request it
// feeds and the task decoding it. Declared opaque here rather than in the
// header so this file stays plain C++: moc compiles it, and so does the
// contract test, neither of which is Objective-C++. Defined in the .mm.
struct AppleSpeechSession;

// Speech recognition through macOS's own Speech framework (SFSpeechRecognizer).
// Alone among Mudlet's backends this one needs no library and no model on
// disk: the engine ships with the operating system, so it is usable the moment
// the player grants permission.
//
// Recognition is pinned on-device (requiresOnDeviceRecognition), which is what
// makes the onDevice capability - docs/stt-api.md's promise that audio never
// leaves the machine - true for this backend rather than aspirational. It is
// deliberately not configurable: a switch here would let a profile silently
// start streaming a player's microphone to Apple.
//
// Apple's newer SpeechAnalyzer/SpeechTranscriber is the better engine, but it
// is Swift-only - the macOS 26.5 SDK exposes no Objective-C interface to it -
// and Mudlet has no Swift in its build. SFSpeechRecognizer is what is
// reachable, and it predates the 13.0 deployment target, so no availability
// guarding is needed anywhere below.

class AppleSpeechRecognizer : public SpeechRecognizer
{
    Q_OBJECT

public:
    explicit AppleSpeechRecognizer(QObject* parent = nullptr);
    ~AppleSpeechRecognizer() override;

    // The path is accepted and ignored: this engine has no model on disk, and
    // taking the argument anyway keeps one stt.init() call working across
    // every backend.
    bool initialize(const QString& modelPath) override;

    // Fixed for the engine rather than read from a model, so unlike the other
    // backends there is nothing here to announce a change in.
    // - biasing: contextualStrings on each request
    // - wordResults: SFTranscriptionSegment carries substring, timestamp,
    //   duration and confidence
    // - onDevice: requiresOnDeviceRecognition is set unconditionally, and a
    //   locale that cannot honour it is refused rather than quietly sent to
    //   Apple's servers
    // - grammar: there is no grammar-constraint API to back the claim
    Capabilities capabilities() const override
    {
        Capabilities answer;
        answer.biasing = true;
        answer.wordResults = true;
        answer.onDevice = true;
        return answer;
    }

    void setSilenceTimeout(int msec) override;
    int silenceTimeout() const override;

    float audioLevel() const override { return listening() ? mRecentAudioLevel : 0.0f; }
    bool hasLiveNativeResources() const override;
    void releaseResources() override;

    // Always empty, and never anything else: a package that gates its setup on
    // a model path must not be sent off to install a model that does not exist
    // for this engine.
    QString modelPath() const override { return QString(); }

    // Answered plainly rather than gated on a live handle the way the
    // model-loading backends gate theirs: the locale is a setting here, not a
    // property of something loaded, and it outlives releaseResources().
    QString currentLanguage() const override { return mCurrentLanguage; }
    bool setLanguage(const QString& languageCode) override;

    QString backendName() const override { return qsl("Apple Speech"); }
    // The engine ships with the system and carries no version of its own, so
    // the system's version is the only honest answer.
    QString backendVersion() const override;

    // Refused rather than remembered: the framework decides end-of-speech
    // itself and exposes no way to tune it, so agreeing here would hand the
    // caller a readback that disagrees with the engine.
    bool setSensitivity(Sensitivity sensitivity) override;
    Sensitivity sensitivity() const override { return Sensitivity::Default; }

    // Whether the system recognizer exists and is usable for the current
    // locale. Deliberately not gated on authorisation: a player who has not
    // been asked yet still has a working backend.
    static bool speechAvailable();

protected:
    VocabularyResult applyVocabulary(const QStringList& words) override;

    void doStartListening() override;
    void doStopListening() override;
    void doCancel() override;

private slots:
    // Consumes 16kHz mono Int16 PCM from the shared capture component
    void slot_pcmReady(const QByteArray& pcmData);
    void slot_captureError(const QString& message);

private:
    // The second half of doStartListening()'s permission work, reached either
    // directly (speech recognition already authorised) or from the
    // authorisation callback
    void requestMicrophoneThenStart();
    void startListeningInternal();

    // Build a request and hand it to a fresh task. Every utterance gets its
    // own: the framework ends a task at the first final result, so continuing
    // to listen means starting another.
    bool beginRecognitionTask();
    // Drop the request and task, voiding anything the old one still has in
    // flight
    void releaseTask();
    void cancelTask();

    // Marshalled onto the main thread out of the task's result handler, which
    // Apple calls on an arbitrary queue. generation identifies the task it
    // came from, so a result outrunning a cancel is discarded rather than
    // reported over a session nobody is waiting on.
    void handleRecognition(quint64 generation, const QString& text, bool finalUtterance, const QVariantList& words, const QString& failure);
    // A task is over, for good or ill: start the next one if this session is
    // still listening, settle to Ready if it was finishing, and stop
    // altogether if tasks are failing the instant they start.
    void handleTaskEnded(const QString& failure);

    float calculateAudioLevel(const QByteArray& data) const;

    AppleSpeechSession* mpSession = nullptr;

    // Shared microphone capture and resampling; delivers ready-to-decode PCM
    SpeechAudioCapture* mpCapture = nullptr;

    QString mCurrentLanguage;
    QString mLastPartialResult;

    // Smoothed microphone level, reported so a consumer can tell a phrase the
    // engine misheard from one the microphone barely received
    float mRecentAudioLevel = 0.0f;

    // Bumped every time a task is dropped, so a late callback from it can tell
    // that it is answering for a session that has moved on
    quint64 mTaskGeneration = 0;

    // How long the current task has been running, and how many in a row have
    // died immediately. Only an instant death counts: a task that ran for a
    // while and then reported "no speech detected" is the framework's normal
    // way of ending a quiet stretch, and treating that as a fault would end
    // listening every time nobody spoke.
    QElapsedTimer mTaskLifetime;
    int mConsecutiveImmediateFailures = 0;
    static constexpr int scmMinTaskLifetimeMsec = 500;
    static constexpr int scmMaxImmediateFailures = 3;

    // How long to wait for the last utterance after endAudio before giving up
    // on it. Without this a framework that never delivers the final result
    // would leave the recognizer in Processing for ever, and nothing else
    // would ever be allowed to start.
    static constexpr int scmFinaliseTimeoutMsec = 3000;
};

#endif // MUDLET_APPLESPEECHRECOGNIZER_H
