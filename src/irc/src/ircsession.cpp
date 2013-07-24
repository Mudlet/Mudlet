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

#include "../include/ircsession.h"
#include "../include/ircsession_p.h"
#include "../include/ircbuffer.h"
#include "../include/ircbuffer_p.h"
#include "../include/ircutil.h"
#include <QSet>
#include <QTimer>
#include <QBuffer>
#include <QPointer>
#include <QTcpSocket>
#include <QTextCodec>
#include <QStringList>
#include <QMetaMethod>
#ifdef HAVE_ICU
#include <unicode/ucsdet.h>
#endif // HAVE_ICU

/*!
    \class Irc::Session ircsession.h
    \brief The Irc::Session class provides an IRC session.

    IRC (Internet Relay Chat protocol) is a simple communication protocol.
    Irc::Session provides means to establish a connection to an IRC server.
    Irc::Session works asynchronously. None of the functions block the
    calling thread but they return immediately and the actual work is done
    behind the scenes in the event loop.

    Example usage:
    \code
    Irc::Session* session = new Irc::Session(this);
    session->setNick("jpnurmi");
    session->setIdent("jpn");
    session->setRealName("J-P Nurmi");
    session->connectToServer("irc.freenode.net", 6667);
    \endcode

    \note Irc::Session supports SSL (Secure Sockets Layer) connections since version 0.3.0

    Example SSL usage:
    \code
    Irc::Session* session = new Irc::Session(this);
    // ...
    QSslSocket* socket = new QSslSocket(session);
    socket->ignoreSslErrors();
    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    session->setSocket(socket);
    session->connectToServer("irc.secure.ssl", 6669);
    \endcode

    \sa setSocket()
 */

/*! \enum Irc::Session::Option

    This enum describes special options that may be used by Irc::Session.
 */

/*! \var Irc::Session::NoOptions

    Clears options.
 */

/*! \var Irc::Session::StripNicks

    Strips origins automatically.

    For every IRC server message, the origin is sent in standard form:
    \code
    nick!ident@host
    \endcode
    i.e. like \e jpnurmi!jpnurmi\@xob.kapsi.fi. Such origins can not be used
    in IRC commands but need to be stripped (i.e. ident and host part
    should be cut off) before using. This can be done either explicitly,
    by calling Irc::Util::nickFromTarget(), or implicitly for all the
    messages - by setting this option with Irc::Session::setOptions().
 */

/*! \var Irc::Session::EchoMessages

    Echo own messages, notices and actions.

    IRC servers do not echo users own messages, notices or actions.
    This option makes Irc::Session to echo them to ease up IRC client
    implementation.
 */

/*!
    \fn void Irc::Session::connected()

    This signal is emitted after a connection has been successfully established.
 */

/*!
    \fn void Irc::Session::welcomed()

    This signal is emitted when the welcome message has been received.

    \sa Irc::Rfc::RPL_WELCOME
 */

/*!
    \fn void Irc::Session::reconnecting()

    This signal is emitted when the session is about to reconnect.
 */

/*!
    \fn void Irc::Session::disconnected()

    This signal is emitted when the session has been disconnected.
 */

/*!
    \fn void Irc::Session::bufferAdded(Irc::Buffer* buffer)

    This signal is emitted whenever a \a buffer was added.
 */

/*!
    \fn void Irc::Session::bufferRemoved(Irc::Buffer* buffer)

    This signal is emitted whenever a \a buffer was removed.
 */

/*!
    \fn void Irc::Session::capabilitiesListed(const QStringList& capabilities)

    This signal is emitted when the library receives a list of
    supported capabilities for this session.

    \sa requestCapabilities()
 */

/*!
    \fn void Irc::Session::capabilitiesAcked(const QStringList& capabilities)

    This signal is emitted when the server acknowledges the use of
    a previously requested list of capabilities. Note that if you
    request a list of capabilities, either all of them will be
    acked, or all of them will be nacked.
 */

/*!
    \fn void Irc::Session::capabilitiesNotAcked(const QStringList& capabilities)

    This signal is emitted when the server disacknowledges enabling
    a previously requested list of capabilities. This means that none
    of the requested capabilities will be enabled, even if some of them
    were valid. (Any previously enabled capabilities will still be
    enabled.)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgJoined ( const QString &origin, const QString &channel)

    \deprecated Use Irc::Buffer::joined(const QString& origin)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgParted (const QString &origin, const QString &channel, const QString &message)

    \deprecated Use Irc::Buffer::parted(const QString& origin, const QString& message)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgQuit (const QString &origin, const QString &message)

    \deprecated Use Irc::Buffer::quit(const QString& origin, const QString& message)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgNickChanged (const QString &origin, const QString &nick)

    \deprecated Use Irc::Buffer::nickChanged(const QString& origin, const QString& nick)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgModeChanged (const QString &origin, const QString &receiver, const QString &mode, const QString &args)

    \deprecated Use Irc::Buffer::modeChanged(const QString& origin, const QString& mode, const QString& args)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgTopicChanged (const QString &origin, const QString &channel, const QString &topic)

    \deprecated Use Irc::Buffer::topicChanged(const QString& origin, const QString& topic)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgInvited (const QString &origin, const QString &receiver, const QString &channel)

    \deprecated Use Irc::Buffer::invited(const QString& origin, const QString& receiver, const QString& channel)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgKicked (const QString &origin, const QString &channel, const QString &nick, const QString &message)

    \deprecated Use Irc::Buffer::kicked(const QString& origin, const QString& nick, const QString& message)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgMessageReceived (const QString &origin, const QString &receiver, const QString &message)

    \deprecated Use Irc::Buffer::messageReceived(const QString& origin, const QString& message)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgNoticeReceived (const QString &origin, const QString &receiver, const QString &notice)

    \deprecated Use Irc::Buffer::noticeReceived(const QString& origin, const QString& notice)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgCtcpRequestReceived (const QString &origin, const QString &request)

    \deprecated Use Irc::Buffer::ctcpRequestReceived(const QString& origin, const QString& request)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgCtcpReplyReceived (const QString &origin, const QString &reply)

    \deprecated Use Irc::Buffer::ctcpReplyReceived(const QString& origin, const QString& reply)
 */

 /*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgCtcpActionReceived (const QString &origin, const QString &receiver, const QString &action)

    \deprecated Use Irc::Buffer::ctcpActionReceived(const QString& origin, const QString& action)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgNumericMessageReceived (const QString &origin, uint code, const QStringList &params)

    \deprecated Use Irc::Buffer::numericMessageReceived(const QString& origin, uint code, const QStringList& params)
 */

/*!
    \fn Q_DECL_DEPRECATED void Irc::Session::msgUnknownMessageReceived (const QString &origin, const QStringList &params)

    \deprecated Use Irc::Buffer::unknownMessageReceived(const QString& origin, const QStringList& params)
*/

