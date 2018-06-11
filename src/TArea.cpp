/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TArea.h"


#include "Host.h"
#include "TConsole.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QElapsedTimer>
#include "post_guard.h"

// Previous direction #defines here did not match the DIR_ defines in TRoom.h,
// but as they are stored in the map file they ought not to be redefined without
// a map file change - however the data that was stored - though wrong was not
// actually used internally so we will just fix it and carry on!  We will use
// the (supplimented by the addition of a code for OTHER) DIR_**** values set
// in the top of TRoom.h.
// FIXME: Modify mapper "painter" code to use "exits" rather than deriving the
// same information each time it is run ???

TArea::TArea(TMap * map , TRoomDB * pRDB )
: min_x(0)
, min_y(0)
, min_z(0)
, max_x(0)
, max_y(0)
, max_z(0)
, gridMode( false )
, isZone( false )
, zoneAreaRef( 0 )
, mpRoomDB( pRDB )
, mIsDirty( false )
, mpMap( map )
{
}

TArea::~TArea()
{
    if (mpRoomDB) {
        mpRoomDB->removeArea((TArea*)this);
    } else {
        qDebug() << "ERROR: In TArea::~TArea(), instance has no mpRoomDB";
    }
}

int TArea::getAreaID()
{
    if (mpRoomDB) {
        return mpRoomDB->getAreaID(this);
    } else {
        qDebug() << "ERROR: TArea::getAreaID() instance has no mpRoomDB, returning -1 as ID";
        return -1;
    }
}

QMap<int, QMap<int, QMultiMap<int, int>>> TArea::koordinatenSystem()
{
    QMap<int, QMap<int, QMultiMap<int, int>>> kS;
    QList<TRoom*> roomList = mpRoomDB->getRoomPtrList();
    for (auto room : roomList) {
        int id = room->getId();
        int x = room->x;
        int y = room->y;
        int z = room->z;
        QMap<int, QMultiMap<int, int>> _y;
        QMultiMap<int, int> _z;
        if (!kS.contains(x)) {
            kS[x] = _y;
        }
        if (!kS[x].contains(y)) {
            kS[x][y] = _z;
        }
        kS[x][y].insertMulti(z, id);
    }
    //qDebug()<< "kS="<<kS;
    return kS;
}

QList<int> TArea::getRoomsByPosition(int x, int y, int z)
{
    QList<int> dL;
    QSetIterator<int> itAreaRoom(rooms);
    while (itAreaRoom.hasNext()) {
        int roomId = itAreaRoom.next();
        TRoom* pR = mpRoomDB->getRoom(roomId);
        if (pR) {
            if (pR->x == x && pR->y == y && pR->z == z) {
                dL.push_back(roomId);
            }
        }
    }
    // Only used by TLuaInterpreter::getRoomsByPosition(), so might as well sort
    // results
    if (dL.size() > 1) {
        std::sort(dL.begin(), dL.end());
    }
    return dL;
}

QList<int> TArea::getCollisionNodes()
{
    QList<int> problems;
    QMap<int, QMap<int, QMultiMap<int, int>>> kS = koordinatenSystem();
    QMapIterator<int, QMap<int, QMultiMap<int, int>>> it(kS);
    while (it.hasNext()) {
        it.next();
        QMap<int, QMultiMap<int, int>> x_val = it.value();
        QMapIterator<int, QMultiMap<int, int>> it2(x_val);
        while (it2.hasNext()) {
            it2.next();
            QMultiMap<int, int> y_val = it2.value();
            QMapIterator<int, int> it3(y_val);
            QList<int> z_coordinates;
            while (it3.hasNext()) {
                it3.next();
                int z = it3.key();
                int node = it3.value();

                if (!z_coordinates.contains(node)) {
                    z_coordinates.append(node);
                } else {
                    if (!problems.contains(node)) {
                        auto it4 = y_val.find(z);
                        problems.append(it4.value());
                        //qDebug()<<"problem node="<<node;
                    }
                }
            }
        }
    }
    return problems;
}

