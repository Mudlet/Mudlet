#ifndef MUDLET_TSPLITTER_H
#define MUDLET_TSPLITTER_H

/***************************************************************************
 *   Copyright (C) 2009 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
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


#include <QSplitter>
#include "TSplitterHandle.h"

class TSplitterHandle;

class TSplitter : public QSplitter
{
Q_OBJECT
 public:
     TSplitter( Qt::Orientation orientation, QWidget *parent = 0 );

 protected:
     QSplitterHandle * createHandle();

     TSplitterHandle * mpSplitterHandle;
};

#endif // MUDLET_TSPLITTER_H
