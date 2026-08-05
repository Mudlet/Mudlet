/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020, 2022-2026 by Stephen Lyons                   *
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

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaMethod>
#include <QNetworkDiskCache>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

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

        // A relative path can still escape the profile's media directory via ".."
        // segments; reject those so a hostile server cannot read or overwrite
        // files elsewhere on disk (relative sub-directories remain permitted).
        if ((mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP) && mediaFilePathEscapesMediaDir(mediaData)) {
            qWarning() << qsl("TMedia::playMedia() WARNING - rejected a media file name that escapes the profile media directory: %1.").arg(mediaData.mediaFileName());
            return;
        }

        if (!mediaData.mediaFileName().contains(QLatin1Char('*')) && !mediaData.mediaFileName().contains(QLatin1Char('?'))) { // File path wildcards are * and ?
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
    QList<TMediaData> matchingMediaDataList;

    if (!isMediaProtocolAllowed(mediaData)) {
        return matchingMediaDataList;
    }

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mediaPlayerList.isEmpty()) {
        return matchingMediaDataList;
    }

    if (!mediaData.mediaFileName().isEmpty()) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return matchingMediaDataList; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute, but get copied into the media folder for processing. Trim the path from the file name.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section('/', -1));
        }
    }

    for (const auto& pPlayer : std::as_const(mediaPlayerList)) {
        if (!pPlayer) {
            continue;
        }

        if (pPlayer->getPlaybackState() != QMediaPlayer::PlayingState && pPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet
            && pPlayer->mediaData().mediaPriority() > mediaData.mediaPriority()) {
            continue;
        }

        matchingMediaDataList.append(pPlayer->mediaData());
    }

    return matchingMediaDataList;
}

QList<TMediaData> TMedia::pausedMedia(TMediaData& mediaData)
{
    QList<TMediaData> matchingMediaDataList;

    if (!isMediaProtocolAllowed(mediaData)) {
        return matchingMediaDataList;
    }

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mediaPlayerList.isEmpty()) {
        return matchingMediaDataList;
    }

    if (!mediaData.mediaFileName().isEmpty()) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return matchingMediaDataList; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute but get copied into the media folder. Trim the path.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section('/', -1));
        }
    }

    for (const auto& pPlayer : std::as_const(mediaPlayerList)) {
        if (!pPlayer) {
            continue;
        }

        if (pPlayer->getPlaybackState() != QMediaPlayer::PausedState) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet
            && pPlayer->mediaData().mediaPriority() > mediaData.mediaPriority()) {
            continue;
        }

        matchingMediaDataList.append(pPlayer->mediaData());
    }

    return matchingMediaDataList;
}

void TMedia::pauseMedia(TMediaData& mediaData)
{
    if (!isMediaProtocolAllowed(mediaData)) {
        return;
    }

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mediaPlayerList.isEmpty()) {
        return;
    }

    if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute but get copied into the media folder. Trim the path.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }
    }

    for (const auto& pPlayer : std::as_const(mediaPlayerList)) {
        if (!pPlayer) {
            continue;
        }

        if (pPlayer->getPlaybackState() != QMediaPlayer::PlayingState) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet
            && pPlayer->mediaData().mediaPriority() >= mediaData.mediaPriority()) {
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

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mediaPlayerList.isEmpty()) {
        return;
    }

    if (!mediaData.mediaFileName().isEmpty() && mediaData.mediaInput() == TMediaData::MediaInputFile) {
        const bool fileRelative = TMedia::isFileRelative(mediaData);

        if (!fileRelative && (mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP || mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP)) {
            return; // MSP and GMCP files should not have absolute paths.
        }

        // API files may start as absolute but get copied into the media folder. Trim the path.
        if (!fileRelative) {
            mediaData.setMediaFileName(mediaData.mediaFileName().section(QLatin1Char('/'), -1));
        }
    }

    for (auto& pPlayer : mediaPlayerList) {
        if (!pPlayer) {
            continue;
        }

        if (!isMediaMatch(pPlayer, mediaData)) {
            continue;
        }

        if (mediaData.mediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet
            && pPlayer->mediaData().mediaPriority() >= mediaData.mediaPriority()) {
            continue;
        }

        // A pooled player between tracks holds no source and has nothing to stop. Criteria this
        // broad are the common case - a bare stopMusic() or Client.Media.Stop {} matches every
        // player there is - so without this each idle one would be ended all over again, and
        // told about with an empty file name and the key and tag of its last track.
        if (!pPlayer->mediaPlayer() || pPlayer->mediaPlayer()->source().isEmpty()) {
            continue;
        }

        // Whichever way this track is being ended below, it is not to start again. A looping
        // or multi-entry track restarts itself from the EndOfMedia handler in
        // connectMediaPlayer(), which would undo the stop that was just asked for - on a
        // StoppedState-first backend that signal can still be on its way when the stop
        // arrives. An emptied playlist is what that handler checks, and play() builds a fresh
        // one whenever this player is picked up again.
        if (pPlayer->playlist()) {
            pPlayer->playlist()->clear();
        }

        if ((mediaData.mediaFadeAway() == TMediaData::MediaFadeAwayEnabled || mediaData.mediaFadeOut() != TMediaData::MediaFadeNotSet)
            && pPlayer->mediaData().mediaEnd() == TMediaData::MediaEndNotSet) {
            const int finishPosition = pPlayer->mediaData().mediaFinish();
            const int duration = pPlayer->mediaPlayer()->duration();
            const int currentPosition = pPlayer->mediaPlayer()->position();
            const int fadeOut = pPlayer->mediaData().mediaFadeOut() ? pPlayer->mediaData().mediaFadeOut() : mediaData.mediaFadeOut();
            const int remainingDuration = (finishPosition != TMediaData::MediaFinishNotSet ? finishPosition : duration) - currentPosition;
            const int endDuration = fadeOut != TMediaData::MediaFadeNotSet ? std::min(remainingDuration, fadeOut) : std::min(remainingDuration, 5000);
            const int endPosition = currentPosition + endDuration;

            //: This word is part of a sentence like "Music fades" when the music is about to stop.
            printClosedCaption(pPlayer->mediaData(), tr("fades"));

            TMediaData updateMediaData = pPlayer->mediaData();
            updateMediaData.setMediaFadeOut(endDuration);
            updateMediaData.setMediaEnd(endPosition);
            pPlayer->setMediaData(updateMediaData);
            TMedia::updateMediaPlayerList(std::move(pPlayer));

            continue;
        }

        // **Stop the player but keep it for reuse**
        // Only a player that had started reports a change back to StoppedState, and that
        // signal is what ends the playback and releases the source. One that is still loading
        // - where a stop issued soon after a play lands on an asynchronous backend - is
        // already stopped as far as Qt is concerned, so it reports nothing and its source
        // would be held for good.
        const bool willReportItsOwnStop = pPlayer->getPlaybackState() != QMediaPlayer::StoppedState;

        pPlayer->mediaPlayer()->stop();

        if (!willReportItsOwnStop) {
            releaseMediaSourceAfterEvents(pPlayer, pPlayer->mediaData(), PlaybackEnd::Stopped);
            // Announced at most once per playback, so a handler that stops the media it has
            // just been told about does not arrive back here for the same track: the source it
            // reads as live stays set until the deferred release above runs.
            raiseMediaFinishedEvent(pPlayer, pPlayer->mediaPlayer()->source(), pPlayer->mediaData());
        }
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

    if (!mediaDir.removeRecursively()) {
        qWarning() << qsl("TMedia::purgeMediaCache() WARNING - not able to remove all of directory: %1").arg(mediaPath);
        return false;
    }

    return true;
}

