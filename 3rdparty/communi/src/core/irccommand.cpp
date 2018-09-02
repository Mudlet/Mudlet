/*
  Copyright (C) 2008-2016 The Communi Project

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

#include "irccommand.h"
#include "irccommand_p.h"
#include "ircconnection.h"
#include "ircmessage.h"
#include <QTextCodec>
#include <QMetaEnum>
#include <QDebug>

IRC_BEGIN_NAMESPACE

/*!
    \file irccommand.h
    \brief \#include &lt;IrcCommand&gt;
 */

/*!
    \class IrcCommand irccommand.h <IrcCommand>
    \ingroup core
    \brief Provides the most common commands.

    The IrcCommand class supports the most common IRC commands out of the box,
    and can be extended for custom commands as well. See IrcCommand::Type for
    the list of built-in command types. IRC commands, as in IrcCommand instances,
    are sent to the IRC server via IrcConnection::sendCommand().

    \section creating-commands Creating commands

    It is recommended to create IrcCommand instances via static
    IrcCommand::createXxx() methods.

    \warning IrcCommand instances must be allocated on the heap, since
    IrcConnection::sendCommand() takes ownership of the command and deletes
    it once it has been sent.

    \section custom-commands Custom commands

    A "custom command" here refers to command types not listed in IrcCommand::Type,
    the list of built-in command types. There are two ways to send custom commands:
    \li by passing the string representation of a command directly to
    IrcConnection::sendRaw() or IrcConnection::sendData(), or
    \li by subclassing IrcCommand and reimplementing
    IrcCommand::toString(), which eventually creates the string representation
    of the command.

    Example implementation of a custom command:
    \code
    class IrcServerCommand : public IrcCommand
    {
        Q_OBJECT
    public:
        explicit IrcServerCommand(QObject* parent = 0) : IrcCommand(parent)
        {
        }

        // provided for convenience, to ensure correct parameter order
        static IrcCommand* create(const QString& serverName, int hopCount, const QString& info)
        {
            IrcCommand* command = new IrcServerCommand;
            command->setParameters(QStringList() << serverName << QString::number(hopCount) << info);
            return command;
        }

        // reimplemented from IrcCommand::toString()
        virtual toString() const
        {
            // SERVER <servername> <hopcount> <info>
            return QString("SERVER %1 %2 %3").arg(params.value(0), params.value(1), params.value(2));
        }
    };
    \endcode

    \sa IrcConnection::sendCommand(), IrcConnection::sendRaw(), IrcCommand::Type
 */

/*!
    \enum IrcCommand::Type
    This enum describes the built-in command types.
 */

/*!
    \var IrcCommand::Admin
    \brief An admin command (ADMIN) is used to query server admin info.
 */

/*!
    \var IrcCommand::Away
    \brief An away command (AWAY) is used to set the away status.
 */

/*!
    \var IrcCommand::Capability
    \brief A capability command (CAP) is used to manage connection capabilities.
 */

/*!
    \var IrcCommand::CtcpAction
    \brief A CTCP action command is used to send an action message to channels and users.
 */

/*!
    \var IrcCommand::CtcpReply
    \brief A CTCP reply command is used to send a reply to a request.
 */

/*!
    \var IrcCommand::CtcpRequest
    \brief A CTCP request command is used to send a request.
 */

/*!
    \var IrcCommand::Custom
    \brief A custom command
 */

/*!
    \var IrcCommand::Info
    \brief An info command (INFO) is used to query server info.
 */

/*!
    \var IrcCommand::Invite
    \brief An invite command (INVITE) is used to invite users to a channel.
 */

/*!
    \var IrcCommand::Join
    \brief A join command (JOIN) is used to start listening a specific channel.
 */

/*!
    \var IrcCommand::Kick
    \brief A kick command (KICK) is used to forcibly remove a user from a channel.
 */

/*!
    \var IrcCommand::Knock
    \brief A knock command (KNOCK) is used to request channel invitation.
 */

/*!
    \var IrcCommand::List
    \brief A list command (LIST) is used to list channels and their topics.
 */

/*!
    \var IrcCommand::Message
    \brief A message command (PRIVMSG) is used to send private messages to channels and users.
 */

/*!
    \var IrcCommand::Mode
    \brief A mode command (MODE) is used to change the mode of users and channels.
 */

