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

#include "TMxpColorTagHandler.h"
#include "TMxpClient.h"
bool TMxpColorTagHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    return tag->isNamed("COLOR") || tag->isNamed("C");
}
TMxpTagHandlerResult TMxpColorTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    QString fg = tag->getAttributeByNameOrIndex("FORE", 0);
    QString bg = tag->getAttributeByNameOrIndex("BACK", 1);

    client.pushColor(fg, bg);

    return MXP_TAG_HANDLED;
}
TMxpTagHandlerResult TMxpColorTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    client.popColor();
    return MXP_TAG_HANDLED;
}
