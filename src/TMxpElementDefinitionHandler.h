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

#ifndef MUDLET_TMXPELEMENTDEFINITIONHANDLER_H
#define MUDLET_TMXPELEMENTDEFINITIONHANDLER_H
#include "TMxpElementRegistry.h"
#include "TMxpTagHandler.h"

// https://www.zuggsoft.com/zmud/mxp.htm#ELEMENT
// <!ELEMENT element-name [definition] [ATT=attribute-list] [TAG=tag] [FLAG=flags] [OPEN] [DELETE] [EMPTY]>
class TMxpElementDefinitionHandler : public TMxpTagHandler
{
public:
    bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override { return tag->isNamed("!EL") || tag->isNamed(("!ELEMENT")); }

    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
};
#include "TMxpTagHandler.h"
#endif //MUDLET_TMXPELEMENTDEFINITIONHANDLER_H
