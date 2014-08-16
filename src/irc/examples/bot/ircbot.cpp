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

#include "ircbot.h"
#include <IrcCommand>
#include <IrcMessage>

IrcBot::IrcBot(QObject* parent) : IrcSession(parent)
{
    connect(this, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(this, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
}

QString IrcBot::channel() const
{
    return m_channel;
}

void IrcBot::setChannel(const QString& channel)
{
    m_channel = channel;
}

void IrcBot::onConnected()
{
    sendCommand(IrcCommand::createJoin(m_channel));
}

void IrcBot::onMessageReceived(IrcMessage* message)
{
    if (message->type() == IrcMessage::Private)
    {
        IrcPrivateMessage* msg = static_cast<IrcPrivateMessage*>(message);

        if (!msg->target().compare(nickName(), Qt::CaseInsensitive))
        {
            // echo private message
            sendCommand(IrcCommand::createMessage(msg->sender().name(), msg->message()));
        }
        else if (msg->message().startsWith(nickName(), Qt::CaseInsensitive))
        {
            // echo prefixed channel message
            QString reply = msg->message().mid(msg->message().indexOf(" "));
            sendCommand(IrcCommand::createMessage(m_channel, msg->sender().name() + ":" + reply));
        }
    }
}
