/***************************************************************************
 *   Copyright (C) 2020 by Mike Conley - sousesider[at]gmail.com           *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "TMxpSoundTagHandler.h"
#include "TMxpClient.h"

TMxpTagHandlerResult TMxpSoundTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)

    QString fileName = extractFileName(tag);

    if (!fileName.isEmpty()) {
        QString volume = extractVolume(tag);
        QString loops = extractLoops(tag);
        QString priority = extractPriority(tag);
        QString type = extractType(tag);
        QString url = extractUrl(tag);

        TMediaData mediaData {};

        mediaData.setMediaProtocol(TMediaData::MediaProtocolMSP);
        mediaData.setMediaType(TMediaData::MediaTypeSound);

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

        if (!priority.isEmpty()) {
            mediaData.setMediaPriority(priority.toInt());

            if (mediaData.getMediaPriority() > TMediaData::MediaPriorityMax) {
                mediaData.setMediaPriority(TMediaData::MediaPriorityMax);
            } else if (mediaData.getMediaPriority() < TMediaData::MediaPriorityMin) {
                mediaData.setMediaPriority(TMediaData::MediaPriorityMin);
            }
        } else {
            mediaData.setMediaPriority(TMediaData::MediaPriorityDefault);
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

        if (mediaData.getMediaFileName() == "Off" && mediaData.getMediaUrl().isEmpty()) {
            client.stopMedia(mediaData);
        } else {
            client.playMedia(mediaData);
        }
    }

    return MXP_TAG_HANDLED;
}

QString TMxpSoundTagHandler::extractFileName(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_FNAME)) {
        return tag->getAttributeValue(ATTR_FNAME);
    } else if (tag->getAttributesCount() > 0) {
        return tag->getAttrName(0);
    }

    return QString();
}

QString TMxpSoundTagHandler::extractVolume(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_V)) {
        return tag->getAttributeValue(ATTR_V);
    } else if (tag->getAttributesCount() > 1) {
        return tag->getAttrName(1);
    }

    return QString();
}

QString TMxpSoundTagHandler::extractLoops(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_L)) {
        return tag->getAttributeValue(ATTR_L);
    } else if (tag->getAttributesCount() > 2) {
        return tag->getAttrName(2);
    }

    return QString();
}

QString TMxpSoundTagHandler::extractPriority(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_P)) {
        return tag->getAttributeValue(ATTR_P);
    } else if (tag->getAttributesCount() > 3) {
        return tag->getAttrName(3);
    }

    return QString();
}

QString TMxpSoundTagHandler::extractType(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_T)) {
        return tag->getAttributeValue(ATTR_T);
    } else if (tag->getAttributesCount() > 4) {
        return tag->getAttrName(4);
    }

    return QString();
}

QString TMxpSoundTagHandler::extractUrl(MxpStartTag* tag)
{
    if (tag->hasAttribute(ATTR_U)) {
        return tag->getAttributeValue(ATTR_U);
    } else if (tag->getAttributesCount() > 5) {
        return tag->getAttrName(5);
    }

    return QString();
}