void TArea::determineAreaExitsOfRoom(int id)
{
    if (!mpRoomDB) {
        return;
    }

    TRoom* pR = mpRoomDB->getRoom(id);
    if (!pR) {
        return;
    }

    exits.remove(id);
    int exitId = pR->getNorth();
    // The second term in the ifs below looks for exit room id in TArea
    // instance's own list of rooms which will fail (with a -1 if it is NOT in
    // the list and hence the area.
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTH);
        exits.insertMulti(id, p);
    }
    exitId = pR->getNortheast();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHEAST);
        exits.insertMulti(id, p);
    }
    exitId = pR->getEast();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_EAST);
        exits.insertMulti(id, p);
    }
    exitId = pR->getSoutheast();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHEAST);
        exits.insertMulti(id, p);
    }
    exitId = pR->getSouth();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTH);
        exits.insertMulti(id, p);
    }
    exitId = pR->getSouthwest();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHWEST);
        exits.insertMulti(id, p);
    }
    exitId = pR->getWest();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_WEST);
        exits.insertMulti(id, p);
    }
    exitId = pR->getNorthwest();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHWEST);
        exits.insertMulti(id, p);
    }
    exitId = pR->getUp();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_UP);
        exits.insertMulti(id, p);
    }
    exitId = pR->getDown();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_DOWN);
        exits.insertMulti(id, p);
    }
    exitId = pR->getIn();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_IN);
        exits.insertMulti(id, p);
    }
    exitId = pR->getOut();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_OUT);
        exits.insertMulti(id, p);
    }
    const QMap<int, QString> otherMap = pR->getOtherMap();
    QMapIterator<int, QString> it(otherMap);
    while (it.hasNext()) {
        it.next();
        int _exit = it.key();
        TRoom* pO = mpRoomDB->getRoom(_exit);
        if (pO) {
            if (pO->getArea() != getAreaID()) {
                QPair<int, int> p = QPair<int, int>(pO->getId(), DIR_OTHER);
                exits.insertMulti(id, p);
            }
        }
    }
}

void TArea::determineAreaExits()
{
    exits.clear();
    QSetIterator<int> itRoom(rooms);
    while (itRoom.hasNext()) {
        int id = itRoom.next();
        TRoom* pR = mpRoomDB->getRoom(id);
        if (!pR) {
            continue;
        }

        int exitId = pR->getNorth();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTH);
            exits.insertMulti(id, p);
        }
        exitId = pR->getNortheast();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHEAST);
            exits.insertMulti(id, p);
        }
        exitId = pR->getEast();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_EAST);
            exits.insertMulti(id, p);
        }
        exitId = pR->getSoutheast();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHEAST);
            exits.insertMulti(id, p);
        }
        exitId = pR->getSouth();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTH);
            exits.insertMulti(id, p);
        }
        exitId = pR->getSouthwest();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHWEST);
            exits.insertMulti(id, p);
        }
        exitId = pR->getWest();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_WEST);
            exits.insertMulti(id, p);
        }
        exitId = pR->getNorthwest();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHWEST);
            exits.insertMulti(id, p);
        }
        exitId = pR->getUp();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_UP);
            exits.insertMulti(id, p);
        }
        exitId = pR->getDown();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_DOWN);
            exits.insertMulti(id, p);
        }
        exitId = pR->getIn();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_IN);
            exits.insertMulti(id, p);
        }
        exitId = pR->getOut();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_OUT);
            exits.insertMulti(id, p);
        }
        const QMap<int, QString> otherMap = pR->getOtherMap();
        QMapIterator<int, QString> it(otherMap);
        while (it.hasNext()) {
            it.next();
            int _exit = it.key();
            TRoom* pO = mpRoomDB->getRoom(_exit);
            if (pO) {
                if (pO->getArea() != getAreaID()) {
                    QPair<int, int> p = QPair<int, int>(pO->getId(), DIR_OTHER);
                    exits.insertMulti(id, p);
                }
            }
        }
    }
}

void TArea::fast_calcSpan(int id)
{
    TRoom* pR = mpRoomDB->getRoom(id);
    if (!pR) {
        return;
    }

    int x = pR->x;
    int y = pR->y;
    int z = pR->z;
    if (x > max_x) {
        max_x = x;
    }
    if (x < min_x) {
        min_x = x;
    }
    if (y > max_y) {
        max_y = y;
    }
    if (y < min_y) {
        min_y = y;
    }
    if (z > max_z) {
        max_z = z;
    }
    if (z < min_z) {
        min_z = z;
    }
}

void TArea::addRoom(int id)
{
    TRoom* pR = mpRoomDB->getRoom(id);
    if (pR) {
        if (!rooms.contains(id)) {
            rooms.insert(id);
        } else {
            qDebug() << "TArea::addRoom(" << id << ") No creation! room already exists";
        }
    } else {
        QString error = tr("roomID=%1 does not exist, can not set properties of a non-existent room!").arg(id);
        mpMap->mpHost->mpConsole->printSystemMessage(error);
    }
}

