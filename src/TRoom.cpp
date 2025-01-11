/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018, 2020-2021, 2023, 2025 by Stephen Lyons *
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


#include "TRoom.h"


#include "TArea.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringBuilder>
#include "post_guard.h"


// Helper needed to allow Qt::PenStyle enum to be unserialised (read from file)
// in Qt5 - the compilation errors that result in not having this are really
// confusing!
QDataStream &operator>>(QDataStream& ds, Qt::PenStyle& value)
{
    int temporary;
    ds >> temporary;
    switch (temporary) {
    case Qt::DotLine:
        [[fallthrough]];
    case Qt::DashLine:
        [[fallthrough]];
    case Qt::DashDotLine:
        [[fallthrough]];
    case Qt::DashDotDotLine:
        value = static_cast<Qt::PenStyle>(temporary);
        break;
    case Qt::SolidLine:
        [[fallthrough]];
    default:
    // Force anything else to be a solidline
        value = Qt::SolidLine;
    }
    return ds;
}

static const QColor scDefaultHighlightForeground(QColor(255, 150, 0));
static const QColor scDefaultHighlightBackground(QColor(0, 0, 0));

TRoom::TRoom(TRoomDB* pRDB)
: highlightColor(scDefaultHighlightForeground)
, highlightColor2(scDefaultHighlightBackground)
, mpRoomDB(pRDB)
{
}

TRoom::~TRoom()
{
    if (mpRoomDB) {
        mpRoomDB->__removeRoom(id);
    }
}

QString TRoom::dirCodeToDisplayName(const int dirCode) const
{
    switch (dirCode) {
    case DIR_NORTH:     return tr("North");
    case DIR_NORTHEAST: return tr("North-east");
    case DIR_NORTHWEST: return tr("North-west");
    case DIR_SOUTH:     return tr("South");
    case DIR_SOUTHEAST: return tr("South-east");
    case DIR_SOUTHWEST: return tr("South-west");
    case DIR_EAST:      return tr("East");
    case DIR_WEST:      return tr("West");
    case DIR_UP:        return tr("Up");
    case DIR_DOWN:      return tr("Down");
    case DIR_IN:        return tr("In");
    case DIR_OUT:       return tr("Out");
    case DIR_OTHER:     return tr("Other");
    default:            return tr("Unknown");
    }
}

/* static */ QString TRoom::dirCodeToShortString(const int dirCode)
{
    switch (dirCode) {
    case DIR_NORTH:     return QLatin1String("n");
    case DIR_NORTHEAST: return QLatin1String("ne");
    case DIR_NORTHWEST: return QLatin1String("nw");
    case DIR_EAST:      return QLatin1String("e");
    case DIR_WEST:      return QLatin1String("w");
    case DIR_SOUTH:     return QLatin1String("s");
    case DIR_SOUTHEAST: return QLatin1String("se");
    case DIR_SOUTHWEST: return QLatin1String("sw");
    case DIR_UP:        return QLatin1String("up");
    case DIR_DOWN:      return QLatin1String("down");
    case DIR_IN:        return QLatin1String("in");
    case DIR_OUT:       return QLatin1String("out");
    default:
        Q_UNREACHABLE();
    }
}

/* static */ QString TRoom::dirCodeToString(const int dirCode)
{
    switch (dirCode) {
    case DIR_NORTH:     return QLatin1String("north");
    case DIR_NORTHEAST: return QLatin1String("northeast");
    case DIR_NORTHWEST: return QLatin1String("northwest");
    case DIR_EAST:      return QLatin1String("east");
    case DIR_WEST:      return QLatin1String("west");
    case DIR_SOUTH:     return QLatin1String("south");
    case DIR_SOUTHEAST: return QLatin1String("southeast");
    case DIR_SOUTHWEST: return QLatin1String("southwest");
    case DIR_UP:        return QLatin1String("up");
    case DIR_DOWN:      return QLatin1String("down");
    case DIR_IN:        return QLatin1String("in");
    case DIR_OUT:       return QLatin1String("out");
    default:            Q_UNREACHABLE();
    }
}

/*static*/ int TRoom::stringToDirCode(const QString& string)
{
    if (string == QLatin1String("north")) {
        return DIR_NORTH;
    }
    if (string == QLatin1String("east")) {
        return DIR_EAST;
    }
    if (string == QLatin1String("south")) {
        return DIR_SOUTH;
    }
    if (string == QLatin1String("west")) {
        return DIR_WEST;
    }
    if (string == QLatin1String("up")) {
        return DIR_UP;
    }
    if (string == QLatin1String("down")) {
        return DIR_DOWN;
    }
    if (string == QLatin1String("northeast")) {
        return DIR_NORTHEAST;
    }
    if (string == QLatin1String("northwest")) {
        return DIR_NORTHWEST;
    }
    if (string == QLatin1String("southeast")) {
        return DIR_SOUTHEAST;
    }
    if (string == QLatin1String("southwest")) {
        return DIR_SOUTHWEST;
    }
    if (string == QLatin1String("in")) {
        return DIR_IN;
    }
    if (string == QLatin1String("out")) {
        return DIR_OUT;
    }
    return DIR_OTHER;
}

/*static*/ int TRoom::shortStringToDirCode(const QString& string)
{
    if (string == QLatin1String("n")) {
        return DIR_NORTH;
    }
    if (string == QLatin1String("e")) {
        return DIR_EAST;
    }
    if (string == QLatin1String("s")) {
        return DIR_SOUTH;
    }
    if (string == QLatin1String("w")) {
        return DIR_WEST;
    }
    if (string == QLatin1String("up")) {
        return DIR_UP;
    }
    if (string == QLatin1String("down")) {
        return DIR_DOWN;
    }
    if (string == QLatin1String("ne")) {
        return DIR_NORTHEAST;
    }
    if (string == QLatin1String("nw")) {
        return DIR_NORTHWEST;
    }
    if (string == QLatin1String("se")) {
        return DIR_SOUTHEAST;
    }
    if (string == QLatin1String("sw")) {
        return DIR_SOUTHWEST;
    }
    if (string == QLatin1String("in")) {
        return DIR_IN;
    }
    if (string == QLatin1String("out")) {
        return DIR_OUT;
    }
    return DIR_OTHER;
}

bool TRoom::hasExitStub(int direction) const
{
    return (getExit(direction) == 0);
}

// This can get called whilst scanning a map file from another profile within
// TMap::retrieveMapFileStats(...) and mpRoomDB is a nullptr so check before
// dereferencing it:
void TRoom::setExitStub(int direction, bool status)
{
    bool doneSomething = false;
    const auto currentTarget = getExit(direction);
    if (status) {
        if (currentTarget == -1) {
            setExit(0, direction);
            doneSomething = true;
        } else {
            if (currentTarget > 0 && mpRoomDB) {
                QString error = QString("Set exit stub in given direction in RoomID(%1) - there is already an exit there!").arg(id);
                mpRoomDB->mpMap->logError(error);
            }
            // ELSE currentTarget == 0 so already a stub in this direction so a na-op
        }
    } else {
        if (!currentTarget) {
            // There IS a stub in this direction - so clear it
            setExit(-1, direction);
            doneSomething = true;
        }
    }

    if (doneSomething && mpRoomDB) {
        mpRoomDB->mpMap->setUnsaved(__func__);
    }
}

int TRoom::getExitWeight(const QString& cmd)
{
    if (exitWeights.contains(cmd)) {
        return exitWeights[cmd];
    } else {
        return weight; // NOTE: if no exit weight has been set: exit weight = room weight
    }
}