static QByteArray detectEncoding(const QByteArray& text)
{
    Q_UNUSED(text);
    QByteArray encoding;
#ifdef HAVE_ICU
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector* detector = ucsdet_open(&status);
    if (detector && !U_FAILURE(status))
    {
        ucsdet_setText(detector, text.constData(), text.length(), &status);
        if (!U_FAILURE(status))
        {
            const UCharsetMatch* match = ucsdet_detect(detector, &status);
            if (match && !U_FAILURE(status))
                encoding = ucsdet_getName(match, &status);
        }
    }

    if (U_FAILURE(status)) {
        qWarning("detectEncoding() failed: %s", u_errorName(status));
    }

    ucsdet_close(detector);
#endif // HAVE_ICU
    return encoding;
}

namespace Irc
{
    SessionPrivate::SessionPrivate() :
        q_ptr(0),
        buffer(),
        options(Session::StripNicks | Session::EchoMessages),
        socket(0),
        ident(QLatin1String("libircclient-qt")),
        password(),
        nick(QLatin1String("libircclient")),
        realName(QLatin1String("libircclient-qt")),
        host(),
        port(6667),
        motd(),
        channels(),
        encoding(),
        delay(-1),
        timer(),
        defaultBuffer(),
        buffers(),
        welcomed(false),
        capabilitiesSupported(false)
    {
    }

    void SessionPrivate::init(Session* session)
    {
        q_ptr = session;

        Q_Q(Session);
        q->setSocket(new QTcpSocket(q));
        q->connect(&timer, SIGNAL(timeout()), q, SLOT(_q_reconnect()));
    }

    void SessionPrivate::_q_connected()
    {
        Q_Q(Session);

        Q_ASSERT(!nick.isEmpty());
        Q_ASSERT(!ident.isEmpty());

        // stop autoreconnecting...
        if (timer.isActive())
            timer.stop();

        // Send CAP LS first; if the server understands it this will
        // temporarily pause the handshake until CAP END is sent, so we
        // know whether the server supports the CAP extension.
        socket->write("CAP LS\r\n");

        if (!password.isEmpty())
            socket->write(QString(QLatin1String("PASS %1\r\n")).arg(password).toLocal8Bit());

        socket->write(QString(QLatin1String("NICK %1\r\n")).arg(nick).toLocal8Bit());

        // RFC 1459 states that "hostname and servername are normally
        // ignored by the IRC server when the USER command comes from
        // a directly connected client (for security reasons)", therefore
        // we don't need them.
        socket->write(QString(QLatin1String("USER %1 unknown unknown :%2\r\n")).arg(ident, realName).toLocal8Bit());

        defaultBuffer = createBuffer(host);
        emit q->connected();
        welcomed = false;
        capabilitiesSupported = false;
    }

    void SessionPrivate::_q_disconnected()
    {
        Q_Q(Session);
        emit q->disconnected();
    }

    void SessionPrivate::_q_reconnect()
    {
        if (socket)
        {
            socket->connectToHost(host, port);
            if (socket->inherits("QSslSocket"))
                QMetaObject::invokeMethod(socket, "startClientEncryption");
        }

        // stop autoreconnecting...
        if (timer.isActive())
            timer.stop();
    }

    void SessionPrivate::_q_error()
    {
        Q_Q(Session);
        // start reconnecting...
        if (delay >= 0)
        {
            timer.start(delay * 1000);
            emit q->reconnecting();
        }
    }

    void SessionPrivate::_q_state(QAbstractSocket::SocketState state)
    {
    }

    void SessionPrivate::_q_readData()
    {
        buffer += socket->readAll();
        // try reading RFC compliant message lines first
        readLines("\r\n");
        // fall back to RFC incompliant lines...
        readLines("\n");
    }

    QString SessionPrivate::readString(const QByteArray& data) const
    {
        QByteArray enc = encoding;
        if (enc.isNull())
            enc = detectEncoding(data);
        QTextCodec *codec = QTextCodec::codecForName(enc);
        if (!codec)
            codec = QTextCodec::codecForLocale();
        return codec->toUnicode(data);
    }

    void SessionPrivate::readLines(const QByteArray& delimiter)
    {
        int i = -1;
        while ((i = buffer.indexOf(delimiter)) != -1)
        {
            QByteArray line = buffer.left(i).trimmed();
            buffer = buffer.mid(i + delimiter.length());
            if (!line.isEmpty())
                processLine(line);
        }
    }

