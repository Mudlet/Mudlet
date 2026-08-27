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
#include "updater/UpdateDialog.h"
#include "utils.h"

#include <QtTest/QtTest>

#include <QCryptographicHash>
#include <QFile>
#include <QHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QPointer>
#include <QProgressBar>
#include <QSettings>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTimer>

#include <memory>

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
 *
 * The rest of the file gates the five signals dblsqd::Feed offers the rest of
 * Mudlet - ready, loadError, downloadProgress, downloadFinished and
 * downloadError - by driving each one out of real network traffic and asserting
 * what UpdateDialog did with it. Nothing here connects to a Feed signal in
 * order to assert on it: a spy on the emitter would stay green with the
 * dialog's own connections cut, which is the failure mode these guard against.
 */

namespace {
const auto assetName = qsl("Mudlet-9.9.9-linux-x64.AppImage.tar");
const auto checksumsName = qsl("SHA256SUMS.txt");
const auto stubVersion = qsl("9.9.9");
const auto changelogMarker = qsl("stub release notes");
constexpr int downloadChunkSize = 8192;
// Qt throttles QNetworkReply::downloadProgress to one emission per 100ms and
// drops the rest, so a body written in a single burst arrives as a single
// emission at completion (measured: exactly one, 65536 of 65536). That is
// enough to move a progress bar, but it cannot tell a bar that follows the
// download from one that only ever jumps to 100% at the end. Dribbling the body
// out at more than one Qt interval per chunk is what produces the intermediate
// emissions worth asserting on, and the throttle is why a stalled event loop
// cannot suppress them: it drops an emission only when the previous one was
// under 100ms ago, so a stall long enough to merge the chunks is also long
// enough to guarantee the merged one gets through.
constexpr int downloadChunkDelayMs = 120;

// Every wait in this file. A healthy run answers each of them in under two
// seconds; the rest is headroom for an AddressSanitizer build on a loaded
// runner. QTRY_* spends up to three times what it is given before it fails -
// once polling, then twice more to build its "would have passed after N ms"
// message - which is what the ctest TIMEOUT in
// test/functional_tests/CMakeLists.txt is derived from.
constexpr int waitMs = 20000;

QJsonObject stubAsset(const QString& name, const QString& url, qint64 size)
{
    QJsonObject asset;
    asset.insert(qsl("name"), name);
    asset.insert(qsl("browser_download_url"), url);
    asset.insert(qsl("size"), static_cast<double>(size));
    return asset;
}

QJsonObject stubReleaseInfo(const QString& base, qint64 downloadSize)
{
    QJsonObject releaseInfo;
    releaseInfo.insert(qsl("tag_name"), qsl("Mudlet-") + stubVersion);
    releaseInfo.insert(qsl("published_at"), qsl("2026-08-01T00:00:00Z"));
    releaseInfo.insert(qsl("body"), changelogMarker);
    releaseInfo.insert(qsl("assets"), QJsonArray({stubAsset(checksumsName, base + checksumsName, 0), stubAsset(assetName, base + assetName, downloadSize)}));
    return releaseInfo;
}

QByteArray stubReleasesJson(const QString& base, qint64 downloadSize)
{
    return QJsonDocument(QJsonArray({stubReleaseInfo(base, downloadSize)})).toJson(QJsonDocument::Compact);
}

QByteArray stubDownloadPayload()
{
    return QByteArray(64 * 1024, 'm');
}
} // namespace

