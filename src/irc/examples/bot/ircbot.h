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

#ifndef IRCBOT_H
#define IRCBOT_H

#include <IrcSession>

class IrcBot : public IrcSession
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel WRITE setChannel)

public:
    IrcBot(QObject* parent = 0);

    QString channel() const;
    void setChannel(const QString& channel);

private slots:
    void onConnected();
    void onMessageReceived(IrcMessage* message);

private:
    QString m_channel;
};

#endif // IRCBOT_H
