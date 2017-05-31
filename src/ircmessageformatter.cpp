/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ircmessageformatter.h"

#include "pre_guard.h"
#include <IrcTextFormat>
#include "post_guard.h"

QString IrcMessageFormatter::formatMessage(IrcMessage* message)
{
    QString formatted;
    QString color = "#f29010";

    IrcNumericMessage* nMsg;

    switch (message->type()) {
        case IrcMessage::Away:
            formatted = formatAwayMessage(static_cast<IrcAwayMessage*>(message));
            break;
        case IrcMessage::Invite:
            formatted = formatInviteMessage(static_cast<IrcInviteMessage*>(message));
            break;
        case IrcMessage::Join:
            formatted = formatJoinMessage(static_cast<IrcJoinMessage*>(message));
            break;
        case IrcMessage::Mode:
            formatted = formatModeMessage(static_cast<IrcModeMessage*>(message));
            break;
        case IrcMessage::Motd:
            formatted = formatMotdMessage(static_cast<IrcMotdMessage*>(message));
            break;
        case IrcMessage::Names:
            formatted = formatNamesMessage(static_cast<IrcNamesMessage*>(message));
            break;
        case IrcMessage::Nick:
            formatted = formatNickMessage(static_cast<IrcNickMessage*>(message));
            break;
        case IrcMessage::Notice:
            formatted = formatNoticeMessage(static_cast<IrcNoticeMessage*>(message));
            break;
        case IrcMessage::Part:
            formatted = formatPartMessage(static_cast<IrcPartMessage*>(message));
            break;
        case IrcMessage::Pong:
            formatted = formatPongMessage(static_cast<IrcPongMessage*>(message));
            break;
        case IrcMessage::Private:
            formatted = formatPrivateMessage(static_cast<IrcPrivateMessage*>(message));
            break;
        case IrcMessage::Quit:
            formatted = formatQuitMessage(static_cast<IrcQuitMessage*>(message));
            break;
        case IrcMessage::Topic:
            formatted = formatTopicMessage(static_cast<IrcTopicMessage*>(message));
            color = "#3283bc";
            break;
        case IrcMessage::Whois:
            formatted = formatWhoisMessage(static_cast<IrcWhoisMessage*>(message));
            break;
        case IrcMessage::Whowas:
            formatted = formatWhowasMessage(static_cast<IrcWhowasMessage*>(message));
            break;
        case IrcMessage::WhoReply:
            formatted = formatWhoReplyMessage(static_cast<IrcWhoReplyMessage*>(message));
            break;
        case IrcMessage::Error:
            formatted = formatErrorMessage(static_cast<IrcErrorMessage*>(message));
            color = "indianred";
            break;
        case IrcMessage::Unknown:
            formatted = formatUnknownMessage(message);
            break;
        case IrcMessage::Numeric:
            nMsg = static_cast<IrcNumericMessage*>(message);
            formatted = formatNumericMessage(nMsg);
            // if you change this, change formatErrorMessage too
            if (Irc::codeToString(nMsg->code()).startsWith("ERR_")) {
                color = "indianred";
            }
            break;
        default:
            break;
    }
    return formatMessage(formatted, color);
}

QString IrcMessageFormatter::formatMessage(const QString& message, QString color)
{
    if (!message.isEmpty()) {
        QString formatted = QString("[%1] %2").arg(QTime::currentTime().toString(), message);
        if (message.startsWith("!"))
            formatted = QString("<font color='gray'>%1</font>").arg(formatted);
        else if (message.startsWith("*"))
            formatted = QString("<font color='maroon'>%1</font>").arg(formatted);
        else if (message.startsWith("$"))
            formatted = QString("<font color='#3cc46e'>%1</font>").arg(formatted);
        else if (message.startsWith("[")) {
            if (color.isEmpty()) {
                color = "#f29010";
            }
            formatted = QString("<font color='%2'>%1</font>").arg(formatted, color);
        }
        return formatted;
    }
    return QString();
}

QString IrcMessageFormatter::formatAwayMessage(IrcAwayMessage* message)
{
    if (message->flags() & IrcMessage::Own)
        return QObject::tr("! %1").arg(IrcTextFormat().toHtml(message->content()));
    else if (!message->content().isEmpty())
        return QObject::tr("! %1 is away (%2)").arg(message->nick(), IrcTextFormat().toHtml(message->content()));
    return QObject::tr("! %1 is back").arg(message->nick());
}

QString IrcMessageFormatter::formatInviteMessage(IrcInviteMessage* message)
{
    if (message->isReply())
        return QObject::tr("! invited %1 to %2").arg(message->user(), message->channel());

    return QObject::tr("! %2 invited to %3").arg(message->nick(), message->channel());
}

