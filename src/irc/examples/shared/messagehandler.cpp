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

#include "messagehandler.h"
#include "messagereceiver.h"
#include "session.h"
#include <qabstractsocket.h>
#include <qvariant.h>
#include <qdebug.h>
#include <irc.h>

MessageHandler::MessageHandler(QObject* parent) : QObject(parent)
{
    d.session = 0;
    d.defaultReceiver = 0;
    d.currentReceiver = 0;
    setSession(qobject_cast<Session*>(parent));
}

MessageHandler::~MessageHandler()
{
    d.defaultReceiver = 0;
    d.currentReceiver = 0;
    d.receivers.clear();
    if (d.session && d.session->isActive())
        d.session->socket()->waitForDisconnected(500);
}

Session* MessageHandler::session() const
{
    return d.session;
}

void MessageHandler::setSession(Session* session)
{
    if (d.session != session)
    {
        if (d.session)
        {
            disconnect(d.session, SIGNAL(destroyed()), this, SLOT(onSessionDestroyed()));
            disconnect(d.session, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(handleMessage(IrcMessage*)));
        }

        if (session)
        {
            connect(session, SIGNAL(destroyed()), this, SLOT(onSessionDestroyed()));
            connect(session, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(handleMessage(IrcMessage*)));
        }

        d.session = session;
    }
}

MessageReceiver* MessageHandler::defaultReceiver() const
{
    return d.defaultReceiver;
}

void MessageHandler::setDefaultReceiver(MessageReceiver* receiver)
{
    d.defaultReceiver = receiver;
}

MessageReceiver* MessageHandler::currentReceiver() const
{
    return d.currentReceiver;
}

void MessageHandler::setCurrentReceiver(MessageReceiver* receiver)
{
    d.currentReceiver = receiver;
}

void MessageHandler::addReceiver(const QString& name, MessageReceiver* receiver)
{
    d.receivers.insert(name.toLower(), receiver);
}

MessageReceiver* MessageHandler::getReceiver(const QString& name) const
{
    return d.receivers.value(name.toLower());
}

void MessageHandler::removeReceiver(const QString& name)
{
    const QString lower = name.toLower();
    if (d.receivers.contains(lower))
    {
        d.receivers.remove(lower);
        emit receiverToBeRemoved(name);
    }
}

void MessageHandler::handleMessage(IrcMessage* message)
{
    switch (message->type())
    {
    case IrcMessage::Invite:
        handleInviteMessage(static_cast<IrcInviteMessage*>(message));
        break;
    case IrcMessage::Join:
        handleJoinMessage(static_cast<IrcJoinMessage*>(message));
        break;
    case IrcMessage::Kick:
        handleKickMessage(static_cast<IrcKickMessage*>(message));
        break;
    case IrcMessage::Mode:
        handleModeMessage(static_cast<IrcModeMessage*>(message));
        break;
    case IrcMessage::Nick:
        handleNickMessage(static_cast<IrcNickMessage*>(message));
        break;
    case IrcMessage::Notice:
        handleNoticeMessage(static_cast<IrcNoticeMessage*>(message));
        break;
    case IrcMessage::Numeric:
        handleNumericMessage(static_cast<IrcNumericMessage*>(message));
        break;
    case IrcMessage::Part:
        handlePartMessage(static_cast<IrcPartMessage*>(message));
        break;
    case IrcMessage::Pong:
        handlePongMessage(static_cast<IrcPongMessage*>(message));
        break;
    case IrcMessage::Private:
        handlePrivateMessage(static_cast<IrcPrivateMessage*>(message));
        break;
    case IrcMessage::Quit:
        handleQuitMessage(static_cast<IrcQuitMessage*>(message));
        break;
    case IrcMessage::Topic:
        handleTopicMessage(static_cast<IrcTopicMessage*>(message));
        break;
    case IrcMessage::Unknown:
        handleUnknownMessage(static_cast<IrcMessage*>(message));
        break;
    default:
        break;
    }
}

void MessageHandler::handleInviteMessage(IrcInviteMessage* message)
{
    sendMessage(message, d.currentReceiver);
}

void MessageHandler::handleJoinMessage(IrcJoinMessage* message)
{
    sendMessage(message, message->channel());
}

void MessageHandler::handleKickMessage(IrcKickMessage* message)
{
    sendMessage(message, message->channel());
}

void MessageHandler::handleModeMessage(IrcModeMessage* message)
{
    if (message->sender().name() == message->target())
        sendMessage(message, d.defaultReceiver);
    else
        sendMessage(message, message->target());
}

void MessageHandler::handleNickMessage(IrcNickMessage* message)
{
    bool received = false;
    QString nick = message->sender().name().toLower();
    foreach (MessageReceiver* receiver, d.receivers)
    {
        if (receiver->hasUser(nick))
        {
            received = true;
            receiver->receiveMessage(message);
        }
    }
    if (!received && d.currentReceiver)
        d.currentReceiver->receiveMessage(message);

    foreach (const QString& receiver, d.receivers.keys())
    {
        if (!nick.compare(receiver, Qt::CaseInsensitive))
        {
            emit receiverToBeRenamed(receiver, message->nick());
            MessageReceiver* object = d.receivers.take(nick);
            d.receivers.insert(nick, object);
            sendMessage(message, object);
        }
    }
}

