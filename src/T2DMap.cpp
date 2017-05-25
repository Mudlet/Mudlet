/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "Host.h"
#include "TArea.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgMapper.h"
#include "dlgRoomExits.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QtUiTools>
#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFontDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalMapper>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "post_guard.h"


T2DMap::T2DMap(QWidget * parent)
: QWidget(parent)
, mDialogLock(false)
, mMultiSelectionListWidget(this)
, mMultiSelectionHighlightRoomId(-1)
, mIsSelectionSorting(true)
, mIsSelectionSortByNames(false)
, mIsSelectionUsingNames(false)
, mpMap()
, _rx()
, _ry()
, mTX()
, mTY()
, mChosenRoomColor()
, xspan()
, yspan()
, mMultiSelection()
, mPopupMenu()
, mRID()
, mAID()
, mOx()
, mOy()
, mOz()
, arealist_combobox()
, mpCurrentLineStyle()
, mpCurrentLineColor()
, mpCurrentLineArrow()
, mCurrentLineArrow()
, mShowGrid()
{
    mMultiSelectionListWidget.setColumnCount(2);
    mMultiSelectionListWidget.hideColumn(1);
    QStringList headerLabels;
    headerLabels << tr("Room Id") << tr("Room Name");
    mMultiSelectionListWidget.setHeaderLabels(headerLabels);
    mMultiSelectionListWidget.setToolTip(tr("<html><head/><body><p>Click on a line to select or deselect that room number (with the given name if the rooms are named) to add or remove the room from the selection.  Click on the relevant header to sort by that method.  Note that the name column willl only show if at least one of the rooms has a name.</p></body></html>"));
    mMultiSelectionListWidget.setUniformRowHeights(true);
    mMultiSelectionListWidget.setItemsExpandable(false);
    mMultiSelectionListWidget.setSelectionMode(QAbstractItemView::MultiSelection); // Was ExtendedSelection
    mMultiSelectionListWidget.setRootIsDecorated(false);
    QSizePolicy multiSelectionSizePolicy( QSizePolicy::Maximum, QSizePolicy::Expanding );
    mMultiSelectionListWidget.setSizePolicy( multiSelectionSizePolicy );
    mMultiSelectionListWidget.setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
    mMultiSelectionListWidget.setFrameShape(QFrame::NoFrame);
    mMultiSelectionListWidget.setFrameShadow(QFrame::Plain);
    mMultiSelectionListWidget.header()->setProperty("showSortIndicator",QVariant(true));
    mMultiSelectionListWidget.header()->setSectionsMovable(false);
    mMultiSelectionListWidget.header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    mMultiSelectionListWidget.header()->setStretchLastSection(true);
    mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
    mMultiSelectionListWidget.resize(120,100);
    mMultiSelectionListWidget.move(0,0);
    mMultiSelectionListWidget.hide();
    connect(&mMultiSelectionListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slot_roomSelectionChanged()));
    mMoveLabel = false;
    mLabelHilite = false;
    xzoom = 20;
    yzoom = 20;
    gzoom = 20;
    mPick = false;
    mTarget = 0;
    //mRoomSelection = 0;
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
    mBubbleMode = false;
    mMapperUseAntiAlias = true;
    mMoveLabel = false;
    mCustomLineSelectedRoom = 0;
    mCustomLineSelectedExit = "";
    mCustomLineSelectedPoint = -1;
    mCustomLinesRoomFrom = 0;
    mCustomLinesRoomTo = 0;
    mSizeLabel = false;
    gridMapSizeChange = true;
    isCenterViewCall = false;
    mCurrentLineColor = QColor(255,255,100);
    //setFocusPolicy( Qt::ClickFocus);
}

