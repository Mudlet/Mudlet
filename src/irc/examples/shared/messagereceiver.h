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

#ifndef MESSAGERECEIVER_H
#define MESSAGERECEIVER_H

#include <QString>

class IrcMessage;

class MessageReceiver
{
public:
    virtual ~MessageReceiver() {}

    virtual QString receiver() const = 0;
    virtual void setReceiver(const QString& receiver) = 0;

    virtual void receiveMessage(IrcMessage* message) = 0;
    virtual bool hasUser(const QString& user) const = 0;
};

#endif // MESSAGERECEIVER_H
