#ifndef MUDLET_TMXPFRAMETAGHANDLER_H
#define MUDLET_TMXPFRAMETAGHANDLER_H

/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TMxpTagHandler.h"

// Handles MXP <FRAME> tag for creating internal/external windows
// Usage: <FRAME name="frameName" INTERNAL align="left" width="25%" height="30c" scrolling="YES">
class TMxpFrameTagHandler : public TMxpSingleTagHandler
{
public:
    TMxpFrameTagHandler()
    : TMxpSingleTagHandler(qsl("FRAME"))
    {}

    bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override;
    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;

private:
    QMap<QString, QString> extractAttributes(MxpStartTag* tag);
};

#endif // MUDLET_TMXPFRAMETAGHANDLER_H
