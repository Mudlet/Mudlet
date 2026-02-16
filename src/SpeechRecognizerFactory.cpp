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

#include "SpeechRecognizerFactory.h"
#include "SpeechRecognizer.h"
#include "VoskRecognizer.h"

SpeechRecognizer* SpeechRecognizerFactory::create(Backend backend, QObject* parent)
{
    // Handle Auto selection - pick the first available backend
    if (backend == Backend::Auto) {
        const auto backends = availableBackends();
        if (backends.isEmpty()) {
            return nullptr;
        }
        backend = backends.first();
    }

    switch (backend) {
    case Backend::Vosk:
        return new VoskRecognizer(parent);

    case Backend::Whisper:
        // Future: return new WhisperRecognizer(parent);
        return nullptr;

    case Backend::Platform:
        // Future: return platform-specific implementation
        return nullptr;

    case Backend::Auto:
        // Already handled above, but needed for compiler warning
        return nullptr;
    }

    return nullptr;
}

QList<SpeechRecognizerFactory::Backend> SpeechRecognizerFactory::availableBackends()
{
    QList<Backend> backends;

    if (VoskRecognizer::isVoskAvailable()) {
        backends.append(Backend::Vosk);
    }

    // Future: Check for Whisper availability
    // Future: Check for platform API availability

    return backends;
}

bool SpeechRecognizerFactory::isBackendAvailable(Backend backend)
{
    switch (backend) {
    case Backend::Vosk:
        return VoskRecognizer::isVoskAvailable();

    case Backend::Whisper:
        return false; // Not yet implemented

    case Backend::Platform:
        return false; // Not yet implemented

    case Backend::Auto:
        return !availableBackends().isEmpty();
    }

    return false;
}

QString SpeechRecognizerFactory::backendDisplayName(Backend backend)
{
    switch (backend) {
    case Backend::Vosk:
        return QObject::tr("Vosk (Offline)");
    case Backend::Whisper:
        return QObject::tr("Whisper (Offline)");
    case Backend::Platform:
#if defined(Q_OS_MACOS)
        return QObject::tr("macOS Speech Recognition");
#elif defined(Q_OS_WIN)
        return QObject::tr("Windows Speech Recognition");
#else
        return QObject::tr("Platform Speech Recognition");
#endif
    case Backend::Auto:
        return QObject::tr("Automatic");
    }

    return QObject::tr("Unknown");
}

QString SpeechRecognizerFactory::backendIdentifier(Backend backend)
{
    switch (backend) {
    case Backend::Vosk:
        return QStringLiteral("vosk");
    case Backend::Whisper:
        return QStringLiteral("whisper");
    case Backend::Platform:
        return QStringLiteral("platform");
    case Backend::Auto:
        return QStringLiteral("auto");
    }

    return QStringLiteral("auto");
}

SpeechRecognizerFactory::Backend SpeechRecognizerFactory::backendFromIdentifier(const QString& identifier)
{
    if (identifier == QLatin1String("vosk")) {
        return Backend::Vosk;
    }
    if (identifier == QLatin1String("whisper")) {
        return Backend::Whisper;
    }
    if (identifier == QLatin1String("platform")) {
        return Backend::Platform;
    }

    return Backend::Auto;
}
