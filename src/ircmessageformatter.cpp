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

QString IrcMessageFormatter::formatMessage(IrcMessage* message, bool isForLua)
{
    QString formatted;
    QString color = "#f29010";

    IrcNumericMessage* nMsg;

    switch (message->type()) {
    case IrcMessage::Away:
        formatted = formatAwayMessage(static_cast<IrcAwayMessage*>(message), isForLua);
        break;
    case IrcMessage::Invite:
        formatted = formatInviteMessage(static_cast<IrcInviteMessage*>(message), isForLua);
        break;
    case IrcMessage::Join:
        formatted = formatJoinMessage(static_cast<IrcJoinMessage*>(message), isForLua);
        break;
    case IrcMessage::Mode:
        formatted = formatModeMessage(static_cast<IrcModeMessage*>(message), isForLua);
        break;
    case IrcMessage::Motd:
        formatted = formatMotdMessage(static_cast<IrcMotdMessage*>(message), isForLua);
        break;
    case IrcMessage::Names:
        formatted = formatNamesMessage(static_cast<IrcNamesMessage*>(message), isForLua);
        break;
    case IrcMessage::Nick:
        formatted = formatNickMessage(static_cast<IrcNickMessage*>(message), isForLua);
        break;
    case IrcMessage::Notice:
        formatted = formatNoticeMessage(static_cast<IrcNoticeMessage*>(message), isForLua);
        break;
    case IrcMessage::Part:
        formatted = formatPartMessage(static_cast<IrcPartMessage*>(message), isForLua);
        break;
    case IrcMessage::Pong:
        formatted = formatPongMessage(static_cast<IrcPongMessage*>(message), isForLua);
        break;
    case IrcMessage::Private:
        formatted = formatPrivateMessage(static_cast<IrcPrivateMessage*>(message), isForLua);
        break;
    case IrcMessage::Quit:
        formatted = formatQuitMessage(static_cast<IrcQuitMessage*>(message), isForLua);
        break;
    case IrcMessage::Topic:
        formatted = formatTopicMessage(static_cast<IrcTopicMessage*>(message), isForLua);
        color = "#3283bc";
        break;
    case IrcMessage::Whois:
        formatted = formatWhoisMessage(static_cast<IrcWhoisMessage*>(message), isForLua);
        break;
    case IrcMessage::Whowas:
        formatted = formatWhowasMessage(static_cast<IrcWhowasMessage*>(message), isForLua);
        break;
    case IrcMessage::WhoReply:
        formatted = formatWhoReplyMessage(static_cast<IrcWhoReplyMessage*>(message), isForLua);
        break;
    case IrcMessage::Error:
        formatted = formatErrorMessage(static_cast<IrcErrorMessage*>(message), isForLua);
        color = "indianred";
        break;
    case IrcMessage::Unknown:
        formatted = formatUnknownMessage(message, isForLua);
        break;
    case IrcMessage::Numeric:
        nMsg = static_cast<IrcNumericMessage*>(message);
        formatted = formatNumericMessage(nMsg, isForLua);
        // if you change this, change formatErrorMessage too
        if (Irc::codeToString(nMsg->code()).startsWith("ERR_")) {
            color = "indianred";
        }
        break;
    default:
        break;
    }
    return formatMessage(formatted, color, isForLua);
}

