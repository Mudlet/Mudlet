/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "TMedia.h"

#include "pre_guard.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaPlaylist>
#include <QNetworkDiskCache>
#include <QRandomGenerator>
#include <QStandardPaths>
#include "post_guard.h"

// Public
TMedia::TMedia(Host* pHost, const QString& profileName)
{
    mpHost = pHost;
    mProfileName = profileName;

    mpNetworkAccessManager = new QNetworkAccessManager(this);
    connect(mpNetworkAccessManager, &QNetworkAccessManager::finished, this, &TMedia::slot_writeFile);
}

void TMedia::playMedia(TMediaData& mediaData)
{
    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    mediaData.setMediaFileName(mediaData.getMediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/')));

    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && mediaData.getMediaFileName() == qsl("Off")) {
        return;
    }

    bool fileRelative = TMedia::isFileRelative(mediaData);

    if (!fileRelative && (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP)) {
        return; // MSP and MCMP files will not have absolute paths. Something is wrong.
    }

    if (!mediaData.getMediaFileName().contains('*') && !mediaData.getMediaFileName().contains('?')) { // File path wildcards are * and ?
        if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mediaData.getMediaFileName().contains('.')) {
            switch (mediaData.getMediaType()) {
            case TMediaData::MediaTypeSound:
                mediaData.setMediaFileName(mediaData.getMediaFileName().append(".wav"));
                break;
            case TMediaData::MediaTypeMusic:
                mediaData.setMediaFileName(mediaData.getMediaFileName().append(".mid"));
                break;
            }
        }

        if (!fileRelative) { // API files may start as absolute, but get copied into the media folder for processing.
            TMedia::transitionNonRelativeFile(mediaData);
        }

        QString absolutePathFileName = TMedia::setupMediaAbsolutePathFileName(mediaData);
        QFile mediaFile(absolutePathFileName);

        if (!mediaFile.exists()) {
            if (fileRelative) {
                if (!TMedia::processUrl(mediaData)) {
                    return;
                }

                TMedia::downloadFile(mediaData);
            }

            return;
        }

        if (mediaData.getMediaVolume() == TMediaData::MediaVolumePreload) {
            return; // A "feature", primarily for MSP & API, to enable volume 0 (zero) to preload files (already loaded above).  Processing complete.  Exit!
        }
    }

    TMedia::play(mediaData);
}

void TMedia::stopMedia(TMediaData& mediaData)
{
    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    QList<TMediaPlayer> mTMediaPlayerList;

    switch (mediaData.getMediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mMSPSoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mMSPMusicList;
            break;
        }
        break;

    case TMediaData::MediaProtocolGMCP:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mGMCPSoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mGMCPMusicList;
            break;
        case TMediaData::MediaTypeVideo:
            mTMediaPlayerList = mGMCPVideoList;
            break;
        case TMediaData::MediaTypeNotSet:
            mTMediaPlayerList = (mGMCPSoundList + mGMCPMusicList + mGMCPVideoList);
            break;
        }
        break;


    case TMediaData::MediaProtocolAPI:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mAPISoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mAPIMusicList;
            break;
        case TMediaData::MediaTypeVideo:
            mTMediaPlayerList = mAPIVideoList;
            break;
        case TMediaData::MediaTypeNotSet:
            mTMediaPlayerList = (mAPISoundList + mAPIMusicList + mAPIVideoList);
            break;
        }
        break;

    default:
        return;
    }

    if (!mediaData.getMediaFileName().isEmpty()) {
        bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and MCMP files will not have absolute paths. Something is wrong.
        }

        // API files may start as absolute, but get copied into the media folder for processing. Trim the path from the file name.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.getMediaFileName().section('/', -1));
        }
    }

    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer pPlayer = itTMediaPlayer.next();

        if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.getMediaProtocol() == TMediaData::MediaProtocolAPI) {
            if (!mediaData.getMediaKey().isEmpty() && !pPlayer.getMediaData().getMediaKey().isEmpty() && pPlayer.getMediaData().getMediaKey() != mediaData.getMediaKey()) {
                continue;
            }

            if (!mediaData.getMediaFileName().isEmpty() && !pPlayer.getMediaData().getMediaFileName().isEmpty() && pPlayer.getMediaData().getMediaFileName() != mediaData.getMediaFileName()) {
                continue;
            }

            if (!mediaData.getMediaTag().isEmpty() && !pPlayer.getMediaData().getMediaTag().isEmpty() && pPlayer.getMediaData().getMediaTag() != mediaData.getMediaTag()) {
                continue;
            }

            if (mediaData.getMediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer.getMediaData().getMediaPriority() != TMediaData::MediaPriorityNotSet
                && pPlayer.getMediaData().getMediaPriority() >= mediaData.getMediaPriority()) {
                continue;
            }
        }

        pPlayer.getMediaPlayer()->stop();
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#Client.Media
void TMedia::parseGMCP(QString& packageMessage, QString& gmcp)
{
    if (!mpHost->mAcceptServerMedia) {
        return;
    }

    auto document = QJsonDocument::fromJson(gmcp.toUtf8());

    if (!document.isObject()) {
        return;
    }

    // This is JSON
    auto json = document.object();

    QString package = packageMessage.toLower(); // Don't change original variable

    if (package == "client.media.stop") {
        TMedia::parseJSONForMediaStop(json);
        return;
    }

    if (json.isEmpty()) {
        return;
    }

    if (package == "client.media.default" || package == "client.media") { // Client.Media obsolete
        TMedia::parseJSONForMediaDefault(json);
    } else if (package == "client.media.load") {
        TMedia::parseJSONForMediaLoad(json);
    } else if (package == "client.media.play") {
        TMedia::parseJSONForMediaPlay(json);
    }
}

