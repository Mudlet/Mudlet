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

#include "ircmessage.h"
#include "ircmessage_p.h"
#include "ircconnection.h"
#include "ircconnection_p.h"
#include "ircmessagecomposer_p.h"
#include "ircnetwork_p.h"
#include "irccommand.h"
#include "irccore_p.h"
#include "irc.h"
#include <QMetaEnum>
#include <QVariant>
#include <QDebug>
#include <functional>

IRC_BEGIN_NAMESPACE

/*!
    \file ircmessage.h
    \brief \#include &lt;IrcMessage&gt;
 */

/*!
    \class IrcMessage ircmessage.h <IrcMessage>
    \ingroup core
    \ingroup message
    \brief The base class of all messages.

    IRC messages are received from an IRC server. IrcConnection translates received
    messages into IrcMessage instances and emits the IrcConnection::messageReceived()
    signal upon message received.

    Subclasses of IrcMessage contain specialized accessors for parameters that
    are specific to the particular type of message. See IrcMessage::Type for
    the list of supported message types.

    \sa IrcConnection::messageReceived(), IrcMessage::Type
 */

/*!
    \enum IrcMessage::Type
    This enum describes the supported message types.
 */

/*!
    \since 3.3
    \var IrcMessage::Account
    \brief An account notify message (IrcAccountMessage).
    \sa \ref ircv3
 */

/*!
    \since 3.3
    \var IrcMessage::Away
    \brief An away message (IrcAwayMessage).
 */

/*!
    \since 3.5
    \var IrcMessage::Batch
    \brief A batch message (IrcBatchMessage).
    \sa \ref ircv3
 */

/*!
    \var IrcMessage::Capability
    \brief A capability message (IrcCapabilityMessage).
    \sa \ref ircv3
 */

/*!
    \var IrcMessage::Error
    \brief An error message (IrcErrorMessage).
 */

/*!
    \since 3.4
    \var IrcMessage::HostChange
    \brief A host change message (IrcHostChangeMessage).
 */

/*!
    \var IrcMessage::Invite
    \brief An invite message (IrcInviteMessage).
 */

/*!
    \var IrcMessage::Join
    \brief A join message (IrcJoinMessage).
 */

/*!
    \var IrcMessage::Kick
    \brief A kick message (IrcKickMessage).
 */

/*!
    \var IrcMessage::Mode
    \brief A mode message (IrcModeMessage).
 */

/*!
    \var IrcMessage::Motd
    \brief A message of the day (IrcMotdMessage).
 */

/*!
    \var IrcMessage::Names
    \brief A names message (IrcNamesMessage).
 */

/*!
    \var IrcMessage::Nick
    \brief A nick message (IrcNickMessage).
 */

/*!
    \var IrcMessage::Notice
    \brief A notice message (IrcNoticeMessage).
 */

/*!
    \var IrcMessage::Numeric
    \brief A numeric message (IrcNumericMessage).
 */

/*!
    \var IrcMessage::Part
    \brief A part message (IrcPartMessage).
 */

/*!
    \var IrcMessage::Ping
    \brief A ping message (IrcPingMessage).
 */

/*!
    \var IrcMessage::Pong
    \brief A pong message (IrcPongMessage).
 */

/*!
    \var IrcMessage::Private
    \brief A private message (IrcPrivateMessage).
 */

/*!
    \var IrcMessage::Quit
    \brief A quit message (IrcQuitMessage).
 */

/*!
    \var IrcMessage::Topic
    \brief A topic message (IrcTopicMessage).
 */

/*!
    \var IrcMessage::Unknown
    \brief An unknown message (IrcMessage).
 */

/*!
    \since 3.3
    \var IrcMessage::Whois
    \brief A whois reply message (IrcWhoisMessage).
 */

/*!
    \since 3.3
    \var IrcMessage::Whowas
    \brief A whowas reply message (IrcWhowasMessage).
 */

/*!
    \since 3.1
    \var IrcMessage::WhoReply
    \brief A who reply message (IrcWhoReplyMessage).
 */

/*!
    \enum IrcMessage::Flag
    This enum describes the supported message flags.
 */

/*!
    \var IrcMessage::None
    \brief The message has no flags.
 */

/*!
    \var IrcMessage::Own
    \brief The message is user's own message.
 */

/*!
    \var IrcMessage::Identified
    \brief The message is identified.
    \deprecated Use IrcMessage::account instead.
 */

/*!
    \var IrcMessage::Unidentified
    \brief The message is unidentified.
    \deprecated Use IrcMessage::account instead.
 */

/*!
    \since 3.2
    \var IrcMessage::Playback
    \brief The message is playback.
 */

/*!
    \since 3.3
    \var IrcMessage::Implicit
    \brief The message is an implicit "reply" after joining a channel.
 */

extern bool irc_is_supported_encoding(const QByteArray& encoding); // ircmessagedecoder.cpp

static IrcMessage* irc_create_message(const QString& command, IrcConnection* connection)
{
    typedef std::function<IrcMessage *(IrcConnection *)> IrcMessageFactory;

    static const QHash<QString, IrcMessageFactory> factories = {
        {"ACCOUNT", [](IrcConnection* c) { return new IrcAccountMessage(c); }},
        {"AWAY", [](IrcConnection* c) { return new IrcAwayMessage(c); }},
        {"BATCH", [](IrcConnection* c) { return new IrcBatchMessage(c); }},
        {"CAP", [](IrcConnection* c) { return new IrcCapabilityMessage(c); }},
        {"ERROR", [](IrcConnection* c) { return new IrcErrorMessage(c); }},
        {"CHGHOST", [](IrcConnection* c) { return new IrcHostChangeMessage(c); }},
        {"INVITE", [](IrcConnection* c) { return new IrcInviteMessage(c); }},
        {"JOIN", [](IrcConnection* c) { return new IrcJoinMessage(c); }},
        {"KICK", [](IrcConnection* c) { return new IrcKickMessage(c); }},
        {"MODE", [](IrcConnection* c) { return new IrcModeMessage(c); }},
        {"NICK", [](IrcConnection* c) { return new IrcNickMessage(c); }},
        {"NOTICE", [](IrcConnection* c) { return new IrcNoticeMessage(c); }},
        {"PART", [](IrcConnection* c) { return new IrcPartMessage(c); }},
        {"PING", [](IrcConnection* c) { return new IrcPingMessage(c); }},
        {"PONG", [](IrcConnection* c) { return new IrcPongMessage(c); }},
        {"PRIVMSG", [](IrcConnection* c) { return new IrcPrivateMessage(c); }},
        {"QUIT", [](IrcConnection* c) { return new IrcQuitMessage(c); }},
        {"TOPIC", [](IrcConnection* c) { return new IrcTopicMessage(c); }},
    };

    IrcMessageFactory factory = factories.value(command);
    if (factory)
        return factory(connection);

    bool isNumeric = false;
    int number = command.toInt(&isNumeric);
    if (isNumeric && number > 0)
        return new IrcNumericMessage(connection);

    return new IrcMessage(connection);
}

