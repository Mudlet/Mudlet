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


#ifndef __TREE__H
#define __TREE__H



#include <iostream>
#include <fstream>
#include <list>
#include <string>

#include <QMutex>
#include <QMutexLocker>

//#include "TTrigger.h"

#include <QDebug>

template<class T>
class Tree
{
public:
                       Tree();
                       Tree( T * parent );		     
    virtual            ~Tree(); 
                       T * getParent()      { return mpParent; }
    std::list<T *> *   getChildrenList();
    bool               hasChildren()   { return (mpMyChildrenList->size() > 0); }
    int                getChildCount()  { return mpMyChildrenList->size(); }
    void               DumpFamily();
    void               Dump();
    void               setFullyExpanded()    { FullyExpanded = true; }
    bool               isFullyExpanded()     { return FullyExpanded; }
    qint64             getID()               { return mID; }
    void               setID( qint64 id )       { QMutexLocker locker(& mLock); mID=id; }
    void               addChild( T * newChild );
    bool               popChild( T * removeChild );
    void               setParent( T * parent );
    
    T *                mpParent;
    std::list<T *> *   mpMyChildrenList;
    qint64             mID;
    
    
    bool               FullyExpanded;
    QMutex             mLock;
    
};

template<class T>
Tree<T>::Tree() 
: mpParent( 0 )
, mpMyChildrenList( new std::list<T *> )
, mID( 0 ) 
{
}

template<class T>
Tree<T>::Tree( T *  pParent ) 
: mpParent( pParent )
, mpMyChildrenList( new std::list<T *> )
, mID( 0 )
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
void Tree<T>::addChild( T * newChild )
{
    QMutexLocker locker(& mLock); 
    mpMyChildrenList->push_back( newChild );
}

template<class T>
void Tree<T>::setParent( T * pParent )
{
    QMutexLocker locker(& mLock);
    mpParent = pParent;
}

template<class T>
bool Tree<T>::popChild( T * pChild )
{
    QMutexLocker locker(& mLock);
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