void TArea::calcSpan()
{
    xminEbene.clear();
    yminEbene.clear();
    xmaxEbene.clear();
    ymaxEbene.clear();
    ebenen.clear();

    bool isFirstDone = false;
    QSetIterator<int> itRoom(rooms);
    while (itRoom.hasNext()) {
        int id = itRoom.next();
        TRoom* pR = mpRoomDB->getRoom(id);
        if (!pR) {
            continue;
        }

        if (!isFirstDone) {
            // Only do this initialization for the first valid room
            min_x = pR->x;
            max_x = min_x;
            min_y = pR->y * -1;
            max_y = min_y;
            min_z = pR->z;
            max_z = min_z;
            ebenen.push_back(pR->z);
            xminEbene.insert(pR->z, pR->x);
            xmaxEbene.insert(pR->z, pR->x);
            yminEbene.insert(pR->z, pR->y);
            ymaxEbene.insert(pR->z, pR->y);
            isFirstDone = true;
            continue;
        } else {
            // Already had one valid room so now must check more things

            if (!ebenen.contains(pR->z)) {
                ebenen.push_back(pR->z);
            }

            if (!xminEbene.contains(pR->z)) {
                xminEbene.insert(pR->z, pR->x);
            } else if (pR->x < xminEbene.value(pR->z)) {
                xminEbene.insert(pR->z, pR->x);
            }

            if (pR->x < min_x) {
                min_x = pR->x;
            }

            if (!xmaxEbene.contains(pR->z)) {
                xmaxEbene.insert(pR->z, pR->x);
            } else if (pR->x > xmaxEbene.value(pR->z)) {
                xmaxEbene.insert(pR->z, pR->x);
            }

            if (pR->x > max_x) {
                max_x = pR->x;
            }

            if (!yminEbene.contains(pR->z)) {
                yminEbene.insert(pR->z, (-1 * pR->y));
            } else if ((-1 * pR->y) < yminEbene.value(pR->z)) {
                yminEbene.insert(pR->z, (-1 * pR->y));
            }

            if ((-1 * pR->y) < min_y) {
                min_y = (-1 * pR->y);
            }

            if ((-1 * pR->y) > max_y) {
                max_y = (-1 * pR->y);
            }

            if (!ymaxEbene.contains(pR->z)) {
                ymaxEbene.insert(pR->z, (-1 * pR->y));
            } else if ((-1 * pR->y) > ymaxEbene.value(pR->z)) {
                ymaxEbene.insert(pR->z, (-1 * pR->y));
            }

            if (pR->z < min_z) {
                min_z = pR->z;
            }

            if (pR->z > max_z) {
                max_z = pR->z;
            }
        }
    }

    if (ebenen.size() > 1) {
        // Not essential but it makes debugging a bit clearer if they are sorted
        // The {x|y}{min|max}Ebene are, by definition!
        std::sort(ebenen.begin(), ebenen.end());
    }
}

// Added a second argument to cut-out extremes recalculation if not required
// Currently called from:
// bool TRoom::setArea( int, bool )  -- the second arg there can be used for this
// bool TRoomDB::__removeRoom( int ) -- automatically skipped for area deletion
//                                      (when this would not be needed)
void TArea::removeRoom(int room, bool isToDeferAreaRelatedRecalculations)
{
    static double cumulativeMean = 0.0;
    static quint64 runCount = 0;
    QElapsedTimer timer;
    timer.start();

    // Will use to flag whether some things have to be recalcuated.
    bool isOnExtreme = false;
    if (rooms.contains(room) && !isToDeferAreaRelatedRecalculations) {
        // just a check, if the area DOESN'T have the room then it is not wise
        // to behave as if it did
        TRoom* pR = mpRoomDB->getRoom(room);
        if (pR) {
            // Now see if the room is on an extreme - if it the only room on a
            // particular z-coordinate it will be on all four
            if (xminEbene.contains(pR->z) && xminEbene.value(pR->z) >= pR->x) {
                isOnExtreme = true;
            } else if (xmaxEbene.contains(pR->z) && xmaxEbene.value(pR->z) <= pR->x) {
                isOnExtreme = true;
            } else if (yminEbene.contains(pR->z) && yminEbene.value(pR->z) >= (-1 * pR->y)) {
                isOnExtreme = true;
            } else if (ymaxEbene.contains(pR->z) && ymaxEbene.value(pR->z) <= (-1 * pR->y)) {
                isOnExtreme = true;
            } else if (min_x >= pR->x || min_y >= (-1 * pR->y) || max_x <= pR->x || max_y <= (-1 * pR->y)) {
                isOnExtreme = true;
            }
        }
    }
    rooms.remove(room);
    exits.remove(room);
    if (isOnExtreme) {
        calcSpan();
    }
    quint64 thisTime = timer.nsecsElapsed();
    cumulativeMean += (((thisTime * 1.0e-9) - cumulativeMean) / ++runCount);
    if (runCount % 1000 == 0) {
        qDebug() << "TArea::removeRoom(" << room << ") from Area took" << thisTime * 1.0e-9 << "sec. this time and after" << runCount << "times the average is" << cumulativeMean << "sec.";
    }
}