    void SessionPrivate::processLine(const QByteArray& line)
    {
        Q_Q(Session);
        QString process = readString(line);
        QString prefix, command;
        QStringList params;

        // From RFC 1459:
        //  <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
        //  <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
        //  <command>  ::= <letter> { <letter> } | <number> <number> <number>
        //  <SPACE>    ::= ' ' { ' ' }
        //  <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
        //  <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
        //                 or NUL or CR or LF, the first of which may not be ':'>
        //  <trailing> ::= <Any, possibly *empty*, sequence of octets not including
        //                   NUL or CR or LF>

        // parse <prefix>
        if (process.startsWith(QLatin1Char(':')))
        {
            prefix = process.mid(1, process.indexOf(QLatin1Char(' ')) - 1);
            process.remove(0, prefix.length() + 2);

            if (options & Session::StripNicks)
            {
                int index = prefix.indexOf(QRegExp(QLatin1String("[@!]")));
                if (index != -1)
                    prefix.truncate(index);
            }
        }

        // parse <command>
        command = process.mid(0, process.indexOf(QLatin1Char(' ')));
        process.remove(0, command.length() + 1);
        bool isNumeric = false;
        uint code = command.toInt(&isNumeric);

        // parse middle/params
        while (!process.isEmpty())
        {
            if (process.startsWith(QLatin1Char(':')))
            {
                process.remove(0, 1);
                params << process;
                process.clear();
            }
            else
            {
                QString param = process.mid(0, process.indexOf(QLatin1Char(' ')));
                process.remove(0, param.length() + 1);
                params << param;
            }
        }

        // handle PING/PONG
        if (command == QLatin1String("PING"))
        {
            QString arg = params.value(0);
            q->raw(QString(QLatin1String("PONG %1")).arg(arg));
            return;
        }

        // and dump
        if (isNumeric)
        {
            switch (code)
            {
                case Irc::Rfc::RPL_WELCOME:
                {
                    Q_ASSERT(defaultBuffer);
                    defaultBuffer->d_func()->setReceiver(prefix, false);

                    emit q->welcomed();
                    welcomed = true;

                    if (!capabilitiesSupported && !wantedCapabilities.isEmpty())
                        emit q->capabilitiesNotAcked(wantedCapabilities);
                    wantedCapabilities.clear();

                    break;
                }

                case Irc::Rfc::RPL_TOPIC:
                {
                    QString topic = params.value(1);
                    QString target = resolveTarget(QString(), topic);
                    Buffer* buffer = createBuffer(target);
                    buffer->d_func()->topic = topic;
                    break;
                }

                case Irc::Rfc::RPL_NAMREPLY:
                {
                    QStringList list = params;
                    list.removeAll(QLatin1String("="));
                    list.removeAll(QLatin1String("@"));
                    list.removeAll(QLatin1String("*"));

                    QString target = resolveTarget(QString(), list.value(1));
                    Buffer* buffer = createBuffer(target);
                    QStringList names = list.value(2).split(QLatin1String(" "), QString::SkipEmptyParts);
                    foreach (const QString& name, names)
                        buffer->d_func()->addName(name);
                    break;
                }

                case Irc::Rfc::RPL_MOTDSTART:
                    motd.clear();
                    break;

                case Irc::Rfc::RPL_MOTD:
                    motd.append(params.value(1) + QLatin1Char('\n'));
                    break;

                case Irc::Rfc::RPL_ENDOFMOTD:
                    if (defaultBuffer)
                        emit defaultBuffer->motdReceived(motd);
                    motd.clear();
                    break;

                default:
                    break;
            }

            if (code == Rfc::RPL_TOPICSET && options & Session::StripNicks)
            {
                QString user = params.value(2);
                int index = user.indexOf(QRegExp(QLatin1String("[@!]")));
                if (index != -1)
                {
                    user.truncate(index);
                    params.replace(2, user);
                }
            }

            if (defaultBuffer)
                emit defaultBuffer->numericMessageReceived(prefix, code, params);

            // join auto-join channels after MOTD
            if (code == Rfc::RPL_ENDOFMOTD || code == Rfc::ERR_NOMOTD)
            {
                foreach (const QString& channel, channels)
                    q->join(channel);
            }
        }
        else
        {
            if (command == QLatin1String("NICK"))
            {
                QString oldNick = Util::nickFromTarget(prefix);
                QString newNick = params.value(0);

                if (nick == oldNick)
                    nick = newNick;

                foreach (Buffer* buffer, buffers)
                {
                    if (buffer->receiver() == oldNick)
                        buffer->d_func()->setReceiver(newNick);

                    if (buffer->names().contains(oldNick))
                    {
                        buffer->d_func()->removeName(oldNick);
                        buffer->d_func()->addName(newNick);
                        emit buffer->nickChanged(prefix, newNick);
                    }
                }
            }
            else if (command == QLatin1String("QUIT"))
            {
                QString reason = params.value(0);
                QString nick = Util::nickFromTarget(prefix);
                foreach (Buffer* buffer, buffers)
                {
                    if (buffer->names().contains(nick))
                    {
                        buffer->d_func()->removeName(nick);
                        emit buffer->quit(prefix, reason);
                    }
                }
            }
            else if (command == QLatin1String("JOIN"))
            {
                QString channel = params.value(0);
                QString target = resolveTarget(prefix, channel);
                Buffer* buffer = createBuffer(target);
                buffer->d_func()->addName(Util::nickFromTarget(prefix));
                emit buffer->joined(prefix);
            }
            else if (command == QLatin1String("PART"))
            {
                QString channel = params.value(0);
                QString target = resolveTarget(prefix, channel);
                if (nick != Util::nickFromTarget(prefix))
                {
                    QString message = params.value(1);
                    Buffer* buffer = createBuffer(target);
                    buffer->d_func()->removeName(Util::nickFromTarget(prefix));
                    emit buffer->parted(prefix, message);
                }
                else if (buffers.contains(target))
                {
                    Buffer* buffer = buffers.value(target);
                    removeBuffer(buffer);
                    buffer->deleteLater();
                }
            }
            else if (command == QLatin1String("MODE"))
            {
                QString receiver = params.value(0);
                QString mode = params.value(1);
                QString args = params.value(2);
                QString target = resolveTarget(prefix, receiver);
                Buffer* buffer = createBuffer(target);
                buffer->d_func()->updateMode(args, mode);
                emit buffer->modeChanged(prefix, mode, args);
            }
            else if (command == QLatin1String("TOPIC"))
            {
                QString channel = params.value(0);
                QString topic = params.value(1);
                QString target = resolveTarget(prefix, channel);
                Buffer* buffer = createBuffer(target);
                buffer->d_func()->topic = topic;
                emit buffer->topicChanged(prefix, topic);
            }
            else if (command == QLatin1String("INVITE"))
            {
                QString receiver = params.value(0);
                QString channel = params.value(1);
                if (defaultBuffer)
                    emit defaultBuffer->invited(prefix, receiver, channel);
            }
            else if (command == QLatin1String("KICK"))
            {
                QString channel = params.value(0);
                QString nick = params.value(1);
                QString message = params.value(2);
                QString target = resolveTarget(prefix, channel);
                Buffer* buffer = createBuffer(target);
                buffer->d_func()->removeName(nick);
                emit buffer->kicked(prefix, nick, message);
            }
            else if (command == QLatin1String("PRIVMSG"))
            {
                QString message = params.value(1);

                Irc::Buffer::MessageFlags flags = getMessageFlags(message);

                if (message.startsWith(QLatin1Char('\1')) && message.endsWith(QLatin1Char('\1')))
                {
                    message.remove(0, 1);
                    message.remove(message.length() - 1, 1);

                    if (message.startsWith(QLatin1String("ACTION ")))
                    {
                        QString receiver = params.value(0);
                        QString target = resolveTarget(prefix, receiver);
                        Buffer* buffer = createBuffer(target);
                        emit buffer->ctcpActionReceived(prefix, message.mid(7), flags);
                    }
                    else
                    {
                        // TODO: check params
                        if (defaultBuffer)
                            emit defaultBuffer->ctcpRequestReceived(prefix, /*receiver,*/ message, flags);
                    }
                }
                else
                {
                    QString receiver = params.value(0);
                    QString target = resolveTarget(prefix, receiver);
                    Buffer* buffer = createBuffer(target);
                    emit buffer->messageReceived(prefix, message, flags);
                }
            }
            else if (command == QLatin1String("NOTICE"))
            {
                if (!welcomed)
                {
                    Q_ASSERT(defaultBuffer);
                    defaultBuffer->d_func()->setReceiver(prefix, false);
                }

                QString receiver = params.value(0);
                QString message = params.value(1);

                Irc::Buffer::MessageFlags flags = getMessageFlags(message);

                if (message.startsWith(QLatin1Char('\1')) && message.endsWith(QLatin1Char('\1')))
                {
                    message.remove(0, 1);
                    message.remove(message.length() - 1, 1);

                    // TODO: check params
                    if (defaultBuffer)
                        emit defaultBuffer->ctcpReplyReceived(prefix, /*receiver,*/ message, flags);
                }
                else
                {
                    QString target = resolveTarget(prefix, receiver);
                    Buffer* buffer = createBuffer(target);
                    emit buffer->noticeReceived(prefix, message, flags);
                }
            }
            else if (command == QLatin1String("KILL"))
            {
                ; // ignore this event - not all servers generate this
            }
            else if (command == QLatin1String("CAP"))
            {
                QString subcommand = params.at(1);
                bool endList = params.size() < 3
                            || params.at(2) != QLatin1String("*");

                QString rawCapabilities = params.at(params.size()-1);
                tempCapabilities.append(rawCapabilities.split(QLatin1String(" ")));

                if (subcommand == QLatin1String("LS") && endList)
                {
                    capabilities = tempCapabilities;

                    emit q->capabilitiesListed(capabilities);

                    // Request in-library supported capabilities right away
                    wantedCapabilities.append(QLatin1String("identify-msg"));

                    if (!welcomed)
                    {
                        capabilitiesSupported = true;
                        if (wantedCapabilities.isEmpty())
                            socket->write("CAP END\r\n");
                        else
                            q->requestCapabilities(wantedCapabilities);
                    }
                }
                else if (subcommand == QLatin1String("LIST") && endList)
                {
                    enabledCapabilities = tempCapabilities;
                }
                else if (subcommand == QLatin1String("ACK") && endList)
                {
                    foreach (const QString &cap, tempCapabilities)
                    {
                        if (cap.startsWith(QString::fromLatin1("-")))
                            // remove disabled capabilities from the list
                            for (int capi = 0; capi <
                                   enabledCapabilities.size(); ++capi)
                            {
                                if (enabledCapabilities.at(capi).compare(
                                        cap.mid(1), Qt::CaseInsensitive)
                                  == 0)
                                    enabledCapabilities.removeAt(capi);
                            }
                        else
                            enabledCapabilities.append(cap);
                    }

                    if (!welcomed)
                        socket->write("CAP END\r\n");
                    emit q->capabilitiesAcked(tempCapabilities);
                }
                else if (subcommand == QLatin1String("NAK") && endList)
                {
                    if (!welcomed)
                        socket->write("CAP END\r\n");
                    emit q->capabilitiesNotAcked(tempCapabilities);
                }

                tempCapabilities.clear();
            }
            else
            {
                // The "unknown" event is triggered upon receipt of any number of
                // unclassifiable miscellaneous messages, which aren't handled by
                // the library.
                if (defaultBuffer)
                    emit defaultBuffer->unknownMessageReceived(prefix, params);
            }
        }
    }

