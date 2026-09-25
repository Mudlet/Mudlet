/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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

#include <IrcTextFormat>

// communi escapes & and < before it strips the IRC formatting codes, and hands
// out the plain text with those entities still in it; a script wants the text
// the way it was sent. Only those two are escaped, and &lt; is undone first so
// that a literal "&lt;" (which came through as "&amp;lt;") survives the trip.
static QString plainTextForLua(const QString& text)
{
    return IrcTextFormat().toPlainText(text).replace(QStringLiteral("&lt;"), QStringLiteral("<")).replace(QStringLiteral("&amp;"), QStringLiteral("&"));
}

// Whatever a server or another user fills in reaches the IRC window as HTML and
// a script as it was sent, so every such field goes through here, or through
// nameFor() if it is a name, rather than being interpolated raw into a line
// the IRC window renders as markup, opening any link in it when clicked.
static QString contentFor(const QString& text, bool isForLua)
{
    if (isForLua) {
        return plainTextForLua(text);
    }
    return IrcTextFormat().toHtml(text);
}

// Nicks, channels, hosts and the like are names rather than text, so they are
// only escaped: toHtml() would turn a channel such as #www.example.org into a
// link, and a script is given the name exactly as the server spelled it.
static QString nameFor(const QString& name, bool isForLua)
{
    if (isForLua) {
        return name;
    }
    return name.toHtmlEscaped();
}

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
    case IrcMessage::Kick:
        formatted = formatKickMessage(static_cast<IrcKickMessage*>(message), isForLua);
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

        if (message.startsWith("!")) {
            formatted = QString("<font color='gray'>%1</font>").arg(formatted);
        } else if (message.startsWith("*")) {
            formatted = QString("<font color='maroon'>%1</font>").arg(formatted);
        } else if (message.startsWith("$")) {
            formatted = QString("<font color='#3cc46e'>%1</font>").arg(formatted);
        } else if (message.startsWith("[")) {
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
        content = plainTextForLua(message->content());
    } else {
        content = IrcTextFormat().toHtml(message->content());
    }

    if (message->flags() & IrcMessage::Own) {
        return QObject::tr("! %1").arg(content);
    }
    const QString nick = nameFor(message->nick(), isForLua);
    if (!message->content().isEmpty()) {
        return QObject::tr("! %1 is away (%2)").arg(nick, content);
    }
    return QObject::tr("! %1 is back").arg(nick);
}

QString IrcMessageFormatter::formatInviteMessage(IrcInviteMessage* message, bool isForLua)
{
    const QString channel = nameFor(message->channel(), isForLua);
    if (message->isReply()) {
        return QObject::tr("! invited %1 to %2").arg(nameFor(message->user(), isForLua), channel);
    }

    return QObject::tr("! %2 invited to %3").arg(nameFor(message->nick(), isForLua), channel);
}

QString IrcMessageFormatter::formatJoinMessage(IrcJoinMessage* message, bool isForLua)
{
    const QString nick = nameFor(message->nick(), isForLua);
    const QString channel = nameFor(message->channel(), isForLua);
    if (message->flags() & IrcMessage::Own) {
        return QObject::tr("! You have joined %1 as %2").arg(channel, nick);
    }
    return QObject::tr("! %1 has joined %2").arg(nick, channel);
}

QString IrcMessageFormatter::formatKickMessage(IrcKickMessage* message, bool isForLua)
{
    const QString nick = nameFor(message->nick(), isForLua);
    const QString user = nameFor(message->user(), isForLua);
    const QString channel = nameFor(message->channel(), isForLua);

    if (message->reason().isEmpty()) {
        //: Shown in the IRC client when someone is kicked out of a channel without a reason being given. %1 is the nickname doing the kicking, %2 the nickname being kicked, %3 the channel.
        return QObject::tr("! %1 kicked %2 from %3").arg(nick, user, channel);
    }

    //: Shown in the IRC client when someone is kicked out of a channel. %1 is the nickname doing the kicking, %2 the nickname being kicked, %3 the channel, %4 the reason the kicker gave.
    return QObject::tr("! %1 kicked %2 from %3 (%4)").arg(nick, user, channel, contentFor(message->reason(), isForLua));
}

