/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020, 2022-2025 by Stephen Lyons                   *
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
#include "TLabel.h"

#include "pre_guard.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDiskCache>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QVideoWidget>
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
    if (!isMediaProtocolAllowed(mediaData)) {
        return;
    }

    // Check if we should resume an existing paused media
    if ((mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI) && resume(mediaData)) {
        return; // Paused media was resumed. Processing complete.
    }

    if (mediaData.mediaInput() == TMediaData::MediaInputNotSet) {
        mediaData.setMediaInput(TMediaData::MediaInputFile);
    }

    // Normalize file paths
    mediaData.setMediaFileName(mediaData.mediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/')));

    // Handle MSP special case
    if (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && mediaData.mediaFileName() == qsl("Off")) {
        TMedia::processUrl(mediaData);
        return;
    }

    if (mediaData.mediaInput() == TMediaData::MediaInputStream) {
        TMedia::setupMediaAbsolutePathFileName(mediaData);
    } else if (mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and GMCP files should not have absolute paths.
        }

        if (!mediaData.mediaFileName().contains(QLatin1Char('*')) &&
            !mediaData.mediaFileName().contains(QLatin1Char('?'))) { // File path wildcards are * and ?
            // Append appropriate file extension for MSP files
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

            // Preload check (volume 0 is used to preload media)
            if (mediaData.mediaVolume() == TMediaData::MediaVolumePreload) {
                return;
            }
        }
    }

    TMedia::play(mediaData);
}

QList<TMediaData> TMedia::playingMedia(TMediaData& mediaData)
{
    QList<TMediaData> mMatchingTMediaDataList;

    if (!isMediaProtocolAllowed(mediaData)) {
        return mMatchingTMediaDataList;
    }

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mTMediaPlayerList.isEmpty()) {
        return mMatchingTMediaDataList;
    }

    if (!mediaData.mediaFileName().isEmpty()) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP ||
                              mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return mMatchingTMediaDataList; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute, but get copied into the media folder for processing. Trim the path from the file name.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section('/', -1));
        }
    }

    for (const auto& pPlayer : mTMediaPlayerList) {
        if (!pPlayer) {
            continue;
        }

        if (pPlayer->getPlaybackState() != QMediaPlayer::PlayingState &&
            pPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() > mediaData.mediaPriority()) {
            continue;
        }

        mMatchingTMediaDataList.append(pPlayer->mediaData());
    }

    return mMatchingTMediaDataList;
}

QList<TMediaData> TMedia::pausedMedia(TMediaData& mediaData)
{
    QList<TMediaData> mMatchingTMediaDataList;

    if (!isMediaProtocolAllowed(mediaData)) {
        return mMatchingTMediaDataList;
    }

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mTMediaPlayerList.isEmpty()) {
        return mMatchingTMediaDataList;
    }

    if (!mediaData.mediaFileName().isEmpty()) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP ||
                              mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return mMatchingTMediaDataList; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute but get copied into the media folder. Trim the path.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section('/', -1));
        }
    }

    for (const auto& pPlayer : mTMediaPlayerList) {
        if (!pPlayer) {
            continue;
        }

        if (pPlayer->getPlaybackState() != QMediaPlayer::PausedState) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() > mediaData.mediaPriority()) {
            continue;
        }

        mMatchingTMediaDataList.append(pPlayer->mediaData());
    }

    return mMatchingTMediaDataList;
}

void TMedia::pauseMedia(TMediaData& mediaData)
{
    if (!isMediaProtocolAllowed(mediaData)) {
        return;
    }

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mTMediaPlayerList.isEmpty()) {
        return;
    }

    if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP ||
                              mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute but get copied into the media folder. Trim the path.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }
    }

    for (const auto& pPlayer : mTMediaPlayerList) {
        if (!pPlayer) {
            continue;
        }

        if (pPlayer->getPlaybackState() != QMediaPlayer::PlayingState) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() >= mediaData.mediaPriority()) {
            continue;
        }

        pPlayer->mediaPlayer()->pause();
    }
}

