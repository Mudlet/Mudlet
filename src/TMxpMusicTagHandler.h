/***************************************************************************
 *   Copyright (C) 2020 by Mike Conley - sousesider[at]gmail.com           *
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

#ifndef MUDLET_TMXPMUSICTAGHANDLER_H
#define MUDLET_TMXPMUSICTAGHANDLER_H

#include "TMxpTagHandler.h"

// <music FName="never_gonna_give_you_up.mp3" [V=100] [L=1] [C=1] [T="misc"] [U="https://www.example.com/sounds/"]>
class TMxpMusicTagHandler : public TMxpSingleTagHandler
{
public:
    TMxpMusicTagHandler() : TMxpSingleTagHandler("MUSIC") {}

    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
};
#endif //MUDLET_TMXPMUSICTAGHANDLER_H
