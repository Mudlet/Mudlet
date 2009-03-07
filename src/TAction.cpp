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
#include <string>
#include <vector>
#include <math.h>
#include <QDataStream>
#include <QRegExp>
#include <QString>
#include <QTextDocument>
#include <QIcon>

#include <QObject>
#include "TAction.h"
#include "Host.h"
#include "HostManager.h"
#include "EAction.h"
#include "TFlipButton.h"
#include "TToolBar.h"

using namespace std;

TAction::TAction( TAction * parent, Host * pHost ) 
: Tree<TAction>( parent )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mpToolBar( 0 )
, mButtonColumns( 0 )
{
} 

TAction::TAction( QString name, Host * pHost ) 
: Tree<TAction>(0)
, mName( name )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mpToolBar( 0 )
, mButtonColumns( 0 )
{
}

TAction::~TAction()
{
    if( mpParent == 0 )
    {
        if( ! mpHost )
        {
            qDebug() << "ERROR: TAction::**UN**registerTrigger() pHost=0";
            return;
        }
        mpHost->getActionUnit()->unregisterAction(this);     
    }
    
}

bool TAction::match( QString & toMatch )
{
}


bool TAction::registerAction()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TAction::registerTrigger() pHost=0";
        return false;
    }
    qDebug()<<"calling ActionUnit->registerAlias(this) ...";
    return mpHost->getActionUnit()->registerAction( this );    
}

void TAction::compile()
{
}

void TAction::execute(QStringList & list)
{
    if( ( mCommandButtonUp.size() > 0 ) && ( mButtonState == 1 ) )
    {
        mpHost->send( mCommandButtonUp );
    }
    if( ( mCommandButtonDown.size() > 0 ) && ( mButtonState == 2 ) )
    {
        mpHost->send( mCommandButtonDown );
    }
    if( mNeedsToBeCompiled )
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("function Action") + QString::number( mID ) + QString("()\n"); 
        QString code = funcName + mScript + QString("\nend\n");
        if( pL->compile( code ) )
        {
            mNeedsToBeCompiled = false;
        }
        funcName = QString("Action") + QString::number( mID ); 
        pL->call( funcName, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("Action") + QString::number( mID ); 
        pL->call( funcName, mName );
    }
    // move focus back to the active console / command line
    mpHost->mpConsole->activateWindow();
    mpHost->mpConsole->setFocus();
}

void TAction::expandToolbar( mudlet * pMainWindow, TToolBar * pT, QMenu * menu )
{
   typedef list<TAction *>::const_iterator I;
   for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
   {
       TAction * pChild = *it;
       
       QIcon icon( pChild->mIcon );
       QString name = pChild->getName();
       TFlipButton * button = new TFlipButton( pT,pChild, pChild->mID, mpHost );
       
       //QSizePolicy sizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum);
       //button->setSizePolicy( sizePolicy );
       button->setIcon( icon );
       button->setText( name );
       button->setCheckable( pChild->mIsPushDownButton );
      
       pT->addButton( button );
       QPalette palette;
       palette.setColor( QPalette::Button, button->mpTAction->mButtonColor );
       button->setPalette( palette );
       
       if( pChild->mIsFolder )
       {
           QMenu * newMenu = new QMenu( pT );
           button->setMenu( newMenu );
           pChild->insertActions( pMainWindow, pT, newMenu );
       }
   }
}


void TAction::insertActions( mudlet * pMainWindow, TToolBar * pT, QMenu * menu )
{
    QMutexLocker locker(& mLock);
    mpToolBar = pT;
    
    QIcon icon( mIcon );
    EAction * action = new EAction( icon, mName, pMainWindow );
    action->setCheckable( mIsPushDownButton );
    action->mID = mID;
    action->mpHost = mpHost;
    action->setStatusTip( mName );
    menu->addAction( action );
    mudlet::self()->bindMenu( menu, action );
    
    if( mIsFolder )
    {
        QMenu * newMenu = new QMenu( pMainWindow );
        action->setMenu( newMenu );
        /*QWidget * pButton = pT->widgetForAction( action );
        if( pButton )
        {
            ((QToolButton*)pButton)->setPopupMode( QToolButton::InstantPopup );
        } */
        
        typedef list<TAction *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TAction * pChild = *it;
            pChild->insertActions( pMainWindow, pT, newMenu );
        }
    }
}
                                   
                                   

bool TAction::serialize( QDataStream & ofs )
{
    QMutexLocker locker(& mLock);
    ofs << mName;
    ofs << mScript;
    qDebug()<<"serializing:"<< mName;
    ofs << mID;
    ofs << mIsActive;
    ofs << mIsPushDownButton;
    ofs << mIsFolder;
    ofs << mIcon;
    ofs << mCommandButtonUp;
    ofs << mCommandButtonDown;
    ofs << (qint64)mpMyChildrenList->size();
    bool ret = true;
    typedef list<TAction *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAction * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
} 

bool TAction::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mName;
    ifs >> mScript;
    ifs >> mID;
    ifs >> mIsActive;
    ifs >> mIsPushDownButton;
    ifs >> mIsFolder;
    ifs >> mIcon;
    ifs >> mCommandButtonUp;
    ifs >> mCommandButtonDown;
    qint64 children;
    ifs >> children;
    
    mID = mpHost->getActionUnit()->getNewID();
    
    bool ret = false;
    
    if( ifs.status() == QDataStream::Ok )
        ret = true;
    
    for( qint64 i=0; i<children; i++ )
    {
        TAction * pChild = new TAction( this, mpHost );
        ret = pChild->restore( ifs, initMode );
        if( initMode )
            pChild->registerAction();
    }

    if (getChildrenList()->size() > 0)
        mIsFolder = true;
    return ret;
}

TAction& TAction::clone(const TAction& b)
{
    mName = b.mName;
    mCommandButtonUp = b.mCommandButtonUp;
    mCommandButtonDown = b.mCommandButtonDown;
    mRegex = b.mRegex;
    mScript = b.mScript;
    mIsPushDownButton = b.mIsPushDownButton;
    mIsActive = b.mIsActive;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    mIcon = b.mIcon;
    mpToolBar = b.mpToolBar;
    return *this;
}

bool TAction::isClone( TAction & b ) const 
{
    return ( mName == b.mName 
             && mCommandButtonUp == b.mCommandButtonUp 
             && mCommandButtonDown == b.mCommandButtonDown 
             && mRegex == b.mRegex 
             && mScript == b.mScript 
             && mIsPushDownButton == b.mIsPushDownButton 
             && mIsActive == b.mIsActive 
             && mIsFolder == b.mIsFolder 
             && mpHost == b.mpHost 
             && mNeedsToBeCompiled == b.mNeedsToBeCompiled 
             && mpToolBar == b.mpToolBar
             && mIcon == b.mIcon );
}