bool TMedia::purgeMediaCache()
{
    QString mediaPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << qsl("TMedia::purgeMediaCache() WARNING - not able to reference directory: %1").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
        return false;
    }

    stopAllMediaPlayers();
    mediaDir.removeRecursively();
    return true;
}
// End Public

// Private
void TMedia::stopAllMediaPlayers()
{
    QList<TMediaPlayer> mTMediaPlayerList = (mMSPSoundList + mMSPMusicList + mGMCPSoundList + mGMCPMusicList + mGMCPVideoList + mAPISoundList + mAPIMusicList + mAPIVideoList);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer pPlayer = itTMediaPlayer.next();
        pPlayer.getMediaPlayer()->stop();
    }
}

void TMedia::transitionNonRelativeFile(TMediaData& mediaData)
{
    QString mediaPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << qsl("TMedia::playMedia() WARNING - attempt made to create a directory failed: %1").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
    } else {
        QString mediaFilePath = qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName().section('/', -1));
        QFile mediaFile(mediaFilePath);

        if (!mediaFile.exists() && !QFile::copy(mediaData.getMediaFileName(), mediaFilePath)) {
            qWarning() << qsl("TMedia::playMedia() WARNING - attempt made to copy file %1 to a directory %2 failed.").arg(mediaData.getMediaFileName(), mediaFilePath);
        } else {
            mediaData.setMediaFileName(mediaData.getMediaFileName().section('/', -1));
        }
    }
}

QUrl TMedia::parseUrl(TMediaData& mediaData)
{
    QUrl url;

    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && mediaData.getMediaFileName() == qsl("Off")) {
        if (mediaData.getMediaUrl().isEmpty()) { // MSP is !!SOUND(Off) or !!MUSIC(Off)
            mpHost->mpMedia->stopMedia(mediaData);
        } else { // MSP is !!SOUND(Off U=https://example.com/sounds) or !!MUSIC(Off U=https://example.com/sounds)
            url = QUrl::fromUserInput(mediaData.getMediaUrl());
        }
    } else if (mediaData.getMediaUrl().isEmpty()) {
        if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->getMediaLocationMSP().isEmpty()) {
            url = QUrl::fromUserInput(mpHost->getMediaLocationMSP());
        } else if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->getMediaLocationGMCP().isEmpty()) {
            url = QUrl::fromUserInput(mpHost->getMediaLocationGMCP());
        } else {
            url = QUrl::fromUserInput(qsl("https://www.%1/media/").arg(mpHost->mUrl));
        }
    } else {
        url = QUrl::fromUserInput(mediaData.getMediaUrl());
    }

    return url;
}

bool TMedia::isValidUrl(QUrl& url)
{
    bool isValid = false;

    if (!url.isValid()) {
        qWarning() << qsl("TMedia::validUrl() WARNING - attempt made to reference an invalid URL: %1 and the error message was:\"%2\".").arg(url.toString(), url.errorString());
    } else {
        isValid = true;
    }

    return isValid;
}

bool TMedia::isFileRelative(TMediaData& mediaData)
{
    bool isFileRelative = false;

    if (!QFileInfo(mediaData.getMediaFileName()).isRelative()) {
        if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP) {
            qWarning()
                    << qsl("TMedia::isFileRelative() WARNING - attempt made to send an absolute path as a media file name: %1.  Only relative paths are permitted.").arg(mediaData.getMediaFileName());
        }
    } else {
        isFileRelative = true;
    }

    return isFileRelative;
}

QStringList TMedia::parseFileNameList(TMediaData& mediaData, QDir& dir)
{
    QStringList fileNameList;

    // No more than one '*' wildcard per the specification
    if ((mediaData.getMediaFileName().contains('*') || mediaData.getMediaFileName().contains('?')) && mediaData.getMediaFileName().count('*') < 2) {
        if (!mediaData.getMediaFileName().contains('/')) {
            dir.setNameFilters(QStringList() << mediaData.getMediaFileName());
        } else { // Directory information needs filtered from the filter
            dir.setNameFilters(QStringList() << mediaData.getMediaFileName().section('/', -1));
        }

        QStringList fileNames(dir.entryList(QDir::Files | QDir::Readable, QDir::Name));

        for (auto& fileName : qAsConst(fileNames)) {
            fileNameList << qsl("%1/%2").arg(dir.path(), fileName);
        }
    } else {
        if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mediaData.getMediaFileName().contains('.')) {
            switch (mediaData.getMediaType()) {
            case TMediaData::MediaTypeSound:
                mediaData.setMediaFileName(mediaData.getMediaFileName().append(".wav"));
                break;
            case TMediaData::MediaTypeMusic:
                mediaData.setMediaFileName(mediaData.getMediaFileName().append(".mid"));
                break;
            }
        }

        fileNameList << qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName());
    }

    return fileNameList;
}

