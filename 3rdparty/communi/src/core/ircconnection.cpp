/*
  Copyright (C) 2008-2020 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ircconnection.h"
#include "ircconnection_p.h"
#include "ircnetwork_p.h"
#include "irccommand_p.h"
#include "ircprotocol.h"
#include "ircnetwork.h"
#include "irccommand.h"
#include "ircmessage.h"
#include "ircdebug_p.h"
#include "ircfilter.h"
#include "irccore_p.h"
#include "irc.h"
#include <QLocale>
#include <QRegularExpression>
#include <QDateTime>
#include <QTcpSocket>
#include <QTextCodec>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaEnum>
#ifndef QT_NO_SSL
#include <QSslSocket>
#include <QSslError>
#endif // QT_NO_SSL
#include <QDataStream>
#include <QVariantMap>

IRC_BEGIN_NAMESPACE

/*!
    \file ircconnection.h
    \brief \#include &lt;IrcConnection&gt;
 */

/*!
    \class IrcConnection ircconnection.h IrcConnection
    \ingroup core
    \brief Provides means to establish a connection to an IRC server.

    \section connection-management Connection management

    Before \ref open() "opening" a connection, it must be first initialized
    with \ref host, \ref userName, \ref nickName and \ref realName.

    The connection status may be queried at any time via status(). Also
    \ref active "isActive()" and \ref connected "isConnected()" are provided
    for convenience. In addition to the \ref status "statusChanged()" signal,
    the most important statuses are informed via the following convenience
    signals:
    \li connecting() -
        The connection is being established.
    \li \ref connected "connected()" -
        The IRC connection has been established, and the server is ready to receive commands.
    \li disconnected() -
        The connection has been lost.

    \section receiving-messages Receiving messages

    Whenever a message is received from the server, the messageReceived()
    signal is emitted. Also message type specific signals are provided
    for convenience. See messageReceived() and IrcMessage and its
    subclasses for more details.

    \section sending-commands Sending commands

    Sending commands to a server is most conveniently done by creating
    them via the various static \ref IrcCommand "IrcCommand::createXxx()"
    methods and passing them to sendCommand(). Also sendData() is provided
    for more low-level access. See IrcCommand for more details.

    \section example Example

    \code
    IrcConnection* connection = new IrcConnection(this);
    connect(connection, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
    connection->setHost("irc.server.com");
    connection->setUserName("me");
    connection->setNickName("myself");
    connection->setRealName("And I");
    connection->sendCommand(IrcCommand::createJoin("#mine"));
    connection->open();
    \endcode

    \sa IrcNetwork, IrcMessage, IrcCommand
 */

/*!
    \enum IrcConnection::Status
    This enum describes the connection status.
 */

/*!
    \var IrcConnection::Inactive
    \brief The connection is inactive.
 */

/*!
    \var IrcConnection::Waiting
    \brief The connection is waiting for a reconnect.
 */

/*!
    \var IrcConnection::Connecting
    \brief The connection is being established.
 */

/*!
    \var IrcConnection::Connected
    \brief The connection has been established.
 */

/*!
    \var IrcConnection::Closing
    \brief The connection is being closed.
 */

/*!
    \var IrcConnection::Closed
    \brief The connection has been closed.
 */

/*!
    \var IrcConnection::Error
    \brief The connection has encountered an error.
 */

/*!
    \fn void IrcConnection::connecting()

    This signal is emitted when the connection is being established.

    The underlying \ref socket has connected, but the IRC handshake is
    not yet finished and the server is not yet ready to receive commands.
 */

/*!
    \fn void IrcConnection::nickNameRequired(const QString& reserved, QString* alternate)

    This signal is emitted when the requested nick name is \a reserved
    and an \a alternate nick name should be provided.

    An alternate nick name may be set via the provided argument, by changing
    the \ref nickName property, or by sending a nick command directly.

    \sa nickNames, IrcCommand::createNick(), Irc::ERR_NICKNAMEINUSE, Irc::ERR_NICKCOLLISION
 */

/*!
    \fn void IrcConnection::channelKeyRequired(const QString& channel, QString* key)

    This signal is emitted when joining a \a channel requires a \a key.
    The key may be set via the provided argument, or by sending a new
    join command directly.

    \sa IrcCommand::createJoin(), Irc::ERR_BADCHANNELKEY
 */

/*!
    \fn void IrcConnection::disconnected()

    This signal is emitted when the connection has been lost.
 */

/*!
    \fn void IrcConnection::socketError(QAbstractSocket::SocketError error)

    This signal is emitted when a \ref socket \a error occurs.

    \sa QAbstractSocket::error()
 */

/*!
    \fn void IrcConnection::socketStateChanged(QAbstractSocket::SocketState state)

    This signal is emitted when the \a state of the underlying \ref socket changes.

    \sa QAbstractSocket::stateChanged()
 */

/*!
    \since 3.2
    \fn void IrcConnection::secureError()

    This signal is emitted when SSL socket error occurs.

    Either QSslSocket::sslErrors() was emitted, or the handshake failed
    meaning that the server is not SSL-enabled.
 */

/*!
    \fn void IrcConnection::messageReceived(IrcMessage* message)

    This signal is emitted when a \a message is received.

    In addition, message type specific signals are provided for convenience:
    \li void <b>accountMessageReceived</b>(\ref IrcAccountMessage* message) (\b since 3.3)
    \li void <b>awayMessageReceived</b>(\ref IrcAwayMessage* message) (\b since 3.3)
    \li void <b>batchMessageReceived</b>(\ref IrcBatchMessage* message) (\b since 3.5)
    \li void <b>capabilityMessageReceived</b>(\ref IrcCapabilityMessage* message)
    \li void <b>errorMessageReceived</b>(\ref IrcErrorMessage* message)
    \li void <b>hostChangeMessageReceived</b>(\ref IrcHostChangeMessage* message) (\b since 3.4)
    \li void <b>inviteMessageReceived</b>(\ref IrcInviteMessage* message)
    \li void <b>joinMessageReceived</b>(\ref IrcJoinMessage* message)
    \li void <b>kickMessageReceived</b>(\ref IrcKickMessage* message)
    \li void <b>modeMessageReceived</b>(\ref IrcModeMessage* message)
    \li void <b>motdMessageReceived</b>(\ref IrcMotdMessage* message)
    \li void <b>namesMessageReceived</b>(\ref IrcNamesMessage* message)
    \li void <b>nickMessageReceived</b>(\ref IrcNickMessage* message)
    \li void <b>noticeMessageReceived</b>(\ref IrcNoticeMessage* message)
    \li void <b>numericMessageReceived</b>(\ref IrcNumericMessage* message)
    \li void <b>partMessageReceived</b>(\ref IrcPartMessage* message)
    \li void <b>pingMessageReceived</b>(\ref IrcPingMessage* message)
    \li void <b>pongMessageReceived</b>(\ref IrcPongMessage* message)
    \li void <b>privateMessageReceived</b>(\ref IrcPrivateMessage* message)
    \li void <b>quitMessageReceived</b>(\ref IrcQuitMessage* message)
    \li void <b>topicMessageReceived</b>(\ref IrcTopicMessage* message)
    \li void <b>whoisMessageReceived</b>(\ref IrcWhoisMessage* message) (\b since 3.3)
    \li void <b>whowasMessageReceived</b>(\ref IrcWhowasMessage* message) (\b since 3.3)
    \li void <b>whoReplyMessageReceived</b>(\ref IrcWhoReplyMessage* message) (\b since 3.1)
 */

