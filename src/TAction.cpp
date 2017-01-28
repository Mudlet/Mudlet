/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "TAction.h"


#include "Host.h"
#include "EAction.h"
#include "mudlet.h"
#include "TDebug.h"
#include "TConsole.h"
#include "TEasyButtonBar.h"
#include "TFlipButton.h"
#include "TToolBar.h"


using namespace std;

TAction::TAction( TAction * parent, Host * pHost )
: Tree<TAction>( parent )
, mpToolBar( 0 )
, mpEasyButtonBar( 0 )
, mButtonState( 1 )
, mPosX( 0 )
, mPosY( 0 )
, mNeedsToBeCompiled( true )
, mButtonColumns( 1 )
, mIsLabel( false )
, mUseCustomLayout( false )
, mButtonColor( QColor( 255,255,255) )
, mpHost( pHost )
, exportItem(true)
, mModuleMasterFolder(false)
, mModuleMember(false)
{
}

TAction::TAction(const QString& name, Host * pHost )
: Tree<TAction>(0)
, mpToolBar( 0 )
, mpEasyButtonBar( 0 )
, mButtonState( 1 )
, mPosX( 0 )
, mPosY( 0 )
, mName( name )
, mNeedsToBeCompiled( true )
, mButtonColumns( 1 )
, mIsLabel( false )
, mUseCustomLayout( false )
, mButtonColor( QColor( 255,255,255) )
, mpHost( pHost )
, exportItem(true)
, mModuleMasterFolder(false)
, mModuleMember(false)
{
}

TAction::~TAction()
{
    if( ! mpHost )
    {
        return;
    }
    mpHost->getActionUnit()->unregisterAction(this);
}


bool TAction::registerAction()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TAction::registerTrigger() pHost=0";
        return false;
    }
    return mpHost->getActionUnit()->registerAction( this );
}

void TAction::compileAll()
{
    mNeedsToBeCompiled = true;
    if( ! compileScript() )
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of action:"<<mName<<"\n">>0;}
        mOK_code = false;
    }
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAction * pChild = *it;
        pChild->compileAll();
    }
}

void TAction::compile()
{
    if( mNeedsToBeCompiled )
    {
        if( ! compileScript() )
        {
            if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: Lua compile error. compiling script of action:"<<mName<<"\n">>0;}
            mOK_code = false;
        }
    }
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAction * pChild = *it;
        pChild->compile();
    }
}

bool TAction::setScript(const QString & script )
{
    mScript = script;
    mNeedsToBeCompiled = true;
    mOK_code = compileScript();
    return mOK_code;
}

bool TAction::compileScript()
{
    mFuncName = QString("Action")+QString::number( mID );
    QString code = QString("function ")+ mFuncName + QString("()\n") + mScript + QString("\nend\n");
    QString error;
    if( mpHost->mLuaInterpreter.compile( code, error, QString("Button: ") + getName() ) )
    {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        return true;
    }
    else
    {
        mOK_code = false;
        setError( error );
        return false;
    }
}

void TAction::execute(QStringList & list )
{
    qDebug()<<"TAction::execute() called: depricated!";
}

void TAction::_execute(QStringList & list)
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
        if( ! compileScript() )
        {
            mpHost->mpConsole->activateWindow();
            mpHost->mpConsole->setFocus();
            return;
        }
    }
    mpHost->mpConsole->mButtonState = mButtonState;
    mpHost->mLuaInterpreter.call( mFuncName, mName );
    // move focus back to the active console / command line
    mpHost->mpConsole->activateWindow();
    mpHost->mpConsole->setFocus();
}

void TAction::expandToolbar( mudlet * pMainWindow, TToolBar * pT, QMenu * menu )
{
   for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
   {
       TAction * pChild = *it;

       QIcon icon( pChild->mIcon );
       QString name = pChild->getName();
       TFlipButton * button = new TFlipButton( pT,pChild, pChild->mID, mpHost );
       button->setIcon( icon );
       button->setText( name );
       button->setCheckable( pChild->mIsPushDownButton );
       button->setChecked( (pChild->mButtonState==2) );
qDebug()<<"button="<<pChild->mName<<" checked="<<(pChild->mButtonState==2);
       button->setFlat( mButtonFlat );
       button->setStyleSheet( css );

       pT->addButton( button );

       if( pChild->mIsFolder )
       {
           QMenu * newMenu = new QMenu( pT );
           button->setMenu( newMenu );
           newMenu->setStyleSheet( css );
           pChild->insertActions( pMainWindow, pT, newMenu );
       }
   }
}