QStringList TMedia::getFileNameList(TMediaData& mediaData)
{
    QStringList fileNameList;

    QString mediaPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << qsl("TMedia::getFileNameList() WARNING - attempt made to create a directory failed: %1").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
        return fileNameList;
    }

    if (!mediaData.getMediaFileName().isEmpty() && mediaData.getMediaFileName().contains('/')) {
        QString mediaSubPath = qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName().section('/', 0, -2));
        QDir mediaSubDir(mediaSubPath);

        if (!mediaSubDir.mkpath(mediaSubPath)) {
            qWarning() << qsl("TMedia::getFileNameList() WARNING - attempt made to create a directory failed: %1")
                                  .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName().section('/', 0, -2));
            return fileNameList;
        }

        fileNameList = TMedia::parseFileNameList(mediaData, mediaSubDir);
    }

    // If we did declare a sub directory, but didn't find the file there, we'll try a search in the root directory per the specification.
    if (fileNameList.isEmpty()) {
        fileNameList = TMedia::parseFileNameList(mediaData, mediaDir);
    }

    return fileNameList;
}

QUrl TMedia::getFileUrl(TMediaData& mediaData)
{
    QUrl fileUrl;

    QString mediaLocation = QString();

    switch (mediaData.getMediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        mediaLocation = mpHost->getMediaLocationMSP();
        break;

    case TMediaData::MediaProtocolGMCP:
        mediaLocation = mpHost->getMediaLocationGMCP();
        break;

    case TMediaData::MediaProtocolAPI:
        mediaLocation = mediaData.getMediaUrl();
        break;
    }

    if (!mediaLocation.isEmpty()) {
        bool endsWithSlash = mediaLocation.endsWith('/');

        if (!endsWithSlash) {
            fileUrl = QUrl::fromUserInput(qsl("%1/%2").arg(mediaLocation, mediaData.getMediaFileName()));
        } else {
            fileUrl = QUrl::fromUserInput(qsl("%1%2").arg(mediaLocation, mediaData.getMediaFileName()));
        }
    }

    return fileUrl;
}

bool TMedia::processUrl(TMediaData& mediaData)
{
    bool continueProcessing = true;

    QUrl url = TMedia::parseUrl(mediaData);

    if (!TMedia::isValidUrl(url)) {
        continueProcessing = false;
    } else {
        switch (mediaData.getMediaProtocol()) {
        case TMediaData::MediaProtocolMSP:
            if (mpHost->getMediaLocationMSP().isEmpty() || url.toString() != mpHost->getMediaLocationMSP()) {
                mpHost->setMediaLocationMSP(url.toString());
            }
            break;

        case TMediaData::MediaProtocolGMCP:
            if (mpHost->getMediaLocationGMCP().isEmpty() || url.toString() != mpHost->getMediaLocationGMCP()) {
                mpHost->setMediaLocationGMCP(url.toString());
            }
            break;

        case TMediaData::MediaProtocolAPI:
            break;

        default:
            continueProcessing = false;
        }
    }

    return continueProcessing;
}

void TMedia::slot_writeFile(QNetworkReply* reply)
{
    TEvent event{};
    TMediaData mediaData = mMediaDownloads.value(reply);
    mMediaDownloads.remove(reply);

    if (reply->error() != QNetworkReply::NoError) {
        event.mArgumentList << qsl("sysDownloadError");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->errorString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

        reply->deleteLater();
        mpHost->raiseEvent(event);
    } else {
        QFile localFile(mediaData.getMediaAbsolutePathFileName());

        if (!localFile.open(QFile::WriteOnly)) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't open the destination file for writing (permission errors?)");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

            reply->deleteLater();
            mpHost->raiseEvent(event);
        }

        qint64 bytesWritten = localFile.write(reply->readAll());

        if (bytesWritten == -1) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't write downloaded content into the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

            reply->deleteLater();
            mpHost->raiseEvent(event);
        } else {
            localFile.flush();

            if (localFile.error() == QFile::NoError) {
                event.mArgumentList << QLatin1String("sysDownloadDone");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << QString::number(bytesWritten);
                event.mArgumentTypeList << ARGUMENT_TYPE_NUMBER;

                localFile.close();

                reply->deleteLater();
                mpHost->raiseEvent(event);

                TMedia::play(mediaData);
            } else {
                event.mArgumentList << QLatin1String("sysDownloadError");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << QLatin1String("Couldn't save to the destination file");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << localFile.errorString();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

                reply->deleteLater();
                mpHost->raiseEvent(event);
            }
        }
    }
}

