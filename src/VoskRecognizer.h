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

#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QLibrary>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVariantList>

// Forward declarations for Vosk types (opaque pointers)
struct VoskModel;
struct VoskRecognizer;

// Helper class for audio input buffering in push mode
class AudioInputBuffer : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioInputBuffer(QObject* parent = nullptr)
    : QIODevice(parent)
    {
    }

    bool open(OpenMode mode) override
    {
        setOpenMode(mode);
        mBuffer.clear();
        mTotalBytesWritten = 0;
        return true;
    }

    void close() override
    {
        mBuffer.clear();
        QIODevice::close();
    }

    // Sequential device - audio is a stream
    bool isSequential() const override { return true; }

    qint64 readData(char* data, qint64 maxlen) override
    {
        Q_UNUSED(data);
        Q_UNUSED(maxlen);
        return 0; // Push mode - we don't read from here
    }

    qint64 writeData(const char* data, qint64 len) override;

    QByteArray takeAll()
    {
        QByteArray result = mBuffer;
        mBuffer.clear();
        return result;
    }

    qint64 bytesAvailable() const override { return mBuffer.size(); }

    qint64 totalBytesWritten() const { return mTotalBytesWritten; }

signals:
    void dataAvailable(const QByteArray& data);

private:
    QByteArray mBuffer;
    qint64 mTotalBytesWritten = 0;
};

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

    State state() const override { return mState; }

    QStringList availableLanguages() const override;
    QString currentLanguage() const override { return mCurrentLanguage; }
    bool setLanguage(const QString& languageCode) override;

    QString backendName() const override { return QStringLiteral("Vosk"); }
    QString backendVersion() const override;
    bool isBackendAvailable() const override;

    // SpeechRecognizer sensitivity interface (maps to EndpointerMode)
    void setSensitivity(Sensitivity sensitivity) override;
    Sensitivity sensitivity() const override;

    // SpeechRecognizer words interface
    void setWordsEnabled(bool enabled) override;
    bool wordsEnabled() const override { return mWordsEnabled; }

    // Vosk-specific: Finer-grained control over end-of-speech detection
    void setEndpointerMode(EndpointerMode mode);
    EndpointerMode endpointerMode() const { return mEndpointerMode; }

    // Static method to check if Vosk library is available on this system
    static bool isVoskAvailable();

    // Check if the Vosk library is available (can be loaded)
    static bool isLibraryAvailable();

    // Reset library load state to allow re-checking (e.g., after installation)
    static void resetLibraryLoadState();

    // Get the list of paths where Vosk library is searched
    static QStringList librarySearchPaths();

    // Get the default model path for the current platform
    static QString defaultModelPath();

    // Get the recommended model download URL for a language
    static QString modelDownloadUrl(const QString& languageCode);

    // Model selection and management
    // Get the path to the currently selected model (from settings, or auto-detect best available)
    static QString getSelectedModelPath();

    // Set the selected model path (saves to settings)
    static void setSelectedModelPath(const QString& modelPath);

    // Get the base directory where models are stored
    static QString modelsDirectoryPath();

    // Get list of installed models (directory names)
    static QStringList getInstalledModels();

    // Get the "best" available model (prefers larger models over smaller ones)
    static QString getBestAvailableModel();

private slots:
    void processAudioData();
    void processAudioDataFromBuffer(const QByteArray& data);
    void handleAudioStateChanged(QAudio::State newState);

private:
    // Load the Vosk library dynamically
    bool loadVoskLibrary();

    // Release Vosk resources
    void releaseVoskResources();

    // Set up audio input from the microphone
    bool setupAudioInput();

    // Internal method that actually starts listening (called after permission check)
    void startListeningInternal();

    // Calculate audio level for visual feedback
    float calculateAudioLevel(const QByteArray& data) const;

    // State management
    void setState(State newState);

    // Find an installed model path for a given language code
    QString findModelPathForLanguage(const QString& languageCode) const;

    // Member variables
    State mState = State::Uninitialized;
    QString mModelPath;
    QString mCurrentLanguage;

    // Vosk handles (opaque pointers)
    VoskModel* mVoskModel = nullptr;
    ::VoskRecognizer* mVoskRecognizer = nullptr;

    // Audio capture
    QPointer<QAudioSource> mAudioSource;
    QPointer<QIODevice> mAudioDevice;
    QAudioFormat mAudioFormat;                     // Vosk's expected format (16kHz mono Int16)
    QAudioFormat mActualAudioFormat;               // Device's actual capture format
    AudioInputBuffer* mAudioInputBuffer = nullptr; // For push mode

    // Timer for periodic audio processing
    QTimer mProcessTimer;

    // Audio buffer
    QByteArray mAudioBuffer;

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
    using vosk_recognizer_new_fn = ::VoskRecognizer* (*)(VoskModel*, float);
    using vosk_recognizer_free_fn = void (*)(::VoskRecognizer*);
    using vosk_recognizer_accept_waveform_fn = int (*)(::VoskRecognizer*, const char*, int);
    using vosk_recognizer_result_fn = const char* (*)(::VoskRecognizer*);
    using vosk_recognizer_partial_result_fn = const char* (*)(::VoskRecognizer*);
    using vosk_recognizer_final_result_fn = const char* (*)(::VoskRecognizer*);
    using vosk_recognizer_reset_fn = void (*)(::VoskRecognizer*);
    using vosk_set_log_level_fn = void (*)(int);
    using vosk_recognizer_set_endpointer_mode_fn = void (*)(::VoskRecognizer*, int);
    using vosk_recognizer_set_words_fn = void (*)(::VoskRecognizer*, int);

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

    // Settings
    EndpointerMode mEndpointerMode = EndpointerMode::Default;
    bool mWordsEnabled = false;
};

#endif // MUDLET_VOSKRECOGNIZER_H