void T2DMap::init()
{
    isCenterViewCall = false;
    QTime _time; _time.start();
    //setFocusPolicy( Qt::ClickFocus);

    if( ! mpMap ) return;
    gridMapSizeChange = false;
    eSize = mpMap->mpHost->mLineSize;
    rSize = mpMap->mpHost->mRoomSize;

    mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
    mPixMap.clear();
    QFont f = QFont( QFont("Bitstream Vera Sans Mono", 20, QFont::Normal ) );//( QFont("Monospace", 10, QFont::Courier) );
    f.setPointSize(gzoom);
    f.setBold(true);
    int j=0;
    for( int k=1; k<17; k++ )
    {
       for( int i=1;i<255;i++ )
       {
           j++;
           QPixmap b(gzoom,gzoom);
           b.fill(Qt::transparent);
           QPainter p(&b);
           QColor c = _getColor(k);
           p.setPen(c);
           p.setFont(f);
           QRect r(0,0,gzoom,gzoom);
           p.drawText(r, Qt::AlignHCenter|Qt::AlignVCenter, QChar(i) );
           mPixMap[j]=b;
       }
    }
    mCurrentLineArrow = true;
    mCurrentLineColor = QColor("red");
    mCurrentLineStyle = QString("solid line"); // Configure custom line drawing starting points here
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

    TRoom * pR = mpMap->mpRoomDB->getRoom(id);
    if( !pR ) return c;

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
        break;
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


void T2DMap::slot_switchArea(QString name)
{
    Host * pHost = mpHost;
    if( !pHost || !mpMap )
    {
        return;
    }

    int playerRoomId = mpMap->mRoomIdHash.value( pHost->getName() );
    TRoom * pPlayerRoom = mpMap->mpRoomDB->getRoom( playerRoomId );
    int playerAreaId = -2; // Cannot be valid (but -1 can be)!
    if( pPlayerRoom ) {
        playerAreaId = pPlayerRoom->getArea();
    }

    QMapIterator<int, QString> it( mpMap->mpRoomDB->getAreaNamesMap() );
    while( it.hasNext() )
    {
        it.next();
        int areaID = it.key();

        QString _n = it.value();
        TArea * pA = mpMap->mpRoomDB->getArea( areaID );
        if( name == _n && pA )
        {

            mAID = areaID;
            mShiftMode = true;
            pA->calcSpan();

            if( areaID == playerAreaId ) {
                // We are switching back to the area that has the player in it
                // recenter view on that room!
                mOx = pPlayerRoom->x;
                mOy = -pPlayerRoom->y;  // Map y coordinates are reversed on 2D map!
                mOz = pPlayerRoom->z;
                repaint();
                mpMap->set3DViewCenter( mAID, mOx, -mOy, mOz ); // Pass the coordinates to the TMap instance to pass to the 3D mapper
                return; // escape early
            }

            bool isAValidRoomFound = false;
            if( ! pA->ebenen.contains(mOz) )
            {
                // If the current map z-coordinate value is NOT one that is used
                // for this then get the FIRST room in the area and goto the
                // mathmatical midpoint of all the rooms on the same
                // z-coordinate.
                QSetIterator<int> itRoom( pA->getAreaRooms() );
                QMap<int, int> roomsCountLevelMap; // key is z-coordinate, value is count of rooms on that level
                while( itRoom.hasNext() )
                {
                    int checkRoomId = itRoom.next();
                    TRoom * pR = mpMap->mpRoomDB->getRoom( checkRoomId );
                    if( pR )
                    {
                        isAValidRoomFound = true;
                        if( roomsCountLevelMap.contains( pR->z ) )
                        {
                            ++roomsCountLevelMap[ pR->z ];
                        }
                        else
                        {
                            roomsCountLevelMap[ pR->z ] = 1;
                        }
                    }
                }

                if( isAValidRoomFound )
                {
                    QMapIterator<int, int> itRoomsCount( roomsCountLevelMap );
                    itRoomsCount.toBack(); // Start at highest value and work down
                    itRoomsCount.previous(); // This will be Okay as we KNOW there is at least one entry
                    int maxRoomCountOnLevel = 0;
                    int minLevelWithMaxRoomCount = itRoomsCount.key(); // Initalisation value, will get overwritten
                    itRoomsCount.next(); // Return to the back so the previous() in the do loop works correctly
                    do
                    {
                        itRoomsCount.previous();
                        if( maxRoomCountOnLevel < itRoomsCount.value() )
                        {
                            maxRoomCountOnLevel = itRoomsCount.value();
                            minLevelWithMaxRoomCount = itRoomsCount.key();
                        }
                    } while( itRoomsCount.hasPrevious() );

                    // We now have lowest level with the highest number of rooms
                    // Now find the geometry center of the rooms on THAT level
                    // In a similar manner to the getCenterSelection() method
                    itRoom.toFront();
                    float mean_x = 0.0;
                    float mean_y = 0.0;
                    uint processedRoomCount = 0;
                    QSet<TRoom *> pSRoom; // Hold on to relevent rooms for
                                          // following step
                    while( itRoom.hasNext() )
                    {
                        TRoom * pR = mpMap->mpRoomDB->getRoom( itRoom.next() );
                        if( ! pR || pR->z != minLevelWithMaxRoomCount )
                        {
                            continue;
                        }

                        pSRoom.insert( pR );
                        mean_x += (static_cast<float>(pR->x - mean_x)) / ++processedRoomCount;
                        mean_y += (static_cast<float>(pR->y - mean_y)) / processedRoomCount;
                    }

                    // We now have the position that is the "centre" of the
                    // rooms on this level - just need to find the room nearest
                    // to that:
                    QSetIterator<TRoom *> itpRoom( pSRoom );
                    float closestSquareDistance = -1.0;
                    TRoom * pClosestRoom = 0;
                    while( itpRoom.hasNext() )
                    {
                        TRoom * pR = itpRoom.next();
                        QVector2D meanToRoom( static_cast<float>(pR->x)-mean_x, static_cast<float>(pR->y)-mean_y);
                        if( closestSquareDistance < -0.5 )
                        {
                            // Test for first time around loop - for initalisation
                            // Don't use an equality to zero test, we are using floats so
                            // need to allow for a little bit of fuzzzyness!
                            closestSquareDistance = meanToRoom.lengthSquared();
                            pClosestRoom = pR;
                        }
                        else
                        {
                            float currentRoomSquareDistance = meanToRoom.lengthSquared();
                            if( closestSquareDistance > currentRoomSquareDistance  )
                            {
                                closestSquareDistance = currentRoomSquareDistance;
                                pClosestRoom = pR;
                            }
                        }
                    }

                    mOx = pClosestRoom->x;
                    mOy = - pClosestRoom->y; // Map y coordinates are reversed on 2D map!
                    mOz = pClosestRoom->z;
                }

                if( ! isAValidRoomFound )
                {
                    //no rooms, go to 0,0,0
                    mOx = 0;
                    mOy = 0;
                    mOz = 0;
                }
            }
            else
            {
                // Else the selected area DOES have rooms on the same z-coordinate
                // Now find the geometry center of the rooms on the given level
                // In a similar manner to the getCenterSelection() method
                float mean_x = 0.0;
                float mean_y = 0.0;
                uint processedRoomCount = 0;
                QSet<TRoom *> pSRoom; // Hold on to relevent rooms for
                                      // following step
                QSetIterator<int> itRoom( pA->getAreaRooms() );
                while( itRoom.hasNext() ) {
                    TRoom * pR = mpMap->mpRoomDB->getRoom( itRoom.next() );
                    if( ! pR || pR->z != mOz ) {
                        continue;
                    }

                    pSRoom.insert( pR );
                    mean_x += (static_cast<float>(pR->x - mean_x)) / ++processedRoomCount;
                    mean_y += (static_cast<float>(pR->y - mean_y)) / processedRoomCount;
                }

                // We now have the position that is the "centre" of the
                // rooms on this level - just need to find the room nearest
                // to that:
                QSetIterator<TRoom *> itpRoom( pSRoom );
                float closestSquareDistance = -1.0;
                TRoom * pClosestRoom = 0;
                while( itpRoom.hasNext() ) {
                    TRoom * pR = itpRoom.next();
                    QVector2D meanToRoom( static_cast<float>(pR->x)-mean_x, static_cast<float>(pR->y)-mean_y);
                    if( closestSquareDistance < -0.5 ) {
                        // Test for first time around loop - for initalisation
                        // Don't use an equality to zero test, we are using floats so
                        // need to allow for a little bit of fuzzzyness!
                        closestSquareDistance = meanToRoom.lengthSquared();
                        pClosestRoom = pR;
                    }
                    else {
                        float currentRoomSquareDistance = meanToRoom.lengthSquared();
                        if( closestSquareDistance > currentRoomSquareDistance  ) {
                            closestSquareDistance = currentRoomSquareDistance;
                            pClosestRoom = pR;
                        }
                    }
                }

                if (pClosestRoom) {
                    mOx = pClosestRoom->x;
                    mOy = -pClosestRoom->y; // Map y coordinates are reversed on 2D map!
                }
            }
            repaint();
            mpMap->set3DViewCenter( mAID, mOx, -mOy, mOz ); // Pass the coordinates to the TMap instance to pass to the 3D mapper
            return;
        }
    }
}


void T2DMap::paintEvent( QPaintEvent * e )
{
    if( !mpMap )
    {
        return;
    }
    bool __Pick = mPick;
    QElapsedTimer __time;
    __time.start();

    QPainter p( this );
    if( ! p.isActive() )
    {
        return;
    }

    mAreaExitList.clear();

    float _w = width();
    float _h = height();

    if( _w < 10 || _h < 10 )
    {
        return;
    }

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
    QList<int> oneWayExits;
    TRoom * pPlayerRoom = mpMap->mpRoomDB->getRoom( mpMap->mRoomIdHash.value( mpHost->getName() ) );
    if( !pPlayerRoom )
    {
        p.drawText(_w/2,_h/2,"No map or no valid position.");

        return;
    }

    int ox, oy; // N/U: oz;
    if( mRID != mpMap->mRoomIdHash.value( mpHost->getName() ) && mShiftMode )
    {
        mShiftMode = false;
    }
    TArea * pAID;
    TRoom * pRID;
    int playerArea = pPlayerRoom->getArea();
    if( (! __Pick && ! mShiftMode ) || mpMap->mNewMove )
    {

        mShiftMode = true;
        mpMap->mNewMove = false; // das ist nur hier von Interesse, weil es nur hier einen map editor gibt -> map wird unter Umstaenden nicht geupdated, deshalb force ich mit mNewRoom ein map update bei centerview()

        if( !mpMap->mpRoomDB->getArea( playerArea ) )
        {
            return;
        }
        mRID = mpMap->mRoomIdHash.value( mpHost->getName() );
        pRID = mpMap->mpRoomDB->getRoom( mRID );
        mAID = pRID->getArea();
        pAID = mpMap->mpRoomDB->getArea( mAID );
        if( !pRID || !pAID ) return;
        ox = pRID->x;
        oy = pRID->y*-1;
        mOx = ox;
        mOy = oy;
        mOz = pRID->z;
    }
    else
    {
        pRID = mpMap->mpRoomDB->getRoom( mRID );
        pAID = mpMap->mpRoomDB->getArea( mAID );
        if( !pRID || !pAID ) return;
        ox = mOx;
        oy = mOy;
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

    TArea * pArea = pAID;

    int zEbene;
    zEbene = mOz;

    float wegBreite = 1/eSize * tx * rSize;

    p.fillRect( 0, 0, width(), height(), mpHost->mBgColor_2 );

    QPen pen;

    pen = p.pen();
    pen.setColor( mpHost->mFgColor_2 );
    pen.setWidthF(wegBreite);
    if(mMapperUseAntiAlias)
        p.setRenderHint(QPainter::Antialiasing);
    else
        p.setRenderHint(QPainter::NonCosmeticDefaultPen);
    p.setPen( pen );

    if( mpMap->mapLabels.contains( mAID ) )
    {
        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAID]);
        while( it.hasNext() )
        {
            it.next();
            if( it.value().pos.z() != mOz ) continue;
            if( it.value().text.length() < 1 )
            {
                mpMap->mapLabels[mAID][it.key()].text = "no text";
            }
            QPointF lpos;
            int _lx = it.value().pos.x()*tx+_rx;
            int _ly = it.value().pos.y()*ty*-1+_ry;

            lpos.setX( _lx );
            lpos.setY( _ly );
            int _lw = abs(it.value().size.width())*tx;
            int _lh = abs(it.value().size.height())*ty;
            if( ! ( ( 0<_lx || 0<_lx+_lw ) && (_w>_lx || _w>_lx+_lw ) ) ) continue;
            if( ! ( ( 0<_ly || 0<_ly+_lh ) && (_h>_ly || _h>_ly+_lh ) ) ) continue;
            QRectF _drawRect = QRect(it.value().pos.x()*tx+_rx, it.value().pos.y()*ty*-1+_ry, _lw, _lh);
            if( ! it.value().showOnTop )
            {
                if( ! it.value().noScaling )
                {
                    p.drawPixmap( lpos, it.value().pix.scaled(_drawRect.size().toSize()) );
                    mpMap->mapLabels[mAID][it.key()].clickSize.setWidth(_drawRect.width());
                    mpMap->mapLabels[mAID][it.key()].clickSize.setHeight(_drawRect.height());
                }
                else
                {
                    p.drawPixmap( lpos, it.value().pix );
                    mpMap->mapLabels[mAID][it.key()].clickSize.setWidth(it.value().pix.width());
                    mpMap->mapLabels[mAID][it.key()].clickSize.setHeight(it.value().pix.height());
                }
            }

            if( it.value().hilite )
            {
                _drawRect.setSize(it.value().clickSize);
                p.fillRect(_drawRect, QColor(255, 155, 55, 190));
            }
        }
    }

    if( ! pArea->gridMode )
    {
        int customLineDestinationTarget = 0;
        if( mCustomLinesRoomTo > 0 )
        {
            customLineDestinationTarget = mCustomLinesRoomTo;
        }
        else if( mCustomLineSelectedRoom > 0 && ! mCustomLineSelectedExit.isEmpty() )
        {
            TRoom * pSR = mpMap->mpRoomDB->getRoom( mCustomLineSelectedRoom );
            if( pSR )
            {
                if( mCustomLineSelectedExit == "NW" || mCustomLineSelectedExit == "nw" )
                    customLineDestinationTarget = pSR->getNorthwest();
                else if( mCustomLineSelectedExit == "N" || mCustomLineSelectedExit == "n" )
                    customLineDestinationTarget = pSR->getNorth();
                else if( mCustomLineSelectedExit == "NE" || mCustomLineSelectedExit == "ne" )
                    customLineDestinationTarget = pSR->getNortheast();
                else if( mCustomLineSelectedExit == "UP" || mCustomLineSelectedExit == "up" )
                    customLineDestinationTarget = pSR->getUp();
                else if( mCustomLineSelectedExit == "W" || mCustomLineSelectedExit == "w" )
                    customLineDestinationTarget = pSR->getWest();
                else if( mCustomLineSelectedExit == "E" || mCustomLineSelectedExit == "e" )
                    customLineDestinationTarget = pSR->getEast();
                else if( mCustomLineSelectedExit == "DOWN" || mCustomLineSelectedExit == "down" )
                    customLineDestinationTarget = pSR->getDown();
                else if( mCustomLineSelectedExit == "SW" || mCustomLineSelectedExit == "sw" )
                    customLineDestinationTarget = pSR->getSouthwest();
                else if( mCustomLineSelectedExit == "S" || mCustomLineSelectedExit == "s" )
                    customLineDestinationTarget = pSR->getSouth();
                else if( mCustomLineSelectedExit == "SE" || mCustomLineSelectedExit == "se" )
                    customLineDestinationTarget = pSR->getSoutheast();
                else if( mCustomLineSelectedExit == "IN" || mCustomLineSelectedExit == "in" )
                    customLineDestinationTarget = pSR->getIn();
                else if( mCustomLineSelectedExit == "OUT" || mCustomLineSelectedExit == "out" )
                    customLineDestinationTarget = pSR->getOut();
                else
                {
                    QMultiMap<int, QString> otherExits = pSR->getOtherMap();
                    QMapIterator<int, QString> otherExitIt = otherExits;
                    while( otherExitIt.hasNext() )
                    {
                        otherExitIt.next();
                        if( otherExitIt.value().startsWith("0") || otherExitIt.value().startsWith("1") )
                        {
                            if( otherExitIt.value().mid(1) == mCustomLineSelectedExit )
                            {
                                customLineDestinationTarget = otherExitIt.key();
                                break;
                            }
                        }
                        else if( otherExitIt.value() == mCustomLineSelectedExit )
                        {
                            customLineDestinationTarget = otherExitIt.key();
                            break;
                        }
                    }
                }
            }
        }
        QSetIterator<int> itRoom2( pArea->getAreaRooms() );
        while( itRoom2.hasNext() )
        {
            int _id = itRoom2.next();
            TRoom * pR = mpMap->mpRoomDB->getRoom( _id );
            if( !pR ) continue;
            float rx = pR->x*tx+_rx;
            float ry = pR->y*-1*ty+_ry;
            int rz = pR->z;

            if( rz != zEbene ) continue;

            if( pR->customLines.size() == 0 )
            {
                if( rx < 0 || ry < 0 || rx > _w || ry > _h ) continue;
            }
            else
            {
                float miny = pR->min_y*-1*ty+(float)_ry;
                float maxy = pR->max_y*-1*ty+(float)_ry;
                float minx = pR->min_x*tx+(float)_rx;
                float maxx = pR->max_x*tx+(float)_rx;

                if( !( (minx>0.0 || maxx>0.0) && ((float)_w>minx || (float)_w>maxx) ) )
                {
                    continue;
                }
                if( !( (miny>0.0 || maxy>0.0) && ((float)_h>miny || (float)_h>maxy) ) )
                {
                    continue;
                }
            }

            pR->rendered = true;

            exitList.clear();
            oneWayExits.clear();
            if( pR->customLines.size() > 0 )
            {
                if( ! pR->customLines.contains("N") )
                {
                    exitList.push_back( pR->getNorth() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getNorth());
                    if( pER )
                    {
                        if( pER->getSouth() != _id )
                        {
                            oneWayExits.push_back( pR->getNorth() );
                        }
                    }
                }
                if( !pR->customLines.contains("NW") )
                {
                    exitList.push_back( pR->getNorthwest() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getNorthwest());
                    if( pER )
                    {
                        if( pER->getSoutheast() != _id )
                        {
                            oneWayExits.push_back( pR->getNorthwest() );
                        }
                    }
                }
                if( !pR->customLines.contains("E") )
                {
                    exitList.push_back( pR->getEast() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getEast());
                    if( pER )
                    {
                        if( pER->getWest() != _id )
                        {
                            oneWayExits.push_back( pR->getEast() );
                        }
                    }
                }
                if( !pR->customLines.contains("SE") )
                {
                    exitList.push_back( pR->getSoutheast() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getSoutheast());
                    if( pER )
                    {
                        if( pER->getNorthwest() != _id )
                        {
                            oneWayExits.push_back( pR->getSoutheast() );
                        }
                    }
                }
                if( !pR->customLines.contains("S") )
                {
                    exitList.push_back( pR->getSouth() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getSouth());
                    if( pER )
                    {
                        if( pER->getNorth() != _id )
                        {
                            oneWayExits.push_back( pR->getSouth() );
                        }
                    }
                }
                if( !pR->customLines.contains("SW") )
                {
                    exitList.push_back( pR->getSouthwest() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getSouthwest());
                    if( pER )
                    {
                        if( pER->getNortheast() != _id )
                        {
                            oneWayExits.push_back( pR->getSouthwest() );
                        }
                    }
                }
                if( !pR->customLines.contains("W") )
                {
                    exitList.push_back( pR->getWest() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getWest());
                    if( pER )
                    {
                        if( pER->getEast() != _id )
                        {
                            oneWayExits.push_back( pR->getWest() );
                        }
                    }
                }
                if( !pR->customLines.contains("NE") )
                {
                    exitList.push_back( pR->getNortheast() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getNortheast());
                    if( pER )
                    {
                        if( pER->getSouthwest() != _id )
                        {
                            oneWayExits.push_back( pR->getNortheast() );
                        }
                    }
                }
            }
            else
            {
                if( pR->getNorth() > 0 )
                {
                    exitList.push_back( pR->getNorth() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getNorth());
                    if( pER )
                    {
                        if( pER->getSouth() != _id )
                        {
                            oneWayExits.push_back( pR->getNorth() );
                        }
                    }
                }
                if( pR->getNorthwest() > 0 )
                {
                    exitList.push_back( pR->getNorthwest() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getNorthwest());
                    if( pER )
                    {
                        if( pER->getSoutheast() != _id )
                        {
                            oneWayExits.push_back( pR->getNorthwest() );
                        }
                    }
                }
                if( pR->getEast() > 0 )
                {
                    exitList.push_back( pR->getEast() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getEast());
                    if( pER )
                    {
                        if( pER->getWest() != _id )
                        {
                            oneWayExits.push_back( pR->getEast() );
                        }
                    }
                }
                if( pR->getSoutheast() > 0 )
                {
                    exitList.push_back( pR->getSoutheast() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getSoutheast());
                    if( pER )
                    {
                        if( pER->getNorthwest() != _id )
                        {
                            oneWayExits.push_back( pR->getSoutheast() );
                        }
                    }
                }
                if( pR->getSouth() > 0 )
                {
                    exitList.push_back( pR->getSouth() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getSouth());
                    if( pER )
                    {
                        if( pER->getNorth() != _id )
                        {
                            oneWayExits.push_back( pR->getSouth() );
                        }
                    }
                }
                if( pR->getSouthwest() > 0 )
                {
                    exitList.push_back( pR->getSouthwest() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getSouthwest());
                    if( pER )
                    {
                        if( pER->getNortheast() != _id )
                        {
                            oneWayExits.push_back( pR->getSouthwest() );
                        }
                    }
                }
                if( pR->getWest() > 0 )
                {
                    exitList.push_back( pR->getWest() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getWest());
                    if( pER )
                    {
                        if( pER->getEast() != _id )
                        {
                            oneWayExits.push_back( pR->getWest() );
                        }
                    }
                }
                if( pR->getNortheast() > 0 )
                {
                    exitList.push_back( pR->getNortheast() );
                    TRoom * pER = mpMap->mpRoomDB->getRoom(pR->getNortheast());
                    if( pER )
                    {
                        if( pER->getSouthwest() != _id )
                        {
                            oneWayExits.push_back( pR->getNortheast() );
                        }
                    }
                }
            }

            if( pR->customLines.size() > 0 )
            {
                QPen oldPen = p.pen();
                QMapIterator<QString, QList<QPointF> > itk(pR->customLines);
                while( itk.hasNext() )
                {
                    itk.next();
                    QColor _color;
                    if( _id == mCustomLineSelectedRoom && itk.key()== mCustomLineSelectedExit )
                    {
                        _color.setRed( 255 );
                        _color.setGreen( 155 );
                        _color.setBlue( 55 );
                    }
                    else if( pR->customLinesColor[itk.key()].size() == 3 )
                    {
                        _color.setRed( pR->customLinesColor[itk.key()][0] );
                        _color.setGreen( pR->customLinesColor[itk.key()][1] );
                        _color.setBlue( pR->customLinesColor[itk.key()][2] );
                    }
                    else
                        _color = QColor(Qt::red);
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
                    customLinePen.setCapStyle( Qt::RoundCap );
                    customLinePen.setJoinStyle( Qt::RoundJoin );

                    if( _style == "solid line" )
                        customLinePen.setStyle( Qt::SolidLine );
                    else if( _style == "dot line" )
                        customLinePen.setStyle( Qt::DotLine );
                    else if( _style == "dash line" )
                        customLinePen.setStyle( Qt::DashLine );
                    else if( _style == "dash dot line")
                        customLinePen.setStyle( Qt::DashDotLine );
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

                        if( _id == mCustomLineSelectedRoom && itk.key()== mCustomLineSelectedExit )
                        {
                            QPen _savedPen = p.pen();
                            QPen _pen;
                            QBrush _brush = p.brush();
                            if( pk == mCustomLineSelectedPoint )
                                _pen = QPen( QColor(255,255,55),
                                             _savedPen.width(),
                                             Qt::SolidLine,
                                             Qt::FlatCap,
                                             _savedPen.joinStyle() ); // Draw the selected point in yellow not orange.
                            else
                                _pen = QPen( _savedPen.color(),
                                             _savedPen.width(),
                                             Qt::SolidLine,
                                             Qt::FlatCap,
                                             _savedPen.joinStyle() );
                            p.setBrush(Qt::NoBrush); // Draw hollow circles not default filled ones!
                            p.setPen(_pen);
                            p.drawEllipse( _cendP, mTX/4, mTX/4 );
                            p.setPen(_savedPen);
                            p.setBrush(_brush);
                        }

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

// N/U:             int e = pR->z;

            // draw exit stubs
            QMap<int, QVector3D> unitVectors = mpMap->unitVectors;
            for(int direction : pR->exitStubs)
            {
                QVector3D uDirection = unitVectors[direction];
                p.drawLine(rx+rSize*(int)uDirection.x()/2, ry+rSize*(int)uDirection.y(),rx+(int)uDirection.x()*(rSize*3/4*tx), ry+uDirection.y()*(rSize*3/4*ty));
            }

            QPen __pen;
            for(int & k : exitList)
            {
                int rID = k;
                if( rID <= 0 ) continue;

                bool areaExit;

                TRoom * pE = mpMap->mpRoomDB->getRoom(rID);
                if( !pE ) continue;

                if( pE->getArea() != mAID )
                {
                    areaExit = true;
                }
                else
                    areaExit = false;
                float ex = pE->x*tx+_rx;
                float ey = pE->y*ty*-1+_ry;
                int ez = pE->z;

                QVector3D p1( ex, ey, ez );
                QVector3D p2( rx, ry, rz );
                QLine _line;
                if( ! areaExit )
                {
                    // one way exit or 2 way exit?
                    if( ! oneWayExits.contains( rID ) )
                    {
                        p.drawLine( (int)p1.x(), (int)p1.y(), (int)p2.x(), (int)p2.y() );
                    }
                    else
                    {
                        // one way exit draw arrow

                        QLineF l0 = QLineF( p2.x(), p2.y(), p1.x(), p1.y() );
                        QLineF k0 = l0;
                        k0.setLength( (l0.length()-wegBreite*5)*0.5 );
                        qreal dx = k0.dx(); qreal dy = k0.dy();
                        QPen _tp = p.pen();
                        QPen _tp2 = _tp;
                        _tp2.setStyle(Qt::DotLine);
                        p.setPen(_tp2);
                        p.drawLine( l0 );
                        p.setPen(_tp);
                        l0.setLength(wegBreite*5);
                        QPointF _p1 = l0.p2();
                        QPointF _p2 = l0.p1();
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
                        brush.setColor( QColor(255,100,100) );
                        brush.setStyle( Qt::SolidPattern );
                        QPen arrowPen = p.pen();
                        arrowPen.setCosmetic( mMapperUseAntiAlias );
                        arrowPen.setStyle(Qt::SolidLine);
                        p.setPen( arrowPen );
                        p.setBrush( brush );
                        p.drawPolygon(_poly.translated(dx,dy));
                    }

                }
                else
                {
                    __pen = p.pen();
                    QPoint _p;
                    pen = p.pen();
                    pen.setWidthF(wegBreite);
                    pen.setCosmetic( mMapperUseAntiAlias );
                    pen.setColor(getColor(k));
                    p.setPen( pen );
                    if( pR->getSouth() == rID )
                    {
                        _line = QLine( p2.x(), p2.y()+ty,p2.x(), p2.y() );
                        _p = QPoint(p2.x(), p2.y()+ty/2);
                    }
                    else if( pR->getNorth() == rID )
                    {
                        _line = QLine( p2.x(), p2.y()-ty, p2.x(), p2.y() );
                        _p = QPoint(p2.x(), p2.y()-ty/2);
                    }
                    else if( pR->getWest() == rID )
                    {
                        _line = QLine(p2.x()-tx, p2.y(),p2.x(), p2.y() );
                        _p = QPoint(p2.x()-tx/2, p2.y());
                    }
                    else if( pR->getEast() == rID )
                    {
                        _line = QLine(p2.x()+tx, p2.y(),p2.x(), p2.y() );
                        _p = QPoint(p2.x()+tx/2, p2.y());
                    }
                    else if( pR->getNorthwest() == rID )
                    {
                        _line = QLine(p2.x()-tx, p2.y()-ty,p2.x(), p2.y() );
                        _p = QPoint(p2.x()-tx/2, p2.y()-ty/2);
                    }
                    else if( pR->getNortheast() == rID )
                    {
                        _line = QLine(p2.x()+tx, p2.y()-ty,p2.x(), p2.y());
                        _p = QPoint(p2.x()+tx/2, p2.y()-ty/2);
                    }
                    else if( pR->getSoutheast() == rID )
                    {
                        _line = QLine(p2.x()+tx, p2.y()+ty, p2.x(), p2.y());
                        _p = QPoint(p2.x()+tx/2, p2.y()+ty/2);
                    }
                    else if( pR->getSouthwest() == rID )
                    {
                        _line = QLine(p2.x()-tx, p2.y()+ty, p2.x(), p2.y());
                        _p = QPoint(p2.x()-tx/2, p2.y()+ty/2);
                    }
                    p.drawLine( _line );
                    mAreaExitList[k] = _p;
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
                    brush.setColor( getColor(k) );
                    brush.setStyle( Qt::SolidPattern );
                    QPen arrowPen = p.pen();
                    arrowPen.setCosmetic( mMapperUseAntiAlias );
                    p.setPen( arrowPen );
                    p.setBrush( brush );
                    p.drawPolygon(_poly);
                    p.setPen( __pen );
                }
                // doors
                if( pR->doors.size() > 0 )
                {
                    int doorStatus = 0;
                    if( pR->getSouth() == rID && pR->doors.contains("s") )
                    {
                        doorStatus = pR->doors["s"];
                    }
                    else if( pR->getNorth() == rID && pR->doors.contains("n") )
                    {
                        doorStatus = pR->doors["n"];
                    }
                    else if( pR->getSouthwest() == rID && pR->doors.contains("sw") )
                    {
                        doorStatus = pR->doors["sw"];
                    }
                    else if( pR->getSoutheast() == rID && pR->doors.contains("se") )
                    {
                        doorStatus = pR->doors["se"];
                    }
                    else if( pR->getNortheast() == rID && pR->doors.contains("ne") )
                    {
                        doorStatus = pR->doors["ne"];
                    }
                    else if( pR->getNorthwest() == rID && pR->doors.contains("nw") )
                    {
                        doorStatus = pR->doors["nw"];
                    }
                    else if( pR->getWest() == rID && pR->doors.contains("w") )
                    {
                        doorStatus = pR->doors["w"];
                    }
                    else if( pR->getEast() == rID && pR->doors.contains("e") )
                    {
                        doorStatus = pR->doors["e"];
                    }
                    if( doorStatus > 0 )
                    {
                        QLineF k0;
                        QRectF rect;
                        rect.setWidth(0.25*mTX);
                        rect.setHeight(0.25*mTY);
                        if ( areaExit )
                            k0 = QLineF(_line);
                        else
                            k0 = QLineF( p2.x(), p2.y(), p1.x(), p1.y() );
                        k0.setLength( (k0.length())*0.5 );
                        rect.moveCenter(k0.p2());
                        QPen arrowPen = p.pen();
                        QPen _tp = p.pen();
                        arrowPen.setCosmetic( mMapperUseAntiAlias );
                        arrowPen.setStyle(Qt::SolidLine);
                        if( doorStatus == 1 ) //open door
                            arrowPen.setColor(QColor(10,155,10));
                        else if( doorStatus == 2 ) //closed door
                            arrowPen.setColor(QColor(155,155,10));
                        else //locked door
                            arrowPen.setColor(QColor(155,10,10));
                        QBrush brush;
                        QBrush oldBrush;
                        p.setPen( arrowPen );
                        p.setBrush(brush);
                        p.drawRect(rect);
                        p.setBrush(oldBrush);
                        p.setPen(_tp);
                    }
                }
            } // End of for( exitList )

            // Indicate destination for custom exit line drawing - double size
            // target yellow hollow circle
            // Similar code is now also used to indicate center target of
            // multiple selected rooms but that must be done after all the rooms
            // have been drawn otherwise later drawn rooms will overwrite the
            // mark, especially on areas in gridmode.
            if( customLineDestinationTarget > 0 && customLineDestinationTarget == _id ) {
                QPen savePen = p.pen();
                QBrush saveBrush = p.brush();
                float _radius = tx * 1.2;
                float _diagonal = tx * 1.2;
                QPointF _center = QPointF( rx, ry );

                QPen myPen( QColor( 255, 255, 50, 192) );  // Quarter opaque yellow pen
                myPen.setWidth(tx * 0.1);
                QPainterPath myPath;
                p.setPen( myPen );
                p.setBrush( Qt::NoBrush);
                myPath.addEllipse( _center, _radius, _radius );
                myPath.addEllipse( _center, _radius / 2.0, _radius / 2.0 );
                myPath.moveTo( rx - _diagonal, ry - _diagonal );
                myPath.lineTo( rx + _diagonal, ry + _diagonal );
                myPath.moveTo( rx + _diagonal, ry - _diagonal );
                myPath.lineTo( rx - _diagonal, ry + _diagonal );
                p.drawPath( myPath );
                p.setPen( savePen );
                p.setBrush( saveBrush );
            }
        } // End of for( area Rooms )
    } // End of NOT area gridmode

    // Draw label sizing or group selection box
    if( mSizeLabel )
        p.fillRect(mMultiRect,QColor(250,190,0,190));
    else
        p.fillRect(mMultiRect,QColor(190,190,190,60));

    QSetIterator<int> itRoom( pArea->getAreaRooms() );
    while( itRoom.hasNext() )
    {
        int currentAreaRoom = itRoom.next();
        TRoom * pR = mpMap->mpRoomDB->getRoom( currentAreaRoom );
        if( !pR )
            continue; // Was missing this safety step to skip missing rooms
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
            break;
        default: //user defined room color
            if( mpMap->customEnvColors.contains(env) )
            {
                c = mpMap->customEnvColors[env];
            }
        }
        if( ( ( mPick || __Pick )
              && mPHighlight.x() >= dr.x()-(tx*rSize)
              && mPHighlight.x() <= dr.x()+(tx*rSize)
              && mPHighlight.y() >= dr.y()-(ty*rSize)
              && mPHighlight.y() <= dr.y()+(ty*rSize) )
            || mMultiSelectionSet.contains( currentAreaRoom ) ) {
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
                QPen myPen(Qt::transparent);
                QPainterPath myPath;
                p.setBrush(_gradient);
                p.setPen(myPen);
                myPath.addEllipse(_center,_radius,_radius);
                p.drawPath(myPath);


                mTarget = currentAreaRoom;
                if( mpMap->mpRoomDB->getRoom(mTarget) )
                {
                    mpMap->mTargetID = mTarget;
                    if( mpMap->findPath( mpMap->mRoomIdHash.value( mpHost->getName() ), mpMap->mTargetID) )
                    {
                        mpHost->startSpeedWalk();
                    }
                    else
                    {
                        QString msg = "Mapper: Cannot find a path to this room using known exits.\n";
                        mpHost->mpConsole->printSystemMessage(msg);
                    }
                }
            }
        }
        else
        {
            char _ch = pR->c;
            if( _ch >= 33 /* && _ch < 255 seems that _ch is a signed char so will always be less than 255 */)
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
                    QPen myPen(Qt::transparent);
                    QPainterPath myPath;
                    p.setBrush(_gradient);
                    p.setPen(myPen);
                    myPath.addEllipse(_center,_radius,_radius);
                    p.drawPath(myPath);
                }
                else
                {
                    p.fillRect(dr,c);
                }
            }

            if( pR->highlight )
            {
                float _radius = (pR->highlightRadius*tx)/2;
                QPointF _center = QPointF(rx,ry);
                QRadialGradient _gradient(_center,_radius);
                _gradient.setColorAt(0.85, pR->highlightColor);
                _gradient.setColorAt(0, pR->highlightColor2 );
                QPen myPen(Qt::transparent);
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
                    lc=QColor(Qt::black);
                else
                    lc=QColor(Qt::white);
                p.setPen(QPen(lc));
                p.drawText(dr, Qt::AlignHCenter|Qt::AlignVCenter,QString::number( currentAreaRoom ));
                p.setPen(__pen);
            }

            if( mShiftMode && currentAreaRoom == mpMap->mRoomIdHash.value( mpHost->getName() ) )
            {
                float _radius = (1.2*tx)/2;
                QPointF _center = QPointF(rx,ry);
                QRadialGradient _gradient(_center,_radius);
                _gradient.setColorAt(0.95, QColor(255,0,0,150));
                _gradient.setColorAt(0.80, QColor(150,100,100,150));
                _gradient.setColorAt(0.799,QColor(150,100,100,100));
                _gradient.setColorAt(0.7, QColor(255,0,0,200));
                _gradient.setColorAt(0, QColor(255,255,255,255));
                QPen myPen(Qt::transparent);
                QPainterPath myPath;
                p.setBrush(_gradient);
                p.setPen(myPen);
                myPath.addEllipse(_center,_radius,_radius);
                p.drawPath(myPath);

            }
        }

        QColor lc;
        if( c.red()+c.green()+c.blue() > 200 )
            lc=QColor(Qt::black);
        else
            lc=QColor(Qt::white);
        pen = p.pen();
        pen.setColor( lc );
        pen.setWidthF(0);//wegBreite?);
        pen.setCosmetic( mMapperUseAntiAlias );
        pen.setCapStyle( Qt::RoundCap );
        pen.setJoinStyle( Qt::RoundJoin );
        p.setPen( pen );

        //FIXME: redo exit stubs here since the room will draw over up/down stubs -- its repetitive though
        QMap<int, QVector3D> unitVectors = mpMap->unitVectors;
        for( int k=0; k<pR->exitStubs.size(); k++ )
        {
            int direction = pR->exitStubs[k];
            QVector3D uDirection = unitVectors[direction];
            if (direction > 8)
            {
                float rx = pR->x*tx+_rx;
                float ry = pR->y*-1*ty+_ry;
                QPolygonF _poly;
                QPointF _pt;
                _pt = QPointF( rx, ry+(ty*rSize)*uDirection.z()/20 );
                _poly.append( _pt );
                _pt = QPointF( rx+(tx*rSize)/3.1, ry+(ty*rSize)*uDirection.z()/3.1);
                _poly.append(_pt);
                _pt = QPointF( rx-(tx*rSize)/3.1, ry+(ty*rSize)*uDirection.z()/3.1 );
                _poly.append( _pt );
                QBrush brush = p.brush();
                brush.setColor( Qt::black );
                brush.setStyle( Qt::NoBrush );
                p.setBrush( brush );
                p.drawPolygon(_poly);
            }
        }

        if( pR->getUp() > 0 )
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
            brush.setColor( Qt::black );
            brush.setStyle( Qt::SolidPattern );
            p.setBrush( brush );
            p.drawPolygon(_poly);
        }

        if( pR->getDown() > 0 )
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
            brush.setColor( Qt::black );
            brush.setStyle( Qt::SolidPattern );
            p.setBrush( brush );
            p.drawPolygon(_poly);
        }

        if( pR->getIn() > 0 )
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
            brush.setColor( Qt::black );
            brush.setStyle( Qt::SolidPattern );
            p.setBrush( brush );
            p.drawPolygon(_poly);
        }

        if( pR->getOut() > 0 )
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
            brush.setColor( Qt::black );
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

               QRectF dr = QRectF(rx-tx/2, ry-ty/2,tx,ty);
               if( ( (mPick || __Pick)
                     && mPHighlight.x() >= dr.x()-tx/2
                     && mPHighlight.x() <= dr.x()+tx/2
                     && mPHighlight.y() >= dr.y()-ty/2
                     && mPHighlight.y() <= dr.y()+ty/2 )
                   || mMultiSelectionSet.contains( currentAreaRoom ) ) {
                   p.fillRect(dr,QColor(50,255,50));
                   mPick = false;
                   mTarget = it.key();
                   if( mpMap->mpRoomDB->getRoom(mTarget) )
                   {
                       mpMap->mTargetID = mTarget;
                       if( mpMap->findPath( mpMap->mRoomIdHash.value( mpHost->getName() ), mpMap->mTargetID) )
                       {
                            mpHost->startSpeedWalk();
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

                QRectF dr = QRectF(rx,ry,tx*rSize,ty*rSize);//rx-(tx*rSize)/2,ry-(ty*rSize)/2,tx*rSize,ty*rSize);
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
                    QPen myPen(Qt::transparent);
                    QPainterPath myPath;
                    p.setBrush(_gradient);
                    p.setPen(myPen);
                    myPath.addEllipse(_center,_radius,_radius);
                    p.drawPath(myPath);

                    mPick = false;
                    mTarget = it.key();
                    if( mpMap->mpRoomDB->getRoom(mTarget) )
                    {
                        mpMap->mTargetID = mTarget;
                        if( mpMap->findPath( mpMap->mRoomIdHash.value( mpHost->getName() ), mpMap->mTargetID) )
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

    if( mpMap->mapLabels.contains( mAID ) )
    {
        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAID]);
        while( it.hasNext() )
        {
            it.next();
            if( it.value().pos.z() != mOz ) continue;
            if( it.value().text.length() < 1 )
            {
                mpMap->mapLabels[mAID][it.key()].text = "no text";
            }
            QPointF lpos;
            int _lx = it.value().pos.x()*tx+_rx;
            int _ly = it.value().pos.y()*ty*-1+_ry;

            lpos.setX( _lx );
            lpos.setY( _ly );
            int _lw = abs(it.value().size.width())*tx;
            int _lh = abs(it.value().size.height())*ty;

            if( ! ( ( 0<_lx || 0<_lx+_lw ) && (_w>_lx || _w>_lx+_lw ) ) ) continue;
            if( ! ( ( 0<_ly || 0<_ly+_lh ) && (_h>_ly || _h>_ly+_lh ) ) ) continue;
            QRectF _drawRect = QRect(it.value().pos.x()*tx+_rx, it.value().pos.y()*ty*-1+_ry, _lw, _lh);
            if( it.value().showOnTop )
            {
                if( ! it.value().noScaling )
                {
                    p.drawPixmap( lpos, it.value().pix.scaled(_drawRect.size().toSize()) );
                    mpMap->mapLabels[mAID][it.key()].clickSize.setWidth(_drawRect.width());
                    mpMap->mapLabels[mAID][it.key()].clickSize.setHeight(_drawRect.height());
                }
                else
                {
                    p.drawPixmap( lpos, it.value().pix );
                    mpMap->mapLabels[mAID][it.key()].clickSize.setWidth(it.value().pix.width());
                    mpMap->mapLabels[mAID][it.key()].clickSize.setHeight(it.value().pix.height());
                }
            }
            if( it.value().hilite )
            {
                _drawRect.setSize(it.value().clickSize);
                p.fillRect(_drawRect, QColor(255, 155, 55, 190));
            }
        }
    }

    // Similar code was used to indicate target of custom exit line selected for
    // editing but this could not be done there because gridmode areas don't hit
    // that bit of code and later rooms would overwrite the target...
    if( mMultiSelectionHighlightRoomId > 0 && mMultiSelectionSet.size() > 1 ) {
        TRoom * pR_multiSelectionHighlight = mpMap->mpRoomDB->getRoom( mMultiSelectionHighlightRoomId );
        if( pR_multiSelectionHighlight ) {
            float r_mSx = pR_multiSelectionHighlight->x   *tx+_rx;
            float r_mSy = pR_multiSelectionHighlight->y*-1*ty+_ry;
            QPen savePen = p.pen();
            QBrush saveBrush = p.brush();
            float _radius = tx * 1.2;
            float _diagonal = tx * 1.2;
            QPointF _center = QPointF( r_mSx, r_mSy );

            QPen myPen( QColor( 255, 255, 50, 192 ) );  // Quarter opaque yellow pen
            myPen.setWidth( tx * 0.1 );
            QPainterPath myPath;
            p.setPen( myPen );
            p.setBrush( Qt::NoBrush );
            myPath.addEllipse( _center, _radius, _radius );
            myPath.addEllipse( _center, _radius / 2.0, _radius / 2.0 );
            myPath.moveTo( r_mSx - _diagonal, r_mSy - _diagonal );
            myPath.lineTo( r_mSx + _diagonal, r_mSy + _diagonal );
            myPath.moveTo( r_mSx + _diagonal, r_mSy - _diagonal );
            myPath.lineTo( r_mSx - _diagonal, r_mSy + _diagonal );
            p.drawPath( myPath );
            p.setPen( savePen );
            p.setBrush( saveBrush );
        }
    }

    QColor infoColor;
    if( mpHost->mBgColor_2.lightness() > 127 ) {
        infoColor=QColor( Qt::black );
    }
    else {
        infoColor=QColor( Qt::white );
    }

    // Draw central red circle:
    if( ! mShiftMode )
    {
        p.save();
        QPen myPen(Qt::transparent);
        QPainterPath myPath;
        if( mpHost->mMapStrongHighlight )
        {   // Never set, no means to except via XMLImport, as dlgMapper class's
            // slot_toggleStrongHighlight is not wired up to anything
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
            p.setBrush(_gradient);
            p.setPen(myPen);
            myPath.addEllipse(_center,_radius,_radius);
        }
        else
        {
            float _radius = (1.9*tx)/2;
            QPointF _center = QPointF(px,py);
            QRadialGradient _gradient(_center,_radius);
            _gradient.setColorAt(0.95, QColor(255,0,0,150));
            _gradient.setColorAt(0.80, QColor(150,100,100,150));
            _gradient.setColorAt(0.799,QColor(150,100,100,100));
            _gradient.setColorAt(0.3,QColor(150,150,150,100));
            _gradient.setColorAt(0.1, QColor(255,255,255,100));
            _gradient.setColorAt(0, QColor(255,255,255,255));
            p.setBrush(_gradient);
            p.setPen(myPen);
            myPath.addEllipse(_center,_radius,_radius);
        }
        p.drawPath(myPath);
        p.restore();
    }


    // Work out text for information box, need to offset if room selection widget is present
    if( mShowInfo )
    {
        QString infoText;
        int __rid = mRID;
        if( !isCenterViewCall && mMultiSelectionSet.size() )
        {
            if( mpMap->mpRoomDB->getRoom( *(mMultiSelectionSet.constBegin()) ) )
            {
                __rid = mMultiSelectionHighlightRoomId;
            }
        }

        p.save(); // Save painter state
        QFont f = p.font();
        TRoom * _prid = mpMap->mpRoomDB->getRoom( __rid );
        if( _prid )
        {
            int _iaid = _prid->getArea();
            TArea * _paid = mpMap->mpRoomDB->getArea( _iaid );
            QString _paid_name = mpMap->mpRoomDB->getAreaNamesMap().value( _iaid );
            if( _paid )
            {
                infoText = tr( "Area: %1 ID:%2 x:%3 <-> %4 y:%5 <-> %6 z:%7 <-> %8\n").arg(_paid_name, QString::number(_iaid), QString::number(_paid->min_x), QString::number(_paid->max_x), QString::number(_paid->min_y), QString::number(_paid->max_y), QString::number(_paid->min_z), QString::number((_paid->max_z)));
            }
            else
            {
                infoText = QStringLiteral( "\n" );
            }

            if( ! _prid->name.isEmpty() )
            {
                infoText.append( tr("Room Name: %1\n").arg(_prid->name) );
            }

            uint selectionSize = mMultiSelectionSet.size();
            // Italicise the text if the current display area {mAID} is not the
            // same as the displayed text information - which happens when NO
            // room is selected AND the current area is NOT the one the player
            // is in (to emphasis that the displayed data is {mostly} not about
            // the CURRENTLY VISIBLE area)... make it bold if the player room IS
            // in the displayed map

            // If one or more rooms are selected - make the text slightly orange.
            switch( selectionSize )
            {
            case 0:
                infoText.append( tr("Room ID: %1 (Current) Position on Map: (%2,%3,%4)\n").arg(QString::number(__rid), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z)) );
                if( playerArea != mAID ) {
                    f.setItalic( true );
                }
                else {
                    f.setBold( true );
                }
                break;
            case 1:
                infoText.append( tr("Room ID: %1 (Selected) Position on Map: (%2,%3,%4)\n").arg(QString::number(__rid), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z)) );
                f.setBold( true );
                if( infoColor.lightness() > 127 ) {
                    infoColor = QColor( 255, 223, 191 ); // Slightly orange white
                }
                else {
                    infoColor = QColor( 96, 48, 0 ); // Dark, slightly orange grey
                }
                break;
            default:
                infoText.append( tr("Room ID: %1 (%5 Selected) Position on Map: (%2,%3,%4)\n").arg(QString::number(__rid), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z), QString::number(selectionSize)) );
                f.setBold( true );
                if( infoColor.lightness() > 127 ) {
                    infoColor = QColor( 255, 223, 191 ); // Slightly orange white
                }
                else {
                    infoColor = QColor( 96, 48, 0 ); // Dark, slightly orange grey
                }
                break;
            }
        }

        infoText.append( tr("render time: %1S mO: (%2,%3,%4)").arg( __time.nsecsElapsed() * 1.0e-9,0,'f',3).arg(QString::number(mOx), QString::number(mOy), QString::number(mOz)) );

        uint infoLeftSideAvoid = 10; // Left margin for info widget
        if( mMultiSelectionListWidget.isVisible() ) { // Room Selection Widget showing, so increase margin to avoid
            infoLeftSideAvoid += mMultiSelectionListWidget.x() + mMultiSelectionListWidget.rect().width();
        }

        uint infoHeight = 5 + mFontHeight; // Account for first iteration
        QRect testRect;
        // infoRect has a 10 margin on either side and on top to widget frame.
        mMapInfoRect = QRect( infoLeftSideAvoid, 10, width() - 10 - infoLeftSideAvoid, infoHeight );
        do {
            infoHeight += mFontHeight;
            mMapInfoRect.setHeight( infoHeight );
            // Test in a rectangle that is 10 less on all sides:
            testRect = p.boundingRect( mMapInfoRect.left()+10, mMapInfoRect.top()+10, mMapInfoRect.width()-20, mMapInfoRect.height()-20, Qt::TextWordWrap|Qt::AlignLeft|Qt::AlignTop, infoText );
        } while( ( testRect.height() > mMapInfoRect.height()-20
                   || testRect.width()  > mMapInfoRect.width()-20 )
                 && infoHeight < height() ); // Last term is needed to prevent runaway under "odd" conditions

        p.fillRect( mMapInfoRect, QColor(150,150,150,80) ); // Restore Grey translucent background, was useful for debugging!
        p.setPen( infoColor );
        p.setFont( f );
        p.drawText( mMapInfoRect.left()+10, mMapInfoRect.top()+10, mMapInfoRect.width()-20, mMapInfoRect.height()-20, Qt::TextWordWrap|Qt::AlignLeft|Qt::AlignTop, infoText );
        p.restore(); //forget about font size changing and bolding/italicisation
    }

    static bool isAreaWidgetValid = true; // Remember between uses
    QFont _f =  mpMap->mpMapper->showArea->font();
    if( isAreaWidgetValid ) {
        if(     mAID == -1                                       // * the map being shown is the "default" area
                && ! mpMap->mpMapper->getDefaultAreaShown() ) {  // * the area widget is not showing the "default" area

            isAreaWidgetValid = false;                      // So the widget CANNOT indicate the correct area
            // Set the area widget to indicate the area widget is NOT
            // showing valid text - so make it italic and crossed out
            _f.setItalic( true );
            _f.setUnderline( true );
            _f.setStrikeOut( true );
            _f.setOverline( true );
        }
    }
    else {
        if( ! ( mAID == -1
                && ! mpMap->mpMapper->getDefaultAreaShown() ) ) {

            isAreaWidgetValid = true;                      // So the widget CAN now indicate the correct area
            // Reset to normal
            _f.setItalic( false );
            _f.setUnderline( false );
            _f.setStrikeOut( false );
            _f.setOverline( false );
        }
    }

    mpMap->mpMapper->showArea->setFont( _f );

    if( mHelpMsg.size() > 0 )
    {
        p.setPen(QColor(255,155,50));
        QFont _f = p.font();
        QFont _f2 = _f;
        _f.setPointSize(12); // 20 was a little large
        _f.setBold(true);
        p.setFont(_f);
        QRect _r = QRect(0,0,_w,_h);
        p.drawText( _r,
                    Qt::AlignHCenter|Qt::AlignBottom|Qt::TextWordWrap,
                    mHelpMsg );
        // Now draw text centered at bottom, so it does not clash with info window
        p.setFont(_f2);
    }

}



