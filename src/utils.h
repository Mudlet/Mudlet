#ifndef MUDLET_UTILS_H
#define MUDLET_UTILS_H

/***************************************************************************
 *   Copyright (C) 2021 by Vadim Peretokin - vperetokin@hey.com            *
 *   Copyright (C) 2021, 2023, 2025-2026 by Stephen Lyons                  *
 *                                               - slysven@virginmedia.com *
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

#include <QDateTime>
#include <QEnterEvent>
#include <QString>

#include <cstdint>
#include <cstring>

#define qsl(s) QStringLiteral(s)

// user-defined literals to represent kilobytes and megabytes
// C++ standard requires unsigned long long parameter for integer literal operators
constexpr auto operator""_KB(unsigned long long const x) -> int64_t // NOLINT(runtime/int)
{
    return 1024LL * x;
}

constexpr auto operator""_MB(unsigned long long const x) -> int64_t // NOLINT(runtime/int)
{
    return 1024LL * 1024LL * x;
}

using TEnterEvent = QEnterEvent;

using NameGroupMatches = QVector<QPair<QString, QString>>;

// Common enum for specifying insertion mode for tree items
// Used across all editor item types (triggers, aliases, timers, scripts, actions, keys)
enum class TreeItemInsertMode {
    Append,      // Add to end of parent's child list
    AtPosition   // Insert at specific position
};

class utils
{
public:
    // Safe string copy: copies up to destSize-1 bytes and always null-terminates.
    // Returns the number of bytes copied (excluding null terminator).
    static size_t copyString(char* dest, size_t destSize, const char* src, size_t srcLen)
    {
        if (destSize == 0) {
            return 0;
        }
        const size_t copyLen = (srcLen < destSize) ? srcLen : destSize - 1;
        std::memcpy(dest, src, copyLen);
        dest[copyLen] = '\0';
        return copyLen;
    }

    // As copyString(), but for UTF-8 data that has to stay valid UTF-8: the copy
    // stops at the last character that fits whole rather than at the last byte,
    // so no trailing half-character is left behind. Use it wherever a truncated
    // copy is handed on to something that decodes it - Discord discards an
    // entire presence frame whose JSON payload carries an incomplete sequence.
    // Returns the number of bytes copied (excluding the null terminator).
    static size_t copyUtf8String(char* dest, size_t destSize, const char* src, size_t srcLen)
    {
        if (destSize == 0) {
            return 0;
        }
        size_t copyLen = (srcLen < destSize) ? srcLen : destSize - 1;
        // Every byte after the first of a multi-byte character has the form
        // 10xxxxxx, so a cut in front of one is a cut inside a character: walk
        // back to where that character starts. A cut that took everything (or
        // that landed on a character start) needs no adjustment.
        while (copyLen > 0 && copyLen < srcLen && (static_cast<unsigned char>(src[copyLen]) & 0xC0u) == 0x80u) {
            --copyLen;
        }
        std::memcpy(dest, src, copyLen);
        dest[copyLen] = '\0';
        return copyLen;
    }

    // This construct will be very useful for formatting tooltips and by
    // defining a static function/method here we can save using the same
    // qsl all over the place:
    static QString richText(const QString& text) { return qsl("<p>%1</p>").arg(text); }

    // Qt 6.9 deprecated QDateTime::setOffsetFromUtc(int) and made it hard to
    // replicate the exact strings that we had before:
    static QString dateStamp() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        auto localNow = QDateTime::currentDateTime();
        const int offset = localNow.offsetFromUtc();
        if (offset) {
            unsigned hoursOff = abs(offset/3600);
            unsigned minutesOff = (abs(offset) - hoursOff * 3600) / 60;
            return localNow.toString(Qt::ISODate).append(qsl("%1%2:%3")
                                                                 .arg(offset >= 0 ? QLatin1Char('+') : QLatin1Char('-'))
                                                                 .arg(hoursOff, 2, 10, QLatin1Char('0'))
                                                                 .arg(minutesOff, 2, 10, QLatin1Char('0')));
        }
        return localNow.toString(Qt::ISODate).append(qsl("+00:00"));
#else
        auto localNow = QDateTime::currentDateTime();
        const int offset = localNow.offsetFromUtc();
        localNow.setOffsetFromUtc(offset);
        return localNow.toString(Qt::ISODate);
#endif
    }
};

#endif // MUDLET_UTILS_H
