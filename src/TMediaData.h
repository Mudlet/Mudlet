#ifndef MUDLET_TMEDIA_DATA_H
#define MUDLET_TMEDIA_DATA_H

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


#include <QString>

class TMediaData
{
public:
    enum MediaProtocol { MediaProtocolMSP = 90, MediaProtocolGMCP = 201, MediaProtocolNotSet = 0 };

    enum MediaType { MediaTypeSound = 1, MediaTypeMusic = 2, MediaTypeNotSet = 0 };

    enum MediaVolume { MediaVolumeMax = 100, MediaVolumeHigh = 75, MediaVolumeDefault = 50, MediaVolumeLow = 25, MediaVolumeMin = 1, MediaVolumePreload = 0 };

    enum MediaLoops { MediaLoopsDefault = 1, MediaLoopsRepeat = -1 };

    enum MediaPriority { MediaPriorityMax = 100, MediaPriorityHigh = 75, MediaPriorityDefault = 50, MediaPriorityLow = 25, MediaPriorityMin = 1, MediaPriorityNotSet = 0 };

    enum MediaContinue { MediaContinueDefault = true, MediaContinueRestart = false };

    TMediaData()
    {
        mMediaProtocol = MediaProtocolNotSet;
        mMediaType = MediaTypeNotSet;
        mMediaVolume = MediaVolumeDefault;
        mMediaLoops = MediaLoopsDefault;
        mMediaPriority = MediaPriorityNotSet;
        mMediaContinue = MediaContinueDefault;
    }

    int getMediaProtocol() { return mMediaProtocol; }
    void setMediaProtocol(int mediaProtocol) { mMediaProtocol = mediaProtocol; }
    int getMediaType() { return mMediaType; }
    void setMediaType(int mediaType) { mMediaType = mediaType; }
    QString getMediaFileName() { return mMediaFileName; }
    void setMediaFileName(QString mediaFileName) { mMediaFileName = mediaFileName; }
    int getMediaVolume() { return mMediaVolume; }
    void setMediaVolume(int mediaVolume) { mMediaVolume = mediaVolume; }
    int getMediaLoops() { return mMediaLoops; }
    void setMediaLoops(int mediaLoops) { mMediaLoops = mediaLoops; }
    int getMediaPriority() { return mMediaPriority; }
    void setMediaPriority(int mediaPriority) { mMediaPriority = mediaPriority; }
    bool getMediaContinue() { return mMediaContinue; }
    void setMediaContinue(bool mediaContinue) { mMediaContinue = mediaContinue; }
    QString getMediaTag() { return mMediaTag; }
    void setMediaTag(QString mediaTag) { mMediaTag = mediaTag; }
    QString getMediaUrl() { return mMediaUrl; }
    void setMediaUrl(QString mediaUrl) { mMediaUrl = mediaUrl; }
    QString getMediaKey() { return mMediaKey; }
    void setMediaKey(QString mediaKey) { mMediaKey = mediaKey; }
    QString getMediaAbsolutePathFileName() { return mMediaAbsolutePathFileName; }
    void setMediaAbsolutePathFileName(QString mediaAbsolutePathFileName) { mMediaAbsolutePathFileName = mediaAbsolutePathFileName; }

private:
    int mMediaProtocol;
    int mMediaType;
    QString mMediaFileName;
    int mMediaVolume;
    int mMediaLoops;
    int mMediaPriority;
    bool mMediaContinue;
    QString mMediaTag;
    QString mMediaUrl;
    QString mMediaKey;
    QString mMediaAbsolutePathFileName;
};

#endif // MUDLET_TMEDIA_DATA_H
