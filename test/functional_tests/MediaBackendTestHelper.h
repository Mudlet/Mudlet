#ifndef MUDLET_MEDIABACKENDTESTHELPER_H
#define MUDLET_MEDIABACKENDTESTHELPER_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QAudioOutput>
#include <QDeadlineTimer>
#include <QMediaPlayer>
#include <QUrl>
#include <QtTest/QtTest>

#include <chrono>

#include "utils.h"

// A skip is how the media tests stay honest on a backend that cannot stage what they need. It is
// also how they could all go green everywhere and mean nothing, if a CI image lost its
// codecs or swapped its default backend. Runners known to carry a backend
// that can demonstrate everything here set MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK, which turns
// every capability skip into a failure and makes that loss a red build instead of silence.
#define SKIP_OR_FAIL_WITHOUT(reason)                                                                                                                                                                   \
    do {                                                                                                                                                                                               \
        const QString incapable = (reason);                                                                                                                                                            \
        if (!incapable.isEmpty()) {                                                                                                                                                                    \
            if (qEnvironmentVariableIsSet("MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK")) {                                                                                                                    \
                QFAIL(qPrintable(qsl("MUDLET_MEDIA_TESTS_REQUIRE_PLAYBACK is set for this runner, so a backend that cannot do this is a failure and "                                                  \
                                     "not a skip: %1")                                                                                                                                                 \
                                         .arg(incapable)));                                                                                                                                            \
            }                                                                                                                                                                                          \
            QSKIP(qPrintable(incapable));                                                                                                                                                              \
        }                                                                                                                                                                                              \
    } while (false)

// Empty when the backend reports the undecodable file at path as an error, as TMedia needs it to
// before it has any failure to act on; otherwise why that cannot be observed here.
inline QString undecodableFileUnreportedReason(const QString& path)
{
    using namespace std::chrono_literals;

    QMediaPlayer probe;
    auto* output = new QAudioOutput(&probe);
    output->setMuted(true);
    probe.setAudioOutput(output);

    bool sawError = false;
    QObject::connect(&probe, &QMediaPlayer::errorOccurred, &probe, [&](QMediaPlayer::Error error, const QString&) {
        if (error != QMediaPlayer::NoError) {
            sawError = true;
        }
    });

    probe.setSource(QUrl::fromLocalFile(path));
    probe.play();

    const bool reported = QTest::qWaitFor(
            [&]() {
                return sawError;
            },
            QDeadlineTimer(10s));
    probe.stop();

    if (reported) {
        return QString();
    }
    return qsl("This Qt Multimedia backend does not report an error for an undecodable file within 10s, so there is no failure for TMedia to act on. Backend: \"%1\", final media status: %2.")
            .arg(QString::fromLocal8Bit(qgetenv("QT_MEDIA_BACKEND")), QString::number(static_cast<int>(probe.mediaStatus())));
}

#endif // MUDLET_MEDIABACKENDTESTHELPER_H