/*!
    \var IrcCommand::Motd
    \brief A message of the day command (MOTD) is used to query the message of the day.
 */

/*!
    \var IrcCommand::Names
    \brief A names command (NAMES) is used to list all nicknames on a channel.
 */

/*!
    \var IrcCommand::Nick
    \brief A nick command (NICK) is used to give user a nickname or change the previous one.
 */

/*!
    \var IrcCommand::Notice
    \brief A notice command (NOTICE) is used to send notice messages to channels and users.
 */

/*!
    \var IrcCommand::Part
    \brief A part command (PART) causes the client to be removed from the channel.
 */

/*!
    \var IrcCommand::Quit
    \brief A quit command (QUIT) is used to end a client connection.
 */

/*!
    \var IrcCommand::Quote
    \brief A quote command is used to send a raw message to the server.
 */

/*!
    \var IrcCommand::Stats
    \brief A stats command (STATS) is used to query server statistics.
 */

/*!
    \var IrcCommand::Time
    \brief A time command (TIME) is used to query local server time.
 */

/*!
    \var IrcCommand::Topic
    \brief A topic command (TOPIC) is used to change or view the topic of a channel.
 */

/*!
    \var IrcCommand::Trace
    \brief A trace command (TRACE) is used to trace the connection path to a target.
 */

/*!
    \var IrcCommand::Users
    \brief A users command (USERS) is used to query server users.
 */

/*!
    \var IrcCommand::Version
    \brief A version command (VERSION) is used to query user or server version.
 */

/*!
    \var IrcCommand::Who
    \brief A who command (WHO) is used to generate a query which returns a list of matching users.
 */

/*!
    \var IrcCommand::Whois
    \brief A whois command (WHOIS) is used to query information about a particular user.
 */

/*!
    \var IrcCommand::Whowas
    \brief A whowas command (WHOWAS) is used to query information about a user that no longer exists.
 */

#ifndef IRC_DOXYGEN
IrcCommandPrivate::IrcCommandPrivate() : type(IrcCommand::Custom), encoding("UTF-8")
{
}

QString IrcCommandPrivate::params(int index) const
{
    return QStringList(parameters.mid(index)).join(QLatin1String(" "));
}

IrcCommand* IrcCommandPrivate::createCommand(IrcCommand::Type type, const QStringList& parameters)
{
    IrcCommand* command = new IrcCommand;
    command->setType(type);
    command->setParameters(parameters);
    return command;
}
#endif // IRC_DOXYGEN

/*!
    Constructs a new IrcCommand with \a parent.
 */
IrcCommand::IrcCommand(QObject* parent) : QObject(parent), d_ptr(new IrcCommandPrivate)
{
}

/*!
    Destructs the IRC command.
 */
IrcCommand::~IrcCommand()
{
}

/*!
    \since 3.3

    This property holds the connection that this command was sent to.

    The connection is only set if the command has been passed to IrcConnection::sendCommand().
    It is mostly usable to know the associated connection in IrcCommandFilter::commandFilter().

    \par Access function:
    \li \ref IrcConnection* <b>connection</b>() const
 */
IrcConnection* IrcCommand::connection() const
{
    Q_D(const IrcCommand);
    return d->connection;
}

/*!
    \since 3.5

    This property holds the network that this command was sent to.

    The network is only set if the command has been passed to IrcConnection::sendCommand().

    \par Access function:
    \li \ref IrcNetwork* <b>network</b>() const
 */
IrcNetwork* IrcCommand::network() const
{
    Q_D(const IrcCommand);
    return d->connection ? d->connection->network() : 0;
}

/*!
    This property holds the command type.

    \par Access functions:
    \li IrcCommand::Type <b>type</b>() const
    \li void <b>setType</b>(IrcCommand::Type type)
 */
IrcCommand::Type IrcCommand::type() const
{
    Q_D(const IrcCommand);
    return d->type;
}

void IrcCommand::setType(Type type)
{
    Q_D(IrcCommand);
    d->type = type;
}

/*!
    This property holds the command parameters.

    \par Access functions:
    \li QStringList <b>parameters</b>() const
    \li void <b>setParameters</b>(const QStringList& parameters)
 */
QStringList IrcCommand::parameters() const
{
    Q_D(const IrcCommand);
    return d->parameters;
}

