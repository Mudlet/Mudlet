
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
#include <QInputDialog>

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
    gzoom = 20;
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
    gzoom = 20;
    mPick = false;
    mTarget = 0;
    mRoomSelection = 0;
    mStartSpeedWalk = false;
    mRoomBeingMoved = false;
    mPHighlightMove = QPoint(width()/2, height()/2);
    mNewMoveAction = false;
    mFontHeight = 20;
    mShowRoomID = false;



}

void T2DMap::init()
{
    if( ! mpMap ) return;
    mPixMap.clear();
    mGridPix.clear();
    QFont f = QFont( QFont("Bitstream Vera Sans Mono", 20, QFont::Courier ) );//( QFont("Monospace", 10, QFont::Courier) );
    f.setPointSize(gzoom);
    f.setBold(true);
    int j=0;
    for( int k=1; k<17; k++ )
    {
        for( int i=1;i<255;i++ )
        {
            j++;
            QPixmap b(gzoom,gzoom);
            b.fill(QColor(0,0,0,0));
            QPainter p(&b);
            QColor c = _getColor(k);
            p.setPen(c);
            p.setFont(f);
            QRect r(0,0,gzoom,gzoom);
            p.drawText(r, Qt::AlignHCenter|Qt::AlignVCenter, QChar(i) );
            mPixMap[j]=b;
        }
    }
    // bilder aller grid areas erzeugen
    QList<int> kL = mpMap->areas.keys();
    for( int i=0; i<kL.size(); i++ )
    {
        if( ! mpMap->areas[kL[i]]->gridMode ) continue;

        TArea * pA = mpMap->areas[kL[i]];

        int x_min, x_max, y_min, y_max;
        TRoom * pK = mpMap->rooms[pA->rooms[0]];
        y_max = pK->y;
        y_min = pK->y;
        x_max = pK->x;
        x_min = pK->x;
        for( int k=0; k<pA->rooms.size(); k++ )
        {
            TRoom * pR = mpMap->rooms[pA->rooms[k]];
            if( pR->x <= x_min ) x_min = pR->x;
            if( pR->y <= y_min ) y_min = pR->y;
            if( pR->x >= x_max ) x_max = pR->x;
            if( pR->y >= y_max ) y_max = pR->y;
        }
        pA->max_x = x_max;
        pA->min_x = x_min;
        pA->min_y = y_min;
        pA->max_y = y_max;
        QPixmap * pix = new QPixmap((abs(x_min)+abs(x_max))*gzoom, (abs(y_min)+abs(y_max))*gzoom);
        QPainter p( pix );
        p.fillRect( 0, 0, pix->width(), pix->height(), mpHost->mBgColor_2 );

        for( int k=0; k<pA->rooms.size(); k++ )
        {
            TRoom * pR = mpMap->rooms[pA->rooms[k]];
            char _ch = pR->c;
            QRectF dr = QRectF((pR->x+abs(x_min))*gzoom,(pR->y*-1+abs(y_min))*gzoom,gzoom,gzoom);
            QColor c = mpMap->customEnvColors[pR->environment];
            if( _ch >= 33 && _ch < 255 )
            {
                int _color = (pR->environment - 257 ) * 254 + _ch;
                if( mPixMap.contains( _color ) )
                {
                    QPixmap _pix = mPixMap[_color].scaled(dr.width(), dr.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    p.drawPixmap(dr.topLeft(), _pix);
                }
                else
                    p.fillRect(dr,c);
            }
            else
                p.fillRect(dr,c);
        }
        if( mpMap->areas[kL[i]]->rooms.size() > 0 )
        {
            mGridPix[mpMap->rooms[mpMap->areas[kL[i]]->rooms[0]]->area] = pix;
        }
    }
}

QColor T2DMap::_getColor( int id )
{
    QColor c;

    switch( id )
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

    }
    return c;
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
    bool __Pick = mPick;
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

    if( ! mpMap ) return;
    if( ! mpMap->rooms.contains(mpMap->mRoomId) ) return;

    p.fillRect(0,0,2000,2000,mpHost->mBgColor_2);

    if( pArea->gridMode )
    {
        int areaID = mpMap->rooms[mpMap->mRoomId]->area;
        QRectF rect = QRectF(0,0,width(),height());

        if( mGridPix.contains( mpMap->rooms[mpMap->mRoomId]->area ) )
        {
            int _x = mpMap->rooms[mpMap->mRoomId]->x;
            int _y = mpMap->rooms[mpMap->mRoomId]->y*-1;
            int vx = (abs(pArea->min_x)+_x-(width()/gzoom)/2)*gzoom;
            int vy = (abs(pArea->min_y)+_y-(height()/gzoom)/2)*gzoom;

            int _b = width();
            int _h = height();
            QRect _re = QRect( vx, vy, width(), height() );
            int offx = 0;
            int offy = 0;
            if( vx < 0 )
                offx = abs(vx);
            if( vy < 0 )
                offy = abs(vy);
            if( vx < 0 ) vx = 0;
            if( vy < 0 ) vy = 0;
            p.drawPixmap(offx,offy, *mGridPix[areaID],vx,vy,width(),height());//.scaled(width(),height(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
            QColor _infoCol = mpHost->mBgColor_2;
            if( _infoCol.red()+_infoCol.green()+_infoCol.blue() > 200 )
                _infoCol=QColor(0,0,0);
            else
                _infoCol=QColor(255,255,255);
            p.setPen(_infoCol);
            float _radius = gzoom;
            float _px = (width()/gzoom/2)*gzoom+gzoom/2;
            float _py = (height()/gzoom/2)*gzoom+gzoom/2;
            QString text = QString("Area: %1 ID:%2").arg(mpMap->areaNamesMap[mpMap->rooms[mpMap->mRoomId]->area]).arg(mpMap->rooms[mpMap->mRoomId]->area);
            p.drawText( 10, mFontHeight, text );
            text = QString("Room Name: %1").arg(mpMap->rooms[mpMap->mRoomId]->name);
            p.drawText( 10, 2*mFontHeight, text );
            text = QString("Room ID: %1 Position on Map: (%2/%3/%4)").arg(QString::number(mpMap->mRoomId)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->x)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->y)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->z));
            p.drawText( 10, 3*mFontHeight, text );
            p.fillRect(mMultiRect,QColor(190,190,190,60));
            text = QString("render time:%1ms").arg(QString::number(__time.elapsed()));
            p.setPen(QColor(255,255,255));
            p.drawText( 10, 4*mFontHeight, text );
            QPointF _center = QPointF( _px, _py );
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
            return;
        }
    }




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
        if( ( mPick || __Pick ) && mPHighlight.x() >= dr.x()-tx/2 && mPHighlight.x() <= dr.x()+tx/2 && mPHighlight.y() >= dr.y()-ty/2 && mPHighlight.y() <= dr.y()+ty/2
            || mMultiSelectionList.contains(pArea->rooms[i]) )
        {
            p.fillRect(dr,QColor(55,255,55));
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
            char _ch = mpMap->rooms[pArea->rooms[i]]->c;
            if( _ch >= 33 && _ch < 255 )
            {
                int _color = (mpMap->rooms[pArea->rooms[i]]->environment - 257 ) * 254 + _ch;
                p.fillRect(dr,QColor(90,90,90));
                if( mPixMap.contains( _color ) )
                {
                    QPixmap pix = mPixMap[_color].scaled(dr.width(), dr.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    p.drawPixmap(dr.topLeft(), pix);
                }
            }
            else
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
               if( (mPick || __Pick) && mPHighlight.x() >= dr.x()-tx/2 && mPHighlight.x() <= dr.x()+tx/2 && mPHighlight.y() >= dr.y()-ty/2 && mPHighlight.y() <= dr.y()+ty/2
                   || mMultiSelectionList.contains(pArea->rooms[i]) )
               {
                   p.fillRect(dr,QColor(50,255,50));
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

    if( mpMap->rooms.contains( mRoomSelection ) && __Pick )
    {
        text = QString("Area: %1 ID:%2 x:%3-%4 y:%5-%6").arg(mpMap->areaNamesMap[mpMap->rooms[mRoomSelection]->area]).arg(mpMap->rooms[mRoomSelection]->area).arg(mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area]->min_x).arg(mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area]->max_x).arg(mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area]->min_y).arg(mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area]->max_y);
        p.drawText( 10, mFontHeight, text );
        text = QString("Room Name: %1").arg(mpMap->rooms[mRoomSelection]->name);
        p.drawText( 10, 2*mFontHeight, text );
        text = QString("Room ID: %1 Position on Map: (%2/%3/%4)").arg(QString::number(mpMap->mRoomId)).arg(QString::number(mpMap->rooms[mRoomSelection]->x)).arg(QString::number(mpMap->rooms[mRoomSelection]->y)).arg(QString::number(mpMap->rooms[mRoomSelection]->z));
        p.drawText( 10, 3*mFontHeight, text );
    }
    else
    {
        text = QString("Area: %1 ID:%2").arg(mpMap->areaNamesMap[mpMap->rooms[mpMap->mRoomId]->area]).arg(mpMap->rooms[mpMap->mRoomId]->area);
        p.drawText( 10, mFontHeight, text );
        text = QString("Room Name: %1").arg(mpMap->rooms[mpMap->mRoomId]->name);
        p.drawText( 10, 2*mFontHeight, text );
        text = QString("Room ID: %1 Position on Map: (%2/%3/%4)").arg(QString::number(mpMap->mRoomId)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->x)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->y)).arg(QString::number(mpMap->rooms[mpMap->mRoomId]->z));
        p.drawText( 10, 3*mFontHeight, text );
    }

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

    text = QString("render time:%1ms").arg(QString::number(__time.elapsed()));
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
            mPHighlightMove = event->pos();
            mPick = true;

            setMouseTracking(false);
            mRoomBeingMoved = false;
        }
        else if( ! mPopupMenu )
        {
            mMultiSelection = true;
            mMultiRect = QRect(event->pos(), event->pos());
            mPHighlight = event->pos();
            mPick = true;
            update();
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

        QAction * action = new QAction("move", this );
        action->setStatusTip(tr("move room"));
        connect( action, SIGNAL(triggered()), this, SLOT(slot_moveRoom()));
        QAction * action2 = new QAction("delete", this );
        action2->setStatusTip(tr("delete room"));
        connect( action2, SIGNAL(triggered()), this, SLOT(slot_deleteRoom()));
        QAction * action3 = new QAction("color", this );
        action3->setStatusTip(tr("change room color"));
        connect( action3, SIGNAL(triggered()), this, SLOT(slot_changeColor()));
        QAction * action4 = new QAction("spread", this );
        action4->setStatusTip(tr("increase map grid size for the selected group of rooms"));
        connect( action4, SIGNAL(triggered()), this, SLOT(slot_spread()));
        QAction * action9 = new QAction("shrink", this );
        action9->setStatusTip(tr("shrink map grid size for the selected group of rooms"));
        connect( action9, SIGNAL(triggered()), this, SLOT(slot_shrink()));

        QAction * action5 = new QAction("user data", this );
        action5->setStatusTip(tr("set user data"));
        connect( action5, SIGNAL(triggered()), this, SLOT(slot_setUserData()));
        QAction * action6 = new QAction("lock", this );
        action6->setStatusTip(tr("lock room for speed walks"));
        connect( action6, SIGNAL(triggered()), this, SLOT(slot_lockRoom()));
        QAction * action7 = new QAction("weight", this );
        action7->setStatusTip(tr("set room weight"));
        connect( action7, SIGNAL(triggered()), this, SLOT(slot_setRoomWeight()));
        QAction * action8 = new QAction("exits", this );
        action8->setStatusTip(tr("set room exits"));
        connect( action8, SIGNAL(triggered()), this, SLOT(slot_setExits()));
        QAction * action10 = new QAction("letter", this );
        action10->setStatusTip(tr("set a letter to mark special rooms"));
        connect( action10, SIGNAL(triggered()), this, SLOT(slot_setCharacter()));
        QAction * action11 = new QAction("image", this );
        action11->setStatusTip(tr("set an image to mark special rooms"));
        connect( action11, SIGNAL(triggered()), this, SLOT(slot_setImage()));

        QAction * action12 = new QAction("move to", this );
        action12->setStatusTip(tr("move selected group to a given position"));
        connect( action12, SIGNAL(triggered()), this, SLOT(slot_movePosition()));

        QAction * action13 = new QAction("area", this );
        action13->setStatusTip(tr("set room area ID"));
        connect( action13, SIGNAL(triggered()), this, SLOT(slot_setArea()));

        mPopupMenu = true;
        QMenu * popup = new QMenu( this );

        popup->addAction( action );
        popup->addAction( action12 );
        popup->addAction( action8 );
        popup->addAction( action3 );
        popup->addAction( action10 );
        popup->addAction( action11 );
        popup->addAction( action4 );
        popup->addAction( action5 );
        popup->addAction( action6 );
        popup->addAction( action7 );
        popup->addAction( action2 );

        popup->addAction( action9 );
        popup->addAction( action13 );

        popup->popup( mapToGlobal( event->pos() ) );
    }
    update();
}

