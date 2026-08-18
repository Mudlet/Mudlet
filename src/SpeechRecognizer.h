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
    // Recognition engine state
    enum class State {
        Uninitialized, // No model loaded
        Ready,         // Model loaded, not listening
        Listening,     // Actively capturing and processing audio
        Processing,    // Processing final audio after stop
        Error          // An error occurred
    };
    Q_ENUM(State)

    explicit SpeechRecognizer(QObject* parent = nullptr)
    : QObject(parent)
    {
    }
    ~SpeechRecognizer() override = default;

    // Prevent copying
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

    // Abandon the phrase currently being decoded and begin a new one, while
    // staying in Listening. Used when the text recognised so far has been
    // committed elsewhere - without it the backend keeps reporting that phrase,
    // and those results arrive as if they were newly spoken.
    // No finalResult() is emitted for the abandoned phrase.
    virtual void resetUtterance() {}

    // Stop listening automatically after this many milliseconds of continuous
    // silence, finalising the utterance as stopListening() would. 0 (the
    // default) keeps listening open-ended. Backends without the capability
    // ignore it and keep reporting 0.
    virtual void setSilenceTimeout(int msec) { Q_UNUSED(msec) }
    virtual int silenceTimeout() const { return 0; }

    // === Capabilities ===
    // What this backend can actually do, so consumers adapt instead of
    // guessing. Defaults describe the least capable backend; overrides only
    // claim what the implementation genuinely delivers.

    // Recognition can be biased toward a supplied vocabulary
    virtual bool supportsBiasing() const { return false; }
    // Recognition can be constrained to a supplied grammar
    virtual bool supportsGrammar() const { return false; }
    // Results carry per-word confidence and timing (wordsResult signal)
    virtual bool supportsWordResults() const { return false; }
    // Audio is processed locally; false means an off-device service is
    // involved and no privacy guarantee about audio leaving the machine holds
    virtual bool onDevice() const { return true; }

    // Supply vocabulary for biasing or grammar constraint. Returns true only
    // when the backend applied it; false means the words were ignored and a
    // consumer should rely on client-side correction instead. The default
    // matches the default capabilities: nothing applied.
    virtual bool setVocabulary(const QStringList& words)
    {
        Q_UNUSED(words)
        return false;
    }

    // Level of the audio last received from the microphone, 0.0 to 1.0, or 0
    // while not listening. Reported so a consumer can tell a phrase the engine
    // misheard from one the microphone barely received - two failures that
    // look identical in the results and need opposite remedies.
    virtual float audioLevel() const { return 0.0f; }

    // === State Queries ===

    // Get current state of the recognizer
    virtual State state() const = 0;

    // Convenience methods
    bool isListening() const { return state() == State::Listening; }
    bool isReady() const { return state() == State::Ready; }
    bool isInitialized() const { return state() != State::Uninitialized && state() != State::Error; }

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

    // Get list of available language codes (e.g., "en-US", "de-DE")
    virtual QStringList availableLanguages() const = 0;

    // Get currently selected language code
    virtual QString currentLanguage() const = 0;

    // Set the recognition language. Returns true on success.
    // May require model reload depending on backend.
    virtual bool setLanguage(const QString& languageCode) = 0;

    // === Backend Information ===

    // Get human-readable name of the backend (e.g., "Vosk", "Whisper")
    virtual QString backendName() const = 0;

    // Get version string of the backend library
    virtual QString backendVersion() const = 0;

    // Check if the backend is available (library loaded, etc.)
    virtual bool isBackendAvailable() const = 0;

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
    virtual void setSensitivity(Sensitivity sensitivity) = 0;
    virtual Sensitivity sensitivity() const = 0;

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

    // Emitted when the recognizer state changes.
    void stateChanged(SpeechRecognizer::State newState);

    // Emitted when an error occurs.
    void errorOccurred(const QString& errorMessage);

    // Emitted periodically with the current audio input level (0.0 to 1.0).
    // Useful for visual feedback (e.g., microphone level indicator).
    void audioLevelChanged(float level);
};

#endif // MUDLET_SPEECHRECOGNIZER_H