void IrcCommand::setParameters(const QStringList& parameters)
{
    Q_D(IrcCommand);
    d->parameters = parameters;
}

/*!
    This property holds the encoding that is used when
    sending the command via IrcConnection::sendCommand().

    See QTextCodec::availableCodes() for the list of
    supported encodings. The default value is \c "UTF-8".

    \par Access functions:
    \li QByteArray <b>encoding</b>() const
    \li void <b>setEncoding</b>(const QByteArray& encoding)

    \sa QTextCodec::availableCodecs()
 */
QByteArray IrcCommand::encoding() const
{
    Q_D(const IrcCommand);
    return d->encoding;
}

void IrcCommand::setEncoding(const QByteArray& encoding)
{
    Q_D(IrcCommand);
    extern bool irc_is_supported_encoding(const QByteArray& encoding); // ircmessagedecoder.cpp
    if (!irc_is_supported_encoding(encoding)) {
        qWarning() << "IrcCommand::setEncoding(): unsupported encoding" << encoding;
        return;
    }
    d->encoding = encoding;
}

/*!
    Returns the command as a string.

    Reimplement for custom commands.
    \sa IrcCommand::Custom
 */
QString IrcCommand::toString() const
{
    Q_D(const IrcCommand);
    const QString p0 = d->parameters.value(0);
    const QString p1 = d->parameters.value(1);
    const QString p2 = d->parameters.value(2);

    switch (d->type) {
        case Admin:         return QString("ADMIN %1").arg(p0); // server
        case Away:          return QString("AWAY :%1").arg(d->params(0)); // reason
        case Capability:    return QString("CAP %1 :%2").arg(p0, d->params(1)); // subcmd, caps
        case CtcpAction:    return QString("PRIVMSG %1 :\1ACTION %2\1").arg(p0, d->params(1)); // target, msg
        case CtcpRequest:   return QString("PRIVMSG %1 :\1%2\1").arg(p0, d->params(1)); // target, msg
        case CtcpReply:     return QString("NOTICE %1 :\1%2\1").arg(p0, d->params(1)); // target, msg
        case Info:          return QString("INFO %1").arg(p0); // server
        case Invite:        return QString("INVITE %1 %2").arg(p0, p1); // user, chan
        case Join:          return p1.isNull() ? QString("JOIN %1").arg(p0) : QString("JOIN %1 %2").arg(p0, p1); // chan, key
        case Kick:          return p2.isNull() ? QString("KICK %1 %2").arg(p0, p1) : QString("KICK %1 %2 :%3").arg(p0, p1, d->params(2)); // chan, user, reason
        case Knock:         return QString("KNOCK %1 %2").arg(p0, p1); // chan, msg
        case List:          return p1.isNull() ? QString("LIST %1").arg(p0) : QString("LIST %1 %2").arg(p0, p1); // chan, server
        case Message:       return QString("PRIVMSG %1 :%2").arg(p0, d->params(1)); // target, msg
        case Mode:          return QString("MODE ") + d->parameters.join(" "); // target, mode, arg
        case Monitor:       return QString("MONITOR %1 %2").arg(p0, p1); // cmd, target
        case Motd:          return QString("MOTD %1").arg(p0); // server
        case Names:         return QString("NAMES %1").arg(p0); // chan
        case Nick:          return QString("NICK %1").arg(p0); // nick
        case Notice:        return QString("NOTICE %1 :%2").arg(p0, d->params(1)); // target, msg
        case Part:          return p1.isNull() ? QString("PART %1").arg(p0) : QString("PART %1 :%2").arg(p0, d->params(1)); // chan, reason
        case Ping:          return QString("PING %1").arg(p0); // argument
        case Pong:          return QString("PONG %1").arg(p0); // argument
        case Quit:          return QString("QUIT :%1").arg(d->params(0)); // reason
        case Quote:         return d->parameters.join(" ");
        case Stats:         return QString("STATS %1 %2").arg(p0, p1); // query, server
        case Time:          return QString("TIME %1").arg(p0); // server
        case Topic:         return p1.isNull() ? QString("TOPIC %1").arg(p0) : QString("TOPIC %1 :%2").arg(p0, d->params(1)); // chan, topic
        case Trace:         return QString("TRACE %1").arg(p0); // target
        case Users:         return QString("USERS %1").arg(p0); // server
        case Version:       return p0.isNull() ? QString("VERSION") : QString("PRIVMSG %1 :\1VERSION\1").arg(p0); // user
        case Who:           return QString("WHO %1").arg(p0); // user
        case Whois:         return QString("WHOIS %1 %1").arg(p0); // user
        case Whowas:        return QString("WHOWAS %1 %1").arg(p0); // user

        case Custom:        qWarning("Reimplement IrcCommand::toString() for IrcCommand::Custom");
        default:            return QString();
    }
}

