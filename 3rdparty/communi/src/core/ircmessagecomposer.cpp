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

#include "ircmessagecomposer_p.h"
#include "ircmessage.h"
#include "irccore_p.h"
#include "ircdebug_p.h"
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
        startCompose(new IrcMotdMessage(d.connection), message);
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setParameters(QStringList(message->parameters().value(0)));
        break;
    case Irc::RPL_MOTD: {
        // The line belongs to the open MOTD even when another block has been opened
        // above it. A server is also free to send an RPL_MOTD with no MOTD open at all,
        // and adding it to whatever else is being composed would corrupt that message.
        //
        // Dropping the line instead would lose it for good: IrcNumericMessage::isComposed()
        // answers per code rather than per message, so RPL_MOTD is suppressed wherever a
        // client shows numerics, on the understanding that it will arrive as part of an
        // IrcMotdMessage. Give it one of its own, finished immediately, so the line is
        // shown and a later RPL_ENDOFMOTD cannot close it in place of a real MOTD.
        const qsizetype index = indexOf(IrcMessage::Motd);
        if (index != -1) {
            d.messages.at(index)->setParameters(d.messages.at(index)->parameters() << message->parameters().value(1));
        } else {
            d.messages.push(new IrcMotdMessage(d.connection));
            d.messages.top()->setPrefix(message->prefix());
            d.messages.top()->setParameters(QStringList() << message->parameters().value(0) << message->parameters().value(1));
            finishCompose(message, IrcMessage::Motd);
        }
        break;
    }
    case Irc::RPL_ENDOFMOTD:
        finishCompose(message, IrcMessage::Motd);
        break;

    case Irc::RPL_NAMREPLY: {
        int count = message->parameters().count();
        QString channel = message->parameters().value(count - 2);
        qsizetype index = indexOf(IrcMessage::Names);
        if (index == -1 || d.messages.at(index)->parameters().value(0).compare(channel, Qt::CaseInsensitive)) {
            startCompose(new IrcNamesMessage(d.connection), message);
            index = d.messages.count() - 1;
        }
        IrcMessage* composed = d.messages.at(index);
        composed->setPrefix(message->prefix());
        QStringList names = composed->parameters().mid(1);
        names += message->parameters().value(count - 1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        composed->setParameters(QStringList() << channel << names);
        break;
    }
    case Irc::RPL_ENDOFNAMES:
        finishCompose(message, IrcMessage::Names, message->parameters().value(1));
        break;

    case Irc::RPL_TOPIC:
    case Irc::RPL_NOTOPIC:
        d.messages.push(new IrcTopicMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setCommand(QString::number(message->code()));
        d.messages.top()->setParameters(QStringList() << message->parameters().value(1) << message->parameters().value(2));
        finishCompose(message, IrcMessage::Topic);
        break;

    case Irc::RPL_INVITING:
    case Irc::RPL_INVITED:
        d.messages.push(new IrcInviteMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setCommand(QString::number(message->code()));
        d.messages.top()->setParameters(QStringList() << message->parameters().value(1) << message->parameters().value(2));
        finishCompose(message, IrcMessage::Invite);
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
        finishCompose(message, IrcMessage::WhoReply);
        break;
    }

    case Irc::RPL_CHANNELMODEIS:
        d.messages.push(new IrcModeMessage(d.connection));
        d.messages.top()->setPrefix(message->prefix());
        d.messages.top()->setCommand(QString::number(message->code()));
        d.messages.top()->setParameters(message->parameters().mid(1));
        finishCompose(message, IrcMessage::Mode);
        break;

    case Irc::RPL_AWAY:
        // IRC nicks are case-insensitive, and a server may echo the one the user typed
        if (const qsizetype index = indexOf(IrcMessage::Whois); index != -1 && !d.messages.at(index)->nick().compare(message->parameters().value(1), Qt::CaseInsensitive)) {
            QStringList params = d.messages.at(index)->parameters();
            if (params.count() > 9)
                params.replace(9, message->parameters().value(2)); // away reason
            d.messages.at(index)->setParameters(params);
            break;
        }
        Q_FALLTHROUGH();
    case Irc::RPL_UNAWAY:
        Q_FALLTHROUGH();
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
        finishCompose(message, IrcMessage::Away);
        break;

    case Irc::RPL_WHOISUSER:
        startCompose(new IrcWhoisMessage(d.connection), message);
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
        startCompose(new IrcWhowasMessage(d.connection), message);
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
        finishCompose(message, IrcMessage::Whois, message->parameters().value(1));
        break;
    case Irc::RPL_ENDOFWHOWAS:
        finishCompose(message, IrcMessage::Whowas);
        break;
    }
}