/*!
    Constructs a new IrcMessage with \a connection.
 */
IrcMessage::IrcMessage(IrcConnection* connection) : QObject(connection), d_ptr(new IrcMessagePrivate)
{
    Q_D(IrcMessage);
    d->connection = connection;
}

/*!
    Destructs the message.
 */
IrcMessage::~IrcMessage()
{
}

/*!
    This property holds the message connection.

    \par Access function:
    \li \ref IrcConnection* <b>connection</b>() const
 */
IrcConnection* IrcMessage::connection() const
{
    Q_D(const IrcMessage);
    return d->connection;
}

/*!
    This property holds the message network.

    \par Access function:
    \li \ref IrcNetwork* <b>network</b>() const
 */
IrcNetwork* IrcMessage::network() const
{
    Q_D(const IrcMessage);
    return d->connection ? d->connection->network() : nullptr;
}

/*!
    This property holds the message type.

    \par Access function:
    \li \ref IrcMessage::Type <b>type</b>() const
 */
IrcMessage::Type IrcMessage::type() const
{
    Q_D(const IrcMessage);
    return d->type;
}

/*!
    \since 3.2
    \property bool IrcMessage::own

    This property holds whether the message is user's own.

    This property is provided for convenience. It is equivalent
    of testing for the IrcMessage::Own flag.

    \par Access function:
    \li bool <b>isOwn</b>() const
 */
bool IrcMessage::isOwn() const
{
    return flags() & Own;
}

/*!
    \since 3.5
    \property bool IrcMessage::implicit

    This property holds whether the message is an implicit "reply"
    after joining a channel.

    This property is provided for convenience. It is equivalent
    of testing for the IrcMessage::Implicit flag.

    \par Access function:
    \li bool <b>isImplicit</b>() const
 */
bool IrcMessage::isImplicit() const
{
    return flags() & Implicit;
}

/*!
    This property holds the message flags.

    \par Access function:
    \li \ref IrcMessage::Flag "IrcMessage::Flags" <b>flags</b>() const
    \li void <b>setFlags</b>(\ref IrcMessage::Flag "IrcMessage::Flags" flags) (\b Since 3.2)

    \sa testFlag(), setFlag()
 */
IrcMessage::Flags IrcMessage::flags() const
{
    Q_D(const IrcMessage);
    if (d->flags == -1) {
        d->flags = IrcMessage::None;
        if (d->connection && !d->prefix().isEmpty() && d->nick() == d->connection->nickName())
            d->flags |= IrcMessage::Own;
    }
    return IrcMessage::Flags(d->flags);
}

void IrcMessage::setFlags(IrcMessage::Flags flags)
{
    Q_D(IrcMessage);
    d->flags = flags;
}

/*!
    \since 3.5

    Returns \c true if the \a flag is on; otherwise \c false.

    \sa IrcMessage::flags
 */
bool IrcMessage::testFlag(Flag flag) const
{
    return flags().testFlag(flag);
}

/*!
    \since 3.5

    Sets whether the \a flag is \a on.

    \sa IrcMessage::flags
 */
void IrcMessage::setFlag(Flag flag, bool on)
{
    if (on)
        setFlags(flags() | flag);
    else
        setFlags(flags() & ~flag);
}

/*!
    This property holds the message command.

    \par Access functions:
    \li QString <b>command</b>() const
    \li void <b>setCommand</b>(const QString& command)
 */
QString IrcMessage::command() const
{
    Q_D(const IrcMessage);
    return d->command();
}

void IrcMessage::setCommand(const QString& command)
{
    Q_D(IrcMessage);
    d->setCommand(command);
}

/*!
    This property holds the message sender prefix.

    The prefix consists of \ref nick, \ref ident and \ref host as specified in RFC 1459:
    <pre>
    &lt;prefix&gt; ::= &lt;\ref nick&gt; [ '!' &lt;\ref ident&gt; ] [ '@' &lt;\ref host&gt; ]
    </pre>

    \par Access functions:
    \li QString <b>prefix</b>() const
    \li void <b>setPrefix</b>(const QString& prefix)
 */
QString IrcMessage::prefix() const
{
    Q_D(const IrcMessage);
    return d->prefix();
}

void IrcMessage::setPrefix(const QString& prefix)
{
    Q_D(IrcMessage);
    d->setPrefix(prefix);
}

/*!
    This property holds the message sender nick.

    Nick is part of the sender \ref prefix as specified in RFC 1459:
    <pre>
    <b>&lt;nick&gt;</b> [ '!' &lt;\ref ident&gt; ] [ '@' &lt;\ref host&gt; ]
    </pre>

    \par Access function:
    \li QString <b>nick</b>() const
 */
QString IrcMessage::nick() const
{
    Q_D(const IrcMessage);
    return d->nick();
}

/*!
    This property holds the message sender ident.

    Ident is part of the sender \ref prefix as specified in RFC 1459:
    <pre>
    &lt;\ref nick&gt; [ '!' <b>&lt;ident&gt;</b> ] [ '@' &lt;\ref host&gt; ]
    </pre>

    \par Access function:
    \li QString <b>ident</b>() const
 */
QString IrcMessage::ident() const
{
    Q_D(const IrcMessage);
    return d->ident();
}

/*!
    This property holds the message sender host.

    Host is part of the sender \ref prefix as specified in RFC 1459:
    <pre>
    &lt;\ref nick&gt; [ '!' &lt;\ref ident&gt; ] [ '@' <b>&lt;host&gt;</b> ]
    </pre>

    \par Access function:
    \li QString <b>host</b>() const
 */
QString IrcMessage::host() const
{
    Q_D(const IrcMessage);
    return d->host();
}

