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

#include "messageformatter.h"
#include "usermodel.h"
#include <ircsender.h>
#include <ircutil.h>
#include <irc.h>
#include <QHash>
#include <QTime>
#include <QColor>
#include <QTextBoundaryFinder>

MessageFormatter::MessageFormatter(QObject* parent) : QObject(parent)
{
    d.highlight = false;
    d.timeStamp = false;
    d.stripNicks = true;
    d.userModel = 0;
}

MessageFormatter::~MessageFormatter()
{
}

QStringList MessageFormatter::highlights() const
{
    return d.highlights;
}

void MessageFormatter::setHighlights(const QStringList& highlights)
{
    d.highlights = highlights;
}

bool MessageFormatter::timeStamp() const
{
    return d.timeStamp;
}

void MessageFormatter::setTimeStamp(bool timeStamp)
{
    d.timeStamp = timeStamp;
}

bool MessageFormatter::stripNicks() const
{
    return d.stripNicks;
}

void MessageFormatter::setStripNicks(bool strip)
{
    d.stripNicks = strip;
}

QString MessageFormatter::timeStampFormat() const
{
    return d.timeStampFormat;
}

void MessageFormatter::setTimeStampFormat(const QString& format)
{
    d.timeStampFormat = format;
}

QString MessageFormatter::messageFormat() const
{
    return d.messageFormat;
}

void MessageFormatter::setMessageFormat(const QString& format)
{
    d.messageFormat = format;
}

QString MessageFormatter::eventFormat() const
{
    return d.prefixedFormats.value("!");
}

void MessageFormatter::setEventFormat(const QString& format)
{
    d.prefixedFormats.insert("!", format);
}

QString MessageFormatter::noticeFormat() const
{
    return d.prefixedFormats.value("[");
}

void MessageFormatter::setNoticeFormat(const QString& format)
{
    d.prefixedFormats.insert("[", format);
}

QString MessageFormatter::actionFormat() const
{
    return d.prefixedFormats.value("*");
}

void MessageFormatter::setActionFormat(const QString& format)
{
    d.prefixedFormats.insert("*", format);
}

QString MessageFormatter::unknownFormat() const
{
    return d.prefixedFormats.value("?");
}

void MessageFormatter::setUnknownFormat(const QString& format)
{
    d.prefixedFormats.insert("?", format);
}

QString MessageFormatter::highlightFormat() const
{
    return d.highlightFormat;
}

void MessageFormatter::setHighlightFormat(const QString& format)
{
    d.highlightFormat = format;
}

QString MessageFormatter::formatMessage(IrcMessage* message, UserModel* userModel) const
{
    QString formatted;
    d.highlight = false;
    d.userModel = userModel;
    switch (message->type())
    {
    case IrcMessage::Invite:
        formatted = formatInviteMessage(static_cast<IrcInviteMessage*>(message));
        break;
    case IrcMessage::Join:
        if (message->isOwn())
            d.receivedCodes.clear();
        formatted = formatJoinMessage(static_cast<IrcJoinMessage*>(message));
        break;
    case IrcMessage::Kick:
        formatted = formatKickMessage(static_cast<IrcKickMessage*>(message));
        break;
    case IrcMessage::Mode:
        formatted = formatModeMessage(static_cast<IrcModeMessage*>(message));
        break;
    case IrcMessage::Nick:
        formatted = formatNickMessage(static_cast<IrcNickMessage*>(message));
        break;
    case IrcMessage::Notice:
        formatted = formatNoticeMessage(static_cast<IrcNoticeMessage*>(message));
        break;
    case IrcMessage::Numeric:
        formatted = formatNumericMessage(static_cast<IrcNumericMessage*>(message));
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
        break;
    case IrcMessage::Unknown:
        formatted = formatUnknownMessage(static_cast<IrcMessage*>(message));
        break;
    default:
        break;
    }

    return formatMessage(formatted);
}

QString MessageFormatter::formatMessage(const QString& message) const
{
    QString formatted = message;
    if (formatted.isEmpty())
        return QString();

    QString format = d.messageFormat;
    if (d.highlight && !d.highlightFormat.isEmpty())
        format = d.highlightFormat;
    else if (d.prefixedFormats.contains(formatted.left(1)))
        format = d.prefixedFormats.value(formatted.left(1));

    if (d.timeStamp)
        formatted = tr("<span %1>[%2]</span> %3").arg(d.timeStampFormat, QTime::currentTime().toString(), formatted);

    if (!format.isNull())
        formatted = tr("<span %1>%2</span>").arg(format, formatted);

    return formatted;
}

