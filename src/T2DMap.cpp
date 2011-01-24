
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

#include <QColorDialog>

#include "T2DMap.h"
#include "TMap.h"
#include "TArea.h"
#include "TRoom.h"
#include "Host.h"
#include "TConsole.h"
#include <QPixmap>

T2DMap::T2DMap()
{
    xzoom = 30;
    yzoom = 30;
    mPick = false;
    mTarget = 0;
    mRoomSelection = 0;
    mStartSpeedWalk = false;
    mRoomBeingMoved = false;
    mPHighlightMove = QPoint(width()/2, height()/2);
    mNewMoveAction = false;

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
    mRoomBeingMoved = false;
    mPHighlightMove = QPoint(width()/2, height()/2);
    mNewMoveAction = false;
    mFontHeight = 20;
    mShowRoomID = false;
    QFont f =QFont( QFont("Bitstream Vera Sans Mono", 20, QFont::Courier ) );//( QFont("Monospace", 10, QFont::Courier) );
    f.setPointSize(25);
    f.setBold(true);
    for( int i=33;i<255;i++ )
    {
        QPixmap b(26,26);
        b.fill(QColor(0,0,0,0));
        QPainter p(&b);
        p.setPen(QColor(0,0,0,255));
        p.setFont(f);
        QRect r(0,0,26,26);
        p.drawText(r, Qt::AlignHCenter|Qt::AlignVCenter, QChar(i) );
        mPixMap[i]=b;
    }


}

QColor T2DMap::getColor( int id )
{
    QColor c;

    if( ! mpMap->rooms.contains(id) ) return c;

    int env = mpMap->rooms[id]->environment;
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
    case 16:
        c = mpHost->mLightBlack_2;
    default: //user defined room color
        if( ! mpMap->customEnvColors.contains(env) ) break;
        c = mpMap->customEnvColors[env];
    }
    return c;
}