/*!
    \since 3.4

    This property holds the services account of the message sender.

    \note Only set if the \c account-tag capability is
    enabled and the user has identified with services.

    \par Access function:
    \li QString <b>account</b>() const

    \sa \ref ircv3
 */
QString IrcMessage::account() const
{
    Q_D(const IrcMessage);
    return d->tags().value(QStringLiteral("account")).toString();
}

/*!
    This property holds the message parameters.

    \par Access functions:
    \li QStringList <b>parameters</b>() const
    \li void <b>setParameters</b>(const QStringList& parameters)
 */
QStringList IrcMessage::parameters() const
{
    Q_D(const IrcMessage);
    return d->params();
}

void IrcMessage::setParameters(const QStringList& parameters)
{
    Q_D(IrcMessage);
    d->setParams(parameters);
}

/*!
    \since 3.5

    Returns the parameter at the specified \a index.
 */
QString IrcMessage::parameter(int index) const
{
    Q_D(const IrcMessage);
    return d->param(index);
}

/*!
    \since 3.5

    Sets the \a parameter at the specified \a index.
 */
void IrcMessage::setParameter(int index, const QString& parameter)
{
    Q_D(IrcMessage);
    QStringList params = d->params();
    while (index >= params.count())
        params.append(QString());
    params[index] = parameter;
    d->setParams(params);
}

/*!
    This property holds the message time stamp.

    \note When the \c server-time capability is enabled, the time
          stamp is automatically set from the \c time message tag.

    \par Access functions:
    \li QDateTime <b>timeStamp</b>() const
    \li void <b>setTimeStamp</b>(const QDateTime& timeStamp)

    \sa \ref ircv3
 */
QDateTime IrcMessage::timeStamp() const
{
    Q_D(const IrcMessage);
    return d->timeStamp;
}

void IrcMessage::setTimeStamp(const QDateTime& timeStamp)
{
    Q_D(IrcMessage);
    d->timeStamp = timeStamp;
}

/*!
    This property holds the FALLBACK encoding for the message.

    The fallback encoding is used when the message is detected not
    to be valid UTF-8 and the consequent auto-detection of message
    encoding fails. See QTextCodec::availableCodes() for the list of
    supported encodings.

    The default value is ISO-8859-15.

    \par Access functions:
    \li QByteArray <b>encoding</b>() const
    \li void <b>setEncoding</b>(const QByteArray& encoding)

    \sa QTextCodec::availableCodecs(), QTextCodec::codecForLocale()
 */
QByteArray IrcMessage::encoding() const
{
    Q_D(const IrcMessage);
    return d->encoding;
}

void IrcMessage::setEncoding(const QByteArray& encoding)
{
    Q_D(IrcMessage);
    if (!irc_is_supported_encoding(encoding)) {
        qWarning() << "IrcMessage::setEncoding(): unsupported encoding" << encoding;
        return;
    }
    d->encoding = encoding;
    d->invalidate();
}

/*!
    \since 3.1

    This property holds the message tags.

    \par Access functions:
    \li QVariantMap <b>tags</b>() const
    \li void <b>setTags</b>(const QVariantMap& tags)

    \sa \ref ircv3
 */
QVariantMap IrcMessage::tags() const
{
    Q_D(const IrcMessage);
    return d->tags();
}

void IrcMessage::setTags(const QVariantMap& tags)
{
    Q_D(IrcMessage);
    d->setTags(tags);
}

/*!
    \since 3.5

    Returns the value of the specified tag.

    \sa \ref ircv3
 */
QVariant IrcMessage::tag(const QString& name) const
{
    Q_D(const IrcMessage);
    return d->tags().value(name);
}

/*!
    \since 3.5

    Sets the value of the specified tag.

    \sa \ref ircv3
 */
void IrcMessage::setTag(const QString& name, const QVariant& value)
{
    Q_D(IrcMessage);
    QVariantMap tags = d->tags();
    tags.insert(name, value);
    d->setTags(tags);
}

/*!
    Creates a new message from \a data and \a connection.
 */
IrcMessage* IrcMessage::fromData(const QByteArray& data, IrcConnection* connection)
{
    IrcMessageData md = IrcMessageData::fromData(data);
    IrcMessage* message = irc_create_message(md.command, connection);
    Q_ASSERT(message);
    message->d_ptr->data = md;
    QByteArray tag = md.tags.value("time");
    if (!tag.isEmpty()) {
        QDateTime ts = QDateTime::fromString(QString::fromUtf8(tag), Qt::ISODate);
        if (ts.isValid())
            message->d_ptr->timeStamp = ts.toTimeSpec(Qt::LocalTime);
    }
    return message;
}

/*!
    Creates a new message from \a prefix, \a command and \a parameters with \a connection.
 */
IrcMessage* IrcMessage::fromParameters(const QString& prefix, const QString& command, const QStringList& parameters, IrcConnection* connection)
{
    IrcMessage* message = irc_create_message(command, connection);
    Q_ASSERT(message);
    message->setPrefix(prefix);
    message->setCommand(command);
    message->setParameters(parameters);
    return message;
}

/*!
    \since 3.5

    Clones the message.
 */
IrcMessage* IrcMessage::clone(QObject* parent) const
{
    Q_D(const IrcMessage);
    IrcMessage* msg = irc_create_message(d->command(), d->connection);
    if (msg) {
        msg->setParent(parent);
        IrcMessagePrivate* p = IrcMessagePrivate::get(msg);
        p->timeStamp = d->timeStamp;
        p->encoding = d->encoding;
        p->flags = d->flags;
        p->data = d->data;
        foreach (IrcMessage* bm, d->batch)
            p->batch += bm->clone(msg);
        p->m_nick = d->m_nick;
        p->m_ident = d->m_ident;
        p->m_host = d->m_host;
        p->m_prefix = d->m_prefix;
        p->m_command = d->m_command;
        p->m_params = d->m_params;
        p->m_tags = d->m_tags;
    }
    return msg;
}

/*!
    \property bool IrcMessage::valid
    This property is \c true if the message is valid; otherwise \c false.

    A message is considered valid if the prefix is not empty
    and the parameters match the message.

    \par Access function:
    \li bool <b>isValid</b>() const
 */
bool IrcMessage::isValid() const
{
    Q_D(const IrcMessage);
    return d->connection && !prefix().isNull();
}

