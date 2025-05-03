#ifndef MUDLET_TMEDIA_H
#define MUDLET_TMEDIA_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2019, 2022. 2024 by Stephen Lyons                  *
 *                                            - slysven@virginmedia.com    *
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


#include "Host.h"
#include "TEvent.h"
#include "mudlet.h"
#include "TMediaData.h"
#include "TMediaPlaylist.h"

#include "pre_guard.h"
#include <memory> // std::shared_ptr
#include <QAudioOutput>
#include <QMediaPlayer>
#include "post_guard.h"

using QMediaPlayerPlaybackState = QMediaPlayer::PlaybackState;
class TMediaPlayer
{
public:
    TMediaPlayer()
    : mMediaData()
    {}
    TMediaPlayer(Host* pHost, TMediaData& mediaData)
    : mpHost(pHost),
      mMediaData(mediaData),
      mMediaPlayer(new QMediaPlayer(pHost)),
      mPlaylist(new TMediaPlaylist),
      initialized(true)
    {
        mMediaPlayer->setAudioOutput(new QAudioOutput());
    }
    ~TMediaPlayer() = default;

    TMediaData mediaData() const { return mMediaData; }
    void setMediaData(TMediaData& mediaData) { mMediaData = mediaData; }
    QMediaPlayer* mediaPlayer() const { return mMediaPlayer; }
    bool isInitialized() const { return initialized; }
    QMediaPlayer::PlaybackState getPlaybackState() const {
        if (!mMediaPlayer) {
            qWarning() << "TMediaPlayer::getPlaybackState() - mMediaPlayer is nullptr!";
            return QMediaPlayer::StoppedState; // Safe default state
        }
        return mMediaPlayer->playbackState();
    }
    void setVolume(int volume) const {
        return mMediaPlayer->audioOutput()->setVolume(volume / 100.0f);
    }
    TMediaPlaylist* playlist() const {
        return mPlaylist;
    }
    void setPlaylist(TMediaPlaylist* playlist) {
        if (mPlaylist != playlist) {
            if (mPlaylist) delete mPlaylist;
            mPlaylist = playlist;
        }
    }

private:
    QPointer<Host> mpHost;
    TMediaData mMediaData;
    QMediaPlayer* mMediaPlayer;
    TMediaPlaylist* mPlaylist;
    bool initialized = false;
};

class TMedia : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TMedia)
    TMedia(Host* pHost, const QString& profileName);
    ~TMedia() = default;

    void playMedia(TMediaData& mediaData);
    QList<TMediaData> playingMedia(TMediaData& mediaData);
    QList<TMediaData> pausedMedia(TMediaData& mediaData);
    void pauseMedia(TMediaData& mediaData);
    void stopMedia(TMediaData& mediaData);
    void parseGMCP(QString& packageMessage, QString& gmcp);
    bool purgeMediaCache();
    void muteMedia(const TMediaData::MediaProtocol mediaProtocol);
    void unmuteMedia(const TMediaData::MediaProtocol mediaProtocol);

private slots:
    void slot_writeFile(QNetworkReply* reply);

private:
    bool isMediaProtocolAllowed(const TMediaData& mediaData) const;
    QList<std::shared_ptr<TMediaPlayer>> findMediaPlayersByCriteria(const TMediaData& mediaData);
    bool isMediaMatch(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& mediaData);
    bool resume(TMediaData mediaData);
    void stopAllMediaPlayers();
    void setMediaPlayersMuted(const TMediaData::MediaProtocol mediaProtocol, const bool state);
    void transitionNonRelativeFile(TMediaData& mediaData);
    QString getStreamUrl(const TMediaData& mediaData);
    QUrl parseUrl(TMediaData& mediaData);
    static bool isValidUrl(QUrl& url);
    static bool isFileRelative(TMediaData& mediaData);
    QStringList parseFileNameList(TMediaData& mediaData, QDir& dir);
    QStringList getFileNameList(TMediaData& mediaData);
    QUrl getFileUrl(TMediaData& mediaData);
    bool processUrl(TMediaData& mediaData);
    void downloadFile(TMediaData& mediaData);
    QString setupMediaAbsolutePathFileName(TMediaData& mediaData);
    void connectMediaPlayer(std::shared_ptr<TMediaPlayer>& player);
    void updateMediaPlayerList(std::shared_ptr<TMediaPlayer> player);
    std::shared_ptr<TMediaPlayer> getMediaPlayer(TMediaData& mediaData);
    std::shared_ptr<TMediaPlayer> matchMediaPlayer(TMediaData& mediaData);
    bool doesMediaHavePriorityToPlay(TMediaData& mediaData, const QString& absolutePathFileName);
    void matchMediaKeyAndStopMediaVariants(TMediaData& mediaData, const QString& absolutePathFileName);
    void handlePlayerPlaybackStateChanged(QMediaPlayerPlaybackState playbackState, const std::shared_ptr<TMediaPlayer>& player);
    bool setupVideo(const std::shared_ptr<TMediaPlayer>& player);

    void play(TMediaData& mediaData);

    static TMediaData::MediaType parseJSONByMediaType(QJsonObject& json);
    static int parseJSONByMediaInput(QJsonObject& json);
    static QString parseJSONByMediaFileName(QJsonObject& json);
    static int parseJSONByMediaVolume(QJsonObject& json);
    static int parseJSONByMediaFadeIn(QJsonObject& json);
    static int parseJSONByMediaFadeOut(QJsonObject& json);
    static int parseJSONByMediaStart(QJsonObject& json);
    static int parseJSONByMediaFinish(QJsonObject& json);
    static int parseJSONByMediaPriority(QJsonObject& json);
    static int parseJSONByMediaLoops(QJsonObject& json);
    static TMediaData::MediaContinue parseJSONByMediaContinue(QJsonObject& json);
    static QString parseJSONByMediaTag(QJsonObject& json);
    static QString parseJSONByMediaUrl(QJsonObject& json);
    static QString parseJSONByMediaKey(QJsonObject& json);
    static TMediaData::MediaFadeAway parseJSONByMediaFadeAway(QJsonObject& json);
    static TMediaData::MediaClose parseJSONByMediaClose(QJsonObject& json);

    void parseJSONForMediaDefault(QJsonObject& json);
    void parseJSONForMediaLoad(QJsonObject& json);
    void parseJSONForMediaPlay(QJsonObject& json);
    void parseJSONForMediaPause(QJsonObject& json);
    void parseJSONForMediaStop(QJsonObject& json);

    QPointer<Host> mpHost;
    QString mProfileName;

    QList<std::shared_ptr<TMediaPlayer>> mMSPSoundList;
    QList<std::shared_ptr<TMediaPlayer>> mMSPMusicList;
    QList<std::shared_ptr<TMediaPlayer>> mGMCPSoundList;
    QList<std::shared_ptr<TMediaPlayer>> mGMCPMusicList;
    QList<std::shared_ptr<TMediaPlayer>> mGMCPVideoList;
    QList<std::shared_ptr<TMediaPlayer>> mAPISoundList;
    QList<std::shared_ptr<TMediaPlayer>> mAPIMusicList;
    QList<std::shared_ptr<TMediaPlayer>> mAPIVideoList;

    QNetworkAccessManager* mpNetworkAccessManager = nullptr;
    QMap<QNetworkReply*, TMediaData> mMediaDownloads;
};
#endif // MUDLET_TMEDIA_H