void T2DMap::slot_movePosition()
{
    QDialog * pD = new QDialog(this);
    QVBoxLayout * pL = new QVBoxLayout;
    pD->setLayout( pL );
    pD->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    pD->setContentsMargins(0,0,0,0);
    QLineEdit * pLEx = new QLineEdit(pD);
    QLineEdit * pLEy = new QLineEdit(pD);
    QLineEdit * pLEz = new QLineEdit(pD);

//    connect(pLW, SIGNAL(itemDoubleClicked(QListWidgetItem*)), pD, SLOT(accept()));
//    connect(pLW, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slot_selectRoomColor(QListWidgetItem*)));

    if( mMultiSelection )
    {
        int tLC = getTopLeftSelection();
        if( tLC < 0 ) return;
        mRoomSelection = mMultiSelectionList[tLC];
    }

    if( ! mpHost->mpMap->rooms.contains( mRoomSelection ) ) return;

    pLEx->setText(QString::number(mpHost->mpMap->rooms[mRoomSelection]->x));
    pLEy->setText(QString::number(mpHost->mpMap->rooms[mRoomSelection]->y));
    pLEz->setText(QString::number(mpHost->mpMap->rooms[mRoomSelection]->z));
    QLabel * pLa1 = new QLabel("x coordinate");
    QLabel * pLa2 = new QLabel("y coordinate");
    QLabel * pLa3 = new QLabel("z coordinate");
    pL->addWidget(pLa1);
    pL->addWidget(pLEx);
    pL->addWidget(pLa2);
    pL->addWidget(pLEy);
    pL->addWidget(pLa3);
    pL->addWidget(pLEz);

    QWidget * pButtonBar = new QWidget(pD);

    QHBoxLayout * pL2 = new QHBoxLayout;
    pButtonBar->setLayout( pL2 );
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    QPushButton * pB_ok = new QPushButton(pButtonBar);
    pB_ok->setText("Ok");
    pL2->addWidget(pB_ok);
    connect(pB_ok, SIGNAL(clicked()), pD, SLOT(accept()));

    QPushButton * pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText("Cancel");
    connect(pB_abort, SIGNAL(clicked()), pD, SLOT(reject()));
    pL2->addWidget(pB_abort);
    pL->addWidget(pButtonBar);

    pD->exec();
    int x,y,z;
    x = pLEx->text().toInt();
    y = pLEy->text().toInt();
    z = pLEz->text().toInt();

    if( mMultiSelection )
    {
        if( mMultiSelectionList.size() < 1 ) return;
        int topLeftCorner = getTopLeftSelection();
        if( topLeftCorner < 0 ) return;

        if( ! mpMap->rooms.contains( mMultiSelectionList[topLeftCorner] ) ) return;

        int dx,dy;

        dx = x - mpMap->rooms[mMultiSelectionList[topLeftCorner]]->x;
        dy = y - mpMap->rooms[mMultiSelectionList[topLeftCorner]]->y;
        int dz = z - mpMap->rooms[mMultiSelectionList[topLeftCorner]]->z;

        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                mpMap->rooms[mMultiSelectionList[j]]->x+=dx;
                mpMap->rooms[mMultiSelectionList[j]]->y+=dy;
                mpMap->rooms[mMultiSelectionList[j]]->z+=dz;
            }
        }
    }
    else if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
    {
        mpHost->mpMap->rooms[mRoomSelection]->x = x;
        mpHost->mpMap->rooms[mRoomSelection]->y = y;
        mpHost->mpMap->rooms[mRoomSelection]->z = z;
    }
    repaint();
}


