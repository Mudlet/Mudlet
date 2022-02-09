/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2020-2021 by Stephen Lyons                   *
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


#include "TArea.h"


#include "Host.h"
#include "TConsole.h"
#include "TRoomDB.h"

#include "pre_guard.h"
#include <QBuffer>
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

static const QColor defaultLabelForeground(QColor(0, 0, 0));
static const QColor defaultLabelBackground(QColor(0, 0, 0));
static const int kPixmapDataLineSize = 64;


TArea::TArea(TMap* pMap, TRoomDB* pRDB)
: min_x(0)
, min_y(0)
, min_z(0)
, max_x(0)
, max_y(0)
, max_z(0)
, gridMode(false)
, isZone(false)
, zoneAreaRef(0)
, mpRoomDB(pRDB)
, mIsDirty(false)
, mpMap(pMap)
{
}

TArea::~TArea()
{
    if (mpRoomDB) {
        mpRoomDB->removeArea(this);
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
        kS[x][y].insert(z, id);
    }
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

    mAreaExits.remove(id);
    int exitId = pR->getNorth();
    // The second term in the ifs below looks for exit room id in TArea
    // instance's own list of rooms which will fail (with a -1 if it is NOT in
    // the list and hence the area.
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTH);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getNortheast();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHEAST);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getEast();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_EAST);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getSoutheast();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHEAST);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getSouth();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTH);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getSouthwest();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHWEST);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getWest();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_WEST);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getNorthwest();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHWEST);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getUp();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_UP);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getDown();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_DOWN);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getIn();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_IN);
        mAreaExits.insert(id, p);
    }
    exitId = pR->getOut();
    if (exitId > 0 && !rooms.contains(exitId)) {
        QPair<int, int> p = QPair<int, int>(exitId, DIR_OUT);
        mAreaExits.insert(id, p);
    }
    QMapIterator<QString, int> it(pR->getSpecialExits());
    while (it.hasNext()) {
        it.next();
        TRoom* pO = mpRoomDB->getRoom(it.value());
        if (pO) {
            if (pO->getArea() != getAreaID()) {
                QPair<int, int> p = QPair<int, int>(pO->getId(), DIR_OTHER);
                mAreaExits.insert(id, p);
            }
        }
    }
}

