/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020, 2022-2024 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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
    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    if (mediaData.mediaInput() == TMediaData::MediaInputNotSet) {
        mediaData.setMediaInput(TMediaData::MediaInputFile);
    }

    mediaData.setMediaFileName(mediaData.mediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/')));

    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && mediaData.mediaFileName() == qsl("Off")) {
        TMedia::processUrl(mediaData);
        return;
    }

    if (mediaData.mediaInput() == TMediaData::MediaInputStream) {
        TMedia::setupMediaAbsolutePathFileName(mediaData);
    } else if (mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and MCMP files will not have absolute paths. Something is wrong.
        }

        if (!mediaData.mediaFileName().contains(QLatin1Char('*')) && !mediaData.mediaFileName().contains(QLatin1Char('?'))) { // File path wildcards are * and ?
            if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mediaData.mediaFileName().contains(QLatin1Char('.'))) {
                switch (mediaData.mediaType()) {
                case TMediaData::MediaTypeSound:
                    mediaData.setMediaFileName(mediaData.mediaFileName().append(".wav"));
                    break;
                case TMediaData::MediaTypeMusic:
                    mediaData.setMediaFileName(mediaData.mediaFileName().append(".mid"));
                    break;
                }
            }

            if (!fileRelative) { // API files may start as absolute, but get copied into the media folder for processing.
                TMedia::transitionNonRelativeFile(mediaData);
            }

            const QString absolutePathFileName = TMedia::setupMediaAbsolutePathFileName(mediaData);
            const QFile mediaFile(absolutePathFileName);

            if (!mediaFile.exists()) {
                if (fileRelative) {
                    if (!TMedia::processUrl(mediaData)) {
                        return;
                    }

                    TMedia::downloadFile(mediaData);
                }

                return;
            }

            if (mediaData.mediaVolume() == TMediaData::MediaVolumePreload) {
                return; // A "feature", primarily for MSP & API, to enable volume 0 (zero) to preload files (already loaded above).  Processing complete.  Exit!
            }
        }
    }

    TMedia::play(mediaData);
}

QList<TMediaData> TMedia::playingMedia(TMediaData& mediaData)
{
    QList<TMediaData> mMatchingTMediaDataList;

    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return mMatchingTMediaDataList;
    }

    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return mMatchingTMediaDataList;
    }

    QList<TMediaPlayer> mTMediaPlayerList;

    switch (mediaData.mediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        switch (mediaData.mediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mMSPSoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mMSPMusicList;
            break;
        }
        break;

    case TMediaData::MediaProtocolGMCP:
        switch (mediaData.mediaType()) {
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
        switch (mediaData.mediaType()) {
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
        return mMatchingTMediaDataList;
    }

    if (!mediaData.mediaFileName().isEmpty()) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return mMatchingTMediaDataList; // MSP and MCMP files will not have absolute paths. Something is wrong.
        }

        // API files may start as absolute, but get copied into the media folder for processing. Trim the path from the file name.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section('/', -1));
        }
    }

    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer const pPlayer = itTMediaPlayer.next();

        if (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI) {
            if (pPlayer.getPlaybackState() != QMediaPlayer::PlayingState && pPlayer.mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
                continue;
            }

            if (!mediaData.mediaKey().isEmpty() && !pPlayer.mediaData().mediaKey().isEmpty() && pPlayer.mediaData().mediaKey() != mediaData.mediaKey()) {
                continue;
            }

            if (!mediaData.mediaFileName().isEmpty() && !pPlayer.mediaData().mediaFileName().isEmpty() && pPlayer.mediaData().mediaFileName() != mediaData.mediaFileName()) {
                continue;
            }

            if (!mediaData.mediaTag().isEmpty() && !pPlayer.mediaData().mediaTag().isEmpty() && pPlayer.mediaData().mediaTag() != mediaData.mediaTag()) {
                continue;
            }

            if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer.mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet
                && pPlayer.mediaData().mediaPriority() > mediaData.mediaPriority()) {
                continue;
            }
       }

        mMatchingTMediaDataList.append(pPlayer.mediaData());
    }

    return mMatchingTMediaDataList;
}

