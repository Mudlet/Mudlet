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

#ifndef MUDLET_TMXPELEMENTREGISTRY_H
#define MUDLET_TMXPELEMENTREGISTRY_H

#include "MxpTag.h"
#include "pre_guard.h"
#include <QMap>
#include <QStringList>
#include <QList>
#include <QSharedPointer>
#include "post_guard.h"

struct TMxpElement
{
    QString name;
    QString definition;
    QStringList attrs;
    QString tag;
    QString flags;
    bool open;
    bool empty;

    QString href;
    QString hint;

    QList<QSharedPointer<MxpNode>> parsedDefinition;
};

class TMxpElementRegistry
{
    QMap<QString, TMxpElement> mMXP_Elements;

public:
    void registerElement(const TMxpElement& element);
    void unregisterElement(const QString& name);

    bool containsElement(const QString& name) const;
    TMxpElement getElement(const QString& name) const;
};

#endif //MUDLET_TMXPELEMENTREGISTRY_H