// Self-signed loopback certificate, valid until 2126, shared with
// GMCPCharLoginTest.cpp. The feed URL is fixed at https://api.github.com, so the
// only way to answer it locally is to be its proxy and terminate the tunnel's
// TLS here; the client accepts this certificate because the test turns peer
// verification off in the default QSslConfiguration.
static const char* csmStubCertificatePem = R"PEM(-----BEGIN CERTIFICATE-----
MIIDJzCCAg+gAwIBAgIULa4vwGAVOB+r6qtcLMPqwzBlEJgwDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDgwNjE0MTM0OFoYDzIxMjYw
NzEzMTQxMzQ4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDEg1HE09f69FW/OLD0jrWEQRbKkSkIkexLfV5OtzbI
ZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5LBEKGP1TTs6C9R+e5hlI6McoKown
ha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon7M4wzVvUO5VIRhFeA2AO6LRVQhyi
9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp4SbuHuXPwgiZ9hYEIX2xAFSR48OB
bjWgqVISptu/s+UkK2XckI42qdxqzwglLIIqjFYJ1HvGqhqV69DeqB0XNw6qp8W2
qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTfG1y8DnnLAgMBAAGjbzBtMB0GA1Ud
DgQWBBSyDWWzo202vFbYncaD2crvY5V2pDAfBgNVHSMEGDAWgBSyDWWzo202vFbY
ncaD2crvY5V2pDAPBgNVHRMBAf8EBTADAQH/MBoGA1UdEQQTMBGCCWxvY2FsaG9z
dIcEfwAAATANBgkqhkiG9w0BAQsFAAOCAQEAs5nw4GBPPHc9Nc08uLUYTDLkA2XM
WPugjSO7OxUe8NptVh/v4GbeKzQ4FRIF6rca8De15+OOZgIDppRUoy+fd+ncoDan
flw38rIj13XfV/3WF33Uag2xtZG0Hrpu4PFZQyIzr0MwGJJ/v2uRjMiV0CX+rc0L
BJg2JS4oCbNdQpwH81qOktoH8aHirAyLjtm732GQgAGLe0fIBBsb4Dg2ZdvN+TF5
xfKoFfri3H1rwju43zHXmUyCE/RPdIBR8flO6gzdgWAVY0jaixZi1fzQEQuReh2j
d2iZYOFSrVDea41ltrUvRC6q6gxe/REVjj1nCSYU1x44J9DQ6n6ljvJCVw==
-----END CERTIFICATE-----)PEM";

