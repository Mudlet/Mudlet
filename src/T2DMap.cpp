
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
    xzoom = 30;
    yzoom = 30;
    mPick = false;
    mTarget = 0;
    mRoomSelection = 0;
    mStartSpeedWalk = false;
}

T2DMap::T2DMap(QWidget * parent)
: QWidget(parent)
{
    xzoom = 30;
    yzoom = 30;
    mPick = false;
    mTarget = 0;
    mRoomSelection = 0;
    mStartSpeedWalk = false;
}

void T2DMap::paintEvent( QPaintEvent * e )
{
    const QRect & rect = e->rect();

    QPainter p( this );
    if( ! p.isActive() ) return;

    mAreaExitList.clear();

    float _w = rect.width();
    float _h = rect.height();
    int xspan;
    int yspan;
    if( _w < 10 || _h < 10 ) return;

    if( _w > _h )
    {
        xspan = xzoom*(_w/_h);
        yspan = xzoom;
    }
    else
    {
        xspan = yzoom;
        yspan = yzoom*(_h/_w);
    }


    float tx = _w/xspan;
    float ty = _h/yspan;

    mTX = tx;
    mTY = ty;

    p.setRenderHint(QPainter::Antialiasing);

    int px,py,pz;
    if( ! mpMap->rooms.contains( mpMap->mRoomId ) )
    {
        qDebug()<<"ERROR: roomID not in rooms map";
        return;
    }

    if( ! mpMap->areas.contains( mpMap->rooms[mpMap->mRoomId]->area) ) return;

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

    if( tx != 0 && ty != 0 && mPHighlightMove.x() != 0 && mPHighlightMove.y() != 0 )
    {
        int mx = mPHighlightMove.x()/tx;
        int my = mPHighlightMove.y()/ty;
        mx = mx - xspan/2+mpMap->rooms[mpMap->mRoomId]->x;
        my = (my - yspan/2+mpMap->rooms[mpMap->mRoomId]->y)*-1;
        mMoveTarget = QPoint( mx, my );
        if( mRoomBeingMoved )
        {
            QRectF dr = QRectF(mx*tx+_rx-tx/2, my*-1*ty+_ry-ty/2,tx,ty);
            p.fillRect(dr,QColor(255,155,80));
        }
    }

    p.setBackground( mpHost->mBgColor_2 );


    TArea * pArea = mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area];
    if( ! pArea ) return;

    int zEbene;
    zEbene = mpMap->rooms[mpMap->mRoomId]->z;
    if( ! mpMap->rooms.contains(mpMap->mRoomId) ) return;
    if( ! mpMap ) return;

    p.setPen(QPen( mpHost->mFgColor_2) );
    p.fillRect(0,0,_w,_h,mpHost->mBgColor_2);
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
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->northeast );

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
                if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->south ) && mpMap->rooms[pArea->rooms[i]]->south == exitList[k] )
                {
                    p.drawLine( p2.x(), p2.y()+ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x(), p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->north ) && mpMap->rooms[pArea->rooms[i]]->north == exitList[k] )
                {
                    p.drawLine( p2.x(), p2.y()-ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x(), p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->west ) && mpMap->rooms[pArea->rooms[i]]->west == exitList[k] )
                {
                    p.drawLine( p2.x()-tx, p2.y(),p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y());
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->east ) && mpMap->rooms[pArea->rooms[i]]->east == exitList[k] )
                {
                    p.drawLine( p2.x()+tx, p2.y(),p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y());
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->northwest ) && mpMap->rooms[pArea->rooms[i]]->northwest == exitList[k] )
                {
                    p.drawLine( p2.x()-tx, p2.y()-ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->northeast ) && mpMap->rooms[pArea->rooms[i]]->northeast == exitList[k] )
                {
                    p.drawLine( p2.x()+tx, p2.y()-ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->southeast ) && mpMap->rooms[pArea->rooms[i]]->southeast == exitList[k] )
                {
                    p.drawLine( p2.x()+tx, p2.y()+ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->southwest ) && mpMap->rooms[pArea->rooms[i]]->southwest == exitList[k] )
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
        if( ! mpMap->rooms.contains( pArea->rooms[i] ) )
        {
            qDebug()<<"ERROR: mapper 2d-draw: area room not in map roomID="<<pArea->rooms[i]    ;
            continue;
        }
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
                c = mpHost->mRed_2;
                break;

            case 2:
                c = mpHost->mGreen_2;
                break;
            case 3:
                c = mpHost->mYellow_2;
                break;

            case 4:
                c = mpHost->mBlue_2;
                break;

            case 5:
                c = mpHost->mMagenta_2;
                break;
            case 6:
                c = mpHost->mCyan_2;
                break;
            case 7:
                c = mpHost->mWhite_2;
                break;
            case 8:
                c = mpHost->mBlack_2;
                break;

            case 9:
                c = mpHost->mLightRed_2;
                break;

            case 10:
                c = mpHost->mLightGreen_2;
                break;
            case 11:
                c = mpHost->mLightYellow_2;
                break;

            case 12:
                c = mpHost->mLightBlue_2;
                break;

            case 13:
                c = mpHost->mLightMagenta_2;
                break;
            case 14:
                c = mpHost->mLightCyan_2;
                break;
            case 15:
                c = mpHost->mLightWhite_2;
                break;
            default: //user defined room color
                if( ! mpMap->customEnvColors.contains(env) ) break;
                c = mpMap->customEnvColors[env];
            }
            if( mPick && mPHighlight.x() >= dr.x()-tx/2 && mPHighlight.x() <= dr.x()+tx/2 && mPHighlight.y() >= dr.y()-ty/2 && mPHighlight.y() <= dr.y()+ty/2 )
            {
                p.fillRect(dr,QColor(255,155,0));
                mPick = false;
                if( mStartSpeedWalk )
                {
                    mStartSpeedWalk = false;
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
                {
                    mRoomSelection = pArea->rooms[i];


                }
            }
            else
                p.fillRect(dr,c);
        }

        QColor lc;
        if( c.red()+c.green()+c.blue() > 200 )
            lc=QColor(0,0,0);
        else
            lc=QColor(255,255,255);
        p.setPen(QPen(lc));
        if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->up ) && mpMap->rooms[pArea->rooms[i]]->up > 0 )
        {
            p.drawLine(rx-(tx*0.75)/5, ry+(ty*0.75)/4, rx, ry+(ty*0.75)/7);
            p.drawLine(rx,ry+(ty*0.75)/7,rx+(tx*0.75)/5, ry+(ty*0.75)/4);
        }
        if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->down ) && mpMap->rooms[pArea->rooms[i]]->down > 0 )
        {
            p.drawLine(rx-(tx*0.75)/5, ry-(ty*0.75)/4, rx, ry-(ty*0.75)/7);
            p.drawLine(rx,ry-(ty*0.75)/7,rx+(tx*0.75)/5, ry-(ty*0.75)/4);
        }
        if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->in ) && mpMap->rooms[pArea->rooms[i]]->in > 0 )
        {
            p.drawLine(rx-(tx*0.75)/5, ry-(ty*0.75)/4, rx-(tx*0.75)/7, ry);
            p.drawLine(rx-(tx*0.75)/7,ry,rx-(tx*0.75)/5, ry+(ty*0.75)/4);
        }
        if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->out ) && mpMap->rooms[pArea->rooms[i]]->out > 0 )
        {
            p.drawLine(rx+(tx*0.75)/5, ry-(ty*0.75)/4, rx+(tx*0.75)/7, ry);
            p.drawLine(rx+(tx*0.75)/7,ry,rx+(tx*0.75)/5, ry+(ty*0.75)/4);
        }
        p.setPen(QColor(0,0,0));

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
    if( event->buttons() & Qt::LeftButton )
    {
        if( mRoomBeingMoved )
        {
            setMouseTracking(false);
            mRoomBeingMoved = false;
        }
        else
        {
            int x = event->x();
            int y = event->y();
            mPHighlight = QPoint(x,y);
            mPick = true;
            mStartSpeedWalk = true;
            update();
        }

    }
    if( event->buttons() & Qt::RightButton )
    {
        int x = event->x();
        int y = event->y();
        mPHighlight = QPoint(x,y);

        mPick = true;
        repaint();

        QAction * action = new QAction("move room", this );
        action->setStatusTip(tr("move room"));
        connect( action, SIGNAL(triggered()), this, SLOT(slot_moveRoom()));
        QAction * action2 = new QAction("delete room", this );
        action2->setStatusTip(tr("delete room"));
        connect( action2, SIGNAL(triggered()), this, SLOT(slot_deleteRoom()));
        QAction * action3 = new QAction("change room color", this );
        action3->setStatusTip(tr("change room color"));
        connect( action3, SIGNAL(triggered()), this, SLOT(slot_changeColor()));
        QAction * action4 = new QAction("add special exit", this );
        action4->setStatusTip(tr("add special exit"));
        connect( action4, SIGNAL(triggered()), this, SLOT(slot_addSpecialExit()));
        QAction * action5 = new QAction("set user data", this );
        action5->setStatusTip(tr("set user data"));
        connect( action5, SIGNAL(triggered()), this, SLOT(slot_setUserData()));
        QAction * action6 = new QAction("lock room", this );
        action6->setStatusTip(tr("lock room for speed walks"));
        connect( action6, SIGNAL(triggered()), this, SLOT(slot_lockRoom()));
        QAction * action7 = new QAction("set room weight", this );
        action7->setStatusTip(tr("set room weight"));
        connect( action7, SIGNAL(triggered()), this, SLOT(slot_setRoomWeight()));
        QAction * action8 = new QAction("set regular exits", this );
        action8->setStatusTip(tr("set regular exits"));
        connect( action8, SIGNAL(triggered()), this, SLOT(slot_setExits()));

        QMenu * popup = new QMenu( this );

        popup->addAction( action );
        popup->addAction( action3 );
        popup->addAction( action8 );
        popup->addAction( action4 );
        popup->addAction( action5 );
        popup->addAction( action6 );
        popup->addAction( action7 );
        popup->addAction( action2 );
        popup->popup( mapToGlobal( event->pos() ) );
    }
}

