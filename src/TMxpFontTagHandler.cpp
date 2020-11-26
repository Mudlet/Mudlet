/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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

#include "TMxpFontTagHandler.h"
#include "TMxpClient.h"

TMxpTagHandlerResult TMxpFontTagHandler::handleStartTag(TMxpContext& context, TMxpClient& client, MxpStartTag* tag)
{
    QString fontFace = tag->getAttributeByNameOrIndex("FACE", 0);
    QString fontSize = tag->getAttributeByNameOrIndex("SIZE", 1);
    client.pushFont(fontFace, fontSize);

    QString fgColor = tag->getAttributeByNameOrIndex("COLOR", 2);
    QString bgColor = tag->getAttributeByNameOrIndex("BACK", 3);
    client.pushColor(fgColor, bgColor);

    return MXP_TAG_HANDLED;
}
TMxpTagHandlerResult TMxpFontTagHandler::handleEndTag(TMxpContext& context, TMxpClient& client, MxpEndTag* tag)
{
    client.popFont();
    client.popColor();

    return MXP_TAG_HANDLED;
}