// NOTE: needed so dialogRoomExit code can tell if an exit weight has been set
// now that they are private!
bool TRoom::hasExitWeight(const QString& cmd)
{
    if (exitWeights.contains(cmd)) {
        if (exitWeights.value(cmd) > 0) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void TRoom::setWeight(int w)
{
    if (w < 1) {
        w = 1;
    }
    weight = w;
    mpRoomDB->mpMap->setUnsaved(__func__);
}

void TRoom::setExitWeight(const QString& cmd, int w)
{
    if (w > 0) {
        exitWeights[cmd] = w;
        mpRoomDB->mpMap->setUnsaved(__func__);
        mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
    } else if (exitWeights.contains(cmd)) {
        exitWeights.remove(cmd);
        mpRoomDB->mpMap->setUnsaved(__func__);
        mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
    }
}

// Uses lower case initials: n,ne,e,se,s,sw,w,nw
//
// also: up, down, in, out or any unprefixed special exit command
// all of which can be stored but aren't (yet?) showable on the 2D mapper
bool TRoom::setDoor(const QString& cmd, const int doorStatus)
{
    if (doorStatus > 0 && doorStatus <= 3) {
        if (doors.value(cmd, 0) != doorStatus) {
            // .value will return 0 if there ISN'T a door for this cmd
            doors[cmd] = doorStatus;
            mpRoomDB->mpMap->setUnsaved(__func__);
            return true; // As we have changed things
        }

        return false; // Valid but ineffective

    }

    if (doors.contains(cmd) && !doorStatus) {
        doors.remove(cmd);
        mpRoomDB->mpMap->setUnsaved(__func__);
        return true; // As we have changed things
    }

    return false; // As we have not changed things
}

int TRoom::getDoor(const QString& cmd) const
{
    return doors.value(cmd, 0);
    // Second argument is the result if cmd is not in the doors QMap
}

void TRoom::setId(const int roomId)
{
    id = roomId;
}

// The second optional argument delays area related recaluclations when true
// until called with false (the default) - it records the "dirty" areas so that
// the affected areas can be identified.
// The caller, should set the argument true for all but the last when working
// through a list of rooms.
// There IS a theoretical risk that if the last called room "doesn't exist" then
// the area related recalculations won't get done - so had better provide an
// alternative means to do them as a fault recovery
bool TRoom::setArea(int areaID, bool deferAreaRecalculations)
{
    static QSet<TArea*> dirtyAreas;
    TArea* pA = mpRoomDB->getArea(areaID);
    if (!pA) {
        // There is no TArea instance with that _areaID
        // So try and make it
        mpRoomDB->addArea(areaID);
        pA = mpRoomDB->getArea(areaID);
        if (!pA) { // Oh dear, THAT didn't work
            QString error = qApp->translate("TRoom", "No area created!  Requested area ID=%1. Note: Area IDs must be > 0").arg(areaID);
            mpRoomDB->mpMap->logError(error);
            return false;
        }
    }

    //remove from the old area
    TArea* pA2 = mpRoomDB->getArea(area);
    if (pA2) {
        pA2->removeRoom(id, deferAreaRecalculations);
        // Ah, all rooms in the OLD area that led to the room now become area
        // exits for that OLD area {so must run determineAreaExits() for the
        // old area after the room has moved to the new area see other
        // "if( pA2 )" below} - other exits that led to the room from other
        // areas are still "out of area exits" UNLESS the room moves to the SAME
        // area that the other exits are in.
        // Add to local store of dirty areas
        dirtyAreas.insert(pA2);
        // Flag the area itself in case something goes
        // wrong on last room in a series
        pA2->mIsDirty = true;
    } else {
        QString error = qApp->translate("TRoom", "Warning: When setting the Area for Room (Id: %1) it did not have a current area!").arg(id);
        mpRoomDB->mpMap->logError(error);
    }

    area = areaID;
    pA->addRoom(id);

    dirtyAreas.insert(pA);
    pA->mIsDirty = true;

    if (!deferAreaRecalculations) {
        QSetIterator<TArea*> itpArea = dirtyAreas;
        while (itpArea.hasNext()) {
            TArea* pArea = itpArea.next();
            pArea->clean();
        }
        dirtyAreas.clear();
    }

    mpRoomDB->mpMap->setUnsaved(__func__);
    return true;
}

// This can get called whilst scanning a map file from another profile within
// TMap::retrieveMapFileStats(...) and mpRoomDB is a nullptr so check before
// dereferencing it:
bool TRoom::setExit(const int to, const int direction)
{
    switch (direction) {
    case DIR_NORTH:     north     = to; break;
    case DIR_NORTHEAST: northeast = to; break;
    case DIR_NORTHWEST: northwest = to; break;
    case DIR_EAST:      east      = to; break;
    case DIR_WEST:      west      = to; break;
    case DIR_SOUTH:     south     = to; break;
    case DIR_SOUTHEAST: southeast = to; break;
    case DIR_SOUTHWEST: southwest = to; break;
    case DIR_UP:        up        = to; break;
    case DIR_DOWN:      down      = to; break;
    case DIR_IN:        in        = to; break;
    case DIR_OUT:       out       = to; break;
    default:
        return false;
    }
    if (mpRoomDB) {
        mpRoomDB->updateEntranceMap(this);
        mpRoomDB->mpMap->setUnsaved(__func__);
    }
    return true;
}

bool TRoom::hasExit(const int direction) const
{
    switch (direction) {
    case DIR_NORTH:     if (north     != -1) { return true; } break;
    case DIR_NORTHEAST: if (northeast != -1) { return true; } break;
    case DIR_NORTHWEST: if (northwest != -1) { return true; } break;
    case DIR_EAST:      if (east      != -1) { return true; } break;
    case DIR_WEST:      if (west      != -1) { return true; } break;
    case DIR_SOUTH:     if (south     != -1) { return true; } break;
    case DIR_SOUTHEAST: if (southeast != -1) { return true; } break;
    case DIR_SOUTHWEST: if (southwest != -1) { return true; } break;
    case DIR_UP:        if (up        != -1) { return true; } break;
    case DIR_DOWN:      if (down      != -1) { return true; } break;
    case DIR_IN:        if (in        != -1) { return true; } break;
    case DIR_OUT:       if (out       != -1) { return true; } break;
    }
    return false;
}

// Confirms whether the text identifies a valid exit, given the context in which
// it is called it does not have to worry about the casing of "text" as that has
// already been handled in the current usages:
bool TRoom::hasExitOrSpecialExit(const QString& text) const
{
    // First check the normal ones:
    if (text == QLatin1String("n")) {
        return hasExit(DIR_NORTH);
    } else if (text == QLatin1String("ne")) {
        return hasExit(DIR_NORTHEAST);
    } else if (text == QLatin1String("e")) {
        return hasExit(DIR_EAST);
    } else if (text == QLatin1String("se")) {
        return hasExit(DIR_SOUTHEAST);
    } else if (text == QLatin1String("s")) {
        return hasExit(DIR_SOUTH);
    } else if (text == QLatin1String("sw")) {
        return hasExit(DIR_SOUTHWEST);
    } else if (text == QLatin1String("w")) {
        return hasExit(DIR_WEST);
    } else if (text == QLatin1String("nw")) {
        return hasExit(DIR_NORTHWEST);
    } else if (text == QLatin1String("up")) {
        return hasExit(DIR_UP);
    } else if (text == QLatin1String("down")) {
        return hasExit(DIR_DOWN);
    } else if (text == QLatin1String("in")) {
        return hasExit(DIR_IN);
    } else if (text == QLatin1String("out")) {
        return hasExit(DIR_OUT);
    } else {
        // Then check the special exits:
        return mSpecialExits.contains(text);
    }

    return false;
}

int TRoom::getExit(const int direction) const
{
    switch (direction) {
    case DIR_NORTH:
        return north;
    case DIR_NORTHEAST:
        return northeast;
    case DIR_NORTHWEST:
        return northwest;
    case DIR_EAST:
        return east;
    case DIR_WEST:
        return west;
    case DIR_SOUTH:
        return south;
    case DIR_SOUTHEAST:
        return southeast;
    case DIR_SOUTHWEST:
        return southwest;
    case DIR_UP:
        return up;
    case DIR_DOWN:
        return down;
    case DIR_IN:
        return in;
    case DIR_OUT:
        return out;
    }
    return -1;
}

QHash<int, int> TRoom::getExits() const
{
    // key is room id we exit to, value is type of exit. 0 is normal, 1 is special
    QHash<int, int> exitList;
    if (north != -1) {
        exitList[north] = 0;
    }
    if (northeast != -1) {
        exitList[northeast] = 0;
    }
    if (east != -1) {
        exitList[east] = 0;
    }
    if (southeast != -1) {
        exitList[southeast] = 0;
    }
    if (south != -1) {
        exitList[south] = 0;
    }
    if (southwest != -1) {
        exitList[southwest] = 0;
    }
    if (west != -1) {
        exitList[west] = 0;
    }
    if (northwest != -1) {
        exitList[northwest] = 0;
    }
    if (up != -1) {
        exitList[up] = 0;
    }
    if (down != -1) {
        exitList[down] = 0;
    }
    if (in != -1) {
        exitList[in] = 0;
    }
    if (out != -1) {
        exitList[out] = 0;
    }
    QMapIterator<QString, int> it(mSpecialExits);
    while (it.hasNext()) {
        it.next();
        exitList[it.value()] = 1;
    }
    return exitList;
}

void TRoom::setExitLock(int exit, bool state)
{
    if (state) {
        if ((!exitLocks.contains(exit)) && (exit >= DIR_NORTH && exit <= DIR_OUT)) {
            exitLocks.push_back(exit);
        }
    } else {
        exitLocks.removeAll(exit);
    }
    mpRoomDB->mpMap->setUnsaved(__func__);
}

bool TRoom::setSpecialExitLock(const QString& cmd, const bool doLock)
{
    if (!mSpecialExits.contains(cmd)) {
        return false;
    }

    if (doLock) {
        mSpecialExitLocks.insert(cmd);
    } else {
        mSpecialExitLocks.remove(cmd);
    }

    mpRoomDB->mpMap->setUnsaved(__func__);
    return true;
}

bool TRoom::hasExitLock(const int dir) const
{
    return exitLocks.contains(dir);
}

bool TRoom::hasSpecialExitLock(const QString& cmd) const
{
    return mSpecialExitLocks.contains(cmd);
}

// Original addSpecialExit...() code had limitation that it used the "to" room
// as part of the things to look for to identify a particular special exit
// indeed the use of the "to" room as the key for the "other" exit map does seem
// a poorer choice than the "command" which is currently the value item...
// Changing to setSpecialExit(), "to" values less than 1 remove exit...
void TRoom::setSpecialExit(const int to, const QString& cmd)
{
    if (cmd.isEmpty()) {
        return;
    }

    auto currentTo = mSpecialExits.value(cmd, -1);
    bool doneSomething = false;
    if (to >= 0) {
        mSpecialExits[cmd] = to;
        if (currentTo != to) {
            doneSomething = true;
        }

    } else {
        // to is less than 1 - but use a value clamped to just two alternatives:
        auto constrainedTo = std::max(-1, to);
        if (constrainedTo != currentTo) {
            // and it doesn't match the existing one when confined to the range
            // of just the two values -1 (no exit) and 0 (a stub)
            if (!constrainedTo) {
                // So add it in if we now want a stub exit:
                mSpecialExits[cmd] = constrainedTo;
            }
            doneSomething = true;
        }
        // Clean up related data if the exit has been deleted:
        if (constrainedTo == -1) {
            customLinesArrow.remove(cmd);
            customLinesColor.remove(cmd);
            customLinesStyle.remove(cmd);
            customLines.remove(cmd);
            exitWeights.remove(cmd);
            doors.remove(cmd);
            mSpecialExitLocks.remove(cmd);
            mSpecialExits.remove(cmd);
        }
    }

    if (doneSomething) {
        TArea* pA = mpRoomDB->getArea(area);
        if (pA) {
            pA->determineAreaExitsOfRoom(id);
            // This updates the (TArea *)->exits map even for exit REMOVALS
        }
        mpRoomDB->updateEntranceMap(this);
        mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
        mpRoomDB->mpMap->setUnsaved(__func__);
    }
}

void TRoom::clearSpecialExits()
{
    if (mSpecialExits.isEmpty()) {
        return;
    }

    QMutableMapIterator<QString, int> itSpecialExit(mSpecialExits);
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        // Clean up related elements first:
        mSpecialExitLocks.remove(itSpecialExit.key());
        doors.remove(itSpecialExit.key());
        customLines.remove(itSpecialExit.key());
        customLinesColor.remove(itSpecialExit.key());
        customLinesStyle.remove(itSpecialExit.key());
        customLinesArrow.remove(itSpecialExit.key());
        // Then remove the exit itself from the QMap:
        itSpecialExit.remove();
    }
    mpRoomDB->updateEntranceMap(this);
    mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
    mpRoomDB->mpMap->setUnsaved(__func__);
}

void TRoom::removeAllSpecialExitsToRoom(const int roomId)
{
    QMutableMapIterator<QString, int> itSpecialExit(mSpecialExits);
    bool exitFound = false;
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        if (itSpecialExit.value() != roomId) {
            continue;
        }

        exitFound = true;
        // Clean up related elements first:
        mSpecialExitLocks.remove(itSpecialExit.key());
        doors.remove(itSpecialExit.key());
        customLines.remove(itSpecialExit.key());
        customLinesColor.remove(itSpecialExit.key());
        customLinesStyle.remove(itSpecialExit.key());
        customLinesArrow.remove(itSpecialExit.key());
        // Then remove the exit itself from the QMap:
        itSpecialExit.remove();
    }

    if (exitFound) {
        TArea* pA = mpRoomDB->getArea(area);
        if (pA) {
            pA->determineAreaExitsOfRoom(id);
        }
        mpRoomDB->updateEntranceMap(this);
        mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
        mpRoomDB->mpMap->setUnsaved(__func__);
    }
}

void TRoom::calcRoomDimensions()
{
    min_x = mX;
    max_x = mX;
    min_y = mY;
    max_y = mY;

    if (customLines.empty()) {
        return;
    }

    QMapIterator<QString, QList<QPointF>> it(customLines);
    while (it.hasNext()) {
        it.next();
        const QList<QPointF>& pointsInLine = it.value();
        if (pointsInLine.empty()) {
            continue;
        }
        for (auto pointInLine : pointsInLine) {
            const qreal pointX = pointInLine.x();
            const qreal pointY = pointInLine.y();
            if (pointX < min_x) {
                min_x = pointX;
            }
            if (pointX > max_x) {
                max_x = pointX;
            }
            if (pointY < min_y) {
                min_y = pointY;
            }
            if (pointY > max_y) {
                max_y = pointY;
            }
        }
    }
}

