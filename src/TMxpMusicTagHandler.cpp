/***************************************************************************
 *   Copyright (C) 2020 by Mike Conley - sousesider[at]gmail.com           *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@bvirginmedia.com        *
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

#include "TMediaData.h"
#include "TMxpMusicTagHandler.h"
#include "TMxpClient.h"

TMxpTagHandlerResult TMxpMusicTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)

    QString fileName = extractFileName(tag);

    if (!fileName.isEmpty()) {
        QString volume = extractVolume(tag);
        QString loops = extractLoops(tag);
        QString musicContinue = extractMusicContinue(tag);
        QString type = extractType(tag);
        QString url = extractUrl(tag);

        TMediaData mediaData {};

        mediaData.setMediaProtocol(TMediaData::MediaProtocolMSP);
        mediaData.setMediaType(TMediaData::MediaTypeMusic);

        mediaData.setMediaFileName(fileName);

        if (!volume.isEmpty()) {
            mediaData.setMediaVolume(volume.toInt());

            if (mediaData.getMediaVolume() == TMediaData::MediaVolumePreload) {
                // Support preloading
            } else if (mediaData.getMediaVolume() > TMediaData::MediaVolumeMax) {
                mediaData.setMediaVolume(TMediaData::MediaVolumeMax);
            } else if (mediaData.getMediaVolume() < TMediaData::MediaVolumeMin) {
                mediaData.setMediaVolume(TMediaData::MediaVolumeMin);
            }
        } else {
            mediaData.setMediaVolume(TMediaData::MediaVolumeMax); // MSP the Max is the Default
        }

        if (!loops.isEmpty()) {
            mediaData.setMediaLoops(loops.toInt());

            if (mediaData.getMediaLoops() < TMediaData::MediaLoopsRepeat || mediaData.getMediaLoops() == 0) {
                mediaData.setMediaLoops(TMediaData::MediaLoopsDefault);
            }
        } else {
            mediaData.setMediaLoops(TMediaData::MediaLoopsDefault);
        }

        if (!musicContinue.isEmpty()) {
            if (musicContinue.toInt() == 0) {
                mediaData.setMediaContinue(TMediaData::MediaContinueRestart);
            } else {
                mediaData.setMediaContinue(TMediaData::MediaContinueDefault);
            }
        } else {
            mediaData.setMediaContinue(TMediaData::MediaContinueDefault);
        }

        if (!type.isEmpty()) {
            mediaData.setMediaTag(type.toLower());
        }

        if (!url.isEmpty()) {
            mediaData.setMediaUrl(url);
        }

        if (mediaData.getMediaFileName() == "Off" && mediaData.getMediaUrl().isEmpty()) {
            client.stopMedia(mediaData);
        } else {
            client.playMedia(mediaData);
        }
    }

    return MXP_TAG_HANDLED;
}

QString TMxpMusicTagHandler::extractFileName(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_FNAME)) {
        return tag->getAttributeValue(ATTR_FNAME);
    } else if (tag->getAttributesCount() > 0) {
        return tag->getAttrName(0);
    }

    return QString();
}

QString TMxpMusicTagHandler::extractVolume(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_V)) {
        return tag->getAttributeValue(ATTR_V);
    } else if (tag->getAttributesCount() > 1) {
        return tag->getAttrName(1);
    }

    return QString();
}

QString TMxpMusicTagHandler::extractLoops(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_L)) {
        return tag->getAttributeValue(ATTR_L);
    } else if (tag->getAttributesCount() > 2) {
        return tag->getAttrName(2);
    }

    return QString();
}

QString TMxpMusicTagHandler::extractMusicContinue(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_C)) {
        return tag->getAttributeValue(ATTR_C);
    } else if (tag->getAttributesCount() > 3) {
        return tag->getAttrName(3);
    }

    return QString();
}

QString TMxpMusicTagHandler::extractType(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_T)) {
        return tag->getAttributeValue(ATTR_T);
    } else if (tag->getAttributesCount() > 4) {
        return tag->getAttrName(4);
    }

    return QString();
}

QString TMxpMusicTagHandler::extractUrl(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_U)) {
        return tag->getAttributeValue(ATTR_U);
    } else if (tag->getAttributesCount() > 5) {
        return tag->getAttrName(5);
    }

    return QString();
}