static const char* csmStubPrivateKeyPem = R"PEM(-----BEGIN PRIVATE KEY-----
MIIEwAIBADANBgkqhkiG9w0BAQEFAASCBKowggSmAgEAAoIBAQDEg1HE09f69FW/
OLD0jrWEQRbKkSkIkexLfV5OtzbIZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5
LBEKGP1TTs6C9R+e5hlI6McoKownha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon
7M4wzVvUO5VIRhFeA2AO6LRVQhyi9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp
4SbuHuXPwgiZ9hYEIX2xAFSR48OBbjWgqVISptu/s+UkK2XckI42qdxqzwglLIIq
jFYJ1HvGqhqV69DeqB0XNw6qp8W2qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTf
G1y8DnnLAgMBAAECggEBALRPHebwzfrI2CilttAeZXTdWDEzsifX5K17cd3eBBkp
xVuNShuCupZq9bUNOhl4ghlDPALmpRTFDHp78YKHXWFkLN5CVeoxjL+2Po6fQ4w7
/3zOtWNMYp/q32Kn+4ocjaLT0U+SDs0G6LR7dtGWjAyXQylWiTbu9+OWJ2kXSTlH
QbdtamymoJrrjRTV1HUEq/a3qSHlqTA5/EKIcGeiETq2NR0fZ3NFbe+PLiOSpiNg
uIiVEdsItuZTdINSEzOtMFvRd2od0ITDpMtLG404aGsI4Zisiuhr5naf4DWqK2aL
n9Z/55LSuAdBqvrtJ9XVdtNsFdCRjbIj2R1qqDTFm/kCgYEA6W/+ufDXrhPi8XWS
8+7tlOoUd0jYZL9N+N1hfho21SN3eH5TtNO0b/os/PN/M+5dKeWPtnyzwg49EksF
Es9Z4+lLt/Z+71RDmYqSCwaLhXNKtUZluZmrGHcRogd4hJDYv9icgmpwMK5Hg634
PYCgVYb9C1Wug/mZhgLg7Aw3hn8CgYEA14GxAPViaSszVNRps+a9WVEJklPbPR8U
kAxWTP6n1SdT3Z9HRcHH9inIdTLyC/3ti4+4dc1pDkMrq+MUTjvF8BqN35uzJa7l
6dnsXBmWvB1cIcwQb4SLnDb7jzmiK2uIjMrO54x3+atB83GdvESLOQ/9NAJL/+NX
ILq5kAs2nrUCgYEAqq7/8pceLKNPybttMr0drEenpTx3NNsIORItydWDCD8BiPHd
ZJdzFHk5Uc780EzWg97dQNJXYWmlz+1YjVNdZ57ahW1PjNDxCKBgfn1PoMkW9ArA
MIAisSXGl9GcllmOkl/guB75Xy7fDXIz00xsb3zfIt2IV+k2Dt2l9hJMuyMCgYEA
lv45ZHCJeSJZntANF41NkazjxfCXJaYHJD5goSWztfcOHbOhnlB9qA3yc5s0WA6c
RzJ1jaRUPTf2+0HpUj8zGl2gldFjnb2DPWwA3S7YnAj+Knft9BSsNNGZQ+qfo0h+
rhbTDQ0wanABj25FlEl6OornX29UjH9e5oGtziztIhkCgYEAppTHqOgLiKmeV15d
i850uRyh7X6whywY8gm0VLO+xzCVsCR6CvgZY1MwwFuDwu2d/d5jdJXLpHueQwNU
3HipTI77OuIRv4ykXwPOIemT9VmL/N21CgrckJGA6dYywTnc/JNpOKxdTM9srOyr
Rcsgla9jttJevaHI71x2jLNBaKk=
-----END PRIVATE KEY-----)PEM";

// Every connection is a QSslSocket left in plaintext until a proxy CONNECT
// upgrades it, so one port serves both the tunnelled feed request and the plain
// HTTP asset downloads the release JSON points at
class StubTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    using QTcpServer::QTcpServer;

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        auto* socket = new QSslSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            delete socket;
            return;
        }
        socket->setLocalCertificate(QSslCertificate(QByteArray(csmStubCertificatePem)));
        socket->setPrivateKey(QSslKey(QByteArray(csmStubPrivateKeyPem), QSsl::Rsa));
        addPendingConnection(socket);
    }
};

// A minimum-viable GitHub: it answers the feed request, serves the checksum
// file, and by default answers the download request with a Content-Length it
// never satisfies, so that download stays unfinished for the whole test
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

    void setFeedBody(const QByteArray& body) { mFeedBody = body; }
    void setChecksum(const QByteArray& hex) { mChecksum = hex; }
    // Switches the download from never finishing to this body, delivered in
    // chunks
    void setDownloadPayload(const QByteArray& payload) { mDownloadPayload = payload; }

    // Stops the body at this many bytes until releaseDownload() lets the rest
    // through. That is what makes "the progress bar moved while bytes were
    // still outstanding" a fact rather than a race against the runner: the
    // remaining bytes cannot arrive early, so the bar cannot reach its maximum
    // before the assertion reads it
    void holdDownloadAfter(int bytes) { mHoldAfter = bytes; }
    void releaseDownload()
    {
        mHoldAfter = -1;
        if (mHeldSocket) {
            auto* socket = mHeldSocket.data();
            mHeldSocket.clear();
            sendDownloadChunk(socket, mHeldOffset);
        }
    }

    int checksumRequests() const { return mChecksumRequests; }
    int downloadRequests() const { return mDownloadRequests; }

