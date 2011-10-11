/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn                                *
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
#include "TEasyButtonBar.h"
#include <QMenu>

class TToolBar;
class TEasyButtonBar;
class TAction;
class Host;

class ActionUnit
{
    friend class XMLexport;
    friend class XMLimport;

public:
                          ActionUnit( Host * pHost ) : mpHost(pHost), mMaxID(0) {;}
    std::list<TAction *>  getActionRootNodeList()   { QMutexLocker locker(& mActionUnitLock); return mActionRootNodeList; }
    TAction *             getAction( int id );
    TAction *            findAction( QString & );
    void                  compileAll();
    bool                  registerAction( TAction * pT );
    void                  unregisterAction( TAction * pT );
    bool                  serialize( QDataStream & );
    bool                  restore( QDataStream &, bool );
    void                  reParentAction( int childID, int oldParentID, int newParentID, int parentPostion = -1, int childPosition = -1 );
    qint64                getNewID();
    void                  uninstall( QString );
    void                  _uninstall( TAction * pChild, QString packageName );
    void                  updateToolbar();
    std::list<TToolBar *> getToolBarList();
    std::list<TEasyButtonBar *> getEasyButtonBarList();
    TAction *             getHeadAction( TToolBar * );
    TAction *             getHeadAction( TEasyButtonBar * );
    void                  processDataStream( QString & );
    void                  constructToolbar( TAction *, mudlet * pMainWindow, TToolBar * pTB );
    void                  constructToolbar( TAction *, mudlet * pMainWindow, TEasyButtonBar * pTB );
    void                  showToolBar( QString & );
    void                  hideToolBar( QString & );

    QMutex                mActionUnitLock;
    QList<TAction*>       uninstallList;

private:
                          ActionUnit(){;}
    TAction *             getActionPrivate( int id );
    void                  addActionRootNode( TAction * pT, int parentPosition = -1, int childPosition = -1 );
    void                  addAction( TAction * pT );
    void                  removeActionRootNode( TAction * pT );
    void                  removeAction( TAction *);
    Host *                mpHost;
    QMap<int, TAction *>  mActionMap;
    std::list<TAction *>  mActionRootNodeList;
    qint64                mMaxID;
    TToolBar *            mpToolBar;
    TEasyButtonBar *      mpEasyButtonBar;
    bool                  mModuleMember;
    std::list<TToolBar *> mToolBarList;
    std::list<TEasyButtonBar *> mEasyButtonBarList;

};


#endif