extern bool irc_is_supported_encoding(const QByteArray& encoding); // ircmessagedecoder.cpp

#ifndef IRC_DOXYGEN
IrcConnectionPrivate::IrcConnectionPrivate() :
    encoding("ISO-8859-15"),
    host(),
    userName(),
    nickName(),
    realName()
{
}

void IrcConnectionPrivate::init(IrcConnection* connection)
{
    q_ptr = connection;
    network = IrcNetworkPrivate::create(connection);
    connection->setSocket(new QTcpSocket(connection));
    connection->setProtocol(new IrcProtocol(connection));
    QObject::connect(&reconnecter, SIGNAL(timeout()), connection, SLOT(_irc_reconnect()));
}

void IrcConnectionPrivate::_irc_connected()
{
    Q_Q(IrcConnection);
    closed = false;
    pendingOpen = false;
    emit q->connecting();
    if (q->isSecure())
        QMetaObject::invokeMethod(socket, "startClientEncryption");
    protocol->open();
}

void IrcConnectionPrivate::_irc_disconnected()
{
    Q_Q(IrcConnection);
    protocol->close();
    emit q->disconnected();
    reconnect();
}

void IrcConnectionPrivate::_irc_error(QAbstractSocket::SocketError error)
{
    Q_Q(IrcConnection);
    if (error == QAbstractSocket::SslHandshakeFailedError) {
        ircDebug(q, IrcDebug::Error) << error;
        setStatus(IrcConnection::Error);
        emit q->secureError();
    } else if (!closed || (error != QAbstractSocket::RemoteHostClosedError && error != QAbstractSocket::UnknownSocketError)) {
        ircDebug(q, IrcDebug::Error) << error;
        emit q->socketError(error);
        setStatus(IrcConnection::Error);
        reconnect();
    }
}

void IrcConnectionPrivate::_irc_sslErrors()
{
    Q_Q(IrcConnection);
    QStringList errors;
#ifndef QT_NO_SSL
    QSslSocket* ssl = qobject_cast<QSslSocket*>(socket);
    if (ssl) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        foreach (const QSslError& error, ssl->sslHandshakeErrors())
#else
        foreach (const QSslError& error, ssl->sslErrors())
#endif
            errors += error.errorString();
    }
#endif
    ircDebug(q, IrcDebug::Error) << errors;
    emit q->secureError();
}

void IrcConnectionPrivate::_irc_state(QAbstractSocket::SocketState state)
{
    Q_Q(IrcConnection);
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        if (closed)
            setStatus(IrcConnection::Closed);
        break;
    case QAbstractSocket::ClosingState:
        if (status != IrcConnection::Error && status != IrcConnection::Waiting)
            setStatus(IrcConnection::Closing);
        break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::ConnectedState:
    default:
        setStatus(IrcConnection::Connecting);
        break;
    }
    emit q->socketStateChanged(state);
}

void IrcConnectionPrivate::_irc_reconnect()
{
    Q_Q(IrcConnection);
    if (!q->isActive()) {
        reconnecter.stop();
        q->open();
    }
}

void IrcConnectionPrivate::_irc_readData()
{
    protocol->read();
}

void IrcConnectionPrivate::_irc_filterDestroyed(QObject* filter)
{
    messageFilters.removeAll(filter);
    commandFilters.removeAll(filter);
}

static bool parseServer(const QString& server, QString* host, int* port, bool* ssl)
{
    QStringList p = server.split(QRegularExpression("[: ]"), Qt::SkipEmptyParts);
    *host = p.value(0);
    *ssl = p.value(1).startsWith(QLatin1Char('+'));
    bool ok = false;
    *port = p.value(1).toInt(&ok);
    if (*port == 0)
        *port = 6667;
    return !host->isEmpty() && (p.value(1).isEmpty() || ok) && (p.count() == 1 || p.count() == 2);
}

void IrcConnectionPrivate::open()
{
    Q_Q(IrcConnection);
    if (q->isActive()) {
        pendingOpen = true;
    } else {
        closed = false;
        if (!servers.isEmpty()) {
            QString h; int p; bool s;
            QString server = servers.value((++currentServer) % servers.count());
            if (!parseServer(server, &h, &p, &s))
                qWarning() << "IrcConnection::servers: malformed line" << server;
            q->setHost(h);
            q->setPort(p);
            q->setSecure(s);
        }
        socket->connectToHost(host, port);
        setConnectionCount(connectionCount + 1);
    }
}

void IrcConnectionPrivate::reconnect()
{
    if (enabled && (status != IrcConnection::Closed || !closed || pendingOpen) && !reconnecter.isActive() && reconnecter.interval() > 0) {
        pendingOpen = false;
        reconnecter.start();
        setStatus(IrcConnection::Waiting);
    }
}

void IrcConnectionPrivate::setConnectionCount(int count)
{
    Q_Q(IrcConnection);
    if (connectionCount != count) {
        connectionCount = count;
        emit q->connectionCountChanged(count);
    }
}

void IrcConnectionPrivate::setNick(const QString& nick)
{
    Q_Q(IrcConnection);
    if (nickName != nick) {
        nickName = nick;
        emit q->nickNameChanged(nick);
    }
}

void IrcConnectionPrivate::setStatus(IrcConnection::Status value)
{
    Q_Q(IrcConnection);
    if (status != value) {
        const bool wasConnected = q->isConnected();
        status = value;
        emit q->statusChanged(value);

        if (!wasConnected && q->isConnected()) {
            emit q->connected();
            foreach (const QByteArray& data, pendingData)
                q->sendRaw(data);
            pendingData.clear();
        }
        ircDebug(q, IrcDebug::Status) << status << qPrintable(host) << port;
    }
}

