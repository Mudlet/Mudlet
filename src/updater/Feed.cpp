#include "Feed.h"

#include "../utils.h"

#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTemporaryFile>

namespace dblsqd {

Feed::Feed()
: feedReply(nullptr)
, downloadReply(nullptr)
, downloadFile(nullptr)
, redirects(0)
, _ready(false)
{
}

void Feed::setRepo(const QString& owner, const QString& repo, bool prerelease, const QString& os, const QString& arch)
{
    m_owner = owner;
    m_repo = repo;
    m_prerelease = prerelease;
    m_os = os.isEmpty() ? detectOs() : os;
    m_arch = arch.isEmpty() ? detectArch() : arch;

    if (prerelease) {
        this->url = QUrl(qsl("https://api.github.com/repos/%1/%2/releases/tags/public-test-build").arg(owner, repo));
    } else {
        this->url = QUrl(qsl("https://api.github.com/repos/%1/%2/releases").arg(owner, repo));
    }
}

QString Feed::detectOs()
{
    QString autoOs = QSysInfo::productType().toLower();
    if (autoOs == qsl("windows")) {
        return qsl("win");
    } else if (autoOs == qsl("osx") || autoOs == qsl("macos")) {
        return qsl("mac");
    }
    return QSysInfo::kernelType();
}

QString Feed::detectArch()
{
    QString autoArch = QSysInfo::buildCpuArchitecture();
    if (autoArch == qsl("i386") || autoArch == qsl("i586")) {
        return qsl("x86");
    }
    return autoArch;
}

QUrl Feed::getUrl()
{
    return QUrl(url);
}

QList<Release> Feed::getReleases()
{
    return releases;
}

QList<Release> Feed::getUpdates(Release currentRelease)
{
    QList<Release> updates;
    for (const auto& release : releases) {
        if (currentRelease.getVersion().toLower() != release.getVersion().toLower() && currentRelease < release) {
            updates << release;
        }
    }
    return updates;
}

QTemporaryFile* Feed::getDownloadFile()
{
    return downloadFile;
}

bool Feed::isReady()
{
    return _ready;
}

void Feed::load()
{
    if (feedReply != nullptr && !feedReply->isFinished()) {
        return;
    }

    QNetworkRequest request(getUrl());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "Mudlet-Updater");
    feedReply = nam.get(request);
    connect(feedReply, &QNetworkReply::finished, this, &Feed::handleFeedFinished);
}

void Feed::downloadRelease(Release release)
{
    redirects = 0;

    // First fetch the checksums, then start the actual download
    this->release = release;
    const QUrl checksumsUrl = release.getChecksumsUrl();
    if (checksumsUrl.isValid() && !checksumsUrl.isEmpty()) {
        fetchChecksums(checksumsUrl);
    } else {
        makeDownloadRequest(release.getDownloadUrl());
    }
}

void Feed::fetchChecksums(const QUrl& checksumsUrl)
{
    QNetworkRequest request(checksumsUrl);
    request.setRawHeader("Accept", "application/octet-stream");
    request.setRawHeader("User-Agent", "Mudlet-Updater");
    auto* reply = nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            const QString checksumData = QString::fromUtf8(reply->readAll());
            const QStringList lines = checksumData.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (const auto& line : lines) {
                // Format: "hash  filename" or "hash *filename"
                const int separatorPos = line.indexOf(QRegularExpression(qsl("[\\s*]+")), 64);
                if (separatorPos <= 0) {
                    continue;
                }
                const QString hash = line.left(separatorPos).trimmed();
                const QString filename = line.mid(separatorPos).trimmed().remove(QLatin1Char('*'));

                // Match against the download URL filename
                const QString downloadFilename = this->release.getDownloadUrl().fileName();
                if (!downloadFilename.isEmpty() && filename.contains(downloadFilename, Qt::CaseInsensitive)) {
                    this->release.setDownloadSHA256(hash);
                    break;
                }
            }
        }
        reply->deleteLater();
        makeDownloadRequest(this->release.getDownloadUrl());
    });
}