/*!
    Returns the message as received from an IRC server.
 */
QByteArray IrcMessage::toData() const
{
    Q_D(const IrcMessage);
    return d->content();
}

/*!
    \since 3.3
    \class IrcAwayMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents an away message.
 */

/*!
    Constructs a new IrcAwayMessage with \a connection.
 */
IrcAwayMessage::IrcAwayMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Away;
}

/*!
    This property holds the message content.

    \par Access function:
    \li QString <b>content</b>() const
 */
QString IrcAwayMessage::content() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    \property bool IrcAwayMessage::reply
    This property holds whether the message is a reply.

    Away messages are sent in three situations:
    \li as a reply when sending a message (\c true),
    \li as a reply when setting away status (\c true),
    \li as an away notification when the \c away-notify capability is enabled (\c false).

    \par Access function:
    \li bool <b>isReply</b>() const

    \sa Irc::RPL_AWAY, Irc::RPL_NOWAWAY, Irc::RPL_UNAWAY, \ref ircv3
 */
bool IrcAwayMessage::isReply() const
{
    Q_D(const IrcMessage);
    return d->command().toInt() != 0;
}

/*!
    \property bool IrcAwayMessage::away
    This property holds whether the user is away or back.

    \par Access function:
    \li bool <b>isAway</b>() const
 */
bool IrcAwayMessage::isAway() const
{
    Q_D(const IrcMessage);
    int rpl = d->command().toInt();
    return rpl == Irc::RPL_AWAY || rpl == Irc::RPL_NOWAWAY
            || (d->command() == QLatin1String("AWAY") && !d->param(0).isEmpty());
}

bool IrcAwayMessage::isValid() const
{
    return IrcMessage::isValid();
}

/*!
    \since 3.3
    \class IrcAccountMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents an account notify message.
    \sa \ref ircv3
 */

/*!
    Constructs a new IrcAccountMessage with \a connection.
 */
IrcAccountMessage::IrcAccountMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Account;
}

/*!
    This property holds the account name.

    \note The account name is empty if the user has logged out.

    \par Access function:
    \li QString <b>account</b>() const
 */
QString IrcAccountMessage::account() const
{
    Q_D(const IrcMessage);
    const QString p = d->param(0);
    if (p != QLatin1String("*"))
        return p;
    return QString();
}

bool IrcAccountMessage::isValid() const
{
    Q_D(const IrcMessage);
    return IrcMessage::isValid() && !d->param(0).isEmpty();
}

/*!
    \since 3.5
    \class IrcBatchMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a batch message.
    \sa \ref ircv3
 */

/*!
    Constructs a new IrcBatchMessage with \a connection.
 */
IrcBatchMessage::IrcBatchMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Batch;
}

/*!
    This property holds the batch tag.

    \par Access function:
    \li QString <b>tag</b>() const
 */
QString IrcBatchMessage::tag() const
{
    Q_D(const IrcMessage);
    return d->param(0).mid(1);
}

/*!
    This property holds the batch type.

    \par Access function:
    \li QString <b>type</b>() const
 */
QString IrcBatchMessage::batch() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    This property holds the list of batched messages.

    \par Access function:
    \li QList<IrcMessage*> <b>messages</b>() const
 */
QList<IrcMessage*> IrcBatchMessage::messages() const
{
    Q_D(const IrcMessage);
    return d->batch;
}

bool IrcBatchMessage::isValid() const
{
    return IrcMessage::isValid() && !batch().isEmpty();
}

/*!
    \class IrcCapabilityMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a capability message.
    \sa \ref ircv3
 */

/*!
    Constructs a new IrcCapabilityMessage with \a connection.
 */
IrcCapabilityMessage::IrcCapabilityMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Capability;
}

/*!
    This property holds the subcommand.

    The following capability subcommands are defined:
    LS, LIST, REQ, ACK, NAK, CLEAR, END, NEW, DEL.

    \par Access function:
    \li QString <b>subCommand</b>() const
 */
QString IrcCapabilityMessage::subCommand() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    This property holds the capabilities.

    A list of capabilities may exist for the following
    subcommands: LS, LIST, REQ, ACK, NAK, NEW, DEL.

    \par Access function:
    \li QStringList <b>capabilities</b>() const
 */
QStringList IrcCapabilityMessage::capabilities() const
{
    Q_D(const IrcMessage);
    QStringList caps;
    QStringList params = d->params();
    if (params.count() > 2)
        caps = params.last().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    return caps;
}

bool IrcCapabilityMessage::isValid() const
{
    return IrcMessage::isValid();
}

/*!
    \class IrcErrorMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents an error message.
 */

/*!
    Constructs a new IrcErrorMessage with \a connection.
 */
IrcErrorMessage::IrcErrorMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Error;
}

/*!
    This property holds the error.

    \par Access function:
    \li QString <b>error</b>() const
 */
QString IrcErrorMessage::error() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

bool IrcErrorMessage::isValid() const
{
    return IrcMessage::isValid() && !error().isEmpty();
}

/*!
    \since 3.4
    \class IrcHostChangeMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a host change message.

    \sa \ref ircv3
 */

/*!
    Constructs a new IrcHostChangeMessage with \a connection.
 */
IrcHostChangeMessage::IrcHostChangeMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = HostChange;
}

/*!
    This property holds the user name.

    \par Access function:
    \li QString <b>user</b>() const
 */
QString IrcHostChangeMessage::user() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the new host.

    \par Access function:
    \li QString <b>host</b>() const
 */
QString IrcHostChangeMessage::host() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

bool IrcHostChangeMessage::isValid() const
{
    return IrcMessage::isValid() && !user().isEmpty() && !host().isEmpty();
}

/*!
    \class IrcInviteMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents an invite message.
 */

/*!
    Constructs a new IrcInviteMessage with \a connection.
 */
IrcInviteMessage::IrcInviteMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Invite;
}

/*!
    This property holds the user in question.

    \par Access function:
    \li QString <b>user</b>() const

    \sa \ref ircv3
 */
QString IrcInviteMessage::user() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the channel in question.

    \par Access function:
    \li QString <b>channel</b>() const
 */