void T2DMap::slot_moveRoom()
{
    mRoomBeingMoved = true;
    setMouseTracking(true);
    mNewMoveAction = true;

}


void T2DMap::slot_setCharacter()
{
    if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
    {
        QString s = QInputDialog::getText(this,"enter marker letter","letter");
        if( s.size() < 1 ) return;
        mpHost->mpMap->rooms[mRoomSelection]->c = s[0].toAscii();
        repaint();
    }
}

void T2DMap::slot_setImage()
{

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
    if( mMultiSelection )
    {
        if( mMultiSelectionList.size() < 1 )
        {
            mMultiSelectionList.push_back( mRoomSelection );
        }

        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                if( mpMap->customEnvColors.contains( mChosenRoomColor) )
                {
                    mpHost->mpMap->rooms[mMultiSelectionList[j]]->environment = mChosenRoomColor;
                }
            }
        }

        if( mMultiSelectionList.size() == 1 )
            if( mMultiSelectionList[0] == mRoomSelection )
                mMultiSelectionList.clear();

    }
    else if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
    {
        if( mpMap->customEnvColors.contains( mChosenRoomColor) )
        {
            mpHost->mpMap->rooms[mRoomSelection]->environment = mChosenRoomColor;
        }
    }
    update();
}

void T2DMap::slot_spread()
{
    int _spread = QInputDialog::getInt(this, "set grid","grid:",5);
    if( mMultiSelection )
    {
        mMultiRect = QRect(0,0,0,0);
        if( mMultiSelectionList.size() < 1 ) return;
        int _x = mpMap->rooms[mMultiSelectionList[0]]->x;
        int _y = mpMap->rooms[mMultiSelectionList[0]]->y;
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            TRoom * pR = mpHost->mpMap->rooms[mMultiSelectionList[j]];
            pR->x *= _spread;
            pR->y *= _spread;
        }
    }
}

