/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
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

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include "Host.h"
#include "TLuaInterpreter.h"
#include "TConsole.h"

#include <QDebug>
#include "AliasUnit.h"

using namespace std;


void AliasUnit::_uninstall( TAlias * pChild, QString packageName )
{
    typedef list<TAlias *>::const_iterator I;
    list<TAlias*> * childrenList = pChild->mpMyChildrenList;
    for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
    {
        TAlias * pT = *it2;
        _uninstall( pT, packageName );
        if( pT->mPackageName == packageName )
        {
            uninstallList.append( pT );
        }
    }
}


void AliasUnit::uninstall( QString packageName )
{
    typedef std::list<TAlias *>::iterator IT;
    for( IT it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it ++ )
    {
        TAlias * pT = *it;
        _uninstall( pT, packageName );
        if( pT->mPackageName == packageName )
        {
            uninstallList.append( pT );
        }
    }
    for( int i=0; i<uninstallList.size(); i++ )
    {
        unregisterAlias(uninstallList[i]);

    }
}

void AliasUnit::compileAll()
{
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        if( pChild->isActive() )
        {
            pChild->compileAll();
        }
    }
    for( int i=0; i<uninstallList.size(); i++ )
    {
        unregisterAlias(uninstallList[i]);
    }
}

void AliasUnit::initStats()
{
    statsAliasTotal = 0;
    statsTempAliass = 0;
    statsActiveAliass = 0;
    statsActiveAliassMax = 0;
    statsActiveAliassMin = 0;
    statsActiveAliassAverage = 0;
    statsTempAliassCreated = 0;
    statsTempAliassKilled = 0;
    statsAverageLineProcessingTime = 0;
    statsMaxLineProcessingTime = 0;
    statsMinLineProcessingTime = 0;
    statsRegexAliass = 0;
}

void AliasUnit::addAliasRootNode( TAlias * pT, int parentPosition, int childPosition, bool moveAlias )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    if( ( parentPosition == -1 ) || ( childPosition >= static_cast<int>(mAliasRootNodeList.size()) ) )
    {
        mAliasRootNodeList.push_back( pT );
    }
    else
    {
         // insert item at proper position
        int cnt = 0;
        typedef std::list<TAlias *>::iterator IT;
        for( IT it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mAliasRootNodeList.insert( it, pT );
                break;
            }
            cnt++;
        }
    }

    if( ! moveAlias )
    {
        mAliasMap.insert( pT->getID(), pT );
    }
}

void AliasUnit::reParentAlias( int childID, int oldParentID, int newParentID, int parentPosition, int childPosition )
{
    TAlias * pOldParent = getAliasPrivate( oldParentID );
    TAlias * pNewParent = getAliasPrivate( newParentID );
    TAlias * pChild = getAliasPrivate( childID );
    if( ! pChild )
    {
        return;
    }
    if( pOldParent )
    {
        pOldParent->popChild( pChild );
    }
    else
    {
        mAliasRootNodeList.remove( pChild );
    }
    if( pNewParent )
    {
        pNewParent->addChild( pChild, parentPosition, childPosition );
        if( pChild ) pChild->setParent( pNewParent );
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    else
    {
        pChild->Tree<TAlias>::setParent( 0 );
        addAliasRootNode( pChild, parentPosition, childPosition, true );
    }
}

void AliasUnit::removeAliasRootNode( TAlias * pT )
{
    if( ! pT ) return;
    if( ! pT->mIsTempAlias )
    {
        mLookupTable.remove( pT->mName, pT );
    }
    else
    {
        mLookupTable.remove( pT->getName() );
    }
    mAliasMap.remove( pT->getID() );
    mAliasRootNodeList.remove( pT );
}

TAlias * AliasUnit::getAlias( int id )
{
    QMutexLocker locker(& mAliasUnitLock);
    if( mAliasMap.find( id ) != mAliasMap.end() )
    {
        return mAliasMap.value( id );
    }
    else
    {
        return 0;
    }
}

TAlias * AliasUnit::getAliasPrivate( int id )
{
    if( mAliasMap.find( id ) != mAliasMap.end() )
    {
        return mAliasMap.value( id );
    }
    else
    {
        return 0;
    }
}

bool AliasUnit::registerAlias( TAlias * pT )
{
    if( ! pT ) return false;

    if( pT->getParent() )
    {
        addAlias( pT );
        return true;
    }
    else
    {
        addAliasRootNode( pT );
        return true;
    }
}

void AliasUnit::unregisterAlias( TAlias * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeAlias( pT );
        return;
    }
    else
    {
        removeAliasRootNode( pT );
        return;
    }
}


void AliasUnit::addAlias( TAlias * pT )
{
    if( ! pT ) return;

    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }

    mAliasMap.insert( pT->getID(), pT );
}

void AliasUnit::removeAlias( TAlias * pT )
{
    if( ! pT ) return;
    if( ! pT->mIsTempAlias )
    {
        mLookupTable.remove( pT->mName, pT );
    }
    else
        mLookupTable.remove( pT->getName() );

    mAliasMap.remove(pT->getID());
}


qint64 AliasUnit::getNewID()
{
    return ++mMaxID;
}

