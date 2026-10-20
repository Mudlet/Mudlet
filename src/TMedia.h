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
#include "TMediaData.h"
#include "TMediaPlaylist.h"
#include "TMediaVoice.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <QAudioOutput>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QTimer>
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
            // The unload below releases the media file, and announces itself wherever it
            // changes something - sourceChanged from clearing the source, playbackStateChanged
            // from a stop that had something to stop - synchronously, into handlers reading a
            // TMediaPlayer whose members are about to go. The handlers TMedia installs decline
            // by way of a weak_ptr that has already expired by now; blocking makes that
            // structural rather than something each new handler has to remember. The unload
            // still happens, and Qt unblocks in ~QObject so destroyed() still arrives.
            mMediaPlayer->blockSignals(true);
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
    // load asynchronously, such as FFmpeg; darwin does not.
    void claimSource(const QUrl& media)
    {
        // Bumped before the source is touched because setSource() can raise errorOccurred
        // synchronously, and that handler snapshots these counters to arm its own release.
        ++mClaimGeneration;
        mEndAnnounced = false;
        stopVolumeGlide();
        if (mpVoice) {
            mpVoice->beginTrack();
        }
        if (mMediaPlayer) {
            // A stopped player still holding anything has that media loaded, so handing it a
            // source now starts playback synchronously and raises sysMediaStarted inside the
            // script call that asked for it. Unloading first restores the usual asynchronous
            // start. The reported symptom was replaying the same file (#9611).
            if (mMediaPlayer->playbackState() == QMediaPlayer::StoppedState && !mMediaPlayer->source().isEmpty()) {
                releaseSource();
            }
            mClaimingSource = true;
            mMediaPlayer->setSource(media);
            mClaimingSource = false;
        }
    }
    void continuePlaying(const QUrl& media)
    {
        ++mContinuationGeneration;
        if (mMediaPlayer) {
            mMediaPlayer->setSource(media);
            // Cleared after the source is installed, not before. On a backend that delivers
            // EndOfMedia while the player is still playing this setSource() is a real
            // playing-to-stopped transition, and its handler announces the pass that just ended.
            // Clearing first leaves that announcement standing for the pass about to start, and
            // the last pass has no continuation left to clear it again.
            mEndAnnounced = false;
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

    // True only while claimSource() is installing a new source. A stop delivered during that is
    // the previous track being displaced rather than this one ending, and whoever displaced it
    // has already said so - with the metadata the player no longer holds.
    bool claimingSource() const { return mClaimingSource; }

    // A play() call owns the player it is setting up until it returns, because the events it
    // raises run script handlers synchronously. A handler that starts media of its own must be
    // given a different player: two play() calls sharing one overwrite each other's playlist and
    // media data.
    bool reservedForPlay() const { return mReservedForPlay; }
    void setReservedForPlay(const bool reserved) { mReservedForPlay = reserved; }

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
    void setVolume(int volume)
    {
        // A fade sets the same level on every tick, which must not cut short a longer glide to it
        if (mpVolumeGlide && mpVolumeGlide->isActive() && volume == mVolumeGlideTarget) {
            return;
        }
        stopVolumeGlide();
        if (mpVoice) {
            // Glides only for the moment that keeps a fade's steps from clicking once the sound plays
            mpVoice->setGain(volume / 100.0f, 0);
            return;
        }
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
    void stopVolumeGlide()
    {
        if (mpVolumeGlide) {
            mpVolumeGlide->stop();
        }
    }
    void glideVolume(int volume, int milliseconds)
    {
        if (mpVoice) {
            stopVolumeGlide();
            mpVoice->setGain(volume / 100.0f, milliseconds);
            return;
        }
        QAudioOutput* audioOutput = mMediaPlayer ? mMediaPlayer->audioOutput() : nullptr;
        if (!audioOutput || milliseconds <= 0) {
            setVolume(volume);
            return;
        }
        if (!mpVolumeGlide) {
            mpVolumeGlide = std::make_unique<QTimer>();
            mpVolumeGlide->setInterval(20);
        }
        mpVolumeGlide->disconnect();
        mVolumeGlideTarget = volume;
        const float from = audioOutput->volume();
        const float to = volume / 100.0f;
        auto elapsed = std::make_shared<QElapsedTimer>();
        elapsed->start();
        QObject::connect(mpVolumeGlide.get(), &QTimer::timeout, mpVolumeGlide.get(), [this, from, to, milliseconds, elapsed]() {
            const float progress = std::min(1.0f, static_cast<float>(elapsed->elapsed()) / static_cast<float>(milliseconds));
            if (QAudioOutput* output = mMediaPlayer ? mMediaPlayer->audioOutput() : nullptr) {
                output->setVolume(from + (to - from) * progress);
            }
            if (progress >= 1.0f) {
                mpVolumeGlide->stop();
            }
        });
        mpVolumeGlide->start();
    }
    TMediaPlaylist* playlist() const { return mPlaylist.get(); }
    void setPlaylist(TMediaPlaylist* playlist)
    {
        if (mPlaylist.get() != playlist) {
            mPlaylist.reset(playlist);
        }
    }
    void setMuted(bool muted)
    {
        if (mpVoice) {
            mpVoice->setMuted(muted);
        } else if (mMediaPlayer && mMediaPlayer->audioOutput()) {
            mMediaPlayer->audioOutput()->setMuted(muted);
        }
    }

    // Whether this player plays through the shared output rather than an output of its own
    bool mixed() const { return mpVoice != nullptr; }
    const TMediaVoice* voice() const { return mpVoice.get(); }
    // Set before a track is played. Returns whether the player is mixed afterwards: a backend that
    // hands over no audio keeps it on an output of its own.
    bool setMixed(bool mixed)
    {
        if (!mMediaPlayer || mixed == (mpVoice != nullptr)) {
            return mpVoice != nullptr;
        }
        // Its timer steps the output being replaced
        stopVolumeGlide();
        if (mixed) {
            if (TMediaVoice::unsupported()) {
                return false;
            }
            QAudioOutput* oldOutput = mMediaPlayer->audioOutput();
            mpVoice = std::make_unique<TMediaVoice>(mMediaPlayer.get());
            mpVoice->setGain(oldOutput ? oldOutput->volume() : 1.0f, 0);
            mpVoice->setMuted(oldOutput && oldOutput->isMuted());
            // Without an output of its own the player only feeds the voice
            mMediaPlayer->setAudioOutput(nullptr);
            if (oldOutput) {
                oldOutput->setParent(nullptr);
                oldOutput->deleteLater();
            }
            QObject::connect(mpVoice.get(), &TMediaVoice::signal_unsupported, mpVoice.get(), [this]() {
                moveOffTheMix();
            });
            return true;
        }
        auto* output = new QAudioOutput(mMediaPlayer.get());
        output->setVolume(mpVoice->gain());
        output->setMuted(mpVoice->muted());
        // Detached now, or the sound plays through both until the voice goes; deleted later, as this
        // can be reached from the voice's own signal
        mMediaPlayer->setAudioBufferOutput(nullptr);
        QObject::disconnect(mMediaPlayer.get(), nullptr, mpVoice.get(), nullptr);
        mpVoice.release()->deleteLater();
        mMediaPlayer->setAudioOutput(output);
        return false;
    }
    // -100 is fully left, 100 fully right; none plays the sound as it was recorded
    void setPan(std::optional<int> pan, int glideMilliseconds)
    {
        if (!mpVoice) {
            return;
        }
        if (pan) {
            mpVoice->setPan(*pan / 100.0f, glideMilliseconds);
        } else {
            mpVoice->unplace(glideMilliseconds);
        }
    }
    bool placed() const { return mpVoice && mpVoice->placed(); }

    void refreshAudioOutput()
    {
        // The shared output follows the device itself
        if (!mMediaPlayer || mpVoice) {
            return;
        }
        QAudioOutput* oldOutput = mMediaPlayer->audioOutput();
        float volume = oldOutput ? oldOutput->volume() : 1.0f;
        bool muted = oldOutput ? oldOutput->isMuted() : false;

        auto* newOutput = new QAudioOutput(mMediaPlayer.get());
        newOutput->setVolume(volume);
        newOutput->setMuted(muted);
        // Detach first: displacing an installed output runs its disconnect function, which
        // would null the player's pointer to the replacement installed in the same call
        mMediaPlayer->setAudioOutput(nullptr);
        mMediaPlayer->setAudioOutput(newOutput);
        if (oldOutput) {
            oldOutput->setParent(nullptr);
            oldOutput->deleteLater();
        }
    }

private:
    // Given an output while it plays, FFmpeg rebuilds the player's pipeline, and one rebuilt before the
    // first audio reaches the voice was seen never to end. So the track starts over instead, where it
    // was; blocked, so scripts are not told it stopped and started again.
    void moveOffTheMix()
    {
        if (mMediaPlayer->playbackState() != QMediaPlayer::PlayingState) {
            setMixed(false);
            return;
        }
        const qint64 position = mMediaPlayer->position();
        const QSignalBlocker unannounced(mMediaPlayer.get());
        mMediaPlayer->stop();
        setMixed(false);
        mMediaPlayer->setPosition(position);
        mMediaPlayer->play();
    }

    QPointer<Host> mpHost;
    TMediaData mMediaData;
    std::unique_ptr<QMediaPlayer> mMediaPlayer;
    std::unique_ptr<TMediaPlaylist> mPlaylist;
    bool initialized = false;
    quint64 mClaimGeneration = 0;
    quint64 mContinuationGeneration = 0;
    bool mEndAnnounced = false;
    bool mClaimingSource = false;
    bool mReservedForPlay = false;
    std::unique_ptr<QTimer> mpVolumeGlide;
    int mVolumeGlideTarget = 0;
    // After mMediaPlayer, so it is destroyed first and can still detach itself from the player
    std::unique_ptr<TMediaVoice> mpVoice;
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
    std::pair<bool, QString> purgeMediaCache();
    void refreshAudioDevices();
    void muteMedia(const TMediaData::MediaProtocol mediaProtocol);
    void unmuteMedia(const TMediaData::MediaProtocol mediaProtocol);
    void printClosedCaption(const TMediaData& mediaData, const QString& action) const;
    void stopAllMediaPlayers();
    // Changes the volume, pan or direction of media already playing or paused, gliding there. Not
    // captioned: the play was, and an update only changes how that sound is heard.
    void updateMedia(TMediaData& mediaData, std::optional<int> volume, std::optional<int> pan, const QString& direction, int glideMilliseconds);
    enum class FacingSetBy { Game, Script };
    // A compass word or degrees, which turns every compass direction with it; false for anything else
    bool setListenerFacing(const QString& facing, FacingSetBy setBy);
    QString listenerFacing() const { return mListenerFacing; }
    // The game gives its facing again each session, so on a disconnect the player turns back to the
    // last one a script gave, or north
    void resetListenerFacing();

    // "north" for "n" or "North", whole degrees 0 to 359 for a number, empty for anything else
    static QString canonicalMediaDirection(const QString& direction);
    static QString canonicalMediaDirection(double degrees);
    // A move that gives no time of its own happens at once; TMediaVoice smooths it so it does not click
    static constexpr int defaultGlideMilliseconds = 0;
    // Where between -100 and 100 a stereo output places it, for a player facing that way: its pan if it
    // has one, else its direction. None for a direction with no bearing, which plays as with no direction.
    static std::optional<int> stereoPan(const TMediaData& mediaData, const QString& facing);

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
    // Players playing through the shared output, and the voice of the first of them
    int playersMixed() const;
    const TMediaVoice* firstMixedVoice() const;
    // Players placing their sound left or right, and of those the ones that have had audio
    int playersPlaced() const;
    int playersPlacedWithAudio() const;
    // The voice of the playing sound with this key, if it is placed, for what it is applying
    const TMediaVoice* placedSound(const QString& key) const;

    // Returns true if mediaFileName would resolve to a location outside mediaRoot, either
    // lexically (e.g. via "../" traversal) or through a symlink component that already exists
    // under mediaRoot but points elsewhere. Static so it can be unit-tested without a Host.
    static bool mediaFilePathEscapesMediaDir(const QString& mediaRoot, const QString& mediaFileName);
    // Where, relative to mediaRoot, a file an API call names by absolute path is played from: its copy,
    // or the file itself when it is already under mediaRoot. Static so it can be unit-tested without a Host.
    static QString mediaCacheFileName(const QString& profileHome, const QString& mediaRoot, const QString& absolutePath);
    // A path with its symlinks resolved, as mediaCacheFileName() files copies by; one to a file that has gone is
    // resolved through the folders that remain. Static so it can be unit-tested without a Host.
    static QString resolvedMediaPath(const QString& path);
    // Whether a name sits in a folder mediaCacheFileName() keeps for its copies, where a game must not reach
    static bool mediaFileNameReservedForCopies(const QString& mediaFileName);

signals:
    void signal_setupVideoOutput(TMediaPlayer* player, bool& setupSucceeded);
    void signal_hideVideoOutput(TMediaPlayer* player);

private slots:
    void slot_writeFile(QNetworkReply* reply);

private:
    bool isMediaProtocolAllowed(const TMediaData& mediaData) const;
    QList<std::shared_ptr<TMediaPlayer>> findMediaPlayersByCriteria(const TMediaData& mediaData);
    // resume() keeps MatchesCacheNameOnly, so a bare name never resumes another folder's copy of a same-named file
    enum class BareName { MatchesCacheNameOnly, AlsoMatchesSourceFile };
    bool isMediaMatch(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& mediaData, BareName bareName = BareName::MatchesCacheNameOnly);
    static bool isMediaMatch(const TMediaData& playing, const TMediaData& mediaData, BareName bareName);
    TMediaData requestForFilePlayers(const TMediaData& mediaData) const;
    void turnListener(const QString& facing);
    QString mediaCacheFileName(const QString& absolutePath) const;
    bool resume(TMediaData mediaData);
    void setMediaPlayersMuted(const TMediaData::MediaProtocol mediaProtocol, const bool state);
    bool transitionNonRelativeFile(TMediaData& mediaData);
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
    static void seekToMediaStart(const std::shared_ptr<TMediaPlayer>& player);
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
    void claimPlayerFor(const std::shared_ptr<TMediaPlayer>& player, TMediaData& mediaData, const QUrl& mediaSource);
    void endDisplacedPlayback(const std::shared_ptr<TMediaPlayer>& player);
    void releaseMediaSourceAfterEvents(const std::shared_ptr<TMediaPlayer>& player, const TMediaData& endedData, const PlaybackEnd endedBy);
    void handlePlayerPlaybackStateChanged(QMediaPlayerPlaybackState playbackState, const std::shared_ptr<TMediaPlayer>& player);
    bool setupVideo(const std::shared_ptr<TMediaPlayer>& player);
    static QString mediaTypeToString(int mediaType);

    void play(TMediaData& mediaData);
    // Degrees clockwise from north, or from straight ahead for a relative word; none for up and down
    static std::optional<double> mediaDirectionBearing(const QString& direction, bool& relativeToPlayer);
    static QString mediaDirectionCaption(const QString& direction);

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
    static std::optional<int> parseJSONByMediaPan(QJsonObject& json);
    static QString parseJSONByMediaDirection(QJsonObject& json);

    void parseJSONForMediaDefault(QJsonObject& json);
    void parseJSONForMediaLoad(QJsonObject& json);
    void parseJSONForMediaPlay(QJsonObject& json);
    void parseJSONForMediaPause(QJsonObject& json);
    void parseJSONForMediaStop(QJsonObject& json);
    void parseJSONForMediaUpdate(QJsonObject& json);
    void parseJSONForMediaListener(QJsonObject& json);

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
    QString mListenerFacing = qsl("north");
    QString mScriptListenerFacing;
    QMap<QNetworkReply*, TMediaData> mMediaDownloads;
};
#endif // MUDLET_TMEDIA_H