void Feed::makeDownloadRequest(QUrl url)
{
    if (downloadReply != nullptr && !downloadReply->isFinished()) {
        disconnect(downloadReply);
        downloadReply->abort();
        downloadReply->deleteLater();
    }
    if (downloadFile != nullptr) {
        disconnect(downloadFile);
        downloadFile->close();
        downloadFile->deleteLater();
        downloadFile = nullptr;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mudlet-Updater");
    downloadReply = nam.get(request);
    connect(downloadReply, &QNetworkReply::downloadProgress, this, &Feed::handleDownloadProgress);
    connect(downloadReply, &QNetworkReply::readyRead, this, &Feed::handleDownloadReadyRead);
    connect(downloadReply, &QNetworkReply::finished, this, &Feed::handleDownloadFinished);
}

void Feed::handleFeedFinished()
{
    if (feedReply->error() != QNetworkReply::NoError) {
        emit loadError(feedReply->errorString());
        feedReply->deleteLater();
        feedReply = nullptr;
        return;
    }

    releases.clear();
    const QByteArray json = feedReply->readAll();
    feedReply->deleteLater();
    feedReply = nullptr;

    const QJsonDocument doc = QJsonDocument::fromJson(json);

    if (m_prerelease) {
        // PTB channel: single release object from /releases/tags/public-test-build
        if (doc.isObject()) {
            const QJsonObject releaseObj = doc.object();
            if (!releaseObj.value(qsl("draft")).toBool()) {
                releases << Release(releaseObj, m_os, m_arch);
            }
        }
    } else {
        // Release channel: array of all releases
        if (doc.isArray()) {
            const QJsonArray releasesArray = doc.array();
            for (const auto& val : releasesArray) {
                const QJsonObject releaseObj = val.toObject();

                if (releaseObj.value(qsl("prerelease")).toBool()) {
                    continue;
                }
                if (releaseObj.value(qsl("draft")).toBool()) {
                    continue;
                }

                releases << Release(releaseObj, m_os, m_arch);
            }
        }
    }

    std::sort(releases.begin(), releases.end());
    std::reverse(releases.begin(), releases.end());

    _ready = true;
    emit ready();
}

void Feed::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void Feed::handleDownloadReadyRead()
{
    if (downloadFile == nullptr) {
        QString fileName = downloadReply->url().fileName();
        int extensionPos = fileName.indexOf(QRegularExpression(qsl("(?:\\.tar)?\\.[a-zA-Z0-9]+$")));
        if (extensionPos > -1) {
            fileName.insert(extensionPos, qsl("-XXXXXX"));
        }
        downloadFile = new QTemporaryFile(QDir::tempPath() + qsl("/") + fileName);
        downloadFile->open();
    }
    downloadFile->write(downloadReply->readAll());
}

void Feed::handleDownloadFinished()
{
    if (downloadReply->error() != QNetworkReply::NoError) {
        emit downloadError(downloadReply->errorString());
        return;
    } else if (!downloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute).isNull()) {
        if (redirects >= 8) {
            emit downloadError(tr("Too many redirects."));
            return;
        }
        QUrl redirectionTarget = downloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        QUrl redirectedUrl = downloadReply->url().resolved(redirectionTarget);
        redirects++;
        makeDownloadRequest(redirectedUrl);
        return;
    } else if (downloadFile == nullptr) {
        emit downloadError(tr("No data received from server"));
        return;
    }

    downloadFile->flush();
    downloadFile->seek(0);
    QCryptographicHash fileHash(QCryptographicHash::Sha256);
    fileHash.addData(downloadFile->readAll());
    QString hashResult = fileHash.result().toHex();
    if (!release.getDownloadSHA256().isEmpty() && hashResult.toLower() != release.getDownloadSHA256().toLower()) {
        emit downloadError(tr("Could not verify download integrity."));
        return;
    }

    downloadFile->close();
    emit downloadFinished();
}

} // namespace dblsqd