// Reconstruct the area exit data in a format that actually makes sense - only
// needed until the TRoom & TArea classes can be restructured to store exits
// using the exit direction as a key and the to room as a value instead of vice-versa
const QMultiMap<int, QPair<QString, int>> TArea::getAreaExitRoomData() const
{
    QMultiMap<int, QPair<QString, int>> results;
    QSet<int> roomsWithOtherAreaSpecialExits;

    QMapIterator<int, QPair<int, int>> itAreaExit = exits;
    // First parse the normal exits and also find the rooms where there is at
    // least one special area exit
    while (itAreaExit.hasNext()) {
        itAreaExit.next();
        QPair<QString, int> exitData;
        exitData.second = itAreaExit.value().first;
        switch (itAreaExit.value().second) {
        case DIR_NORTH:     exitData.first = QString("north");                         break;
        case DIR_NORTHEAST: exitData.first = QString("northeast");                     break;
        case DIR_NORTHWEST: exitData.first = QString("northwest");                     break;
        case DIR_SOUTH:     exitData.first = QString("south");                         break;
        case DIR_WEST:      exitData.first = QString("west");                          break;
        case DIR_EAST:      exitData.first = QString("east");                          break;
        case DIR_SOUTHEAST: exitData.first = QString("southeast");                     break;
        case DIR_SOUTHWEST: exitData.first = QString("southwest");                     break;
        case DIR_UP:        exitData.first = QString("up");                            break;
        case DIR_DOWN:      exitData.first = QString("down");                          break;
        case DIR_IN:        exitData.first = QString("in");                            break;
        case DIR_OUT:       exitData.first = QString("out");                           break;
        case DIR_OTHER:     roomsWithOtherAreaSpecialExits.insert(itAreaExit.key());   break;
        default:
            qWarning("TArea::getAreaExitRoomData() Warning: unrecognised exit code %i found for exit from room %i to room %i.", itAreaExit.value().second, itAreaExit.key(), itAreaExit.value().first);
        }
        if (!exitData.first.isEmpty()) {
            results.insert(itAreaExit.key(), exitData);
        }
    }
    // Now have to find the special area exits in the rooms where we know there
    // IS one
    QSetIterator<int> itRoomWithOtherAreaSpecialExit = roomsWithOtherAreaSpecialExits;
    while (itRoomWithOtherAreaSpecialExit.hasNext()) {
        int fromRoomId = itRoomWithOtherAreaSpecialExit.next();
        TRoom* pFromRoom = mpRoomDB->getRoom(fromRoomId);
        if (pFromRoom) {
            QMapIterator<int, QString> itOtherExit = pFromRoom->getOtherMap();
            while (itOtherExit.hasNext()) {
                itOtherExit.next();
                QPair<QString, int> exitData;
                exitData.second = itOtherExit.key();
                TRoom* pToRoom = mpRoomDB->getRoom(exitData.second);
                if (pToRoom && mpRoomDB->getArea(pToRoom->getArea()) != this) {
                    // Note that pToRoom->getArea() is misnamed, should be getAreaId() !
                    if (itOtherExit.value().mid(0, 1) == QStringLiteral("0") || itOtherExit.value().mid(0, 1) == QStringLiteral("1")) {
                        exitData.first = itOtherExit.value().mid(1);
                    } else {
                        exitData.first = itOtherExit.value();
                    }
                    if (!exitData.first.isEmpty()) {
                        results.insert(fromRoomId, exitData);
                    }
                }
            }
        }
    }
    return results;
}