bool AliasUnit::processDataStream( QString & data )
{
    TLuaInterpreter * Lua = mpHost->getLuaInterpreter();
    QString lua_command_string = "command";
    Lua->set_lua_string( lua_command_string, data );
    bool state = false;
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        // = data.replace( "\n", "" );
        if( pChild->match( data ) )
        {
            state = true;
        }
    }
    // the idea to get "command" after alias processing is finished and send its value
    // was too difficult for users because if multiple alias change the value of command it becomes too difficult to handle for many users
    // it's easier if we simply intercepts the command and hand responsibility for
    // sending a command to the user scripts.
    //data = Lua->get_lua_string( lua_command_string );
    return state;
}



void AliasUnit::stopAllTriggers()
{
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        QString name = pChild->getName();
        pChild->disableFamily();
    }
}

void AliasUnit::reenableAllTriggers()
{
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        pChild->enableFamily();
    }
}

//bool AliasUnit::serialize( QDataStream & ofs )
//{
//    return true;
//}


//bool AliasUnit::restore( QDataStream & ifs, bool initMode )
//{
//    return true;
//}

TAlias * AliasUnit::findAlias( QString & name )
{
    //TAlias * pT = 0;
    QMap<QString, TAlias *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TAlias * pT = it.value();
        return pT;
    }
    return 0;
}

bool AliasUnit::enableAlias( QString & name )
{
    bool found = false;
    QMap<QString, TAlias *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TAlias * pT = it.value();
        pT->setIsActive( true );
        ++it;
        found = true;
    }
    return found;
}

bool AliasUnit::disableAlias( QString & name )
{
    bool found = false;
    QMap<QString, TAlias *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TAlias * pT = it.value();
        pT->setIsActive( false );
        ++it;
        found = true;
    }
    return found;
}


bool AliasUnit::killAlias( QString & name )
{
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        if( pChild->getName() == name )
        {
            // only temporary Aliass can be killed
            if( ! pChild->isTempAlias() )
            {
                return false;
            }
            else
            {
                pChild->setIsActive( false );
                markCleanup( pChild );
                return true;
            }
        }
    }
    return false;
}


void AliasUnit::dump()
{
    //bool ret = true;

    typedef list<TAlias *>::const_iterator I;
    cout << "AliasUnit::dump() entries="<<mAliasRootNodeList.size()<<endl;

    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        pChild->DumpFamily();
    }
}

void AliasUnit::_assembleReport( TAlias * pChild )
{
    typedef list<TAlias *>::const_iterator I;
    list<TAlias*> * childrenList = pChild->mpMyChildrenList;
    for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
    {
        TAlias * pT = *it2;
        _assembleReport( pT );
        if( pT->isActive() ) statsActiveAliass++;
        if( pT->isTempAlias() ) statsTempAliass++;
        statsAliasTotal++;
    }
}

QString AliasUnit::assembleReport()
{
    statsActiveAliass = 0;
    statsAliasTotal = 0;
    statsTempAliass = 0;
    typedef list<TAlias *>::const_iterator I;
    for( I it = mAliasRootNodeList.begin(); it != mAliasRootNodeList.end(); it++)
    {
        TAlias * pChild = *it;
        if( pChild->isActive() ) statsActiveAliass++;
        if( pChild->isTempAlias() ) statsTempAliass++;
        statsAliasTotal++;
        list<TAlias*> * childrenList = pChild->mpMyChildrenList;
        for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
        {
            TAlias * pT = *it2;
            _assembleReport( pT );
            if( pT->isActive() ) statsActiveAliass++;
            if( pT->isTempAlias() ) statsTempAliass++;
            statsAliasTotal++;
        }
    }
    QStringList msg;
    msg << "Aliass current total: " << QString::number(statsAliasTotal) << "\n"
        << "tempAliass current total: " << QString::number(statsTempAliass) << "\n"
        << "active Aliass: " << QString::number(statsActiveAliass) << "\n";
        /*<< "active Aliass max this session: " << QString::number(statsActiveAliassMax) << "\n"
        << "active Aliass min this session: " << QString::number(statsActiveAliassMin) << "\n"
        << "active Aliass average this session: " << QString::number(statsActiveAliassAverage) << "\n"*/
        //<< "tempAliass created this session: " << QString::number(statsTempAliassCreated) << "\n"
        //<< "tempAliass killed this session: " << QString::number(statsTempAliassKilled) << "\n"
        //<< "current total regex Aliass: " << QString::number(statsRegexAliass) << "\n"
        //<< "average line processing time: " << QString::number(statsAverageLineProcessingTime) << "\n"
        //<< "max line processing time: " << QString::number(statsMaxLineProcessingTime) << "\n"
        //<< "min line processing time: " << QString::number(statsMinLineProcessingTime) << "\n";
    return msg.join("");

}

void AliasUnit::doCleanup()
{
    typedef list<TAlias *>::iterator I;
    for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++)
    {
        delete *it;
    }
    mCleanupList.clear();
}

void AliasUnit::markCleanup( TAlias * pT )
{
    typedef list<TAlias *>::iterator I;
    for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++)
    {
        if( *it == pT )
        {
            return;
        }
    }
    mCleanupList.push_back( pT );
}






