/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QHTTPNETWORKCONNECTIONCHANNEL_H
#define QHTTPNETWORKCONNECTIONCHANNEL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qabstractsocket.h>

#include <private/qobject_p.h>
#include <qauthenticator.h>
#include <qnetworkproxy.h>
#include <qbuffer.h>

#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qhttpnetworkreply_p.h>

#include <private/qhttpnetworkconnection_p.h>
#include <private/qabstractprotocolhandler_p.h>

#ifndef QT_NO_SSL
#    include <QtNetwork/qsslsocket.h>
#    include <QtNetwork/qsslerror.h>
#    include <QtNetwork/qsslconfiguration.h>
#else
#   include <QtNetwork/qtcpsocket.h>
#endif

#include <QtCore/qscopedpointer.h>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttpNetworkRequest;
class QHttpNetworkReply;
class QByteArray;

#ifndef HttpMessagePair
typedef QPair<QHttpNetworkRequest, QHttpNetworkReply*> HttpMessagePair;
#endif

class QHttpNetworkConnectionChannel : public QObject {
    Q_OBJECT
public:
    // TODO: Refactor this to add an EncryptingState (and remove pendingEncrypt).
    // Also add an Unconnected state so IdleState does not have double meaning.
    enum ChannelState {
        IdleState = 0,          // ready to send request
        ConnectingState = 1,    // connecting to host
        WritingState = 2,       // writing the data
        WaitingState = 4,       // waiting for reply
        ReadingState = 8,       // reading the reply
        ClosingState = 16,
        BusyState = (ConnectingState|WritingState|WaitingState|ReadingState|ClosingState)
    };
    QAbstractSocket *socket;
    bool ssl;
    bool isInitialized;
    ChannelState state;
    QHttpNetworkRequest request; // current request, only used for HTTP
    QHttpNetworkReply *reply; // current reply for this request, only used for HTTP
    qint64 written;
    qint64 bytesTotal;
    bool resendCurrent;
    int lastStatus; // last status received on this channel
    bool pendingEncrypt; // for https (send after encrypted)
    int reconnectAttempts; // maximum 2 reconnection attempts
    QAuthenticatorPrivate::Method authMethod;
    QAuthenticatorPrivate::Method proxyAuthMethod;
    QAuthenticator authenticator;
    QAuthenticator proxyAuthenticator;
    bool authenticationCredentialsSent;
    bool proxyCredentialsSent;
    QScopedPointer<QAbstractProtocolHandler> protocolHandler;
    // SPDY or HTTP/2 requests; SPDY is TLS-only, but
    // HTTP/2 can be cleartext also, that's why it's
    // outside of QT_NO_SSL section. Sorted by priority:
    QMultiMap<int, HttpMessagePair> spdyRequestsToSend;
    bool switchedToHttp2 = false;
#ifndef QT_NO_SSL
    bool ignoreAllSslErrors;
    QList<QSslError> ignoreSslErrorsList;
    QScopedPointer<QSslConfiguration> sslConfiguration;
    void ignoreSslErrors();
    void ignoreSslErrors(const QList<QSslError> &errors);
    void setSslConfiguration(const QSslConfiguration &config);
    void requeueSpdyRequests(); // when we wanted SPDY but got HTTP
#endif
    // to emit the signal for all in-flight replies:
    void emitFinishedWithError(QNetworkReply::NetworkError error, const char *message);
#ifndef QT_NO_BEARERMANAGEMENT
    QSharedPointer<QNetworkSession> networkSession;
#endif

    // HTTP pipelining -> http://en.wikipedia.org/wiki/Http_pipelining
    enum PipeliningSupport {
        PipeliningSupportUnknown, // default for a new connection
        PipeliningProbablySupported, // after having received a server response that indicates support
        PipeliningNotSupported // currently not used
    };
    PipeliningSupport pipeliningSupported;
    QList<HttpMessagePair> alreadyPipelinedRequests;
    QByteArray pipeline; // temporary buffer that gets sent to socket in pipelineFlush
    void pipelineInto(HttpMessagePair &pair);
    void pipelineFlush();
    void requeueCurrentlyPipelinedRequests();
    void detectPipeliningSupport();

    QHttpNetworkConnectionChannel();

    QAbstractSocket::NetworkLayerProtocol networkLayerPreference;

    void setConnection(QHttpNetworkConnection *c);
    QPointer<QHttpNetworkConnection> connection;

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    void setProxy(const QNetworkProxy &networkProxy);
#endif

    void init();
    void close();
    void abort();

    bool sendRequest();
    void sendRequestDelayed();

    bool ensureConnection();

    void allDone(); // reply header + body have been read
    void handleStatus(); // called from allDone()

    bool resetUploadData(); // return true if resetting worked or there is no upload data

    void handleUnexpectedEOF();
    void closeAndResendCurrentRequest();
    void resendCurrentRequest();

    bool isSocketBusy() const;
    bool isSocketWriting() const;
    bool isSocketWaiting() const;
    bool isSocketReading() const;

    protected slots:
    void _q_receiveReply();
    void _q_bytesWritten(qint64 bytes); // proceed sending
    void _q_readyRead(); // pending data to read
    void _q_disconnected(); // disconnected from host
    void _q_connected(); // start sending request
    void _q_error(QAbstractSocket::SocketError); // error from socket
#ifndef QT_NO_NETWORKPROXY
    void _q_proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth); // from transparent proxy
#endif

    void _q_uploadDataReadyRead();

#ifndef QT_NO_SSL
    void _q_encrypted(); // start sending request (https)
    void _q_sslErrors(const QList<QSslError> &errors); // ssl errors from the socket
    void _q_preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*); // tls-psk auth necessary
    void _q_encryptedBytesWritten(qint64 bytes); // proceed sending
#endif

    friend class QHttpProtocolHandler;
};

QT_END_NAMESPACE

#endif
