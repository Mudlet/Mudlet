/***************************************************************************
 *   Copyright (C) 2020 by Mike Conley - sousesider[at]gmail.com           *
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
#include "TMxpMusicTagHandler.h"
#include "TMxpClient.h"

TMxpTagHandlerResult TMxpMusicTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    TMediaData mediaData {};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolMSP);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);
    mediaData.setMediaFileName(tag->getAttributeValue("FName"));

    if (tag->hasAttribute("V")) {
        QString volume = tag->getAttributeValue("V");

        if (!volume.isEmpty()) {
            mediaData.setMediaVolume(volume.toInt());

            if (mediaData.getMediaVolume() == TMediaData::MediaVolumePreload) {
                // Support preloading
            } else if (mediaData.getMediaVolume() > TMediaData::MediaVolumeMax) {
                mediaData.setMediaVolume(TMediaData::MediaVolumeMax);
            } else if (mediaData.getMediaVolume() < TMediaData::MediaVolumeMin) {
                mediaData.setMediaVolume(TMediaData::MediaVolumeMin);
            }
        }
    }

    if (tag->hasAttribute("L")) {
        QString loops = tag->getAttributeValue("L");

        if (!loops.isEmpty()) {
            mediaData.setMediaLoops(loops.toInt());

            if (mediaData.getMediaLoops() < TMediaData::MediaLoopsRepeat || mediaData.getMediaLoops() == 0) {
                mediaData.setMediaLoops(TMediaData::MediaLoopsDefault);
            }
        }
    }

    if (tag->hasAttribute("P")) {
        QString priority = tag->getAttributeValue("P");

        if (!priority.isEmpty()) {
            mediaData.setMediaPriority(priority.toInt());

            if (mediaData.getMediaPriority() > TMediaData::MediaPriorityMax) {
                mediaData.setMediaPriority(TMediaData::MediaPriorityMax);
            } else if (mediaData.getMediaPriority() < TMediaData::MediaPriorityMin) {
                mediaData.setMediaPriority(TMediaData::MediaPriorityMin);
            }
        }
    }

    if (tag->hasAttribute("C")) {
        QString musicContinue = tag->getAttributeValue("C");

        if (!musicContinue.isEmpty()) {
            if (musicContinue.toInt() == 0) {
                mediaData.setMediaContinue(TMediaData::MediaContinueRestart);
            } else {
                mediaData.setMediaContinue(TMediaData::MediaContinueDefault);
            }
        }
    }

    if (tag->hasAttribute("T")) {
        QString type = tag->getAttributeValue("T");

        if (!type.isEmpty()) {
            mediaData.setMediaTag(type.toLower());
        }
    }

    if (tag->hasAttribute("U")) {
        QString url = tag->getAttributeValue("U");

        if (!url.isEmpty()) {
            mediaData.setMediaUrl(url);
        }
    }

    if (mediaData.getMediaFileName() == "Off" && mediaData.getMediaUrl().isEmpty()) {
        client.stopMedia(mediaData);
    } else {
        client.playMedia(mediaData);
    }

    return MXP_TAG_HANDLED;
}
