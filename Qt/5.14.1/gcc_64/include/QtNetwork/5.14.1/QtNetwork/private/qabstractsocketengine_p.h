/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QABSTRACTSOCKETENGINE_P_H
#define QABSTRACTSOCKETENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtNetwork/qhostaddress.h"
#include "QtNetwork/qabstractsocket.h"
#include "private/qobject_p.h"
#include "private/qnetworkdatagram_p.h"

QT_BEGIN_NAMESPACE

class QAuthenticator;
class QAbstractSocketEnginePrivate;
#ifndef QT_NO_NETWORKINTERFACE
class QNetworkInterface;
#endif
class QNetworkProxy;

class QAbstractSocketEngineReceiver {
public:
    virtual ~QAbstractSocketEngineReceiver(){}
    virtual void readNotification()= 0;
    virtual void writeNotification()= 0;
    virtual void closeNotification()= 0;
    virtual void exceptionNotification()= 0;
    virtual void connectionNotification()= 0;
#ifndef QT_NO_NETWORKPROXY
    virtual void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)= 0;
#endif
};

class Q_AUTOTEST_EXPORT QAbstractSocketEngine : public QObject
{
    Q_OBJECT
public:

    static QAbstractSocketEngine *createSocketEngine(QAbstractSocket::SocketType socketType, const QNetworkProxy &, QObject *parent);
    static QAbstractSocketEngine *createSocketEngine(qintptr socketDescriptor, QObject *parent);

    QAbstractSocketEngine(QObject *parent = nullptr);

    enum SocketOption {
        NonBlockingSocketOption,
        BroadcastSocketOption,
        ReceiveBufferSocketOption,
        SendBufferSocketOption,
        AddressReusable,
        BindExclusively,
        ReceiveOutOfBandData,
        LowDelayOption,
        KeepAliveOption,
        MulticastTtlOption,
        MulticastLoopbackOption,
        TypeOfServiceOption,
        ReceivePacketInformation,
        ReceiveHopLimit,
        MaxStreamsSocketOption,
        PathMtuInformation
    };

    enum PacketHeaderOption {
        WantNone = 0,
        WantDatagramSender = 0x01,
        WantDatagramDestination = 0x02,
        WantDatagramHopLimit = 0x04,
        WantStreamNumber = 0x08,
        WantEndOfRecord = 0x10,

        WantAll = 0xff
    };
    Q_DECLARE_FLAGS(PacketHeaderOptions, PacketHeaderOption)

    virtual bool initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::IPv4Protocol) = 0;

    virtual bool initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState = QAbstractSocket::ConnectedState) = 0;

    virtual qintptr socketDescriptor() const = 0;

    virtual bool isValid() const = 0;

    virtual bool connectToHost(const QHostAddress &address, quint16 port) = 0;
    virtual bool connectToHostByName(const QString &name, quint16 port) = 0;
    virtual bool bind(const QHostAddress &address, quint16 port) = 0;
    virtual bool listen() = 0;
    virtual int accept() = 0;
    virtual void close() = 0;

    virtual qint64 bytesAvailable() const = 0;

    virtual qint64 read(char *data, qint64 maxlen) = 0;
    virtual qint64 write(const char *data, qint64 len) = 0;

#ifndef QT_NO_UDPSOCKET
#ifndef QT_NO_NETWORKINTERFACE
    virtual bool joinMulticastGroup(const QHostAddress &groupAddress,
                                    const QNetworkInterface &iface) = 0;
    virtual bool leaveMulticastGroup(const QHostAddress &groupAddress,
                                     const QNetworkInterface &iface) = 0;
    virtual QNetworkInterface multicastInterface() const = 0;
    virtual bool setMulticastInterface(const QNetworkInterface &iface) = 0;
#endif // QT_NO_NETWORKINTERFACE

    virtual bool hasPendingDatagrams() const = 0;
    virtual qint64 pendingDatagramSize() const = 0;
