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
#include "TMxpVarTagHandler.h"
#include "TMxpClient.h"
bool TMxpVarTagHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    return tag->isNamed("VAR") || tag->isNamed("V");
}


TMxpTagHandlerResult TMxpVarTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    mCurrentStartTag = *tag;
    mCurrentVarContent.clear();
    return MXP_TAG_HANDLED;
}

//<VAR>     <V>
//<VAR Name [DESC=description] [PRIVATE] [PUBLISH] [DELETE] [ADD] [REMOVE]>Value</VAR>
TMxpTagHandlerResult TMxpVarTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    const QString& name = mCurrentStartTag.getAttrName(0);
    const QString& value = mCurrentVarContent;

    if (mCurrentStartTag.hasAttribute("PUBLISH") || !mCurrentStartTag.hasAttribute("DELETE")) {
        client.setVariable(name, value);
    }

    return MXP_TAG_HANDLED;
}
void TMxpVarTagHandler::handleContent(char ch)
{
    mCurrentVarContent.append(ch);
}