    Irc::Buffer::MessageFlags SessionPrivate::getMessageFlags(QString& message) const
    {
        Q_Q(const Session);
        if (q->capabilityEnabled(QLatin1String("identify-msg")))
        {
            const QChar identstate = message.at(0);
            message.remove(0, 1);
            switch (identstate.toLatin1())
            {
            case '+':
                return Irc::Buffer::IdentifiedFlag;
            case '-':
                break;
            default:
                qWarning() << "Unknown identification state character"
                              " in PRIVMSG with identify-msg enabled:"
                           << identstate;
            }
        }

        return Irc::Buffer::NoFlags;
    }

    bool SessionPrivate::isConnected() const
    {
        return socket &&
            (socket->state() == QAbstractSocket::ConnectingState
             || socket->state() == QAbstractSocket::ConnectedState);
    }

    QString SessionPrivate::resolveTarget(const QString& sender, const QString& receiver) const
    {
        QString target = receiver;

        if (target.contains(QLatin1Char('*')) || target.contains(QLatin1Char('?')))
            target = nick;

        if (target == nick)
        {
            if (target == sender)
                target = host;
            else
                target = sender;
        }

        if (target.isEmpty() || target == QLatin1String("AUTH"))
            target = host;

        return target;
    }

    Buffer* SessionPrivate::createBuffer(const QString& receiver)
    {
        Q_Q(Session);
        QString lower = receiver.toLower();
        QString lowerNick = Util::nickFromTarget(receiver).toLower();
        if (lower != lowerNick && buffers.contains(lowerNick))
        {
            Buffer* buffer = buffers.value(lowerNick);
            buffer->d_func()->setReceiver(lower);
            buffers.insert(lower, buffer);
            buffers.remove(lowerNick);
        }
        else if (!buffers.contains(lower) && !buffers.contains(lowerNick))
        {
            Buffer* buffer = q->createBuffer(receiver);
            buffers.insert(lower, buffer);
            emit q->bufferAdded(buffer);
        }
        return buffers.value(lower);
    }