void TArea::determineAreaExits()
{
    mAreaExits.clear();
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
            mAreaExits.insert(id, p);
        }
        exitId = pR->getNortheast();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHEAST);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getEast();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_EAST);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getSoutheast();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHEAST);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getSouth();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTH);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getSouthwest();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_SOUTHWEST);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getWest();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_WEST);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getNorthwest();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_NORTHWEST);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getUp();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_UP);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getDown();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_DOWN);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getIn();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_IN);
            mAreaExits.insert(id, p);
        }
        exitId = pR->getOut();
        if (exitId > 0 && !rooms.contains(exitId)) {
            QPair<int, int> p = QPair<int, int>(exitId, DIR_OUT);
            mAreaExits.insert(id, p);
        }
        QMapIterator<QString, int> itSpecialExit(pR->getSpecialExits());
        while (itSpecialExit.hasNext()) {
            itSpecialExit.next();
            TRoom* pO = mpRoomDB->getRoom(itSpecialExit.value());

            if (pO) {
                if (pO->getArea() != getAreaID()) {
                    QPair<int, int> p = QPair<int, int>(pO->getId(), DIR_OTHER);
                    mAreaExits.insert(id, p);
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
    xminForZ.clear();
    yminForZ.clear();
    xmaxForZ.clear();
    ymaxForZ.clear();
    zLevels.clear();

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
            zLevels.push_back(pR->z);
            xminForZ.insert(pR->z, pR->x);
            xmaxForZ.insert(pR->z, pR->x);
            yminForZ.insert(pR->z, pR->y);
            ymaxForZ.insert(pR->z, pR->y);
            isFirstDone = true;
            continue;
        } else {
            // Already had one valid room so now must check more things

            if (!zLevels.contains(pR->z)) {
                zLevels.push_back(pR->z);
            }

            if (!xminForZ.contains(pR->z)) {
                xminForZ.insert(pR->z, pR->x);
            } else if (pR->x < xminForZ.value(pR->z)) {
                xminForZ.insert(pR->z, pR->x);
            }

            if (pR->x < min_x) {
                min_x = pR->x;
            }

            if (!xmaxForZ.contains(pR->z)) {
                xmaxForZ.insert(pR->z, pR->x);
            } else if (pR->x > xmaxForZ.value(pR->z)) {
                xmaxForZ.insert(pR->z, pR->x);
            }

            if (pR->x > max_x) {
                max_x = pR->x;
            }

            if (!yminForZ.contains(pR->z)) {
                yminForZ.insert(pR->z, (-1 * pR->y));
            } else if ((-1 * pR->y) < yminForZ.value(pR->z)) {
                yminForZ.insert(pR->z, (-1 * pR->y));
            }

            if ((-1 * pR->y) < min_y) {
                min_y = (-1 * pR->y);
            }

            if ((-1 * pR->y) > max_y) {
                max_y = (-1 * pR->y);
            }

            if (!ymaxForZ.contains(pR->z)) {
                ymaxForZ.insert(pR->z, (-1 * pR->y));
            } else if ((-1 * pR->y) > ymaxForZ.value(pR->z)) {
                ymaxForZ.insert(pR->z, (-1 * pR->y));
            }

            if (pR->z < min_z) {
                min_z = pR->z;
            }

            if (pR->z > max_z) {
                max_z = pR->z;
            }
        }
    }

    if (zLevels.size() > 1) {
        // Not essential but it makes debugging a bit clearer if they are sorted
        // The {x|y}{min|max}ForZ are, by definition!
        std::sort(zLevels.begin(), zLevels.end());
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

    // Will use to flag whether some things have to be recalculated.
    bool isOnExtreme = false;
    if (rooms.contains(room) && !isToDeferAreaRelatedRecalculations) {
        // just a check, if the area DOESN'T have the room then it is not wise
        // to behave as if it did
        TRoom* pR = mpRoomDB->getRoom(room);
        if (pR) {
            // Now see if the room is on an extreme - if it the only room on a
            // particular z-coordinate it will be on all four
            if (xminForZ.contains(pR->z) && xminForZ.value(pR->z) >= pR->x) {
                isOnExtreme = true;
            } else if (xmaxForZ.contains(pR->z) && xmaxForZ.value(pR->z) <= pR->x) {
                isOnExtreme = true;
            } else if (yminForZ.contains(pR->z) && yminForZ.value(pR->z) >= (-1 * pR->y)) {
                isOnExtreme = true;
            } else if (ymaxForZ.contains(pR->z) && ymaxForZ.value(pR->z) <= (-1 * pR->y)) {
                isOnExtreme = true;
            } else if (min_x >= pR->x || min_y >= (-1 * pR->y) || max_x <= pR->x || max_y <= (-1 * pR->y)) {
                isOnExtreme = true;
            }
        }
    }
    rooms.remove(room);
    mAreaExits.remove(room);
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

    QMapIterator<int, QPair<int, int>> itAreaExit = mAreaExits;
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
            QMapIterator<QString, int> itSpecialExit(pFromRoom->getSpecialExits());
            while (itSpecialExit.hasNext()) {
                itSpecialExit.next();
                QPair<QString, int> exitData;
                exitData.first = itSpecialExit.key();
                exitData.second = itSpecialExit.value();
                TRoom* pToRoom = mpRoomDB->getRoom(exitData.second);
                if (pToRoom && mpRoomDB->getArea(pToRoom->getArea()) != this) {
                    // Note that pToRoom->getArea() is misnamed, should be getAreaId() !
                    if (!exitData.first.isEmpty()) {
                        results.insert(fromRoomId, exitData);
                    }
                }
            }
        }
    }
    return results;
}

int TArea::createLabelId() const
{
    int labelId = -1;
    do {} while (mMapLabels.contains(++labelId));
    if (labelId < 0) {
        labelId = -1;
    }
    return labelId;
}