void T2DMap::slot_moveRoom()
{
    mRoomBeingMoved = true;
    setMouseTracking(true);
}

void T2DMap::slot_deleteRoom()
{
    if( mpHost->mpMap->rooms.contains(mRoomSelection) )
        mpHost->mpMap->deleteRoom( mRoomSelection );
}

void T2DMap::slot_changeColor()
{


}

void T2DMap::slot_addSpecialExit()
{

}

void T2DMap::slot_setExits()
{

}


void T2DMap::slot_setUserData()
{

}

void T2DMap::slot_lockRoom()
{
    if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
        mpHost->mpMap->rooms[mRoomSelection]->isLocked = true;
}

void T2DMap::slot_setRoomWeight()
{
    if( mpHost->mpMap->rooms.contains(mRoomSelection) )
    {
        //mpHost->mpMap->rooms[mRoomSelection]->weight = w;
    }
}



void T2DMap::mouseMoveEvent( QMouseEvent * event )
{
    if( mRoomBeingMoved )
    {
        QPoint P = event->pos() - mPHighlightMove;
        mPHighlightMove = event->pos();
        if( mpMap->rooms.contains( mRoomSelection ) )
        {
            mpMap->rooms[mRoomSelection]->x = mMoveTarget.x();
            mpMap->rooms[mRoomSelection]->y = mMoveTarget.y();
        }
        update();
    }
}

void T2DMap::wheelEvent ( QWheelEvent * e )
{
    int delta = e->delta() / 8 / 15;
    if( e->delta() < 0 )
    {
        mPick = false;
        xzoom += delta;
        yzoom += delta;
        if( yzoom < 3 || xzoom < 3 )
        {
            xzoom = 3;
            yzoom = 3;
        }
        update();
        e->accept();
        update();
        return;
    }
    if( e->delta() > 0 )
    {
        mPick = false;
        xzoom += delta;
        yzoom += delta;

        e->accept();
        update();
        return;
    }
    e->ignore();
    return;
}