private:
    void acceptConnections()
    {
        while (auto* pending = mServer.nextPendingConnection()) {
            auto* socket = qobject_cast<QSslSocket*>(pending);
            connect(socket, &QSslSocket::readyRead, this, [this, socket]() {
                readRequest(socket);
            });
            connect(socket, &QSslSocket::disconnected, socket, &QObject::deleteLater);
            // A socket that dies mid-header never reaches the removal in
            // readRequest(), and the address it leaves behind can be handed to
            // the next socket - which would then inherit a partial request
            connect(socket, &QObject::destroyed, this, [this, socket]() {
                mBuffers.remove(socket);
            });
        }
    }

    void readRequest(QSslSocket* socket)
    {
        QByteArray& buffer = mBuffers[socket];
        buffer.append(socket->readAll());
        const int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }
        const QByteArray requestLine = buffer.left(buffer.indexOf("\r\n"));
        mBuffers.remove(socket);

        if (requestLine.startsWith("CONNECT ")) {
            socket->write("HTTP/1.1 200 Connection established\r\n\r\n");
            socket->flush();
            socket->startServerEncryption();
            return;
        }

        // Feed::setRepo() builds /repos/<owner>/<repo>/releases?per_page=N, and
        // nothing else this stub serves has "/releases" in its path: the asset
        // and checksum URLs are the port root with a filename appended
        if (requestLine.contains("/releases")) {
            sendBody(socket, "application/json", mFeedBody);
            return;
        }

        if (requestLine.contains(checksumsName.toLatin1())) {
            ++mChecksumRequests;
            // The first answer is immediate so its download is running before
            // any second one lands; a later one is held back to guarantee that
            const int delayMs = mChecksumRequests == 1 ? 0 : 300;
            QTimer::singleShot(delayMs, socket, [this, socket]() {
                sendBody(socket, "text/plain", mChecksum + "  " + assetName.toLatin1() + "\n");
            });
            return;
        }

        if (requestLine.contains(assetName.toLatin1())) {
            ++mDownloadRequests;
            sendDownload(socket);
            return;
        }

        socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
    }

    static void sendBody(QSslSocket* socket, const QByteArray& contentType, const QByteArray& body)
    {
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n");
        socket->write(body);
        socket->flush();
        socket->disconnectFromHost();
    }

    void sendDownload(QSslSocket* socket)
    {
        if (mDownloadPayload.isEmpty()) {
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 4194304\r\nConnection: close\r\n\r\n");
            socket->write(QByteArray(16, 'x'));
            socket->flush();
            return;
        }
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + QByteArray::number(mDownloadPayload.size()) + "\r\nConnection: close\r\n\r\n");
        socket->flush();
        sendDownloadChunk(socket, 0);
    }

    void sendDownloadChunk(QSslSocket* socket, int offset)
    {
        if (offset >= mDownloadPayload.size()) {
            socket->disconnectFromHost();
            return;
        }
        if (mHoldAfter >= 0 && offset >= mHoldAfter) {
            mHeldSocket = socket;
            mHeldOffset = offset;
            return;
        }
        const int chunk = qMin<int>(downloadChunkSize, mDownloadPayload.size() - offset);
        socket->write(mDownloadPayload.mid(offset, chunk));
        socket->flush();
        QTimer::singleShot(downloadChunkDelayMs, socket, [this, socket, offset, chunk]() {
            sendDownloadChunk(socket, offset + chunk);
        });
    }

    QHash<QSslSocket*, QByteArray> mBuffers;
    QByteArray mFeedBody{"[]"};
    QByteArray mChecksum{QByteArray(64, 'a')};
    QByteArray mDownloadPayload;
    int mChecksumRequests{0};
    int mDownloadRequests{0};
    int mHoldAfter{-1};
    int mHeldOffset{0};
    QPointer<QSslSocket> mHeldSocket;
    // Last on purpose, so it is destroyed first: its child sockets prune
    // mBuffers as they go, and with the usual declaration order that hash would
    // already be gone by then
    StubTcpServer mServer;
};

