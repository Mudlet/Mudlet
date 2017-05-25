/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2017 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TRoomDB.h"

#include "Host.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "mudlet.h"


#include "pre_guard.h"
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QStringBuilder>
#include "post_guard.h"


TRoomDB::TRoomDB( TMap * pMap )
: mpMap( pMap )
, mpTempRoomDeletionSet( 0 )
, mUnnamedAreaName( tr( "Unnamed Area" ) )
, mDefaultAreaName( tr( "Default Area" ) )
{
    // Ensure the default area is created, the area/areaName items that get
    // created here will get blown away when a map is loaded but that is expected...
    addArea(-1, mDefaultAreaName);
}

TRoom* TRoomDB::getRoom(int id)
{
    if (id < 0)
        return 0;
    auto i = rooms.find(id);
    if (i != rooms.end() && i.key() == id) {
        return i.value();
    }
    return 0;
}

bool TRoomDB::addRoom(int id)
{
    qDebug() << "addRoom(" << id << ")";
    if (!rooms.contains(id) && id > 0) {
        rooms[id] = new TRoom(this);
        rooms[id]->setId(id);
        // there is no point to update the entranceMap here, as the room has no exit information
        return true;
    } else {
        if (id <= 0) {
            QString error = QString("illegal room id=%1. roomID must be > 0").arg(id);
            mpMap->logError(error);
        }
        return false;
    }
}

bool TRoomDB::addRoom(int id, TRoom* pR, bool isMapLoading)
{
    if (!rooms.contains(id) && id > 0 && pR) {
        rooms[id] = pR;
        pR->setId(id);
        updateEntranceMap(pR, isMapLoading);
        return true;
    } else {
        return false;
    }
}

void TRoomDB::deleteValuesFromEntranceMap(int value)
{
    QList<int> keyList = entranceMap.keys();
    QList<int> valueList = entranceMap.values();
    QList<uint> deleteEntries;
    int index = valueList.indexOf(value);
    while (index != -1) {
        deleteEntries.append(index);
        index = valueList.indexOf(value, index + 1);
    }
    for (int i = deleteEntries.size() - 1; i >= 0; --i) {
        entranceMap.remove(keyList.at(deleteEntries.at(i)), valueList.at(deleteEntries.at(i)));
    }
}