void TArea::writeJsonArea(QJsonArray& array) const
{
    QJsonObject areaObj;
    const int id = mpRoomDB->getAreaID(const_cast<TArea*>(this));
    areaObj.insert(QLatin1String("id"), static_cast<double>(id));

    const QJsonValue areaNameValue{mpRoomDB->getAreaNamesMap().value(id)};
    areaObj.insert(QLatin1String("name"), areaNameValue);

    if (gridMode) {
        areaObj.insert(QLatin1String("gridMode"), true);
    }

    writeJsonUserData(areaObj);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QList<int> roomList{rooms.begin(), rooms.end()};
#else
    QList<int> roomList = rooms.toList();
#endif
    int roomCount = roomList.count();
    if (roomCount > 1) {
        std::sort(roomList.begin(), roomList.end());
    }
    areaObj.insert(QLatin1String("roomCount"), roomCount);

    QJsonArray roomsArray;
    int currentRoomCount = 0;
    for (auto roomId : roomList) {
        auto pR = mpRoomDB->getRoom(roomId);
        if (pR) {
            ++currentRoomCount;
            pR->writeJsonRoom(roomsArray);
            if (currentRoomCount % 10 == 0) {
                if (mpMap->incrementJsonProgressDialog(true, true, 10)) {
                    // Cancel has been hit - so give up straight away:
                    return;
                }
            }
        }
    }
    if (currentRoomCount % 10 != 0) {
        // Must add on any remainder otherwise the total will be wrong:
        mpMap->incrementJsonProgressDialog(true, true, currentRoomCount % 10);
    }
    QJsonValue roomsValue{roomsArray};
    areaObj.insert(QLatin1String("rooms"), roomsValue);

    // Process the labels after the rooms so that the first area shows something
    // quickly (from the rooms) even if it has a number of labels to do.

    writeJsonLabels(areaObj);
    QJsonValue areaValue{areaObj};
    array.append(areaValue);
}

std::pair<int, QString> TArea::readJsonArea(const QJsonArray& array, const int areaIndex)
{
    const QJsonObject areaObj{array.at(areaIndex).toObject()};
    const int id = areaObj.value(QLatin1String("id")).toInt();
    const QString name{areaObj.value(QLatin1String("name")).toString()};
    gridMode = areaObj.value(QLatin1String("gridMode")).toBool();
    readJsonUserData(areaObj.value(QLatin1String("userData")).toObject());
    int roomCount = 0;
    for (int roomIndex = 0, total = areaObj.value(QLatin1String("rooms")).toArray().count(); roomIndex < total; ++roomIndex) {
        TRoom* pR = new TRoom(mpRoomDB);
        int roomId = pR->readJsonRoom(areaObj.value(QLatin1String("rooms")).toArray(), roomIndex, id);
        rooms.insert(roomId);
        // This also sets the room id for the TRoom:
        mpRoomDB->addRoom(roomId, pR, true);
        if (++roomCount % 10 == 0) {
            if (mpMap->incrementJsonProgressDialog(false, true, 10)) {
                // Cancel has been hit - so give up straight away:
                return {0, {}};
            }
        }
    }
    if (roomCount % 10 != 0) {
        // Must add on any remainder otherwise the total will be wrong:
        mpMap->incrementJsonProgressDialog(false, true, roomCount % 10);
    }

    if (areaObj.contains(QLatin1String("labels")) && areaObj.value(QLatin1String("labels")).isArray()) {
        readJsonLabels(areaObj);
    }
    return {id, name};
}

void TArea::writeJsonUserData(QJsonObject& obj) const
{
    QJsonObject userDataObj;
    if (mUserData.isEmpty()) {
        // Skip creating a user data array if it will be empty:
        return;
    }
    QMapIterator<QString, QString> itDataItem(mUserData);
    while (itDataItem.hasNext()) {
        itDataItem.next();
        QJsonValue userDataValue{itDataItem.value()};
        userDataObj.insert(itDataItem.key(), userDataValue);
    }
    const QJsonValue userDatasValue{userDataObj};
    obj.insert(QLatin1String("userData"), userDatasValue);
}

// Takes a userData object and parses all its elements
void TArea::readJsonUserData(const QJsonObject& obj)
{
    if (obj.isEmpty()) {
        // Skip doing anything more if there is nothing to do:
        return;
    }

    for (auto& key : obj.keys()) {
        if (obj.value(key).isString()) {
            mUserData.insert(key, obj.value(key).toString());
        }
    }
}

void TArea::writeJsonLabels(QJsonObject& obj) const
{
    if (mMapLabels.isEmpty()) {
        // No labels in this area - so nothing to do
        return;
    }

    QJsonArray labelArray;
    QMapIterator<int, TMapLabel> itMapLabel(mMapLabels);
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        writeJsonLabel(labelArray, itMapLabel.key(), &itMapLabel.value());
        if (mpMap->incrementJsonProgressDialog(true, false, 1)) {
            // Cancel has been hit - so give up straight away:
            return;
        }
    }
    QJsonValue labelsValue{labelArray};
    obj.insert(QLatin1String("labels"), labelsValue);
}

// obj is the (area) container that contains the label array
void TArea::readJsonLabels(const QJsonObject& obj)
{
    const QJsonArray labelsArray = obj.value(QLatin1String("labels")).toArray();

    if (labelsArray.isEmpty()) {
        // No labels at all in this area
        return;
    }

    for (const auto labelValue : labelsArray) {
        readJsonLabel(labelValue.toObject());
        if (mpMap->incrementJsonProgressDialog(false, false, 1)) {
            // Cancel has been hit - so give up straight away:
            return;
        }
    }
}