void TMedia::downloadFile(TMediaData& mediaData)
{
    QString mediaPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << qsl("TMedia::downloadFile() WARNING - attempt made to create a directory failed: %1").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
        return;
    }

    if (!mediaData.getMediaFileName().isEmpty() && mediaData.getMediaFileName().contains('/')) {
        QString mediaSubPath = qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName().section('/', 0, -2));
        QDir mediaSubDir(mediaSubPath);

        if (!mediaSubDir.mkpath(mediaSubPath)) {
            qWarning() << qsl("TMedia::downloadFile() WARNING - attempt made to create a directory failed: %1")
                                  .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName().section('/', 0, -2));
            return;
        }
    }

    QDir dir;
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    if (!dir.mkpath(cacheDir)) {
        qWarning() << "TMedia::downloadFile() WARNING - couldn't create cache directory for sound file(s): " << cacheDir;
        return;
    }

    QUrl fileUrl = getFileUrl(mediaData);

    if (!TMedia::isValidUrl(fileUrl)) {
        return;
    } else {
        auto diskCache = new QNetworkDiskCache(this);
        diskCache->setCacheDirectory(cacheDir);
        mpNetworkAccessManager->setCache(diskCache);

        QNetworkRequest request = QNetworkRequest(fileUrl);
        request.setRawHeader(QByteArray("User-Agent"), QByteArray(qsl("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, APP_BUILD).toUtf8().constData()));
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
#if !defined(QT_NO_SSL)
        if (fileUrl.scheme() == qsl("https")) {
            QSslConfiguration config(QSslConfiguration::defaultConfiguration());
            request.setSslConfiguration(config);
        }
#endif
        mpHost->updateProxySettings(mpNetworkAccessManager);
        QNetworkReply* getReply = mpNetworkAccessManager->get(request);
        mMediaDownloads.insert(getReply, mediaData);
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
        connect(getReply, &QNetworkReply::errorOccurred, this, [=](QNetworkReply::NetworkError) {
#else
        connect(getReply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), this, [=](QNetworkReply::NetworkError) {
#endif
            qWarning() << "TMedia::downloadFile() WARNING - couldn't download sound from " << fileUrl.url();
            getReply->deleteLater();
        });
    }
}

QString TMedia::setupMediaAbsolutePathFileName(TMediaData& mediaData)
{
    QString absolutePathFileName;

    absolutePathFileName = qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName());

    mediaData.setMediaAbsolutePathFileName(absolutePathFileName);

    return absolutePathFileName;
}

QList<TMediaPlayer> TMedia::getMediaPlayerList(TMediaData& mediaData)
{
    QList<TMediaPlayer> mTMediaPlayerList;

    switch (mediaData.getMediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mMSPSoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mMSPMusicList;
            break;
        }
        break;

    case TMediaData::MediaProtocolGMCP:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mGMCPSoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mGMCPMusicList;
            break;
        case TMediaData::MediaTypeVideo:
            mTMediaPlayerList = mGMCPVideoList;
            break;
        }
        break;

    case TMediaData::MediaProtocolAPI:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mAPISoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mAPIMusicList;
            break;
        case TMediaData::MediaTypeVideo:
            mTMediaPlayerList = mAPIVideoList;
            break;
        }
        break;
    }

    return mTMediaPlayerList;
}

void TMedia::updateMediaPlayerList(TMediaPlayer& player)
{
    int matchedMediaPlayerIndex = -1;
    TMediaData mediaData = player.getMediaData();
    QList<TMediaPlayer> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);

    for (int i = 0; i < mTMediaPlayerList.size(); ++i) {
        TMediaPlayer pTestPlayer = mTMediaPlayerList.at(i);

        if (pTestPlayer.getMediaPlayer() == player.getMediaPlayer()) {
            matchedMediaPlayerIndex = i;
            break;
        }
    }

    switch (mediaData.getMediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            if (matchedMediaPlayerIndex == -1) {
                mMSPSoundList.append(player);
            } else {
                mMSPSoundList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        case TMediaData::MediaTypeMusic:
            if (matchedMediaPlayerIndex == -1) {
                mMSPMusicList.append(player);
            } else {
                mMSPMusicList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        }
        break;

    case TMediaData::MediaProtocolGMCP:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            if (matchedMediaPlayerIndex == -1) {
                mGMCPSoundList.append(player);
            } else {
                mGMCPSoundList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        case TMediaData::MediaTypeMusic:
            if (matchedMediaPlayerIndex == -1) {
                mGMCPMusicList.append(player);
            } else {
                mGMCPMusicList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        case TMediaData::MediaTypeVideo:
            if (matchedMediaPlayerIndex == -1) {
                mGMCPVideoList.append(player);
            } else {
                mGMCPVideoList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        }
        break;

    case TMediaData::MediaProtocolAPI:
        switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            if (matchedMediaPlayerIndex == -1) {
                mAPISoundList.append(player);
            } else {
                mAPISoundList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        case TMediaData::MediaTypeMusic:
            if (matchedMediaPlayerIndex == -1) {
                mAPIMusicList.append(player);
            } else {
                mAPIMusicList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        case TMediaData::MediaTypeVideo:
            if (matchedMediaPlayerIndex == -1) {
                mAPIVideoList.append(player);
            } else {
                mAPIVideoList.replace(matchedMediaPlayerIndex, player);
            }
            break;
        }
        break;
    }
}

TMediaPlayer TMedia::getMediaPlayer(TMediaData& mediaData)
{
    TMediaPlayer pPlayer{};
    QList<TMediaPlayer> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) { // Find first available inactive QMediaPlayer
        TMediaPlayer pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getMediaPlayer()->state() != QMediaPlayer::PlayingState && pTestPlayer.getMediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            pPlayer = pTestPlayer;
            pPlayer.setMediaData(mediaData);
            break;
        }
    }

    if (!pPlayer.isInitialized()) { // No available QMediaPlayer, create a new one.
        pPlayer = TMediaPlayer(mpHost, mediaData);

        if (!pPlayer.getMediaPlayer()) { // It should be impossible to ever reach this.
            qWarning() << qsl("TMedia::getMediaPlayer() WARNING - unable to create new QMediaPlayer object.");
            return pPlayer;
        }
    }

    disconnect(pPlayer.getMediaPlayer(), &QMediaPlayer::stateChanged, nullptr, nullptr);
    disconnect(pPlayer.getMediaPlayer(), &QMediaPlayer::positionChanged, nullptr, nullptr);

    connect(pPlayer.getMediaPlayer(), &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            TEvent mediaFinished{};
            mediaFinished.mArgumentList.append("sysMediaFinished");
            mediaFinished.mArgumentList.append(pPlayer.getMediaPlayer()->media().request().url().fileName());
            mediaFinished.mArgumentList.append(pPlayer.getMediaPlayer()->media().request().url().path());
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            if (mpHost) {
                // The host may have gone away if the sound was a long one
                // and we are multi-playing so we ought to test it...
                mpHost->raiseEvent(mediaFinished);
            }
        }
    });

    connect(pPlayer.getMediaPlayer(), &QMediaPlayer::positionChanged, [=](qint64 progress) {
        int volume = pPlayer.getMediaData().getMediaVolume();
        int fadeInPosition = pPlayer.getMediaData().getMediaFadeIn();
        int fadeOutPosition = pPlayer.getMediaData().getMediaFadeOut();
        int startPosition = pPlayer.getMediaData().getMediaStart();
        bool fadeInUsed = fadeInPosition != TMediaData::MediaFadeNotSet;
        bool fadeOutUsed = fadeOutPosition != TMediaData::MediaFadeNotSet;
        bool actionTaken = false;

        if (fadeInUsed) {
            if (progress < fadeInPosition) {
                double fadeInVolume = static_cast<double>(volume * (progress - startPosition)) / static_cast<double>((fadeInPosition - startPosition) * 1.0);

                pPlayer.getMediaPlayer()->setVolume(qRound(fadeInVolume));
                actionTaken = true;
            } else if (progress == fadeInPosition) {
                pPlayer.getMediaPlayer()->setVolume(volume);
                actionTaken = true;
            }
        }

        if (!actionTaken && fadeOutUsed && progress > 0) {
            int duration = pPlayer.getMediaPlayer()->duration();

            if (progress > duration - fadeOutPosition) {
                double fadeOutVolume = static_cast<double>(volume * (duration - progress)) / static_cast<double>(fadeOutPosition * 1.0);

                pPlayer.getMediaPlayer()->setVolume(qRound(fadeOutVolume));
                actionTaken = true;
            }
        }

        if (!actionTaken && ((fadeInUsed && progress > fadeInPosition) || (fadeOutUsed && progress < fadeOutPosition))) {
            pPlayer.getMediaPlayer()->setVolume(volume); // Added to support multiple continue = true calls of same music
        }
    });

    return pPlayer;
}

