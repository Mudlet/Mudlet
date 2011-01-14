
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

#ifndef T2DMAP_H
#define T2DMAP_H

#include <QWidget>
#include <TMap.h>

class T2DMap : public QWidget
{
    Q_OBJECT

public:

    T2DMap();
    explicit T2DMap( QWidget *parent = 0);
    void     paintEvent( QPaintEvent * );
    void     mousePressEvent(QMouseEvent * );
    void     wheelEvent ( QWheelEvent * );
    void     mouseMoveEvent( QMouseEvent * event );
    TMap *   mpMap;
    Host *   mpHost;
    int      xzoom;
    int      yzoom;
    int      _rx;
    int      _ry;
    QPoint   mPHighlight;
    bool     mPick;
    int      mTarget;
    int      mRoomSelection;
    bool     mStartSpeedWalk;
    QMap<int, QPoint> mAreaExitList;
    QPoint   mMoveTarget;
    bool     mRoomBeingMoved;
    QPoint   mPHighlightMove;
    float    mTX;
    float    mTY;

signals:

public slots:

    void slot_moveRoom();
    void slot_deleteRoom();
    void slot_changeColor();
    void slot_addSpecialExit();
    void slot_setExits();
    void slot_setUserData();
    void slot_lockRoom();
    void slot_setRoomWeight();
};

#endif // T2DMAP_H
