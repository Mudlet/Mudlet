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

    const QString fileName = extractFileName(tag);

    if (!fileName.isEmpty()) {
        const QString volume = extractVolume(tag);
        const QString loops = extractLoops(tag);
        const QString musicContinue = extractMusicContinue(tag);
        const QString type = extractType(tag);
        const QString url = extractUrl(tag);

        TMediaData mediaData {};

        mediaData.setMediaProtocol(TMediaData::MediaProtocolMSP);
        mediaData.setMediaType(TMediaData::MediaTypeMusic);

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
    return tag->getAttributeByNameOrIndex(qsl("fname"), 0);
}

QString TMxpMusicTagHandler::extractVolume(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("v"), 1);
}

QString TMxpMusicTagHandler::extractLoops(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("l"), 2);
}

QString TMxpMusicTagHandler::extractMusicContinue(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("c"), 3);
}

QString TMxpMusicTagHandler::extractType(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("t"), 4);
}

QString TMxpMusicTagHandler::extractUrl(MxpStartTag* tag)
{
    return tag->getAttributeByNameOrIndex(qsl("u"), 5);
}