QString IrcInviteMessage::channel() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    \property bool IrcInviteMessage::reply
    This property holds whether the message is a reply.

    Invite messages are sent in three situations:
    \li as a notification of a received invitation (\c false),
    \li as a reply when sending an invitation (\c true), or
    \li as an invite notification when the \c invite-notify capability is enabled (\c false).

    \par Access function:
    \li bool <b>isReply</b>() const

    \sa Irc::RPL_INVITING, Irc::RPL_INVITED, IrcInviteCommand, \ref ircv3
 */
bool IrcInviteMessage::isReply() const
{
    Q_D(const IrcMessage);
    int rpl = d->command().toInt();
    return rpl == Irc::RPL_INVITING || rpl == Irc::RPL_INVITED;
}

bool IrcInviteMessage::isValid() const
{
    return IrcMessage::isValid() && !user().isEmpty() && !channel().isEmpty();
}

/*!
    \class IrcJoinMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a join message.
 */

/*!
    Constructs a new IrcJoinMessage with \a connection.
 */
IrcJoinMessage::IrcJoinMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Join;
}

/*!
    This property holds the channel in question.

    \par Access function:
    \li QString <b>channel</b>() const
 */
QString IrcJoinMessage::channel() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    \since 3.3

    This property holds the account name of the user.

    \note Only set if the \c extended-join capability is
    enabled and the user has identified with services.

    \par Access function:
    \li QString <b>account</b>() const

    \sa \ref ircv3
 */
QString IrcJoinMessage::account() const
{
    Q_D(const IrcMessage);
    const QString p = d->param(1);
    if (p != QLatin1String("*"))
        return p;
    return QString();
}

/*!
    \since 3.3

    This property holds the real name of the user.

    \note Only set if the \c extended-join capability is enabled.

    \par Access function:
    \li QString <b>realName</b>() const

    \sa \ref ircv3
 */
QString IrcJoinMessage::realName() const
{
    Q_D(const IrcMessage);
    return d->param(2);
}

bool IrcJoinMessage::isValid() const
{
    return IrcMessage::isValid() && !channel().isEmpty();
}

/*!
    \class IrcKickMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a kick message.
 */

/*!
    Constructs a new IrcKickMessage with \a connection.
 */
IrcKickMessage::IrcKickMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Kick;
}

/*!
    This property holds the channel in question.

    \par Access function:
    \li QString <b>channel</b>() const
 */
QString IrcKickMessage::channel() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the user in question.

    \par Access function:
    \li QString <b>user</b>() const
 */
QString IrcKickMessage::user() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    This property holds the optional kick reason.

    \par Access function:
    \li QString <b>reason</b>() const
 */
QString IrcKickMessage::reason() const
{
    Q_D(const IrcMessage);
    return d->param(2);
}

bool IrcKickMessage::isValid() const
{
    return IrcMessage::isValid() && !channel().isEmpty() && !user().isEmpty();
}

/*!
    \class IrcModeMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a mode message.
 */

/*!
    \enum IrcModeMessage::Kind
    This enum describes the kind of modes.
 */

/*!
    \var IrcModeMessage::Channel
    \brief Channel mode
 */

/*!
    \var IrcModeMessage::User
    \brief User mode
 */

/*!
    Constructs a new IrcModeMessage with \a connection.
 */
IrcModeMessage::IrcModeMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Mode;
}

/*!
    This property holds the target channel or user in question.

    \par Access function:
    \li QString <b>target</b>() const
 */
QString IrcModeMessage::target() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the channel or user mode.

    \par Access function:
    \li QString <b>mode</b>() const
 */
QString IrcModeMessage::mode() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    This property holds the first mode argument.

    \par Access function:
    \li QString <b>argument</b>() const
 */
QString IrcModeMessage::argument() const
{
    Q_D(const IrcMessage);
    return d->param(2);
}

/*!
    \since 3.1

    This property holds the all mode arguments.

    \par Access function:
    \li QStringList <b>arguments</b>() const
 */
QStringList IrcModeMessage::arguments() const
{
    Q_D(const IrcMessage);
    return d->params().mid(2);
}

/*!
    \property bool IrcModeMessage::reply
    This property holds whether the message is a reply.

    Mode messages are sent when a mode changes (\c false)
    and when joining a channel (\c true).

    \par Access function:
    \li bool <b>isReply</b>() const

    \sa Irc::RPL_CHANNELMODEIS
 */
bool IrcModeMessage::isReply() const
{
    Q_D(const IrcMessage);
    int rpl = d->command().toInt();
    return rpl == Irc::RPL_CHANNELMODEIS;
}

/*!
    This property holds the kind of the mode.

    \par Access function:
    \li Kind <b>kind</b>() const
 */
IrcModeMessage::Kind IrcModeMessage::kind() const
{
    const QString m = mode().remove(QLatin1Char('+')).remove(QLatin1Char('-'));
    if (!m.isEmpty()) {
        QStringList channelModes;
        if (const IrcNetwork* net = network())
            channelModes = net->channelModes(IrcNetwork::AllTypes);
        if (!channelModes.isEmpty()) {
            for (int i = 0; i < m.length(); ++i) {
                if (!channelModes.contains(m.at(i)))
                    return User;
            }
        }
    }
    return Channel;
}

bool IrcModeMessage::isValid() const
{
    return IrcMessage::isValid() && !target().isEmpty() && !mode().isEmpty();
}

/*!
    \class IrcMotdMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a message of the day.
 */

/*!
    Constructs a new IrcMotdMessage with \a connection.
 */
IrcMotdMessage::IrcMotdMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Motd;
    setCommand(QStringLiteral("MOTD"));
}

/*!
    This property holds the message of the day lines.

    \par Access function:
    \li QStringList <b>lines</b>() const
 */
QStringList IrcMotdMessage::lines() const
{
    Q_D(const IrcMessage);
    return d->params().mid(1);
}

bool IrcMotdMessage::isValid() const
{
    Q_D(const IrcMessage);
    return IrcMessage::isValid() && !d->params().isEmpty();
}

/*!
    \class IrcNamesMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a names list message.
 */

/*!
    Constructs a new IrcNamesMessage with \a connection.
 */
IrcNamesMessage::IrcNamesMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Names;
    setCommand(QStringLiteral("NAMES"));
}

/*!
    This property holds the channel.

    \par Access function:
    \li QString <b>channel</b>() const
 */
QString IrcNamesMessage::channel() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the list of names.

    \par Access function:
    \li QStringList <b>names</b>() const
 */