void MessageHandler::handleNoticeMessage(IrcNoticeMessage* message)
{
    sendMessage(message, d.currentReceiver);
}

void MessageHandler::handleNumericMessage(IrcNumericMessage* message)
{
    if (QByteArray(Irc::toString(message->code())).startsWith("ERR_"))
    {
        sendMessage(message, d.currentReceiver);
        return;
    }

    switch (message->code())
    {
    case Irc::RPL_ENDOFWHO:
    case Irc::RPL_WHOREPLY:
    case Irc::RPL_UNAWAY:
    case Irc::RPL_NOWAWAY:
    case Irc::RPL_AWAY:
    case Irc::RPL_WHOISOPERATOR:
    case Irc::RPL_WHOISMODES: // "is using modes"
    case Irc::RPL_WHOISREGNICK: // "is a registered nick"
    case Irc::RPL_WHOISHELPOP: // "is available for help"
    case Irc::RPL_WHOISSPECIAL: // "is identified to services"
    case Irc::RPL_WHOISHOST: // nick is connecting from <...>
    case Irc::RPL_WHOISSECURE: // nick is using a secure connection
    case Irc::RPL_WHOISUSER:
    case Irc::RPL_WHOISSERVER:
    case Irc::RPL_WHOISACCOUNT: // nick user is logged in as
    case Irc::RPL_WHOWASUSER:
    case Irc::RPL_WHOISIDLE:
    case Irc::RPL_WHOISCHANNELS:
    case Irc::RPL_ENDOFWHOIS:
    case Irc::RPL_INVITING:
    case Irc::RPL_VERSION:
    case Irc::RPL_TIME:
        sendMessage(message, d.currentReceiver);
        break;

    case Irc::RPL_ENDOFBANLIST:
    case Irc::RPL_ENDOFEXCEPTLIST:
    case Irc::RPL_ENDOFINFO:
    case Irc::RPL_ENDOFINVITELIST:
    case Irc::RPL_ENDOFLINKS:
    case Irc::RPL_ENDOFSTATS:
    case Irc::RPL_ENDOFUSERS:
    case Irc::RPL_ENDOFWHOWAS:
        break; // ignore

    case Irc::RPL_CHANNELMODEIS:
    case Irc::RPL_CHANNEL_URL:
    case Irc::RPL_CREATIONTIME:
    case Irc::RPL_NOTOPIC:
    case Irc::RPL_TOPIC:
    case Irc::RPL_TOPICWHOTIME:
        sendMessage(message, message->parameters().value(1));
        break;

    case Irc::RPL_NAMREPLY: {
        const int count = message->parameters().count();
        const QString channel = message->parameters().value(count - 2);
        MessageReceiver* receiver = d.receivers.value(channel.toLower());
        if (receiver)
            receiver->receiveMessage(message);
        else if (d.currentReceiver)
            d.currentReceiver->receiveMessage(message);
        break;
        }

    case Irc::RPL_ENDOFNAMES:
        if (d.receivers.contains(message->parameters().value(1).toLower()))
            sendMessage(message, message->parameters().value(1));
        break;

    default:
        sendMessage(message, d.defaultReceiver);
        break;
    }
}

void MessageHandler::handlePartMessage(IrcPartMessage* message)
{
    if (message->isOwn())
        removeReceiver(message->channel());
    else
        sendMessage(message, message->channel());
}

void MessageHandler::handlePongMessage(IrcPongMessage* message)
{
    sendMessage(message, d.currentReceiver);
}

void MessageHandler::handlePrivateMessage(IrcPrivateMessage* message)
{
    if (message->isRequest())
        sendMessage(message, d.currentReceiver);
    else if (message->target() == d.session->nickName())
        sendMessage(message, message->sender().name());
    else
        sendMessage(message, message->target());
}

void MessageHandler::handleQuitMessage(IrcQuitMessage* message)
{
    QString nick = message->sender().name();
    foreach (MessageReceiver* receiver, d.receivers)
    {
        if (receiver->hasUser(nick))
            receiver->receiveMessage(message);
    }

    if (d.receivers.contains(nick.toLower()))
        sendMessage(message, nick);
}

void MessageHandler::handleTopicMessage(IrcTopicMessage* message)
{
    sendMessage(message, message->channel());
}

void MessageHandler::handleUnknownMessage(IrcMessage* message)
{
    sendMessage(message, d.defaultReceiver);
}

void MessageHandler::sendMessage(IrcMessage* message, MessageReceiver* receiver)
{
    if (receiver)
        receiver->receiveMessage(message);
}

void MessageHandler::sendMessage(IrcMessage* message, const QString& receiver)
{
    if (!d.receivers.contains(receiver.toLower()))
        emit receiverToBeAdded(receiver);
    sendMessage(message, getReceiver(receiver));
}

void MessageHandler::onSessionDestroyed()
{
    setSession(0);
}