void T2DMap::paintEvent( QPaintEvent * e )
{
    QTime __time; __time.start();

    const QRect & rect = e->rect();

    QPainter p( this );
    if( ! p.isActive() ) return;

    mAreaExitList.clear();

    float _w = width();
    float _h = height();

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


   // p.setRenderHint(QPainter::NonCosmeticDefaultPen);

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

    TArea * pArea = mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area];
    if( ! pArea ) return;

    int zEbene;
    zEbene = mpMap->rooms[mpMap->mRoomId]->z;
    if( ! mpMap->rooms.contains(mpMap->mRoomId) ) return;
    if( ! mpMap ) return;


    p.fillRect(0,0,_w,_h,mpHost->mBgColor_2);


    p.setPen(QPen( mpHost->mFgColor_2) );
 if( ! pArea->gridMode )
 {
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
                QPen __pen = p.pen();
                if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->south ) && mpMap->rooms[pArea->rooms[i]]->south == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x(), p2.y()+ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x(), p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->north ) && mpMap->rooms[pArea->rooms[i]]->north == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x(), p2.y()-ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x(), p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->west ) && mpMap->rooms[pArea->rooms[i]]->west == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x()-tx, p2.y(),p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y());
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->east ) && mpMap->rooms[pArea->rooms[i]]->east == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x()+tx, p2.y(),p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y());
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->northwest ) && mpMap->rooms[pArea->rooms[i]]->northwest == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x()-tx, p2.y()-ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->northeast ) && mpMap->rooms[pArea->rooms[i]]->northeast == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x()+tx, p2.y()-ty,p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y()-ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->southeast ) && mpMap->rooms[pArea->rooms[i]]->southeast == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x()+tx, p2.y()+ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()+tx, p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                else if( mpMap->rooms.contains( mpMap->rooms[pArea->rooms[i]]->southwest ) && mpMap->rooms[pArea->rooms[i]]->southwest == exitList[k] )
                {
                    p.setPen(getColor(exitList[k]));
                    p.drawLine( p2.x()-tx, p2.y()+ty, p2.x(), p2.y() );
                    QPoint _p = QPoint(p2.x()-tx, p2.y()+ty);
                    mAreaExitList[exitList[k]] = _p;
                }
                p.setPen( __pen );
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
        case 16:
            c = mpHost->mLightBlack_2;
        default: //user defined room color
            if( ! mpMap->customEnvColors.contains(env) ) break;
            c = mpMap->customEnvColors[env];
        }
        if( mPick && mPHighlight.x() >= dr.x()-tx/2 && mPHighlight.x() <= dr.x()+tx/2 && mPHighlight.y() >= dr.y()-ty/2 && mPHighlight.y() <= dr.y()+ty/2
            || mMultiSelectionList.contains(pArea->rooms[i]) )
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
        {
            p.fillRect(dr,c);
            if( mShowRoomID )
            {
                QPen __pen = p.pen();
                QColor lc;
                if( c.red()+c.green()+c.blue() > 200 )
                    lc=QColor(0,0,0);
                else
                    lc=QColor(255,255,255);
                p.setPen(QPen(lc));
                p.drawText(dr, Qt::AlignHCenter|Qt::AlignVCenter,QString::number(pArea->rooms[i]));
                p.setPen(__pen);
            }
            char _ch = mpMap->rooms[pArea->rooms[i]]->c;
            if( _ch >= 33 && _ch < 255 )
            {
                QPixmap pix = mPixMap[_ch].scaled(dr.width(), dr.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                p.drawPixmap(dr.topLeft(), pix);
            }
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
        if( pArea->gridMode )
        {
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

    QColor infoCol = mpHost->mBgColor_2;
    QColor _infoCol;
    if( infoCol.red()+infoCol.green()+infoCol.blue() > 200 )
        _infoCol=QColor(0,0,0);
    else
        _infoCol=QColor(255,255,255);

    p.setPen(_infoCol);

    p.fillRect( 0,0,width(), 5*mFontHeight, QColor(150,150,150,80) );
    QString text;
    text = QString("Area: %1").arg(mpMap->areaNamesMap[mpMap->rooms[mpMap->mRoomId]->area]);
    p.drawText( 10, mFontHeight, text );
    text = QString("Room Name: %1").arg(mpMap->rooms[mpMap->mRoomId]->name);
    p.drawText( 10, 2*mFontHeight, text );
    text = QString("Room ID: %1 Position on Map: (%2/%3/%3)").arg(QString::number(mpMap->mRoomId)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->x)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->y)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->z));
    p.drawText( 10, 3*mFontHeight, text );

    if( mMapInfoRect == QRect(0,0,0,0) ) mMapInfoRect = QRect(0,0,width(),height()/10);


    if( mpHost->mMapStrongHighlight )
    {
        QRectF dr = QRectF(px-(tx*0.75)/2,py-(ty*0.75)/2,tx*0.75,ty*0.75);
        p.fillRect(dr,QColor(255,0,0,150));

        float _radius = (1.9*tx)/2;
        QPointF _center = QPointF(px,py);
        QRadialGradient _gradient(_center,_radius);
        _gradient.setColorAt(0.95, QColor(255,0,0,150));
        _gradient.setColorAt(0.80, QColor(150,100,100,150));
        _gradient.setColorAt(0.799,QColor(150,100,100,100));
        _gradient.setColorAt(0.7, QColor(255,0,0,200));
        _gradient.setColorAt(0, QColor(255,255,255,255));
        QPen myPen(QColor(0,0,0,0));
        QPainterPath myPath;
        p.setBrush(_gradient);
        p.setPen(myPen);
        myPath.addEllipse(_center,_radius,_radius);
        p.drawPath(myPath);
    }
    else
    {
        QPen _pen = p.pen();
        float _radius = (1.9*tx)/2;
        QPointF _center = QPointF(px,py);
        QRadialGradient _gradient(_center,_radius);
        _gradient.setColorAt(0.95, QColor(255,0,0,150));
        _gradient.setColorAt(0.80, QColor(150,100,100,150));
        _gradient.setColorAt(0.799,QColor(150,100,100,100));
        _gradient.setColorAt(0.3,QColor(150,150,150,100));
        _gradient.setColorAt(0.1, QColor(255,255,255,100));
        _gradient.setColorAt(0, QColor(255,255,255,255));
        QPen myPen(QColor(0,0,0,0));
        QPainterPath myPath;
        p.setBrush(_gradient);
        p.setPen(myPen);
        myPath.addEllipse(_center,_radius,_radius);
        p.drawPath(myPath);
    }
    p.setPen(QColor(0,255,0,90));
    p.fillRect(mMultiRect,QColor(190,190,190,60));

    text = QString("Frames per second:%1").arg(QString::number(__time.elapsed()));
    p.setPen(QColor(255,255,255));
    p.drawText( 10, 4*mFontHeight, text );
    qDebug()<<"render time:"<<__time.elapsed();
}