QStringList IrcNamesMessage::names() const
{
    Q_D(const IrcMessage);
    return d->params().mid(1);
}

bool IrcNamesMessage::isValid() const
{
    Q_D(const IrcMessage);
    return IrcMessage::isValid() && !d->params().isEmpty();
}

/*!
    \class IrcNickMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a nick message.
 */

/*!
    Constructs a new IrcNickMessage with \a connection.
 */
IrcNickMessage::IrcNickMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Nick;
}

/*!
    This property holds the old nick.

    This property is provided for symmetry with \ref newNick
    and is equal to \ref nick.

    \par Access function:
    \li QString <b>oldNick</b>() const
 */
QString IrcNickMessage::oldNick() const
{
    Q_D(const IrcMessage);
    return d->nick();
}

/*!
    This property holds the new nick.

    \par Access function:
    \li QString <b>newNick</b>() const
 */
QString IrcNickMessage::newNick() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

bool IrcNickMessage::isValid() const
{
    return IrcMessage::isValid() && !newNick().isEmpty();
}

/*!
    \class IrcNoticeMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a notice message.
 */

/*!
    Constructs a new IrcNoticeMessage with \a connection.
 */
IrcNoticeMessage::IrcNoticeMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Notice;
}

/*!
    This property holds the target channel or user in question.

    \par Access function:
    \li QString <b>target</b>() const
 */
QString IrcNoticeMessage::target() const
{
    Q_D(const IrcMessage);
    if (d->connection) {
        const IrcNetwork* network = d->connection->network();
        return IrcNetworkPrivate::removePrefix(d->param(0), network->statusPrefixes());
    }
    return d->param(0);
}

/*!
    This property holds the message content.

    \par Access function:
    \li QString <b>content</b>() const
 */
QString IrcNoticeMessage::content() const
{
    Q_D(const IrcMessage);
    QString msg = d->param(1);
    if (isReply()) {
        msg.remove(0, 1);
        msg.chop(1);
    }
    return msg;
}

/*!
    \since 3.4

    This property holds the status prefix of the message.

    \par Access function:
    \li QString <b>statusPrefix</b>() const
 */
QString IrcNoticeMessage::statusPrefix() const
{
    Q_D(const IrcMessage);
    if (d->connection) {
        const IrcNetwork* network = d->connection->network();
        return IrcNetworkPrivate::getPrefix(d->param(0), network->statusPrefixes());
    }
    return QString();
}

/*!
    \property bool IrcNoticeMessage::private
    This property is \c true if the notice is private,
    or \c false if it is a channel notice.

    \par Access function:
    \li bool <b>isPrivate</b>() const
 */
bool IrcNoticeMessage::isPrivate() const
{
    Q_D(const IrcMessage);
    if (d->connection)
        return !target().compare(d->connection->nickName(), Qt::CaseInsensitive);
    return false;
}

/*!
    \property bool IrcNoticeMessage::reply
    This property is \c true if the message is a reply; otherwise \c false.

    \par Access function:
    \li bool <b>isReply</b>() const
 */
bool IrcNoticeMessage::isReply() const
{
    Q_D(const IrcMessage);
    QString msg = d->param(1);
    return msg.startsWith('\1') && msg.endsWith('\1');
}

bool IrcNoticeMessage::isValid() const
{
    return IrcMessage::isValid() && !target().isEmpty() && !content().isEmpty();
}

/*!
    \class IrcNumericMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a numeric message.
 */

/*!
    Constructs a new IrcNumericMessage with \a connection.
 */
IrcNumericMessage::IrcNumericMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Numeric;
}

/*!
    This property holds the numeric code.

    \par Access function:
    \li int <b>code</b>() const
 */
int IrcNumericMessage::code() const
{
    Q_D(const IrcMessage);
    bool ok = false;
    int number = d->command().toInt(&ok);
    return ok ? number : -1;
}

/*!
    \since 3.3
    \property bool IrcNumericMessage::composed

    This property holds whether the message is composed.

    \li \c RPL_MOTDSTART, \c RPL_MOTD, and \c RPL_ENDOFMOTD are composed as IrcMotdMessage
    \li \c RPL_NAMREPLY and \c RPL_ENDOFNAMES are composed as IrcNamesMessage
    \li \c RPL_TOPIC and \c RPL_NOTOPIC are composed as IrcTopicMessage
    \li \c RPL_INVITING and \c RPL_INVITED are composed as IrcInviteMessage
    \li \c RPL_WHOREPLY is composed as IrcWhoReplyMessage
    \li \c RPL_CHANNELMODEIS is composed as IrcModeMessage
    \li \c RPL_AWAY, \c RPL_UNAWAY, \c RPL_NOWAWAY are composed as as IrcAwayMessage

    \par Access function:
    \li bool <b>isComposed</b>() const
 */
bool IrcNumericMessage::isComposed() const
{
    return IrcMessageComposer::isComposed(code());
}

bool IrcNumericMessage::isValid() const
{
    return IrcMessage::isValid() && code() != -1;
}

/*!
    \class IrcPartMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a part message.
 */

/*!
    Constructs a new IrcPartMessage with \a connection.
 */
IrcPartMessage::IrcPartMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Part;
}

/*!
    This property holds the channel in question.

    \par Access function:
    \li QString <b>channel</b>() const
 */
QString IrcPartMessage::channel() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the optional part reason.

    \par Access function:
    \li QString <b>reason</b>() const
 */
QString IrcPartMessage::reason() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

bool IrcPartMessage::isValid() const
{
    return IrcMessage::isValid() && !channel().isEmpty();
}

/*!
    \class IrcPingMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a ping message.
 */

/*!
    Constructs a new IrcPingMessage with \a connection.
 */
IrcPingMessage::IrcPingMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Ping;
}

/*!
    This property holds the optional message argument.

    \par Access function:
    \li QString <b>argument</b>() const
 */
QString IrcPingMessage::argument() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

bool IrcPingMessage::isValid() const
{
    return IrcMessage::isValid();
}

/*!
    \class IrcPongMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a pong message.
 */

/*!
    Constructs a new IrcPongMessage with \a connection.
 */
IrcPongMessage::IrcPongMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Pong;
}

/*!
    This property holds the optional message argument.

    \par Access function:
    \li QString <b>argument</b>() const
 */
