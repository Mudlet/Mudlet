
#ifndef _TACTION_H_
#define _TACTION_H_

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



#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <QMutex>
#include <QTimer>
#include <QString>
#include <QRegExp>
#include "Tree.h"
#include <QDataStream>
#include "Host.h"
#include <QTextBlock>
#include "TToolBar.h"
#include "TFlipButton.h"
#include <QMenu>

using namespace std;



class TAction : public Tree<TAction>, QObject
{
    friend class XMLexport;
    friend class XMLimport;
    
public:
    
    
    virtual          ~TAction();
                     TAction( TAction * parent, Host * pHost ); 
                     TAction( QString name, Host * pHost ); 
                     TAction & clone(const TAction & );
    
    QString          getName()                       { QMutexLocker locker(& mLock); return mName; }
    void             setName( QString name )         { QMutexLocker locker(& mLock); mName = name; }
    void             compile();
    void             execute(QStringList &);
    QString          getIcon()                     { QMutexLocker locker(& mLock); return mIcon; }
    void             setIcon( QString & icon )   { QMutexLocker locker(& mLock); mIcon = icon; }
    QString          getScript()                     { QMutexLocker locker(& mLock); return mScript; }
    void             setScript( QString & script )   { QMutexLocker locker(& mLock); mScript = script; mNeedsToBeCompiled=true; }
    QString          getCommandButtonUp()                  { QMutexLocker locker(& mLock); return mCommandButtonUp; }
    void             setCommandButtonUp( QString cmd )   { QMutexLocker locker(& mLock); mCommandButtonUp = cmd; }
    void             setCommandButtonDown( QString command )   { QMutexLocker locker(& mLock); mCommandButtonDown = command; }
    QString          getCommandButtonDown()                    { QMutexLocker locker(& mLock); return mCommandButtonDown; }
    bool             isActive()                      { QMutexLocker locker(& mLock); return mIsActive; }  
    bool             isFolder()                      { QMutexLocker locker(& mLock); return mIsFolder; }
    bool             isPushDownButton()              { QMutexLocker locker(& mLock); return mIsPushDownButton; }
    void             setIsPushDownButton( bool b )   { QMutexLocker locker(& mLock); mIsPushDownButton = b; }
    void             setIsActive( bool b )           { QMutexLocker locker(& mLock); mIsActive = b; }
    void             setIsFolder( bool b )           { QMutexLocker locker(& mLock); mIsFolder = b; }
    bool             match( QString & toMatch );
    bool             registerAction();
    bool             serialize( QDataStream & );
    bool             restore( QDataStream & fs, bool );
    void             insertActions( mudlet * pMainWindow, TToolBar * pT, QMenu * menu );
    void             expandToolbar( mudlet * pMainWindow, TToolBar * pT, QMenu * menu );
    bool             isClone(TAction & ) const;
    TToolBar *       mpToolBar;
    int              mButtonState;
    
    int              mPosX;
    int              mPosY;
    int              mOrientation;
    int              mLocation;
    QString          mName;
    QString          mCommandButtonUp;
    QString          mCommandButtonDown;
    QRegExp          mRegex;
    QString          mScript;
    bool             mIsPushDownButton;
    bool             mIsActive;
    bool             mIsFolder;
    Host *           mpHost;
    bool             mNeedsToBeCompiled;
    QString          mIcon;
    QIcon            mIconPix;
    
private:
    
    TAction(){};
    QMutex           mLock;
};

#endif

