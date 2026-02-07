/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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
#ifndef MUDLET_TMXPSTATTAGHANDLER_H
#define MUDLET_TMXPSTATTAGHANDLER_H

#include "TMxpClient.h"
#include "TMxpContext.h"
#include "TMxpTagHandler.h"

// <STAT EntityName [Max=EntityName] [Caption=text]>
// <GAUGE EntityName [Max=EntityName] [Caption=text] [Color=color]>
// These tags create status bar entries from MXP entities.
// Mudlet doesn't have a built-in status bar, so we silently consume these
// tags and queue an MXP event so Lua scripts can handle them.
class TMxpStatTagHandler : public TMxpTagHandler
{
public:
    bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override
    {
        Q_UNUSED(ctx)
        Q_UNUSED(client)
        return tag->isNamed(qsl("STAT")) || tag->isNamed(qsl("GAUGE"));
    }

    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
};

#endif // MUDLET_TMXPSTATTAGHANDLER_H