void TMedia::stopMedia(TMediaData& mediaData)
{
    if (!isMediaProtocolAllowed(mediaData)) {
        return;
    }

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mTMediaPlayerList.isEmpty()) {
        return;
    }

    if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP ||
                              mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute but get copied into the media folder. Trim the path.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }
    }

    for (auto& pPlayer : mTMediaPlayerList) {
        if (!pPlayer) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet &&
            pPlayer->mediaData().mediaPriority() >= mediaData.mediaPriority()) {
            continue;
        }

        if ((mediaData.mediaFadeAway() == TMediaData::MediaFadeAwayEnabled || mediaData.mediaFadeOut() != TMediaData::MediaFadeNotSet) &&
            pPlayer->mediaData().mediaEnd() == TMediaData::MediaEndNotSet) {
            const int finishPosition = pPlayer->mediaData().mediaFinish();
            const int duration = pPlayer->mediaPlayer()->duration();
            const int currentPosition = pPlayer->mediaPlayer()->position();
            const int fadeOut = pPlayer->mediaData().mediaFadeOut() ? pPlayer->mediaData().mediaFadeOut() : mediaData.mediaFadeOut();
            const int remainingDuration = (finishPosition != TMediaData::MediaFinishNotSet ? finishPosition : duration) - currentPosition;
            const int endDuration = fadeOut != TMediaData::MediaFadeNotSet ? std::min(remainingDuration, fadeOut) : std::min(remainingDuration, 5000);
            const int endPosition = currentPosition + endDuration;

            printClosedCaption(pPlayer->mediaData(), tr("fades"));

            TMediaData updateMediaData = pPlayer->mediaData();
            updateMediaData.setMediaFadeOut(endDuration);
            updateMediaData.setMediaEnd(endPosition);
            pPlayer->setMediaData(updateMediaData);
            TMedia::updateMediaPlayerList(std::move(pPlayer));

            continue;
        }

        // **Stop the player but keep it for reuse**
        pPlayer->mediaPlayer()->stop();
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
    } else if (package == "client.media.pause") {
        TMedia::parseJSONForMediaPause(json);
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
bool TMedia::isMediaProtocolAllowed(const TMediaData& mediaData) const
{
    if ((mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) ||
        (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia)) {
        return false;
    }
    return true;
}

QList<std::shared_ptr<TMediaPlayer>> TMedia::findMediaPlayersByCriteria(const TMediaData& mediaData)
{
    switch (mediaData.mediaProtocol()) {
        case TMediaData::MediaProtocolMSP:
            return (mediaData.mediaType() == TMediaData::MediaTypeSound) ? mMSPSoundList : mMSPMusicList;

        case TMediaData::MediaProtocolGMCP:
            if (mediaData.mediaType() == TMediaData::MediaTypeNotSet) {
                QList<std::shared_ptr<TMediaPlayer>> combinedList;
                combinedList.append(mGMCPSoundList);
                combinedList.append(mGMCPMusicList);
                combinedList.append(mGMCPVideoList);
                return combinedList;
            }
            return (mediaData.mediaType() == TMediaData::MediaTypeSound) ? mGMCPSoundList :
                   (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? mGMCPMusicList :
                   mGMCPVideoList;

        case TMediaData::MediaProtocolAPI:
            if (mediaData.mediaType() == TMediaData::MediaTypeNotSet) {
                QList<std::shared_ptr<TMediaPlayer>> combinedList;
                combinedList.append(mAPISoundList);
                combinedList.append(mAPIMusicList);
                combinedList.append(mAPIVideoList);
                return combinedList;
            }
            return (mediaData.mediaType() == TMediaData::MediaTypeSound) ? mAPISoundList :
                   (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? mAPIMusicList :
                   mAPIVideoList;

        case TMediaData::MediaProtocolNotSet:
            if (mediaData.mediaType() == TMediaData::MediaTypeNotSet) {
                QList<std::shared_ptr<TMediaPlayer>> combinedList;
                combinedList.append(mMSPSoundList);
                combinedList.append(mMSPMusicList);
                combinedList.append(mGMCPSoundList);
                combinedList.append(mGMCPMusicList);
                combinedList.append(mGMCPVideoList);
                combinedList.append(mAPISoundList);
                combinedList.append(mAPIMusicList);
                combinedList.append(mAPIVideoList);
                return combinedList;
            }
            return (mediaData.mediaType() == TMediaData::MediaTypeSound) ? mMSPSoundList :
                   (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? mMSPMusicList :
                   (mediaData.mediaType() == TMediaData::MediaTypeVideo) ? mGMCPVideoList :
                   QList<std::shared_ptr<TMediaPlayer>>(); // Return empty list
    }

    return {}; // Default empty list fallback
}

bool TMedia::isMediaMatch(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& mediaData)
{
    if (!player) {
        return false;
    }

    if (!mediaData.mediaKey().isEmpty() && !player->mediaData().mediaKey().isEmpty() &&
        player->mediaData().mediaKey() != mediaData.mediaKey()) {
        return false;
    }

    if (!mediaData.mediaFileName().isEmpty() && !player->mediaData().mediaFileName().isEmpty() &&
        player->mediaData().mediaFileName() != mediaData.mediaFileName()) {
        return false;
    }

    if (!mediaData.mediaTag().isEmpty() && !player->mediaData().mediaTag().isEmpty() &&
        player->mediaData().mediaTag() != mediaData.mediaTag()) {
        return false;
    }

    return true;
}

bool TMedia::resume(TMediaData mediaData)
{
    bool resumed = false;

    if (!isMediaProtocolAllowed(mediaData)) {
        return resumed;
    }

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mTMediaPlayerList.isEmpty()) {
        return resumed;
    }

    if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return resumed; // MSP and GMCP files will not have absolute paths. Something is wrong.
        }

        // API files may start as absolute, but get copied into the media folder for processing. Trim the path from the file name.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }
    }

    for (const auto& pPlayer : mTMediaPlayerList) {
        if (!pPlayer) {
            continue;
        }

        if (pPlayer->getPlaybackState() != QMediaPlayer::PausedState) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        pPlayer->mediaPlayer()->play();
        resumed = true;
    }

    return resumed;
}

void TMedia::stopAllMediaPlayers()
{
    TMediaData mediaData{};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolNotSet);
    mediaData.setMediaType(TMediaData::MediaTypeNotSet);

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    for (const auto& pPlayer : mTMediaPlayerList) {
        if (!pPlayer) {
            continue;
        }

        pPlayer->mediaPlayer()->stop();
    }
}

