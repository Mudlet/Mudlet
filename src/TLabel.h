#ifndef MUDLET_TLABEL_H
#define MUDLET_TLABEL_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "pre_guard.h"
#include <QLabel>
#include <QString>
#include "TEvent.h"
#include "post_guard.h"

class Host;
class TEvent;

class QMouseEvent;


class TLabel : public QLabel
{
Q_OBJECT

public:

                  TLabel( QWidget * pW = Q_NULLPTR );

void              setScript( Host * pHost, QString & func, TEvent * args )
                  {
                      mpHost = pHost;
                      mScript = func;
                      mParameters.mArgumentList = args->mArgumentList;
                      mParameters.mArgumentTypeList = args->mArgumentTypeList;
                      mParameters.mArgumentList.detach();
                      mParameters.mArgumentTypeList.detach();
                      // Must take a local copy of these as the caller's TEvent
                      // that args point to is likely an automatic
                      // (Stack allocated) instance that will not persist as
                      // long as this label...
                  }

void              setEnter( Host * pHost, QString & func, TEvent * args )
                  {   mpHost = pHost;
                      mEnter = func;
                      mEnterParams.mArgumentList = args->mArgumentList;
                      mEnterParams.mArgumentTypeList = args->mArgumentTypeList;
                      mEnterParams.mArgumentList.detach();
                      mEnterParams.mArgumentTypeList.detach();
                  }

void              setLeave( Host * pHost, QString & func, TEvent * args )
                  {   mpHost = pHost;
                      mLeave = func;
                      mLeaveParams.mArgumentList = args->mArgumentList;
                      mLeaveParams.mArgumentTypeList = args->mArgumentTypeList;
                      mLeaveParams.mArgumentList.detach();
                      mLeaveParams.mArgumentTypeList.detach();
                  }

void              mousePressEvent( QMouseEvent *  );
void              leaveEvent(QEvent *);
void              enterEvent(QEvent *);

Host *            mpHost;
QString           mScript;
QString           mEnter;
QString           mLeave;
TEvent            mParameters;
TEvent            mLeaveParams;
TEvent            mEnterParams;
bool              mouseInside;
};

#endif // MUDLET_TLABEL_H
