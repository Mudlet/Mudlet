/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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

#include "TMxpFormattingTagsHandler.h"
#include "TMxpClient.h"

bool TMxpFormattingTagsHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    Q_UNUSED(ctx)
    Q_UNUSED(client)

    return tag->isNamed(qsl("B")) || tag->isNamed(qsl("BOLD")) || tag->isNamed(qsl("STRONG")) ||
           tag->isNamed(qsl("H")) || tag->isNamed(qsl("HIGH")) ||
           tag->isNamed(qsl("I")) || tag->isNamed(qsl("ITALIC")) || tag->isNamed(qsl("EM")) ||
           tag->isNamed(qsl("U")) || tag->isNamed(qsl("UNDERLINE")) ||
           tag->isNamed(qsl("S")) || tag->isNamed(qsl("STRIKEOUT")) ||
           // Additional HTML tags - recognized but not styled differently:
           tag->isNamed(qsl("H1")) || tag->isNamed(qsl("H2")) || tag->isNamed(qsl("H3")) ||
           tag->isNamed(qsl("H4")) || tag->isNamed(qsl("H5")) || tag->isNamed(qsl("H6")) ||
           tag->isNamed(qsl("SMALL")) || tag->isNamed(qsl("TT"));
}

TMxpTagHandlerResult TMxpFormattingTagsHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)

    setAttribute(client, tag, true);

    return MXP_TAG_HANDLED;
}

TMxpTagHandlerResult TMxpFormattingTagsHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    Q_UNUSED(ctx)

    setAttribute(client, tag, false);

    return MXP_TAG_HANDLED;
}

void TMxpFormattingTagsHandler::setAttribute(TMxpClient& client, MxpTag* tag, bool value) const
{
    if (tag->isNamed("B") || tag->isNamed("BOLD") || tag->isNamed("STRONG") || tag->isNamed("H") || tag->isNamed("HIGH")) {
        client.setBold(value);
    } else if (tag->isNamed("I") || tag->isNamed("ITALIC") || tag->isNamed("EM")) {
        client.setItalic(value);
    } else if (tag->isNamed("U") || tag->isNamed("UNDERLINE")) {
        client.setUnderline(value);
    } else if (tag->isNamed("S") || tag->isNamed("STRIKEOUT")) {
        client.setStrikeOut(value);
    } else if (tag->isNamed("H1") || tag->isNamed("H2") || tag->isNamed("H3") ||
               tag->isNamed("H4") || tag->isNamed("H5") || tag->isNamed("H6") ||
               tag->isNamed("SMALL") || tag->isNamed("TT")) {
        // Recognized but no styling applied
    } else {
        // do nothing
    }
}
