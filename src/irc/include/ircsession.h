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

#ifndef IRCSESSION_H
#define IRCSESSION_H

#include <IrcGlobal>
#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qabstractsocket.h>

class IrcCommand;
class IrcMessage;
class IrcPrivateMessage;
class IrcSessionPrivate;

class COMMUNI_EXPORT IrcSession : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString nickName READ nickName WRITE setNickName NOTIFY nickNameChanged)
    Q_PROPERTY(QString realName READ realName WRITE setRealName NOTIFY realNameChanged)
    Q_PROPERTY(QByteArray encoding READ encoding WRITE setEncoding)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QAbstractSocket* socket READ socket WRITE setSocket)

public:
    explicit IrcSession(QObject* parent = 0);
    virtual ~IrcSession();

    QString host() const;
    void setHost(const QString& host);

    int port() const;
    void setPort(int port);

    QString userName() const;
    void setUserName(const QString& name);

    QString nickName() const;
    void setNickName(const QString& name);

    QString realName() const;
    void setRealName(const QString& name);

    QByteArray encoding() const;
    void setEncoding(const QByteArray& encoding);

    bool isActive() const;
    bool isConnected() const;

    QAbstractSocket* socket() const;
    void setSocket(QAbstractSocket* socket);

    Q_INVOKABLE bool sendCommand(IrcCommand* command);
    Q_INVOKABLE bool sendData(const QByteArray& data);
    Q_INVOKABLE bool sendRaw(const QString& message);

public Q_SLOTS:
    void open();
    void close();

Q_SIGNALS:
    void connecting();
    void password(QString* password);
    void capabilities(const QStringList& available, QStringList* request);
    void connected();
    void disconnected();
    void socketError(QAbstractSocket::SocketError error);
    void socketStateChanged(QAbstractSocket::SocketState state);

    void messageReceived(IrcMessage* message);

    void hostChanged(const QString& host);
    void portChanged(int port);
    void userNameChanged(const QString& name);
    void nickNameChanged(const QString& name);
    void realNameChanged(const QString& name);

    void activeChanged(bool active);
    void connectedChanged(bool connected);

protected:
    virtual IrcCommand* createCtcpReply(IrcPrivateMessage* request) const;

private:
    QScopedPointer<IrcSessionPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcSession)
    Q_DISABLE_COPY(IrcSession)
    friend class IrcMessage;
    friend class tst_IrcMessage;

    Q_PRIVATE_SLOT(d_func(), void _q_connected())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnected())
    Q_PRIVATE_SLOT(d_func(), void _q_reconnect())
    Q_PRIVATE_SLOT(d_func(), void _q_error(QAbstractSocket::SocketError))
    Q_PRIVATE_SLOT(d_func(), void _q_state(QAbstractSocket::SocketState))
    Q_PRIVATE_SLOT(d_func(), void _q_readData())
};

#ifndef QT_NO_DEBUG_STREAM
COMMUNI_EXPORT QDebug operator<<(QDebug debug, const IrcSession* session);
#endif // QT_NO_DEBUG_STREAM

#endif // IRCSESSION_H