QString IrcMessageFormatter::formatJoinMessage(IrcJoinMessage* message)
{
    if (message->flags() & IrcMessage::Own)
        return QObject::tr("! You have joined %1 as %2").arg(message->channel(), message->nick());
    else
        return QObject::tr("! %1 has joined %2").arg(message->nick(), message->channel());
}

QString IrcMessageFormatter::formatKickMessage(IrcKickMessage* message)
{
    return QObject::tr("! %1 kicked %2").arg(message->nick(), message->user());
}

QString IrcMessageFormatter::formatModeMessage(IrcModeMessage* message)
{
    QString args = message->arguments().join(" ");
    if (message->isReply())
        return QObject::tr("! %1 mode is %2 %3").arg(message->target(), message->mode(), args);
    else
        return QObject::tr("! %1 sets mode %2 %3 %4").arg(message->nick(), message->target(), message->mode(), args);
}

QString IrcMessageFormatter::formatMotdMessage(IrcMotdMessage* message)
{
    QString motdData;
    foreach (const QString& line, message->lines()) {
        motdData += QObject::tr("[MOTD] %1").arg(IrcTextFormat().toHtml(line));
        motdData += "<br />\n";
    }
    return motdData;
}

QString IrcMessageFormatter::formatNamesMessage(IrcNamesMessage* message)
{
    return QObject::tr("! %1 has %2 users").arg(message->channel()).arg(message->names().count());
}

QString IrcMessageFormatter::formatNickMessage(IrcNickMessage* message)
{
    return QObject::tr("! %1 has changed nick to %2").arg(message->oldNick(), message->newNick());
}

QString IrcMessageFormatter::formatNoticeMessage(IrcNoticeMessage* message)
{
    if (message->isReply()) {
       const QStringList params = message->content().split(" ", QString::SkipEmptyParts);
       const QString cmd = params.value(0);
       if (cmd.toUpper() == "PING") {
           const QString secs = formatSeconds(params.value(1).toInt());
           return QObject::tr("! %1 replied in %2").arg(message->nick(), secs);
       } else if (cmd.toUpper() == "TIME") {
           const QString rest = QStringList(params.mid(1)).join(" ");
           return QObject::tr("! %1 time is %2").arg(message->nick(), rest);
       } else if (cmd.toUpper() == "VERSION") {
            const QString rest = QStringList(params.mid(1)).join(" ");
            return QObject::tr("! %1 version is %2").arg(message->nick(), rest);
       }
    }

    QString pfx = message->statusPrefix();
    if (!pfx.isEmpty())
        pfx = ":" + pfx;

    if (message->isPrivate()) {
        const QString content = IrcTextFormat().toHtml(message->content());
        return QObject::tr("[%1%2] %3").arg(message->nick(), pfx, content);
    }

    const QString content = IrcTextFormat().toHtml(message->content());
    return QObject::tr("&lt;%1%2&gt; [%3] %4").arg(message->nick(), pfx, message->target(), content);
}

QString IrcMessageFormatter::formatNumericMessage(IrcNumericMessage* message)
{
    if (message->code() < 300) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        return QObject::tr("[INFO] %1").arg(IrcTextFormat().toHtml(info));
    }

    switch (message->code()) {
        case Irc::RPL_VERSION:
            return QObject::tr("! %1 version is %2").arg(message->nick(), message->parameters().value(1));

        case Irc::RPL_TIME:
            return QObject::tr("! %1 time is %2").arg(message->parameters().value(1), message->parameters().value(2));

        default:
            break;
    }

    if (message->isComposed() || message->flags() & IrcMessage::Implicit)
        return QString();

    // if you change this, change formatErrorMessage too
    if (Irc::codeToString(message->code()).startsWith("ERR_")) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        return QObject::tr("[ERROR] %1").arg(IrcTextFormat().toHtml(info));
    }
    if (message->code() == Irc::RPL_CHANNEL_URL) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        return QObject::tr("[Channel URL] %1").arg(IrcTextFormat().toHtml(info));
    }
    const QString info = QStringList(message->parameters().mid(1)).join(" ");
    return QObject::tr("[%1] %2").arg(QString::number(message->code()), IrcTextFormat().toHtml(info));
}

QString IrcMessageFormatter::formatErrorMessage(IrcErrorMessage* message)
{
    // if you change this, change ERR_ in formatNumericMessage too
    return QObject::tr("[ERROR] %1").arg(message->error());
}

QString IrcMessageFormatter::formatPartMessage(IrcPartMessage* message)
{
    if (message->reason().isEmpty())
        return QObject::tr("! %1 has left %2").arg(message->nick(), message->channel());
    else
        return QObject::tr("! %1 has left %2 (%3)").arg(message->nick(), message->channel(), message->reason());
}