void IrcConnectionPrivate::setInfo(const QHash<QString, QString>& info)
{
    Q_Q(IrcConnection);
    const QString oldName = q->displayName();
    IrcNetworkPrivate* priv = IrcNetworkPrivate::get(network);
    priv->setInfo(info);
    const QString newName = q->displayName();
    if (oldName != newName)
        emit q->displayNameChanged(newName);
}

bool IrcConnectionPrivate::receiveMessage(IrcMessage* msg)
{
    Q_Q(IrcConnection);
    if (msg->type() == IrcMessage::Join && msg->isOwn()) {
        replies.clear();
    } else if (msg->type() == IrcMessage::Numeric) {
        int code = static_cast<IrcNumericMessage*>(msg)->code();
        if (code == Irc::RPL_NAMREPLY || code == Irc::RPL_ENDOFNAMES) {
            if (!replies.contains(Irc::RPL_ENDOFNAMES))
                msg->setFlag(IrcMessage::Implicit);
        } else if (code == Irc::RPL_TOPIC || code == Irc::RPL_NOTOPIC || code == Irc::RPL_TOPICWHOTIME || code == Irc::RPL_CHANNEL_URL || code == Irc::RPL_CREATIONTIME) {
            if (!replies.contains(code))
                msg->setFlag(IrcMessage::Implicit);
        }
        replies.insert(code);
    }

    bool filtered = false;
    for (int i = messageFilters.count() - 1; !filtered && i >= 0; --i) {
        IrcMessageFilter* filter = qobject_cast<IrcMessageFilter*>(messageFilters.at(i));
        if (filter)
            filtered |= filter->messageFilter(msg);
    }

    if (!filtered) {
        emit q->messageReceived(msg);

        switch (msg->type()) {
        case IrcMessage::Account:
            emit q->accountMessageReceived(static_cast<IrcAccountMessage*>(msg));
            break;
        case IrcMessage::Away:
            emit q->awayMessageReceived(static_cast<IrcAwayMessage*>(msg));
            break;
        case IrcMessage::Batch:
            emit q->batchMessageReceived(static_cast<IrcBatchMessage*>(msg));
            break;
        case IrcMessage::Capability:
            emit q->capabilityMessageReceived(static_cast<IrcCapabilityMessage*>(msg));
            break;
        case IrcMessage::Error:
            emit q->errorMessageReceived(static_cast<IrcErrorMessage*>(msg));
            break;
        case IrcMessage::HostChange:
            emit q->hostChangeMessageReceived(static_cast<IrcHostChangeMessage*>(msg));
            break;
        case IrcMessage::Invite:
            emit q->inviteMessageReceived(static_cast<IrcInviteMessage*>(msg));
            break;
        case IrcMessage::Join:
            emit q->joinMessageReceived(static_cast<IrcJoinMessage*>(msg));
            break;
        case IrcMessage::Kick:
            emit q->kickMessageReceived(static_cast<IrcKickMessage*>(msg));
            break;
        case IrcMessage::Mode:
            emit q->modeMessageReceived(static_cast<IrcModeMessage*>(msg));
            break;
        case IrcMessage::Motd:
            emit q->motdMessageReceived(static_cast<IrcMotdMessage*>(msg));
            break;
        case IrcMessage::Names:
            emit q->namesMessageReceived(static_cast<IrcNamesMessage*>(msg));
            break;
        case IrcMessage::Nick:
            emit q->nickMessageReceived(static_cast<IrcNickMessage*>(msg));
            break;
        case IrcMessage::Notice:
            emit q->noticeMessageReceived(static_cast<IrcNoticeMessage*>(msg));
            break;
        case IrcMessage::Numeric:
            emit q->numericMessageReceived(static_cast<IrcNumericMessage*>(msg));
            break;
        case IrcMessage::Part:
            emit q->partMessageReceived(static_cast<IrcPartMessage*>(msg));
            break;
        case IrcMessage::Ping:
            emit q->pingMessageReceived(static_cast<IrcPingMessage*>(msg));
            break;
        case IrcMessage::Pong:
            emit q->pongMessageReceived(static_cast<IrcPongMessage*>(msg));
            break;
        case IrcMessage::Private:
            emit q->privateMessageReceived(static_cast<IrcPrivateMessage*>(msg));
            break;
        case IrcMessage::Quit:
            emit q->quitMessageReceived(static_cast<IrcQuitMessage*>(msg));
            break;
        case IrcMessage::Topic:
            emit q->topicMessageReceived(static_cast<IrcTopicMessage*>(msg));
            break;
        case IrcMessage::Whois:
            emit q->whoisMessageReceived(static_cast<IrcWhoisMessage*>(msg));
            break;
        case IrcMessage::Whowas:
            emit q->whowasMessageReceived(static_cast<IrcWhowasMessage*>(msg));
            break;
        case IrcMessage::WhoReply:
            emit q->whoReplyMessageReceived(static_cast<IrcWhoReplyMessage*>(msg));
            break;
        case IrcMessage::Unknown:
        default:
            break;
        }
    }

    if (!msg->parent() || msg->parent() == q)
        msg->deleteLater();

    return !filtered;
}

IrcCommand* IrcConnectionPrivate::createCtcpReply(IrcPrivateMessage* request)
{
    Q_Q(IrcConnection);
    IrcCommand* reply = nullptr;
    const QMetaObject* metaObject = q->metaObject();
    int idx = metaObject->indexOfMethod("createCtcpReply(QVariant)");
    if (idx != -1) {
        // QML: QVariant createCtcpReply(QVariant)
        QVariant ret;
        QMetaMethod method = metaObject->method(idx);
        method.invoke(q, Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QVariant::fromValue(request)));
        reply = ret.value<IrcCommand*>();
    } else {
        // C++: IrcCommand* createCtcpReply(IrcPrivateMessage*)
        idx = metaObject->indexOfMethod("createCtcpReply(IrcPrivateMessage*)");
        QMetaMethod method = metaObject->method(idx);
        method.invoke(q, Q_RETURN_ARG(IrcCommand*, reply), Q_ARG(IrcPrivateMessage*, request));
    }
    return reply;
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new IRC connection with \a parent.
 */
IrcConnection::IrcConnection(QObject* parent) : QObject(parent), d_ptr(new IrcConnectionPrivate)
{
    Q_D(IrcConnection);
    d->init(this);
}

