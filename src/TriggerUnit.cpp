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


#include "TriggerUnit.h"


#include "Host.h"
#include "TConsole.h"
#include "TLuaInterpreter.h"
#include "TTrigger.h"

#include <iostream>
#include <ostream>


using namespace std;

void TriggerUnit::initStats()
{
    statsTriggerTotal = 0;
    statsTempTriggers = 0;
    statsActiveTriggers = 0;
    statsActiveTriggersMax = 0;
    statsActiveTriggersMin = 0;
    statsActiveTriggersAverage = 0;
    statsTempTriggersCreated = 0;
    statsTempTriggersKilled = 0;
    statsAverageLineProcessingTime = 0;
    statsMaxLineProcessingTime = 0;
    statsMinLineProcessingTime = 0;
    statsRegexTriggers = 0;
}

void TriggerUnit::_uninstall( TTrigger * pChild, QString packageName )
{
    typedef list<TTrigger *>::const_iterator I;
    list<TTrigger*> * childrenList = pChild->mpMyChildrenList;
    for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
    {
        TTrigger * pT = *it2;
        _uninstall( pT, packageName );
        uninstallList.append( pT );
    }
}


void TriggerUnit::uninstall( QString packageName )
{
    typedef std::list<TTrigger *>::iterator IT;
    for( IT it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it ++ )
    {
        TTrigger * pT = *it;

        if( pT->mPackageName == packageName )
        {
            _uninstall( pT, packageName );
            uninstallList.append( pT );
        }
    }
    for( int i=0; i<uninstallList.size(); i++ )
    {
        unregisterTrigger(uninstallList[i]);

    }
     uninstallList.clear();
}

void TriggerUnit::removeAllTempTriggers()
{
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        if( pChild->isTempTrigger() )
        {
            pChild->setIsActive( false );
            markCleanup( pChild );
        }
    }
}

void TriggerUnit::addTriggerRootNode( TTrigger * pT, int parentPosition, int childPosition, bool moveTrigger )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    if( ( parentPosition == -1 ) || ( childPosition >= static_cast<int>(mTriggerRootNodeList.size()) ) )
    {
        mTriggerRootNodeList.push_back( pT );
    }
    else
    {
         // insert item at proper position
        int cnt = 0;
        typedef std::list<TTrigger *>::iterator IT;
        for( IT it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mTriggerRootNodeList.insert( it, pT );
                break;
            }
            cnt++;
        }
    }

    if( ! moveTrigger )
    {
        mTriggerMap.insert( pT->getID(), pT );
    }
}

void TriggerUnit::reParentTrigger( int childID, int oldParentID, int newParentID, int parentPosition, int childPosition )
{
    TTrigger * pOldParent = getTriggerPrivate( oldParentID );
    TTrigger * pNewParent = getTriggerPrivate( newParentID );
    TTrigger * pChild = getTriggerPrivate( childID );
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
        mTriggerRootNodeList.remove( pChild );
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
        pChild->Tree<TTrigger>::setParent( 0 );
        addTriggerRootNode( pChild, parentPosition, childPosition, true );
    }
}

void TriggerUnit::removeTriggerRootNode( TTrigger * pT )
{
    if( ! pT ) return;
    if( ! pT->mIsTempTrigger )
    {
        mLookupTable.remove( pT->mName, pT );
    }
    else
    {
        mLookupTable.remove( pT->getName() );
    }
    mTriggerMap.remove( pT->getID() );
    mTriggerRootNodeList.remove( pT );
}

TTrigger * TriggerUnit::getTrigger( int id )
{
    QMutexLocker locker(& mTriggerUnitLock);
    if( mTriggerMap.find( id ) != mTriggerMap.end() )
    {
        return mTriggerMap.value( id );
    }
    else
    {
        return 0;
    }
}

TTrigger * TriggerUnit::getTriggerPrivate( int id )
{
    if( mTriggerMap.find( id ) != mTriggerMap.end() )
    {
        return mTriggerMap.value( id );
    }
    else
    {
        return 0;
    }
}

bool TriggerUnit::registerTrigger( TTrigger * pT )
{
    if( ! pT ) return false;

    if( pT->getParent() )
    {
        addTrigger( pT );
        return true;
    }
    else
    {
        addTriggerRootNode( pT );
        return true;
    }
}

void TriggerUnit::unregisterTrigger( TTrigger * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeTrigger( pT );
        return;
    }
    else
    {
        removeTriggerRootNode( pT );
        return;
    }
}


void TriggerUnit::addTrigger( TTrigger * pT )
{
    if( ! pT ) return;

    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }

    mTriggerMap.insert( pT->getID(), pT );
}

void TriggerUnit::removeTrigger( TTrigger * pT )
{
    if( ! pT ) return;
    if( ! pT->mIsTempTrigger )
    {
        mLookupTable.remove( pT->mName, pT );
    }
    else
        mLookupTable.remove( pT->getName() );

    mTriggerMap.remove(pT->getID());
}

// trigger matching order is permantent trigger objects first, temporary objects second
// after package import or module sync this order needs to be reset
void TriggerUnit::reorderTriggersAfterPackageImport()
{
    QList<TTrigger *> tempList;
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        if( pChild->isTempTrigger() )
        {
            tempList.push_back( pChild );
        }
    }
    for( int i=0; i<tempList.size(); i++ )
    {
        mTriggerRootNodeList.remove( tempList[i] );
    }
    for( int i=0; i<tempList.size(); i++ )
    {
        mTriggerRootNodeList.push_back( tempList[i] );
    }

}

qint64 TriggerUnit::getNewID()
{
    return ++mMaxID;
}

