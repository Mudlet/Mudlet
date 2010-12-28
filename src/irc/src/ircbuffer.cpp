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

#include "ircbuffer.h"
#include "ircbuffer_p.h"
#include "ircsession.h"
#include "ircsession_p.h"

/*!
    \class Irc::Buffer ircbuffer.h
    \brief The Irc::Buffer class provides an IRC buffer.

    The Irc::Buffer class acts as a buffer for a single receiver. Receivers
    can be:
    \li the server
    \li channels
    \li queries.

    Server/channel/query specific messages are delivered to the corresponding
    buffer.

    \note Buffers are not intended to be instantiated directly, but via the
    virtual factory method Irc::Session::createBuffer().
 */

/*!
    \enum Irc::Buffer::MessageFlag

    This enum describes available message flags.
 */

/*!
    \var Irc::Buffer::NoFlags

    No flags for the message.
 */

/*!
    \var Irc::Buffer::IdentifiedFlag

    Message was sent from an identified nick.
 */

/*!
    \var Irc::Buffer::EchoFlag

    Message echoed back from library.
 */


/*!
    \fn void Irc::Buffer::receiverChanged(const QString& receiver)

    This signal is emitted whenever \a receiver has changed.
 */

/*!
    \fn void Irc::Buffer::motdReceived(const QString& motd)

    This signal is emitted when message of the day \a motd has been received.
 */

/*!
    \fn void Irc::Buffer::joined(const QString& origin)

    This signal is emitted when \a origin has joined.
 */

/*!
    \fn void Irc::Buffer::parted(const QString& origin, const QString& message)

    This signal is emitted when \a origin has parted with \a message.
 */

/*!
    \fn void Irc::Buffer::quit(const QString& origin, const QString& message)

    This signal is emitted when \a origin has quit with \a message.
 */

/*!
    \fn void Irc::Buffer::nickChanged(const QString& origin, const QString& nick)

    This signal is emitted when \a origin has changed \a nick.
 */

/*!
    \fn void Irc::Buffer::modeChanged(const QString& origin, const QString& mode, const QString& args)

    This signal is emitted when \a origin has changed \a mode with \a args.
 */

/*!
    \fn void Irc::Buffer::topicChanged(const QString& origin, const QString& topic)

    This signal is emitted when \a origin has changed \a topic.
 */

/*!
    \fn void Irc::Buffer::invited(const QString& origin, const QString& receiver, const QString& channel)

    This signal is emitted when \a origin has invited \a receiver to \a channel.
 */

/*!
    \fn void Irc::Buffer::kicked(const QString& origin, const QString& nick, const QString& message)

    This signal is emitted when \a origin has kicked \a nick with \a message.
 */

/*!
    \fn void Irc::Buffer::messageReceived(const QString& origin, const QString& message, Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags)

    This signal is emitted when \a origin has sent \a message.
 */

/*!
    \fn void Irc::Buffer::noticeReceived(const QString& origin, const QString& notice, Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags)

    This signal is emitted when \a origin has sent \a notice.
 */

/*!
    \fn void Irc::Buffer::ctcpRequestReceived(const QString& origin, const QString& request, Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags)

    This signal is emitted when \a origin has sent a CTCP \a request.
 */

/*!
    \fn void Irc::Buffer::ctcpReplyReceived(const QString& origin, const QString& reply, Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags)

    This signal is emitted when \a origin has sent a CTCP \a reply.
 */

/*!
    \fn void Irc::Buffer::ctcpActionReceived(const QString& origin, const QString& action, Irc::Buffer::MessageFlags flags = Irc::Buffer::NoFlags)

    This signal is emitted when \a origin has sent a CTCP \a action.
 */

/*!
    \fn void Irc::Buffer::numericMessageReceived(const QString& origin, uint code, const QStringList& params)

    This signal is emitted when \a origin has sent a numeric message with \a code and \a params.
 */

/*!
    \fn void Irc::Buffer::unknownMessageReceived(const QString& origin, const QStringList& params)

    This signal is emitted when \a origin has sent an unknown message with \a params.
 */

namespace Irc
{
    BufferPrivate::BufferPrivate() :
        q_ptr(0)
    {
    }

    void BufferPrivate::addName(QString name)
    {
        QString mode;
        if (name.startsWith(QLatin1Char('@')))
        {
            mode = QLatin1Char('o');
            name = name.remove(0, 1);
        }
        else if (name.startsWith(QLatin1Char('+')))
        {
            mode = QLatin1Char('v');
            name = name.remove(0, 1);
        }
        names.insert(name, mode);
    }

