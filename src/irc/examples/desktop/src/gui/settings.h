/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QHash>
#include <QVariant>
#include <QMetaType>
#include "streamer.h"

struct Settings
{
    Settings();
    operator QVariant() const;
    bool operator==(const Settings& other) const;
    bool operator!=(const Settings& other) const;

    enum MessageType
    {
        Joins,
        Parts,
        Nicks,
        Modes,
        Kicks,
        Quits,
        Topics
    };

    enum ColorType
    {
        Background,
        Message,
        Event,
        Notice,
        Action,
        Highlight,
        TimeStamp,
        Link
    };

    enum ShortcutType
    {
        NavigateUp,
        NavigateDown,
        NavigateLeft,
        NavigateRight,
        NextUnreadUp,
        NextUnreadDown,
        NextUnreadLeft,
        NextUnreadRight
    };

    QHash<int, bool> messages;
    QHash<int, bool> highlights;
    QHash<int, QString> colors;
    QHash<int, QString> shortcuts;
    QString language;
    QString font;
    int maxBlockCount;
    bool timeStamp;
    QString layout;
    bool stripNicks;
};
Q_DECLARE_METATYPE(Settings);

inline QDataStream& operator<<(QDataStream& out, const Settings& settings)
{
    out << quint32(129); // version
    out << settings.messages;
    out << settings.highlights;
    out << settings.language;
    out << settings.font;
    out << settings.colors;
    out << settings.shortcuts;
    out << settings.maxBlockCount;
    out << settings.timeStamp;
    out << settings.layout;
    out << settings.stripNicks;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, Settings& settings)
{
    quint32 version = readStreamValue<quint32>(in, 0);
    settings.messages = readStreamValue< QHash<int, bool> >(in, settings.messages);
    settings.highlights = readStreamValue< QHash<int, bool> >(in, settings.highlights);
    settings.language = readStreamValue<QString>(in, settings.language);
    settings.font = readStreamValue<QString>(in, settings.font);
    settings.colors = readStreamValue< QHash<int, QString> >(in, settings.colors);
    settings.shortcuts = readStreamValue< QHash<int, QString> >(in, settings.shortcuts);
    settings.maxBlockCount = readStreamValue<int>(in, settings.maxBlockCount);
    settings.timeStamp = readStreamValue<bool>(in, settings.timeStamp);
    settings.layout = readStreamValue<QString>(in, settings.layout);
    settings.stripNicks = readStreamValue<bool>(in, settings.stripNicks);

    Settings defaults; // default values
    if (version < 125)
        settings.colors[Settings::TimeStamp] = defaults.colors.value(Settings::TimeStamp);
    if (version < 126)
        settings.layout = defaults.layout;
    if (version < 127)
    {
        settings.shortcuts[Settings::NextUnreadUp] = defaults.shortcuts.value(Settings::NextUnreadUp);
        settings.shortcuts[Settings::NextUnreadDown] = defaults.shortcuts.value(Settings::NextUnreadDown);
        settings.shortcuts[Settings::NextUnreadLeft] = defaults.shortcuts.value(Settings::NextUnreadLeft);
        settings.shortcuts[Settings::NextUnreadRight] = defaults.shortcuts.value(Settings::NextUnreadRight);
    }
    if (version < 128)
        settings.colors[Settings::Link] = defaults.colors.value(Settings::Link);
    if (version < 129)
        settings.stripNicks = defaults.stripNicks;
    return in;
}

#endif // SETTINGS_H