QString IrcMessageFormatter::formatModeMessage(IrcModeMessage* message, bool isForLua)
{
    const QString target = nameFor(message->target(), isForLua);
    const QString mode = nameFor(message->mode(), isForLua);
    const QString args = nameFor(message->arguments().join(" "), isForLua);
    if (message->isReply()) {
        return QObject::tr("! %1 mode is %2 %3").arg(target, mode, args);
    }
    return QObject::tr("! %1 sets mode %2 %3 %4").arg(nameFor(message->nick(), isForLua), target, mode, args);
}

QString IrcMessageFormatter::formatMotdMessage(IrcMotdMessage* message, bool isForLua)
{
    QString motdData;
    for (const auto& line : message->lines()) {
        QString content, lineEnd;
        if (isForLua) {
            lineEnd = "\n";
            content = plainTextForLua(line);
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
    const QString channel = nameFor(message->channel(), isForLua);
    if (isForLua) {
        // lua actually needs the names for parsing, since getting a names
        // list from the UI userModel alone would be limiting to the IRC commands.
        const QString nameList = nameFor(message->names().join(" "), isForLua);
        return QObject::tr("! %1 has %2 users: %3").arg(channel, count, nameList);
    }
    return QObject::tr("! %1 has %2 users").arg(channel, count);
}

QString IrcMessageFormatter::formatNickMessage(IrcNickMessage* message, bool isForLua)
{
    return QObject::tr("! %1 has changed nick to %2").arg(nameFor(message->oldNick(), isForLua), nameFor(message->newNick(), isForLua));
}

QString IrcMessageFormatter::formatNoticeMessage(IrcNoticeMessage* message, bool isForLua)
{
    const QString nick = nameFor(message->nick(), isForLua);
    if (message->isReply()) {
        const QStringList params = message->content().split(" ", Qt::SkipEmptyParts);
        const QString cmd = params.value(0);
        if (cmd.toUpper() == "PING") {
            const QString secs = formatSeconds(params.value(1).toInt());
            return QObject::tr("! %1 replied in %2").arg(nick, secs);
        }

        if (cmd.toUpper() == "TIME") {
            const QString rest = contentFor(QStringList(params.mid(1)).join(" "), isForLua);
            return QObject::tr("! %1 time is %2").arg(nick, rest);
        }

        if (cmd.toUpper() == "VERSION") {
            const QString rest = contentFor(QStringList(params.mid(1)).join(" "), isForLua);
            return QObject::tr("! %1 version is %2").arg(nick, rest);
        }
    }

    QString pfx = nameFor(message->statusPrefix(), isForLua);
    if (!pfx.isEmpty()) {
        pfx = ":" + pfx;
    }

    if (message->isPrivate()) {
        QString content;
        if (isForLua) {
            content = plainTextForLua(message->content());
        } else {
            content = IrcTextFormat().toHtml(message->content());
        }
        return QObject::tr("[%1%2] %3").arg(nick, pfx, content);
    }

    if (isForLua) {
        // lua only needs the message text.
        return plainTextForLua(message->content());
    }
    const QString content = IrcTextFormat().toHtml(message->content());
    return QObject::tr("&lt;%1%2&gt; [%3] %4").arg(nick, pfx, nameFor(message->target(), isForLua), content);
}

QString IrcMessageFormatter::formatNumericMessage(IrcNumericMessage* message, bool isForLua)
{
    if (message->code() < 300) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        QString content;
        if (isForLua) {
            content = plainTextForLua(info);
        } else {
            content = IrcTextFormat().toHtml(info);
        }
        return QObject::tr("[INFO] %1").arg(content);
    }

    switch (message->code()) {
    case Irc::RPL_VERSION:
        return QObject::tr("! %1 version is %2").arg(nameFor(message->nick(), isForLua), contentFor(message->parameters().value(1), isForLua));

    case Irc::RPL_TIME:
        return QObject::tr("! %1 time is %2").arg(nameFor(message->parameters().value(1), isForLua), contentFor(message->parameters().value(2), isForLua));

    default:
        break;
    }

    if (message->isComposed() || message->flags() & IrcMessage::Implicit) {
        return QString();
    }

    // if you change this, change formatErrorMessage too
    if (Irc::codeToString(message->code()).startsWith("ERR_")) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        QString content;
        if (isForLua) {
            content = plainTextForLua(info);
        } else {
            content = IrcTextFormat().toHtml(info);
        }
        return QObject::tr("[ERROR] %1").arg(content);
    }
    if (message->code() == Irc::RPL_CHANNEL_URL) {
        const QString info = QStringList(message->parameters().mid(1)).join(" ");
        QString content;
        if (isForLua) {
            content = plainTextForLua(info);
        } else {
            content = IrcTextFormat().toHtml(info);
        }
        return QObject::tr("[Channel URL] %1").arg(content);
    }
    const QString info = QStringList(message->parameters().mid(1)).join(" ");
    QString content;
    if (isForLua) {
        content = plainTextForLua(info);
    } else {
        content = IrcTextFormat().toHtml(info);
    }
    return QObject::tr("[%1] %2").arg(QString::number(message->code()), content);
}

