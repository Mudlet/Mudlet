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

#ifndef IRCCODECPLUGIN_H
#define IRCCODECPLUGIN_H

#include <IrcGlobal>
#include <QtCore/qplugin.h>
#include <QtCore/qbytearray.h>

struct COMMUNI_EXPORT IrcCodecInterface
{
    virtual QByteArray key() const = 0;
    virtual QByteArray codecForData(const QByteArray& data) = 0;
};

Q_DECLARE_INTERFACE(IrcCodecInterface, "Communi.IrcCodecInterface")

class COMMUNI_EXPORT IrcCodecPlugin : public QObject, public IrcCodecInterface
{
    Q_OBJECT
    Q_INTERFACES(IrcCodecInterface)

public:
    explicit IrcCodecPlugin(QObject* parent = 0);
    virtual ~IrcCodecPlugin();

    virtual QByteArray key() const = 0;
    virtual QByteArray codecForData(const QByteArray& data) = 0;
};

#endif // IRCCODECPLUGIN_H
