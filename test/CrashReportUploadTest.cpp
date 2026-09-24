/***************************************************************************
 *   Copyright (C) 2026 by missionz3r0 - github_xu8w@missionz3r0.com       *
 *                                                                         *
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

#include <QtNetwork/QNetworkProxyFactory>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include "crashReporter.h"

static const QByteArray sentrySerializedEnvelope = QByteArrayLiteral("{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\"}\n"
                                                                     "{\"type\":\"event\",\"length\":71}\n"
                                                                     "{\"event_id\":\"c993afb6-b4ac-48a6-b61b-2558e601d65d\",\"some-context\":1234}\n"
                                                                     "{\"type\":\"minidump\",\"length\":11}\n"
                                                                     "MDMP\x93\xa7\x00\x00\r\n\xff"
                                                                     "\n"
                                                                     "{\"type\":\"attachment\",\"length\":12}\n"
                                                                     "Hello World!");

struct ReceivedRequest
{
    QByteArray method;
    QByteArray target;
    QHash<QByteArray, QByteArray> headers;
    QByteArray body;
};

class FakeSentryServer : public QObject
{
    Q_OBJECT

public:
    explicit FakeSentryServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &FakeSentryServer::slot_accept);
        mListening = mServer.listen(QHostAddress::LocalHost, 0);
    }

    bool listening() const { return mListening; }

    quint16 port() const { return mServer.serverPort(); }

    int connectionCount() const { return mConnectionCount; }

    const QList<ReceivedRequest>& requests() const { return mRequests; }

private slots:
    void slot_accept()
    {
        while (QTcpSocket* client = mServer.nextPendingConnection()) {
            ++mConnectionCount;
            connect(client, &QTcpSocket::readyRead, this, [this, client]() {
                readFrom(client);
            });
            connect(client, &QTcpSocket::disconnected, client, &QObject::deleteLater);
        }
    }

private:
    void readFrom(QTcpSocket* client)
    {
        QByteArray& buffer = mBuffers[client];
        buffer += client->readAll();
        const qsizetype headersEnd = buffer.indexOf("\r\n\r\n");
        if (headersEnd == -1) {
            return;
        }

        ReceivedRequest request;
        const QList<QByteArray> lines = buffer.left(headersEnd).split('\n');
        const QList<QByteArray> requestLine = lines.constFirst().trimmed().split(' ');
        request.method = requestLine.value(0);
        request.target = requestLine.value(1);
        for (const QByteArray& line : lines.mid(1)) {
            const qsizetype colon = line.indexOf(':');
            if (colon != -1) {
                request.headers.insert(line.left(colon).trimmed().toLower(), line.mid(colon + 1).trimmed());
            }
        }

        const qsizetype bodyStart = headersEnd + 4;
        const qsizetype bodyLength = request.headers.value("content-length").toLongLong();
        if (buffer.size() - bodyStart < bodyLength) {
            return;
        }
        request.body = buffer.mid(bodyStart, bodyLength);
        mBuffers.remove(client);
        mRequests << request;

        client->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}");
        client->disconnectFromHost();
    }

    QTcpServer mServer;
    QHash<QTcpSocket*, QByteArray> mBuffers;
    QList<ReceivedRequest> mRequests;
    int mConnectionCount = 0;
    bool mListening = false;
};

class CrashReportUploadTest : public QObject
{
    Q_OBJECT

private:
    FakeSentryServer* mpServer = nullptr;
    QTemporaryDir mEnvelopeDir;

    QString dsnFor(QString dsnTemplate) const { return dsnTemplate.replace(QStringLiteral("{port}"), QString::number(mpServer->port())); }

    QString writeEnvelope(const QByteArray& contents) const
    {
        const QString path = mEnvelopeDir.filePath(QStringLiteral("crash.envelope"));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(contents) != contents.size()) {
            return QString();
        }
        return path;
    }

    static QHash<QByteArray, QByteArray> authFields(const QByteArray& header)
    {
        QHash<QByteArray, QByteArray> fields;
        if (!header.startsWith("Sentry ")) {
            return fields;
        }
        for (const QByteArray& field : header.mid(7).split(',')) {
            const qsizetype equals = field.indexOf('=');
            if (equals != -1) {
                fields.insert(field.left(equals).trimmed(), field.mid(equals + 1).trimmed());
            }
        }
        return fields;
    }

private slots:
    void initTestCase() { QNetworkProxyFactory::setUseSystemConfiguration(false); }

    void init()
    {
        QVERIFY(mEnvelopeDir.isValid());
        mpServer = new FakeSentryServer(this);
        QVERIFY2(mpServer->listening(), "could not listen on the loopback interface");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
    }

    void postsTheEnvelopeWhereSentryDid_data()
    {
        QTest::addColumn<QString>("dsnTemplate");
        QTest::addColumn<QByteArray>("expectedTarget");
        QTest::addColumn<QByteArray>("expectedKey");

        QTest::newRow("dsn_auth_header_no_user_agent") << QStringLiteral("http://key@127.0.0.1:{port}/42") << QByteArray("/api/42/envelope/") << QByteArray("key");
        QTest::newRow("dsn_store_url_without_path") << QStringLiteral("http://username:password@127.0.0.1:{port}/42?x=y#z") << QByteArray("/api/42/envelope/") << QByteArray("username");
        QTest::newRow("dsn_store_url_with_path") << QStringLiteral("http://username:password@127.0.0.1:{port}/foo/bar/42?x=y#z") << QByteArray("/foo/bar/api/42/envelope/") << QByteArray("username");
        QTest::newRow("dsn_with_ending_forward_slash_will_be_cleaned") << QStringLiteral("http://foo@127.0.0.1:{port}/42/43/44////") << QByteArray("/42/43/api/44/envelope/") << QByteArray("foo");
        QTest::newRow("dsn_parsing_complete") << QStringLiteral("http://username:password@127.0.0.1:{port}/foo/bar/42%21?x=y#z") << QByteArray("/foo/bar/api/42%21/envelope/")
                                              << QByteArray("username");
    }

    void postsTheEnvelopeWhereSentryDid()
    {
        QFETCH(QString, dsnTemplate);
        QFETCH(QByteArray, expectedTarget);
        QFETCH(QByteArray, expectedKey);
        const QString envelopePath = writeEnvelope(sentrySerializedEnvelope);
        QVERIFY(!envelopePath.isEmpty());

        sendCrashReport(envelopePath, dsnFor(dsnTemplate));

        QCOMPARE(mpServer->requests().size(), 1);
        const ReceivedRequest& request = mpServer->requests().constFirst();
        QCOMPARE(request.method, QByteArray("POST"));
        QCOMPARE(request.target, expectedTarget);
        QCOMPARE(request.headers.value("content-type"), QByteArray("application/x-sentry-envelope"));
        QVERIFY(!request.headers.contains("content-encoding"));
        const QHash<QByteArray, QByteArray> auth = authFields(request.headers.value("x-sentry-auth"));
        QCOMPARE(auth.value("sentry_key"), expectedKey);
        QCOMPARE(auth.value("sentry_version"), QByteArray("7"));
        QVERIFY(!auth.value("sentry_client").isEmpty());
        QVERIFY(!auth.contains("sentry_secret"));
        QCOMPARE(request.body, sentrySerializedEnvelope);
    }

    void sendsNothingForADsnSentryRejected_data()
    {
        QTest::addColumn<QString>("dsnTemplate");

        QTest::newRow("no dsn") << QString();
        QTest::newRow("dsn_auth_header_invalid_dsn") << QStringLiteral("whatever");
        QTest::newRow("dsn_parsing_invalid (no public key)") << QStringLiteral("http://127.0.0.1:{port}/1234567");
        QTest::newRow("dsn_parsing_invalid (no path)") << QStringLiteral("http://key@127.0.0.1:{port}");
        QTest::newRow("dsn_without_project_id_is_invalid") << QStringLiteral("http://foo@127.0.0.1:{port}/");
        QTest::newRow("dsn_with_non_http_scheme_is_invalid") << QStringLiteral("ftp://key@127.0.0.1:{port}/42");
        QTest::newRow("dsn_without_url_scheme_is_invalid") << QStringLiteral("//key@127.0.0.1:{port}/42");
    }

    void sendsNothingForADsnSentryRejected()
    {
        QFETCH(QString, dsnTemplate);
        const QString envelopePath = writeEnvelope(sentrySerializedEnvelope);
        QVERIFY(!envelopePath.isEmpty());

        sendCrashReport(envelopePath, dsnFor(dsnTemplate));
        QCoreApplication::processEvents();

        QCOMPARE(mpServer->connectionCount(), 0);
    }

    void sendsNothingForAnEnvelopeSentryCouldNotSend_data()
    {
        QTest::addColumn<QByteArray>("contents");

        QTest::newRow("deserialize_envelope_invalid (empty)") << QByteArray();
        QTest::newRow("deserialize_envelope_invalid (bare newline)") << QByteArray("\n");
        QTest::newRow("deserialize_envelope_invalid (not json)") << QByteArray("invalid");
        QTest::newRow("deserialize_envelope_invalid (item header not json)") << QByteArray("{}\ninvalid\n");
        QTest::newRow("deserialize_envelope_invalid (item header with no payload line)") << QByteArray("{}\n{}");
        QTest::newRow("deserialize_envelope_invalid (negative length)") << QByteArray("{}\n{\"length\":-1}\n");
        QTest::newRow("deserialize_envelope_empty (no items)") << QByteArray("{}\n");
    }

    void sendsNothingForAnEnvelopeSentryCouldNotSend()
    {
        QFETCH(QByteArray, contents);
        const QString envelopePath = writeEnvelope(contents);
        QVERIFY(!envelopePath.isEmpty());

        sendCrashReport(envelopePath, dsnFor(QStringLiteral("http://key@127.0.0.1:{port}/42")));
        QCoreApplication::processEvents();

        QCOMPARE(mpServer->connectionCount(), 0);
    }

    void sendsNothingWithoutAnEnvelopeFile()
    {
        const QString missingPath = mEnvelopeDir.filePath(QStringLiteral("missing.envelope"));

        sendCrashReport(missingPath, dsnFor(QStringLiteral("http://key@127.0.0.1:{port}/42")));
        QCoreApplication::processEvents();

        QCOMPARE(mpServer->connectionCount(), 0);
    }
};

QTEST_GUILESS_MAIN(CrashReportUploadTest)

#include "CrashReportUploadTest.moc"