/*!
    Creates a new message from this command for \a prefix and \a connection.

    Notice that IRC servers do not echo sent message commands back to the client.
    This function is particularly useful for converting sent message commands as
    messages for presentation purposes.

    \code
    if (command->type() == IrcCommand::Message) {
        IrcMessage* message = command->toMessage(connection->nickName(), connection);
        receiveMessage(message);
        message->deleteLater();
    }
    \endcode
 */
IrcMessage* IrcCommand::toMessage(const QString& prefix, IrcConnection* connection) const
{
    return IrcMessage::fromData(":" + prefix.toUtf8() + " " + toString().toUtf8(), connection);
}

/*!
    Creates a new ADMIN command with type IrcCommand::Admin and optional parameter \a server.

    This command shows admin info for the specified \a server,
    or the current server if not specified.
 */
IrcCommand* IrcCommand::createAdmin(const QString& server)
{
    return IrcCommandPrivate::createCommand(Admin, QStringList() << server);
}

/*!
    Creates a new AWAY command with type IrcCommand::Away and optional parameter \a reason.

    Provides the server with \a reason to automatically send in reply to a private
    message directed at the user. If \a reason is omitted, the away status is removed.
 */
IrcCommand* IrcCommand::createAway(const QString& reason)
{
    return IrcCommandPrivate::createCommand(Away, QStringList() << reason);
}

/*!
    Creates a new capability command with type IrcCommand::Capability and parameters \a subCommand and a \a capability.

    Available subcommands are: LS, LIST, REQ, ACK, NAK, CLEAR and END.

    \sa \ref ircv3
 */
IrcCommand* IrcCommand::createCapability(const QString& subCommand, const QString& capability)
{
    return createCapability(subCommand, QStringList() << capability);
}

/*!
    Creates a new capability command with type IrcCommand::Capability and parameters \a subCommand and optional \a capabilities.

    Available subcommands are: LS, LIST, REQ, ACK, NAK, CLEAR and END.

    \sa \ref ircv3
 */
IrcCommand* IrcCommand::createCapability(const QString& subCommand, const QStringList& capabilities)
{
    return IrcCommandPrivate::createCommand(Capability, QStringList() << subCommand << capabilities.join(QLatin1String(" ")));
}

/*!
    Creates a new CTCP action command with type IrcCommand::CtcpAction and parameters \a target and \a action.
 */
IrcCommand* IrcCommand::createCtcpAction(const QString& target, const QString& action)
{
    return IrcCommandPrivate::createCommand(CtcpAction, QStringList() << target << action);
}

/*!
    Creates a new CTCP reply command with type IrcCommand::CtcpReply and parameters \a target and \a reply.
 */
IrcCommand* IrcCommand::createCtcpReply(const QString& target, const QString& reply)
{
    return IrcCommandPrivate::createCommand(CtcpReply, QStringList() << target << reply);
}

/*!
    Creates a new CTCP request command with type IrcCommand::CtcpRequest and parameters \a target and \a request.
 */
IrcCommand* IrcCommand::createCtcpRequest(const QString& target, const QString& request)
{
    return IrcCommandPrivate::createCommand(CtcpRequest, QStringList() << target << request);
}

/*!
    Creates a new INFO command with type IrcCommand::Info and optional parameter \a server.

    This command shows info for the specified \a server,
    or the current server if not specified.
 */
IrcCommand* IrcCommand::createInfo(const QString& server)
{
    return IrcCommandPrivate::createCommand(Info, QStringList() << server);
}

/*!
    Creates a new INVITE command with type IrcCommand::Invite and parameters \a user and \a channel.

    This command invites \a user to the \a channel. The channel does not have to exist, but
    if it does, only members of the channel are allowed to invite other clients. if the
    channel mode +i (invite-only) is set, only channel operators may invite other clients.
 */