void T2DMap::mouseDoubleClickEvent ( QMouseEvent * event )
{
    if( mDialogLock )
        return;
    int x = event->x();
    int y = event->y();
    mPHighlight = QPoint(x,y);
    mPick = true;
    mStartSpeedWalk = true;
    repaint();
}

void T2DMap::createLabel( QRectF labelRect )
{
// N/U:     QRectF selectedRegion = labelRect;
    TMapLabel label;
    QFont _font;
    QString t = "no text";
    QString imagePath ;

    mHelpMsg.clear();

    QMessageBox msgBox;
    msgBox.setText("Text label or image label?");
    QPushButton *textButton = msgBox.addButton(tr("Text Label"), QMessageBox::ActionRole);
    QPushButton *imageButton = msgBox.addButton(tr("Image Label"), QMessageBox::ActionRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.exec();
    if( msgBox.clickedButton() == textButton)
    {
        QString title = "Enter label text.";
        _font = QFontDialog::getFont(0);
        t = QInputDialog::getText(0, title, title );
        if( t.length() < 1 ) t = "no text";
        label.text = t;
        label.bgColor = QColorDialog::getColor(QColor(50,50,150,100),0,"Background color");
        label.fgColor = QColorDialog::getColor(QColor(255,255,50,255),0,"Foreground color");
    }
    else if(msgBox.clickedButton() == imageButton)
    {
        label.bgColor = QColor(50,50,150,100);
        label.fgColor = QColor(255,255,50,255);
        label.text = "";
        imagePath = QFileDialog::getOpenFileName( 0, "Select image");
    }
    else
    {
        return;
    }

    QMessageBox msgBox2;
    msgBox2.setStandardButtons(QMessageBox::Cancel);
    msgBox2.setText("Draw label as background or on top of everything?");
    QPushButton *textButton2 = msgBox2.addButton(tr("Background"), QMessageBox::ActionRole);
    QPushButton *imageButton2 = msgBox2.addButton(tr("Foreground"), QMessageBox::ActionRole);
    msgBox2.exec();
    bool showOnTop = false;
    if( msgBox2.clickedButton() == textButton2 )
    {
        showOnTop = false;
    }
    else if( msgBox2.clickedButton() == imageButton2 )
    {
        showOnTop = true;
    }
    else
    {
        return;
    }

    label.showOnTop = showOnTop;
    QPixmap pix( abs(labelRect.width()), abs(labelRect.height()) );
    QRect drawRect = labelRect.normalized().toRect();
    drawRect.moveTo(0,0);
    //pix.fill(QColor(0,255,0,0));
    QPainter lp( &pix );
    QPen lpen;
    lp.setFont(_font);
    lpen.setColor( label.fgColor );
    lp.setPen( lpen );
    lp.fillRect( drawRect, label.bgColor );
    if( msgBox.clickedButton() == textButton)
       lp.drawText( drawRect, Qt::AlignHCenter|Qt::AlignCenter, t, 0);
    else
    {
        QPixmap imagePixmap = QPixmap(imagePath);
        lp.drawPixmap(QPoint(0,0), imagePixmap.scaled(drawRect.size()));
    }
    label.pix = pix.copy(drawRect);
    labelRect = labelRect.normalized();
    float mx = labelRect.topLeft().x()/mTX + mOx;
    float my = labelRect.topLeft().y()/mTY + mOy;
    mx = mx - xspan/2;
    my = yspan/2 - my;

    float mx2 = labelRect.bottomRight().x()/mTX + mOx;
    float my2 = labelRect.bottomRight().y()/mTY + mOy;
    mx2 = mx2 - xspan/2;
    my2 = yspan/2 - my2;
    label.pos = QVector3D( mx, my, mOz );
    label.size = QRectF(QPointF(mx,my),QPointF(mx2,my2)).normalized().size();
    if( ! mpMap->mpRoomDB->getArea(mAID) ) return;
    int labelID;
    if( ! mpMap->mapLabels.contains( mAID ) )
    {
        QMap<int, TMapLabel> m;
        m[0] = label;
        mpMap->mapLabels[mAID] = m;
    }
    else
    {
        labelID = mpMap->createMapLabelID( mAID );
        mpMap->mapLabels[mAID].insert(labelID, label);
    }
    update();
}

void T2DMap::mouseReleaseEvent(QMouseEvent * e )
{
    if( mMoveLabel )
    {
        mMoveLabel = false;
    }

    //move map with left mouse button + ALT (->
    if( mpMap->mLeftDown )
    {
        mpMap->mLeftDown = false;
    }

    if( e->button() & Qt::LeftButton )
    {
        mMultiSelection = false; // End drag-to-select rectangle resizing
        mHelpMsg.clear();
        if( mSizeLabel )
        {
            mSizeLabel = false;
            QRectF labelRect = mMultiRect;
            createLabel( labelRect );
        }
        mMultiRect = QRect(0,0,0,0);
        update();
    }
}

bool T2DMap::event( QEvent * event )
{
    //NOTE: key events aren't being forwarded to T2DMap because the widget currently never has focus because it's more comfortable for the user to always have focus on the command line
    //      If this were to be changed some day the setFocusPolicy() calls in the constructor need to be uncommented

    if( event->type() == QEvent::KeyPress )
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>( event );
        qDebug()<<"modifier="<<ke->modifiers()<<" key="<<ke->key();
//        if( ke->key() == Qt::Key_Delete )
//        {
//            if( mCustomLineSelectedRoom != 0  )
//            {
//                if( mpMap->rooms.contains(mCustomLineSelectedRoom) )
//                {
//                    TRoom * pR = mpMap->rooms[mCustomLineSelectedRoom];
//                    if( pR->customLines.contains( mCustomLineSelectedExit) )
//                    {
//                        pR->customLines.remove(mCustomLineSelectedExit);
//                        repaint();
//                        mCustomLineSelectedRoom = 0;
//                        mCustomLineSelectedExit = "";
//                        mCustomLineSelectedPoint = -1;
//                        return QWidget::event(event);
//                    }
//                }
//            }
//        }
    }
    else if( event->type() == QEvent::Resize )
    { // Tweak the room selection widget to fit
        resizeMultiSelectionWidget();
    }
    return QWidget::event(event);
}

