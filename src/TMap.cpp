/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
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


#include "TMap.h"
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include "dlgMapper.h"

TMap::TMap( Host * pH )
: mpM( 0 )
, mpMapper( 0 )
, mpHost( pH )
{
}

bool TMap::addRoom( int id )
{
    TRoom * pT = new TRoom;

    if( ! rooms.contains( id ) && id != 0 )
    {
        rooms[id] = pT;
        pT->id = id;
        return true;
    }
    else
    {
        delete pT;
        return false;
    }
}

bool TMap::setRoomCoordinates( int id, int x, int y, int z )
{
    if( ! rooms.contains( id ) ) return false;

    rooms[id]->x = x;
    rooms[id]->y = y;
    rooms[id]->z = z;
    return true;
}

int TMap::createNewRoomID()
{
    int _id = 100000000;
    for( ; _id > 0; _id++ )
    {
        if( ! rooms.contains( _id ) )
        {
            return _id;
        }
    }
    return 0;
}

bool TMap::setExit( int from, int to, int dir )
{
    if( ! rooms.contains( from ) ) return false;
    if( ! rooms.contains( to ) ) return false;

    switch( dir )
    {
        case DIR_NORTH: rooms[from]->north = to; break;
        case DIR_NORTHEAST: rooms[from]->northeast = to; break;
        case DIR_NORTHWEST: rooms[from]->northwest = to; break;
        case DIR_EAST: rooms[from]->east = to; break;
        case DIR_WEST: rooms[from]->west = to; break;
        case DIR_SOUTH: rooms[from]->south = to; break;
        case DIR_SOUTHEAST: rooms[from]->southeast = to; break;
        case DIR_SOUTWEST: rooms[from]->southwest = to; break;
        case DIR_UP: rooms[from]->up = to; break;
        case DIR_DOWN: rooms[from]->down = to; break;
        case DIR_IN: rooms[from]->in = to; break;
        case DIR_OUT: rooms[from]->out = to; break;
        default: return false;
    }
}

void TMap::init( Host * pH )
{
    qDebug()<<"TMap::init() host="<<mpHost->getName();
    areas.clear();
    int s_areas = 0;
    QMap<int,int> s_area_exits;
    buildAreas();
    QMapIterator<int, TArea *> it( areas );
    while( it.hasNext() )
    {
        it.next();
        int id = it.key();
        // area mit raeumen fuellen
        QMapIterator<int, TRoom *> it2( rooms );
        while( it2.hasNext() )
        {
            it2.next();
            int id2 = it2.key();
            if( rooms[id2]->area == id )
            {
                areas[id]->rooms.push_back(id2);
            }
        }
        areas[id]->ausgaengeBestimmen();
        s_areas++;
        s_area_exits[areas[id]->exits.size()]++;
        it.value()->calcSpan();
    }
    qDebug()<<"statistics: areas:"<<s_areas;
    qDebug()<<"area exit stats:" <<s_area_exits;

    //pM->raise();
}

void TMap::buildAreas()
{
    QMapIterator<int, TRoom *> it( rooms );
    while( it.hasNext() )
    {
        it.next();
        int id = it.key();
        if( ! areas.contains( rooms[id]->area ) )
        {
            areas[rooms[id]->area] = new TArea( this );
        }
    }
}

void TMap::setView(float x, float y, float z, float zoom )
{
    if( rooms.contains(mRoomId) )
    {
        rooms[mRoomId]->xRot = x;
        rooms[mRoomId]->yRot = y;
        rooms[mRoomId]->zRot = z;
        rooms[mRoomId]->zoom = zoom;
    }
}

bool TMap::gotoRoom( int r )
{
    mTargetID = r;
    return findPath( mRoomId, r );
}

bool TMap::gotoRoom( int r1, int r2 )
{
    //mTargetID = r2;
    return findPath( r1, r2 );
}