void TMedia::refreshAudioDevices()
{
    TMediaData mediaData{};
    mediaData.setMediaProtocol(TMediaData::MediaProtocolNotSet);
    mediaData.setMediaType(TMediaData::MediaTypeNotSet);

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    for (const auto& player : mediaPlayerList) {
        if (!player) {
            continue;
        }
        auto* mp = player->mediaPlayer();
        if (mp->playbackState() == QMediaPlayer::StoppedState) {
            continue;
        }
        player->refreshAudioOutput();
    }
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
    if ((mediaData.mediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) || (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia)) {
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
        return (mediaData.mediaType() == TMediaData::MediaTypeSound) ? mGMCPSoundList : (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? mGMCPMusicList : mGMCPVideoList;

    case TMediaData::MediaProtocolAPI:
        if (mediaData.mediaType() == TMediaData::MediaTypeNotSet) {
            QList<std::shared_ptr<TMediaPlayer>> combinedList;
            combinedList.append(mAPISoundList);
            combinedList.append(mAPIMusicList);
            combinedList.append(mAPIVideoList);
            return combinedList;
        }
        return (mediaData.mediaType() == TMediaData::MediaTypeSound) ? mAPISoundList : (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? mAPIMusicList : mAPIVideoList;

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
        return (mediaData.mediaType() == TMediaData::MediaTypeSound)   ? mMSPSoundList
               : (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? mMSPMusicList
               : (mediaData.mediaType() == TMediaData::MediaTypeVideo) ? mGMCPVideoList
                                                                       : QList<std::shared_ptr<TMediaPlayer>>(); // Return empty list
    }

    return {}; // Default empty list fallback
}

bool TMedia::isMediaMatch(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& mediaData)
{
    if (!player) {
        return false;
    }

    if (!mediaData.mediaKey().isEmpty() && !player->mediaData().mediaKey().isEmpty() && player->mediaData().mediaKey() != mediaData.mediaKey()) {
        return false;
    }

    if (!mediaData.mediaFileName().isEmpty() && !player->mediaData().mediaFileName().isEmpty() && player->mediaData().mediaFileName() != mediaData.mediaFileName()) {
        return false;
    }

    if (!mediaData.mediaTag().isEmpty() && !player->mediaData().mediaTag().isEmpty() && player->mediaData().mediaTag() != mediaData.mediaTag()) {
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

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    if (mediaPlayerList.isEmpty()) {
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

    for (const auto& pPlayer : std::as_const(mediaPlayerList)) {
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

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    for (const auto& pPlayer : std::as_const(mediaPlayerList)) {
        if (!pPlayer || !pPlayer->mediaPlayer() || pPlayer->mediaPlayer()->source().isEmpty()) {
            continue; // A pooled player between tracks has nothing playing to stop
        }

        // Everything the ending is described by has to be read before the source goes, because
        // releasing is what makes it unreadable.
        const TMediaData endedData = pPlayer->mediaData();
        const QUrl endedUrl = pPlayer->mediaPlayer()->source();
        const bool hadVideoOutput = pPlayer->mediaPlayer()->videoOutput() != nullptr;
        const quint64 claimedAt = pPlayer->claimGeneration();

        // No loop is to survive this: the EndOfMedia handler restarts one from the playlist,
        // and a StoppedState-first backend can still have that signal on its way.
        if (pPlayer->playlist()) {
            pPlayer->playlist()->clear();
        }

        pPlayer->mediaPlayer()->stop();

        // stop() can deliver StoppedState synchronously, whose handler raises sysMediaFinished
        // and so lets a script hand this player straight to another track. The release below is
        // direct - it carries no generation of its own for releaseMediaSourceAfterEvents()'
        // checks to catch - so this is the one thing standing between that new track and having
        // its source cleared out from under it.
        if (pPlayer->claimGeneration() != claimedAt) {
            continue;
        }

        // Released here rather than left to releaseMediaSourceAfterEvents(): this is a
        // teardown, so there is no loop left to restart and no reason to wait a turn, and a
        // caller may need the files free straight away - purgeMediaCache() deletes them. The
        // empty source left behind is also what tells any release already scheduled for this
        // player to stay quiet when its turn comes, so nothing is said twice.
        pPlayer->releaseSource();

        if (endedData.mediaWidget() == TMediaData::MediaWidgetLabel && endedData.mediaClose() == TMediaData::MediaCloseEnabled && hadVideoOutput) {
            emit signal_hideVideoOutput(pPlayer.get());
        }

        // Announced from here because releasing synchronously means no deferred turn will do
        // it: on a backend that reports StoppedState asynchronously nothing else ever would,
        // and a script waiting on sysMediaFinished would sit through the teardown none the
        // wiser. Skipped when stop() above already announced it - see endAnnounced().
        raiseMediaFinishedEvent(pPlayer, endedUrl, endedData);

        //: This word is part of a sentence like "Music stops" when the music is about to stop.
        printClosedCaption(endedData, tr("stops"));
    }
}

int TMedia::playersHoldingSource() const
{
    const auto countHeld = [](const QList<std::shared_ptr<TMediaPlayer>>& list) {
        int held = 0;
        for (const auto& player : list) {
            if (player && player->mediaPlayer() && !player->mediaPlayer()->source().isEmpty()) {
                ++held;
            }
        }
        return held;
    };

    return countHeld(mMSPSoundList) + countHeld(mMSPMusicList) + countHeld(mGMCPSoundList) + countHeld(mGMCPMusicList) + countHeld(mGMCPVideoList) + countHeld(mAPISoundList) + countHeld(mAPIMusicList)
           + countHeld(mAPIVideoList);
}

int TMedia::playersInPlayingState() const
{
    const auto countPlaying = [](const QList<std::shared_ptr<TMediaPlayer>>& list) {
        int playing = 0;
        for (const auto& player : list) {
            if (player && player->getPlaybackState() == QMediaPlayer::PlayingState) {
                ++playing;
            }
        }
        return playing;
    };

    return countPlaying(mMSPSoundList) + countPlaying(mMSPMusicList) + countPlaying(mGMCPSoundList) + countPlaying(mGMCPMusicList) + countPlaying(mGMCPVideoList) + countPlaying(mAPISoundList)
           + countPlaying(mAPIMusicList) + countPlaying(mAPIVideoList);
}

int TMedia::mediaPlayerCount() const
{
    return mMSPSoundList.size() + mMSPMusicList.size() + mGMCPSoundList.size() + mGMCPMusicList.size() + mGMCPVideoList.size() + mAPISoundList.size() + mAPIMusicList.size() + mAPIVideoList.size();
}

void TMedia::setMediaPlayersMuted(const TMediaData::MediaProtocol mediaProtocol, const bool state)
{
    TMediaData mediaData{};
    mediaData.setMediaProtocol(mediaProtocol);

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    for (const auto& player : std::as_const(mediaPlayerList)) {
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
            qWarning() << qsl("TMedia::isFileRelative() WARNING - attempt made to send an absolute path as a media file name: %1.  Only relative paths are permitted.").arg(mediaData.mediaFileName());
        }
    } else {
        isFileRelative = true;
    }

    return isFileRelative;
}

bool TMedia::mediaFilePathEscapesMediaDir(TMediaData& mediaData) const
{
    return mediaFilePathEscapesMediaDir(mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName()), mediaData.mediaFileName());
}

// Returns true if mediaFileName would resolve to a location outside mediaRoot. Two layers:
//  1. A lexical check (QDir::cleanPath) rejects "../" traversal without touching the disk.
//  2. A canonical check resolves symlinks in the existing path components, so a symlink that
//     already lives under the media directory but points elsewhere cannot be used to escape.
//     The target file itself normally does not exist yet (it is about to be downloaded), so we
//     canonicalise the deepest ancestor that does exist and re-check containment against the
//     canonicalised media root. Legitimate relative sub-directories and wildcards stay inside
//     mediaRoot and are permitted.
bool TMedia::mediaFilePathEscapesMediaDir(const QString& mediaRoot, const QString& mediaFileName)
{
    if (mediaFileName.isEmpty()) {
        return false;
    }

    const QString root = QDir::cleanPath(mediaRoot);
    const QString resolved = QDir::cleanPath(qsl("%1/%2").arg(root, mediaFileName));

    // Layer 1 - lexical containment.
    if (resolved != root && !resolved.startsWith(root + QLatin1Char('/'))) {
        return true;
    }

    // Layer 2 - canonical containment (symlink-aware).
    const QString canonicalRoot = QFileInfo(root).canonicalFilePath();

    if (canonicalRoot.isEmpty()) {
        // The media root does not exist yet, so there are no symlink components to follow; the
        // lexical check above is authoritative.
        return false;
    }

    QString ancestor = resolved;
    QString canonicalAncestor;

    while (!ancestor.isEmpty()) {
        canonicalAncestor = QFileInfo(ancestor).canonicalFilePath();

        if (!canonicalAncestor.isEmpty()) {
            break; // deepest existing ancestor found
        }

        const int lastSlash = ancestor.lastIndexOf(QLatin1Char('/'));

        if (lastSlash <= 0) {
            break;
        }

        ancestor = ancestor.left(lastSlash);
    }

    if (canonicalAncestor.isEmpty()) {
        return false;
    }

    return canonicalAncestor != canonicalRoot && !canonicalAncestor.startsWith(canonicalRoot + QLatin1Char('/'));
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
            return;
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
                if (!localFile.commit()) {
                    event.mArgumentList << QLatin1String("sysDownloadError");
                    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                    event.mArgumentList << QLatin1String("Couldn't save to the destination file");
                    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                    event.mArgumentList << mediaData.mediaAbsolutePathFileName();
                    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                    event.mArgumentList << qsl("Failed to commit file: %1").arg(localFile.errorString());
                    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

                    reply->deleteLater();
                    mpHost->raiseEvent(event);
                    return;
                }

                event.mArgumentList << QLatin1String("sysDownloadDone");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << mediaData.mediaAbsolutePathFileName();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << QString::number(bytesWritten);
                event.mArgumentTypeList << ARGUMENT_TYPE_NUMBER;

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
    // Central guard for every download/write path (Client.Media.Play preloads
    // via playMedia(), Client.Media.Load via parseJSONForMediaLoad()): never
    // write a server-supplied file name that escapes the profile media
    // directory through ".." segments.
    if (mediaFilePathEscapesMediaDir(mediaData)) {
        qWarning() << qsl("TMedia::downloadFile() WARNING - refused a media file name that escapes the profile media directory: %1.").arg(mediaData.mediaFileName());
        return;
    }

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
    }

    // Media is fetched from the network only. Refuse other schemes (e.g. file://) so a
    // server-supplied media URL cannot turn this download into a local file read.
    const QString scheme = fileUrl.scheme();
    if (scheme != qsl("http") && scheme != qsl("https")) {
        qWarning() << qsl("TMedia::downloadFile() WARNING - refused to download media from a non-HTTP(S) URL: %1").arg(fileUrl.toString());
        return;
    }

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
    if (!player || !player->mediaPlayer()) {
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
                        lockedPlayer->continuePlaying(nextMedia);
                    } else if (lockedPlayer->playlist()->playbackMode() == TMediaPlaylist::Loop) {
                        lockedPlayer->playlist()->setCurrentIndex(0);
                        lockedPlayer->continuePlaying(lockedPlayer->playlist()->currentMedia());
                    }
                }
            }
        }
    });

    // Error connection
    disconnect(player->mediaPlayer(), &QMediaPlayer::errorOccurred, nullptr, nullptr);
    connect(player->mediaPlayer(), &QMediaPlayer::errorOccurred, this, [this, weakPlayer](QMediaPlayer::Error error, const QString& errorString) {
        const auto lockedPlayer = weakPlayer.lock();

        if (!lockedPlayer || !lockedPlayer->mediaPlayer() || error == QMediaPlayer::NoError) {
            return;
        }

        qWarning().noquote() << qsl("TMedia::connectMediaPlayer() WARNING - media player error %1 on \"%2\": %3")
                                        .arg(QString::number(static_cast<int>(error)), lockedPlayer->mediaPlayer()->source().toString(), errorString);

        if (mudlet::smDebugMode && mpHost && mpHost->mpConsole) {
            //: %1 is the media backend's own description of what went wrong, e.g. "Failed to load media".
            mpHost->mpConsole->printSystemMessage(qsl("%1\n").arg(tr("Media error: %1").arg(errorString)));
        }

        // Only a failure nothing else will report is ended from here. A track that was playing
        // reports StoppedState when the error takes it down, and the playback state handler
        // ends it from there. That leaves two cases: a player already stopped, which is where a
        // load failure lands because claimSource() leaves it stopped and there is no state to
        // change from; and Qt's darwin backend, which reports PlayingState for media it has
        // just failed to load and then never moves off it. InvalidMedia catches that second
        // case and only that one - it cannot be asked to carry the first, because Qt's FFmpeg
        // backend raises this signal before it sets the status.
        if (lockedPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::InvalidMedia && lockedPlayer->getPlaybackState() != QMediaPlayer::StoppedState) {
            return;
        }

        // Nothing else will end it: a source set on a player that was already stopped - which
        // is what every claimSource() on a new or finished player does, and what a loop restart
        // or playlist advance does from the EndOfMedia handler - has no state to change from.
        // Left alone the track falls silent still holding a source nothing will ever release,
        // and a script waiting on sysMediaFinished to start the next one waits forever.
        //
        // Ended a turn from now rather than here, because setSource() can deliver this error
        // synchronously from inside claimSource(): sysMediaFinished would then reach a script
        // in the middle of the playMusic() call that asked for the track, and a handler that
        // responds by playing the same undecodable file again would recurse until the stack
        // gave out. The claim generation says whether this failure is still anyone's to report
        // by the time the turn comes: a track that took the player over in between owns it now.
        const quint64 claimedAt = lockedPlayer->claimGeneration();

        QTimer::singleShot(0, this, [this, weakPlayer, claimedAt] {
            const auto endingPlayer = weakPlayer.lock();

            if (!endingPlayer || !endingPlayer->mediaPlayer() || endingPlayer->claimGeneration() != claimedAt) {
                return;
            }

            // The release armed below ignores playback state by design, since darwin claims to
            // be playing media it has just failed to load. That makes this the only place an
            // error the backend recovered from can be told apart from one it did not: a turn
            // on, media it has condemned says so with InvalidMedia, and media that is playing
            // without having been condemned is fine after all and must be left alone.
            if (endingPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::InvalidMedia && endingPlayer->getPlaybackState() == QMediaPlayer::PlayingState) {
                return;
            }

            releaseMediaSourceAfterEvents(endingPlayer, endingPlayer->mediaData(), PlaybackEnd::Failed);
            raiseMediaFinishedEvent(endingPlayer, endingPlayer->mediaPlayer()->source(), endingPlayer->mediaData());
        });
    });

    // Playback state changed connection
    disconnect(player->mediaPlayer(), &QMediaPlayer::playbackStateChanged, nullptr, nullptr);
    connect(player->mediaPlayer(), &QMediaPlayer::playbackStateChanged, this, [this, weakPlayer](QMediaPlayerPlaybackState playbackState) {
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
                        const double fadeInVolume = static_cast<double>(volume * (progress - startPosition)) / static_cast<double>(relativeFadeInPosition - startPosition);
                        lockedPlayer->setVolume(qRound(fadeInVolume));
                        actionTaken = true;
                    } else if (progress == relativeFadeInPosition) {
                        lockedPlayer->setVolume(volume);
                        actionTaken = true;
                    }
                }

                if (!actionTaken && fadeOutUsed && progress > 0) {
                    if (progress > relativeFadeOutPosition) {
                        const double fadeOutVolume = static_cast<double>(volume * (relativeDuration - progress)) / static_cast<double>(fadeOutDuration);
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
    mediaList.erase(std::remove_if(mediaList.begin(),
                                   mediaList.end(),
                                   [](const std::shared_ptr<TMediaPlayer>& player) {
                                       return player->getPlaybackState() == QMediaPlayer::StoppedState;
                                   }),
                    mediaList.end());
}

// Helper for updating media player lists (now a static TMedia method)
template <typename T>
void TMedia::updateList(QList<std::shared_ptr<T>>& list, int index, std::shared_ptr<T> player, TMedia* mediaInstance)
{
    if (index == -1) {
        qDebug() << "TMedia::updateList() - Adding new player to list (index == -1)";
        list.append(std::move(player));
    } else if (index >= 0 && index < list.size()) {
        qDebug() << "TMedia::updateList() - Replacing existing player at index:" << index;
        list[index] = std::move(player);
    } else {
        qWarning() << "TMedia::updateList() - Invalid index:" << index << " for list size:" << list.size() << ". Appending instead.";
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

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = findMediaPlayersByCriteria(mediaData);

    qDebug() << "TMedia::updateMediaPlayerList() - Searching for existing player in list.";
    for (int i = 0; i < mediaPlayerList.size(); ++i) {
        if (mediaPlayerList[i] && mediaPlayerList[i]->mediaPlayer() == player->mediaPlayer()) {
            matchedMediaPlayerIndex = i;
            connectMediaPlayer(mediaPlayerList[i]); // Ensure it's connected
            break;
        }
    }
    if (matchedMediaPlayerIndex != -1) {
        qDebug() << "TMedia::updateMediaPlayerList() - Found existing player at index:" << matchedMediaPlayerIndex;
    } else {
        qDebug() << "TMedia::updateMediaPlayerList() - No existing player found. Appending new one.";
    }

    QList<std::shared_ptr<TMediaPlayer>>* list = nullptr;

    qDebug() << "TMedia::updateMediaPlayerList() - Updating list for protocol:" << mediaData.mediaProtocol() << "and type:" << mediaData.mediaType();
    switch (mediaData.mediaProtocol()) {
    case TMediaData::MediaProtocolMSP:
        list = (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? &mMSPMusicList : &mMSPSoundList;
        break;
    case TMediaData::MediaProtocolGMCP:
        list = (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? &mGMCPMusicList : (mediaData.mediaType() == TMediaData::MediaTypeVideo) ? &mGMCPVideoList : &mGMCPSoundList;
        break;
    case TMediaData::MediaProtocolAPI:
        list = (mediaData.mediaType() == TMediaData::MediaTypeMusic) ? &mAPIMusicList : (mediaData.mediaType() == TMediaData::MediaTypeVideo) ? &mAPIVideoList : &mAPISoundList;
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

        if (existingPlayer->getPlaybackState() != QMediaPlayer::PlayingState && existingPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            existingPlayer->setMediaData(mediaData);
            return existingPlayer; // Reuse existing player
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
int TMedia::getMaxUnprunedPlayers() const
{
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(10, 25 / hostCount);
}

int TMedia::getMaxAllowedSoundPlayers() const
{
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(16, 65 / hostCount);
}

int TMedia::getMaxAllowedMusicPlayers() const
{
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(4, 20 / hostCount);
}

int TMedia::getMaxAllowedVideoPlayers() const
{
    int hostCount = std::max(1, mudlet::self()->getHostManager().getHostCount());
    return std::max(2, 10 / hostCount);
}

#ifdef MUDLET_MEMORY_TRACKING
void TMedia::getMediaPlayerCounts(int& soundPlayers, int& musicPlayers, int& stoppedPlayers) const
{
    soundPlayers = mAPISoundList.size() + mMSPSoundList.size() + mGMCPSoundList.size();
    musicPlayers = mAPIMusicList.size() + mMSPMusicList.size() + mGMCPMusicList.size();

    const auto countStopped = [](const QList<std::shared_ptr<TMediaPlayer>>& lst) {
        int n = 0;
        for (const auto& p : lst) {
            if (p && p->getPlaybackState() == QMediaPlayer::StoppedState) {
                ++n;
            }
        }
        return n;
    };

    stoppedPlayers = countStopped(mAPISoundList) + countStopped(mMSPSoundList) + countStopped(mGMCPSoundList) + countStopped(mAPIMusicList) + countStopped(mMSPMusicList) + countStopped(mGMCPMusicList)
                     + countStopped(mAPIVideoList) + countStopped(mGMCPVideoList);
}
#endif // MUDLET_MEMORY_TRACKING

// Tells scripts a playback is over. Raised for a failed load as well as for a stop, because a
// script that starts its next track from sysMediaFinished otherwise waits forever on the first
// file the backend cannot decode.
void TMedia::raiseMediaFinishedEvent(const std::shared_ptr<TMediaPlayer>& player, const QUrl& endedUrl, const TMediaData& endedData)
{
    if (!mpHost || !player) {
        return;
    }

    if (endedUrl.isEmpty()) {
        // A pooled player between tracks. There is no playback to report, and the event would
        // carry an empty file name and path with the key and tag of whatever it last played.
        return;
    }

    if (player->endAnnounced()) {
        // Already reported by whichever of the stop, the error and the StoppedState got here
        // first - see TMediaPlayer::endAnnounced().
        return;
    }

    // Set before the handlers run, not after: raiseEvent() dispatches synchronously, and a
    // handler that stops this player would otherwise arrive back here and announce again.
    player->noteEndAnnounced();

    TEvent mediaFinished{};
    mediaFinished.mArgumentList.append(qsl("sysMediaFinished"));

    mediaFinished.mArgumentList.append(endedUrl.fileName());
    mediaFinished.mArgumentList.append(endedUrl.path());
    mediaFinished.mArgumentList.append(mediaTypeToString(endedData.mediaType()));
    mediaFinished.mArgumentList.append(endedData.mediaKey());
    mediaFinished.mArgumentList.append(endedData.mediaTag());
    mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    mpHost->raiseEvent(mediaFinished);
}

// Ends a playback: releases the media source and prints the closing caption, one event-loop
// turn from now. Deferred so a StoppedState-first backend can still emit the EndOfMedia that
// restarts a loop - clearing the source immediately destroys the playback engine and that
// signal never arrives (#9566). See TMediaPlayer for the generation counters this compares.
//
// endedBy decides whether the player's own state gets a say in the deferred turn. A stop needs
// it: on an EndOfMedia-first backend the restart happened before the snapshot, so the
// continuation counter cannot see it and a player still reporting PlayingState is the only sign
// the track carried on. A failure must not have it, because a backend can report PlayingState
// for media it has just failed to load (Qt 6.9's darwin backend does), and believing that would
// leave the dead source held forever - no state change follows to schedule another release.
void TMedia::releaseMediaSourceAfterEvents(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& endedData, const PlaybackEnd endedBy)
{
    const bool playbackStateDecides = (endedBy == PlaybackEnd::Stopped);

    if (!player->mediaPlayer() || player->mediaPlayer()->source().isEmpty()) {
        // Nothing left to end, so no caption for it either
        qDebug() << "TMedia::releaseMediaSourceAfterEvents() - asked to end a playback that is already holding no source; nothing to do.";
        return;
    }

    const std::weak_ptr<TMediaPlayer> weakPlayer = player;
    const quint64 claimedAt = player->claimGeneration();
    const quint64 continuedAt = player->continuationGeneration();

    QTimer::singleShot(0, this, [this, weakPlayer, endedData, claimedAt, continuedAt, playbackStateDecides] {
        const auto lockedPlayer = weakPlayer.lock();
        const bool stillOurs = lockedPlayer && lockedPlayer->claimGeneration() == claimedAt;
        // Two ways the same playback can have carried on during the deferred turn. On a
        // StoppedState-first backend the loop restarts from the EndOfMedia handler after the
        // snapshot above, so the counter is what sees it; on an EndOfMedia-first backend the
        // restart already happened before the snapshot, so the counter cannot see it and the
        // player still reporting PlayingState is.
        const bool sameMediaContinues =
                lockedPlayer && (lockedPlayer->continuationGeneration() != continuedAt || (playbackStateDecides && stillOurs && lockedPlayer->getPlaybackState() == QMediaPlayer::PlayingState));

        if (sameMediaContinues) {
            // No caption either: nothing ended, so "stops" between the passes of a looping
            // track would be wrong. Logged because this is the one outcome that keeps a source
            // on purpose, which makes it the first thing to rule out when one is held too long.
            qDebug() << "TMedia::releaseMediaSourceAfterEvents() - the same playback carried on into another pass; keeping its source.";
            return;
        }

        // Releasing bumps no generation, so any path that clears the source itself leaves a
        // pending turn still looking entitled to end this playback - an error and the stop
        // that follows it, stopAllMediaPlayers(), the setupVideo() failure in play(). Each of
        // those announces its own ending, so this one has nothing left to do or to say.
        if (stillOurs && lockedPlayer->mediaPlayer() && lockedPlayer->mediaPlayer()->source().isEmpty()) {
            qDebug() << "TMedia::releaseMediaSourceAfterEvents() - this playback was already ended by whoever released the source; nothing left to do.";
            return;
        }

        if (!lockedPlayer) {
            qDebug() << "TMedia::releaseMediaSourceAfterEvents() - player was destroyed before its deferred release ran; its destructor released the source.";
        } else if (!stillOurs) {
            // A claimed player is already loading the source of the track that took it over,
            // which on an asynchronous backend still reads as stopped.
            qDebug() << "TMedia::releaseMediaSourceAfterEvents() - another track claimed this player before its deferred release ran; keeping the new source.";
        } else if (!lockedPlayer->mediaPlayer()) {
            qWarning() << "TMedia::releaseMediaSourceAfterEvents() WARNING - mediaPlayer() is null, cannot release the media source.";
        } else if (playbackStateDecides && lockedPlayer->getPlaybackState() != QMediaPlayer::StoppedState) {
            qDebug() << "TMedia::releaseMediaSourceAfterEvents() - player is no longer stopped, keeping its source.";
        } else {
            qDebug() << "TMedia::releaseMediaSourceAfterEvents() - releasing the media source of the playback that ended.";
            lockedPlayer->releaseSource();

            if (endedData.mediaWidget() == TMediaData::MediaWidgetLabel && endedData.mediaClose() == TMediaData::MediaCloseEnabled && lockedPlayer->mediaPlayer()->videoOutput() != nullptr) {
                emit signal_hideVideoOutput(lockedPlayer.get());
            }
        }

        // Printed on every path that got past the continuation check: the track this release
        // was scheduled for is over regardless of what has become of the player since.
        //: This word is part of a sentence like "Music stops" when the music is about to stop.
        printClosedCaption(endedData, tr("stops"));
    });
}

void TMedia::handlePlayerPlaybackStateChanged(QMediaPlayerPlaybackState playbackState, const std::shared_ptr<TMediaPlayer>& player)
{
    if (!player) {
        return;
    }

    if (playbackState == QMediaPlayer::StoppedState) {
        if (!player->mediaPlayer() || player->mediaPlayer()->source().isEmpty()) {
            // Whoever released the source already ended this playback and raised its event. A
            // second one from here would carry an empty file name and path, because the URL
            // they describe is exactly what was just cleared.
            qDebug() << "TMedia::handlePlayerPlaybackStateChanged() - stopped a player that is already holding no source; its playback was ended elsewhere.";
            return;
        }

        // Scheduled before the event below, because a sysMediaFinished handler runs
        // synchronously and may hand this player to the next track - which would change both
        // the media data and the generations the release has to be judged against.
        releaseMediaSourceAfterEvents(player, player->mediaData(), PlaybackEnd::Stopped);
        raiseMediaFinishedEvent(player, player->mediaPlayer()->source(), player->mediaData());

        return;
    } else if (playbackState == QMediaPlayer::PlayingState && player->mediaData().mediaVolume() != TMediaData::MediaVolumePreload) { // NOLINT(readability-else-after-return)
        TEvent mediaStarted{};
        mediaStarted.mArgumentList.append(qsl("sysMediaStarted"));

        const QUrl mediaUrl = player->mediaPlayer()->source();
        mediaStarted.mArgumentList.append(mediaUrl.fileName());
        mediaStarted.mArgumentList.append(mediaUrl.path());
        mediaStarted.mArgumentList.append(mediaTypeToString(player->mediaData().mediaType()));
        mediaStarted.mArgumentList.append(player->mediaData().mediaKey());
        mediaStarted.mArgumentList.append(player->mediaData().mediaTag());
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaStarted.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (mpHost) {
            mpHost->raiseEvent(mediaStarted);
        }

        //: This word is part of a sentence like "Music plays" when the music is starting to play.
        printClosedCaption(player->mediaData(), tr("plays"));
        return;
    } else if (playbackState == QMediaPlayer::PausedState) { // NOLINT(readability-else-after-return)
        TEvent mediaPaused{};
        mediaPaused.mArgumentList.append(qsl("sysMediaPaused"));

        const QUrl mediaUrl = player->mediaPlayer()->source();
        mediaPaused.mArgumentList.append(mediaUrl.fileName());
        mediaPaused.mArgumentList.append(mediaUrl.path());
        mediaPaused.mArgumentList.append(mediaTypeToString(player->mediaData().mediaType()));
        mediaPaused.mArgumentList.append(player->mediaData().mediaKey());
        mediaPaused.mArgumentList.append(player->mediaData().mediaTag());
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mediaPaused.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (mpHost) {
            mpHost->raiseEvent(mediaPaused);
        }

        //: This word is part of a sentence like "Music pauses" when the music stops playing for a while.
        printClosedCaption(player->mediaData(), tr("pauses"));
    }
}

std::shared_ptr<TMediaPlayer> TMedia::matchMediaPlayer(TMediaData& mediaData)
{
    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = TMedia::findMediaPlayersByCriteria(mediaData);

    for (auto& pTestPlayer : mediaPlayerList) {
        if (!pTestPlayer) {
            continue;
        }

        if (pTestPlayer->getPlaybackState() == QMediaPlayer::PlayingState && pTestPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (pTestPlayer->mediaData().mediaAbsolutePathFileName().endsWith(mediaData.mediaAbsolutePathFileName())) {
                // Is the same sound or music playing?
                pTestPlayer->setMediaData(mediaData);
                pTestPlayer->setVolume(mediaData.mediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.mediaVolume());
                return pTestPlayer; // Return a pointer to the matched player
            }
        }
    }

    return nullptr; // No matching player found
}

// Documentation: https://wiki.mudlet.org/w/Manual:Scripting#priority:_1_to_100
bool TMedia::doesMediaHavePriorityToPlay(TMediaData& mediaData, const QString& absolutePathFileName)
{
    if (mediaData.mediaPriority() == TMediaData::MediaPriorityNotSet) {
        return true; // Default behavior: assume priority if not set
    }

    int maxMediaPriority = 0;

    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = TMedia::findMediaPlayersByCriteria(mediaData);

    for (const auto& pTestPlayer : std::as_const(mediaPlayerList)) {
        if (!pTestPlayer) {
            continue;
        }

        if (pTestPlayer->getPlaybackState() == QMediaPlayer::PlayingState && pTestPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (!pTestPlayer->mediaData().mediaAbsolutePathFileName().endsWith(absolutePathFileName)) {
                // Is it a different sound or music than specified?
                if (pTestPlayer->mediaData().mediaPriority() != TMediaData::MediaPriorityNotSet && pTestPlayer->mediaData().mediaPriority() > maxMediaPriority) {
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
    QList<std::shared_ptr<TMediaPlayer>> mediaPlayerList = TMedia::findMediaPlayersByCriteria(mediaData);

    for (const auto& pTestPlayer : std::as_const(mediaPlayerList)) {
        if (!pTestPlayer) {
            continue;
        }

        if (pTestPlayer->getPlaybackState() == QMediaPlayer::PlayingState && pTestPlayer->mediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (!mediaData.mediaKey().isEmpty() && !pTestPlayer->mediaData().mediaKey().isEmpty() && mediaData.mediaKey() == pTestPlayer->mediaData().mediaKey()) {
                // Does it have the same key?
                if (!pTestPlayer->mediaData().mediaAbsolutePathFileName().endsWith(absolutePathFileName)
                    || (!mediaData.mediaUrl().isEmpty() && !pTestPlayer->mediaData().mediaUrl().isEmpty() && mediaData.mediaUrl() != pTestPlayer->mediaData().mediaUrl())) {
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

    if (!isSignalConnected(QMetaMethod::fromSignal(&TMedia::signal_setupVideoOutput))) {
        qWarning() << "TMedia::setupVideo() WARNING - no receiver connected to signal_setupVideoOutput, video cannot be displayed";
    }

    bool setupSucceeded = false;
    emit signal_setupVideoOutput(player.get(), setupSucceeded);
    return setupSucceeded;
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
    TMediaPlaylist* playlist = pPlayer->playlist();

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

        absolutePathFileName = fileNameList.size() > 1 ? fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()))
                                                       : (mediaData.mediaInput() == TMediaData::MediaInputStream ? TMedia::getStreamUrl(mediaData) : fileNameList.at(0));

        if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) {
            return;
        }

        if (!mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
            TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName);
        }

        const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile ? QUrl::fromLocalFile(absolutePathFileName) : QUrl(absolutePathFileName);
        pPlayer->claimSource(mediaSource);
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

            absolutePathFileName = fileNameList.size() > 1 ? fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()))
                                                           : (mediaData.mediaInput() == TMediaData::MediaInputStream ? TMedia::getStreamUrl(mediaData) : fileNameList.at(0));

            if (!TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) {
                return;
            }

            if (!mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
                TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName);
            }

            const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile ? QUrl::fromLocalFile(absolutePathFileName) : QUrl(absolutePathFileName);
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
                absolutePathFileName = fileNameList.size() > 1 ? fileNameList.at(QRandomGenerator::global()->bounded(fileNameList.size()))
                                                               : (mediaData.mediaInput() == TMediaData::MediaInputStream ? TMedia::getStreamUrl(mediaData) : fileNameList.at(0));

                if (k == 0 && !TMedia::doesMediaHavePriorityToPlay(mediaData, absolutePathFileName)) {
                    return;
                }

                if (k == 0 && !mediaData.mediaKey().isEmpty() && (mediaData.mediaProtocol() == TMediaData::MediaProtocolGMCP || mediaData.mediaProtocol() == TMediaData::MediaProtocolAPI)) {
                    TMedia::matchMediaKeyAndStopMediaVariants(mediaData, absolutePathFileName);
                }

                const QUrl mediaSource = mediaData.mediaInput() == TMediaData::MediaInputFile ? QUrl::fromLocalFile(absolutePathFileName) : QUrl(absolutePathFileName);
                playlist->addMedia(mediaSource);
            }
        }

        if (sameMediaIsPlaying && mediaData.mediaContinue() == TMediaData::MediaContinueDefault) {
            return;
        }

        playlist->setCurrentIndex(0);
        pPlayer->setPlaylist(playlist);
        pPlayer->claimSource(playlist->currentMedia());
    }

    // Set volume and start position
    pPlayer->setVolume(mediaData.mediaFadeIn() != TMediaData::MediaFadeNotSet ? 1 : mediaData.mediaVolume());
    pPlayer->mediaPlayer()->setPosition(mediaData.mediaStart());

    // Set mute state based on protocol
    QAudioOutput* audioOutput = pPlayer->mediaPlayer()->audioOutput();
    if (audioOutput) {
        switch (mediaData.mediaProtocol()) {
        case TMediaData::MediaProtocolAPI:
            audioOutput->setMuted(mudlet::self()->muteAPI());
            break;
        case TMediaData::MediaProtocolGMCP:
        case TMediaData::MediaProtocolMSP:
            audioOutput->setMuted(mudlet::self()->muteGame());
            break;
        }
    } else {
        qWarning() << "TMedia::play() - audioOutput is nullptr, skipping mute state update.";
    }

    // Handle video setup if applicable
    if (mediaData.mediaType() == TMediaData::MediaTypeVideo && !setupVideo(pPlayer)) {
        // Claiming the player disarmed any release still pending on it, so drop the source it
        // is now never going to play rather than leave it held indefinitely.
        pPlayer->releaseSource();

        // Same guards as the deferred release: a reused player can still be showing the widget
        // of an earlier clip, and hiding that is only wanted when this request asked for it.
        if (mediaData.mediaWidget() == TMediaData::MediaWidgetLabel && mediaData.mediaClose() == TMediaData::MediaCloseEnabled && pPlayer->mediaPlayer()->videoOutput() != nullptr) {
            emit signal_hideVideoOutput(pPlayer.get());
        }

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

QString TMedia::mediaTypeToString(int mediaType)
{
    switch (mediaType) {
    case TMediaData::MediaTypeSound:
        return qsl("sound");
    case TMediaData::MediaTypeMusic:
        return qsl("music");
    case TMediaData::MediaTypeVideo:
        return qsl("video");
    default:
        return QString();
    }
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

    auto mediaFinishJSON = json.value(qsl("finish"));

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
        //: This word is part of a sentence like "Music stops" when Mudlet handles a piece of music.
        const QString mediaType = mediaData.mediaType() == TMediaData::MediaTypeMusic ? tr("music") :
                                                                                      //: This word is part of a sentence like "Video stops" when Mudlet handles a video.
                                          mediaData.mediaType() == TMediaData::MediaTypeVideo ? tr("video")
                                                                                              //: This word is part of a sentence like "Sound stops" when Mudlet handles neither music nor video.
                                                                                              : tr("sound");
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
