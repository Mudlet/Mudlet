/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
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

#ifndef TMAPINFOCONTRIBUTORMANAGER_H
#define TMAPINFOCONTRIBUTORMANAGER_H

#include "pre_guard.h"
#include <QMap>
#include <QObject>
#include <QString>
#include "post_guard.h"

#include "Host.h"

struct mapInfoProperties
{
    QString text;
    bool isBold;
    bool isItalic;
    QColor color;
};

class mapInfoContributorManager : QObject
{
public:
    mapInfoContributorManager(QObject* parent, Host* ph);
    ~mapInfoContributorManager();

    QMap<QString, std::function<mapInfoProperties(int roomID, int selectionSize, int areaId, bool showingCurrentArea, QColor& infoColor)>> contributors;
    mapInfoProperties fullInfo(int roomID, int selectionSize, int areaId, bool showingCurrentArea, QColor& infoColor);
    mapInfoProperties shortInfo(int roomID, int selectionSize, int areaId, bool showingCurrentArea, QColor& infoColor);

private:
    Host* mpHost;
};
#endif