void TriggerUnit::processDataStream( QString & data, int line )
{
    if( data.size() > 0 )
    {
        char * subject = (char *) malloc( strlen( data.toLocal8Bit().data() ) + 1 );
        strcpy( subject, data.toLocal8Bit().data() );

        typedef list<TTrigger *>::const_iterator I;
        for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
        {
            TTrigger * pChild = *it;
            //QFuture<bool> future = QtConcurrent::run( pChild, &TTrigger::match, subject, data, line, 0 );
            //future.waitForFinished();

            pChild->match( subject, data, line );
        }
        free( subject );

        for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++ )
        {
            delete *it;
        }
        mCleanupList.clear();
    }

}

void TriggerUnit::compileAll()
{
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        if( pChild->isActive() )
        {
            pChild->compileAll();
        }
    }
}

void TriggerUnit::stopAllTriggers()
{
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        QString name = pChild->getName();
        pChild->disableFamily();
    }
}

void TriggerUnit::reenableAllTriggers()
{
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        pChild->enableFamily();
    }
}

TTrigger * TriggerUnit::findTrigger( QString & name )
{
    QMap<QString, TTrigger *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TTrigger * pT = it.value();
        return pT;
    }
    return 0;
}

bool TriggerUnit::enableTrigger( QString & name )
{
    bool found = false;
    QMap<QString, TTrigger *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TTrigger * pT = it.value();
        pT->setIsActive( true );
        ++it;
        found = true;
    }
    return found;
}

bool TriggerUnit::disableTrigger( QString & name )
{
    bool found = false;
    QMap<QString, TTrigger *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TTrigger * pT = it.value();
        pT->setIsActive( false );
        ++it;
        found = true;
    }
    return found;
}

void TriggerUnit::setTriggerStayOpen( QString name, int lines )
{
    QMap<QString, TTrigger *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TTrigger * pT = it.value();
        pT->mKeepFiring = lines;
        ++it;
    }
}

bool TriggerUnit::killTrigger( QString & name )
{
    QMap<QString, TTrigger *>::const_iterator it = mLookupTable.find( name );
    while( it != mLookupTable.end() && it.key() == name )
    {
        TTrigger * pT = it.value();
        if( pT->isTempTrigger() ) //this function is only defined for tempTriggers, permanent objects cannot be removed
        {
            // there can only be a single tempTrigger by this name and this function ignores non-tempTriggers by definition
            markCleanup( pT );
            return true;
        }
        it++;
    }
    return false;
}

void TriggerUnit::_assembleReport( TTrigger * pChild )
{
    typedef list<TTrigger *>::const_iterator I;
    list<TTrigger*> * childrenList = pChild->mpMyChildrenList;
    for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
    {
        TTrigger * pT = *it2;
        _assembleReport( pT );
        if( pT->isActive() ) statsActiveTriggers++;
        if( pT->isTempTrigger() ) statsTempTriggers++;
        statsPatterns += pT->mRegexCodeList.size();
        statsTriggerTotal++;
    }
}

QString TriggerUnit::assembleReport()
{
    statsActiveTriggers = 0;
    statsTriggerTotal = 0;
    statsTempTriggers = 0;
    statsPatterns = 0;
    typedef list<TTrigger *>::const_iterator I;
    for( I it = mTriggerRootNodeList.begin(); it != mTriggerRootNodeList.end(); it++)
    {
        TTrigger * pChild = *it;
        if( pChild->isActive() ) statsActiveTriggers++;
        if( pChild->isTempTrigger() ) statsTempTriggers++;
        statsPatterns += pChild->mRegexCodeList.size();
        statsTriggerTotal++;
        list<TTrigger*> * childrenList = pChild->mpMyChildrenList;
        for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
        {
            TTrigger * pT = *it2;
            _assembleReport( pT );
            if( pT->isActive() ) statsActiveTriggers++;
            if( pT->isTempTrigger() ) statsTempTriggers++;
            statsPatterns += pT->mRegexCodeList.size();
            statsTriggerTotal++;
        }
    }
    QStringList msg;
    msg << "triggers current total: " << QString::number(statsTriggerTotal) << "\n"
        << "trigger patterns total: " << QString::number(statsPatterns) << "\n"
        << "tempTriggers current total: " << QString::number(statsTempTriggers) << "\n"
        << "active triggers: " << QString::number(statsActiveTriggers) << "\n";
        /*<< "active triggers max this session: " << QString::number(statsActiveTriggersMax) << "\n"
        << "active triggers min this session: " << QString::number(statsActiveTriggersMin) << "\n"
        << "active triggers average this session: " << QString::number(statsActiveTriggersAverage) << "\n"*/
        //<< "tempTriggers created this session: " << QString::number(statsTempTriggersCreated) << "\n"
        //<< "tempTriggers killed this session: " << QString::number(statsTempTriggersKilled) << "\n"
        //<< "current total regex triggers: " << QString::number(statsRegexTriggers) << "\n"
        //<< "average line processing time: " << QString::number(statsAverageLineProcessingTime) << "\n"
        //<< "max line processing time: " << QString::number(statsMaxLineProcessingTime) << "\n"
        //<< "min line processing time: " << QString::number(statsMinLineProcessingTime) << "\n";
    return msg.join("");

}

void TriggerUnit::doCleanup()
{
    typedef list<TTrigger *>::iterator I;
    for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++)
    {
        delete *it;
    }
    mCleanupList.clear();
}

void TriggerUnit::markCleanup( TTrigger * pT )
{
    typedef list<TTrigger *>::iterator I;
    for( I it = mCleanupList.begin(); it != mCleanupList.end(); it++)
    {
        if( *it == pT )
        {
            return;
        }
    }
    mCleanupList.push_back( pT );
}
