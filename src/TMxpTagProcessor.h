#ifndef MUDLET_TMXPTAGPROCESSOR_H
#define MUDLET_TMXPTAGPROCESSOR_H

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

#include "pre_guard.h"
#include <QMap>
#include <QString>
#include <QVector>
#include "post_guard.h"

#include "MxpTag.h"
#include "TEntityResolver.h"
#include "TMxpClient.h"
#include "TMxpContext.h"
#include "TMxpElementRegistry.h"
#include "TMxpTagHandler.h"
#include "TMxpTagHandlerResult.h"

typedef QPair<QString, QVector<QString>> TMxpFeatureOptions;

class TMxpTagProcessor : public TMxpContext
{
    QMap<QString, QVector<QString>> mSupportedMxpElements;
    QList<QSharedPointer<TMxpTagHandler>> mRegisteredHandlers;

    TMxpElementRegistry mMxpElementRegistry;
    TEntityResolver mEntityResolver;

public:
    TMxpTagProcessor();
    TMxpTagHandlerResult process(TMxpContext& ctx, TMxpClient& client, const std::string& currentToken);

    TMxpTagHandlerResult handleTag(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override;
    void handleContent(char ch) override;

    void registerHandler(const TMxpFeatureOptions& supports, TMxpTagHandler* handler);
    void registerHandler(TMxpTagHandler* handler);

    TMxpElementRegistry& getElementRegistry() override;
    QMap<QString, QVector<QString>>& getSupportedElements() override;
    TMxpTagHandler& getMainHandler() override;

    TEntityResolver& getEntityResolver() override;
};

#endif //MUDLET_TMXPTAGPROCESSOR_H
