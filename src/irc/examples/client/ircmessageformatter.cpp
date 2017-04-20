/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircmessageformatter.h"

#include <IrcTextFormat>
#include <IrcConnection>
#include <QTime>
#include <Irc>

QString IrcMessageFormatter::formatMessage(IrcMessage* message)
{
    QString formatted;
    switch (message->type()) {
        case IrcMessage::Join:
            formatted = formatJoinMessage(static_cast<IrcJoinMessage*>(message));
            break;
        case IrcMessage::Mode:
            formatted = formatModeMessage(static_cast<IrcModeMessage*>(message));
            break;
        case IrcMessage::Names:
            formatted = formatNamesMessage(static_cast<IrcNamesMessage*>(message));
            break;
        case IrcMessage::Nick:
            formatted = formatNickMessage(static_cast<IrcNickMessage*>(message));
            break;
        case IrcMessage::Part:
            formatted = formatPartMessage(static_cast<IrcPartMessage*>(message));
            break;
        case IrcMessage::Private:
            formatted = formatPrivateMessage(static_cast<IrcPrivateMessage*>(message));
            break;
        case IrcMessage::Quit:
            formatted = formatQuitMessage(static_cast<IrcQuitMessage*>(message));
            break;
        default:
            break;
    }
    return formatMessage(formatted);
}

QString IrcMessageFormatter::formatMessage(const QString& message)
{
    if (!message.isEmpty()) {
        QString formatted = QObject::tr("[%1] %2").arg(QTime::currentTime().toString(), message);
        if (message.startsWith("!"))
            formatted = QObject::tr("<font color='gray'>%1</font>").arg(formatted);
        else if (message.startsWith("*"))
            formatted = QObject::tr("<font color='maroon'>%1</font>").arg(formatted);
        else if (message.startsWith("["))
            formatted = QObject::tr("<font color='indianred'>%1</font>").arg(formatted);
        return formatted;
    }
    return QString();
}

QString IrcMessageFormatter::formatJoinMessage(IrcJoinMessage* message)
{
    if (message->flags() & IrcMessage::Own)
        return QObject::tr("! You have joined %1 as %2").arg(message->channel(), message->nick());
    else
        return QObject::tr("! %1 has joined %2").arg(message->nick(), message->channel());
}

QString IrcMessageFormatter::formatModeMessage(IrcModeMessage* message)
{
    QString args = message->arguments().join(" ");
    if (message->isReply())
        return QObject::tr("! %1 mode is %2 %3").arg(message->target(), message->mode(), args);
    else
        return QObject::tr("! %1 sets mode %2 %3 %4").arg(message->nick(), message->target(), message->mode(), args);
}

QString IrcMessageFormatter::formatNamesMessage(IrcNamesMessage* message)
{
    return QObject::tr("! %1 has %2 users").arg(message->channel()).arg(message->names().count());
}

QString IrcMessageFormatter::formatNickMessage(IrcNickMessage* message)
{
    return QObject::tr("! %1 has changed nick to %2").arg(message->oldNick(), message->newNick());
}

QString IrcMessageFormatter::formatPartMessage(IrcPartMessage* message)
{
    if (message->reason().isEmpty())
        return QObject::tr("! %1 has left %2").arg(message->nick(), message->channel());
    else
        return QObject::tr("! %1 has left %2 (%3)").arg(message->nick(), message->channel(), message->reason());
}

QString IrcMessageFormatter::formatPrivateMessage(IrcPrivateMessage* message)
{
    const QString content = IrcTextFormat().toHtml(message->content());
    if (message->isAction())
        return QObject::tr("* %1 %2").arg(message->nick(), content);
    else
        return QObject::tr("&lt;%1&gt; %2").arg(message->nick(),content);
}

QString IrcMessageFormatter::formatQuitMessage(IrcQuitMessage* message)
{
    if (message->reason().isEmpty())
        return QObject::tr("! %1 has quit").arg(message->nick());
    else
        return QObject::tr("! %1 has quit (%2)").arg(message->nick(), message->reason());
}
