#ifndef TLABEL_H
#define TLABEL_H


/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

#include "mudlet.h"
#include "Host.h"
#include <QLabel>
#include "TEvent.h"

class TEvent;

class TLabel : public QLabel
{
Q_OBJECT

public:

                  TLabel( QWidget * pW=0 );
void              setScript( Host * pHost, QString & func, TEvent * args ){ mpHost = pHost; mScript = func; mpParameters = args; }
void              setEnter( Host * pHost, QString & func, TEvent * args ){ mpHost = pHost; mEnter = func; mEnterParams = args; }
void              setLeave( Host * pHost, QString & func, TEvent * args ){ mpHost = pHost; mLeave = func; mLeaveParams = args; }
void              mousePressEvent( QMouseEvent *  );
void              leaveEvent(QEvent *);
void              enterEvent(QEvent *);

Host *            mpHost;
QString           mScript;
QString           mEnter;
QString           mLeave;
TEvent *          mpParameters;
TEvent *          mLeaveParams;
TEvent *          mEnterParams;
bool              mouseInside;
public slots:

signals:


};



#endif // TLABEL_H