/*!
    Constructs a new IRC connection with \a host and \a parent.
 */
IrcConnection::IrcConnection(const QString& host, QObject* parent) : QObject(parent), d_ptr(new IrcConnectionPrivate)
{
    Q_D(IrcConnection);
    d->init(this);
    setHost(host);
}

/*!
    Destructs the IRC connection.
 */
IrcConnection::~IrcConnection()
{
    close();
    emit destroyed(this);
}

/*!
    \since 3.4

    Clones the IRC connection.
 */
IrcConnection* IrcConnection::clone(QObject *parent) const
{
    IrcConnection* connection = new IrcConnection(parent);
    connection->setHost(host());
    connection->setPort(port());
    connection->setServers(servers());
    connection->setUserName(userName());
    connection->setNickName(nickName());
    connection->setRealName(realName());
    connection->setPassword(password());
    connection->setNickNames(nickNames());
    connection->setDisplayName(displayName());
    connection->setUserData(userData());
    connection->setEncoding(encoding());
    connection->setEnabled(isEnabled());
    connection->setReconnectDelay(reconnectDelay());
    connection->setSecure(isSecure());
    connection->setSaslMechanism(saslMechanism());
    return connection;
}

/*!
    This property holds the FALLBACK encoding for received messages.

    The fallback encoding is used when the message is detected not
    to be valid \c UTF-8 and the consequent auto-detection of message
    encoding fails. See QTextCodec::availableCodecs() for the list of
    supported encodings.

    The default value is \c ISO-8859-15.

    \par Access functions:
    \li QByteArray <b>encoding</b>() const
    \li void <b>setEncoding</b>(const QByteArray& encoding)

    \sa QTextCodec::availableCodecs(), QTextCodec::codecForLocale()
 */
QByteArray IrcConnection::encoding() const
{
    Q_D(const IrcConnection);
    return d->encoding;
}

void IrcConnection::setEncoding(const QByteArray& encoding)
{
    Q_D(IrcConnection);
    if (!irc_is_supported_encoding(encoding)) {
        qWarning() << "IrcConnection::setEncoding(): unsupported encoding" << encoding;
        return;
    }
    d->encoding = encoding;
}

/*!
    This property holds the server host.

    \par Access functions:
    \li QString <b>host</b>() const
    \li void <b>setHost</b>(const QString& host)

    \par Notifier signal:
    \li void <b>hostChanged</b>(const QString& host)
 */
QString IrcConnection::host() const
{
    Q_D(const IrcConnection);
    return d->host;
}

void IrcConnection::setHost(const QString& host)
{
    Q_D(IrcConnection);
    if (d->host != host) {
        if (isActive())
            qWarning("IrcConnection::setHost() has no effect until re-connect");
        const QString oldName = displayName();
        d->host = host;
        emit hostChanged(host);
        const QString newName = displayName();
        if (oldName != newName)
            emit displayNameChanged(newName);
    }
}

/*!
    This property holds the server port.

    The default value is \c 6667.

    \par Access functions:
    \li int <b>port</b>() const
    \li void <b>setPort</b>(int port)

    \par Notifier signal:
    \li void <b>portChanged</b>(int port)
 */
int IrcConnection::port() const
{
    Q_D(const IrcConnection);
    return d->port;
}

void IrcConnection::setPort(int port)
{
    Q_D(IrcConnection);
    if (d->port != port) {
        if (isActive())
            qWarning("IrcConnection::setPort() has no effect until re-connect");
        d->port = port;
        emit portChanged(port);
    }
}

/*!
    \since 3.3

    This property holds the list of servers.

    The list of servers is automatically cycled through when reconnecting.

    \par Access functions:
    \li QStringList <b>servers</b>() const
    \li void <b>setServers</b>(const QStringList& servers)

    \par Notifier signal:
    \li void <b>serversChanged</b>(const QStringList& servers)

    \sa isValidServer()
 */
QStringList IrcConnection::servers() const
{
    Q_D(const IrcConnection);
    return d->servers;
}

void IrcConnection::setServers(const QStringList& servers)
{
    Q_D(IrcConnection);
    if (d->servers != servers) {
        d->servers = servers;
        emit serversChanged(servers);
    }
}

/*!
    \since 3.3

    Returns \c true if the server line syntax is valid.

    The syntax is:
    \code
    host <[+]port>
    \endcode
    where port is optional (defaults to \c 6667) and \c + prefix denotes SSL.

    \sa servers
 */
bool IrcConnection::isValidServer(const QString& server)
{
    QString h; int p; bool s;
    return parseServer(server, &h, &p, &s);
}

/*!
    This property holds the user name.

    \note Changing the user name has no effect until the connection is re-established.

    \par Access functions:
    \li QString <b>userName</b>() const
    \li void <b>setUserName</b>(const QString& name)

    \par Notifier signal:
    \li void <b>userNameChanged</b>(const QString& name)
 */
QString IrcConnection::userName() const
{
    Q_D(const IrcConnection);
    return d->userName;
}

void IrcConnection::setUserName(const QString& name)
{
    Q_D(IrcConnection);
    QString user = name.split(" ", Qt::SkipEmptyParts).value(0).trimmed();
    if (d->userName != user) {
        if (isActive())
            qWarning("IrcConnection::setUserName() has no effect until re-connect");
        d->userName = user;
        emit userNameChanged(user);
    }
}

/*!
    This property holds the current nick name.

    \par Access functions:
    \li QString <b>nickName</b>() const
    \li void <b>setNickName</b>(const QString& name)

    \par Notifier signal:
    \li void <b>nickNameChanged</b>(const QString& name)

    \sa nickNames
 */
QString IrcConnection::nickName() const
{
    Q_D(const IrcConnection);
    return d->nickName;
}

void IrcConnection::setNickName(const QString& name)
{
    Q_D(IrcConnection);
    QString nick = name.split(" ", Qt::SkipEmptyParts).value(0).trimmed();
    if (d->nickName != nick) {
        if (isActive())
            sendCommand(IrcCommand::createNick(nick));
        else
            d->setNick(nick);
    }
}

/*!
    This property holds the real name.

    \note Changing the real name has no effect until the connection is re-established.

    \par Access functions:
    \li QString <b>realName</b>() const
    \li void <b>setRealName</b>(const QString& name)

    \par Notifier signal:
    \li void <b>realNameChanged</b>(const QString& name)
 */
QString IrcConnection::realName() const
{
    Q_D(const IrcConnection);
    return d->realName;
}

