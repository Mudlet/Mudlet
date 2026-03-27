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
: mFeedReply(nullptr)
, mDownloadReply(nullptr)
, mDownloadFile(nullptr)
, mReady(false)
{
}

void Feed::setRepo(const QString& owner, const QString& repo, bool prerelease, const QString& os, const QString& arch)
{
    mOwner = owner;
    mRepo = repo;
    mPrerelease = prerelease;
    mOs = os.isEmpty() ? detectOs() : os;
    mArch = arch.isEmpty() ? detectArch() : arch;

    if (prerelease) {
        mUrl = QUrl(qsl("https://api.github.com/repos/%1/%2/releases/tags/public-test-build").arg(owner, repo));
    } else {
        mUrl = QUrl(qsl("https://api.github.com/repos/%1/%2/releases").arg(owner, repo));
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

QUrl Feed::getUrl() const
{
    return mUrl;
}

QList<Release> Feed::getReleases() const
{
    return mReleases;
}

QList<Release> Feed::getUpdates(const Release& currentRelease)
{
    QList<Release> updates;
    for (const auto& release : mReleases) {
        if (currentRelease.getVersion().toLower() != release.getVersion().toLower() && currentRelease < release) {
            updates << release;
        }
    }
    return updates;
}

QTemporaryFile* Feed::getDownloadFile()
{
    return mDownloadFile;
}

bool Feed::isReady() const
{
    return mReady;
}

void Feed::load()
{
    if (mFeedReply != nullptr && !mFeedReply->isFinished()) {
        qWarning() << "Update check already in progress, ignoring duplicate request";
        emit loadError(tr("Update check already in progress"));
        return;
    }

    QNetworkRequest request(getUrl());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "Mudlet-Updater");
    request.setTransferTimeout(30000);
    mFeedReply = mNam.get(request);
    connect(mFeedReply, &QNetworkReply::finished, this, &Feed::handleFeedFinished);
}

void Feed::downloadRelease(const Release& release)
{
    // First fetch the checksums, then start the actual download
    mCurrentDownload = release;
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
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(30000);
    auto* reply = mNam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Failed to fetch checksums:" << reply->errorString() << "- download will proceed without integrity verification";
        } else {
            const QString checksumData = QString::fromUtf8(reply->readAll());
            const QStringList lines = checksumData.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            // SHA256 hex digest is 64 characters; search for separator after that
            static const QRegularExpression separatorRx(qsl("[\\s*]+"));
            static const QRegularExpression hexRx(qsl("^[0-9a-fA-F]{64}$"));
            for (const auto& line : lines) {
                // Format: "hash  filename" or "hash *filename"
                const int separatorPos = line.indexOf(separatorRx, 64);
                if (separatorPos <= 0) {
                    continue;
                }
                const QString hash = line.left(separatorPos).trimmed();
                if (!hexRx.match(hash).hasMatch()) {
                    continue;
                }
                const QString filename = line.mid(separatorPos).trimmed().remove(QLatin1Char('*'));

                // Match against the download URL filename
                const QString downloadFilename = mCurrentDownload.getDownloadUrl().fileName();
                if (!downloadFilename.isEmpty() && filename.contains(downloadFilename, Qt::CaseInsensitive)) {
                    mCurrentDownload.setDownloadSHA256(hash);
                    break;
                }
            }
            if (mCurrentDownload.getDownloadSHA256().isEmpty()) {
                qWarning() << "Checksum file downloaded but no matching hash found for" << mCurrentDownload.getDownloadUrl().fileName() << "- download will proceed without integrity verification";
            }
        }
        reply->deleteLater();
        makeDownloadRequest(mCurrentDownload.getDownloadUrl());
    });
}

