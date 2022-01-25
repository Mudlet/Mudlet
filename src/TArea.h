#ifndef MUDLET_TAREA_H
#define MUDLET_TAREA_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2020-2022 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
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


#include "TMap.h"

#include "TMapLabel.h"

#include "pre_guard.h"
#include <QList>
#include <QMap>
#include <QPair>
#include <QVector3D>
#include "post_guard.h"

class TRoomDB;

class TArea
{
    Q_DECLARE_TR_FUNCTIONS(TArea) // Needed so we can use tr() even though TArea is NOT derived from QObject

    friend bool TMap::serialize(QDataStream&, int);
    friend bool TMap::restore(QString, bool);
    friend bool TMap::retrieveMapFileStats(QString, QString*, int*, int*, int*, int*);

public:
    TArea(TMap*, TRoomDB*);
    ~TArea();
    int getAreaID();
    void addRoom(int id);
    const QSet<int>& getAreaRooms() const { return rooms; }
    const QList<int> getAreaExitRoomIds() const { return mAreaExits.uniqueKeys(); }
    const QMultiMap<int, QPair<QString, int>> getAreaExitRoomData() const;
    void calcSpan();
    void fast_calcSpan(int);
    void determineAreaExits();
    void determineAreaExitsOfRoom(int);
    void removeRoom(int, bool isToDeferAreaRelatedRecalculations = false);
    QList<int> getCollisionNodes();
    QList<int> getRoomsByPosition(int x, int y, int z);
    QMap<int, QMap<int, QMultiMap<int, int>>> koordinatenSystem();
    int createLabelId() const;
    void writeJsonArea(QJsonArray&) const;
    std::pair<int, QString> readJsonArea(const QJsonArray&, const int);


    QSet<int> rooms; // rooms of this area
    // TODO: These next 2 members have not been used for some time - if at all
    // - maybe they can go?
    QVector3D pos;   // pos auf der map und 0 punkt des area internen koordinatensystems
    QVector3D span;
    int min_x = 0;
    int min_y = 0;
    int min_z = 0;
    int max_x = 0;
    int max_y = 0;
    int max_z = 0;
    // Key = z-level, Value = the relevant x or y extreme:
    QMap<int, int> xminForZ;
    QMap<int, int> xmaxForZ;
    QMap<int, int> yminForZ;
    QMap<int, int> ymaxForZ;
    QList<int> zLevels; // The z-levels that ARE used, not guaranteed to be in order
    bool gridMode = false;
    bool isZone = false;
    int zoneAreaRef = 0;
    TRoomDB* mpRoomDB = nullptr;
    bool mIsDirty = false;
    QMap<QString, QString> mUserData;
    QMap<int, TMapLabel> mMapLabels;


private:
    TArea() { qFatal("FATAL: illegal default constructor use of TArea()"); };


    void readJsonUserData(const QJsonObject& obj);
    void writeJsonUserData(QJsonObject&) const;

    void readJsonLabels(const QJsonObject&);
    void writeJsonLabels(QJsonObject&) const;

    void writeJsonLabel(QJsonArray&, const int, const TMapLabel*) const;
    void readJsonLabel(const QJsonObject&);

    QSizeF readJsonSize(const QJsonObject&, const QString&) const;
    void writeJsonSize(QJsonObject&, const QString&, const QSizeF&) const;

    void writeTwinValues(QJsonObject&, const QString&, const QPointF&) const;

    QVector3D readJson3DCoordinates(const QJsonObject&, const QString&) const;
    void writeJson3DCoordinates(QJsonObject&, const QString&, const QVector3D&) const;

    QList<QByteArray> convertImageToBase64Data(const QPixmap&) const;
    QPixmap convertBase64DataToImage(const QList<QByteArray> &) const;


    // Supplied by C'tor and now needed to pass an error message upwards:
    TMap* mpMap = nullptr;
    // Rooms that border on this area:
    // key=in_area room id, pair.first=out_of_area room id pair.second=direction
    // Made private as we may change implementation detail
    QMultiMap<int, QPair<int, int>> mAreaExits;
};

#endif // MUDLET_TAREA_H
