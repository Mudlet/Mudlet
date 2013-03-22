/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#ifndef TROOMDB_H
#define TROOMDB_H


#include "TArea.h"
#include "TMap.h"
#include <QMap>
#include <QString>
#include "XMLexport.h"

//class TMap;
//class TArea;
#include "TRoom.h"

class TRoomDB
{
public:
    TRoomDB( TMap * );

    TRoom * getRoom( int id );
    TArea * getArea( int id );
    int getArea( TArea * pA );
    bool addRoom( int id );
    int size() { return rooms.size(); }
    bool removeRoom( int id );
    bool removeArea( int id );
    bool removeArea( QString name );
    void removeArea( TArea * );
    bool addArea(int id);
    int addArea( QString name );
    bool addArea( int id, QString name );
    void setAreaName( int areaID, QString name );
    const QList<TRoom *> getRoomPtrList();
    const QList<TArea *> getAreaPtrList();
    const QMap<int, TRoom *> & getRoomMap() const { return rooms; }
    const QMap<int, TArea *> & getAreaMap() const { return areas; }
    QList<int> getRoomIDList();
    QList<int> getAreaIDList();
    const QMap<int, QString> & getAreaNamesMap() const { return areaNamesMap; }


    void buildAreas();
    void clearMapDB();
    void initAreasForOldMaps();
    void auditRooms();
    bool addRoom(int id, TRoom *pR);
    int getAreaID(TArea * pA);
    void restoreAreaMap( QDataStream & );
    void restoreSingleArea( QDataStream &, int, TArea * );
    void restoreSingleRoom( QDataStream &, int, TRoom * );
    QMap<QString,int> hashTable;



private:
    TRoomDB(){}
    int createNewAreaID();
    bool __removeRoom( int id );

    QMap<int, TRoom *> rooms;
    QMap<int, TArea *> areas;
    QMap<int, QString> areaNamesMap;
    TMap * mpMap;

    friend class TRoom;//friend TRoom::~TRoom();
    //friend class TMap;//bool TMap::restore(QString location);
    //friend bool TMap::serialize(QDataStream &);
    friend class XMLexport;
};

#endif // TROMMDB_H
