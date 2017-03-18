/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "pre_guard.h"
#include <QDir>
#include "post_guard.h"

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

TAction::TAction( QString name, Host * pHost )
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
    typedef list<TAction *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
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
    typedef list<TAction *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAction * pChild = *it;
        pChild->compile();
    }
}

bool TAction::setScript( QString & script )
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
    if( mpHost->mLuaInterpreter.compile( code, error ) )
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

void TAction::execute()
{
    if( mIsPushDownButton )
    {
        if( mButtonState == 2 )
        {
            if( ! mCommandButtonUp.isEmpty() )
            {
                mpHost->send( mCommandButtonUp );
            }
        }
        else if( mButtonState == 1 )
        {
            if( ! mCommandButtonDown.isEmpty() )
            {
                mpHost->send( mCommandButtonDown );
            }
        }
        else {
            qWarning() << "TAction::execute() PushDown button:" << mName
                       << "has an invalid button state of:"
                       << mButtonState
                       << "forcing it back to a value of 1!";
            mButtonState = 1;
        }
    }
    else {
        if( mButtonState == 1 )
        {
            if( ! mCommandButtonUp.isEmpty() )
            {
                mpHost->send( mCommandButtonUp );
            }
        }
        else {
            qWarning() << "TAction::execute() NON-PushDown button:" << mName
                       << "has an invalid button state of:"
                       << mButtonState
                       << "forcing it back to a value of 1!";
            mButtonState = 1;
        }
    }

    qDebug() << "TAction::slot_execute() called for: "
             << mName
             << ( mIsPushDownButton ? "a PushDown button" : "a Clickable button" )
             << "it has set the mButtonState to:"
             << mButtonState
             << "and will now call the Lua interpreter to run the lua code.";

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
   typedef list<TAction *>::const_iterator I;
   for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
   {
       TAction * pChild = *it;

       QIcon icon( pChild->getIconPathFileName( false ) );
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
    QIcon icon( getIconPathFileName( false ) );
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

        typedef list<TAction *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TAction * pChild = *it;
            pChild->insertActions( pMainWindow, pT, newMenu );
        }
    }
}


void TAction::expandToolbar( mudlet * pMainWindow, TEasyButtonBar * pT, QMenu * menu )
{
   typedef list<TAction *>::const_iterator I;
   for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
   {
       TAction * pChild = *it;
       if( ! pChild->isActive() )
       {
           continue;
       }
       QIcon icon( pChild->getIconPathFileName( false ) );
       QString name = pChild->getName();
       TFlipButton * button = new TFlipButton( pT,pChild, pChild->mID, mpHost );
       button->setIcon( icon );
       button->setText( name );
       button->setCheckable( pChild->mIsPushDownButton );
       button->setFlat( mButtonFlat );
       button->setStyleSheet( css );
       button->setChecked( (pChild->mButtonState==2) );
       //FIXME: Heiko April 2012: only run checkbox button scripts, but run them even if unchecked
       if( pChild->mIsPushDownButton && mpHost->mIsProfileLoadingSequence )
       {
           qDebug() << "TAction::expandToolBar() in Host::mIsProfileLoadingSequence = true state for PushDown"
                    << pChild->getName()
                    << "button - test executing script...";
           pChild->execute();
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
    typedef list<TAction *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
    {
        TAction * pChild = *it;
        if( ! pChild->isActive() )
        {
            continue;
        }
        mpEasyButtonBar = pT;
        QIcon icon( getIconPathFileName( false ) );
        EAction * action = new EAction( icon, pChild->mName, mudlet::self() );
        action->setCheckable( pChild->mIsPushDownButton );
        action->mID = pChild->mID;
        action->mpHost = mpHost;
        action->setStatusTip( pChild->mName );
        action->setChecked( pChild->mButtonState == 2 );
        //FIXME: Heiko April 2012 -> expandToolBar()
        if( pChild->mIsPushDownButton && mpHost->mIsProfileLoadingSequence )
        {
            qDebug() << "TAction::fillMenu() for menu:"
                     << mName
                     << "in Host::mIsProfileLoadingSequence = true state for PushDown"
                     << pChild->getName()
                     << "button - test executing script...";
            pChild->execute();
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
    QIcon icon( getIconPathFileName( false ) );
    EAction * action = new EAction( icon, mName, pMainWindow );
    action->setCheckable( mIsPushDownButton );
    action->mID = mID;
    action->mpHost = mpHost;
    action->setStatusTip( mName );
    menu->addAction( action );
}

// Returns Absolute path by default that past code would expect from old
// getIcon() method but can convert details to a relative (to profile home)
// directory path and file name which is more "portable" when used on different
// PCs/profiles...
QString TAction::getIconPathFileName( const bool isToForceRelative ) const
{
    if( mIcon.isEmpty() )
    { // Return null QString if no entry
        return QString();
    }

    if( QDir::isRelativePath( mIcon ) )
    { // Is a relative path
        if( isToForceRelative )
        { // And that is what we want
//            qDebug() << "TAction::getIconPathFileName(isToForceRelative: true) is returning a relative mIcon pathFile as:"
//                     << QDir::cleanPath( mIcon );
            return QDir::cleanPath( mIcon );
        }
        else
        { // But we want absolute one
            QString profileHomeDirectoryPathFilename = QStringLiteral( "%1/.config/mudlet/profiles/%2/" )
                                                       .arg( QDir::homePath() )
                                                       .arg( mpHost->getName() );
            QDir profileHomeDirectory( profileHomeDirectoryPathFilename );
//            qDebug() << "TAction::getIconPathFileName(isToForceRelative: false) is returning a relative mIcon pathFile as:"
//                     << QDir::cleanPath( profileHomeDirectory.absoluteFilePath( mIcon ) );
            return QDir::cleanPath( profileHomeDirectory.absoluteFilePath( mIcon ) );
        }
    }
    else
    { // Is an absolute path - not likely to be encountered with current code
      // so leave test qDebug() code uncommented in case it is
        if( isToForceRelative )
        { // But we want a relative one:
            QString profileHomeDirectoryPathFilename = QStringLiteral( "%1/.config/mudlet/profiles/%2/" )
                                                       .arg( QDir::homePath() )
                                                       .arg( mpHost->getName() );
            QDir profileHomeDirectory( profileHomeDirectoryPathFilename );
            qDebug() << "TAction::getIconPathFileName( isToForceRelative: true) is returning an absolute mIcon pathFile as:"
                     << profileHomeDirectory.relativeFilePath( mIcon );
            return profileHomeDirectory.relativeFilePath( mIcon );
        }
        else
        { // And that is what we want
            qDebug() << "TAction::getIconPathFileName( isToForceRelative: false) is returning an absolute mIcon pathFile as:"
                     << QDir::cleanPath( mIcon );
            return QDir::cleanPath( mIcon );
        }
    }
}