void IrcConnection::setRealName(const QString& name)
{
    Q_D(IrcConnection);
    if (d->realName != name) {
        if (isActive())
            qWarning("IrcConnection::setRealName() has no effect until re-connect");
        d->realName = name;
        emit realNameChanged(name);
    }
}

/*!
    This property holds the password.

    \par Access functions:
    \li QString <b>password</b>() const
    \li void <b>setPassword</b>(const QString& password)

    \par Notifier signal:
    \li void <b>passwordChanged</b>(const QString& password)
 */
QString IrcConnection::password() const
{
    Q_D(const IrcConnection);
    return d->password;
}

void IrcConnection::setPassword(const QString& password)
{
    Q_D(IrcConnection);
    if (d->password != password) {
        if (isActive())
            qWarning("IrcConnection::setPassword() has no effect until re-connect");
        d->password = password;
        emit passwordChanged(password);
    }
}

/*!
    \since 3.3

    This property holds the nick names.

    The list of nick names is automatically cycled through when the
    current nick name is reserved. If all provided nick names are
    reserved, the nickNameRequired() signal is emitted.

    \par Access functions:
    \li QStringList <b>nickNames</b>() const
    \li void <b>setNickNames</b>(const QStringList& names)

    \par Notifier signal:
    \li void <b>nickNamesChanged</b>(const QStringList& names)

    \sa nickName, nickNameRequired()
 */
QStringList IrcConnection::nickNames() const
{
    Q_D(const IrcConnection);
    return d->nickNames;
}

void IrcConnection::setNickNames(const QStringList& names)
{
    Q_D(IrcConnection);
    if (d->nickNames != names) {
        d->nickNames = names;
        emit nickNamesChanged(names);
    }
}

/*!
    This property holds the display name.

    Unless explicitly set, display name resolves to IrcNetwork::name
    or IrcConnection::host while the former is not known.

    \par Access functions:
    \li QString <b>displayName</b>() const
    \li void <b>setDisplayName</b>(const QString& name)

    \par Notifier signal:
    \li void <b>displayNameChanged</b>(const QString& name)
 */
QString IrcConnection::displayName() const
{
    Q_D(const IrcConnection);
    QString name = d->displayName;
    if (name.isEmpty())
        name = d->network->name();
    if (name.isEmpty())
        name = d->host;
    return name;
}

void IrcConnection::setDisplayName(const QString& name)
{
    Q_D(IrcConnection);
    if (d->displayName != name) {
        d->displayName = name;
        emit displayNameChanged(name);
    }
}

/*!
    \since 3.1

    This property holds arbitrary user data.

    \par Access functions:
    \li QVariantMap <b>userData</b>() const
    \li void <b>setUserData</b>(const QVariantMap& data)

    \par Notifier signal:
    \li void <b>userDataChanged</b>(const QVariantMap& data)
 */
QVariantMap IrcConnection::userData() const
{
    Q_D(const IrcConnection);
    return d->userData;
}

void IrcConnection::setUserData(const QVariantMap& data)
{
    Q_D(IrcConnection);
    if (d->userData != data) {
        d->userData = data;
        emit userDataChanged(data);
    }
}

/*!
    \property Status IrcConnection::status
    This property holds the connection status.

    \par Access function:
    \li Status <b>status</b>() const

    \par Notifier signal:
    \li void <b>statusChanged</b>(Status status)
 */
IrcConnection::Status IrcConnection::status() const
{
    Q_D(const IrcConnection);
    return d->status;
}

/*!
    \property bool IrcConnection::active
    This property holds whether the connection is active.

    The connection is considered active when its either connecting, connected or closing.

    \par Access function:
    \li bool <b>isActive</b>() const
 */
bool IrcConnection::isActive() const
{
    Q_D(const IrcConnection);
    return d->status == Connecting || d->status == Connected || d->status == Closing;
}

/*!
    \property bool IrcConnection::connected
    This property holds whether the connection has been established.

    The connection has been established when the welcome message
    has been received and the server is ready to receive commands.

    \sa Irc::RPL_WELCOME

    \par Access function:
    \li bool <b>isConnected</b>() const

    \par Notifier signal:
    \li void <b>connected</b>()
 */
bool IrcConnection::isConnected() const
{
    Q_D(const IrcConnection);
    return d->status == Connected;
}

/*!
    \property bool IrcConnection::enabled
    This property holds whether the connection is enabled.

    The default value is \c true.

    When set to \c false, a disabled connection does nothing when open() is called.

    \par Access functions:
    \li bool <b>isEnabled</b>() const
    \li void <b>setEnabled</b>(bool enabled) [slot]
    \li void <b>setDisabled</b>(bool disabled) [slot]

    \par Notifier signal:
    \li void <b>enabledChanged</b>(bool enabled)
 */
bool IrcConnection::isEnabled() const
{
    Q_D(const IrcConnection);
    return d->enabled;
}

void IrcConnection::setEnabled(bool enabled)
{
    Q_D(IrcConnection);
    if (d->enabled != enabled) {
        d->enabled = enabled;
        emit enabledChanged(enabled);
    }
}

void IrcConnection::setDisabled(bool disabled)
{
    setEnabled(!disabled);
}

/*!
    \property int IrcConnection::reconnectDelay
    This property holds the reconnect delay in seconds.

    A positive (greater than zero) value enables automatic reconnect.
    When the connection is lost due to a socket error, IrcConnection
    will automatically attempt to reconnect after the specified delay.

    The default value is \c 0 (automatic reconnect disabled).

    \par Access functions:
    \li int <b>reconnectDelay</b>() const
    \li void <b>setReconnectDelay</b>(int seconds)

    \par Notifier signal:
    \li void <b>reconnectDelayChanged</b>(int seconds)
 */
int IrcConnection::reconnectDelay() const
{
    Q_D(const IrcConnection);
    return d->reconnecter.interval() / 1000;
}

void IrcConnection::setReconnectDelay(int seconds)
{
    Q_D(IrcConnection);
    const int interval = qMax(0, seconds) * 1000;
    if (d->reconnecter.interval() != interval) {
        d->reconnecter.setInterval(interval);
        emit reconnectDelayChanged(interval);
    }
}

/*!
    \property int IrcConnection::connectionCount
    This property holds the amount of times a connection was established.

    The default value is \c 0 (no connections where established).

    \par Access functions:
    \li int <b>connectionCount</b>() const
 */
int IrcConnection::connectionCount() const
{
    Q_D(const IrcConnection);
    return d->connectionCount;
}

