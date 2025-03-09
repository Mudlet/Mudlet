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

#include "ircprotocol.h"
#include "ircconnection_p.h"
#include "ircmessagecomposer_p.h"
#include "ircnetwork_p.h"
#include "ircconnection.h"
#include "ircmessage_p.h"
#include "irccommand.h"
#include "ircdebug_p.h"
#include "irccore_p.h"
#include "irc.h"
#include <QDebug>

IRC_BEGIN_NAMESPACE

/*!
    \file ircprotocol.h
    \brief \#include &lt;IrcProtocol&gt;
 */

/*!
    \since 3.2
    \class IrcProtocol ircprotocol.h IrcProtocol
    \ingroup core
    \brief Implements the IRC protocol and provides means for implementing support for custom protocols.

    \sa IrcConnection::protocol
 */

#ifndef IRC_DOXYGEN
class IrcProtocolPrivate
{
    Q_DECLARE_PUBLIC(IrcProtocol)

public:
    IrcProtocolPrivate();

    void authenticate(bool secure);

    void readLines(const QByteArray& delimiter);
    void processLine(const QByteArray& line);

    bool batchMessage(IrcMessage* msg);
    bool handleBatchMessage(IrcBatchMessage* msg);

    void handleNumericMessage(IrcNumericMessage* msg);
    void handlePrivateMessage(IrcPrivateMessage* msg);
    void handleCapabilityMessage(IrcCapabilityMessage* msg);

    void _irc_pauseHandshake();
    void _irc_resumeHandshake();

    IrcProtocol* q_ptr = nullptr;
    IrcConnection* connection = nullptr;
    IrcMessageComposer* composer = nullptr;
    QHash<QString, IrcBatchMessage*> batches;
    QHash<QString, QString> info;
    QByteArray buffer;
    int currentNick = -1;
    bool resumed = false;
    bool authed = false;
    bool motd = false;
};

IrcProtocolPrivate::IrcProtocolPrivate()
{
}

void IrcProtocolPrivate::authenticate(bool secure)
{
    const QString password = connection->password();
    if (!password.isEmpty()) {
        if (secure) {
            const QByteArray userName = connection->userName().toUtf8();
            const QByteArray data = userName + '\0' + userName + '\0' + password.toUtf8();
            authed = connection->sendData("AUTHENTICATE " + data.toBase64());
        } else {
            authed = connection->sendRaw(QString("PASS :%1").arg(password));
        }
    }
}

void IrcProtocolPrivate::readLines(const QByteArray& delimiter)
{
    int i = -1;
    while ((i = buffer.indexOf(delimiter)) != -1) {
        QByteArray line = buffer.left(i).trimmed();
        buffer = buffer.mid(i + delimiter.length());
        if (!line.isEmpty())
            processLine(line);
    }
}

void IrcProtocolPrivate::processLine(const QByteArray& line)
{
    Q_Q(IrcProtocol);
    ircDebug(connection, IrcDebug::Read) << line;

    if (line.startsWith("AUTHENTICATE") && !connection->saslMechanism().isEmpty()) {
        const QList<QByteArray> args = line.split(' ');
        if (args.count() == 2 && args.at(1) == "+")
            authenticate(true);
        if (!connection->isConnected())
            QMetaObject::invokeMethod(q, "_irc_resumeHandshake", Qt::QueuedConnection);
        return;
    }

    IrcMessage* msg = IrcMessage::fromData(line, connection);
    if (msg) {
        msg->setEncoding(connection->encoding());

        if (!msg->tag("batch").isNull() && batchMessage(msg))
            return;

        switch (msg->type()) {
        case IrcMessage::Batch:
            if (handleBatchMessage(static_cast<IrcBatchMessage*>(msg)))
                return;
            break;
        case IrcMessage::Capability:
            handleCapabilityMessage(static_cast<IrcCapabilityMessage*>(msg));
            break;
        case IrcMessage::Nick:
            if (msg->isOwn())
                q->setNickName(static_cast<IrcNickMessage*>(msg)->newNick());
            break;
        case IrcMessage::Numeric:
            handleNumericMessage(static_cast<IrcNumericMessage*>(msg));
            break;
        case IrcMessage::Ping:
            connection->sendRaw("PONG " + static_cast<IrcPingMessage*>(msg)->argument());
            break;
        case IrcMessage::Private:
            handlePrivateMessage(static_cast<IrcPrivateMessage*>(msg));
            break;
        default:
            break;
        }
        q->receiveMessage(msg);
    } else {
        qWarning() << "IrcProtocolPrivate::processLine(): unknown message:" << line;
    }
}

bool IrcProtocolPrivate::batchMessage(IrcMessage* msg)
{
    QString tag = msg->tags().value("batch").toString();
    IrcBatchMessage* batch = batches.value(tag);
    if (batch) {
        msg->setParent(batch);
        IrcMessagePrivate::get(batch)->batch += msg;
        return true;
    }
    return false;
}