QString MessageFormatter::formatInviteMessage(IrcInviteMessage* message) const
{
    const QString sender = formatSender(message->sender());
    return tr("! %1 invited to %3").arg(sender, message->channel());
}

QString MessageFormatter::formatJoinMessage(IrcJoinMessage* message) const
{
    const QString sender = formatSender(message->sender(), d.stripNicks);
    return tr("! %1 joined %2").arg(sender, message->channel());
}

QString MessageFormatter::formatKickMessage(IrcKickMessage* message) const
{
    const QString sender = formatSender(message->sender());
    const QString user = formatUser(message->user());
    if (!message->reason().isEmpty())
        return tr("! %1 kicked %2 (%3)").arg(sender, user, message->reason());
    else
        return tr("! %1 kicked %2").arg(sender, user);
}

QString MessageFormatter::formatModeMessage(IrcModeMessage* message) const
{
    const QString sender = formatSender(message->sender());
    return tr("! %1 sets mode %2 %3").arg(sender, message->mode(), message->argument());
}

QString MessageFormatter::formatNickMessage(IrcNickMessage* message) const
{
    const QString sender = formatSender(message->sender());
    const QString nick = formatUser(message->nick());
    return tr("! %1 changed nick to %2").arg(sender, nick);
}

QString MessageFormatter::formatNoticeMessage(IrcNoticeMessage* message) const
{
    if (message->isReply())
    {
        const QStringList params = message->message().split(" ", QString::SkipEmptyParts);
        const QString cmd = params.value(0);
        const QString arg = params.value(1);
        if (cmd.toUpper() == "PING")
            return formatPingReply(message->sender(), arg);
        else if (cmd.toUpper() == "TIME")
            return tr("! %1 time is %2").arg(formatSender(message->sender()), QStringList(params.mid(1)).join(" "));
        else if (cmd.toUpper() == "VERSION")
            return tr("! %1 version is %2").arg(formatSender(message->sender()), QStringList(params.mid(1)).join(" "));
    }

    foreach (const QString& hilite, d.highlights)
        if (message->message().contains(hilite))
            d.highlight = true;
    const QString sender = formatSender(message->sender());
    const QString msg = formatHtml(message->message());
    return tr("[%1] %2").arg(sender, msg);
}

#define P_(x) message->parameters().value(x)
#define MID_(x) QStringList(message->parameters().mid(x)).join(" ")

