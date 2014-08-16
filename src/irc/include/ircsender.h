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

#ifndef IRCSENDER_H
#define IRCSENDER_H

#include <IrcGlobal>
#include <QtCore/qstring.h>
#include <QtCore/qmetatype.h>

class COMMUNI_EXPORT IrcSender
{
public:
    IrcSender(const QString& prefix = QString());

    bool isValid() const;

    QString prefix() const;
    void setPrefix(const QString& prefix);

    QString name() const { return n; }
    void setName(const QString& name) { n = name; }

    QString user() const { return u; }
    void setUser(const QString& user) { u = user; }

    QString host() const { return h; }
    void setHost(const QString& host) { h = host; }

private:
    QString n;
    QString u;
    QString h;
};

Q_DECLARE_METATYPE(IrcSender)

#endif // IRCSENDER_H