void T2DMap::mousePressEvent(QMouseEvent *event)
{
    mNewMoveAction = true;
    if( event->buttons() & Qt::LeftButton )
    {
        // move map with left mouse button + ALT
        if( event->modifiers().testFlag(Qt::AltModifier) )
        {
            mpMap->mLeftDown = true;
        }

        // drawing new custom exit line
        if( mCustomLinesRoomFrom > 0 )
        {
            if( mDialogLock )
            {
                 return; // Prevent any line drawing until ready
            }

            TRoom * pR = mpMap->mpRoomDB->getRoom( mCustomLinesRoomFrom );
            if( pR )
            {
                float mx = event->pos().x()/mTX + mOx;
                float my = event->pos().y()/mTY + mOy;
                mx = mx - xspan/2;
                my = yspan/2 - my;
                // might be useful to have a snap to grid type option
                pR->customLines[mCustomLinesRoomExit].push_back( QPointF( mx, my ) );
                pR->calcRoomDimensions();
                repaint();
                return;
            }
        }

        // check click on custom exit lines
        if( mMultiSelectionSet.isEmpty() )
        { // But NOT if got one or more rooms already selected!
            TArea * pA = mpMap->mpRoomDB->getArea(mAID);
            if( pA )
            {
                TArea * pArea = pA;
                float mx = event->pos().x()/mTX + mOx;
                float my = event->pos().y()/mTY + mOy;
                mx = mx - xspan/2;
                my = yspan/2 - my;
                QPointF pc = QPointF(mx,my);
                QSetIterator<int> itRoom = pArea->rooms;
                while( itRoom.hasNext() ) {
                    int currentRoomId = itRoom.next();
                    TRoom * pR = mpMap->mpRoomDB->getRoom(currentRoomId);
                    if( !pR ) continue;
                    QMapIterator<QString, QList<QPointF> > it(pR->customLines);
                    while( it.hasNext() )
                    {
                        it.next();
                        const QList<QPointF> & _pL= it.value();
                        if( _pL.size() )
                        {
                            // The way this code is structured means that EARLIER
                            // points are selected in preference to later ones!
                            // This might not be intuative to the users...
                            float olx, oly, lx, ly;
                            for( int j=0; j<_pL.size(); j++ )
                            {
                                if( j==0 )
                                {  // First segment of a custom line
                                   // start it at the centre of the room
                                    olx = pR->x;
                                    oly = pR->y; //FIXME: exit richtung beachten, um den Linienanfangspunkt zu berechnen
                                    lx = _pL[0].x();
                                    ly = _pL[0].y();
                                }
                                else
                                {  // Not the first segment of a custom line
                                   // so start it at the end of the previous one
                                    olx = lx;
                                    oly = ly;
                                    lx = _pL[j].x();
                                    ly = _pL[j].y();
                                }
                                // End of each custom line segment is given

                                // click auf einen edit - punkt
                                if( mCustomLineSelectedRoom != 0 )
                                { // We have already choosen a line to edit
                                    if( abs(mx-lx)<=0.25 && abs(my-ly)<=0.25 )
                                    { // And this looks close enough to a point that we should edit it
                                        mCustomLineSelectedPoint = j;
                                        return;
                                    }
                                }

                                // We have not previously choosen a line to edit
                                QLineF line = QLineF(olx,oly, lx,ly);
                                QLineF normal = line.normalVector();
                                QLineF tl;
                                tl.setP1(pc);
                                tl.setAngle(normal.angle());
                                tl.setLength(0.1);
                                QLineF tl2;
                                tl2.setP1(pc);
                                tl2.setAngle(normal.angle());
                                tl2.setLength(-0.1);
                                QPointF pi;
                                if(    ( line.intersect( tl, &pi) == QLineF::BoundedIntersection )
                                    || ( line.intersect( tl2, &pi) == QLineF::BoundedIntersection ) )
                                { // Choose THIS line to edit as we have clicked close enough to it...
                                    mCustomLineSelectedRoom = pR->getId();
                                    mCustomLineSelectedExit = it.key();
                                    repaint();
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            mCustomLineSelectedRoom = 0;
            mCustomLineSelectedExit = "";
        }

        if( mRoomBeingMoved ) // Moving rooms so end that
        {
            mPHighlightMove = event->pos();
            mPick = true;

            setMouseTracking(false);
            mRoomBeingMoved = false;
        }
        else if( ! mPopupMenu ) // Not in a context menu, so start selection mode - including drag to select
        {
            mMultiSelection = true;
            mMultiRect = QRect(event->pos(), event->pos());
            int _roomID = mRID;
            if( ! mpMap->mpRoomDB->getRoom( _roomID ) ) return;
            int _areaID = mAID;
            TArea * pArea = mpMap->mpRoomDB->getArea(_areaID);
            if( !pArea ) return;
            float _rx = xspan / 2 * mTX - mTX * mOx;
            float _ry = yspan / 2 * mTY - mTY * mOy;

            if( ! event->modifiers().testFlag(Qt::ControlModifier ) )
            { // If control key NOT down then clear selection, and put up helpful text
                mHelpMsg = tr("Drag to select multiple rooms or labels, release to finish...");
                mMultiSelectionSet.clear();
            }

            QSetIterator<int> itRoom( pArea->getAreaRooms() );
            while( itRoom.hasNext() )
            { // Scan to find rooms in selection
                int currentAreaRoom = itRoom.next();
                TRoom * pR = mpMap->mpRoomDB->getRoom( currentAreaRoom );
                if( !pR ) continue;
                int rx = pR->x*mTX+_rx;
                int ry = pR->y*-1*mTY+_ry;
                int rz = pR->z;

                int mx = event->pos().x();
                int my = event->pos().y();
                int mz = mOz;
                if( (abs(mx-rx)<mTX*rSize/2) && (abs(my-ry)<mTY*rSize/2) && (mz == rz) )
                {
                    if(    mMultiSelectionSet.contains( currentAreaRoom )
                        && event->modifiers().testFlag(Qt::ControlModifier) )
                    {
                        mMultiSelectionSet.remove( currentAreaRoom );
                    }
                    else
                    {
                        mMultiSelectionSet.insert( currentAreaRoom );
                    }
                    if( mMultiSelectionSet.size() > 0 )
                    {
                        mMultiSelection = false;
                    }
                }
            }
            switch( mMultiSelectionSet.size() )
            {
            case 0:
                mMultiSelectionHighlightRoomId = -1;
                break;
            case 1:
                mMultiSelection = false; // OK, found one room so stop
                mMultiSelectionHighlightRoomId = mMultiSelectionSet.toList().first();
                mHelpMsg.clear();
                break;
            default:
                mMultiSelection = false; // OK, found more than one room so stop
                mHelpMsg.clear();
                getCenterSelection();
            }

            // select labels
            if( mpMap->mapLabels.contains( mAID ) )
            {
                QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAID]);
                while( it.hasNext() )
                {
                    it.next();
                    if( it.value().pos.z() != mOz )
                    {
                        continue;
                    }

                    QPointF lpos;
                    float _lx = it.value().pos.x()*mTX+_rx;
                    float _ly = it.value().pos.y()*mTY*-1+_ry;

                    lpos.setX( _lx );
                    lpos.setY( _ly );
                    int mx = event->pos().x();
                    int my = event->pos().y();
// N/U:                         int mz = mOz;
                    QPoint click = QPoint(mx,my);
                    QRectF br = QRect(_lx, _ly, it.value().clickSize.width(), it.value().clickSize.height());
                    if( br.contains( click ))
                    {
                        if( ! it.value().hilite )
                        {
                            mLabelHilite = true;
                            mpMap->mapLabels[mAID][it.key()].hilite = true;
                        }
                        else
                        {
                            mpMap->mapLabels[mAID][it.key()].hilite = false;
                            mLabelHilite = false;
                        }
                        update();
                        return;
                    }
                }
            }

            mLabelHilite = false;
            update();

            if( mMultiSelection && mMultiSelectionSet.size() > 0 && ( event->modifiers().testFlag(Qt::ControlModifier) ) )
            {
                // We were dagging multi-selection rectange, we had selected at
                // least one room and the user has <CTRL>-clicked with the mouse
                // so switch off the dragging
                mMultiSelection = false;
                mHelpMsg.clear();
            }

        }
        else
        { // In popup menu, so end that
            mPopupMenu = false;
        }

        // display room selection list widget if more than 1 room has been selected
        // -> user can manually change current selection if rooms are overlapping
        if( mMultiSelectionSet.size() > 1 )
        {
            mMultiSelectionListWidget.blockSignals(true); // We don't want to cause calls to slot_roomSelectionChanged() here!
            mIsSelectionSorting = mMultiSelectionListWidget.isSortingEnabled();
            mIsSelectionSortByNames = (mMultiSelectionListWidget.sortColumn() == 1);
            mMultiSelectionListWidget.clear();
            mMultiSelectionListWidget.setSortingEnabled(false); // Do NOT sort whilst inserting items!
            QSetIterator<int> itRoom = mMultiSelectionSet;
            mIsSelectionUsingNames = false;
            while( itRoom.hasNext() )
            {
                auto _item = new QTreeWidgetItem;
                int multiSelectionRoomId = itRoom.next();
                _item->setText(0,QStringLiteral("%1").arg(multiSelectionRoomId,7)); // Pad with spaces so sorting works
                _item->setTextAlignment(0, Qt::AlignRight);
                TRoom * pR_multiSelection = mpMap->mpRoomDB->getRoom(multiSelectionRoomId);
                if( pR_multiSelection )
                {
                    QString multiSelectionRoomName = pR_multiSelection->name;
                    if( ! multiSelectionRoomName.isEmpty() )
                    {
                        _item->setText(1,multiSelectionRoomName);
                        _item->setTextAlignment(1, Qt::AlignLeft);
                        mIsSelectionUsingNames = true;
                    }
                }
                mMultiSelectionListWidget.addTopLevelItem( _item );
            }
            mMultiSelectionListWidget.setColumnHidden(1, !mIsSelectionUsingNames );
            // Can't sort if nothing to sort on, switch to sorting by room number
            if( (! mIsSelectionUsingNames) && mIsSelectionSortByNames && mIsSelectionSorting )
            {
                mIsSelectionSortByNames = false;
            }
            mMultiSelectionListWidget.sortByColumn( mIsSelectionSortByNames ? 1 : 0 );
            mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
            resizeMultiSelectionWidget();
            mMultiSelectionListWidget.selectAll();
            mMultiSelectionListWidget.blockSignals(false);
            mMultiSelectionListWidget.show();
            update();
        }
        else
        {
            mMultiSelectionListWidget.hide();
        }
    }


    if( event->buttons() & Qt::RightButton )
    {
        auto popup = new QMenu( this );

        if( mCustomLinesRoomFrom > 0 )
        {
            if( mDialogLock )
            {
                 return;
            }

            TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
            if( pR )
            {

                QAction * action = new QAction( "undo", this );
                action->setStatusTip(tr( "undo last point" ));
                if( pR->customLines.value( mCustomLinesRoomExit ).count() > 1 )
                    connect( action, SIGNAL( triggered() ), this, SLOT( slot_undoCustomLineLastPoint() ) );
                else
                    action->setEnabled( false );

                QAction * action2 = new QAction("properties", this );
                action2->setText("properties...");
                action2->setStatusTip(tr("change the properties of this line"));
                connect( action2, SIGNAL(triggered()), this, SLOT(slot_customLineProperties()));

                QAction * action3 = new QAction("finish", this );
                action3->setStatusTip(tr("finish drawing this line"));
                connect( action3, SIGNAL(triggered()), this, SLOT(slot_doneCustomLine()));

                mPopupMenu = true;

                pR->calcRoomDimensions();
                popup->addAction( action );
                popup->addAction( action2 );
                popup->addAction( action3 );

                popup->popup( mapToGlobal( event->pos() ) );
                update();
                return;
            }
        }

        if( ! mLabelHilite && mCustomLineSelectedRoom == 0 )
        {
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
            action4->setStatusTip(tr("increase map X-Y spacing for the selected group of rooms"));
            connect( action4, SIGNAL(triggered()), this, SLOT(slot_spread()));
            QAction * action9 = new QAction("shrink", this );
            action9->setStatusTip(tr("decrease map X-Y spacing for the selected group of rooms"));
            connect( action9, SIGNAL(triggered()), this, SLOT(slot_shrink()));

            //QAction * action5 = new QAction("user data", this );
            //action5->setStatusTip(tr("set user data"));
            //connect( action5, SIGNAL(triggered()), this, SLOT(slot_setUserData()));
            QAction * action6 = new QAction("lock", this );
            action6->setStatusTip(tr("lock room for speed walks"));
            connect( action6, SIGNAL(triggered()), this, SLOT(slot_lockRoom()));
            QAction * action17 = new QAction("unlock", this );
            action17->setStatusTip(tr("unlock room for speed walks"));
            connect( action17, SIGNAL(triggered()), this, SLOT(slot_unlockRoom()));
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
            TArea * pArea = mpMap->mpRoomDB->getArea(mAID);
            if( ! pArea )
                return;
            if( pArea->gridMode )
            { // Disable custom exit lines in grid mode as they aren't visible anyway
                action14->setStatusTip(tr("custom exit lines are not shown and are not editable in grid mode"));
                action14->setEnabled(false);
            }
            else
            {
                action14->setStatusTip(tr("replace an exit line with a custom line"));
                connect( action14, SIGNAL(triggered()), this, SLOT(slot_setCustomLine()));
            }

            QAction * action15 = new QAction("create Label", this );
            action15->setStatusTip(tr("Create labels to show text or images."));
            connect( action15, SIGNAL(triggered()), this, SLOT(slot_createLabel()));

            QAction * action16 = new QAction("set location", this );
            if( mMultiSelectionSet.size() == 1 )
            { // Only enable if ONE room is highlighted
                action16->setStatusTip(tr("set player current location to here"));
                connect( action16, SIGNAL(triggered()), this, SLOT(slot_setPlayerLocation()));
            }
            else
            {
                action16->setEnabled(false);
                action16->setStatusTip(tr("cannot set location when not exactly one room selected"));
            }

            mPopupMenu = true;
//            QMenu * popup = new QMenu( this );

            popup->addAction( action );
            popup->addAction( action8 );
            popup->addAction( action14 );
            popup->addAction( action3 );
            popup->addAction( action10 );
            //popup->addAction( action11 );
            popup->addAction( action4 );
            popup->addAction( action9 );
            //popup->addAction( action5 );
            popup->addAction( action6 );
            popup->addAction( action17 );
            popup->addAction( action7 );
            popup->addAction( action2 );
            popup->addAction( action12 );

            popup->addAction( action13 );

            popup->addAction( action15 );
            popup->addAction( action16 );

            popup->popup( mapToGlobal( event->pos() ) );
        }
        else if( mLabelHilite )
        {
            QAction * action = new QAction("move", this );
            action->setStatusTip(tr("move label"));
            connect( action, SIGNAL(triggered()), this, SLOT(slot_moveLabel()));
            QAction * action2 = new QAction("delete", this );
            action2->setStatusTip(tr("delete label"));
            connect( action2, SIGNAL(triggered()), this, SLOT(slot_deleteLabel()));
            mPopupMenu = true;
            popup->addAction( action );
            popup->addAction( action2 );
            popup->popup( mapToGlobal( event->pos() ) );
        }
        else
        { // seems that if we get here then we have right clicked on a selected custom line?
//            qDebug("T2DMap::mousePressEvent(): reached else case, mCustomLineSelectedRoom=%i, Exit=%s, Point=%i",
//                   mCustomLineSelectedRoom,
//                   qPrintable(mCustomLineSelectedExit),
//                   mCustomLineSelectedPoint);

            if( mCustomLineSelectedRoom > 0 )
            {
                TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
                if( pR )
                {
                    QAction * action = new QAction("add point", this );
                    if( mCustomLineSelectedPoint > -1 )
                        // The first user manipulable point IS zero - line is
                        // drawn to it from a point around room symbol dependent
                        // on the exit direction - and we can now add even to it
                    {
                        connect( action, SIGNAL(triggered()), this, SLOT(slot_customLineAddPoint()));
                        action->setStatusTip(tr("divide segment by adding a new point mid-way along"));
                    }
                    else
                    {
                        action->setEnabled(false);
                        action->setStatusTip(tr("select a point first, then add a new point mid-way along the segment towards room"));
                    }

                    QAction * action2 = new QAction("remove point", this );
                    // Permit this to be enabled if the current point is 0 or greater, but not if there is no others
                    if( mCustomLineSelectedPoint > -1 )
                    {
                        if( pR->customLines.value(mCustomLineSelectedExit).count() > 1 )
                        {
                            connect( action2, SIGNAL(triggered()), this, SLOT(slot_customLineRemovePoint()));
                            if( ( mCustomLineSelectedPoint + 1 ) < pR->customLines.value(mCustomLineSelectedExit).count() )
                            {
                                action2->setStatusTip(tr("merge pair of segments by removing this point"));
                            }
                            else
                            {
                                action2->setStatusTip(tr("remove last segment by removing this point"));
                            }
                        }
                        else
                        {
                            action2->setEnabled(false);
                            action2->setStatusTip(tr("use \"delete line\" to remove the only segment ending in an editable point"));
                        }
                    }
                    else
                    {
                        action2->setEnabled(false);
                        action2->setStatusTip(tr("select a point first, then remove it"));
                    }

                    QAction * action3 = new QAction("properties", this );
                    action3->setText("properties...");
                    action3->setStatusTip(tr("change the properties of this custom line"));
                    connect( action3, SIGNAL(triggered()), this, SLOT(slot_customLineProperties()));

                    QAction * action4 = new QAction("delete line", this );
                    action4->setStatusTip(tr("delete all of this custom line"));
                    connect( action4, SIGNAL(triggered()), this, SLOT(slot_deleteCustomExitLine()));

                    mPopupMenu = true;

                    popup->addAction( action );
                    popup->addAction( action2 );
                    popup->addAction( action3 );
                    popup->addAction( action4 );
                    popup->popup( mapToGlobal( event->pos() ) );
                }
            }
        }        //this is placed at the end since it is likely someone will want to hook anywhere
        QMap<QString, QMenu *> userMenus;
        QMapIterator<QString, QStringList> it(mUserMenus);
        while (it.hasNext()){
            it.next();
            QStringList menuInfo = it.value();
            QString displayName = menuInfo[1];
            auto userMenu = new QMenu(displayName, this);
            userMenus.insert(it.key(), userMenu);
        }
        it.toFront();
        while (it.hasNext()){//take care of nested menus now since they're all made
            it.next();
            QStringList menuInfo = it.value();
            QString menuParent = menuInfo[0];
            if (menuParent == ""){//parentless
                popup->addMenu(userMenus[it.key()]);
            }
            else{//has a parent
                userMenus[menuParent]->addMenu(userMenus[it.key()]);
            }
        }
        //add our actions
        QMapIterator<QString, QStringList> it2(mUserActions);
        auto mapper = new QSignalMapper(this);
        while (it2.hasNext()){
            it2.next();
            QStringList actionInfo = it2.value();
            auto action = new QAction(actionInfo[2], this );
            if (actionInfo[1] == "")//no parent
                popup->addAction(action);
            else if (userMenus.contains(actionInfo[1]))
                userMenus[actionInfo[1]]->addAction(action);
            else
                continue;
            mapper->setMapping(action, it2.key());
            connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
        }
        connect(mapper, SIGNAL(mapped(QString)), this, SLOT(slot_userAction(QString)));
        mLastMouseClick = event->localPos();
    }
    update();
}

// Used both by "properties..." context menu item for existing lines AND
// during drawing new ones.
void T2DMap::slot_customLineProperties()
{

    QString exit;
    TRoom * pR;

    if( mCustomLineSelectedRoom > 0 )
    {
        pR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        exit = mCustomLineSelectedExit;
    }
    else
    {
        pR = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
        exit = mCustomLinesRoomExit;
    }

    if( pR )
    {
        if( exit.isEmpty() ) {
            qDebug("T2DMap::slot_customLineProperties() called but no exit is selected...");
            return;
        }
        if( pR->customLines.contains( exit ) )
        {
            QUiLoader loader;

            QFile file(":/ui/custom_lines_properties.ui");
            file.open(QFile::ReadOnly);
            QDialog *d = qobject_cast<QDialog *>(loader.load(&file, this));
            file.close();
            if( ! d )
            {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to create the dialog!");
                return;
            }
            d->setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_custom_exit_properties.png" ) ) );
            QLineEdit * le_toId = d->findChild<QLineEdit*>("toId");
            QLineEdit * le_fromId = d->findChild<QLineEdit*>("fromId");
            QLineEdit * le_cmd = d->findChild<QLineEdit*>("cmd");

            mpCurrentLineStyle = d->findChild<QComboBox*>("lineStyle");
            mpCurrentLineColor = d->findChild<QPushButton*>("lineColor");
            mpCurrentLineArrow = d->findChild<QCheckBox*>("arrow");
            if( ! le_toId || ! le_cmd || ! le_fromId
                || ! mpCurrentLineStyle
                || ! mpCurrentLineColor
                || ! mpCurrentLineArrow )
            {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to find an element in the dialog!");
                return;
            }
            le_cmd->setText( exit );
            le_fromId->setText( QString::number( pR->getId() ) );
            if( exit == "NW" )
                le_toId->setText( QString::number( pR->getNorthwest() ) );
            else if( exit == "N" )
                le_toId->setText( QString::number( pR->getNorth() ) );
            else if( exit == "NE" )
                le_toId->setText( QString::number( pR->getNortheast() ) );
            else if( exit == "UP" )
                le_toId->setText( QString::number( pR->getUp() ) );
            else if( exit == "W" )
                le_toId->setText( QString::number( pR->getWest() ) );
            else if( exit == "E" )
                le_toId->setText( QString::number( pR->getEast() ) );
            else if( exit == "DOWN" )
                le_toId->setText( QString::number( pR->getDown() ) );
            else if( exit == "SW" )
                le_toId->setText( QString::number( pR->getSouthwest() ) );
            else if( exit == "S" )
                le_toId->setText( QString::number( pR->getSouth() ) );
            else if( exit == "SE" )
                le_toId->setText( QString::number( pR->getSoutheast() ) );
            else if( exit == "IN" )
                le_toId->setText( QString::number( pR->getIn() ) );
            else if( exit == "OUT" )
                le_toId->setText( QString::number( pR->getOut() ) );
            else
            {
                bool isFound = false;
                QMultiMap<int, QString> otherExits = pR->getOtherMap();
                QMapIterator<int, QString> otherExitIt = otherExits;
                while( otherExitIt.hasNext() )
                {
                    otherExitIt.next();
                    if( otherExitIt.value().startsWith("0") || otherExitIt.value().startsWith("1") )
                    {
                        if( otherExitIt.value().mid(1) == exit )
                        {
                            le_toId->setText( QString::number( otherExitIt.key() ) );
                            isFound = true;
                            break;
                        }
                    }
                    else if( otherExitIt.value() == exit )
                    {
                        le_toId->setText( QString::number( otherExitIt.key() ) );
                        isFound = true;
                        break;
                    }
                }
                if( ! isFound )
                    qWarning("T2DMap::slot_customLineProperties() - WARNING: missing command \"%s\" from custom lines for room id %i",
                             qPrintable( exit ),
                             pR->getId() );
            }

            QStringList _lineStyles;
            _lineStyles << "solid line" << "dot line" << "dash line" << "dash dot line" << "dash dot dot line";
            mpCurrentLineStyle->addItems( _lineStyles );
            QString _lineStyle = pR->customLinesStyle.value(exit);
            mpCurrentLineStyle->setCurrentIndex( mpCurrentLineStyle->findText( _lineStyle ) );

            mpCurrentLineArrow->setChecked( pR->customLinesArrow.value(exit) );
            mCurrentLineColor.setRed( pR->customLinesColor.value(exit).at(0) );
            mCurrentLineColor.setGreen( pR->customLinesColor.value(exit).at(1) );
            mCurrentLineColor.setBlue( pR->customLinesColor.value(exit).at(2) );

            QString _styleSheet = QString("background-color:"+ mCurrentLineColor.name() );
            mpCurrentLineColor->setStyleSheet( _styleSheet );
            connect( mpCurrentLineColor, SIGNAL(clicked()), this, SLOT(slot_customLineColor()));

            if( d->exec() == QDialog::Accepted )
            {
                // Make the changes
                pR->customLinesStyle[exit] = mpCurrentLineStyle->currentText();
                mCurrentLineStyle = mpCurrentLineStyle->currentText();

                pR->customLinesColor[exit][0] = mCurrentLineColor.red();
                pR->customLinesColor[exit][1] = mCurrentLineColor.green();
                pR->customLinesColor[exit][2] = mCurrentLineColor.blue();

                pR->customLinesArrow[exit] = mpCurrentLineArrow->checkState();
                mCurrentLineArrow = mpCurrentLineArrow->checkState();
            }
        }
        repaint();
    }
    else
    {
        qDebug("T2DMap::slot_customLineProperties() called but no line is selected...");
    }
}


void T2DMap::slot_customLineAddPoint()
{
    TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
    if( ! pR )
        return;

    if( mCustomLineSelectedPoint > 0 )
    {
        QLineF segment = QLineF( pR->customLines.value(mCustomLineSelectedExit).at(mCustomLineSelectedPoint - 1),
                                 pR->customLines.value(mCustomLineSelectedExit).at(mCustomLineSelectedPoint) );
        segment.setLength(segment.length() / 2.0);
        pR->customLines[mCustomLineSelectedExit].insert(mCustomLineSelectedPoint, segment.p2() );
        mCustomLineSelectedPoint++;
        repaint();
    }
    else if( mCustomLineSelectedPoint == 0 )
    {
        // The first user manipulable point IS zero - line is drawn to it from a
        // point around room symbol dependent on the exit direction
        // The first segment of custom line stick out half of the distance
        // between two rooms at a map unit vector distance apart. so an added
        // point inserted before the first must be placed halfway between
        // the offset point and the previosu first point
        QPointF customLineStartPoint;
        if( mCustomLineSelectedExit == "N" )
            customLineStartPoint = QPointF( pR->x      , pR->y + 0.5 );
        else if( mCustomLineSelectedExit == "S" )
            customLineStartPoint = QPointF( pR->x      , pR->y - 0.5 );
        else if( mCustomLineSelectedExit == "E" )
            customLineStartPoint = QPointF( pR->x + 0.5, pR->y       );
        else if( mCustomLineSelectedExit == "W" )
            customLineStartPoint = QPointF( pR->x - 0.5, pR->y       );
        else if( mCustomLineSelectedExit == "NE" )
            customLineStartPoint = QPointF( pR->x + 0.5, pR->y + 0.5 );
        else if( mCustomLineSelectedExit == "NW" )
            customLineStartPoint = QPointF( pR->x - 0.5, pR->y + 0.5 );
        else if( mCustomLineSelectedExit == "SE" )
            customLineStartPoint = QPointF( pR->x + 0.5, pR->y - 0.5 );
        else if( mCustomLineSelectedExit == "SW" )
            customLineStartPoint = QPointF( pR->x - 0.5, pR->y - 0.5 );
        else
            customLineStartPoint = QPointF( pR->x      , pR->y       );
        QLineF segment = QLineF( customLineStartPoint,
                                 pR->customLines.value(mCustomLineSelectedExit).at(0) );
        segment.setLength(segment.length() / 2.0);
        pR->customLines[mCustomLineSelectedExit].insert(mCustomLineSelectedPoint, segment.p2() );
        mCustomLineSelectedPoint++;
        repaint();
    }
}


void T2DMap::slot_customLineRemovePoint()
{
    TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
    if( ! pR )
        return;

    if( mCustomLineSelectedPoint > 0 )
    {
        pR->customLines[mCustomLineSelectedExit].removeAt(mCustomLineSelectedPoint);
        mCustomLineSelectedPoint--;
        repaint();
    }
    else if( mCustomLineSelectedPoint == 0 && pR->customLines.value(mCustomLineSelectedExit).count() > 1 )
    {
        // The first user manipulable point IS zero - line is drawn to it from a
        // point around room symbol dependent on the exit direction.  We can only
        // allow it's deletion if there is at least another one left.
        pR->customLines[mCustomLineSelectedExit].removeAt(mCustomLineSelectedPoint);
        repaint();
    }
}


void T2DMap::slot_undoCustomLineLastPoint()
{
    if( mCustomLinesRoomFrom > 0 )
    {
        TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
        if( pR )
        {
            if( pR->customLines.value(mCustomLinesRoomExit).count() > 0 )
                pR->customLines[mCustomLinesRoomExit].pop_back();
            pR->calcRoomDimensions();
        }
        repaint();
    }
}

void T2DMap::slot_doneCustomLine()
{
    if( mpCustomLinesDialog )
    {
        mpCustomLinesDialog->accept();
        mpCustomLinesDialog = 0;
    }
    mHelpMsg = "";
    mCustomLinesRoomFrom = 0;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit.clear();
    if( mMultiSelectionSet.size()>0)
    {
        TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if( pR )
        {
            pR->calcRoomDimensions();
        }
    }
    update();
}

void T2DMap::slot_deleteCustomExitLine()
{

    if( mCustomLineSelectedRoom > 0  )
    {
        TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if( pR )
        {
            pR->customLinesArrow.remove(mCustomLineSelectedExit);
            pR->customLinesColor.remove(mCustomLineSelectedExit);
            pR->customLinesStyle.remove(mCustomLineSelectedExit);
            pR->customLines.remove(mCustomLineSelectedExit);
            mCustomLineSelectedRoom = 0;
            mCustomLineSelectedExit = "";
            mCustomLineSelectedPoint = -1;
            repaint();
            pR->calcRoomDimensions();
            TArea * pA = mpMap->mpRoomDB->getArea( pR->getArea() );
            if( pA )
                pA->calcSpan();
        }
    }
}

void T2DMap::slot_moveLabel()
{
    mMoveLabel = true;
}

void T2DMap::slot_deleteLabel()
{
    if( mpMap->mapLabels.contains( mAID ) )
    {
        QList<int> deleteList;
        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAID]);
        while( it.hasNext() )
        {
            it.next();
            int _zlevel = static_cast<int>(it.value().pos.z());
            if( _zlevel != mOz ) continue;
            if( it.value().hilite )
            {
                deleteList.push_back(it.key());
            }
        }
        for(int & i : deleteList)
        {
            mpMap->mapLabels[mAID].remove(i);
        }
    }
    update();
}

void T2DMap::slot_editLabel()
{
}

void T2DMap::slot_setPlayerLocation()
{
    if( mMultiSelectionSet.size() != 1 ) {
        return; // Was <= 1 but that can't be right, and >1 doesn't seem right either
    }

    int _newRoomId = *( mMultiSelectionSet.constBegin() );
    if( mpMap->mpRoomDB->getRoom( _newRoomId ) ) {
        // No need to check it is a DIFFERENT room - that is taken care of by en/dis-abling the control
        mpMap->mRoomIdHash[ mpHost->getName() ] = _newRoomId;
        mpMap->mNewMove = true;
        TEvent manualSetEvent;
        manualSetEvent.mArgumentList.append(QLatin1String("sysManualLocationSetEvent"));
        manualSetEvent.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        manualSetEvent.mArgumentList.append( QString::number( _newRoomId ) );
        manualSetEvent.mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        mpHost->raiseEvent( manualSetEvent );
        update();
    }
}

void T2DMap::slot_userAction(QString uniqueName)
{
    TEvent event;
    QStringList userEvent = mUserActions[uniqueName];
    event.mArgumentList.append( userEvent[0] );
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(uniqueName );
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    QSetIterator<int> itRoom(mMultiSelectionSet);
    if( itRoom.hasNext() )
    {
        while( itRoom.hasNext() )
        {
            event.mArgumentList.append(QString::number(itRoom.next()));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        }
        mpHost->raiseEvent( event );
    }
// Unreachable code as had effectively the same test as the previous "if"
// mMultiSelectionList is now mMultiSelectionSet:
//    else if( mMultiSelectionList.size() > 0 )
//    {
//        event.mArgumentList.append(QString::number(mMultiSelectionList[0]));
//        event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
//        mpHost->raiseEvent( & event );
//    }
    else
    {
        event.mArgumentList.append(uniqueName);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        for (int i=0;i<userEvent.size();i++)
        {
            event.mArgumentList.append(userEvent[i]);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        }
        mpHost->raiseEvent( event );
    }
}

void T2DMap::slot_movePosition()
{
    if( ! getCenterSelection() )
    {
        return;
    }

    TRoom * pR_start = mpMap->mpRoomDB->getRoom( mMultiSelectionHighlightRoomId );
    // pR has already been validated by getCenterSelection()

    auto pD = new QDialog(this);
    auto pL = new QGridLayout;
    pD->setLayout( pL );
    pD->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    pD->setContentsMargins(0,0,0,0);
    auto pLEx = new QLineEdit(pD);
    auto pLEy = new QLineEdit(pD);
    auto pLEz = new QLineEdit(pD);

    pLEx->setText(QString::number(pR_start->x));
    pLEy->setText(QString::number(pR_start->y));
    pLEz->setText(QString::number(pR_start->z));
    QLabel * pLa0 = new QLabel( tr("Move the selection, centered on\nthe highlighted room (%1) to:").arg(mMultiSelectionHighlightRoomId) );
    // Record the starting coordinates - can be a help when working out how to move a block of rooms!
    QLabel * pLa1 = new QLabel( tr("x coordinate (was %1):").arg(pR_start->x) );
    QLabel * pLa2 = new QLabel( tr("y coordinate (was %1):").arg(pR_start->y) );
    QLabel * pLa3 = new QLabel( tr("z coordinate (was %1):").arg(pR_start->z) );
    pL->addWidget(pLa0,0,0,1,2,Qt::AlignCenter);
    pL->addWidget(pLa1,1,0,Qt::AlignVCenter|Qt::AlignRight);
    pL->addWidget(pLEx,1,1,Qt::AlignVCenter|Qt::AlignLeft);
    pL->addWidget(pLa2,2,0,Qt::AlignVCenter|Qt::AlignRight);
    pL->addWidget(pLEy,2,1,Qt::AlignVCenter|Qt::AlignLeft);
    pL->addWidget(pLa3,3,0,Qt::AlignVCenter|Qt::AlignRight);
    pL->addWidget(pLEz,3,1,Qt::AlignVCenter|Qt::AlignLeft);
    auto pButtonBar = new QWidget(pD);

    auto pL2 = new QHBoxLayout;
    pButtonBar->setLayout( pL2 );
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    auto pB_ok = new QPushButton(pButtonBar);
    pB_ok->setText("Ok");
    pL2->addWidget(pB_ok);
    connect(pB_ok, SIGNAL(clicked()), pD, SLOT(accept()));

    auto pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText("Cancel");
    connect(pB_abort, SIGNAL(clicked()), pD, SLOT(reject()));
    pL2->addWidget(pB_abort);
    pL->addWidget(pButtonBar,4,0,1,2,Qt::AlignCenter);

    if( pD->exec() == QDialog::Accepted )
    {
        int dx = pLEx->text().toInt() - pR_start->x;
        int dy = pLEy->text().toInt() - pR_start->y;
        int dz = pLEz->text().toInt() - pR_start->z;

        mMultiRect = QRect(0,0,0,0);

        QSetIterator<int> itRoom = mMultiSelectionSet;
        while( itRoom.hasNext() )
        {
            TRoom *pR = mpMap->mpRoomDB->getRoom( itRoom.next() );
            if( ! pR )
            {
                continue;
            }

            pR->x += dx;
            pR->y += dy;
            pR->z += dz;
        }
    }
    repaint();
}


void T2DMap::slot_moveRoom()
{
    mRoomBeingMoved = true;
    setMouseTracking(true);
    mNewMoveAction = true;

}

// TODO: Convert pR->c (a single ASCII character) to pR->symbol (a single Unicode Grapheme, possibly containing several QChars - as a QString)
void T2DMap::slot_setCharacter()
{
// Now analysises and reports the existing letters used in ALL the selected
// rooms if more than once (and sorts by their frequency)
// Also allows the existing letters to be deleted (by clearing all the displayed
// letters) as previous code DID NOT - and the Cancel option works as well!
    if( mMultiSelectionSet.size() < 1 )
    {
        return;
    }

    // First scan and count all the different letters used
    QMap<QString, uint> usedLetters;
    QSetIterator<int> itRoom = mMultiSelectionSet;
    TRoom * pR;
    while( itRoom.hasNext() )
    {
        pR = mpMap->mpRoomDB->getRoom( itRoom.next() ) ;
        if( !pR )
        {
            continue;
        }

        if( pR->c )
        {
            QString thisLetter = QString(pR->c);
            if( ! thisLetter.isEmpty() )
            {
                if( usedLetters.contains(thisLetter) )
                {
                    usedLetters[thisLetter] += 1;
                }
                else
                {
                    usedLetters[thisLetter] = 1;
                }
            }
        }
    }

    QString newLetterText;
    bool isOk = false;
    // Choose most appropriate text depending on number of rooms selected and
    // how many of them have letters already:
    if( mMultiSelectionSet.size() == 1)
    {
        if( usedLetters.isEmpty() )
        {
            newLetterText = QInputDialog::getText( this, // QWidget * parent
                                               tr("Enter room marker"), // const QString & title
                                               tr("Enter new (not space)\n"
                                                  "marker letter:"), // const QString & label
                                               QLineEdit::Normal, // QLineEdit::EchoMode mode = QLineEdit::Normal
                                               QString(), // const QString & text = QString()
                                               &isOk, // bool * ok = 0
                                               0, // Qt::WindowFlags flags = 0
                                               Qt::ImhLatinOnly ); // Qt::InputMethodHints inputMethodHints = Qt::ImhNone
        }
        else
        {
            newLetterText = QInputDialog::getText( this,
                                               tr("Enter rooom marker"),
                                               tr("Delete the existing, or\n"
                                                  "enter a new (non-space),\n"
                                                  "marker letter:"),
                                               QLineEdit::Normal,
                                               usedLetters.keys().first(),
                                               &isOk,
                                               0,
                                               Qt::ImhLatinOnly );
        }
    }
    else
    {
        if( usedLetters.isEmpty() )
        {
            newLetterText = QInputDialog::getText( this,
                                               tr("Enter room marker"),
                                               tr("Enter new (not space)\n"
                                                  "marker letter:"),
                                               QLineEdit::Normal,
                                               QString(),
                                               &isOk,
                                               0,
                                               Qt::ImhLatinOnly );
        }
        else if( usedLetters.size() == 1 )
        {
            newLetterText = QInputDialog::getText( this,
                                               tr("Enter room marker"),
                                               tr("Delete the (only)\n"
                                                  "existing (in some rooms),\n"
                                                  "or enter a new (not\n"
                                                  "space) marker letter for\n"
                                                  "all selected rooms:"),
                                               QLineEdit::Normal,
                                               usedLetters.keys().first(),
                                               &isOk,
                                               0,
                                               Qt::ImhLatinOnly );
        }
        else
        {
            QMapIterator<QString, uint> itSymbolUsed = usedLetters;
            QSet<uint> symbolCountsSet;
            while( itSymbolUsed.hasNext() )
            {
                itSymbolUsed.next();
                symbolCountsSet.insert( itSymbolUsed.value() );
            }
            QList<uint> symbolCountsList = symbolCountsSet.toList();
            if( symbolCountsList.size() > 1 )
            {
                std::sort( symbolCountsList.begin(), symbolCountsList.end() );
            }
            QStringList displayStrings;
            for( int i = symbolCountsList.size()-1; i >= 0; --i )
            {
                itSymbolUsed.toFront();
                while( itSymbolUsed.hasNext() )
                {
                    itSymbolUsed.next();
                    if( itSymbolUsed.value() == symbolCountsList.at(i) )
                    {
                        displayStrings.append( tr("%1 {count:%2}").arg(itSymbolUsed.key(), itSymbolUsed.value()));
                    }
                }
            }
            newLetterText = QInputDialog::getItem( this, // QWidget * parent
                                                           tr("Enter room marker"), // const QString & title
                                                           tr("Choose an existing marker\n"
                                                              "letter) from the list\n"
                                                              "(sorted by most commonly\n"
                                                              "used first) or enter a\n"
                                                              "new single ASCII\n"
                                                              "character (not space) for\n"
                                                              "all selected rooms:",
                                                              // Intentional comment to separate two strings!
                                                              "Use line feeds to format text into a reasonable rectangle."), // const QString & label
                                                           displayStrings, // QStringList & items
                                                           0, // int current = 0
                                                           true, // bool editable = true
                                                           &isOk, // bool * ok = 0
                                                           0, // Qt::WindowFlags flags = 0
                                                           Qt::ImhLatinOnly); // Qt::InputMethodHints inputMethodHints = Qt::ImhNone,
                                                              // to change when I rework the room symbols for any Unicode Grapheme!
        }
    }

    QString newLetter;
    if( isOk )
    {  // Don't proceed if cancel was pressed, will need revision to extract the
       // first grapheme when we support all printable unicode characters as room symbols!
        if( newLetterText.isEmpty() )
        {
            newLetter = QStringLiteral(" ");
        }
        else if( ! newLetterText.at(0).isHighSurrogate() )
        { // The first QChar of the QString is NOT the start of a Unicode character beyond the BMP!
            if( newLetterText.at(0).row() == 0 )
            { // The first QChar of the QString has a zero high byte
                if( newLetterText.at(0).cell() < 128 )
                { // The first QChar of the QString has a low byte within the range for ASCII
                    newLetter = newLetterText.at(0);
                }
                else
                {
                    isOk = false; // Prevent any change if the value is not reasonable
                }
            }
            else
            {
                isOk = false; // Prevent any change if the value is not reasonable
            }
        }
        else
        {
            isOk = false; // Prevent any change if the value is not reasonable
        }
    }

    if( isOk )
    {
        itRoom.toFront();
        while( itRoom.hasNext() )
        {
            pR = mpMap->mpRoomDB->getRoom( itRoom.next() ) ;
            if( !pR )
            {
                continue;
            }

            if( newLetter.at(0) == QStringLiteral(" ") )
            {
                if( pR->c )
                {
                    pR->c = 0;
                }
            }
            else
            {
                pR->c = newLetter.at(0).toLatin1();
            }
        }
    }
}

void T2DMap::slot_setImage()
{

}


void T2DMap::slot_deleteRoom()
{
    mpMap->mpRoomDB->removeRoom( mMultiSelectionSet );
    // mMultiSelectionSet gets cleared as rooms are removed by
    // TRoomDB::removeRoom() so no need to clear it here!
    mMultiRect = QRect(0,0,0,0);
    mMultiSelectionListWidget.clear();
    mMultiSelectionListWidget.hide();
    repaint();
}

void T2DMap::slot_selectRoomColor(QListWidgetItem * pI )
{
    mChosenRoomColor = pI->text().toInt();
}

void T2DMap::slot_defineNewColor()
{
    auto color = QColorDialog::getColor( mpHost->mRed, this );
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
    auto pD = new QDialog(this);
    auto pL = new QVBoxLayout;
    pD->setLayout( pL );
    pD->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    pD->setContentsMargins(0,0,0,0);
    auto pLW = new QListWidget(pD);
    pLW->setViewMode(QListView::IconMode);

    connect(pLW, SIGNAL(itemDoubleClicked(QListWidgetItem*)), pD, SLOT(accept()));
    connect(pLW, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slot_selectRoomColor(QListWidgetItem*)));

    pL->addWidget(pLW);
    auto pButtonBar = new QWidget(pD);

    auto pL2 = new QHBoxLayout;
    pButtonBar->setLayout( pL2 );
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    auto pB_newColor = new QPushButton(pButtonBar);
    pB_newColor->setText("define new color");

    connect(pB_newColor, SIGNAL(clicked()), pD, SLOT(reject()));
    connect(pB_newColor, SIGNAL(clicked()), this, SLOT(slot_defineNewColor()));

    pL2->addWidget(pB_newColor);

    auto pB_ok = new QPushButton(pButtonBar);
    pB_ok->setText("ok");
    pL2->addWidget(pB_ok);
    connect(pB_ok, SIGNAL(clicked()), pD, SLOT(accept()));

    auto pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText("abort");
    connect(pB_abort, SIGNAL(clicked()), pD, SLOT(reject()));
    pL2->addWidget(pB_abort);
    pL->addWidget(pButtonBar);

    QMapIterator<int, QColor> it(mpMap->customEnvColors);
    while( it.hasNext() )
    {
        it.next();
        QColor c;
        c = it.value();
        auto pI = new QListWidgetItem( pLW );
        QPixmap pix = QPixmap(50,50);
        pix.fill( c );
        QIcon mi( pix );
        pI->setIcon(mi);
        pI->setText(QString::number(it.key()));
        pLW->addItem(pI);
    }

    if( pD->exec() == QDialog::Accepted && mpMap->customEnvColors.contains( mChosenRoomColor) )
    { // Only proceed if OK - "abort" now prevents change AND check for a valid
      // color here rather than inside the room change loop as before (only test
      // once rather than for each room)
        mMultiRect = QRect(0,0,0,0);
        QSetIterator<int> itSelectedRoom( mMultiSelectionSet );
        while( itSelectedRoom.hasNext() )
        {
            TRoom * pR = mpMap->mpRoomDB->getRoom( itSelectedRoom.next() );
            if( pR )
            {
                pR->environment = mChosenRoomColor;
            }
        }

        update();
    }
}

void T2DMap::slot_spread()
{
    if( mMultiSelectionSet.size() < 2 )
    { // nothing to do!
        return;
    }

    TRoom * pR_centerRoom = mpMap->mpRoomDB->getRoom( mMultiSelectionHighlightRoomId );
    if( ! pR_centerRoom ) {
        return;
    }

    // Move the dialog down to here so it doesn't fire up for some already
    // determined to be null (no change) case, also handle "Cancel" being pressed
    bool isOk = false;
    int spread = QInputDialog::getInt( this,
                                       tr("Spread out rooms"),
                                       tr("Increase the spacing of\n"
                                          "the selected rooms,\n"
                                          "centered on the\n"
                                          "highlighted room by a\n"
                                          "factor of:"),
                                       5, // Initial value
                                       1, // Minimum value
                                       2147483647,// Maximum value
                                       1, // Step
                                       &isOk );
    if( spread == 1 || !isOk) {
        return;
    }

    mMultiRect = QRect(0,0,0,0);
    int dx = pR_centerRoom->x * ( 1 - spread );
    int dy = pR_centerRoom->y * ( 1 - spread );
    QSetIterator<int> itSelectionRoom = mMultiSelectionSet;
    while( itSelectionRoom.hasNext() )
    {
        TRoom * pMovingR = mpMap->mpRoomDB->getRoom( itSelectionRoom.next() );
        if( !pMovingR )
        {
            continue;
        }

        pMovingR->x *= spread;
        pMovingR->y *= spread;
        pMovingR->x += dx;
        pMovingR->y += dy;
        QMapIterator<QString, QList<QPointF> > itCustomLine( pMovingR->customLines );
        QMap<QString, QList<QPointF> > newCustomLinePointsMap;
        while( itCustomLine.hasNext() )
        {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for(auto & customLinePoint : customLinePoints)
            {
                QPointF movingPoint = customLinePoint;
                customLinePoint.setX( (float)(movingPoint.x()*spread+dx) );
                customLinePoint.setY( (float)(movingPoint.y()*spread+dy) );
            }
            newCustomLinePointsMap.insert( itCustomLine.key(), customLinePoints );
        }
        pMovingR->customLines = newCustomLinePointsMap;
        pMovingR->calcRoomDimensions();
    }
    repaint();
}

void T2DMap::slot_shrink()
{
    if( mMultiSelectionSet.size() < 2 )
    { // nothing to do!
        return;
    }

    TRoom * pR_centerRoom = mpMap->mpRoomDB->getRoom( mMultiSelectionHighlightRoomId );
    if( ! pR_centerRoom ) {
        return;
    }

    // Move the dialog down to here so it doesn't fire up for some already
    // determined to be null (no change) case, also handle "Cancel" being pressed
    bool isOk = false;
    int spread = QInputDialog::getInt( this,
                                       tr("Shrink in rooms"),
                                       tr("Decrease the spacing of\n"
                                          "the selected rooms,\n"
                                          "centered on the\n"
                                          "highlighted room by a\n"
                                          "factor of:"),
                                       5, // Initial value
                                       1, // Minimum value
                                       2147483647,// Maximum value
                                       1, // Step
                                       &isOk );
    if( spread == 1 || !isOk) {
        return;
    }

    mMultiRect = QRect(0,0,0,0);
    int dx = pR_centerRoom->x * ( 1 - 1 / spread );
    int dy = pR_centerRoom->y * ( 1 - 1 / spread );

    QSetIterator<int> itSelectionRoom( mMultiSelectionSet );
    while( itSelectionRoom.hasNext() )
    {
        TRoom * pMovingR = mpMap->mpRoomDB->getRoom( itSelectionRoom.next() );
        if( !pMovingR )
        {
            continue;
        }
        pMovingR->x /= spread;
        pMovingR->y /= spread;
        pMovingR->x += dx;
        pMovingR->y += dy;
        QMapIterator<QString, QList<QPointF> > itCustomLine( pMovingR->customLines );
        QMap<QString, QList<QPointF> > newCustomLinePointsMap;
        while( itCustomLine.hasNext() )
        {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for(auto & customLinePoint : customLinePoints)
            {
                QPointF movingPoint = customLinePoint;
                customLinePoint.setX( (float)(movingPoint.x()/spread+dx) );
                customLinePoint.setY( (float)(movingPoint.y()/spread+dy) );
            }
            newCustomLinePointsMap.insert( itCustomLine.key(), customLinePoints );
        }
        pMovingR->customLines = newCustomLinePointsMap;
        pMovingR->calcRoomDimensions();
    }
    repaint();
}

void T2DMap::slot_setExits()
{
    if( mMultiSelectionSet.size() < 1 )
    {
        return;
    }
    if( mpMap->mpRoomDB->getRoom( mMultiSelectionHighlightRoomId ) )
    {
        auto pD = new dlgRoomExits( mpHost, this );
        pD->init( mMultiSelectionHighlightRoomId );
        pD->show();
        pD->raise();
    }
}


void T2DMap::slot_setUserData()
{

}

void T2DMap::slot_lockRoom()
{
    if( mMultiSelectionSet.size() < 1 ) {
        return;
    }

    mMultiRect = QRect(0,0,0,0);
    QSetIterator<int> itSelectedRoom( mMultiSelectionSet );
    while( itSelectedRoom.hasNext() )
    {
        TRoom * pR = mpMap->mpRoomDB->getRoom( itSelectedRoom.next() );
        if( pR )
        {
            pR->isLocked = true;
            mpMap->mMapGraphNeedsUpdate = true;
        }
    }
}

void T2DMap::slot_unlockRoom()
{
    if( mMultiSelectionSet.size() < 1 ) {
        return;
    }

    mMultiRect = QRect(0,0,0,0);
    QSetIterator<int> itSelectedRoom( mMultiSelectionSet );
    while( itSelectedRoom.hasNext() )
    {
        TRoom * pR = mpMap->mpRoomDB->getRoom( itSelectedRoom.next() );
        if( pR )
        {
            pR->isLocked = false;
            mpMap->mMapGraphNeedsUpdate = true;
        }
    }
}

void T2DMap::slot_setRoomWeight()
{
    if( mMultiSelectionSet.isEmpty() )
    {
        return;
    }

    // First scan and count all the different weights used
    QMap<uint, uint> usedWeights; // key is weight, value is count of uses
    QSetIterator<int> itSelectedRoom = mMultiSelectionSet;
    TRoom * pR;
    while( itSelectedRoom.hasNext() )
    {
        pR = mpMap->mpRoomDB->getRoom( itSelectedRoom.next() );
        if( !pR )
        {
            continue;
        }

        int w = pR->getWeight();
        if( w > 0 )
        {
            if( usedWeights.contains( w ) )
            {
                usedWeights[w] += 1;
            }
            else
            {
                usedWeights[w] = 1;
            }
        }
    }

    int newWeight = 1;
    bool isOk = false;
    // Choose most appropriate weight dialog on number of rooms selected and
    // how many of them have different weights already:
    if( mMultiSelectionSet.size() == 1)
    { // Just one room selected
        newWeight = QInputDialog::getInt(this,
                                         tr( "Enter room weight" ),
                                         tr( "Enter new roomweight\n"
                                             "(= travel time), mimimum\n"
                                             "(and default) is 1:", "Use line feeds to format text into a reasonable rectangle." ),
                                         usedWeights.keys().first(),
                                         1,
                                         2147483647,
                                         1,
                                         &isOk);
    }
    else
    { // More than one room selected
        if( usedWeights.size() == 1 )
        {
            newWeight = QInputDialog::getInt(this,
                                             tr( "Enter room weight" ),
                                             tr( "Enter new roomweight\n"
                                                 "(= travel time) for all\n"
                                                 "selected rooms, mimimum\n"
                                                 "(and default) is 1 and\n"
                                                 "the only current value\n"
                                                 "used is:", "Use line feeds to format text into a reasonable rectangle." ),
                                             usedWeights.keys().first(),
                                             1,
                                             2147483647,
                                             1,
                                             &isOk);
        }
        else
        {
            QMapIterator<uint, uint> itWeightsUsed = usedWeights;
            // Obtain a set of "used" weights
            QSet<uint> weightCountsSet;
            while( itWeightsUsed.hasNext() )
            {
                itWeightsUsed.next();
                weightCountsSet.insert( itWeightsUsed.value() );
            }
            // Obtains a list of those weights sorted in ascending count of used
            QList<uint> weightCountsList = weightCountsSet.toList();
            if( weightCountsList.size() > 1 )
            {
                std::sort( weightCountsList.begin(), weightCountsList.end() );
            }
            // Build a list of the "used" weights in decending count of use
            QStringList displayStrings;
            for( int i = weightCountsList.size()-1; i >= 0; --i )
            {
                itWeightsUsed.toFront();
                while( itWeightsUsed.hasNext() )
                {
                    itWeightsUsed.next();
                    if( itWeightsUsed.value() == weightCountsList.at(i) )
                    {
                        if( itWeightsUsed.key() == 1 )
                        { // Indicate the "default" value which is unity weight
                            displayStrings.append( tr("%1 {count:%2, default}").arg(itWeightsUsed.key(), itWeightsUsed.value()));
                        }
                        else
                        {
                            displayStrings.append( tr("%1 {count:%2}").arg(itWeightsUsed.key(), itWeightsUsed.value()));
                        }
                    }
                }
            }
            if( ! usedWeights.contains(1) )
            { // If unity weight was not used insert it at end of list
                displayStrings.append( tr("1 {count 0, default}") );
            }
            QString newWeightText = QInputDialog::getItem( this, // QWidget * parent
                                                           tr("Enter room weight"), // const QString & title
                                                           tr("Choose an existing\n"
                                                              "roomweight (= travel\n"
                                                              "time) from the list\n"
                                                              "(sorted by most commonly\n"
                                                              "used first) or enter a\n"
                                                              "new (positive) integer\n"
                                                              "value for all selected\n"
                                                              "rooms:", "Use line feeds to format text into a reasonable rectangle."), // const QString & label
                                                           displayStrings, // QStringList & items
                                                           0, // int current = 0, last value in list
                                                           true, // bool editable = true
                                                           &isOk, // bool * ok = 0
                                                           0, // Qt::WindowFlags flags = 0
                                                           Qt::ImhDigitsOnly); // Qt::InputMethodHints inputMethodHints = Qt::ImhNone
            newWeight = 1;
            if( isOk )
            { // Don't do anything if cancel was pressed
                if( newWeightText.toInt() > 0 )
                {
                    newWeight = newWeightText.toInt();
                }
                else
                {
                    isOk = false; // Prevent any change if the value is not reasonable
                }
            }
        }
    }

    if( isOk && newWeight > 0 )
    { // Don't proceed if cancel was pressed or the value is not valid
        itSelectedRoom.toFront();
        while( itSelectedRoom.hasNext() )
        {
            pR = mpMap->mpRoomDB->getRoom( itSelectedRoom.next() ) ;
            if( !pR ) {
                continue;
            }

            pR->setWeight( newWeight );
        }
        mpMap->mMapGraphNeedsUpdate = true;
        repaint();
    }
}

void T2DMap::slot_setArea()
{
    QUiLoader loader;

    QFile file(":/ui/set_room_area.ui");
    file.open(QFile::ReadOnly);
    QDialog *set_room_area_dialog = dynamic_cast<QDialog *>(loader.load(&file, this));
    file.close();
    if( ! set_room_area_dialog ) {
        return;
    }
    arealist_combobox = set_room_area_dialog->findChild<QComboBox*>("arealist_combobox");
    if( !arealist_combobox ) {
        return;
    }

    QMapIterator<int, QString> it( mpMap->mpRoomDB->getAreaNamesMap() );
    while( it.hasNext() ) {
        it.next();
        int areaID = it.key();
        if( areaID > 0 ) {
            arealist_combobox->addItem( QStringLiteral( "%1 (%2)" ).arg( it.value(), QString::number(areaID) ), QVariant(areaID) );
        }
    }

    if( set_room_area_dialog->exec() == QDialog::Rejected )
    { // Don't proceed if "cancel" was pressed
        return;
    }

    int newAreaId = arealist_combobox->itemData( arealist_combobox->currentIndex() ).toInt();
    mMultiRect = QRect(0,0,0,0);
    QSetIterator<int> itSelectedRoom = mMultiSelectionSet;
    while( itSelectedRoom.hasNext() )
    {
        int currentRoomId = itSelectedRoom.next();
        if( itSelectedRoom.hasNext() )
        { // NOT the last room in set -  so defer some area related recalculations
            mpMap->setRoomArea( currentRoomId, newAreaId, true );
        }
        else
        {
            // Is the LAST room, so be careful to do all that is needed to clean
            // up the affected areas (triggered by last "false" argument in next
            // line)...
            if( ! ( mpMap->setRoomArea( currentRoomId, newAreaId, false ) ) )
            {
                // Failed on the last of multiple room area move so do the missed
                // out recalculations for the dirtied areas
                QSetIterator<TArea *> itpArea = mpMap->mpRoomDB->getAreaPtrList().toSet();
                while( itpArea.hasNext() ) {
                    TArea * pArea = itpArea.next();
                    if( pArea->mIsDirty ) {
                        pArea->determineAreaExits();
                        pArea->calcSpan();
                        pArea->mIsDirty =  false;
                    }
                }
            }
        }
    }
    repaint();
}


void T2DMap::mouseMoveEvent( QMouseEvent * event )
{
    if( mpMap->mLeftDown && !mpMap->m2DPanMode && event->modifiers().testFlag(Qt::AltModifier) )
    {
        mpMap->m2DPanXStart = event->x();
        mpMap->m2DPanYStart = event->y();
        mpMap->m2DPanMode=true;
    }
    if( mpMap->m2DPanMode && !event->modifiers().testFlag(Qt::AltModifier))
    {
        mpMap->m2DPanMode = false;
        mpMap->mLeftDown = false;
    }
    if( mpMap->m2DPanMode )
    {
        int x = event->x();
        int y = height()-event->y();
        if( (mpMap->m2DPanXStart-x) > 1)
        {
            shiftRight();
            mpMap->m2DPanXStart = x;
        }
        else if( (mpMap->m2DPanXStart-x) < -1 )
        {
            shiftLeft();
            mpMap->m2DPanXStart = x;
        }
        if( (mpMap->m2DPanYStart-y) > 1)
        {
            shiftDown();
            mpMap->m2DPanYStart = y;
        }
        else if ((mpMap->m2DPanYStart-y) < -1)
        {
            shiftUp();
            mpMap->m2DPanYStart = y;
        }
        return;
    }

    if( mCustomLineSelectedRoom != 0 && mCustomLineSelectedPoint >= 0 )
    {
        TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if( pR )
        {
            if( pR->customLines.contains( mCustomLineSelectedExit) )
            {
                if( pR->customLines[mCustomLineSelectedExit].size()> mCustomLineSelectedPoint )
                {
                    float mx = event->pos().x()/mTX + mOx;
                    float my = event->pos().y()/mTY + mOy;
                    mx = mx - xspan/2;
                    my = yspan/2 - my;
                    QPointF pc = QPointF(mx,my);
                    pR->customLines[mCustomLineSelectedExit][mCustomLineSelectedPoint] = pc;
                    pR->calcRoomDimensions();
                    repaint();
                    return;
                }
            }
        }
    }

    mCustomLineSelectedPoint = -1;

    //FIXME:
    if(  mLabelHilite )//mMoveLabel )//&& mLabelHilite )
    {
        if( mpMap->mapLabels.contains( mAID ) )
        {
            QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAID]);
            while( it.hasNext() )
            {
                it.next();
                if( it.value().pos.z() != mOz ) continue;
                if( ! it.value().hilite ) continue;
                int mx = event->pos().x()/mTX + mOx;
                int my = event->pos().y()/mTY + mOy;
                mx = mx - xspan/2;
                my = yspan/2 - my;
                QVector3D p = QVector3D(mx,my,mOz);
                mpMap->mapLabels[mAID][it.key()].pos = p;
            }
        }
        update();
    }
    else
        mMoveLabel = false;

    if( (mMultiSelection && ! mRoomBeingMoved) || mSizeLabel )
    {
        //    (The drag to select (or size) rectangle is being actively resized and rooms are not being moved)
        // OR (We are sizing up a label)
        if( mNewMoveAction )
        {
            mMultiRect = QRect(event->pos(), event->pos());
            mNewMoveAction = false;
        }
        else
            mMultiRect.setBottomLeft( event->pos() );

        int _roomID = mRID;
        if( ! mpMap->mpRoomDB->getRoom( _roomID ) ) return;
        int _areaID = mAID;
        TArea * pArea = mpMap->mpRoomDB->getArea(_areaID);
        if( ! pArea ) return;

        float _rx = xspan/2*mTX-mTX*mOx;
        float _ry = yspan/2*mTY-mTY*mOy;

        if( ! mSizeLabel )
        { // NOT sizing a label
            mMultiSelectionSet.clear();
            QSetIterator<int> itSelectedRoom( pArea->getAreaRooms() );
            while( itSelectedRoom.hasNext() )
            {
                int currentRoomId = itSelectedRoom.next();
                TRoom * pR = mpMap->mpRoomDB->getRoom( currentRoomId );
                if( !pR ) continue;
                int rx = pR->x*mTX+_rx;
                int ry = pR->y*-1*mTY+_ry;
                int rz = pR->z;

                // copy rooms on all z-levels if the shift key is being pressed
                // CHECK: Consider adding z-level to multi-selection Widget?
                if( rz != mOz && ! ( event->modifiers().testFlag(Qt::ShiftModifier)) ) continue;

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
                    mMultiSelectionSet.insert( currentRoomId );
                }
            }
            switch( mMultiSelectionSet.size() ) {
            case 0:
                mMultiSelectionHighlightRoomId = -1;
                break;
            case 1:
                mMultiSelectionHighlightRoomId = mMultiSelectionSet.toList().first();
                break;
            default:
                getCenterSelection(); // Sets mMultiSelectionHighlightRoomId to (a) central room
            }

            int mx = event->pos().x()/mTX;
            int my = event->pos().y()/mTY;
            mx = mx - xspan/2 + 1;
            my = yspan/2 - my - 1;
            TRoom * pR = mpMap->mpRoomDB->getRoom(mRID);
            if( pR )
            {
                mx += pR->x;
                my += pR->y;
                mOldMousePos = QPoint(mx,my);
            }

            if( mMultiSelectionSet.size() > 1 )
            {
                mMultiSelectionListWidget.blockSignals(true); // We don't want to cause calls to slot_roomSelectionChanged() here!
                mIsSelectionSorting = mMultiSelectionListWidget.isSortingEnabled(); // Save sorting state before we switch it off
                mIsSelectionSortByNames = (mMultiSelectionListWidget.sortColumn() == 1);
                mMultiSelectionListWidget.clear();
                mMultiSelectionListWidget.setSortingEnabled(false); // Do NOT sort whilst inserting items!
                QSetIterator<int> itRoom = mMultiSelectionSet;
                mIsSelectionUsingNames = false;
                while( itRoom.hasNext() )
                {
                    auto _item = new QTreeWidgetItem;
                    int multiSelectionRoomId = itRoom.next();
                    _item->setText(0,QStringLiteral("%1").arg(multiSelectionRoomId,7));
                    _item->setTextAlignment(0, Qt::AlignRight);
                    TRoom * pR_multiSelection = mpMap->mpRoomDB->getRoom(multiSelectionRoomId);
                    if( pR_multiSelection )
                    {
                        QString multiSelectionRoomName = pR_multiSelection->name;
                        if( ! multiSelectionRoomName.isEmpty() )
                        {
                            _item->setText(1,multiSelectionRoomName);
                            _item->setTextAlignment(1, Qt::AlignLeft);
                            mIsSelectionUsingNames = true;
                        }
                    }
                    mMultiSelectionListWidget.addTopLevelItem( _item );
                }
                mMultiSelectionListWidget.setColumnHidden(1, !mIsSelectionUsingNames );
                // Can't sort if nothing to sort on, switch to sorting by room number
                if( (! mIsSelectionUsingNames ) && mIsSelectionSortByNames && mIsSelectionSorting )
                {
                    mIsSelectionSortByNames = false;
                }
                mMultiSelectionListWidget.sortByColumn( mIsSelectionSortByNames ? 1 : 0 );
                mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
                resizeMultiSelectionWidget();
                mMultiSelectionListWidget.selectAll();
                mMultiSelectionListWidget.blockSignals(false);
                mMultiSelectionListWidget.show();
            }
            else
            {
                mMultiSelectionListWidget.hide();
            }
        }

        update();
        return;
    }

    if( mRoomBeingMoved && !mSizeLabel && ! mMultiSelectionSet.isEmpty() )
    {
        mMultiRect = QRect(0,0,0,0);
        int _roomID = mRID;
        if( ! mpMap->mpRoomDB->getRoom( _roomID ) ) return;
        int _areaID = mAID;
        TArea * pArea = mpMap->mpRoomDB->getArea(_areaID);
        if( ! pArea ) return;

// N/U:        int ox = mOx;
// N/U:        int oy = mOy;
// N/U:        float _rx;
// N/U:        float _ry;
// These tests are pointless - both branches produce the same results
//        if( ox*mTX > xspan/2*mTX )
//            _rx = -(mTX*ox-xspan/2*mTX);
//        else
//            _rx = xspan/2*mTX-mTX*ox;
//        if( oy*mTY > yspan/2*mTY )
//            _ry = -(mTY*oy-yspan/2*mTY);
//        else
//            _ry = yspan/2*mTY-mTY*oy;
        int mx = event->pos().x()/mTX + mOx;
        int my = event->pos().y()/mTY + mOy;
        mx = mx - xspan/2 + 1;
        my = yspan/2 - my - 1;

        int dx,dy;

        if( ! getCenterSelection() ) {
            return;
        }

        TRoom * pR = mpMap->mpRoomDB->getRoom( mMultiSelectionHighlightRoomId );
        if( !pR )
        {
            return;
        }

        dx = mx - pR->x;
        dy = my - pR->y;
        QSetIterator<int> itRoom = mMultiSelectionSet;
        while( itRoom.hasNext() )
        {
            pR = mpMap->mpRoomDB->getRoom( itRoom.next() );
            if( pR )
            {
                pR->x += dx;
                pR->y += dy;
                pR->z = mOz; // allow groups to be moved to a different z-level with the map editor

                QMapIterator<QString, QList<QPointF> > itk(pR->customLines);
                QMap<QString, QList<QPointF> > newMap;
                while( itk.hasNext() )
                {
                    itk.next();
                    QList<QPointF> _pL = itk.value();
                    for(auto & point : _pL)
                    {
                        QPointF op = point;
                        point.setX( (float)(op.x()+dx) );
                        point.setY( (float)(op.y()+dy) );
                    }
                    newMap.insert(itk.key(), _pL );
                }
                pR->customLines = newMap;
                pR->calcRoomDimensions();
            }
        }
        repaint();
    }
}