void TMedia::setMediaPlayersMuted(const TMediaData::MediaProtocol mediaProtocol, const bool state)
{
    TMediaData mediaData{};
    mediaData.setMediaProtocol(mediaProtocol);

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    for (const auto& player : mTMediaPlayerList) {
        if (!player) {
            continue;
        }

        if (player->mediaPlayer()->audioOutput()) {
            player->mediaPlayer()->audioOutput()->setMuted(state);
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
        connect(getReply, &QNetworkReply::errorOccurred, this, [=](QNetworkReply::NetworkError) {
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

void TMedia::connectMediaPlayer(std::shared_ptr<TMediaPlayer>& player)
{
    if (!player) {
        qWarning() << qsl("TMedia::connectMediaPlayer() WARNING - Attempted to connect a null TMediaPlayer.");
        return;
    }

    std::weak_ptr<TMediaPlayer> weakPlayer = player; // Safe handling for longer-lived connections

    // Seekable changed connection
    disconnect(player->mediaPlayer(), &QMediaPlayer::seekableChanged, nullptr, nullptr);
    connect(player->mediaPlayer(), &QMediaPlayer::seekableChanged, this, [weakPlayer](bool seekable) {
        if (auto lockedPlayer = weakPlayer.lock()) { // Ensure the player is still valid
            if (seekable) {
                lockedPlayer->mediaPlayer()->setPosition(lockedPlayer->mediaData().mediaStart());
            }
        }
    });

    // Media status changed connection
    disconnect(player->mediaPlayer(), &QMediaPlayer::mediaStatusChanged, nullptr, nullptr);
    connect(player->mediaPlayer(), &QMediaPlayer::mediaStatusChanged, this, [this, weakPlayer](QMediaPlayer::MediaStatus mediaStatus) {
        if (auto lockedPlayer = weakPlayer.lock()) {
            if (mediaStatus == QMediaPlayer::EndOfMedia) {
                if (lockedPlayer->playlist() && !lockedPlayer->playlist()->isEmpty()) {
                    QUrl nextMedia = lockedPlayer->playlist()->next();

                    if (!nextMedia.isEmpty()) {
                        lockedPlayer->mediaPlayer()->setSource(nextMedia);
                        lockedPlayer->mediaPlayer()->play();
                    } else if (lockedPlayer->playlist()->playbackMode() == TMediaPlaylist::Loop) {
                        lockedPlayer->playlist()->setCurrentIndex(0);
                        lockedPlayer->mediaPlayer()->setSource(lockedPlayer->playlist()->currentMedia());
                        lockedPlayer->mediaPlayer()->play();
                    }
                }
            }
        }
    });

    // Playback state changed connection
    disconnect(player->mediaPlayer(), &QMediaPlayer::playbackStateChanged, nullptr, nullptr);
    connect(player->mediaPlayer(), &QMediaPlayer::playbackStateChanged, this,
            [this, weakPlayer](QMediaPlayerPlaybackState playbackState) {
                if (auto lockedPlayer = weakPlayer.lock()) {
                    handlePlayerPlaybackStateChanged(playbackState, lockedPlayer);
                }
            });

    // Position changed connection (handles fade-in/fade-out effects)
    disconnect(player->mediaPlayer(), &QMediaPlayer::positionChanged, nullptr, nullptr);
    connect(player->mediaPlayer(), &QMediaPlayer::positionChanged, this, [this, weakPlayer](qint64 progress) {
        if (auto lockedPlayer = weakPlayer.lock()) { // Ensure the player is still valid
            const int volume = lockedPlayer->mediaData().mediaVolume();
            const int duration = lockedPlayer->mediaPlayer()->duration();
            const int fadeInDuration = lockedPlayer->mediaData().mediaFadeIn();
            const int fadeOutDuration = lockedPlayer->mediaData().mediaFadeOut();
            const int startPosition = lockedPlayer->mediaData().mediaStart();
            const int finishPosition = lockedPlayer->mediaData().mediaFinish();
            const int endPosition = lockedPlayer->mediaData().mediaEnd();
            const bool fadeInUsed = fadeInDuration != TMediaData::MediaFadeNotSet;
            const bool fadeOutUsed = fadeOutDuration != TMediaData::MediaFadeNotSet;
            const bool finishUsed = finishPosition != TMediaData::MediaFinishNotSet;
            const bool endUsed = endPosition != TMediaData::MediaEndNotSet;
            const int relativeDuration = endUsed ? endPosition : finishUsed ? finishPosition : duration;
            const int relativeFadeInPosition = fadeInUsed ? startPosition + fadeInDuration : TMediaData::MediaFadeNotSet;
            const int relativeFadeOutPosition = fadeOutUsed ? relativeDuration - fadeOutDuration : TMediaData::MediaFadeNotSet;
            bool actionTaken = false;

            if (progress > relativeDuration && (endUsed || finishUsed)) {
                lockedPlayer->mediaPlayer()->stop();
            } else {
                if (fadeInUsed) {
                    if (progress < relativeFadeInPosition) {
                        const double fadeInVolume = static_cast<double>(volume * (progress - startPosition)) /
                                                    static_cast<double>(relativeFadeInPosition - startPosition);
                        lockedPlayer->setVolume(qRound(fadeInVolume));
                        actionTaken = true;
                    } else if (progress == relativeFadeInPosition) {
                        lockedPlayer->setVolume(volume);
                        actionTaken = true;
                    }
                }

                if (!actionTaken && fadeOutUsed && progress > 0) {
                    if (progress > relativeFadeOutPosition) {
                        const double fadeOutVolume = static_cast<double>(volume * (relativeDuration - progress)) /
                                                     static_cast<double>(fadeOutDuration);
                        lockedPlayer->setVolume(qRound(fadeOutVolume));
                        actionTaken = true;
                    }
                }

                if (!actionTaken && ((fadeInUsed && progress > relativeFadeInPosition) || (fadeOutUsed && progress < relativeFadeOutPosition))) {
                    lockedPlayer->setVolume(volume);
                }
            }
        }
    });
}

void TMedia::purgeStoppedMediaPlayers(QList<std::shared_ptr<TMediaPlayer>>& mediaList)
{
    mediaList.erase(std::remove_if(mediaList.begin(), mediaList.end(),
                                   [](const std::shared_ptr<TMediaPlayer>& player) {
                                       return player->getPlaybackState() == QMediaPlayer::StoppedState;
                                   }),
                    mediaList.end());
}

// Helper for updating media player lists (now a static TMedia method)
template<typename T>
void TMedia::updateList(QList<std::shared_ptr<T>>& list, int index, std::shared_ptr<T> player, TMedia* mediaInstance)
{
    if (index == -1) {
        qDebug() << "TMedia::updateList() - Adding new player to list (index == -1)";
        list.append(std::move(player));
    } else if (index >= 0 && index < list.size()) {
        qDebug() << "TMedia::updateList() - Replacing existing player at index:" << index;
        list[index] = std::move(player);
    } else {
        qWarning() << "TMedia::updateList() - Invalid index:" << index
                   << " for list size:" << list.size() << ". Appending instead.";
        list.append(std::move(player));
    }

    if (list.size() > mediaInstance->getMaxUnprunedPlayers()) {
        qDebug() << "TMedia::updateList() - List exceeded max allowed size (" << mediaInstance->getMaxUnprunedPlayers() << "). Purging stopped players.";
        TMedia::purgeStoppedMediaPlayers(list);

        if (mudlet::smDebugMode && mediaInstance && mediaInstance->mpHost && mediaInstance->mpHost->mpConsole) {
            mediaInstance->mpHost->mpConsole->printSystemMessage(qsl("%1\n").arg(tr("Too many stopped media players. Purging stopped players.")));
        }

        if (list.size() > mediaInstance->getMaxUnprunedPlayers()) {
            qWarning() << "TMedia::updateList() - List still exceeds max size after purging. Removing oldest active player.";
            list.removeFirst(); // Evict the oldest player to enforce cap

            if (mudlet::smDebugMode && mediaInstance && mediaInstance->mpHost && mediaInstance->mpHost->mpConsole) {
                mediaInstance->mpHost->mpConsole->printSystemMessage(qsl("%1\n").arg(tr("Too many stopped media players. Removed oldest active player.")));
            }
        }
    }

    qDebug() << "TMedia::updateList() - List size after update:" << list.size();
}

void TMedia::updateMediaPlayerList(std::shared_ptr<TMediaPlayer> player)
{
    if (!player) {
        qDebug() << "TMedia::updateMediaPlayerList() - Player is null. Aborting update.";
        return;
    }

    int matchedMediaPlayerIndex = -1;
    TMediaData mediaData = player->mediaData();

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = findMediaPlayersByCriteria(mediaData);

    qDebug() << "TMedia::updateMediaPlayerList() - Searching for existing player in list.";
    for (int i = 0; i < mTMediaPlayerList.size(); ++i) {
        if (mTMediaPlayerList[i] && mTMediaPlayerList[i]->mediaPlayer() == player->mediaPlayer()) {
            matchedMediaPlayerIndex = i;
            connectMediaPlayer(mTMediaPlayerList[i]); // Ensure it's connected
            break;
        }
    }
    if (matchedMediaPlayerIndex != -1) {
        qDebug() << "TMedia::updateMediaPlayerList() - Found existing player at index:" << matchedMediaPlayerIndex;
    } else {
        qDebug() << "TMedia::updateMediaPlayerList() - No existing player found. Appending new one.";
    }

    QList<std::shared_ptr<TMediaPlayer>>* list = nullptr;

    qDebug() << "TMedia::updateMediaPlayerList() - Updating list for protocol:"
             << mediaData.mediaProtocol()
             << "and type:" << mediaData.mediaType();
    switch (mediaData.mediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        list = (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? &mMSPMusicList : &mMSPSoundList;
        break;
    case TMediaData::MediaProtocolGMCP:
        list = (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? &mGMCPMusicList :
               (mediaData.mediaType() == TMediaData::MediaTypeVideo) ? &mGMCPVideoList : &mGMCPSoundList;
        break;
    case TMediaData::MediaProtocolAPI:
        list = (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? &mAPIMusicList :
               (mediaData.mediaType() == TMediaData::MediaTypeVideo) ? &mAPIVideoList : &mAPISoundList;
        break;
    }

    if (list) {
        TMedia::updateList<TMediaPlayer>(*list, matchedMediaPlayerIndex, std::move(player), this);
    } else {
        qWarning() << "TMedia::updateMediaPlayerList() - Could not determine appropriate list for player.";
    }
}

std::shared_ptr<TMediaPlayer> TMedia::getMediaPlayer(TMediaData& mediaData)
{
    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    for (auto& existingPlayer : mediaPlayerList) {
        if (!existingPlayer || !existingPlayer->mediaPlayer()) {
            qWarning() << "TMedia::getMediaPlayer() - Skipping a null TMediaPlayer.";
            continue;
        }

        if (existingPlayer->getPlaybackState() != QMediaPlayer::PlayingState &&
            existingPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            existingPlayer->setMediaData(mediaData);
            return existingPlayer;  // Reuse existing player
        }
    }

    // Cap to prevent overflow per media type
    int maxAllowed = getMaxAllowedSoundPlayers(); // Default fallback

    switch (mediaData.mediaType()) {
        case TMediaData::MediaTypeMusic:
            maxAllowed = getMaxAllowedMusicPlayers();
            break;
        case TMediaData::MediaTypeVideo:
            maxAllowed = getMaxAllowedVideoPlayers();
            break;
        case TMediaData::MediaTypeSound:
        default:
            maxAllowed = getMaxAllowedSoundPlayers();
            break;
    }

    if (mediaPlayerList.size() >= maxAllowed) {
        qWarning() << "TMedia::getMediaPlayer() - Too many active players for media type. Skipping creation.";

        if (mudlet::smDebugMode && mpHost && mpHost->mpConsole) {
            mpHost->mpConsole->printSystemMessage(qsl("%1\n").arg(tr("Maximum allowed active media players reached for media type. Cannot play additional media.")));
        }

        return nullptr;
    }

    // No available player, create a new one
    auto newPlayer = std::make_shared<TMediaPlayer>(mpHost, mediaData);

    if (!newPlayer || !newPlayer->mediaPlayer()) {
        qWarning() << "TMedia::getMediaPlayer() - Failed to create new TMediaPlayer or its QMediaPlayer is null.";
        return nullptr;
    }

    newPlayer->setMediaData(mediaData);
    connectMediaPlayer(newPlayer);
    mediaPlayerList.append(newPlayer);

    return newPlayer;
}

// Dynamic limits for media players, based on host count
int TMedia::getMaxUnprunedPlayers() const {
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(10, 25 / hostCount);
}

int TMedia::getMaxAllowedSoundPlayers() const {
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(16, 65 / hostCount);
}

int TMedia::getMaxAllowedMusicPlayers() const {
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(4, 20 / hostCount);
}

int TMedia::getMaxAllowedVideoPlayers() const {
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(2, 10 / hostCount);
}

void TMedia::handlePlayerPlaybackStateChanged(QMediaPlayerPlaybackState playbackState, const std::shared_ptr<TMediaPlayer>& player)
{
    if (!player) {
        return;
    }

    if (playbackState == QMediaPlayer::StoppedState) {
        TEvent mediaFinished{};
        mediaFinished.mArgumentList.append("sysMediaFinished");

        const QUrl mediaUrl = player->mediaPlayer()->source();
        mediaFinished.mArgumentList.append(mediaUrl.fileName());
        mediaFinished.mArgumentList.append(mediaUrl.path());
        mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (mpHost) {
            mpHost->raiseEvent(mediaFinished);
        }

        if (player->mediaData().mediaWidget() == TMediaData::MediaWidgetLabel &&
            player->mediaData().mediaClose() == TMediaData::MediaCloseEnabled &&
            player->mediaPlayer()->videoOutput() != nullptr) {
            QVideoWidget* videoOutput = qobject_cast<QVideoWidget*>(player->mediaPlayer()->videoOutput());

            if (videoOutput != nullptr) {
                QWidget* parent = videoOutput->parentWidget();

                if (parent != nullptr && parent->isVisible()) {
                    parent->hide();
                }
            }
        }

        printClosedCaption(player->mediaData(), tr("stops"));
        return;
    } else if (playbackState == QMediaPlayer::PlayingState && player->mediaData().mediaVolume() != TMediaData::MediaVolumePreload) {
        TEvent mediaStarted{};
        mediaStarted.mArgumentList.append("sysMediaStarted");

        const QUrl mediaUrl = player->mediaPlayer()->source();
        mediaStarted.mArgumentList.append(mediaUrl.fileName());
        mediaStarted.mArgumentList.append(mediaUrl.path());
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (mpHost) {
            mpHost->raiseEvent(mediaStarted);
        }

        printClosedCaption(player->mediaData(), tr("plays"));
        return;
    } else if (playbackState == QMediaPlayer::PausedState) {
        TEvent mediaPaused{};
        mediaPaused.mArgumentList.append("sysMediaPaused");

        const QUrl mediaUrl = player->mediaPlayer()->source();
        mediaPaused.mArgumentList.append(mediaUrl.fileName());
        mediaPaused.mArgumentList.append(mediaUrl.path());
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (mpHost) {
            mpHost->raiseEvent(mediaPaused);
        }

        printClosedCaption(player->mediaData(), tr("pauses"));
    }
}

std::shared_ptr<TMediaPlayer> TMedia::matchMediaPlayer(TMediaData& mediaData)
{
    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = TMedia::findMediaPlayersByCriteria(mediaData);

    for (auto& pTestPlayer : mTMediaPlayerList) {
        if (!pTestPlayer) {
            continue;
        }

        if (pTestPlayer->getPlaybackState() == QMediaPlayer::PlayingState &&
            pTestPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (pTestPlayer->mediaData().mediaAbsolutePathFileName().endsWith(mediaData.mediaAbsolutePathFileName())) {
                // Is the same sound or music playing?
                pTestPlayer->setMediaData(mediaData);
                pTestPlayer->setVolume(mediaData.mediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.mediaVolume());
                return pTestPlayer;  // Return a pointer to the matched player
            }
        }
    }

    return nullptr;  // No matching player found
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#priority:_1_to_100
bool TMedia::doesMediaHavePriorityToPlay(TMediaData& mediaData, const QString& absolutePathFileName)
{
    if (mediaData.mediaPriority() == TMediaData::MediaPriorityNotSet) {
        return true;  // Default behavior: assume priority if not set
    }

    int maxMediaPriority = 0;

    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = TMedia::findMediaPlayersByCriteria(mediaData);

    for (const auto& pTestPlayer : mTMediaPlayerList) {
        if (!pTestPlayer) {
            continue;
        }

        if (pTestPlayer->getPlaybackState() == QMediaPlayer::PlayingState &&
            pTestPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (!pTestPlayer->mediaData().mediaAbsolutePathFileName().endsWith(absolutePathFileName)) {
                // Is it a different sound or music than specified?
                if (pTestPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet &&
                    pTestPlayer->mediaData().mediaPriority() > maxMediaPriority) {
                    maxMediaPriority = pTestPlayer->mediaData().mediaPriority();
                }
            }
        }
    }

    if (maxMediaPriority >= mediaData.mediaPriority()) { // Our media has a lower priority
        return false;
    }

    // Stop lower-priority media if we have a higher priority
    TMediaData stopMediaData{};
    stopMediaData.setMediaProtocol(mediaData.mediaProtocol());
    stopMediaData.setMediaType(mediaData.mediaType());
    stopMediaData.setMediaPriority(mediaData.mediaPriority());
    mpHost->mpMedia->stopMedia(stopMediaData);

    return true;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#key
void TMedia::matchMediaKeyAndStopMediaVariants(TMediaData& mediaData, const QString& absolutePathFileName)
{
    QList<std::shared_ptr<TMediaPlayer>> mTMediaPlayerList = TMedia::findMediaPlayersByCriteria(mediaData);

    for (const auto& pTestPlayer : mTMediaPlayerList) {
        if (!pTestPlayer) {
            continue;
        }

        if (pTestPlayer->getPlaybackState() == QMediaPlayer::PlayingState &&
            pTestPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {

            if (!mediaData.mediaKey().isEmpty() && !pTestPlayer->mediaData().mediaKey().isEmpty()
                && mediaData.mediaKey() == pTestPlayer->mediaData().mediaKey()) {
                // Does it have the same key?
                if (!pTestPlayer->mediaData().mediaAbsolutePathFileName().endsWith(absolutePathFileName)
                    || (!mediaData.mediaUrl().isEmpty() && !pTestPlayer->mediaData().mediaUrl().isEmpty()
                        && mediaData.mediaUrl() != pTestPlayer->mediaData().mediaUrl())) {
                    // Is it a different sound or music than specified?
                    TMediaData stopMediaData = pTestPlayer->mediaData();
                    mpHost->mpMedia->stopMedia(stopMediaData); // Stop it!
                }
            }
        }
    }
}

bool TMedia::setupVideo(const std::shared_ptr<TMediaPlayer>& player)
{
    if (!player) {
        return false;
    }

    auto mpConsole = mpHost->mpConsole;

    if (!mpConsole) {
        return false;
    }

    auto target = player->mediaData().mediaKey();

    if (target.isEmpty()) {
        qWarning() << qsl("TMedia::setupVideo() ERROR - 'key' not specified for video.");
        return false;
    }

    QString widgetType = TMediaData::MediaWidgetLabel;
    QWidget* targetWidget = nullptr;

    // Attempt to retrieve the existing widget, labels first
    targetWidget = mpConsole->mLabelMap.value(target);

    if (!targetWidget) {
        targetWidget = mpConsole->mSubConsoleMap.value(target);
        if (targetWidget) {
            widgetType = TMediaData::MediaWidgetWindow;
        }
    }

    // Ensure we now have a valid target widget
    if (!targetWidget) {
        qWarning() << qsl("TMedia::setupVideo() ERROR - No matching widget for 'key' = %1 to present video.").arg(target);
        return false;
    }

    player->mediaData().setMediaWidget(widgetType);

    // Assign video widget to the target widget
    QVideoWidget* myVideoWidget = nullptr;
    if (widgetType == TMediaData::MediaWidgetLabel) {
        myVideoWidget = qobject_cast<TLabel*>(targetWidget)->mpVideoWidget;
    } else if (widgetType == TMediaData::MediaWidgetWindow) {
        myVideoWidget = qobject_cast<TConsole*>(targetWidget)->mpVideoWidget;
    }

    if (!myVideoWidget) {
        myVideoWidget = new QVideoWidget();
        myVideoWidget->setParent(targetWidget);
        myVideoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        if (widgetType == TMediaData::MediaWidgetLabel) {
            QObject::connect(qobject_cast<TLabel*>(targetWidget), &TLabel::resized, myVideoWidget, [targetWidget, myVideoWidget]() {
                myVideoWidget->resize(targetWidget->size());
            });
        } else if (widgetType == TMediaData::MediaWidgetWindow) {
            QObject::connect(qobject_cast<TConsole*>(targetWidget), &TConsole::resized, myVideoWidget, [targetWidget, myVideoWidget]() {
                myVideoWidget->resize(targetWidget->size());
            });
        }
    }

    if (targetWidget->isHidden()) {
        targetWidget->show();
    }

    myVideoWidget->resize(targetWidget->size());
    player->mediaPlayer()->setVideoOutput(myVideoWidget);
    myVideoWidget->show();

    return true;
}

void TMedia::play(TMediaData& mediaData)
{
    if (!isMediaProtocolAllowed(mediaData)) {
        return;
    }

    if (mediaData.mediaInput() == TMediaData::MediaInputNotSet) {
        return;
    }

    const QStringList fileNameList = TMedia::getFileNameList(mediaData);

    if (fileNameList.isEmpty()) {
        qWarning() << qsl("TMedia::play() WARNING - could not generate a list of media file names.");
        return;
    }

    std::shared_ptr<TMediaPlayer> pPlayer;

    // Only match an existing media player for music and video
    if (mediaData.mediaType() == TMediaData::MediaTypeMusic || mediaData.mediaType() == TMediaData::MediaTypeVideo) {
        pPlayer = TMedia::matchMediaPlayer(mediaData);
    }

    bool sameMediaIsPlaying = (pPlayer != nullptr);

    // If no existing player is found, create a new one
    if (!sameMediaIsPlaying) {
        pPlayer = getMediaPlayer(mediaData);

        if (!pPlayer) {
            qWarning() << "TMedia::play() - Failed to create a new TMediaPlayer.";
            return;
        }
    }

    if (!pPlayer->mediaPlayer()) {
        qWarning() << "TMedia::play() - mediaPlayer() is null!";
        return;
    }

    // Ensure the player has a valid playlist
    TMediaPlaylist *playlist = pPlayer->playlist();

    if (!sameMediaIsPlaying) {
        if (!playlist) {
            playlist = new TMediaPlaylist;
        }

        playlist->clear();
        playlist->setPlaybackMode(TMediaPlaylist::Sequential);
    }

    QString absolutePathFileName;

    if (mediaData.mediaLoops() == TMediaData::MediaLoopsDefault) { // Play once
        playlist->setPlaybackMode(TMediaPlaylist::Sequential);

        if (sameMediaIsPlaying) {
            if (mediaData.mediaContinue() == TMediaData::MediaContinueRestart) {
                mpHost->mpMedia->stopMedia(mediaData); // Stop the media; Restart it below.
                playlist->clear();
            } else {
                if (!playlist->isEmpty() && playlist->mediaCount() > 1) { // Purge media from the previous playlist
                    playlist->removeMedia(playlist->nextIndex(), playlist->mediaCount());
                }

                return; // No action required. Continue playing the same media.
            }
        }

        absolutePathFileName = fileNameList.size() > 1
                                   ? fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()))
                                   : (mediaData.mediaInput() == TMediaData::MediaInputStream
                                          ? TMedia::getStreamUrl(mediaData)
                                          : fileNameList.at(0));

        if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) {
            return;
        }

        if (!mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP ||
                                                mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
            TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName);
        }

        const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile
                                     ? QUrl::fromLocalFile(absolutePathFileName)
                                     : QUrl(absolutePathFileName);
        pPlayer->mediaPlayer()->setSource(mediaSource);
    } else {
        if (mediaData.mediaLoops() == TMediaData::MediaLoopsRepeat) { // Repeat indefinitely
            playlist->setPlaybackMode(TMediaPlaylist::Loop);

            if (sameMediaIsPlaying) {
                if (mediaData.mediaContinue() == TMediaData::MediaContinueRestart) {
                    mpHost->mpMedia->stopMedia(mediaData);
                    playlist->clear();
                } else {
                    return; // Continue playing
                }
            }

            absolutePathFileName = fileNameList.size() > 1
                                       ? fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()))
                                       : (mediaData.mediaInput() == TMediaData::MediaInputStream
                                              ? TMedia::getStreamUrl(mediaData)
                                              : fileNameList.at(0));

            if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) {
                return;
            }

            if (!mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP ||
                                                    mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
                TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName);
            }

            const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile
                                         ? QUrl::fromLocalFile(absolutePathFileName)
                                         : QUrl(absolutePathFileName);
            playlist->addMedia(mediaSource);
        } else { // Play a finite number of times
            playlist->setPlaybackMode(TMediaPlaylist::Sequential);

            if (sameMediaIsPlaying) {
                if (mediaData.mediaContinue() == TMediaData::MediaContinueRestart) {
                    mpHost->mpMedia->stopMedia(mediaData); // Stop the media; Restart it below.
                    playlist->clear();
                } else {
                    if (!playlist->isEmpty() && playlist->mediaCount() > 1) { // Purge media from the previous playlist
                        playlist->removeMedia(playlist->nextIndex(), playlist->mediaCount());
                    }

                    mediaData.setMediaLoops(mediaData.mediaLoops() - 1); // Subtract the currently playing media from the total
                }
            }

            for (int k = 0; k < mediaData.mediaLoops(); k++) {
                absolutePathFileName = fileNameList.size() > 1
                                           ? fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()))
                                           : (mediaData.mediaInput() == TMediaData::MediaInputStream
                                                  ? TMedia::getStreamUrl(mediaData)
                                                  : fileNameList.at(0));

                if (k == 0 && !TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) {
                    return;
                }

                if (k == 0 && !mediaData.mediaKey().isEmpty() &&
                    (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP ||
                     mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
                    TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName);
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
        pPlayer->setPlaylist(playlist);
        pPlayer->mediaPlayer()->setSource(playlist->currentMedia());
    }

    // Set volume and start position
    pPlayer->setVolume(mediaData.mediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.mediaVolume());
    pPlayer->mediaPlayer()->setPosition(mediaData.mediaStart());

    // Set mute state based on protocol
    switch (mediaData.mediaProtocol()) {
        case TMediaData::MediaProtocolAPI:
            pPlayer->mediaPlayer()->audioOutput()->setMuted(mudlet::self()->muteAPI());
            break;
        case TMediaData::MediaProtocolGMCP:
        case TMediaData::MediaProtocolMSP:
            pPlayer->mediaPlayer()->audioOutput()->setMuted(mudlet::self()->muteGame());
            break;
    }

    // Handle video setup if applicable
    if (mediaData.mediaType() == TMediaData::MediaTypeVideo && !setupVideo(pPlayer)) {
        return;
    }

    pPlayer->mediaPlayer()->play();
    updateMediaPlayerList(std::move(pPlayer));
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

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#close
TMediaData::MediaClose TMedia::parseJSONByMediaClose(QJsonObject& json)
{
    TMediaData::MediaClose mediaClose = TMediaData::MediaCloseDefault;

    auto mediaCloseJSON = json.value(qsl("close"));

    if (mediaCloseJSON != QJsonValue::Undefined && mediaCloseJSON.isString() && !mediaCloseJSON.toString().isEmpty()) {
        if (mediaCloseJSON.toString() == "false") {
            mediaClose = TMediaData::MediaCloseDefault;
        } else {
            mediaClose = TMediaData::MediaCloseEnabled;
        }
    } else if (mediaCloseJSON != QJsonValue::Undefined && mediaCloseJSON.toBool(true)) {
        mediaClose = TMediaData::MediaCloseEnabled;
    }

    return mediaClose;
}

// Documentation: https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol#caption
QString TMedia::parseJSONByMediaCaption(QJsonObject& json)
{
    // Returns the 'caption' field if present, else empty string
    return json.contains("caption") && json["caption"].isString() ? json["caption"].toString() : QString();
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
    mediaData.setMediaCaption(TMedia::parseJSONByMediaCaption(json));

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
    mediaData.setMediaClose(TMedia::parseJSONByMediaClose(json));
    mediaData.setMediaCaption(TMedia::parseJSONByMediaCaption(json));

    TMedia::playMedia(mediaData);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#Pausing_Media
void TMedia::parseJSONForMediaPause(QJsonObject& json)
{
    TMediaData mediaData{};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaType(TMedia::parseJSONByMediaType(json));
    mediaData.setMediaFileName(TMedia::parseJSONByMediaFileName(json));
    mediaData.setMediaKey(TMedia::parseJSONByMediaKey(json));
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaPriority(TMedia::parseJSONByMediaPriority(json));

    TMedia::pauseMedia(mediaData);
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

void TMedia::printClosedCaption(const TMediaData& mediaData, const QString& action) const
{
    if (!mpHost || !mpHost->mEnableClosedCaption || !mpHost->mpConsole)
        return;

    QString message;

    if (!mediaData.mediaCaption().isEmpty()) {
        message = qsl("[%1 %2]\n").arg(mediaData.mediaCaption(), action);
    } else {
        const QString mediaType = mediaData.mediaType() == TMediaData::MediaTypeMusic ? tr("music") :
                                  mediaData.mediaType() == TMediaData::MediaTypeVideo ? tr("video") : tr("sound");
        const QString mediaKey = mediaData.mediaKey();
        const QString mediaFileName = mediaData.mediaFileName();
        if (mediaKey.isEmpty()) {
            message = qsl("[%1 \"%2\" %3]\n").arg(mediaType, mediaFileName, action);
        } else {
            message = qsl("[%1 %2 \"%3\" %4]\n").arg(mediaType, mediaKey, mediaFileName, action);
        }
    }

    mpHost->mpConsole->print(message);
}
// End Private
