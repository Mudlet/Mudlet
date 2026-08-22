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

#ifndef MUDLET_SPEECHRECOGNIZER_H
#define MUDLET_SPEECHRECOGNIZER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

// Abstract base class for speech recognition backends.
// This abstraction allows swapping between different speech recognition
// engines (Vosk, Whisper, platform APIs) without changing UI or Lua code.

class SpeechRecognizer : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Uninitialized, // No model loaded
        Ready,         // Model loaded, not listening
        // Asked to listen, not yet listening: something outside this process
        // has to answer first, and the answer is not synchronous. Permission
        // to use a microphone is the usual reason - macOS asks the player the
        // first time, and a browser asks on every session - which is why this
        // is a state of the contract rather than a quirk of one platform. A
        // consumer shows "waiting", and a second request while here is
        // refused rather than asked twice.
        Starting,
        Listening,  // Actively capturing and processing audio
        Processing, // Processing final audio after stop
        Error       // An error occurred
    };
    Q_ENUM(State)

    explicit SpeechRecognizer(QObject* parent = nullptr)
    : QObject(parent)
    {
    }
    ~SpeechRecognizer() override = default;

    SpeechRecognizer(const SpeechRecognizer&) = delete;
    SpeechRecognizer& operator=(const SpeechRecognizer&) = delete;

    // === Core Operations ===

    // Initialize the recognizer with a model. Returns true on success.
    // @param modelPath: Path to the speech recognition model directory
    virtual bool initialize(const QString& modelPath) = 0;

    // Start listening for speech input from the microphone.
    // Emits partialResult() as speech is recognized in real-time.
    // Emits finalResult() when an utterance is complete.
    virtual void startListening() = 0;

    // Stop listening and process any remaining audio.
    // Will emit finalResult() with any pending recognition.
    virtual void stopListening() = 0;

    // Cancel listening without processing remaining audio.
    // No finalResult() will be emitted.
    virtual void cancel() = 0;

    // Stop listening automatically after this many milliseconds of continuous
    // silence, finalising the utterance as stopListening() would. 0 (the
    // default) keeps listening open-ended. Backends without the capability
    // ignore it and keep reporting 0.
    virtual void setSilenceTimeout(int msec) { Q_UNUSED(msec) }
    virtual int silenceTimeout() const { return 0; }

    // === Capabilities ===
    // What this backend can actually do, so consumers adapt instead of
    // guessing. Answered as a group rather than one virtual at a time because
    // they change together and, on some backends, change at all: whether
    // recognition can be biased is a property of the loaded model rather than
    // of the engine, so a backend re-reads them when a model loads and says so
    // through capabilitiesChanged().
    struct Capabilities
    {
        // Recognition can be biased toward a supplied vocabulary
        bool biasing = false;
        // Recognition can be constrained to a supplied grammar
        bool grammar = false;
        // Results carry per-word confidence and timing (wordsResult signal)
        bool wordResults = false;
        // Audio is processed locally. False is the honest default: a backend
        // that has not said otherwise has not promised recordings stay on this
        // machine, and inheriting that promise by omission is the one way this
        // particular flag must never be wrong.
        bool onDevice = false;

        bool operator==(const Capabilities&) const = default;
    };

    virtual Capabilities capabilities() const { return {}; }

    // Named readers over the same answers, for a caller that wants one of
    // them. Not virtual: a backend overrides capabilities() alone, so the
    // group and the individual answers cannot come to disagree.
    bool supportsBiasing() const { return capabilities().biasing; }
    bool supportsGrammar() const { return capabilities().grammar; }
    bool supportsWordResults() const { return capabilities().wordResults; }
    bool onDevice() const { return capabilities().onDevice; }

    // What became of a vocabulary offered to the engine. Three outcomes, not
    // two: a backend that cannot use vocabulary at all is a different matter
    // from one that can and failed this time, and collapsing them tells a
    // consumer to fall back to correcting results itself when what actually
    // happened was a fault worth reporting.
    enum class VocabularyResult {
        Applied,     // In effect; the engine is biasing toward these words
        Unsupported, // This backend cannot use vocabulary - correct client-side
        Failed       // It can, and this attempt did not work
    };
    Q_ENUM(VocabularyResult)

    // Supply vocabulary for biasing or grammar constraint. The default
    // matches the default capabilities: nothing applied, because nothing can be.
    virtual VocabularyResult setVocabulary(const QStringList& words)
    {
        Q_UNUSED(words)
        return VocabularyResult::Unsupported;
    }

    // Level of the audio last received from the microphone, 0.0 to 1.0, or 0
    // while not listening. Reported so a consumer can tell a phrase the engine
    // misheard from one the microphone barely received - two failures that
    // look identical in the results and need opposite remedies.
    virtual float audioLevel() const { return 0.0f; }

    // === State Queries ===

    // The recognizer's state, and the only truth about it: there is no
    // parallel "active" flag anywhere. Held here rather than by each backend
    // so the rule below - a change is announced exactly once, and only when it
    // is a change - holds for every implementation instead of being
    // re-established correctly by each one.
    State state() const { return mState; }

    // Convenience methods
    bool listening() const { return state() == State::Listening; }
    bool ready() const { return state() == State::Ready; }
    bool initialized() const { return state() != State::Uninitialized && state() != State::Error; }
    // Asked to listen and not yet refused: either already listening, or still
    // waiting on the permission that decides it
    bool starting() const { return state() == State::Starting; }

    // Whether the backend currently holds live native resources (e.g. handles
    // into a dynamically-loaded library) that must be released before that
    // library can be safely unloaded. Defaults to false for backends with no
    // such resources.
    virtual bool hasLiveNativeResources() const { return false; }

    // Release any native resources held by the backend and return to an
    // uninitialized state. Default is a no-op for backends holding nothing.
    virtual void releaseResources() {}

    // Path of the model the backend is currently working from. Empty when the
    // backend has never been given one, or has no concept of a model on disk.
    virtual QString modelPath() const { return QString(); }

    // === Language/Model Support ===

    virtual QString currentLanguage() const = 0;

    // Set the recognition language. Returns true on success.
    // May require model reload depending on backend.
    virtual bool setLanguage(const QString& languageCode) = 0;

    // === Backend Information ===

    // Get human-readable name of the backend (e.g., "Vosk", "Whisper")
    virtual QString backendName() const = 0;

    virtual QString backendVersion() const = 0;

    // Check if the backend is available (library loaded, etc.)
    virtual bool backendAvailable() const = 0;

    // === Recognition Settings ===

    // Sensitivity mode controls how quickly the recognizer detects end of speech.
    // Short: Fast detection, good for commands (may cut off longer phrases)
    // Default: Balanced for typical use
    // Long: Slower detection, good for dictation (waits longer before ending)
    enum class Sensitivity {
        Short,   // Fast end-of-speech detection (commands)
        Default, // Balanced (typical use)
        Long     // Slow end-of-speech detection (dictation)
    };
    Q_ENUM(Sensitivity)

    // Set how quickly end-of-speech is detected
    // False when this engine cannot tune end-of-speech detection at all, so a
    // caller is told rather than being handed a readback that agrees with the
    // request and disagrees with the engine
    virtual bool setSensitivity(Sensitivity sensitivity) = 0;
    virtual Sensitivity sensitivity() const = 0;

protected:
    // Move to a new state, announcing it only when it differs from the
    // current one. Backends transition through this rather than assigning, so
    // no consumer sees a stateChanged that changed nothing, or misses one
    // that did.
    void setState(State newState)
    {
        if (mState == newState) {
            return;
        }
        mState = newState;
        emit stateChanged(newState);
    }

signals:
    // Emitted during recognition with partial (non-final) text.
    // This text may change as more audio is processed.
    void partialResult(const QString& text);

    // Emitted when an utterance is complete with the final transcription.
    void finalResult(const QString& text);

    // Emitted alongside each final result on backends whose
    // supportsWordResults() is true. Each word is a QVariantMap with keys
    // "word", "start", "end", "conf".
    void wordsResult(const QVariantList& words);

    void stateChanged(SpeechRecognizer::State newState);

    // Emitted when what the backend can do changes, which is when a model is
    // loaded or replaced. Without it a consumer that read the capabilities
    // once - the obvious thing to do - would go on believing them after they
    // stopped being true.
    void capabilitiesChanged(SpeechRecognizer::Capabilities newCapabilities);

    void errorOccurred(const QString& errorMessage);

private:
    State mState = State::Uninitialized;
};

#endif // MUDLET_SPEECHRECOGNIZER_H
