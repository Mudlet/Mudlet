#ifndef MUDLET_TROOM_H
#define MUDLET_TROOM_H

/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2015 by Stephen Lyons - slysven@virginmedia.com    *
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
#include <QColor>
#include <QMap>
#include <QVector3D>
#include <QHash>
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
public:
    TRoom( TRoomDB* pRDB );
    ~TRoom();
    void setId( int );
    bool setExit( int to, int direction );
    int getExit( int direction );
    QSet<QPair<quint8, quint64> > getNormalExits();
    QSet<QPair<QString, quint64> > getSpecialExits();
    QSet<QPair<quint8, quint64> > getNormalEntrances() const { return mNormalEntrances; }
    QSet<QPair<QString, quint64> > getSpecialEntrances() const { return mSpecialEntrances; }
    void setEntrance( QPair<quint8, quint64> );
    void setEntrance( QPair<QString, quint64> );
    bool hasExit( int direction );
    void setWeight( int );
    void setExitLock( int, bool );
    void setSpecialExitLock( int to, const QString& cmd, bool doLock );
    bool setSpecialExitLock(const QString& cmd, bool doLock );
    bool hasExitLock( int to );
    bool hasSpecialExitLock( int, const QString& );
    bool setSpecialExit( int, const QString& );
    void clearSpecialExits() { other.clear(); }
    const QMultiMap<int, QString> & getOtherMap() const { return other; }
    const QMap<QString, int> & getExitWeights() const { return exitWeights; }
    void setExitWeight(const QString& cmd, int w );
    bool hasExitWeight(const QString& cmd );
    void setDoor(const QString& cmd, int doorStatus );//0=no door, 1=open door, 2=closed, 3=locked
    int getDoor(const QString& cmd );
    bool hasExitStub( int direction );
    void setExitStub( int direction, bool status );
    void calcRoomDimensions();
    bool setArea( int , bool isToDeferAreaRelatedRecalculations = false );
    int getExitWeight(const QString& cmd );
    int getWeight() { return weight; }
    int getNorth() { return north; }
    int getNorthwest() { return northwest; }
    int getNortheast() { return northeast; }
    int getSouth() { return south; }
    int getSouthwest() { return southwest; }
    int getSoutheast() { return southeast; }
    int getWest() { return west; }
    int getEast() { return east; }
    int getUp() { return up; }
    int getDown() { return down; }
    int getIn() { return in; }
    int getOut() { return out; }
    int getId() { return id; }
    int getArea() { return area; }
    void auditExits();
    /*bool*/ void restore( QDataStream & ifs, int version );


    int x;
    int y;
    int z;
    int environment;
    bool isLocked;
    qreal min_x;
    qreal min_y;
    qreal max_x;
    qreal max_y;
    qint8 c;
    QString name;
//    QVector3D v;
    QList<int> exitStubs; //contains a list of: exittype (according to defined values above)
    QMap<QString, QString> userData;
    QList<int> exitLocks;
    QMap<QString, QList<QPointF> > customLines;
    QMap<QString, QList<int> > customLinesColor;
    QMap<QString, QString> customLinesStyle;
    QMap<QString, bool> customLinesArrow;
    bool highlight;
    QColor highlightColor;
    QColor highlightColor2;
    float highlightRadius;
    bool rendered;
    QMap<QString, int> doors; //0=no door 1=open 2=closed 3=locked


private:
    void setEntrance( int, quint8 );
    void setEntrance( int, QString );
    void resetEntrance( int, quint8 );
    void resetEntrance( int, QString );

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
// To add in the next map file format increment so we don't have to regenerate
// them on loading:
    QSet<QPair<quint8, quint64> > mNormalEntrances;
// Normal exits that have THIS room as a destination
// first is direction code from the OTHER room, but NOT DIR_OTHER, second is fromRoomId.
    QSet<QPair<QString, quint64> > mSpecialEntrances;
// Special exits that have THIS room as a destination
// first is exit text from the OTHER room, second is fromRoomId.
// Reason for separation for special from normal entrances:
// using text for normal exits is language dependent would make code for normal
// exits more complex (slower) than needs be - this is based on the assumption
// that a MUD has many more normal than special exits.
//
// Actually the same concept could be done for:
//    QHash<quint8, quint64> normalExits;
//    QHash<QString, quint64> SpecialExits;
// Where second is the toRoomId in both cases and the first is the normal exit
// code or special exit name...


    // FIXME: This should be a map of String->room id because there can be multiple special exits to the same room
    QMultiMap<int, QString> other; // es knnen mehrere exits zum gleichen raum verlaufen
                                   //verbotene exits werden mit 0 geprefixed, offene mit 1

    TRoomDB * mpRoomDB;
    friend class XMLimport;
    friend class XMLexport;
};

#endif // MUDLET_TROOM_H
