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

#ifndef SESSIONCHILDITEM_H
#define SESSIONCHILDITEM_H

#include "abstractsessionitem.h"

class SessionItem;
class IrcMessage;
class IrcCommand;
class UserModel;

class SessionChildItem : public AbstractSessionItem
{
    Q_OBJECT
    Q_PROPERTY(bool channel READ isChannel NOTIFY channelChanged)
    Q_PROPERTY(SessionItem* sessionItem READ sessionItem)
    Q_PROPERTY(QStringList users READ users)

public:
    explicit SessionChildItem(SessionItem* parent);
    virtual ~SessionChildItem();

    bool isChannel() const;
    QStringList users() const;
    SessionItem* sessionItem() const;

    QStringList completions(const QString& prefix, const QString& word) const;
    void updateCurrent(AbstractSessionItem* item);

public slots:
    void sendUiCommand(IrcCommand* command);

signals:
    void channelChanged();
    void alerted(const QString& text);
    void namesReceived(const QStringList& names);
    void whoisReceived(const QStringList& whois);

protected slots:
    void receiveMessage(IrcMessage* message);

private:
    SessionItem* m_parent;
    QStringList m_whois;
    UserModel* m_usermodel;
    QSet<int> m_sent;
};

#endif // SESSIONCHILDITEM_H
