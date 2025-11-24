#ifndef MUDLET_TMXPDESTTAGHANDLER_H
#define MUDLET_TMXPDESTTAGHANDLER_H

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

// Handles MXP <DEST> tag for redirecting output to frames
// Usage: <DEST frameName EOF>output here</DEST>
class TMxpDestTagHandler : public TMxpTagHandler
{
public:
    TMxpDestTagHandler() = default;

    bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override;
    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
    TMxpTagHandlerResult handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag) override;

private:
    QString extractFrameName(MxpStartTag* tag);
    bool hasEOL(MxpStartTag* tag);
    bool hasEOF(MxpStartTag* tag);
};

#endif // MUDLET_TMXPDESTTAGHANDLER_H