// Replacement for getTopLeftCenter - determines a room closest to geometrical
// mean of all the room selected, the result is stored in the class member
// mMultiSelectionHighlightRoomId and this returns true on successfully finding
// such a room
bool T2DMap::getCenterSelection()
{
    mMultiSelectionHighlightRoomId = -1;
    if( mMultiSelectionSet.isEmpty() ) {
        return false;
    }

    QSetIterator<int> itRoom = mMultiSelectionSet;
    float mean_x = 0.0;
    float mean_y = 0.0;
    float mean_z = 0.0;
    uint processedRoomCount = 0;
    while( itRoom.hasNext() ) {
        int currentRoomId = itRoom.next();
        TRoom * pR = mpMap->mpRoomDB->getRoom( currentRoomId );
        if( ! pR ) {
            continue;
        }

        mean_x += (static_cast<float>(pR->x - mean_x)) / ++processedRoomCount;
        mean_y += (static_cast<float>(pR->y - mean_y)) / processedRoomCount;
        mean_z += (static_cast<float>(pR->z - mean_z)) / processedRoomCount;
    }

    if( processedRoomCount ) {
        itRoom.toFront();
        float closestSquareDistance = -1.0;
        while( itRoom.hasNext() ) {
            int currentRoomId = itRoom.next();
            TRoom * pR = mpMap->mpRoomDB->getRoom( currentRoomId );
            if( ! pR ) {
                continue;
            }

            QVector3D meanToRoom( static_cast<float>(pR->x)-mean_x, static_cast<float>(pR->y)-mean_y, static_cast<float>(pR->z)-mean_z );
            if( closestSquareDistance < -0.5 ) {
                // Don't use an equality to zero test, we are using floats so
                // need to allow for a little bit of fuzzzyness!
                closestSquareDistance = meanToRoom.lengthSquared();
                mMultiSelectionHighlightRoomId = currentRoomId;
            }
           else {
                float currentRoomSquareDistance = meanToRoom.lengthSquared();
                if( closestSquareDistance > currentRoomSquareDistance  ) {
                    closestSquareDistance = currentRoomSquareDistance;
                    mMultiSelectionHighlightRoomId = currentRoomId;
                }
            }
        }
        return true;
    }
    else {
        return false;
    }
}