#endif // QT_NO_UDPSOCKET

    virtual qint64 readDatagram(char *data, qint64 maxlen, QIpPacketHeader *header = nullptr,
                                PacketHeaderOptions = WantNone) = 0;
    virtual qint64 writeDatagram(const char *data, qint64 len, const QIpPacketHeader &header) = 0;
    virtual qint64 bytesToWrite() const = 0;

    virtual int option(SocketOption option) const = 0;
    virtual bool setOption(SocketOption option, int value) = 0;

    virtual bool waitForRead(int msecs = 30000, bool *timedOut = nullptr) = 0;
    virtual bool waitForWrite(int msecs = 30000, bool *timedOut = nullptr) = 0;
    virtual bool waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                            bool checkRead, bool checkWrite,
                            int msecs = 30000, bool *timedOut = nullptr) = 0;

    QAbstractSocket::SocketError error() const;
    QString errorString() const;
    QAbstractSocket::SocketState state() const;
    QAbstractSocket::SocketType socketType() const;
    QAbstractSocket::NetworkLayerProtocol protocol() const;

    QHostAddress localAddress() const;
    quint16 localPort() const;
    QHostAddress peerAddress() const;
    quint16 peerPort() const;
    int inboundStreamCount() const;
    int outboundStreamCount() const;

    virtual bool isReadNotificationEnabled() const = 0;
    virtual void setReadNotificationEnabled(bool enable) = 0;
    virtual bool isWriteNotificationEnabled() const = 0;
    virtual void setWriteNotificationEnabled(bool enable) = 0;
    virtual bool isExceptionNotificationEnabled() const = 0;
    virtual void setExceptionNotificationEnabled(bool enable) = 0;

public Q_SLOTS:
    void readNotification();
    void writeNotification();
    void closeNotification();
    void exceptionNotification();
    void connectionNotification();
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#endif

public:
    void setReceiver(QAbstractSocketEngineReceiver *receiver);
protected:
    QAbstractSocketEngine(QAbstractSocketEnginePrivate &dd, QObject* parent = nullptr);

    void setError(QAbstractSocket::SocketError error, const QString &errorString) const;
    void setState(QAbstractSocket::SocketState state);
    void setSocketType(QAbstractSocket::SocketType socketType);
    void setProtocol(QAbstractSocket::NetworkLayerProtocol protocol);
    void setLocalAddress(const QHostAddress &address);
    void setLocalPort(quint16 port);
    void setPeerAddress(const QHostAddress &address);
    void setPeerPort(quint16 port);

private:
    Q_DECLARE_PRIVATE(QAbstractSocketEngine)
    Q_DISABLE_COPY_MOVE(QAbstractSocketEngine)
};

class QAbstractSocketEnginePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractSocketEngine)
public:
    QAbstractSocketEnginePrivate();

    mutable QAbstractSocket::SocketError socketError;
    mutable bool hasSetSocketError;
    mutable QString socketErrorString;
    QAbstractSocket::SocketState socketState;
    QAbstractSocket::SocketType socketType;
    QAbstractSocket::NetworkLayerProtocol socketProtocol;
    QHostAddress localAddress;
    quint16 localPort;
    QHostAddress peerAddress;
    quint16 peerPort;
    int inboundStreamCount;
    int outboundStreamCount;
    QAbstractSocketEngineReceiver *receiver;
};


class Q_AUTOTEST_EXPORT QSocketEngineHandler
{
protected:
    QSocketEngineHandler();
    virtual ~QSocketEngineHandler();
    virtual QAbstractSocketEngine *createSocketEngine(QAbstractSocket::SocketType socketType,
                                                      const QNetworkProxy &, QObject *parent) = 0;
    virtual QAbstractSocketEngine *createSocketEngine(qintptr socketDescriptor, QObject *parent) = 0;

private:
    friend class QAbstractSocketEngine;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractSocketEngine::PacketHeaderOptions)

QT_END_NAMESPACE

#endif // QABSTRACTSOCKETENGINE_P_H
