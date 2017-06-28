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

#ifndef IRCBUFFER_H
#define IRCBUFFER_H

#include <Irc>
#include <IrcGlobal>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qscopedpointer.h>

IRC_BEGIN_NAMESPACE

class IrcChannel;
class IrcCommand;
class IrcMessage;
class IrcNetwork;
class IrcConnection;
class IrcBufferModel;
class IrcBufferPrivate;

class IRC_MODEL_EXPORT IrcBuffer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
    Q_PROPERTY(IrcConnection* connection READ connection CONSTANT)
    Q_PROPERTY(IrcNetwork* network READ network CONSTANT)
    Q_PROPERTY(IrcBufferModel* model READ model CONSTANT)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool channel READ isChannel CONSTANT)
    Q_PROPERTY(bool sticky READ isSticky WRITE setSticky NOTIFY stickyChanged)
    Q_PROPERTY(bool persistent READ isPersistent WRITE setPersistent NOTIFY persistentChanged)
    Q_PROPERTY(QVariantMap userData READ userData WRITE setUserData NOTIFY userDataChanged)

public:
    Q_INVOKABLE explicit IrcBuffer(QObject* parent = 0);
    virtual ~IrcBuffer();

    QString title() const;
    QString name() const;
    QString prefix() const;

    bool isChannel() const;
    Q_INVOKABLE IrcChannel* toChannel();

    IrcConnection* connection() const;
    IrcNetwork* network() const;
    IrcBufferModel* model() const;

    virtual bool isActive() const;

    bool isSticky() const;
    void setSticky(bool sticky);

    bool isPersistent() const;
    void setPersistent(bool persistent);

    QVariantMap userData() const;
    void setUserData(const QVariantMap& data);

    Q_INVOKABLE bool sendCommand(IrcCommand* command);

public Q_SLOTS:
    void setName(const QString& name);
    void setPrefix(const QString& prefix);
    void receiveMessage(IrcMessage* message);
    virtual void close(const QString& reason = QString());

Q_SIGNALS:
    void titleChanged(const QString& title);
    void nameChanged(const QString& name);
    void prefixChanged(const QString& name);
    void messageReceived(IrcMessage* message);
    void destroyed(IrcBuffer* buffer);
    void activeChanged(bool active);
    void stickyChanged(bool sticky);
    void persistentChanged(bool persistent);
    void userDataChanged(const QVariantMap& data);

protected:
    IrcBuffer(IrcBufferPrivate& dd, QObject* parent);

    QScopedPointer<IrcBufferPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcBuffer)
    Q_DISABLE_COPY(IrcBuffer)
};

#ifndef QT_NO_DEBUG_STREAM
IRC_MODEL_EXPORT QDebug operator<<(QDebug debug, const IrcBuffer* buffer);
#endif // QT_NO_DEBUG_STREAM

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcBuffer*))
Q_DECLARE_METATYPE(QList<IRC_PREPEND_NAMESPACE(IrcBuffer*)>)

#endif // IRCBUFFER_H