void T2DMap::exportAreaImage( int id )
{
    paintMap();
}

void T2DMap::wheelEvent ( QWheelEvent * e )
{
    // If the mouse wheel is scrolling up and down through the
    // mMultiSelectionListWidget the wheelevents from that get passed up to here
    // when the end of the list is reached (i.e when it rejects those events)
    // - so that the mapper window doesn't THEN zoom in or out, swallow the
    // events, i.e. accept() them and return before hitting the zoom altering
    // code that follows.
    // However the event "pos()" depends on the widget it came from so we have
    // to use "globalPos()" instead and see how it lies in relation to the child
    // widget:
    QRect selectionListWidgetGlobalRect = QRect(mapToGlobal(mMultiSelectionListWidget.frameRect().topLeft()), mapToGlobal(mMultiSelectionListWidget.frameRect().bottomRight()));
    if( mMultiSelectionListWidget.isVisible() && selectionListWidgetGlobalRect.contains( e->globalPos() ) )
    {
        e->accept();
        return;
    }

    if( ! (mpMap->mpRoomDB->getRoom(mRID) && mpMap->mpRoomDB->getArea(mAID) ) )
    {
        return;
    }

    // int delta = e->delta() / 8 / 15; // Deprecated in Qt 5.x ...!
    int delta = e->angleDelta().y() / ( 8 * 15 );
    if( e->modifiers() & Qt::ControlModifier )
    { // Increase rate 10-fold if control key down - it makes scrolling through
      // a large nuber of items in a listwidget's contents easier AND this make it
      // easier to zoom in and out on LARGE area maps
        delta *= 10;
    }
    if( delta != 0 )
    {
        mPick = false;
        xzoom += delta;
        yzoom += delta;
        if( yzoom < 3 )
        { // At present xzoom and yzoom alway will be the same I think
            yzoom = 3;
        }
        if( xzoom < 3 )
        {
            xzoom = 3;
        }
        update();
        e->accept();
        return;
    }
    e->ignore();
    return;
}

void T2DMap::setMapZoom( int zoom )
{
    xzoom = zoom;
    yzoom = zoom;
    if( yzoom < 3 || xzoom < 3 )
    {
        xzoom = 3;
        yzoom = 3;
    }
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

void T2DMap::slot_setCustomLine()
{
    if( mMultiSelectionSet.isEmpty() ) {
        return;
    }
    TRoom * pR = mpMap->mpRoomDB->getRoom( mMultiSelectionHighlightRoomId );
    if( !pR ) {
        return;
    }

    if( mpCustomLinesDialog ) {
        // Refuse to create another instance if one is already present!
        // Just show it...
        mpCustomLinesDialog->raise();
        return;
    }

    QUiLoader loader;

    QFile file(":/ui/custom_lines.ui");
    file.open(QFile::ReadOnly);
    QDialog *d = dynamic_cast<QDialog *>(loader.load(&file, this));
    file.close();
    if( ! d ) {
        return;
    }
    d->setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_custom_exit.png" ) ) );
    mCustomLinesRoomFrom = mMultiSelectionHighlightRoomId;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit = "";
    QPushButton * b_ = d->findChild<QPushButton*>("nw");
    QTreeWidget * specialExits = d->findChild<QTreeWidget*>("specialExits");
    mpCurrentLineStyle = d->findChild<QComboBox*>("lineStyle");
    mpCurrentLineColor = d->findChild<QPushButton*>("lineColor");
    mpCurrentLineArrow = d->findChild<QCheckBox*>("arrow");
    if( ! b_
        || ! specialExits
        || ! mpCurrentLineColor
        || ! mpCurrentLineStyle
        || ! mpCurrentLineArrow )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"nw\" exit line button or another element of the dialog!");
        return;
    }
    else if( pR->getNorthwest() <= 0 )
    {
        b_->setCheckable(false);
        b_->setDisabled(true);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("NW")||pR->customLines.contains("nw") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("n");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"n\" exit line button!");
        return;
    }
    else if( pR->getNorth() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("N")||pR->customLines.contains("n") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("ne");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"ne\" exit line button!");
        return;
    }
    else if( pR->getNortheast() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("NE")||pR->customLines.contains("ne") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("up");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"up\" exit line button!");
        return;
    }
    else if( pR->getUp() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("UP")||pR->customLines.contains("up") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("w");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"w\" exit line button!");
        return;
    }
    else if( pR->getWest() <= 0 )
    {
        b_->setCheckable(false);
        b_->setDisabled(true);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("W")||pR->customLines.contains("w") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("e");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"e\" exit line button!");
        return;
    }
    else if( pR->getEast() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("E")||pR->customLines.contains("e") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("down");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"down\" exit line button!");
        return;
    }
    else if( pR->getDown() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("DOWN")||pR->customLines.contains("down") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("sw");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"sw\" exit line button!");
        return;
    }
    else if( pR->getSouthwest() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("SW")||pR->customLines.contains("sw") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("s");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"s\" exit line button!");
        return;
    }
    else if( pR->getSouth() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("S")||pR->customLines.contains("s") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("se");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"se\" exit line button!");
        return;
    }
    else if( pR->getSoutheast() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("SE")||pR->customLines.contains("se") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("in");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"in\" exit line button!");
        return;
    }
    else if( pR->getIn() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("IN")||pR->customLines.contains("in") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    b_ = d->findChild<QPushButton*>("out");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"out\" exit line button!");
        return;
    }
    else if( pR->getOut() <= 0 )
    {
        b_->setDisabled(true);
        b_->setCheckable(false);
    }
    else
    {
        b_->setCheckable(true);
        b_->setChecked( pR->customLines.contains("OUT")||pR->customLines.contains("out") );
        connect(b_, SIGNAL(clicked()), this, SLOT(slot_setCustomLine2()));
    }

    QMapIterator<int, QString> it(pR->getOtherMap());
    while( it.hasNext() )
    {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        if( dir.size() > 1 )
            if( dir.startsWith('0')|| dir.startsWith('1') )
                dir = dir.mid(1);
        auto pI = new QTreeWidgetItem(specialExits);
        if( pR->customLines.contains(dir) )
            pI->setCheckState( 0, Qt::Checked );
        else
            pI->setCheckState( 0, Qt::Unchecked );
        pI->setTextAlignment( 0, Qt::AlignHCenter );
        pI->setText( 1, QString::number(id_to) );
        pI->setTextAlignment( 1, Qt::AlignRight );
        pI->setText( 2, dir );
        pI->setTextAlignment( 2, Qt::AlignLeft );
    }

    b_ = d->findChild<QPushButton*>("button_cancel");
    if( !b_ )
    {
        qWarning("T2DMap::slot_setCustomLine() ERROR: failed to find \"cancel\" button!");
        return;
    }
    connect(b_, SIGNAL(clicked()), d, SLOT(reject()));
    connect(d, SIGNAL(rejected()), this, SLOT(slot_cancelCustomLineDialog()));

    QStringList _lineStyles;
    _lineStyles << "solid line" << "dot line" << "dash line" << "dash dot line" << "dash dot dot line";
    mpCurrentLineStyle->addItems(_lineStyles);
    mpCurrentLineStyle->setCurrentText( mCurrentLineStyle );
    mpCurrentLineArrow->setChecked(mCurrentLineArrow);
    mpCurrentLineColor->setStyleSheet("background-color:" + mCurrentLineColor.name());
    connect(specialExits, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(slot_setCustomLine2B(QTreeWidgetItem*, int)));
    connect(mpCurrentLineColor, SIGNAL(clicked()), this, SLOT(slot_customLineColor()));
    mpCustomLinesDialog = d; // Don't assign the pointer value to the class memeber until ready to go
    mpCustomLinesDialog->show();
    mpCustomLinesDialog->raise();
    mDialogLock = true; // Prevent any line drawing until dialog has been used
}