void TRoomDB::deleteValuesFromEntranceMap(QSet<int>& valueSet)
{
    QElapsedTimer timer;
    timer.start();
    QList<int> keyList = entranceMap.keys();
    QList<int> valueList = entranceMap.values();
    QList<uint> deleteEntries;
    foreach (int roomId, valueSet) {
        int index = valueList.indexOf(roomId);
        while (index >= 0) {
            deleteEntries.append(index);
            index = valueList.indexOf(roomId, index + 1);
        }
    }
    for (unsigned int entry : deleteEntries) {
        entranceMap.remove(keyList.at(entry), valueList.at(entry));
    }
    qDebug() << "TRoomDB::deleteValuesFromEntranceMap() with a list of:" << valueSet.size() << "items, run time:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}

void TRoomDB::updateEntranceMap(int id)
{
    TRoom* pR = getRoom(id);
    updateEntranceMap(pR);
}

void TRoomDB::updateEntranceMap(TRoom* pR, bool isMapLoading)
{
    static bool showDebug = false; // Enable this at runtime (set a breakpoint on it) for debugging!

    // entranceMap maps the room to rooms it has a viable exit to. So if room b and c both have
    // an exit to room a, upon deleting room a we want a map that allows us to find
    // room b and c efficiently.
    // So we create a mapping like: {room_a: room_b, room_a: room_c}. This allows us to delete
    // rooms and know which other rooms are impacted by this change in a single lookup.
    if (pR) {
        int id = pR->getId();
        QHash<int, int> exits = pR->getExits();
        QList<int> toExits = exits.keys();
        QString values;
        // to update this we need to iterate the entire entranceMap and remove invalid
        // connections. I'm not sure if this is efficient for every update, and given
        // that we check for rooms existance when the map is used, we'll deal with
        // possible spurious exits for now.
        // entranceMap.remove(id); // <== not what is wanted
        // We need to remove all values == id NOT keys == id, so try and do that
        // now. SlySven

        if (!isMapLoading) {
            deleteValuesFromEntranceMap(id); // When LOADING a map, will never need to do this
        }
        for (int toExit : toExits) {
            if (showDebug) {
                values.append(QStringLiteral("%1,").arg(toExit));
            }
            if (!entranceMap.contains(toExit, id)) {
                // entranceMap is a QMultiHash, so multiple, identical entries is
                // more than possible - it was actually happening and making
                // entranceMap get larger than needed...!
                entranceMap.insert(toExit, id);
            }
        }
        if (showDebug) {
            if (!values.isEmpty()) {
                values.chop(1);
            }
            if (values.isEmpty()) {
                qDebug() << "TRoomDB::updateEntranceMap(TRoom * pR) called for room with id:" << id << ", it is not an Entrance for any Rooms.";
            } else {
                qDebug() << "TRoomDB::updateEntranceMap(TRoom * pR) called for room with id:" << id << ", it is an Entrance for Room(s):" << values << ".";
            }
        }
    }
}

// this is call by TRoom destructor only
bool TRoomDB::__removeRoom(int id)
{
    static QMultiHash<int, int> _entranceMap; // Make it persistant - for multiple room deletions
    static bool isBulkDelete = false;
    // Gets set / reset by mpTempRoomDeletionSet being non-null, used to setup
    // _entranceMap the first time around for multi-room deletions

    TRoom* pR = getRoom(id);
    // This will FAIL during map deletion as TRoomDB::rooms has already been
    // zapped, so can use to skip everything...
    if (pR) {
        if (mpTempRoomDeletionSet && mpTempRoomDeletionSet->size() > 1) { // We are deleting multiple rooms
            if (!isBulkDelete) {
                _entranceMap = entranceMap;
                _entranceMap.detach(); // MUST take a deep copy of the data
                isBulkDelete = true;   // But only do it the first time for a bulk delete
            }
        } else {                // We are deleting a single room
            if (isBulkDelete) { // Last time was a bulk delete but it isn't one now
                isBulkDelete = false;
            }
            _entranceMap.clear();
            _entranceMap = entranceMap; // Refresh our local copy
            _entranceMap.detach();      // MUST take a deep copy of the data
        }

        // FIXME: make a proper exit controller so we don't need to do all these if statements
        // Remove the links from the rooms entering this room
        QMultiHash<int, int>::const_iterator i = _entranceMap.find(id);
        // The removeAllSpecialExitsToRoom below modifies the entranceMap - and
        // it is unsafe to modify (use copy operations on) something that an STL
        // iterator is active on - see "Implicit sharing iterator problem" in
        // "Container Class | Qt 5.x Core" - this is now avoid by taking a deep
        // copy and iterating through that instead whilst modifying the original
        while (i != entranceMap.end() && i.key() == id) {
            if (i.value() == id || (mpTempRoomDeletionSet && mpTempRoomDeletionSet->size() > 1 && mpTempRoomDeletionSet->contains(i.value()))) {
                ++i;
                continue; // Bypass rooms we know are also to be deleted
            }
            TRoom* r = getRoom(i.value());
            if (r) {
                if (r->getNorth() == id)
                    r->setNorth(-1);
                if (r->getNortheast() == id)
                    r->setNortheast(-1);
                if (r->getNorthwest() == id)
                    r->setNorthwest(-1);
                if (r->getEast() == id)
                    r->setEast(-1);
                if (r->getWest() == id)
                    r->setWest(-1);
                if (r->getSouth() == id)
                    r->setSouth(-1);
                if (r->getSoutheast() == id)
                    r->setSoutheast(-1);
                if (r->getSouthwest() == id)
                    r->setSouthwest(-1);
                if (r->getUp() == id)
                    r->setUp(-1);
                if (r->getDown() == id)
                    r->setDown(-1);
                if (r->getIn() == id)
                    r->setIn(-1);
                if (r->getOut() == id)
                    r->setOut(-1);
                r->removeAllSpecialExitsToRoom(id);
            }
            ++i;
        }
        rooms.remove(id);
        // FIXME: make hashTable a bimap
        QList<QString> keyList = hashTable.keys();
        QList<int> valueList = hashTable.values();
        for (int i = 0; i < valueList.size(); i++) {
            if (valueList[i] == id) {
                hashTable.remove(keyList[i]);
            }
        }
        int areaID = pR->getArea();
        TArea* pA = getArea(areaID);
        if (pA)
            pA->removeRoom(id);
        if ((!mpTempRoomDeletionSet) || mpTempRoomDeletionSet->size() == 1) { // if NOT deleting multiple rooms
            entranceMap.remove(id);                                           // Only removes matching keys
            deleteValuesFromEntranceMap(id);                                  // Needed to remove matching values
        }
        // Because we clear the graph in initGraph which will be called
        // if mMapGraphNeedsUpdate is true -- we don't need to
        // remove the vertex using clear_vertex and remove_vertex here
        mpMap->mMapGraphNeedsUpdate = true;
        return true;
    }
    return false;
}

bool TRoomDB::removeRoom(int id)
{
    if (rooms.contains(id) && id > 0) {
        if (mpMap->mRoomIdHash.value(mpMap->mpHost->getName()) == id) {
            // Now we store mRoomId for each profile, we must remove any where
            // this room was used
            QList<QString> profilesWithUserInThisRoom = mpMap->mRoomIdHash.keys(id);
            foreach (QString key, profilesWithUserInThisRoom) {
                mpMap->mRoomIdHash[key] = 0;
            }
        }
        if (mpMap->mTargetID == id) {
            mpMap->mTargetID = 0;
        }
        TRoom* pR = getRoom(id);
        delete pR;
        return true;
    }
    return false;
}

void TRoomDB::removeRoom(QSet<int>& ids)
{
    QElapsedTimer timer;
    timer.start();
    QSet<int> deletedRoomIds;
    mpTempRoomDeletionSet = &ids; // Will activate "bulk room deletion" code
                                  // When used by TLuaInterpreter::deleteArea()
                                  // via removeArea(int) the list of rooms to
                                  // delete - as suppplied by the reference
                                  // type argument IS NOT CONSTANT - it is
                                  // ALTERED by TArea::removeRoom( int room )
                                  // for each room that is removed
    quint64 roomcount = mpTempRoomDeletionSet->size();
    while (!mpTempRoomDeletionSet->isEmpty()) {
        int deleteRoomId = *(mpTempRoomDeletionSet->constBegin());
        TRoom* pR = getRoom(deleteRoomId);
        if (pR) {
            deletedRoomIds.insert(deleteRoomId);
            delete pR;
        }
        mpTempRoomDeletionSet->remove(deleteRoomId);
    }
    foreach (int deleteRoomId, deletedRoomIds) {
        entranceMap.remove(deleteRoomId); // This has been deferred from __removeRoom()
    }
    deleteValuesFromEntranceMap(deletedRoomIds);
    mpTempRoomDeletionSet->clear();
    mpTempRoomDeletionSet = 0;
    qDebug() << "TRoomDB::removeRoom(QList<int>) run time for" << roomcount << "rooms:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}

bool TRoomDB::removeArea(int id)
{
    if (TArea* pA = areas.value(id)) {
        if (!rooms.isEmpty()) {
            // During map deletion rooms will already
            // have been cleared so this would not
            // be wanted to be done in that case.
            removeRoom(pA->rooms);
        }
        // During map deletion areaNamesMap will
        // already have been cleared !!!
        areaNamesMap.remove(id);
        // This means areas.clear() is not needed during map
        // deletion
        areas.remove(id);

        mpMap->mMapGraphNeedsUpdate = true;
        return true;
    } else if (areaNamesMap.contains(id)) {
        // Handle corner case where the area name was created but not used
        areaNamesMap.remove(id);
        return true;
    }
    return false;
}

bool TRoomDB::removeArea(QString name)
{
    if (areaNamesMap.values().contains(name)) {
        return removeArea(areaNamesMap.key(name)); // i.e. call the removeArea(int) method
    } else {
        return false;
    }
}

void TRoomDB::removeArea(TArea* pA)
{
    if (!pA) {
        qWarning() << "TRoomDB::removeArea(TArea *) Warning - attempt to remove an area with a NULL TArea pointer!";
        return;
    }

    int areaId = areas.key(pA, 0);
    if (areaId == areas.key(pA, -1)) {
        // By testing twice with different default keys to return if value NOT
        // found, we can be certain we have an actual valid value
        removeArea(areaId);
    } else {
        qWarning() << "TRoomDB::removeArea(TArea *) Warning - attempt to remove an area NOT in TRoomDB::areas!";
    }
}

int TRoomDB::getAreaID(TArea* pA)
{
    return areas.key(pA);
}

void TRoomDB::buildAreas()
{
    QElapsedTimer timer;
    timer.start();
    QHashIterator<int, TRoom*> it(rooms);
    while (it.hasNext()) {
        it.next();
        int id = it.key();
        TRoom* pR = getRoom(id);
        if (!pR)
            continue;
        TArea* pA = getArea(pR->getArea());
        if (!pA) {
            areas[pR->getArea()] = new TArea(mpMap, this);
        }
    }

    // if the area has been created without any rooms add the area ID
    QMapIterator<int, QString> it2(areaNamesMap);
    while (it2.hasNext()) {
        it2.next();
        int id = it2.key();
        if (!areas.contains(id)) {
            areas[id] = new TArea(mpMap, this);
        }
    }
    qDebug() << "TRoomDB::buildAreas() run time:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}


const QList<TRoom*> TRoomDB::getRoomPtrList()
{
    return rooms.values();
}

QList<int> TRoomDB::getRoomIDList()
{
    return rooms.keys();
}

TArea* TRoomDB::getArea(int id)
{
    //area id of -1 is a room in the "void", 0 is a failure
    if (id > 0 || id == -1) {
        return areas.value(id, 0);
    } else {
        return 0;
    }
}

// Used by TMap::audit() - can detect and return areas with normally invalids Id (less than -1 or zero)!
TArea* TRoomDB::getRawArea(int id, bool* isValid = 0)
{
    if (areas.contains(id)) {
        if (isValid) {
            *isValid = true;
        }
        return areas.value(id);
    } else {
        if (isValid) {
            *isValid = false;
        }
        return 0;
    }
}

bool TRoomDB::setAreaName(int areaID, QString name)
{
    if (areaID < 1) {
        qWarning() << "TRoomDB::setAreaName((int)areaID, (QString)name): WARNING: Suspect area id:" << areaID << "supplied.";
        return false;
    }
    if (name.isEmpty()) {
        qWarning() << "TRoomDB::setAreaName((int)areaID, (QString)name): WARNING: Empty name supplied.";
        return false;
    } else if (areaNamesMap.values().count(name) > 0) {
        // That name is already IN the areaNamesMap
        if (areaNamesMap.value(areaID) == name) {
            // The trivial case, the given areaID already IS that name
            return true;
        } else {
            qWarning() << "TRoomDB::setAreaName((int)areaID, (QString)name): WARNING: Duplicate name supplied" << name << "- that is not permitted any longer!";
            return false;
        }
    }
    areaNamesMap[areaID] = name;
    // This creates a NEW area name with given areaID if the ID was not
    // previously used - but the TArea only gets created if the user manually
    // creates it with TLuaInterpreter::addArea(areaId), OR moves a room to the
    // area with either a Lua command or GUI action.
    return true;
}

bool TRoomDB::addArea(int id)
{
    if (!areas.contains(id)) {
        areas[id] = new TArea(mpMap, this);
        if (!areaNamesMap.contains(id)) {
            // Must provide a name for this new area
            QString newAreaName = mUnnamedAreaName;
            if (areaNamesMap.values().contains(newAreaName)) {
                // We already have an "unnamed area"
                uint deduplicateSuffix = 0;
                do {
                    newAreaName = QStringLiteral("%1_%2").arg(mUnnamedAreaName).arg(++deduplicateSuffix, 3, 10, QLatin1Char('0'));
                } while (areaNamesMap.values().contains(newAreaName));
            }
            areaNamesMap.insert(id, newAreaName);
        }
        return true;
    } else {
        QString error = tr("Area with ID=%1 already exists!").arg(id);
        mpMap->logError(error);
        return false;
    }
}

int TRoomDB::createNewAreaID()
{
    int id = 1;
    while (areas.contains(id)) {
        id++;
    }
    return id;
}

int TRoomDB::addArea(QString name)
{
    // reject it if area name already exists or is empty
    if (name.isEmpty()) {
        QString error = tr("An Unnamed Area is (no longer) permitted!");
        mpMap->logError(error);
        return 0;
    } else if (areaNamesMap.values().contains(name)) {
        QString error = tr("An area called %1 already exists!").arg(name);
        mpMap->logError(error);
        return 0;
    }

    int areaID = createNewAreaID();
    if (addArea(areaID)) {
        areaNamesMap[areaID] = name;
        // This will overwrite the "Unnamed Area_###" that addArea( areaID )
        // will generate - but that is fine.
        return areaID;
    } else {
        return 0; //fail
    }
}

// this func is called by the xml map importer
// NOTE: we no longer accept duplicate IDs or duplicate area names
//       duplicate definitions are ignored
//       Unless the area name is empty, in which case we provide one!
bool TRoomDB::addArea(int id, QString name)
{
    if (((!name.isEmpty()) && areaNamesMap.values().contains(name)) || areaNamesMap.keys().contains(id)) {
        return false;
    } else if (addArea(id)) {
        // This will generate an "Unnamed Area_###" area name which we should
        // overwrite only if we have a name!
        if (!name.isEmpty()) {
            areaNamesMap[id] = name;
        }
        return true;
    } else {
        return false;
    }
}

const QList<TArea*> TRoomDB::getAreaPtrList()
{
    return areas.values();
}

QList<int> TRoomDB::getAreaIDList()
{
    return areas.keys();
}

/*
 * Tasks to perform
 * 1) Validate all room Ids and remap any problem ones ( <1 ), retaining
 *    remappings for fixups later on
 * 2) Find all areas that the rooms demand
 * 3) Find all areas that the areanames demand
 * 4) Validate all existing area Ids and remap any problem one
 * 5) Merge in area Ids that rooms and areanames demand, remapping any problem
 *    ones
 * 6) If rooms have been remapped, fixup rooms' exits' to use the new room Ids
 * 7) If areas have been remapped, fixup rooms to use new area Ids
 * 8) Validate area::rooms - adding or removing roomId based on the rooms' idea
 *    of the area they should be in - not forgetting any remappings of area/room
 *    Ids..
 * 9) Validate TRoom elements: exits, stubs (beware of duplicate QList<T>
 *    elements), doors (must have a real or stub in that direction), custom
 *    lines (no key for an normal or special exit that isn't there), locks
 *    (beware of duplicate QList<T> elements), room and exit weights (can only
 *    have a weight on an actual exit).
 *10) Validate TArea elements: ebenen (beware of duplicate QList<T> elements)
 */
void TRoomDB::auditRooms(QHash<int, int>& roomRemapping, QHash<int, int>& areaRemapping)
{
    QSet<int> validUsedRoomIds; // Used good ids (>= 1)
    QSet<int> validUsedAreaIds; // As rooms

    // START OF TASK 1 & 2:
    // Scan through all rooms and record their idea of which area they
    // should be in.  Note invalid room Ids those will be fixed up later...
    QHash<int, int> roomAreaHash;           // Key: room Id, Value: area it believes it should be in
    QMultiHash<int, int> areaRoomMultiHash; // Key: areaId, ValueS: rooms that believe they belong there

    { // Block this code to limit scope of iterator
        QMutableHashIterator<int, TRoom*> itRoom(rooms);
        while (itRoom.hasNext()) {
            itRoom.next();
            TRoom* pR = itRoom.value();
            if (!pR) {
                if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                    QString warnMsg = tr("[ WARN ]  - Problem with data structure associated with room id: %1 - that\n"
                                         "room's data has been lost so the id is now being deleted.  This\n"
                                         "suggests serious problems with the currently running version of\n"
                                         "Mudlet - is your system running out of memory?")
                                              .arg(itRoom.key());
                    mpMap->postMessage(warnMsg);
                }
                mpMap->appendRoomErrorMsg(itRoom.key(),
                                          tr("[ WARN ]  - Problem with data structure associated with this room."
                                             "  The room's data has been lost so the id is now being deleted."
                                             "  This suggests serious problems with the currently running version of Mudlet"
                                             " - is your system running out of memory?"),
                                          true);
                itRoom.remove();
                continue;
            }
            if (itRoom.key() >= 1) {
                validUsedRoomIds.insert(itRoom.key());
            } else {
                roomRemapping.insert(itRoom.key(), itRoom.key());
                //Store them for now, will assign new values when we have the
                // set of good ones already used
            }

            int areaId = pR->getArea();
            areaRoomMultiHash.insert(areaId, itRoom.key());
            roomAreaHash.insert(itRoom.key(), areaId);
        }
    }

    // Check for existance of all areas needed by rooms
    QSet<int> areaIdSet = areaRoomMultiHash.keys().toSet();

    // START OF TASK 3
    // Throw in the area Ids from the areaNamesMap:
    areaIdSet.unite(areaNamesMap.keys().toSet());

    // And the area Ids used by the map labels:
    areaIdSet.unite(mpMap->mapLabels.keys().toSet());

    // Check the set of area Ids against the ones we actually have:
    QSetIterator<int> itUsedArea(areaIdSet);
    // This is a later fix, as I forgot to handle wanted area ids that are +ve
    // (and thus valid) but absent:
    QList<int> missingAreasNeeded;
    while (itUsedArea.hasNext()) {
        int usedAreaId = itUsedArea.next();
        if (usedAreaId < -1 || !usedAreaId) {
            areaRemapping.insert(usedAreaId, usedAreaId); // Will find new value to use later
        } else {
            validUsedAreaIds.insert(usedAreaId);
        }

        if (!areas.contains(usedAreaId)) {
            if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                QString warnMsg = tr("[ ALERT ] - Area with id: %1 expected but not found, will be created.").arg(usedAreaId);
                mpMap->postMessage(warnMsg);
            }
            mpMap->appendAreaErrorMsg(usedAreaId, tr("[ ALERT ] - Area with this id expected but not found, will be created."), true);
            missingAreasNeeded.append(usedAreaId);
        }
    }

    // START OF TASK 4
    // Check for any problem Id in original areas
    QMapIterator<int, TArea*> itArea(areas);
    while (itArea.hasNext()) {
        itArea.next();
        int areaId = itArea.key();
        if (areaId < -1 || !areaId) {
            areaRemapping.insert(areaId, areaId); // Will find new value to use later
        } else {
            validUsedAreaIds.insert(areaId);
        }
    }

    // START OF TASK 5.0 (previously omitted) - add in areas that we have an id
    // for but no actual TArea, at this point:
    // * id WILL be in validUsedAreaIds
    // * id WILL NOT be in areaRemapping, but represents a id number that cannot be
    // remapped as a new area id
    // * id MAY be in TRoomDB::areaNamesMap
    // * id MAY be used by rooms which will have to be added to it at some point
    // * WILL NOT be in TRoomDB::areas - as that is the source of the error this
    // bit of the task being addressed here is fixing
    if (!missingAreasNeeded.isEmpty()) {
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            QString alertMsg = tr( "[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.\n"
                                   " Look for further messsages related to the rooms that are supposed to\n"
                                   " be in this/these area(s)...",
                                   "Making use of %n to allow quantity dependent message form 8-) !",
                                   missingAreasNeeded.count() );
            mpMap->postMessage(alertMsg);
        }
        mpMap->appendErrorMsgWithNoLf(tr("[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.\n"
                                         " Look for further messsages related to the rooms that is/are supposed to\n"
                                         " be in this/these area(s)...",
                                         "Making use of %n to allow quantity dependent message form 8-) !",
                                         missingAreasNeeded.count() ),
                              true);

        QString infoMsg;
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            infoMsg = tr("[ INFO ]  - The missing area(s) are now called:\n"
                         "(ID) ==> \"name\"",
                         "Making use of %n to allow quantity dependent message form 8-) !",
                         missingAreasNeeded.count() );
        }

        if (missingAreasNeeded.count() > 1) {
            // Sort the ids so that the reporting is ordered, which could be
            // helpful if there is a large number of faults
            std::sort(missingAreasNeeded.begin(), missingAreasNeeded.end());
            for (int newAreaId : missingAreasNeeded) {

                // This will create a new "Default" area name if there is not one
                // already for this id - and we do not anticipate that it could ever
                // fail and return false...
                addArea(newAreaId);
                infoMsg.append(QStringLiteral("\n%1 ==> \"%2\"")
                               .arg(newAreaId)
                               .arg(areaNamesMap.value(newAreaId)));
            }

            // Didn't really needed to be done, but as we have finished with it
            // now, clearing it may make tracking the overall processes going on
            // in the debugger a little clearer...
            missingAreasNeeded.clear();
        }

        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            mpMap->postMessage(infoMsg);
        }
        mpMap->appendErrorMsg(infoMsg, true);
    }

    // START OF TASK 5.1
    // Now process problem areaIds
    if (!areaRemapping.isEmpty()) {
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            QString alertMsg = tr("[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1)\n"
                                  "in map, now working out what new id numbers to use...")
                                       .arg(areaRemapping.count());
            mpMap->postMessage(alertMsg);
        }
        mpMap->appendErrorMsg(tr("[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1) in map!"
                                 "  Look for further messsages related to this for each affected area ...")
                                      .arg(areaRemapping.count()),
                              true);

        QString infoMsg;
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            infoMsg = tr("[ INFO ]  - The renumbered area ids will be:\n"
                         "Old ==> New");
        }

        QMutableHashIterator<int, int> itRemappedArea(areaRemapping);
        while (itRemappedArea.hasNext()) {
            itRemappedArea.next();
            int faultyAreaId = itRemappedArea.key();
            int replacementAreaId = 0;
            do {
                ; // No-op, increment done in test
            } while (areas.contains(++replacementAreaId));
            // Insert replacement value into hash
            itRemappedArea.setValue(replacementAreaId);
            if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                infoMsg.append(QStringLiteral("\n%1 ==> %2").arg(QString::number(faultyAreaId), QString::number(replacementAreaId)));
            }

            mpMap->appendAreaErrorMsg(faultyAreaId, tr("[ INFO ]  - The area with this bad id was renumbered to: %1.").arg(replacementAreaId), true);
            mpMap->appendAreaErrorMsg(replacementAreaId, tr("[ INFO ]  - This area was renumbered from the bad id: %1.").arg(faultyAreaId), true);

            TArea* pA = 0;
            if (areas.contains(faultyAreaId)) {
                pA = areas.take(faultyAreaId);
            } else {
                pA = new TArea(mpMap, this);
            }
            if (areaNamesMap.contains(faultyAreaId)) {
                QString areaName = areaNamesMap.value(faultyAreaId);
                areaNamesMap.remove(faultyAreaId);
                areaNamesMap.insert(replacementAreaId, areaName);
            } else {
                // I think this is unlikely but better provide code to cover it
                // if it does arise that we need a new area but do not have a
                // provided name
                QString newAreaName = mUnnamedAreaName;
                if (areaNamesMap.values().contains(newAreaName)) {
                    // We already have an "unnamed area"
                    uint deduplicateSuffix = 0;
                    do {
                        newAreaName = QStringLiteral("%1_%2").arg(mUnnamedAreaName).arg(++deduplicateSuffix, 3, 10, QLatin1Char('0'));
                    } while (areaNamesMap.values().contains(newAreaName));
                }
                areaNamesMap.insert(replacementAreaId, newAreaName);
            }
            pA->mUserData.insert(QStringLiteral("audit.remapped_id"), QString::number(faultyAreaId));
            validUsedAreaIds.insert(replacementAreaId);
            areas.insert(replacementAreaId, pA);

            // Fixup map labels as well
            if (mpMap->mapLabels.contains(faultyAreaId)) {
                QMap<qint32, TMapLabel> areaMapLabels = mpMap->mapLabels.take(faultyAreaId);
                mpMap->mapLabels.insert(replacementAreaId, areaMapLabels);
            }

            pA->mIsDirty = true;
        }
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            mpMap->postMessage(infoMsg);
        }
    } else {
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            QString infoMsg = tr("[ INFO ]  - Area id numbering is satisfactory.");
            mpMap->postMessage(infoMsg);
        }
        mpMap->appendErrorMsg(tr("[ INFO ]  - Area id numbering is satisfactory."), false);
    }
    // END OF TASK 2,3,4,5 - all needed areas exist and remap details are in
    // areaRemapping - still need to update rooms and areaNames and mapLabels

    // Now complete TASK 1 - find the new room Ids to use
    if (!roomRemapping.isEmpty()) {
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            QString alertMsg = tr("[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map, now working\n"
                                  "out what new id numbers to use.")
                                       .arg(roomRemapping.count());
            mpMap->postMessage(alertMsg);
        }
        mpMap->appendErrorMsg(tr("[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map!"
                                 "  Look for further messsages related to this for each affected room ...")
                                      .arg(roomRemapping.count()),
                              true);

        QString infoMsg;
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            infoMsg = tr("[ INFO ]  - The renumbered rooms will be:\n");
        }
        QMutableHashIterator<int, int> itRenumberedRoomId(roomRemapping);
        while (itRenumberedRoomId.hasNext()) {
            itRenumberedRoomId.next();
            unsigned int newRoomId = 0;
            do {
                ; // Noop - needed increment is done in test condition!
            } while (validUsedRoomIds.contains(++newRoomId));

            itRenumberedRoomId.setValue(newRoomId); // Update the QHash
            validUsedRoomIds.insert(newRoomId);
            if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                infoMsg.append(QStringLiteral("%1 ==> %2").arg(QString::number(itRenumberedRoomId.key()), QString::number(itRenumberedRoomId.value())));
            }

            mpMap->appendRoomErrorMsg(itRenumberedRoomId.key(), tr("[ INFO ]  - This room with the bad id was renumbered to: %1.").arg(itRenumberedRoomId.value()), true);
            mpMap->appendRoomErrorMsg(itRenumberedRoomId.value(), tr("[ INFO ]  - This room was renumbered from the bad id: %1.").arg(itRenumberedRoomId.key()), true);
        }
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            mpMap->postMessage(infoMsg);
        }

        QSet<TRoom*> holdingSet;
        { // Block this code to limit scope of iterator
            QMutableHashIterator<int, TRoom*> itRoom(rooms);
            // Although this is "mutable" we can only changed the VALUE not the KEY
            // Also we cannot directly change the collection being iterated over -
            // though we can REMOVE items via the iterator - so pull the
            // affected TRoom instances out of the Hash, renumber them and then
            // hold onto them before reinserting them after we have finished
            // this iteration
            while (itRoom.hasNext()) {
                itRoom.next();
                TRoom* pR = itRoom.value();
                if (roomRemapping.contains(itRoom.key())) {
                    pR->userData.insert(QStringLiteral("audit.remapped_id"), QString::number(itRoom.key()));
                    pR->setId(roomRemapping.value(itRoom.key()));
                    itRoom.remove();
                    holdingSet.insert(pR);
                }
            }
        }

        // Now stuff the modified values back in under the new key values
        QSetIterator<TRoom*> itModifiedRoom(holdingSet);
        while (itModifiedRoom.hasNext()) {
            TRoom* pR = itModifiedRoom.next();
            int newRoomId = pR->getId();
            rooms.insert(newRoomId, pR);
        }
    } else {
        if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
            QString infoMsg = tr("[ INFO ]  - Room id numbering is satisfactory.");
            mpMap->postMessage(infoMsg);
        }
        mpMap->appendErrorMsg(tr("[ INFO ]  - Room id numbering is satisfactory."), false);
    }
    // END OF TASK 1

    // START OF TASK 6,7 & 9
    { // Block the following code to limit the scope of the itRoom iterator
        QMutableHashIterator<int, TRoom*> itRoom(rooms);
        while (itRoom.hasNext()) {
            itRoom.next();
            TRoom* pR = itRoom.value();

            // Purges any duplicates that a QList structure DOES permit, but a QSet does NOT:
            // Exit stubs:
            unsigned int _listCount = pR->exitStubs.count();
            QSet<int> _set = pR->exitStubs.toSet();
            if (_set.count() < _listCount) {
                if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                    QString infoMsg = tr("[ INFO ]  - Duplicate exit stub identifiers found in room id: %1, this is an\n"
                                         "anomaly but has been cleaned up easily.")
                                              .arg(itRoom.key());
                    mpMap->postMessage(infoMsg);
                }
                mpMap->appendRoomErrorMsg(itRoom.key(), tr("[ INFO ]  - Duplicate exit stub identifiers found in room, this is an anomaly but has been cleaned up easily."), false);
            }
            pR->exitStubs = _set.toList();

            // Exit locks:
            _listCount = pR->exitLocks.count();
            _set = pR->exitLocks.toSet();
            if (_set.count() < _listCount) {
                if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                    QString infoMsg = tr("[ INFO ]  - Duplicate exit lock identifiers found in room id: %1, this is an\n"
                                         "anomaly but has been cleaned up easily.")
                                              .arg(itRoom.key());
                    mpMap->postMessage(infoMsg);
                }
                mpMap->appendRoomErrorMsg(itRoom.key(), tr("[ INFO ]  - Duplicate exit lock identifiers found in room, this is an anomaly but has been cleaned up easily."), false);
            }
            pR->exitLocks = _set.toList();

            // TASK 9 IS DONE INSIDE THIS METHOD:
            pR->audit(roomRemapping, areaRemapping);
        }
    }
    // END OF TASK 6,7 & 9

    // START TASK 8
    {
        QMapIterator<int, TArea*> itArea(areas);
        while (itArea.hasNext()) {
            itArea.next();
            TArea* pA = itArea.value();
            QSet<int> replacementRoomsSet;
            { // Block code to limit scope of iterator, find and pull out renumbered rooms
                QMutableSetIterator<int> itAreaRoom(pA->rooms);
                if (!roomRemapping.isEmpty()) {
                    while (itAreaRoom.hasNext()) {
                        int originalRoomId = itAreaRoom.next();
                        if (roomRemapping.contains(originalRoomId)) {
                            itAreaRoom.remove();
                            replacementRoomsSet.insert(roomRemapping.value(originalRoomId));
                        }
                    }
                }
            }
            // Merge back in the renumbered rooms
            if (!replacementRoomsSet.isEmpty()) {
                pA->rooms.unite(replacementRoomsSet);
            }

            // Now compare pA->rooms to areaRoomMultiHash.values( itArea.key() )
            QSet<int> foundRooms = areaRoomMultiHash.values(itArea.key()).toSet();
            QSetIterator<int> itFoundRoom(foundRooms);
// Original form of code which was slower because the two sets of rooms were
// compared TWICE:
//            extraRooms = pA->rooms;
//            extraRooms.subtract( foundRooms );
//            missingRooms = foundRooms;
//            missingRooms.subtract( pA->rooms );

            // Revised code that only makes one pass
            QSet<int> extraRooms(pA->rooms); //Take common rooms from here so that any left are the "extras"
            extraRooms.detach();             // We'll need a deep copy so might as well explicitly say so to make the copy
            QSet<int> missingRooms;          // Add rooms not in pA->room to here
            int checkRoom;
            while (itFoundRoom.hasNext()) {
                checkRoom = itFoundRoom.next();
                if (pA->rooms.contains(checkRoom)) {
                    extraRooms.remove(checkRoom);
                } else {
                    missingRooms.insert(checkRoom);
                }
            }

            // Report differences:
            if (!missingRooms.isEmpty()) {
                QStringList roomList;
                QList<int> missingRoomsList = missingRooms.toList();
                if (missingRoomsList.size() > 1) {
                    // The on-screen listing are clearer if we sort the rooms
                    std::sort(missingRoomsList.begin(), missingRoomsList.end());
                }
                QListIterator<int> itMissingRoom(missingRoomsList);
                while (itMissingRoom.hasNext()) {
                    int missingRoomId = itMissingRoom.next();
                    roomList.append(QString::number(missingRoomId));
                    mpMap->appendRoomErrorMsg(missingRoomId,
                                              tr("[ INFO ]  - This room claims to be in area id: %1, but that did not have a record of it."
                                                 "  The area has been updated to include this room.")
                                                      .arg(itArea.key()),
                                              true);
                }
                if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                    QString infoMsg = tr("[ INFO ]  - In area with id: %1 there were %2 rooms missing from those it\n"
                                         "should be recording as possessing, they were:\n%3\nthey have been added.")
                                              .arg(itArea.key())
                                              .arg(missingRooms.count())
                                              .arg(roomList.join(QStringLiteral(", ")));
                    mpMap->postMessage(infoMsg);
                }
                mpMap->appendAreaErrorMsg(itArea.key(),
                                          tr("[ INFO ]  - In this area there were %1 rooms missing from those it should be recorded as possessing."
                                             "  They are: %2."
                                             "  They have been added.")
                                                  .arg(missingRooms.count())
                                                  .arg(roomList.join(QStringLiteral(", "))),
                                          true);

                pA->mIsDirty = true;
            }

            if (!extraRooms.isEmpty()) {
                QStringList roomList;
                QList<int> extraRoomsList = extraRooms.toList();
                if (extraRoomsList.size() > 1) {
                    std::sort(extraRoomsList.begin(), extraRoomsList.end());
                }
                QListIterator<int> itExtraRoom(extraRoomsList);
                while (itExtraRoom.hasNext()) {
                    int extraRoomId = itExtraRoom.next();
                    roomList.append(QString::number(extraRoomId));
                    mpMap->appendRoomErrorMsg(extraRoomId,
                                              tr("[ INFO ]  - This room was claimed by area id: %1, but it does not belong there."
                                                 "  The area has been updated to not include this room.")
                                                      .arg(itArea.key()),
                                              true);
                }
                if (mudlet::self()->getAuditErrorsToConsoleEnabled()) {
                    QString infoMsg = tr("[ INFO ]  - In area with id: %1 there were %2 extra rooms compared to those it\n"
                                         "should be recording as possessing, they were:\n%3\nthey have been removed.")
                                              .arg(itArea.key())
                                              .arg(extraRooms.count())
                                              .arg(roomList.join(QStringLiteral(", ")));
                    mpMap->postMessage(infoMsg);
                }
                mpMap->appendAreaErrorMsg(itArea.key(),
                                          tr("[ INFO ]  - In this area there were %1 extra rooms that it should not be recorded as possessing."
                                             "  They were: %2."
                                             "  They have been removed.")
                                                  .arg(extraRooms.count())
                                                  .arg(roomList.join(QStringLiteral(", "))),
                                          true);
                pA->mIsDirty = true;
            }
            pA->rooms = foundRooms;
        }
    }
    // END OF TASK 8
}

