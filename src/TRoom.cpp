/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018, 2020 by Stephen Lyons                  *
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

TRoom::TRoom(TRoomDB* pRDB)
: x(0)
, y(0)
, z(0)
, environment(-1)
, isLocked(false)
, min_x(0)
, min_y(0)
, max_x(0)
, max_y(0)
, mSymbol(QString())
, highlight(false)
, highlightColor(QColor(255, 150, 0))
, highlightRadius()
, rendered(false)
, id(0)
, area(-1)
, weight(1)
, north(-1)
, northeast(-1)
, east(-1)
, southeast(-1)
, south(-1)
, southwest(-1)
, west(-1)
, northwest(-1)
, up(-1)
, down(-1)
, in(-1)
, out(-1)
, mpRoomDB(pRDB)
{
}

TRoom::~TRoom()
{
    if (mpRoomDB) {
        mpRoomDB->__removeRoom(id);
    }
}

const QString TRoom::dirCodeToDisplayName(const int dirCode)
{
    switch (dirCode) {
    case DIR_NORTH:     return tr("North");       break;
    case DIR_NORTHEAST: return tr("North-east");  break;
    case DIR_NORTHWEST: return tr("North-west");  break;
    case DIR_SOUTH:     return tr("South");       break;
    case DIR_SOUTHEAST: return tr("South-east");  break;
    case DIR_SOUTHWEST: return tr("South-west");  break;
    case DIR_EAST:      return tr("East");        break;
    case DIR_WEST:      return tr("West");        break;
    case DIR_UP:        return tr("Up");          break;
    case DIR_DOWN:      return tr("Down");        break;
    case DIR_IN:        return tr("In");          break;
    case DIR_OUT:       return tr("Out");         break;
    case DIR_OTHER:     return tr("Other");       break;
    default:
        return tr("Unknown");
    }
}

bool TRoom::hasExitStub(int direction)
{
    if (exitStubs.contains(direction)) {
        return true;
    } else {
        return false;
    }
}

void TRoom::setExitStub(int direction, bool status)
{
    if (status) {
        if (!hasExit(direction)) {
            if (!exitStubs.contains(direction)) {
                // Previous code did not check for an existing entry for given
                // direction and QList container permits duplicates of same value!
                exitStubs.append(direction);
            }
        } else {
            QString error = QString("Set exit stub in given direction in RoomID(%1) - there is already an exit there!").arg(id);
            mpRoomDB->mpMap->logError(error);
        }
    } else {
        exitStubs.removeAll(direction);
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
}

// Previous implementations did not allow for REMOVAL of an exit weight (by
// setting it to zero)
void TRoom::setExitWeight(const QString& cmd, int w)
{
    if (w > 0) {
        exitWeights[cmd] = w;
        if (mpRoomDB && mpRoomDB->mpMap) {
            mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
        }
    } else if (exitWeights.contains(cmd)) {
        exitWeights.remove(cmd);
        if (mpRoomDB && mpRoomDB->mpMap) {
            mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
        }
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
            return true; // As we have changed things
        } else {
            return false; // Valid but ineffective
        }
    } else if (doors.contains(cmd) && !doorStatus) {
        doors.remove(cmd);
        return true; // As we have changed things
    } else {
        return false; // As we have not changed things
    }
}

int TRoom::getDoor(const QString& cmd)
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
bool TRoom::setArea(int areaID, bool isToDeferAreaRelatedRecalculations)
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
        pA2->removeRoom(id, isToDeferAreaRelatedRecalculations);
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

    if (!isToDeferAreaRelatedRecalculations) {
        QSetIterator<TArea*> itpArea = dirtyAreas;
        while (itpArea.hasNext()) {
            TArea* pArea = itpArea.next();
            pArea->calcSpan();
            pArea->determineAreaExits();
            pArea->mIsDirty = false;
        }
        dirtyAreas.clear();
    }

    return true;
}

