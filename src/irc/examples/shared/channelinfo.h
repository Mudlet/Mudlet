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

#ifndef CHANNELINFO_H
#define CHANNELINFO_H

#include <QString>
#include <QMetaType>
#include <QDataStream>
#include "streamer.h"

struct ChannelInfo
{
    QString channel;
    QString key;
};
Q_DECLARE_METATYPE(ChannelInfo);

inline QDataStream& operator<<(QDataStream& out, const ChannelInfo& channel)
{
    out << quint32(123); // version
    out << channel.channel;
    out << channel.key;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, ChannelInfo& channel)
{
    quint32 version = readStreamValue<quint32>(in, 0);
    channel.channel = readStreamValue<QString>(in, channel.channel);
    channel.key = readStreamValue<QString>(in, channel.key);
    Q_UNUSED(version);
    return in;
}

typedef QList<ChannelInfo> ChannelInfos;
Q_DECLARE_METATYPE(ChannelInfos);

#endif // CHANNELINFO_H
