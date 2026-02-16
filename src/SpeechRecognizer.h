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

// Abstract base class for speech recognition backends.
// This abstraction allows swapping between different speech recognition
// engines (Vosk, Whisper, platform APIs) without changing UI or Lua code.

class SpeechRecognizer : public QObject
{
    Q_OBJECT

public:
    // Recognition engine state
    enum class State {
        Uninitialized,  // No model loaded
        Ready,          // Model loaded, not listening
        Listening,      // Actively capturing and processing audio
        Processing,     // Processing final audio after stop
        Error           // An error occurred
    };
    Q_ENUM(State)

    explicit SpeechRecognizer(QObject* parent = nullptr)
        : QObject(parent)
    {}
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

    // === State Queries ===

    // Get current state of the recognizer
    virtual State state() const = 0;

    // Convenience methods
    bool isListening() const { return state() == State::Listening; }
    bool isReady() const { return state() == State::Ready; }
    bool isInitialized() const { return state() != State::Uninitialized && state() != State::Error; }

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

signals:
    // Emitted during recognition with partial (non-final) text.
    // This text may change as more audio is processed.
    void partialResult(const QString& text);

    // Emitted when an utterance is complete with the final transcription.
    void finalResult(const QString& text);

    // Emitted when the recognizer state changes.
    void stateChanged(SpeechRecognizer::State newState);

    // Emitted when an error occurs.
    void errorOccurred(const QString& errorMessage);

    // Emitted periodically with the current audio input level (0.0 to 1.0).
    // Useful for visual feedback (e.g., microphone level indicator).
    void audioLevelChanged(float level);
};

#endif // MUDLET_SPEECHRECOGNIZER_H