// Mudlet changes to the vendored communi, as are the helpers below: a server may open a block
// inside another or send an "end of" numeric for a block that is not open, so each numeric
// finds the block it belongs to by type, wherever that sits on the stack.

// A NAMES or WHOIS of several targets ends with one reply naming them all, and NAMES of every
// channel ends with "*"
static bool endNumericCovers(const QString& targets, const QString& target)
{
    if (targets.isEmpty())
        return true;
    for (const QString& entry : targets.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        if (!entry.compare(target, Qt::CaseInsensitive))
            return true;
        if (!entry.contains(QLatin1Char('*')) && !entry.contains(QLatin1Char('?')))
            continue;
        QString pattern;
        for (const QChar character : entry) {
            if (character == QLatin1Char('*'))
                pattern += QLatin1String(".*");
            else if (character == QLatin1Char('?'))
                pattern += QLatin1Char('.');
            else
                pattern += QRegularExpression::escape(QString(character));
        }
        if (QRegularExpression(QRegularExpression::anchoredPattern(pattern), QRegularExpression::CaseInsensitiveOption).match(target).hasMatch())
            return true;
    }
    return false;
}

qsizetype IrcMessageComposer::indexOf(IrcMessage::Type type) const
{
    for (qsizetype index = d.messages.count() - 1; index >= 0; --index) {
        if (d.messages.at(index)->type() == type)
            return index;
    }
    return -1;
}

void IrcMessageComposer::deliver(qsizetype index, IrcMessage* message)
{
    IrcMessage* composed = d.messages.takeAt(index);
    composed->setTimeStamp(message->timeStamp());
    if (message->testFlag(IrcMessage::Implicit))
        composed->setFlag(IrcMessage::Implicit);
    emit messageComposed(composed);
}

// Every continuation finds the newest block of its type, so an older one would collect the
// next block's lines; it is delivered as it stands instead
void IrcMessageComposer::startCompose(IrcMessage* composed, IrcMessage* message)
{
    const qsizetype index = indexOf(composed->type());
    if (index != -1) {
        ircDebug(d.connection, IrcDebug::Status) << "delivering unfinished composed message" << composed->type();
        deliver(index, message);
    }
    d.messages.push(composed);
}

// targets, when given, is what the "end of" numeric names, and the block closes only if it is
// among them, so a stray one cannot emit a block half-built
void IrcMessageComposer::finishCompose(IrcMessage* message, IrcMessage::Type type, const QString& targets)
{
    const qsizetype index = indexOf(type);
    if (index != -1) {
        IrcMessage* composed = d.messages.at(index);
        const QString target = type == IrcMessage::Names ? composed->parameters().value(0) : composed->nick();
        if (targets.isNull() || endNumericCovers(targets, target)) {
            deliver(index, message);
            return;
        }
    }
    ircDebug(d.connection, IrcDebug::Status) << "dropping orphaned end of composed message" << type << targets;
}

void IrcMessageComposer::replaceParam(int index, const QString& param)
{
    // Every caller is a WHOIS or WHOWAS numeric, and the indexes are the slots
    // IrcMessageComposer::composeMessage() lays those two out in, so they go to the
    // newest of those two even when another block has been opened above it. With
    // neither open there is no message of their own to fall back to, since a single
    // WHOIS field is not one, so an orphaned one is dropped; the debug channel says
    // so, as it is not visible anywhere else.
    const qsizetype block = qMax(indexOf(IrcMessage::Whois), indexOf(IrcMessage::Whowas));
    if (block == -1) {
        ircDebug(d.connection, IrcDebug::Status) << "dropping orphaned WHOIS/WHOWAS parameter" << index << param;
        return;
    }

    IrcMessage* composed = d.messages.at(block);
    QStringList params = composed->parameters();
    if (index < params.count())
        params.replace(index, param);
    composed->setParameters(params);
}
#endif // IRC_DOXYGEN

#include "moc_ircmessagecomposer_p.cpp"

IRC_END_NAMESPACE
