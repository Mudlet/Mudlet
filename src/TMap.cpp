/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2024 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "Host.h"
#include "TArea.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TMapLabel.h"
#include "TRoomDB.h"
#include "XMLimport.h"
#include "dlgMapper.h"
#include "TLuaInterpreter.h"
#include "mapInfoContributorManager.h"
#include "mudlet.h"

#include <QElapsedTimer>
#include <QFileDialog>
#include <QJsonParseError>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPainter>
#include <QBuffer>


TMap::TMap(Host* pH, const QString& profileName)
: mDefaultAreaName(tr("Default Area"))
, mUnnamedAreaName(tr("Unnamed Area"))
, mpRoomDB(new TRoomDB(this))
, mpHost(pH)
, mProfileName(profileName)
{
    restore16ColorSet();

    // TODO: https://github.com/Mudlet/Mudlet/issues/6436
    // According to Qt Docs we should really only have one of these
    // (QNetworkAccessManager) for the whole application, but: each profile's
    // TLuaInterpreter; each profile's ctelnet and now each profile's TMap
    // (was dlgMapper) instance has one...!
    mpNetworkAccessManager = new QNetworkAccessManager(this);

    mMapInfoContributorManager = new MapInfoContributorManager(this, pH);

    connect(mpNetworkAccessManager, &QNetworkAccessManager::finished, this, &TMap::slot_replyFinished);
}

TMap::~TMap()
{
    delete mpRoomDB;
    if (!mStoredMessages.isEmpty()) {
        qWarning() << "TMap::~TMap() Instance being destroyed before it could display some messages,\n"
                   << "messages are:\n"
                   << "------------";
        for (const auto& message : mStoredMessages) {
            qWarning() << message << "\n------------";
        }
    }
}

void TMap::mapClear()
{
    mpRoomDB->clearMapDB();
    mEnvColors.clear();
    mRoomIdHash.clear();
    mTargetID = 0;
    mPathList.clear();
    mDirList.clear();
    mWeightList.clear();
    mCustomEnvColors.clear();
    // Need to restore the default colours:
    restore16ColorSet();
    roomidToIndex.clear();
    edgeHash.clear();
    locations.clear();
    mMapGraphNeedsUpdate = true;
    mNewMove = true;
    mVersion = mDefaultVersion;
    mUserData.clear();

    // mSaveVersion is not reset - so that any new Mudlet map file saves are to
    // whatever version was previously set/deduced

    // Must also reset the mapper area selection control to reflect that it now
    // only has the "Default Area" after TRoomDB::clearMapDB() has been run.
    if (mpMapper) {
        mpMapper->updateAreaComboBox();

        auto map = mpMapper->mp2dMap;
        if (map) {
            map->mMultiSelectionListWidget.clear();
            map->mMultiSelectionListWidget.hide();
        }
    }
}

void TMap::logError(QString& msg)
{
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl("%1\n").arg(tr("[MAP ERROR:]%1").arg(msg)), QColor(255, 128, 0), QColor(Qt::black));
    }
}

// Not used:
//void TMap::exportMapToDatabase()
//{
//    QString dbName = QFileDialog::getSaveFileName( 0, "Chose db file name." );
//    QString script = QString("exportMapToDatabse([[%1]])").arg(dbName);
//    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
//}

//void TMap::importMapFromDatabase()
//{
//    QString dbName = QFileDialog::getOpenFileName( 0, "Chose db file name." );
//    QString script = QString("importMapFromDatabase([[%1]])").arg(dbName);
//    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
//}

bool TMap::setRoomArea(int id, int area, bool deferAreaRecalculations)
{
    TRoom* pR = mpRoomDB->getRoom(id);
    if (!pR) {
        QString msg = tr("RoomID=%1 does not exist, can not set AreaID=%2 for non-existing room!").arg(id).arg(area);
        logError(msg);
        return false;
    }

    TArea* pA = mpRoomDB->getArea(area);
    if (!pA) {
        // Uh oh, the area doesn't seem to exist as a TArea instance, let's check
        // to see if it exists as a name only:
        if (!mpRoomDB->getAreaNamesMap().contains(area)) {
            // Ah, no it doesn't so moan:
            QString msg = tr("AreaID=%2 does not exist, can not set RoomID=%1 to non-existing area!").arg(id).arg(area);
            logError(msg);
            return false;
        }
        // If got to this point then there is NOT a TArea instance for the given
        // area Id but there is a Name - and the pR->setArea(...) call WILL
        // instantiate the required TArea structure - this seems a bit twisty
        // and convoluted, but it was how previous code was wired up and we need
        // to retain the API for the lua subsystem...
    }

    const bool result = pR->setArea(area, deferAreaRecalculations);
    if (result) {
        mMapGraphNeedsUpdate = true;
        setUnsaved(__func__);
    }
    return result;
}

bool TMap::addRoom(int id)
{
    if (mpRoomDB->addRoom(id)) {
        mMapGraphNeedsUpdate = true;
        setUnsaved(__func__);
        return true;
    }
    return false;
}

bool TMap::setRoomCoordinates(int id, int x, int y, int z)
{
    TRoom* pR = mpRoomDB->getRoom(id);
    if (!pR) {
        return false;
    }

    pR->setCoordinates(x, y, z);

    setUnsaved(__func__);
    return true;
}

int compSign(int a, int b)
{
    return (a < 0) == (b < 0);
}

// Will connect the exit stub in the indicated direction to a suitable room
// i.e. in the "right" (x,y,z) location AND with a stub in the reverse direction
// IN THE SAME AREA as the fromRoomId numbered room and also create the exit in
// the reverse direction from the other room - otherwise it will report the
// reason why it cannot.
QString TMap::connectExitStubByDirection(const int fromRoomId, const int dirType)
{
    Q_ASSERT_X(scmUnitVectors.contains(dirType), "TMap::connectExitStubByDirection(...)", "there is no unitVector.value() for the given dirType");
    Q_ASSERT_X(scmReverseDirections.contains(dirType), "TMap::connectExitStubByDirection(...)", "there is no scmReverseDirections.value() for the given dirType");

    TRoom* pFromR = mpRoomDB->getRoom(fromRoomId);
    if (!pFromR) {
        return qsl("fromID (%1) does not exist").arg(fromRoomId);
    }
    const int area = pFromR->getArea();
    // This will get converted to a positive value on first use:
    int minDistance = -1;
    int minDistanceRoom = 0;
    int meanSquareDistance = 0;

    if (!pFromR->exitStubs.contains(dirType)) {
        return qsl("fromID (%1) does not have an exit stub in the given direction '%2' (%3)")
                .arg(QString::number(fromRoomId), TRoom::dirCodeToString(dirType), QString::number(dirType));
    }

    const int reverseDir = scmReverseDirections.value(dirType);
    const QVector3D unitVector = scmUnitVectors.value(dirType);
    // QVector3D is composed of floating point values so we need to round them
    // if we want to assign them to integral variables without compiler warnings!
    const int ux = qRound(unitVector.x());
    const int uy = qRound(unitVector.y());
    const int uz = qRound(unitVector.z());
    const int rx = pFromR->x();
    const int ry = pFromR->y();
    const int rz = pFromR->z();
    int dx = 0;
    int dy = 0;
    int dz = 0;
    TArea* pA = mpRoomDB->getArea(area);
    if (!pA) {
        return qsl("fromID (%1) room does not have an area").arg(fromRoomId);
    }

    QSetIterator<int> itRoom(pA->getAreaRooms());
    while (itRoom.hasNext()) {
        auto toRoom = itRoom.next();
        auto pToR = mpRoomDB->getRoom(toRoom);
        if (!pToR || pToR->getId() == fromRoomId) {
            continue;
        }

        // New test - does this room have a stub exit in the wanted reverse
        // direction:
        if (!pToR->exitStubs.contains(reverseDir)) {
            continue;
        }

        if (uz) {
            dz = pToR->z() - rz;
            if (!compSign(dz, uz) || !dz) {
                continue;
            }

        } else {
            //to avoid lower/upper floors from stealing stubs
            if (pToR->z() != rz) {
                continue;
            }
        }

        if (ux) {
            dx = pToR->x() - rx;
            if (!compSign(dx, ux) || !dx) {
                //we do !dx pRto make sure we have a component in the desired direction
                continue;
            }

        } else {
            //to avoid rooms on same plane from stealing stubs
            if (pToR->x() != rx) {
                continue;
            }
        }

        if (uy) {
            dy = pToR->y() - ry;
            //if the sign is the SAME here we keep it b/c we flip our y coordinate.
            if (compSign(dy, uy) || !dy) {
                continue;
            }

        } else {
            //to avoid rooms on same plane from stealing stubs
            if (pToR->y() != ry) {
                continue;
            }
        }

        meanSquareDistance = dx * dx + dy * dy + dz * dz;
        if (Q_UNLIKELY(minDistance == -1) || (meanSquareDistance < minDistance)) {
            // The first alternative above is the initialisaton case:
            minDistanceRoom = toRoom;
            minDistance = meanSquareDistance;
        }
    }

    if (minDistanceRoom) {
        auto pToR = mpRoomDB->getRoom(minDistanceRoom);
        if (!pToR) {
            // Technically this should be redundant as we have already checked
            // that this room existed in the above while() loop!
            return qsl("nearest room in the indicated direction (%1) does not exist").arg(minDistanceRoom);
        }

        setExit(fromRoomId, minDistanceRoom, dirType);
        setExit(minDistanceRoom, fromRoomId, scmReverseDirections.value(dirType));
        setUnsaved(__func__);
        return {};
    }

    return qsl("fromID (%1) does not have another room in the indicated direction '%2' (%3) with an exit stub in the reverse direction to connect to in its area").arg(QString::number(fromRoomId), TRoom::dirCodeToString(dirType), QString::number(dirType));
}

// Will connect an exit stub from the fromRoomId numbered room to the toRoomId
// numbered room and also connect the corresponding stub exit in the reverse
// direction of the toRoomId room back to the fromRoomId provided the second
// room has a stub in the reverse direction.
// Unlike the connectExitStubByDirection(...) method the relative placement of
// the two rooms is not considered - and indeed the toRoomId room need not be
// IN THE SAME AREA as the fromRoomId numbered room - otherwise it will report
// the reason why it cannot.
// It will only work if there is a single matching pair of stub exits between
// the two rooms - if there are more than one it will fail and invite the
// use of the Lua function with three arguments that include a direction and
// thus use connectExitStubByDirectionAndToId(...) instead:
QString TMap::connectExitStubByToId(const int fromRoomId, const int toRoomId)
{
    auto pFromR = mpRoomDB->getRoom(fromRoomId);
    if (!pFromR) {
        return qsl("fromID (%1) does not exist").arg(fromRoomId);
    }

    if (toRoomId == fromRoomId) {
        return qsl("fromID and toID are the same (%1)").arg(fromRoomId);
    }

    auto pToR = mpRoomDB->getRoom(toRoomId);
    if (!pToR) {
        return qsl("toID (%1) room does not exist").arg(toRoomId);
    }

    if (pFromR->exitStubs.isEmpty()) {
        return qsl("fromID (%1) does not have any stub exits").arg(fromRoomId);
    }

    if (pToR->exitStubs.isEmpty()) {
        return qsl("toID (%1) does not have any stub exits").arg(toRoomId);
    }

    QSet<int> const fromRoomStubs{pFromR->exitStubs.cbegin(), pFromR->exitStubs.cend()};
    QListIterator<int> itToRoomStubs{pToR->exitStubs};
    QSet<int> toReverseStubDirections;
    while (itToRoomStubs.hasNext()) {
        auto direction = itToRoomStubs.next();
        Q_ASSERT_X(scmReverseDirections.contains(direction), "TMap::connectExitStubByToId(...)", "there is no scmReverseDirections.value() for a particular direction encountered");
        toReverseStubDirections.insert(scmReverseDirections.value(direction));
    }

    QSet<int> usableStubDirections{fromRoomStubs};
    usableStubDirections.detach();
    usableStubDirections = usableStubDirections.intersect(toReverseStubDirections);
    // Now we need to count how big this set is:
    if (usableStubDirections.isEmpty()) {
        return qsl("no pairs of reverse stubs found between rooms %1 and %2").arg(QString::number(fromRoomId), QString::number(toRoomId));
    }
    if (usableStubDirections.count() > 1) {
        QStringList useableStubDirectionTexts;
        QSetIterator<int> itUseableStub(usableStubDirections);
        while (itUseableStub.hasNext()) {
            auto direction = itUseableStub.next();
            useableStubDirectionTexts << qsl("'%1' (%2)").arg(TRoom::dirCodeToString(direction), QString::number(direction));
        }
        return qsl("multiple pairs of reverse stubs found between rooms %1 and %2, please try again with the three argument function and one of the follow directions: %3")
                .arg(QString::number(fromRoomId), QString::number(toRoomId), useableStubDirectionTexts.join(QLatin1String(", ")));
    }

    // else we must have just one direction:
    const int usableStubDirection = *(usableStubDirections.constBegin());
    setExit(fromRoomId, toRoomId, usableStubDirection);
    setExit(toRoomId, fromRoomId, scmReverseDirections.value(usableStubDirection));
    setUnsaved(__func__);
    return {};
}

// Will connect an exit stub in the indicated direction from the fromRoomId
// numbered room to the toRoomId numbered room and also connect the
// corresponding stub exit in the reverse direction of the toRoomId room back to
// the fromRoomId provided the second room has a stub in the reverse direction.
// Unlike the connectExitStubByDirection(...) method the relative placement of
// the two rooms is not considered - and indeed the toRoomId room need not be
// IN THE SAME AREA as the fromRoomId numbered room - otherwise it will report
// the reason why it cannot.
QString TMap::connectExitStubByDirectionAndToId(const int fromRoomId, const int dirType, const int toRoomId)
{
    Q_ASSERT_X(scmReverseDirections.contains(dirType), "TMap::connectExitStubByDirectionAndToId(...)", "there is no scmReverseDirections.value() for the given dirType");

    auto pFromR = mpRoomDB->getRoom(fromRoomId);
    if (!pFromR) {
        return qsl("fromID (%1) does not exist").arg(fromRoomId);
    }

    if (toRoomId == fromRoomId) {
        return qsl("fromID and toID are the same (%1)").arg(fromRoomId);
    }

    if (!pFromR->exitStubs.contains(dirType)) {
        return qsl("fromID (%1) does not have an exit stub in the given direction '%2' (%3)")
                .arg(QString::number(fromRoomId), TRoom::dirCodeToString(dirType), QString::number(dirType));
    }

    auto pToR = mpRoomDB->getRoom(toRoomId);
    if (!pToR) {
        return qsl("toID (%1) room does not exist").arg(toRoomId);
    }

    if (!pToR->exitStubs.contains(scmReverseDirections.value(dirType))) {
        return qsl("toID (%1) does not have an exit stub in the reverse direction '%2' (%3) of that given '%4' (%5)")
                .arg(QString::number(toRoomId),
                     TRoom::dirCodeToString(scmReverseDirections.value(dirType)),
                     QString::number(scmReverseDirections.value(dirType)),
                     TRoom::dirCodeToString(dirType),
                     QString::number(dirType));
    }

    setExit(fromRoomId, toRoomId, dirType);
    setExit(toRoomId, fromRoomId, scmReverseDirections.value(dirType));
    setUnsaved(__func__);
    return {};
}

int TMap::createNewRoomID(int minimumId)
{
    int _id = 0;
    if (minimumId > 0) {
        _id = minimumId - 1;
    }

    do {
        ; // Empty loop as increment done in test
    } while (mpRoomDB->getRoom(++_id));

    return _id;
}

bool TMap::setExit(int from, int to, int dir)
{
    // FIXME: This along with TRoom->setExit need to be unified to a controller.
    TRoom* pR = mpRoomDB->getRoom(from);
    TRoom* pR_to = mpRoomDB->getRoom(to);

    if (!pR) {
        return false;
    }
    if (!pR_to && to > 0) {
        return false;
    }
    if (to < 1) {
        to = -1;
    }

    bool ret = true;

    switch (dir) {
    case DIR_NORTH:
        pR->setNorth(to);
        break;
    case DIR_NORTHEAST:
        pR->setNortheast(to);
        break;
    case DIR_NORTHWEST:
        pR->setNorthwest(to);
        break;
    case DIR_EAST:
        pR->setEast(to);
        break;
    case DIR_WEST:
        pR->setWest(to);
        break;
    case DIR_SOUTH:
        pR->setSouth(to);
        break;
    case DIR_SOUTHEAST:
        pR->setSoutheast(to);
        break;
    case DIR_SOUTHWEST:
        pR->setSouthwest(to);
        break;
    case DIR_UP:
        pR->setUp(to);
        break;
    case DIR_DOWN:
        pR->setDown(to);
        break;
    case DIR_IN:
        pR->setIn(to);
        break;
    case DIR_OUT:
        pR->setOut(to);
        break;
    default:
        ret = false;
    }
    pR->setExitStub(dir, false);
    mMapGraphNeedsUpdate = true;
    TArea* pA = mpRoomDB->getArea(pR->getArea());
    if (!pA) {
        return false;
    }
    pA->determineAreaExitsOfRoom(pR->getId());
    mpRoomDB->updateEntranceMap(pR);
    setUnsaved(__func__);
    return ret;
}

