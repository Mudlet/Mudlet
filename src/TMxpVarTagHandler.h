/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
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

#ifndef MUDLET__TMXPVARTAGHANDLER_H
#define MUDLET__TMXPVARTAGHANDLER_H

//<VAR>     <V>
//<VAR Name [DESC=description] [PRIVATE] [PUBLISH] [DELETE] [ADD] [REMOVE]>Value</VAR>
#include "TMxpTagHandler.h"
class TMxpVarTagHandler : public TMxpTagHandler {
    MxpStartTag mCurrentStartTag;
    QString mCurrentVarContent;
public:
    TMxpVarTagHandler() : mCurrentStartTag(MxpStartTag("VAR")) {}
    bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override;
    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
    TMxpTagHandlerResult handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag) override;

    void handleContent(char ch) override;
};

#endif//MUDLET__TMXPVARTAGHANDLER_H