TMediaPlayer TMedia::matchMediaPlayer(TMediaData& mediaData, const QString& absolutePathFileName)
{
    TMediaPlayer pPlayer{};
    QList<TMediaPlayer> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getMediaPlayer()->state() == QMediaPlayer::PlayingState && pTestPlayer.getMediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (pTestPlayer.getMediaData().getMediaAbsolutePathFileName().endsWith(absolutePathFileName)) { // Is the same sound or music playing?
                pPlayer = pTestPlayer;
                pPlayer.setMediaData(mediaData);
                pPlayer.getMediaPlayer()->setVolume(mediaData.getMediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.getMediaVolume());
                break;
            }
        }
    }

    return pPlayer;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#priority:_1_to_100
bool TMedia::doesMediaHavePriorityToPlay(TMediaData& mediaData, const QString& absolutePathFileName)
{
    bool doesMediaHavePriorityToPlay = true;

    if (mediaData.getMediaPriority() == TMediaData::MediaPriorityNotSet) {
        return doesMediaHavePriorityToPlay;
    }

    int maxMediaPriority = 0;

    QList<TMediaPlayer> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) { // Find the maximum priority of all playing sounds
        TMediaPlayer pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getMediaPlayer()->state() == QMediaPlayer::PlayingState && pTestPlayer.getMediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (!pTestPlayer.getMediaData().getMediaAbsolutePathFileName().endsWith(absolutePathFileName)) { // Is it a different sound or music than specified?
                if (pTestPlayer.getMediaData().getMediaPriority() != TMediaData::MediaPriorityNotSet && pTestPlayer.getMediaData().getMediaPriority() > maxMediaPriority) {
                    maxMediaPriority = pTestPlayer.getMediaData().getMediaPriority();
                }
            }
        }
    }

    if (maxMediaPriority >= mediaData.getMediaPriority()) { // Our media has a lower priority
        doesMediaHavePriorityToPlay = false;
    } else {
        TMediaData stopMediaData{};
        stopMediaData.setMediaProtocol(mediaData.getMediaProtocol());
        stopMediaData.setMediaType(mediaData.getMediaType());
        stopMediaData.setMediaPriority(mediaData.getMediaPriority());
        mpHost->mpMedia->stopMedia(stopMediaData); // If we have the highest priority, stop everything else.
    }

    return doesMediaHavePriorityToPlay;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#key
void TMedia::matchMediaKeyAndStopMediaVariants(TMediaData& mediaData, const QString& absolutePathFileName)
{
    QList<TMediaPlayer> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getMediaPlayer()->state() == QMediaPlayer::PlayingState && pTestPlayer.getMediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (!mediaData.getMediaKey().isEmpty() && !pTestPlayer.getMediaData().getMediaKey().isEmpty()
                && mediaData.getMediaKey() == pTestPlayer.getMediaData().getMediaKey()) { // Does it have the same key?
                if (!pTestPlayer.getMediaData().getMediaAbsolutePathFileName().endsWith(absolutePathFileName)
                    || (!mediaData.getMediaUrl().isEmpty() && !pTestPlayer.getMediaData().getMediaUrl().isEmpty()
                        && mediaData.getMediaUrl() != pTestPlayer.getMediaData().getMediaUrl())) { // Is it a different sound or music than specified?
                    TMediaData stopMediaData = pTestPlayer.getMediaData();
                    mpHost->mpMedia->stopMedia(stopMediaData); // Stop it!
                }
            }
        }
    }
}

