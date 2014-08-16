/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
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

#include "irccommand.h"
#include <QTextCodec>
#include <QDebug>

/*!
    \file irccommand.h
    \brief #include &lt;IrcCommand&gt;
 */

/*!
    \class IrcCommand irccommand.h <IrcCommand>
    \ingroup core
    \brief The IrcCommand class provides the most common IRC commands.

    The IrcCommand class supports the most common IRC commands out of the box,
    and can be extended for custom commands as well. See IrcCommand::Type for
    the list of built-in command types. IRC commands, as in IrcCommand instances,
    are sent to the IRC server via IrcSession::sendCommand().

    \section create Creating commands

    It is recommended to create IrcCommand instances via static
    IrcCommand::createXxx() methods.

    \warning IrcCommand instances must be allocated on the heap, since
    IrcSession::sendCommand() takes ownership of the command and deletes
    it once it has been sent.

    \section custom Custom commands

    A "custom command" here refers to command types not listed in IrcCommand::Type,
    the list of built-in command types. There are two ways to send custom commands:
    \li by passing the string representation of a command directly to
    IrcSession::sendRaw() or
    \li by subclassing IrcCommand and reimplementing
    IrcCommand::toString(), which essentially creates the string representation
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

    \sa IrcSession::sendCommand(), IrcSession::sendRaw(), IrcCommand::Type
 */

/*!
    \enum IrcCommand::Type
    This enum describes the built-in command types.
 */

/*!
    \var IrcCommand::Custom
    \brief A custom command

    \sa IrcCommand::toString()
 */

/*!
    \var IrcCommand::Nick
    \brief A nick command (NICK) is used to give user a nickname or change the previous one.
 */

/*!
    \var IrcCommand::Quit
    \brief A quit command (QUIT) is used to end a client session.
 */

/*!
    \var IrcCommand::Join
    \brief A join command (JOIN) is used to start listening a specific channel.
 */

/*!
    \var IrcCommand::Part
    \brief A part command (PART) causes the client to be removed from the channel.
 */

/*!
    \var IrcCommand::Topic
    \brief A topic command (TOPIC) is used to change or view the topic of a channel.
 */

/*!
    \var IrcCommand::Names
    \brief A names command (NAMES) is used to list all nicknames on a channel.
 */

/*!
    \var IrcCommand::List
    \brief A list command (LIST) is used to list channels and their topics.
 */

/*!
    \var IrcCommand::Invite
    \brief An invite command (INVITE) is used to invite users to a channel.
 */

/*!
    \var IrcCommand::Kick
    \brief A kick command (KICK) is used to forcibly remove a user from a channel.
 */

/*!
    \var IrcCommand::Mode
    \brief A mode command (MODE) is used to change the mode of users and channels.
 */

/*!
    \var IrcCommand::Message
    \brief A message command (PRIVMSG) is used to send private messages to channels and users.
 */

/*!
    \var IrcCommand::Notice
    \brief A notice command (NOTICE) is used to send notice messages to channels and users.
 */

/*!
    \var IrcCommand::CtcpAction
    \brief A CTCP action command is used to send an action message to channels and users.
 */

/*!
    \var IrcCommand::CtcpRequest
    \brief A CTCP request command is used to send a request.
 */

/*!
    \var IrcCommand::CtcpReply
    \brief A CTCP reply command is used to send a reply to a request.
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

/*!
    \var IrcCommand::Away
    \brief An away command (AWAY) is used to set the away status.
 */

/*!
    \var IrcCommand::Quote
    \brief A quote command is used to send a raw message to the server.
 */

/*!
    \var IrcCommand::Capability
    \brief A capability command (CAP) is used to manage connection capabilities.
 */

class IrcCommandPrivate
{
public:
    IrcCommandPrivate() : encoding("UTF-8") { }

    IrcCommand::Type type;
    QStringList parameters;
    QByteArray encoding;

    static IrcCommand* createCommand(IrcCommand::Type type, const QStringList& parameters);
};

IrcCommand* IrcCommandPrivate::createCommand(IrcCommand::Type type, const QStringList& parameters)
{
    IrcCommand* command = new IrcCommand;
    command->setType(type);
    command->setParameters(parameters);
    return command;
}

