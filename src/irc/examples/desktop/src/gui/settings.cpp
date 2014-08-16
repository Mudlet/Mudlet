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

#include "settings.h"
#include <QApplication>

Settings::Settings() : maxBlockCount(-1), timeStamp(true), layout("tree"), stripNicks(true)
{
#ifdef Q_OS_MAC
    QString navigate("Ctrl+Alt+%1");
    QString nextUnread("Shift+Ctrl+Alt+%1");
#else
    QString navigate("Alt+%1");
    QString nextUnread("Shift+Alt+%1");
#endif
    shortcuts[NavigateUp] = navigate.arg("Up");
    shortcuts[NavigateDown] = navigate.arg("Down");
    shortcuts[NavigateLeft] = navigate.arg("Left");
    shortcuts[NavigateRight] = navigate.arg("Right");

    shortcuts[NextUnreadUp] = nextUnread.arg("Up");
    shortcuts[NextUnreadDown] = nextUnread.arg("Down");
    shortcuts[NextUnreadLeft] = nextUnread.arg("Left");
    shortcuts[NextUnreadRight] = nextUnread.arg("Right");

    messages[Joins] = true;
    messages[Parts] = true;
    messages[Nicks] = true;
    messages[Modes] = true;
    messages[Kicks] = true;
    messages[Quits] = true;
    messages[Topics] = true;

    highlights[Joins] = false;
    highlights[Parts] = false;
    highlights[Nicks] = false;
    highlights[Modes] = false;
    highlights[Kicks] = false;
    highlights[Quits] = false;
    highlights[Topics] = false;

    // TODO: the default values should respect palette
    colors[Background] = "#ffffff";
    colors[Message] = "#000000";
    colors[Event] = "#808080";
    colors[Notice] = "#a54242";
    colors[Action] = "#8b388b";
    colors[Highlight] = "#ff4040";
    colors[TimeStamp] = "#808080";
    colors[Link] = "#4040ff";
}

Settings::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

bool Settings::operator==(const Settings& other) const
{
    return messages == other.messages && highlights == other.highlights
        && language == other.language && font == other.font && colors == other.colors
        && shortcuts == other.shortcuts && maxBlockCount == other.maxBlockCount
        && timeStamp == other.timeStamp && layout == other.layout
        && stripNicks == other.stripNicks;
}

bool Settings::operator!=(const Settings& other) const
{
    return !(*this == other);
}
