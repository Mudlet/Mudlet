#ifndef MUDLET_TMEDIA_DATA_H
#define MUDLET_TMEDIA_DATA_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2019, 2022 by Stephen Lyons                        *
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


#include <QString>

class TMediaData
{
public:
    enum MediaProtocol {
        MediaProtocolAPI = -1,
        MediaProtocolNotSet = 0,
        MediaProtocolMSP = 90,
        MediaProtocolGMCP = 201};

    enum MediaType {
        MediaTypeNotSet = 0,
        MediaTypeSound = 1,
        MediaTypeMusic = 2,
        MediaTypeVideo = 3};

    enum MediaInput {
        MediaInputNotSet = 0,
        MediaInputFile = 1,
        MediaInputStream = 2};

    enum MediaVolume {
        MediaVolumePreload = 0,
        MediaVolumeMin = 1,
        MediaVolumeLow = 25,
        MediaVolumeDefault = 50,
        MediaVolumeHigh = 75,
        MediaVolumeMax = 100};

    enum MediaLoops {
        MediaLoopsRepeat = -1,
        MediaLoopsDefault = 1};

    enum MediaPriority {
        MediaPriorityNotSet = 0,
        MediaPriorityMin = 1,
        MediaPriorityLow = 25,
        MediaPriorityDefault = 50,
        MediaPriorityHigh = 75,
        MediaPriorityMax = 100};

    enum MediaContinue {
        MediaContinueRestart = false,
        MediaContinueDefault = true};

    enum MediaFadeAway {
        MediaFadeAwayEnabled = true,
        MediaFadeAwayDefault = false};

    static constexpr int MediaFadeNotSet = 0;
    static constexpr int MediaStartDefault = 0;
    static constexpr int MediaEndNotSet = 0;
    static constexpr int MediaFinishNotSet = 0;

    static constexpr const char* MediaWidgetLabel = "label";
    static constexpr const char* MediaWidgetWindow = "window";

    enum MediaClose {
        MediaCloseEnabled = true,
        MediaCloseDefault = false};

    int mediaProtocol() const { return mMediaProtocol; }
    void setMediaProtocol(int mediaProtocol) { mMediaProtocol = mediaProtocol; }

    int mediaType() const { return mMediaType; }
    void setMediaType(int mediaType) { mMediaType = mediaType; }

    int mediaInput() const { return mMediaInput; }
    void setMediaInput(int mediaInput) { mMediaInput = mediaInput; }

    QString mediaFileName() const { return mMediaFileName; }
    void setMediaFileName(QString mediaFileName) { mMediaFileName = mediaFileName; }

    int mediaVolume() const { return mMediaVolume; }
    void setMediaVolume(int mediaVolume)
    {
        if (mediaVolume == TMediaData::MediaVolumePreload) {
            // Support preloading
            mMediaVolume = TMediaData::MediaVolumePreload;
        } else {
            mMediaVolume = qBound(static_cast<int>(TMediaData::MediaVolumeMin), mediaVolume, static_cast<int>(TMediaData::MediaVolumeMax));
        }
    }

    int mediaLoops() const { return mMediaLoops; }
    void setMediaLoops(int mediaLoops)
    {
        if (mediaLoops < TMediaData::MediaLoopsRepeat || mediaLoops == 0) {
            mMediaLoops = TMediaData::MediaLoopsDefault;
        } else {
            mMediaLoops = mediaLoops;
        }
    }

    int mediaPriority() const { return mMediaPriority; }
    void setMediaPriority(int mediaPriority)
    {
        if (mediaPriority == TMediaData::MediaPriorityNotSet) {
            mMediaPriority = TMediaData::MediaPriorityNotSet;
        } else {
            mMediaPriority = qBound(static_cast<int>(TMediaData::MediaPriorityMin), mediaPriority, static_cast<int>(TMediaData::MediaPriorityMax));
        }
    }

    bool mediaContinue() const { return mMediaContinue; }
    void setMediaContinue(bool mediaContinue) { mMediaContinue = mediaContinue; }