void TMedia::stopMedia(TMediaData& mediaData)
{
    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    QList<TMediaPlayer> mTMediaPlayerList;

    switch (mediaData.mediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        switch (mediaData.mediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mMSPSoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mMSPMusicList;
            break;
        }
        break;

    case TMediaData::MediaProtocolGMCP:
        switch (mediaData.mediaType()) {
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
        switch (mediaData.mediaType()) {
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

    if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and MCMP files will not have absolute paths. Something is wrong.
        }

        // API files may start as absolute, but get copied into the media folder for processing. Trim the path from the file name.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }
    }

    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer const pPlayer = itTMediaPlayer.next();

        if (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI) {
            if (!mediaData.mediaKey().isEmpty() && !pPlayer.mediaData().mediaKey().isEmpty() && pPlayer.mediaData().mediaKey() != mediaData.mediaKey()) {
                continue;
            }

            if (!mediaData.mediaFileName().isEmpty() && !pPlayer.mediaData().mediaFileName().isEmpty() && pPlayer.mediaData().mediaFileName() != mediaData.mediaFileName()) {
                continue;
            }

            if (!mediaData.mediaTag().isEmpty() && !pPlayer.mediaData().mediaTag().isEmpty() && pPlayer.mediaData().mediaTag() != mediaData.mediaTag()) {
                continue;
            }

            if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer.mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet
                && pPlayer.mediaData().mediaPriority() >= mediaData.mediaPriority()) {
                continue;
            }

            if ((mediaData.mediaFadeAway() == TMediaData::MediaFadeAwayEnabled || mediaData.mediaFadeOut() != TMediaData::MediaFadeNotSet)
                && pPlayer.mediaData().mediaEnd() == TMediaData::MediaEndNotSet) {
                const int finishPosition = pPlayer.mediaData().mediaFinish();
                const int duration = pPlayer.mediaPlayer()->duration();
                const int currentPosition = pPlayer.mediaPlayer()->position();
                const int fadeOut = pPlayer.mediaData().mediaFadeOut() ? pPlayer.mediaData().mediaFadeOut() : mediaData.mediaFadeOut();
                const int remainingDuration = (finishPosition != TMediaData::MediaFinishNotSet ? finishPosition : duration) - currentPosition;
                const int endDuration = fadeOut != TMediaData::MediaFadeNotSet ? std::min(remainingDuration, fadeOut) : std::min(remainingDuration, 5000);
                const int endPosition = currentPosition + endDuration;
                TMediaPlayer pUpdatePlayer = pPlayer;
                TMediaData updateMediaData = pUpdatePlayer.mediaData();
                updateMediaData.setMediaFadeOut(endDuration);
                updateMediaData.setMediaEnd(endPosition);
                pUpdatePlayer.setMediaData(updateMediaData);
                TMedia::updateMediaPlayerList(pUpdatePlayer);
                continue;
            }
       }

        pPlayer.mediaPlayer()->stop();
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

    const QString package = packageMessage.toLower(); // Don't change original variable

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

// Documentation: https://wiki.mudlet.org/w/Manual:Miscellaneous_Functions#purgeMediaCache
bool TMedia::purgeMediaCache()
{
    const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
    QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << qsl("TMedia::purgeMediaCache() WARNING - not able to reference directory: %1").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()));
        return false;
    }

    stopAllMediaPlayers();
    mediaDir.removeRecursively();
    return true;
}

void TMedia::muteMedia(const TMediaData::MediaProtocol mediaProtocol)
{
    setMediaPlayersMuted(mediaProtocol, true);
}

void TMedia::unmuteMedia(const TMediaData::MediaProtocol mediaProtocol)
{
    setMediaPlayersMuted(mediaProtocol, false);
}
// End Public

// Private
void TMedia::stopAllMediaPlayers()
{
    QList<TMediaPlayer> const mTMediaPlayerList = (mMSPSoundList + mMSPMusicList + mGMCPSoundList + mGMCPMusicList + mGMCPVideoList + mAPISoundList + mAPIMusicList + mAPIVideoList);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer const pPlayer = itTMediaPlayer.next();
        pPlayer.mediaPlayer()->stop();
    }
}

void TMedia::setMediaPlayersMuted(const TMediaData::MediaProtocol mediaProtocol, const bool state)
{
    QList<TMediaPlayer> mTMediaPlayerList;

    if (mediaProtocol == TMediaData::MediaProtocolMSP) {
        mTMediaPlayerList = mMSPSoundList + mMSPMusicList;
    } else if (mediaProtocol == TMediaData::MediaProtocolGMCP) {
        mTMediaPlayerList = mGMCPSoundList + mGMCPMusicList + mGMCPVideoList;
    } else if (mediaProtocol == TMediaData::MediaProtocolAPI) {
        mTMediaPlayerList = mAPISoundList + mAPIMusicList + mAPIVideoList;
    } else if (mediaProtocol == TMediaData::MediaProtocolNotSet) {
        mTMediaPlayerList = mMSPSoundList + mMSPMusicList + mGMCPSoundList + mGMCPMusicList + mGMCPVideoList + mAPISoundList + mAPIMusicList + mAPIVideoList;
    }

    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        const TMediaPlayer player = itTMediaPlayer.next();

        if (player.mediaPlayer()->audioOutput()) {
            player.mediaPlayer()->audioOutput()->setMuted(state);
        }
    }
}

void TMedia::transitionNonRelativeFile(TMediaData& mediaData)
{
    const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
    const QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << qsl("TMedia::playMedia() WARNING - attempt made to create a directory failed: %1").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()));
    } else {
        const QString mediaFilePath = qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        const QFile mediaFile(mediaFilePath);

        if (!mediaFile.exists() && !QFile::copy(mediaData.mediaFileName(), mediaFilePath)) {
            qWarning() << qsl("TMedia::playMedia() WARNING - attempt made to copy file %1 to a directory %2 failed.").arg(mediaData.mediaFileName(), mediaFilePath);
        } else {
            mediaData.setMediaFileName(mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }
    }
}

