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
#include <QDebug>
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
    int getDoor(const QString& cmd) const;
    bool hasExitStub(int direction);
    void setExitStub(int direction, bool status);
    void calcRoomDimensions();
    bool setArea(int, bool deferAreaRecalculations = false);
    int getExitWeight(const QString& cmd);

    inline int x() const { return mX; }
    inline int y() const { return mY; }
    inline int z() const { return mZ; }
    inline void setCoordinates(const int x, const int y, const int z) {
        mX = x;
        mY = y;
        mZ = z;
    }
    inline void offset(const int deltaX, const int deltaY, const int deltaZ) {
        mX += deltaX;
        mY += deltaY;
        mZ += deltaZ;
    }
    int getWeight() const { return weight; }
    int getNorth() const { return north; }
    void setNorth(int id) { north = id; }
    int getNorthwest() const { return northwest; }
    void setNorthwest(int id) { northwest = id; }
    int getNortheast() const { return northeast; }
    void setNortheast(int id) { northeast = id; }
    int getSouth() const { return south; }
    void setSouth(int id) { south = id; }
    int getSouthwest() const { return southwest; }
    void setSouthwest(int id) { southwest = id; }
    int getSoutheast() const { return southeast; }
    void setSoutheast(int id) { southeast = id; }
    int getWest() const { return west; }
    void setWest(int id) { west = id; }
    int getEast() const { return east; }
    void setEast(int id) { east = id; }
    int getUp() const { return up; }
    void setUp(int id) { up = id; }
    int getDown() const { return down; }
    void setDown(int id) { down = id; }
    int getIn() const { return in; }
    void setIn(int id) { in = id; }
    int getOut() const { return out; }
    void setOut(int id) { out = id; }
    int getId() const { return id; }
    int getArea() const { return area; }
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

    int environment = -1;

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
    // Made private so we can catch all cases where they are to be modified:
    int mX = 0;
    int mY = 0;
    int mZ = 0;
    // Uses "shortStrings" as keys for normal exits:
    QMap<QString, int> exitWeights;
    int north = -1;
    int northeast = -1;
    int east = -1;
    int southeast = -1;
    int south = -1;
    int southwest = -1;
    int west = -1;
    int northwest = -1;
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

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug debug, const TRoom* room)
{
    if (!room) {
        return debug << "TRoom(0x0) ";
    }
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)

    debug.nospace() << "TRoom(" << room->getId() << ")";
    debug.nospace() << ", name=" << room->name;
    debug.nospace() << ", area=" << room->getArea();
    debug.nospace() << ", pos=" << room->x() << "," << room->y() << "," << room->z();

    debug.nospace() << ", exits:";
    if (room->getNorth() != -1) {
        debug.nospace() << ", north=" << room->getNorth();
    }
    if (room->getNortheast() != -1) {
        debug.nospace() << ", northeast=" << room->getNortheast();
    }
    if (room->getEast() != -1) {
        debug.nospace() << ", east=" << room->getEast();
    }
    if (room->getSoutheast() != -1) {
        debug.nospace() << ", southeast=" << room->getSoutheast();
    }
    if (room->getSouth() != -1) {
        debug.nospace() << ", south=" << room->getSouth();
    }
    if (room->getSouthwest() != -1) {
        debug.nospace() << ", southwest=" << room->getSouthwest();
    }
    if (room->getWest() != -1) {
        debug.nospace() << ", west=" << room->getWest();
    }
    if (room->getNorthwest() != -1) {
        debug.nospace() << ", northwest=" << room->getNorthwest();
    }
    if (room->getUp() != -1) {
        debug.nospace() << ", up=" << room->getUp();
    }
    if (room->getDown() != -1) {
        debug.nospace() << ", down=" << room->getDown();
    }
    if (room->getIn() != -1) {
        debug.nospace() << ", in=" << room->getIn();
    }
    if (room->getOut() != -1) {
        debug.nospace() << ", out=" << room->getOut();
    }

    QMap<QString, int> specialExits = room->getSpecialExits();
    if (!specialExits.isEmpty()) {
        debug.nospace() << ", specialExits=(";
        for (QMap<QString, int>::const_iterator it = specialExits.begin(); it != specialExits.end(); ++it) {
            debug.nospace() << it.key() << "." << it.value() << ", ";
        }
        debug.nospace() << ")";
    }

    QMap<QString, QList<QPointF>> customLines = room->customLines;
    QMap<QString, QColor> customLinesColor = room->customLinesColor;
    QMap<QString, bool> customLinesArrow = room->customLinesArrow;
    QMap<QString, Qt::PenStyle> customLinesStyle = room->customLinesStyle;
    if (!customLines.isEmpty()) {
        debug.nospace() << ", customlines=(";
        for (auto it = customLines.constBegin(); it != customLines.constEnd(); ++it) {
            debug.nospace() << it.key() << ": " << it.value() << " (color: " << customLinesColor.value(it.key()).name().toLower()
                            << ", arrow: " << (customLinesArrow.value(it.key()) ? "yes" : "no") << ", style: " << static_cast<int>(customLinesStyle.value(it.key())) << "), ";
        }
        debug.nospace() << ")";
    }


    int weight = room->getWeight();
    if (weight != -1) {
        debug.nospace() << ", weight=" << weight;
    }

    QString symbol = room->mSymbol;
    if (!symbol.isEmpty()) {
        debug.nospace() << ", symbol=" << symbol;
    }

    auto exitWeights = room->getExitWeights();
    if (!exitWeights.isEmpty()) {
        debug.nospace() << ", exitWeights=(";
        for (auto it = exitWeights.begin(); it != exitWeights.end(); ++it) {
            auto exit = it.key();
            auto weight = it.value();
            debug.nospace() << exit << "." << weight << ", ";
        }
        debug.nospace() << ")";
    }
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TROOM_H
