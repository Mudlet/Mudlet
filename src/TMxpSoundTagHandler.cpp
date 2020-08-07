/***************************************************************************
 *   Copyright (C) 2020 by Mike Conley - sousesider@gmail.com              *
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

#include "TMxpSoundTagHandler.h"
#include "TMxpClient.h"
#include "TMedia.h"
bool TMxpSoundTagHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    return tag->isNamed("SOUND") || tag->isNamed("MUSIC");
}
TMxpTagHandlerResult TMxpColorTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{// <sound FName="mm_door_close1.*" [V=100] [L=1] [P=50] [T="misc"] [U="https://www.example.com/sounds/"]>
    TMediaData mediaData {};

	if (tag->isNamed("MUSIC")) {
		mediaData.setMediaType(TMediaData::MediaTypeMusic);

		QString musicContinue = tag->getAttributeByNameOrIndex("C", 1)
	    if (!musicContinue.isEmpty()) {
	        if (musicContinue.toInt() == 0) {
	            mediaData.setMediaContinue(TMediaData::MediaContinueRestart);
	        } else {
	            mediaData.setMediaContinue(TMediaData::MediaContinueDefault);
	        }
	    }
	} else {
		mediaData.setMediaType(TMediaData::MediaTypeSound);
	}

    mediaData.setMediaFile(tag->getAttributeByNameOrIndex("FName", 0));

	QString volume = tag->getAttributeByNameOrIndex("V", 1)
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

	QString loops = tag->getAttributeByNameOrIndex("L", 1)
    if (!loops.isEmpty()) {
    	mediaData.setMediaLoops(loops.toInt());

        if (mediaData.getMediaLoops() < TMediaData::MediaLoopsRepeat || mediaData.getMediaLoops() == 0) {
            mediaData.setMediaLoops(TMediaData::MediaLoopsDefault);
        }
    }

	QString priority = tag->getAttributeByNameOrIndex("P", 1)
    if (!priority.isEmpty()) {
    	mediaData.setMediaPriority(priority.toInt());

        if (mediaData.getMediaPriority() > TMediaData::MediaPriorityMax) {
            mediaData.setMediaPriority(TMediaData::MediaPriorityMax);
        } else if (mediaData.getMediaPriority() < TMediaData::MediaPriorityMin) {
            mediaData.setMediaPriority(TMediaData::MediaPriorityMin);
        }
    }

	QString tag = tag->getAttributeByNameOrIndex("T", 1)
    if (!tag.isEmpty()) {
    	mediaData.setMediaTag(tag.toLower());
    }

	QString url = tag->getAttributeByNameOrIndex("U", 1)
    if (!url.isEmpty()) {
    	mediaData.setMediaUrl(url);
    }

    client->mpMedia->playMedia(mediaData);

    return MXP_TAG_HANDLED;
}