/*!
    Constructs a new IrcCommand with \a parent.
 */
IrcCommand::IrcCommand(QObject* parent) : QObject(parent), d_ptr(new IrcCommandPrivate)
{
    Q_D(IrcCommand);
    d->type = Custom;
}

/*!
    Destructs the IRC command.
 */
IrcCommand::~IrcCommand()
{
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
    sending the command via IrcSession::sendCommand().

    See QTextCodec::availableCodes() for the list of
    supported encodings. The default value is "UTF-8".

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
    extern bool irc_is_supported_encoding(const QByteArray& encoding); // ircdecoder.cpp
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

    switch (d->type)
    {

    case Away:          return QString("AWAY :%1").arg(p0); // reason
    case Capability:    return QString("CAP %1 :%2").arg(p0, p1); // subcmd, caps
    case CtcpAction:    return QString("PRIVMSG %1 :\1ACTION %2\1").arg(p0, p1); // target, msg
    case CtcpRequest:   return QString("PRIVMSG %1 :\1%2\1").arg(p0, p1); // target, msg
    case CtcpReply:     return QString("NOTICE %1 :\1%2\1").arg(p0, p1); // target, msg
    case Invite:        return QString("INVITE %1 %2").arg(p0, p1); // user, chan
    case Join:          return p1.isNull() ? QString("JOIN %1").arg(p0) : QString("JOIN %1 %2").arg(p0, p1); // chan, key
    case Kick:          return p2.isNull() ? QString("KICK %1 %2").arg(p0, p1) : QString("KICK %1 %2 :%3").arg(p0, p1, p2); // chan, user, reason
    case List:          return p1.isNull() ? QString("LIST %1").arg(p0) : QString("LIST %1 %2").arg(p0, p1); // chan, server
    case Message:       return QString("PRIVMSG %1 :%2").arg(p0, p1); // target, msg
    case Mode:          return QString("MODE ") + d->parameters.join(" "); // target, mode, arg
    case Names:         return QString("NAMES %1").arg(p0); // chan
    case Nick:          return QString("NICK %1").arg(p0); // nick
    case Notice:        return QString("NOTICE %1 :%2").arg(p0, p1); // target, msg
    case Part:          return p1.isNull() ? QString("PART %1").arg(p0) : QString("PART %1 :%2").arg(p0, p1); // chan, reason
    case Quit:          return QString("QUIT :%1").arg(p0); // reason
    case Quote:         return d->parameters.join(" ");
    case Topic:         return p1.isNull() ? QString("TOPIC %1").arg(p0) : QString("TOPIC %1 :%2").arg(p0, p1); // chan, topic
    case Who:           return QString("WHO %1").arg(p0); // user
    case Whois:         return QString("WHOIS %1 %1").arg(p0); // user
    case Whowas:        return QString("WHOWAS %1 %1").arg(p0); // user

    case Custom:        qWarning("Reimplement IrcCommand::toString() for IrcCommand::Custom");
    default:            return QString();
    }
}

/*!
    Creates a new away command with type IrcCommand::Away and optional parameter \a reason.
 */
IrcCommand* IrcCommand::createAway(const QString& reason)
{
    return IrcCommandPrivate::createCommand(Away, QStringList() << reason);
}

/*!
    Creates a new capability command with type IrcCommand::Capability and parameters \a subCommand and optional \a capabilities.

    Available subcommands are: LS, LIST, REQ, ACK, NAK, CLEAR and END.
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
    Creates a new invite command with type IrcCommand::Invite and parameters \a user and \a channel.
 */
IrcCommand* IrcCommand::createInvite(const QString& user, const QString& channel)
{
    return IrcCommandPrivate::createCommand(Invite, QStringList() << user << channel);
}

/*!
    Creates a new join command with type IrcCommand::Join and parameters \a channel and optional \a key.
 */
IrcCommand* IrcCommand::createJoin(const QString& channel, const QString& key)
{
    return IrcCommandPrivate::createCommand(Join, QStringList() << channel << key);
}

/*!
    Creates a new kick command with type IrcCommand::Kick and parameters \a channel, \a user and optional \a reason.
 */
