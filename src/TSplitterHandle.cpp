/***************************************************************************
 *   Copyright (C) 2009 by Heiko Koehn - KoehnHeiko@googlemail.com         *
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


#include "TSplitterHandle.h"

#include "TSplitter.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QPainter>
#include "post_guard.h"


TSplitterHandle::TSplitterHandle( Qt::Orientation orientation, TSplitter * parent )
: QSplitterHandle( orientation, (QSplitter*)parent )
{
}

void TSplitterHandle::paintEvent( QPaintEvent * event )
{
    QPainter painter( this );
    QLinearGradient gradient(QPointF(100, 100), QPointF(200, 200));
    gradient.setColorAt(0, Qt::black);
    gradient.setColorAt(1, Qt::white);
    if(orientation() == Qt::Horizontal)
    {
        gradient.setStart(rect().left(), rect().height()/2);
        gradient.setFinalStop(rect().right(), rect().height()/2);
    }
    else
    {
        gradient.setStart(rect().width()/2, rect().top());
        gradient.setFinalStop(rect().width()/2, rect().bottom());
    }
    painter.fillRect(event->rect(), QBrush(gradient));
}