void TArea::writeJsonLabel(QJsonArray& array, const int id, const TMapLabel* pLabel) const
{
    QJsonObject labelObj;

    labelObj.insert(QLatin1String("id"), static_cast<double>(id));

    writeJson3DCoordinates(labelObj, QLatin1String("coordinates"), pLabel->pos);

    writeJsonSize(labelObj, QLatin1String("size"), pLabel->size);

    if (!(pLabel->text.isEmpty() || !pLabel->text.compare(tr("no text", "Default text if a label is created in mapper with no text")))) {
        // Don't include the text if it is am image:
        QJsonValue textValue{pLabel->text};
        labelObj.insert(QLatin1String("text"), textValue);
    }

    if (!(pLabel->fgColor.red() == defaultLabelForeground.red()
          && pLabel->fgColor.green() == defaultLabelForeground.green()
          && pLabel->fgColor.blue() == defaultLabelForeground.blue()
          && pLabel->fgColor.alpha() == defaultLabelForeground.alpha()
          && pLabel->bgColor.red() == defaultLabelBackground.red()
          && pLabel->bgColor.green() == defaultLabelBackground.green()
          && pLabel->bgColor.blue() == defaultLabelBackground.blue()
          && pLabel->bgColor.alpha() == defaultLabelBackground.alpha())) {

        // For an image the colors are not used and tend to be set to black, if
        // so skip them. Unfortunately because of the way QColour s are
        // assembled the operator== is too picky for our purposes as even the
        // way the colour was put together (color spec type) can make them NOT
        // seem to be the same when we'd think they were...
        QJsonArray colorsArray;
        QJsonObject foregroundColorObj;
        QJsonObject backgroundColorObj;
        TMap::writeJsonColor(foregroundColorObj, pLabel->fgColor);
        TMap::writeJsonColor(backgroundColorObj, pLabel->bgColor);
        QJsonValue foregroundColorValue{foregroundColorObj};
        QJsonValue backgroundColorValue{backgroundColorObj};
        colorsArray.append(foregroundColorValue);
        colorsArray.append(backgroundColorValue);
        QJsonValue colorsValue{colorsArray};
        labelObj.insert(QLatin1String("colors"), colorsValue);
    }

    QList<QByteArray> pixmapData = convertImageToBase64Data(pLabel->pix);
    QJsonArray imageArray;
    for (auto imageLine : pixmapData) {
        const QJsonValue imageLineValue{imageLine.data()};
        imageArray.append(imageLineValue);
    }
    const QJsonValue imageValue{imageArray};
    labelObj.insert(QLatin1String("image"), imageValue);

    // (bool) pLabel->highlight is not saved as it is only used during editing
    labelObj.insert(QLatin1String("showOnTop"), pLabel->showOnTop);
    // Invert the logic here as we are saying "scaled" rather than "unscaled":
    labelObj.insert(QLatin1String("scaledels"), !pLabel->noScaling);

    const QJsonValue labelValue{labelObj};
    array.append(labelValue);
}

void TArea::readJsonLabel(const QJsonObject& labelObj)
{
    TMapLabel label;

    int labelId = labelObj.value(QLatin1String("id")).toInt();

    label.pos = readJson3DCoordinates(labelObj, QLatin1String("coordinates"));

    label.size = readJsonSize(labelObj, QLatin1String("size"));

    if (labelObj.contains(QLatin1String("text")) && labelObj.value(QLatin1String("text")).isString()) {
        label.text = labelObj.value(QLatin1String("text")).toString();
    }

    if (labelObj.contains(QLatin1String("colors")) && labelObj.value(QLatin1String("colors")).isArray() && labelObj.value(QLatin1String("colors")).toArray().size() == 2) {
        // For an image the colors are not used and tend to be set to black, if
        // so skip them. Unfortunately because of the way QColour s are
        // assembled the operator== is too picky for our purposes as even the
        // way the colour was put together (color spec type) can make them NOT
        // seem to be the same when we'd think they were...
        QJsonArray colorsArray = labelObj.value(QLatin1String("colors")).toArray();
        label.fgColor = TMap::readJsonColor(colorsArray.at(0).toObject());
        label.bgColor = TMap::readJsonColor(colorsArray.at(1).toObject());
    } else {
        label.fgColor = defaultLabelForeground;
        label.bgColor = defaultLabelBackground;
    }

    QJsonArray imageArray = labelObj.value(QLatin1String("image")).toArray();
    QList<QByteArray> pixmapData;
    for (int i = 0, total = imageArray.size(); i < total; ++i) {
        pixmapData.append(imageArray.at(i).toString().toLatin1());
    }
    label.pix = convertBase64DataToImage(pixmapData);

    label.showOnTop = labelObj.value(QLatin1String("showOnTop")).toBool();

    label.noScaling = !labelObj.value(QLatin1String("scaledels")).toBool(true);

    mMapLabels.insert(labelId, label);
}

