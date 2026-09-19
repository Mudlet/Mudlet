/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include "crashReporter.h"

static bool sentryWouldSend(const QByteArray& envelope)
{
    const auto lineEnd = [&envelope](qsizetype from) {
        const qsizetype newline = envelope.indexOf('\n', from);
        return newline < 0 ? envelope.size() : newline;
    };

    qsizetype position = lineEnd(0);
    if (!QJsonDocument::fromJson(envelope.first(position)).isObject()) {
        return false;
    }
    ++position;

    int items = 0;
    while (position < envelope.size()) {
        const qsizetype headerEnd = lineEnd(position);
        const QJsonDocument header = QJsonDocument::fromJson(envelope.sliced(position, headerEnd - position));
        if (!header.isObject() || headerEnd == envelope.size()) {
            return false;
        }
        position = headerEnd + 1;

        const QJsonValue length = header.object().value(QLatin1String("length"));
        if (length.isUndefined() || length.isNull()) {
            position = lineEnd(position);
        } else {
            const qint64 payloadLength = length.toInteger(-1);
            if (payloadLength < 0 || payloadLength > envelope.size() - position) {
                return false;
            }
            position += payloadLength;
        }
        while (position < envelope.size() && envelope.at(position) == '\n') {
            ++position;
        }
        ++items;
    }
    return items > 0;
}

void sendCrashReport(const char* envelopePath, const QString& dsn)
{
    // A DSN is <scheme>://<public key>@<host>/<optional path>/<project id>, and
    // the envelope endpoint is that host and path with /api/<project id>/envelope/
    // appended: https://develop.sentry.dev/sdk/foundations/transport/authentication/#parsing-the-dsn
    const QUrl parsedDsn(dsn);
    if (!parsedDsn.isValid() || parsedDsn.userName().isEmpty() || parsedDsn.host().isEmpty()) {
        return;
    }
    QStringList segments = parsedDsn.path(QUrl::FullyEncoded).split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (segments.isEmpty()) {
        return;
    }
    const QString projectId = segments.takeLast();
    const QString pathPrefix = segments.isEmpty() ? QString() : QLatin1Char('/') + segments.join(QLatin1Char('/'));

    QUrl endpoint;
    endpoint.setScheme(parsedDsn.scheme());
    endpoint.setHost(parsedDsn.host());
    if (parsedDsn.port() != -1) {
        endpoint.setPort(parsedDsn.port());
    }
    endpoint.setPath(QStringLiteral("%1/api/%2/envelope/").arg(pathPrefix, projectId), QUrl::TolerantMode);

    QFile envelope(QString::fromUtf8(envelopePath));
    if (!envelope.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray payload = envelope.readAll();
    envelope.close();
    if (!sentryWouldSend(payload)) {
        return;
    }

    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-sentry-envelope");
    // Store endpoint auth: sentry_key is the DSN's public key, and sentry_version
    // has been 7 since the DSN format settled.
    // https://develop.sentry.dev/sdk/foundations/transport/authentication/
    request.setRawHeader("X-Sentry-Auth", QStringLiteral("Sentry sentry_version=7, sentry_client=mudlet-crash-reporter/1.0, sentry_key=%1").arg(parsedDsn.userName()).toUtf8());

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // The reply is parented to the manager, so it goes with it at end of scope.
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply* reply = manager.post(request, payload);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(30 * 1000);

    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        qWarning() << "Crash report upload timed out.";
    } else if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Crash report upload failed:" << reply->errorString();
    }
}