bool IrcProtocolPrivate::handleBatchMessage(IrcBatchMessage* msg)
{
    Q_Q(IrcProtocol);
    QString tag = msg->parameters().value(0);
    if (tag.startsWith("+")) {
        batches.insert(msg->tag(), msg);
        return true;
    } else if (tag.startsWith("-")) {
        IrcBatchMessage* batch = batches.take(msg->tag());
        if (batch) {
            q->receiveMessage(batch);
            msg->deleteLater();
            return true;
        }
    }
    return false;
}

void IrcProtocolPrivate::handleNumericMessage(IrcNumericMessage* msg)
{
    Q_Q(IrcProtocol);
    switch (msg->code()) {
    case Irc::RPL_WELCOME:
        motd = false;
        q->setNickName(msg->parameters().value(0));
        q->setStatus(IrcConnection::Connected);
        break;
    case Irc::RPL_ISUPPORT: {
        foreach (const QString& param, msg->parameters().mid(1)) {
            QStringList keyValue = param.split("=", Qt::SkipEmptyParts);
            info.insert(keyValue.value(0), keyValue.value(1));
        }
        if (motd)
            q->setInfo(info);
        break;
    }
    case Irc::ERR_NOMOTD:
    case Irc::RPL_MOTDSTART:
        motd = true;
        q->setInfo(info);
        break;
    case Irc::ERR_NICKNAMEINUSE:
    case Irc::ERR_NICKCOLLISION: {
        QString reserved = msg->parameters().value(1);
        while (currentNick + 1 < connection->nickNames().count()) {
            QString alternate = connection->nickNames().value(++currentNick);
            if (!alternate.isEmpty() && alternate != reserved) {
                connection->setNickName(alternate);
                return;
            }
        }
        QString alternate = connection->nickName();
        emit connection->nickNameReserved(&alternate);
        if (!alternate.isEmpty() && alternate != connection->nickName()) {
            connection->setNickName(alternate);
        } else {
            emit connection->nickNameRequired(reserved, &alternate);
            if (!alternate.isEmpty() && alternate != connection->nickName())
                connection->setNickName(alternate);
        }
        break;
    }
    case Irc::ERR_BADCHANNELKEY: {
        QString key;
        QString channel = msg->parameters().value(1);
        emit connection->channelKeyRequired(channel, &key);
        if (!key.isEmpty())
            connection->sendCommand(IrcCommand::createJoin(channel, key));
        break;
    }
    default:
        break;
    }
}

void IrcProtocolPrivate::handlePrivateMessage(IrcPrivateMessage* msg)
{
    if (msg->isRequest()) {
        IrcCommand* reply = IrcConnectionPrivate::get(connection)->createCtcpReply(msg);
        if (reply)
            connection->sendCommand(reply);
    }
}

static void handleCapability(QSet<QString>* caps, const QString& cap)
{
    Q_ASSERT(caps);
    // sticky modifier (once the cap is enabled, it cannot be disabled)
    QLatin1Char stickyMod('=');
    // ack modifier (the cap must be acked by the client to fully enable/disable)
    QLatin1Char ackMod('~');
    // disable modifier (the cap should be disabled)
    QLatin1Char disMod('-');

    QString name = cap;
    while (name.startsWith(stickyMod) || name.startsWith(ackMod))
        name.remove(0, 1);

    if (name.startsWith(disMod))
        caps->remove(name.mid(1));
    else
        caps->insert(name);
}

