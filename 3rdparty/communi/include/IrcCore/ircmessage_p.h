/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef IRCMESSAGE_P_H
#define IRCMESSAGE_P_H

#include <QtCore/qmap.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstringlist.h>

#include "ircmessage.h"

IRC_BEGIN_NAMESPACE

class IrcConnection;

template <class T>
class IrcExplicitValue
{
public:
    IrcExplicitValue() : exp(false), null(true) { }
    IrcExplicitValue(const T& value) : v(value), exp(false), null(false) { }

    bool isNull() const { return null; }
    bool isExplicit() const { return exp; }

    const T& value() const { return v; }
    void setValue(const T& value) { v = value; exp = true; null = false; }

    void clear() { v = T(); exp = false; null = true; }

private:
    T v;
    bool exp;
    bool null;
};

class IrcMessageData
{
public:
    static IrcMessageData fromData(const QByteArray& data);

    QByteArray content;
    QByteArray prefix;
    QByteArray command;
    QList<QByteArray> params;
    QMap<QByteArray, QByteArray> tags;
};

class IrcMessagePrivate
{
public:
    IrcMessagePrivate();

    static IrcMessagePrivate* get(IrcMessage* msg)
    {
        return msg->d_func();
    }

    QString prefix() const;
    void setPrefix(const QString& prefix);

    QString nick() const;
    QString ident() const;
    QString host() const;

    QString command() const;
    void setCommand(const QString& command);

    QStringList params() const;
    QString param(int index) const;
    void setParams(const QStringList& params);

    QVariantMap tags() const;
    void setTags(const QVariantMap& tags);

    QByteArray content() const;

    void invalidate();

    static QString decode(const QByteArray& data, const QByteArray& encoding);
    static bool parsePrefix(const QString& prefix, QString* nick, QString* ident, QString* host);

    IrcConnection* connection;
    IrcMessage::Type type;
    QDateTime timeStamp;
    QByteArray encoding;
    mutable int flags;
    IrcMessageData data;
    QList<IrcMessage*> batch;

    mutable QString m_nick, m_ident, m_host;
    mutable IrcExplicitValue<QString> m_prefix;
    mutable IrcExplicitValue<QString> m_command;
    mutable IrcExplicitValue<QStringList> m_params;
    mutable IrcExplicitValue<QVariantMap> m_tags;
};

IRC_END_NAMESPACE

#endif // IRCMESSAGE_P_H
