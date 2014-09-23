/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/

#ifndef IRCUTIL_H
#define IRCUTIL_H

#include <IrcGlobal>
#include <QString>

class COMMUNI_EXPORT IrcUtil
{
public:
    static QString messageToHtml(const QString& message);
    static QString colorCodeToName(int code, const QString& defaultColor = QLatin1String("black"));
};

#endif // IRCUTIL_H