void Feed::makeDownloadRequest(const QUrl& url)
{
    if (mDownloadReply != nullptr && !mDownloadReply->isFinished()) {
        disconnect(mDownloadReply);
        mDownloadReply->abort();
        mDownloadReply->deleteLater();
    }
    if (mDownloadFile != nullptr) {
        disconnect(mDownloadFile);
        mDownloadFile->close();
        mDownloadFile->deleteLater();
        mDownloadFile = nullptr;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mudlet-Updater");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(60000);
    mDownloadReply = mNam.get(request);
    connect(mDownloadReply, &QNetworkReply::downloadProgress, this, &Feed::downloadProgress);
    connect(mDownloadReply, &QNetworkReply::readyRead, this, &Feed::handleDownloadReadyRead);
    connect(mDownloadReply, &QNetworkReply::finished, this, &Feed::handleDownloadFinished);
}

void Feed::handleFeedFinished()
{
    if (mFeedReply->error() != QNetworkReply::NoError) {
        emit loadError(mFeedReply->errorString());
        mFeedReply->deleteLater();
        mFeedReply = nullptr;
        return;
    }

    mReleases.clear();
    const QByteArray json = mFeedReply->readAll();
    mFeedReply->deleteLater();
    mFeedReply = nullptr;

    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull()) {
        emit loadError(tr("Could not parse update information from server"));
        return;
    }

    // Check for GitHub API error responses (e.g. rate limiting)
    if (doc.isObject() && doc.object().contains(qsl("message")) && !mPrerelease) {
        emit loadError(doc.object().value(qsl("message")).toString());
        return;
    }

    if (mPrerelease) {
        // PTB channel: single release object from /releases/tags/public-test-build
        if (doc.isObject()) {
            const QJsonObject releaseObj = doc.object();
            if (releaseObj.contains(qsl("message"))) {
                emit loadError(releaseObj.value(qsl("message")).toString());
                return;
            }
            if (!releaseObj.value(qsl("draft")).toBool()) {
                mReleases << Release(releaseObj, mOs, mArch);
            }
        } else {
            emit loadError(tr("Unexpected response format from update server"));
            return;
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

                mReleases << Release(releaseObj, mOs, mArch);
            }
        } else {
            emit loadError(tr("Unexpected response format from update server"));
            return;
        }
    }

    std::sort(mReleases.begin(), mReleases.end(), [](const Release& a, const Release& b) {
        return b < a;
    });

    mReady = true;
    emit ready();
}

void Feed::handleDownloadReadyRead()
{
    if (mDownloadFile == nullptr) {
        QString fileName = mDownloadReply->url().fileName();
        // Insert QTemporaryFile placeholder pattern before the file extension so Qt generates a unique filename
        static const QRegularExpression extensionRx(qsl("(?:\\.tar)?\\.[a-zA-Z0-9]+$"));
        int extensionPos = fileName.indexOf(extensionRx);
        if (extensionPos > -1) {
            fileName.insert(extensionPos, qsl("-XXXXXX"));
        }
        mDownloadFile = new QTemporaryFile(QDir::tempPath() + qsl("/") + fileName);
        if (!mDownloadFile->open()) {
            qWarning() << "Failed to create temporary file for download:" << mDownloadFile->errorString();
            emit downloadError(tr("Could not create temporary file for download: %1").arg(mDownloadFile->errorString()));
            mDownloadReply->abort();
            delete mDownloadFile;
            mDownloadFile = nullptr;
            return;
        }
    }
    const QByteArray data = mDownloadReply->readAll();
    const qint64 bytesWritten = mDownloadFile->write(data);
    if (bytesWritten == -1) {
        qWarning() << "Failed to write download data to temporary file:" << mDownloadFile->errorString();
        emit downloadError(tr("Failed to save download data: %1").arg(mDownloadFile->errorString()));
        mDownloadReply->abort();
        mDownloadFile->close();
        delete mDownloadFile;
        mDownloadFile = nullptr;
        return;
    }
}

void Feed::handleDownloadFinished()
{
    if (mDownloadReply->error() != QNetworkReply::NoError) {
        emit downloadError(mDownloadReply->errorString());
        if (mDownloadFile) {
            mDownloadFile->close();
            delete mDownloadFile;
            mDownloadFile = nullptr;
        }
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
        return;
    }

    if (mDownloadFile == nullptr) {
        emit downloadError(tr("No data received from server"));
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
        return;
    }

    if (!mDownloadFile->flush()) {
        qWarning() << "Failed to flush download file:" << mDownloadFile->errorString();
        emit downloadError(tr("Failed to save download: %1").arg(mDownloadFile->errorString()));
        mDownloadFile->close();
        delete mDownloadFile;
        mDownloadFile = nullptr;
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
        return;
    }
    if (!mDownloadFile->seek(0)) {
        qWarning() << "Failed to seek in download file:" << mDownloadFile->errorString();
        emit downloadError(tr("Failed to verify download integrity"));
        mDownloadFile->close();
        delete mDownloadFile;
        mDownloadFile = nullptr;
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
        return;
    }
    QCryptographicHash fileHash(QCryptographicHash::Sha256);
    fileHash.addData(mDownloadFile);
    const QString hashResult = fileHash.result().toHex();
    if (!mCurrentDownload.getDownloadSHA256().isEmpty() && hashResult.toLower() != mCurrentDownload.getDownloadSHA256().toLower()) {
        qWarning() << "SHA256 mismatch - expected:" << mCurrentDownload.getDownloadSHA256() << "got:" << hashResult;
        emit downloadError(tr("Could not verify download integrity."));
        mDownloadFile->close();
        delete mDownloadFile;
        mDownloadFile = nullptr;
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
        return;
    }

    mDownloadFile->close();
    mDownloadReply->deleteLater();
    mDownloadReply = nullptr;
    emit downloadFinished();
    // Ownership of mDownloadFile transfers to the caller via getDownloadFile()
    // which must be called from the synchronous downloadFinished() handler.
    mDownloadFile = nullptr;
}

} // namespace dblsqd