void TMedia::play(TMediaData& mediaData)
{
    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    QStringList fileNameList = TMedia::getFileNameList(mediaData);

    if (fileNameList.isEmpty()) { // This should not happen.
        qWarning() << qsl("TMedia::play() WARNING - could not generate a list of media file names.");
        return;
    }

    TMediaPlayer pPlayer{};

    if (mediaData.getMediaType() == TMediaData::MediaTypeMusic) {
        pPlayer = TMedia::matchMediaPlayer(mediaData, fileNameList.at(0));
    }

    bool sameMusicIsPlaying = false;

    if (pPlayer.isInitialized()) {
        sameMusicIsPlaying = true; // Same music is playing; Reuse that MediaPlayer
    } else {
        pPlayer = TMedia::getMediaPlayer(mediaData);
    }

    // It should be impossible to ever reach this.
    if (!pPlayer.getMediaPlayer()) {
        qWarning() << qsl("TMedia::play() WARNING - unable to create new QMediaPlayer object.");
        return;
    }

    auto playlist = !sameMusicIsPlaying ? new QMediaPlaylist : (pPlayer.getMediaPlayer()->playlist() != nullptr ? pPlayer.getMediaPlayer()->playlist() : new QMediaPlaylist);
    QString absolutePathFileName;

    if (mediaData.getMediaLoops() == TMediaData::MediaLoopsDefault) { // Play once
        playlist->setPlaybackMode(QMediaPlaylist::Sequential);

        if (sameMusicIsPlaying) {
            if (mediaData.getMediaContinue() == TMediaData::MediaContinueRestart) {
                mpHost->mpMedia->stopMedia(mediaData); // Stop the music; Restart it below.

                if (!playlist->isEmpty()) {
                    playlist->clear();
                }
            } else {
                if (!playlist->isEmpty() && playlist->mediaCount() > 1) { // Purge media from the previous playlist
                    playlist->removeMedia(playlist->nextIndex(), playlist->mediaCount());
                }

                return; // No action required. Continue playing the same music.
            }
        }

        if (fileNameList.size() > 1) {
            absolutePathFileName = fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()));
        } else {
            absolutePathFileName = fileNameList.at(0);
        }

        if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) { // Filter out other media
            return;
        }

        if (!mediaData.getMediaKey().isEmpty() && (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.getMediaProtocol() == TMediaData::MediaProtocolAPI)) {
            TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName); // If mediaKey matches, check for uniqueness.
        }

        pPlayer.getMediaPlayer()->setMedia(QUrl::fromLocalFile(absolutePathFileName));
    } else {
        if (mediaData.getMediaLoops() == TMediaData::MediaLoopsRepeat) { // Repeat indefinitely
            playlist->setPlaybackMode(QMediaPlaylist::Loop);

            if (sameMusicIsPlaying) {
                if (mediaData.getMediaContinue() == TMediaData::MediaContinueRestart) {
                    mpHost->mpMedia->stopMedia(mediaData); // Stop the music; Restart it below.

                    if (!playlist->isEmpty()) {
                        playlist->clear();
                    }
                } else {
                    return; // No action required. Continue playing the same music.
                }
            }

            if (fileNameList.size() > 1) {
                absolutePathFileName = fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()));
            } else {
                absolutePathFileName = fileNameList.at(0);
            }

            if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) { // Filter out other media
                return;
            }

            if (!mediaData.getMediaKey().isEmpty() && (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.getMediaProtocol() == TMediaData::MediaProtocolAPI)) {
                TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName); // If mediaKey matches, check for uniqueness.
            }

            playlist->addMedia(QUrl::fromLocalFile(absolutePathFileName));
        } else {
            playlist->setPlaybackMode(QMediaPlaylist::Sequential);

            if (sameMusicIsPlaying) {
                if (mediaData.getMediaContinue() == TMediaData::MediaContinueRestart) {
                    mpHost->mpMedia->stopMedia(mediaData); // Stop the music; Restart it below.

                    if (!playlist->isEmpty()) {
                        playlist->clear();
                    }
                } else {
                    if (!playlist->isEmpty() && playlist->mediaCount() > 1) { // Purge media from the previous playlist
                        playlist->removeMedia(playlist->nextIndex(), playlist->mediaCount());
                    }

                    mediaData.setMediaLoops(mediaData.getMediaLoops() - 1); // Subtract the currently playing music from the total
                }
            }

            for (int k = 0; k < mediaData.getMediaLoops(); k++) { // Repeat a finite number of times
                if (fileNameList.size() > 1) {
                    absolutePathFileName = fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()));
                } else {
                    absolutePathFileName = fileNameList.at(0);
                }

                if (k < 1) {
                    if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) { // Filter out other sound
                        return;
                    }

                    if (!mediaData.getMediaKey().isEmpty() && (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.getMediaProtocol() == TMediaData::MediaProtocolAPI)) {
                        TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName); // If mediaKey matches, check for uniqueness.
                    }
                }

                playlist->addMedia(QUrl::fromLocalFile(absolutePathFileName));
            }
        }

        if (sameMusicIsPlaying && mediaData.getMediaContinue() == TMediaData::MediaContinueDefault) {
            return;
        }

        playlist->setCurrentIndex(1);
        pPlayer.getMediaPlayer()->setPlaylist(playlist);
    }

    // Set volume, start and play media
    pPlayer.getMediaPlayer()->setVolume(mediaData.getMediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.getMediaVolume());
    pPlayer.getMediaPlayer()->setPosition(mediaData.getMediaStart());

    if (mediaData.getMediaFadeIn() != TMediaData::MediaFadeNotSet || mediaData.getMediaFadeOut() != TMediaData::MediaFadeNotSet) {
        pPlayer.getMediaPlayer()->setNotifyInterval(50); // Smoother volume changes with the tighter interval (default = 1000).
    }

    if (mediaData.getMediaType() == TMediaData::MediaTypeVideo) {
        if (!mpHost->mpMedia->mpVideoPlayer.data()) {
            mpHost->showHideOrCreateVideoPlayer();
        }

        auto videoWidget = mpHost->mpMedia->mpVideoPlayer->mpVideoPlayer;
        pPlayer.getMediaPlayer()->setVideoOutput(videoWidget);
        videoWidget->show();
    }

    pPlayer.getMediaPlayer()->play();

    updateMediaPlayerList(pPlayer);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#type:_sound