void IrcProtocolPrivate::handleCapabilityMessage(IrcCapabilityMessage* msg)
{
    Q_Q(IrcProtocol);
    const bool connected = connection->isConnected();
    const QString subCommand = msg->subCommand();
    if (subCommand == "LS") {
        QSet<QString> availableCaps = IrcPrivate::listToSet(connection->network()->availableCapabilities());
        foreach (const QString& cap, msg->capabilities())
            handleCapability(&availableCaps, cap);
        q->setAvailableCapabilities(availableCaps);

        if (!connected && msg->parameter(2) != "*") {
            QMetaObject::invokeMethod(connection->network(), "requestingCapabilities");
            QSet<QString> requestedCaps;
            QSet<QString> activeCaps = IrcPrivate::listToSet(connection->network()->activeCapabilities());
            foreach (const QString& cap, connection->network()->requestedCapabilities()) {
                if (availableCaps.contains(cap) && !activeCaps.contains(cap))
                    requestedCaps += cap;
            }
            const QStringList params = msg->parameters();
            if (params.value(params.count() - 1) != QLatin1String("*")) {
                if (!connection->saslMechanism().isEmpty()) {
                    foreach (const QString& cap, availableCaps) {
                        QStringList capParts = cap.split('=');
                        if (capParts.length() == 2) {
                            QString capName = capParts[0];
                            if (capName.compare(QLatin1String("sasl"), Qt::CaseInsensitive) == 0) {
                                // The server has advertised supporting SASL with a list of supported SASL Mechanisms, ensure our supported SASL Mechanism is part of that list before accepting the SASL capability
                                QStringList serverSaslMethods = capParts[1].split(',');
                                if (serverSaslMethods.contains(connection->saslMechanism())) {
                                    requestedCaps += QLatin1String("sasl");
                                }
                            }
                        } else if (cap.compare(QLatin1String("sasl"), Qt::CaseInsensitive) == 0) {
                            requestedCaps += QLatin1String("sasl");
                        }
                    }
                }
            }
            if (!requestedCaps.isEmpty())
                connection->sendRaw("CAP REQ :" + QStringList(IrcPrivate::setToList(requestedCaps)).join(" "));
            else
                QMetaObject::invokeMethod(q, "_irc_resumeHandshake", Qt::QueuedConnection);
        }
    } else if (subCommand == "ACK" || subCommand == "NAK") {
        bool auth = false;
        if (subCommand == "ACK") {
            QSet<QString> activeCaps = IrcPrivate::listToSet(connection->network()->activeCapabilities());
            foreach (const QString& cap, msg->capabilities()) {
                handleCapability(&activeCaps, cap);
                if (cap == "sasl" && !connection->saslMechanism().isEmpty() && !connection->password().isEmpty())
                    auth = connection->sendRaw("AUTHENTICATE " + connection->saslMechanism());
            }
            q->setActiveCapabilities(activeCaps);
        }

        if (!connected && !auth)
            QMetaObject::invokeMethod(q, "_irc_resumeHandshake", Qt::QueuedConnection);
    } else if (subCommand == "NEW") {
        QStringList requestedCaps;
        QSet<QString> availableCaps = IrcPrivate::listToSet(connection->network()->availableCapabilities());
        foreach (const QString& cap, msg->capabilities()) {
            if (connection->network()->requestedCapabilities().contains(cap))
                requestedCaps += cap;
            availableCaps.insert(cap);
        }
        q->setAvailableCapabilities(availableCaps);

        if (!requestedCaps.empty()) {
            QMetaObject::invokeMethod(connection->network(), "requestingCapabilities");
            connection->sendRaw("CAP REQ :" + requestedCaps.join(" "));
        }
    } else if (subCommand == "DEL") {
        QSet<QString> activeCaps =  IrcPrivate::listToSet(connection->network()->activeCapabilities());
        QSet<QString> availableCaps = IrcPrivate::listToSet(connection->network()->availableCapabilities());
        foreach (const QString& cap, msg->capabilities()) {
            activeCaps.remove(cap);
            availableCaps.remove(cap);
        }
        q->setActiveCapabilities(activeCaps);
        q->setAvailableCapabilities(availableCaps);
    }
}

void IrcProtocolPrivate::_irc_pauseHandshake()
{
    if (connection->network()->skipCapabilityValidation()) {
        connection->sendRaw("CAP REQ :" + connection->network()->requestedCapabilities().join(" "));
    } else {
        // Send CAP LS first; if the server understands it this will
        // temporarily pause the handshake until CAP END is sent, so we
        // know whether the server supports the CAP extension.
        connection->sendData("CAP LS 302");
    }
    resumed = false;
    authed = false;
}