// Feed builds its URL from a hardcoded https://api.github.com, so only the feed
// request is tunnelled through the stub. The asset URLs already name it
// directly and have to stay direct, or the download would arrive over the same
// TLS tunnel the feed uses.
class StubProxyFactory : public QNetworkProxyFactory
{
public:
    explicit StubProxyFactory(quint16 port)
    : mPort(port)
    {
    }

    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery& query) override
    {
        if (query.peerHostName() == qsl("api.github.com")) {
            return {QNetworkProxy(QNetworkProxy::HttpProxy, qsl("127.0.0.1"), mPort)};
        }
        return {QNetworkProxy(QNetworkProxy::NoProxy)};
    }

private:
    quint16 mPort;
};

// UpdateDialog::handleDownloadError() reports through QMessageBox::warning(),
// which runs a nested event loop of its own - and QTRY_* cannot expire above
// one, so a box nobody closes hangs the test until QtTest's watchdog kills the
// process. Closing them from the show event rather than from a poll is what
// makes that deterministic: the accept is already queued before exec() starts
// its loop, and every box is caught, not just the first.
class ModalBoxCatcher : public QObject
{
public:
    QStringList texts() const { return mTexts; }
    void clear() { mTexts.clear(); }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Show) {
            if (auto* box = qobject_cast<QMessageBox*>(watched)) {
                mTexts << box->text();
                // Deferred rather than immediate: the box is not in its own
                // event loop yet, and closing it before exec() starts leaves
                // exec() with nothing to return on
                QTimer::singleShot(0, box, &QMessageBox::accept);
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QStringList mTexts;
};

// What Updater::setupOnLinux() assembles: a Feed and an UpdateDialog listening
// to it, wired by nothing but their own constructors
class UpdateHarness
{
public:
    UpdateHarness()
    : mSettings(mSettingsDir.filePath(qsl("updater-test.ini")), QSettings::IniFormat)
    {
    }

    ~UpdateHarness()
    {
        // Before the Feed, so a reply abandoned mid-flight cannot reach a
        // dialog that would answer it with a modal box no event loop is left
        // to dismiss
        mDialog.reset();
        // Feed deliberately leaves a finished download on disk for the next
        // launch to reuse (#9985), so the test has to collect it
        const QString downloaded = mFeed.getDownloadFilePath();
        if (!downloaded.isEmpty()) {
            QFile::remove(downloaded);
        }
        QNetworkProxyFactory::setApplicationProxyFactory(nullptr);
    }

    bool start()
    {
        if (!QSslSocket::supportsSsl() || !mSettingsDir.isValid() || !mServer.listen()) {
            return false;
        }
        QNetworkProxyFactory::setApplicationProxyFactory(new StubProxyFactory(mServer.port()));
        return true;
    }

    void openDialog(dblsqd::UpdateDialog::Type type)
    {
        // The os and arch Release matches its assets against, so the stub
        // release offers an installable download whatever the test runs on
        mFeed.setRepo(qsl("Mudlet"), qsl("Mudlet"), false, qsl("linux"), qsl("x64"));
        mDialog = std::make_unique<dblsqd::UpdateDialog>(&mFeed, type, &mSettings);
    }

    QString assetBaseUrl() const { return qsl("http://127.0.0.1:%1/").arg(mServer.port()); }
    StubFeedServer& server() { return mServer; }
    QSettings& settings() { return mSettings; }
    dblsqd::Feed& feed() { return mFeed; }
    dblsqd::UpdateDialog& dialog() { return *mDialog; }

private:
    QTemporaryDir mSettingsDir;
    StubFeedServer mServer;
    QSettings mSettings;
    dblsqd::Feed mFeed;
    std::unique_ptr<dblsqd::UpdateDialog> mDialog;
};

class FeedChecksumRaceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void secondDownloadRequestDuringChecksumFetchDoesNotCrash();
    void feedReadyReachesTheUpdateDialog();
    void feedLoadErrorReachesTheUpdateDialog();
    void downloadProgressReachesTheUpdateDialog();
    void downloadFinishedReachesTheUpdateDialog();
    void downloadErrorReachesTheUpdateDialog();

private:
    // What the dialog told the user, for the failure messages: a bare
    // QTRY_VERIFY prints its expression and nothing about why the wire stayed
    // quiet, and the reason is usually already in a warning box the dialog raised
    QString whatTheDialogWasTold() const { return mModalBoxes.texts().isEmpty() ? qsl("no error was reported to the user") : mModalBoxes.texts().join(qsl("; ")); }

    ModalBoxCatcher mModalBoxes;
    QSslConfiguration mOriginalSslConfiguration;
    QString mOriginalApplicationVersion;
};