QString IrcMessageFormatter::formatMessage(const QString& message, QString color, bool isForLua)
{
    if (!message.isEmpty()) {
        if (isForLua) { // lua has no need for the timestamp or HTML added here.
            return message;
        }

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

QString IrcMessageFormatter::formatAwayMessage(IrcAwayMessage* message, bool isForLua)
{
    QString content;
    if (isForLua) {
        content = IrcTextFormat().toPlainText(message->content());
    } else {
        content = IrcTextFormat().toHtml(message->content());
    }

    if (message->flags() & IrcMessage::Own)
        return QObject::tr("! %1").arg(content);
    else if (!message->content().isEmpty())
        return QObject::tr("! %1 is away (%2)").arg(message->nick(), content);
    return QObject::tr("! %1 is back").arg(message->nick());
}

QString IrcMessageFormatter::formatInviteMessage(IrcInviteMessage* message, bool isForLua)
{
    if (message->isReply())
        return QObject::tr("! invited %1 to %2").arg(message->user(), message->channel());

    return QObject::tr("! %2 invited to %3").arg(message->nick(), message->channel());
}

QString IrcMessageFormatter::formatJoinMessage(IrcJoinMessage* message, bool isForLua)
{
    if (message->flags() & IrcMessage::Own)
        return QObject::tr("! You have joined %1 as %2").arg(message->channel(), message->nick());
    else
        return QObject::tr("! %1 has joined %2").arg(message->nick(), message->channel());
}

QString IrcMessageFormatter::formatKickMessage(IrcKickMessage* message, bool isForLua)
{
    return QObject::tr("! %1 kicked %2").arg(message->nick(), message->user());
}

QString IrcMessageFormatter::formatModeMessage(IrcModeMessage* message, bool isForLua)
{
    QString args = message->arguments().join(" ");
    if (message->isReply())
        return QObject::tr("! %1 mode is %2 %3").arg(message->target(), message->mode(), args);
    else
        return QObject::tr("! %1 sets mode %2 %3 %4").arg(message->nick(), message->target(), message->mode(), args);
}

QString IrcMessageFormatter::formatMotdMessage(IrcMotdMessage* message, bool isForLua)
{
    QString motdData;
    for (auto line : message->lines()) {
        QString content, lineEnd;
        if (isForLua) {
            lineEnd = "\n";
            content = IrcTextFormat().toPlainText(line);
        } else {
            lineEnd = "<br />\n";
            content = IrcTextFormat().toHtml(line);
        }

        motdData += QObject::tr("[MOTD] %1%2").arg(content, lineEnd);
    }
    return motdData;
}

QString IrcMessageFormatter::formatNamesMessage(IrcNamesMessage* message, bool isForLua)
{
    const QString count = QString::number(message->names().count());
    if (isForLua) {
        // lua actually needs the names for parsing, since getting a names
        // list from the UI userModel alone would be limiting to the IRC commands.
        QString nameList = message->names().join(" ");
        return QObject::tr("! %1 has %2 users: %3").arg(message->channel(), count, nameList);
    } else {
        return QObject::tr("! %1 has %2 users").arg(message->channel(), count);
    }
}

QString IrcMessageFormatter::formatNickMessage(IrcNickMessage* message, bool isForLua)
{
    return QObject::tr("! %1 has changed nick to %2").arg(message->oldNick(), message->newNick());
}

QString IrcMessageFormatter::formatNoticeMessage(IrcNoticeMessage* message, bool isForLua)
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
        QString content;
        if (isForLua) {
            content = IrcTextFormat().toPlainText(message->content());
        } else {
            content = IrcTextFormat().toHtml(message->content());
        }
        return QObject::tr("[%1%2] %3").arg(message->nick(), pfx, content);
    }

    if (isForLua) {
        // lua only needs the message text.
        return IrcTextFormat().toPlainText(message->content());
    } else {
        QString content = IrcTextFormat().toHtml(message->content());
        return QObject::tr("&lt;%1%2&gt; [%3] %4").arg(message->nick(), pfx, message->target(), content);
    }
}

QString IrcMessageFormatter::formatNumericMessage(IrcNumericMessage* message, bool isForLua)
{
    if (message->code() < 300) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        QString content;
        if (isForLua) {
            content = IrcTextFormat().toPlainText(info);
        } else {
            content = IrcTextFormat().toHtml(info);
        }
        return QObject::tr("[INFO] %1").arg(info);
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
        QString content;
        if (isForLua) {
            content = IrcTextFormat().toPlainText(info);
        } else {
            content = IrcTextFormat().toHtml(info);
        }
        return QObject::tr("[ERROR] %1").arg(content);
    }
    if (message->code() == Irc::RPL_CHANNEL_URL) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        QString content;
        if (isForLua) {
            content = IrcTextFormat().toPlainText(info);
        } else {
            content = IrcTextFormat().toHtml(info);
        }
        return QObject::tr("[Channel URL] %1").arg(content);
    }
    const QString info = QStringList(message->parameters().mid(1)).join(" ");
    QString content;
    if (isForLua) {
        content = IrcTextFormat().toPlainText(info);
    } else {
        content = IrcTextFormat().toHtml(info);
    }
    return QObject::tr("[%1] %2").arg(QString::number(message->code()), content);
}

