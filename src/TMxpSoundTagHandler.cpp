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

    const QString fileName = extractFileName(tag);

    if (!fileName.isEmpty()) {
        const QString volume = extractVolume(tag);
        const QString loops = extractLoops(tag);
        const QString priority = extractPriority(tag);
        const QString type = extractType(tag);
        const QString url = extractUrl(tag);

        TMediaData mediaData {};

        mediaData.setMediaProtocol(TMediaData::MediaProtocolMSP);
        mediaData.setMediaType(TMediaData::MediaTypeSound);

        mediaData.setMediaFileName(fileName);

        if (!volume.isEmpty()) {
            mediaData.setMediaVolume(volume.toInt());
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
    }

    return MXP_TAG_HANDLED;
}

QString TMxpSoundTagHandler::extractFileName(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("fname"), 0);
}

QString TMxpSoundTagHandler::extractVolume(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("v"), 1);
}

QString TMxpSoundTagHandler::extractLoops(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("l"), 2);
}

QString TMxpSoundTagHandler::extractPriority(MxpStartTag* tag)
{

    return tag->getAttributeByNameOrIndex(qsl("p"), 3);
}

QString TMxpSoundTagHandler::extractType(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("t"), 4);
}

QString TMxpSoundTagHandler::extractUrl(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("u"), 5);
}