void FeedChecksumRaceTest::initTestCase()
{
    // Everything set here is process-global, so it is all put back in
    // cleanupTestCase(). This file is its own binary today, but a grouped one
    // would carry all of it into every test that followed.

    // Release::getCurrentRelease() reads this, and a test binary has no version
    // of its own for the stub release to be newer than
    mOriginalApplicationVersion = QCoreApplication::applicationVersion();
    QCoreApplication::setApplicationVersion(qsl("1.0.0"));

    // The stub answers for api.github.com with a certificate issued to
    // localhost, which is only acceptable with verification off
    mOriginalSslConfiguration = QSslConfiguration::defaultConfiguration();
    QSslConfiguration configuration = mOriginalSslConfiguration;
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(configuration);

    qApp->installEventFilter(&mModalBoxes);
}

void FeedChecksumRaceTest::init()
{
    mModalBoxes.clear();
}

void FeedChecksumRaceTest::cleanupTestCase()
{
    qApp->removeEventFilter(&mModalBoxes);
    QSslConfiguration::setDefaultConfiguration(mOriginalSslConfiguration);
    QCoreApplication::setApplicationVersion(mOriginalApplicationVersion);
    // Qt offers no getter for the application proxy factory, so the best that
    // can be restored is its default of having none
    QNetworkProxyFactory::setApplicationProxyFactory(nullptr);
}