// This can get called whilst scanning a map file from another profile within
// TMap::retrieveMapFileStats(...) and mpRoomDB is a nullptr so check before
// dereferencing it:
void TRoom::restore(QDataStream& ifs, int roomID, int version)
{
    id = roomID;
#if defined(DEBUG_MAP_FILE_PROCESSING) && (DEBUG_MAP_FILE_PROCESSING > 1)
    qDebug().noquote().nospace() << "TRoom::restore(...) INFO - about to read room (ID: " << roomID << ") data at: " << ifs.device()->pos();
#endif
    ifs >> area;
    // Can be useful when analysing suspect map files!
    //     qDebug() << "TRoom::restore(...," << roomID << ",...) has AreaId:" << area;
    ifs >> mX;
    ifs >> mY;
    ifs >> mZ;
    // For versions >= 21 these can contain 0 for stub exits, they will be -1
    // in the absence of an exit for those and previous versions:
    ifs >> north;
    ifs >> northeast;
    ifs >> east;
    ifs >> southeast;
    ifs >> south;
    ifs >> southwest;
    ifs >> west;
    ifs >> northwest;
    ifs >> up;
    ifs >> down;
    ifs >> in;
    ifs >> out;
    ifs >> environment;
    ifs >> weight;

    // force room weight >= 1 otherwise pathfinding chooses random paths.
    if (weight < 1) {
        weight = 1;
    }

    if (version < 8) {
        float f1, f2, f3, f4;
        ifs >> f1; //rooms[i]->xRot;
        ifs >> f2; //rooms[i]->yRot;
        ifs >> f3; //rooms[i]->zRot;
        ifs >> f4; //rooms[i]->zoom;
    }
    ifs >> name;
#if defined(DEBUG_MAP_FILE_PROCESSING) && (DEBUG_MAP_FILE_PROCESSING > 2)
    qDebug().noquote().nospace() << "TRoom::restore(...) INFO - read room name (\"" << name << "\") at: " << ifs.device()->pos();
#endif
    ifs >> isLocked;
    if (version >= 21) {
        ifs >> mSpecialExits;
    } else if (version >= 6) {
        // Before version 21 the special exits were stored as a QMultiMap<int, QString>
        // with the lock information prepended as a '1' locked or '0' unlocked
        // from version 11-ish - unfortunetly this made it impossible to use
        // to store multiple stubs all with the same (zero) destination room,
        // to allow them to be retained their details will be converted into
        // strings and saved as room user data - but this will not be processed
        // by Mudlet versions prior to the code for this being introduced
        // expected to be 4.19.1:
        QMultiMap<int, QString> oldSpecialExits;
        ifs >> oldSpecialExits;
        QMultiMapIterator<int, QString> itOldSpecialExit(oldSpecialExits);
        while (itOldSpecialExit.hasNext()) {
            itOldSpecialExit.next();
            const QString cmd{itOldSpecialExit.value()};
            if (cmd.startsWith(QLatin1String("1"))) {
                // Is locked:
                mSpecialExits.insert(cmd.mid(1), itOldSpecialExit.key());
                mSpecialExitLocks.insert(cmd.mid(1));
            } else if (Q_LIKELY(cmd.startsWith(QLatin1String("0")))) {
                // Is not locked:
                mSpecialExits.insert(cmd.mid(1), itOldSpecialExit.key());
            } else {
                // Has no lock prefix at all
                mSpecialExits.insert(cmd, itOldSpecialExit.key());
            }
        }
    }

    qint8 oldCharacterCode = 0;
    if (version >= 19) {
        // From version 19 we use a QString for one or more (Unicode) Graphemes
        ifs >> mSymbol;
    } else if (version >= 9 && version < 19) {
        // For older versions we note the prior unsigned short in case
        // there is no fallback carried in the room user data
        ifs >> oldCharacterCode;
    }

    if (version >= 21) {
        ifs >> mSymbolColor;
    }

    if (version >= 10) {
        ifs >> userData;
#if defined(DEBUG_MAP_FILE_PROCESSING) && (DEBUG_MAP_FILE_PROCESSING > 2)
        if (!userData.isEmpty()) {
            qDebug().noquote().nospace() << "TRoom::restore(...) INFO - read some room user data at: " << ifs.device()->pos();
        }
#endif
        if (version < 19) {
            // Recover and remove backup values from the user data
            const QString symbolString = userData.take(QLatin1String("system.fallback_symbol"));
            if (!symbolString.isEmpty()) {
                // There is a fallback in the user data
                mSymbol = symbolString;
            } else if (oldCharacterCode > 32) {
                // There is an old format unsigned short represeting a printable
                // ASCII or ISO 8859-1 (Latin1) character:
                mSymbol = QChar(oldCharacterCode);
            }
        }
    }

    if (version < 21) {
        auto symbolColorFallbackKey = QLatin1String("system.fallback_symbol_color");
        if (userData.contains(symbolColorFallbackKey)) {
            mSymbolColor = QColor(userData.take(symbolColorFallbackKey));
        }
    }

    if (version >= 11) {
        if (version >= 20) {
            // In version 20 we stopped storing a QString form for the line
            // style - and we also switched to lower case for the normal exit
            // direction keys and using a QColor to store the color.
            // In version 21 we added custom lines to stubs AND we allowed
            // for special stub exits - this will recover all of those for that
            // version but for version 20 we will need to retrieve the stub
            // custom lines details from the room user data:
            ifs >> customLines;
            ifs >> customLinesArrow;
            ifs >> customLinesColor;
            ifs >> customLinesStyle;
#if defined(DEBUG_MAP_FILE_PROCESSING) && (DEBUG_MAP_FILE_PROCESSING > 2)
            qDebug().noquote().nospace() << "TRoom::restore(...) INFO - read custom line data at: " << ifs.device()->pos();
#endif
        } else {
            QMap<QString, QList<QPointF>> oldLinesData;
            ifs >> oldLinesData;
            QMapIterator<QString, QList<QPointF>> itCustomLine(oldLinesData);
            while (itCustomLine.hasNext()) {
                itCustomLine.next();
                const QString direction(itCustomLine.key());
                if (direction == QLatin1String("N") || direction == QLatin1String("E") || direction == QLatin1String("S") || direction == QLatin1String("W") || direction == QLatin1String("UP")
                    || direction == QLatin1String("DOWN")
                    || direction == QLatin1String("NE")
                    || direction == QLatin1String("SE")
                    || direction == QLatin1String("SW")
                    || direction == QLatin1String("NW")
                    || direction == QLatin1String("IN")
                    || direction == QLatin1String("OUT")) {
                    customLines.insert(itCustomLine.key().toLower(), itCustomLine.value());
                } else {
                    customLines.insert(itCustomLine.key(), itCustomLine.value());
                }
            }

            QMap<QString, bool> oldLinesArrowData;
            ifs >> oldLinesArrowData;
            QMapIterator<QString, bool> itCustomLineArrow(oldLinesArrowData);
            while (itCustomLineArrow.hasNext()) {
                itCustomLineArrow.next();
                const QString direction(itCustomLineArrow.key());
                if (direction == QLatin1String("N") || direction == QLatin1String("E") || direction == QLatin1String("S") || direction == QLatin1String("W") || direction == QLatin1String("UP")
                    || direction == QLatin1String("DOWN")
                    || direction == QLatin1String("NE")
                    || direction == QLatin1String("SE")
                    || direction == QLatin1String("SW")
                    || direction == QLatin1String("NW")
                    || direction == QLatin1String("IN")
                    || direction == QLatin1String("OUT")) {
                    customLinesArrow.insert(itCustomLineArrow.key().toLower(), itCustomLineArrow.value());
                } else {
                    customLinesArrow.insert(itCustomLineArrow.key(), itCustomLineArrow.value());
                }
            }

            QMap<QString, QList<int>> oldLinesColorData;
            ifs >> oldLinesColorData;
            QMapIterator<QString, QList<int>> itCustomLineColor(oldLinesColorData);
            while (itCustomLineColor.hasNext()) {
                itCustomLineColor.next();
                const QString direction(itCustomLineColor.key());
                if (direction == QLatin1String("N") || direction == QLatin1String("E") || direction == QLatin1String("S") || direction == QLatin1String("W") || direction == QLatin1String("UP")
                    || direction == QLatin1String("DOWN")
                    || direction == QLatin1String("NE")
                    || direction == QLatin1String("SE")
                    || direction == QLatin1String("SW")
                    || direction == QLatin1String("NW")
                    || direction == QLatin1String("IN")
                    || direction == QLatin1String("OUT")) {

                    // Fixup broken custom lines caused by maps saved prior to
                    // https://github.com/Mudlet/Mudlet/pull/2559 going into the
                    // code by only adding the value if it contains enough
                    // colour components to be a valid colour:
                    if (itCustomLineColor.value().count() > 2) {
                        customLinesColor.insert(itCustomLineColor.key().toLower(), QColor(itCustomLineColor.value().at(0), itCustomLineColor.value().at(1), itCustomLineColor.value().at(2)));
                    }
                    // Otherwise we will fixup both empty
                    // itCustomLineColor.value() entities AND altogether missing
                    // ones outside of the while() {...}:
                } else {
                    if (itCustomLineColor.value().count() > 2) {
                        customLinesColor.insert(itCustomLineColor.key(), QColor(itCustomLineColor.value().at(0), itCustomLineColor.value().at(1), itCustomLineColor.value().at(2)));
                    }
                }
            }

            // Create new (RED) colour customLinesColor entities for any custom
            // exit lines that does not have one or has an empty one as a result
            // of https://github.com/Mudlet/Mudlet/issues/2558 - use a QSet
            // rather than a QList when manipulating the present/absent
            // direction keys as there won't be any duplicate keys and the
            // operation we want to perform (obtain the directions of all custom
            // exit lines and remove those which are already included in the
            // colours) is much easier to perform on a QSet rather than a QList:
            auto customLineKeys = customLines.keys();
            QSet<QString> missingKeys{customLineKeys.begin(), customLineKeys.end()};
            if (!customLinesColor.isEmpty()) {
                auto customLinesColorKeys = customLinesColor.keys();
                QSet<QString> const customLinesColorKeysSet{customLinesColorKeys.begin(), customLinesColorKeys.end()};
                missingKeys.subtract(customLinesColorKeysSet);
            }
            QSetIterator<QString> itMissingCustomLineColourKey(missingKeys);
            while (itMissingCustomLineColourKey.hasNext()) {
                customLinesColor.insert(itMissingCustomLineColourKey.next(), QColor(Qt::red));
            }

            QMap<QString, QString> oldLineStyleData;
            ifs >> oldLineStyleData;
            QMapIterator<QString, QString> itCustomLineStyle(oldLineStyleData);
            while (itCustomLineStyle.hasNext()) {
                itCustomLineStyle.next();
                QString direction{itCustomLineStyle.key()};
                if (direction == QLatin1String("N") || direction == QLatin1String("E") || direction == QLatin1String("S") || direction == QLatin1String("W") || direction == QLatin1String("UP")
                    || direction == QLatin1String("DOWN")
                    || direction == QLatin1String("NE")
                    || direction == QLatin1String("SE")
                    || direction == QLatin1String("SW")
                    || direction == QLatin1String("NW")
                    || direction == QLatin1String("IN")
                    || direction == QLatin1String("OUT")) {
                    direction = direction.toLower();
                }

                if (itCustomLineStyle.value() == QLatin1String("dot line")) {
                    customLinesStyle.insert(direction, Qt::DotLine);
                } else if (itCustomLineStyle.value() == QLatin1String("dash line")) {
                    customLinesStyle.insert(direction, Qt::DashLine);
                } else if (itCustomLineStyle.value() == QLatin1String("dash dot line")) {
                    customLinesStyle.insert(direction, Qt::DashDotLine);
                } else if (itCustomLineStyle.value() == QLatin1String("dash dot dot line")) {
                    customLinesStyle.insert(direction, Qt::DashDotDotLine);
                } else {
                    customLinesStyle.insert(direction, Qt::SolidLine);
                }
            }
#if defined(DEBUG_MAP_FILE_PROCESSING) && (DEBUG_MAP_FILE_PROCESSING > 2)
            qDebug().noquote().nospace() << "TRoom::restore(...) INFO - read custom line data at: " << ifs.device()->pos();
#endif
        }
        if (version >= 21) {
            ifs >> mSpecialExitLocks;
        }
        ifs >> exitLocks;
#if defined(DEBUG_MAP_FILE_PROCESSING) && (DEBUG_MAP_FILE_PROCESSING > 2)
        if (!exitLocks.isEmpty()) {
            qDebug().noquote().nospace() << "TRoom::restore(...) INFO - read exit lock data at: " << ifs.device()->pos();
        }
#endif
    }
    if (version >= 13 && version <= 20) {
        // Prior to version 21 only normal exit stubs were supported and stored
        // as a separate element:
        QList<int> exitStubs;
        ifs >> exitStubs;
        QListIterator<int> itStub(exitStubs);
        while (itStub.hasNext()) {
            setExitStub(itStub.next(), true);
        }
    }
    if (version >= 16) {
        ifs >> exitWeights;
        ifs >> doors;
    }
    if (version <= 20) {
        // Custom exit lines for Special AND normal exit stubs were stored
        // in fallback locations in the user data - recover and remove those
        // backup values from that user data - first do special exits with keys
        // beginning with "system.fallback_specialStubExit.#." where # is a
        // monotonically increasing series of numbers starting at 0:
        int index = 0;
        QString name = userData.take(qsl("system.fallback_specialStubExit.%1.name").arg(QString::number(index)));
        while (!name.isNull()) {
            mSpecialExits.insert(name, 0);

            const int door = userData.take(qsl("system.fallback_specialStubExit.%1.door")
                                                   .arg(QString::number(index))).toInt();
            if (door) {
                doors.insert(name, door);
            }

            const QStringList pointsStringList = userData.take(qsl("system.fallback_specialStubExit.%1.customLine.points")
                                                                       .arg(QString::number(index)))
                                                         .split(QLatin1Char(';'), Qt::SkipEmptyParts);

            const QString style = userData.take(qsl("system.fallback_specialStubExit.%1.customLine.style")
                                                        .arg(QString::number(index)));

            const QStringList colorStringList = userData.take(qsl("system.fallback_specialStubExit.%1.customLine.color")
                                                                      .arg(QString::number(index)))
                                                        .split(QLatin1Char(','), Qt::SkipEmptyParts);

            const bool hasArrow = !userData.take(qsl("system.fallback_specialStubExit.%1.customLine.arrow")
                                                         .arg(QString::number(index)))
                                           .compare(QLatin1String("true"));

            if (!pointsStringList.isEmpty()) {
                QList<QPointF> points;
                for (int pointIndex = 0, total = pointsStringList.count(); pointIndex < total; ++pointIndex) {
                    QStringList pointCoordinates = pointsStringList.at(pointIndex).split(QLatin1Char(','));
                    if (pointCoordinates.count() > 1) {
                        bool xOkay = false;
                        bool yOkay = false;
                        QPointF point = QPointF(pointCoordinates.at(0).toDouble(&xOkay),
                                                pointCoordinates.at(1).toDouble(&yOkay));
                        if (xOkay && yOkay) {
                            points.append(point);
                        }
                    }
                }

                customLines.insert(name, points);

                if (!style.compare(QLatin1String("dot line"))) {
                    customLinesStyle.insert(name, Qt::DotLine);
                } else if (!style.compare(QLatin1String("dash line"))) {
                    customLinesStyle.insert(name, Qt::DashLine);
                } else if (!style.compare(QLatin1String("dash dot line"))) {
                    customLinesStyle.insert(name, Qt::DashDotLine);
                } else if (!style.compare(QLatin1String("dash dot dot line"))) {
                    customLinesStyle.insert(name, Qt::DashDotDotLine);
                } else {
                    customLinesStyle.insert(name, Qt::SolidLine);
                }

                QColor lineColor;
                if (colorStringList.count() > 2) {
                    lineColor = QColor(colorStringList.at(0).toInt(),
                                       colorStringList.at(1).toInt(),
                                       colorStringList.at(2).toInt());
                }

                customLinesColor.insert(name, lineColor.isValid() ? lineColor : QColorConstants::Red);

                customLinesArrow.insert(name, hasArrow);

            }
            name = userData.take(qsl("system.fallback_specialStubExit.%1.name").arg(QString::number(++index)));
        }

        // Now do the same for any normal stub exits:
        for (int dirCode = DIR_NORTH; dirCode < DIR_OTHER; ++dirCode) {
            const auto door = userData.take(qsl("system.fallback_normalStubExit.%1.door")
                                                   .arg(QString::number(dirCode))).toInt();
            const auto direction = dirCodeToShortString(dirCode);
            if (door) {
                doors.insert(direction, door);
            }

            const QStringList pointsStringList = userData.take(qsl("system.fallback_normalStubExit.%1.customLine.points")
                                                                       .arg(QString::number(dirCode)))
                                                         .split(QLatin1Char(';'), Qt::SkipEmptyParts);


            const QString style = userData.take(qsl("system.fallback_normalStubExit.%1.customLine.style")
                                                        .arg(QString::number(dirCode)));

            const QStringList colorStringList = userData.take(qsl("system.fallback_normalStubExit.%1.customLine.color")
                                                                      .arg(QString::number(dirCode)))
                                                        .split(QLatin1Char(','), Qt::SkipEmptyParts);

            const bool hasArrow = !userData.take(qsl("system.fallback_normalStubExit.%1.customLine.arrow")
                                                         .arg(QString::number(dirCode)))
                                           .compare(QLatin1String("true"));

            if (pointsStringList.size()) {
                // We have some points so we have a custom normal stub exit
                QList<QPointF> points;
                for (int pointIndex = 0, total = pointsStringList.count(); pointIndex < total; ++pointIndex) {
                    QStringList pointCoordinates = pointsStringList.at(pointIndex).split(QLatin1Char(','));
                    if (pointCoordinates.count() > 1) {
                        bool xOkay = false;
                        bool yOkay = false;
                        QPointF point = QPointF(pointCoordinates.at(0).toDouble(&xOkay),
                                                pointCoordinates.at(1).toDouble(&yOkay));
                        if (xOkay && yOkay) {
                            points.append(point);
                        }
                    }
                }

                customLines.insert(direction, points);

                if (!style.compare(QLatin1String("dot line"))) {
                    customLinesStyle.insert(direction, Qt::DotLine);
                } else if (!style.compare(QLatin1String("dash line"))) {
                    customLinesStyle.insert(direction, Qt::DashLine);
                } else if (!style.compare(QLatin1String("dash dot line"))) {
                    customLinesStyle.insert(direction, Qt::DashDotLine);
                } else if (!style.compare(QLatin1String("dash dot dot line"))) {
                    customLinesStyle.insert(direction, Qt::DashDotDotLine);
                } else {
                    customLinesStyle.insert(direction, Qt::SolidLine);
                }

                QColor lineColor;
                if (colorStringList.count() > 2) {
                    lineColor = QColor(colorStringList.at(0).toInt(),
                                       colorStringList.at(1).toInt(),
                                       colorStringList.at(2).toInt());
                }

                customLinesColor.insert(direction, lineColor.isValid() ? lineColor : QColorConstants::Red);

                customLinesArrow.insert(direction, hasArrow);
            }
        }
    }

    calcRoomDimensions();
}