/*!
    This property holds the socket. The default value is an instance of QTcpSocket.

    The previously set socket is deleted if its parent is \c this.

    \note IrcConnection supports QSslSocket in the way that it automatically
    calls QSslSocket::startClientEncryption() while connecting.

    \par Access functions:
    \li \ref QAbstractSocket* <b>socket</b>() const
    \li void <b>setSocket</b>(\ref QAbstractSocket* socket)

    \sa IrcConnection::secure
 */
QAbstractSocket* IrcConnection::socket() const
{
    Q_D(const IrcConnection);
    return d->socket;
}

void IrcConnection::setSocket(QAbstractSocket* socket)
{
    Q_D(IrcConnection);
    if (d->socket != socket) {
        if (d->socket) {
            d->socket->disconnect(this);
            if (d->socket->parent() == this)
                d->socket->deleteLater();
        }

        d->socket = socket;
        if (socket) {
            connect(socket, SIGNAL(connected()), this, SLOT(_irc_connected()));
            connect(socket, SIGNAL(disconnected()), this, SLOT(_irc_disconnected()));
            connect(socket, SIGNAL(readyRead()), this, SLOT(_irc_readData()));
            connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(_irc_error(QAbstractSocket::SocketError)));
            connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(_irc_state(QAbstractSocket::SocketState)));
            if (isSecure())
                connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(_irc_sslErrors()));
        }
    }
}

/*!
    \property bool IrcConnection::secure
    This property holds whether the socket is an SSL socket.

    This property is provided for convenience. Calling
    \code
    connection->setSecure(true);
    \endcode

    is equivalent to:

    \code
    QSslSocket* socket = new QSslSocket(socket);
    socket->setPeerVerifyMode(QSslSocket::QueryPeer);
    connection->setSocket(socket);
    \endcode

    \note IrcConnection does not handle SSL errors, see
    QSslSocket::sslErrors() for more details on the subject.

    \par Access functions:
    \li bool <b>isSecure</b>() const
    \li void <b>setSecure</b>(bool secure)

    \par Notifier signal:
    \li void <b>secureChanged</b>(bool secure)

    \sa secureSupported, IrcConnection::socket
 */
bool IrcConnection::isSecure() const
{
#ifdef QT_NO_SSL
    return false;
#else
    return qobject_cast<QSslSocket*>(socket());
#endif // QT_NO_SSL
}

void IrcConnection::setSecure(bool secure)
{
#ifdef QT_NO_SSL
    if (secure) {
        qWarning("IrcConnection::setSecure(): the Qt build does not support SSL");
        return;
    }
#else
    if (secure && !QSslSocket::supportsSsl()) {
        qWarning("IrcConnection::setSecure(): the platform does not support SSL - try installing OpenSSL");
        return;
    }

    QSslSocket* sslSocket = qobject_cast<QSslSocket*>(socket());
    if (secure && !sslSocket) {
        sslSocket = new QSslSocket(this);
        sslSocket->setPeerVerifyMode(QSslSocket::QueryPeer);
        setSocket(sslSocket);
        emit secureChanged(true);
    } else if (!secure && sslSocket) {
        setSocket(new QTcpSocket(this));
        emit secureChanged(false);
    }
#endif // !QT_NO_SSL
}

/*!
    \deprecated Use Irc::isSecureSupported() instead.
 */
bool IrcConnection::isSecureSupported()
{
    return Irc::isSecureSupported();
}

/*!
    This property holds the used SASL (Simple Authentication and Security Layer) mechanism.

    \par Access functions:
    \li QString <b>saslMechanism</b>() const
    \li void <b>setSaslMechanism</b>(const QString& mechanism)

    \par Notifier signal:
    \li void <b>saslMechanismChanged</b>(const QString& mechanism)

    \sa supportedSaslMechanisms, \ref ircv3
 */
QString IrcConnection::saslMechanism() const
{
    Q_D(const IrcConnection);
    return d->saslMechanism;
}

void IrcConnection::setSaslMechanism(const QString& mechanism)
{
    Q_D(IrcConnection);
    if (!mechanism.isEmpty() && !supportedSaslMechanisms().contains(mechanism.toUpper())) {
        qWarning("IrcConnection::setSaslMechanism(): unsupported mechanism: '%s'", qPrintable(mechanism));
        return;
    }
    if (d->saslMechanism != mechanism) {
        if (isActive())
            qWarning("IrcConnection::setSaslMechanism() has no effect until re-connect");
        d->saslMechanism = mechanism.toUpper();
        emit saslMechanismChanged(mechanism);
    }
}

/*!
    \deprecated Use Irc::supportedSaslMechanisms() instead.
 */
QStringList IrcConnection::supportedSaslMechanisms()
{
    return Irc::supportedSaslMechanisms();
}

/*!
    \since 3.5

    This property holds CTCP (client to client protocol) replies.

    This is a convenient request-reply map for customized static
    CTCP replies. For dynamic replies, override createCtcpReply()
    instead.

    \note Set an empty reply to omit the automatic reply.

    \par Access functions:
    \li QVariantMap <b>ctcpReplies</b>() const
    \li void <b>setCtcpReplies</b>(const QVariantMap& replies)

    \par Notifier signal:
    \li void <b>ctcpRepliesChanged</b>(const QVariantMap& replies)

    \sa createCtcpReply()
 */
QVariantMap IrcConnection::ctcpReplies() const
{
    Q_D(const IrcConnection);
    return d->ctcpReplies;
}

void IrcConnection::setCtcpReplies(const QVariantMap& replies)
{
    Q_D(IrcConnection);
    if (d->ctcpReplies != replies) {
        d->ctcpReplies = replies;
        emit ctcpRepliesChanged(replies);
    }
}

/*!
    This property holds the network information.

    \par Access function:
    \li IrcNetwork* <b>network</b>() const
 */
IrcNetwork* IrcConnection::network() const
{
    Q_D(const IrcConnection);
    return d->network;
}

/*!
    Opens a connection to the server.

    The function does nothing when the connection is already \ref active
    or explicitly \ref enabled "disabled".

    \note The function merely outputs a warnings and returns immediately if
    either \ref host, \ref userName, \ref nickName or \ref realName is empty.
 */
