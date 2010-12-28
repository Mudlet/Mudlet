/*
* Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
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

#ifndef IRC_BUFFER_P_H
#define IRC_BUFFER_P_H

#include <QHash>

namespace Irc
{
    class Buffer;

    class BufferPrivate
    {
        Q_DECLARE_PUBLIC(Buffer)

    public:
        BufferPrivate();

        void addName(QString name);
        void removeName(const QString& name);
        void setReceiver(const QString& rec, bool replace = true);
        void updateMode(const QString& name, const QString& mode);

        Buffer* q_ptr;
        QString receiver;
        QString topic;
        QHash<QString, QString> names;
    };
}

#endif // IRC_BUFFER_P_H