void T2DMap::slot_customLineColor()
{
    QColor color;
    if( mCurrentLineColor.isValid() )
        color = QColorDialog::getColor( mCurrentLineColor, this );
    else
        color = QColorDialog::getColor( mpHost->mFgColor_2, this );

    if ( color.isValid() )
    {
        mCurrentLineColor = color;
        QString styleSheet = QString("background-color:"+color.name());
        mpCurrentLineColor->setStyleSheet( styleSheet );
    }
}

// Called by dialog's reject event which is caused at least by "X" button on
// title bar and by ESC keypress...
void T2DMap::slot_cancelCustomLineDialog()
{
    mpCustomLinesDialog->deleteLater();
    mpCustomLinesDialog = 0;
    mCustomLinesRoomFrom = 0;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit.clear();
    mDialogLock = false;
}

void T2DMap::slot_setCustomLine2()
{
    QPushButton* pB = qobject_cast<QPushButton *>( sender() );
    if( ! pB )
    {
        if( mpCustomLinesDialog ) {
            mpCustomLinesDialog->reject();
        }
        else {
            // This is needed to escape from custom line exit drawing mode if
            // the dialog has disappeared, not likely I think/hope
            mCustomLinesRoomFrom = 0;
            mCustomLinesRoomTo = 0;
            mCustomLinesRoomExit.clear();
            mDialogLock = false;
        }
        return;
    }
    QString exit = pB->text();
    mpCustomLinesDialog->hide(); // Hide but don't delete until done the custom line
    mCustomLinesRoomExit = exit;
    mDialogLock = false;
    TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
    if( ! pR )
        return;
    if( exit == "NW" )
        mCustomLinesRoomTo = pR->getNorthwest();  // mCustomLinesRoomTo - wasn't being set!
    else if( exit == "N" )
        mCustomLinesRoomTo = pR->getNorth();
    else if( exit == "NE" )
        mCustomLinesRoomTo = pR->getNortheast();
    else if( exit == "UP" )
        mCustomLinesRoomTo = pR->getUp();
    else if( exit == "W" )
        mCustomLinesRoomTo = pR->getWest();
    else if( exit == "E" )
        mCustomLinesRoomTo = pR->getEast();
    else if( exit == "DOWN" )
        mCustomLinesRoomTo = pR->getDown();
    else if( exit == "SW" )
        mCustomLinesRoomTo = pR->getSouthwest();
    else if( exit == "S" )
        mCustomLinesRoomTo = pR->getSouth();
    else if( exit == "SE" )
        mCustomLinesRoomTo = pR->getSoutheast();
    else if( exit == "IN" )
        mCustomLinesRoomTo = pR->getIn();
    else if( exit == "OUT" )
        mCustomLinesRoomTo = pR->getOut();
    else
    {
        qWarning("T2DMap::slot_setCustomLine2(): unable to identify exit \"%s\"to use!", qPrintable(exit));
        return;
    }
    QList<QPointF> _list;
    pR->customLines[exit] = _list;
    QList<int> _colorList;
//    qDebug("T2DMap::slot_setCustomLine2() NORMAL EXIT: %s", qPrintable(exit));
    _colorList << mCurrentLineColor.red() << mCurrentLineColor.green() << mCurrentLineColor.blue();
    pR->customLinesColor[exit] = _colorList;
/*
 *    qDebug("   COLOR(r,g,b): %i,%i,%i",
 *            mCurrentLineColor.red(),
 *            mCurrentLineColor.green(),
 *            mCurrentLineColor.blue() );
 */
    pR->customLinesStyle[exit] = mCurrentLineStyle;
//    qDebug("   LINE STYLE: %s", qPrintable(mCurrentLineStyle) );
    pR->customLinesArrow[exit] = mCurrentLineArrow;
//    qDebug("   ARROW: %s", mCurrentLineArrow ? "Yes" : "No");

    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...");
    // This message was previously being put up AFTER first click to set first segment was made....
    update();
}

void T2DMap::slot_setCustomLine2B(QTreeWidgetItem * special_exit, int column )
{
    Q_UNUSED( column );
    if( ! special_exit )
        return;
    QString exit = special_exit->text(2);
    mpCustomLinesDialog->hide(); // Hide but don't delete until done the custom line
    mCustomLinesRoomExit = exit;
    mCustomLinesRoomTo = special_exit->text(1).toInt(); // Wasn't being set !
    mDialogLock = false;
    TRoom * pR = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
    if( ! pR )
        return;
    QList<QPointF> _list;
    pR->customLines[exit] = _list;
    QList<int> _colorList;
    _colorList << mCurrentLineColor.red() << mCurrentLineColor.green() << mCurrentLineColor.blue();
//    qDebug("T2DMap::slot_setCustomLine2B() SPECIAL EXIT: %s", qPrintable(exit));
    pR->customLinesColor[exit] = _colorList;
/*
 *     qDebug("   COLOR(r,g,b): %i,%i,%i",
 *            mCurrentLineColor.red(),
 *            mCurrentLineColor.green(),
 *            mCurrentLineColor.blue() );
 */
    pR->customLinesStyle[exit] = mCurrentLineStyle;
//    qDebug("   LINE STYLE: %s", qPrintable(mCurrentLineStyle) );
    pR->customLinesArrow[exit] = mCurrentLineArrow;
//    qDebug("   ARROW: %s", mCurrentLineArrow ? "Yes" : "No");
    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...");
    // This message was previously being put up AFTER first click to set first segment was made....
    update();
}

void T2DMap::slot_createLabel()
{
    if( ! mpMap->mpRoomDB->getArea( mAID ) ) return;

    mHelpMsg = tr("Left-click and drag a square for the size and position of your label");
    mSizeLabel = true;
    mMultiSelection = true;
    update();
}

void T2DMap::slot_roomSelectionChanged()
{
    QList<QTreeWidgetItem *> _sl = mMultiSelectionListWidget.selectedItems();
    mMultiSelectionSet.clear();
    for(auto treeWidgetItem : _sl) {
        int currentRoomId = treeWidgetItem->text(0).toInt();
        mMultiSelectionSet.insert( currentRoomId );
    }
    switch( mMultiSelectionSet.size() ) {
    case 0:
        mMultiSelectionHighlightRoomId = -1;
        break;
    case 1:
        mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.constBegin());
        break;
    default:
        getCenterSelection();
    }
    update();
}

void T2DMap::paintMap()
{
//    if( !mpMap ) return;
//    bool __Pick = mPick;
//    QTime __time; __time.start();

//    mAreaExitList.clear();
//    int px,py;
//    QList<int> exitList;
//    QList<int> oneWayExits;

//    int ox, oy, oz;

//    if( !mpMap ) return;
//    if( !mpMap->areas.contains(mAID)) return;

//    mpMap->areas[mAID]->calcSpan();
//    int x_min;
//    int y_min;
//    int x_max;
//    int y_max;
//    if( ! mpMap->areas[mAID]->xminEbene.contains(mOz) )
//    {
//        ox = 0;
//        oy = 0;
//        oz = 0;
//    }
//    else
//    {
//        x_min = mpMap->areas[mAID]->xminEbene[mOz];
//        y_min = mpMap->areas[mAID]->yminEbene[mOz];
//        x_max = mpMap->areas[mAID]->xmaxEbene[mOz];
//        y_max = mpMap->areas[mAID]->ymaxEbene[mOz];
//        ox = x_min + ( abs( x_max - x_min ) / 2 );
//        oy = ( y_min + ( abs( y_max - y_min ) / 2 ) ) * -1;
//        oz = 0;
//    }

//    int sizex, sizey;
//    if( x_max < 0 ) sizex = abs(abs(x_min)-abs(x_max));
//    if( x_max > 0 && x_min > 0 ) sizex = x_max-x_min;
//    if( x_max >= 0 && x_min <= 0 ) sizex = abs(x_max+abs(x_min));

//    if( y_max < 0 ) sizey = abs(abs(y_min)-abs(y_max));
//    if( y_max > 0 && y_min > 0 ) sizey = y_max-y_min;
//    if( y_max >= 0 && y_min <= 0 ) sizey = abs(y_max+abs(y_min));

//    sizex += 10;
//    sizey += 10;

//    if( sizex > sizey )
//        sizey = sizex;
//    else
//        sizex = sizey;

//    xzoom = 30;
//    yzoom = 30;

//    // Qt png limits
//    //if( sizex*xzoom >= 32768 )
//        xzoom = 10000/sizex;
//    //if( sizey*yzoom >= 32768 )
//        yzoom = 10000/sizey;

//    if(xzoom > yzoom)
//        xzoom = yzoom;
//    else
//        yzoom = xzoom;

//    xspan = xzoom;
//    yspan = yzoom;

//    float _w = sizex*xspan;
//    float _h = sizey*yspan;
//    float tx = xspan;
//    float ty = yspan;

//    mTX = tx;
//    mTY = ty;

//    oy *= -1;

//    if( ox*tx > (sizex/2)*tx )
//        _rx = -(tx*ox-(sizex/2)*tx);
//    else
//        _rx = (sizex/2)*tx-tx*ox;
//    if( oy*ty > (sizey/2)*ty )
//        _ry = -(ty*oy-(sizey/2)*ty);
//    else
//        _ry = (sizey/2)*ty-ty*oy;

//    px = ox*tx+_rx;
//    py = oy*ty+_ry;

//    TArea * pArea = mpMap->areas[mAID];
//    if( ! pArea ) return;

//    int zEbene;
//    zEbene = mOz;

//    if( ! mpMap ) return;
//    if( ! mpMap->rooms.contains( mRID ) ) return;

//    float wegBreite = 1/eSize * tx * rSize;

//    if( ! mpMap->areas.contains(mAID) ) return;

//    TArea * pA = mpMap->areas[mAID];

//    if( pA->rooms.size() < 1 ) return;

//    QPixmap pix = QPixmap(sizex*xspan, sizey*yspan);
//    QPainter p( &pix );
//    if( !p.isActive() )
//    {
//        cout << "ERROR: no active painter"<<endl;
//        return;
//    }
//    pix.fill( mpHost->mBgColor_2 );

//    QPen pen;

//    pen = p.pen();
//    pen.setColor( mpHost->mFgColor_2 );
//    pen.setWidthF(wegBreite);
//    if(mMapperUseAntiAlias)
//        p.setRenderHint(QPainter::Antialiasing);
//    else
//        p.setRenderHint(QPainter::NonCosmeticDefaultPen);
//    p.setPen( pen );

//    if( mpMap->mapLabels.contains( mAID ) )
//    {
//        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAID]);
//        while( it.hasNext() )
//        {
//            it.next();
//            if( it.value().pos.z() != mOz ) continue;
//            if( it.value().text.length() < 1 )
//            {
//                mpMap->mapLabels[mAID][it.key()].text = "no text";
//            }
//            QPointF lpos;
//            int _lx = it.value().pos.x()*tx+_rx;
//            int _ly = it.value().pos.y()*ty*-1+_ry;

//            lpos.setX( _lx );
//            lpos.setY( _ly );
//            int _lw = abs(it.value().size.width())*tx;
//            int _lh = abs(it.value().size.height())*ty;

//            QRectF _drawRect = QRect(it.value().pos.x()*tx+_rx, it.value().pos.y()*ty*-1+_ry, _lw, _lh);
//            if ( !it.value().pix.isNull() )
//            {
//                p.drawPixmap( lpos, it.value().pix.scaled(_drawRect.size().toSize()) );
//            }
//            else
//            {
//                if( it.value().text.length() < 1 )
//                {
//                    mpMap->mapLabels[mAID][it.key()].text = "no text";
//                }
//                QRectF lr = QRectF( 0, 0, 1000, 100 );
//                QPixmap pix( lr.size().toSize() );
//                pix.fill(Qt::transparent);
//                QPainter lp( &pix );

//                if( it.value().hilite )
//                {
//                    lp.fillRect( lr, QColor(255,155,55) );
//                }
//                else
//                {
//                    lp.fillRect( lr, it.value().bgColor );
//                }
//                QPen lpen;
//                lpen.setColor( it.value().fgColor );
//                lp.setPen( lpen );
//                QRectF br;
//                lp.drawText( lr, Qt::AlignLeft, it.value().text, &br );
//                QPointF lpos;
//                lpos.setX( it.value().pos.x()*tx+_rx );
//                lpos.setY( it.value().pos.y()*ty*-1+_ry );
//                p.drawPixmap( lpos, pix, br.toRect() );
//            }
//            if( it.value().hilite )
//            {
//                p.fillRect(_drawRect, QColor(255, 155, 55, 190));
//            }

//        }
//    }

//    if( ! pArea->gridMode )
//    {
//        for( int i=0; i<pArea->rooms.size(); i++ )
//        {
//            TRoom * pR = mpMap->rooms[pArea->rooms[i]];
//            int trID = pArea->rooms[i];
//            float rx = pR->x*tx+_rx;
//            float ry = pR->y*-1*ty+_ry;
//            int rz = pR->z;

//            if( rz != zEbene && !mMultiSelectionList.contains(i)) continue;

//            pR->rendered = true;

//            exitList.clear();
//            oneWayExits.clear();
//            if( pR->customLines.size() > 0 )
//            {
//                if( ! pR->customLines.contains("N") )
//                {
//                    exitList.push_back( pR->north );
//                    if( mpMap->rooms.contains(pR->north) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->north];
//                        if( pER->south != pR->id )
//                        {
//                            oneWayExits.push_back( pR->north );
//                        }
//                    }
//                }
//                if( !pR->customLines.contains("NW") )
//                {
//                    exitList.push_back( pR->northwest );
//                    if( mpMap->rooms.contains(pR->northwest) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->northwest];
//                        if( pER->southeast != pR->id )
//                        {
//                            oneWayExits.push_back( pR->northwest );
//                        }
//                    }
//                }
//                if( !pR->customLines.contains("E") )
//                {
//                    exitList.push_back( pR->east );
//                    if( mpMap->rooms.contains(pR->east) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->east];
//                        if( pER->west != pR->id )
//                        {
//                            oneWayExits.push_back( pR->east );
//                        }
//                    }
//                }
//                if( !pR->customLines.contains("SE") )
//                {
//                    exitList.push_back( pR->southeast );
//                    if( mpMap->rooms.contains(pR->southeast) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->southeast];
//                        if( pER->northwest != pR->id )
//                        {
//                            oneWayExits.push_back( pR->southeast );
//                        }
//                    }
//                }
//                if( !pR->customLines.contains("S") )
//                {
//                    exitList.push_back( pR->south );
//                    if( mpMap->rooms.contains(pR->south) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->south];
//                        if( pER->north != pR->id )
//                        {
//                            oneWayExits.push_back( pR->south );
//                        }
//                    }
//                }
//                if( !pR->customLines.contains("SW") )
//                {
//                    exitList.push_back( pR->southwest );
//                    if( mpMap->rooms.contains(pR->southwest) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->southwest];
//                        if( pER->northeast != pR->id )
//                        {
//                            oneWayExits.push_back( pR->southwest );
//                        }
//                    }
//                }
//                if( !pR->customLines.contains("W") )
//                {
//                    exitList.push_back( pR->west );
//                    if( mpMap->rooms.contains(pR->west) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->west];
//                        if( pER->east != pR->id )
//                        {
//                            oneWayExits.push_back( pR->west );
//                        }
//                    }
//                }
//                if( !pR->customLines.contains("NE") )
//                {
//                    exitList.push_back( pR->northeast );
//                    if( mpMap->rooms.contains(pR->northeast) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->northeast];
//                        if( pER->southwest != pR->id )
//                        {
//                            oneWayExits.push_back( pR->northeast );
//                        }
//                    }
//                }
//            }
//            else
//            {
//                if( pR->north > 0 )
//                {
//                    exitList.push_back( pR->north );
//                    if( mpMap->rooms.contains(pR->north) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->north];
//                        if( pER->south != pR->id )
//                        {
//                            oneWayExits.push_back( pR->north );
//                        }
//                    }
//                }
//                if( pR->northwest > 0 )
//                {
//                    exitList.push_back( pR->northwest );
//                    if( mpMap->rooms.contains(pR->northwest) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->northwest];
//                        if( pER->southeast != pR->id )
//                        {
//                            oneWayExits.push_back( pR->northwest );
//                        }
//                    }
//                }
//                if( pR->east > 0 )
//                {
//                    exitList.push_back( pR->east );
//                    if( mpMap->rooms.contains(pR->east) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->east];
//                        if( pER->west != pR->id )
//                        {
//                            oneWayExits.push_back( pR->east );
//                        }
//                    }
//                }
//                if( pR->southeast > 0 )
//                {
//                    exitList.push_back( pR->southeast );
//                    if( mpMap->rooms.contains(pR->southeast) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->southeast];
//                        if( pER->northwest != pR->id )
//                        {
//                            oneWayExits.push_back( pR->southeast );
//                        }
//                    }
//                }
//                if( pR->south > 0 )
//                {
//                    exitList.push_back( pR->south );
//                    if( mpMap->rooms.contains(pR->south) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->south];
//                        if( pER->north != pR->id )
//                        {
//                            oneWayExits.push_back( pR->south );
//                        }
//                    }
//                }
//                if( pR->southwest > 0 )
//                {
//                    exitList.push_back( pR->southwest );
//                    if( mpMap->rooms.contains(pR->southwest) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->southwest];
//                        if( pER->northeast != pR->id )
//                        {
//                            oneWayExits.push_back( pR->southwest );
//                        }
//                    }
//                }
//                if( pR->west > 0 )
//                {
//                    exitList.push_back( pR->west );
//                    if( mpMap->rooms.contains(pR->west) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->west];
//                        if( pER->east != pR->id )
//                        {
//                            oneWayExits.push_back( pR->west );
//                        }
//                    }
//                }
//                if( pR->northeast > 0 )
//                {
//                    exitList.push_back( pR->northeast );
//                    if( mpMap->rooms.contains(pR->northeast) )
//                    {
//                        TRoom * pER = mpMap->rooms[pR->northeast];
//                        if( pER->southwest != pR->id )
//                        {
//                            oneWayExits.push_back( pR->northeast );
//                        }
//                    }
//                }
//            }

//            if( pR->customLines.size() > 0 )
//            {
//                QPen oldPen = p.pen();
//                QMapIterator<QString, QList<QPointF> > itk(pR->customLines);
//                while( itk.hasNext() )
//                {
//                    itk.next();
//                    QColor _color;
//                    if( pR->id == mCustomLineSelectedRoom && itk.key()== mCustomLineSelectedExit )
//                    {
//                        _color.setRed( 255 );
//                        _color.setGreen( 155 );
//                        _color.setBlue( 55 );
//                    }
//                    else if( pR->customLinesColor[itk.key()].size() == 3 )
//                    {
//                        _color.setRed( mpMap->rooms[pArea->rooms[i]]->customLinesColor[itk.key()][0] );
//                        _color.setGreen( mpMap->rooms[pArea->rooms[i]]->customLinesColor[itk.key()][1] );
//                        _color.setBlue( mpMap->rooms[pArea->rooms[i]]->customLinesColor[itk.key()][2] );
//                    }
//                    else
//                        _color = QColor(Qt::red);
//                    bool _arrow = pR->customLinesArrow[itk.key()];
//                    QString _style = pR->customLinesStyle[itk.key()];
//                    QPointF _cstartP;
//                    float ex = pR->x*tx+_rx;
//                    float ey = pR->y*ty*-1+_ry;
//                    if( itk.key() == "N" )
//                        _cstartP = QPoint(ex,ey-ty/2);
//                    else if( itk.key() == "NW" )
//                        _cstartP = QPoint(ex-tx/2,ey-ty/2);
//                    else if( itk.key() == "NE" )
//                        _cstartP = QPoint(ex+tx/2,ey-ty/2);
//                    else if( itk.key() == "S" )
//                        _cstartP = QPoint(ex,ey+ty/2);
//                    else if( itk.key() == "SW" )
//                        _cstartP = QPoint(ex-tx/2,ey+ty/2);
//                    else if( itk.key() == "SE" )
//                        _cstartP = QPoint(ex+tx/2,ey+ty/2);
//                    else if( itk.key() == "W" )
//                        _cstartP = QPoint(ex-tx/2, ey);
//                    else if( itk.key() == "E" )
//                        _cstartP = QPoint(ex+tx/2, ey);
//                    else
//                        _cstartP = QPointF(ex, ey);
//                    QPointF ursprung = QPointF(ex,ey);
//                    QPen customLinePen = p.pen();
//                    customLinePen.setCosmetic(mMapperUseAntiAlias);
//                    customLinePen.setColor( _color );
//                    customLinePen.setCapStyle( Qt::RoundCap );
//                    customLinePen.setJoinStyle( Qt::RoundJoin );

//                    if( _style == "solid line" )
//                        customLinePen.setStyle( Qt::SolidLine );
//                    else if( _style == "dot line" )
//                        customLinePen.setStyle( Qt::DotLine );
//                    else if( _style == "dash line" )
//                        customLinePen.setStyle( Qt::DashLine );
//                    else
//                        customLinePen.setStyle( Qt::DashDotDotLine );

//                    QList<QPointF> _pL = itk.value();
//                    if( _pL.size() > 0 )
//                    {
//                        p.setPen(customLinePen);
//                        p.drawLine( ursprung, _cstartP );
//                    }
//                    for( int pk=0; pk<_pL.size(); pk++ )
//                    {
//                        QPointF _cendP;
//                        _cendP.setX( _pL[pk].x()*tx+_rx );
//                        _cendP.setY( _pL[pk].y()*ty*-1+_ry );
//                        p.drawLine( _cstartP, _cendP );

//                        if( pR->id == mCustomLineSelectedRoom && itk.key()== mCustomLineSelectedExit )
//                        {
//                            QBrush _brush = p.brush();
//                            p.setBrush(QColor(255,155,55));
//                            p.drawEllipse( _cendP, mTX/4, mTX/4 );
//                            p.setBrush(_brush);
//                        }

//                        if( pk == _pL.size()-1 && _arrow )
//                        {
//                            QLineF l0 = QLineF( _cendP, _cstartP );
//                            l0.setLength(wegBreite*5);
//                            QPointF _p1 = l0.p1();
//                            QPointF _p2 = l0.p2();
//                            QLineF l1 = QLineF( l0 );
//                            qreal w1 = l1.angle()-90.0;
//                            QLineF l2;
//                            l2.setP1(_p2);
//                            l2.setAngle(w1);
//                            l2.setLength(wegBreite*2);
//                            QPointF _p3 = l2.p2();
//                            l2.setAngle( l2.angle()+180.0 );
//                            QPointF _p4 = l2.p2();
//                            QPolygonF _poly;
//                            _poly.append( _p1 );
//                            _poly.append( _p3 );
//                            _poly.append( _p4 );
//                            QBrush brush = p.brush();
//                            brush.setColor( _color );
//                            brush.setStyle( Qt::SolidPattern );
//                            QPen arrowPen = p.pen();
//                            arrowPen.setCosmetic( mMapperUseAntiAlias );
//                            arrowPen.setStyle(Qt::SolidLine);
//                            p.setPen( arrowPen );
//                            p.setBrush( brush );
//                            p.drawPolygon(_poly);
//                        }
//                        _cstartP = _cendP;
//                    }
//                }
//                p.setPen(oldPen);
//            }

//            int e = pR->z;

//            // draw exit stubs
//            QMap<int, QVector3D> unitVectors = mpMap->unitVectors;
//            for( int k=0; k<pR->exitStubs.size(); k++ )
//            {
//                int direction = pR->exitStubs[k];
//                QVector3D uDirection = unitVectors[direction];
//                p.drawLine(rx+rSize*(int)uDirection.x()/2, ry+rSize*(int)uDirection.y(),rx+(int)uDirection.x()*(rSize*3/4*tx), ry+uDirection.y()*(rSize*3/4*ty));
//            }

//            QPen __pen;
//            for( int k=0; k<exitList.size(); k++ )
//            {
//                int rID = exitList[k];
//                if( rID <= 0 ) continue;

//                bool areaExit;

//                TRoom * pE = mpMap->rooms[rID];

//                if( pE->area != mAID )
//                {
//                    areaExit = true;
//                }
//                else
//                    areaExit = false;
//                float ex = pE->x*tx+_rx;
//                float ey = pE->y*ty*-1+_ry;
//                int ez = pE->z;

//                QVector3D p1( ex, ey, ez );
//                QVector3D p2( rx, ry, rz );
//                QLine _line;
//                if( ! areaExit )
//                {
//                    // one way exit or 2 way exit?
//                    if( ! oneWayExits.contains( rID ) )
//                    {
//                        p.drawLine( (int)p1.x(), (int)p1.y(), (int)p2.x(), (int)p2.y() );
//                    }
//                    else
//                    {
//                        // one way exit draw arrow

//                        QLineF l0 = QLineF( p2.x(), p2.y(), p1.x(), p1.y() );
//                        QLineF k0 = l0;
//                        k0.setLength( (l0.length()-wegBreite*5)*0.5 );
//                        qreal dx = k0.dx(); qreal dy = k0.dy();
//                        QPen _tp = p.pen();
//                        QPen _tp2 = _tp;
//                        _tp2.setStyle(Qt::DotLine);
//                        p.setPen(_tp2);
//                        p.drawLine( l0 );
//                        p.setPen(_tp);
//                        l0.setLength(wegBreite*5);
//                        QPointF _p1 = l0.p2();
//                        QPointF _p2 = l0.p1();
//                        QLineF l1 = QLineF( l0 );
//                        qreal w1 = l1.angle()-90.0;
//                        QLineF l2;
//                        l2.setP1(_p2);
//                        l2.setAngle(w1);
//                        l2.setLength(wegBreite*2);
//                        QPointF _p3 = l2.p2();
//                        l2.setAngle( l2.angle()+180.0 );
//                        QPointF _p4 = l2.p2();
//                        QPolygonF _poly;
//                        _poly.append( _p1 );
//                        _poly.append( _p3 );
//                        _poly.append( _p4 );

//                        QBrush brush = p.brush();
//                        brush.setColor( QColor(255,100,100) );
//                        brush.setStyle( Qt::SolidPattern );
//                        QPen arrowPen = p.pen();
//                        arrowPen.setCosmetic( mMapperUseAntiAlias );
//                        arrowPen.setStyle(Qt::SolidLine);
//                        p.setPen( arrowPen );
//                        p.setBrush( brush );
//                        p.drawPolygon(_poly.translated(dx,dy));
//                    }
//                }
//                else
//                {
//                    __pen = p.pen();
//                    QLine _line;
//                    if( pR->south == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine( p2.x(), p2.y()+ty,p2.x(), p2.y() );
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x(), p2.y()+ty/2);
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    else if( pR->north == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine( p2.x(), p2.y()-ty, p2.x(), p2.y() );
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x(), p2.y()-ty/2);
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    else if( pR->west == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine(p2.x()-tx, p2.y(),p2.x(), p2.y() );
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x()-tx/2, p2.y());
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    else if( pR->east == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine(p2.x()+tx, p2.y(),p2.x(), p2.y() );
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x()+tx/2, p2.y());
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    else if( pR->northwest == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine(p2.x()-tx, p2.y()-ty,p2.x(), p2.y() );
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x()-tx/2, p2.y()-ty/2);
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    else if( pR->northeast == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine(p2.x()+tx, p2.y()-ty,p2.x(), p2.y());
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x()+tx/2, p2.y()-ty/2);
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    else if( pR->southeast == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine(p2.x()+tx, p2.y()+ty, p2.x(), p2.y());
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x()+tx/2, p2.y()+ty/2);
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    else if( pR->southwest == rID )
//                    {
//                        pen = p.pen();
//                        pen.setWidthF(wegBreite);
//                        pen.setCosmetic( mMapperUseAntiAlias );
//                        pen.setColor(getColor(exitList[k]));
//                        p.setPen( pen );
//                        _line = QLine(p2.x()-tx, p2.y()+ty, p2.x(), p2.y());
//                        p.drawLine( _line );
//                        QPoint _p = QPoint(p2.x()-tx/2, p2.y()+ty/2);
//                        mAreaExitList[exitList[k]] = _p;
//                    }
//                    QLineF l0 = QLineF( _line );
//                    l0.setLength(wegBreite*5);
//                    QPointF _p1 = l0.p1();
//                    QPointF _p2 = l0.p2();
//                    QLineF l1 = QLineF( l0 );
//                    qreal w1 = l1.angle()-90.0;
//                    QLineF l2;
//                    l2.setP1(_p2);
//                    l2.setAngle(w1);
//                    l2.setLength(wegBreite*2);
//                    QPointF _p3 = l2.p2();
//                    l2.setAngle( l2.angle()+180.0 );
//                    QPointF _p4 = l2.p2();
//                    QPolygonF _poly;
//                    _poly.append( _p1 );
//                    _poly.append( _p3 );
//                    _poly.append( _p4 );
//                    QBrush brush = p.brush();
//                    brush.setColor( getColor(exitList[k]) );
//                    brush.setStyle( Qt::SolidPattern );
//                    QPen arrowPen = p.pen();
//                    arrowPen.setCosmetic( mMapperUseAntiAlias );
//                    p.setPen( arrowPen );
//                    p.setBrush( brush );
//                    p.drawPolygon(_poly);
//                    p.setPen( __pen );
//                }
//                if( pR->doors.size() > 0 )
//                {
//                    int doorStatus = 0;
//                    if( pR->south == rID && pR->doors.contains("s") )
//                    {
//                        doorStatus = pR->doors["s"];
//                    }
//                    else if( pR->north == rID && pR->doors.contains("n") )
//                    {
//                        doorStatus = pR->doors["n"];
//                    }
//                    else if( pR->southwest == rID && pR->doors.contains("sw") )
//                    {
//                        doorStatus = pR->doors["sw"];
//                    }
//                    else if( pR->southeast == rID && pR->doors.contains("se") )
//                    {
//                        doorStatus = pR->doors["se"];
//                    }
//                    else if( pR->northeast == rID && pR->doors.contains("ne") )
//                    {
//                        doorStatus = pR->doors["ne"];
//                    }
//                    else if( pR->northwest == rID && pR->doors.contains("nw") )
//                    {
//                        doorStatus = pR->doors["nw"];
//                    }
//                    else if( pR->west == rID && pR->doors.contains("w") )
//                    {
//                        doorStatus = pR->doors["w"];
//                    }
//                    else if( pR->east == rID && pR->doors.contains("e") )
//                    {
//                        doorStatus = pR->doors["e"];
//                    }
//                    if( doorStatus > 0 )
//                    {
//                        QLineF k0;
//                        QRectF rect;
//                        rect.setWidth(0.25*mTX);
//                        rect.setHeight(0.25*mTY);
//                        if ( areaExit )
//                            k0 = QLineF(_line);
//                        else
//                            k0 = QLineF( p2.x(), p2.y(), p1.x(), p1.y() );
//                        k0.setLength( (k0.length())*0.5 );
//                        rect.moveCenter(k0.p2());
//                        QPen arrowPen = p.pen();
//                        QPen _tp = p.pen();
//                        arrowPen.setCosmetic( mMapperUseAntiAlias );
//                        arrowPen.setStyle(Qt::SolidLine);
//                        if( doorStatus == 1 ) //open door
//                            arrowPen.setColor(QColor(10,155,10));
//                        else if( doorStatus == 2 ) //closed door
//                            arrowPen.setColor(QColor(155,155,10));
//                        else //locked door
//                            arrowPen.setColor(QColor(155,10,10));
//                        QBrush brush;
//                        QBrush oldBrush;
//                        p.setPen( arrowPen );
//                        p.setBrush(brush);
//                        p.drawRect(rect);
//                        p.setBrush(oldBrush);
//                        p.setPen(_tp);
//                    }
//                }
//            }
//        }
//    }
//    // draw group selection box
//    if( mSizeLabel )
//        p.fillRect(mMultiRect,QColor(250,190,0,190));
//    else
//        p.fillRect(mMultiRect,QColor(190,190,190,60));
//    for( int i=0; i<pArea->rooms.size(); i++ )
//    {
//        TRoom * pR = mpMap->rooms[pArea->rooms[i]];
//        float rx = pR->x*tx+_rx;
//        float ry = pR->y*-1*ty+_ry;
//        int rz = pR->z;