void TMap::audit()
{
    // init areas
    QElapsedTimer _time;
    _time.start();

    { // Blocked - just to limit the scope of infoMsg...!
        const QString infoMsg = tr("[ INFO ]  - Map audit starting...");
        postMessage(infoMsg);
    }

    // The old mpRoomDB->initAreasForOldMaps() was a subset of these checks

    QHash<int, int> roomRemapping; // These are populated by the auditRooms(...)
    QHash<int, int> areaRemapping; // call and contain "Keys" of old ids and
                                   // "Values" of new ids to use in their stead

    if (mVersion < 16) {
        // convert old style labels, wasn't made version conditional in past but
        // not likely to be an issue in recent map file format versions (say 16+)

        QMapIterator<int, TArea*> itArea(mpRoomDB->getAreaMap());
        while (itArea.hasNext()) {
            itArea.next();
            const int areaID = itArea.key();
            TArea* pArea = mpRoomDB->getArea(areaID);
            if (!pArea) {
                continue;
            }
            if (!pArea->mMapLabels.isEmpty()) {
                QList<int> const labelIDList = pArea->mMapLabels.keys();
                for (const int& i : labelIDList) {
                    TMapLabel const l = pArea->mMapLabels.value(i);
                    if (l.pix.isNull()) {
                        // Note that two of the last three arguments here
                        // (false, 40.0) are not the defaults (true, 30.0) used
                        // now:
                        const int newID = createMapLabel(areaID, l.text, l.pos.x(), l.pos.y(), l.pos.z(), l.fgColor, l.bgColor, true, false, false, 40.0, 50, std::nullopt, l.fgColor);
                        if (newID > -1) {
                            if (mudlet::self()->showMapAuditErrors()) {
                                const QString msg = tr("[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2.").arg(areaID).arg(i);
                                postMessage(msg);
                            }
                            appendAreaErrorMsg(areaID, tr("[ INFO ] - Converting old style label id: %1.").arg(i));
                            pArea->mMapLabels[i] = pArea->mMapLabels.take(newID);

                        } else {
                            if (mudlet::self()->showMapAuditErrors()) {
                                const QString msg = tr("[ WARN ] - CONVERTING: cannot convert old style label in area with id: %1,  label id is: %2.").arg(areaID).arg(i);
                                postMessage(msg);
                            }
                            appendAreaErrorMsg(areaID, tr("[ WARN ] - CONVERTING: cannot convert old style label with id: %1.").arg(i));
                        }
                    }
                    if ((l.size.width() > std::numeric_limits<qreal>::max()) || (l.size.width() < -std::numeric_limits<qreal>::max())) {
                        pArea->mMapLabels[i].size.setWidth(l.pix.width());
                    }
                    if ((l.size.height() > std::numeric_limits<qreal>::max()) || (l.size.height() < -std::numeric_limits<qreal>::max())) {
                        pArea->mMapLabels[i].size.setHeight(l.pix.height());
                    }
                }
            }
        }
    }

    mpRoomDB->auditRooms(roomRemapping, areaRemapping);

    // The second half of old mpRoomDB->initAreasForOldMaps() - needed to fixup
    // all the (TArea *)->areaExits() that were built wrongly previously,
    // calcSpan() may not be required to be done here and now but it is in my
    // sights as a target for revision in the future. Slysven
    QMapIterator<int, TArea*> itArea(mpRoomDB->getAreaMap());
    while (itArea.hasNext()) {
        itArea.next();
        itArea.value()->clean();
    }

    { // Blocked - just to limit the scope of infoMsg...!
        const QString infoMsg = tr("[  OK  ]  - Auditing of map completed (%1s). Enjoy your game...").arg(_time.nsecsElapsed() * 1.0e-9, 0, 'f', 2);
        postMessage(infoMsg);
        appendErrorMsg(infoMsg);
    }

    mpHost->getLuaInterpreter()->condenseMapLoad();
}

// This may be duplicating TArea class functionality:
QList<int> TMap::detectRoomCollisions(int id)
{
    QList<int> collList;
    TRoom* pR = mpRoomDB->getRoom(id);
    if (!pR) {
        return collList;
    }
    const int area = pR->getArea();
    const int x = pR->x();
    const int y = pR->y();
    const int z = pR->z();
    TArea* pA = mpRoomDB->getArea(area);
    if (!pA) {
        return collList;
    }

    QSetIterator<int> itRoom(pA->getAreaRooms());
    while (itRoom.hasNext()) {
        const int checkRoomId = itRoom.next();
        pR = mpRoomDB->getRoom(checkRoomId);
        if (!pR) {
            continue;
        }
        if (pR->x() == x && pR->y() == y && pR->z() == z) {
            collList.push_back(checkRoomId);
        }
    }

    return collList;
}

// Not used:
//void TMap::astBreitenAnpassung( int id, int id2 )
//{
//}

//void TMap::astHoehenAnpassung( int id, int id2 )
//{
//}

//void TMap::getConnectedNodesGreaterThanX( int id, int min )
//{
//}

//void TMap::getConnectedNodesSmallerThanX( int id, int min )
//{
//}

//void TMap::getConnectedNodesGreaterThanY( int id, int min )
//{
//}

//void TMap::getConnectedNodesSmallerThanY( int id, int min )
//{
//}

bool TMap::gotoRoom(int r)
{
    mTargetID = r;
    return findPath(mRoomIdHash.value(mProfileName), r);
}

// As can be seen this only sets the target and start point for a path find
// the speedwalk is instigated by the Host class caller...
bool TMap::gotoRoom(int r1, int r2)
{
    return findPath(r1, r2);
}

void TMap::addDirectionalRoute(QHash<unsigned int, route>& bestRoutes,
                               const QMap<QString, int>& exitWeights,
                               unsigned int source,
                               TRoom* pSourceR,
                               int target,
                               quint8 direction,
                               const QString& exitKey,
                               const QSet<unsigned int>& unUsableRoomSet)
{
    // Skip self-edges and any exits that lead to rooms already known to be unusable.
    if (target <= 0 || static_cast<int>(source) == target) {
        return;
    }

    TLuaInterpreter* interpreter = mpHost ? mpHost->getLuaInterpreter() : nullptr;
    TLuaInterpreter::ExitWeightFilterResult filterResult;
    TLuaInterpreter::ExitWeightFilterResult* filterResultPtr = nullptr;
    if (interpreter && interpreter->hasExitWeightFilter()) {
        filterResult = interpreter->applyExitWeightFilter(static_cast<int>(source), exitKey);
        if (filterResult.blocked) {
            return;
        }
        filterResultPtr = &filterResult;
    }

    const bool filterOverridesBlocks = filterResultPtr && filterResultPtr->weightOverride.has_value();

    if (pSourceR->isLocked && !filterOverridesBlocks) {
        return;
    }

    const bool isSpecialExit = direction == DIR_OTHER;

    if (!filterOverridesBlocks) {
        if (isSpecialExit) {
            if (pSourceR->hasSpecialExitLock(exitKey)) {
                return;
            }
        } else if (pSourceR->hasExitLock(direction)) {
            return;
        }
    }

    TRoom* pTargetR = mpRoomDB->getRoom(target);
    if (!pTargetR) {
        return;
    }

    if (!filterOverridesBlocks && (pTargetR->isLocked || unUsableRoomSet.contains(target))) {
        return;
    }

    route r;
    r.direction = direction;
    if (isSpecialExit) {
        r.specialExitName = exitKey;
    }

    int cost = exitWeights.value(exitKey, pTargetR->getWeight());
    if (filterOverridesBlocks) {
        cost = filterResultPtr->weightOverride.value();
    }
    r.cost = cost;

    if (!bestRoutes.contains(target) || bestRoutes.value(target).cost > r.cost) {
        bestRoutes.insert(target, r);
    }
}

