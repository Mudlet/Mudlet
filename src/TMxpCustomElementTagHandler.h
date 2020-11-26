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

#ifndef MUDLET_TMXPCUSTOMELEMENTTAGHANDLER_H
#define MUDLET_TMXPCUSTOMELEMENTTAGHANDLER_H

#include "TMxpContext.h"
#include "TMxpElementRegistry.h"
#include "TMxpTagHandler.h"

class TMxpCustomElementTagHandler : public TMxpTagHandler
{
    QString mCurrentFlagName;
    QString mCurrentFlagContent;
    QMap<QString, QString> mCurrentFlagAttributes;

    MxpStartTag resolveElementDefinition(const TMxpElement& element, MxpStartTag* definitionTag, MxpStartTag* customTag) const;
    static QString mapAttributes(const TMxpElement& element, const QString& input, MxpStartTag* tag);
    void setFlag(TMxpClient& ctx, const MxpStartTag* tag, const TMxpElement& el);
    void configFlag(TMxpClient& client, MxpStartTag* tag, const TMxpElement& el);
    const QMap<QString, QString>& parseFlagAttributes(const MxpStartTag* tag, const TMxpElement& el);

public:
    bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override { return ctx.getElementRegistry().containsElement(tag->getName()); }

    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
    TMxpTagHandlerResult handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag) override;
    void handleContent(char ch) override;
};
#include "TMxpTagHandler.h"
#endif //MUDLET_TMXPCUSTOMELEMENTTAGHANDLER_H
