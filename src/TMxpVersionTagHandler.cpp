/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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


#include "TMxpVersionTagHandler.h"
#include "TMxpClient.h"

TMxpTagHandlerResult TMxpVersionTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)
    const QString& version = client.getVersion();

    if (tag->getAttributesCount() > 0) {
        // Get the first arg (spaces or = in StyleId must be quoted)
        client.setStyle(tag->getAttrName(0));
        // Don't return a version string if there was an argument!
        return MXP_TAG_HANDLED;
    }

    // version (aka client.getVersion()) starts with the client name 'mudlet' which
    // is already in scmVersionString as the CLIENT attribute. We just need the version
    // number here which follows after a space in version.
    //
    // As an attribute/version number with spaces probably should be quoted following
    // MXP/XML syntax we just take everything after the last space to be on the safe side.

    QString payload = scmVersionString.arg(version.section(' ', -1));

    // Add the style, if it had been set
    if (!client.getStyle().isNull()) {
        payload.replace(qsl(">"), qsl(" STYLE=%1>").arg(client.getStyle()));
    }


    client.sendToServer(payload);

    return MXP_TAG_HANDLED;
}