    void BufferPrivate::removeName(const QString& name)
    {
        names.remove(name);
    }

    void BufferPrivate::updateMode(const QString& name, const QString& mode)
    {
        bool add = true;
        QString updated = names.value(name);
        for (int i = 0; i < mode.size(); ++i)
        {
            QChar c = mode.at(i);
            switch (c.toAscii())
            {
                case '+':
                    add = true;
                    break;
                case '-':
                    add = false;
                    break;
                default:
                    if (add)
                    {
                        if (!updated.contains(c))
                            updated += c;
                    }
                    else
                    {
                        updated.remove(c);
                    }
                    break;
            }
        }
        names.insert(name, updated);
    }

    void BufferPrivate::setReceiver(const QString& rec, bool replace)
    {
        Q_Q(Buffer);
        if (receiver != rec)
        {
            Session* s = q->session();
            if (s)
            {
                if (replace)
                    s->d_func()->buffers.remove(receiver);
                s->d_func()->buffers.insert(rec, q);
            }
            receiver = rec;
            emit q->receiverChanged(receiver);
        }
    }

    /*!
        Constructs a new IRC buffer with \a receiver and \a parent.

        \sa Session::createBuffer()
     */
    Buffer::Buffer(const QString& receiver, Session* parent) : QObject(parent), d_ptr(new BufferPrivate)
    {
        Q_D(Buffer);
        d->q_ptr = this;
        d->receiver = receiver;
    }

    Buffer::Buffer(BufferPrivate& dd, const QString& receiver, Session* parent) : QObject(parent), d_ptr(&dd)
    {
        Q_D(Buffer);
        d->q_ptr = this;
        d->receiver = receiver;
    }

    /*!
        Destructs the IRC buffer.
     */
    Buffer::~Buffer()
    {
        Session* s = session();
        if (s)
            s->d_func()->removeBuffer(this);

        Q_D(Buffer);
        delete d;
    }

    /*!
        Returns the session.
     */
    Session* Buffer::session() const
    {
        return qobject_cast<Session*>(parent());
    }

    /*!
        Returns the receiver.
     */
    QString Buffer::receiver() const
    {
        Q_D(const Buffer);
        return d->receiver;
    }

    /*!
        Returns the topic.
     */
    QString Buffer::topic() const
    {
        Q_D(const Buffer);
        return d->topic;
    }

    /*!
        Returns the names.
     */
    QStringList Buffer::names() const
    {
        Q_D(const Buffer);
        return d->names.keys();
    }

    /*!
        Returns the modes of \a name.
     */
    QString Buffer::modes(const QString& name) const
    {
        Q_D(const Buffer);
        return d->names.value(name);
    }

    /*!
        Returns the visual mode of \a name.
     */
    QString Buffer::visualMode(const QString& name) const
    {
        Q_D(const Buffer);
        QString modes = d->names.value(name);
        if (modes.contains(QLatin1Char('o')))
            return QLatin1String("@");
        if (modes.contains(QLatin1Char('v')))
            return QLatin1String("+");
        return QString();
    }

    /*!
        This convenience function sends a \a message to the buffer's receiver.

        \sa Session::message()
     */
    bool Buffer::message(const QString& message)
    {
        Q_D(Buffer);
        Session* s = session();
        return s && s->message(d->receiver, message);
    }

    /*!
        This convenience function sends a \a notice to the buffer's receiver.

        \sa Session::notice()
     */
    bool Buffer::notice(const QString& notice)
    {
        Q_D(Buffer);
        Session* s = session();
        return s && s->notice(d->receiver, notice);
    }

    /*!
        This convenience function sends a CTCP \a action to the buffers' receiver.

        \sa Session::ctcpAction()
     */
    bool Buffer::ctcpAction(const QString& action)
    {
        Q_D(Buffer);
        Session* s = session();
        return s && s->ctcpAction(d->receiver, action);
    }
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const Irc::Buffer* buffer)
{
    if (!buffer)
        return debug << "Irc::Buffer(0x0) ";
    debug.nospace() << buffer->metaObject()->className() << '(' << (void*) buffer;
    if (!buffer->objectName().isEmpty())
        debug << ", name = " << buffer->objectName();
    if (!buffer->receiver().isEmpty())
        debug << ", receiver = " << buffer->receiver();
    debug << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircbuffer.cpp"
