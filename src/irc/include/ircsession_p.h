/*
* Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/

#ifndef IRC_SESSION_P_H
#define IRC_SESSION_P_H

#include <QHash>
#include <QTimer>
#include <QStringList>
#include <QVariantList>
#include <QAbstractSocket>
#include "ircbuffer.h"

namespace Irc
{
    class SessionPrivate
    {
        Q_DECLARE_PUBLIC(Session)

    public:
        SessionPrivate();

        void init(Session* session);

        void _q_connected();
        void _q_disconnected();
        void _q_reconnect();
        void _q_error();
        void _q_state(QAbstractSocket::SocketState state);
        void _q_readData();

        QString readString(const QByteArray& data) const;
        void readLines(const QByteArray& delimiter);
        void processLine(const QByteArray& line);
        Irc::Buffer::MessageFlags getMessageFlags(QString& message) const;
        bool isConnected() const;
        QString resolveTarget(const QString& sender, const QString& receiver) const;

        Buffer* createBuffer(const QString& receiver);
        void removeBuffer(Buffer* buffer);

        Session* q_ptr;
        QByteArray buffer;
        Session::Options options;
        QAbstractSocket* socket;
        QString ident;
        QString password;
        QString nick;
        QString realName;
        QString host;
        quint16 port;
        QString motd;
        QStringList channels;
        QByteArray encoding;
        int delay;
        QTimer timer;
        Buffer* defaultBuffer;
        QHash<QString, Buffer*> buffers;
        bool welcomed;

        // Capabilities supported by the server
        QStringList capabilities;
        // Capabilities enabled for this connection
        QStringList enabledCapabilities;
        // Capabilities to request only when connecting
        QStringList wantedCapabilities;
        // Temporary building list of capabilities in transmission
        QStringList tempCapabilities;
        // Whether the server has CAP implemented
        bool        capabilitiesSupported;

#ifndef IRC_NO_DEPRECATED
        // TODO: for backwards compatibility, to be removed in 1.0
        void emitCompatSignal(const char* signal, const QVariantList& params);
        void _q_joined(const QString& origin);
        void _q_parted(const QString& origin, const QString& message);
        void _q_quit(const QString& origin, const QString& message);
        void _q_nickChanged(const QString& origin, const QString& nick);
        void _q_modeChanged(const QString& origin, const QString& mode, const QString& args);
        void _q_topicChanged(const QString& origin, const QString& topic);
        void _q_invited(const QString& origin, const QString& receiver, const QString& channel);
        void _q_kicked(const QString& origin, const QString& nick, const QString& message);
        void _q_messageReceived(const QString& origin, const QString& message);
        void _q_noticeReceived(const QString& origin, const QString& notice);
        void _q_ctcpRequestReceived(const QString& origin, const QString& request);
        void _q_ctcpReplyReceived(const QString& origin, const QString& reply);
        void _q_ctcpActionReceived(const QString& origin, const QString& action);
        void _q_numericMessageReceived(const QString& origin, uint code, const QStringList& params);
        void _q_unknownMessageReceived(const QString& origin, const QStringList& params);
#endif // IRC_NO_DEPRECATED
    };
}

#endif // IRC_BUFFER_P_H