void TAction::insertActions( mudlet * pMainWindow, TToolBar * pT, QMenu * menu )
{
    mpToolBar = pT;
    QIcon icon( mIcon );
    EAction * action = new EAction( icon, mName, pMainWindow );
    action->setCheckable( mIsPushDownButton );
    action->mID = mID;
    action->mpHost = mpHost;
    action->setStatusTip( mName );
    menu->addAction( action );
    //mudlet::self()->bindMenu( menu, action );


    if( mIsFolder )
    {
        QMenu * newMenu = new QMenu( pMainWindow );
        newMenu->setStyleSheet( css );
        action->setMenu( newMenu );

        for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TAction * pChild = *it;
            pChild->insertActions( pMainWindow, pT, newMenu );
        }
    }
}


void TAction::expandToolbar( mudlet * pMainWindow, TEasyButtonBar * pT, QMenu * menu )
{
   for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
   {
       TAction * pChild = *it;
       if( ! pChild->isActive() ) continue;
       QIcon icon( pChild->mIcon );
       QString name = pChild->getName();
       TFlipButton * button = new TFlipButton( pT,pChild, pChild->mID, mpHost );
       button->setIcon( icon );
       button->setText( name );
       button->setCheckable( pChild->mIsPushDownButton );
       button->setFlat( mButtonFlat );
       button->setStyleSheet( css );
       button->setChecked( (pChild->mButtonState==2) );
       //FIXME: Heiko April 2012: only run checkbox button scripts, but run them even if unchecked
       if( pChild->mIsPushDownButton && mpHost->mIsProfileLoadingSequence ) //&& pChild->mButtonState == 2 )
       {
           qDebug()<<"expandToolBar() name="<<pChild->mName<<" executing script";
           QStringList bla;
           pChild->_execute(bla);
       }

       pT->addButton( button );

       if( pChild->mIsFolder )
       {
           QMenu * newMenu = new QMenu( button );
           button->setMenu( newMenu );
           newMenu->setStyleSheet( css );
           pChild->fillMenu( pT, newMenu );
       }
   }
}

void TAction::fillMenu( TEasyButtonBar * pT, QMenu * menu )
{
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAction * pChild = *it;
        if( ! pChild->isActive() ) continue;
        mpEasyButtonBar = pT;
        QIcon icon( mIcon );
        EAction * action = new EAction( icon, pChild->mName, mudlet::self() );
        action->setCheckable( pChild->mIsPushDownButton );
        action->mID = pChild->mID;
        action->mpHost = mpHost;
        action->setStatusTip( pChild->mName );
        action->setChecked((pChild->mButtonState==2));
        //FIXME: Heiko April 2012 -> expandToolBar()
        if( pChild->mIsPushDownButton && mpHost->mIsProfileLoadingSequence )//&& pChild->mButtonState == 2 )
        {
            qDebug()<<"fillMenu() name="<<pChild->mName<<" executing script";
            QStringList bla;
            pChild->_execute(bla);
        }
        menu->addAction( action );
        if( pChild->mIsFolder )
        {
           QMenu * newMenu = new QMenu;
           action->setMenu( newMenu );
           newMenu->setStyleSheet( css );
           pChild->fillMenu( pT, newMenu );
           //mudlet::self()->bindMenu( menu, action );
        }
    }
}

void TAction::insertActions( mudlet * pMainWindow, TEasyButtonBar * pT, QMenu * menu )
{
    mpEasyButtonBar = pT;
    QIcon icon( mIcon );
    EAction * action = new EAction( icon, mName, pMainWindow );
    action->setCheckable( mIsPushDownButton );
    action->mID = mID;
    action->mpHost = mpHost;
    action->setStatusTip( mName );
    menu->addAction( action );
    //mudlet::self()->bindMenu( menu, action );
}