IrcCommand* IrcCommand::createInvite(const QString& user, const QString& channel)
{
    return IrcCommandPrivate::createCommand(Invite, QStringList() << user << channel);
}

/*!
    Creates a new JOIN command with type IrcCommand::Join and parameters \a channel and optional \a key.

    This command joins the \a channel using \a key if specified.
    If the channel does not exist, it will be created.
 */
IrcCommand* IrcCommand::createJoin(const QString& channel, const QString& key)
{
    return IrcCommandPrivate::createCommand(Join, QStringList() << channel << key);
}

/*!
    This overload is provided for convenience.
 */
IrcCommand* IrcCommand::createJoin(const QStringList& channels, const QStringList& keys)
{
    if (keys.join("").isEmpty())
        return IrcCommandPrivate::createCommand(Join, QStringList() << channels.join(","));
    return IrcCommandPrivate::createCommand(Join, QStringList() << channels.join(",") << keys.join(","));
}

/*!
    Creates a new KICK command with type IrcCommand::Kick and parameters \a channel, \a user and optional \a reason.

    This command forcibly removes \a user from \a channel,
    and may only be issued by channel operators.
 */
IrcCommand* IrcCommand::createKick(const QString& channel, const QString& user, const QString& reason)
{
    return IrcCommandPrivate::createCommand(Kick, QStringList() << channel << user << reason);
}

/*!
    Creates a new KNOCK command with type IrcCommand::Knock and parameters \a channel and optional \a message.

    This command sends an invitation request to a \a channel with an optional \a message.

    \note The command is not formally defined by an RFC, but is supported by most major IRC daemons.
    Support is indicated in a RPL_ISUPPORT reply (numeric 005) with the KNOCK keyword.
 */
IrcCommand* IrcCommand::createKnock(const QString& channel, const QString& message)
{
    return IrcCommandPrivate::createCommand(Knock, QStringList() << channel << message);
}

/*!
    Creates a new LIST command with type IrcCommand::List and optional parameters \a channels and \a server.

    This command lists all channels on the server. If \a channels are given, it will list the channel topics.
    If \a server is given, the command will be forwarded to \a server for evaluation.
 */
IrcCommand* IrcCommand::createList(const QStringList& channels, const QString& server)
{
    return IrcCommandPrivate::createCommand(List, QStringList() << channels.join(",") << server);
}

/*!
    Creates a new PRIVMSG command with type IrcCommand::Message and parameters \a target and \a message.

    This command sends \a message to \a target, which is usually a user or channel.
 */
IrcCommand* IrcCommand::createMessage(const QString& target, const QString& message)
{
    return IrcCommandPrivate::createCommand(Message, QStringList() << target << message);
}

/*!
    Creates a new MODE command with type IrcCommand::Mode and parameters \a target and optional \a mode and \a arg.

    This command is used to set both user and channel modes.
 */
IrcCommand* IrcCommand::createMode(const QString& target, const QString& mode, const QString& arg)
{
    return IrcCommandPrivate::createCommand(Mode, QStringList() << target << mode << arg);
}

/*!
    \since 3.4

    Creates a new MONITOR command with type IrcCommand::Monitor and parameters \a command and and optional \a target.

    Available commands are:
    \li \c + - Adds the given list of targets to the list of targets being monitored.
    \li \c + - Removes the given list of targets from the list of targets being monitored.
               No output will be returned for use of this command.
    \li \c C - Clears the list of targets being monitored. No output will be returned for use of this command.
    \li \c L - Outputs the current list of targets being monitored. All output will use RPL_MONLIST,
               and the output will be terminated with RPL_ENDOFMONLIST.
    \li \c S - Outputs for each target in the list being monitored, whether the client is online or offline.
               All targets that are online will be sent using RPL_MONONLINE, all targets that are offline will
               be sent using RPL_MONOFFLINE.

    \sa \ref ircv3
 */
IrcCommand* IrcCommand::createMonitor(const QString& command, const QString& target)
{
    return IrcCommandPrivate::createCommand(Monitor, QStringList() << command << target);
}