    QString mediaTag() const { return mMediaTag; }
    void setMediaTag(QString mediaTag) { mMediaTag = mediaTag; }

    QString mediaUrl() const { return mMediaUrl; }
    void setMediaUrl(QString mediaUrl) { mMediaUrl = mediaUrl; }

    QString mediaKey() const { return mMediaKey; }
    void setMediaKey(QString mediaKey) { mMediaKey = mediaKey; }

    int mediaFadeIn() const { return mMediaFadeIn; }
    void setMediaFadeIn(int mediaFadeIn)
    {
        if (mediaFadeIn < TMediaData::MediaFadeNotSet) {
            mMediaFadeIn = TMediaData::MediaFadeNotSet;
        } else {
            mMediaFadeIn = mediaFadeIn;
        }
    }

    int mediaFadeOut() const { return mMediaFadeOut; }
    void setMediaFadeOut(int mediaFadeOut)
    {
        if (mediaFadeOut < TMediaData::MediaFadeNotSet) {
            mMediaFadeOut = TMediaData::MediaFadeNotSet;
        } else {
            mMediaFadeOut = mediaFadeOut;
        }
    }

    int mediaStart() const { return mMediaStart; }
    void setMediaStart(int mediaStart)
    {
        if (mediaStart < TMediaData::MediaStartDefault) {
            mMediaStart = TMediaData::MediaStartDefault;
        } else {
            mMediaStart = mediaStart;
        }
    }

    int mediaFinish() const { return mMediaFinish; }
    void setMediaFinish(int mediaFinish)
    {
        if (mediaFinish < TMediaData::MediaFinishNotSet) {
            mMediaFinish = TMediaData::MediaFinishNotSet;
        } else {
            mMediaFinish = mediaFinish;
        }
    }

    bool mediaFadeAway() const { return mMediaFadeAway; }
    void setMediaFadeAway(bool mediaFadeAway) { mMediaFadeAway = mediaFadeAway; }

    int mediaEnd() const { return mMediaEnd; }
    void setMediaEnd(int mediaEnd)
    {
        if (mediaEnd < TMediaData::MediaEndNotSet) {
            mMediaEnd = TMediaData::MediaEndNotSet;
        } else {
            mMediaEnd = mediaEnd;
        }
    }

    QString mediaWidget() const { return mMediaWidget; }
    void setMediaWidget(QString mediaWidget) { mMediaWidget = mediaWidget; }

    bool mediaClose() const { return mMediaClose; }
    void setMediaClose(bool mediaClose) { mMediaClose = mediaClose; }

    QString mediaAbsolutePathFileName() const { return mMediaAbsolutePathFileName; }
    void setMediaAbsolutePathFileName(QString mediaAbsolutePathFileName) { mMediaAbsolutePathFileName = mediaAbsolutePathFileName; }

private:
    int mMediaProtocol = MediaProtocolNotSet;
    int mMediaType = MediaTypeNotSet;
    int mMediaInput = MediaInputNotSet;
    QString mMediaFileName;
    int mMediaVolume = MediaVolumeDefault;
    int mMediaFadeIn = MediaFadeNotSet;
    int mMediaFadeOut = MediaFadeNotSet;
    int mMediaStart = MediaStartDefault;
    int mMediaFinish = MediaFinishNotSet;
    int mMediaLoops = MediaLoopsDefault;
    int mMediaPriority = MediaPriorityNotSet;
    bool mMediaContinue = MediaContinueDefault;
    bool mMediaFadeAway = MediaFadeAwayDefault;
    int mMediaEnd = MediaEndNotSet;
    bool mMediaClose = MediaCloseDefault;
    QString mMediaTag;
    QString mMediaUrl;
    QString mMediaKey;
    QString mMediaWidget = MediaWidgetLabel;
    QString mMediaAbsolutePathFileName;
};

#endif // MUDLET_TMEDIA_DATA_H