QString MessageFormatter::formatNumericMessage(IrcNumericMessage* message) const
{
    if (message->code() == Irc::RPL_WELCOME)
        d.receivedCodes.clear();
    d.receivedCodes += message->code();

    if (message->code() < 300)
        return tr("[INFO] %1").arg(formatHtml(MID_(1)));
    if (QByteArray(Irc::toString(message->code())).startsWith("ERR_"))
        return tr("[ERROR] %1").arg(formatHtml(MID_(1)));

    switch (message->code())
    {
    case Irc::RPL_MOTDSTART:
    case Irc::RPL_MOTD:
        return tr("[MOTD] %1").arg(formatHtml(MID_(1)));
    case Irc::RPL_ENDOFMOTD:
        return QString();
    case Irc::RPL_AWAY:
        return tr("! %1 is away (%2)").arg(P_(1), MID_(2));
    case Irc::RPL_ENDOFWHOIS:
        return QString();
    case Irc::RPL_WHOISOPERATOR:
    case Irc::RPL_WHOISMODES: // "is using modes"
    case Irc::RPL_WHOISREGNICK: // "is a registered nick"
    case Irc::RPL_WHOISHELPOP: // "is available for help"
    case Irc::RPL_WHOISSPECIAL: // "is identified to services"
    case Irc::RPL_WHOISHOST: // nick is connecting from <...>
    case Irc::RPL_WHOISSECURE: // nick is using a secure connection
        return tr("! %1").arg(MID_(1));
    case Irc::RPL_WHOISUSER:
        return tr("! %1 is %2@%3 (%4)").arg(P_(1), P_(2), P_(3), formatHtml(MID_(5)));
    case Irc::RPL_WHOISSERVER:
        return tr("! %1 is online via %2 (%3)").arg(P_(1), P_(2), P_(3));
    case Irc::RPL_WHOISACCOUNT: // nick user is logged in as
        return tr("! %1 %3 %2").arg(P_(1), P_(2), P_(3));
    case Irc::RPL_WHOWASUSER:
        return tr("! %1 was %2@%3 %4 %5").arg(P_(1), P_(2), P_(3), P_(4), P_(5));
    case Irc::RPL_WHOISIDLE: {
        QDateTime signon = QDateTime::fromTime_t(P_(3).toInt());
        QString idle = formatIdleTime(P_(2).toInt());
        return tr("! %1 has been online since %2 (idle for %3)").arg(P_(1), signon.toString(), idle);
    }
    case Irc::RPL_WHOISCHANNELS:
        return tr("! %1 is on channels %2").arg(P_(1), P_(2));
    case Irc::RPL_CHANNELMODEIS:
        return tr("! %1 mode is %2").arg(P_(1), P_(2));
    case Irc::RPL_CHANNEL_URL:
        return tr("! %1 url is %2").arg(P_(1), formatHtml(P_(2)));
    case Irc::RPL_CREATIONTIME: {
        QDateTime dateTime = QDateTime::fromTime_t(P_(2).toInt());
        return tr("! %1 was created %2").arg(P_(1), dateTime.toString());
    }
    case Irc::RPL_NOTOPIC:
        return tr("! %1 has no topic set").arg(P_(1));
    case Irc::RPL_TOPIC:
        return tr("! %1 topic is \"%2\"").arg(P_(1), formatHtml(P_(2)));
    case Irc::RPL_TOPICWHOTIME: {
        QDateTime dateTime = QDateTime::fromTime_t(P_(3).toInt());
        return tr("! %1 topic was set %2 by %3").arg(P_(1), dateTime.toString(), formatUser(P_(2), d.stripNicks));
    }
    case Irc::RPL_INVITING:
        return tr("! inviting %1 to %2").arg(formatUser(P_(1)), P_(2));
    case Irc::RPL_VERSION:
        return tr("! %1 version is %2").arg(formatSender(message->sender()), P_(1));
    case Irc::RPL_TIME:
        return tr("! %1 time is %2").arg(formatUser(P_(1)), P_(2));
    case Irc::RPL_UNAWAY:
    case Irc::RPL_NOWAWAY:
        return tr("! %1").arg(P_(1));

    case Irc::RPL_NAMREPLY:
        if (d.receivedCodes.contains(Irc::RPL_ENDOFNAMES))
        {
            int count = message->parameters().count();
            QString channel = message->parameters().value(count - 2);
            QStringList names;
            foreach (const QString& name, message->parameters().value(count - 1).split(" ", QString::SkipEmptyParts))
                names += IrcSender(name).name();
            return tr("! %1 users: %2").arg(channel).arg(names.join(" "));
        }
        return QString();

    case Irc::RPL_ENDOFNAMES:
        if (d.userModel && !d.receivedCodes.mid(0, d.receivedCodes.count() - 1).contains(Irc::RPL_ENDOFNAMES))
            return tr("! %1 has %2 users").arg(message->parameters().value(1)).arg(d.userModel->rowCount());
        return QString();

    default:
        return tr("[%1] %2").arg(message->code()).arg(QStringList(message->parameters().mid(1)).join(" "));
    }
}

QString MessageFormatter::formatPartMessage(IrcPartMessage* message) const
{
    const QString sender = formatSender(message->sender(), d.stripNicks);
    if (!message->reason().isEmpty())
        return tr("! %1 parted %2 (%3)").arg(sender, message->channel(), formatHtml(message->reason()));
    else
        return tr("! %1 parted %2").arg(sender, message->channel());
}

QString MessageFormatter::formatPongMessage(IrcPongMessage* message) const
{
    return formatPingReply(message->sender(), message->argument());
}

QString MessageFormatter::formatPrivateMessage(IrcPrivateMessage* message) const
{
    foreach (const QString& hilite, d.highlights)
        if (message->message().contains(hilite))
            d.highlight = true;
    const QString sender = formatSender(message->sender());
    const QString msg = formatHtml(message->message());
    if (message->isAction())
        return tr("* %1 %2").arg(sender, msg);
    else if (message->isRequest())
        return tr("! %1 requested %2").arg(sender, msg.split(" ").value(0).toLower());
    else
        return tr("&lt;%1&gt; %2").arg(sender, msg);
}