/*!
    \since 3.4

    Creates a new MONITOR command with type IrcCommand::Monitor and parameters \a command and \a targets.

    Available commands are:
    \li \c + - Adds the given list of targets to the list of targets being monitored.
    \li \c + - Removes the given list of targets from the list of targets being monitored.
               No output will be returned for use of this command.
    \li \c C - Clears the list of targets being monitored. No output will be returned for use of this command.
    \li \c L - Outputs the current list of targets being monitored. All output will use RPL_MONLIST,
               and the output will be terminated with RPL_ENDOFMONLIST.
    \li \c S - Outputs for each target in the list being monitored, whether the client is online or offline.
               All targets that are online will be sent using RPL_MONONLINE, all targets that are offline will
               be sent using RPL_MONOFFLINE.

    \sa \ref ircv3
 */
IrcCommand* IrcCommand::createMonitor(const QString& command, const QStringList& targets)
{
    return IrcCommandPrivate::createCommand(Monitor, QStringList() << command << targets.join(","));
}

/*!
    Creates a new MOTD command with type IrcCommand::Motd and optional parameter \a server.

    This command shows the message of the day on the specified \a server,
    or the current server if not specified.
 */
IrcCommand* IrcCommand::createMotd(const QString& server)
{
    return IrcCommandPrivate::createCommand(Motd, QStringList() << server);
}

/*!
    Creates a new NAMES command with type IrcCommand::Names and parameter \a channel.

    This command lists all users on the \a channel, optionally limiting to the given \a server.

    If \a channel is omitted, all users are shown, grouped by channel name with
    all users who are not on a channel being shown as part of channel "*".
    If \a server is specified, the command is sent to \a server for evaluation.
*/
IrcCommand* IrcCommand::createNames(const QString& channel, const QString& server)
{
    return IrcCommandPrivate::createCommand(Names, QStringList() << channel << server);
}

/*!
    This overload is provided for convenience.
 */
IrcCommand* IrcCommand::createNames(const QStringList& channels, const QString& server)
{
    return IrcCommandPrivate::createCommand(Names, QStringList() << channels.join(",") << server);
}

/*!
    Creates a new NICK command with type IrcCommand::Nick and parameter \a nick.

    This command allows a client to change their IRC nickname.
 */
IrcCommand* IrcCommand::createNick(const QString& nick)
{
    return IrcCommandPrivate::createCommand(Nick, QStringList() << nick);
}

/*!
    Creates a new NOTICE command with type IrcCommand::Notice and parameters \a target and \a message.

    This command sends \a notice to \a target, which is usually a user or channel.

    \note The command works similarly to PRIVMSG, except automatic replies must never be sent in reply to NOTICE messages.
 */
IrcCommand* IrcCommand::createNotice(const QString& target, const QString& message)
{
    return IrcCommandPrivate::createCommand(Notice, QStringList() << target << message);
}

/*!
    Creates a new PART command with type IrcCommand::Part and parameters \a channel and optional \a reason.

    This command causes the client to leave the specified channel.
 */
IrcCommand* IrcCommand::createPart(const QString& channel, const QString& reason)
{
    return IrcCommandPrivate::createCommand(Part, QStringList() << channel << reason);
}

/*!
    This overload is provided for convenience.
 */
IrcCommand* IrcCommand::createPart(const QStringList& channels, const QString& reason)
{
    return IrcCommandPrivate::createCommand(Part, QStringList() << channels.join(",") << reason);
}

/*!
    Creates a new PING command with type IrcCommand::Ping and \a argument.
 */
IrcCommand* IrcCommand::createPing(const QString& argument)
{
    return IrcCommandPrivate::createCommand(Ping, QStringList() << argument);
}

/*!
    Creates a new PONG command with type IrcCommand::Pong and \a argument.
 */
IrcCommand* IrcCommand::createPong(const QString& argument)
{
    return IrcCommandPrivate::createCommand(Pong, QStringList() << argument);
}

/*!
    Creates a new QUIT command with type IrcCommand::Quit and optional parameter \a reason.
 */
IrcCommand* IrcCommand::createQuit(const QString& reason)
{
    return IrcCommandPrivate::createCommand(Quit, QStringList() << reason);
}

/*!
    Creates a new QUOTE command with type IrcCommand::Quote and \a raw.
 */
IrcCommand* IrcCommand::createQuote(const QString& raw)
{
    return IrcCommandPrivate::createCommand(Quote, QStringList() << raw);
}

/*!
    Creates a new QUOTE command with type IrcCommand::Quote and \a parameters.
 */
IrcCommand* IrcCommand::createQuote(const QStringList& parameters)
{
    return IrcCommandPrivate::createCommand(Quote, parameters);
}

