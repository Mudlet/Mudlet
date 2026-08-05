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

#include <memory>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QUrl>

class QJsonObject;

using QMediaPlayerPlaybackState = QMediaPlayer::PlaybackState;
class TMediaPlayer
{
public:
    TMediaPlayer() = default;
    TMediaPlayer(Host* pHost, TMediaData& mediaData)
    : mpHost(pHost)
    , mMediaData(mediaData)
    , mMediaPlayer(new QMediaPlayer(nullptr))
    , mPlaylist(std::make_unique<TMediaPlaylist>())
    , initialized(true)
    {
        // QAudioOutput is parented to mMediaPlayer so it is destroyed with it
        mMediaPlayer->setAudioOutput(new QAudioOutput(mMediaPlayer.get()));
    }
    ~TMediaPlayer()
    {
        if (mMediaPlayer) {
            mMediaPlayer->stop();
            mMediaPlayer->setSource(QUrl());
        }
    }

    TMediaData mediaData() const { return mMediaData; }
    void setMediaData(TMediaData& mediaData) { mMediaData = mediaData; }

    // TMedia::releaseMediaSourceAfterEvents() ends a playback one event-loop turn late, by which
    // time a stopped player is indistinguishable from one asynchronously loading a source set
    // since. These two counters record what happened in between: a claim is this player being
    // given a new track to play, a continuation is its own playlist advancing or looping.
    // Outside this class, install a source only through claimSource() or continuePlaying() -
    // never through mediaPlayer()->setSource() directly, since a missed bump lets that pending
    // release clear the new source again. As of Qt 6.9 that reproduces only on backends that
    // load asynchronously, so it will not show up on a macOS-only test run.
    void claimSource(const QUrl& media)
    {
        // Bumped before the source is touched because setSource() can raise errorOccurred
        // synchronously, and that handler snapshots these counters to arm its own release.
        ++mClaimGeneration;
        mEndAnnounced = false;
        if (mMediaPlayer) {
            // A stopped player still holding anything has that media loaded, so handing it a
            // source now starts playback synchronously and raises sysMediaStarted inside the
            // script call that asked for it. Unloading first restores the usual asynchronous
            // start. The reported symptom was replaying the same file (#9611).
            if (mMediaPlayer->playbackState() == QMediaPlayer::StoppedState && !mMediaPlayer->source().isEmpty()) {
                releaseSource();
            }
            mMediaPlayer->setSource(media);
        }
    }
    void continuePlaying(const QUrl& media)
    {
        ++mContinuationGeneration;
        mEndAnnounced = false;
        if (mMediaPlayer) {
            mMediaPlayer->setSource(media);
            mMediaPlayer->play();
        }
    }
    // No bump: an empty source cannot be mistaken for a track that needs protecting from a
    // pending release. A release already scheduled therefore still fires, and recognises that
    // it has nothing left to do by the source being empty - see releaseMediaSourceAfterEvents().
    void releaseSource()
    {
        if (mMediaPlayer) {
            mMediaPlayer->setSource(QUrl());
        }
    }
    quint64 claimGeneration() const { return mClaimGeneration; }
    quint64 continuationGeneration() const { return mContinuationGeneration; }

    // One ended playback can be reported from three places - a stop, a load error and the
    // StoppedState that follows either - and the source stays set until the deferred release
    // runs, so each of them still finds a playback that looks live. Only the first may tell
    // scripts about it: a second sysMediaFinished for the same track is at best a duplicate,
    // and at worst unbounded recursion when the handler stops the media it was told about.
    // Cleared by the two ways this player is given something new to play, above.
    bool endAnnounced() const { return mEndAnnounced; }
    void noteEndAnnounced() { mEndAnnounced = true; }

    // Read-only uses and playback control are fine; do not setSource() on it, for the reason
    // given above claimSource().
    QMediaPlayer* mediaPlayer() const { return mMediaPlayer.get(); }
    bool isInitialized() const { return initialized; }
    QMediaPlayer::PlaybackState getPlaybackState() const
    {
        if (!mMediaPlayer) {
            qWarning() << "TMediaPlayer::getPlaybackState() - mMediaPlayer is nullptr!";
            return QMediaPlayer::StoppedState; // Safe default state
        }
        return mMediaPlayer->playbackState();
    }
    void setVolume(int volume) const
    {
        if (!mMediaPlayer) {
            qWarning() << "TMediaPlayer::setVolume() - mMediaPlayer is nullptr!";
            return;
        }
        QAudioOutput* audioOutput = mMediaPlayer->audioOutput();
        if (!audioOutput) {
            qWarning() << "TMediaPlayer::setVolume() - audioOutput is nullptr!";
            return;
        }
        audioOutput->setVolume(volume / 100.0f);
    }
    TMediaPlaylist* playlist() const { return mPlaylist.get(); }
    void setPlaylist(TMediaPlaylist* playlist)
    {
        if (mPlaylist.get() != playlist) {
            mPlaylist.reset(playlist);
        }
    }
    void refreshAudioOutput()
    {
        if (!mMediaPlayer) {
            return;
        }
        QAudioOutput* oldOutput = mMediaPlayer->audioOutput();
        float volume = oldOutput ? oldOutput->volume() : 1.0f;
        bool muted = oldOutput ? oldOutput->isMuted() : false;

        auto* newOutput = new QAudioOutput(mMediaPlayer.get());
        newOutput->setVolume(volume);
        newOutput->setMuted(muted);
        mMediaPlayer->setAudioOutput(newOutput);
        if (oldOutput) {
            oldOutput->setParent(nullptr);
            oldOutput->deleteLater();
        }
    }

private:
    QPointer<Host> mpHost;
    TMediaData mMediaData;
    std::unique_ptr<QMediaPlayer> mMediaPlayer;
    std::unique_ptr<TMediaPlaylist> mPlaylist;
    bool initialized = false;
    quint64 mClaimGeneration = 0;
    quint64 mContinuationGeneration = 0;
    bool mEndAnnounced = false;
};

