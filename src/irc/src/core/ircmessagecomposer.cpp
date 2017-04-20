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

#include "ircmessagecomposer_p.h"
#include "ircmessage.h"
#include "irc.h"

IRC_BEGIN_NAMESPACE

#ifndef IRC_DOXYGEN
IrcMessageComposer::IrcMessageComposer(IrcConnection* connection)
{
    d.connection = connection;
}

bool IrcMessageComposer::isComposed(int code)
{
    switch (code) {
    case Irc::RPL_MOTDSTART:
    case Irc::RPL_MOTD:
    case Irc::RPL_ENDOFMOTD:
    case Irc::RPL_NAMREPLY:
    case Irc::RPL_ENDOFNAMES:
    case Irc::RPL_TOPIC:
    case Irc::RPL_NOTOPIC:
    case Irc::RPL_INVITING:
    case Irc::RPL_INVITED:
    case Irc::RPL_WHOREPLY:
    case Irc::RPL_ENDOFWHO:
    case Irc::RPL_CHANNELMODEIS:
    case Irc::RPL_AWAY:
    case Irc::RPL_UNAWAY:
    case Irc::RPL_NOWAWAY:
    case Irc::RPL_WHOISUSER:
    case Irc::RPL_WHOWASUSER:
    case Irc::RPL_WHOISSERVER:
    case Irc::RPL_WHOISACCOUNT:
    case Irc::RPL_WHOISHOST:
    case Irc::RPL_WHOISIDLE:
    case Irc::RPL_WHOISSECURE:
    case Irc::RPL_WHOISCHANNELS:
    case Irc::RPL_ENDOFWHOIS:
    case Irc::RPL_ENDOFWHOWAS:
        return true;
    default:
        return false;
    }
}