QString IrcPongMessage::argument() const
{
    Q_D(const IrcMessage);
    QStringList params = d->params();
    return params.value(params.count() - 1);
}

bool IrcPongMessage::isValid() const
{
    return IrcMessage::isValid();
}

/*!
    \class IrcPrivateMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a private message.
 */

/*!
    Constructs a new IrcPrivateMessage with \a connection.
 */
IrcPrivateMessage::IrcPrivateMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Private;
}

/*!
    This property holds the target channel or user in question.

    \par Access function:
    \li QString <b>target</b>() const
 */
QString IrcPrivateMessage::target() const
{
    Q_D(const IrcMessage);
    if (d->connection) {
        const IrcNetwork* network = d->connection->network();
        return IrcNetworkPrivate::removePrefix(d->param(0), network->statusPrefixes());
    }
    return d->param(0);
}

/*!
    This property holds the message content.

    \par Access function:
    \li QString <b>content</b>() const
 */
QString IrcPrivateMessage::content() const
{
    Q_D(const IrcMessage);
    QString msg = d->param(1);
    const bool act = isAction();
    const bool req = isRequest();
    if (act) msg.remove(0, 8);
    if (req) msg.remove(0, 1);
    if (act || req) msg.chop(1);
    return msg;
}

/*!
    \since 3.4

    This property holds the status prefix of the message.

    \par Access function:
    \li QString <b>statusPrefix</b>() const
 */
QString IrcPrivateMessage::statusPrefix() const
{
    Q_D(const IrcMessage);
    if (d->connection) {
        const IrcNetwork* network = d->connection->network();
        return IrcNetworkPrivate::getPrefix(d->param(0), network->statusPrefixes());
    }
    return QString();
}

/*!
    \property bool IrcPrivateMessage::private
    This property is \c true if the message is private,
    or \c false if it is a channel message.

    \par Access function:
    \li bool <b>isPrivate</b>() const
 */
bool IrcPrivateMessage::isPrivate() const
{
    Q_D(const IrcMessage);
    if (d->connection)
        return !target().compare(d->connection->nickName(), Qt::CaseInsensitive);
    return false;
}

/*!
    \property bool IrcPrivateMessage::action
    This property is \c true if the message is an action; otherwise \c false.

    \par Access function:
    \li bool <b>isAction</b>() const
 */
bool IrcPrivateMessage::isAction() const
{
    Q_D(const IrcMessage);
    QString msg = d->param(1);
    return msg.startsWith("\1ACTION ") && msg.endsWith('\1');
}

/*!
    \property bool IrcPrivateMessage::request
    This property is \c true if the message is a request; otherwise \c false.

    \par Access function:
    \li bool <b>isRequest</b>() const
 */
bool IrcPrivateMessage::isRequest() const
{
    Q_D(const IrcMessage);
    QString msg = d->param(1);
    return msg.startsWith('\1') && msg.endsWith('\1') && !isAction();
}

bool IrcPrivateMessage::isValid() const
{
    return IrcMessage::isValid() && !target().isEmpty() && !content().isEmpty();
}

/*!
    \class IrcQuitMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a quit message.
 */

/*!
    Constructs a new IrcQuitMessage with \a connection.
 */
IrcQuitMessage::IrcQuitMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Quit;
}

/*!
    This property holds the optional quit reason.

    \par Access function:
    \li QString <b>reason</b>() const
 */
QString IrcQuitMessage::reason() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

bool IrcQuitMessage::isValid() const
{
    return IrcMessage::isValid();
}

/*!
    \class IrcTopicMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a topic message.
 */

/*!
    Constructs a new IrcTopicMessage with \a connection.
 */
IrcTopicMessage::IrcTopicMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Topic;
}

/*!
    This property holds the channel in question.

    \par Access function:
    \li QString <b>channel</b>() const
 */
QString IrcTopicMessage::channel() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the new channel topic.

    \par Access function:
    \li QString <b>topic</b>() const
 */
QString IrcTopicMessage::topic() const
{
    Q_D(const IrcMessage);
    if (d->command().toInt() == Irc::RPL_NOTOPIC)
        return QString();
    return d->param(1);
}

/*!
    \property bool IrcTopicMessage::reply
    This property holds whether the message is a reply.

    Topic messages are sent in three situations:
    \li as a notification of a topic change (\c false),
    \li as a reply when joining a channel (\c true), or
    \li as a reply when explicitly querying the channel topic (\c true).

    \par Access function:
    \li bool <b>isReply</b>() const

    \sa Irc::RPL_TOPIC, Irc::RPL_NOTOPIC, IrcTopicCommand
 */
bool IrcTopicMessage::isReply() const
{
    Q_D(const IrcMessage);
    int rpl = d->command().toInt();
    return rpl == Irc::RPL_TOPIC || rpl == Irc::RPL_NOTOPIC;
}

bool IrcTopicMessage::isValid() const
{
    return IrcMessage::isValid() && !channel().isEmpty();
}

/*!
    \since 3.3
    \class IrcWhoisMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a reply message to a WHOIS command.
 */

/*!
    Constructs a new IrcWhoisMessage with \a connection.
 */
IrcWhoisMessage::IrcWhoisMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Whois;
    setCommand(QStringLiteral("WHOIS"));
}

/*!
    This property holds the real name of the user.

    \par Access function:
    \li QString <b>realName</b>() const
 */
QString IrcWhoisMessage::realName() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the server address user is on.

    \par Access function:
    \li QString <b>server</b>() const
 */
QString IrcWhoisMessage::server() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    This property holds info of the server the user is on.

    \par Access function:
    \li QString <b>info</b>() const
 */
QString IrcWhoisMessage::info() const
{
    Q_D(const IrcMessage);
    return d->param(2);
}

/*!
    This property holds the account name of the user.

    \par Access function:
    \li QString <b>account</b>() const
 */
QString IrcWhoisMessage::account() const
{
    Q_D(const IrcMessage);
    return d->param(3);
}

/*!
    This property holds the address the user is connecting from.

    \par Access function:
    \li QString <b>address</b>() const
 */
QString IrcWhoisMessage::address() const
{
    Q_D(const IrcMessage);
    return d->param(4);
}

/*!
    This property holds the time since user has been online.

    \par Access function:
    \li QDateTime <b>since</b>() const
 */