void TRoom::audit(const QHash<int, int> roomRemapping, const QHash<int, int> areaRemapping)
{
    if (areaRemapping.contains(area)) {
        userData.insert(qsl("audit.remapped_area"), QString::number(area));
        area = areaRemapping.value(area);
    }

    auditExits(roomRemapping);
}

void TRoom::auditExits(const QHash<int, int> roomRemapping)
{
    // Clone all the structures into working copies that we can eliminate valid
    // members from to identify any rogue members before removing them:

    QMap<QString, int> exitWeightsCopy = exitWeights;
    QSet<int> exitLocksCopy{exitLocks.begin(), exitLocks.end()};
    QSet<QString> specialExitLockCopy = mSpecialExitLocks;
    QMap<QString, int> doorsCopy = doors;
    QMap<QString, QList<QPointF>> customLinesCopy = customLines;
    QMap<QString, QColor> customLinesColorCopy = customLinesColor;
    QMap<QString, Qt::PenStyle> customLinesStyleCopy = customLinesStyle;
    QMap<QString, bool> customLinesArrowCopy = customLinesArrow;

    exitWeightsCopy.detach(); // Make deep copies now, this will happen anyhow once we start to remove valid members
    exitLocksCopy.detach();
    specialExitLockCopy.detach();
    doorsCopy.detach();
    customLinesCopy.detach();
    customLinesColorCopy.detach();
    customLinesStyleCopy.detach();
    customLinesArrowCopy.detach();

    auditExit(north,
              DIR_NORTH,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(northeast,
              DIR_NORTHEAST,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(northwest,
              DIR_NORTHWEST,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(south,
              DIR_SOUTH,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(southeast,
              DIR_SOUTHEAST,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(southwest,
              DIR_SOUTHWEST,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(east,
              DIR_EAST,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(west,
              DIR_WEST,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(up,
              DIR_UP,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(down,
              DIR_DOWN,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(in,
              DIR_IN,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(out,
              DIR_OUT,
              exitWeightsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    // If we use the Mutable iterator we don't have to restart after a deletion
    { // Block code to limit scope of iterator
        QMutableMapIterator<QString, int> it(mSpecialExits);
        while (it.hasNext()) {
            it.next();
            const QString exitName = it.key();
            const int exitRoomId = it.value();
            if (exitName.isEmpty()) {
                if (mudlet::self()->showMapAuditErrors()) {
                    const QString warnMsg = tr("[ WARN ]  - In room id: %1 removing invalid (special) exit to %2 {with no name!}")
                                                    .arg(QString::number(id), QString::number(exitRoomId));
                    // If size is less than or equal to 0 then there is nothing to print!!!
                    mpRoomDB->mpMap->postMessage(warnMsg);
                }
                mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                                    tr("[ WARN ]  - Room had an invalid (special) exit to %1 {with no name!} it was removed.")
                                                            .arg(QString::number(exitRoomId)));
                it.remove();
                continue;
            }

            if (roomRemapping.contains(exitRoomId)) {
                const QString auditKey = qsl("audit.remapped_special_exit.%1").arg(exitName);
                userData.insert(auditKey, QString::number(exitRoomId));
                if (mudlet::self()->showMapAuditErrors()) {
                    const QString infoMsg = tr("[ INFO ]  - In room with id: %1 correcting special exit \"%2\" that\n"
                                                           "was to room with an exit to invalid room: %3 to now go\n"
                                                           "to: %4.")
                                                    .arg(QString::number(id), exitName,
                                                         QString::number(exitRoomId),
                                                         QString::number(roomRemapping.value(exitRoomId)));
                    mpRoomDB->mpMap->postMessage(infoMsg);
                }
                mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                                    tr(R"([ INFO ]  - Room needed correcting of special exit "%1" that was to room with an exit to invalid room: %2 to now go to: %3.)")
                                                            .arg(exitName,
                                                                 QString::number(exitRoomId),
                                                                 QString::number(roomRemapping.value(exitRoomId))));
                it.setValue(roomRemapping.value(exitRoomId));
            }
        }
    }

    {
        // Now check for the validity of the special exit room destinations after
        // remapping - and clean up any exit elements related to the invalid or
        // missing ones
        QMutableMapIterator<QString, int> it(mSpecialExits);
        while (it.hasNext()) {
            it.next();
            const int exitRoomId = it.value();
            const QString exitName = it.key();

            if (exitRoomId > 0) {
                // A real exit - should have a real destination
                if (Q_UNLIKELY(!mpRoomDB->getRoom(exitRoomId))) {
                    // But it doesn't exist
                    const QString auditKey = qsl("audit.convert_valid_but_missing_special_exit_to_stub.%1").arg(exitName);
                    if (mudlet::self()->showMapAuditErrors()) {
                        const QString warnMsg = tr("[ WARN ]  - Room with id: %1 has a special exit \"%2\" with an\n"
                                                               "exit to: %3 but that room does not exist.  The exit will\n"
                                                               "be converted to a stub (but the destination room id will be stored\n"
                                                               "in the room user data under a key:\n"
                                                               "\"%4\").")
                                                        .arg(QString::number(id),
                                                             exitName,
                                                             QString::number(exitRoomId),
                                                             auditKey);
                        mpRoomDB->mpMap->postMessage(warnMsg);
                    }
                    mpRoomDB->mpMap->appendRoomErrorMsg(
                            id,
                            tr(R"([ WARN ]  - Room has a special exit "%1" with an exit to: %2 but that room does not exist.  The exit will be converted to a stub (but the destination room id will be stored in the room user data under a key: "%3").)")
                                    .arg(exitName, QString::number(exitRoomId), auditKey),
                            true);
                    userData.insert(auditKey, QString::number(exitRoomId));
                    // Now we can use 0 to represent a stub we no longer use
                    // it.remove() but instead revise the value:
                    it.setValue(0);

                    // Remove the things that a special exit stub cannot have:
                    exitWeights.remove(exitName);
                    mSpecialExitLocks.remove(exitName);

                } else {
                    // Nothing special to do for a real special exit
                }

            } else if (!exitRoomId) {
                // A special stub exit which now can have a door or a custom
                // exit line but not a lock or a weight:
                exitWeights.remove(exitName);
                mSpecialExitLocks.remove(exitName);

            } else {
                // < 0 and not renumbered because the bad room Id DID NOT exist
                const QString auditKey = qsl("audit.removed_invalid_special_exit.%1").arg(exitName);
                userData.insert(auditKey, QString::number(exitRoomId));
                if (mudlet::self()->showMapAuditErrors()) {
                    const QString infoMsg = tr("[ INFO ]  - In room with id: %1 special exit \"%2\"\n"
                                                           "that was to room with an invalid room: %3 that does not exist.\n"
                                                           "The exit will be converted to a stub (the bad destination room id will be\n"
                                                           "stored in the room user data under a key:\n"
                                                           "\"%4\").")
                                                    .arg(QString::number(id),
                                                         exitName,
                                                         QString::number(exitRoomId),
                                                         auditKey);
                    mpRoomDB->mpMap->postMessage(infoMsg);
                }
                mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                                    tr("[ INFO ]  - Room had special exit \"%1\" that was to an invalid room: %2 that does not exist.  The exit will be converted to a stub (the bad destination room id will be stored in the room user data under a key: \"%3\").")
                                                            .arg(exitName,
                                                                 QString::number(exitRoomId),
                                                                 auditKey),
                                                    true);
                // We don't it.remove() now - instead change the value to zero
                it.setValue(0);

                // We cannot have a lock or a weight on a special stub exit
                exitWeights.remove(exitName);
                mSpecialExitLocks.remove(exitName);

            }

            // Remove the corresponding things from the pools of things
            // that have to be checked:
            exitWeightsCopy.remove(exitName);
            specialExitLockCopy.remove(exitName);
            doorsCopy.remove(exitName);
            customLinesCopy.remove(exitName);
            customLinesColorCopy.remove(exitName);
            customLinesStyleCopy.remove(exitName);
            customLinesArrowCopy.remove(exitName);
        }
    }

    // Finally check for any left over exit elements that should not be there:
    // Doors:
    if (!doorsCopy.isEmpty()) {
        QStringList extras;
        QMapIterator<QString, int> itSpareDoors(doorsCopy);
        while (itSpareDoors.hasNext()) {
            itSpareDoors.next();
            doors.remove(itSpareDoors.key());
            switch (itSpareDoors.value()) {
            case 0:
                extras.append(tr("%1 {none}").arg(itSpareDoors.key()));
                break;
            case 1:
                extras.append(tr("%1 (open)").arg(itSpareDoors.key()));
                break;
            case 2:
                extras.append(tr("%1 (closed)").arg(itSpareDoors.key()));
                break;
            case 3:
                extras.append(tr("%1 (locked)").arg(itSpareDoors.key()));
                break;
            default:
                extras.append(tr("%1 {invalid}").arg(itSpareDoors.key()));
            }
        }
        if (mudlet::self()->showMapAuditErrors()) {
            const QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus door items that were removed:\n"
                                                   "%2.")
                                      .arg(QString::number(id),
                                                 extras.join(QLatin1String(", ")));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr("[ INFO ]  - Room had one or more surplus door items that were removed:"
                                                           "%1.")
                                                    .arg(extras.join(QLatin1String(", "))),
                                            true);
    }

    // ExitWeights:
    if (!exitWeightsCopy.isEmpty()) {
        QStringList extras;
        QMapIterator<QString, int> itSpareExitWeight(exitWeightsCopy);
        while (itSpareExitWeight.hasNext()) {
            itSpareExitWeight.next();
            exitWeights.remove(itSpareExitWeight.key());
            extras << qsl("\"%1\"(%2)").arg(itSpareExitWeight.key()).arg(itSpareExitWeight.value());
        }
        if (mudlet::self()->showMapAuditErrors()) {
            const QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus weight items that were removed:\n"
                                 "%2.")
                                            .arg(QString::number(id),
                                                 extras.join(QLatin1String(", ")));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr("[ INFO ]  - Room had one or more surplus weight items that were removed: "
                                                           "%1.")
                                                    .arg(extras.join(QLatin1String(", "))),
                                            true);
    }

    // ExitLocks:
    if (!exitLocksCopy.isEmpty()) {
        QStringList extras;
        QSetIterator<int> itSpareExitLock(exitLocksCopy);
        while (itSpareExitLock.hasNext()) {
            const int dirCode = itSpareExitLock.next();
            extras.append(dirCodeToDisplayName(dirCode));
            exitLocks.removeAll(dirCode);
        }
        if (mudlet::self()->showMapAuditErrors()) {
            const QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus normal exit lock items\n"
                                                   "that were removed:\n"
                                                   "%2.")
                                            .arg(QString::number(id),
                                                 extras.join(QLatin1String(", ")));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr("[ INFO ]  - Room had one or more surplus nornal exit lock items that were removed: "
                                                           "%1.")
                                                    .arg(extras.join(QLatin1String(", "))),
                                            true);
    }

    // Special exit Locks:
    if (!exitLocksCopy.isEmpty()) {
        QStringList extras;
        QMutableSetIterator<QString> itSpareExitLock(mSpecialExitLocks);
        while (itSpareExitLock.hasNext()) {
            const QString direction = itSpareExitLock.next();
            extras.append(direction);
            itSpareExitLock.remove();
        }
        if (mudlet::self()->showMapAuditErrors()) {
            const QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus special exit lock\n"
                                                   "items that were removed:\n"
                                                   "%2.")
                                            .arg(QString::number(id),
                                                 extras.join(QChar::LineFeed));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr("[ INFO ]  - Room had one or more surplus exit lock items that were removed: "
                                                           "%1.")
                                                    .arg(extras.join(QLatin1String(", "))),
                                            true);
    }

    // Custom Lines - points - the master element - if the entry for an exit is
    // not valid then remove corresponding entries in other elements
    {
        QStringList extras;
        if (!customLinesCopy.isEmpty()) {
            QMapIterator<QString, QList<QPointF>> itSpareCustomLine(customLinesCopy);
            while (itSpareCustomLine.hasNext()) {
                itSpareCustomLine.next();
                customLines.remove(itSpareCustomLine.key());
                customLinesColor.remove(itSpareCustomLine.key());
                customLinesStyle.remove(itSpareCustomLine.key());
                customLinesArrow.remove(itSpareCustomLine.key());
                customLinesColorCopy.remove(itSpareCustomLine.key());
                customLinesStyleCopy.remove(itSpareCustomLine.key());
                customLinesArrowCopy.remove(itSpareCustomLine.key());
                if (itSpareCustomLine.key().isEmpty()) {
                    extras.append("<empty string>");
                } else {
                    extras.append(itSpareCustomLine.key());
                }
            }
        }

        // Custom Lines - colors
        if (!customLinesColorCopy.isEmpty()) {
            QMapIterator<QString, QColor> itSpareCustomLine(customLinesColorCopy);
            while (itSpareCustomLine.hasNext()) {
                itSpareCustomLine.next();
                customLinesColor.remove(itSpareCustomLine.key());
                customLinesStyle.remove(itSpareCustomLine.key());
                customLinesArrow.remove(itSpareCustomLine.key());
                customLinesStyleCopy.remove(itSpareCustomLine.key());
                customLinesArrowCopy.remove(itSpareCustomLine.key());
                if (itSpareCustomLine.key().isEmpty()) {
                    extras.append("<empty string>");
                } else {
                    extras.append(itSpareCustomLine.key());
                }
            }
        }

        // Custom Lines - styles
        if (!customLinesStyleCopy.isEmpty()) {
            QMapIterator<QString, Qt::PenStyle> itSpareCustomLine(customLinesStyleCopy);
            while (itSpareCustomLine.hasNext()) {
                itSpareCustomLine.next();
                customLinesStyle.remove(itSpareCustomLine.key());
                customLinesArrow.remove(itSpareCustomLine.key());
                customLinesArrowCopy.remove(itSpareCustomLine.key());
                if (itSpareCustomLine.key().isEmpty()) {
                    extras.append("<empty string>");
                } else {
                    extras.append(itSpareCustomLine.key());
                }
            }
        }

        // Custom Lines - ending arrow
        if (!customLinesArrowCopy.isEmpty()) {
            QMapIterator<QString, bool> itSpareCustomLine(customLinesArrowCopy);
            while (itSpareCustomLine.hasNext()) {
                itSpareCustomLine.next();
                customLinesArrow.remove(itSpareCustomLine.key());
                if (itSpareCustomLine.key().isEmpty()) {
                    extras.append("<empty string>");
                } else {
                    extras.append(itSpareCustomLine.key());
                }
            }
        }

        if (!extras.isEmpty()) {
            if (mudlet::self()->showMapAuditErrors()) {
                const QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus custom line elements that\n"
                                                       "were removed: %2.")
                                          .arg(QString::number(id),
                                                     extras.join(QLatin1String(", ")));
                mpRoomDB->mpMap->postMessage(infoMsg);
            }
            mpRoomDB->mpMap->appendRoomErrorMsg(id, tr("[ INFO ]  - Room had one or more surplus custom line elements that were removed: %1.").arg(extras.join(QLatin1String(", "))), true);
        }
    }
}

void TRoom::auditExit(int& exitRoomId,                     // Reference to where exit goes to
                      const int dirCode,                   // DIR_xxx code for this exit - to access stubs & locks
                      QMap<QString, int>& exitWeightsPool, // References to working copies of things - valid ones will be removed
                      QSet<int>& exitLocksPool,
                      QMap<QString, int>& doorsPool,
                      QMap<QString, QList<QPointF>>& customLinesPool,
                      QMap<QString, QColor>& customLinesColorPool,
                      QMap<QString, Qt::PenStyle>& customLinesStylePool,
                      QMap<QString, bool>& customLinesArrowPool,
                      const QHash<int, int> roomRemapping)
{
    const QString exitKey = dirCodeToShortString(dirCode);
    const QString displayName = dirCodeToDisplayName(dirCode);
    if (roomRemapping.contains(exitRoomId)) {
        const QString auditKey = qsl("audit.remapped_exit.%1").arg(dirCode);
        userData.insert(auditKey, QString::number(exitRoomId));
        if (mudlet::self()->showMapAuditErrors()) {
            const QString infoMsg = tr("[ INFO ]  - In room with id: %1 correcting exit \"%2\" that was to room with\n"
                                                   "an exit to invalid room: %3 to now go to: %4.")
                    .arg(QString::number(id),
                         displayName,
                         QString::number(exitRoomId),
                         QString::number(roomRemapping.value(exitRoomId)));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr(R"([ INFO ]  - Correcting exit "%1" that was to invalid room id: %2 to now go to: %3.)")
                                            .arg(displayName,
                                                 QString::number(exitRoomId),
                                                 QString::number(roomRemapping.value(exitRoomId))),
                                            true);
        exitRoomId = roomRemapping.value(exitRoomId);
    }

    if (exitRoomId > 0) {
        // A real exit - should have a real destination, and NOT have a stub
        if (Q_UNLIKELY(!mpRoomDB->getRoom(exitRoomId))) {
            // But it doesn't exist
            const QString auditKey = qsl("audit.made_stub_of_valid_but_missing_exit.%1")
                    .arg(dirCode);
            if (mudlet::self()->showMapAuditErrors()) {
                const QString warnMsg = tr("[ WARN ]  - Room with id: %1 has an exit \"%2\" to: %3 but that room\n"
                                                       "does not exist.  The exit will be removed (but the destination room\n"
                                                       "Id will be stored in the room user data under a key:\n"
                                                       "\"%4\")\n"
                                                       "and the exit will be turned into a stub.")
                        .arg(QString::number(id),
                             displayName,
                             QString::number(exitRoomId),
                             auditKey);
                mpRoomDB->mpMap->postMessage(warnMsg);
            }
            mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                                tr(R"([ WARN ]  - Room has an exit "%1" to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key: "%4") and the exit will be turned into a stub.)")
                                                .arg(displayName,
                                                     QString::number(exitRoomId),
                                                     auditKey),
                                                true);
            userData.insert(auditKey, QString::number(exitRoomId));
            // This is now the way to flag an exit as a stub:
            exitRoomId = 0;

            // Remove the following things that a stub exit does not handle
            // Elimination of the corresponding things from the pools of things
            // that have to be checked is done outside of this scope:
            exitLocks.removeAll(dirCode);
            exitWeights.remove(exitKey);
        }
        // ELSE we do have a valid exit destination room

    } else {
        if (!exitRoomId) {
            // We have a stub

            // A stub exit cannot have a lock, or a weight - so remove them if
            // they exist:
            exitLocks.removeAll(dirCode);
            exitWeights.remove(exitKey);
            // We now allow stubs to have custom exit lines so no need to nuke
            // any of those things.

        } else if (exitRoomId != -1) {
            // We have an invalid negative number and not renumbered because
            // the bad room Id DID NOT exist so turn the exit into a stub:
            const QString auditKey = qsl("audit.made_stub_of_invalid_exit.%1").arg(dirCode);
            userData.insert(auditKey, QString::number(exitRoomId));
            QString infoMsg;
            if (mudlet::self()->showMapAuditErrors()) {
                infoMsg = tr("[ INFO ]  - In room with id: %1 exit \"%2\" that was to room with an invalid\n"
                             "room: %3 that does not exist.  The exit will be removed (the bad destination\n"
                             "room id will be stored in the room user data under a key:\n"
                             "\"%4\")\n"
                             "and the exit will be turned into a stub.")
                                  .arg(QString::number(id),
                                       displayName,
                                       QString::number(exitRoomId),
                                       auditKey);
            }
            QString logMsg = tr(R"([ INFO ]  - Room exit "%1" that was to a room with an invalid id: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:"%4") and the exit will be turned into a stub.)")
                                     .arg(displayName,
                                          QString::number(exitRoomId),
                                          auditKey);

            // This is now the way to flag an exit as a stub:
            exitRoomId = 0;

            // Remove the following things that a stub exit does not handle
            // and eliminate the corresponding things from the pools of
            // things that have to be checked - and append extra details
            // about this to the base message:
            if (exitLocks.contains(dirCode)) {
                const QString auditKeyLocked = qsl("audit.invalid_exit.%1.isLocked")
                                                       .arg(dirCode);
                userData.insert(auditKeyLocked, qsl("true"));
                if (mudlet::self()->showMapAuditErrors()) {
                    infoMsg.append(qsl("\n%1")
                                           .arg(tr("It was locked, this is recorded as user data with key:\n\"%1\".")
                                                        .arg(auditKeyLocked)));
                }
                logMsg.append(qsl("  %1")
                                      .arg(tr(R"(It was locked, this is recorded as user data with key: "%1".)")
                                                   .arg(auditKeyLocked)));
                exitLocks.removeAll(dirCode);
            }

            if (exitWeights.contains(exitKey)) {
                const QString auditKeyWeight = qsl("audit.invalid_exit.%1.weight")
                                                       .arg(dirCode);
                userData.insert(auditKeyWeight, QString::number(exitWeights.value(exitKey)));
                if (mudlet::self()->showMapAuditErrors()) {
                    infoMsg.append(qsl("\n%1")
                                           .arg(tr("It had a weight, this is recorded as user data with key:\n\"%1\".")
                                                        .arg(auditKeyWeight)));
                }
                logMsg.append(qsl("  %1")
                                      .arg(tr(R"(It had a weight, this is recorded as user data with key: "%1".)")
                                                   .arg(auditKeyWeight)));
                exitWeights.remove(exitKey);
            }
            if (mudlet::self()->showMapAuditErrors()) {
                mpRoomDB->mpMap->postMessage(infoMsg);
            }
            mpRoomDB->mpMap->appendRoomErrorMsg(id, logMsg, true);

        }
        // ELSE: Is -1 - a non-existant exit
    }
    // We have handled all the things that we want to check the
    // existence of so take them out of the pools of things left to
    // check after all the exits have been looked at:
    exitLocksPool.remove(dirCode);
    doorsPool.remove(exitKey);
    exitWeightsPool.remove(exitKey);
    customLinesPool.remove(exitKey);
    customLinesColorPool.remove(exitKey);
    customLinesStylePool.remove(exitKey);
    customLinesArrowPool.remove(exitKey);
}

void TRoom::writeJsonRoom(QJsonArray& obj) const
{
    QJsonObject roomObj;
    // Static casts are used as reminders of what value types are actually used:
    roomObj.insert(QLatin1String("id"), static_cast<double>(id));

    if (!name.isEmpty()) {
        const QJsonValue nameValue{name};
        roomObj.insert(QLatin1String("name"), nameValue);
    }

    QJsonArray coordinateArray;
    coordinateArray.append(static_cast<double>(mX));
    coordinateArray.append(static_cast<double>(mY));
    coordinateArray.append(static_cast<double>(mZ));
    const QJsonValue coordinatesValue{coordinateArray};
    roomObj.insert(QLatin1String("coordinates"), coordinatesValue);

    if (isLocked) {
        roomObj.insert(QLatin1String("locked"), true);
    }

    if (weight != 1) {
        roomObj.insert(QLatin1String("weight"), static_cast<double>(weight));
    }

    if (!mSymbol.isEmpty()) {
        writeJsonSymbol(roomObj);
    }

    roomObj.insert(QLatin1String("environment"), static_cast<double>(environment));

    const QString hashForRoomID{mpRoomDB->roomIDToHash.value(id)};
    if (!hashForRoomID.isEmpty()) {
        const QJsonValue hashForRoomIDValue{hashForRoomID};
        roomObj.insert(QLatin1String("hash"), hashForRoomIDValue);
    }

    // Not currently done in the binary map format so don't save it here either;
    // reader code left active in case that situation changes:
    // writeJsonHighlight(roomObj);

    writeJsonExits(roomObj);

    writeJsonExitStubs(roomObj);

    writeJsonUserData(roomObj);

    const QJsonValue roomValue{roomObj};
    obj.append(roomValue);
}

int TRoom::readJsonRoom(const QJsonArray& array, const int index, const int areaId)
{
    const QJsonObject roomObj{array.at(index).toObject()};
    // This is not needed to be stored into id as that is done when the room is
    // added to the TRoomDB via a TRoomDB::addRoom(...) call:
    const int roomId = roomObj.value(QLatin1String("id")).toInt();
    name = roomObj.value(QLatin1String("name")).toString();
    area = areaId;
    readJsonUserData(roomObj.value(QLatin1String("userData")).toObject());

    const QJsonArray coordinatesArray = roomObj.value(QLatin1String("coordinates")).toArray();
    mX = coordinatesArray.at(0).toInt();
    mY = coordinatesArray.at(1).toInt();
    mZ = coordinatesArray.at(2).toInt();

    if (roomObj.contains(QLatin1String("locked")) && roomObj.value(QLatin1String("locked")).toBool()) {
        isLocked = true;
    }

    if (roomObj.contains(QLatin1String("weight")) && roomObj.value(QLatin1String("weight")).isDouble()) {
        weight = roomObj.value(QLatin1String("weight")).toInt();
    }

    if (roomObj.contains(QLatin1String("symbol")) && roomObj.value(QLatin1String("symbol")).isObject()) {
        readJsonSymbol(roomObj);
    }

    if (roomObj.contains(QLatin1String("environment")) && roomObj.value(QLatin1String("environment")).isDouble()) {
        environment = roomObj.value(QLatin1String("environment")).toInt();
    }

    if (roomObj.contains(QLatin1String("highlight")) && roomObj.value(QLatin1String("highlight")).isObject()) {
        const QJsonObject highlightObj{roomObj.value(QLatin1String("highlight")).toObject()};
        readJsonHighlight(highlightObj);
    }

    if (roomObj.contains(QLatin1String("hash")) && roomObj.value(QLatin1String("hash")).isString()) {
        const QString hashForRoomID{roomObj.value(QLatin1String("hash")).toString()};
        if (!hashForRoomID.isEmpty()) {
            mpRoomDB->hashToRoomID.insert(hashForRoomID, roomId);
            mpRoomDB->roomIDToHash.insert(roomId, hashForRoomID);
        }
    }

    if (readJsonExits(roomObj)) {
        calcRoomDimensions();
    }

    readJsonExitStubs(roomObj);

    return roomId;
}

// This inserts an array of exits into the provided QJsonObject container
// under the key "exits":
void TRoom::writeJsonExits(QJsonObject& obj) const
{
    QJsonArray exitArray;
    if (north > 0) {
        writeJsonNormalExit(exitArray, DIR_NORTH);
    }
    if (northeast > 0) {
        writeJsonNormalExit(exitArray, DIR_NORTHEAST);
    }
    if (northwest > 0) {
        writeJsonNormalExit(exitArray, DIR_NORTHWEST);
    }
    if (east > 0) {
        writeJsonNormalExit(exitArray, DIR_EAST);
    }
    if (west > 0) {
        writeJsonNormalExit(exitArray, DIR_WEST);
    }
    if (south > 0) {
        writeJsonNormalExit(exitArray, DIR_SOUTH);
    }
    if (southeast > 0) {
        writeJsonNormalExit(exitArray, DIR_SOUTHEAST);
    }
    if (southwest > 0) {
        writeJsonNormalExit(exitArray, DIR_SOUTHWEST);
    }
    if (up > 0) {
        writeJsonNormalExit(exitArray, DIR_UP);
    }
    if (down > 0) {
        writeJsonNormalExit(exitArray, DIR_DOWN);
    }
    if (in > 0) {
        writeJsonNormalExit(exitArray, DIR_IN);
    }
    if (out > 0) {
        writeJsonNormalExit(exitArray, DIR_OUT);
    }
    QMapIterator<QString, int> itSpecialExit(mSpecialExits);
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        writeJsonSpecialExit(exitArray, itSpecialExit.key(), itSpecialExit.value());
    }
    const QJsonValue exitsValue{exitArray};
    obj[QLatin1String("exits")] = exitsValue;
}

bool TRoom::readJsonExits(const QJsonObject& obj)
{
    const QJsonArray exitArray = obj.value(QLatin1String("exits")).toArray();
    bool hasCustomExits = false;
    for (const QJsonValue& exitValue : exitArray) {
        const QJsonObject exitObj{exitValue.toObject()};
        const QString dirString{exitObj.value(QLatin1String("name")).toString()};
        const int dirCode = stringToDirCode(dirString);
        if (dirCode != DIR_OTHER) {
            if (readJsonNormalExit(exitObj, dirCode)) {
                hasCustomExits = true;
            }
        } else {
            if (readJsonSpecialExit(exitObj, dirString)) {
                hasCustomExits = true;
            }
        }
    }

    return hasCustomExits;
}

// This inserts a newly constructed QJsonObject into the provided QJsonArray
// container for the specified normal exit:
void TRoom::writeJsonNormalExit(QJsonArray& array, const int dir) const
{
    QJsonObject exitObj;
    const QString directionString = dirCodeToString(dir);
    const int exitId = getExit(dir);
    const QString directionKey = dirCodeToShortString(dir);
    // Skip any unreal exits:
    if (exitId < 1) {
        return;
    }

    const QJsonValue directionStringValue{directionString};
    exitObj.insert(QLatin1String("name"), directionStringValue);
    exitObj.insert(QLatin1String("exitId"), static_cast<double>(exitId));

    // The second argument is used to look-up the right value for the door- so
    // needs to be the "shortstring":
    writeJsonDoor(exitObj, directionKey);

    if (exitWeights.contains(directionKey)) {
        exitObj.insert(QLatin1String("weight"), static_cast<double>(exitWeights.value(directionKey)));
    }

    if (exitLocks.contains(dir)) {
        // We won't bother to include this item for unlocked exits but we will
        // check that it is true when reading in case the file gets edited:
        exitObj.insert(QLatin1String("locked"), true);
    }

    if (customLines.contains(directionKey)) {
        writeJsonCustomExitLine(exitObj, directionKey);
    }
    const QJsonValue exitValue{exitObj};
    array.append(exitValue);
}

bool TRoom::readJsonNormalExit(const QJsonObject& exitObj, const int dir)
{
    const int exitRoomId = exitObj.value(QLatin1String("exitId")).toInt();
    bool hasCustomExit = false;
    if (exitRoomId < 1) {
        qDebug().nospace().noquote() << "TRoom::readJsonNormalExit(...) INFO - when reading exits for room id: " << id << " the normal \"" << dirCodeToString(dir)
                                     << "\" exit had an invalid exit room id of: " << exitRoomId;
        return hasCustomExit;
    }

    const QString shortString{dirCodeToShortString(dir)};
    switch (dir) {
    case DIR_NORTH:
        north = exitRoomId;
        break;
    case DIR_NORTHEAST:
        northeast = exitRoomId;
        break;
    case DIR_NORTHWEST:
        northwest = exitRoomId;
        break;
    case DIR_EAST:
        east = exitRoomId;
        break;
    case DIR_WEST:
        west = exitRoomId;
        break;
    case DIR_SOUTH:
        south = exitRoomId;
        break;
    case DIR_SOUTHEAST:
        southeast = exitRoomId;
        break;
    case DIR_SOUTHWEST:
        southwest = exitRoomId;
        break;
    case DIR_UP:
        up = exitRoomId;
        break;
    case DIR_DOWN:
        down = exitRoomId;
        break;
    case DIR_IN:
        in = exitRoomId;
        break;
    case DIR_OUT:
        out = exitRoomId;
        break;
    default:
        Q_UNREACHABLE(); // Special exits should never have gotten into this method!
    }

    if (exitObj.contains(QLatin1String("weight")) && exitObj.value(QLatin1String("weight")).isDouble() && exitObj.value(QLatin1String("weight")).toInt() > 0) {
        exitWeights.insert(shortString, exitObj.value(QLatin1String("weight")).toInt());
    }

    if (exitObj.contains(QLatin1String("locked")) && exitObj.value(QLatin1String("locked")).isBool() && exitObj.value(QLatin1String("locked")).toBool()) {
        // We don't bother to include this item for unlocked exits but we will
        // check that it is true when reading in case the file was edited:
        exitLocks.append(dir);
    }

    if (exitObj.contains(QLatin1String("customLine")) && exitObj.value(QLatin1String("customLine")).isObject()) {
        readJsonCustomExitLine(exitObj, shortString);
        hasCustomExit = true;
    }

    if (exitObj.contains(QLatin1String("door")) && exitObj.value(QLatin1String("door")).isString()) {
        readJsonDoor(exitObj, shortString);
    }

    return hasCustomExit;
}

void TRoom::writeJsonSpecialExit(QJsonArray& array, const QString& dir, const int exitId) const
{
    QJsonObject exitObj;
    const bool exitLocked = mSpecialExitLocks.contains(dir);

    // Safety step to avoid insertion of any unreal exits:
    if (exitId < 1 || dir.isEmpty()) {
        return;
    }

    const QJsonValue dirValue{dir};
    exitObj.insert(QLatin1String("name"), dirValue);
    exitObj.insert(QLatin1String("exitId"), static_cast<double>(exitId));

    writeJsonDoor(exitObj, dir);

    if (exitWeights.contains(dir)) {
        exitObj.insert(QLatin1String("weight"), static_cast<double>(exitWeights.value(dir)));
    }

    if (exitLocked) {
        // We won't bother to include this item for unlocked exits but we will
        // check that it is true when reading in case the file gets edited:
        exitObj.insert(QLatin1String("locked"), true);
    }

    if (customLines.contains(dir)) {
        writeJsonCustomExitLine(exitObj, dir);
    }

    const QJsonValue exitValue{exitObj};
    array.append(exitValue);
}

bool TRoom::readJsonSpecialExit(const QJsonObject& exitObj, const QString& dir)
{
    const int exitRoomId = exitObj.value(QLatin1String("exitId")).toInt();
    bool hasCustomExit = false;
    if (exitRoomId < 1) {
        qDebug().nospace().noquote() << "TRoom::readJsonSpecialExit(...) INFO - when reading exits for room id: " << id << " the special \"" << dir
                                     << "\" exit had an invalid exit room id of: " << exitRoomId;
        return hasCustomExit;
    }

    mSpecialExits.insert(dir, exitRoomId);

    if (exitObj.contains(QLatin1String("weight")) && exitObj.value(QLatin1String("weight")).isDouble() && exitObj.value(QLatin1String("weight")).toInt() > 0) {
        exitWeights.insert(dir, exitObj.value(QLatin1String("weight")).toInt());
    }

    if (exitObj.contains(QLatin1String("locked")) && exitObj.value(QLatin1String("locked")).isBool() && exitObj.value(QLatin1String("locked")).toBool()) {
        // We don't bother to include this item for unlocked exits but we will
        // check that it is true when reading in case the file was edited:
        mSpecialExitLocks.insert(dir);
    }

    if (exitObj.contains(QLatin1String("customLine")) && exitObj.value(QLatin1String("customLine")).isObject()) {
        readJsonCustomExitLine(exitObj, dir);
        hasCustomExit = true;
    }

    if (exitObj.contains(QLatin1String("door")) && exitObj.value(QLatin1String("door")).isString()) {
        readJsonDoor(exitObj, dir);
    }

    return hasCustomExit;
}

void TRoom::writeJsonDoor(QJsonObject& obj, const QString& key) const
{
    QString door;
    switch (doors.value(key)) {
        // We won't bother to include this item for exits without doors but we
        // will check that it is one of these values (and not "none") when
        // reading in case the file gets edited:
    case 1: door = QLatin1String("open");    break;
    case 2: door = QLatin1String("closed");  break;
    case 3: door = QLatin1String("locked");  break;
    default:
        return;
    }
    const QJsonValue doorValue{door};
    obj.insert(QLatin1String("door"), doorValue);
}

// The first argument is an exit, special exit or stub QJsonObject, the second
// is a "shortstring" (for normal exit directions):
void TRoom::readJsonDoor(const QJsonObject& obj, const QString& dir)
{
    if (!obj.contains(QLatin1String("door")) || !obj.value(QLatin1String("door")).isString()) {
        return;
    }

    const QString doorString{obj.value(QLatin1String("door")).toString()};
    if (doorString == QLatin1String("open")) {
        doors.insert(dir, 1);
        return;
    }
    if (doorString == QLatin1String("closed")) {
        doors.insert(dir, 2);
        return;
    }
    if (doorString == QLatin1String("locked")) {
        doors.insert(dir, 3);
        return;
    }
    if (Q_UNLIKELY(doorString == QLatin1String("none"))) {
        return;
    }
    qCritical().nospace().noquote() << "TRoom::readJsonDoor(...) CRITICAL - a type of door: \"" << dir << "\" is not understood!";
    Q_UNREACHABLE(); // No other string expected
}

// This tacks on extra details onto the calling exitObj if there IS a custom line:
void TRoom::writeJsonCustomExitLine(QJsonObject& exitObj, const QString& directionString) const
{
    // Safety step to not insert anything for an empty custom line
    if (!customLines.contains(directionString) || customLines.value(directionString).isEmpty()) {
        return;
    }

    QJsonObject customLineObj;
    QJsonArray customLinePointsArray;
    const QList<QPointF> points{customLines.value(directionString)};
    for (int i = 0, total = points.count(); i < total; ++i) {
        QJsonArray customLinePointCoordinateArray;
        const QPointF point{points.at(i)};
        customLinePointCoordinateArray.append(static_cast<double>(point.x()));
        customLinePointCoordinateArray.append(static_cast<double>(point.y()));
        // We might wish to consider storing a z in the future to accommodate 3D
        // custom lines...!
        const QJsonValue customLinePointCoordinatesValue{customLinePointCoordinateArray};
        customLinePointsArray.append(customLinePointCoordinatesValue);
    }
    const QJsonValue customLineCoordinatesValue{customLinePointsArray};
    customLineObj.insert(QLatin1String("coordinates"), customLineCoordinatesValue);

    TMap::writeJsonColor(customLineObj, customLinesColor.value(directionString));

    customLineObj.insert(QLatin1String("endsInArrow"), customLinesArrow.value(directionString));

    QString lineStyle;
    switch (customLinesStyle.value(directionString)) {
    case Qt::DotLine:           lineStyle = QLatin1String("dot line");           break;
    case Qt::DashDotLine:       lineStyle = QLatin1String("dash dot line");      break;
    case Qt::DashDotDotLine:    lineStyle = QLatin1String("dash dot dot line");  break;
    case Qt::DashLine:          lineStyle = QLatin1String("dash line");          break;
    default:
        {} // Use this (nothing) for Solid but will also convert anything else we do not handle to it as well
    }
    if (!lineStyle.isEmpty()) {
        const QJsonValue customLineStyleValue{lineStyle};
        customLineObj.insert(QLatin1String("style"), customLineStyleValue);
    }

    const QJsonValue customLineValue{customLineObj};
    exitObj.insert(QLatin1String("customLine"), customLineValue);
}

void TRoom::readJsonCustomExitLine(const QJsonObject& exitObj, const QString& directionString)
{
    const QJsonObject customLineObj{exitObj.value(QLatin1String("customLine")).toObject()};

    // Safety step to not insert anything for an empty custom line
    if (!customLineObj.contains(QLatin1String("coordinates")) || !customLineObj.value(QLatin1String("coordinates")).isArray()) {
        return;
    }

    const QJsonArray customLinePointsArray = customLineObj.value(QLatin1String("coordinates")).toArray();
    if (customLinePointsArray.isEmpty()) {
        return;
    }

    QList<QPointF> points;
    for (int i = 0, total = customLinePointsArray.count(); i < total; ++i) {
        const QJsonArray customLinePointCoordinateArray = customLinePointsArray.at(i).toArray();
        if (customLinePointCoordinateArray.size() == 2 && customLinePointCoordinateArray.at(0).isDouble() && customLinePointCoordinateArray.at(1).isDouble()) {
            const QPointF point{customLinePointCoordinateArray.at(0).toDouble(), customLinePointCoordinateArray.at(1).toDouble()};

            // We might wish to consider if there is a z in the future to
            // accommodate 3D custom lines...!
            points.append(point);
        }
    }
    customLines.insert(directionString, points);

    customLinesColor.insert(directionString, TMap::readJsonColor(customLineObj));

    if (customLineObj.contains(QLatin1String("endsInArrow")) && customLineObj.value(QLatin1String("endsInArrow")).isBool()) {
        customLinesArrow.insert(directionString, customLineObj.value(QLatin1String("endsInArrow")).toBool());
    } else {
        customLinesArrow.insert(directionString, false);
    }

    if (customLineObj.contains(QLatin1String("style")) && customLineObj.value(QLatin1String("style")).isString()) {
        const QString lineStyle{customLineObj.value(QLatin1String("style")).toString()};
        if (lineStyle == QLatin1String("dash line")) {
            customLinesStyle.insert(directionString, Qt::DashLine);
        } else if (lineStyle == QLatin1String("dash dot dot line")) {
            customLinesStyle.insert(directionString, Qt::DashDotDotLine);
        } else if (lineStyle == QLatin1String("dash dot line")) {
            customLinesStyle.insert(directionString, Qt::DashDotLine);
        } else if (lineStyle == QLatin1String("dot line")) {
            customLinesStyle.insert(directionString, Qt::DotLine);
        } else {
            customLinesStyle.insert(directionString, Qt::SolidLine);
        }
    } else {
        customLinesStyle.insert(directionString, Qt::SolidLine);
    }
}

void TRoom::writeJsonUserData(QJsonObject& obj) const
{
    QJsonObject userDataObj;
    if (userData.isEmpty()) {
        // Skip creating a user data array if it will be empty:
        return;
    }
    QMapIterator<QString, QString> itDataItem(userData);
    while (itDataItem.hasNext()) {
        itDataItem.next();
        const QJsonValue userDataValue{itDataItem.value()};
        userDataObj.insert(itDataItem.key(), userDataValue);
    }
    const QJsonValue userDatasValue{userDataObj};
    obj.insert(QLatin1String("userData"), userDatasValue);
}

void TRoom::readJsonUserData(const QJsonObject& obj)
{
    if (obj.isEmpty()) {
        // Bail out immediately if there is nothing to do:
        return;
    }

    for (auto& key : obj.keys()) {
        if (obj.value(key).isString()) {
            userData.insert(key, obj.value(key).toString());
        }
    }
}

void TRoom::writeJsonExitStubs(QJsonObject& obj) const
{
    QJsonArray exitStubsArray;
    // Includes any special exit stubs:
    if (!hasAStubExit(true)) {
        // Don't add an empty stub array
        return;
    }

    // We might have special exit stubs, which is why we use strings not numbers
    // for the identifiers:
    QList<QString> exitStubsList;

    auto isNormalExitAStub = [&] (int direction) {
        if (!getExit(direction)) {
            exitStubsList.append(dirCodeToString(direction));
        }
    };
    isNormalExitAStub(DIR_NORTH);
    isNormalExitAStub(DIR_NORTHEAST);
    isNormalExitAStub(DIR_EAST);
    isNormalExitAStub(DIR_SOUTHEAST);
    isNormalExitAStub(DIR_SOUTH);
    isNormalExitAStub(DIR_SOUTHWEST);
    isNormalExitAStub(DIR_WEST);
    isNormalExitAStub(DIR_NORTHWEST);
    isNormalExitAStub(DIR_UP);
    isNormalExitAStub(DIR_DOWN);
    isNormalExitAStub(DIR_IN);
    isNormalExitAStub(DIR_OUT);

    // Gather the special exits that are stubs as well:
    QMapIterator<QString, int> itSpecialExit(mSpecialExits);
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        if (!itSpecialExit.value()) {
            exitStubsList.append(itSpecialExit.key());
        }
    }

    if (exitStubsList.count() > 1) {
        std::sort(exitStubsList.begin(), exitStubsList.end());
    }

    for (const auto& stubName : exitStubsList) {
        QJsonObject exitStubObj;
        const QJsonValue stubNameValue{stubName};
        exitStubObj.insert(QLatin1String("name"), stubNameValue);
        writeJsonDoor(exitStubObj, stubName);
        const QJsonValue exitStubValue{exitStubObj};
        exitStubsArray.append(exitStubValue);
    }

    const QJsonValue exitStubsValues{exitStubsArray};
    obj.insert(QLatin1String("stubExits"), exitStubsValues);
}

void TRoom::readJsonExitStubs(const QJsonObject& obj)
{
    // There shouldn't be an empty stub array
    if (!obj.contains(QLatin1String("stubExits")) || !obj.value(QLatin1String("stubExits")).isArray()) {
        return;
    }
    const QJsonArray exitStubsArray = obj.value(QLatin1String("stubExits")).toArray();
    if (exitStubsArray.isEmpty()) {
        return;
    }

    // we allow special exit stubs:
    for (const auto exitStubValue : exitStubsArray) {
        const QJsonObject exitStubObj{exitStubValue.toObject()};
        const QString direction{exitStubObj.value(QLatin1String("name")).toString()};
        const int dir = stringToDirCode(direction);
        QString doorKey;
        if (dir != DIR_OTHER) {
            doorKey = dirCodeToShortString(dir);
            setExit(0, dir);
        } else {
            doorKey = direction;
            setSpecialExit(0, direction);
            qWarning().nospace().noquote() << "TRoom::readJsonExitStubs(...) WARNING - a special exit stub for the name/command: \"" << direction
                                           << "\" has been detected - but Mudlet does not currently support stubs in non-normal exit directions!";
        }

        // Will now get here for all exit directions:
        if (exitStubObj.contains(QLatin1String("door")) && exitStubObj.value(QLatin1String("door")).isString()) {
            readJsonDoor(exitStubObj, doorKey);
        }
    }
}

// Not currently used (it isn't saved in the binary format either) but reserved
// in case we do desire it - leaving the reader code in place so that if it
// does get used then no change to the format is needed:
void TRoom::writeJsonHighlight(QJsonObject& obj) const
{
    const bool noColor = (highlightColor == scDefaultHighlightForeground) && (highlightColor2 == scDefaultHighlightBackground);
    if (!highlight && qFuzzyCompare(1.0f + highlightRadius, 1.0f) && noColor) {
        // The default case so no need to include it:
        return;
    }

    QJsonObject highlightObj;
    highlightObj.insert(QLatin1String("active"), highlight);

    if (!noColor) {
        QJsonArray highlightColorArray;
        QJsonObject highlightColorFgObj;
        QJsonObject highlightColorBgObj;
        TMap::writeJsonColor(highlightColorFgObj, highlightColor);
        TMap::writeJsonColor(highlightColorBgObj, highlightColor2);
        const QJsonValue highlightColorFgValue{highlightColorFgObj};
        const QJsonValue highlightColorBgValue{highlightColorBgObj};
        highlightColorArray.append(highlightColorFgValue);
        highlightColorArray.append(highlightColorBgValue);
        const QJsonValue highlightColorValue{highlightColorArray};
        highlightObj.insert(QLatin1String("colors"), highlightColorValue);
    }

    highlightObj.insert(QLatin1String("radius"), static_cast<double>(highlightRadius));
    const QJsonValue highlightValue{highlightObj};
    obj.insert(QLatin1String("highlight"), highlightValue);
}

void TRoom::readJsonHighlight(const QJsonObject& highlightObj)
{
    if (highlightObj.contains(QLatin1String("active")) && highlightObj.value(QLatin1String("active")).isBool()) {
        highlight = highlightObj.value(QLatin1String("active")).toBool();
    }

    if (highlightObj.contains(QLatin1String("colors")) && highlightObj.value(QLatin1String("colors")).isArray() && highlightObj.value(QLatin1String("colors")).toArray().size() == 2) {
        const QJsonArray highlightColorArray = highlightObj.value(QLatin1String("colors")).toArray();

        highlightColor = TMap::readJsonColor(highlightColorArray.at(0).toObject());
        highlightColor2 = TMap::readJsonColor(highlightColorArray.at(1).toObject());
    }

    highlightRadius = highlightObj.value(QLatin1String("radius")).toDouble();
}

void TRoom::writeJsonSymbol(QJsonObject& roomObj) const
{
    if (mSymbol.isEmpty()) {
        return;
    }

    QJsonObject symbolObj;
    const QJsonValue symbolText{mSymbol};
    symbolObj.insert(QLatin1String("text"), symbolText);
    if (mSymbolColor.isValid()) {
        TMap::writeJsonColor(symbolObj, mSymbolColor);
    }

    const QJsonValue symbolValue{symbolObj};
    roomObj.insert(QLatin1String("symbol"), symbolValue);
}

void TRoom::readJsonSymbol(const QJsonObject& roomObj)
{
    const QJsonObject symbolObj{roomObj.value(QLatin1String("symbol")).toObject()};
    if (symbolObj.contains(QLatin1String("text")) && symbolObj.value(QLatin1String("text")).isString()) {
        mSymbol = symbolObj.value(QLatin1String("text")).toString();
    }

    const QColor color = TMap::readJsonColor(symbolObj);
    if (color.isValid()) {
        mSymbolColor = color;
    }
}

QSet<int> TRoom::normalStubExits() const
{
    QSet<int> results;
    if (!north) {
        results << DIR_NORTH;
    }
    if (!south) {
        results << DIR_SOUTH;
    }
    if (!east) {
        results << DIR_EAST;
    }
    if (!west) {
        results << DIR_WEST;
    }
    if (!up) {
        results << DIR_UP;
    }
    if (!down) {
        results << DIR_DOWN;
    }
    if (!northeast) {
        results << DIR_NORTHEAST;
    }
    if (!southeast) {
        results << DIR_SOUTHEAST;
    }
    if (!southwest) {
        results << DIR_SOUTHWEST;
    }
    if (!northwest) {
        results << DIR_NORTHWEST;
    }
    if (!in) {
        results << DIR_IN;
    }
    if (!out) {
        results << DIR_OUT;
    }
    return results;
}

QSet<QString> TRoom::specialStubExits() const
{
    QSet<QString> results;
    QMapIterator<QString, int> itSpecialExit(mSpecialExits);
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        if (!itSpecialExit.value()) {
            results.insert(itSpecialExit.key());
        }
    }
    return results;
}

bool TRoom::hasAStubExit(const bool alsoConsiderSpecialExits) const
{
    if (!north|!south|!east|!west|!northeast|!northwest|!southeast|!southwest|!up|!down|!in|!out) {
        return true;
    }

    if (alsoConsiderSpecialExits) {
        QMapIterator<QString, int> itSpecialExit(mSpecialExits);
        while (itSpecialExit.hasNext()) {
            itSpecialExit.next();
            if (itSpecialExit.value() == 0) {
                return true;
            }
        }
    }

    return false;
}

bool TRoom::hasANormalExitCustomLine(const int direction) const
{
    const QString exitKey = dirCodeToShortString(direction);
    return (customLines.value(exitKey).count() > 0);
}

QMap<QString, int> TRoom::getSpecialExits(const bool includeStubs) const
{
    if (includeStubs) {
        return mSpecialExits;
    }

    QMap<QString, int> results;
    QMapIterator<QString, int> itSpecialExit(mSpecialExits);
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        if (itSpecialExit.value() > 0) {
            results.insert(itSpecialExit.key(), itSpecialExit.value());
        }
    }
    return results;
}