    void SessionPrivate::removeBuffer(Buffer* buffer)
    {
        Q_Q(Session);
        QString lower = buffer->receiver().toLower();
        if (buffers.take(lower) == buffer)
            emit q->bufferRemoved(buffer);
    }

#ifndef IRC_NO_DEPRECATED
    // TODO: for backwards compatibility, to be removed in 1.0
    inline void SessionPrivate::emitCompatSignal(const char* signal, const QVariantList& params)
    {
        Q_Q(Session);
        if (q->receivers(signal)) {
            qWarning("IrcSession::%s [signal] is DEPRECATED", signal+1);
            QByteArray method(signal);
            method.remove(0, 1);
            method.truncate(method.indexOf("("));
            QMetaObject::invokeMethod(q, method,
                                      params.count() > 0 ? QGenericArgument(QMetaType::typeName(params.at(0).type()), &params[0]) : QGenericArgument(),
                                      params.count() > 1 ? QGenericArgument(QMetaType::typeName(params.at(1).type()), &params[1]) : QGenericArgument(),
                                      params.count() > 2 ? QGenericArgument(QMetaType::typeName(params.at(2).type()), &params[2]) : QGenericArgument(),
                                      params.count() > 3 ? QGenericArgument(QMetaType::typeName(params.at(3).type()), &params[3]) : QGenericArgument());
        }
    }
    void SessionPrivate::_q_joined(const QString& origin)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgJoined(QString,QString)), QVariantList() << origin << buffer->receiver());
    }
    void SessionPrivate::_q_parted(const QString& origin, const QString& message)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgParted(QString,QString,QString)), QVariantList() << origin << buffer->receiver() << message);
    }
    void SessionPrivate::_q_quit(const QString& origin, const QString& message)
    {
        emitCompatSignal(SIGNAL(msgQuit(QString,QString)), QVariantList() << origin << message);
    }
    void SessionPrivate::_q_nickChanged(const QString& origin, const QString& nick)
    {
        emitCompatSignal(SIGNAL(msgNickChanged(QString,QString)), QVariantList() << origin << nick);
    }
    void SessionPrivate::_q_modeChanged(const QString& origin, const QString& mode, const QString& args)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgModeChanged(QString,QString,QString,QString)), QVariantList() << origin << buffer->receiver() << mode << args);
    }
    void SessionPrivate::_q_topicChanged(const QString& origin, const QString& topic)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgTopicChanged(QString,QString,QString)), QVariantList() << origin << buffer->receiver() << topic);
    }
    void SessionPrivate::_q_invited(const QString& origin, const QString& receiver, const QString& channel)
    {
        emitCompatSignal(SIGNAL(msgInvited(QString,QString,QString)), QVariantList() << origin << receiver << channel);
    }
    void SessionPrivate::_q_kicked(const QString& origin, const QString& nick, const QString& message)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgKicked(QString,QString,QString,QString)), QVariantList() << origin << buffer->receiver() << nick << message);
    }
    void SessionPrivate::_q_messageReceived(const QString& origin, const QString& message)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgMessageReceived(QString,QString,QString)), QVariantList() << origin << buffer->receiver() << message);
    }
    void SessionPrivate::_q_noticeReceived(const QString& origin, const QString& notice)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgNoticeReceived(QString,QString,QString)), QVariantList() << origin << buffer->receiver() << notice);
    }
    void SessionPrivate::_q_ctcpRequestReceived(const QString& origin, const QString& request)
    {
        emitCompatSignal(SIGNAL(msgCtcpRequestReceived(QString,QString)), QVariantList() << origin << request);
    }
    void SessionPrivate::_q_ctcpReplyReceived(const QString& origin, const QString& reply)
    {
        emitCompatSignal(SIGNAL(msgCtcpReplyReceived(QString,QString)), QVariantList() << origin << reply);
    }
    void SessionPrivate::_q_ctcpActionReceived(const QString& origin, const QString& action)
    {
        Q_Q(Session);
        Buffer* buffer = qobject_cast<Buffer*>(q->sender());
        Q_ASSERT(buffer);
        emitCompatSignal(SIGNAL(msgCtcpActionReceived(QString,QString,QString)), QVariantList() << origin << buffer->receiver() << action);
    }
    void SessionPrivate::_q_numericMessageReceived(const QString& origin, uint code, const QStringList& params)
    {
        emitCompatSignal(SIGNAL(msgNumericMessageReceived(QString,uint,QStringList)), QVariantList() << origin << code << params);
    }
    void SessionPrivate::_q_unknownMessageReceived(const QString& origin, const QStringList& params)
    {
        emitCompatSignal(SIGNAL(msgUnknownMessageReceived(QString,QStringList)), QVariantList() << origin << params);
    }