void IrcMessageComposer::composeMessage(IrcNumericMessage* message)
{
    switch (message->code()) {
    case Irc::RPL_MOTDSTART:
        d.messages.push(new IrcMotdMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setParameters(QStringList(message->parameters().value(0)));
        break;
    case Irc::RPL_MOTD:
        d.messages.top()->setParameters(d.messages.top()->parameters() << message->parameters().value(1));
        break;
    case Irc::RPL_ENDOFMOTD:
        finishCompose(message);
        break;

    case Irc::RPL_NAMREPLY: {
        if (d.messages.empty() || d.messages.top()->type() != IrcMessage::Names)
            d.messages.push(new IrcNamesMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        int count = message->parameters().count();
        QString channel = message->parameters().value(count - 2);
        QStringList names = d.messages.top()->parameters().mid(1);
        names += message->parameters().value(count - 1).split(QLatin1Char(' '), QString::SkipEmptyParts);
        d.messages.top()->setParameters(QStringList() << channel << names);
        break;
    }
    case Irc::RPL_ENDOFNAMES:
        finishCompose(message);
        break;

    case Irc::RPL_TOPIC:
    case Irc::RPL_NOTOPIC:
        d.messages.push(new IrcTopicMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setCommand(QString::number(message->code()));
        d.messages.top()->setParameters(QStringList() << message->parameters().value(1) << message->parameters().value(2));
        finishCompose(message);
        break;

    case Irc::RPL_INVITING:
    case Irc::RPL_INVITED:
        d.messages.push(new IrcInviteMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setCommand(QString::number(message->code()));
        d.messages.top()->setParameters(QStringList() << message->parameters().value(1) << message->parameters().value(2));
        finishCompose(message);
        break;

    case Irc::RPL_WHOREPLY: {
        d.messages.push(new IrcWhoReplyMessage(d.connection));
        d.messages.top()->setPrefix(message->parameters().value(5) // nick
                                    + QLatin1Char('!') + message->parameters().value(2) // ident
                                    + QLatin1Char('@') + message->parameters().value(3)); // host
        d.messages.top()->setCommand(QString::number(message->code()));
        d.messages.top()->setParameters(QStringList() << message->parameters().value(1) // mask
                                                      << message->parameters().value(4) // server
                                                      << message->parameters().value(6)); // status
        QString last = message->parameters().value(7);
        int index = last.indexOf(QLatin1Char(' ')); // ignore hopcount
        if (index != -1)
            d.messages.top()->setParameters(d.messages.top()->parameters() << last.mid(index + 1)); // real name
        finishCompose(message);
        break;
    }

    case Irc::RPL_CHANNELMODEIS:
        d.messages.push(new IrcModeMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setCommand(QString::number(message->code()));
        d.messages.top()->setParameters(message->parameters().mid(1));
        finishCompose(message);
        break;

    case Irc::RPL_AWAY:
        if (!d.messages.isEmpty() && d.messages.top()->type() == IrcMessage::Whois) {
            replaceParam(9, message->parameters().value(2)); // away reason
            break;
        }
        // flow through
    case Irc::RPL_UNAWAY:
    case Irc::RPL_NOWAWAY:
        d.messages.push(new IrcAwayMessage(d.connection));
        d.messages.top()->setCommand(QString::number(message->code()));
        if (message->code() == Irc::RPL_AWAY) {
            d.messages.top()->setPrefix(message->parameters().value(1));
            d.messages.top()->setParameters(message->parameters().mid(2));
        } else {
            d.messages.top()->setPrefix(message->parameters().value(0));
            d.messages.top()->setParameters(message->parameters().mid(1));
        }
        finishCompose(message);
        break;

    case Irc::RPL_WHOISUSER:
        d.messages.push(new IrcWhoisMessage(d.connection));
        d.messages.top()->setPrefix(message->parameters().value(1)
                                    + "!" + message->parameters().value(2)
                                    + "@" + message->parameters().value(3));
        d.messages.top()->setParameters(QStringList() << message->parameters().value(5)
                                                      << QString()   // server
                                                      << QString()   // info
                                                      << QString()   // account
                                                      << QString()   // address
                                                      << QString()   // since
                                                      << QString()   // idle
                                                      << QString()   // secure
                                                      << QString()   // channels
                                                      << QString()); // away reason
        break;

    case Irc::RPL_WHOWASUSER:
        d.messages.push(new IrcWhowasMessage(d.connection));
        d.messages.top()->setPrefix(message->parameters().value(1)
                                    + "!" + message->parameters().value(2)
                                    + "@" + message->parameters().value(3));
        d.messages.top()->setParameters(QStringList() << message->parameters().value(5)
                                                      << QString()   // server
                                                      << QString()   // info
                                                      << QString()   // account
                                                      << QString()   // address
                                                      << QString()   // since
                                                      << QString()   // idle
                                                      << QString()   // secure
                                                      << QString()); // channels
        break;

    case Irc::RPL_WHOISSERVER:
        replaceParam(1, message->parameters().value(2)); // server
        replaceParam(2, message->parameters().value(3)); // info
        break;

    case Irc::RPL_WHOISACCOUNT:
        replaceParam(3, message->parameters().value(2));
        break;

    case Irc::RPL_WHOISHOST:
        replaceParam(4, QStringList(message->parameters().mid(2)).join(QLatin1String(" ")));
        break;

    case Irc::RPL_WHOISIDLE:
        replaceParam(5, message->parameters().value(3)); // since
        replaceParam(6, message->parameters().value(2)); // idle
        break;

    case Irc::RPL_WHOISSECURE:
        replaceParam(7, "using a secure connection");
        break;

    case Irc::RPL_WHOISCHANNELS:
        replaceParam(8, message->parameters().value(2)); // channels
        break;

    case Irc::RPL_ENDOFWHOIS:
    case Irc::RPL_ENDOFWHOWAS:
        finishCompose(message);
        break;
    }
}

void IrcMessageComposer::finishCompose(IrcMessage* message)
{
    if (!d.messages.isEmpty()) {
        IrcMessage* composed = d.messages.pop();
        composed->setTimeStamp(message->timeStamp());
        if (message->testFlag(IrcMessage::Implicit))
            composed->setFlag(IrcMessage::Implicit);
        emit messageComposed(composed);
    }
}

void IrcMessageComposer::replaceParam(int index, const QString& param)
{
    if (!d.messages.isEmpty()) {
        QStringList params = d.messages.top()->parameters();
        if (index < params.count())
            params.replace(index, param);
        d.messages.top()->setParameters(params);
    }
}
#endif // IRC_DOXYGEN

#include "moc_ircmessagecomposer_p.cpp"

IRC_END_NAMESPACE
