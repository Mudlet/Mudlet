
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
    eSize = 3.0;
    rSize = 0.5;
    mShiftMode = false;
    mShowInfo = true;
    mShowGrid = false;
    mBubbleMode = false;
    mMapperUseAntiAlias = true;
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
    rSize = 0.5;
    eSize = 3.0;
    mShiftMode = false;
    mShowInfo = true;
    mShowGrid = false;
    mBubbleMode = false;
    mMapperUseAntiAlias = true;
}

void T2DMap::init()
{
    if( ! mpMap ) return;
    eSize = mpMap->mpHost->mLineSize;
    rSize = mpMap->mpHost->mRoomSize;

    mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
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

void T2DMap::shiftDown()
{
    mShiftMode = true;
    mOy--;
    update();
}

void T2DMap::toggleShiftMode()
{
    mShiftMode = !mShiftMode;
    update();
}

void T2DMap::shiftUp()
{
    mShiftMode = true;
    mOy++;
    update();
}

void T2DMap::shiftLeft()
{
    mShiftMode = true;
    mOx--;
    update();
}

void T2DMap::shiftRight()
{
    mShiftMode = true;
    mOx++;
    update();
}
void T2DMap::shiftZup()
{
    mShiftMode = true;
    mOz++;
    update();
}

void T2DMap::shiftZdown()
{
    mShiftMode = true;
    mOz--;
    update();
}

void T2DMap::showInfo()
{
    mShowInfo = !mShowInfo;
    update();
}

void T2DMap::switchArea(QString name)
{
    if( !mpMap ) return;
    QMapIterator<int, QString> it( mpMap->areaNamesMap );
    while( it.hasNext() )
    {
        it.next();
        int areaID = it.key();
        QString _n = it.value();
        if( name == _n )
        {
            mAID = areaID;
            mShiftMode = true;
            mOx = 0;
            mOy = 0;
            mOz = 0;
            update();
            break;
        }
    }
}

void T2DMap::paintEvent( QPaintEvent * e )
{
    if( !mpMap ) return;
    bool __Pick = mPick;
    QTime __time; __time.start();

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


    int px,py;
    QList<int> exitList;
    if( ! mpMap->rooms.contains( mpMap->mRoomId ) )
    {
        p.drawText(_w/2,_h/2,"No map or no valid position.");

        return;
    }

    int ox, oy, oz;
    if( mRID != mpMap->mRoomId && mShiftMode ) mShiftMode = false;
    if( (! __Pick && ! mShiftMode ) || mpMap->mNewMove )
    {
        mShiftMode = true;
        mpMap->mNewMove = false; // das ist nur hier von Interesse, weil es nur hier einen map editor gibt -> map wird unter Umstaenden nicht geupdated, deshalb force ich mit mNewRoom ein map update bei centerview()
        if( mpMap->rooms.contains(mpMap->mRoomId) )
            if( ! mpMap->areas.contains( mpMap->rooms[mpMap->mRoomId]->area) )
                return;
        mRID = mpMap->mRoomId;
        mAID = mpMap->rooms[mRID]->area;
        ox = mpMap->rooms[mRID]->x;
        oy = mpMap->rooms[mRID]->y*-1;
        mOx = ox;
        mOy = oy;
        mOz = mpMap->rooms[mRID]->z;
    }
    else
    {
        ox = mOx;
        oy = mOy;
        oz = mOz;
    }
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

    TArea * pArea = mpMap->areas[mAID];
    if( ! pArea ) return;

    int zEbene;
    zEbene = mOz;

    if( ! mpMap ) return;
    if( ! mpMap->rooms.contains( mRID ) ) return;

    float wegBreite = 1/eSize * tx * rSize;

    p.fillRect( 0, 0, width(), height(), mpHost->mBgColor_2 );

    if( pArea->gridMode )
    {
        int areaID = mpMap->rooms[mRID]->area;

        if( mGridPix.contains( mpMap->rooms[mRID]->area ) )
        {
            int _x = mpMap->rooms[mRID]->x;
            int _y = mpMap->rooms[mRID]->y*-1;
            int vx = (abs(pArea->min_x)+_x-(width()/gzoom)/2)*gzoom;
            int vy = (abs(pArea->min_y)+_y-(height()/gzoom)/2)*gzoom;

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
            if( mShowInfo )
            {
                QString text = QString("Area: %1 ID:%2").arg(mpMap->areaNamesMap[mpMap->rooms[mRID]->area]).arg(mpMap->rooms[mRID]->area);
                p.drawText( 10, mFontHeight, text );
                text = QString("Room Name: %1").arg(mpMap->rooms[mRID]->name);
                p.drawText( 10, 2*mFontHeight, text );
                text = QString("Room ID: %1 Position on Map: (%2/%3/%4)").arg(QString::number(mRID)).arg(QString::number(mpMap->rooms[mRID]->x)).arg(QString::number(mpMap->rooms[mRID]->y)).arg(QString::number(mpMap->rooms[mRID]->z));
                p.drawText( 10, 3*mFontHeight, text );
                p.fillRect(mMultiRect,QColor(190,190,190,60));
                text = QString("render time:%1ms").arg(QString::number(__time.elapsed()));
                p.setPen(QColor(255,255,255));
                p.drawText( 10, 4*mFontHeight, text );
            }
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

    QPen pen;
    if( mShowGrid )
    {
        pen = p.pen();
        pen.setWidthF(0);
        pen.setCosmetic( mMapperUseAntiAlias );
        QColor _gridColor = mpHost->mFgColor_2;
        _gridColor.setAlpha(10);
        pen.setColor( _gridColor );
        p.setPen( pen );

        for( int x=0; x<xspan; x++ )
            p.drawLine( (int)(x*tx), 0, (int)(x*tx), (int)(yspan*ty) );

        for( int x=0; x<yspan; x++ )
            p.drawLine( 0, (int)(x*ty), (int)(xspan*tx), (int)(x*ty) );
    }
    pen = p.pen();
    pen.setColor( mpHost->mFgColor_2 );
    pen.setWidthF(wegBreite);
    if(mMapperUseAntiAlias)
        p.setRenderHint(QPainter::Antialiasing);
    else
        p.setRenderHint(QPainter::NonCosmeticDefaultPen);
    p.setPen( pen );

    //mpMap->auditRooms();

    if( ! pArea->gridMode )
    {
        for( int i=0; i<pArea->rooms.size(); i++ )
        {
            TRoom * pR = mpMap->rooms[pArea->rooms[i]];
            int trID = pArea->rooms[i];
            float rx = pR->x*tx+_rx;
            float ry = pR->y*-1*ty+_ry;
            int rz = pR->z;

            if( rz != zEbene ) continue;
            if( rx < 0 || ry < 0 || rx > _w || ry > _h ) continue;

            pR->rendered = true;

            exitList.clear();
            if( pR->customLines.size() > 0 )
            {
                if( ! pR->customLines.contains("N") )
                {
                    exitList.push_back( pR->north );
                }
                if( !pR->customLines.contains("NW") )
                {
                    exitList.push_back( pR->northwest );
                }
                if( !pR->customLines.contains("E") )
                {
                    exitList.push_back( pR->east );
                }
                if( !pR->customLines.contains("SE") )
                {
                    exitList.push_back( pR->southeast );
                }
                if( !pR->customLines.contains("S") )
                {
                    exitList.push_back( pR->south );
                }
                if( !pR->customLines.contains("SW") )
                {
                    exitList.push_back( pR->southwest );
                }
                if( !pR->customLines.contains("W") )
                {
                    exitList.push_back( pR->west );
                }
                if( !pR->customLines.contains("NE") )
                {
                    exitList.push_back( pR->northeast );
                }
            }
            else
            {
                if( pR->north > 0 )
                    exitList.push_back( pR->north );
                if( pR->northwest > 0 )
                    exitList.push_back( pR->northwest );
                if( pR->east > 0 )
                    exitList.push_back( pR->east );
                if( pR->southeast > 0 )
                    exitList.push_back( pR->southeast );
                if( pR->south > 0 )
                    exitList.push_back( pR->south );
                if( pR->southwest > 0 )
                    exitList.push_back( pR->southwest );
                if( pR->west > 0 )
                    exitList.push_back( pR->west );
                if( pR->northeast > 0 )
                    exitList.push_back( pR->northeast );
            }

            if( pR->customLines.size() > 0 )
            {
                QPen oldPen = p.pen();
                QMapIterator<QString, QList<QPointF> > itk(pR->customLines);
                while( itk.hasNext() )
                {
                    itk.next();
                    QColor _color;
                    if( pR->customLinesColor[itk.key()].size() == 3 )
                    {
                        _color.setRed( mpMap->rooms[pArea->rooms[i]]->customLinesColor[itk.key()][0] );
                        _color.setGreen( mpMap->rooms[pArea->rooms[i]]->customLinesColor[itk.key()][1] );
                        _color.setBlue( mpMap->rooms[pArea->rooms[i]]->customLinesColor[itk.key()][2] );
                    }
                    else
                        _color = QColor(255,0,0);
                    bool _arrow = pR->customLinesArrow[itk.key()];
                    QString _style = pR->customLinesStyle[itk.key()];
                    QPointF _cstartP;
                    float ex = pR->x*tx+_rx;
                    float ey = pR->y*ty*-1+_ry;
                    if( itk.key() == "N" )
                        _cstartP = QPoint(ex,ey-ty/2);
                    else if( itk.key() == "NW" )
                        _cstartP = QPoint(ex-tx/2,ey-ty/2);
                    else if( itk.key() == "NE" )
                        _cstartP = QPoint(ex+tx/2,ey-ty/2);
                    else if( itk.key() == "S" )
                        _cstartP = QPoint(ex,ey+ty/2);
                    else if( itk.key() == "SW" )
                        _cstartP = QPoint(ex-tx/2,ey+ty/2);
                    else if( itk.key() == "SE" )
                        _cstartP = QPoint(ex+tx/2,ey+ty/2);
                    else if( itk.key() == "W" )
                        _cstartP = QPoint(ex-tx/2, ey);
                    else if( itk.key() == "E" )
                        _cstartP = QPoint(ex+tx/2, ey);
                    else
                        _cstartP = QPointF(ex, ey);
                    QPointF ursprung = QPointF(ex,ey);
                    QPen customLinePen = p.pen();
                    customLinePen.setCosmetic(mMapperUseAntiAlias);
                    customLinePen.setColor( _color );
                    if( _style == "solid line" )
                        customLinePen.setStyle( Qt::SolidLine );
                    else if( _style == "dot line" )
                        customLinePen.setStyle( Qt::DotLine );
                    else if( _style == "dash line" )
                        customLinePen.setStyle( Qt::DashLine );
                    else
                        customLinePen.setStyle( Qt::DashDotDotLine );

                    QList<QPointF> _pL = itk.value();
                    if( _pL.size() > 0 )
                    {
                        p.setPen(customLinePen);
                        p.drawLine( ursprung, _cstartP );
                    }
                    for( int pk=0; pk<_pL.size(); pk++ )
                    {
                        QPointF _cendP;
                        _cendP.setX( _pL[pk].x()*tx+_rx );
                        _cendP.setY( _pL[pk].y()*ty*-1+_ry );
                        p.drawLine( _cstartP, _cendP );

                        if( pk == _pL.size()-1 && _arrow )
                        {
                            QLineF l0 = QLineF( _cendP, _cstartP );
                            l0.setLength(wegBreite*5);
                            QPointF _p1 = l0.p1();
                            QPointF _p2 = l0.p2();
                            QLineF l1 = QLineF( l0 );
                            qreal w1 = l1.angle()-90.0;
                            QLineF l2;
                            l2.setP1(_p2);
                            l2.setAngle(w1);
                            l2.setLength(wegBreite*2);
                            QPointF _p3 = l2.p2();
                            l2.setAngle( l2.angle()+180.0 );
                            QPointF _p4 = l2.p2();
                            QPolygonF _poly;
                            _poly.append( _p1 );
                            _poly.append( _p3 );
                            _poly.append( _p4 );
                            QBrush brush = p.brush();
                            brush.setColor( _color );
                            brush.setStyle( Qt::SolidPattern );
                            QPen arrowPen = p.pen();
                            arrowPen.setCosmetic( mMapperUseAntiAlias );
                            arrowPen.setStyle(Qt::SolidLine);
                            p.setPen( arrowPen );
                            p.setBrush( brush );
                            p.drawPolygon(_poly);
                        }
                        _cstartP = _cendP;
                    }
                }
                p.setPen(oldPen);
            }

            int e = pR->z;

            QPen __pen;
            for( int k=0; k<exitList.size(); k++ )
            {
                int rID = exitList[k];
                if( rID <= 0 ) continue;

                bool areaExit;

                TRoom * pE = mpMap->rooms[rID];
//                if( ! mpMap->rooms.contains( exitList[k] ) )
//                {
//                    continue;
//                }
                if( pE->area != mAID )
                {
                    areaExit = true;
                }
                else
                    areaExit = false;
                float ex = pE->x*tx+_rx;
                float ey = pE->y*ty*-1+_ry;
                int ez = pE->z;
//                if( ez != zEbene || e != ez ) continue;

                //p.setPen(QPen( mpHost->mFgColor_2) );
//                pen = p.pen();
//                pen.setColor( mpHost->mFgColor_2 );
//                pen.setWidthF( wegBreite );
//                pen.setJoinStyle( Qt::RoundJoin );
//                pen.setCapStyle( Qt::RoundCap );
//                pen.setCosmetic( mMapperUseAntiAlias );
//                p.setPen( pen );

                QVector3D p1( ex, ey, ez );
                QVector3D p2( rx, ry, rz );
                if( ! areaExit )
                {
                   p.drawLine( (int)p1.x(), (int)p1.y(), (int)p2.x(), (int)p2.y() );
                }
                else
                {
                    __pen = p.pen();
                    QLine _line;
                    if( pR->south == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine( p2.x(), p2.y()+ty,p2.x(), p2.y() );
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x(), p2.y()+ty/2);
                        mAreaExitList[exitList[k]] = _p;
                    }
                    else if( pR->north == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine( p2.x(), p2.y()-ty, p2.x(), p2.y() );
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x(), p2.y()-ty/2);
                        mAreaExitList[exitList[k]] = _p;
                    }
                    else if( pR->west == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine(p2.x()-tx, p2.y(),p2.x(), p2.y() );
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x()-tx/2, p2.y());
                        mAreaExitList[exitList[k]] = _p;
                    }
                    else if( pR->east == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine(p2.x()+tx, p2.y(),p2.x(), p2.y() );
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x()+tx/2, p2.y());
                        mAreaExitList[exitList[k]] = _p;
                    }
                    else if( pR->northwest == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine(p2.x()-tx, p2.y()-ty,p2.x(), p2.y() );
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x()-tx/2, p2.y()-ty/2);
                        mAreaExitList[exitList[k]] = _p;
                    }
                    else if( pR->northeast == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine(p2.x()+tx, p2.y()-ty,p2.x(), p2.y());
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x()+tx/2, p2.y()-ty/2);
                        mAreaExitList[exitList[k]] = _p;
                    }
                    else if( pR->southeast == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine(p2.x()+tx, p2.y()+ty, p2.x(), p2.y());
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x()+tx/2, p2.y()+ty/2);
                        mAreaExitList[exitList[k]] = _p;
                    }
                    else if( pR->southwest == rID )
                    {
                        pen = p.pen();
                        pen.setWidthF(wegBreite);
                        pen.setCosmetic( mMapperUseAntiAlias );
                        pen.setColor(getColor(exitList[k]));
                        p.setPen( pen );
                        _line = QLine(p2.x()-tx, p2.y()+ty, p2.x(), p2.y());
                        p.drawLine( _line );
                        QPoint _p = QPoint(p2.x()-tx/2, p2.y()+ty/2);
                        mAreaExitList[exitList[k]] = _p;
                    }
                    QLineF l0 = QLineF( _line );
                    l0.setLength(wegBreite*5);
                    QPointF _p1 = l0.p1();
                    QPointF _p2 = l0.p2();
                    QLineF l1 = QLineF( l0 );
                    qreal w1 = l1.angle()-90.0;
                    QLineF l2;
                    l2.setP1(_p2);
                    l2.setAngle(w1);
                    l2.setLength(wegBreite*2);
                    QPointF _p3 = l2.p2();
                    l2.setAngle( l2.angle()+180.0 );
                    QPointF _p4 = l2.p2();
                    QPolygonF _poly;
                    _poly.append( _p1 );
                    _poly.append( _p3 );
                    _poly.append( _p4 );
                    QBrush brush = p.brush();
                    brush.setColor( getColor(exitList[k]) );
                    brush.setStyle( Qt::SolidPattern );
                    QPen arrowPen = p.pen();
                    arrowPen.setCosmetic( mMapperUseAntiAlias );
                    p.setPen( arrowPen );
                    p.setBrush( brush );
                    p.drawPolygon(_poly);
                    p.setPen( __pen );
                }
            }
        }
    }
    for( int i=0; i<pArea->rooms.size(); i++ )
    {
        TRoom * pR = mpMap->rooms[pArea->rooms[i]];
//        if( ! mpMap->rooms.contains( pArea->rooms[i] ) )
//        {
//            continue;
//        }
        float rx = pR->x*tx+_rx;
        float ry = pR->y*-1*ty+_ry;
        int rz = pR->z;

        if( rz != zEbene ) continue;
        if( rx < 0 || ry < 0 || rx > _w || ry > _h ) continue;

        pR->rendered = false;
        QRectF dr;
        if( pArea->gridMode )
        {
            dr = QRectF(rx-tx/2, ry-ty/2,tx,ty);
        }
        else
        {
            dr = QRectF(rx-(tx*rSize)/2,ry-(ty*rSize)/2,tx*rSize,ty*rSize);
        }

        QColor c;
        int env = pR->environment;
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
        if( ( mPick || __Pick ) && mPHighlight.x() >= dr.x()-(tx*rSize) && mPHighlight.x() <= dr.x()+(tx*rSize) && mPHighlight.y() >= dr.y()-(ty*rSize) && mPHighlight.y() <= dr.y()+(ty*rSize)
            || mMultiSelectionList.contains(pArea->rooms[i]) )
        {
            p.fillRect(dr,QColor(255,155,55));
            mPick = false;
            if( mStartSpeedWalk )
            {
                mStartSpeedWalk = false;
                float _radius = (0.8*tx)/2;
                QPointF _center = QPointF(rx,ry);
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


                mTarget = pArea->rooms[i];
                if( mpMap->rooms.contains(mTarget) )
                {
                    mpMap->mTargetID = mTarget;
                    if( mpMap->findPath( mpMap->mRoomId, mpMap->mTargetID) )
                    {
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
            char _ch = pR->c;
            if( _ch >= 33 && _ch < 255 )
            {
                int _color = ( 265 - 257 ) * 254 + _ch;//(mpMap->rooms[pArea->rooms[i]]->environment - 257 ) * 254 + _ch;

                if( c.red()+c.green()+c.blue() > 260 )
                    _color = ( 7 ) * 254 + _ch;
                else
                    _color = ( 6 ) * 254 + _ch;

                p.fillRect( dr, c );
                if( mPixMap.contains( _color ) )
                {
                    QPixmap pix = mPixMap[_color].scaled(dr.width(), dr.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    p.drawPixmap(dr.topLeft(), pix);
                }
            }
            else
            {
                if( mBubbleMode )
                {
                    float _radius = (rSize*tx)/2;
                    QPointF _center = QPointF(rx,ry);
                    QRadialGradient _gradient(_center,_radius);
                    _gradient.setColorAt(0.85, c);
                    _gradient.setColorAt(0, QColor(255,255,255,255));
                    QPen myPen(QColor(0,0,0,0));
                    QPainterPath myPath;
                    p.setBrush(_gradient);
                    p.setPen(myPen);
                    myPath.addEllipse(_center,_radius,_radius);
                    p.drawPath(myPath);
                }
                else
                    p.fillRect(dr,c);
            }
            if( pR->highlight )
            {
                float _radius = (mpMap->rooms[pArea->rooms[i]]->highlightRadius*tx)/2;
                QPointF _center = QPointF(rx,ry);
                QRadialGradient _gradient(_center,_radius);
                _gradient.setColorAt(0.85, mpMap->rooms[pArea->rooms[i]]->highlightColor);
                _gradient.setColorAt(0, mpMap->rooms[pArea->rooms[i]]->highlightColor2 );
                QPen myPen(QColor(0,0,0,0));
                QPainterPath myPath;
                p.setBrush(_gradient);
                p.setPen(myPen);
                myPath.addEllipse(_center,_radius,_radius);
                p.drawPath(myPath);
            }
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
            if( mShiftMode && pArea->rooms[i] == mpMap->mRoomId )
            {
                float _radius = (1.2*tx)/2;
                QPointF _center = QPointF(rx,ry);
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
        }

        QColor lc;
        if( c.red()+c.green()+c.blue() > 200 )
            lc=QColor(0,0,0);
        else
            lc=QColor(255,255,255);
        pen = p.pen();
        pen.setColor( lc );
        pen.setWidthF(0);//wegBreite?);
        pen.setCosmetic( mMapperUseAntiAlias );
        pen.setCapStyle( Qt::RoundCap );
        pen.setJoinStyle( Qt::RoundJoin );
        p.setPen( pen );

        if( pR->up > 0 )
        {
            QPolygonF _poly;
            QPointF _pt;
            _pt = QPointF( rx, ry+(ty*rSize)/20 );
            _poly.append( _pt );
            _pt = QPointF( rx-(tx*rSize)/3.1, ry+(ty*rSize)/3.1 );
            _poly.append( _pt );
            _pt = QPointF( rx+(tx*rSize)/3.1, ry+(ty*rSize)/3.1);
            _poly.append(_pt);
            QBrush brush = p.brush();
            brush.setColor( QColor(0, 0 ,0) );
            brush.setStyle( Qt::SolidPattern );
            p.setBrush( brush );
            p.drawPolygon(_poly);
        }
        if( pR->down > 0 )
        {
            QPolygonF _poly;
            QPointF _pt;
            _pt = QPointF( rx, ry-(ty*rSize)/20 );
            _poly.append( _pt );
            _pt = QPointF( rx-(tx*rSize)/3.1, ry-(ty*rSize)/3.1 );
            _poly.append( _pt );
            _pt = QPointF( rx+(tx*rSize)/3.1, ry-(ty*rSize)/3.1);
            _poly.append(_pt);
            QBrush brush = p.brush();
            brush.setColor( QColor(0, 0 ,0) );
            brush.setStyle( Qt::SolidPattern );
            p.setBrush( brush );
            p.drawPolygon(_poly);
        }
        if( pR->in > 0 )
        {
            QPolygonF _poly;
            QPointF _pt;
            _pt = QPointF( rx+(tx*rSize)/20, ry );
            _poly.append( _pt );
            _pt = QPointF( rx-(tx*rSize)/3.1, ry-(ty*rSize)/3.1 );
            _poly.append( _pt );
            _pt = QPointF( rx-(tx*rSize)/3.1, ry+(ty*rSize)/3.1);
            _poly.append(_pt);
            QBrush brush = p.brush();
            brush.setColor( QColor(0, 0 ,0) );
            brush.setStyle( Qt::SolidPattern );
            p.setBrush( brush );
            p.drawPolygon(_poly);
        }
        if( pR->out > 0 )
        {
            QPolygonF _poly;
            QPointF _pt;
            _pt = QPointF( rx-(tx*rSize)/20, ry);
            _poly.append( _pt );
            _pt = QPointF( rx+(tx*rSize)/3.1, ry-(ty*rSize)/3.1 );
            _poly.append( _pt );
            _pt = QPointF( rx+(tx*rSize)/3.1, ry+(ty*rSize)/3.1);
            _poly.append(_pt);
            QBrush brush = p.brush();
            brush.setColor( QColor(0, 0 ,0) );
            brush.setStyle( Qt::SolidPattern );
            p.setBrush( brush );
            p.drawPolygon(_poly);
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
                   dr = QRectF(rx-(tx*rSize)/2,ry-(ty*rSize)/2,tx*rSize,ty*rSize);
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
        else
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
                    dr = QRectF(rx,ry,tx*rSize,ty*rSize);//rx-(tx*rSize)/2,ry-(ty*rSize)/2,tx*rSize,ty*rSize);
                }
                if( ((mPick || __Pick) && mPHighlight.x() >= dr.x()-tx/3 && mPHighlight.x() <= dr.x()+tx/3 && mPHighlight.y() >= dr.y()-ty/3 && mPHighlight.y() <= dr.y()+ty/3
                    ) && mStartSpeedWalk )
                {
                    mStartSpeedWalk = false;
                    float _radius = (0.8*tx)/2;
                    QPointF _center = QPointF(rx,ry);
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

                    mPick = false;
                    mTarget = it.key();
                    if( mpMap->rooms.contains(mTarget) )
                    {
                        mpMap->mTargetID = mTarget;
                        if( mpMap->findPath( mpMap->mRoomId, mpMap->mTargetID) )
                        {
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

    if( mShowInfo )
    {
        p.fillRect( 0,0,width(), 5*mFontHeight, QColor(150,150,150,80) );
        QString text;

        //if( mpMap->rooms.contains( mRoomSelection ) && __Pick )
        //{
        int __rid = mRID;
        if( mRoomSelection > 0 && mpMap->rooms.contains( mRoomSelection ) )
        {
            __rid = mRoomSelection;
        }
        if( mpMap->areaNamesMap.contains(__rid))
        {
            text = QString("Area: %1 ID:%2 x:%3-%4 y:%5-%6").arg(mpMap->areaNamesMap[mpMap->rooms[__rid]->area]).arg(mpMap->rooms[__rid]->area).arg(mpMap->areas[mpMap->rooms[__rid]->area]->min_x).arg(mpMap->areas[mpMap->rooms[__rid]->area]->max_x).arg(mpMap->areas[mpMap->rooms[__rid]->area]->min_y).arg(mpMap->areas[mpMap->rooms[__rid]->area]->max_y);
            p.drawText( 10, mFontHeight, text );
        }
        text = QString("Room Name: %1").arg(mpMap->rooms[__rid]->name);
        p.drawText( 10, 2*mFontHeight, text );
        text = QString("Room ID: %1 Position on Map: (%2/%3/%4)").arg(QString::number(__rid)).arg(QString::number(mpMap->rooms[__rid]->x)).arg(QString::number(mpMap->rooms[__rid]->y)).arg(QString::number(mpMap->rooms[__rid]->z));
        p.drawText( 10, 3*mFontHeight, text );
//        else
//        {
//            text = QString("Area: %1 ID:%2").arg(mpMap->areaNamesMap[mpMap->rooms[mRID]->area]).arg(mpMap->rooms[mRID]->area);
//            p.drawText( 10, mFontHeight, text );
//            text = QString("Room Name: %1").arg(mpMap->rooms[mRID]->name);
//            p.drawText( 10, 2*mFontHeight, text );
//            text = QString("Room ID: %1 Position on Map: (%2/%3/%4)").arg(QString::number(mRID)).arg(QString::number(mpMap->rooms[mRID]->x)).arg(QString::number(mpMap->rooms[mRID]->y)).arg(QString::number(mpMap->rooms[mRID]->z));
//            p.drawText( 10, 3*mFontHeight, text );
//        }
    }
    if( mMapInfoRect == QRect(0,0,0,0) ) mMapInfoRect = QRect(0,0,width(),height()/10);

    if( ! mShiftMode )
    {

        if( mpHost->mMapStrongHighlight )
        {
            QRectF dr = QRectF(px-(tx*rSize)/2,py-(ty*rSize)/2,tx*rSize,ty*rSize);
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
    }

    if( mpMap->mapLabels.contains( mAID ) )
    {
        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAID]);
        while( it.hasNext() )
        {
            it.next();

            QRectF lr = QRectF( 0, 0, 1000, 100 );
            QPixmap pix( lr.size().toSize() );
            pix.fill(QColor(0,0,0,0));
            QPainter lp( &pix );

            lp.fillRect( lr, it.value().bgColor );
            QPen lpen;
            lpen.setColor( it.value().fgColor );
            lp.setPen( lpen );
            QRectF br;
            lp.drawText( lr, Qt::AlignLeft, it.value().text, &br );
            QPointF lpos;
            lpos.setX( it.value().pos.x()*tx+_rx );
            lpos.setY( it.value().pos.y()*ty*-1+_ry );
            p.drawPixmap( lpos, pix, br.toRect() );

        }
    }
    if( mShowInfo )
    {
        p.setPen(QColor(0,255,0,90));
        p.fillRect(mMultiRect,QColor(190,190,190,60));

        QString text = QString("render time:%1ms").arg(QString::number(__time.elapsed()));
        p.setPen(QColor(255,255,255));
        p.drawText( 10, 4*mFontHeight, text );
    }

}

void T2DMap::mouseDoubleClickEvent ( QMouseEvent * event )
{
    int x = event->x();
    int y = event->y();
    mPHighlight = QPoint(x,y);
    mPick = true;
    mStartSpeedWalk = true;
    std::cout << "double click x="<<x<<" y="<<y<<endl;
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
        if( mCustomLinesRoomFrom > 0 )
        {
            if( mpMap->rooms.contains( mCustomLinesRoomFrom ) )
            {
                float ox = mOx;
                float oy = mOy;
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
                float mx = event->pos().x()/mTX + mOx;
                float my = event->pos().y()/mTY + mOy;
                mx = mx - xspan/2;
                my = yspan/2 - my;;
                mpMap->rooms[mCustomLinesRoomFrom]->customLines[mCustomLinesRoomExit].push_back( QPointF( mx, my ) );
            }
            repaint();
        }
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
        if( mCustomLinesRoomFrom > 0 )
        {
            mCustomLinesRoomFrom = 0;
        }
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
//        QAction * action11 = new QAction("image", this );
//        action11->setStatusTip(tr("set an image to mark special rooms"));
//        connect( action11, SIGNAL(triggered()), this, SLOT(slot_setImage()));

        QAction * action12 = new QAction("move to", this );
        action12->setStatusTip(tr("move selected group to a given position"));
        connect( action12, SIGNAL(triggered()), this, SLOT(slot_movePosition()));

        QAction * action13 = new QAction("area", this );
        action13->setStatusTip(tr("set room area ID"));
        connect( action13, SIGNAL(triggered()), this, SLOT(slot_setArea()));

        QAction * action14 = new QAction("custom exit lines", this );
        action14->setStatusTip(tr("replace an exit line with a custom line"));
        connect( action14, SIGNAL(triggered()), this, SLOT(slot_setCustomLine()));

        QAction * action15 = new QAction("Create Label", this );
        action15->setStatusTip(tr("Create labels to show text or images."));
        connect( action15, SIGNAL(triggered()), this, SLOT(slot_createLabel()));

        mPopupMenu = true;
        QMenu * popup = new QMenu( this );

        popup->addAction( action );
        popup->addAction( action8 );
        popup->addAction( action14 );
        popup->addAction( action3 );
        popup->addAction( action10 );
        //popup->addAction( action11 );
        popup->addAction( action4 );
        popup->addAction( action5 );
        popup->addAction( action6 );
        popup->addAction( action7 );
        popup->addAction( action2 );
        popup->addAction( action12 );
        popup->addAction( action9 );
        popup->addAction( action13 );

        popup->addAction( action15 );

        popup->popup( mapToGlobal( event->pos() ) );
        mLastMouseClick = event->posF();
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
                mpHost->mpMap->mMapGraphNeedsUpdate = true;
            }
        }
    }
    else if( mpHost->mpMap->rooms.contains( mRoomSelection ) )
    {
        mpHost->mpMap->rooms[mRoomSelection]->isLocked = true;
        mpHost->mpMap->mMapGraphNeedsUpdate = true;
    }
}


void T2DMap::slot_setRoomWeight()
{


    if( mMultiSelectionList.size() > 0 )
    {
        int w = QInputDialog::getInt(this,"Enter a room weight (= travel time)","room weight:", 1);
        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                mpMap->rooms[mMultiSelectionList[j]]->weight = w;
                mpHost->mpMap->mMapGraphNeedsUpdate = true;
            }
        }
        //repaint();
    }
    else
    {
        if( mpHost->mpMap->rooms.contains(mRoomSelection) )
        {
            int _w = mpHost->mpMap->rooms[mRoomSelection]->weight;
            qDebug()<<"vorher weight:"<<_w;
            int w = QInputDialog::getInt(this, "Enter a room weight (= travel time).","room weight:",_w);
            qDebug()<<"--> nachher weight:"<<w;
            mpHost->mpMap->rooms[mRoomSelection]->weight = w;
            mpHost->mpMap->mMapGraphNeedsUpdate = true;
        }
    }
}

#include <QtUiTools>

void T2DMap::slot_setArea()
{
    QUiLoader loader;

    QFile file(":/ui/set_room_area.ui");
    file.open(QFile::ReadOnly);
    QDialog *set_room_area_dialog = dynamic_cast<QDialog *>(loader.load(&file, this));
    file.close();
    if( ! set_room_area_dialog ) return;
    arealist_combobox = set_room_area_dialog->findChild<QComboBox*>("arealist_combobox");
    if( !arealist_combobox ) return;

    QMapIterator<int, QString> it( mpMap->areaNamesMap );
    while( it.hasNext() )
    {
        it.next();
        int areaID = it.key();
        if( areaID > 0 )
        {
            arealist_combobox->addItem(QString(it.value() + " ("+QString::number(areaID)+")"), QVariant(areaID));
        }
    }
    if( set_room_area_dialog->exec() == QDialog::Rejected ) return;

    int w = arealist_combobox->itemData(arealist_combobox->currentIndex()).toInt();


    if( mMultiSelection )
    {
        //int w = QInputDialog::getInt(this,"Enter the area ID:", "area ID:", 1);

        mMultiRect = QRect(0,0,0,0);
        for( int j=0; j<mMultiSelectionList.size(); j++ )
        {
            if( mpMap->rooms.contains( mMultiSelectionList[j] ) )
            {
                mpMap->setRoomArea( mMultiSelectionList[j], w );
                //mpMap->rooms[mMultiSelectionList[j]]->area = w;
            }
        }
        repaint();
    }
    else
    {
        if( mpMap->rooms.contains(mRoomSelection) )
        {
            //int _w = mpHost->mpMap->rooms[mRoomSelection]->area;
            //int w = QInputDialog::getInt(this, "Enter area ID:","area ID:",_w);
            mpMap->setRoomArea( mRoomSelection, w );
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

        int _roomID = mRID;
        if( ! mpMap->rooms.contains( _roomID ) ) return;
        int _areaID = mAID;
        if( ! mpMap->areas.contains(_areaID) ) return;
        TArea * pArea = mpMap->areas[_areaID];
        int ox = mOx;//mpMap->rooms[mRID]->x;
        int oy = mOy;//mpMap->rooms[mRID]->y*-1;
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

            // NOTE: in multi room selection mode you select all rooms within the selection rectangle on *all* z-levels to make area merges easier

            QRectF dr;
            if( pArea->gridMode )
            {
                dr = QRectF(rx-mTX/2, ry-mTY/2,mTX,mTY);
            }
            else
            {
                dr = QRectF(rx-(mTX*rSize)/2,ry-(mTY*rSize)/2,mTX*rSize,mTY*rSize);
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
        if( mpMap->rooms.contains( mRID ) )
        {
            mx += mpMap->rooms[mRID]->x;//FIXME
            my += mpMap->rooms[mRID]->y;
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
                int ox = mOx;
                int oy = mOy;
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
                int mx = event->pos().x()/mTX + mOx;
                int my = event->pos().y()/mTY + mOy;
                mx = mx - xspan/2;// + 1;
                my = yspan/2 - my;// - 1;
                mpMap->rooms[mRoomSelection]->x = mx;
                mpMap->rooms[mRoomSelection]->y = my;

                QMapIterator<QString, QList<QPointF> > itk(mpMap->rooms[mRoomSelection]->customLines);
                while( itk.hasNext() )
                {
                    itk.next();
                    QList<QPointF> _pL = itk.value();
                    for( int pk=0; pk<_pL.size(); pk++ )
                    {
                        QPointF op = _pL[pk];
                        _pL[pk].setX( (float)(mx - op.x()) );
                        _pL[pk].setY( (float)(my - op.y()) );
                    }
                }
            }
            repaint();
        }
        else
        {
            mMultiRect = QRect(0,0,0,0);
            int _roomID = mRID;
            if( ! mpMap->rooms.contains( _roomID ) ) return;
            int _areaID = mAID;
            if( ! mpMap->areas.contains(_areaID) ) return;
            TArea * pArea = mpMap->areas[_areaID];
            int ox = mOx;
            int oy = mOy;
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
            int mx = event->pos().x()/mTX + mOx;
            int my = event->pos().y()/mTY + mOy;
            mx = mx - xspan/2 + 1;
            my = yspan/2 - my - 1;

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

                    QMapIterator<QString, QList<QPointF> > itk(mpMap->rooms[mMultiSelectionList[j]]->customLines);
                    QMap<QString, QList<QPointF> > newMap;
                    while( itk.hasNext() )
                    {
                        itk.next();
                        QList<QPointF> _pL = itk.value();
                        for( int pk=0; pk<_pL.size(); pk++ )
                        {
                            QPointF op = _pL[pk];
                            _pL[pk].setX( (float)(op.x()+dx) );
                            _pL[pk].setY( (float)(op.y()+dy) );
                        }
                        newMap.insert(itk.key(), _pL );
                    }
                    mpMap->rooms[mMultiSelectionList[j]]->customLines = newMap;
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
        if( ! mpMap->rooms.contains(mRID) ) return;
        if( ! mpMap->areas.contains(mAID) ) return;
        if( mpMap->areas[mpMap->rooms[mRID]->area]->gridMode )
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
        if( ! mpMap->rooms.contains(mRID) ) return;
        if( ! mpMap->areas.contains(mAID) ) return;
        if( mpMap->areas[mAID]->gridMode )
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


void T2DMap::setRoomSize( double f )
{
    rSize = f;
    if( mpHost ) mpHost->mRoomSize = f;

}

void T2DMap::setExitSize( double f )
{
    eSize = f;
    if( mpHost ) mpHost->mLineSize = f;
}

#include <QTreeWidget>

void T2DMap::slot_setCustomLine()
{
    if( ! mpHost->mpMap->rooms.contains( mRoomSelection ) ) return;
    QUiLoader loader;

    QFile file(":/ui/custom_lines.ui");
    file.open(QFile::ReadOnly);
    QDialog *d = dynamic_cast<QDialog *>(loader.load(&file, this));
    file.close();
    if( ! d ) return;
    mpCustomLinesDialog = d;
    TRoom * pR = mpHost->mpMap->rooms[mRoomSelection];

    mCustomLinesRoomFrom = mRoomSelection;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit = "";
    QPushButton * b_ = d->findChild<QPushButton*>("nw");
    QTreeWidget * specialExits = d->findChild<QTreeWidget*>("specialExits");
    mpCurrentLineStyle = d->findChild<QComboBox*>("lineStyle");
    mpCurrentLineColor = d->findChild<QPushButton*>("lineColor");
    mpCurrentLineArrow = d->findChild<QCheckBox*>("arrow");
    if( ! b_ || ! specialExits || ! mpCurrentLineColor || ! mpCurrentLineStyle || ! mpCurrentLineArrow ) return;

    b_->setCheckable(false);
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    if( pR->northwest <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("n");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->north <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("ne");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->northeast <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("w");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->west <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("e");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->east <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("s");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->south <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("se");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->southeast <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("sw");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->southwest <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("up");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->up <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("down");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->down <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("in");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->in <= 0 ) b_->setDisabled(true);

    b_ = d->findChild<QPushButton*>("out");
    if( !b_ ) return;
    connect(b_, SIGNAL(pressed()), this, SLOT(slot_setCustomLine2()));
    b_->setCheckable(false);
    if( pR->out <= 0 ) b_->setDisabled(true);

    QMapIterator<int, QString> it(pR->other);
    while( it.hasNext() )
    {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        QTreeWidgetItem * pI = new QTreeWidgetItem(specialExits);
        pI->setText( 0, QString::number(id_to) );
        if( dir.size() > 1 )
            if( dir.startsWith('0')|| dir.startsWith('1') )
                dir = dir.mid(1);
        pI->setText( 1, dir );
    }

    QStringList _lineStyles;
    _lineStyles << "solid line" << "dot line" << "dash line" << " dash dot line" << "dash dot dot line";
    mpCurrentLineStyle->addItems(_lineStyles);
    connect(specialExits, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(slot_setCustomLine2B(QTreeWidgetItem*, int)));
    connect( mpCurrentLineColor, SIGNAL(pressed()), this, SLOT(slot_customLineColor()));

    d->show();
    d->raise();
}

void T2DMap::slot_customLineColor()
{
    QColor color = QColorDialog::getColor( mpHost->mFgColor_2, this );
    if ( color.isValid() )
    {
        mCurrentLineColor = color;
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        mpCurrentLineColor->setPalette( palette );
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        mpCurrentLineColor->setStyleSheet( styleSheet );
    }
    else
        mCurrentLineColor = mpHost->mFgColor_2;
}

#include "TRoom.h"

void T2DMap::slot_setCustomLine2()
{
    QPushButton* pB = (QPushButton*)sender();
    if( ! pB ) return;
    QString exit = pB->text();
    mpCustomLinesDialog->close();
    mCustomLinesRoomExit = exit;
    if( ! mpHost->mpMap->rooms.contains(mCustomLinesRoomFrom) ) return;
    QList<QPointF> _list;
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLines[exit] = _list;
    QList<int> _colorList;
    _colorList << mCurrentLineColor.red() << mCurrentLineColor.green() << mCurrentLineColor.blue();
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLinesColor[exit] = _colorList;
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLinesStyle[exit] = mpCurrentLineStyle->currentText();
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLinesArrow[exit] = mpCurrentLineArrow->isChecked();
    qDebug()<<"exit="<<exit;
}

void T2DMap::slot_setCustomLine2B(QTreeWidgetItem * special_exit, int column )
{
    if( ! special_exit ) return;
    QString exit = special_exit->text(1);
    mpCustomLinesDialog->close();
    mCustomLinesRoomExit = exit;
    if( ! mpHost->mpMap->rooms.contains(mCustomLinesRoomFrom) ) return;
    QList<QPointF> _list;
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLines[exit] = _list;
    QList<int> _colorList;
    _colorList << mCurrentLineColor.red() << mCurrentLineColor.green() << mCurrentLineColor.blue();
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLinesColor[exit] = _colorList;
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLinesStyle[exit] = mpCurrentLineStyle->currentText();
    mpHost->mpMap->rooms[mCustomLinesRoomFrom]->customLinesArrow[exit] = mpCurrentLineArrow->isChecked();
    qDebug()<<"exit="<<exit;
}

void T2DMap::slot_createLabel()
{
    if( ! mpMap->areas.contains( mAID ) ) return;

    TMapLabel label;
    QString title = "Enter label text.";
    QString t = QInputDialog::getText(0, title, title );
    label.text = t;
    label.bgColor = QColor(50,50,150,100);
    label.fgColor = QColor(255,255,50,255);
    label.size = QSizeF(100,100);
    int ox = mOx;
    int oy = mOy;
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
    float mx = mLastMouseClick.x()/mTX + mOx;
    float my = mLastMouseClick.y()/mTY + mOy;
    mx = mx - xspan/2;
    my = yspan/2 - my;
    label.pos = QPointF( mx, my );
    if( ! mpMap->areas.contains(mAID) ) return;
    int labelID;
    if( ! mpMap->mapLabels.contains( mAID ) )
    {
        QMap<int, TMapLabel> m;
        m[0] = label;
        mpMap->mapLabels[mAID] = m;
    }
    else
    {
        labelID = mpMap->mapLabels[mAID].size();
        mpMap->mapLabels[mAID].insert(labelID, label);
    }
}

