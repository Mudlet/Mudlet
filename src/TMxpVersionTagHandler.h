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

#ifndef MUDLET_TMXPVERSIONTAGHANDLER_H
#define MUDLET_TMXPVERSIONTAGHANDLER_H
#include "TMxpTagHandler.h"

class TMxpVersionTagHandler : public TMxpSingleTagHandler
{
public:
    TMxpVersionTagHandler() : TMxpSingleTagHandler("VERSION") {}
    inline static const QString scmVersionString = QStringLiteral("\n\x1b[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=%1>\n");

    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
};
#include "TMxpTagHandler.h"
#endif //MUDLET_TMXPVERSIONTAGHANDLER_H