void TArea::writeTwinValues(QJsonObject& obj, const QString& title, const QPointF& point) const
{
    QJsonArray valueArray;
    valueArray.append(static_cast<double>(point.x()));
    valueArray.append(static_cast<double>(point.y()));
    const QJsonValue valuesValue{valueArray};
    obj.insert(title, valuesValue);
}

void TArea::writeJsonSize(QJsonObject& obj, const QString& title, const QSizeF& size) const
{
    QJsonArray valueArray;
    valueArray.append(static_cast<double>(size.width()));
    valueArray.append(static_cast<double>(size.height()));
    const QJsonValue valuesValue{valueArray};
    obj.insert(title, valuesValue);
}

QSizeF TArea::readJsonSize(const QJsonObject& obj, const QString& title) const
{
    QSizeF size;
    if (!obj.value(title).isArray() || obj.value(title).toArray().size() != 2) {
        return size;
    }

    QJsonArray valueArray = obj.value(title).toArray();
    if (valueArray.at(0).isDouble()) {
        size.setWidth(valueArray.at(0).toDouble());
    }
    if (valueArray.at(1).isDouble()) {
        size.setHeight(valueArray.at(1).toDouble());
    }
    return size;
}

void TArea::writeJson3DCoordinates(QJsonObject& obj, const QString& title, const QVector3D& vector) const
{
    QJsonArray valueArray;
    valueArray.append(static_cast<double>(vector.x()));
    valueArray.append(static_cast<double>(vector.y()));
    valueArray.append(static_cast<double>(vector.z()));
    const QJsonValue valuesValue{valueArray};
    obj.insert(title, valuesValue);
}

QVector3D TArea::readJson3DCoordinates(const QJsonObject& obj, const QString& title) const
{
    QVector3D position;
    if (!obj.value(title).isArray() || obj.value(title).toArray().size() != 3) {
        return position;
    }

    QJsonArray valueArray = obj.value(title).toArray();
    if (valueArray.at(0).isDouble()) {
        position.setX(valueArray.at(0).toDouble());
    }
    if (valueArray.at(1).isDouble()) {
        position.setY(valueArray.at(1).toDouble());
    }
    if (valueArray.at(2).isDouble()) {
        position.setZ(valueArray.at(2).toDouble());
    }
    return position;
}

// Serialize a QPixmap in a format that can be conveyed in a text file...
QList<QByteArray> TArea::convertImageToBase64Data(const QPixmap& pixmap) const
{
    QBuffer imageInputBuffer;

    imageInputBuffer.open(QIODevice::WriteOnly);
    // Go for maximum compression - for the smallest amount of data, the second
    // argument is a const char[] so does not require a QString wrapper:
    pixmap.save(&imageInputBuffer, "PNG", 0);
    QBuffer imageOutputBuffer;
    QByteArray encodedImageArray{imageInputBuffer.buffer().toBase64()};
    imageInputBuffer.close();
    imageOutputBuffer.setBuffer(&encodedImageArray);
    imageOutputBuffer.open(QIODevice::ReadOnly);

    QList<QByteArray> pixmapArray;
    // Extract the image into lines of bytes (unsigned chars):
    char lineBuffer[kPixmapDataLineSize + 1];
    qint64 bytesRead = kPixmapDataLineSize;
    for (int i = 0, total = imageOutputBuffer.size(); bytesRead == kPixmapDataLineSize && i < total; i += kPixmapDataLineSize) {
        bytesRead = imageOutputBuffer.read(lineBuffer, kPixmapDataLineSize);
        if (bytesRead) {
            lineBuffer[bytesRead] = '\0';
            pixmapArray.append(lineBuffer);
        }
    }

    return pixmapArray;
}

QPixmap TArea::convertBase64DataToImage(const QList<QByteArray>& pixmapArray) const
{
    QByteArray decodedImageArray = QByteArray::fromBase64(pixmapArray.join());
    QPixmap pixmap;
    pixmap.loadFromData(decodedImageArray);

    return pixmap;
}