class TMedia : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TMedia)
    TMedia(Host* pHost, const QString& profileName);
    ~TMedia() = default;

    int getMaxUnprunedPlayers() const;
    int getMaxAllowedSoundPlayers() const;
    int getMaxAllowedMusicPlayers() const;
    int getMaxAllowedVideoPlayers() const;
#ifdef MUDLET_MEMORY_TRACKING
    void getMediaPlayerCounts(int& soundPlayers, int& musicPlayers, int& stoppedPlayers) const;
#endif

    void playMedia(TMediaData& mediaData);
    QList<TMediaData> playingMedia(TMediaData& mediaData);
    QList<TMediaData> pausedMedia(TMediaData& mediaData);
    void pauseMedia(TMediaData& mediaData);
    void stopMedia(TMediaData& mediaData);
    void parseGMCP(QString& packageMessage, QString& gmcp);
    bool purgeMediaCache();
    void refreshAudioDevices();
    void muteMedia(const TMediaData::MediaProtocol mediaProtocol);
    void unmuteMedia(const TMediaData::MediaProtocol mediaProtocol);
    void printClosedCaption(const TMediaData& mediaData, const QString& action) const;
    void stopAllMediaPlayers();

    // Read-only diagnostics for the media tests. A deferred release is otherwise hard to
    // observe: playingMedia() has already dropped the player, the closed caption needs captions
    // enabled and signal_hideVideoOutput needs a video widget.
    int playersHoldingSource() const;
    // Players that have actually started. playingMedia() deliberately counts one that is still
    // loading as playing, which is not enough for a test that needs playback truly under way.
    int playersInPlayingState() const;
    // Players registered in the protocol lists, so a reuse test can tell a claimed player from
    // a second one allocated alongside it. A player play() abandons before it finishes is never
    // registered and so is never counted.
    int mediaPlayerCount() const;

    // Returns true if mediaFileName would resolve to a location outside mediaRoot, either
    // lexically (e.g. via "../" traversal) or through a symlink component that already exists
    // under mediaRoot but points elsewhere. Static so it can be unit-tested without a Host.
    static bool mediaFilePathEscapesMediaDir(const QString& mediaRoot, const QString& mediaFileName);

signals:
    void signal_setupVideoOutput(TMediaPlayer* player, bool& setupSucceeded);
    void signal_hideVideoOutput(TMediaPlayer* player);

private slots:
    void slot_writeFile(QNetworkReply* reply);

private:
    bool isMediaProtocolAllowed(const TMediaData& mediaData) const;
    QList<std::shared_ptr<TMediaPlayer>> findMediaPlayersByCriteria(const TMediaData& mediaData);
    bool isMediaMatch(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& mediaData);
    bool resume(TMediaData mediaData);
    void setMediaPlayersMuted(const TMediaData::MediaProtocol mediaProtocol, const bool state);
    void transitionNonRelativeFile(TMediaData& mediaData);
    QString getStreamUrl(const TMediaData& mediaData);
    QUrl parseUrl(TMediaData& mediaData);
    static bool isValidUrl(QUrl& url);
    static bool isFileRelative(TMediaData& mediaData);
    bool mediaFilePathEscapesMediaDir(TMediaData& mediaData) const;
    QStringList parseFileNameList(TMediaData& mediaData, QDir& dir);
    QStringList getFileNameList(TMediaData& mediaData);
    QUrl getFileUrl(TMediaData& mediaData);
    bool processUrl(TMediaData& mediaData);
    void downloadFile(TMediaData& mediaData);
    QString setupMediaAbsolutePathFileName(TMediaData& mediaData);
    void connectMediaPlayer(std::shared_ptr<TMediaPlayer>& player);
    static void purgeStoppedMediaPlayers(QList<std::shared_ptr<TMediaPlayer>>& mediaList);
    template <typename T>
    static void updateList(QList<std::shared_ptr<T>>& list, int index, std::shared_ptr<T> player, TMedia* mediaInstance);
    void updateMediaPlayerList(std::shared_ptr<TMediaPlayer> player);
    std::shared_ptr<TMediaPlayer> getMediaPlayer(TMediaData& mediaData);
    std::shared_ptr<TMediaPlayer> matchMediaPlayer(TMediaData& mediaData);
    bool doesMediaHavePriorityToPlay(TMediaData& mediaData, const QString& absolutePathFileName);
    void matchMediaKeyAndStopMediaVariants(TMediaData& mediaData, const QString& absolutePathFileName);
    // Why a playback ended, which decides whether the player's own state is worth consulting
    // when the deferred release comes around. See releaseMediaSourceAfterEvents().
    enum class PlaybackEnd { Stopped, Failed };
    // endedUrl and endedData are passed in rather than read off the player, so a caller that has
    // already released the source can still say what it was that ended.
    void raiseMediaFinishedEvent(const std::shared_ptr<TMediaPlayer>& player, const QUrl& endedUrl, const TMediaData& endedData);
    void releaseMediaSourceAfterEvents(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& endedData, const PlaybackEnd endedBy);
    void handlePlayerPlaybackStateChanged(QMediaPlayerPlaybackState playbackState, const std::shared_ptr<TMediaPlayer>& player);
    bool setupVideo(const std::shared_ptr<TMediaPlayer>& player);
    static QString mediaTypeToString(int mediaType);

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
    static QString parseJSONByMediaCaption(QJsonObject& json);

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