QString IrcMessageFormatter::formatPongMessage(IrcPongMessage* message)
{
    quint64 msec = message->timeStamp().toMSecsSinceEpoch();
    quint64 dms = (QDateTime::currentMSecsSinceEpoch() - msec);
    const QString secs = QString().sprintf("%04.3f", (float)((float)dms/(float)1000));
    return QObject::tr("! %1 replied in %2 seconds").arg(message->nick(), secs);
}

// Normal messages sent to channels are processed by our client as if they are
// private messages.  This is maybe a bug in Communi..
QString IrcMessageFormatter::formatPrivateMessage(IrcPrivateMessage* message)
{
    const QString content = IrcTextFormat().toHtml(message->content());
    if (message->isAction())
        return QObject::tr("* %1 %2").arg(message->nick(), content);
    else
        return QObject::tr("<b>&lt;%1&gt;</b> %2").arg(message->nick(),content);
}

QString IrcMessageFormatter::formatQuitMessage(IrcQuitMessage* message)
{
    if (message->reason().isEmpty())
        return QObject::tr("! %1 has quit").arg(message->nick());
    else
        return QObject::tr("! %1 has quit (%2)").arg(message->nick(), message->reason());
}

QString IrcMessageFormatter::formatTopicMessage(IrcTopicMessage* message)
{
    if (message->isReply()) {
        if (message->topic().isEmpty())
            return QObject::tr("! no topic");
        return QObject::tr("[TOPIC] %1").arg(IrcTextFormat().toHtml(message->topic()));
    }

    if (message->topic().isEmpty())
        return QObject::tr("! %2 cleared topic").arg(message->nick());

    return QObject::tr("! %2 changed topic").arg(message->nick());
}

QString IrcMessageFormatter::formatUnknownMessage(IrcMessage* message)
{
    return QObject::tr("? %2 %3 %4").arg(message->nick(), message->command(), message->parameters().join(" ") );
}

QString IrcMessageFormatter::formatWhoisMessage(IrcWhoisMessage* message)
{
    QString wData;
    wData = QObject::tr("[WHOIS] %1 is %2@%3 (%4)").arg(message->nick(), message->ident(), message->host(), message->realName());
    wData += QObject::tr("[WHOIS] %1 is connected via %2 (%3)").arg(message->nick(), message->server(), message->info());
    wData += QObject::tr("[WHOIS] %1 is connected since %2 (idle %3)").arg(message->nick(), message->since().toString(), formatDuration(message->idle()));
    if (!message->awayReason().isEmpty())
        wData += QObject::tr("[WHOIS] %1 is away: %2").arg(message->nick(), message->awayReason());
    if (!message->account().isEmpty())
        wData += QObject::tr("[WHOIS] %1 is logged in as %2").arg(message->nick(), message->account());
    if (!message->address().isEmpty())
        wData += QObject::tr("[WHOIS] %1 is connected from %2").arg(message->nick(), message->address());
    if (message->isSecure())
        wData += QObject::tr("[WHOIS] %1 is using a secure connection").arg(message->nick());
    if (!message->channels().isEmpty())
        wData += QObject::tr("[WHOIS] %1 is on %2").arg(message->nick(), message->channels().join(" "));
    return wData;
}

QString IrcMessageFormatter::formatWhowasMessage(IrcWhowasMessage* message)
{
    QString wData;
    wData = QObject::tr("[WHOWAS] %1 was %2@%3 (%4)").arg(message->nick(), message->ident(), message->host(), message->realName());
    wData += QObject::tr("[WHOWAS] %1 was connected via %2 (%3)").arg(message->nick(), message->server(), message->info());
    if (!message->account().isEmpty())
        wData += QObject::tr("[WHOWAS] %1 was logged in as %2").arg(message->nick(), message->account());
    return wData;
}

QString IrcMessageFormatter::formatWhoReplyMessage(IrcWhoReplyMessage* message)
{
    QString format = QObject::tr("[WHO] %1 (%2)").arg(message->nick(), message->realName());
    if (message->isAway())
        format += QObject::tr(" - away");
    if (message->isServOp())
        format += QObject::tr(" - server operator");
    return format;
}

QString IrcMessageFormatter::formatSeconds(int secs)
{
    const QDateTime time = QDateTime::fromTime_t(secs);
    return QObject::tr("%1s").arg(time.secsTo(QDateTime::currentDateTime()));
}

QString IrcMessageFormatter::formatDuration(int secs)
{
    QStringList idle;
    if (int days = secs / 86400)
        idle += QObject::tr("%1 days").arg(days);
    secs %= 86400;
    if (int hours = secs / 3600)
        idle += QObject::tr("%1 hours").arg(hours);
    secs %= 3600;
    if (int mins = secs / 60)
        idle += QObject::tr("%1 mins").arg(mins);
    idle += QObject::tr("%1 secs").arg(secs % 60);
    return idle.join(" ");
}