IrcCommand* IrcCommand::createKick(const QString& channel, const QString& user, const QString& reason)
{
    return IrcCommandPrivate::createCommand(Kick, QStringList() << channel << user << reason);
}

/*!
    Creates a new list command with type IrcCommand::List and parameters \a channel and optional \a server.
 */
IrcCommand* IrcCommand::createList(const QString& channel, const QString& server)
{
    return IrcCommandPrivate::createCommand(List, QStringList() << channel << server);
}

/*!
    Creates a new message command with type IrcCommand::Message and parameters \a target and \a message.
 */
IrcCommand* IrcCommand::createMessage(const QString& target, const QString& message)
{
    return IrcCommandPrivate::createCommand(Message, QStringList() << target << message);
}

/*!
    Creates a new mode command with type IrcCommand::Mode and parameters \a target, \a mode and optional \a arg.
 */
IrcCommand* IrcCommand::createMode(const QString& target, const QString& mode, const QString& arg)
{
    return IrcCommandPrivate::createCommand(Mode, QStringList() << target << mode << arg);
}

/*!
    Creates a new names command with type IrcCommand::Names and parameter \a channel.
 */
IrcCommand* IrcCommand::createNames(const QString& channel)
{
    return IrcCommandPrivate::createCommand(Names, QStringList() << channel);
}

/*!
    Creates a new nick command with type IrcCommand::Nick and parameter \a nick.
 */
IrcCommand* IrcCommand::createNick(const QString& nick)
{
    return IrcCommandPrivate::createCommand(Nick, QStringList() << nick);
}

/*!
    Creates a new notice command with type IrcCommand::Notice and parameters \a target and \a message.
 */
IrcCommand* IrcCommand::createNotice(const QString& target, const QString& message)
{
    return IrcCommandPrivate::createCommand(Notice, QStringList() << target << message);
}

/*!
    Creates a new part command with type IrcCommand::Part and parameters \a channel and optional \a reason.
 */
IrcCommand* IrcCommand::createPart(const QString& channel, const QString& reason)
{
    return IrcCommandPrivate::createCommand(Part, QStringList() << channel << reason);
}

/*!
    Creates a new quit command with type IrcCommand::Quit and optional parameter \a reason.
 */
IrcCommand* IrcCommand::createQuit(const QString& reason)
{
    return IrcCommandPrivate::createCommand(Quit, QStringList() << reason);
}

/*!
    Creates a new quote command with type IrcCommand::Quote and \a parameters.
 */
IrcCommand* IrcCommand::createQuote(const QStringList& parameters)
{
    return IrcCommandPrivate::createCommand(Quote, parameters);
}

/*!
    Creates a new topic command with type IrcCommand::Topic and parameters \a channel and optional \a topic.
 */
IrcCommand* IrcCommand::createTopic(const QString& channel, const QString& topic)
{
    return IrcCommandPrivate::createCommand(Topic, QStringList() << channel << topic);
}

/*!
    Creates a new who command with type IrcCommand::Who and parameter \a mask.
 */
IrcCommand* IrcCommand::createWho(const QString& mask)
{
    return IrcCommandPrivate::createCommand(Who, QStringList() << mask);
}

/*!
    Creates a new whois command with type IrcCommand::Whois and parameter \a user.
 */
IrcCommand* IrcCommand::createWhois(const QString& user)
{
    return IrcCommandPrivate::createCommand(Whois, QStringList() << user);
}

/*!
    Creates a new whowas command with type IrcCommand::Whowas and parameter \a user.
 */
IrcCommand* IrcCommand::createWhowas(const QString& user)
{
    return IrcCommandPrivate::createCommand(Whowas, QStringList() << user);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const IrcCommand* command)
{
    if (!command)
        return debug << "IrcCommand(0x0) ";
    debug.nospace() << command->metaObject()->className() << '(' << (void*) command;
    if (!command->objectName().isEmpty())
        debug << ", name = " << command->objectName();
    QString str = command->toString();
    if (!str.isEmpty())
        debug << "'" << str.left(20) << "'";
    debug.nospace() << ')';
    return debug.space();
}
#endif // QT_NO_DEBUG_STREAM
