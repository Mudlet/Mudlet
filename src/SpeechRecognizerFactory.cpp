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
        if (!VoskRecognizer::libraryAvailable()) {
            qWarning() << "SpeechRecognizerFactory: Vosk backend requested but not available";
            return nullptr;
        }
        return new VoskRecognizer(parent);

    case Backend::Whisper:
        // The Whisper backend has no implementation yet
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

    if (VoskRecognizer::libraryAvailable()) {
        backends.append(Backend::Vosk);
    }

    // Future: Check for Whisper availability
    // Future: Check for platform API availability

    return backends;
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

    case Backend::Whisper:
        // The Whisper backend has no implementation yet
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