bool TRoom::setExit(const int to, const int direction)
{
    // FIXME: This along with TRoom->setExit need to be unified to a controller.
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
    mpRoomDB->updateEntranceMap(this);
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
        return; // Refuse to create an unnamed special exit!!!
    }

    if (to > 0) {
        mSpecialExits[cmd] = to;
    } else {
        // Clean up related data:
        customLinesArrow.remove(cmd);
        customLinesColor.remove(cmd);
        customLinesStyle.remove(cmd);
        customLines.remove(cmd);
        exitWeights.remove(cmd);
        doors.remove(cmd);
        mSpecialExitLocks.remove(cmd);
        mSpecialExits.remove(cmd);
    }

    TArea* pA = mpRoomDB->getArea(area);
    if (pA) {
        pA->determineAreaExitsOfRoom(id);
        // This updates the (TArea *)->exits map even for exit REMOVALS
    }
    mpRoomDB->updateEntranceMap(this);
    mpRoomDB->mpMap->mMapGraphNeedsUpdate = true;
}

void TRoom::clearSpecialExits()
{
    if (mSpecialExits.isEmpty()) {
        // Nothing to do:
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
    }
}

void TRoom::calcRoomDimensions()
{
    if (customLines.empty()) {
        return;
    }
    min_x = 0.0;
    min_y = 0.0;
    max_x = 0.0;
    max_y = 0.0;
    bool needInit = true;

    QMapIterator<QString, QList<QPointF>> it(customLines);
    while (it.hasNext()) {
        it.next();
        const QList<QPointF>& _pL = it.value();
        if (_pL.empty()) {
            continue;
        }
        if (needInit) {
            needInit = false;
            min_x = _pL[0].x();
            max_x = min_x;
            min_y = _pL[0].y();
            max_y = min_y;
        }
        for (auto point : _pL) {
            qreal _x = point.x();
            qreal _y = point.y();
            if (_x < min_x) {
                min_x = _x;
            }
            if (_x > max_x) {
                max_x = _x;
            }
            if (_y < min_y) {
                min_y = _y;
            }
            if (_y > max_y) {
                max_y = _y;
            }
        }
    }
}