QDateTime IrcWhoisMessage::since() const
{
    Q_D(const IrcMessage);
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    return QDateTime::fromTime_t(d->param(5).toInt());
#else
    return QDateTime::fromSecsSinceEpoch(d->param(5).toInt());
#endif
}

/*!
    This property holds the number of seconds the user has been idle.

    \par Access function:
    \li int <b>idle</b>() const
 */
int IrcWhoisMessage::idle() const
{
    Q_D(const IrcMessage);
    return d->param(6).toInt();
}

/*!
    \property bool IrcWhoisMessage::secure
    This property holds whether the user is using a secure connection.

    \par Access function:
    \li bool <b>isSecure</b>() const
 */
bool IrcWhoisMessage::isSecure() const
{
    Q_D(const IrcMessage);
    return !d->param(7).isEmpty();
}

/*!
    This property holds the visible list of channels of the user.

    \par Access function:
    \li QStringList <b>channels</b>() const
 */
QStringList IrcWhoisMessage::channels() const
{
    Q_D(const IrcMessage);
    return d->params().value(8).split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

/*!
    \since 3.5

    This property holds the away reason of the user.

    \par Access function:
    \li QString <b>awayReason</b>() const
 */
QString IrcWhoisMessage::awayReason() const
{
    Q_D(const IrcMessage);
    return d->param(9);
}

bool IrcWhoisMessage::isValid() const
{
    Q_D(const IrcMessage);
    return IrcMessage::isValid() && d->params().count() == 10;
}

/*!
    \since 3.3
    \class IrcWhowasMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a reply message to a WHOWAS command.
 */

/*!
    Constructs a new IrcWhowasMessage with \a connection.
 */
IrcWhowasMessage::IrcWhowasMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = Whowas;
    setCommand(QStringLiteral("WHOWAS"));
}

/*!
    This property holds the real name of the user.

    \par Access function:
    \li QString <b>realName</b>() const
 */
QString IrcWhowasMessage::realName() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the server address user was on.

    \par Access function:
    \li QString <b>server</b>() const
 */
QString IrcWhowasMessage::server() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    This property holds info of the server the user was on.

    \par Access function:
    \li QString <b>info</b>() const
 */
QString IrcWhowasMessage::info() const
{
    Q_D(const IrcMessage);
    return d->param(2);
}

/*!
    This property holds the account of the user.

    \par Access function:
    \li QString <b>account</b>() const
 */
QString IrcWhowasMessage::account() const
{
    Q_D(const IrcMessage);
    return d->param(3);
}

bool IrcWhowasMessage::isValid() const
{
    Q_D(const IrcMessage);
    return IrcMessage::isValid() && d->params().count() == 9;
}

/*!
    \since 3.1
    \class IrcWhoReplyMessage ircmessage.h <IrcMessage>
    \ingroup message
    \brief Represents a reply message to a WHO command.
 */

/*!
    Constructs a new IrcWhoReplyMessage with \a connection.
 */
IrcWhoReplyMessage::IrcWhoReplyMessage(IrcConnection* connection) : IrcMessage(connection)
{
    Q_D(IrcMessage);
    d->type = WhoReply;
}

/*!
    This property holds the mask.

    \par Access function:
    \li QString <b>mask</b>() const
 */
QString IrcWhoReplyMessage::mask() const
{
    Q_D(const IrcMessage);
    return d->param(0);
}

/*!
    This property holds the server of the user.

    \par Access function:
    \li QString <b>server</b>() const
 */
QString IrcWhoReplyMessage::server() const
{
    Q_D(const IrcMessage);
    return d->param(1);
}

/*!
    \property bool IrcWhoReplyMessage::away
    This property holds whether the user is away.

    \par Access function:
    \li QString <b>isAway</b>() const
 */
bool IrcWhoReplyMessage::isAway() const
{
    Q_D(const IrcMessage);
    return d->param(2).contains(QLatin1String("G"));
}

/*!
    \property bool IrcWhoReplyMessage::servOp
    This property holds whether the user is a server operator.

    \par Access function:
    \li QString <b>isServOp</b>() const
 */
bool IrcWhoReplyMessage::isServOp() const
{
    Q_D(const IrcMessage);
    return d->param(2).contains(QLatin1String("*"));
}

/*!
    This property holds the real name of the user.

    \par Access function:
    \li QString <b>realName</b>() const
 */
QString IrcWhoReplyMessage::realName() const
{
    Q_D(const IrcMessage);
    return d->param(3);
}

bool IrcWhoReplyMessage::isValid() const
{
    return IrcMessage::isValid() && !mask().isEmpty() && !nick().isEmpty();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, IrcMessage::Type type)
{
    const int index = IrcMessage::staticMetaObject.indexOfEnumerator("Type");
    QMetaEnum enumerator = IrcMessage::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(type);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, IrcMessage::Flag flag)
{
    const int index = IrcMessage::staticMetaObject.indexOfEnumerator("Flag");
    QMetaEnum enumerator = IrcMessage::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(flag);
    debug << (key ? key : "None");
    return debug;
}

QDebug operator<<(QDebug debug, IrcMessage::Flags flags)
{
    QStringList lst;
    if (flags == IrcMessage::None)
        lst << QStringLiteral("None");
    if (flags & IrcMessage::Own)
        lst << QStringLiteral("Own");
    if (flags & IrcMessage::Playback)
        lst << QStringLiteral("Playback");
    if (flags & IrcMessage::Implicit)
        lst << QStringLiteral("Implicit");
    debug.nospace() << '(' << qPrintable(lst.join("|")) << ')';
    return debug;
}

QDebug operator<<(QDebug debug, IrcModeMessage::Kind kind)
{
    const int index = IrcModeMessage::staticMetaObject.indexOfEnumerator("Kind");
    QMetaEnum enumerator = IrcModeMessage::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(kind);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, const IrcMessage* message)
{
    if (!message)
        return debug << "IrcMessage(0x0) ";
    debug.nospace() << message->metaObject()->className() << '(' << (void*) message;
    if (!message->objectName().isEmpty())
        debug.nospace() << ", name=" << qPrintable(message->objectName());
    debug.nospace() << ", flags=" << message->flags();
    if (!message->prefix().isEmpty())
        debug.nospace() << ", prefix=" << qPrintable(message->prefix());
    if (!message->command().isEmpty())
        debug.nospace() << ", command=" << qPrintable(message->command());
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_ircmessage.cpp"

IRC_END_NAMESPACE