QString MessageFormatter::formatQuitMessage(IrcQuitMessage* message) const
{
    const QString sender = formatSender(message->sender(), d.stripNicks);
    if (!message->reason().isEmpty())
        return tr("! %1 has quit (%2)").arg(sender, formatHtml(message->reason()));
    else
        return tr("! %1 has quit").arg(sender);
}

QString MessageFormatter::formatTopicMessage(IrcTopicMessage* message) const
{
    const QString sender = formatSender(message->sender());
    const QString topic = formatHtml(message->topic());
    return tr("! %1 sets topic \"%2\" on %3").arg(sender, topic, message->channel());
}

QString MessageFormatter::formatUnknownMessage(IrcMessage* message) const
{
    const QString sender = formatSender(message->sender());
    return tr("? %1 %2 %3").arg(sender, message->command(), message->parameters().join(" "));
}

QString MessageFormatter::formatPingReply(const IrcSender& sender, const QString& arg)
{
    bool ok;
    int seconds = arg.toInt(&ok);
    if (ok)
    {
        QDateTime time = QDateTime::fromTime_t(seconds);
        QString result = QString::number(time.secsTo(QDateTime::currentDateTime()));
        return tr("! %1 replied in %2s").arg(formatSender(sender), result);
    }
    return QString();
}

QString MessageFormatter::formatSender(const IrcSender& sender, bool strip)
{
    QString name = sender.name();
    if (sender.isValid())
    {
        QColor color = QColor::fromHsl(qHash(name) % 359, 255, 64);
        name = QString("<strong style='color:%1'>%2</strong>").arg(color.name()).arg(name);
        if (!strip && !sender.user().isEmpty() && !sender.host().isEmpty())
            name = QString("%1 (%2@%3)").arg(name, sender.user(), sender.host());
    }
    return name;
}

QString MessageFormatter::formatUser(const QString& user, bool strip)
{
    return formatSender(IrcSender(user), strip);
}

QString MessageFormatter::formatIdleTime(int secs)
{
    QStringList idle;
    if (int days = secs / 86400)
        idle += tr("%1 days").arg(days);
    secs %= 86400;
    if (int hours = secs / 3600)
        idle += tr("%1 hours").arg(hours);
    secs %= 3600;
    if (int mins = secs / 60)
        idle += tr("%1 mins").arg(mins);
    idle += tr("%1 secs").arg(secs % 60);
    return idle.join(" ");
}

#if QT_VERSION >= 0x050000
#   define BOUNDARY_REASON_START QTextBoundaryFinder::StartOfItem
#   define BOUNDARY_REASON_END   QTextBoundaryFinder::EndOfItem
#else
#   define BOUNDARY_REASON_START QTextBoundaryFinder::StartWord
#   define BOUNDARY_REASON_END   QTextBoundaryFinder::EndWord
#endif

QString MessageFormatter::formatHtml(const QString& message) const
{
    QString msg = IrcUtil::messageToHtml(message);
    if (d.userModel)
    {
        foreach (const QString& user, d.userModel->users())
        {
            int pos = 0;
            while ((pos = msg.indexOf(user, pos)) != -1)
            {
                QTextBoundaryFinder finder(QTextBoundaryFinder::Word, msg);

                finder.setPosition(pos);
                if (!finder.isAtBoundary() || !finder.boundaryReasons().testFlag(BOUNDARY_REASON_START))
                {
                    pos += user.length();
                    continue;
                }

                finder.setPosition(pos + user.length());
                if (!finder.isAtBoundary() || !finder.boundaryReasons().testFlag(BOUNDARY_REASON_END))
                {
                    pos += user.length();
                    continue;
                }

                const int anchor = msg.indexOf("</a>", pos + user.length() + 1);
                if (anchor != -1 && anchor <= msg.indexOf('<', pos + user.length() + 1))
                {
                    pos += user.length();
                    continue;
                }

                QString formatted = formatUser(msg.mid(pos, user.length()));
                msg.replace(pos, user.length(), formatted);
                pos += formatted.length();
            }
        }
    }
    return msg;
}
