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

#include <QCoreApplication>
#include <QSettings>
#include "ircbot.h"

int main (int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    IrcBot bot;

    QSettings settings("settings.ini", QSettings::IniFormat);
    bot.setHost(settings.value("host", "irc.freenode.net").toString());
    bot.setPort(settings.value("port", 6667).toInt());
    bot.setUserName(settings.value("username", "communi").toString());
    bot.setNickName(settings.value("nickname", "communi-bot").toString());
    bot.setRealName(settings.value("realname", "communi bot").toString());
    bot.setChannel(settings.value("channel", "#communi").toString());

    bot.open();
    return app.exec();
}