void FeedChecksumRaceTest::secondDownloadRequestDuringChecksumFetchDoesNotCrash()
{
    StubFeedServer server;
    QVERIFY2(server.listen(), "the stub update server could not listen on localhost");
    const QString base = qsl("http://127.0.0.1:%1/").arg(server.port());

    const dblsqd::Release release(stubReleaseInfo(base, 4194304), qsl("linux"), qsl("x64"));
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

// Feed::ready() -> UpdateDialog::handleFeedReady(). Cut it and the dialog is
// left showing its loading spinner: nothing tells the user an update exists.
void FeedChecksumRaceTest::feedReadyReachesTheUpdateDialog()
{
    UpdateHarness harness;
    QVERIFY2(harness.start(), "the stub update server needs TLS support and a free loopback port");
    harness.server().setFeedBody(stubReleasesJson(harness.assetBaseUrl(), stubDownloadPayload().size()));
    dblsqd::UpdateDialog::enableAutoDownload(false, &harness.settings());

    harness.openDialog(dblsqd::UpdateDialog::Manual);
    QSignalSpy readySpy(&harness.dialog(), &dblsqd::UpdateDialog::ready);
    QVERIFY2(readySpy.wait(waitMs), qPrintable(qsl("the update dialog never became ready - the feed either did not load or did not reach it. It reported: %1").arg(whatTheDialogWasTold())));

    auto* changelog = harness.dialog().findChild<QTextBrowser*>(qsl("labelChangelog"));
    QVERIFY2(changelog, "the dialog has no child named labelChangelog - update_dialog.ui renamed it");
    QVERIFY2(changelog->toPlainText().contains(changelogMarker), qPrintable(qsl("the dialog did not build its changelog from the feed, it shows: %1").arg(changelog->toPlainText())));

    // handleFeedReady() re-arms its own single-shot connections on the way out,
    // and only a second load proves it: without that, the twice-daily check and
    // every manual check after the first reach nobody
    harness.feed().load();
    QVERIFY2(readySpy.wait(waitMs),
             qPrintable(qsl("a repeated update check never reached the dialog - handleFeedReady() did not re-arm its Feed connections. It reported: %1").arg(whatTheDialogWasTold())));
}

// Feed::loadError() -> UpdateDialog::handleLoadError(). Cut it and a failed
// update check leaves the dialog spinning forever with no explanation.
void FeedChecksumRaceTest::feedLoadErrorReachesTheUpdateDialog()
{
    UpdateHarness harness;
    QVERIFY2(harness.start(), "the stub update server needs TLS support and a free loopback port");
    harness.server().setFeedBody("<html>an error page served as 200</html>");

    harness.openDialog(dblsqd::UpdateDialog::Manual);
    // handleLoadError() only rebuilds the interface on a dialog the user can see
    harness.dialog().show();

    auto* headline = harness.dialog().findChild<QLabel*>(qsl("labelHeadlineNoUpdates"));
    QVERIFY2(headline, "the dialog has no child named labelHeadlineNoUpdates - update_dialog.ui renamed it");
    QTRY_COMPARE_WITH_TIMEOUT(headline->text(), qsl("Could not check for updates"), waitMs);

    auto* changelog = harness.dialog().findChild<QTextBrowser*>(qsl("labelChangelog"));
    QVERIFY2(changelog, "the dialog has no child named labelChangelog - update_dialog.ui renamed it");
    QVERIFY2(changelog->toPlainText().contains(qsl("Could not read update information from the server")),
             qPrintable(qsl("the dialog did not show what went wrong, it shows: %1").arg(changelog->toPlainText())));

    // handleLoadError() re-arms its own single-shot connections too, so a check
    // that failed once does not condemn every check after it. It takes a second
    // failure to prove: a success would arrive on the ready() connection the
    // constructor made, which a loadError never consumed, and so would pass with
    // the re-arm gone. The message differs on purpose, since the dialog is
    // already in the state the first failure left it in
    harness.server().setFeedBody(R"({"message":"stub API failure"})");
    harness.feed().load();
    QTRY_VERIFY2_WITH_TIMEOUT(
            changelog->toPlainText().contains(qsl("stub API failure")),
            qPrintable(qsl("a second failed update check never reached the dialog - handleLoadError() did not re-arm its Feed connections. It still shows: %1").arg(changelog->toPlainText())),
            waitMs);
}

// Feed::downloadProgress() -> UpdateDialog::updateProgressBar(). Cut it and a
// 135MB download runs behind a progress bar that never moves.
void FeedChecksumRaceTest::downloadProgressReachesTheUpdateDialog()
{
    const QByteArray payload = stubDownloadPayload();

    UpdateHarness harness;
    QVERIFY2(harness.start(), "the stub update server needs TLS support and a free loopback port");
    harness.server().setFeedBody(stubReleasesJson(harness.assetBaseUrl(), payload.size()));
    harness.server().setChecksum(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
    harness.server().setDownloadPayload(payload);
    // Held at half, so the bar has to be seen moving while bytes are still
    // outstanding. Without the hold the only guaranteed emission is the one at
    // completion, which a bar that never moved until the end would satisfy too
    harness.server().holdDownloadAfter(static_cast<int>(payload.size() / 2));
    // What starts the download: handleFeedReady() acts on this before it builds
    // the interface the progress bar lives in
    dblsqd::UpdateDialog::enableAutoDownload(true, &harness.settings());

    harness.openDialog(dblsqd::UpdateDialog::Manual);

    auto* progressBar = harness.dialog().findChild<QProgressBar*>(qsl("progressBar"));
    QVERIFY2(progressBar, "the dialog has no child named progressBar - update_dialog.ui renamed it");
    QTRY_VERIFY2_WITH_TIMEOUT(progressBar->value() > 0, qPrintable(qsl("the progress bar never moved. The dialog reported: %1").arg(whatTheDialogWasTold())), waitMs);
    QCOMPARE(progressBar->maximum(), static_cast<int>(payload.size() / 1024));
    QVERIFY2(progressBar->value() < progressBar->maximum(),
             qPrintable(qsl("the bar only moved once the download was over - it read %1 of %2 while half the body was still held back").arg(progressBar->value()).arg(progressBar->maximum())));

    harness.server().releaseDownload();
    QTRY_COMPARE_WITH_TIMEOUT(progressBar->value(), progressBar->maximum(), waitMs);
}

// Feed::downloadFinished() -> UpdateDialog::handleDownloadFinished(). Cut it
// and the finished download is never recorded, so the next launch downloads the
// same release again and the install button stays disabled.
void FeedChecksumRaceTest::downloadFinishedReachesTheUpdateDialog()
{
    const QByteArray payload = stubDownloadPayload();

    UpdateHarness harness;
    QVERIFY2(harness.start(), "the stub update server needs TLS support and a free loopback port");
    harness.server().setFeedBody(stubReleasesJson(harness.assetBaseUrl(), payload.size()));
    harness.server().setChecksum(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
    harness.server().setDownloadPayload(payload);
    dblsqd::UpdateDialog::enableAutoDownload(true, &harness.settings());

    harness.openDialog(dblsqd::UpdateDialog::Manual);

    QTRY_VERIFY2_WITH_TIMEOUT(!harness.settings().value(qsl("DBLSQD/updateFilePath")).toString().isEmpty(),
                              qPrintable(qsl("the finished download was never recorded. The dialog reported: %1").arg(whatTheDialogWasTold())),
                              waitMs);
    QCOMPARE(harness.settings().value(qsl("DBLSQD/updateFilePath")).toString(), harness.feed().getDownloadFilePath());
    QCOMPARE(harness.settings().value(qsl("DBLSQD/updateFileVersion")).toString(), stubVersion);
}

// Feed::downloadError() -> UpdateDialog::handleDownloadError(). Cut it and a
// download that failed verification is reported to nobody: the dialog sits
// there with its buttons disabled by startDownload().
void FeedChecksumRaceTest::downloadErrorReachesTheUpdateDialog()
{
    const QByteArray payload = stubDownloadPayload();

    UpdateHarness harness;
    QVERIFY2(harness.start(), "the stub update server needs TLS support and a free loopback port");
    harness.server().setFeedBody(stubReleasesJson(harness.assetBaseUrl(), payload.size()));
    // A checksum that does not describe what the server sends, which is what a
    // tampered or truncated download looks like
    harness.server().setChecksum(QByteArray(64, 'b'));
    harness.server().setDownloadPayload(payload);
    dblsqd::UpdateDialog::enableAutoDownload(true, &harness.settings());

    // mModalBoxes, installed for the whole class, reads and closes the warning
    // box; doing it here with a local timer would leave any box opened while the
    // harness is being torn down with nothing to close it

    harness.openDialog(dblsqd::UpdateDialog::Manual);
    QSignalSpy rejectedSpy(&harness.dialog(), &QDialog::rejected);

    QTRY_VERIFY2_WITH_TIMEOUT(!mModalBoxes.texts().isEmpty(), "the download failed verification and nobody was told - no warning box was raised", waitMs);
    const QString warningText = mModalBoxes.texts().constFirst();
    QVERIFY2(warningText.contains(qsl("Could not verify download integrity")), qPrintable(qsl("the dialog warned about something else: %1").arg(warningText)));
    QTRY_COMPARE_WITH_TIMEOUT(rejectedSpy.count(), 1, waitMs);
}

QTEST_MAIN(FeedChecksumRaceTest)
#include "FeedChecksumRaceTest.moc"