/*!
    Creates a new STATS command with type IrcCommand::Stats and parameters \a query and optional \a server.

    This command queries statistics about the specified \a server,
    or the current server if not specified.
 */
IrcCommand* IrcCommand::createStats(const QString& query, const QString& server)
{
    return IrcCommandPrivate::createCommand(Stats, QStringList() << query << server);
}

/*!
    Creates a new TIME command with type IrcCommand::Time and optional parameter \a server.

    This command queries local time of the specified \a server,
    or the current server if not specified.
 */
IrcCommand* IrcCommand::createTime(const QString& server)
{
    return IrcCommandPrivate::createCommand(Time, QStringList() << server);
}

/*!
    Creates a new TOPIC command with type IrcCommand::Topic and parameters \a channel and optional \a topic.

    This command allows the client to query or set the channel topic on \a channel.
    If \a topic is given, it sets the channel topic to \a topic.
    If channel mode +t is set, only a channel operator may set the topic.
 */
IrcCommand* IrcCommand::createTopic(const QString& channel, const QString& topic)
{
    return IrcCommandPrivate::createCommand(Topic, QStringList() << channel << topic);
}

/*!
    Creates a new TRACE command with type IrcCommand::Trace and optional parameter \a target.

    This command traces the connection path across the IRC network
    to the current server or to a specific \a target (server or client)
    in a similar method to traceroute.
 */
IrcCommand* IrcCommand::createTrace(const QString& target)
{
    return IrcCommandPrivate::createCommand(Trace, QStringList() << target);
}

/*!
    Creates a new USERS command with type IrcCommand::Users and optional parameter \a server.

    This command queries the users of the specified \a server,
    or the current server if not specified.
 */
IrcCommand* IrcCommand::createUsers(const QString& server)
{
    return IrcCommandPrivate::createCommand(Users, QStringList() << server);
}

/*!
    Creates a new command with type IrcCommand::Version and optional parameter \a user.

    This command queries the version of the specified \a user's client (CTCP REQUEST VERSION),
    or the current server (VERSION) if not specified.
 */
IrcCommand* IrcCommand::createVersion(const QString& user)
{
    return IrcCommandPrivate::createCommand(Version, QStringList() << user);
}

/*!
    Creates a new WHO command with type IrcCommand::Who and parameters \a mask and optional \a operators.

    This command returns a list of users who match \a mask,
    optionally matching only IRC \a operators.
 */
IrcCommand* IrcCommand::createWho(const QString& mask, bool operators)
{
    return IrcCommandPrivate::createCommand(Who, QStringList() << mask << (operators ? "o" : ""));
}

/*!
    Creates a new WHOIS command with type IrcCommand::Whois and parameter \a user.

    This command returns information about \a user.
 */
IrcCommand* IrcCommand::createWhois(const QString& user)
{
    return IrcCommandPrivate::createCommand(Whois, QStringList() << user);
}

/*!
    Creates a new WHOWAS command with type IrcCommand::Whowas and parameters \a user and optional \a count.

    This command returns information about a \a user that is no longer online
    (due to client disconnection, or nickname changes). If given, the server
    will return information from the last \a count times the nickname has been used.
 */
IrcCommand* IrcCommand::createWhowas(const QString& user, int count)
{
    return IrcCommandPrivate::createCommand(Whowas, QStringList() << user << QString::number(count));
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, IrcCommand::Type type)
{
    const int index = IrcCommand::staticMetaObject.indexOfEnumerator("Type");
    QMetaEnum enumerator = IrcCommand::staticMetaObject.enumerator(index);
    const char* key = enumerator.valueToKey(type);
    debug << (key ? key : "Unknown");
    return debug;
}

QDebug operator<<(QDebug debug, const IrcCommand* command)
{
    if (!command)
        return debug << "IrcCommand(0x0) ";
    debug.nospace() << command->metaObject()->className() << '(' << (void*) command;
    if (!command->objectName().isEmpty())
        debug.nospace() << ", name=" << qPrintable(command->objectName());
    debug.nospace() << ", type=" << command->type();
    QString str = command->toString();
    if (!str.isEmpty())
        debug.nospace() << ", " << str.left(20);
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM

#include "moc_irccommand.cpp"

IRC_END_NAMESPACE
