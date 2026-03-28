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
        mUrl = QUrl(qsl("https://api.github.com/repos/%1/%2/releases/tags/public-test-build").arg(owner, repo));
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

void Feed::load()
{
    if (mFeedReply != nullptr && !mFeedReply->isFinished()) {
        qWarning() << "Update check already in progress, ignoring duplicate request";
        //: Error shown when the user triggers an update check while one is already running
        emit loadError(tr("Update check already in progress"));
        return;
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
        //: Error shown when a manual update cannot be verified as safe to install
        emit downloadError(tr("Could not verify the download is safe. Please try again later."));
    } else {
        makeDownloadRequest(downloadUrl);
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
            if (mRequireChecksums) {
                reply->deleteLater();
                //: Error shown when a manual update cannot be verified as safe to install
                emit downloadError(tr("Could not verify the download is safe. Please try again later."));
                return;
            }
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
                if (mRequireChecksums) {
                    reply->deleteLater();
                    //: Error shown when a manual update cannot be verified as safe to install
                    emit downloadError(tr("Could not verify the download is safe. Please try again later."));
                    return;
                }
                qCritical() << "Checksum file downloaded but no matching hash found for" << mCurrentDownload.getDownloadUrl().fileName() << "- download will proceed without integrity verification";
            }
        }
        reply->deleteLater();
        makeDownloadRequest(mCurrentDownload.getDownloadUrl());
    });
}

void Feed::makeDownloadRequest(const QUrl& url)
{
    mDownloadFilePath.clear();

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

    if (mPrerelease) {
        // PTB channel: single release object from /releases/tags/public-test-build
        if (doc.isObject()) {
            const QJsonObject releaseObj = doc.object();
            if (!releaseObj.value(qsl("draft")).toBool()) {
                Release rel(releaseObj, mOs, mArch);
                if (!rel.getVersion().isEmpty()) {
                    mReleases << rel;
                }
            }
        } else {
            //: Error shown when the update server response cannot be understood
            emit loadError(tr("Could not read update information from the server"));
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

                Release rel(releaseObj, mOs, mArch);
                if (!rel.getVersion().isEmpty()) {
                    mReleases << rel;
                }
            }
        } else {
            //: Error shown when the update server response cannot be understood
            emit loadError(tr("Could not read update information from the server"));
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
        // handle compound extensions like .tar.gz when generating unique temp filenames
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
        emit downloadError(mDownloadReply->errorString());
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
        delete mDownloadFile;
        mDownloadFile = nullptr;
    }
}

} // namespace dblsqd
