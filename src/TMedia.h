#ifndef MUDLET_TMEDIA_H
#define MUDLET_TMEDIA_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2019 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "Host.h"
#include "TEvent.h"
#include "mudlet.h"

class TMediaData
{
public:
    enum MediaCategory {
        MediaCategorySound = 1,
        MediaCategoryMusic = 2,
        MediaCategoryVideo = 3
    };

	enum MediaVolume {
        MediaVolumeMax = 100,
        MediaVolumeHigh = 75,
        MediaVolumeDefault = 50,
        MediaVolumeLow = 25
    };

    enum MediaLength {
        MediaLengthDefault = 1,
        MediaLengthRepeat = -1
    };

    enum MediaPriority {
        MediaPriorityMax = 100,
        MediaPriorityHigh = 75,
        MediaPriorityDefault = 50,
        MediaPriorityLow = 25
    };

    enum MediaContinue {
        MediaContinueDefault = 1,
        MediaContinueRestart = 0
    };

    TMediaData()
    {
        mMediaVolume = MediaVolumeDefault;
        mMediaLength = MediaLengthDefault;
        mSoundPriority = MediaPriorityDefault;
        mMusicContinue = MediaContinueDefault;
    }

    TMediaData(int mediaCategory, QString mediaFileName, int mediaVolume = MediaVolumeDefault, int mediaLength = MediaLengthDefault,
        QString mediaType = QString(), QString mediaUrl = QString())
    {
        mMediaCategory = mediaCategory;
        mMediaFileName = mediaFileName;
        mMediaVolume = mediaVolume;
        mMediaLength = mediaLength;
        mMediaType = mediaType;
        mMediaUrl = mediaUrl;
    }

    int getMediaCategory() { return mMediaCategory; }
    void setMediaCategory(int mediaCategory) { mMediaCategory = mediaCategory; }
    QString getMediaFileName() { return mMediaFileName; }
    void setMediaFileName(QString mediaFileName) { mMediaFileName = mediaFileName; }
    int getMediaVolume() { return mMediaVolume; }
    void setMediaVolume(int mediaVolume) { mMediaVolume = mediaVolume; }
    int getMediaLength() { return mMediaLength; }
    void setMediaLength(int mediaLength) { mMediaLength = mediaLength; }
    int getSoundPriority() { return mSoundPriority; }
    void setSoundPriority(int soundPriority) { mSoundPriority = soundPriority; }
    int getMusicContinue() { return mMusicContinue; }
    void setMusicContinue(int musicContinue) { mMusicContinue = musicContinue; }
    QString getMediaType() { return mMediaType; }
    void setMediaType(QString mediaType) { mMediaType = mediaType; }
    QString getMediaUrl() { return mMediaUrl; }
    void setMediaUrl(QString mediaUrl) { mMediaUrl = mediaUrl; }
    QString getMediaAbsolutePathFileName() { return mMediaAbsolutePathFileName; }
    void setMediaAbsolutePathFileName(QString mediaAbsolutePathFileName) { mMediaAbsolutePathFileName = mediaAbsolutePathFileName; }

private:
    int mMediaCategory;
    QString mMediaFileName;
    int mMediaVolume;
    int mMediaLength;
    QString mMediaType;
    int mSoundPriority;
    int mMusicContinue;
    QString mMediaUrl;
    QString mMediaAbsolutePathFileName;
};

class TMedia: public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TMedia)
	TMedia(Host* pHost, const QString& profileName);
	~TMedia();

    void playMedia(TMediaData& mediaData);
    void stopMedia(TMediaData::MediaCategory mediaCategory);

private:
    QUrl parseUrl(TMediaData& mediaData);
    bool isValidUrl(QUrl& url);
    bool isFileRelative(TMediaData& mediaData);
    QStringList parseFileNameList(TMediaData& mediaData, QDir &dir);
    QStringList getFileNameList(TMediaData& mediaData);
    QUrl getFileUrl(TMediaData& mediaData);
    void writeFile(QNetworkReply* reply);
    void downloadFile(TMediaData& mediaData);
    QMediaPlayer* getMediaPlayer(TMediaData& mediaData);
    QMediaPlayer* matchMediaPlayer(TMediaData& mediaData, QString absolutePathFileName);
    void playSound(TMediaData& soundData);
    void stopSound();
    void playMusic(TMediaData& musicData);
    void stopMusic();

    QPointer<Host> mpHost;
    QString mProfileName;

	QList<QMediaPlayer*> mSoundList;
	QList<QMediaPlayer*> mMusicList;

	QNetworkAccessManager* mpNetworkAccessManager;
    QMap<QNetworkReply*, TMediaData> mMediaDownloads;
};
#endif // MUDLET_TMEDIA_H