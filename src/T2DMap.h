
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
    TMap *   mpMap;
    Host *   mpHost;
    int      xspan;
    int      yspan;
    int      _rx;
    int      _ry;
    QPoint   mPHighlight;
    bool     mPick;
    int      mTarget;
    QMap<int, QPoint> mAreaExitList;

signals:

public slots:

};

#endif // T2DMAP_H
