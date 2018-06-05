#ifndef MUDLET_TROOM_H
#define MUDLET_TROOM_H

/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2015, 2018 by Stephen Lyons                        *
 *                                            - slysven@virginmedia.com    *
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

#include "pre_guard.h"
#include <QApplication>
#include <QColor>
#include <QHash>
#include <QMap>
#include <QVector3D>
#include "post_guard.h"


#define DIR_NORTH 1
#define DIR_NORTHEAST 2
#define DIR_NORTHWEST 3
#define DIR_EAST 4
#define DIR_WEST 5
#define DIR_SOUTH 6
#define DIR_SOUTHEAST 7
#define DIR_SOUTHWEST 8
#define DIR_UP 9
#define DIR_DOWN 10
#define DIR_IN 11
#define DIR_OUT 12
#define DIR_OTHER 13

class XMLimport;
class XMLexport;
class TRoomDB;

class TRoom
{
    Q_DECLARE_TR_FUNCTIONS(TRoom) // Needed so we can use tr() even though TRoom is NOT derived from QObject

public:
    TRoom(TRoomDB* pRDB);
    ~TRoom();
    void setId(int);
    bool setExit(int to, int direction);
    int getExit(int direction);
    QHash<int, int> getExits();
    bool hasExit(int direction);
    void setWeight(int);
    void setExitLock(int, bool);
    void setSpecialExitLock(int to, const QString& cmd, bool doLock);
    bool setSpecialExitLock(const QString& cmd, bool doLock);
    bool hasExitLock(int to);
    bool hasSpecialExitLock(int, const QString&);
    void removeAllSpecialExitsToRoom(int _id);
    void setSpecialExit(int to, const QString& cmd);
    void clearSpecialExits();
    const QMultiMap<int, QString>& getOtherMap() const { return other; }
    const QMap<QString, int>& getExitWeights() const { return exitWeights; }
    void setExitWeight(const QString& cmd, int w);
    bool hasExitWeight(const QString& cmd);
    const bool setDoor(const QString& cmd, int doorStatus); //0=no door, 1=open door, 2=closed, 3=locked
    int getDoor(const QString& cmd);
    bool hasExitStub(int direction);
    void setExitStub(int direction, bool status);
    void calcRoomDimensions();
    bool setArea(int, bool isToDeferAreaRelatedRecalculations = false);
    int getExitWeight(const QString& cmd);

    int getWeight() { return weight; }
    int getNorth() { return north; }
    void setNorth(int id) { north = id; }
    int getNorthwest() { return northwest; }
    void setNorthwest(int id) { northwest = id; }
    int getNortheast() { return northeast; }
    void setNortheast(int id) { northeast = id; }
    int getSouth() { return south; }
    void setSouth(int id) { south = id; }
    int getSouthwest() { return southwest; }
    void setSouthwest(int id) { southwest = id; }
    int getSoutheast() { return southeast; }
    void setSoutheast(int id) { southeast = id; }
    int getWest() { return west; }
    void setWest(int id) { west = id; }
    int getEast() { return east; }
    void setEast(int id) { east = id; }
    int getUp() { return up; }
    void setUp(int id) { up = id; }
    int getDown() { return down; }
    void setDown(int id) { down = id; }
    int getIn() { return in; }
    void setIn(int id) { in = id; }
    int getOut() { return out; }
    void setOut(int id) { out = id; }
    int getId() { return id; }
    int getArea() { return area; }
    void audit(QHash<int, int>, QHash<int, int>);
    void auditExits(QHash<int, int>);
    /*bool*/ void restore(QDataStream& ifs, int roomID, int version);
    void auditExit(int&,
                   int,
                   QString,
                   QString,
                   QString,
                   QMap<QString, int>&,
                   QSet<int>&,
                   QSet<int>&,
                   QMap<QString, int>&,
                   QMap<QString, QList<QPointF>>&,
                   QMap<QString, QList<int>>&,
                   QMap<QString, QString>&,
                   QMap<QString, bool>&,
                   QHash<int, int>);
    const QString dirCodeToDisplayName(int dirCode);


    int x;
    int y;
    int z;
    int environment;

    bool isLocked;
    qreal min_x;
    qreal min_y;
    qreal max_x;
    qreal max_y;
    QString mSymbol;
    QString name;
    QVector3D v;
    QList<int> exitStubs; //contains a list of: exittype (according to defined values above)
    QMap<QString, QString> userData;
    QList<int> exitLocks;
    QMap<QString, QList<QPointF>> customLines;
    QMap<QString, QList<int>> customLinesColor;
    QMap<QString, QString> customLinesStyle;
    QMap<QString, bool> customLinesArrow;
    bool highlight;
    QColor highlightColor;
    QColor highlightColor2;
    float highlightRadius;
    bool rendered;
    QMap<QString, int> doors; //0=no door 1=open 2=closed 3=locked


private:
    int id;
    int area;
    int weight;
    QMap<QString, int> exitWeights;
    int north;
    int northeast;
    int east;
    int southeast;
    int south;
    int southwest;
    int west;
    int northwest;
    int up;
    int down;
    int in;
    int out;

    // FIXME: This should be a map of String->room id because there can be multiple special exits to the same room
    QMultiMap<int, QString> other; // es knnen mehrere exits zum gleichen raum verlaufen
                                   //verbotene exits werden mit 0 geprefixed, offene mit 1

    TRoomDB* mpRoomDB;
    friend class XMLimport;
    friend class XMLexport;
};

#endif // MUDLET_TROOM_H