//        if( rz != zEbene ) continue;
//        //if( rx < 0 || ry < 0 || rx > _w || ry > _h ) continue;

//        pR->rendered = false;
//        QRectF dr;
//        if( pArea->gridMode )
//        {
//            dr = QRectF(rx-tx/2, ry-ty/2,tx,ty);
//        }
//        else
//        {
//            dr = QRectF(rx-(tx*rSize)/2,ry-(ty*rSize)/2,tx*rSize,ty*rSize);
//        }

//        QColor c;
//        int env = pR->environment;
//        if( mpMap->envColors.contains(env) )
//            env = mpMap->envColors[env];
//        else
//        {
//            if( ! mpMap->customEnvColors.contains(env))
//            {
//                env = 1;
//            }
//        }
//        switch( env )
//        {
//        case 1:
//            c = mpHost->mRed_2;
//            break;

//        case 2:
//            c = mpHost->mGreen_2;
//            break;
//        case 3:
//            c = mpHost->mYellow_2;
//            break;

//        case 4:
//            c = mpHost->mBlue_2;
//            break;

//        case 5:
//            c = mpHost->mMagenta_2;
//            break;
//        case 6:
//            c = mpHost->mCyan_2;
//            break;
//        case 7:
//            c = mpHost->mWhite_2;
//            break;
//        case 8:
//            c = mpHost->mBlack_2;
//            break;

//        case 9:
//            c = mpHost->mLightRed_2;
//            break;

//        case 10:
//            c = mpHost->mLightGreen_2;
//            break;
//        case 11:
//            c = mpHost->mLightYellow_2;
//            break;

//        case 12:
//            c = mpHost->mLightBlue_2;
//            break;

//        case 13:
//            c = mpHost->mLightMagenta_2;
//            break;
//        case 14:
//            c = mpHost->mLightCyan_2;
//            break;
//        case 15:
//            c = mpHost->mLightWhite_2;
//            break;
//        case 16:
//            c = mpHost->mLightBlack_2;
//        default: //user defined room color
//            if( ! mpMap->customEnvColors.contains(env) ) break;
//            c = mpMap->customEnvColors[env];
//        }
//        if( ( mPick || __Pick ) && mPHighlight.x() >= dr.x()-(tx*rSize) && mPHighlight.x() <= dr.x()+(tx*rSize) && mPHighlight.y() >= dr.y()-(ty*rSize) && mPHighlight.y() <= dr.y()+(ty*rSize)
//            || mMultiSelectionList.contains(pArea->rooms[i]) )
//        {
//            p.fillRect(dr,QColor(255,155,55));
//            mPick = false;
//            if( mStartSpeedWalk )
//            {
//                mStartSpeedWalk = false;
//                float _radius = (0.8*tx)/2;
//                QPointF _center = QPointF(rx,ry);
//                QRadialGradient _gradient(_center,_radius);
//                _gradient.setColorAt(0.95, QColor(255,0,0,150));
//                _gradient.setColorAt(0.80, QColor(150,100,100,150));
//                _gradient.setColorAt(0.799,QColor(150,100,100,100));
//                _gradient.setColorAt(0.7, QColor(255,0,0,200));
//                _gradient.setColorAt(0, QColor(255,255,255,255));
//                QPen myPen(Qt::transparent);
//                QPainterPath myPath;
//                p.setBrush(_gradient);
//                p.setPen(myPen);
//                myPath.addEllipse(_center,_radius,_radius);
//                p.drawPath(myPath);


//                mTarget = pArea->rooms[i];
//                if( mpMap->rooms.contains(mTarget) )
//                {
//                    mpMap->mTargetID = mTarget;
//                    if( mpMap->findPath( mpMap->mRoomId, mpMap->mTargetID) )
//                    {
//                       mpMap->mpHost->startSpeedWalk();
//                    }
//                    else
//                    {
//                        QString msg = "Mapper: Cannot find a path to this room using known exits.\n";
//                        mpHost->mpConsole->printSystemMessage(msg);
//                    }
//                }
//            }
//        }
//        else
//        {
//            char _ch = pR->c;
//            if( _ch >= 33 && _ch < 255 )
//            {
//                int _color = ( 265 - 257 ) * 254 + _ch;//(mpMap->rooms[pArea->rooms[i]]->environment - 257 ) * 254 + _ch;

//                if( c.red()+c.green()+c.blue() > 260 )
//                    _color = ( 7 ) * 254 + _ch;
//                else
//                    _color = ( 6 ) * 254 + _ch;

//                p.fillRect( dr, c );
//                if( mPixMap.contains( _color ) )
//                {
//                    QPixmap pix = mPixMap[_color].scaled(dr.width(), dr.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
//                    p.drawPixmap(dr.topLeft(), pix);
//                }
//            }
//            else
//            {
//                if( mBubbleMode )
//                {
//                    float _radius = (rSize*tx)/2;
//                    QPointF _center = QPointF(rx,ry);
//                    QRadialGradient _gradient(_center,_radius);
//                    _gradient.setColorAt(0.85, c);
//                    _gradient.setColorAt(0, QColor(255,255,255,255));
//                    QPen myPen(Qt::transparent);
//                    QPainterPath myPath;
//                    p.setBrush(_gradient);
//                    p.setPen(myPen);
//                    myPath.addEllipse(_center,_radius,_radius);
//                    p.drawPath(myPath);
//                }
//                else
//                    p.fillRect(dr,c);
//            }
//            if( pR->highlight )
//            {
//                float _radius = (mpMap->rooms[pArea->rooms[i]]->highlightRadius*tx)/2;
//                QPointF _center = QPointF(rx,ry);
//                QRadialGradient _gradient(_center,_radius);
//                _gradient.setColorAt(0.85, mpMap->rooms[pArea->rooms[i]]->highlightColor);
//                _gradient.setColorAt(0, mpMap->rooms[pArea->rooms[i]]->highlightColor2 );
//                QPen myPen(Qt::transparent);
//                QPainterPath myPath;
//                p.setBrush(_gradient);
//                p.setPen(myPen);
//                myPath.addEllipse(_center,_radius,_radius);
//                p.drawPath(myPath);
//            }
//            if( mShowRoomID )
//            {
//                QPen __pen = p.pen();
//                QColor lc;
//                if( c.red()+c.green()+c.blue() > 200 )
//                    lc=QColor(Qt::black);
//                else
//                    lc=QColor(Qt::white);
//                p.setPen(QPen(lc));
//                p.drawText(dr, Qt::AlignHCenter|Qt::AlignVCenter,QString::number(pArea->rooms[i]));
//                p.setPen(__pen);
//            }
//            if( mShiftMode && pArea->rooms[i] == mpMap->mRoomId )
//            {
//                float _radius = (1.2*tx)/2;
//                QPointF _center = QPointF(rx,ry);
//                QRadialGradient _gradient(_center,_radius);
//                _gradient.setColorAt(0.95, QColor(255,0,0,150));
//                _gradient.setColorAt(0.80, QColor(150,100,100,150));
//                _gradient.setColorAt(0.799,QColor(150,100,100,100));
//                _gradient.setColorAt(0.7, QColor(255,0,0,200));
//                _gradient.setColorAt(0, QColor(255,255,255,255));
//                QPen myPen(Qt::transparent);
//                QPainterPath myPath;
//                p.setBrush(_gradient);
//                p.setPen(myPen);
//                myPath.addEllipse(_center,_radius,_radius);
//                p.drawPath(myPath);

//            }
//        }

//        QColor lc;
//        if( c.red()+c.green()+c.blue() > 200 )
//            lc=QColor(Qt::black);
//        else
//            lc=QColor(Qt::white);
//        pen = p.pen();
//        pen.setColor( lc );
//        pen.setWidthF(0);//wegBreite?);
//        pen.setCosmetic( mMapperUseAntiAlias );
//        pen.setCapStyle( Qt::RoundCap );
//        pen.setJoinStyle( Qt::RoundJoin );
//        p.setPen( pen );

//        //FIXME: redo exit stubs here since the room will draw over up/down stubs -- its repetitive though
//        QMap<int, QVector3D> unitVectors = mpMap->unitVectors;
//        for( int k=0; k<pR->exitStubs.size(); k++ )
//        {
//            int direction = pR->exitStubs[k];
//            QVector3D uDirection = unitVectors[direction];
//            if (direction > 8)
//            {
//                float rx = pR->x*tx+_rx;
//                float ry = pR->y*-1*ty+_ry;
//                QPolygonF _poly;
//                QPointF _pt;
//                _pt = QPointF( rx, ry+(ty*rSize)*uDirection.z()/20 );
//                _poly.append( _pt );
//                _pt = QPointF( rx+(tx*rSize)/3.1, ry+(ty*rSize)*uDirection.z()/3.1);
//                _poly.append(_pt);
//                _pt = QPointF( rx-(tx*rSize)/3.1, ry+(ty*rSize)*uDirection.z()/3.1 );
//                _poly.append( _pt );
//                QBrush brush = p.brush();
//                brush.setColor( QColor(Qt::black) );
//                brush.setStyle( Qt::NoBrush );
//                p.setBrush( brush );
//                p.drawPolygon(_poly);
//           }
//        }

//        if( pR->up > 0 )
//        {
//            QPolygonF _poly;
//            QPointF _pt;
//            _pt = QPointF( rx, ry+(ty*rSize)/20 );
//            _poly.append( _pt );
//            _pt = QPointF( rx-(tx*rSize)/3.1, ry+(ty*rSize)/3.1 );
//            _poly.append( _pt );
//            _pt = QPointF( rx+(tx*rSize)/3.1, ry+(ty*rSize)/3.1);
//            _poly.append(_pt);
//            QBrush brush = p.brush();
//            brush.setColor( QColor(Qt::black) );
//            brush.setStyle( Qt::SolidPattern );
//            p.setBrush( brush );
//            p.drawPolygon(_poly);
//        }
//        if( pR->down > 0 )
//        {
//            QPolygonF _poly;
//            QPointF _pt;
//            _pt = QPointF( rx, ry-(ty*rSize)/20 );
//            _poly.append( _pt );
//            _pt = QPointF( rx-(tx*rSize)/3.1, ry-(ty*rSize)/3.1 );
//            _poly.append( _pt );
//            _pt = QPointF( rx+(tx*rSize)/3.1, ry-(ty*rSize)/3.1);
//            _poly.append(_pt);
//            QBrush brush = p.brush();
//            brush.setColor( QColor(Qt::black) );
//            brush.setStyle( Qt::SolidPattern );
//            p.setBrush( brush );
//            p.drawPolygon(_poly);
//        }
//        if( pR->in > 0 )
//        {
//            QPolygonF _poly;
//            QPointF _pt;
//            _pt = QPointF( rx+(tx*rSize)/20, ry );
//            _poly.append( _pt );
//            _pt = QPointF( rx-(tx*rSize)/3.1, ry-(ty*rSize)/3.1 );
//            _poly.append( _pt );
//            _pt = QPointF( rx-(tx*rSize)/3.1, ry+(ty*rSize)/3.1);
//            _poly.append(_pt);
//            QBrush brush = p.brush();
//            brush.setColor( QColor(Qt::black) );
//            brush.setStyle( Qt::SolidPattern );
//            p.setBrush( brush );
//            p.drawPolygon(_poly);
//        }
//        if( pR->out > 0 )
//        {
//            QPolygonF _poly;
//            QPointF _pt;
//            _pt = QPointF( rx-(tx*rSize)/20, ry);
//            _poly.append( _pt );
//            _pt = QPointF( rx+(tx*rSize)/3.1, ry-(ty*rSize)/3.1 );
//            _poly.append( _pt );
//            _pt = QPointF( rx+(tx*rSize)/3.1, ry+(ty*rSize)/3.1);
//            _poly.append(_pt);
//            QBrush brush = p.brush();
//            brush.setColor( QColor(Qt::black) );
//            brush.setStyle( Qt::SolidPattern );
//            p.setBrush( brush );
//            p.drawPolygon(_poly);
//        }
//    }
//    QString _zeit = QTime().currentTime().toString("hh-mm-ss");
//    QString home = QDir::homePath();
//    home.append( "/.config/mudlet/profiles/" );
//    QString name = mpHost->getName();
//    home.append( name );

//    QString name2 = "/mapOfAreaID_"+QString::number(mAID)+"_"+_zeit+".PNG";
//    home.append( name2 );
//    QString erg = QDir::toNativeSeparators( home );
//    pix.save(erg);

}

void T2DMap::resizeMultiSelectionWidget()
{
    int _newWidth;
    if( mIsSelectionUsingNames )
    {
        if( width() <= 300 )
        { // 0 - 300 => 0 - 200
            _newWidth = 2 * width()/3;
        }
        else if( width() <= 600 )
        { // 300 - 600 => 200 - 300
            _newWidth = 100 + width()/3;
        }
        else
        { // 600+ => 300
            _newWidth = 300;
        }
    }
    else
    {
        if( width() <= 300 )
        { // 0 - 300 => 0 - 120
            _newWidth = 2 * width()/3;
        }
        else
        { // 300+ => 120
            _newWidth = 120;
        }
    }
    int _newHeight = 300;
    if( mMultiSelectionListWidget.topLevelItemCount() > 0 )
    {
        QTreeWidgetItem * rowItem = mMultiSelectionListWidget.topLevelItem(1);
        // The following factors are tweaks to ensure that the widget shows all
        // the rows, as the header seems biggger than the value returned, statics
        // used to enable values to be change by debugger at runtime!
        static float headerFactor = 1.2;
        static float rowFactor = 1.0;
        _newHeight = headerFactor * mMultiSelectionListWidget.header()->height();
        if( rowItem )
        { // Have some data rows - and we have forced them to be the same height:
            _newHeight += rowFactor * mMultiSelectionListWidget.topLevelItemCount() * mMultiSelectionListWidget.visualItemRect(rowItem).height();
        }
//        qDebug() << "Row count:" << mMultiSelectionListWidget.topLevelItemCount() << "header height:" << mMultiSelectionListWidget.header()->height() << "row height:" << mMultiSelectionListWidget.visualItemRect(rowItem).height();
    }
    if( _newHeight < height() )
    {
        mMultiSelectionListWidget.resize( _newWidth, _newHeight );
    }
    else
    {
        mMultiSelectionListWidget.resize( _newWidth, height() );
    }
}