void T2DMap::mouseDoubleClickEvent ( QMouseEvent * event )
{
    int x = event->x();
    int y = event->y();
    mPHighlight = QPoint(x,y);
    mPick = true;
    mStartSpeedWalk = true;
    repaint();
}

void T2DMap::mouseReleaseEvent(QMouseEvent * e )
{
    if( e->buttons() & Qt::LeftButton )
    {
        mMultiSelection = false;
        mMultiRect = QRect(0,0,0,0);
        update();
    }
}

void T2DMap::mousePressEvent(QMouseEvent *event)
{
    mNewMoveAction = true;
    if( event->buttons() & Qt::LeftButton )
    {
        if( mRoomBeingMoved )
        {
            setMouseTracking(false);
            mRoomBeingMoved = false;
        }
        else if( ! mPopupMenu )
        {
            mMultiSelection = true;
            mMultiRect = QRect(event->pos(), event->pos());
        }
        else
            mPopupMenu = false;
    }
    if( event->buttons() & Qt::RightButton )
    {
        int x = event->x();
        int y = event->y();
        mPHighlight = QPoint(x,y);
        mPick = true;
        //repaint();

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

        mPopupMenu = true;
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
    update();
}

void T2DMap::slot_moveRoom()
{
    mRoomBeingMoved = true;
    setMouseTracking(true);

}

void T2DMap::slot_deleteRoom()
{
    if( mMultiSelection )
    {
        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                mpHost->mpMap->deleteRoom( mMultiSelectionList[j] );
            }
        }
    }
    else if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
    {
        mpHost->mpMap->deleteRoom( mRoomSelection );
    }
    repaint();
}

void T2DMap::slot_selectRoomColor(QListWidgetItem * pI )
{
    mChosenRoomColor = pI->text().toInt();
}

void T2DMap::slot_defineNewColor()
{
    QColor color = QColorDialog::getColor( mpHost->mRed, this );
    if ( color.isValid() )
    {
        mpMap->customEnvColors[mpMap->customEnvColors.size()+257+16] = color;
        slot_changeColor();
    }
    repaint();
}

