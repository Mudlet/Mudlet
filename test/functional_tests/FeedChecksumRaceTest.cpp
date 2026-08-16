/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@mudlet.org         *
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

#include "updater/Feed.h"
#include "updater/Release.h"
#include "utils.h"

#include <QtTest/QtTest>

#include <QHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

/*
 * Regression test for https://github.com/Mudlet/Mudlet/issues/9938 (Sentry
 * MUDLET-4M): EXCEPTION_ACCESS_VIOLATION_READ at address 0x8 inside
 * Feed::makeDownloadRequest(), reached from the lambda Feed::fetchChecksums()
 * connects to the checksum reply.
 *
 * Feed::isDownloading() only looked at the download reply, which does not
 * exist yet while the checksums are being fetched - so a second
 * downloadRelease() during that window sailed past the guard and started a
 * second checksum fetch. The update dialog's automatic download and the
 * twice-daily update check both answer the same Feed::ready(), so the second
 * call is an ordinary consequence of leaving automatic updates on.
 *
 * When the second checksum fetch finished, makeDownloadRequest() found the
 * first download already in flight and aborted it. abort() delivers finished()
 * synchronously, so handleDownloadFinished() ran nested inside it and its
 * cleanupDownloadReply() deleted the reply and set mDownloadReply to nullptr -
 * the disconnect() meant to prevent that used the wrong overload and unhooked
 * nothing. makeDownloadRequest() then went on to call
 * mDownloadReply->deleteLater() on that null pointer, and QObject::deleteLater()
 * reads d_ptr at offset 8 of `this`: a read of address 0x8.
 *
 * The stub server answers the second checksum request late on purpose, so the
 * first download is guaranteed to be in flight by the time the second lambda
 * runs. Without the fix this test does not fail an assertion, it segfaults.
 */

namespace {
const auto assetName = qsl("Mudlet-9.9.9-linux-x64.AppImage.tar");
const auto checksumsName = qsl("SHA256SUMS.txt");
} // namespace

// A minimum-viable HTTP server: it serves the checksum file, and answers the
// download request with a Content-Length it never satisfies, so that download
// stays unfinished for the whole test
class StubFeedServer : public QObject
{
    Q_OBJECT

public:
    explicit StubFeedServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &StubFeedServer::acceptConnections);
    }

    bool listen() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 port() const { return mServer.serverPort(); }

    int checksumRequests() const { return mChecksumRequests; }
    int downloadRequests() const { return mDownloadRequests; }

private:
    void acceptConnections()
    {
        while (auto* socket = mServer.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                readRequest(socket);
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

    void readRequest(QTcpSocket* socket)
    {
        QByteArray& buffer = mBuffers[socket];
        buffer.append(socket->readAll());
        const int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }
        const QByteArray requestLine = buffer.left(buffer.indexOf("\r\n"));
        mBuffers.remove(socket);

        if (requestLine.contains(checksumsName.toLatin1())) {
            ++mChecksumRequests;
            // The first answer is immediate so its download is running before
            // any second one lands; a later one is held back to guarantee that
            const int delayMs = mChecksumRequests == 1 ? 0 : 300;
            QTimer::singleShot(delayMs, socket, [socket]() {
                sendChecksums(socket);
            });
            return;
        }

        if (requestLine.contains(assetName.toLatin1())) {
            ++mDownloadRequests;
            sendUnfinishedDownload(socket);
            return;
        }

        socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
    }

    static void sendChecksums(QTcpSocket* socket)
    {
        const QByteArray body = QByteArray(64, 'a') + "  " + assetName.toLatin1() + "\n";
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n");
        socket->write(body);
        socket->flush();
        socket->disconnectFromHost();
    }

    static void sendUnfinishedDownload(QTcpSocket* socket)
    {
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 4194304\r\nConnection: close\r\n\r\n");
        socket->write(QByteArray(16, 'x'));
        socket->flush();
    }

    QTcpServer mServer;
    QHash<QTcpSocket*, QByteArray> mBuffers;
    int mChecksumRequests{0};
    int mDownloadRequests{0};
};

class FeedChecksumRaceTest : public QObject
{
    Q_OBJECT

private slots:
    void secondDownloadRequestDuringChecksumFetchDoesNotCrash();
};

void FeedChecksumRaceTest::secondDownloadRequestDuringChecksumFetchDoesNotCrash()
{
    StubFeedServer server;
    QVERIFY2(server.listen(), "the stub update server could not listen on localhost");
    const QString base = qsl("http://127.0.0.1:%1/").arg(server.port());

    QJsonObject checksumsAsset;
    checksumsAsset.insert(qsl("name"), checksumsName);
    checksumsAsset.insert(qsl("browser_download_url"), base + checksumsName);
    QJsonObject downloadAsset;
    downloadAsset.insert(qsl("name"), assetName);
    downloadAsset.insert(qsl("browser_download_url"), base + assetName);
    downloadAsset.insert(qsl("size"), 4194304);

    QJsonObject releaseInfo;
    releaseInfo.insert(qsl("tag_name"), qsl("Mudlet-9.9.9"));
    releaseInfo.insert(qsl("published_at"), qsl("2026-08-01T00:00:00Z"));
    releaseInfo.insert(qsl("body"), qsl("stub release"));
    releaseInfo.insert(qsl("assets"), QJsonArray({checksumsAsset, downloadAsset}));

    const dblsqd::Release release(releaseInfo, qsl("linux"), qsl("x64"));
    QCOMPARE(release.getDownloadUrl().toString(), base + assetName);
    QCOMPARE(release.getChecksumsUrl().toString(), base + checksumsName);

    dblsqd::Feed feed;
    QStringList downloadErrors;
    connect(&feed, &dblsqd::Feed::downloadError, this, [&downloadErrors](const QString& message) {
        downloadErrors << message;
    });

    // What the update dialog's automatic download and the twice-daily check do
    // when both answer the same ready(): the second call lands while the first
    // is still fetching checksums
    feed.downloadRelease(release, /*requireChecksums=*/true);
    feed.downloadRelease(release, /*requireChecksums=*/true);

    // Long enough for both checksum answers - including the deliberately late
    // one - and the download requests they lead to
    QTest::qWait(1500);

    QCOMPARE(server.downloadRequests(), 1);
    QCOMPARE(server.checksumRequests(), 1);
    QVERIFY2(downloadErrors.isEmpty(), qPrintable(qsl("no download error was expected, got: %1").arg(downloadErrors.join(qsl("; ")))));
    QVERIFY2(feed.isDownloading(), "the download started by the first request must still be running");
}

QTEST_MAIN(FeedChecksumRaceTest)
#include "FeedChecksumRaceTest.moc"