void TRoom::restore(QDataStream& ifs, int roomID, int version)
{
    id = roomID;
    ifs >> area;
    // Can be useful when analysing suspect map files!
    //     qDebug() << "TRoom::restore(...," << roomID << ",...) has AreaId:" << area;
    ifs >> x;
    ifs >> y;
    ifs >> z;
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
    ifs >> isLocked;
    if (version >= 21) {
        ifs >> mSpecialExits;
    } else if (version >= 6) {
        // Before version 21 the special exits were stored as a QMultiMap<int, QString>
        // with the lock information prepended as a '1' locked or '0' unlocked
        // from version 11-ish:
        QMultiMap<int, QString> oldSpecialExits;
        ifs >> oldSpecialExits;
        QMapIterator<int, QString> itOldSpecialExit(oldSpecialExits);
        while (itOldSpecialExit.hasNext()) {
            itOldSpecialExit.next();
            QString cmd{itOldSpecialExit.value()};
            if (cmd.startsWith(QLatin1String("1"))) {
                // Is locked:
                mSpecialExits.insert(cmd.mid(1), itOldSpecialExit.key());
                mSpecialExitLocks.insert(cmd);
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
        if (version < 19) {
            // Recover and remove backup values from the user data
            QString symbolString = userData.take(QLatin1String("system.fallback_symbol"));
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
            // direction keys and using a QColor to store the color:
            ifs >> customLines;
            ifs >> customLinesArrow;
            ifs >> customLinesColor;
            ifs >> customLinesStyle;
        } else {
            QMap<QString, QList<QPointF>> oldLinesData;
            ifs >> oldLinesData;
            QMapIterator<QString, QList<QPointF>> itCustomLine(oldLinesData);
            while (itCustomLine.hasNext()) {
                itCustomLine.next();
                QString direction(itCustomLine.key());
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
                QString direction(itCustomLineArrow.key());
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
                QString direction(itCustomLineColor.key());
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
                    // itCustomLineColor.value() entites AND altogether missing
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            auto customLineKeys = customLines.keys();
            QSet<QString> missingKeys{customLineKeys.begin(), customLineKeys.end()};
            if (!customLinesColor.isEmpty()) {
                auto customLinesColorKeys = customLinesColor.keys();
                QSet<QString> customLinesColorKeysSet{customLinesColorKeys.begin(), customLinesColorKeys.end()};
                missingKeys.subtract(customLinesColorKeysSet);
            }
#else
            QSet<QString> missingKeys{customLines.keys().toSet().subtract(customLinesColor.keys().toSet())};
#endif
            QSetIterator<QString> itMissingCustomLineColourKey(missingKeys);
            while (itMissingCustomLineColourKey.hasNext()) {
                customLinesColor.insert(itMissingCustomLineColourKey.next(), QColor(Qt::red));
            }

            QMap<QString, QString> oldLineStyleData;
            ifs >> oldLineStyleData;
            QMapIterator<QString, QString> itCustomLineStyle(oldLineStyleData);
            while (itCustomLineStyle.hasNext()) {
                itCustomLineStyle.next();
                QString direction(itCustomLineStyle.key());
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
        }
        if (version >= 21) {
            ifs >> mSpecialExitLocks;
        }
        ifs >> exitLocks;
    }
    if (version >= 13) {
        ifs >> exitStubs;
    }
    if (version >= 16) {
        ifs >> exitWeights;
        ifs >> doors;
    }
    calcRoomDimensions();
}

void TRoom::audit(const QHash<int, int> roomRemapping, const QHash<int, int> areaRemapping)
{
    if (areaRemapping.contains(area)) {
        userData.insert(QStringLiteral("audit.remapped_area"), QString::number(area));
        area = areaRemapping.value(area);
    }

    auditExits(roomRemapping);
}

void TRoom::auditExits(const QHash<int, int> roomRemapping)
{
    // Clone all the structures into working copies that we can eliminate valid
    // members from to identify any rogue members before removing them:

    QMap<QString, int> exitWeightsCopy = exitWeights;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QSet<int> exitStubsCopy{exitStubs.begin(), exitStubs.end()};
    QSet<int> exitLocksCopy{exitLocks.begin(), exitLocks.end()};
#else
    QSet<int> exitStubsCopy{exitStubs.toSet()};
    QSet<int> exitLocksCopy{exitLocks.toSet()};
#endif
    QMap<QString, int> doorsCopy = doors;
    QMap<QString, QList<QPointF>> customLinesCopy = customLines;
    QMap<QString, QColor> customLinesColorCopy = customLinesColor;
    QMap<QString, Qt::PenStyle> customLinesStyleCopy = customLinesStyle;
    QMap<QString, bool> customLinesArrowCopy = customLinesArrow;

    exitWeightsCopy.detach(); // Make deep copies now, this will happen anyhow once we start to remove valid members
    exitStubsCopy.detach();
    exitLocksCopy.detach();
    doorsCopy.detach();
    customLinesCopy.detach();
    customLinesColorCopy.detach();
    customLinesStyleCopy.detach();
    customLinesArrowCopy.detach();

    auditExit(north,
              DIR_NORTH,
              tr("North"),
              QStringLiteral("n"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(northeast,
              DIR_NORTHEAST,
              tr("Northeast"),
              QStringLiteral("ne"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(northwest,
              DIR_NORTHWEST,
              tr("Northwest"),
              QStringLiteral("nw"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(south,
              DIR_SOUTH,
              tr("South"),
              QStringLiteral("s"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(southeast,
              DIR_SOUTHEAST,
              tr("Southeast"),
              QStringLiteral("se"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(southwest,
              DIR_SOUTHWEST,
              tr("Southwest"),
              QStringLiteral("sw"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(east,
              DIR_EAST,
              tr("East"),
              QStringLiteral("e"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(west,
              DIR_WEST,
              tr("West"),
              QStringLiteral("w"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(up,
              DIR_UP,
              tr("Up"),
              QStringLiteral("up"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(down,
              DIR_DOWN,
              tr("Down"),
              QStringLiteral("down"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(in,
              DIR_IN,
              tr("In"),
              QStringLiteral("in"),
              exitWeightsCopy,
              exitStubsCopy,
              exitLocksCopy,
              doorsCopy,
              customLinesCopy,
              customLinesColorCopy,
              customLinesStyleCopy,
              customLinesArrowCopy,
              roomRemapping);

    auditExit(out,
              DIR_OUT,
              tr("Out"),
              QStringLiteral("out"),
              exitWeightsCopy,
              exitStubsCopy,
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
            QString exitName = it.key();
            int exitRoomId = it.value();
            if (exitName.isEmpty()) {
                if (mudlet::self()->showMapAuditErrors()) {
                    QString warnMsg = tr("[ WARN ]  - In room id:%1 removing invalid (special) exit to %2 {with no name!}").arg(id, 6, QLatin1Char('0')).arg(exitRoomId, 6, QLatin1Char('0'));
                    // If size is less than or equal to 0 then there is nothing to print!!!
                    mpRoomDB->mpMap->postMessage(warnMsg);
                }
                mpRoomDB->mpMap->appendRoomErrorMsg(id, tr("[ WARN ]  - Room had an invalid (special) exit to %1 {with no name!} it was removed.").arg(exitRoomId, 6, QLatin1Char('0')));
                it.remove();
                continue;
            }

            if (roomRemapping.contains(exitRoomId)) {
                QString auditKey = QStringLiteral("audit.remapped_special_exit.%1").arg(exitName);
                userData.insert(auditKey, QString::number(exitRoomId));
                if (mudlet::self()->showMapAuditErrors()) {
                    QString infoMsg = tr("[ INFO ]  - In room with id: %1 correcting special exit \"%2\" that\n"
                                         "was to room with an exit to invalid room: %3 to now go\n"
                                         "to: %4.")
                                              .arg(id)
                                              .arg(exitName)
                                              .arg(exitRoomId)
                                              .arg(roomRemapping.value(exitRoomId));
                    mpRoomDB->mpMap->postMessage(infoMsg);
                }
                mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                                    tr(R"([ INFO ]  - Room needed correcting of special exit "%1" that was to room with an exit to invalid room: %2 to now go to: %3.)")
                                                            .arg(exitName)
                                                            .arg(exitRoomId)
                                                            .arg(roomRemapping.value(exitRoomId)));
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
            int exitRoomId = it.value();
            QString exitName = it.key();

            if (exitRoomId > 0) {
                // A real exit - should have a real destination
                if (Q_UNLIKELY(!mpRoomDB->getRoom(exitRoomId))) {
                    // But it doesn't exist
                    QString auditKey = QStringLiteral("audit.removed_valid_but_missing_special_exit.%1").arg(exitName);
                    if (mudlet::self()->showMapAuditErrors()) {
                        QString warnMsg = tr("[ WARN ]  - Room with id: %1 has a special exit \"%2\" with an\n"
                                             "exit to: %3 but that room does not exist.  The exit will\n"
                                             "be removed (but the destination room id will be stored in\n"
                                             "the room user data under a key:\n"
                                             "\"%4\").")
                                                  .arg(id)
                                                  .arg(exitName)
                                                  .arg(exitRoomId)
                                                  .arg(auditKey);
                        mpRoomDB->mpMap->postMessage(warnMsg);
                    }
                    mpRoomDB->mpMap->appendRoomErrorMsg(
                            id,
                            tr(R"([ WARN ]  - Room has a special exit "%1" with an exit to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key:"%3").)")
                                    .arg(exitName)
                                    .arg(exitRoomId)
                                    .arg(auditKey),
                            true);
                    userData.insert(auditKey, QString::number(exitRoomId));
                    it.remove();

                    // Remove the corresponding things from the pools of things
                    // that have to be checked:
                    // TODO: Add additional warnings if we ARE deleting any data in following
                    exitWeights.remove(exitName);
                    doors.remove(exitName);
                    customLines.remove(exitName);
                    customLinesColor.remove(exitName);
                    customLinesStyle.remove(exitName);
                    customLinesArrow.remove(exitName);
                    exitWeightsCopy.remove(exitName);
                    doorsCopy.remove(exitName);
                    customLinesCopy.remove(exitName);
                    customLinesColorCopy.remove(exitName);
                    customLinesStyleCopy.remove(exitName);
                    customLinesArrowCopy.remove(exitName);
                } else {
                    // Exit id is for a room that DOES exist and is in the valid range
                    // So remove from the pools of things to check all the things that
                    // CAN be associated with this exit direction:
                    exitWeightsCopy.remove(exitName);
                    doorsCopy.remove(exitName);
                    customLinesCopy.remove(exitName);
                    customLinesColorCopy.remove(exitName);
                    customLinesStyleCopy.remove(exitName);
                    customLinesArrowCopy.remove(exitName);
                }
            } else {
                // < 1 and not renumbered because the bad room Id DID NOT exist
                QString auditKey = QStringLiteral("audit.removed_invalid_special_exit.%1").arg(exitName);
                userData.insert(auditKey, QString::number(exitRoomId));
                if (mudlet::self()->showMapAuditErrors()) {
                    QString infoMsg = tr("[ INFO ]  - In room with id: %1 special exit \"%2\"\n"
                                         "that was to room with an invalid room: %3 that does not exist.\n"
                                         "The exit will be removed (the bad destination room id will be stored in the\n"
                                         "room user data under a key:\n"
                                         "\"%4\").")
                                              .arg(id)
                                              .arg(exitName)
                                              .arg(exitRoomId)
                                              .arg(auditKey);
                    mpRoomDB->mpMap->postMessage(infoMsg);
                }
                mpRoomDB->mpMap->appendRoomErrorMsg(
                        id,
                        tr(R"([ INFO ]  - Room had special exit "%1" that was to room with an invalid room: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:"%3").)")
                                .arg(exitName)
                                .arg(exitRoomId)
                                .arg(auditKey),
                        true);
                it.remove();
                // We cannot have a door or anything else on a non-existent special exit
                doors.remove(exitName);
                exitWeights.remove(exitName);
                customLines.remove(exitName);
                customLinesColor.remove(exitName);
                customLinesStyle.remove(exitName);
                customLinesArrow.remove(exitName);
                doorsCopy.remove(exitName);
                exitWeightsCopy.remove(exitName);
                customLinesCopy.remove(exitName);
                customLinesColorCopy.remove(exitName);
                customLinesStyleCopy.remove(exitName);
                customLinesArrowCopy.remove(exitName);
            }
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
            QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus door items that were removed:\n"
                                 "%2.")
                                      .arg(id)
                                      .arg(extras.join(QStringLiteral(", ")));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr("[ INFO ]  - Room had one or more surplus door items that were removed:"
                                               "%1.")
                                                    .arg(extras.join(QStringLiteral(", "))),
                                            true);
    }

    // ExitWeights:
    if (!exitWeightsCopy.isEmpty()) {
        QStringList extras;
        QMapIterator<QString, int> itSpareExitWeight(exitWeightsCopy);
        while (itSpareExitWeight.hasNext()) {
            itSpareExitWeight.next();
            exitWeights.remove(itSpareExitWeight.key());
            extras.append(QStringLiteral("\"%1\"(%2)").arg(itSpareExitWeight.key()).arg(itSpareExitWeight.value()));
        }
        if (mudlet::self()->showMapAuditErrors()) {
            QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus weight items that were removed:\n"
                                 "%2.")
                                      .arg(id)
                                      .arg(extras.join(QStringLiteral(", ")));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr("[ INFO ]  - Room had one or more surplus weight items that were removed: "
                                               "%1.")
                                                    .arg(extras.join(QStringLiteral(", "))),
                                            true);
    }

    // ExitLocks:
    if (!exitLocksCopy.isEmpty()) {
        QStringList extras;
        QSetIterator<int> itSpareExitLock(exitLocksCopy);
        while (itSpareExitLock.hasNext()) {
            int dirCode = itSpareExitLock.next();
            extras.append(dirCodeToDisplayName(dirCode));
            exitLocks.removeAll(dirCode);
        }
        if (mudlet::self()->showMapAuditErrors()) {
            QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus exit lock items that were removed:\n"
                                 "%2.")
                                      .arg(id)
                                      .arg(extras.join(QStringLiteral(", ")));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id,
                                            tr("[ INFO ]  - Room had one or more surplus exit lock items that were removed: "
                                               "%1.")
                                                    .arg(extras.join(QStringLiteral(", "))),
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
                QString infoMsg = tr("[ INFO ]  - In room with id: %1 found one or more surplus custom line elements that\n"
                                     "were removed: %2.")
                                          .arg(id)
                                          .arg(extras.join(QStringLiteral(", ")));
                mpRoomDB->mpMap->postMessage(infoMsg);
            }
            mpRoomDB->mpMap->appendRoomErrorMsg(id, tr("[ INFO ]  - Room had one or more surplus custom line elements that were removed: %1.").arg(extras.join(QStringLiteral(", "))), true);
        }
    }
}

void TRoom::auditExit(int& exitRoomId,                     // Reference to where exit goes to
                      const int dirCode,                   // DIR_xxx code for this exit - to access stubs & locks
                      const QString displayName,           // What to present as the name of the exit
                      const QString exitKey,               // To access doors, weights and custom exit line elements
                      QMap<QString, int>& exitWeightsPool, // References to working copies of things - valid ones will be removed
                      QSet<int>& exitStubsPool,
                      QSet<int>& exitLocksPool,
                      QMap<QString, int>& doorsPool,
                      QMap<QString, QList<QPointF>>& customLinesPool,
                      QMap<QString, QColor>& customLinesColorPool,
                      QMap<QString, Qt::PenStyle>& customLinesStylePool,
                      QMap<QString, bool>& customLinesArrowPool,
                      const QHash<int, int> roomRemapping)
{
    if (roomRemapping.contains(exitRoomId)) {
        QString auditKey = QStringLiteral("audit.remapped_exit.%1").arg(dirCode);
        userData.insert(auditKey, QString::number(exitRoomId));
        if (mudlet::self()->showMapAuditErrors()) {
            QString infoMsg = tr("[ INFO ]  - In room with id: %1 correcting exit \"%2\" that was to room with\n"
                                 "an exit to invalid room: %3 to now go to: %4.")
                                      .arg(id)
                                      .arg(displayName)
                                      .arg(exitRoomId)
                                      .arg(roomRemapping.value(exitRoomId));
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(
                id, tr(R"([ INFO ]  - Correcting exit "%1" that was to invalid room id: %2 to now go to: %3.)").arg(displayName).arg(exitRoomId).arg(roomRemapping.value(exitRoomId)), true);
        exitRoomId = roomRemapping.value(exitRoomId);
        exitRoomId = roomRemapping.value(exitRoomId);
    }

    if (exitRoomId > 0) {
        // A real exit - should have a real destination, and NOT have a stub
        if (Q_UNLIKELY(!mpRoomDB->getRoom(exitRoomId))) {
            // But it doesn't exist
            QString auditKey = QStringLiteral("audit.made_stub_of_valid_but_missing_exit.%1").arg(dirCode);
            if (mudlet::self()->showMapAuditErrors()) {
                QString warnMsg = tr("[ WARN ]  - Room with id: %1 has an exit \"%2\" to: %3 but that room\n"
                                     "does not exist.  The exit will be removed (but the destination room\n"
                                     "Id will be stored in the room user data under a key:\n"
                                     "\"%4\")\n"
                                     "and the exit will be turned into a stub.")
                                          .arg(id)
                                          .arg(displayName)
                                          .arg(exitRoomId)
                                          .arg(auditKey);
                mpRoomDB->mpMap->postMessage(warnMsg);
            }
            mpRoomDB->mpMap->appendRoomErrorMsg(
                    id,
                    tr(R"([ WARN ]  - Room has an exit "%1" to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key: "%4") and the exit will be turned into a stub.)")
                            .arg(displayName)
                            .arg(exitRoomId)
                            .arg(auditKey),
                    true);
            userData.insert(auditKey, QString::number(exitRoomId));
            if (!exitStubs.contains(dirCode)) {
                // Add a stub (this is so we can retain doors, though exit weights, custom lines and locks will go)
                exitStubs.append(dirCode);
                // Remove a (now valid) stub in this direction from check pool
                exitStubsPool.remove(dirCode);
            }

            exitRoomId = -1;

            // Remove the following things that a stub exit does not handle
            // And eliminate the corresponding things from the pools of things
            // that have to be checked:
            // TODO: Add additional warnings if we ARE deleting any data in following
            exitLocks.removeAll(dirCode);
            exitLocksPool.remove(dirCode);

            exitWeights.remove(exitKey);
            exitWeightsPool.remove(exitKey);

            doorsPool.remove(exitKey); // we now have a stub exit and that can take a door so clear the door from the check pool

            customLines.remove(exitKey);
            customLinesPool.remove(exitKey);

            customLinesColor.remove(exitKey);
            customLinesColorPool.remove(exitKey);

            customLinesStyle.remove(exitKey);
            customLinesStylePool.remove(exitKey);

            customLinesArrow.remove(exitKey);
            customLinesArrowPool.remove(exitKey);
        } else {
            // we do have a valid exit destination room

            // We cannot allow a stub exit at the same time as a real exit:
            if (exitStubs.contains(dirCode)) {
                if (mudlet::self()->showMapAuditErrors()) {
                    QString warnMsg = tr("[ ALERT ] - Room with id: %1 has an exit \"%2\" to: %3 but also\n"
                                         "has a stub exit!  As a real exit precludes a stub, the latter will\n"
                                         "be removed.")
                                              .arg(id)
                                              .arg(displayName)
                                              .arg(exitRoomId);
                    mpRoomDB->mpMap->postMessage(warnMsg);
                }
                mpRoomDB->mpMap->appendRoomErrorMsg(
                        id,
                        tr(R"([ ALERT ] - Room has an exit "%1" to: %2 but also has a stub exit in the same direction!  As a real exit precludes a stub, the latter will be removed.)")
                                .arg(displayName)
                                .arg(exitRoomId),
                        true);
                exitStubs.removeAll(dirCode);
                exitStubsPool.remove(dirCode); // Remove the stub in this direction from check pool as we have handled it
            }

            // Exit id is for a room that DOES exist and is in the valid range
            // So remove from the pools of things to check all the things CAN be
            // associated with this exit direction:
            exitLocksPool.remove(dirCode);
            exitWeightsPool.remove(exitKey);
            doorsPool.remove(exitKey);
            customLinesPool.remove(exitKey);
            customLinesColorPool.remove(exitKey);
            customLinesStylePool.remove(exitKey);
            customLinesArrowPool.remove(exitKey);
        }
    } else if (exitRoomId == -1) {
        // No exit - so do we have a stub?
        if (exitStubs.contains(dirCode)) {
            exitStubsPool.remove(dirCode); // Remove the stub in this direction from check pool as we have handled it
        } else {
            // If NOT we cannot have a door
            doors.remove(exitKey);
        }
        // We have handled whether we can have a door (if there IS a stub) or not (if not)
        // so remove it from the check pool as we have handled it
        doorsPool.remove(exitKey);

        // Whether we do or not have a stub exit we cannot have a lock, custom
        // line or a weight - so remove them if they exist:
        exitLocks.removeAll(dirCode);
        exitWeights.remove(exitKey);
        customLines.remove(exitKey);
        customLinesColor.remove(exitKey);
        customLinesStyle.remove(exitKey);
        customLinesArrow.remove(exitKey);
        // Whether we have a stub or not we have handled all the things that we
        // want to check the existence of so take them out of the pools of
        // things left to check after all the exits have been looked at
        exitLocksPool.remove(dirCode);
        exitWeightsPool.remove(exitKey);
        customLinesPool.remove(exitKey);
        customLinesColorPool.remove(exitKey);
        customLinesStylePool.remove(exitKey);
        customLinesArrowPool.remove(exitKey);
    } else {
        // either 0 or < -1 and not renumbered because the bad room Id DID NOT
        // exist, there could be a "double fault" in that there is also a stub
        // exit, but that will be masked as we turn the exit into a stub anyhow.
        QString auditKey = QStringLiteral("audit.made_stub_of_invalid_exit.%1").arg(dirCode);
        userData.insert(auditKey, QString::number(exitRoomId));
        QString infoMsg;
        if (mudlet::self()->showMapAuditErrors()) {
            infoMsg = tr("[ INFO ]  - In room with id: %1 exit \"%2\" that was to room with an invalid\n"
                         "room: %3 that does not exist.  The exit will be removed (the bad destination\n"
                         "room id will be stored in the room user data under a key:\n"
                         "\"%4\")\n"
                         "and the exit will be turned into a stub.")
                              .arg(id)
                              .arg(displayName)
                              .arg(exitRoomId)
                              .arg(auditKey);
        }
        QString logMsg =
                tr(R"([ INFO ]  - Room exit "%1" that was to a room with an invalid id: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:"%4") and the exit will be turned into a stub.)")
                        .arg(displayName)
                        .arg(exitRoomId)
                        .arg(auditKey);
        exitRoomId = -1;

        if (!exitStubs.contains(dirCode)) {
            // Add the stub
            exitStubs.append(dirCode);
        }
        exitStubsPool.remove(dirCode); // Remove the stub in this direction from check pool as we have handled it

        if (exitLocks.contains(dirCode)) {
            QString auditKeyLocked = QStringLiteral("audit.invalid_exit.%1.isLocked").arg(dirCode);
            userData.insert(auditKeyLocked, QStringLiteral("true"));
            if (mudlet::self()->showMapAuditErrors()) {
                infoMsg.append(tr("\nIt was locked, this is recorded as user data with key:\n"
                                  "\"%1\".")
                                       .arg(auditKeyLocked));
            }
            logMsg.append(tr(R"(  It was locked, this is recorded as user data with key: "%1".)").arg(auditKeyLocked));
            exitLocks.removeAll(dirCode);
        }

        if (exitWeights.contains(exitKey)) {
            QString auditKeyWeight = QStringLiteral("audit.invalid_exit.%1.weight").arg(dirCode);
            userData.insert(auditKeyWeight, QString::number(exitWeights.value(exitKey)));
            if (mudlet::self()->showMapAuditErrors()) {
                infoMsg.append(tr("\nIt had a weight, this is recorded as user data with key:\n"
                                  "\"%1\".")
                                       .arg(auditKeyWeight));
            }
            logMsg.append(tr(R"(  It had a weight, this is recorded as user data with key: "%1".)").arg(auditKeyWeight));
            exitWeights.remove(exitKey);
        }
        if (mudlet::self()->showMapAuditErrors()) {
            mpRoomDB->mpMap->postMessage(infoMsg);
        }
        mpRoomDB->mpMap->appendRoomErrorMsg(id, logMsg, true);


        if (customLines.contains(exitKey)) {
            if (mudlet::self()->showMapAuditErrors()) {
                QString warnMsg = tr("[ WARN ]  - There was a custom exit line associated with the invalid exit but\n"
                                     "it has not been possible to salvage this, it has been lost!");
                mpRoomDB->mpMap->postMessage(warnMsg);
            }
            mpRoomDB->mpMap->appendRoomErrorMsg(
                    id, tr("[ WARN ]  - There was a custom exit line associated with the invalid exit but it has not been possible to salvage this, it has been lost!"), true);
            customLines.remove(exitKey);
        }
        customLinesColor.remove(exitKey);
        customLinesStyle.remove(exitKey);
        customLinesArrow.remove(exitKey);
        // Whether we have a stub or not we have handled all the things that we
        // want to check the existence of so take them out of the pools of
        // things left to check after all the exits have been looked at

        doorsPool.remove(exitKey); // Can still have a door on a stub
        exitLocksPool.remove(dirCode);
        exitWeightsPool.remove(exitKey);
        customLinesPool.remove(exitKey);
        customLinesColorPool.remove(exitKey);
        customLinesStylePool.remove(exitKey);
        customLinesArrowPool.remove(exitKey);
    }
}
