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

#ifndef IRCCONNECTION_P_H
#define IRCCONNECTION_P_H

#include "ircconnection.h"

#include <QSet>
#include <QList>
#include <QHash>
#include <QStack>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QAbstractSocket>

IRC_BEGIN_NAMESPACE

class IrcMessageFilter;
class IrcCommandFilter;

class IrcConnectionPrivate
{
    Q_DECLARE_PUBLIC(IrcConnection)

public:
    IrcConnectionPrivate();

    void init(IrcConnection* connection);

    void _irc_connected();
    void _irc_disconnected();
    void _irc_error(QAbstractSocket::SocketError error);
    void _irc_state(QAbstractSocket::SocketState state);
    void _irc_sslErrors();
    void _irc_reconnect();
    void _irc_readData();

    void _irc_filterDestroyed(QObject* filter);

    void open();
    void reconnect();
    void setNick(const QString& nick);
    void setStatus(IrcConnection::Status status);
    void setInfo(const QHash<QString, QString>& info);

    bool receiveMessage(IrcMessage* msg);
    IrcCommand* createCtcpReply(IrcPrivateMessage* request);

    static IrcConnectionPrivate* get(const IrcConnection* connection)
    {
        return connection->d_ptr.data();
    }

    IrcConnection* q_ptr;
    QByteArray encoding;
    IrcNetwork* network;
    IrcProtocol* protocol;
    QAbstractSocket* socket;
    QString host;
    int port;
    int currentServer;
    QStringList servers;
    QString userName;
    QString nickName;
    QString realName;
    QString password;
    QStringList nickNames;
    QString displayName;
    QVariantMap userData;
    QTimer reconnecter;
    QString saslMechanism;
    QVariantMap ctcpReplies;
    bool enabled;
    IrcConnection::Status status;
    QList<QByteArray> pendingData;
    QList<QObject*> commandFilters;
    QList<QObject*> messageFilters;
    QStack<QObject*> activeCommandFilters;
    QSet<int> replies;
    bool pendingOpen;
    bool closed;
};

IRC_END_NAMESPACE

#endif // IRCCONNECTION_P_H
