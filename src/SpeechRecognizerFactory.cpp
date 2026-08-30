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

#include "utils.h"
#include "SherpaRecognizer.h"
#include "SpeechRecognizer.h"
#include "VoskRecognizer.h"

#include <QDir>

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
        if (!VoskRecognizer::libraryAvailable()) {
            qWarning() << "SpeechRecognizerFactory: Vosk backend requested but not available";
            return nullptr;
        }
        return new VoskRecognizer(parent);

    case Backend::Sherpa:
        if (!SherpaRecognizer::sherpaAvailable()) {
            qWarning() << "SpeechRecognizerFactory: sherpa-onnx backend requested but not available";
            return nullptr;
        }
        return new SherpaRecognizer(parent);

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

    // sherpa first: it is the actively released engine (weekly-ish releases),
    // where Vosk has not shipped since v0.3.50 in April 2024. Auto resolves to
    // availableBackends().first(), so this order is what makes Auto prefer
    // sherpa over Vosk when both are installed.
    if (SherpaRecognizer::sherpaAvailable()) {
        backends.append(Backend::Sherpa);
    }

    if (VoskRecognizer::libraryAvailable()) {
        backends.append(Backend::Vosk);
    }

    // Future: Check for Whisper availability
    // Future: Check for platform API availability

    return backends;
}

bool SpeechRecognizerFactory::backendAvailable(Backend backend)
{
    switch (backend) {
    case Backend::Vosk:
        return VoskRecognizer::libraryAvailable();

    case Backend::Sherpa:
        return SherpaRecognizer::sherpaAvailable();

    case Backend::Whisper:
        return false; // Not yet implemented

    case Backend::Platform:
        return false; // Not yet implemented

    case Backend::Auto:
        return !availableBackends().isEmpty();
    }

    return false;
}

QString SpeechRecognizerFactory::backendIdentifier(Backend backend)
{
    switch (backend) {
    case Backend::Vosk:
        return qsl("vosk");
    case Backend::Sherpa:
        return qsl("sherpa");
    case Backend::Whisper:
        return qsl("whisper");
    case Backend::Platform:
        return qsl("platform");
    case Backend::Auto:
        return qsl("auto");
    }

    return qsl("auto");
}

SpeechRecognizerFactory::Backend SpeechRecognizerFactory::backendFromIdentifier(const QString& identifier)
{
    if (identifier == QLatin1String("vosk")) {
        return Backend::Vosk;
    }
    if (identifier == QLatin1String("sherpa")) {
        return Backend::Sherpa;
    }
    if (identifier == QLatin1String("whisper")) {
        return Backend::Whisper;
    }
    if (identifier == QLatin1String("platform")) {
        return Backend::Platform;
    }

    return Backend::Auto;
}

QString SpeechRecognizerFactory::defaultModelPath(Backend backend)
{
    // Handle Auto selection - pick the first available backend
    if (backend == Backend::Auto) {
        const auto backends = availableBackends();
        if (backends.isEmpty()) {
            return QString();
        }
        backend = backends.first();
    }

    switch (backend) {
    case Backend::Vosk:
        return VoskRecognizer::defaultModelPath();

    case Backend::Sherpa:
        return SherpaRecognizer::defaultModelPath();

    case Backend::Whisper:
        // Future: return WhisperRecognizer::defaultModelPath();
        return QString();

    case Backend::Platform:
        // Platform APIs typically don't use model paths
        return QString();

    case Backend::Auto:
        // Already handled above
        return QString();
    }

    return QString();
}

SpeechRecognizerFactory::Backend SpeechRecognizerFactory::backendForModelDir(const QString& modelPath)
{
    if (SherpaRecognizer::looksLikeModelDir(modelPath)) {
        return Backend::Sherpa;
    }

    // Vosk/Kaldi models carry their acoustic model in an "am" subdirectory
    const QDir modelDir(modelPath);
    if (modelDir.exists(qsl("am")) || modelDir.exists(qsl("conf"))) {
        return Backend::Vosk;
    }

    return Backend::Auto;
}
