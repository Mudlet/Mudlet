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

#ifndef IRCPROTOCOL_H
#define IRCPROTOCOL_H

#include <IrcGlobal>
#include <IrcConnection>
#include <QtCore/qset.h>
#include <QtCore/qhash.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qabstractsocket.h>

IRC_BEGIN_NAMESPACE

class IrcMessage;
class IrcProtocolPrivate;

class IRC_CORE_EXPORT IrcProtocol : public QObject
{
    Q_OBJECT
    Q_PROPERTY(IrcConnection* connection READ connection)
    Q_PROPERTY(QAbstractSocket* socket READ socket)

public:
    explicit IrcProtocol(IrcConnection* connection);
    virtual ~IrcProtocol();

    IrcConnection* connection() const;
    QAbstractSocket* socket() const;

    virtual void open();
    virtual void close();

    virtual void read();
    virtual bool write(const QByteArray& data);

public Q_SLOTS:
    void receiveMessage(IrcMessage* message);

protected Q_SLOTS:
    void setNickName(const QString& name);
    void setStatus(IrcConnection::Status status);
    void setInfo(const QHash<QString, QString>& info);
    void setAvailableCapabilities(const QSet<QString>& capabilities);
    void setActiveCapabilities(const QSet<QString>& capabilities);

private:
    QScopedPointer<IrcProtocolPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcProtocol)
    Q_DISABLE_COPY(IrcProtocol)

    Q_PRIVATE_SLOT(d_func(), void _irc_pauseHandshake())
    Q_PRIVATE_SLOT(d_func(), void _irc_resumeHandshake())
};

IRC_END_NAMESPACE

#endif // IRCPROTOCOL_H