void IrcProtocolPrivate::_irc_resumeHandshake()
{
    if (!resumed && !connection->isConnected()) {
        if (!authed && !connection->saslMechanism().isEmpty() && !connection->password().isEmpty())
            authenticate(false);
        connection->sendData("CAP END");
    }
    resumed = true;
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new IRC protocol for \a connection.
 */
IrcProtocol::IrcProtocol(IrcConnection* connection) : QObject(connection), d_ptr(new IrcProtocolPrivate)
{
    Q_D(IrcProtocol);
    d->q_ptr = this;
    d->connection = connection;
    d->composer = new IrcMessageComposer(connection);
    connect(d->composer, SIGNAL(messageComposed(IrcMessage*)), this, SLOT(receiveMessage(IrcMessage*)));
}

/*!
    Destructs the IRC protocol.
 */
IrcProtocol::~IrcProtocol()
{
    Q_D(IrcProtocol);
    delete d->composer;
}

/*!
    This property holds the connection.

    \par Access function:
    \li \ref IrcConnection* <b>connection</b>() const
 */
IrcConnection* IrcProtocol::connection() const
{
    Q_D(const IrcProtocol);
    return d->connection;
}

/*!
    This property holds the socket.

    \par Access functions:
    \li \ref QAbstractSocket* <b>socket</b>() const
 */
QAbstractSocket* IrcProtocol::socket() const
{
    Q_D(const IrcProtocol);
    return d->connection->socket();
}

/*!
    This method is called when the connection has been established.

    The default implementation sends the \c NICK, \c USER and \c PASSWORD commands as defined in
    <a href="http://tools.ietf.org/html/rfc1459">RFC 1459</a>.

    Furthermore, it sends a <tt>CAP LS</tt> command as specified in
    <a href="http://tools.ietf.org/html/draft-mitchell-irc-capabilities-01">IRC Client Capabilities Extension</a>.
 */
void IrcProtocol::open()
{
    Q_D(IrcProtocol);
    d->_irc_pauseHandshake();

    if (d->connection->saslMechanism().isEmpty() && !d->connection->password().isEmpty())
        d->authenticate(false);

    QString nick = d->connection->nickName();
    if (nick.isEmpty())
        nick = d->connection->nickNames().value(0);

    d->connection->sendRaw(QString("NICK %1").arg(nick));
    d->connection->sendRaw(QString("USER %1 hostname servername :%2").arg(d->connection->userName(), d->connection->realName()));
}

/*!
    This method is called when the connection has been lost.
 */
void IrcProtocol::close()
{
    setActiveCapabilities(QSet<QString>());
    setAvailableCapabilities(QSet<QString>());
}

/*!
    This method is called when the \ref socket has new data available for read.

    The default implementation reads lines as specified in
    <a href="http://tools.ietf.org/html/rfc1459">RFC 1459</a>.

    \sa socket
 */
void IrcProtocol::read()
{
    Q_D(IrcProtocol);
    d->buffer += socket()->readAll();
    // try reading RFC compliant message lines first
    d->readLines("\r\n");
    // fall back to RFC incompliant lines...
    d->readLines("\n");
}

/*!
    This method is called when raw \a data should be written to the \ref socket.

    The default implementation writes the data and appends \c "\r\n" as specified in
    <a href="http://tools.ietf.org/html/rfc1459">RFC 1459</a>.

    \sa socket
 */
bool IrcProtocol::write(const QByteArray& data)
{
    return socket()->write(data + QByteArray("\r\n")) != -1;
}

/*!
    This method should be called by the protocol implementation
    to make the underlying IRC connection receive a \a message.

    \sa IrcConnection::messageReceived()
 */
void IrcProtocol::receiveMessage(IrcMessage* message)
{
    Q_D(IrcProtocol);
    IrcConnectionPrivate* priv = IrcConnectionPrivate::get(d->connection);
    if (priv->receiveMessage(message) && message->type() == IrcMessage::Numeric)
        d->composer->composeMessage(static_cast<IrcNumericMessage*>(message));
}

/*!
    This method should be called by the protocol implementation to
    notify the underlying IRC connection about a nick \a name change.

    \sa IrcConnection::nickName
 */
void IrcProtocol::setNickName(const QString& name)
{
    Q_D(IrcProtocol);
    d->currentNick = d->connection->nickNames().indexOf(name);
    IrcConnectionPrivate* priv = IrcConnectionPrivate::get(d->connection);
    priv->setNick(name);
}

/*!
    This method should be called by the protocol implementation
    to notify the underlying IRC connection about a \a status change.

    \sa IrcConnection::status
 */
void IrcProtocol::setStatus(IrcConnection::Status status)
{
    Q_D(IrcProtocol);
    IrcConnectionPrivate* priv = IrcConnectionPrivate::get(d->connection);
    priv->setStatus(status);
}

/*!
    This method should be called by the protocol implementation
    to initialize the underlying IRC network connection \a info.

    \sa IrcNetwork::initialized
 */
void IrcProtocol::setInfo(const QHash<QString, QString>& info)
{
    Q_D(IrcProtocol);
    if (!info.isEmpty()) {
        IrcConnectionPrivate* priv = IrcConnectionPrivate::get(d->connection);
        priv->setInfo(info);
        d->info.clear();
    }
}

/*!
    This method should be called by the protocol implementation to notify
    the underlying IRC network about a change in available \a capabilities.

    \sa IrcNetwork::availableCapabilities
 */
void IrcProtocol::setAvailableCapabilities(const QSet<QString>& capabilities)
{
    Q_D(IrcProtocol);
    IrcNetworkPrivate* priv = IrcNetworkPrivate::get(d->connection->network());
    priv->setAvailableCapabilities(capabilities);
}

/*!
    This method should be called by the protocol implementation to notify
    the underlying IRC network about a change in active \a capabilities.

    \sa IrcNetwork::activeCapabilities
 */
void IrcProtocol::setActiveCapabilities(const QSet<QString>& capabilities)
{
    Q_D(IrcProtocol);
    IrcNetworkPrivate* priv = IrcNetworkPrivate::get(d->connection->network());
    priv->setActiveCapabilities(capabilities);
}

#include "moc_ircprotocol.cpp"

IRC_END_NAMESPACE