#endif // IRC_NO_DEPRECATED

    /*!
        Constructs a new IRC session with \a parent.
     */
    Session::Session(QObject* parent) : QObject(parent), d_ptr(new SessionPrivate)
    {
        Q_D(Session);
        d->init(this);
    }

    /*!
        Destructs the IRC session.
     */
    Session::~Session()
    {
        Q_D(Session);
        if (d->socket)
            d->socket->close();
        delete d;
    }

    /*!
        Returns the list of auto-join channels.
     */
    QStringList Session::autoJoinChannels() const
    {
        Q_D(const Session);
        return d->channels;
    }

    /*!
        Adds \a channel to the list of auto-join channels.
     */
    void Session::addAutoJoinChannel(const QString& channel)
    {
        Q_D(Session);
        if (!d->channels.contains(channel, Qt::CaseInsensitive))
            d->channels.append(channel);
    }

    /*!
        Removes \a channels from the list of auto-join channels.
     */
    void Session::removeAutoJoinChannel(const QString& channel)
    {
        Q_D(Session);
        int index = d->channels.indexOf(QRegExp(channel, Qt::CaseInsensitive));
        if (index != -1)
            d->channels.removeAt(index);
    }

    /*!
        Sets the list of auto-join \a channels.
     */
    void Session::setAutoJoinChannels(const QStringList& channels)
    {
        Q_D(Session);
        d->channels = channels;
    }

    /*!
        Returns the auto-reconnecting delay.

        A negative value means no auto-reconnecting.

        The default value is \c -1.
     */
    int Session::autoReconnectDelay() const
    {
        Q_D(const Session);
        return d->delay;
    }

    /*!
        Sets auto-reconnecting delay in \a seconds.
     */
    void Session::setAutoReconnectDelay(int seconds)
    {
        Q_D(Session);
        d->delay = seconds;
    }

    /*!
        Returns the encoding.

        The default value is a null QByteArray.
     */
    QByteArray Session::encoding() const
    {
        Q_D(const Session);
        return d->encoding;
    }

    /*!
        Sets the \a encoding.

        See QTextCodec documentation for supported encodings.

        Encoding auto-detection can be turned on by passing a null QByteArray.

        The fallback locale is QTextCodec::codecForLocale().
     */
    void Session::setEncoding(const QByteArray& encoding)
    {
        Q_D(Session);
        d->encoding = encoding;
    }

    /*!
        Returns the ident.
     */
    QString Session::ident() const
    {
        Q_D(const Session);
        return d->ident;
    }

    /*!
        Sets the \a ident.

        \note setIdent() has no effect on already established connection.
     */
    void Session::setIdent(const QString& ident)
    {
        Q_D(Session);
        if (d->isConnected())
            qWarning("Session::setIdent() has no effect until re-connect");
        d->ident = ident;
    }

    /*!
        Returns the nick.
     */
    QString Session::nick() const
    {
        Q_D(const Session);
        return d->nick;
    }

    /*!
        Sets the \a nick.
     */
    void Session::setNick(const QString& nick)
    {
        Q_D(Session);
        if (d->nick != nick)
        {
            d->nick = nick;
            if (d->socket)
                raw(QString(QLatin1String("NICK %1")).arg(nick));
        }
    }

    /*!
        Returns the password.
     */
    QString Session::password() const
    {
        Q_D(const Session);
        return d->password;
    }

    /*!
        Sets the \a password.

        \note setPassword() has no effect on already established connection.
     */
    void Session::setPassword(const QString& password)
    {
        Q_D(Session);
        if (d->isConnected())
            qWarning("Session::setPassword() has no effect until re-connect");
        d->password = password;
    }

    /*!
        Returns the real name.
     */
    QString Session::realName() const
    {
        Q_D(const Session);
        return d->realName;
    }

    /*!
        Sets the \a realName.

        \note setRealName() has no effect on already established connection.
     */
    void Session::setRealName(const QString& realName)
    {
        Q_D(Session);
        if (d->isConnected())
            qWarning("Session::setRealName() has no effect until re-connect");
        d->realName = realName;
    }

    /*!
        Returns the host.
     */
    QString Session::host() const
    {
        Q_D(const Session);
        return d->host;
    }

    /*!
        Sets the \a host.
     */
    void Session::setHost(const QString& host)
    {
        Q_D(Session);
        if (d->isConnected())
            qWarning("Session::setHost() has no effect until re-connect");
        d->host = host;
    }

    /*!
        Returns the port.
     */
    quint16 Session::port() const
    {
        Q_D(const Session);
        return d->port;
    }

    /*!
        Sets the \a port.
     */
    void Session::setPort(quint16 port)
    {
        Q_D(Session);
        if (d->isConnected())
            qWarning("Session::setPort() has no effect until re-connect");
        d->port = port;
    }

    /*!
        Returns the options.

        The default value is \c StripNicks.
     */
    Session::Options Session::options() const
    {
        Q_D(const Session);
        return d->options;
    }

    /*!
        Sets the \a options.
     */
    void Session::setOptions(Options options)
    {
        Q_D(Session);
        d->options = options;
    }

    /*!
        Returns the socket.

        Irc::Session creates an instance of QTcpSocket by default.

        This function was introduced in version 0.3.0.
     */
    QAbstractSocket* Session::socket() const
    {
        Q_D(const Session);
        return d->socket;
    }

    /*!
        Sets the \a socket. The previously set socket is deleted if its parent is \c this.

        Irc::Session supports QSslSocket in the way that it automatically calls
        QSslSocket::startClientEncryption() while connecting.

        This function was introduced in version 0.3.0.
     */
    void Session::setSocket(QAbstractSocket* socket)
    {
        Q_D(Session);
        if (d->socket != socket)
        {
            if (d->socket)
            {
                d->socket->disconnect(this);
                if (d->socket->parent() == this)
                    d->socket->deleteLater();
            }

            d->socket = socket;
            if (socket)
            {
                connect(socket, SIGNAL(connected()), this, SLOT(_q_connected()));
                connect(socket, SIGNAL(disconnected()), this, SLOT(_q_disconnected()));
                connect(socket, SIGNAL(readyRead()), this, SLOT(_q_readData()));
                connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(_q_error()));
                connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(_q_state(QAbstractSocket::SocketState)));
            }
        }
    }

    /*!
        Returns the list of capabilities the server supports, as reported
        by the server (including any modifiers). If empty, the
        server might not implement the capabilities extension, or it might
        simply not support any specific capability.
     */
    QStringList Session::supportedCapabilities() const
    {
        Q_D(const Session);
        return d->capabilities;
    }

    /*!
        Returns the list of enabled capabilities for this session, as reported
        by the server (including any modifiers).

        \sa requestCapabilities()
     */
    QStringList Session::enabledCapabilities() const
    {
        Q_D(const Session);
        return d->enabledCapabilities;
    }

    /*!
        Returns whether a capability is enabled for this session.

        If you give modifier names along with the name (i.e. '=' or '~'), this
        method will remove them; it will match them to any name delivered by
        the server, whether or not it sends any modifiers along. Behaviour when
        including the 'disabled modifier' ('-') is undefined; you should negate
        the return value of this method instead.

        Note: This does not include the network specific modifier (i.e. '.').
        For example, =identify-msg matches ~IDENTIFY-MSG but not .identify-msg.

        \sa enabledCapabilities()
     */
    bool Session::capabilityEnabled(const QString& name) const
    {
        QString cleanName = name;
        while (cleanName.startsWith(QLatin1Char('='))
            || cleanName.startsWith(QLatin1Char('~')))
            cleanName.remove(0, 1);

        foreach (const QString &mod, enabledCapabilities())
        {
            QString copy = mod;
            while (copy.startsWith(QLatin1Char('='))
                || copy.startsWith(QLatin1Char('~')))
                copy.remove(0, 1);

            if (cleanName.compare(copy, Qt::CaseInsensitive) == 0)
                return true;
        }

        return false;
    }

    /*!
        Requests a capability in the server.

        \sa supportedCapabilities()
        \sa clearCapabilities()
        \sa capabilitiesAcked()
        \sa capabilitiesNotAcked()
     */
    void Session::requestCapabilities(const QStringList& capabilities)
    {
        Q_D(Session);

        if (d->capabilitiesSupported)
        {
            // We're far enough in the handshake to send them right away.
            QString caps;
            foreach (const QString &rawCap, d->wantedCapabilities) {
                QString cap = rawCap;
                cap.remove(QLatin1String("\r"));
                cap.remove(QLatin1String("\n"));
                caps.append(cap + QLatin1String(" "));
            }
            d->socket->write(QString::fromLatin1("CAP REQ :%1\r\n")
                              .arg(caps).toLatin1());
            return;
        }

        if (!d->welcomed)
        {
            // Queue the requested capabilities to request them later.
            d->wantedCapabilities.append(capabilities);
            d->wantedCapabilities.removeDuplicates();
            return;
        }

        // Capabilities are requested while they are not supported. Emit
        // an artificial NACK
        emit capabilitiesNotAcked(capabilities);
    }

    /*!
        Clear all non-sticky capabilities for this session.

        \sa enabledCapabilities()
        \sa requestCapabilities()
        \sa supportedCapabilities()
     */
    void Session::clearCapabilities()
    {
        Q_D(Session);
        if (d->welcomed)
            d->socket->write("CAP CLEAR\r\n");
        else
            d->wantedCapabilities.clear();
    }

    /*!
        Returns all allocated buffers.

        \sa buffer()
     */
    QList<Buffer*> Session::buffers() const
    {
        Q_D(const Session);
        return d->buffers.values();
    }

    /*!
        Returns the buffer for \a receiver. Returns \c 0
        if the buffer does not exist.

        \sa buffers()
        \sa addBuffer()
     */
    Buffer* Session::buffer(const QString& receiver) const
    {
        Q_D(const Session);
        if (receiver.isNull())
            return d->buffers.value(d->host.toLower());
        return d->buffers.value(receiver.toLower());
    }

    /*!
        Adds a buffer for \a receiver and returns it.

        \sa buffer()
     */
    Buffer* Session::addBuffer(const QString& receiver)
    {
        Q_D(Session);
        return d->createBuffer(receiver);
    }

    /*!
        Returns the default buffer.
     */
    Buffer* Session::defaultBuffer() const
    {
        Q_D(const Session);
        return d->defaultBuffer;
    }

    /*!
        Sets the default \a buffer.
     */
    void Session::setDefaultBuffer(Buffer* buffer)
    {
        Q_D(Session);
        d->defaultBuffer = buffer;
    }

    /*!
        Connects to server with \a hostName and \a port.
     */
    void Session::connectToServer(const QString& hostName, quint16 port)
    {
        Q_D(Session);
        if (!hostName.isNull())
            d->host = hostName;
        if (port != 6667)
            d->port = port;
        d->_q_reconnect();
    }

    /*!
        Reconnects to server.
     */
    void Session::reconnectToServer()
    {
        Q_D(Session);
        d->_q_reconnect();
    }

    /*!
        Disconnects from the server.
     */
    void Session::disconnectFromServer()
    {
        Q_D(Session);
        if (d->socket)
            d->socket->disconnectFromHost();
    }

    /*!
        Sends a raw message to the server.
     */
    bool Session::raw(const QString& message)
    {
        Q_D(Session);
        qint64 bytes = -1;
        if (d->socket)
            bytes = d->socket->write(message.toUtf8() + QByteArray("\r\n"));
        return bytes != -1;
    }

    /*!
        Requests message of the day.
     */
    bool Session::motd()
    {
        return raw(QLatin1String("MOTD"));
    }

    /*!
        Joins \a channel with optional \a key.
     */
    bool Session::join(const QString& channel, const QString& key)
    {
        if (key.isEmpty())
            return raw(QString(QLatin1String("JOIN %1")).arg(channel));
        else
            return raw(QString(QLatin1String("JOIN %1 %2")).arg(channel, key));
    }

    /*!
        Parts \a channel with \a reason.
     */
    bool Session::part(const QString& channel, const QString& reason)
    {
        if (reason.isEmpty())
            return raw(QString(QLatin1String("PART %1")).arg(channel));
        else
            return raw(QString(QLatin1String("PART %1 :%2")).arg(channel, reason));
    }

    /*!
        Quits with optional \a reason.
     */
    bool Session::quit(const QString& reason)
    {
        return raw(QString(QLatin1String("QUIT :%1")).arg(reason.isEmpty() ? QLatin1String("Quit") : reason));
    }

    /*!
        Requests the list of names on \a channel.
     */
    bool Session::names(const QString& channel)
    {
        return raw(QString(QLatin1String("NAMES %1")).arg(channel));
    }

    /*!
        Requests the list of channels.
     */
    bool Session::list(const QString& channel)
    {
        if (channel.isEmpty())
            return raw(QLatin1String("LIST"));
        else
            return raw(QString(QLatin1String("LIST %1")).arg(channel));
    }

    /*!
        Sends a whois command on \a nick.
     */
    bool Session::whois(const QString& nick)
    {
        return raw(QString(QLatin1String("WHOIS %1 %2")).arg(nick, nick));
    }

    /*!
        Sends a whowas command on \a nick.
     */
    bool Session::whowas(const QString& nick)
    {
        return raw(QString(QLatin1String("WHOWAS %1 %2")).arg(nick, nick));
    }

    /*!
        Sends a mode command on \a target.
     */
    bool Session::mode(const QString& target, const QString& mode)
    {
        if (mode.isEmpty())
            return raw(QString(QLatin1String("MODE %1")).arg(target));
        else
            return raw(QString(QLatin1String("MODE %1 %2")).arg(target, mode));
    }

    /*!
        Sends a topic command on \a channel.
     */
    bool Session::topic(const QString& channel, const QString& topic)
    {
        if (topic.isEmpty())
            return raw(QString(QLatin1String("TOPIC %1")).arg(channel));
        else
            return raw(QString(QLatin1String("TOPIC %1 :%2")).arg(channel, topic));
    }

    /*!
        Sends an invitation to \a nick.
     */
    bool Session::invite(const QString& nick, const QString& channel)
    {
        return raw(QString(QLatin1String("INVITE %1 %2")).arg(nick, channel));
    }

    /*!
        Kicks \a nick from \a channel with \a reason.
     */
    bool Session::kick(const QString& nick, const QString& channel, const QString& reason)
    {
        if (reason.isEmpty())
            return raw(QString(QLatin1String("KICK %1 %2")).arg(channel, nick));
        else
            return raw(QString(QLatin1String("KICK %1 %2 :%3")).arg(channel, nick, reason));
    }

    /*!
        Sends a \a message to \a receiver.
     */
    bool Session::message(const QString& receiver, const QString& message)
    {
        Q_D(Session);
        if (d->options & Session::EchoMessages)
        {
            Buffer* buffer = d->createBuffer(receiver);
            emit buffer->messageReceived(d->nick, message, Irc::Buffer::EchoFlag);
        }
        return raw(QString(QLatin1String("PRIVMSG %1 :%2")).arg(Util::nickFromTarget(receiver), message));
    }

    /*!
        Sends a \a notice to \a receiver.
     */
    bool Session::notice(const QString& receiver, const QString& notice)
    {
        Q_D(Session);
        if (d->options & Session::EchoMessages)
        {
            Buffer* buffer = d->createBuffer(receiver);
            emit buffer->noticeReceived(d->nick, notice, Irc::Buffer::EchoFlag);
        }
        return raw(QString(QLatin1String("NOTICE %1 :%2")).arg(Util::nickFromTarget(receiver), notice));
    }

    /*!
        Sends a CTCP \a action to \a receiver.
     */
    bool Session::ctcpAction(const QString& receiver, const QString& action)
    {
        Q_D(Session);
        if (d->options & Session::EchoMessages)
        {
            Buffer* buffer = d->createBuffer(receiver);
            emit buffer->ctcpActionReceived(d->nick, action, Irc::Buffer::EchoFlag);
        }
        return raw(QString(QLatin1String("PRIVMSG %1 :\x01" "ACTION %2\x01")).arg(Util::nickFromTarget(receiver), action));
    }

    /*!
        Sends a CTCP \a request to \a receiver.
     */
    bool Session::ctcpRequest(const QString& nick, const QString& request)
    {
        return raw(QString(QLatin1String("PRIVMSG %1 :\x01%2\x01")).arg(Util::nickFromTarget(nick), request));
    }

    /*!
        Sends a CTCP \a reply to \a receiver.
     */
    bool Session::ctcpReply(const QString& nick, const QString& reply)
    {
        return raw(QString(QLatin1String("NOTICE %1 :\x01%2\x01")).arg(Util::nickFromTarget(nick), reply));
    }

    /*!
        Returns a new Irc::Buffer object to act as an IRC buffer for \a receiver.

        This virtual factory method can be overridden for example in order to make
        Irc::Session use a subclass of Irc::Buffer.
     */
    Buffer* Session::createBuffer(const QString& receiver)
    {
        Buffer* buffer = new Buffer(receiver, this);
#ifndef IRC_NO_DEPRECATED
        // TODO: for backwards compatibility, to be removed in 1.0
        connect(buffer, SIGNAL(joined(QString)), SLOT(_q_joined(QString)));
        connect(buffer, SIGNAL(parted(QString, QString)), SLOT(_q_parted(QString, QString)));
        connect(buffer, SIGNAL(quit(QString, QString)), SLOT(_q_quit(QString, QString)));
        connect(buffer, SIGNAL(nickChanged(QString, QString)), SLOT(_q_nickChanged(QString, QString)));
        connect(buffer, SIGNAL(modeChanged(QString, QString, QString)), SLOT(_q_modeChanged(QString, QString, QString)));
        connect(buffer, SIGNAL(topicChanged(QString, QString)), SLOT(_q_topicChanged(QString, QString)));
        connect(buffer, SIGNAL(invited(QString, QString, QString)), SLOT(_q_invited(QString, QString, QString)));
        connect(buffer, SIGNAL(kicked(QString, QString, QString)), SLOT(_q_kicked(QString, QString, QString)));
        connect(buffer, SIGNAL(messageReceived(QString, QString)), SLOT(_q_messageReceived(QString, QString)));
        connect(buffer, SIGNAL(noticeReceived(QString, QString)), SLOT(_q_noticeReceived(QString, QString)));
        connect(buffer, SIGNAL(ctcpRequestReceived(QString, QString)), SLOT(_q_ctcpRequestReceived(QString, QString)));
        connect(buffer, SIGNAL(ctcpReplyReceived(QString, QString)), SLOT(_q_ctcpReplyReceived(QString, QString)));
        connect(buffer, SIGNAL(ctcpActionReceived(QString, QString)), SLOT(_q_ctcpActionReceived(QString, QString)));
        connect(buffer, SIGNAL(numericMessageReceived(QString, uint, QStringList)), SLOT(_q_numericMessageReceived(QString, uint, QStringList)));
        connect(buffer, SIGNAL(unknownMessageReceived(QString, QStringList)), SLOT(_q_unknownMessageReceived(QString, QStringList)));
#endif // IRC_NO_DEPRECATED
        return buffer;
    }

