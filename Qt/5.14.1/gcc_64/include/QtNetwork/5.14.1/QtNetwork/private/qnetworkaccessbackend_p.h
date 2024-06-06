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

#ifndef QNETWORKACCESSBACKEND_P_H
#define QNETWORKACCESSBACKEND_P_H

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
#include "qnetworkreplyimpl_p.h"
#include "QtCore/qobject.h"

QT_BEGIN_NAMESPACE

class QAuthenticator;
class QNetworkProxy;
class QNetworkProxyQuery;
class QNetworkRequest;
class QStringList;
class QUrl;
class QSslConfiguration;

class QNetworkAccessManagerPrivate;
class QNetworkReplyImplPrivate;
class QAbstractNetworkCache;
class QNetworkCacheMetaData;
class QNonContiguousByteDevice;

// Should support direct file upload from disk or download to disk.
//
// - The HTTP handler will use two QIODevices for communication (pull mechanism)
// - KIO uses a pull mechanism too (data/dataReq signals)
class QNetworkAccessBackend : public QObject
{
    Q_OBJECT
public:
    QNetworkAccessBackend();
    virtual ~QNetworkAccessBackend();

    // To avoid mistaking with QIODevice names, the functions here
    // have different names. The Connection has two streams:
    //
    // - Upstream:
    //   The upstream uses a QNonContiguousByteDevice provided
    //   by the backend. This device emits the usual readyRead()
    //   signal when the backend has data available for the connection
    //   to write. The different backends can listen on this signal
    //   and then pull upload data from the QNonContiguousByteDevice and
    //   deal with it.
    //
    //
    // - Downstream:
    //   Downstream is the data that is being read from this
    //   connection and is given to the user. Downstream operates in a
    //   semi-"push" mechanism: the Connection object pushes the data
    //   it gets from the network, but it should avoid writing too
    //   much if the data isn't being used fast enough. The value
    //   returned by suggestedDownstreamBlockSize() can be used to
    //   determine how much should be written at a time. The
    //   downstreamBytesConsumed() function will be called when the
    //   downstream buffer is consumed by the user -- the Connection
    //   may choose to re-fill it with more data it has ready or get
    //   more data from the network (for instance, by reading from its
    //   socket).

    virtual void open() = 0;
    virtual bool start();
    virtual void closeDownstreamChannel() = 0;

    // slot-like:
    virtual void downstreamReadyWrite();
    virtual void setDownstreamLimited(bool b);
    virtual void copyFinished(QIODevice *);
    virtual void ignoreSslErrors();
    virtual void ignoreSslErrors(const QList<QSslError> &errors);

    virtual void fetchSslConfiguration(QSslConfiguration &configuration) const;
    virtual void setSslConfiguration(const QSslConfiguration &configuration);

    virtual QNetworkCacheMetaData fetchCacheMetaData(const QNetworkCacheMetaData &metaData) const;

    // information about the request
    QNetworkAccessManager::Operation operation() const;
    QNetworkRequest request() const;
#ifndef QT_NO_NETWORKPROXY
    QList<QNetworkProxy> proxyList() const;
#endif

    QAbstractNetworkCache *networkCache() const;
    void setCachingEnabled(bool enable);
    bool isCachingEnabled() const;

    // information about the reply
    QUrl url() const;
    void setUrl(const QUrl &url);

    // "cooked" headers
    QVariant header(QNetworkRequest::KnownHeaders header) const;
    void setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);

    // raw headers:
    bool hasRawHeader(const QByteArray &headerName) const;
    QList<QByteArray> rawHeaderList() const;
    QByteArray rawHeader(const QByteArray &headerName) const;
    void setRawHeader(const QByteArray &headerName, const QByteArray &value);

    // attributes:
    QVariant attribute(QNetworkRequest::Attribute code) const;
    void setAttribute(QNetworkRequest::Attribute code, const QVariant &value);

    bool isSynchronous() { return synchronous; }
    void setSynchronous(bool sync) { synchronous = sync; }

    // return true if the QNonContiguousByteDevice of the upload
    // data needs to support reset(). Currently needed for HTTP.
    // This will possibly enable buffering of the upload data.
    virtual bool needsResetableUploadData() { return false; }

    // Returns \c true if backend is able to resume downloads.
    virtual bool canResume() const { return false; }
    virtual void setResumeOffset(quint64 offset) { Q_UNUSED(offset); }

    virtual bool processRequestSynchronously() { return false; }

protected:
    // Create the device used for reading the upload data
    QNonContiguousByteDevice* createUploadByteDevice();

    // these functions control the downstream mechanism
    // that is, data that has come via the connection and is going out the backend
    qint64 nextDownstreamBlockSize() const;
    void writeDownstreamData(QByteDataBuffer &list);

    // not actually appending data, it was already written to the user buffer
    void writeDownstreamDataDownloadBuffer(qint64, qint64);
    char* getDownloadBuffer(qint64);

    QSharedPointer<QNonContiguousByteDevice> uploadByteDevice;

public slots:
    // for task 251801, needs to be a slot to be called asynchronously
    void writeDownstreamData(QIODevice *data);

protected slots:
    void finished();
    void error(QNetworkReply::NetworkError code, const QString &errorString);
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth);
#endif
    void authenticationRequired(QAuthenticator *auth);
    void metaDataChanged();
    void redirectionRequested(const QUrl &destination);
    void encrypted();
    void sslErrors(const QList<QSslError> &errors);
    void emitReplyUploadProgress(qint64 bytesSent, qint64 bytesTotal);

protected:
    // FIXME In the long run we should get rid of our QNAM architecture
    // and scrap this ReplyImpl/Backend distinction.
    QNetworkAccessManagerPrivate *manager;
    QNetworkReplyImplPrivate *reply;

private:
    friend class QNetworkAccessManager;
    friend class QNetworkAccessManagerPrivate;
    friend class QNetworkReplyImplPrivate;

    bool synchronous;
};

class QNetworkAccessBackendFactory
{
public:
    QNetworkAccessBackendFactory();
    virtual ~QNetworkAccessBackendFactory();
    virtual QStringList supportedSchemes() const = 0;
    virtual QNetworkAccessBackend *create(QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request) const = 0;
};

QT_END_NAMESPACE

#endif

