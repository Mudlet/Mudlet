#ifndef MUDLET_TMEDIA_H
#define MUDLET_TMEDIA_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2019, 2022 by Stephen Lyons                        *
 *                                            - slysven@virginmedia.com    *
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

#include "pre_guard.h"
#include <QMediaPlayer>
#include "post_guard.h"


class TMediaPlayer
{
public:
    TMediaPlayer()
    : mMediaData()
    {}
    TMediaPlayer(Host* pHost, TMediaData& mediaData)
    : mpHost(pHost)
    , mMediaData(mediaData)
    , mMediaPlayer(new QMediaPlayer(pHost))
    , initialized(true)
    {}
    ~TMediaPlayer() = default;

    TMediaData getMediaData() const { return mMediaData; }
    void setMediaData(TMediaData& mediaData) { mMediaData = mediaData; }
    QMediaPlayer* getMediaPlayer() const { return mMediaPlayer; }
    bool isInitialized() const { return initialized; }

private:
    QPointer<Host> mpHost;
    TMediaData mMediaData;
    QMediaPlayer* mMediaPlayer = nullptr;
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
    void stopMedia(TMediaData& mediaData);
    void parseGMCP(QString& packageMessage, QString& gmcp);
    bool purgeMediaCache();

private slots:
    void slot_writeFile(QNetworkReply* reply);

private:
    void stopAllMediaPlayers();
    void transitionNonRelativeFile(TMediaData& mediaData);
    QUrl parseUrl(TMediaData& mediaData);
    static bool isValidUrl(QUrl& url);
    static bool isFileRelative(TMediaData& mediaData);
    QStringList parseFileNameList(TMediaData& mediaData, QDir& dir);
    QStringList getFileNameList(TMediaData& mediaData);
    QUrl getFileUrl(TMediaData& mediaData);
    bool processUrl(TMediaData& mediaData);
    void downloadFile(TMediaData& mediaData);
    QString setupMediaAbsolutePathFileName(TMediaData& mediaData);
    QList<TMediaPlayer> getMediaPlayerList(TMediaData& mediaData);
    TMediaPlayer getMediaPlayer(TMediaData& mediaData);
    TMediaPlayer matchMediaPlayer(TMediaData& mediaData, const QString& absolutePathFileName);
    bool doesMediaHavePriorityToPlay(TMediaData& mediaData, const QString& absolutePathFileName);
    void matchMediaKeyAndStopMediaVariants(TMediaData& mediaData, const QString& absolutePathFileName);

    void play(TMediaData& mediaData);

    static TMediaData::MediaType parseJSONByMediaType(QJsonObject& json);
    static QString parseJSONByMediaFileName(QJsonObject& json);
    static int parseJSONByMediaVolume(QJsonObject& json);
    static int parseJSONByMediaFadeIn(QJsonObject& json);
    static int parseJSONByMediaFadeOut(QJsonObject& json);
    static int parseJSONByMediaStart(QJsonObject& json);
    static int parseJSONByMediaPriority(QJsonObject& json);
    static int parseJSONByMediaLoops(QJsonObject& json);
    static TMediaData::MediaContinue parseJSONByMediaContinue(QJsonObject& json);
    static QString parseJSONByMediaTag(QJsonObject& json);
    static QString parseJSONByMediaUrl(QJsonObject& json);
    static QString parseJSONByMediaKey(QJsonObject& json);

    void parseJSONForMediaDefault(QJsonObject& json);
    void parseJSONForMediaLoad(QJsonObject& json);
    void parseJSONForMediaPlay(QJsonObject& json);
    void parseJSONForMediaStop(QJsonObject& json);

    QPointer<Host> mpHost;
    QString mProfileName;

    QList<TMediaPlayer> mMSPSoundList;
    QList<TMediaPlayer> mMSPMusicList;
    QList<TMediaPlayer> mGMCPSoundList;
    QList<TMediaPlayer> mGMCPMusicList;
    QList<TMediaPlayer> mAPISoundList;
    QList<TMediaPlayer> mAPIMusicList;

    QNetworkAccessManager* mpNetworkAccessManager = nullptr;
    QMap<QNetworkReply*, TMediaData> mMediaDownloads;
};
#endif // MUDLET_TMEDIA_H
