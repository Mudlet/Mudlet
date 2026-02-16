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

#ifndef MUDLET_SPEECHRECOGNIZERFACTORY_H
#define MUDLET_SPEECHRECOGNIZERFACTORY_H

#include <QList>
#include <QObject>
#include <QString>

class SpeechRecognizer;

// Factory class for creating speech recognition backends.
// Provides a unified way to instantiate different recognition engines
// and query which backends are available on the current system.

class SpeechRecognizerFactory
{
public:
    // Available speech recognition backends
    enum class Backend {
        Vosk,       // Offline recognition using Vosk/Kaldi
        Whisper,    // Offline recognition using whisper.cpp (future)
        Platform,   // Platform-native APIs: macOS Speech, Windows SAPI (future)
        Auto        // Automatically select best available backend
    };

    // Create a speech recognizer with the specified backend.
    // Returns nullptr if the backend is not available.
    // The caller takes ownership of the returned object.
    // @param backend: The backend to use, or Auto to select automatically
    // @param parent: Optional QObject parent for memory management
    static SpeechRecognizer* create(Backend backend = Backend::Auto, QObject* parent = nullptr);

    // Get list of backends that are available on this system.
    // A backend is available if its library is loaded and functional.
    static QList<Backend> availableBackends();

    // Check if a specific backend is available.
    static bool isBackendAvailable(Backend backend);

    // Get human-readable display name for a backend.
    static QString backendDisplayName(Backend backend);

    // Get short identifier for a backend (for settings storage).
    static QString backendIdentifier(Backend backend);

    // Parse a backend identifier string back to enum.
    // Returns Auto if the identifier is not recognized.
    static Backend backendFromIdentifier(const QString& identifier);

private:
    SpeechRecognizerFactory() = default;
};

#endif // MUDLET_SPEECHRECOGNIZERFACTORY_H
