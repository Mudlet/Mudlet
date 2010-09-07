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



#ifndef TMAP_H
#define TMAP_H

class TRoom;
class TArea;
class Host;
class GLWidget;

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
    int  createNewRoomID();
    bool setExit( int from, int to, int dir );
    bool setRoomCoordinates( int id, int x, int y, int z );
    void init(Host*);
    void buildAreas();
    void setRoom( int );
    bool findPath( int from, int to );
    bool gotoRoom( int );
    bool gotoRoom( int, int );
    void setView( float, float, float, float );
    bool serialize( QDataStream & );
    bool restore();
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

};




#endif // TMAP_H