QString IrcMessageFormatter::formatErrorMessage(IrcErrorMessage* message, bool isForLua)
{
    // if you change this, change ERR_ in formatNumericMessage too
    return QObject::tr("[ERROR] %1").arg(contentFor(message->error(), isForLua));
}

QString IrcMessageFormatter::formatPartMessage(IrcPartMessage* message, bool isForLua)
{
    const QString nick = nameFor(message->nick(), isForLua);
    const QString channel = nameFor(message->channel(), isForLua);
    if (message->reason().isEmpty()) {
        return QObject::tr("! %1 has left %2").arg(nick, channel);
    }
    return QObject::tr("! %1 has left %2 (%3)").arg(nick, channel, contentFor(message->reason(), isForLua));
}

QString IrcMessageFormatter::formatPongMessage(IrcPongMessage* message, bool isForLua)
{
    quint64 const msec = message->timeStamp().toMSecsSinceEpoch();
    quint64 const dms = (QDateTime::currentMSecsSinceEpoch() - msec);
    return QObject::tr("! %1 replied in %2 seconds").arg(nameFor(message->nick(), isForLua)).arg(dms / 1000.0, 4, 'f', 3, QLatin1Char('0'));
}

// Normal messages sent to channels are processed by our client as if they are private messages.
QString IrcMessageFormatter::formatPrivateMessage(IrcPrivateMessage* message, bool isForLua)
{
    QString content;
    if (isForLua) {
        content = plainTextForLua(message->content());
    } else {
        content = IrcTextFormat().toHtml(message->content());
    }

    const QString nick = nameFor(message->nick(), isForLua);
    if (message->isAction()) {
        return QObject::tr("* %1 %2").arg(nick, content);
    }
    if (isForLua) {
        // lua only needs the message text here.  Nick and target are sent as arguments to postIrcMessage()
        return content;
    }
    return QObject::tr("<b>&lt;%1&gt;</b> %2").arg(nick, content);
}

QString IrcMessageFormatter::formatQuitMessage(IrcQuitMessage* message, bool isForLua)
{
    if (message->reason().isEmpty()) {
        return QObject::tr("! %1 has quit").arg(nameFor(message->nick(), isForLua));
    }
    return QObject::tr("! %1 has quit (%2)").arg(nameFor(message->nick(), isForLua), contentFor(message->reason(), isForLua));
}

QString IrcMessageFormatter::formatTopicMessage(IrcTopicMessage* message, bool isForLua)
{
    if (message->isReply()) {
        if (message->topic().isEmpty()) {
            return QObject::tr("! no topic");
        }

        QString topic;
        if (isForLua) {
            topic = plainTextForLua(message->topic());
        } else {
            topic = IrcTextFormat().toHtml(message->topic());
        }
        return QObject::tr("[TOPIC] %1").arg(topic);
    }

    if (message->topic().isEmpty()) {
        return QObject::tr("! %2 cleared topic").arg(nameFor(message->nick(), isForLua));
    }

    return QObject::tr("! %2 changed topic").arg(nameFor(message->nick(), isForLua));
}

