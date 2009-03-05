/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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

#ifndef _ACTION_UNIT_H
#define _ACTION_UNIT_H

#include "TAction.h"
#include <list>
#include <map>
#include <QMutex>
#include <QDataStream>
#include "TToolBar.h"
#include <QMenu>

class TToolBar;
class TAction;
class Host;

using namespace std;

class ActionUnit
{
    friend class XMLexport;
    friend class XMLimport;
    
public:
                          ActionUnit( Host * pHost ) : mpHost(pHost), mMaxID(0) {;}
    list<TAction *>       getActionRootNodeList()   { QMutexLocker locker(& mActionUnitLock); return mActionRootNodeList; }
    TAction *             getAction( int id );
    bool                  registerAction( TAction * pT );
    void                  unregisterAction( TAction * pT );
    bool                  serialize( QDataStream & );
    bool                  restore( QDataStream &, bool );
    void                  reParentAction( int childID, int oldParentID, int newParentID );
    qint64                getNewID();
    void                  updateToolbar();
    std::list<TToolBar *> getToolBarList();
    TAction *             getHeadAction( TToolBar * );
    void                  processDataStream( QString & );
    void                  constructToolbar( TAction *, mudlet * pMainWindow, TToolBar * pTB );
    QMutex                mActionUnitLock;
    
private: 
                          ActionUnit(){;}
    TAction *             getActionPrivate( int id );
    void                  addActionRootNode( TAction * pT );
    void                  addAction( TAction * pT );
    void                  removeActionRootNode( TAction * pT );
    void                  removeAction( TAction *);
    Host *                mpHost;
    QMap<int, TAction *>  mActionMap;
    list<TAction *>       mActionRootNodeList;
    qint64                mMaxID;
    TToolBar *            mpToolBar;
    std::list<TToolBar *> mToolBarList;
    
};


#endif