bool TMap::findPath( int from, int to )
{
     typedef adjacency_list<listS, vecS, undirectedS, no_property, property<edge_weight_t, cost> > mygraph_t;
     typedef property_map<mygraph_t, edge_weight_t>::type WeightMap;
     typedef mygraph_t::vertex_descriptor vertex;
     typedef mygraph_t::edge_descriptor edge_descriptor;
     typedef mygraph_t::vertex_iterator vertex_iterator;
     typedef std::pair<int, int> edge;

     std::vector<location> locations;
     mygraph_t g(rooms.size()*10);
     WeightMap weightmap = get(edge_weight, g);
     QMapIterator<int, TRoom *> it( rooms );
     int roomCount=0;
     int edgeCount=0;
     while( it.hasNext() )
     {
         it.next();
         int i = it.key();
         if( ! rooms.contains( i ) || rooms[i]->isLocked )
         {
             qDebug()<<"skipping room ID:"<<i;
             continue;
         }
         roomCount++;
         location l;
         l.x = rooms[i]->x;
         l.y = rooms[i]->y;
         l.z = rooms[i]->z;
         locations.push_back( l );
         if( rooms[i]->north != -1 && rooms.contains( rooms[i]->north ) && !rooms[rooms[i]->north]->isLocked )
         {
             if( rooms[rooms[i]->north]->hasExit(i) )
             {
                 edgeCount++;
                 edge_descriptor e;
                 bool inserted;
                 tie(e, inserted) = add_edge( i,
                                              rooms[i]->north,
                                              g );
                 weightmap[e] = rooms[rooms[i]->north]->weight;
             }
         }
         if( rooms[i]->south != -1 && rooms.contains( rooms[i]->south ) && !rooms[rooms[i]->south]->isLocked )
         {
             if( rooms[rooms[i]->south]->hasExit(i) )
             {
                 edgeCount++;
                 edge_descriptor e;
                 bool inserted;
                 tie(e, inserted) = add_edge( i,
                                              rooms[i]->south,
                                              g );
                 weightmap[e] = rooms[rooms[i]->south]->weight;
             }
         }
         if( rooms[i]->northeast != -1 && rooms.contains( rooms[i]->northeast ) && !rooms[rooms[i]->northeast]->isLocked )
         {
             if( rooms[rooms[i]->northeast]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->northeast,
                                             g );
                weightmap[e] = rooms[rooms[i]->northeast]->weight;
            }
         }
         if( rooms[i]->east != -1 && rooms.contains( rooms[i]->east ) && !rooms[rooms[i]->east]->isLocked )
         {
             if( rooms[rooms[i]->east]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->east,
                                             g );
                weightmap[e] = rooms[rooms[i]->east]->weight;
            }
         }
         if( rooms[i]->west != -1 && rooms.contains( rooms[i]->west ) && !rooms[rooms[i]->west]->isLocked )
         {
             if( rooms[rooms[i]->west]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->west,
                                             g );
                weightmap[e] = rooms[rooms[i]->west]->weight;
            }
         }
         if( rooms[i]->southwest != -1 && rooms.contains( rooms[i]->southwest ) && !rooms[rooms[i]->southwest]->isLocked )
         {
             if( rooms[rooms[i]->southwest]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->southwest,
                                             g );
                weightmap[e] = rooms[rooms[i]->southwest]->weight;
            }
         }
         if( rooms[i]->southeast != -1 && rooms.contains( rooms[i]->southeast ) && !rooms[rooms[i]->southeast]->isLocked )
         {
             if( rooms[rooms[i]->southeast]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->southeast,
                                             g );
                weightmap[e] = rooms[rooms[i]->southeast]->weight;
            }
         }
         if( rooms[i]->northwest != -1 && rooms.contains( rooms[i]->northwest ) && !rooms[rooms[i]->northwest]->isLocked )
         {
             if( rooms[rooms[i]->northwest]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->northwest,
                                             g );
                weightmap[e] = rooms[rooms[i]->northwest]->weight;
            }
         }
         if( rooms[i]->up != -1 && rooms.contains( rooms[i]->up ) && !rooms[rooms[i]->up]->isLocked )
         {
             if( rooms[rooms[i]->up]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->up,
                                             g );
                weightmap[e] = rooms[rooms[i]->up]->weight;
            }
         }
         if( rooms[i]->down != -1 && rooms.contains( rooms[i]->down ) && !rooms[rooms[i]->down]->isLocked )
         {
             if( rooms[rooms[i]->down]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->down,
                                             g );
                weightmap[e] = rooms[rooms[i]->down]->weight;
            }
         }
         if( rooms[i]->in != -1 && rooms.contains( rooms[i]->in ) && !rooms[rooms[i]->in]->isLocked )
         {
             if( rooms[rooms[i]->in]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->in,
                                             g );
                weightmap[e] = rooms[rooms[i]->in]->weight;
            }
         }
         if( rooms[i]->out != -1 && rooms.contains( rooms[i]->out ) && !rooms[rooms[i]->out]->isLocked )
         {
             if( rooms[rooms[i]->out]->hasExit(i) )
             {
                edgeCount++;
                edge_descriptor e;
                bool inserted;
                tie(e, inserted) = add_edge( i,
                                             rooms[i]->out,
                                             g );
                weightmap[e] = rooms[rooms[i]->out]->weight;
            }
         }
     }
     cout << "TOTAL: initialized rooms:"<<roomCount<<" edges:"<<edgeCount<<endl;
     vertex start = from;//mRoomId;
     vertex goal = to;//mTargetID;
     cout<<"SEARCHING PATH from:"<<start<<" to:"<<goal<<endl;
     if( ! rooms.contains( start ) || ! rooms.contains( goal ) )
     {
         cout << "ERROR: start or tartget room not in map!" << endl;
         return false;
     }
     cout << "start vertex: " << start << endl;
     cout << "target vertex: " << goal << endl;

     vector<mygraph_t::vertex_descriptor> p(num_vertices(g));
     vector<cost> d(num_vertices(g));
     try
     {
         astar_search( g,
                       start,
                       distance_heuristic<mygraph_t, cost, std::vector<location> >(locations, goal),
                       predecessor_map(&p[0]).distance_map(&d[0]).
                       visitor(astar_goal_visitor<vertex>(goal)) );
     }
     catch( found_goal fg )
     {
         list<vertex> shortest_path;
         for(vertex v = goal; ; v = p[v])
         {
             cout << "assembling path: v="<<v<<endl;
             if( ! rooms.contains( v ) )
             {
                 cout<<"ERROR path assembly: path room not in map!"<<endl;
                 return false;
             }
             shortest_path.push_front(v);
             if(p[v] == v) break;
         }
         cout << "Shortest path from " << rooms[start]->id << " to "
              << rooms[goal]->id << ": ";
         list<vertex>::iterator spi = shortest_path.begin();
         cout << rooms[start]->id;
         mPathList.clear();
         mDirList.clear();
         int curRoom = start;
         for( ++spi; spi != shortest_path.end(); ++spi )
         {
             if( ! rooms.contains( curRoom ) )
             {
                 cout << "ERROR: path not possible. curRoom not in map!" << endl;
                 return false;
             }
             cout << " -> " << *spi;
             mPathList.push_back( *spi );
             if( rooms[curRoom]->north == rooms[*spi]->id )
             {
                 mDirList.push_back("n");
             }
             else if( rooms[curRoom]->northeast == rooms[*spi]->id )
             {
                 mDirList.push_back("ne");
             }
             else if( rooms[curRoom]->northwest == rooms[*spi]->id )
             {
                 mDirList.push_back("nw");
             }
             else if( rooms[curRoom]->southeast == rooms[*spi]->id )
             {
                 mDirList.push_back("se");
             }
             else if( rooms[curRoom]->southwest == rooms[*spi]->id )
             {
                 mDirList.push_back("sw");
             }
             else if( rooms[curRoom]->south == rooms[*spi]->id )
             {
                 mDirList.push_back("s");
             }
             else if( rooms[curRoom]->east == rooms[*spi]->id )
             {
                 mDirList.push_back("e");
             }
             else if( rooms[curRoom]->west == rooms[*spi]->id )
             {
                 mDirList.push_back("w");
             }
             else if( rooms[curRoom]->up == rooms[*spi]->id )
             {
                 mDirList.push_back("up");
             }
             else if( rooms[curRoom]->down == rooms[*spi]->id )
             {
                 mDirList.push_back("down");
             }
             else if( rooms[curRoom]->in == rooms[*spi]->id )
             {
                 mDirList.push_back("in");
             }else if( rooms[curRoom]->out == rooms[*spi]->id )
             {
                 mDirList.push_back("out");
             }

             curRoom = *spi;

         }
         cout << endl << "PATH FOUND: Total travel time: " << d[goal] << endl;
         return true;
     }
     if( rooms.contains(start) && rooms.contains(goal))
     {
        cout << "Didn't find a path from " << rooms[start]->id << "to"
             << rooms[goal]->id << " ROOM NOT IN MAP!" << endl;
     }
     else
     {
         qDebug()<<"PATH NOT FOUND && start or target not in map!";
     }
     return false;
}