void IrcConnection::open()
{
    Q_D(IrcConnection);
    if (d->host.isEmpty() && d->servers.isEmpty()) {
        qWarning("IrcConnection::open(): host is empty!");
        return;
    }
    if (d->userName.isEmpty()) {
        qWarning("IrcConnection::open(): userName is empty!");
        return;
    }
    if (d->nickName.isEmpty() && d->nickNames.isEmpty()) {
        qWarning("IrcConnection::open(): nickNames is empty!");
        return;
    }
    if (d->realName.isEmpty()) {
        qWarning("IrcConnection::open(): realName is empty!");
        return;
    }
    if (d->enabled && d->socket)
        d->open();
}

/*!
    Immediately closes the connection to the server.

    Calling close() makes the connection close immediately and thus might lead to
    "remote host closed the connection". In order to quit gracefully, call quit()
    first. This function attempts to flush the underlying socket, but this does
    not guarantee that the server ever receives the QUIT command if the connection
    is closed immediately after sending the command. In order to ensure a graceful
    quit, let the server handle closing the connection.

    C++ example:
    \code
    connection->quit(reason);
    QTimer::singleShot(timeout, connection, SLOT(deleteLater()));
    \endcode

    QML example:
    \code
    connection.quit(reason);
    connection.destroy(timeout);
    \endcode

    \sa quit()
 */
void IrcConnection::close()
{
    Q_D(IrcConnection);
    if (d->socket) {
        d->closed = true;
        d->pendingOpen = false;
        d->socket->flush();
        d->socket->abort();
        d->socket->disconnectFromHost();
        if (d->socket->state() == QAbstractSocket::UnconnectedState)
            d->setStatus(Closed);
        d->reconnecter.stop();
    }
}

/*!
    Sends a quit command with an optionally specified \a reason.

    This method is provided for convenience. It is equal to:
    \code
    if (connection->isActive())
        connection->sendCommand(IrcCommand::createQuit(reason));
    \endcode

    \sa IrcCommand::createQuit()
 */
void IrcConnection::quit(const QString& reason)
{
    Q_D(IrcConnection);
    if (isConnected()) {
        d->setConnectionCount(0);
        sendCommand(IrcCommand::createQuit(reason));
    } else {
        close();
    }
}

/*!
    Sends a \a command to the server.

    If the connection is not active, the \a command is queued and sent
    later when the connection has been established.

    \note If the command has a valid parent, it is an indication that
    the caller of this method is be responsible for freeing the command.
    If the command does not have a valid parent (like the commands
    created via various IrcCommand::createXxx() methods) the connection
    will take ownership of the command and delete it once it has been
    sent. Thus, the command must have been allocated on the heap and
    it is not safe to access the command after it has been sent.

    \sa sendData()
 */
bool IrcConnection::sendCommand(IrcCommand* command)
{
    Q_D(IrcConnection);
    bool res = false;
    if (command) {
        bool filtered = false;
        IrcCommandPrivate::get(command)->connection = this;
        for (int i = d->commandFilters.count() - 1; !filtered && i >= 0; --i) {
            QObject* filter = d->commandFilters.at(i);
            IrcCommandFilter* commandFilter = qobject_cast<IrcCommandFilter*>(filter);
            if (commandFilter && !d->activeCommandFilters.contains(filter)) {
                d->activeCommandFilters.push(filter);
                filtered |= commandFilter->commandFilter(command);
                d->activeCommandFilters.pop();
            }
        }
        if (filtered) {
            res = false;
        } else {
            QTextCodec* codec = QTextCodec::codecForName(command->encoding());
            Q_ASSERT(codec);
            res = sendData(codec->fromUnicode(command->toString()));
        }
        if (!command->parent())
            command->deleteLater();
    }
    return res;
}

/*!
    Sends raw \a data to the server.

    \sa sendCommand()
 */
bool IrcConnection::sendData(const QByteArray& data)
{
    Q_D(IrcConnection);
    if (d->socket) {
        if (isActive()) {
            const QByteArray cmd = data.left(5).toUpper();
            if (cmd.startsWith("PASS "))
                ircDebug(this, IrcDebug::Write) << data.left(5) + QByteArray(data.mid(5).length(), 'x');
            else
                ircDebug(this, IrcDebug::Write) << data;
            if (!d->closed && data.length() >= 4) {
                if (cmd.startsWith("QUIT") && (data.length() == 4 || QChar(data.at(4)).isSpace())) {
                    d->closed = true;
                    d->setConnectionCount(0);
                }
            }
            return d->protocol->write(data);
        } else {
            d->pendingData += data;
        }
    }
    return false;
}

/*!
    Sends raw \a message to the server using UTF-8 encoding.

    \sa sendData(), sendCommand()
 */
bool IrcConnection::sendRaw(const QString& message)
{
    return sendData(message.toUtf8());
}

/*!
    Installs a message \a filter on the connection. The \a filter must implement the IrcMessageFilter interface.

    A message filter receives all messages that are sent to the connection. The filter
    receives messages via the \ref IrcMessageFilter::messageFilter() "messageFilter()"
    function. The function must return \c true if the message should be filtered,
    (i.e. stopped); otherwise it must return \c false.

    If multiple message filters are installed on the same connection, the filter
    that was installed last is activated first.

    \sa removeMessageFilter()
 */
void IrcConnection::installMessageFilter(QObject* filter)
{
    Q_D(IrcConnection);
    IrcMessageFilter* msgFilter = qobject_cast<IrcMessageFilter*>(filter);
    if (msgFilter) {
        d->messageFilters += filter;
        connect(filter, SIGNAL(destroyed(QObject*)), this, SLOT(_irc_filterDestroyed(QObject*)), Qt::UniqueConnection);
    }
}

/*!
    Removes a message \a filter from the connection.

    The request is ignored if such message filter has not been installed.
    All message filters for a connection are automatically removed
    when the connection is destroyed.

    \sa installMessageFilter()
 */
void IrcConnection::removeMessageFilter(QObject* filter)
{
    Q_D(IrcConnection);
    IrcMessageFilter* msgFilter = qobject_cast<IrcMessageFilter*>(filter);
    if (msgFilter) {
        d->messageFilters.removeAll(filter);
        disconnect(filter, SIGNAL(destroyed(QObject*)), this, SLOT(_irc_filterDestroyed(QObject*)));
    }
}

/*!
    Installs a command \a filter on the connection. The \a filter must implement the IrcCommandFilter interface.

    A command filter receives all commands that are sent from the connection. The filter
    receives commands via the \ref IrcCommandFilter::commandFilter() "commandFilter()"
    function. The function must return \c true if the command should be filtered,
    (i.e. stopped); otherwise it must return \c false.

    If multiple command filters are installed on the same connection, the filter
    that was installed last is activated first.

    \sa removeCommandFilter()
 */