QString TMedia::getStreamUrl(const TMediaData& mediaData)
{
    return !mediaData.mediaUrl().endsWith(QLatin1Char('/')) ? qsl("%1/%2").arg(mediaData.mediaUrl(), mediaData.mediaFileName()) : qsl("%1%2").arg(mediaData.mediaUrl(), mediaData.mediaFileName());
}

QUrl TMedia::parseUrl(TMediaData& mediaData)
{
    QUrl url;

    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && mediaData.mediaFileName() == qsl("Off")) {
        if (mediaData.mediaUrl().isEmpty()) { // MSP is !!SOUND(Off) or !!MUSIC(Off)
            mpHost->mpMedia->stopMedia(mediaData);
        } else { // MSP is !!SOUND(Off U=https://example.com/sounds) or !!MUSIC(Off U=https://example.com/sounds)
            url = QUrl::fromUserInput(mediaData.mediaUrl());
        }
    } else if (mediaData.mediaUrl().isEmpty()) {
        if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mediaLocationMSP().isEmpty()) {
            url = QUrl::fromUserInput(mpHost->mediaLocationMSP());
        } else if (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mediaLocationGMCP().isEmpty()) {
            url = QUrl::fromUserInput(mpHost->mediaLocationGMCP());
        } else {
            url = QUrl::fromUserInput(qsl("https://www.%1/media/").arg(mpHost->mUrl));
        }
    } else {
        url = QUrl::fromUserInput(mediaData.mediaUrl());
    }

    return url;
}

bool TMedia::isValidUrl(QUrl& url)
{
    bool isValid = false;

    if (!url.isValid()) {
        qWarning() << qsl("TMedia::isValidUrl() WARNING - attempt made to reference an invalid URL: %1 and the error message was: \"%2\".").arg(url.toString(), url.errorString());
    } else {
        isValid = true;
    }

    return isValid;
}

bool TMedia::isFileRelative(TMediaData& mediaData)
{
    bool isFileRelative = false;

    if (!QFileInfo(mediaData.mediaFileName()).isRelative()) {
        if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP) {
            qWarning()
                    << qsl("TMedia::isFileRelative() WARNING - attempt made to send an absolute path as a media file name: %1.  Only relative paths are permitted.").arg(mediaData.mediaFileName());
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
    if ((mediaData.mediaFileName().contains(QLatin1Char('*')) || mediaData.mediaFileName().contains(QLatin1Char('?'))) && mediaData.mediaFileName().count(QLatin1Char('*')) < 2) {
        if (!mediaData.mediaFileName().contains(QLatin1Char('/'))) {
            dir.setNameFilters(QStringList() << mediaData.mediaFileName());
        } else { // Directory information needs filtered from the filter
            dir.setNameFilters(QStringList() << mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }

        QStringList fileNames(dir.entryList(QDir::Files | QDir::Readable, QDir::Name));

        for (auto& fileName : std::as_const(fileNames)) {
            fileNameList << qsl("%1/%2").arg(dir.path(), fileName);
        }
    } else {
        if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mediaData.mediaFileName().contains(QLatin1Char('.'))) {
            switch (mediaData.mediaType()) {
            case TMediaData::MediaTypeSound:
                mediaData.setMediaFileName(mediaData.mediaFileName().append(".wav"));
                break;
            case TMediaData::MediaTypeMusic:
                mediaData.setMediaFileName(mediaData.mediaFileName().append(".mid"));
                break;
            }
        }

        fileNameList << qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName());
    }

    return fileNameList;
}

QStringList TMedia::getFileNameList(TMediaData& mediaData)
{
    QStringList fileNameList;

    if (mediaData.mediaInput() == TMediaData::MediaInputStream) {
        return fileNameList << mediaData.mediaFileName();
    }

    if (mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
        QDir mediaDir(mediaPath);

        if (!mediaDir.mkpath(mediaPath)) {
            qWarning() << qsl("TMedia::getFileNameList() WARNING - attempt made to create a directory failed: %1").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()));
            return fileNameList;
        }

        if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaFileName().contains(QLatin1Char('/'))) {
            const QString mediaSubPath = qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName().section(QLatin1Char('/'), 0, -2));
            QDir mediaSubDir(mediaSubPath);

            if (!mediaSubDir.mkpath(mediaSubPath)) {
                qWarning() << qsl("TMedia::getFileNameList() WARNING - attempt made to create a directory failed: %1")
                                    .arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName().section(QLatin1Char('/'), 0, -2));
                return fileNameList;
            }

            fileNameList = TMedia::parseFileNameList(mediaData, mediaSubDir);
        }

        // If we did declare a sub directory, but didn't find the file there, we'll try a search in the root directory per the specification.
        if (fileNameList.isEmpty()) {
            fileNameList = TMedia::parseFileNameList(mediaData, mediaDir);
        }
    }

    return fileNameList;
}