TMediaData::MediaType TMedia::parseJSONByMediaType(QJsonObject& json)
{
    TMediaData::MediaType mediaType = TMediaData::MediaTypeNotSet;

    auto mediaTypeJSON = json.value(qsl("type"));

    if (mediaTypeJSON != QJsonValue::Undefined && !mediaTypeJSON.toString().isEmpty()) {
        if (mediaTypeJSON.toString().toLower() == "sound") {
            mediaType = TMediaData::MediaTypeSound;
        } else if (mediaTypeJSON.toString().toLower() == "music") {
            mediaType = TMediaData::MediaTypeMusic;
        } else if (mediaTypeJSON.toString().toLower() == "video") {
            mediaType = TMediaData::MediaTypeVideo;
        }
    }

    return mediaType;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#name
QString TMedia::parseJSONByMediaFileName(QJsonObject& json)
{
    QString mediaFileName = QString();

    auto mediaFileNameJSON = json.value(qsl("name"));

    if (mediaFileNameJSON != QJsonValue::Undefined && !mediaFileNameJSON.toString().isEmpty()) {
        mediaFileName = mediaFileNameJSON.toString();
    }

    return mediaFileName;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#volume:_1_to_100
int TMedia::parseJSONByMediaVolume(QJsonObject& json)
{
    int mediaVolume = TMediaData::MediaVolumeDefault;

    auto mediaVolumeJSON = json.value(qsl("volume"));

    if (mediaVolumeJSON != QJsonValue::Undefined && mediaVolumeJSON.isString() && !mediaVolumeJSON.toString().isEmpty()) {
        mediaVolume = mediaVolumeJSON.toString().toInt();
    } else if (mediaVolumeJSON != QJsonValue::Undefined && mediaVolumeJSON.toInt()) {
        mediaVolume = mediaVolumeJSON.toInt();
    }

    return mediaVolume;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#fadein
int TMedia::parseJSONByMediaFadeIn(QJsonObject& json)
{
    int mediaFadeIn = TMediaData::MediaFadeNotSet;

    auto mediaFadeInJSON = json.value(qsl("fadein"));

    if (mediaFadeInJSON != QJsonValue::Undefined && mediaFadeInJSON.isString() && !mediaFadeInJSON.toString().isEmpty()) {
        mediaFadeIn = mediaFadeInJSON.toString().toInt();
    } else if (mediaFadeInJSON != QJsonValue::Undefined && mediaFadeInJSON.toInt()) {
        mediaFadeIn = mediaFadeInJSON.toInt();
    }

    return mediaFadeIn;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#fadeout
int TMedia::parseJSONByMediaFadeOut(QJsonObject& json)
{
    int mediaFadeOut = TMediaData::MediaFadeNotSet;

    auto mediaFadeOutJSON = json.value(qsl("fadeout"));

    if (mediaFadeOutJSON != QJsonValue::Undefined && mediaFadeOutJSON.isString() && !mediaFadeOutJSON.toString().isEmpty()) {
        mediaFadeOut = mediaFadeOutJSON.toString().toInt();
    } else if (mediaFadeOutJSON != QJsonValue::Undefined && mediaFadeOutJSON.toInt()) {
        mediaFadeOut = mediaFadeOutJSON.toInt();
    }

    return mediaFadeOut;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#start
int TMedia::parseJSONByMediaStart(QJsonObject& json)
{
    int mediaStart = TMediaData::MediaStartDefault;

    auto mediaStartJSON = json.value(qsl("start"));

    if (mediaStartJSON != QJsonValue::Undefined && mediaStartJSON.isString() && !mediaStartJSON.toString().isEmpty()) {
        mediaStart = mediaStartJSON.toString().toInt();
    } else if (mediaStartJSON != QJsonValue::Undefined && mediaStartJSON.toInt()) {
        mediaStart = mediaStartJSON.toInt();
    }

    return mediaStart;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#priority:_1_to_100
int TMedia::parseJSONByMediaPriority(QJsonObject& json)
{
    int mediaPriority = TMediaData::MediaPriorityNotSet;

    auto mediaPriorityJSON = json.value(qsl("priority"));

    if (mediaPriorityJSON != QJsonValue::Undefined && mediaPriorityJSON.isString() && !mediaPriorityJSON.toString().isEmpty()) {
        mediaPriority = mediaPriorityJSON.toString().toInt();
    } else if (mediaPriorityJSON != QJsonValue::Undefined && mediaPriorityJSON.toInt()) {
        mediaPriority = mediaPriorityJSON.toInt();
    }

    return mediaPriority;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#loops:_-1.2C_1_or_more
int TMedia::parseJSONByMediaLoops(QJsonObject& json)
{
    int mediaLoops = TMediaData::MediaLoopsDefault;

    auto mediaLoopsJSON = json.value(qsl("loops"));

    if (mediaLoopsJSON != QJsonValue::Undefined && mediaLoopsJSON.isString() && !mediaLoopsJSON.toString().isEmpty()) {
        mediaLoops = mediaLoopsJSON.toString().toInt();
    } else if (mediaLoopsJSON != QJsonValue::Undefined && mediaLoopsJSON.toInt()) {
        mediaLoops = mediaLoopsJSON.toInt();
    }

    return mediaLoops;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#continue:_true_or_false_.28for_music.29
TMediaData::MediaContinue TMedia::parseJSONByMediaContinue(QJsonObject& json)
{
    TMediaData::MediaContinue mediaContinue = TMediaData::MediaContinueDefault;

    auto mediaContinueJSON = json.value(qsl("continue"));

    if (mediaContinueJSON != QJsonValue::Undefined && mediaContinueJSON.isString() && !mediaContinueJSON.toString().isEmpty()) {
        if (mediaContinueJSON.toString() == "false") {
            mediaContinue = TMediaData::MediaContinueRestart;
        } else {
            mediaContinue = TMediaData::MediaContinueDefault;
        }
    } else if (mediaContinueJSON != QJsonValue::Undefined && !mediaContinueJSON.toBool(true)) {
        mediaContinue = TMediaData::MediaContinueRestart;
    }

    return mediaContinue;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#tag
QString TMedia::parseJSONByMediaTag(QJsonObject& json)
{
    QString mediaTag = QString();

    auto mediaTagJSON = json.value(qsl("tag"));

    if (mediaTagJSON != QJsonValue::Undefined && !mediaTagJSON.toString().isEmpty()) {
        mediaTag = mediaTagJSON.toString().toLower(); // To provide case insensitivity of MSP specification
    }

    return mediaTag;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#url
QString TMedia::parseJSONByMediaUrl(QJsonObject& json)
{
    QString mediaUrl = QString();

    auto mediaUrlJSON = json.value(qsl("url"));

    if (mediaUrlJSON != QJsonValue::Undefined && !mediaUrlJSON.toString().isEmpty()) {
        mediaUrl = mediaUrlJSON.toString();
    }

    return mediaUrl;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#key
QString TMedia::parseJSONByMediaKey(QJsonObject& json)
{
    QString mediaKey = QString();

    auto mediaKeyJSON = json.value(qsl("key"));

    if (mediaKeyJSON != QJsonValue::Undefined && !mediaKeyJSON.toString().isEmpty()) {
        mediaKey = mediaKeyJSON.toString();
    }

    return mediaKey;
}
// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#Loading_Media
void TMedia::parseJSONForMediaDefault(QJsonObject& json)
{
    TMediaData mediaData{};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaUrl(TMedia::parseJSONByMediaUrl(json));

    TMedia::processUrl(mediaData);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#Loading_Media
void TMedia::parseJSONForMediaLoad(QJsonObject& json)
{
    TMediaData mediaData{};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaFileName(TMedia::parseJSONByMediaFileName(json)); //Required
    mediaData.setMediaUrl(TMedia::parseJSONByMediaUrl(json));
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

    mediaData.setMediaFileName(mediaData.getMediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/')));

    if (!TMedia::isFileRelative(mediaData)) {
        return;
    }

    if (!TMedia::processUrl(mediaData)) {
        return;
    }

    QString absolutePathFileName = TMedia::setupMediaAbsolutePathFileName(mediaData);

    QFile mediaFile(absolutePathFileName);

    if (!mediaFile.exists()) {
        TMedia::downloadFile(mediaData);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#Playing_Media
void TMedia::parseJSONForMediaPlay(QJsonObject& json)
{
    TMediaData mediaData{};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaType(TMedia::parseJSONByMediaType(json));

    if (mediaData.getMediaType() == TMediaData::MediaTypeNotSet) {
        mediaData.setMediaType(TMediaData::MediaTypeSound);
    }

    mediaData.setMediaFileName(TMedia::parseJSONByMediaFileName(json)); //Required
    mediaData.setMediaUrl(TMedia::parseJSONByMediaUrl(json));
    mediaData.setMediaKey(TMedia::parseJSONByMediaKey(json));
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaVolume(TMedia::parseJSONByMediaVolume(json));
    mediaData.setMediaFadeIn(TMedia::parseJSONByMediaFadeIn(json));
    mediaData.setMediaFadeOut(TMedia::parseJSONByMediaFadeOut(json));
    mediaData.setMediaStart(TMedia::parseJSONByMediaStart(json));
    mediaData.setMediaLoops(TMedia::parseJSONByMediaLoops(json));
    mediaData.setMediaPriority(TMedia::parseJSONByMediaPriority(json));
    mediaData.setMediaContinue(TMedia::parseJSONByMediaContinue(json));

    TMedia::playMedia(mediaData);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#Stopping_Media
void TMedia::parseJSONForMediaStop(QJsonObject& json)
{
    TMediaData mediaData{};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaType(TMedia::parseJSONByMediaType(json));
    mediaData.setMediaFileName(TMedia::parseJSONByMediaFileName(json));
    mediaData.setMediaKey(TMedia::parseJSONByMediaKey(json));
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaPriority(TMedia::parseJSONByMediaPriority(json));

    TMedia::stopMedia(mediaData);
}
// End Private