QString IrcMessageFormatter::formatErrorMessage(IrcErrorMessage* message, bool isForLua)
{
    // if you change this, change ERR_ in formatNumericMessage too
    return QObject::tr("[ERROR] %1").arg(message->error());
}

QString IrcMessageFormatter::formatPartMessage(IrcPartMessage* message, bool isForLua)
{
    if (message->reason().isEmpty())
        return QObject::tr("! %1 has left %2").arg(message->nick(), message->channel());
    else
        return QObject::tr("! %1 has left %2 (%3)").arg(message->nick(), message->channel(), message->reason());
}

QString IrcMessageFormatter::formatPongMessage(IrcPongMessage* message, bool isForLua)
{
    quint64 msec = message->timeStamp().toMSecsSinceEpoch();
    quint64 dms = (QDateTime::currentMSecsSinceEpoch() - msec);
    const QString secs = QString().sprintf("%04.3f", (float)((float)dms / (float)1000));
    return QObject::tr("! %1 replied in %2 seconds").arg(message->nick(), secs);
}

// Normal messages sent to channels are processed by our client as if they are private messages.
QString IrcMessageFormatter::formatPrivateMessage(IrcPrivateMessage* message, bool isForLua)
{
    QString content;
    if (isForLua) {
        content = IrcTextFormat().toPlainText(message->content());
    } else {
        content = IrcTextFormat().toHtml(message->content());
    }

    if (message->isAction()) {
        return QObject::tr("* %1 %2").arg(message->nick(), content);
    } else {
        if (isForLua) {
            // lua only needs the message text here.  Nick and target are sent as arguments to postIrcMessage()
            return content;
        } else {
            return QObject::tr("<b>&lt;%1&gt;</b> %2").arg(message->nick(), content);
        }
    }
}

QString IrcMessageFormatter::formatQuitMessage(IrcQuitMessage* message, bool isForLua)
{
    if (message->reason().isEmpty())
        return QObject::tr("! %1 has quit").arg(message->nick());
    else
        return QObject::tr("! %1 has quit (%2)").arg(message->nick(), message->reason());
}

QString IrcMessageFormatter::formatTopicMessage(IrcTopicMessage* message, bool isForLua)
{
    if (message->isReply()) {
        if (message->topic().isEmpty())
            return QObject::tr("! no topic");

        QString topic;
        if (isForLua) {
            topic = IrcTextFormat().toPlainText(message->topic());
        } else {
            topic = IrcTextFormat().toHtml(message->topic());
        }
        return QObject::tr("[TOPIC] %1").arg(topic);
    }

    if (message->topic().isEmpty())
        return QObject::tr("! %2 cleared topic").arg(message->nick());

    return QObject::tr("! %2 changed topic").arg(message->nick());
}

QString IrcMessageFormatter::formatUnknownMessage(IrcMessage* message, bool isForLua)
{
    return QObject::tr("? %2 %3 %4").arg(message->nick(), message->command(), message->parameters().join(" "));
}

QString IrcMessageFormatter::formatWhoisMessage(IrcWhoisMessage* message, bool isForLua)
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

QString IrcMessageFormatter::formatWhowasMessage(IrcWhowasMessage* message, bool isForLua)
{
    QString wData;
    wData = QObject::tr("[WHOWAS] %1 was %2@%3 (%4)").arg(message->nick(), message->ident(), message->host(), message->realName());
    wData += QObject::tr("[WHOWAS] %1 was connected via %2 (%3)").arg(message->nick(), message->server(), message->info());
    if (!message->account().isEmpty())
        wData += QObject::tr("[WHOWAS] %1 was logged in as %2").arg(message->nick(), message->account());
    return wData;
}

QString IrcMessageFormatter::formatWhoReplyMessage(IrcWhoReplyMessage* message, bool isForLua)
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