bool TMap::serialize( QDataStream & ofs )
{
    int version = 5;
    ofs << version;
    ofs << envColors;
    ofs << areaNamesMap;
    ofs << customEnvColors;
    QMapIterator<int, TRoom *> it( rooms );
    while( it.hasNext() )
    {

        it.next();
        int i = it.key();
        qDebug()<<"SAVING: roomID="<<rooms[i]->id;
        ofs <<  rooms[i]->id;
        ofs << rooms[i]->area;
        ofs << rooms[i]->x;
        ofs << rooms[i]->y;
        ofs << rooms[i]->z;
        ofs << rooms[i]->north;
        ofs << rooms[i]->northeast;
        ofs << rooms[i]->east;
        ofs << rooms[i]->southeast;
        ofs << rooms[i]->south;
        ofs << rooms[i]->southwest;
        ofs << rooms[i]->west;
        ofs << rooms[i]->northwest;
        ofs << rooms[i]->up;
        ofs << rooms[i]->down;
        ofs << rooms[i]->in;
        ofs << rooms[i]->out;
        ofs << rooms[i]->environment;
        ofs << rooms[i]->weight;
        ofs << rooms[i]->xRot;
        ofs << rooms[i]->yRot;
        ofs << rooms[i]->zRot;
        ofs << rooms[i]->zoom;
        ofs << rooms[i]->name;
        ofs << rooms[i]->isLocked;
    }

    return true;
}

