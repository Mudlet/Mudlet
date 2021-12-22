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

    enum MediaFade { MediaFadeNotSet = 0 };

    TMediaData()
    {}

    int getMediaProtocol() const { return mMediaProtocol; }
    void setMediaProtocol(int mediaProtocol) { mMediaProtocol = mediaProtocol; }
    int getMediaType() const { return mMediaType; }
    void setMediaType(int mediaType) { mMediaType = mediaType; }
    QString getMediaFileName() const { return mMediaFileName; }
    void setMediaFileName(QString mediaFileName) { mMediaFileName = mediaFileName; }
    int getMediaVolume() const { return mMediaVolume; }
    void setMediaVolume(int mediaVolume) { mMediaVolume = mediaVolume; }
    int getMediaLoops() const { return mMediaLoops; }
    void setMediaLoops(int mediaLoops) { mMediaLoops = mediaLoops; }
    int getMediaPriority() const { return mMediaPriority; }
    void setMediaPriority(int mediaPriority) { mMediaPriority = mediaPriority; }
    bool getMediaContinue() const { return mMediaContinue; }
    void setMediaContinue(bool mediaContinue) { mMediaContinue = mediaContinue; }
    QString getMediaTag() const { return mMediaTag; }
    void setMediaTag(QString mediaTag) { mMediaTag = mediaTag; }
    QString getMediaUrl() const { return mMediaUrl; }
    void setMediaUrl(QString mediaUrl) { mMediaUrl = mediaUrl; }
    QString getMediaKey() const { return mMediaKey; }
    void setMediaKey(QString mediaKey) { mMediaKey = mediaKey; }
    int getMediaFadeIn() const { return mMediaFadeIn; }
    void setMediaFadeIn(int mediaFadeIn) { mMediaFadeIn = mediaFadeIn; }
    int getMediaFadeOut() const { return mMediaFadeOut; }
    void setMediaFadeOut(int mediaFadeOut) { mMediaFadeOut = mediaFadeOut; }
    QString getMediaAbsolutePathFileName() const { return mMediaAbsolutePathFileName; }
    void setMediaAbsolutePathFileName(QString mediaAbsolutePathFileName) { mMediaAbsolutePathFileName = mediaAbsolutePathFileName; }

private:
    int mMediaProtocol = MediaProtocolNotSet;
    int mMediaType = MediaTypeNotSet;
    QString mMediaFileName;
    int mMediaVolume = MediaVolumeDefault;
    int mMediaLoops = MediaLoopsDefault;
    int mMediaPriority = MediaPriorityNotSet;
    bool mMediaContinue = MediaContinueDefault;
    QString mMediaTag;
    QString mMediaUrl;
    QString mMediaKey;
    int mMediaFadeIn = MediaFadeNotSet;
    int mMediaFadeOut = MediaFadeNotSet;
    QString mMediaAbsolutePathFileName;
};

#endif // MUDLET_TMEDIA_DATA_H
