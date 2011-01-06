
/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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

#include "T2DMap.h"
#include "TMap.h"
#include "TArea.h"
#include "TRoom.h"
#include "Host.h"
#include "TConsole.h"

T2DMap::T2DMap()
{
    xspan = 20;
    yspan = 20;
    mPick = false;
    mTarget = 0;
}

T2DMap::T2DMap(QWidget * parent)
: QWidget(parent)
{
    xspan = 20;
    yspan = 20;
    mPick = false;
    mTarget = 0;
}

void T2DMap::paintEvent( QPaintEvent * e )
{

    const QRect & rect = e->rect();

    QPainter p( this );
    if( ! p.isActive() ) return;

    mAreaExitList.clear();

    int _w = rect.width();
    int _h = rect.height();
    if( _w < 10 || _h < 10 ) return;
    int tx = _w/xspan;
    int ty = _h/yspan;
    if( tx > ty )
        tx = ty;
    if( ty > tx )
        ty = tx;

    p.setRenderHint(QPainter::Antialiasing);

    int px,py,pz;
    if( ! mpMap->rooms.contains( mpMap->mRoomId ) )
    {
        qDebug()<<"ERROR: roomID not in rooms map";
        return;
    }

    int ox = mpMap->rooms[mpMap->mRoomId]->x;
    int oy = mpMap->rooms[mpMap->mRoomId]->y*-1;

    if( ox*tx > xspan/2*tx )
        _rx = -(tx*ox-xspan/2*tx);
    else
        _rx = xspan/2*tx-tx*ox;
    if( oy*ty > yspan/2*ty )
        _ry = -(ty*oy-yspan/2*ty);
    else
        _ry = yspan/2*ty-ty*oy;

    px = ox*tx+_rx;
    py = oy*ty+_ry;

    TArea * pArea = mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area];
    if( ! pArea ) return;

    int zEbene;
    zEbene = mpMap->rooms[mpMap->mRoomId]->z;
    if( ! mpMap->rooms.contains(mpMap->mRoomId) ) return;
    if( ! mpMap ) return;

    for( int i=0; i<pArea->rooms.size(); i++ )
    {
        int rx = mpMap->rooms[pArea->rooms[i]]->x*tx+_rx;
        int ry = mpMap->rooms[pArea->rooms[i]]->y*-1*ty+_ry;
        int rz = mpMap->rooms[pArea->rooms[i]]->z;

        if( rz != zEbene ) continue;

        QList<int> exitList;
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->north );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->northwest );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->east );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->southeast );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->south );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->southwest );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->west );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->northwest );

        int e = mpMap->rooms[pArea->rooms[i]]->z;
        for( int k=0; k<exitList.size(); k++ )
        {
            bool areaExit = false;
            if( exitList[k] == -1 ) continue;
            if( ! mpMap->rooms.contains( exitList[k] ) )
            {
                continue;
            }
            if( mpMap->rooms[exitList[k]]->area != mpMap->rooms[mpMap->mRoomId]->area )
            {
                areaExit = true;
            }
            int ex = mpMap->rooms[exitList[k]]->x*tx+_rx;
            int ey = mpMap->rooms[exitList[k]]->y*ty*-1+_ry;
            int ez = mpMap->rooms[exitList[k]]->z;
            if( ez != zEbene ) continue;
            QVector3D p1( ex, ey, ez );
            QVector3D p2( rx, ry, rz );
            if( ! areaExit )
            {
                p.drawLine( p1.x(), p1.y(), p2.x(), p2.y() );
            }
            else
            {
                if( mpMap->rooms[pArea->rooms[i]]->south == exitList[k] )
                {
                    p.drawLine( p2.x(), p2.y()+ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x(), p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms[pArea->rooms[i]]->north == exitList[k] )
                {
                    p.drawLine( p2.x(), p2.y()-ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x(), p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms[pArea->rooms[i]]->west == exitList[k] )
                {
                    p.drawLine( p2.x()-tx, p2.y(),p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y());
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms[pArea->rooms[i]]->east == exitList[k] )
                {
                    p.drawLine( p2.x()+tx, p2.y(),p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y());
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms[pArea->rooms[i]]->northwest == exitList[k] )
                {
                    p.drawLine( p2.x()-tx, p2.y()-ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms[pArea->rooms[i]]->northeast == exitList[k] )
                {
                    p.drawLine( p2.x()+tx, p2.y()-ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms[pArea->rooms[i]]->southeast == exitList[k] )
                {
                    p.drawLine( p2.x()+tx, p2.y()+ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms[pArea->rooms[i]]->southwest == exitList[k] )
                {
                    p.drawLine( p2.x()-tx, p2.y()+ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
            }
        }
    }

    for( int i=0; i<pArea->rooms.size(); i++ )
    {
        int rx = mpMap->rooms[pArea->rooms[i]]->x*tx+_rx;
        int ry = mpMap->rooms[pArea->rooms[i]]->y*-1*ty+_ry;
        int rz = mpMap->rooms[pArea->rooms[i]]->z;

        if( rz != zEbene ) continue;

        QRectF dr;
        if( pArea->gridMode )
        {
            dr = QRectF(rx-tx/2, ry-ty/2,tx,ty);
        }
        else
        {
            dr = QRectF(rx-(tx*0.75)/2,ry-(ty*0.75)/2,tx*0.75,ty*0.75);
        }

        QColor c;

        if( rx == px && ry == py )
        {
            p.fillRect(dr,QColor(255,0,0));
        }
        else
        {

            int env = mpMap->rooms[pArea->rooms[i]]->environment;
            if( mpMap->envColors.contains(env) )
                env = mpMap->envColors[env];
            else
            {
                if( ! mpMap->customEnvColors.contains(env))
                {
                    env = 1;
                }
            }
            switch( env )
            {
            case 1:
                c = QColor(128, 0, 0);
                break;

            case 2:
                c = QColor(0, 128, 0);
                break;
            case 3:
                c = QColor(128,128,0);
                break;

            case 4:
                c = QColor( 0,0,128);
                break;

            case 5:
                c = QColor( 128,128,0);
                break;
            case 6:
                c = QColor( 0,128,128 );
                break;
            case 7:
                c = QColor( 128, 128, 128 );
                break;
            case 8:
                c = QColor( 55,55,55 );
                break;

            case 9:
                c = QColor( 255,0,0 );
                break;

            case 10:
                c = QColor( 0,255,0);
                break;
            case 11:
                c = QColor( 255,255,0);
                break;

            case 12:
                c = QColor( 0,0,255);
                break;

            case 13:
                c = QColor( 255,0,255);
                break;
            case 14:
                c = QColor( 0,255,255 );
                break;
            case 15:
                c = QColor( 255,255,255);
                break;
            default: //user defined room color
                if( ! mpMap->customEnvColors.contains(env) ) break;
                c = mpMap->customEnvColors[env];
            }
            if( mPick && mPHighlight.x() >= dr.x()-tx/2 && mPHighlight.x() <= dr.x()+tx/2 && mPHighlight.y() >= dr.y()-ty/2 && mPHighlight.y() <= dr.y()+ty/2 )
            {
                p.fillRect(dr,QColor(255,155,0));
                mPick = false;
                mTarget = pArea->rooms[i];
                if( mpMap->rooms.contains(mTarget) )
                {
                    mpMap->mTargetID = mTarget;
                    if( mpMap->findPath( mpMap->mRoomId, mpMap->mTargetID) )
                    {
                       qDebug()<<"T2DMap: starting speedwalk path length="<<mpMap->mPathList.size();
                       mpMap->mpHost->startSpeedWalk();
                    }
                    else
                    {
                        QString msg = "Mapper: Cannot find a path to this room using known exits.\n";
                        mpHost->mpConsole->printSystemMessage(msg);
                    }
                }
            }
            else
                p.fillRect(dr,c);
        }

        QColor lc;
        if( c.red()+c.green()+c.blue() > 300 )
            lc=QColor(0,0,0);
        else
            lc=QColor(255,255,255);
        p.setPen(QPen(lc));
        if( mpMap->rooms[pArea->rooms[i]]->up > 0 )
        {
            p.drawLine(rx-(tx*0.75)/5, ry+(ty*0.75)/4, rx, ry+(ty*0.75)/7);
            p.drawLine(rx,ry+(ty*0.75)/7,rx+(tx*0.75)/5, ry+(ty*0.75)/4);
        }
        if( mpMap->rooms[pArea->rooms[i]]->down > 0 )
        {
            p.drawLine(rx-(tx*0.75)/5, ry-(ty*0.75)/4, rx, ry-(ty*0.75)/7);
            p.drawLine(rx,ry-(ty*0.75)/7,rx+(tx*0.75)/5, ry-(ty*0.75)/4);
        }
        if( mpMap->rooms[pArea->rooms[i]]->in > 0 )
        {
            p.drawLine(rx-(tx*0.75)/5, ry-(ty*0.75)/4, rx-(tx*0.75)/7, ry);
            p.drawLine(rx-(tx*0.75)/7,ry,rx-(tx*0.75)/5, ry+(ty*0.75)/4);
        }
        if( mpMap->rooms[pArea->rooms[i]]->out > 0 )
        {
            p.drawLine(rx+(tx*0.75)/5, ry-(ty*0.75)/4, rx+(tx*0.75)/7, ry);
            p.drawLine(rx+(tx*0.75)/7,ry,rx+(tx*0.75)/5, ry+(ty*0.75)/4);
        }
        p.setPen(QColor(0,0,0));

        qDebug()<<"areaExits:"<<mAreaExitList;
        QMapIterator<int, QPoint> it( mAreaExitList );
        while( it.hasNext() )
        {
            it.next();
            QPoint P = it.value();
            int rx = P.x();
            int ry = P.y();


            QRectF dr;
            if( pArea->gridMode )
            {
                dr = QRectF(rx-tx/2, ry-ty/2,tx,ty);
            }
            else
            {
                dr = QRectF(rx-(tx*0.75)/2,ry-(ty*0.75)/2,tx*0.75,ty*0.75);
            }
            if( mPick && mPHighlight.x() >= dr.x()-tx/2 && mPHighlight.x() <= dr.x()+tx/2 && mPHighlight.y() >= dr.y()-ty/2 && mPHighlight.y() <= dr.y()+ty/2 )
            {
                p.fillRect(dr,QColor(255,155,0));
                mPick = false;
                mTarget = it.key();
                if( mpMap->rooms.contains(mTarget) )
                {
                    mpMap->mTargetID = mTarget;
                    if( mpMap->findPath( mpMap->mRoomId, mpMap->mTargetID) )
                    {
                       qDebug()<<"T2DMap: starting speedwalk path length="<<mpMap->mPathList.size();
                       mpMap->mpHost->startSpeedWalk();
                    }
                    else
                    {
                        QString msg = "Mapper: Cannot find a path to this room using known exits.\n";
                        mpHost->mpConsole->printSystemMessage(msg);
                    }
                }
            }
        }
    }
}

void T2DMap::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int x = event->x();
        int y = event->y();
        mPHighlight = QPoint(x,y);
        mPick = true;
        qDebug()<<"PICK: click auf:"<<mPHighlight;
        update();

    }
}

//void T2DMap::mouseMoveEvent( QMouseEvent * event )
//{
//}

void T2DMap::wheelEvent ( QWheelEvent * e )
{
    //int delta = e->delta() / 8 / 15;
    if( e->delta() < 0 )
    {
        mPick = false;
        xspan--;
        yspan--;
        if( yspan < 3 | xspan < 3 )
        {
            xspan = 3;
            yspan = 3;
        }
        update();
        e->accept();
        update();
        return;
    }
    if( e->delta() > 0 )
    {
        mPick = false;
        xspan++;
        yspan++;

        e->accept();
        update();
        return;
    }
    e->ignore();
    return;
}

