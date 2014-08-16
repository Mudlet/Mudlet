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

#include "sessionchilditem.h"
#include "sortedusermodel.h"
#include "sessionitem.h"
#include "usermodel.h"
#include "session.h"
#include <QDateTime>
#include <ircutil.h>
#include <irc.h>

SessionChildItem::SessionChildItem(SessionItem* parent) :
    AbstractSessionItem(parent), m_parent(parent), m_usermodel(0)
{
    setSession(parent->session());
    m_usermodel = new UserModel(session());
    connect(this, SIGNAL(titleChanged(QString)), SIGNAL(channelChanged()));
    connect(this, SIGNAL(titleChanged(QString)), m_usermodel, SLOT(setChannel(QString)));
}

SessionChildItem::~SessionChildItem()
{
    delete m_usermodel;
}

bool SessionChildItem::isChannel() const
{
    return session()->isChannel(title());
}

QStringList SessionChildItem::users() const
{
    SortedUserModel sorted(session()->prefixModes(), m_usermodel);
    QStringList names;
    for (int i = 0; i < sorted.rowCount(QModelIndex()); ++i)
        names += sorted.data(sorted.index(i, 0)).toString();
    return names;
}

SessionItem* SessionChildItem::sessionItem() const
{
    return m_parent;
}

QStringList SessionChildItem::completions(const QString& prefix, const QString& word) const
{
    if (word != "/" && prefix != "/")
        return m_usermodel->users().filter(QRegExp("^"+word+".*", Qt::CaseInsensitive));
    return AbstractSessionItem::completions(prefix, word);
}

void SessionChildItem::updateCurrent(AbstractSessionItem* item)
{
    m_parent->updateCurrent(item);
}

void SessionChildItem::sendUiCommand(IrcCommand *command)
{
    m_sent.insert(command->type());
    session()->sendUiCommand(command);
}

void SessionChildItem::receiveMessage(IrcMessage* message)
{
    if (m_usermodel)
        m_usermodel->processMessage(message);

    if (message->type() == IrcMessage::Numeric)
    {
        IrcNumericMessage* numeric = static_cast<IrcNumericMessage*>(message);
        switch (numeric->code())
        {
        case Irc::RPL_NAMREPLY:
            if (m_sent.contains(IrcCommand::Names))
                return;
            break;
        case Irc::RPL_ENDOFNAMES:
            if (m_sent.contains(IrcCommand::Names))
            {
                emit namesReceived(m_usermodel->users());
                m_sent.remove(IrcCommand::Names);
                return;
            }
            break;
        case Irc::RPL_TOPIC:
            if (isChannel())
            {
                setSubtitle(message->parameters().value(2));
                setDescription(IrcUtil::messageToHtml(subtitle()));
            }
            break;
        case Irc::RPL_WHOISUSER:
            if (!isChannel())
            {
                setSubtitle(message->parameters().value(5));
                setDescription(IrcUtil::messageToHtml(subtitle()));
            }
            if (m_sent.contains(IrcCommand::Whois))
            {
                m_whois.append(tr("Ident: %1").arg(message->parameters().value(2)));
                m_whois.append(tr("Host: %1").arg(message->parameters().value(3)));
                m_whois.append(tr("Name: %1").arg(message->parameters().value(5)));
                return;
            }
            break;
        case Irc::RPL_WHOISSERVER:
            if (m_sent.contains(IrcCommand::Whois))
            {
                m_whois.append(tr("Server: %1 (%2)").arg(message->parameters().value(2), message->parameters().value(3)));
                return;
            }
            break;
        case Irc::RPL_WHOISOPERATOR:
            if (m_sent.contains(IrcCommand::Whois))
            {
                m_whois.append(tr("IRC operator"));
                return;
            }
            break;
        case Irc::RPL_WHOISACCOUNT:
            if (m_sent.contains(IrcCommand::Whois))
            {
                m_whois.append(tr("Logged in as: %1").arg(message->parameters().value(2)));
                return;
            }
            break;
        case Irc::RPL_WHOISREGNICK:
            if (m_sent.contains(IrcCommand::Whois))
            {
                m_whois.append(tr("Registered nick"));
                return;
            }
            break;
        case Irc::RPL_WHOISSECURE:
            if (m_sent.contains(IrcCommand::Whois))
            {
                m_whois.append(tr("Secure connection"));
                return;
            }
            break;
        case Irc::RPL_WHOISIDLE:
            if (m_sent.contains(IrcCommand::Whois))
            {
                QDateTime signon = QDateTime::fromTime_t(message->parameters().value(3).toInt());
                QTime idle = QTime().addSecs(message->parameters().value(2).toInt());
                m_whois.append(tr("Connected: %1").arg(signon.toString()));
                m_whois.append(tr("Idle: %1").arg(idle.toString()));
                return;
            }
            break;
        case Irc::RPL_WHOISCHANNELS:
            if (m_sent.contains(IrcCommand::Whois))
            {
                m_whois.append(tr("Channels: %1").arg(message->parameters().value(2)));
                return;
            }
            break;
        case Irc::RPL_WHOISHOST:
        case Irc::RPL_WHOISMODES:
            if (m_sent.contains(IrcCommand::Whois))
            {
                return;
            }
            break;
        case Irc::RPL_ENDOFWHOIS:
            if (m_sent.contains(IrcCommand::Whois))
            {
                emit whoisReceived(m_whois);
                m_sent.remove(IrcCommand::Whois);
                m_whois.clear();
            }
        case Irc::RPL_WHOISHELPOP:
        case Irc::RPL_WHOISSPECIAL:
            return;
        default:
            break;
        }
    }

    if (message->type() == IrcMessage::Private)
    {
        IrcPrivateMessage* privMsg = static_cast<IrcPrivateMessage*>(message);

        QString alertText;
        if (isChannel())
        {
            if (privMsg->message().contains(m_parent->session()->nickName(), Qt::CaseInsensitive))
            {
                setHighlighted(true);
                if (!isCurrent())
                    alertText = tr("%1 on %2:\n%3").arg(privMsg->sender().name()).arg(title()).arg(privMsg->message());
            }
        }
        else
        {
            setHighlighted(true);
            if (!isCurrent())
                alertText = tr("%1 in private:\n%2").arg(privMsg->sender().name()).arg(privMsg->message());
        }

        if (!alertText.isEmpty())
            emit alerted(alertText);

        if (!isCurrent())
            setUnreadCount(unreadCount() + 1);
    }

    const QString formatted = messageFormatter()->formatMessage(message, m_usermodel);
    if (!formatted.isEmpty())
        appendMessage(formatted);
}
