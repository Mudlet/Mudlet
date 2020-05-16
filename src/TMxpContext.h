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


#ifndef MUDLET_TMXPCONTEXT_H
#define MUDLET_TMXPCONTEXT_H

#include "TEntityResolver.h"
#include "TMxpElementRegistry.h"
#include "TMxpTagHandler.h"

#include "pre_guard.h"
#include <QMap>
#include <QVector>
#include <QString>
#include "post_guard.h"

class TMxpClient;

class TMxpContext : public TMxpTagHandler
{
public:
    virtual TMxpElementRegistry& getElementRegistry() = 0;
    virtual QMap<QString, QVector<QString>>& getSupportedElements() = 0;
    virtual TEntityResolver& getEntityResolver() = 0;

    virtual TMxpTagHandler& getMainHandler() = 0;
};

#endif //MUDLET_TMXPCONTEXT_H