void TMap::initGraph()
{
    QElapsedTimer _time;
    _time.start();
    locations.clear();
    roomidToIndex.clear();
    g.clear();
    g = mygraph_t();
    unsigned int roomCount = 0;
    unsigned int edgeCount = 0;
    QSet<unsigned int> unUsableRoomSet;
    TLuaInterpreter* interpreter = mpHost ? mpHost->getLuaInterpreter() : nullptr;
    const bool exitWeightFilterActive = interpreter && interpreter->hasExitWeightFilter();
    // Keep track of the unusable rather than the usable ones because that is
    // hopefully a MUCH smaller set in normal situations!
    QHashIterator<int, TRoom*> itRoom = mpRoomDB->getRoomMap();
    while (itRoom.hasNext()) {
        itRoom.next();
        TRoom* pR = itRoom.value();
        if (itRoom.key() < 1 || !pR) {
            unUsableRoomSet.insert(itRoom.key());
            continue;
        }

        if (pR->isLocked) {
            unUsableRoomSet.insert(itRoom.key());
            if (!exitWeightFilterActive) {
                continue;
            }
        }

        location l;
        l.pR = pR;
        l.id = itRoom.key();
        // locations is std::vector<location> and (locations.at(k)).id will give room ID value
        locations.push_back(l);
        // This command maps usable TRooms (key) to index of entry in locations (for route finding).
        // It loses invalid and unusable (i.e. locked) rooms
        roomidToIndex.insert(itRoom.key(), roomCount++);
    }

    for (unsigned int i = 0; i < roomCount; ++i) {
        boost::add_vertex(g);
    }

    // Now identify the routes between rooms, and pick out the best edges of parallel ones
    for (auto l : locations) {
        unsigned const int source = l.id;
        TRoom* pSourceR = l.pR;
        QHash<unsigned int, route> bestRoutes;
        // key is target (destination room),
        // value is data we will need to store later,
        QMap<QString, int> const exitWeights = pSourceR->getExitWeights();

        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getNorth(), DIR_NORTH, qsl("n"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getEast(), DIR_EAST, qsl("e"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getSouth(), DIR_SOUTH, qsl("s"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getWest(), DIR_WEST, qsl("w"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getUp(), DIR_UP, qsl("up"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getDown(), DIR_DOWN, qsl("down"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getNortheast(), DIR_NORTHEAST, qsl("ne"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getSoutheast(), DIR_SOUTHEAST, qsl("se"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getSouthwest(), DIR_SOUTHWEST, qsl("sw"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getNorthwest(), DIR_NORTHWEST, qsl("nw"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getIn(), DIR_IN, qsl("in"), unUsableRoomSet);
        addDirectionalRoute(bestRoutes, exitWeights, source, pSourceR, pSourceR->getOut(), DIR_OUT, qsl("out"), unUsableRoomSet);

        QMapIterator<QString, int> itSpecialExit(pSourceR->getSpecialExits());
        while (itSpecialExit.hasNext()) {
            itSpecialExit.next();

            addDirectionalRoute(bestRoutes,
                                exitWeights,
                                source,
                                pSourceR,
                                itSpecialExit.value(),
                                DIR_OTHER,
                                itSpecialExit.key(),
                                unUsableRoomSet);
        } // End of while(itSpecialExit.hasNext())

        // Now we have eliminated possible duplicate and useless edges we can create and
        // insert the remainder into the BGL graph:
        QHashIterator<unsigned int, route> itRoute = bestRoutes;
        while (itRoute.hasNext()) {
            itRoute.next();
            edge_descriptor e;
            bool inserted; // This is always going to be false as it gets set if
                           // we had tried to insert a parallel edge into a graph
                           // that does not support them - but we've just been
                           // and disposed of those already!
            tie(e, inserted) = add_edge(roomidToIndex.value(source), roomidToIndex.value(itRoute.key()), itRoute.value().cost, g);
            edgeHash.insert(qMakePair(source, itRoute.key()), itRoute.value());
            // The key is made from the QPair<edgeSourceRoomId, edgeTargetRoomId>...
            edgeCount++;
        }
    } // End of foreach(location l, locations)

    mMapGraphNeedsUpdate = false;
    qDebug() << "TMap::initGraph() INFO: built graph with:" << locations.size() << "(" << roomCount << ") locations(roomCount), and discarded" << unUsableRoomSet.count()
             << "other NOT usable rooms and found:" << edgeCount << "distinct, usable edges in:" << _time.nsecsElapsed() * 1.0e-6 << "ms.";
}

bool TMap::findPath(int from, int to)
{
    if (mMapGraphNeedsUpdate) {
        initGraph();
    }

    QElapsedTimer t;
    t.start();

    mPathList.clear();
    mDirList.clear();
    mWeightList.clear();
    // Clear the previous path data here so that if the following test is
    // passed, the data is empty - and valid for THAT case!

    if (from == to) {
        return true; // Take a short-cut for trivial "already there" case!
    }

    TRoom* pFrom = mpRoomDB->getRoom(from);
    TRoom* pTo = mpRoomDB->getRoom(to);

    if (!pFrom || !pTo) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: NULL TRoom pointer for start or target rooms!";
        return false;
    }

    bool hasUsableExit = false;

    if (pFrom->getNorth() > 0 && (!pFrom->hasExitLock(DIR_NORTH))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getSouth() > 0 && (!pFrom->hasExitLock(DIR_SOUTH))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getWest() > 0 && (!pFrom->hasExitLock(DIR_WEST))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getEast() > 0 && (!pFrom->hasExitLock(DIR_EAST))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getUp() > 0 && (!pFrom->hasExitLock(DIR_UP))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getDown() > 0 && (!pFrom->hasExitLock(DIR_DOWN))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getNortheast() > 0 && (!pFrom->hasExitLock(DIR_NORTHEAST))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getNorthwest() > 0 && (!pFrom->hasExitLock(DIR_NORTHWEST))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getSoutheast() > 0 && (!pFrom->hasExitLock(DIR_SOUTHEAST))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getSouthwest() > 0 && (!pFrom->hasExitLock(DIR_SOUTHWEST))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getIn() > 0 && (!pFrom->hasExitLock(DIR_IN))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit && pFrom->getOut() > 0 && (!pFrom->hasExitLock(DIR_OUT))) {
        hasUsableExit = true;
    }
    if (!hasUsableExit) {
        // No available normal exits from this room so check the special ones
        QStringList specialExitCommands = pFrom->getSpecialExits().keys();
        while (!specialExitCommands.isEmpty()) {
            if (!pFrom->hasSpecialExitLock(specialExitCommands.at(0))) {
                hasUsableExit = true;
                break;
            }
            specialExitCommands.removeFirst();
        }
    }
    if (!hasUsableExit) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: no usable exits from start room!";
        return false; // No available exits from the start room so give up!
    }

    if (!roomidToIndex.contains(from)) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: start room not in map graph!";
        return false;
        // The start room is NOT one that has been included in the BGL graph
        // probably because it is locked - so no route finding can be done
    }
    vertex const start = roomidToIndex.value(from);

    if (!roomidToIndex.contains(to)) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: target room not in map graph!";
        return false;
        // The target room is NOT one that has been included in the BGL graph
        // probably because it is locked - so no route finding can be done
    }
    vertex const goal = roomidToIndex.value(to);

    const auto vertexCount = static_cast<std::size_t>(num_vertices(g));
    if (vertexCount == 0) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: map graph has no vertices.";
        return false;
    }

    if (static_cast<std::size_t>(start) >= vertexCount || static_cast<std::size_t>(goal) >= vertexCount) {
        qWarning().nospace().noquote() << "TMap::findPath(" << from << "," << to
                                       << ") FAIL: start or target vertex outside of graph range (vertexCount=" << vertexCount
                                       << ").";
        return false;
    }

    std::vector<vertex> p(vertexCount);
    // Somehow p is an ascending, monotonic series of numbers start at 0, it
    // seems we have a redundant indirection in play there as p[0]=0, p[1]=1,..., p[n]=n ...!
    std::vector<cost> d(vertexCount);
    try {
        astar_search(g, start, distance_heuristic<mygraph_t, cost, std::vector<location>>(locations, goal), predecessor_map(&p[0]).distance_map(&d[0]).visitor(astar_goal_visitor<vertex>(goal)));
    } catch (found_goal) {
        qDebug() << "TMap::findPath(" << from << "," << to << ") INFO: time elapsed in A*:" << t.nsecsElapsed() * 1.0e-6 << "ms.";
        t.restart();
        if (!roomidToIndex.contains(to)) {
            qDebug() << "TMap::findPath(" << from << "," << to << ") FAIL: target room not in map graph!";
            return false;
        }

        vertex currentVertex = roomidToIndex.value(to);
        unsigned int currentRoomId = (locations.at(currentVertex)).id;

        // We step through the found path BACKWARDS so advance (well retard)
        // the "previous" one first, and it will be the SOURCE vertex for the
        // edge and current will be the TARGET vertex:
        vertex previousVertex = currentVertex;
        do {
            previousVertex = p[currentVertex];
            if (previousVertex == currentVertex) {
                qDebug() << "TMap::findPath(" << from << "," << to << ") WARN: unable to build a path in:" << t.nsecsElapsed() * 1.0e-6 << "ms.";
                mPathList.clear();
                mDirList.clear();
                mWeightList.clear(); // Reset any partial results...
                return false;
            }
            const unsigned int previousRoomId = (locations.at(previousVertex)).id;
            QPair<unsigned int, unsigned int> const edgeRoomIdPair = qMakePair(previousRoomId, currentRoomId);
            const route r = edgeHash.value(edgeRoomIdPair);
            mPathList.prepend(currentRoomId);
            Q_ASSERT_X(r.cost > 0, "TMap::findPath()", "broken path {QPair made from source and target roomIds for a path step NOT found in QHash table of all possible steps.}");
            // Above was found to be triggered by the situation described in:
            // https://bugs.launchpad.net/mudlet/+bug/1263447 on 2015-07-17 but
            // this is because previousVertex was the same as currentVertex after
            // the "previousVertex = p[currentVertex]" operation at the start of
            // the do{} loop - added a test for this so should bail out if it
            // happens - Slysven
            mWeightList.prepend(r.cost);
            switch (r.direction) {
                /*
                 * Do not translate the directions into the user's locale here,
                 * that is to be done in the profile specific doSpeedwalk()
                 * function of the mapper package as the language of the MUD
                 * need not be the native language of the user - translating
                 * them here makes the mapper harder to code as it has to
                 * accommodate all the possible languages the GUI of Mudlet was
                 * configured to support!
                 */
            case DIR_NORTH:        mDirList.prepend(qsl("n"));             break;
            case DIR_NORTHEAST:    mDirList.prepend(qsl("ne"));            break;
            case DIR_EAST:         mDirList.prepend(qsl("e"));             break;
            case DIR_SOUTHEAST:    mDirList.prepend(qsl("se"));            break;
            case DIR_SOUTH:        mDirList.prepend(qsl("s"));             break;
            case DIR_SOUTHWEST:    mDirList.prepend(qsl("sw"));            break;
            case DIR_WEST:         mDirList.prepend(qsl("w"));             break;
            case DIR_NORTHWEST:    mDirList.prepend(qsl("nw"));            break;
            case DIR_UP:           mDirList.prepend(qsl("up"));            break;
            case DIR_DOWN:         mDirList.prepend(qsl("down"));          break;
            case DIR_IN:           mDirList.prepend(qsl("in"));            break;
            case DIR_OUT:          mDirList.prepend(qsl("out"));           break;
            case DIR_OTHER:        mDirList.prepend(r.specialExitName);    break;
            default:            qWarning().nospace().noquote() << "TMap::findPath(" << from << ", " << to << ") WARNING - found route between rooms (from id: " << previousRoomId << ", to id: " << currentRoomId << ") with an invalid DIR_xxxx code: " << r.direction << " - the path will not be valid!";
            }
            currentVertex = previousVertex;
            currentRoomId = previousRoomId;
        } while (currentVertex != start);

        qDebug() << "TMap::findPath(" << from << "," << to << ") INFO: found path in:" << t.nsecsElapsed() * 1.0e-6 << "ms.";
        return true;
    }

    qDebug() << "TMap::findPath(" << from << "," << to << ") INFO: did NOT find path in:" << t.nsecsElapsed() * 1.0e-6 << "ms.";
    return false;
}

bool TMap::serialize(QDataStream& ofs, int saveVersion)
{
    // clamp version values
    if (saveVersion < 0) {
        saveVersion = 0;
    } else if (saveVersion > mMaxVersion) {
        saveVersion = mMaxVersion;
        const QString errMsg = tr("[ ERROR ] - The format version \"%1\" you are trying to save the map with is too new\n"
                             "for this version of Mudlet. Supported are only formats up to version %2.")
                                 .arg(QString::number(saveVersion), QString::number(mMaxVersion));
        appendErrorMsgWithNoLf(errMsg, false);
        postMessage(errMsg);
        return false;
    }

    auto oldSaveVersion = mSaveVersion;

    // if 0 we default to current version selected
    if (saveVersion != 0) {
        mSaveVersion = saveVersion;
    }

    if (mSaveVersion != mVersion) {
        const QString message = tr("[ ALERT ] - Saving map in format version \"%1\" that is different than \"%2\" which\n"
                             "it was loaded as. This may be an issue if you want to share the resulting\n"
                             "map with others relying on the original format.")
                                  .arg(mSaveVersion)
                                  .arg(mVersion);
        appendErrorMsgWithNoLf(message, false);
        mpHost->mTelnet.postMessage(message);
    }

    if (mSaveVersion != mDefaultVersion) {
        const QString message = tr("[ WARN ]  - Saving map in format version \"%1\" different from the\n"
                             "recommended map version %2 for this version of Mudlet.")
                                  .arg(mSaveVersion)
                                  .arg(mDefaultVersion);
        appendErrorMsgWithNoLf(message, false);
        postMessage(message);
    }

    ofs << mSaveVersion;
    ofs << mEnvColors;
    ofs << mpRoomDB->getAreaNamesMap();
    ofs << mCustomEnvColors;
    ofs << mpRoomDB->hashToRoomID;
    if (mSaveVersion < 19) {
        // Save the data in the map user data for older versions
        mUserData.insert(qsl("system.fallback_mapSymbolFont"), mMapSymbolFont.toString());
        mUserData.insert(qsl("system.fallback_mapSymbolFontFudgeFactor"), QString::number(mMapSymbolFontFudgeFactor));
        mUserData.insert(qsl("system.fallback_onlyUseMapSymbolFont"), mIsOnlyMapSymbolFontToBeUsed ? qsl("true") : qsl("false"));
    }
    ofs << mUserData;
    if (mSaveVersion >= 19) {
        // Save the data directly in supported format versions (19 and above)
        ofs << mMapSymbolFont;
        ofs << mMapSymbolFontFudgeFactor;
        ofs << mIsOnlyMapSymbolFontToBeUsed;
    }

    // Qt 6 changed the return type of QMap<T1, T2>::size() to qsizetype which
    // is an int64 rather than the int32 of Qt5 - but we need to use the latter
    // to retain compatibility:
    ofs << static_cast<qint32>(mpRoomDB->getAreaMap().size());

    // serialize area table
    QMapIterator<int, TArea*> itAreaList(mpRoomDB->getAreaMap());
    while (itAreaList.hasNext()) {
        itAreaList.next();
        const int areaID = itAreaList.key();
        TArea* pA = itAreaList.value();
        ofs << areaID;
        if (mSaveVersion >= 18) {
            ofs << pA->rooms;
        } else {
            // Switched to a (faster) QSet<int> from a QList<int> in version 18
            QList<int> const _oldList = pA->rooms.values();
            ofs << _oldList;
        }
        ofs << pA->zLevels;
        ofs << pA->mAreaExits;
        ofs << pA->gridMode;
        ofs << pA->max_x;
        ofs << pA->max_y;
        ofs << pA->max_z;
        ofs << pA->min_x;
        ofs << pA->min_y;
        ofs << pA->min_z;
        ofs << pA->span;
        ofs << pA->xmaxForZ;
        ofs << pA->ymaxForZ;
        ofs << pA->xminForZ;
        ofs << pA->yminForZ;
        ofs << pA->pos;
        ofs << pA->isZone;
        ofs << pA->zoneAreaRef;
        if (mSaveVersion >= 21) {
            // Revised in version 21 to store the value directly:
            ofs << pA->mLast2DMapZoom;
        } else {
            pA->mUserData.insert(QLatin1String("system.fallback_map2DZoom"), QString::number(pA->get2DMapZoom()));
        }
        ofs << pA->mUserData;
        if (mSaveVersion >= 21) {
            // Revised in version 21 to store labels within the TArea class:
            // Also we now have temporary labels, so we need to count the
            // permanent ones first to use as the count for ones to store:
            const auto permanentLabelsList{pA->getPermanentLabelIds()};
            ofs << static_cast<qint32>(permanentLabelsList.size());
            QListIterator<int> itMapLabelId(permanentLabelsList);
            while (itMapLabelId.hasNext()) {
                const auto labelID = itMapLabelId.next();
                const auto label = pA->mMapLabels.value(labelID);
                ofs << labelID;
                ofs << label.pos;
                ofs << label.size;
                ofs << label.text;
                ofs << label.fgColor;
                ofs << label.bgColor;
                ofs << label.pix;
                ofs << label.noScaling;
                ofs << label.showOnTop;
            }
        }
    }

    if (mSaveVersion >= 18) {
        // Revised in version 18 to store mRoomId as a per profile case so that
        // sharing/copying between profiles respects each profile's player
        // location
        ofs << mRoomIdHash;
    } else {
        ofs << mRoomIdHash.value(mProfileName);
    }

    if (mSaveVersion < 21) {
        // Before version 21 the map labels were stored within this class:
        // First we have the number of labels per area - we need this as there
        // is no delimiter between each area's map labels
        QMap<int, TArea*> areasWithPermanentLabels;
        // Need to count the areas that have mapLabels:
        QMapIterator<int, TArea*> itArea(mpRoomDB->getAreaMap());
        while (itArea.hasNext()){
            // Now we have temporary labels we need to identify areas with
            // permanent ones:
            itArea.next();
            auto pArea = itArea.value();
            if (pArea && !pArea->mMapLabels.isEmpty() && pArea->hasPermanentLabels()) {
                areasWithPermanentLabels.insert(itArea.key(), itArea.value());
            }
        }
        ofs << static_cast<qint32>(areasWithPermanentLabels.count());
        QMapIterator<int, TArea*> itAreaWithLabels(areasWithPermanentLabels);
        while (itAreaWithLabels.hasNext()) {
            itAreaWithLabels.next();
            auto pArea = itAreaWithLabels.value();
            auto permanentLabelIdsList = pArea->getPermanentLabelIds();
            // number of (permanent) labels in this area:
            ofs << static_cast<qint32>(permanentLabelIdsList.size());

            // only used to assign labels to the area:
            ofs << itAreaWithLabels.key();
            QListIterator<int> itPerminentMapLabelIds(permanentLabelIdsList);
            while (itPerminentMapLabelIds.hasNext()) {
                auto labelID = itPerminentMapLabelIds.next();
                ofs << labelID; //label ID
                TMapLabel const label = pArea->mMapLabels.value(labelID);
                ofs << label.pos;
                ofs << QPointF(); // dummy value - not actually used
                ofs << label.size;
                ofs << label.text;
                ofs << label.fgColor;
                ofs << label.bgColor;
                ofs << label.pix;
                ofs << label.noScaling;
                ofs << label.showOnTop;
            }
        }
    }

    QHashIterator<int, TRoom*> it(mpRoomDB->getRoomMap());
    while (it.hasNext()) {
        it.next();
        TRoom* pR = it.value();
        if (!pR) {
            qDebug() << "TMap::serialize(...) skipping a room with a NULL TRoom pointer:" << it.key();
            continue;
        }

        ofs << pR->getId();
        if (mSaveVersion <= 19) {
            if (!pR->mSymbol.isEmpty()) {
                pR->userData.insert(QLatin1String("system.fallback_symbol"), pR->mSymbol);
            }
        }
        ofs << pR->getArea();
        ofs << pR->x();
        ofs << pR->y();
        ofs << pR->z();
        ofs << pR->getNorth();
        ofs << pR->getNortheast();
        ofs << pR->getEast();
        ofs << pR->getSoutheast();
        ofs << pR->getSouth();
        ofs << pR->getSouthwest();
        ofs << pR->getWest();
        ofs << pR->getNorthwest();
        ofs << pR->getUp();
        ofs << pR->getDown();
        ofs << pR->getIn();
        ofs << pR->getOut();
        ofs << pR->environment;
        ofs << pR->getWeight();
        ofs << pR->name;
        ofs << pR->isLocked;
        if (mSaveVersion >= 21) {
            ofs << pR->getSpecialExits();
        } else {
            QMultiMap<int, QString> oldSpecialExits;
            QMapIterator<QString, int> itSpecialExit(pR->getSpecialExits());
            while (itSpecialExit.hasNext()) {
                itSpecialExit.next();
                oldSpecialExits.insert(itSpecialExit.value(),
                                       (pR->hasSpecialExitLock(itSpecialExit.key())
                                                ? QLatin1Char('1')
                                                : QLatin1Char('0'))
                                               % itSpecialExit.key());
            }
            ofs << oldSpecialExits;
        }
        if (mSaveVersion >= 19) {
            ofs << pR->mSymbol;
        } else {
            qint8 oldCharacterCode = 0;
            if (pR->mSymbol.length()) {
                // There is something for a symbol
                const QChar firstChar = pR->mSymbol.at(0);
                if (pR->mSymbol.length() == 1 && firstChar.row() == 0 && firstChar.cell() > 32) {
                    // It is something that can be represented by the past unsigned short
                    oldCharacterCode = firstChar.toLatin1();
                } else {
                    // Not representable - put in a '?' for older Mudlet
                    // versions that cannot display the character and will not
                    // parse the value placed in the room's user data:
                    oldCharacterCode = QChar('?').toLatin1();
                }
            }
            ofs << oldCharacterCode;
        }

        if (mSaveVersion >= 21) {
            ofs << pR->mSymbolColor;
        } else {
            if (pR->mSymbolColor.isValid()) {
                pR->userData.insert(QLatin1String("system.fallback_symbol_color"), pR->mSymbolColor.name());
            }
        }

        ofs << pR->userData;
        if (mSaveVersion >= 20) {
            // Before version 20 stored the style as an Latin1 string, the color
            // as a QList<int> for the RGB components and used UPPER case for
            // the NORMAL exit direction keys...
            ofs << pR->customLines;
            ofs << pR->customLinesArrow;
            ofs << pR->customLinesColor;
            ofs << pR->customLinesStyle;
        } else {
            QMap<QString, QList<QPointF>> oldLinesData;
            QMapIterator<QString, QList<QPointF>> itCustomLine(pR->customLines);
            while (itCustomLine.hasNext()) {
                itCustomLine.next();
                const QString direction(itCustomLine.key());
                if (direction == QLatin1String("n") || direction == QLatin1String("e") || direction == QLatin1String("s") || direction == QLatin1String("w") || direction == QLatin1String("up")
                    || direction == QLatin1String("down")
                    || direction == QLatin1String("ne")
                    || direction == QLatin1String("se")
                    || direction == QLatin1String("sw")
                    || direction == QLatin1String("nw")
                    || direction == QLatin1String("in")
                    || direction == QLatin1String("out")) {
                    oldLinesData.insert(itCustomLine.key().toUpper(), itCustomLine.value());
                } else {
                    oldLinesData.insert(itCustomLine.key(), itCustomLine.value());
                }
            }
            ofs << oldLinesData;

            QMap<QString, bool> oldLinesArrowData;
            QMapIterator<QString, bool> itCustomLineArrow(pR->customLinesArrow);
            while (itCustomLineArrow.hasNext()) {
                itCustomLineArrow.next();
                const QString direction(itCustomLineArrow.key());
                if (direction == QLatin1String("n") || direction == QLatin1String("e") || direction == QLatin1String("s") || direction == QLatin1String("w") || direction == QLatin1String("up")
                    || direction == QLatin1String("down")
                    || direction == QLatin1String("ne")
                    || direction == QLatin1String("se")
                    || direction == QLatin1String("sw")
                    || direction == QLatin1String("nw")
                    || direction == QLatin1String("in")
                    || direction == QLatin1String("out")) {
                    oldLinesArrowData.insert(itCustomLineArrow.key().toUpper(), itCustomLineArrow.value());
                } else {
                    oldLinesArrowData.insert(itCustomLineArrow.key(), itCustomLineArrow.value());
                }
            }
            ofs << oldLinesArrowData;

            QMap<QString, QList<int>> oldLinesColorData;
            QMapIterator<QString, QColor> itCustomLineColor(pR->customLinesColor);
            while (itCustomLineColor.hasNext()) {
                itCustomLineColor.next();
                const QString direction(itCustomLineColor.key());
                QList<int> colorComponents;
                colorComponents << itCustomLineColor.value().red() << itCustomLineColor.value().green() << itCustomLineColor.value().blue();
                if (direction == QLatin1String("n") || direction == QLatin1String("e") || direction == QLatin1String("s") || direction == QLatin1String("w") || direction == QLatin1String("up")
                    || direction == QLatin1String("down")
                    || direction == QLatin1String("ne")
                    || direction == QLatin1String("se")
                    || direction == QLatin1String("sw")
                    || direction == QLatin1String("nw")
                    || direction == QLatin1String("in")
                    || direction == QLatin1String("out")) {
                    oldLinesColorData.insert(itCustomLineColor.key().toUpper(), colorComponents);
                } else {
                    oldLinesColorData.insert(itCustomLineColor.key(), colorComponents);
                }
            }
            ofs << oldLinesColorData;

            QMap<QString, QString> oldLineStyleData;
            QMapIterator<QString, Qt::PenStyle> itCustomLineStyle(pR->customLinesStyle);
            while (itCustomLineStyle.hasNext()) {
                itCustomLineStyle.next();
                QString direction(itCustomLineStyle.key());
                if (direction == QLatin1String("n") || direction == QLatin1String("e") || direction == QLatin1String("s") || direction == QLatin1String("w") || direction == QLatin1String("up")
                    || direction == QLatin1String("down")
                    || direction == QLatin1String("ne")
                    || direction == QLatin1String("se")
                    || direction == QLatin1String("sw")
                    || direction == QLatin1String("nw")
                    || direction == QLatin1String("in")
                    || direction == QLatin1String("out")) {
                    direction = direction.toUpper();
                }
                switch (itCustomLineStyle.value()) {
                case Qt::DotLine:
                    oldLineStyleData.insert(direction, QLatin1String("dot line"));
                    break;
                case Qt::DashLine:
                    oldLineStyleData.insert(direction, QLatin1String("dash line"));
                    break;
                case Qt::DashDotLine:
                    oldLineStyleData.insert(direction, QLatin1String("dash dot line"));
                    break;
                case Qt::DashDotDotLine:
                    oldLineStyleData.insert(direction, QLatin1String("dash dot dot line"));
                    break;
                case Qt::SolidLine:
                    [[fallthrough]];
                default:
                    oldLineStyleData.insert(direction, QLatin1String("solid line"));
                }
            }
            ofs << oldLineStyleData;
        }
        if (mSaveVersion >= 21) {
            ofs << pR->getSpecialExitLocks();
        }
        ofs << pR->exitLocks;
        ofs << pR->exitStubs;
        ofs << pR->getExitWeights();
        ofs << pR->doors;
    }

    // reset to the old map version
    mSaveVersion = oldSaveVersion;
    return true;
}

// file is expected to be linked to a file name but not be opened; ifs is not
// expected to be linked to any IODevice. On success file will be opened and
// ifs will be part way through it (has read the first 4 bytes which encode the
// map file version). On failure both will be in the same states as initial one:
bool TMap::validatePotentialMapFile(QFile& file, QDataStream& ifs)
{
    int version = 0;
    if (!file.open(QFile::ReadOnly)) {
        const QString errMsg = tr(R"([ ERROR ] - Unable to open map file for reading: "%1"!)").arg(file.fileName());
        appendErrorMsg(errMsg, false);
        postMessage(errMsg);
        return false;
    }

    ifs.setDevice(&file);
    // Is the RUN-TIME version of the Qt libraries equal to or more than
    // Qt 5.13.0? Then force things to use the backwards compatible format
    // - for us - of Qt 5.12.0 - this is needed because the way that the
    // QFont class is stored in a binary format has changed at 5.13 and it
    // causes crashes when a new version of the Qt libraries tries to read
    // the older format:
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        // 18 is the enum value corresponding to QDataStream::Qt_5_12 which
        // we want to force to be used but we cannot use the enum directly
        // because it will not be defined in older versions of the Qt
        // library when the code is compilated:
        ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ifs >> version;
    if ((version < 1) || (version > 127)) {
        const QString errMsg = tr("[ ALERT ] - File does not seem to be a Mudlet Map file. The part that indicates\n"
                            "its format version seems to be \"%1\" and that doesn't make sense. The file is:\n"
                            "\"%2\".")
                                 .arg(version)
                                 .arg(file.fileName());
        appendErrorMsgWithNoLf(errMsg);
        postMessage(errMsg);
        const QString infoMsg = tr("[ INFO ]  - Ignoring this unlikely map file.");
        appendErrorMsgWithNoLf(infoMsg);
        postMessage(infoMsg);
        ifs.setDevice(nullptr);
        file.close();
        return false;
    }
    if (version > mMaxVersion) {
        const QString errMsg = tr("[ ALERT ] - Map file is too new. Its format version \"%1\" is higher than this version of\n"
                            "Mudlet can handle (%2)! The file is:\n\"%3\".")
                                 .arg(version)
                                 .arg(mMaxVersion)
                                 .arg(file.fileName());
        appendErrorMsgWithNoLf(errMsg);
        postMessage(errMsg);
        const QString infoMsg = tr("[ INFO ]  - You will need to update your Mudlet to read the map file.");
        appendErrorMsgWithNoLf(infoMsg);
        postMessage(infoMsg);
        ifs.setDevice(nullptr);
        file.close();
        return false;
    }

    if (version < 4) {
        const QString alertMsg = tr("[ ALERT ] - Map file is really old. Its format version \"%1\" is so ancient that\n"
                              "this version of Mudlet may not gain enough information from\n"
                              "it but it will try! The file is: \"%2\".")
                                   .arg(version)
                                   .arg(file.fileName());
        appendErrorMsgWithNoLf(alertMsg, false);
        postMessage(alertMsg);
        const QString infoMsg = tr("[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!\n"
                             "There is so much data that it DOES NOT have that you could be\n"
                             "better off starting again...");
        appendErrorMsgWithNoLf(infoMsg, false);
        postMessage(infoMsg);
    } else {
        // Less than (but not less than 4) or equal to default version
        const QString infoMsg = tr("[ INFO ]  - Reading map. Format version: %1. File:\n"
                             "\"%2\",\n"
                             "please wait...").arg(version).arg(file.fileName());
        appendErrorMsg(tr(R"([ INFO ]  - Reading map. Format version: %1. File: "%2".)").arg(version).arg(file.fileName()), false);
        postMessage(infoMsg);
    }
    mVersion = version;
    mSaveVersion = mDefaultVersion; // Make the save version the default one - unless the user intervenes

    return true;
}

bool TMap::restore(QString location, bool downloadIfNotFound)
{
    qDebug().noquote().nospace() << "TMap::restore(\"" << location << "\") INFO: restoring map of Profile: \"" << mProfileName << "\" URL: " << mpHost->getUrl();

    QElapsedTimer _time;
    _time.start();
    QString folder;
    QStringList entries;

    if (location.isEmpty()) {
        folder = mudlet::getMudletPath(enums::profileMapsPath, mProfileName);
        const QDir dir(folder);
        QStringList filters;
        filters << qsl("*.[dD][aA][tT]");
        filters << qsl("*.[jJ][sS][oO][nN]");
        entries = dir.entryList(filters, QDir::Files, QDir::Time);
    }

    bool canRestore = true;
    if (entries.empty() && location.isEmpty()) {
        canRestore = false;
    }

    QDataStream ifs;
    QFile file;
    if (canRestore && (!entries.empty() || !location.isEmpty())) {
        // We get to here if there is one or more entries OR location is
        // supplied - if the latter then there is only one file to consider but
        // if the former we may have to check more than one to find a valid
        // map file:
        bool foundValidFile = false;
        if (location.isEmpty()) {
            // Look through the entries:
            QStringListIterator itFileName(entries);
            auto fileName = qsl("%1/%2").arg(folder, itFileName.next());
            if (!fileName.endsWith(qsl(".json"), Qt::CaseInsensitive)) {
                file.setFileName(fileName);
                if (validatePotentialMapFile(file, ifs)) {
                    foundValidFile = true;
                }

            } else {
                if (auto [isOk, message] = readJsonMapFile(fileName, true); !isOk) {
                   // Failed to read the JSON file
                   const QString errMsg = tr("[ ALERT ] - Failed to load a Mudlet JSON Map file, reason:\n"
                                       "%1; the file is:\n"
                                       "\"%2\".").arg(message, fileName);
                   appendErrorMsgWithNoLf(errMsg);
                   postMessage(errMsg);
                   const QString infoMsg = tr("[ INFO ]  - Ignoring this map file.");
                   appendErrorMsgWithNoLf(infoMsg);
                   postMessage(infoMsg);
               } else {
                   // immediately leave on success:
                   return true;
               }
           }

           // Allow for somethings to be updated - especially on Windows?
           qApp->processEvents();
        } else {
            file.setFileName(location);
            if (validatePotentialMapFile(file, ifs)) {
                foundValidFile = true;
            }
        }
        if (!foundValidFile) {
            canRestore = false;
        }
    } else if (canRestore && !location.isEmpty()) {
        file.setFileName(location);
        canRestore = validatePotentialMapFile(file, ifs);
    }

    if (canRestore) {
        // As all but the room reading have version checks the fact that sub-4
        // files will still be parsed despite canRestore being false is probably OK
        if (mVersion >= 4) {
            ifs >> mEnvColors;
            mpRoomDB->restoreAreaMap(ifs);
        }
        if (mVersion >= 5) {
            ifs >> mCustomEnvColors;
        }
        if (mVersion >= 7) {
            ifs >> mpRoomDB->hashToRoomID;
            QMap<QString, int>::const_iterator i;
            for (i = mpRoomDB->hashToRoomID.constBegin(); i != mpRoomDB->hashToRoomID.constEnd(); ++i) {
                mpRoomDB->roomIDToHash.insert(i.value(), i.key());
            }
        }

        if (mVersion >= 17) {
            ifs >> mUserData;
            if (mVersion >= 19) {
                // Read the data from the file directly in version 19 or later
                ifs >> mMapSymbolFont;
                if ((mVersion < 21) && mMapSymbolFont.toString().split(QLatin1String(",")).size() > 15) {
                    // We need to clean up the effects of using QFont(string)
                    // for a format 17 or 18 below - as this fix went in before
                    // 21 was used it only has to be used for map formats 19 and
                    // 20:
                    mMapSymbolFont.fromString(mMapSymbolFont.toString().split(QLatin1String(",")).mid(0, 10).join(QLatin1String(",")));
                }
                ifs >> mMapSymbolFontFudgeFactor;
                ifs >> mIsOnlyMapSymbolFontToBeUsed;
            } else {
                // Fallback to reading the data from the map user data - and
                // remove it from the data the user will see:
                // BUGFIX: Using QFont::toString() and then using that to
                // construct a font again afterwards via a QFont(string) was
                // incorrect as it seemed to cause the last to duplicated the
                // last nine elements each time. The details of the ::toString()
                // ::fromString() methods are not currently documented so the
                // only details are documented in the source:
                // https://code.qt.io/cgit/qt/qtbase.git/tree/src/gui/text/qfont.cpp?h=5.15#n2070
                // and:
                // https://code.qt.io/cgit/qt/qtbase.git/tree/src/gui/text/qfont.cpp?h=5.15#n2128
                // this suggests that only one or ten elements are accepted so
                // we CAN fix past mistakes by only considering the first ten
                // elements:
                const QStringList fontStrings{mUserData.take(qsl("system.fallback_mapSymbolFont")).split(QLatin1Char(','))};
                const QString fontString{fontStrings.mid(0, 10).join(QLatin1Char(','))};
                const QString fontFudgeFactorString = mUserData.take(qsl("system.fallback_mapSymbolFontFudgeFactor"));
                const QString onlyUseSymbolFontString = mUserData.take(qsl("system.fallback_onlyUseMapSymbolFont"));
                if (!fontString.isEmpty()) {
                    mMapSymbolFont.fromString(fontString);
                }
                if (!fontFudgeFactorString.isEmpty()) {
                    mMapSymbolFontFudgeFactor = fontFudgeFactorString.toDouble();
                }
                if (!onlyUseSymbolFontString.isEmpty()) {
                    mIsOnlyMapSymbolFontToBeUsed = (onlyUseSymbolFontString != QLatin1String("false"));
                }
            }
        }

        mMapSymbolFont.setStyleStrategy(static_cast<QFont::StyleStrategy>((mIsOnlyMapSymbolFontToBeUsed ? QFont::NoFontMerging : 0)
                                                                          |QFont::PreferOutline | QFont::PreferAntialias | QFont::PreferQuality
                                                                          |QFont::PreferNoShaping
                                                                          ));
        if (mVersion >= 14) {
            int areaSize = 0;
            ifs >> areaSize;
            // restore area table
            for (int i = 0; i < areaSize; i++) {
                auto pA = new TArea(this, mpRoomDB);
                int areaID = 0;
                ifs >> areaID;
                if (mVersion >= 18) {
                    // In version 18 changed from QList<int> to QSet<int> as the later is
                    // faster in many of the cases where we use it.
                    ifs >> pA->rooms;
                } else {
                    QList<int> oldRoomsList;
                    ifs >> oldRoomsList;
                    pA->rooms = QSet<int>{oldRoomsList.begin(), oldRoomsList.end()};
                }
                // Can be useful when analysing suspect map files!
                //                qDebug() << "TMap::restore(...)" << "Area:" << areaID;
                //                qDebug() << "Rooms:" << pA->rooms;
                ifs >> pA->zLevels;
                ifs >> pA->mAreaExits;
                ifs >> pA->gridMode;
                ifs >> pA->max_x;
                ifs >> pA->max_y;
                ifs >> pA->max_z;
                ifs >> pA->min_x;
                ifs >> pA->min_y;
                ifs >> pA->min_z;
                ifs >> pA->span;
                if (mVersion >= 17) {
                    ifs >> pA->xmaxForZ;
                    ifs >> pA->ymaxForZ;
                    ifs >> pA->xminForZ;
                    ifs >> pA->yminForZ;
                } else {
                    QMap<int, int> dummyMinMaxForZ;
                    ifs >> pA->xmaxForZ;
                    ifs >> pA->ymaxForZ;
                    ifs >> dummyMinMaxForZ;
                    ifs >> pA->xminForZ;
                    ifs >> pA->yminForZ;
                    ifs >> dummyMinMaxForZ;
                }
                ifs >> pA->pos;
                ifs >> pA->isZone;
                ifs >> pA->zoneAreaRef;
                if (mVersion >= 21) {
                    ifs >> pA->mLast2DMapZoom;
                    ifs >> pA->mUserData;
                } else if (mVersion >= 17) {
                    ifs >> pA->mUserData;
                    const qreal fallback_map2DZoom = pA->mUserData.take(QLatin1String("system.fallback_map2DZoom")).toDouble();
                    pA->mLast2DMapZoom = (fallback_map2DZoom >= T2DMap::csmMinXYZoom) ? fallback_map2DZoom : T2DMap::csmDefaultXYZoom;
                }
                if (mVersion >= 21) {
                    int mapLabelsCount = -1;
                    ifs >> mapLabelsCount;
                    for (int i = 0; i < mapLabelsCount; ++i) {
                        int labelId = -1;
                        ifs >> labelId;
                        TMapLabel label;
                        ifs >> label.size;
                        ifs >> label.text;
                        ifs >> label.fgColor;
                        ifs >> label.bgColor;
                        ifs >> label.pix;
                        ifs >> label.noScaling;
                        ifs >> label.showOnTop;
                        pA->mMapLabels.insert(labelId, label);
                    }
                }
                mpRoomDB->restoreSingleArea(areaID, pA);
            }
        }

        if (!mpRoomDB->getAreaMap().keys().contains(-1)) {
            auto pDefaultA = new TArea(this, mpRoomDB);
            mpRoomDB->restoreSingleArea(-1, pDefaultA);
            const QString defaultAreaInsertionMsg = tr("[ INFO ]  - Default (reset) area (for rooms that have not been assigned to an\n"
                                                 "area) not found, adding reserved -1 id.");
            appendErrorMsgWithNoLf(defaultAreaInsertionMsg, false);
            if (mudlet::self()->showMapAuditErrors()) {
                postMessage(defaultAreaInsertionMsg);
            }
        }

        if (mVersion >= 18) {
            // In version 18 we changed to store the "userRoom" for each profile
            // so that when copied/shared between profiles they do not interfere
            // with each other's saved value
            ifs >> mRoomIdHash;
        } else if (mVersion >= 12) {
            int oldRoomId = 0;
            ifs >> oldRoomId;
            mRoomIdHash[mProfileName] = oldRoomId;
        }

        if (mVersion >= 11 && mVersion <= 20) {
            // After version 20 the map labels have been moved to each area
            int areasWithLabelsTotal = 0;
            ifs >> areasWithLabelsTotal;
            int areasWithLabelsCounter = 0;
            while (!ifs.atEnd() && areasWithLabelsCounter < areasWithLabelsTotal) {
                int areaID = -1;
                int areaLabelsTotal = 0;
                ifs >> areaLabelsTotal;
                // Only used to identify the area for this batch of labels:
                ifs >> areaID;
                int areaLabelCounter = 0;
                auto pA = mpRoomDB->getArea(areaID);
                while (!ifs.atEnd() && areaLabelCounter < areaLabelsTotal) {
                    int labelID = 0;
                    ifs >> labelID;
                    TMapLabel label;
                    if (mVersion >= 12) {
                        // From version 12 labels could be placed on any level,
                        // so they have a z coordinate:
                        ifs >> label.pos;
                    } else {
                        QPointF labelPos2D;
                        ifs >> labelPos2D;
                        label.pos = QVector3D(labelPos2D);
                    }
                    // There was an unused QPointF in versions prior to 21
                    QPointF dummyPointF;
                    ifs >> dummyPointF;
                    ifs >> label.size;
                    ifs >> label.text;
                    ifs >> label.fgColor;
                    ifs >> label.bgColor;
                    ifs >> label.pix;
                    if (mVersion >= 15) {
                        ifs >> label.noScaling;
                        ifs >> label.showOnTop;
                    }
                    if (pA) {
                        pA->mMapLabels.insert(labelID, label);
                    }
                    ++areaLabelCounter;
                    // Else: we dump labels for areas not in map - this should
                    // not be happening nowadays but did in the past - see
                    // PR #4369
                }
                ++areasWithLabelsCounter;
            }
        }

        while (!ifs.atEnd()) {
            int i = 0;
            ifs >> i;
            auto pT = new TRoom(mpRoomDB);
            pT->restore(ifs, i, mVersion);
            mpRoomDB->restoreSingleRoom(i, pT);
        }

        restore16ColorSet();

        const QString okMsg = tr("[ INFO ]  - Successfully read the map file (%1s), checking some\n"
                           "consistency details..." )
                                .arg(_time.nsecsElapsed() * 1.0e-9, 0, 'f', 2);

        postMessage(okMsg);
        appendErrorMsgWithNoLf(okMsg);
        if (canRestore) {
            return true;
        }
    }

    if ((!canRestore || entries.empty()) && downloadIfNotFound) {
        QMessageBox msgBox;

        if (!getMmpMapLocation().isEmpty()) {
            msgBox.setText(tr("No map found. Would you like to download the map or start your own?"));
            QPushButton* yesButton = msgBox.addButton(tr("Download the map"), QMessageBox::ActionRole);
            QPushButton* noButton = msgBox.addButton(tr("Start my own"), QMessageBox::ActionRole);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton) {
                downloadMap();
            } else if (msgBox.clickedButton() == noButton) {
                ; //No-op to avoid unused "noButton"
            }
        }
    }

    return canRestore; //FIXME
}

// Reads the newest map file from the profile and retrieves some stats and data,
// including the current player room - was mRoomId in 12 to pre-18 map files and
// is in mRoomIdHash since then so that it can be reinserted into a map that is
// copied across (if the room STILL exists)!  This is to avoid a replacement map
// (copied/shared) from one profile to another from repositioning the other
// player location. Though this is written as a member function it is intended
// also for use to retrieve details from maps from OTHER profiles, importantly
// it does (or should) NOT interact with this TMap instance...!
bool TMap::retrieveMapFileStats(QString profile, QString* latestFileName = nullptr, int* fileVersion = nullptr, int* roomId = nullptr, qsizetype* areaCount = nullptr, qsizetype* roomCount = nullptr)
{
    if (profile.isEmpty()) {
        return false;
    }

    QString folder;
    QStringList entries;
    folder = mudlet::getMudletPath(enums::profileMapsPath, profile);
    QDir dir(folder);
    dir.setSorting(QDir::Time);
    entries = dir.entryList(QDir::Filters(QDir::Files | QDir::NoDotAndDotDot), QDir::Time);

    if (entries.isEmpty()) {
        return false;
    }

    // As the files are sorted by time this gets the latest one
    QFile file(qsl("%1/%2").arg(folder, entries.at(0)));

    if (!file.open(QFile::ReadOnly)) {
        const QString errMsg = tr(R"([ ERROR ] - Unable to open map file for reading: "%1"!)").arg(file.fileName());
        appendErrorMsg(errMsg, false);
        postMessage(errMsg);
        return false;
    }

    if (latestFileName) {
        *latestFileName = file.fileName();
    }
    int otherProfileVersion = 0;
    QDataStream ifs(&file);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }
    ifs >> otherProfileVersion;

    const QString infoMsg = tr(R"([ INFO ]  - Checking map file "%1", format version "%2".)").arg(file.fileName()).arg(otherProfileVersion);
    appendErrorMsg(infoMsg, false);
    if (mudlet::self()->showMapAuditErrors()) {
        postMessage(infoMsg);
    }

    if (otherProfileVersion > mDefaultVersion) {
        if (mudlet::self()->releaseVersion || mudlet::self()->publicTestVersion) {
            // This is a release/public test version - should not support any map file versions higher that it was built for
            if (fileVersion) {
                *fileVersion = otherProfileVersion;
            }
            file.close();
            return true;
        } else {
            // Is a development version so check against mMaxVersion
            if (otherProfileVersion > mMaxVersion) {
                // Oh dear, can't handle THIS
                if (fileVersion) {
                    *fileVersion = otherProfileVersion;
                }
                file.close();
                return true;
            } else {
                if (fileVersion) {
                    *fileVersion = otherProfileVersion;
                }
            }
        }
    } else {
        if (fileVersion) {
            *fileVersion = otherProfileVersion;
        }
    }

    if (otherProfileVersion >= 4) {
        // envColorMap
        QMap<int, int> _dummyQMapIntInt;
        ifs >> _dummyQMapIntInt;

        // AreaNamesMap
        QMap<int, QString> _dummyQMapIntQString;
        ifs >> _dummyQMapIntQString;
    }

    if (otherProfileVersion >= 5) {
        // mCustomEnvColors
        QMap<int, QColor> _dummyQMapIntQColor;
        ifs >> _dummyQMapIntQColor;
    }

    if (otherProfileVersion >= 7) {
        // hashToRoomID
        QMap<QString, int> _dummyQMapQStringInt;
        ifs >> _dummyQMapQStringInt;
    }

    if (otherProfileVersion >= 17) {
        // userMapData
        QMap<QString, QString> _dummyQMapQStringQString;
        ifs >> _dummyQMapQStringQString;
    }

    if (otherProfileVersion >= 14) {
        int readAreaSize;
        ifs >> readAreaSize;
        qsizetype areaSize = static_cast<qsizetype>(readAreaSize);
        if (areaCount) {
            *areaCount = areaSize;
        }
        // read each area
        for (qsizetype i = 0; i < areaSize; ++i) {
            TArea pA(nullptr, nullptr);
            int areaID;
            ifs >> areaID;
            ifs >> pA.rooms;
            ifs >> pA.zLevels;
            ifs >> pA.mAreaExits;
            ifs >> pA.gridMode;
            ifs >> pA.max_x;
            ifs >> pA.max_y;
            ifs >> pA.max_z;
            ifs >> pA.min_x;
            ifs >> pA.min_y;
            ifs >> pA.min_z;
            ifs >> pA.span;
            if (otherProfileVersion >= 17) {
                ifs >> pA.xmaxForZ;
                ifs >> pA.ymaxForZ;
                ifs >> pA.xminForZ;
                ifs >> pA.yminForZ;
            } else {
                QMap<int, int> dummyMinMaxForZ;
                ifs >> pA.xmaxForZ;
                ifs >> pA.ymaxForZ;
                ifs >> dummyMinMaxForZ;
                ifs >> pA.xminForZ;
                ifs >> pA.yminForZ;
                ifs >> dummyMinMaxForZ;
            }
            ifs >> pA.pos;
            ifs >> pA.isZone;
            ifs >> pA.zoneAreaRef;
            if (otherProfileVersion >= 21) {
                ifs >> pA.mLast2DMapZoom;
            }
            if (otherProfileVersion >= 17) {
                ifs >> pA.mUserData;
            }
            if (otherProfileVersion >= 21) {
                int mapLabelsCount = -1;
                ifs >> mapLabelsCount;
                for (int i = 0; i < mapLabelsCount; ++i) {
                    int labelId = -1;
                    ifs >> labelId;
                    TMapLabel label;
                    ifs >> label.pos;
                    ifs >> label.size;
                    ifs >> label.text;
                    ifs >> label.fgColor;
                    ifs >> label.bgColor;
                    ifs >> label.pix;
                    ifs >> label.noScaling;
                    ifs >> label.showOnTop;
                }
            }
        }
    }

    if (otherProfileVersion >= 18) {
        // In version 18 we changed to store the "userRoom" for each profile
        // so that when copied/shared between profiles they do not interfere
        // with each other's saved value
        QHash<QString, int> _dummyQHashQStringInt;
        ifs >> _dummyQHashQStringInt;
        if (roomId) {
            *roomId = _dummyQHashQStringInt.value(profile);
        }
    } else if (otherProfileVersion >= 12) {
        int oldRoomId;
        ifs >> oldRoomId;
        if (roomId) {
            *roomId = oldRoomId;
        }
    } else {
        if (roomId) {
            *roomId = -1; // Not found value
        }
    }

    if (otherProfileVersion >= 11 && otherProfileVersion <= 20) {
        int areasWithLabelsTotal = 0;
        ifs >> areasWithLabelsTotal;
        int areasWithLabelsCounter = 0;
        while (!ifs.atEnd() && areasWithLabelsCounter < areasWithLabelsTotal) {
            int areaID = -1;
            int areaLabelsTotal = 0;
            ifs >> areaLabelsTotal;
            ifs >> areaID;
            int areaLabelCounter = 0;
            while (!ifs.atEnd() && areaLabelCounter < areaLabelsTotal) {
                int labelID;
                ifs >> labelID;
                TMapLabel label;
                if (otherProfileVersion >= 12) {
                    ifs >> label.pos;
                } else {
                    QPointF oldLabelPos;
                    ifs >> oldLabelPos;
                    label.pos = QVector3D(oldLabelPos);
                }
                QPointF dummyPointF;
                ifs >> dummyPointF;
                ifs >> label.size;
                ifs >> label.text;
                ifs >> label.fgColor;
                ifs >> label.bgColor;
                ifs >> label.pix;
                if (otherProfileVersion >= 15) {
                    ifs >> label.noScaling;
                    ifs >> label.showOnTop;
                }
                ++areaLabelCounter;
            }
            ++areasWithLabelsCounter;
        }
    }

    TRoom _pT(nullptr);
    QSet<int> _dummyRoomIdSet;
    while (!ifs.atEnd()) {
        int i;
        ifs >> i;
        _pT.restore(ifs, i, otherProfileVersion);
        // Can't do mpRoomDB->restoreSingleRoom( ifs, i, pT ) as it would mess up
        // this TMap::mpRoomDB
        // So emulate using _dummyRoomIdSet
        if (i > 0 && !_dummyRoomIdSet.contains(i)) {
            _dummyRoomIdSet.insert(i);
        }
    }
    if (roomCount) {
        *roomCount = _dummyRoomIdSet.count();
    }

    return true;
}

//NOLINT(readability-make-member-function-const)
int TMap::createMapLabel(int area, const QString& text, float x, float y, float z, QColor fg, QColor bg, bool showOnTop, bool noScaling, bool temporary, qreal zoom, int fontSize, std::optional<QString> fontName, QColor outline)
{
    auto pA = mpRoomDB->getArea(area);
    if (!pA) {
        return -1;
    }

    if (text.isEmpty()) {
        return -1;
    }

    TMapLabel label;
    label.text = text;
    label.bgColor = bg;
    label.fgColor = fg;
    label.outlineColor = outline;
    label.size = QSizeF(100, 100);
    label.pos = QVector3D(x, y, z);
    label.showOnTop = showOnTop;
    label.noScaling = noScaling;
    label.temporary = temporary;

    const QRectF lr = QRectF(0, 0, 2000, 2000);
    QPixmap pix(lr.size().toSize());
    pix.fill(Qt::transparent);

    QPainter lp(&pix);
    lp.fillRect(lr, label.bgColor);
    lp.setRenderHint(QPainter::Antialiasing);

    QFont font(fontName.has_value() ? fontName.value() : QString(), fontSize);
    lp.setFont(font);

    QPen outlinePen(label.outlineColor);
    outlinePen.setWidth(1);
    lp.setPen(outlinePen);

    QRectF br;

    if (label.fgColor != label.outlineColor) {
        lp.drawText(QRect(19, 70, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, label.text, &br);
        lp.drawText(QRect(21, 70, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, label.text, &br);
        lp.drawText(QRect(20, 69, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, label.text, &br);
        lp.drawText(QRect(20, 71, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, label.text, &br);
    }
    lp.setPen(label.fgColor);
    lp.drawText(QRect(20, 70, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, label.text, &br);

    label.size = br.normalized().size();
    label.pix = pix.copy(br.normalized().topLeft().x(), br.normalized().topLeft().y(), br.normalized().width(), br.normalized().height());
    const QSizeF s = QSizeF(label.size.width() / zoom, label.size.height() / zoom);
    label.size = s;
    label.clickSize = s;

    const int labelId = pA->createLabelId();
    if (Q_LIKELY(labelId >= 0)) {
        pA->mMapLabels.insert(labelId, label);
        if (mpMapper) {
            mpMapper->mp2dMap->update();
        }
    }

    if (!temporary) {
        setUnsaved(__func__);
    }
    return labelId;
}

int TMap::createMapImageLabel(int area, QString imagePath, float x, float y, float z, float width, float height, float zoom, bool showOnTop, bool temporary)
{
    auto pA = mpRoomDB->getArea(area);
    if (!pA) {
        return -1;
    }

    TMapLabel label;
    label.size = QSizeF(width, height);
    label.pos = QVector3D(x, y, z);
    label.showOnTop = showOnTop;
    // This method is only called from the TLuaInterpreter class and the value
    // passed was hard-coded to this value:
    label.noScaling = false;
    label.temporary = temporary;

    const QRectF drawRect = QRectF(0, 0, static_cast<qreal>(width * zoom), static_cast<qreal>(height * zoom));
    const QPixmap imagePixmap = QPixmap(imagePath);
    QPixmap pix = QPixmap(drawRect.size().toSize());
    pix.fill(Qt::transparent);
    QPainter lp(&pix);
    lp.drawPixmap(QPoint(0, 0), imagePixmap.scaled(drawRect.size().toSize()));
    label.size = QSizeF(width, height);
    label.pix = pix;

    const int labelId = pA->createLabelId();
    if (Q_LIKELY(labelId >=0)) {
        pA->mMapLabels.insert(labelId, label);
        if (mpMapper) {
            mpMapper->mp2dMap->update();
        }
    }

    if (!temporary) {
        setUnsaved(__func__);
    }
    return labelId;
}

void TMap::deleteMapLabel(int area, int labelId)
{
    auto pA = mpRoomDB->getArea(area);
    if (!pA) {
        return;
    }

    auto label = pA->mMapLabels.take(labelId);
    if (!label.pos.isNull() || !label.text.isEmpty() || label.bgColor != QColorConstants::Black || label.fgColor != QColorConstants::Black) {
        // If any of the above tests are false then we can take it that we do
        // not have a "default constructed" label - i.e. a real one and not
        // one that the QMap<T1, T2>::take(const T1&) has created for us in the
        // absence of an actual TMapLabel.
        // The TMapLabel default constructor sets the 'temporary' class member
        // to false so we can safely rely on it being true for a temporary one:
        if (!label.temporary) {
            setUnsaved(__func__);
        }
        if (mpMapper) {
            mpMapper->mp2dMap->update();
        }
    }
}

void TMap::postMessage(const QString text)
{
    mStoredMessages.append(text);
    Host* pHost = mpHost;
    if (pHost) {
        while (!mStoredMessages.isEmpty()) {
            pHost->postMessage(mStoredMessages.takeFirst());
        }
    }
}

// Used by the 2D mapper to send view center coordinates to 3D one
void TMap::set3DViewCenter(const int areaId, const int xPos, const int yPos, const int zPos)
{
#if defined(INCLUDE_3DMAPPER)
    if (mpM) {
        if (auto* glWidget = dynamic_cast<GLWidget*>(mpM.data())) {
            glWidget->setViewCenter(areaId, xPos, yPos, zPos);
        } else if (auto* modernWidget = dynamic_cast<ModernGLWidget*>(mpM.data())) {
            modernWidget->setViewCenter(areaId, xPos, yPos, zPos);
        }
    }
#else
    Q_UNUSED(areaId)
    Q_UNUSED(xPos)
    Q_UNUSED(yPos)
    Q_UNUSED(zPos)
#endif
}

void TMap::appendRoomErrorMsg(const int roomId, const QString msg, const bool isToSetFileViewingRecommended)
{
    mMapAuditRoomErrors[roomId].append(msg);
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

void TMap::appendAreaErrorMsg(const int areaId, const QString msg, const bool isToSetFileViewingRecommended)
{
    mMapAuditAreaErrors[areaId].append(msg);
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

void TMap::appendErrorMsg(const QString msg, const bool isToSetFileViewingRecommended)
{
    mMapAuditErrors.append(msg);
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

void TMap::appendErrorMsgWithNoLf(const QString msg, const bool isToSetFileViewingRecommended)
{
    QString text = msg;
    text.replace(QChar::LineFeed, QChar::Space);
    mMapAuditErrors.append(text);
    mIsFileViewingRecommended = isToSetFileViewingRecommended ? true : mIsFileViewingRecommended;
}

const QString TMap::createFileHeaderLine(const QString title, const QChar fillChar)
{
    QString text;
    if (title.length() <= 76) {
        text = qsl("%1 %2 %1\n").arg(QString(fillChar).repeated((78 - title.length()) / 2), title);
    } else {
        text = title;
        text.append(QChar::LineFeed);
    }
    return text;
}

void TMap::pushErrorMessagesToFile(const QString title, const bool isACleanup)
{
    Host* pHost = mpHost;
    if (!pHost) {
        qWarning() << "TMap::pushErrorMessagesToFile( ... ) ERROR: called with a NULL HOST pointer - something is wrong!";
        return;
    }

    // Replacement storage locations:
    QMap<int, QList<QString>> mapAuditRoomErrors; // Key is room number (where renumbered is the original one), Value is the errors, appended as they are found
    QMap<int, QList<QString>> mapAuditAreaErrors; // As for the Room ones but with key as the area number
    QList<QString> mapAuditErrors;                // For the whole map
    // Switch message storage locations to freeze them so we can dump them to
    // file; according to Qt documentation "Swaps XXX other with this XXX. This
    // operation is very fast and never fails."
    mapAuditErrors.swap(mMapAuditErrors);
    mapAuditAreaErrors.swap(mMapAuditAreaErrors);
    mapAuditRoomErrors.swap(mMapAuditRoomErrors);

    if (mapAuditErrors.isEmpty() && mapAuditAreaErrors.isEmpty() && mapAuditRoomErrors.isEmpty() && isACleanup) {
        mIsFileViewingRecommended = false;
        return; // Nothing to do
    }

    pHost->mErrorLogStream << createFileHeaderLine(title, QLatin1Char('#'));
    pHost->mErrorLogStream << createFileHeaderLine(tr("Map issues"), QLatin1Char('='));
    QListIterator<QString> itMapMsg(mapAuditErrors);
    while (itMapMsg.hasNext()) {
        pHost->mErrorLogStream << itMapMsg.next() << QLatin1Char('\n');
        ;
    }

    pHost->mErrorLogStream << createFileHeaderLine(tr("Area issues"), QLatin1Char('='));
    QMapIterator<int, QList<QString>> itAreasMsg(mapAuditAreaErrors);
    while (itAreasMsg.hasNext()) {
        itAreasMsg.next();
        QString titleText;
        if (!mpRoomDB->getAreaNamesMap().value(itAreasMsg.key()).isEmpty()) {
            titleText = tr(R"(Area id: %1 "%2")").arg(itAreasMsg.key()).arg(mpRoomDB->getAreaNamesMap().value(itAreasMsg.key()));
        } else {
            titleText = tr("Area id: %1").arg(itAreasMsg.key());
        }
        pHost->mErrorLogStream << createFileHeaderLine(titleText, QLatin1Char('-'));
        QListIterator<QString> itMapAreaMsg(itAreasMsg.value());
        while (itMapAreaMsg.hasNext()) {
            pHost->mErrorLogStream << itMapAreaMsg.next() << QLatin1Char('\n');
        }
    }

    pHost->mErrorLogStream << createFileHeaderLine(tr("Room issues"), QLatin1Char('='));
    QMapIterator<int, QList<QString>> itRoomsMsg(mapAuditRoomErrors);
    while (itRoomsMsg.hasNext()) {
        itRoomsMsg.next();
        QString titleText;
        TRoom* pR = mpRoomDB->getRoom(itRoomsMsg.key());
        if (pR && !pR->name.isEmpty()) {
            titleText = tr(R"(Room id: %1 "%2")").arg(itRoomsMsg.key()).arg(pR->name);
        } else {
            titleText = tr("Room id: %1").arg(itRoomsMsg.key());
        }
        pHost->mErrorLogStream << createFileHeaderLine(titleText, QLatin1Char('-'));
        QListIterator<QString> itMapRoomMsg(itRoomsMsg.value());
        while (itMapRoomMsg.hasNext()) {
            pHost->mErrorLogStream << itMapRoomMsg.next() << QLatin1Char('\n');
            ;
        }
    }

    pHost->mErrorLogStream << createFileHeaderLine(tr("End of report"), QLatin1Char('#'));
    pHost->mErrorLogStream.flush();
    mapAuditErrors.clear();
    mapAuditAreaErrors.clear();
    mapAuditRoomErrors.clear();
    if (mIsFileViewingRecommended && (!mudlet::self()->showMapAuditErrors())) {
        postMessage(tr("[ ALERT ] - At least one thing was detected during that last map operation\n"
                       "that it is recommended that you review the most recent report in\n"
                       "the file:\n"
                       "\"%1\"\n"
                       "- look for the (last) report with the title:\n"
                       "\"%2\".")
                    .arg(mudlet::getMudletPath(enums::profileLogErrorsFilePath, mProfileName), title));
    } else if (mIsFileViewingRecommended && mudlet::self()->showMapAuditErrors()) {
        postMessage(tr("[ INFO ]  - The equivalent to the above information about that last map\n"
                       "operation has been saved for review as the most recent report in\n"
                       "the file:\n"
                       "\"%1\"\n"
                       "- look for the (last) report with the title:\n"
                       "\"%2\".")
                    .arg(mudlet::getMudletPath(enums::profileLogErrorsFilePath, mProfileName), title));
    }

    mIsFileViewingRecommended = false;
}

void TMap::downloadMap(const QString& remoteUrl, const QString& localFileName)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    // Incidentally this should address: https://bugs.launchpad.net/mudlet/+bug/852861
    if (mImportRunning) {
        const QString warnMsg = tr("[ WARN ]  - Attempt made to download an XML map when one has already been\n"
                                         "requested or is being imported from a local file - wait for that\n"
                                         "operation to complete (if it cannot be canceled) before retrying!");
        postMessage(warnMsg);
        return;
    }
    mImportRunning = true;
    // MUST clear this flag when done under ALL circumstances

    QUrl url;
    if (remoteUrl.isEmpty()) {
        if (!getMmpMapLocation().isEmpty()) {
            url = QUrl::fromUserInput(getMmpMapLocation());
        } else {
            url = QUrl::fromUserInput(qsl("https://www.%1/maps/map.xml").arg(pHost->mUrl));
        }
    } else {
        url = QUrl::fromUserInput(remoteUrl);
    }

    if (!url.isValid()) {
        const QString errMsg = tr("[ WARN ]  - Attempt made to download an XML from an invalid URL.  The URL was:\n"
                                        "%1\n"
                                        "and the error message (may contain technical details) was:"
                                        "\"%2\".")
                                 .arg(url.toString(), url.errorString());
        postMessage(errMsg);
        mImportRunning = false;
        return;
    }

    // Check to ensure we have a map directory to save the map files to.
    const QDir toProfileDir;
    const QString toProfileDirPathString = mudlet::getMudletPath(enums::profileMapsPath, mProfileName);
    if (!toProfileDir.mkpath(toProfileDirPathString)) {
        const QString errMsg = tr("[ ERROR ] - Unable to use or create directory to store map.\n"
                            "Please check that you have permissions/access to:\n"
                            "\"%1\"\n"
                            "and there is enough space. The download operation has failed.")
                                    .arg(toProfileDirPathString);
        pHost->postMessage(errMsg);
        mImportRunning = false;
        return;
    }

    if (localFileName.isEmpty()) {
        if (url.toString().endsWith(QLatin1String("xml"))) {
            mLocalMapFileName = mudlet::getMudletPath(enums::profileXmlMapPathFileName, mProfileName);
        } else {
            mLocalMapFileName = mudlet::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("map.dat"));
        }
    } else {
        mLocalMapFileName = localFileName;
    }

    QNetworkRequest request = QNetworkRequest(url);
    pHost->updateProxySettings(mpNetworkAccessManager);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    mExpectedFileSize = 4000000;

    const QString infoMsg = tr("[ INFO ]  - Map download initiated, please wait...");
    postMessage(infoMsg);
    qApp->processEvents();
    // Attempts to ensure INFO message gets shown before download is initiated!

    mpNetworkReply = mpNetworkAccessManager->get(request);
    // Using zero for both min and max values should cause the bar to oscillate
    // until the first update
    //: %1 is the name of the current Mudlet profile
    mpProgressDialog = new QProgressDialog(tr("Downloading map file for use in %1...")
                                              .arg(mProfileName), tr("Abort"), 0, 0);
    //: This is a title of a progress window.
    mpProgressDialog->setWindowTitle(tr("Map download"));
    mpProgressDialog->setWindowIcon(QIcon(qsl(":/icons/mudlet_map_download.png")));
    mpProgressDialog->setMinimumWidth(300);
    mpProgressDialog->setAutoClose(false);
    mpProgressDialog->setAutoReset(false);
    mpProgressDialog->setMinimumDuration(0); // Normally waits for 4 seconds before showing

    connect(mpNetworkReply, &QNetworkReply::downloadProgress, this, &TMap::slot_setDownloadProgress);
    connect(mpNetworkReply, &QNetworkReply::errorOccurred, this, &TMap::slot_downloadError);
    connect(mpProgressDialog, &QProgressDialog::canceled, this, &TMap::slot_downloadCancel);

    mpProgressDialog->show();
}

// Called from TLuaInterpreter::loadFile() or dlgProfilePreferences's "loadMap"
// both via TConsole::importMap( QFile & ) - it is intended to prevent
// readXmlMapFile( QFile & ) from being used more than once at a time and to
// prevent the above callers from using that when a map download is in progress!
// errMsg if, non-null is for a suitable structured error message to return to
// the TLuaInterpreter::loadFile(...) usage and is also needed to suppress the
// error message to the console
bool TMap::importMap(QFile& file, QString* errMsg)
{
    if (mImportRunning) {
        if (errMsg) {
            *errMsg = tr("loadMap: unable to perform request, a map is already being downloaded or\n"
                         "imported at user request.");
        } else {
            const QString warnMsg = qsl("[ WARN ]  - Attempt made to import an XML map when one is already being\n"
                                             "downloaded or is being imported from a local file - wait for that\n"
                                             "operation to complete (if it cannot be canceled) before retrying!");
            postMessage(warnMsg);
        }
        return false;
    }
    mImportRunning = true;
    // MUST clear this flag when done under ALL circumstances

    const bool result = readXmlMapFile(file, errMsg);
    mImportRunning = false;

    return result;
}

bool TMap::readXmlMapFile(QFile& file, QString* errMsg)
{
    Host* pHost = mpHost;
    bool isLocalImport = false;
    if (!pHost) {
        return false;
    }

    if (!mpProgressDialog) {
        // This is the local import case - which has not got a progress dialog
        // until now:
        isLocalImport = true;
        mpProgressDialog = new QProgressDialog(tr("Importing XML map file for use in %1...").arg(mProfileName), QString(), 0, 0);
        //: This is a title of a progress window.
        mpProgressDialog->setWindowTitle(tr("Map import"));
        mpProgressDialog->setWindowIcon(QIcon(qsl(":/icons/mudlet_map_download.png")));
        mpProgressDialog->setMinimumWidth(300);
        mpProgressDialog->setAutoClose(false);
        mpProgressDialog->setAutoReset(false);
        mpProgressDialog->setMinimumDuration(0); // Normally waits for 4 seconds before showing
    } else {
        ; // This is the download file case which is a no-op
    }

    // It is NOW safe to delete the map as we are in a position to load one
    mapClear();

    XMLimport reader(pHost);
    auto [success, message] = reader.importPackage(&file);

    if (!mpMapper.isNull() && mpMapper->mp2dMap) {
        // probably not needed for the download but might be
        // needed for local file case:
        mpMapper->mp2dMap->init();
        // No need to call audit() as XMLimport::importPackage() does it!
        // audit() produces the successful ending [ OK ] message...!
        mpMapper->updateAreaComboBox();
        if (success) {
            mpMapper->resetAreaComboBoxToPlayerRoomArea();
        } else {
            // Failed...
            if (errMsg) {
                *errMsg = tr("loadMap: failure to import XML map file, further information may be available\n"
                             "in main console!");
            }
        }
    }

    if (!success && errMsg) {
        *errMsg = tr("loadMap: failure to import XML map file, further information may be available\n"
                     "in main console!");
    }

    if (isLocalImport) {
        // clean-up
        mpProgressDialog->deleteLater();
        mpProgressDialog = nullptr;
    }

    if (!mpMapper.isNull()) {
         mpMapper->show();
    }

    return success;
}

void TMap::slot_setDownloadProgress(qint64 got, qint64 total)
{
    if (!mpProgressDialog) {
        return;
    }

    if (!mpProgressDialog->maximum()) {
        // First call, range has not been set;
        mpProgressDialog->setRange(0, mExpectedFileSize);
    } else if (total != -1 && mpProgressDialog->maximum() != static_cast<int>(total)) {
        // total will stick at -1 when we do not know how big the download is
        // which seems to be the case for the IRE MUDS - *sigh* - Slysven
        mpProgressDialog->setRange(0, static_cast<int>(total));
    }

    mpProgressDialog->setValue(static_cast<int>(got));
}

void TMap::slot_downloadCancel()
{
    const QString alertMsg = tr("[ ALERT ] - Map download was canceled, on user's request.");
    postMessage(alertMsg);
    if (mpProgressDialog) {
        mpProgressDialog->deleteLater();
        mpProgressDialog = nullptr; // Must reset this so it can be reused
    }
    if (mpNetworkReply) {
        mpNetworkReply->abort(); // Will indirectly cause error() AND replyFinished signals to be sent
    }
}

void TMap::slot_downloadError(QNetworkReply::NetworkError error)
{
    if (!mpNetworkReply) {
        return;
    }

    if (error != QNetworkReply::OperationCanceledError) {
        // No point in reporting Cancel as that is handled elsewhere
        const QString errMsg = tr("[ ERROR ] - Map download encountered an error:\n%1").arg(mpNetworkReply->errorString());
        postMessage(errMsg);
    }
}

void TMap::slot_replyFinished(QNetworkReply* reply)
{
    auto cleanup = [this, reply](){
        reply->deleteLater();
        mpNetworkReply = nullptr;

        // We don't delete the progress dialog until here as we now use it to inform
        // about post-download operations

        mpProgressDialog->deleteLater();
        mpProgressDialog = nullptr; // Must reset this so it can be reused

        mLocalMapFileName.clear();
        mExpectedFileSize = 0;
        // We have finished with the XMLimporter so must clear the flag
        mImportRunning = false;
    };


    if (reply != mpNetworkReply) {
        qWarning() << "TMap::slot_replyFinished( QNetworkReply * ) ERROR - received argument was not the expected stored pointer.";
    }

    if (reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::OperationCanceledError) {
        // Don't report on any errors here as we've already done so in slot_downloadError(...) previously.
        cleanup();
        return;
        // else was QNetworkReply::OperationCanceledError and we already handle
        // THAT in slot_downloadCancel()
    }
    // Separate the two kinds of files to gain QSaveFile's atomic write behavior
    QSaveFile writeFile(mLocalMapFileName);
    QFile readFile(mLocalMapFileName);
    if (!writeFile.open(QFile::WriteOnly)) {
        const QString alertMsg = tr("[ ALERT ] - Map download failed, unable to open destination file:\n%1.").arg(mLocalMapFileName);
        postMessage(alertMsg);
        cleanup();
        return;
    }
    // The QNetworkReply is Ok here:
    if (writeFile.write(reply->readAll()) == -1) {
        const QString alertMsg = tr("[ ALERT ] - Map download failed, unable to write destination file:\n%1.").arg(mLocalMapFileName);
        postMessage(alertMsg);
        cleanup();
        return;
    }
    if (!writeFile.commit()) {
        qDebug() << "TMap::slot_replyFinished: error saving downloaded map: " << writeFile.errorString();
    }

    Host* pHost = mpHost;
    if (!pHost) {
        qWarning() << "TMap::slot_replyFinished( QNetworkReply * ) ERROR - NULL Host pointer - something is really wrong!";
        cleanup();
        return;
    }

    const QString infoMsg = tr("[ INFO ]  - ... map downloaded and stored, now parsing it...");
    postMessage(infoMsg);

    // Since the download is complete but we do not offer to
    // cancel the required post-processing we should now hide
    // the cancel/abort button:
    mpProgressDialog->setCancelButton(nullptr);

    bool parsingWasSuccessful;
    QString parsingFileName;
    if (!readFile.fileName().endsWith(qsl("xml"), Qt::CaseInsensitive)) {
        parsingFileName = readFile.fileName();
        parsingWasSuccessful = pHost->mpConsole->loadMap(parsingFileName);
    } else {
        parsingFileName = mLocalMapFileName;
        if (!readFile.open(QFile::OpenMode(QFile::ReadOnly | QFile::Text))) {
            const QString alertMsg = tr("[ ERROR ] - Map download problem, unable to read destination file:\n%1.").arg(parsingFileName);
            postMessage(alertMsg);
            cleanup();
            return;
        }

        // The action to parse the XML file has been refactored to
        // a separate method so that it can be shared with the
        // direct importation of a local copy of a map file.
        parsingWasSuccessful = readXmlMapFile(readFile);
        readFile.close();
    }

    if (parsingWasSuccessful) {
        TEvent mapDownloadEvent {};
        mapDownloadEvent.mArgumentList.append(qsl("sysMapDownloadEvent"));
        mapDownloadEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        pHost->raiseEvent(mapDownloadEvent);
    } else {
        // Failure in parse file...
        const QString alertMsg = tr("[ ERROR ] - Map download problem, failure in parsing destination file:\n%1.").arg(parsingFileName);
        postMessage(alertMsg);
    }
    cleanup();
}

void TMap::reportStringToProgressDialog(const QString text)
{
    if (mpProgressDialog) {
        mpProgressDialog->setLabelText(text);
        // Needed to make the changed text show, it does increase the overall
        // time a little but as the main usage is when parsing XML room data
        // and that can take MORE THAN A MINUTE the activity is essential to
        // inform the user that something IS happening...
        qApp->processEvents();
    }
}

void TMap::reportProgressToProgressDialog(const int current, const int maximum)
{
    if (mpProgressDialog) {
        if (mpProgressDialog->maximum() != maximum) {
            mpProgressDialog->setMaximum(maximum);
        }
        mpProgressDialog->setValue(current);
    }
}

QHash<QString, QSet<int>> TMap::roomSymbolsHash()
{
    QHash<QString, QSet<int>> results;
    QHashIterator<int, TRoom*> itRoom(mpRoomDB->getRoomMap());
    while (itRoom.hasNext()) {
        itRoom.next();
        if (itRoom.value() && !itRoom.value()->mSymbol.isEmpty()) {
            if (results.contains(itRoom.value()->mSymbol)) {
                results[itRoom.value()->mSymbol].insert(itRoom.key());
            } else {
                QSet<int> newEntry;
                newEntry << itRoom.key();
                results.insert(itRoom.value()->mSymbol, newEntry);
            }
        }
    }
    return results;
}

void TMap::setMmpMapLocation(const QString &location)
{
    mMmpMapLocation = location;

    qDebug() << "MMP map registered at" << mMmpMapLocation;
}

QString TMap::getMmpMapLocation() const
{
    return mMmpMapLocation;
}

bool TMap::getRoomNamesPresent()
{
    return mUserData.contains(ROOM_UI_SHOWNAME);
}

bool TMap::getRoomNamesShown()
{
    return getUserDataBool(mUserData, ROOM_UI_SHOWNAME, false);
}

void TMap::setRoomNamesShown(bool shown)
{
    setUserDataBool(mUserData, ROOM_UI_SHOWNAME, shown);
}

/*
 * Notes on the format version numbers in JSON files - we use this to track any
 * changes in a major.minor number format, the minor number is to be three
 * digits long.
 *
 * 0.002 was the first published draft
 * 0.003 changed the format to encapsulate the room symbol as an object
 * which contains text and a color which was added separately during the
 * development of the JSON handling code. Also refactored the storage of
 * colors to identify whether there is an alpha component or not in the
 * array of values.
 * 1.000 is identical to 0.003 - but changed to make sense from a release point
 * of view.
 *
 * Currently only version 1.000 is expected or handled
 */
std::pair<bool, QString> TMap::writeJsonMapFile(const QString& dest)
{
    QString destination{dest};

    if (destination.isEmpty()) {
        const QString destFolder = mudlet::getMudletPath(enums::profileMapsPath, mProfileName);
        const QDir destDir(destFolder);
        if (!destDir.exists()) {
            destDir.mkdir(destFolder);
        }
        destination = mudlet::getMudletPath(enums::profileDateTimeStampedJsonMapPathFileName, mProfileName, QDateTime::currentDateTime().toString(qsl("yyyy-MM-dd#HH-mm-ss")));
    }

    if (!destination.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) {
        destination.append(QLatin1String(".json"));
    }

    if (mpProgressDialog) {
        return {false, qsl("import or export already in progress")};
    }

    mProgressDialogRoomsTotal = mpRoomDB->getRoomMap().count();
    mProgressDialogAreasTotal = mpRoomDB->getAreaMap().count();
    mProgressDialogLabelsTotal = 0;
    for (const auto area : mpRoomDB->getAreaMap()) {
        if (area) {
            mProgressDialogLabelsTotal += area->getPermanentLabelIds().count();
        }
    }

    mpProgressDialog = new QProgressDialog(tr("Exporting JSON map data from %1\n"
                                              "Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...")
                                           .arg(mProfileName,
                                                QLatin1String("0"),
                                                QString::number(mProgressDialogAreasTotal),
                                                QLatin1String("0"),
                                                QString::number(mProgressDialogRoomsTotal),
                                                QLatin1String("0"),
                                                QString::number(mProgressDialogLabelsTotal)),
                                           tr("Abort"),
                                           0,
                                           mProgressDialogRoomsTotal,
                                           mpHost->mpConsole);
    mpProgressDialog->setValue(0);
    mpProgressDialog->setWindowModality(Qt::NonModal);
    //: This is a title of a progress window.
    mpProgressDialog->setWindowTitle(tr("Map JSON export"));
    mpProgressDialog->setWindowIcon(QIcon(qsl(":/icons/mudlet_map_download.png")));
    mpProgressDialog->setMinimumWidth(500);
    mpProgressDialog->setAutoClose(false);
    mpProgressDialog->setAutoReset(false);
    mpProgressDialog->setMinimumDuration(1); // Normally waits for 4 seconds before showing
    qApp->processEvents();
    QSaveFile file(destination);
    if (!file.open(QFile::OpenMode(QFile::Text|QFile::WriteOnly))) {
        qWarning().noquote().nospace() << "TMap::writeJsonMapFile(...) WARNING - Could not open save file \"" << destination << "\".";
        mpProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        mpProgressDialog->close();
        mpProgressDialog = nullptr;
        return {false, qsl("could not open save file \"%1\", reason: %2").arg(destination.toHtmlEscaped(), file.errorString())};
    }

    QJsonObject mapObj;
    mapObj.insert(QLatin1String("formatVersion"), static_cast<double>(1.000));

    writeJsonUserData(mapObj);

    QList<int> areaRawIdsList{mpRoomDB->getAreaMap().keys()};
    QList<int> areaNameRawIdsList{mpRoomDB->getAreaNamesMap().keys()};

    QSet<int> areaIdsSet{areaRawIdsList.begin(), areaRawIdsList.end()};
    areaIdsSet.unite(QSet<int>{areaNameRawIdsList.begin(), areaNameRawIdsList.end()});
    QList<int> areaIdsList{areaIdsSet.begin(), areaIdsSet.end()};
    if (areaIdsList.count() > 1) {
        std::sort(areaIdsList.begin(), areaIdsList.end());
    }

    mProgressDialogAreasCount = 0;
    mProgressDialogRoomsCount = 0;
    mProgressDialogLabelsCount = 0;
    bool abort = false;
    QJsonArray areasArray;
    for (const auto area : mpRoomDB->getAreaMap()) {
        if (area) {
            area->writeJsonArea(areasArray);
        }
        ++mProgressDialogAreasCount;
        if (incrementJsonProgressDialog(true, true, 0)) {
            abort = true;
            break;
        }
    }
    if (abort) {
        file.cancelWriting();
        mpProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        mpProgressDialog->close();
        mpProgressDialog = nullptr;
        return {false, qsl("aborted by user")};
    }

    const QJsonValue areasValue{areasArray};
    mapObj.insert(QLatin1String("areas"), areasValue);

    // Should Qt change things so that the order in the file is not
    // alphabetically sorted but instead dependent on actually insertion order
    // then these must be precalculated and put first - as they are needed to
    // drive the progress dialogue:
    mapObj.insert(QLatin1String("areaCount"), static_cast<double>(areaIdsList.count()));
    mapObj.insert(QLatin1String("roomCount"), static_cast<double>(mProgressDialogRoomsCount));
    mapObj.insert(QLatin1String("labelCount"), static_cast<double>(mProgressDialogLabelsTotal));

    const QJsonValue defaultAreaNameValue{mDefaultAreaName};
    mapObj.insert(QLatin1String("defaultAreaName"), defaultAreaNameValue);

    const QJsonValue anonymousAreaNameValue{mUnnamedAreaName};
    mapObj.insert(QLatin1String("anonymousAreaName"), anonymousAreaNameValue);

    if (!mEnvColors.isEmpty()) {
        QJsonObject envColorObj;
        QMapIterator<int, int> itEnvColor(mEnvColors);
        while (itEnvColor.hasNext()) {
            itEnvColor.next();
            envColorObj.insert(QString::number(itEnvColor.key()), static_cast<double>(itEnvColor.value()));
        }
        const QJsonValue mEnvColorsValue{envColorObj};
        mapObj.insert(QLatin1String("envToColorMapping"), mEnvColorsValue);
    }

    QJsonObject playerRoomIdHashObj;
    QHashIterator<QString, int> itplayerRoomIdHash(mRoomIdHash);
    while (itplayerRoomIdHash.hasNext()) {
        itplayerRoomIdHash.next();
        playerRoomIdHashObj.insert(itplayerRoomIdHash.key(), static_cast<double>(itplayerRoomIdHash.value()));
    }
    const QJsonValue playerRoomIdHashsValue{playerRoomIdHashObj};
    mapObj.insert(QLatin1String("playersRoomId"), playerRoomIdHashsValue);

    QJsonArray customEnvColorArray;
    QMapIterator<int, QColor> itCustomEnvColor(mCustomEnvColors);
    while (itCustomEnvColor.hasNext()) {
        itCustomEnvColor.next();
        QJsonObject customEnvColorObj{};
        // Should insert an array value into the customEnvColorObj with the key
        // "colorRGBA"
        writeJsonColor(customEnvColorObj, itCustomEnvColor.value());
        customEnvColorObj.insert(QLatin1String("id"), QJsonValue{itCustomEnvColor.key()});
        // Convert the customEnvColorObj into a QJsonValue:
        const QJsonValue customEnvColorValue{customEnvColorObj};
        // Now append this object onto the array:
        customEnvColorArray.append(customEnvColorValue);
    }
    // Convert the array of all the mCustomEnvColors into a QJsonValue so we
    // can add it to the map object:
    const QJsonValue mCustomEnvColorsValue{customEnvColorArray};
    mapObj.insert(QLatin1String("customEnvColors"), mCustomEnvColorsValue);

    mapObj.insert(QLatin1String("mapSymbolFontDetails"), mMapSymbolFont.toString());
    mapObj.insert(QLatin1String("mapSymbolFontFudgeFactor"), static_cast<double>(mMapSymbolFontFudgeFactor));
    mapObj.insert(QLatin1String("onlyMapSymbolFontToBeUsed"), mIsOnlyMapSymbolFontToBeUsed);

    QJsonArray playerRoomColorsArray;
    QJsonObject playerRoomOuterColorObj;
    QJsonObject playerRoomInnerColorObj;
    writeJsonColor(playerRoomOuterColorObj, mPlayerRoomOuterColor);
    writeJsonColor(playerRoomInnerColorObj, mPlayerRoomInnerColor);
    const QJsonValue playerRoomOuterColorValue{playerRoomOuterColorObj};
    const QJsonValue playerRoomInnerColorValue{playerRoomInnerColorObj};
    playerRoomColorsArray.append(playerRoomOuterColorValue);
    playerRoomColorsArray.append(playerRoomInnerColorValue);
    const QJsonValue playerRoomColorsValue{playerRoomColorsArray};
    mapObj.insert(QLatin1String("playerRoomColors"), playerRoomColorsValue);
    mapObj.insert(QLatin1String("playerRoomStyle"), static_cast<double>(mPlayerRoomStyle));
    mapObj.insert(QLatin1String("playerRoomOuterDiameterPercentage"), static_cast<double>(mPlayerRoomOuterDiameterPercentage));
    mapObj.insert(QLatin1String("playerRoomInnerDiameterPercentage"), static_cast<double>(mPlayerRoomInnerDiameterPercentage));

    mpProgressDialog->setLabelText(tr("Exporting JSON map file from %1 - writing data to file:\n"
                                      "%2 ...").arg(mProfileName, destination));
    mpProgressDialog->setValue(0);
    // Hide the cancel button as we can't stop now:
    mpProgressDialog->setCancelButton(nullptr);
    file.write(QJsonDocument(mapObj).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qDebug() << "TMap::writeJsonMapFile: error saving JSON map: " << file.errorString();
    }

    mpProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    mpProgressDialog->close();
    mpProgressDialog = nullptr;

    return {file.error() == QFileDevice::NoError,
                ((file.error() == QFileDevice::NoError) ? QString() : qsl("could not export file, reason: %1").arg(file.errorString()))};
}

// The translatable messages are used within this file and do not need to
// mention the file concerned whereas the untranslated messages are used by the
// Lua sub-system and do need to report the file:
std::pair<bool, QString> TMap::readJsonMapFile(const QString& source, const bool translatableTexts, const bool allowUserCancellation)
{
    const QString oldDefaultAreaName{mDefaultAreaName};
    const QString oldUnnamedName{mUnnamedAreaName};

    if (mpProgressDialog) {
        return {false, (translatableTexts
                    ? tr("import or export already in progress")
                    : qsl("import or export already in progress"))};
    }

    QFile file(source);
    if (!file.open(QFile::ReadOnly)) {
        qWarning().noquote().nospace() << "TMap::readJsonMapFile(...) WARNING - Could not open JSON file \"" << source << "\".";
        return {false, (translatableTexts
                    ? tr("could not open file")
                    : qsl("could not open file \"%1\"").arg(source))};
    }

    const QByteArray mapData = file.readAll();
    file.close();
    QJsonParseError jsonErr;
    const QJsonDocument doc(QJsonDocument::fromJson(mapData, &jsonErr));
    if (jsonErr.error != QJsonParseError::NoError) {
        return {false, (translatableTexts
                    ? tr("could not parse file, reason: \"%1\" at offset %2")
                      .arg(jsonErr.errorString(), QString::number(jsonErr.offset))
                    : qsl("could not parse file \"%1\", reason: \"%2\" at offset %3")
                      .arg(source, jsonErr.errorString(), QString::number(jsonErr.offset)))};
    }

    if (doc.isEmpty()) {
        qDebug().nospace().noquote() << "TMap::readJsonMapFile(\"" << source << "\") INFO - no Json file data detected, this is not a Mudlet JSON map file.";
        return {false, (translatableTexts
                    ? tr("empty Json file, no map data detected")
                    : qsl("empty Json file, no map data detected"))};
    }

    // Read all the base level stuff:
    QJsonObject mapObj{doc.object()};
    double formatVersion = 0.0f;
    if (mapObj.contains(QLatin1String("formatVersion")) && mapObj[QLatin1String("formatVersion")].isDouble()) {
        formatVersion = mapObj[QLatin1String("formatVersion")].toDouble();
        if (qFuzzyCompare(1.0, formatVersion + 1.0) || formatVersion < 1.0000 || formatVersion > 1.0000) {
            // We only handle 1.000f right now (0.001f was borked, 0.002f
            // didn't include room symbol color, 0.003 is the same as 1.000
            // but the numbered was changed for release into the wild):
            qDebug().nospace().noquote() << "TMap::readJsonMapFile(\"" << source << "\") INFO - Version information \"" << formatVersion << "\" was found, and it is not okay.";
            return {false, (translatableTexts
                        ? tr("invalid format version \"%1\" detected").arg(formatVersion, 0, 'f', 3, QLatin1Char('0'))
                        : qsl("invalid format version \"%1\" detected").arg(formatVersion, 0, 'f', 3, QLatin1Char('0')))};
        }
    } else {
        qDebug().nospace().noquote() << "TMap::readJsonMapFile(\"" << source << "\") INFO - Version information was not found. This is not likely to be a Mudlet JSON map file.";
        return {false, (translatableTexts
                    ? tr("no format version detected")
                    : qsl("no format version detected"))};
    }

    if (!mapObj.contains(QLatin1String("areas")) || !mapObj.value(QLatin1String("areas")).isArray()) {
        return {false, (translatableTexts
                    ? tr("no areas detected")
                    : qsl("no areas detected"))};
    }

    mProgressDialogAreasTotal = qRound(mapObj[QLatin1String("areaCount")].toDouble());
    mProgressDialogAreasCount = 0;
    mProgressDialogRoomsTotal = qRound(mapObj[QLatin1String("roomCount")].toDouble());
    mProgressDialogRoomsCount = 0;
    mProgressDialogLabelsTotal = qRound(mapObj[QLatin1String("labelCount")].toDouble());
    mProgressDialogLabelsCount = 0;
    mpProgressDialog = new QProgressDialog(tr("Importing JSON map data to %1\n"
                                              "Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...")
                                                   .arg(mProfileName,
                                                        QLatin1String("0"),
                                                        QString::number(mProgressDialogAreasTotal),
                                                        QLatin1String("0"),
                                                        QString::number(mProgressDialogRoomsTotal),
                                                        QLatin1String("0"),
                                                        QString::number(mProgressDialogLabelsTotal)),
                                           (allowUserCancellation ? tr("Abort") : QString()),
                                           0,
                                           mProgressDialogRoomsTotal,
                                           mpHost->mpConsole);
    mpProgressDialog->setValue(0);
    mpProgressDialog->setWindowModality(Qt::NonModal);
    //: This is a title of a progress window.
    mpProgressDialog->setWindowTitle(tr("Map JSON import"));
    mpProgressDialog->setWindowIcon(QIcon(qsl(":/icons/mudlet_map_download.png")));
    mpProgressDialog->setMinimumWidth(500);
    mpProgressDialog->setAutoClose(false);
    mpProgressDialog->setAutoReset(false);
    mpProgressDialog->setMinimumDuration(1); // Normally waits for 4 seconds before showing
    qApp->processEvents();

    mDefaultAreaName = mapObj[QLatin1String("defaultAreaName")].toString();
    mUnnamedAreaName = mapObj[QLatin1String("anonymousAreaName")].toString();
    if (mapObj.contains(QLatin1String("userData"))) {
        readJsonUserData(mapObj[QLatin1String("userData")].toObject());
    }
    const QString mapSymbolFontText = mapObj[QLatin1String("mapSymbolFontDetails")].toString();
    const float mapSymbolFontFudgeFactor = (qRound(mapObj[QLatin1String("mapSymbolFontFudgeFactor")].toDouble() * 1000.0)) / 1000;
    const bool isOnlyMapSymbolFontToBeUsed = mapObj[QLatin1String("onlyMapSymbolFontToBeUsed")].toBool();
    const int playerRoomStyle = qRound(mapObj[QLatin1String("playerRoomStyle")].toDouble());
    quint8 const playerRoomOuterDiameterPercentage = qRound(mapObj[QLatin1String("playerRoomOuterDiameterPercentage")].toDouble());
    quint8 const playerRoomInnerDiameterPercentage = qRound(mapObj[QLatin1String("playerRoomInnerDiameterPercentage")].toDouble());
    QColor playerRoomOuterColor;
    QColor playerRoomInnerColor;

    if (mapObj.contains(QLatin1String("playerRoomColors")) && mapObj.value(QLatin1String("playerRoomColors")).isArray()) {
        const QJsonArray playerRoomColorArray = mapObj.value(QLatin1String("playerRoomColors")).toArray();
        if (playerRoomColorArray.size() == 2 && playerRoomColorArray.at(0).isObject() && playerRoomColorArray.at(1).isObject()) {
            playerRoomOuterColor = readJsonColor(playerRoomColorArray.at(0).toObject());
            playerRoomInnerColor = readJsonColor(playerRoomColorArray.at(1).toObject());
        }
    }

    QMap<int, int> envColors;
    if (mapObj.contains(QLatin1String("envToColorMapping")) && mapObj.value(QLatin1String("envToColorMapping")).isObject()) {
        const QJsonObject envColorObj{mapObj.value(QLatin1String("envToColorMapping")).toObject()};
        if (!envColorObj.isEmpty()) {
            for (auto& key : envColorObj.keys()) {
                bool isOk = false;
                const int index = key.toInt(&isOk);
                if (isOk && envColorObj.value(key).isDouble()) {
                    const int value = envColorObj.value(key).toInt();
                    envColors.insert(index, value);
                }
            }
        }
    }

    QMap<int, QColor> customEnvColors;
    if (mapObj.contains(QLatin1String("customEnvColors")) && mapObj.value(QLatin1String("customEnvColors")).isArray()) {
        const QJsonArray customEnvColorArray = mapObj.value(QLatin1String("customEnvColors")).toArray();
        if (!customEnvColorArray.isEmpty()) {
            for (const auto& customEnvColorValue : std::as_const(customEnvColorArray)) {
                const QJsonObject customEnvColorObj{customEnvColorValue.toObject()};
                if (customEnvColorObj.contains(QLatin1String("id"))
                    && ((customEnvColorObj.contains(QLatin1String("color32RGBA")) && customEnvColorObj.value(QLatin1String("color32RGBA")).isArray())
                        ||(customEnvColorObj.contains(QLatin1String("color24RGB")) && customEnvColorObj.value(QLatin1String("color24RGB")).isArray()))
                    && customEnvColorObj.value(QLatin1String("id")).isDouble()) {

                    const int id{customEnvColorObj.value(QLatin1String("id")).toInt()};
                    const QColor color{readJsonColor(customEnvColorObj)};
                    customEnvColors.insert(id, color);
                }
            }
        }
    }

    QHash<QString, int> playersRoomId;
    if (mapObj.contains(QLatin1String("playersRoomId")) && mapObj.value(QLatin1String("playersRoomId")).isObject()) {
        const QJsonObject playersRoomIdObj{mapObj.value(QLatin1String("playersRoomId")).toObject()};
        if (!playersRoomIdObj.isEmpty()) {
            for (auto& profileName : playersRoomIdObj.keys()) {
                if (playersRoomIdObj.value(profileName).isDouble()) {
                    playersRoomId.insert(profileName, playersRoomIdObj.value(profileName).toInt());
                }
            }
        }
    }

    TRoomDB* pNewRoomDB = new TRoomDB(this);
    bool abort = false;
    for (int i = 0, total = mapObj.value(QLatin1String("areas")).toArray().count(); i < total; ++i) {
        std::unique_ptr<TArea> pArea = std::make_unique<TArea>(this, pNewRoomDB);
        auto [id, name] = pArea->readJsonArea(mapObj.value(QLatin1String("areas")).toArray(), i);
        ++mProgressDialogAreasCount;
        if (incrementJsonProgressDialog(false, true, 0)) {
            if (allowUserCancellation) {
                abort = true;
            }
            break;
        }
        // This will populate the TRoomDB::areas and TRoomDB::areaNameMap:
        pNewRoomDB->addArea(pArea.release(), id, name);
    }
    if (abort) {
        mpProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        mpProgressDialog->close();
        mpProgressDialog = nullptr;
        mDefaultAreaName = oldDefaultAreaName;
        mUnnamedAreaName = oldUnnamedName;
        delete pNewRoomDB;
        return {false, (translatableTexts
                    ? tr("aborted by user")
                    : qsl("aborted by user"))};
    }

    mCustomEnvColors.swap(customEnvColors);
    mEnvColors.swap(envColors);
    mIsOnlyMapSymbolFontToBeUsed = isOnlyMapSymbolFontToBeUsed;
    QFont mapSymbolFont;
    mapSymbolFont.fromString(mapSymbolFontText);
    mapSymbolFont.setStyleStrategy(static_cast<QFont::StyleStrategy>((isOnlyMapSymbolFontToBeUsed ? QFont::NoFontMerging : 0)
                                                                     |QFont::PreferOutline | QFont::PreferAntialias | QFont::PreferQuality
                                                                     |QFont::PreferNoShaping));

    mMapSymbolFont.swap(mapSymbolFont);
    mMapSymbolFontFudgeFactor = mapSymbolFontFudgeFactor;
    mPlayerRoomInnerColor = playerRoomInnerColor;
    mPlayerRoomInnerDiameterPercentage = playerRoomInnerDiameterPercentage;
    mPlayerRoomOuterColor = playerRoomOuterColor;
    mPlayerRoomOuterDiameterPercentage = playerRoomOuterDiameterPercentage;
    mPlayerRoomStyle = playerRoomStyle;
    mRoomIdHash = playersRoomId;
    qDebug().nospace().noquote() << "TMap::readJsonMapFile(...) INFO - parsed a file (version: " << formatVersion << ") containing " << mProgressDialogRoomsCount << " rooms.";

    // This is it - the point at which the new map gets activated:
    TRoomDB* pOldRoomDB = mpRoomDB;
    mpRoomDB = pNewRoomDB;
    // Need to update the master copy of these details in the Host class:
    mpHost->setPlayerRoomStyleDetails(mPlayerRoomStyle, mPlayerRoomOuterDiameterPercentage, mPlayerRoomInnerDiameterPercentage, mPlayerRoomOuterColor, mPlayerRoomInnerColor);
    // And redraw the indicator if a 2D map is being shown:
    if (mpMapper && mpMapper->mp2dMap) {
        mpMapper->mp2dMap->setPlayerRoomStyle(mPlayerRoomStyle);
    }
    delete pOldRoomDB;
    mpProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    mpProgressDialog->close();
    mpProgressDialog = nullptr;
    return {true, QString()};
}

void TMap::writeJsonUserData(QJsonObject& obj) const
{
    QJsonObject userDataObj;
    if (mUserData.isEmpty()) {
        // Skip creating a user data array if it will be empty:
        return;
    }
    QMapIterator<QString, QString> itDataItem(mUserData);
    while (itDataItem.hasNext()) {
        itDataItem.next();
        const QJsonValue jsonValue{itDataItem.value()};
        userDataObj.insert(itDataItem.key(), jsonValue);
    }
    const QJsonValue jsonValue{userDataObj};
    obj.insert(QLatin1String("userData"), jsonValue);
}

// Takes a userData object and parses all its elements
void TMap::readJsonUserData(const QJsonObject& obj)
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

// Inserts a color as an array of 3 or 4 ints (cast to doubles) into the
// supplied object.
void TMap::writeJsonColor(QJsonObject& obj, const QColor& color)
{
    QJsonArray colorRGBAArray;
    colorRGBAArray.append(static_cast<double>(color.red()));
    colorRGBAArray.append(static_cast<double>(color.green()));
    colorRGBAArray.append(static_cast<double>(color.blue()));
    if (color.alpha() < 255) {
        colorRGBAArray.append(static_cast<double>(color.alpha()));
        const QJsonValue colorRGBAValue{colorRGBAArray};
        obj.insert(QLatin1String("color32RGBA"), colorRGBAValue);
    } else {
        const QJsonValue colorRGBAValue{colorRGBAArray};
        obj.insert(QLatin1String("color24RGB"), colorRGBAValue);
    }
}

QColor TMap::readJsonColor(const QJsonObject& obj)
{
    if (!(   (obj.contains(QLatin1String("color32RGBA")) && obj.value(QLatin1String("color32RGBA")).isArray())
          || (obj.contains(QLatin1String("color24RGB")) && obj.value(QLatin1String("color24RGB")).isArray()))) {
        // Return a null color if one was not found
        return QColor();
    }

    QJsonArray colorRGBAArray;
    bool hasAlpha = false;
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 255;
    if (obj.contains(QLatin1String("color32RGBA"))) {
        colorRGBAArray = obj.value(QLatin1String("color32RGBA")).toArray();
        hasAlpha = true;
    } else {
        colorRGBAArray = obj.value(QLatin1String("color24RGB")).toArray();
    }
    const int size = colorRGBAArray.size();
    if ((size == 3 || size == 4)
        && colorRGBAArray.at(0).isDouble()
        && colorRGBAArray.at(1).isDouble()
        && colorRGBAArray.at(2).isDouble()) {

        red = qRound(colorRGBAArray.at(0).toDouble());
        green = qRound(colorRGBAArray.at(1).toDouble());
        blue = qRound(colorRGBAArray.at(2).toDouble());
        return QColor(red, green, blue);
    }

    if (hasAlpha && size == 4 && colorRGBAArray.at(3).isDouble()) {
        alpha = qRound(colorRGBAArray.at(3).toDouble());
        return QColor(red, green, blue, alpha);
    }

    return QColor();
}


bool TMap::incrementJsonProgressDialog(const bool isExportNotImport, const bool isRoomNotLabel, const int increment)
{
    if (isRoomNotLabel) {
        mProgressDialogRoomsCount += increment;
    } else {
        mProgressDialogLabelsCount += increment;
    }

    mpProgressDialog->setValue(mProgressDialogRoomsCount);
    if (isExportNotImport) {
        mpProgressDialog->setLabelText(tr("Exporting JSON map data from %1\n"
                                          "Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...")
                                       .arg(mProfileName,
                                            QString::number(mProgressDialogAreasCount),
                                            QString::number(mProgressDialogAreasTotal),
                                            QString::number(mProgressDialogRoomsCount),
                                            QString::number(mProgressDialogRoomsTotal),
                                            QString::number(mProgressDialogLabelsCount),
                                            QString::number(mProgressDialogLabelsTotal)));
    } else {
        mpProgressDialog->setLabelText(tr("Importing JSON map data to %1\n"
                                          "Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...")
                                       .arg(mProfileName,
                                            QString::number(mProgressDialogAreasCount),
                                            QString::number(mProgressDialogAreasTotal),
                                            QString::number(mProgressDialogRoomsCount),
                                            QString::number(mProgressDialogRoomsTotal),
                                            QString::number(mProgressDialogLabelsCount),
                                            QString::number(mProgressDialogLabelsTotal)));
    }
    qApp->processEvents();
    return mpProgressDialog->wasCanceled();
}

/**
 * Update the the 2D and 3D map visually.
 *
 * It ensures debouncing internally to ensure that bulk calls are efficient.
 */
void TMap::update()
{
    static bool debounce;
    if (!debounce) {
        debounce = true;
        QTimer::singleShot(0, this, [this]() {
            debounce = false;

#if defined(INCLUDE_3DMAPPER)
            if (mpM) {
                mpM->update();
            }
#endif
            if (mpMapper) {

                if (mpMapper->mp2dMap) {
                    mpMapper->mp2dMap->mNewMoveAction = true;
                    mpMapper->mp2dMap->update();
                }
            }
        });
    }
}

QColor TMap::getColor(int id)
{
    QColor color;

    TRoom* room = mpRoomDB->getRoom(id);
    if (!room) {
        return color;
    }

    int env = room->environment;
    if (mEnvColors.contains(env)) {
        env = mEnvColors.value(env);
    } else {
        if (!mCustomEnvColors.contains(env)) {
            env = 1;
        }
    }
    switch (env) {
    case 1:     color = mpHost->mRed_2;             break;
    case 2:     color = mpHost->mGreen_2;           break;
    case 3:     color = mpHost->mYellow_2;          break;
    case 4:     color = mpHost->mBlue_2;            break;
    case 5:     color = mpHost->mMagenta_2;         break;
    case 6:     color = mpHost->mCyan_2;            break;
    case 7:     color = mpHost->mWhite_2;           break;
    case 8:     color = mpHost->mBlack_2;           break;
    case 9:     color = mpHost->mLightRed_2;        break;
    case 10:    color = mpHost->mLightGreen_2;      break;
    case 11:    color = mpHost->mLightYellow_2;     break;
    case 12:    color = mpHost->mLightBlue_2;       break;
    case 13:    color = mpHost->mLightMagenta_2;    break;
    case 14:    color = mpHost->mLightCyan_2;       break;
    case 15:    color = mpHost->mLightWhite_2;      break;
    case 16:    color = mpHost->mLightBlack_2;      break;
    default: //user defined room color
        if (!mCustomEnvColors.contains(env)) {
            if (16 < env && env < 232) {
                quint8 const base = env - 16;
                quint8 r = base / 36;
                quint8 g = (base - (r * 36)) / 6;
                quint8 b = (base - (r * 36)) - (g * 6);

                r = r == 0 ? 0 : (r - 1) * 40 + 95;
                g = g == 0 ? 0 : (g - 1) * 40 + 95;
                b = b == 0 ? 0 : (b - 1) * 40 + 95;
                color = QColor(r, g, b, 255);
            } else if (231 < env && env < 256) {
                quint8 const k = ((env - 232) * 10) + 8;
                color = QColor(k, k, k, 255);
            }
            break;
        }
        color = mCustomEnvColors.value(env);
    }
    return color;
}

void TMap::restore16ColorSet()
{
    mCustomEnvColors[257] = mpHost->mRed_2;
    mCustomEnvColors[258] = mpHost->mGreen_2;
    mCustomEnvColors[259] = mpHost->mYellow_2;
    mCustomEnvColors[260] = mpHost->mBlue_2;
    mCustomEnvColors[261] = mpHost->mMagenta_2;
    mCustomEnvColors[262] = mpHost->mCyan_2;
    mCustomEnvColors[263] = mpHost->mWhite_2;
    mCustomEnvColors[264] = mpHost->mBlack_2;
    mCustomEnvColors[265] = mpHost->mLightRed_2;
    mCustomEnvColors[266] = mpHost->mLightGreen_2;
    mCustomEnvColors[267] = mpHost->mLightYellow_2;
    mCustomEnvColors[268] = mpHost->mLightBlue_2;
    mCustomEnvColors[269] = mpHost->mLightMagenta_2;
    mCustomEnvColors[270] = mpHost->mLightCyan_2;
    mCustomEnvColors[271] = mpHost->mLightWhite_2;
    mCustomEnvColors[272] = mpHost->mLightBlack_2;
}

void TMap::setUnsaved(const char* fromWhere)
{
#if !defined(DEBUG_MAPAUTOSAVE)
    Q_UNUSED(fromWhere)
#else
    QString nowString = QDateTime::currentDateTimeUtc().toString("HH:mm:ss.zzz");
    qDebug().nospace().noquote() << "TMap::setUnsaved(...) INFO - called at: " << nowString << " from: " << fromWhere << ".";
#endif
    mUnsavedMap = true;
}

void TMap::setDefaultAreaShown(bool state)
{
    if (mShowDefaultArea != state) {
        mShowDefaultArea = state;
        if (!mpMapper.isNull()) {
            mpMapper->updateAreaComboBox();
        }
    }
}