void T2DMap::slot_changeColor()
{
    mChosenRoomColor = 5;
    QDialog * pD = new QDialog(this);
    QVBoxLayout * pL = new QVBoxLayout;
    pD->setLayout( pL );
    pD->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    pD->setContentsMargins(0,0,0,0);
    QListWidget * pLW = new QListWidget(pD);
    pLW->setViewMode(QListView::IconMode);

    connect(pLW, SIGNAL(itemDoubleClicked(QListWidgetItem*)), pD, SLOT(accept()));
    connect(pLW, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slot_selectRoomColor(QListWidgetItem*)));

    pL->addWidget(pLW);
    QWidget * pButtonBar = new QWidget(pD);

    QHBoxLayout * pL2 = new QHBoxLayout;
    pButtonBar->setLayout( pL2 );
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    QPushButton * pB_newColor = new QPushButton(pButtonBar);
    pB_newColor->setText("define new color");

    connect(pB_newColor, SIGNAL(clicked()), pD, SLOT(reject()));
    connect(pB_newColor, SIGNAL(clicked()), this, SLOT(slot_defineNewColor()));

    pL2->addWidget(pB_newColor);

    QPushButton * pB_ok = new QPushButton(pButtonBar);
    pB_ok->setText("ok");
    pL2->addWidget(pB_ok);
    connect(pB_ok, SIGNAL(clicked()), pD, SLOT(accept()));

    QPushButton * pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText("abort");
    connect(pB_abort, SIGNAL(clicked()), pD, SLOT(reject()));
    pL2->addWidget(pB_abort);
    pL->addWidget(pButtonBar);

    QMapIterator<int, QColor> it(mpMap->customEnvColors);
    while( it.hasNext() )
    {
        it.next();
        int env = it.key();
        QColor c;
        c = it.value();
        QListWidgetItem * pI = new QListWidgetItem( pLW );
        QPixmap pix = QPixmap(50,50);
        pix.fill( c );
        QIcon mi( pix );
        pI->setIcon(mi);
        pI->setText(QString::number(it.key()));
        pLW->addItem(pI);
    }

    pD->exec();

    if( mpHost->mpMap->rooms.contains(mRoomSelection) )
    {
        if( mpMap->customEnvColors.contains( mChosenRoomColor) )
        {
            mpHost->mpMap->rooms[mRoomSelection]->environment = mChosenRoomColor;
        }
    }
    update();
}

void T2DMap::slot_addSpecialExit()
{

}

#include "dlgRoomExits.h"
void T2DMap::slot_setExits()
{
//    if( mMultiSelection ) return;

    if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
    {
        dlgRoomExits * pD = new dlgRoomExits( mpHost, this );
        pD->init( mRoomSelection );
        pD->show();
        pD->raise();
    }
}


void T2DMap::slot_setUserData()
{

}

void T2DMap::slot_lockRoom()
{
    if( mMultiSelection )
    {
        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                mpMap->rooms[mMultiSelectionList[j]]->isLocked = true;
            }
        }
    }
    else if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
    {
        mpHost->mpMap->rooms[mRoomSelection]->isLocked = true;
    }
}

#include <QInputDialog>
void T2DMap::slot_setRoomWeight()
{


    if( mMultiSelection )
    {
        int w = QInputDialog::getInt(this,"Enter a room weight (= travel time)","room weight:", 1);
        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                mpMap->rooms[mMultiSelectionList[j]]->weight = w;
            }
        }
        repaint();
    }
    else
    {
        if( mpHost->mpMap->rooms.contains(mRoomSelection) )
        {
            int _w = mpHost->mpMap->rooms[mRoomSelection]->weight;
            int w = QInputDialog::getInt(this, "Enter a room weight (= travel time)","room weight:",_w);
            mpHost->mpMap->rooms[mRoomSelection]->weight = w;
        }
    }
}



