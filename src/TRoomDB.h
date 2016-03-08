#ifndef MUDLET_TROOMDB_H
#define MUDLET_TROOMDB_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2016 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "pre_guard.h"
#include <QHash>
#include <QMultiHash>
#include <QMap>
#include <QString>
#include "post_guard.h"

class TArea;
class TMap;
class TRoom;


class TRoomDB
{
public:
    TRoomDB( TMap * );

    TRoom * getRoom( int id );
    TArea * getArea( int id );
//     int getArea( TArea * pA ); use duplicate int getAreaID( TArea * pA ) instead
    bool addRoom( int id );
    int size() { return rooms.size(); }
    bool removeRoom( int );
    void removeRoom( QSet<int> & );
    bool removeArea( int id );
    bool removeArea( QString name );
    void removeArea( TArea * );
    bool addArea(int id);
    int addArea( QString name );
    bool addArea( int id, QString name );
    bool setAreaName( int areaID, QString name );
    const QList<TRoom *> getRoomPtrList();
    const QList<TArea *> getAreaPtrList();
    const QHash<int, TRoom *> & getRoomMap() const { return rooms; }
    const QMap<int, TArea *> & getAreaMap() const { return areas; }
    QList<int> getRoomIDList();
    QList<int> getAreaIDList();
    const QMap<int, QString> & getAreaNamesMap() const { return areaNamesMap; }
    void updateEntranceMap(TRoom *, bool isMapLoading = false );
    void updateEntranceMap(int);
    const QMultiHash<int, int> & getEntranceHash() const { return entranceMap; }
    void deleteValuesFromEntranceMap( int );
    void deleteValuesFromEntranceMap( QSet<int> & );

    void buildAreas();
    void clearMapDB();
    void initAreasForOldMaps();
    void auditRooms();
    bool addRoom(int id, TRoom *pR, bool isMapLoading = false);
    int  getAreaID(TArea * pA);
    void restoreAreaMap( QDataStream & );
    void restoreSingleArea( QDataStream &, int, TArea * );
    void restoreSingleRoom( QDataStream &, int, TRoom * );


    QMap<QString,int> hashTable;


private:
    TRoomDB(){}
    int createNewAreaID();
    bool __removeRoom( int id );

    QHash<int, TRoom *> rooms;
    QMultiHash<int, int> entranceMap; // key is exit target, value is exit source
    QMap<int, TArea *> areas;
    QMap<int, QString> areaNamesMap;
    TMap * mpMap;
    QSet<int> * mpTempRoomDeletionSet; // Used during bulk room deletion
    QString mUnnamedAreaName;

    friend class TRoom;//friend TRoom::~TRoom();
    //friend class TMap;//bool TMap::restore(QString location);
    //friend bool TMap::serialize(QDataStream &);
    friend class XMLexport;
};

#endif // MUDLET_TROOMDB_H