QUrl TMedia::getFileUrl(TMediaData& mediaData)
{
    QUrl fileUrl;

    QString mediaLocation = QString();

    switch (mediaData.mediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        mediaLocation = mpHost->mediaLocationMSP();
        break;

    case TMediaData::MediaProtocolGMCP:
        mediaLocation = mpHost->mediaLocationGMCP();
        break;

    case TMediaData::MediaProtocolAPI:
        mediaLocation = mediaData.mediaUrl();
        break;
    }

    if (!mediaLocation.isEmpty()) {
        const bool endsWithSlash = mediaLocation.endsWith(QLatin1Char('/'));

        if (!endsWithSlash) {
            fileUrl = QUrl::fromUserInput(qsl("%1/%2").arg(mediaLocation, mediaData.mediaFileName()));
        } else {
            fileUrl = QUrl::fromUserInput(qsl("%1%2").arg(mediaLocation, mediaData.mediaFileName()));
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
        switch (mediaData.mediaProtocol()) {
        case TMediaData::MediaProtocolMSP:
            if (mpHost->mediaLocationMSP().isEmpty() || url.toString() != mpHost->mediaLocationMSP()) {
                mpHost->setMediaLocationMSP(url.toString());
            }
            break;

        case TMediaData::MediaProtocolGMCP:
            if (mpHost->mediaLocationGMCP().isEmpty() || url.toString() != mpHost->mediaLocationGMCP()) {
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
        event.mArgumentList << mediaData.mediaAbsolutePathFileName();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

        reply->deleteLater();
        mpHost->raiseEvent(event);
    } else {
        QSaveFile localFile(mediaData.mediaAbsolutePathFileName());

        if (!localFile.open(QFile::WriteOnly)) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << mediaData.mediaAbsolutePathFileName();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't open the destination file for writing (permission errors?)");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

            reply->deleteLater();
            mpHost->raiseEvent(event);
        }

        qint64 const bytesWritten = localFile.write(reply->readAll());

        if (bytesWritten == -1) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << mediaData.mediaAbsolutePathFileName();
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
                event.mArgumentList << mediaData.mediaAbsolutePathFileName();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << QString::number(bytesWritten);
                event.mArgumentTypeList << ARGUMENT_TYPE_NUMBER;

                if (!localFile.commit()) {
                    qDebug() << "TMedia::slot_writeFile: error saving downloaded media: " << localFile.errorString();
                }

                reply->deleteLater();
                mpHost->raiseEvent(event);

                TMedia::play(mediaData);
            } else {
                event.mArgumentList << QLatin1String("sysDownloadError");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << QLatin1String("Couldn't save to the destination file");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << mediaData.mediaAbsolutePathFileName();
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
    const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
    const QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << qsl("TMedia::downloadFile() WARNING - attempt made to create a directory failed: %1").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()));
        return;
    }

    if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaFileName().contains(QLatin1Char('/'))) {
        const QString mediaSubPath = qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName().section(QLatin1Char('/'), 0, -2));
        const QDir mediaSubDir(mediaSubPath);

        if (!mediaSubDir.mkpath(mediaSubPath)) {
            qWarning() << qsl("TMedia::downloadFile() WARNING - attempt made to create a directory failed: %1")
                                  .arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName().section(QLatin1Char('/'), 0, -2));
            return;
        }
    }

    const QDir dir;
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    if (!dir.mkpath(cacheDir)) {
        qWarning() << "TMedia::downloadFile() WARNING - couldn't create cache directory for sound file(s): " << cacheDir;
        return;
    }

    QUrl fileUrl = getFileUrl(mediaData);

    if (!TMedia::isValidUrl(fileUrl)) {
        return;
    } else {
        QNetworkRequest request = QNetworkRequest(fileUrl);
        request.setRawHeader(QByteArray("User-Agent"), QByteArray(qsl("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, mudlet::self()->mAppBuild).toUtf8().constData()));
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#if !defined(QT_NO_SSL)
        if (fileUrl.scheme() == qsl("https")) {
            const QSslConfiguration config(QSslConfiguration::defaultConfiguration());
            request.setSslConfiguration(config);
        }
#endif
        mpHost->updateProxySettings(mpNetworkAccessManager);
        QNetworkReply* getReply = mpNetworkAccessManager->get(request);
        mMediaDownloads.insert(getReply, mediaData);
        connect(getReply, &QNetworkReply::errorOccurred, this, [=, this](QNetworkReply::NetworkError) {
            qWarning() << "TMedia::downloadFile() WARNING - couldn't download sound from " << fileUrl.url();
            getReply->deleteLater();
        });
    }
}

QString TMedia::setupMediaAbsolutePathFileName(TMediaData& mediaData)
{
    QString absolutePathFileName;

    if (mediaData.mediaInput() == TMediaData::MediaInputFile) {
        absolutePathFileName = qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName());
    } else if (mediaData.mediaInput() == TMediaData::MediaInputStream) {
        absolutePathFileName = TMedia::getStreamUrl(mediaData);
    }

    mediaData.setMediaAbsolutePathFileName(absolutePathFileName);

    return absolutePathFileName;
}

QList<TMediaPlayer> TMedia::getMediaPlayerList(TMediaData& mediaData)
{
    QList<TMediaPlayer> mTMediaPlayerList;

    switch (mediaData.mediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        switch (mediaData.mediaType()) {
        case TMediaData::MediaTypeSound:
            mTMediaPlayerList = mMSPSoundList;
            break;
        case TMediaData::MediaTypeMusic:
            mTMediaPlayerList = mMSPMusicList;
            break;
        }
        break;

    case TMediaData::MediaProtocolGMCP:
        switch (mediaData.mediaType()) {
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
        switch (mediaData.mediaType()) {
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

void TMedia::connectMediaPlayer(TMediaPlayer& player)
{
    disconnect(player.mediaPlayer(), &QMediaPlayer::mediaStatusChanged, nullptr, nullptr);
    connect(player.mediaPlayer(), &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus mediaStatus) {
        if (mediaStatus == QMediaPlayer::EndOfMedia) {
            if (player.playlist() && !player.playlist()->isEmpty()) {
                QUrl nextMedia = player.playlist()->next();

                if (!nextMedia.isEmpty()) {
                    player.mediaPlayer()->setSource(nextMedia);
                    player.mediaPlayer()->play();
                } else if (player.playlist()->playbackMode() == TMediaPlaylist::Loop) {
                    // Start from the beginning if the playlist is set to loop
                    player.playlist()->setCurrentIndex(0);
                    player.mediaPlayer()->setSource(player.playlist()->currentMedia());
                    player.mediaPlayer()->play();
                }
            }
        }
    });

    disconnect(player.mediaPlayer(), &QMediaPlayer::playbackStateChanged, nullptr, nullptr);
    connect(player.mediaPlayer(), &QMediaPlayer::playbackStateChanged, this,
            [=](QMediaPlayerPlaybackState playbackState) { handlePlayerPlaybackStateChanged(playbackState, player); });
    disconnect(player.mediaPlayer(), &QMediaPlayer::positionChanged, nullptr, nullptr);
    connect(player.mediaPlayer(), &QMediaPlayer::positionChanged, this, [=](qint64 progress) {
        const int volume = player.mediaData().mediaVolume();
        const int duration = player.mediaPlayer()->duration();
        const int fadeInDuration = player.mediaData().mediaFadeIn();
        const int fadeOutDuration = player.mediaData().mediaFadeOut();
        const int startPosition = player.mediaData().mediaStart();
        const int finishPosition = player.mediaData().mediaFinish();
        const int endPosition = player.mediaData().mediaEnd();
        const bool fadeInUsed = fadeInDuration != TMediaData::MediaFadeNotSet;
        const bool fadeOutUsed = fadeOutDuration != TMediaData::MediaFadeNotSet;
        const bool finishUsed = finishPosition != TMediaData::MediaFinishNotSet;
        const bool endUsed = endPosition != TMediaData::MediaEndNotSet;
        const int relativeDuration = endUsed ? endPosition : finishUsed ? finishPosition : duration;
        const int relativeFadeInPosition = fadeInUsed ? startPosition + fadeInDuration : TMediaData::MediaFadeNotSet;
        const int relativeFadeOutPosition = fadeOutUsed ? relativeDuration - fadeOutDuration : TMediaData::MediaFadeNotSet;
        bool actionTaken = false;

        if (progress > relativeDuration && (endUsed || finishUsed)) {
            player.mediaPlayer()->stop();
        } else {
            if (fadeInUsed) {
                if (progress < relativeFadeInPosition) {
                    const double fadeInVolume = static_cast<double>(volume * (progress - startPosition)) / static_cast<double>((relativeFadeInPosition - startPosition) * 1.0);

                    player.setVolume(qRound(fadeInVolume));
                    actionTaken = true;
                } else if (progress == relativeFadeInPosition) {
                    player.setVolume(volume);
                    actionTaken = true;
                }
            }

            if (!actionTaken && fadeOutUsed && progress > 0) {
                if (progress > relativeFadeOutPosition) {
                    const double fadeOutVolume = static_cast<double>(volume * (relativeDuration - progress)) / static_cast<double>(fadeOutDuration * 1.0);

                    player.setVolume(qRound(fadeOutVolume));
                    actionTaken = true;
                }
            }

            if (!actionTaken && ((fadeInUsed && progress > relativeFadeInPosition) || (fadeOutUsed && progress < relativeFadeOutPosition))) {
                player.setVolume(volume); // Added to support multiple continue = true calls of same music
            }
        }
    });
}

void TMedia::updateMediaPlayerList(TMediaPlayer& player)
{
    int matchedMediaPlayerIndex = -1;
    TMediaData mediaData = player.mediaData();
    QList<TMediaPlayer> const mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);

    for (int i = 0; i < mTMediaPlayerList.size(); ++i) {
        TMediaPlayer const pTestPlayer = mTMediaPlayerList.at(i);

        if (pTestPlayer.mediaPlayer() == player.mediaPlayer()) {
            matchedMediaPlayerIndex = i;
            TMedia::connectMediaPlayer(player);
            break;
        }
    }

    switch (mediaData.mediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        switch (mediaData.mediaType()) {
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
        switch (mediaData.mediaType()) {
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
        switch (mediaData.mediaType()) {
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
    QList<TMediaPlayer> const mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) { // Find first available inactive QMediaPlayer
        TMediaPlayer const pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getPlaybackState() != QMediaPlayer::PlayingState && pTestPlayer.mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            pPlayer = pTestPlayer;
            // Discard all information relating to the current media source
            pPlayer.mediaPlayer()->setSource(QUrl());
            pPlayer.setMediaData(mediaData);
            break;
        }
    }

    if (!pPlayer.isInitialized()) { // No available QMediaPlayer, create a new one.
        pPlayer = TMediaPlayer(mpHost, mediaData);

        if (!pPlayer.mediaPlayer()) { // It should be impossible to ever reach this.
            qWarning() << qsl("TMedia::mediaPlayer() WARNING - unable to create new QMediaPlayer object.");
            return pPlayer;
        }
    }

    TMedia::connectMediaPlayer(pPlayer);

    return pPlayer;
}

void TMedia::handlePlayerPlaybackStateChanged(QMediaPlayerPlaybackState playbackState, const TMediaPlayer& pPlayer) {
    if (playbackState == QMediaPlayer::StoppedState) {
        TEvent mediaFinished{};
        mediaFinished.mArgumentList.append("sysMediaFinished");
        const QUrl mediaUrl = pPlayer.mediaPlayer()->source();
        mediaFinished.mArgumentList.append(mediaUrl.fileName());
        mediaFinished.mArgumentList.append(mediaUrl.path());
        mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (mpHost) {
            // The host may have gone away if the sound was a long one
            // and we are multi-playing so we ought to test it...
            mpHost->raiseEvent(mediaFinished);
        }
    }
}

TMediaPlayer TMedia::matchMediaPlayer(TMediaData& mediaData)
{
    TMediaPlayer pPlayer{};
    QList<TMediaPlayer> const mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer const pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getPlaybackState() == QMediaPlayer::PlayingState && pTestPlayer.mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (pTestPlayer.mediaData().mediaAbsolutePathFileName().endsWith(mediaData.mediaAbsolutePathFileName())) { // Is the same sound or music playing?
                pPlayer = pTestPlayer;
                pPlayer.setMediaData(mediaData);
                pPlayer.setVolume(mediaData.mediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.mediaVolume());
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

    if (mediaData.mediaPriority() == TMediaData::MediaPriorityNotSet) {
        return doesMediaHavePriorityToPlay;
    }

    int maxMediaPriority = 0;

    QList<TMediaPlayer> const mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) { // Find the maximum priority of all playing sounds
        TMediaPlayer const pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getPlaybackState() == QMediaPlayer::PlayingState && pTestPlayer.mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (!pTestPlayer.mediaData().mediaAbsolutePathFileName().endsWith(absolutePathFileName)) { // Is it a different sound or music than specified?
                if (pTestPlayer.mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet && pTestPlayer.mediaData().mediaPriority() > maxMediaPriority) {
                    maxMediaPriority = pTestPlayer.mediaData().mediaPriority();
                }
            }
        }
    }

    if (maxMediaPriority >= mediaData.mediaPriority()) { // Our media has a lower priority
        doesMediaHavePriorityToPlay = false;
    } else {
        TMediaData stopMediaData{};
        stopMediaData.setMediaProtocol(mediaData.mediaProtocol());
        stopMediaData.setMediaType(mediaData.mediaType());
        stopMediaData.setMediaPriority(mediaData.mediaPriority());
        mpHost->mpMedia->stopMedia(stopMediaData); // If we have the highest priority, stop everything else.
    }

    return doesMediaHavePriorityToPlay;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#key
void TMedia::matchMediaKeyAndStopMediaVariants(TMediaData& mediaData, const QString& absolutePathFileName)
{
    QList<TMediaPlayer> const mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer const pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer.getPlaybackState() == QMediaPlayer::PlayingState && pTestPlayer.mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (!mediaData.mediaKey().isEmpty() && !pTestPlayer.mediaData().mediaKey().isEmpty()
                && mediaData.mediaKey() == pTestPlayer.mediaData().mediaKey()) { // Does it have the same key?
                if (!pTestPlayer.mediaData().mediaAbsolutePathFileName().endsWith(absolutePathFileName)
                    || (!mediaData.mediaUrl().isEmpty() && !pTestPlayer.mediaData().mediaUrl().isEmpty()
                        && mediaData.mediaUrl() != pTestPlayer.mediaData().mediaUrl())) { // Is it a different sound or music than specified?
                    TMediaData stopMediaData = pTestPlayer.mediaData();
                    mpHost->mpMedia->stopMedia(stopMediaData); // Stop it!
                }
            }
        }
    }
}

void TMedia::play(TMediaData& mediaData)
{
    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    if (mediaData.mediaInput() == TMediaData::MediaInputNotSet) {
        return;
    }

    const QStringList fileNameList = TMedia::getFileNameList(mediaData);

    if (fileNameList.isEmpty()) { // This should not happen.
        qWarning() << qsl("TMedia::play() WARNING - could not generate a list of media file names.");
        return;
    }

    TMediaPlayer pPlayer{};

    if (mediaData.mediaType() == TMediaData::MediaTypeMusic || mediaData.mediaType() == TMediaData::MediaTypeVideo) {
        pPlayer = TMedia::matchMediaPlayer(mediaData);
    }

    bool sameMediaIsPlaying = false;

    if (pPlayer.isInitialized()) {
        sameMediaIsPlaying = true; // Same media is playing; Reuse that MediaPlayer
    } else {
        pPlayer = TMedia::getMediaPlayer(mediaData);
    }

    // It should be impossible to ever reach this.
    if (!pPlayer.mediaPlayer()) {
        qWarning() << qsl("TMedia::play() WARNING - unable to create new QMediaPlayer object.");
        return;
    }

    TMediaPlaylist* playlist = pPlayer.playlist();

    if (!sameMediaIsPlaying) {
        playlist->clear();
        playlist->setPlaybackMode(TMediaPlaylist::Sequential);
    }

    QString absolutePathFileName;

    if (mediaData.mediaLoops() == TMediaData::MediaLoopsDefault) { // Play once
        playlist->setPlaybackMode(TMediaPlaylist::Sequential);

        if (sameMediaIsPlaying) {
            if (mediaData.mediaContinue() == TMediaData::MediaContinueRestart) {
                mpHost->mpMedia->stopMedia(mediaData); // Stop the media; Restart it below.

                if (!playlist->isEmpty()) {
                    playlist->clear();
                }
            } else {
                if (!playlist->isEmpty() && playlist->mediaCount() > 1) { // Purge media from the previous playlist
                    playlist->removeMedia(playlist->nextIndex(), playlist->mediaCount());
                }

                return; // No action required. Continue playing the same media.
            }
        }

        if (fileNameList.size() > 1) {
            absolutePathFileName = fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()));
        } else if (mediaData.mediaInput() == TMediaData::MediaInputStream) {
            absolutePathFileName = TMedia::getStreamUrl(mediaData);
        } else {
            absolutePathFileName = fileNameList.at(0);
        }

        if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) { // Filter out other media
            return;
        }

        if (!mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
            TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName); // If mediaKey matches, check for uniqueness.
        }

        const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile
            ? QUrl::fromLocalFile(absolutePathFileName)
            : QUrl(absolutePathFileName);
        pPlayer.mediaPlayer()->setSource(mediaSource);
    } else {
        if (mediaData.mediaLoops() == TMediaData::MediaLoopsRepeat) { // Repeat indefinitely
            playlist->setPlaybackMode(TMediaPlaylist::Loop);

            if (sameMediaIsPlaying) {
                if (mediaData.mediaContinue() == TMediaData::MediaContinueRestart) {
                    mpHost->mpMedia->stopMedia(mediaData); // Stop the media; Restart it below.

                    if (!playlist->isEmpty()) {
                        playlist->clear();
                    }
                } else {
                    return; // No action required. Continue playing the same media.
                }
            }

            if (fileNameList.size() > 1) {
                absolutePathFileName = fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()));
            } else if (mediaData.mediaInput() == TMediaData::MediaInputStream) {
                absolutePathFileName = TMedia::getStreamUrl(mediaData);
            } else {
                absolutePathFileName = fileNameList.at(0);
            }

            if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) { // Filter out other media
                return;
            }

            if (!mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
                TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName); // If mediaKey matches, check for uniqueness.
            }

            const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile
                ? QUrl::fromLocalFile(absolutePathFileName)
                : QUrl(absolutePathFileName);
            playlist->addMedia(mediaSource);
        } else {
            playlist->setPlaybackMode(TMediaPlaylist::Sequential);

            if (sameMediaIsPlaying) {
                if (mediaData.mediaContinue() == TMediaData::MediaContinueRestart) {
                    mpHost->mpMedia->stopMedia(mediaData); // Stop the media; Restart it below.

                    if (!playlist->isEmpty()) {
                        playlist->clear();
                    }
                } else {
                    if (!playlist->isEmpty() && playlist->mediaCount() > 1) { // Purge media from the previous playlist
                        playlist->removeMedia(playlist->nextIndex(), playlist->mediaCount());
                    }

                    mediaData.setMediaLoops(mediaData.mediaLoops() - 1); // Subtract the currently playing media from the total
                }
            }

            for (int k = 0; k < mediaData.mediaLoops(); k++) { // Repeat a finite number of times
                if (fileNameList.size() > 1) {
                    absolutePathFileName = fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()));
                } else if (mediaData.mediaInput() == TMediaData::MediaInputStream) {
                    absolutePathFileName = TMedia::getStreamUrl(mediaData);
                } else {
                    absolutePathFileName = fileNameList.at(0);
                }

                if (k < 1) {
                    if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) { // Filter out other sound
                        return;
                    }

                    if (!mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
                        TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName); // If mediaKey matches, check for uniqueness.
                    }
                }

                const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile
                    ? QUrl::fromLocalFile(absolutePathFileName)
                    : QUrl(absolutePathFileName);
                playlist->addMedia(mediaSource);
            }
        }

        if (sameMediaIsPlaying && mediaData.mediaContinue() == TMediaData::MediaContinueDefault) {
            return;
        }

        playlist->setCurrentIndex(0);
        pPlayer.setPlaylist(playlist);
        pPlayer.mediaPlayer()->setSource(playlist->currentMedia());
    }

    // Set volume, start and play media
    pPlayer.setVolume(mediaData.mediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.mediaVolume());
    pPlayer.mediaPlayer()->setPosition(mediaData.mediaStart());

    // Set whether or not we should be muted
    switch (mediaData.mediaProtocol()) {
        case TMediaData::MediaProtocolAPI:
            pPlayer.mediaPlayer()->audioOutput()->setMuted(mudlet::self()->muteAPI());
            break;
        case TMediaData::MediaProtocolGMCP:
            pPlayer.mediaPlayer()->audioOutput()->setMuted(mudlet::self()->muteGame());
            break;
        case TMediaData::MediaProtocolMSP:
            pPlayer.mediaPlayer()->audioOutput()->setMuted(mudlet::self()->muteGame());
            break;
    }

    if (mediaData.mediaType() == TMediaData::MediaTypeVideo) {
        // Implement video widget
    }

    pPlayer.mediaPlayer()->play();

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

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#input
int TMedia::parseJSONByMediaInput(QJsonObject& json)
{
    TMediaData::MediaInput mediaInput = TMediaData::MediaInputNotSet;

    auto mediaInputJSON = json.value(qsl("stream"));

    if (mediaInputJSON != QJsonValue::Undefined && mediaInputJSON.isString() && !mediaInputJSON.toString().isEmpty()) {
        if (mediaInputJSON.toString().toLower() == "true") {
            mediaInput = TMediaData::MediaInputStream;
        }
    }

    return mediaInput;
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

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#finish
int TMedia::parseJSONByMediaFinish(QJsonObject& json)
{
    int mediaFinish = TMediaData::MediaFinishNotSet;

    auto mediaFinishJSON = json.value(qsl("end"));

    if (mediaFinishJSON != QJsonValue::Undefined && mediaFinishJSON.isString() && !mediaFinishJSON.toString().isEmpty()) {
        mediaFinish = mediaFinishJSON.toString().toInt();
    } else if (mediaFinishJSON != QJsonValue::Undefined && mediaFinishJSON.toInt()) {
        mediaFinish = mediaFinishJSON.toInt();
    }

    return mediaFinish;
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

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#fadeaway
TMediaData::MediaFadeAway TMedia::parseJSONByMediaFadeAway(QJsonObject& json)
{
    TMediaData::MediaFadeAway mediaFadeAway = TMediaData::MediaFadeAwayDefault;

    auto mediaFadeAwayJSON = json.value(qsl("fadeaway"));

    if (mediaFadeAwayJSON != QJsonValue::Undefined && mediaFadeAwayJSON.isString() && !mediaFadeAwayJSON.toString().isEmpty()) {
        if (mediaFadeAwayJSON.toString() == "true") {
            mediaFadeAway = TMediaData::MediaFadeAwayEnabled;
        } else {
            mediaFadeAway = TMediaData::MediaFadeAwayDefault;
        }
    } else if (mediaFadeAwayJSON != QJsonValue::Undefined && mediaFadeAwayJSON.toBool(true)) {
        mediaFadeAway = TMediaData::MediaFadeAwayEnabled;
    }

    return mediaFadeAway;
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
    mediaData.setMediaInput(TMediaData::MediaInputFile);
    mediaData.setMediaUrl(TMedia::parseJSONByMediaUrl(json));
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

    mediaData.setMediaFileName(mediaData.mediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/')));

    if (!TMedia::isFileRelative(mediaData)) {
        return;
    }

    if (!TMedia::processUrl(mediaData)) {
        return;
    }

    const QString absolutePathFileName = TMedia::setupMediaAbsolutePathFileName(mediaData);

    const QFile mediaFile(absolutePathFileName);

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

    if (mediaData.mediaType() == TMediaData::MediaTypeNotSet) {
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
    mediaData.setMediaFinish(TMedia::parseJSONByMediaFinish(json));
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
    mediaData.setMediaFadeAway(TMedia::parseJSONByMediaFadeAway(json));
    mediaData.setMediaFadeOut(TMedia::parseJSONByMediaFadeOut(json));

    TMedia::stopMedia(mediaData);
}
// End Private