void IrcConnection::installCommandFilter(QObject* filter)
{
    Q_D(IrcConnection);
    IrcCommandFilter* cmdFilter = qobject_cast<IrcCommandFilter*>(filter);
    if (cmdFilter) {
        d->commandFilters += filter;
        connect(filter, SIGNAL(destroyed(QObject*)), this, SLOT(_irc_filterDestroyed(QObject*)), Qt::UniqueConnection);
    }
}

/*!
    Removes a command \a filter from the connection.

    The request is ignored if such command filter has not been installed.
    All command filters for a connection are automatically removed when
    the connection is destroyed.

    \sa installCommandFilter()
 */
void IrcConnection::removeCommandFilter(QObject* filter)
{
    Q_D(IrcConnection);
    IrcCommandFilter* cmdFilter = qobject_cast<IrcCommandFilter*>(filter);
    if (cmdFilter) {
        d->commandFilters.removeAll(filter);
        disconnect(filter, SIGNAL(destroyed(QObject*)), this, SLOT(_irc_filterDestroyed(QObject*)));
    }
}

/*!
    \since 3.1

    Saves the state of the connection. The \a version number is stored as part of the state data.

    To restore the saved state, pass the return value and \a version number to restoreState().
 */
QByteArray IrcConnection::saveState(int version) const
{
    Q_D(const IrcConnection);
    QVariantMap args;
    args.insert("version", version);
    args.insert("host", d->host);
    args.insert("port", d->port);
    args.insert("servers", d->servers);
    args.insert("userName", d->userName);
    args.insert("nickName", d->nickName);
    args.insert("realName", d->realName);
    args.insert("password", d->password);
    args.insert("nickNames", d->nickNames);
    args.insert("displayName", displayName());
    args.insert("userData", d->userData);
    args.insert("encoding", d->encoding);
    args.insert("enabled", d->enabled);
    args.insert("reconnectDelay", reconnectDelay());
    args.insert("secure", isSecure());
    args.insert("saslMechanism", d->saslMechanism);

    QByteArray state;
    QDataStream out(&state, QIODevice::WriteOnly);
    out << args;
    return state;
}

/*!
    \since 3.1

    Restores the \a state of the connection. The \a version number is compared with that stored in \a state.
    If they do not match, the connection state is left unchanged, and this function returns \c false; otherwise,
    the state is restored, and \c true is returned.

    \sa saveState()
 */
bool IrcConnection::restoreState(const QByteArray& state, int version)
{
    Q_D(IrcConnection);
    if (isActive())
        return false;

    QVariantMap args;
    QDataStream in(state);
    in >> args;
    if (in.status() != QDataStream::Ok || args.value("version", -1).toInt() != version)
        return false;

    setHost(args.value("host", d->host).toString());
    setPort(args.value("port", d->port).toInt());
    setServers(args.value("servers", d->servers).toStringList());
    setUserName(args.value("userName", d->userName).toString());
    setNickName(args.value("nickName", d->nickName).toString());
    setRealName(args.value("realName", d->realName).toString());
    setPassword(args.value("password", d->password).toString());
    setNickNames(args.value("nickNames", d->nickNames).toStringList());
    if (!d->nickNames.isEmpty() && d->nickNames.indexOf(d->nickName) != 0)
        setNickName(d->nickNames.first());
    setDisplayName(args.value("displayName").toString());
    setUserData(args.value("userData", d->userData).toMap());
    setEncoding(args.value("encoding", d->encoding).toByteArray());
    setEnabled(args.value("enabled", d->enabled).toBool());
    setReconnectDelay(args.value("reconnectDelay", reconnectDelay()).toInt());
    setSecure(args.value("secure", isSecure()).toBool());
    setSaslMechanism(args.value("saslMechanism", d->saslMechanism).toString());
    return true;
}

/*!
    Creates a reply command for the CTCP \a request.

    The default implementation first checks whether the \ref ctcpReplies
    property contains a user-supplied reply for the request. In case it
    does, the reply is sent automatically. In case there is no user-supplied
    reply, the default implementation handles the following CTCP requests:
    CLIENTINFO, PING, SOURCE, TIME and VERSION.

    Reimplement this function in order to alter or omit the default replies.

    \sa ctcpReplies
 */
IrcCommand* IrcConnection::createCtcpReply(IrcPrivateMessage* request) const
{
    Q_D(const IrcConnection);
    QString reply;
    QString type = request->content().split(" ", Qt::SkipEmptyParts).value(0).toUpper();
    if (d->ctcpReplies.contains(type))
        reply = type + QLatin1String(" ") + d->ctcpReplies.value(type).toString();
    else if (type == "PING")
        reply = request->content();
    else if (type == "TIME")
        reply = QLatin1String("TIME ") + QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
    else if (type == "VERSION")
        reply = QLatin1String("VERSION Communi ") + Irc::version() + QLatin1String(" - https://communi.github.io");
    else if (type == "SOURCE")
        reply = QLatin1String("SOURCE https://communi.github.io");
    else if (type == "CLIENTINFO")
        reply = QLatin1String("CLIENTINFO PING SOURCE TIME VERSION");
    if (!reply.isEmpty())
        return IrcCommand::createCtcpReply(request->nick(), reply);
    return nullptr;
}

/*!
    \since 3.2

    This property holds the protocol.

    The previously set protocol is deleted if its parent is \c this.

    \par Access functions:
    \li \ref IrcProtocol* <b>protocol</b>() const
    \li void <b>setProtocol</b>(\ref IrcProtocol* protocol)
 */
IrcProtocol* IrcConnection::protocol() const
{
    Q_D(const IrcConnection);
    return d->protocol;
}

void IrcConnection::setProtocol(IrcProtocol* proto)
{
    Q_D(IrcConnection);
    if (d->protocol != proto) {
        if (d->protocol && d->protocol->parent() == this)
            delete d->protocol;
        d->protocol = proto;
    }
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, IrcConnection::Status status)
{
    const int index = IrcConnection::staticMetaObject.indexOfEnumerator("Status");
    QMetaEnum enumerator = IrcConnection::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(status);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, const IrcConnection* connection)
{
    if (!connection)
        return debug << "IrcConnection(0x0) ";
    debug.nospace() << connection->metaObject()->className() << '(' << (void*) connection;
    if (!connection->displayName().isEmpty())
        debug.nospace() << ", " << qPrintable(connection->displayName());
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircconnection.cpp"

IRC_END_NAMESPACE
