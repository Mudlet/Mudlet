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

#ifndef IRC_BUFFER_H
#define IRC_BUFFER_H

#include <ircglobal.h>
#include <QStringList>
#include <QObject>

namespace Irc
{
    class Session;
    class BufferPrivate;

    class IRC_EXPORT Buffer : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString receiver READ receiver)
        Q_PROPERTY(QString topic READ topic)
        Q_PROPERTY(QStringList names READ names)
        Q_FLAGS(MessageFlags)
        Q_ENUMS(MessageFlag)

    public:
        ~Buffer();

        Session* session() const;

        QString receiver() const;
        QString topic() const;
        QStringList names() const;
        QString modes(const QString& name) const;
        QString visualMode(const QString& name) const;

        enum MessageFlag
        {
            NoFlags = 0x0,
            IdentifiedFlag = 0x1,
            EchoFlag = 0x2,
        };
        Q_DECLARE_FLAGS(MessageFlags, MessageFlag)

    public Q_SLOTS:
        bool message(const QString& message);
        bool notice(const QString& notice);
        bool ctcpAction(const QString& action);

    Q_SIGNALS:
        void receiverChanged(const QString& receiver);
        void motdReceived(const QString& motd);
        void joined(const QString& origin);
        void parted(const QString& origin, const QString& message);
        void quit(const QString& origin, const QString& message);
        void nickChanged(const QString& origin, const QString& nick);
        void modeChanged(const QString& origin, const QString& mode, const QString& args);
        void topicChanged(const QString& origin, const QString& topic);
        void invited(const QString& origin, const QString& receiver, const QString& channel);
        void kicked(const QString& origin, const QString& nick, const QString& message);
        void messageReceived(const QString& origin, const QString& message,
                             Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags);
        void noticeReceived(const QString& origin, const QString& notice,
                            Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags);
        void ctcpRequestReceived(const QString& origin, const QString& request,
                                 Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags);
        void ctcpReplyReceived(const QString& origin, const QString& reply,
                               Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags);
        void ctcpActionReceived(const QString& origin, const QString& action,
                                Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags);
        void numericMessageReceived(const QString& origin, uint code, const QStringList& params);
        void unknownMessageReceived(const QString& origin, const QStringList& params);

    protected:
        Buffer(const QString& receiver, Session* parent = 0);
        Buffer(BufferPrivate& dd, const QString& receiver, Session* parent = 0);
        BufferPrivate* const d_ptr;

    private:
        Q_DECLARE_PRIVATE(Buffer)
        Q_DISABLE_COPY(Buffer)
        friend class SessionPrivate;
        friend class Session;
    };
}

#ifndef QT_NO_DEBUG_STREAM
IRC_EXPORT QDebug operator<<(QDebug debug, const Irc::Buffer* buffer);
#endif // QT_NO_DEBUG_STREAM

Q_DECLARE_OPERATORS_FOR_FLAGS(Irc::Buffer::MessageFlags)

#endif // IRC_BUFFER_H