#ifndef IRC_NO_DEPRECATED
    // TODO: for backwards compatibility, to be removed in 1.0
    /*! \deprecated Use Irc::Session::raw() */
    bool Session::sendRaw(const QString& message)
    {
        qWarning() << "IrcSession::sendRaw(message) [slot] is DEPRECATED";
        return Session::raw(message);
    }
    /*! \deprecated Use Irc::Session::join() */
    bool Session::cmdJoin(const QString& channel, const QString& key)
    {
        qWarning() << "IrcSession::cmdJoin(channel, key) [slot] is DEPRECATED";
        return Session::join(channel, key);
    }
    /*! \deprecated Use Irc::Session::part() */
    bool Session::cmdPart(const QString& channel, const QString& reason)
    {
        qWarning() << "IrcSession::cmdPart(channel, reason) [slot] is DEPRECATED";
        return Session::part(channel, reason);
    }
    /*! \deprecated Use Irc::Session::quit() */
    bool Session::cmdQuit(const QString& reason)
    {
        qWarning() << "IrcSession::cmdQuit(reason) [slot] is DEPRECATED";
        return Session::quit(reason);
    }
    /*! \deprecated Use Irc::Session::names() */
    bool Session::cmdNames(const QString& channel)
    {
        qWarning() << "IrcSession::cmdNames(channel) [slot] is DEPRECATED";
        return Session::names(channel);
    }
    /*! \deprecated Use Irc::Session::list() */
    bool Session::cmdList(const QString& channel)
    {
        qWarning() << "IrcSession::cmdList(channel) [slot] is DEPRECATED";
        return Session::list(channel);
    }
    /*! \deprecated Use Irc::Session::whois() */
    bool Session::cmdWhois(const QString& nick)
    {
        qWarning() << "IrcSession::cmdWhois(nick) [slot] is DEPRECATED";
        return Session::whois(nick);
    }
    /*! \deprecated Use Irc::Session::mode() */
    bool Session::cmdMode(const QString& target, const QString& mode)
    {
        qWarning() << "IrcSession::cmdMode(target, mode) [slot] is DEPRECATED";
        return Session::mode(target, mode);
    }
    /*! \deprecated Use Irc::Session::topic() */
    bool Session::cmdTopic(const QString& channel, const QString& topic)
    {
        qWarning() << "IrcSession::cmdTopic(channel, topic) [slot] is DEPRECATED";
        return Session::topic(channel, topic);
    }
    /*! \deprecated Use Irc::Session::invite() */
    bool Session::cmdInvite(const QString& nick, const QString& channel)
    {
        qWarning() << "IrcSession::cmdInvite(nick, channel) [slot] is DEPRECATED";
        return Session::invite(nick, channel);
    }
    /*! \deprecated Use Irc::Session::kick() */
    bool Session::cmdKick(const QString& nick, const QString& channel, const QString& reason)
    {
        qWarning() << "IrcSession::cmdKick(nick, channel, reason) [slot] is DEPRECATED";
        return Session::kick(nick, channel, reason);
    }
    /*! \deprecated Use Irc::Session::message() */
    bool Session::cmdMessage(const QString& receiver, const QString& message)
    {
        qWarning() << "IrcSession::cmdMessage(receiver, message) [slot] is DEPRECATED";
        return Session::message(receiver, message);
    }
    /*! \deprecated Use Irc::Session::notice() */
    bool Session::cmdNotice(const QString& receiver, const QString& notice)
    {
        qWarning() << "IrcSession::cmdNotice(receiver, notice) [slot] is DEPRECATED";
        return Session::notice(receiver, notice);
    }
    /*! \deprecated Use Irc::Session::ctcpAction() */
    bool Session::cmdCtcpAction(const QString& receiver, const QString& action)
    {
        qWarning() << "IrcSession::cmdCtcpAction(receiver, action) [slot] is DEPRECATED";
        return Session::ctcpAction(receiver, action);
    }
    /*! \deprecated Use Irc::Session::ctcpRequest() */
    bool Session::cmdCtcpRequest(const QString& nick, const QString& request)
    {
        qWarning() << "IrcSession::cmdCtcpRequest(request) [slot] is DEPRECATED";
        return Session::ctcpRequest(nick, request);
    }
    /*! \deprecated Use Irc::Session::ctcpReply() */
    bool Session::cmdCtcpReply(const QString& nick, const QString& reply)
    {
        qWarning() << "IrcSession::cmdCtcpReply(reply) [slot] is DEPRECATED";
        return Session::ctcpReply(nick, reply);
    }
#endif // IRC_NO_DEPRECATED
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const Irc::Session* session)
{
    if (!session)
        return debug << "Irc::Session(0x0) ";
    debug.nospace() << session->metaObject()->className() << '(' << (void*) session;
    if (!session->objectName().isEmpty())
        debug << ", name = " << session->objectName();
    if (!session->host().isEmpty())
        debug << ", host = " << session->host()
              << ", port = " << session->port();
    debug << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircsession.cpp"