void T2DMap::slot_shrink()
{
    int _spread = QInputDialog::getInt(this, "shrink grid",":",3);
    if( mMultiSelection )
    {
        mMultiRect = QRect(0,0,0,0);
        if( mMultiSelectionList.size() < 1 ) return;
        int _x = mpMap->rooms[mMultiSelectionList[0]]->x;
        int _y = mpMap->rooms[mMultiSelectionList[0]]->y;
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            TRoom * pR = mpHost->mpMap->rooms[mMultiSelectionList[j]];
            pR->x /= _spread;
            pR->y /= _spread;
        }
    }
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

void T2DMap::slot_setArea()
{
    if( mMultiSelection )
    {
        int w = QInputDialog::getInt(this,"Enter the area ID:", "area ID:", 1);
        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                mpMap->rooms[mMultiSelectionList[j]]->area = w;
            }
        }
        repaint();
    }
    else
    {
        if( mpHost->mpMap->rooms.contains(mRoomSelection) )
        {
            int _w = mpHost->mpMap->rooms[mRoomSelection]->area;
            int w = QInputDialog::getInt(this, "Enter area ID:","area ID:",_w);
            mpHost->mpMap->rooms[mRoomSelection]->area = w;
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
            int dx,dy;

            if( mMultiSelectionList.size() < 1 )
            {
                mMultiSelectionList.push_back( mRoomSelection );
            }
            int topLeftCorner = getTopLeftSelection();
            if( topLeftCorner < 0 ) return;
            dx = mx - mpMap->rooms[mMultiSelectionList[topLeftCorner]]->x;
            dy = my - mpMap->rooms[mMultiSelectionList[topLeftCorner]]->y;
            for( int j=0; j<mMultiSelectionList.size(); j++ )
            {
                if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
                {
                    mpMap->rooms[mMultiSelectionList[j]]->x += dx;
                    mpMap->rooms[mMultiSelectionList[j]]->y += dy;
                }
            }
            if( mMultiSelectionList.size() == 1 )
                if( mMultiSelectionList[0] == mRoomSelection )
                    mMultiSelectionList.clear();
            repaint();
        }
    }
}

