#ifndef MUDLET_TROOM_H
#define MUDLET_TROOM_H

/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2015, 2018, 2021 by Stephen Lyons                  *
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

#include "pre_guard.h"
#include <QApplication>
#include <QColor>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QVector3D>
#include "post_guard.h"

class XMLimport;
class XMLexport;
class TRoomDB;
class QJsonArray;
class QJsonObject;


class TRoom
{
    Q_DECLARE_TR_FUNCTIONS(TRoom) // Needed so we can use tr() even though TRoom is NOT derived from QObject

public:
    explicit TRoom(TRoomDB* pRDB);
    ~TRoom();
    void setId(const int);
    bool setExit(const int to, const int direction);
    int getExit(const int) const;
    QHash<int, int> getExits() const;
    bool hasExit(const int) const;
    void setWeight(int);
    void setExitLock(const int, const bool);
    bool setSpecialExitLock(const QString&, const bool);
    bool hasExitLock(const int to) const;
    bool hasSpecialExitLock(const QString&) const;
    void removeAllSpecialExitsToRoom(const int);
    void setSpecialExit(const int, const QString&);
    void clearSpecialExits();
    const QMap<QString, int>& getSpecialExits() const { return mSpecialExits; }
    const QSet<QString>& getSpecialExitLocks() const { return mSpecialExitLocks; }
    const QMap<QString, int>& getExitWeights() const { return exitWeights; }
    void setExitWeight(const QString& cmd, int w);
    bool hasExitWeight(const QString& cmd);
    bool setDoor(const QString& cmd, int doorStatus); //0=no door, 1=open door, 2=closed, 3=locked
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
                   QMap<QString, int>&,
                   QSet<int>&,
                   QSet<int>&,
                   QMap<QString, int>&,
                   QMap<QString, QList<QPointF>>&,
                   QMap<QString, QColor>&,
                   QMap<QString, Qt::PenStyle>&,
                   QMap<QString, bool>&,
                   QHash<int, int>);
    QString dirCodeToDisplayName(int) const;
    static QString dirCodeToShortString(const int);
    static QString dirCodeToString(const int);
    inline int stringToDirCode(const QString&) const;
    bool hasExitOrSpecialExit(const QString&) const;
    void writeJsonRoom(QJsonArray&) const;
    int readJsonRoom(const QJsonArray&, const int, const int);


    int x = 0;
    int y = 0;
    int z = 0;
    int environment = -1;
    int test = 5;

    bool isLocked = false;
    qreal min_x = 0.0;
    qreal min_y = 0.0;
    qreal max_x = 0.0;
    qreal max_y = 0.0;
    QString mSymbol;
    QColor mSymbolColor;
    QString name;

    QList<int> exitStubs; //contains a list of: exittype (according to defined values above)
    QMap<QString, QString> userData;
    QList<int> exitLocks;
    // Uses "shortstrings" for normal exit directions:
    QMap<QString, QList<QPointF>> customLines;
    QMap<QString, QColor> customLinesColor;
    QMap<QString, Qt::PenStyle> customLinesStyle;
    QMap<QString, bool> customLinesArrow;

    bool highlight = false;
    QColor highlightColor;
    QColor highlightColor2;
    float highlightRadius = 0.0f;
    bool rendered = false;
    // Uses "shortstrings" for normal exit directions:
    QMap<QString, int> doors; //0=no door 1=open 2=closed 3=locked


private:
    bool readJsonExits(const QJsonObject&);
    void readJsonExitStubs(const QJsonObject&);
    bool readJsonNormalExit(const QJsonObject&, const int);
    bool readJsonSpecialExit(const QJsonObject&, const QString&);
    void readJsonCustomExitLine(const QJsonObject&, const QString&);
    void readJsonUserData(const QJsonObject&);
    void readJsonDoor(const QJsonObject&, const QString&);
    void readJsonHighlight(const QJsonObject&);
    void readJsonSymbol(const QJsonObject&);

    void writeJsonExits(QJsonObject&) const;
    void writeJsonExitStubs(QJsonObject&) const;
    void writeJsonNormalExit(QJsonArray&, const int) const;
    void writeJsonSpecialExit(QJsonArray&, const QString&, const int) const;
    void writeJsonCustomExitLine(QJsonObject&, const QString&) const;
    void writeJsonUserData(QJsonObject&) const;
    void writeJsonDoor(QJsonObject&, const QString&) const;
    void writeJsonHighlight(QJsonObject&) const;
    void writeJsonSymbol(QJsonObject&) const;


    int id = 0;
    int area = -1;
    int weight = 1;
    // Uses "shortStrings" as keys for normal exits:
    QMap<QString, int> exitWeights;
    int north = -1;
    int northeast = -1;
    int east = -1;
    int southeast = -1;
    int south = -1;
    int southwest = -1;
    int west = -1;
    int northwest =-1;
    int up = -1;
    int down = -1;
    int in = -1;
    int out = -1;

    QMap<QString, int> mSpecialExits;
    QSet<QString> mSpecialExitLocks;

    TRoomDB* mpRoomDB = nullptr;
    friend class XMLimport;
    friend class XMLexport;
};

#endif // MUDLET_TROOM_H
