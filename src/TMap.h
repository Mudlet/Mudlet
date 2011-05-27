/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn  KoehnHeiko@googlemail.com     *
 *                                                                         *
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



#ifndef TMAP_H
#define TMAP_H

class TRoom;
class TArea;
class Host;
class GLWidget;
class T2DMap;

#include <QMap>
#include "TRoom.h"
#include "TArea.h"
#include "glwidget.h"
#include <stdlib.h>
#include "TAstar.h"
//#include "dlgMapper.h"

class dlgMapper;

class TMap
{
public:
    TMap( Host *);
    bool addRoom( int id=0 );
    void deleteRoom( int id );
    void deleteArea( int id );
    int  createNewRoomID();
    bool fixExits2(int);
    void tidyMap( int area );
    void getConnectedNodesGreaterThanX( int id, int x );
    void getConnectedNodesSmallerThanX( int id, int x );
    void getConnectedNodesGreaterThanY( int id, int x );
    void getConnectedNodesSmallerThanY( int id, int x );
    bool plausabilitaetsCheck( int area );
    void astBreitenAnpassung( int id, int );
    void astHoehenAnpassung( int id, int );
    bool setExit( int from, int to, int dir );
    bool setRoomCoordinates( int id, int x, int y, int z );
    void init(Host*);
    void buildAreas();
    bool fixExits( int id, int dir );
    QList<int> detectRoomCollisions( int id );
    void solveRoomCollision( int id, int creationDirection, bool PCheck=true );
    void setRoom( int );
    bool findPath( int from, int to );
    bool gotoRoom( int );
    bool gotoRoom( int, int );
    void setView( float, float, float, float );
    bool serialize( QDataStream & );
    bool restore();
    void initGraph();
    QMap<int, TRoom *> rooms;
    QMap<int, TArea *> areas;
    QMap<int, QString> areaNamesMap;
    QMap<int, int> envColors;
    QVector3D span;
    Host * mpHost;
    int mRoomId;
    int mTargetID;
    QList<int> mPathList;
    QList<QString> mDirList;
    QMap<int,QColor> customEnvColors;
    GLWidget * mpM;
    dlgMapper * mpMapper;
    QList<int> mTestedNodes;
    QList<int> conList;
    int mPlausaOptOut;
    QMap<QString,int> hashTable;
    QMap<QString, int> pixNameTable;
    QMap<int, QPixmap> pixTable;
    typedef adjacency_list<listS, vecS, directedS, no_property, property<edge_weight_t, cost> > mygraph_t;
    typedef property_map<mygraph_t, edge_weight_t>::type WeightMap;
    typedef mygraph_t::vertex_descriptor vertex;
    typedef mygraph_t::edge_descriptor edge_descriptor;
    typedef mygraph_t::vertex_iterator vertex_iterator;
    typedef std::pair<int, int> edge;
    mygraph_t g;
    WeightMap weightmap;
    std::vector<location> locations;
    bool mMapGraphNeedsUpdate;


};




#endif // TMAP_H