QString IrcMessageFormatter::formatUnknownMessage(IrcMessage* message, bool isForLua)
{
    return QObject::tr("? %2 %3 %4").arg(nameFor(message->nick(), isForLua), nameFor(message->command(), isForLua), contentFor(message->parameters().join(" "), isForLua));
}

QString IrcMessageFormatter::formatWhoisMessage(IrcWhoisMessage* message, bool isForLua)
{
    const QString nick = nameFor(message->nick(), isForLua);
    QString wData;
    wData = QObject::tr("[WHOIS] %1 is %2@%3 (%4)").arg(nick, nameFor(message->ident(), isForLua), nameFor(message->host(), isForLua), contentFor(message->realName(), isForLua));
    wData += QObject::tr("[WHOIS] %1 is connected via %2 (%3)").arg(nick, nameFor(message->server(), isForLua), contentFor(message->info(), isForLua));
    wData += QObject::tr("[WHOIS] %1 is connected since %2 (idle %3)").arg(nick, message->since().toString(), formatDuration(message->idle()));
    if (!message->awayReason().isEmpty()) {
        wData += QObject::tr("[WHOIS] %1 is away: %2").arg(nick, contentFor(message->awayReason(), isForLua));
    }
    if (!message->account().isEmpty()) {
        wData += QObject::tr("[WHOIS] %1 is logged in as %2").arg(nick, nameFor(message->account(), isForLua));
    }
    if (!message->address().isEmpty()) {
        wData += QObject::tr("[WHOIS] %1 is connected from %2").arg(nick, nameFor(message->address(), isForLua));
    }
    if (message->isSecure()) {
        wData += QObject::tr("[WHOIS] %1 is using a secure connection").arg(nick);
    }
    if (!message->channels().isEmpty()) {
        wData += QObject::tr("[WHOIS] %1 is on %2").arg(nick, nameFor(message->channels().join(" "), isForLua));
    }
    return wData;
}

QString IrcMessageFormatter::formatWhowasMessage(IrcWhowasMessage* message, bool isForLua)
{
    const QString nick = nameFor(message->nick(), isForLua);
    QString wData;
    wData = QObject::tr("[WHOWAS] %1 was %2@%3 (%4)").arg(nick, nameFor(message->ident(), isForLua), nameFor(message->host(), isForLua), contentFor(message->realName(), isForLua));
    wData += QObject::tr("[WHOWAS] %1 was connected via %2 (%3)").arg(nick, nameFor(message->server(), isForLua), contentFor(message->info(), isForLua));
    if (!message->account().isEmpty()) {
        wData += QObject::tr("[WHOWAS] %1 was logged in as %2").arg(nick, nameFor(message->account(), isForLua));
    }
    return wData;
}

QString IrcMessageFormatter::formatWhoReplyMessage(IrcWhoReplyMessage* message, bool isForLua)
{
    QString format = QObject::tr("[WHO] %1 (%2)").arg(nameFor(message->nick(), isForLua), contentFor(message->realName(), isForLua));
    if (message->isAway()) {
        format += QObject::tr(" - away");
    }
    if (message->isServOp()) {
        format += QObject::tr(" - server operator");
    }
    return format;
}

QString IrcMessageFormatter::formatSeconds(int secs)
{
    const QDateTime time = QDateTime::fromSecsSinceEpoch(secs);
    return QObject::tr("%1s").arg(time.secsTo(QDateTime::currentDateTime()));
}

QString IrcMessageFormatter::formatDuration(int secs)
{
    QStringList idle;
    if (const int days = secs / 86400) {
        idle += QObject::tr("%1 days").arg(days);
    }
    secs %= 86400;
    if (const int hours = secs / 3600) {
        idle += QObject::tr("%1 hours").arg(hours);
    }
    secs %= 3600;
    if (const int mins = secs / 60) {
        idle += QObject::tr("%1 mins").arg(mins);
    }
    idle += QObject::tr("%1 secs").arg(secs % 60);
    return idle.join(" ");
}
