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



TAction::TAction( TAction * parent, Host * pHost ) 
: Tree<TAction>( parent )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
{
} 

TAction::TAction( QString name, Host * pHost ) 
: Tree<TAction>(0)
, mName( name )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
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
    if( mIsActive )
    {
        if( ! mIsFolder )
        {
            if( mRegex.indexIn( toMatch ) == -1 )
            {
                //qDebug()<<"text="<<toMatch;
                //qDebug()<<"Alias("<<mRegexCode<<") did NOT match.";
                //qDebug()<<"sub matches are:"<<mRegex.capturedTexts();
                return false; // regex didn't match
            }
            else
            {
                qDebug()<<"text="<<toMatch;
                qDebug()<<"Action("<<mRegexCode<<") *DID* match!";
                QStringList captureList;
                for( int i=1; i<=mRegex.numCaptures(); i++ )
                {
                    captureList << mRegex.cap(i);
                    qDebug()<<"captured #"<<i<<":"<<mRegex.cap(i);
                }
                
                // call lua alias function with number of matches and matches itselves as arguments
                execute( captureList );    
            }
        }
        
        typedef list<TAction *>::const_iterator I;
        for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++)
        {
            TAction * pChild = *it;
            pChild->match( toMatch );
        }
    }
}


bool TAction::registerAction()
{
    if( ! mpHost )
    {
        qDebug() << "ERROR: TAction::registerTrigger() pHost=0";
        return false;
    }
    qDebug()<<"calling AliasUnit->registerAlias(this) ...";
    return mpHost->getActionUnit()->registerAction( this );    
}

void TAction::compile()
{
}

void TAction::execute(QStringList & list)
{
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
        pL->call( funcName, 0, list, mName );
    }
    else
    {
        TLuaInterpreter * pL = mpHost->getLuaInterpreter();    
        QString funcName = QString("Action") + QString::number( mID ); 
        pL->call( funcName, 0, list, mName );
    }
}

void TAction::insertActions( mudlet * pMainWindow, QToolBar * pT, QMenu * menu )
{
    QMutexLocker locker(& mLock);
        
    const QIcon icon;
    EAction * action = new EAction( pMainWindow, mName );
    action->setIcon( icon ); //FIXME
    action->setCheckable( mIsPushDownButton );
    action->mID = mID;
    action->mpHost = mpHost;
    action->setStatusTip( mName );
    action->setIconText( mName );
    
    
    if( mpParent )
    {
        if( menu)
        {
            menu->addAction( action );
        }
    }
    else
    {
        pT->addAction( action );
    }
    
    if( mIsFolder )
    {
        QMenu * newMenu = new QMenu( pMainWindow );
        action->setMenu( newMenu );
        QWidget * pButton = pT->widgetForAction( action );
        if( pButton )
        {
            ((QToolButton*)pButton)->setPopupMode( QToolButton::InstantPopup );
        }
        //mudlet::connectActionMenu( this );
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

bool TAction::restore( QDataStream & ifs )
{
    ifs >> mName;
    qDebug()<<"restoring:"<<mName;
    ifs >> mScript;
    
    ifs >> mID;
    ifs >> mIsActive;
    ifs >> mIsPushDownButton;
    ifs >> mIsFolder;
    
    qint64 children;
    ifs >> children;
    
    mID = mpHost->getActionUnit()->getNewID();
    
    bool ret = true;
    for( qint64 i=0; i<children; i++ )
    {
        TAction * pChild = new TAction( this, mpHost );
        ret = pChild->restore( ifs );
        pChild->registerAction();
    }

    if (getChildrenList()->size() > 0)
        mIsFolder = true;
    return ret;
}

TAction& TAction::clone(const TAction& b)
{
    mName = b.mName;
    mCommand = b.mCommand;
    mRegexCode = b.mRegexCode;
    mRegex = b.mRegex;
    mScript = b.mScript;
    mIsPushDownButton = b.mIsPushDownButton;
    mIsActive = b.mIsActive;
    mIsFolder = b.mIsFolder;
    mpHost = b.mpHost;
    mNeedsToBeCompiled = b.mNeedsToBeCompiled;
    mIcon = b.mIcon;
    return *this;
}

bool TAction::isClone(TAction &b) const {
    return (mName == b.mName && mCommand == b.mCommand && mRegexCode == b.mRegexCode && mRegex == b.mRegex && mScript == b.mScript && mIsPushDownButton == b.mIsPushDownButton && \
        mIsActive == b.mIsActive && mIsFolder == b.mIsFolder && mpHost == b.mpHost && mNeedsToBeCompiled == b.mNeedsToBeCompiled && mIcon == b.mIcon);
}
