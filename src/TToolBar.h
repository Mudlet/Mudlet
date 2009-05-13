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

#ifndef TTOOLBAR_H
#define TTOOLBAR_H

#include <QDockWidget>
#include <QDebug>
#include <QGridLayout>
#include "TFlipButton.h"
#include "TAction.h"

class Host;
class TFlipButton;
class TAction;

class TToolBar : public QDockWidget
{
Q_OBJECT
        
public:
                     TToolBar( TAction *, QString, QWidget * pW = 0 );
    void             addButton( TFlipButton * pW );
    void             moveEvent( QMoveEvent * e );
    void             setVerticalOrientation(){ mVerticalOrientation = true; }
    void             setHorizontalOrientation(){ mVerticalOrientation = false; }
    void             clear();
    void             finalize();
    TAction *        mpTAction;
    void             recordMove(){ mRecordMove = true; }
    
//private:
        
    bool             mVerticalOrientation;
    QWidget *        mpWidget;
    QString          mName;
    bool             mRecordMove;
    QGridLayout *    mpLayout;
    int              mItemCount;
    
signals:
    
    
public slots:
    
    void slot_pressed();    
    
};

#endif

