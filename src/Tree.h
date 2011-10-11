/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn                                     *
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


#ifndef __TREE__H
#define __TREE__H



#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <QString>
#include <QDebug>

template<class T>
class Tree
{
public:
                       Tree();
                       Tree( T * parent );
    virtual            ~Tree();
                       T * getParent()             { return mpParent; }
    std::list<T *> *   getChildrenList();
    bool               hasChildren()               { return (mpMyChildrenList->size() > 0); }
    int                getChildCount()             { return mpMyChildrenList->size(); }
    void               DumpFamily();
    void               Dump();
    void               setFullyExpanded()          { FullyExpanded = true; }
    bool               isFullyExpanded()           { return FullyExpanded; }
    qint64             getID()                     { return mID; }
    void               setID( qint64 id )          { mID=id; }
    void               addChild( T * newChild, int parentPostion = -1, int parentPosition = -1 );
    bool               popChild( T * removeChild );
    void               setParent( T * parent );
    void               enableFamily();
    void               disableFamily();
    bool               isActive();
    bool               activate();
    void               deactivate();
    bool               setIsActive( bool );
    bool               shouldBeActive();
    void               setShouldBeActive( bool b );
    T *                mpParent;
    std::list<T *> *   mpMyChildrenList;
    qint64             mID;
    QString &          getError();
    void               setError( QString );
    bool               state();
    QString            getPackageName() { return mPackageName; }
    void               setPackageName( QString n ){ mPackageName = n; }
    void               setModuleName( QString n ){ mModuleName = n;}
    QString            getModuleName() {return mModuleName;}
    bool               FullyExpanded;
    QString            mPackageName;
    QString            mModuleName;


protected:

    virtual bool       canBeActivated();
    bool               mOK_init;
    bool               mOK_code;

private:

    bool               mActive;
    bool               mUserActiveState;
    QString            mErrorMessage;


};

template<class T>
Tree<T>::Tree()
: mpParent( 0 )
, mpMyChildrenList( new std::list<T *> )
, mID( 0 )
, mOK_init( true )
, mOK_code( true )
, mActive( false )
, mUserActiveState( false )
{
}

template<class T>
Tree<T>::Tree( T *  pParent )
: mpParent( pParent )
, mpMyChildrenList( new std::list<T *> )
, mID( 0 )
, mOK_init( true )
, mOK_code( true )
, mActive( false )
, mUserActiveState( false )
{
    if( pParent )
    {
        pParent->addChild( (T*)( this ) );
    }
    else mpParent=0;
}

template<class T>
Tree<T>::~Tree()
{
    while( mpMyChildrenList->size() > 0 )
    {
        typename std::list<T*>::iterator it = mpMyChildrenList->begin();
        T * pChild = *it;
        delete pChild;
    }
    delete mpMyChildrenList;
    if( mpParent != 0 )
    {
        mpParent->popChild( (T*)this ); // tell parent about my death
        if( std::uncaught_exception() )
        {
            std::cout << "ERROR: Hook destructed during stack rewind because of an uncaught exception." << std::endl;
        }
    }
}


template<class T>
bool Tree<T>::shouldBeActive()
{
    return mUserActiveState;
}

template<class T>
void Tree<T>::setShouldBeActive( bool b )
{
    mUserActiveState = b;
}

template<class T>
bool Tree<T>::setIsActive( bool b )
{
    setShouldBeActive( b );
    if( b )
    {
        return activate();
    }
    else
    {
        mActive = false;
        return false;
    }
}

template<class T>
inline bool Tree<T>::state()
{
    return (mOK_init && mOK_code);
}

template<class T>
inline bool Tree<T>::canBeActivated()
{
    return (shouldBeActive() && state());
}

template<class T>
bool Tree<T>::activate()
{
    if( canBeActivated() )
    {
        mActive = true;
        return true;
    }
    mActive = false;
    return false;
}

template<class T>
void Tree<T>::deactivate()
{
    mActive = false;
}

template<class T>
bool Tree<T>::isActive()
{
    return (mActive && canBeActivated());
}

template<class T>
void Tree<T>::enableFamily()
{
    activate();
    typedef typename std::list<T *>::const_iterator IT;
    for( IT it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        (*it)->enableFamily();
    }
}

template<class T>
void Tree<T>::disableFamily()
{
    deactivate();
    typedef typename std::list<T *>::const_iterator IT;
    for( IT it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        (*it)->disableFamily();
    }
}

template<class T>
void Tree<T>::addChild( T * newChild, int parentPosition, int childPosition )
{
    if( ( parentPosition == -1 ) || ( childPosition >= static_cast<int>(mpMyChildrenList->size()) ) )
    {
        mpMyChildrenList->push_back( newChild );
    }
    else
    {
        // insert item at proper position
        int cnt = 0;
        typedef typename std::list<T *>::iterator IT;
        for( IT it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mpMyChildrenList->insert( it, newChild );
                break;
            }
            cnt++;
        }
    }
}

template<class T>
void Tree<T>::setParent( T * pParent )
{
    mpParent = pParent;
}

template<class T>
bool Tree<T>::popChild( T * pChild )
{
    typedef typename std::list<T *>::const_iterator IT;
    for( IT it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        if( *it == pChild )
        {
            mpMyChildrenList->remove( pChild );
            return true;
        }
    }
    return false;
}

template<class T>
std::list<T *> * Tree<T>::getChildrenList()
{
    return mpMyChildrenList;
}

template<class T>
QString & Tree<T>::getError()
{
    return mErrorMessage;
}

template<class T>
void Tree<T>::setError( QString error )
{
    mErrorMessage = error;
}


template<class T>
void Tree<T>::DumpFamily()
{
    Dump();
    typedef typename std::list<T *>::const_iterator I;
    for( I it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        T * pChild = *it;
        if( pChild ) pChild->DumpFamily();
    }
}

template<class T>
void Tree<T>::Dump()
{
    std::cout << "My ID=" << mID << " my parent="<< mpParent << std::endl;
    std::cout << " my children are:";
    typedef typename std::list<T *>::const_iterator IT;
    for( IT it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        std::cout << " dumping:"<<std::endl;
        T* pChild = *it;
        if( pChild ) std::cout << pChild->mID << ", ";
    }
    std::cout << "ende dump()"<< std::endl;
}

#endif

