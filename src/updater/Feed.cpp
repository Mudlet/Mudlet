/***************************************************************************
 *   Copyright (C) 2017 by Philipp Medien - hello@dblsqd.com               *
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
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

Feed::Feed(QObject* parent)
: QObject(parent)
{
}

Feed::~Feed()
{
    cleanupDownloadFile();
}

void Feed::setRepo(const QString& owner, const QString& repo, bool prerelease, const QString& os, const QString& arch)
{
    mOwner = owner;
    mRepo = repo;
    mPrerelease = prerelease;
    mOs = os.isEmpty() ? detectOs() : os;
    mArch = arch.isEmpty() ? detectArch() : arch;

    if (prerelease) {
        mUrl = QUrl(qsl("https://api.github.com/repos/%1/%2/releases?per_page=10").arg(owner, repo));
    } else {
        mUrl = QUrl(qsl("https://api.github.com/repos/%1/%2/releases?per_page=100").arg(owner, repo));
    }
}

QString Feed::detectOs()
{
    QString os = QSysInfo::productType().toLower();
    if (os == qsl("windows")) {
        return qsl("win");
    } else if (os == qsl("osx") || os == qsl("macos")) {
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

QList<Release> Feed::getUpdates(const Release& currentRelease) const
{
    QList<Release> updates;
    for (const auto& release : mReleases) {
        if (currentRelease.getVersion().toLower() != release.getVersion().toLower() && currentRelease < release) {
            updates << release;
        }
    }
    return updates;
}

QString Feed::getDownloadFilePath() const
{
    return mDownloadFilePath;
}

bool Feed::isReady() const
{
    return mReady;
}

bool Feed::isDownloading() const
{
    return mDownloadReply != nullptr && !mDownloadReply->isFinished();
}

void Feed::load()
{
    if (mFeedReply != nullptr) {
        if (!mFeedReply->isFinished()) {
            qWarning() << "Update check already in progress, ignoring duplicate request";
            //: Error shown when the user triggers an update check while one is already running
            emit loadError(tr("Update check already in progress"));
            return;
        }
        mFeedReply->deleteLater();
        mFeedReply = nullptr;
    }

    mReady = false;

    QNetworkRequest request(getUrl());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "Mudlet-Updater");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(30000);
    mFeedReply = mNam.get(request);
    connect(mFeedReply, &QNetworkReply::finished, this, &Feed::handleFeedFinished);
}

void Feed::downloadRelease(const Release& release, bool requireChecksums)
{
    if (isDownloading()) {
        qWarning() << "Download already in progress, ignoring duplicate request";
        return;
    }

    const QUrl downloadUrl = release.getDownloadUrl();
    if (!downloadUrl.isValid() || downloadUrl.isEmpty()) {
        //: Error shown when the GitHub release has no binary matching the user's operating system
        emit downloadError(tr("No download available for your platform"));
        return;
    }

    mCurrentDownload = release;
    mRequireChecksums = requireChecksums;
    const QUrl checksumsUrl = release.getChecksumsUrl();
    if (checksumsUrl.isValid() && !checksumsUrl.isEmpty()) {
        fetchChecksums(checksumsUrl);
    } else if (requireChecksums) {
        qWarning() << "Release" << release.getVersion() << "publishes no checksums - refusing to install an unverifiable download";
        //: Error shown when the release publishes no checksums at all, so the download cannot be verified as safe to install
        emit downloadError(tr("This update does not publish the checksums needed to verify it. Please try again later, or download it from https://www.mudlet.org/download/"));
    } else {
        qCritical() << "Release" << release.getVersion() << "publishes no checksums - download will proceed without integrity verification";
        makeDownloadRequest(downloadUrl);
    }
}

QString Feed::findChecksum(const QString& checksumData, const QString& downloadFilename, int* entriesParsed)
{
    if (entriesParsed) {
        *entriesParsed = 0;
    }
    if (downloadFilename.isEmpty()) {
        return QString();
    }

    // SHA256 hex digest is 64 characters; search for separator after that
    static const QRegularExpression separatorRx(qsl("[\\s*]+"));
    static const QRegularExpression hexRx(qsl("^[0-9a-fA-F]{64}$"));

    QString match;
    const QStringList lines = checksumData.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
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
        if (entriesParsed) {
            ++*entriesParsed;
        }
        // Compare the whole name, not a substring of it: SHA256SUMS.txt accumulates
        // entries across builds, so a longer name that happens to contain this one
        // would otherwise hand back the wrong hash. The generators write bare
        // basenames, but tolerate a path in case one ever stops.
        const QString filename = line.mid(separatorPos).trimmed().remove(QLatin1Char('*'));
        if (match.isEmpty() && filename.section(QLatin1Char('/'), -1).compare(downloadFilename, Qt::CaseInsensitive) == 0) {
            match = hash;
        }
    }
    return match;
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
            if (mRequireChecksums) {
                qWarning() << "Failed to fetch checksums:" << reply->errorString() << "- refusing to install an unverifiable download";
                reply->deleteLater();
                //: Error shown when the checksums needed to verify the update could not be downloaded
                emit downloadError(tr("Could not download the checksums needed to verify this update. Please try again later."));
                return;
            }
            qWarning() << "Failed to fetch checksums:" << reply->errorString() << "- download will proceed without integrity verification";
        } else {
            const QString downloadFilename = mCurrentDownload.getDownloadUrl().fileName();
            const QByteArray checksumData = reply->readAll();
            int entriesParsed = 0;
            mCurrentDownload.setDownloadSHA256(findChecksum(QString::fromUtf8(checksumData), downloadFilename, &entriesParsed));
            if (mCurrentDownload.getDownloadSHA256().isEmpty()) {
                qWarning() << "Checksum file has no entry for" << downloadFilename << "- parsed" << entriesParsed << "entries from" << checksumData.size() << "bytes";
                if (mRequireChecksums) {
                    reply->deleteLater();
                    if (entriesParsed == 0) {
                        // Nothing parsed means the payload was not a checksum file at
                        // all - a truncated transfer, or an error page served as 200 -
                        // rather than a release that forgot one platform
                        //: Error shown when the checksum file for the update was downloaded but could not be read
                        emit downloadError(tr("The checksums for this update could not be read, so it cannot be verified. Please try again later."));
                    } else {
                        //: Error shown when the release publishes checksums but none of them cover this platform's download
                        emit downloadError(
                                tr("This update is missing a checksum for your platform, so it cannot be verified. Please try again later, or download it from https://www.mudlet.org/download/"));
                    }
                    return;
                }
                qCritical() << "Proceeding without integrity verification for" << downloadFilename;
            }
        }
        reply->deleteLater();
        makeDownloadRequest(mCurrentDownload.getDownloadUrl());
    });
}

void Feed::makeDownloadRequest(const QUrl& url)
{
    mDownloadFilePath.clear();

    if (mDownloadReply != nullptr) {
        if (!mDownloadReply->isFinished()) {
            disconnect(mDownloadReply);
            mDownloadReply->abort();
        }
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
    }
    if (mDownloadFile != nullptr) {
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
        //: Error shown when the network request to the update server fails. %1 is the technical error description.
        emit loadError(tr("Could not connect to the update server: %1").arg(mFeedReply->errorString()));
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
        //: Error shown when the server response cannot be understood
        emit loadError(tr("Could not read update information from the server"));
        return;
    }

    // Check for GitHub API error responses (e.g. rate limiting, not found)
    // Both prerelease and release endpoints return an object with "message" on error
    if (doc.isObject() && doc.object().contains(qsl("message"))) {
        const QString apiMessage = doc.object().value(qsl("message")).toString();
        qWarning() << "GitHub API error:" << apiMessage;
        if (apiMessage.contains(qsl("rate limit"), Qt::CaseInsensitive)) {
            //: Error shown when the GitHub API rate limit has been exceeded
            emit loadError(tr("Update check temporarily unavailable. Please try again in a few minutes."));
        } else {
            //: Error shown when the GitHub API returns an error. %1 is the error message from the server.
            emit loadError(tr("Could not check for updates: %1").arg(apiMessage));
        }
        return;
    }

    // Both channels use the releases list endpoint and return an array
    if (!doc.isArray()) {
        //: Error shown when the update server response cannot be understood
        emit loadError(tr("Could not read update information from the server"));
        return;
    }

    const QJsonArray releasesArray = doc.array();
    for (const auto& val : releasesArray) {
        const QJsonObject releaseObj = val.toObject();

        // PTB channel: include only prereleases; stable channel: exclude prereleases
        if (mPrerelease != releaseObj.value(qsl("prerelease")).toBool()) {
            continue;
        }
        if (releaseObj.value(qsl("draft")).toBool()) {
            continue;
        }

        Release rel(releaseObj, mOs, mArch);
        if (!rel.getVersion().isEmpty()) {
            mReleases << rel;
        } else {
            qWarning() << "Skipping release with empty version, tag_name:" << releaseObj.value(qsl("tag_name")).toString();
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
        // handle compound extensions like .AppImage.tar when generating unique temp filenames
        static const QRegularExpression extensionRx(qsl("(?:\\.tar)?\\.[a-zA-Z0-9]+$"));
        int extensionPos = fileName.indexOf(extensionRx);
        if (extensionPos > -1) {
            fileName.insert(extensionPos, qsl("-XXXXXX"));
        }
        mDownloadFile = new QTemporaryFile(QDir::tempPath() + qsl("/") + fileName);
        if (!mDownloadFile->open()) {
            qWarning() << "Failed to create temporary file for download:" << mDownloadFile->errorString();
            //: Error shown when a temporary file cannot be created for the update download. %1 is the system error message.
            emit downloadError(tr("Could not create temporary file for download: %1").arg(mDownloadFile->errorString()));
            abortDownload();
            return;
        }
    }
    const QByteArray data = mDownloadReply->readAll();
    const qint64 bytesWritten = mDownloadFile->write(data);
    if (bytesWritten != data.size()) {
        qWarning() << "Failed to write download data to temporary file:" << mDownloadFile->errorString();
        //: Error shown when writing download data to disk fails. %1 is the system error message.
        emit downloadError(tr("Failed to save download data: %1").arg(mDownloadFile->errorString()));
        abortDownload();
        return;
    }
}

void Feed::handleDownloadFinished()
{
    if (mDownloadReply->error() != QNetworkReply::NoError) {
        qWarning() << "Download failed:" << mDownloadReply->errorString() << "URL:" << mDownloadReply->url();
        //: Error shown when the update file download fails. %1 is the network error message.
        emit downloadError(tr("Download failed: %1").arg(mDownloadReply->errorString()));
        cleanupDownloadFile();
        cleanupDownloadReply();
        return;
    }

    if (mDownloadFile == nullptr) {
        //: Error shown when the update download completed but nothing was received
        emit downloadError(tr("Download failed. Please try again."));
        cleanupDownloadReply();
        return;
    }

    if (!mDownloadFile->flush()) {
        qWarning() << "Failed to flush download file:" << mDownloadFile->errorString();
        //: Error shown when flushing the downloaded file to disk fails. %1 is the system error message.
        emit downloadError(tr("Failed to save download: %1").arg(mDownloadFile->errorString()));
        cleanupDownloadFile();
        cleanupDownloadReply();
        return;
    }
    if (!mDownloadFile->seek(0)) {
        qWarning() << "Failed to seek in download file:" << mDownloadFile->errorString();
        //: Error shown when the downloaded file cannot be read back for checksum verification
        emit downloadError(tr("Failed to verify download integrity"));
        cleanupDownloadFile();
        cleanupDownloadReply();
        return;
    }
    QCryptographicHash fileHash(QCryptographicHash::Sha256);
    if (!fileHash.addData(mDownloadFile)) {
        qWarning() << "Failed to read download file for checksum verification:" << mDownloadFile->errorString();
        //: Error shown when the downloaded file cannot be read back for checksum verification
        emit downloadError(tr("Failed to verify download integrity"));
        cleanupDownloadFile();
        cleanupDownloadReply();
        return;
    }
    const QString hashResult = fileHash.result().toHex();
    if (!mCurrentDownload.getDownloadSHA256().isEmpty() && hashResult.toLower() != mCurrentDownload.getDownloadSHA256().toLower()) {
        qWarning() << "SHA256 mismatch - expected:" << mCurrentDownload.getDownloadSHA256() << "got:" << hashResult;
        //: Error shown when the downloaded file's SHA256 checksum does not match the expected value
        emit downloadError(tr("Could not verify download integrity."));
        cleanupDownloadFile();
        cleanupDownloadReply();
        return;
    }

    mDownloadFile->setAutoRemove(false);
    mDownloadFilePath = mDownloadFile->fileName();
    cleanupDownloadFile();
    cleanupDownloadReply();
    emit downloadFinished();
}

void Feed::abortDownload()
{
    if (mDownloadReply) {
        disconnect(mDownloadReply, &QNetworkReply::finished, this, &Feed::handleDownloadFinished);
        mDownloadReply->abort();
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
    }
    cleanupDownloadFile();
}

void Feed::cleanupDownloadReply()
{
    if (mDownloadReply) {
        mDownloadReply->deleteLater();
        mDownloadReply = nullptr;
    }
}

void Feed::cleanupDownloadFile()
{
    if (mDownloadFile) {
        mDownloadFile->close();
        mDownloadFile->deleteLater();
        mDownloadFile = nullptr;
    }
}

} // namespace dblsqd
