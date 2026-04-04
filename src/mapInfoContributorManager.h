#ifndef TMAPINFOCONTRIBUTORMANAGER_H
#define TMAPINFOCONTRIBUTORMANAGER_H

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

#include <QColor>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QtCore>

#include "Host.h"

struct lua_State;

struct MapInfoProperties
{
    bool isBold;
    bool isItalic;
    QString text;
    QColor color;
};

struct LuaCallbackRef
{
    lua_State* L;
    int ref;
};

using MapInfoCallback = std::function<MapInfoProperties(int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor)>;

class MapInfoContributorManager : public QObject
{
    Q_OBJECT

public:
    MapInfoContributorManager(QObject* parent, Host* ph);
    ~MapInfoContributorManager();

    void registerContributor(const QString& name, MapInfoCallback callback);
    void registerContributor(const QString& name, MapInfoCallback callback, lua_State* L, int callbackRef);
    bool removeContributor(const QString& name);
    bool enableContributor(const QString& name);
    bool disableContributor(const QString& name);
    MapInfoCallback getContributor(const QString& name);
    QList<QString>& getContributorKeys();
    MapInfoProperties fullInfo(int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor);
    MapInfoProperties shortInfo(int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor);

signals:
    void signal_contributorsUpdated();

private:
    void releaseLuaCallbackRef(const QString& name);

    QList<QString> ordering;
    QMap<QString, MapInfoCallback> contributors;
    QMap<QString, LuaCallbackRef> mLuaCallbackRefs;
    Host* mpHost;
};
#endif