void T2DMap::mouseMoveEvent( QMouseEvent * event )
{
    if( mMultiSelection && ! mRoomBeingMoved )
    {
        if( mNewMoveAction )
        {
            mMultiRect = QRect(event->pos(), event->pos());
            mNewMoveAction = false;
        }
        else
            mMultiRect.setBottomLeft( event->pos() );

        int _roomID = mpMap->mRoomId;
        if( ! mpMap->rooms.contains( _roomID ) ) return;
        int _areaID = mpMap->rooms[_roomID ]->area;
        if( ! mpMap->areas.contains(_areaID) ) return;
        TArea * pArea = mpMap->areas[_areaID];
        int ox = mpMap->rooms[mpMap->mRoomId]->x;
        int oy = mpMap->rooms[mpMap->mRoomId]->y*-1;
        float _rx;
        float _ry;
        if( ox*mTX > xspan/2*mTX )
            _rx = -(mTX*ox-xspan/2*mTX);
        else
            _rx = xspan/2*mTX-mTX*ox;
        if( oy*mTY > yspan/2*mTY )
            _ry = -(mTY*oy-yspan/2*mTY);
        else
            _ry = yspan/2*mTY-mTY*oy;

        QList<int> roomList = pArea->rooms;
        mMultiSelectionList.clear();
        for( int k=0; k<roomList.size(); k++ )
        {
            int rx = mpMap->rooms[pArea->rooms[k]]->x*mTX+_rx;
            int ry = mpMap->rooms[pArea->rooms[k]]->y*-1*mTY+_ry;
            int rz = mpMap->rooms[pArea->rooms[k]]->z;

            if( rz != mpMap->rooms[pArea->rooms[k]]->z ) continue;

            QRectF dr;
            if( pArea->gridMode )
            {
                dr = QRectF(rx-mTX/2, ry-mTY/2,mTX,mTY);
            }
            else
            {
                dr = QRectF(rx-(mTX*0.75)/2,ry-(mTY*0.75)/2,mTX*0.75,mTY*0.75);
            }
            if( mMultiRect.contains(dr) )
            {
                mMultiSelectionList << pArea->rooms[k];
            }
        }
        int mx = event->pos().x()/mTX;
        int my = event->pos().y()/mTY;
        mx = mx - xspan/2 + 1;
        my = yspan/2 - my - 1;
        if( mpMap->rooms.contains( mpMap->mRoomId ) )
        {
            mx += mpMap->rooms[mpMap->mRoomId]->x;
            my += mpMap->rooms[mpMap->mRoomId]->y;
            mOldMousePos = QPoint(mx,my);
        }

        update();
    }
    if( mRoomBeingMoved )
    {
        if( ! mMultiSelection )
        {
            QPoint P = event->pos();
            mPHighlightMove = event->pos();
            if( mpMap->rooms.contains( mRoomSelection ) )
            {
                int mx = mPHighlightMove.x()/mTX;
                int my = mPHighlightMove.y()/mTY;
                mx = mx - xspan/2 + 1;
                my = yspan/2 - my - 1;
                mx += mpMap->rooms[mpMap->mRoomId]->x;
                my += mpMap->rooms[mpMap->mRoomId]->y;
                mMoveTarget = QPoint( mx, my );

                mpMap->rooms[mRoomSelection]->x = mMoveTarget.x();
                mpMap->rooms[mRoomSelection]->y = mMoveTarget.y();
            }
            repaint();
        }
        else
        {
            mMultiRect = QRect(0,0,0,0);
            int _roomID = mpMap->mRoomId;
            if( ! mpMap->rooms.contains( _roomID ) ) return;
            int _areaID = mpMap->rooms[_roomID ]->area;
            if( ! mpMap->areas.contains(_areaID) ) return;
            TArea * pArea = mpMap->areas[_areaID];
            int ox = mpMap->rooms[mpMap->mRoomId]->x;
            int oy = mpMap->rooms[mpMap->mRoomId]->y*-1;
            float _rx;
            float _ry;
            if( ox*mTX > xspan/2*mTX )
                _rx = -(mTX*ox-xspan/2*mTX);
            else
                _rx = xspan/2*mTX-mTX*ox;
            if( oy*mTY > yspan/2*mTY )
                _ry = -(mTY*oy-yspan/2*mTY);
            else
                _ry = yspan/2*mTY-mTY*oy;
            int mx = event->pos().x()/mTX;
            int my = event->pos().y()/mTY;
            mx = mx - xspan/2 + 1;
            my = yspan/2 - my - 1;
            mx += mpMap->rooms[mpMap->mRoomId]->x;
            my += mpMap->rooms[mpMap->mRoomId]->y;
            QPoint delta = QPoint(mx,my)-mOldMousePos;
            mOldMousePos = QPoint(mx,my);
            for( int j=0; j<mMultiSelectionList.size(); j++ )
            {
                if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
                {
                    mpMap->rooms[mMultiSelectionList[j]]->x+=delta.x();
                    mpMap->rooms[mMultiSelectionList[j]]->y+=delta.y();
                }
            }
            repaint();
        }
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