void TRoomDB::clearMapDB()
{
    QElapsedTimer timer;
    timer.start();
    QList<TRoom*> rPtrL = getRoomPtrList();
    rooms.clear(); // Prevents any further use of TRoomDB::getRoom(int) !!!
    entranceMap.clear();
    areaNamesMap.clear();
    hashTable.clear();
    for (auto room : rPtrL) {
        delete room; // Uses the internally held value of the room Id
                     // (TRoom::id) to call TRoomDB::__removeRoom(id)
    }
//    assert( rooms.size() == 0 ); // Pointless as rooms.clear() will have achieved the test condition

    QList<TArea*> areaList = getAreaPtrList();
    for (auto area : areaList) {
        delete area;
    }
    assert(areas.size() == 0);
    // Must now reinsert areaId -1 name = "Default Area"
    addArea(-1, mDefaultAreaName);
    qDebug() << "TRoomDB::clearMapDB() run time:" << timer.nsecsElapsed() * 1.0e-9 << "sec.";
}

void TRoomDB::restoreAreaMap(QDataStream& ifs)
{
    QMap<int, QString> areaNamesMapWithPossibleEmptyOrDuplicateItems;
    ifs >> areaNamesMapWithPossibleEmptyOrDuplicateItems;
    areaNamesMap.clear(); // Following code assumes areaNamesMap is empty but
                          // under some situations this has not been the case...

    // Validate names: name nameless areas and rename duplicates
    QMultiMap<QString, QString> renamedMap; // For warning message, holds renamed area map
    QMapIterator<int, QString> itArea = areaNamesMapWithPossibleEmptyOrDuplicateItems;
    bool isMatchingSuffixAlreadyPresent = false;
    bool isEmptyAreaNamePresent = false;
    while (itArea.hasNext()) {
        itArea.next();
        QString nonEmptyAreaName;
        if (itArea.value().isEmpty()) {
            isEmptyAreaNamePresent = true;
            nonEmptyAreaName = mUnnamedAreaName;
            // Will trip following if more than one
        } else {
            nonEmptyAreaName = itArea.value();
        }
        if (areaNamesMap.values().contains(nonEmptyAreaName)) {
            // Oh dear, we have a duplicate
            if (nonEmptyAreaName.contains(QRegExp("_\\d\\d\\d$"))) {
                // the areaName already is of form "something_###" where # is a
                // digit, have to strip that off and remember so warning message
                // can include advice on this change
                isMatchingSuffixAlreadyPresent = true;
                nonEmptyAreaName.chop(4); // Take off existing suffix
            }
            uint deduplicateSuffix = 0;
            QString replacementName;
            do {
                replacementName = QStringLiteral("%1_%2").arg(nonEmptyAreaName).arg(++deduplicateSuffix, 3, 10, QLatin1Char('0'));
            } while (areaNamesMap.values().contains(replacementName));
            if ((!itArea.value().isEmpty()) && (!renamedMap.contains(itArea.value()))) {
                // if the renamedMap does not contain the first, unaltered value
                // that a subsequent match has been found for, then include it
                // in the data so the user can see the UNCHANGED one as well
                // Only have to do this once for each duplicate area name, hence
                // the test conditions above
                renamedMap.insert(itArea.value(), itArea.value());
            }
            renamedMap.insert(itArea.value(), replacementName);
            areaNamesMap.insert(itArea.key(), replacementName);
        } else {
            if (itArea.value().isEmpty()) {
                renamedMap.insert(itArea.value(), nonEmptyAreaName);
            }
            areaNamesMap.insert(itArea.key(), nonEmptyAreaName);
        }
    }
    if (renamedMap.size() || isEmptyAreaNamePresent) {
        QString alertText;
        QString informativeText;
        QString extraTextForMatchingSuffixAlreadyUsed;
        QString detailText;
        if (isMatchingSuffixAlreadyPresent) {
            extraTextForMatchingSuffixAlreadyUsed = tr("It has been detected that \"_###\" form suffixes have already been used, for "
                                                       "simplicity in the renaming algorithm these will have been removed and possibly "
                                                       "changed as Mudlet sorts this matter out, if a number assigned in this way "
                                                       "<b>is</b> important to you, you can change it back, provided you rename the area "
                                                       "that has been allocated the suffix that was wanted first...!</p>");
        }
        if (renamedMap.size()) {
            detailText = tr("[  OK  ]  - The changes made are:\n"
                            "(ID) \"old name\" ==> \"new name\"\n");
            QMapIterator<QString, QString> itRemappedNames = renamedMap;
            itRemappedNames.toBack();
            // Seems to look better if we iterate through backwards!
            while (itRemappedNames.hasPrevious()) {
                itRemappedNames.previous();
                QString oldName = itRemappedNames.key().isEmpty() ? tr("<nothing>") : itRemappedNames.key();
                detailText.append(QStringLiteral("(%1) \"%2\" ==> \"%3\"\n").arg(areaNamesMap.key(itRemappedNames.value())).arg(oldName, itRemappedNames.value()));
                mpMap->appendAreaErrorMsg(areaNamesMap.key(itRemappedNames.value()),
                                          tr("[ INFO ]  - Area name changed to prevent duplicates or unnamed ones; old name: \"%1\", new name: \"%2\".").arg(oldName, itRemappedNames.value()),
                                          true);
                ;
            }
            detailText.chop(1); // Trim last "\n" off
        }
        if (renamedMap.size() && isEmptyAreaNamePresent) {
            // At least one unnamed area and at least one duplicate area name
            // - may be the same items
            alertText = tr("[ ALERT ] - Empty and duplicate area names detected in Map file!");
            informativeText = tr("[ INFO ]  - Due to some situations not being checked in the past,  Mudlet had\n"
                                 "allowed the map to have more than one area with the same or no name.\n"
                                 "These make some things confusing and are now disallowed.\n"
                                 "  To resolve these cases, an area without a name here (or created in\n"
                                 "the future) will automatically be assigned the name \"%1\".\n"
                                 "  Duplicated area names will cause all but the first encountered one\n"
                                 "to gain a \"_###\" style suffix where each \"###\" is an increasing\n"
                                 "number; you may wish to change these, perhaps by replacing them with\n"
                                 "a \"(sub-area name)\" but it is entirely up to you how you do this,\n"
                                 "other then you will not be able to set one area's name to that of\n"
                                 "another that exists at the time.\n"
                                 "  If there were more than one area without a name then all but the\n"
                                 "first will also gain a suffix in this manner.\n"
                                 "%2")
                                      .arg(mUnnamedAreaName, extraTextForMatchingSuffixAlreadyUsed);
        } else if (renamedMap.size()) {
            // Duplicates but no unnnamed area
            alertText = tr("[ ALERT ] - Duplicate area names detected in the Map file!");
            informativeText = tr("[ INFO ]  - Due to some situations not being checked in the past, Mudlet had\n"
                                 "allowed the user to have more than one area with the same name.\n"
                                 "These make some things confusing and are now disallowed.\n"
                                 "  Duplicated area names will cause all but the first encountered one\n"
                                 "to gain a \"_###\" style suffix where each \"###\" is an increasing\n"
                                 "number; you may wish to change these, perhaps by replacing them with\n"
                                 "a \"(sub-area name)\" but it is entirely up to you how you do this,\n"
                                 "other then you will not be able to set one area's name to that of\n"
                                 "another that exists at the time.\n"
                                 "  If there were more than one area without a name then all but the\n"
                                 "first will also gain a suffix in this manner.\n"
                                 "%1)")
                                      .arg(extraTextForMatchingSuffixAlreadyUsed);
        } else {
            // A single unnamed area found
            alertText = tr("[ ALERT ] - An empty area name was detected in the Map file!");
            // Use OK for this one because it is the last part and indicates the
            // sucessful end of something, whereas INFO is an intermediate step
            informativeText = tr("[  OK  ]  - Due to some situations not being checked in the past, Mudlet had\n"
                                 "allowed the map to have an area with no name. This can make some\n"
                                 "things confusing and is now disallowed.\n"
                                 "  To resolve this case, the area without a name here (or one created\n"
                                 "in the future) will automatically be assigned the name \"%1\".\n"
                                 "  If this happens more then once the duplication of area names will\n"
                                 "cause all but the first encountered one to gain a \"_###\" style\n"
                                 "suffix where each \"###\" is an increasing number; you may wish to\n"
                                 "change these, perhaps by adding more meaningful area names but it is\n"
                                 "entirely up to you what is used, other then you will not be able to\n"
                                 "set one area's name to that of another that exists at the time.")
                                      .arg(mUnnamedAreaName);
        }
        mpMap->mpHost->postMessage(alertText);
        mpMap->appendErrorMsgWithNoLf(alertText, true);
        mpMap->mpHost->postMessage(informativeText);
        mpMap->appendErrorMsgWithNoLf(informativeText, true);
        if (!detailText.isEmpty()) {
            mpMap->mpHost->postMessage(detailText);
        }
    }

    if (!areaNamesMap.contains(-1)) {
        areaNamesMap.insert(-1, mDefaultAreaName);
        QString defaultAreaNameInsertionMsg = tr("[ INFO ]  - Default (reset) area name (for rooms that have not been assigned to an\n"
                                                 "area) not found, adding \"%1\" against the reserved -1 id.")
                                                      .arg(mDefaultAreaName);
        mpMap->mpHost->postMessage(defaultAreaNameInsertionMsg);
        mpMap->appendErrorMsgWithNoLf(defaultAreaNameInsertionMsg, false);
    }
}

void TRoomDB::restoreSingleArea(int areaID, TArea* pA)
{
    areas[areaID] = pA;
}

void TRoomDB::restoreSingleRoom(int i, TRoom* pT)
{
    addRoom(i, pT, true);
}

// Used by XMLimport to fix TArea::rooms data after import
void TRoomDB::setAreaRooms(const int areaId, const QSet<int>& roomIds)
{
    TArea* pA = areas.value(areaId);
    if (!pA) {
        qWarning() << "TRoomDB::setAreaRooms(" << areaId << ", ... ) ERROR - Non-existant area Id given...!";
        return;
    }

    QSetIterator<int> itAreaRoom(roomIds);
    while (itAreaRoom.hasNext()) {
        pA->addRoom(itAreaRoom.next());
    }

    pA->calcSpan(); // The area extents will need recalculation after adding the rooms
}