// return -1 on error
int T2DMap::getTopLeftSelection()
{
    int min_x, min_y, id;
    if( mMultiSelectionList.size() < 1 ) return -1;
    min_x = mpMap->rooms[mMultiSelectionList[0]]->x;
    min_y = mpMap->rooms[mMultiSelectionList[0]]->y;
    for( int j=0; j<mMultiSelectionList.size(); j++ )
    {
        if( ! mpMap->rooms.contains( mMultiSelectionList[j] ) ) return -1;
        TRoom * pR = mpMap->rooms[mMultiSelectionList[j]];
        if( pR->x <= min_x )
        {
            min_x = pR->x;
            if( pR->y <= min_y )
            {
                min_y = pR->y;
                id = j;
            }
        }
    }
    return id;

}

void T2DMap::wheelEvent ( QWheelEvent * e )
{
    int delta = e->delta() / 8 / 15;
    if( e->delta() < 0 )
    {
        mPick = false;
        if( ! mpMap->rooms.contains(mpMap->mRoomId) ) return;
        if( ! mpMap->areas.contains(mpMap->rooms[mpMap->mRoomId]->area) ) return;
        if( mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area]->gridMode )
        {
            gzoom += delta;
            if( gzoom < 5 ) gzoom = 5;
            init();
        }
        else
        {
            xzoom += delta;
            yzoom += delta;

            if( yzoom < 3 || xzoom < 3 )
            {
                xzoom = 3;
                yzoom = 3;
            }
        }
        update();
        e->accept();
        update();
        return;
    }
    if( e->delta() > 0 )
    {
        if( ! mpMap->rooms.contains(mpMap->mRoomId) ) return;
        if( ! mpMap->areas.contains(mpMap->rooms[mpMap->mRoomId]->area) ) return;
        if( mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area]->gridMode )
        {
            gzoom += delta;
            init();
        }
        else
        {
            mPick = false;
            xzoom += delta;
            yzoom += delta;
        }
        e->accept();
        update();
        return;
    }
    e->ignore();
    return;
}