#include <QDir>

bool TMap::restore()
{
    qDebug()<<"RESTORING MAP";
    QString folder = QDir::homePath()+"/.config/mudlet/profiles/"+mpHost->getName()+"/map/";
    QDir dir( folder );
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList( QDir::Files, QDir::Time );
    for( int i=0;i<entries.size(); i++ )
        qDebug()<<i<<"#"<<entries[i];
    bool canRestore = true;
    if( entries.size() > 0 )
    {
        QFile file(folder+entries[0]);
        file.open( QFile::ReadOnly );
        qDebug()<<"[LOADING MAP]:"<<file.fileName();
        QDataStream ifs( & file );
        int version;
        ifs >> version;
        if( version >= 3 )
        {
            ifs >> envColors;
        }
        else
        {
            canRestore = false;
        }
        if( version >= 4 )
        {
            ifs >> areaNamesMap;
        }
        else
        {
            canRestore = false;
        }
        if( version >= 5 )
        {
            ifs >> customEnvColors;
        }
        while( ! ifs.atEnd() )
        {
            int i;
            ifs >> i;
            qDebug()<<"loading room ID:"<<i;
            TRoom * pT = new TRoom;
            rooms[i] = pT;
            rooms[i]->id = i;
            ifs >> rooms[i]->area;
            ifs >> rooms[i]->x;
            ifs >> rooms[i]->y;
            ifs >> rooms[i]->z;
            ifs >> rooms[i]->north;
            ifs >> rooms[i]->northeast;
            ifs >> rooms[i]->east;
            ifs >> rooms[i]->southeast;
            ifs >> rooms[i]->south;
            ifs >> rooms[i]->southwest;
            ifs >> rooms[i]->west;
            ifs >> rooms[i]->northwest;
            ifs >> rooms[i]->up;
            ifs >> rooms[i]->down;
            ifs >> rooms[i]->in;
            ifs >> rooms[i]->out;
            ifs >> rooms[i]->environment;
            ifs >> rooms[i]->weight;
            ifs >> rooms[i]->xRot;
            ifs >> rooms[i]->yRot;
            ifs >> rooms[i]->zRot;
            ifs >> rooms[i]->zoom;
            ifs >> rooms[i]->name;
            ifs >> rooms[i]->isLocked;
        }


        qDebug()<<"LOADED rooms:"<<rooms.size();
        if( canRestore )
        {
            return true;
        }
    }
    if( ! canRestore || entries.size() == 0 )
    {
        QMessageBox msgBox;

        if( mpHost->mUrl == "achaea.com"
            || mpHost->mUrl == "aetolia.com"
            || mpHost->mUrl == "imperian.com"
            || mpHost->mUrl == "midkemiaonline.com"
            || mpHost->mUrl == "lusternia.com" )
        {
            msgBox.setText("No map found. Going to download the map ...");
            msgBox.exec();
            init( mpHost );
            qDebug()<<"--trace before map download";
            mpMapper->downloadMap();
            qDebug()<<"--trace after map download";
        }
        else
        {
            mpHost->mpMap->init( mpHost );
            //msgBox.setText("Sorry, this early version of the mapper can only be used on IRE games\n.This will change in the near future.");
            //msgBox.exec();
        }
    }
}





