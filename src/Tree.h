#ifndef MUDLET_TREE_H
#define MUDLET_TREE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "pre_guard.h"
#include <QString>
#include "post_guard.h"

#include <iostream>
#include <list>


template <class T>
class Tree
{
public:
    Tree();
    Tree(T* parent);
    virtual ~Tree();
    T* getParent() { return mpParent; }
    std::list<T*>* getChildrenList();
    bool hasChildren() { return (mpMyChildrenList->size() > 0); }
    int getChildCount() { return mpMyChildrenList->size(); }
    int getID() { return mID; }
    void setID(int id) { mID = id; }
    void addChild(T* newChild, int parentPostion = -1, int parentPosition = -1);
    bool popChild(T* removeChild);
    void setParent(T* parent);
    void enableFamily();
    void disableFamily();
    bool isActive();
    bool activate();
    void deactivate();
    bool setIsActive(bool);
    bool shouldBeActive();
    void setShouldBeActive(bool b);

    // Returns true if all the ancesters of this node are active. If there are no ancestors it also returns true.
    bool ancestorsActive();

    T* mpParent;
    std::list<T*>* mpMyChildrenList;
    int mID;
    QString& getError();
    void setError(QString);
    bool state();
    QString getPackageName() { return mPackageName; }
    void setPackageName(const QString& n) { mPackageName = n; }
    void setModuleName(const QString& n) { mModuleName = n; }
    QString getModuleName() { return mModuleName; }
    QString mPackageName;
    QString mModuleName;


protected:
    virtual bool canBeActivated();
    bool mOK_init;
    bool mOK_code;

private:
    bool mActive;
    bool mUserActiveState;
    QString mErrorMessage;
};

template <class T>
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

template <class T>
Tree<T>::Tree( T * pParent )
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
        pParent->addChild((T*)(this));
    }
    else
        mpParent = 0;
}

template <class T>
Tree<T>::~Tree()
{
    while( mpMyChildrenList->size() > 0 )
    {
        auto it = mpMyChildrenList->begin();
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

template <class T>
bool Tree<T>::ancestorsActive()
{
    Tree<T> * node(mpParent);
    while(node)
    {
        if(!node->isActive())
        {
            return false;
        }
        node=node->mpParent;
    }
    return true;
}

template <class T>
bool Tree<T>::shouldBeActive()
{
    return mUserActiveState;
}

template <class T>
void Tree<T>::setShouldBeActive( bool b )
{
    mUserActiveState = b;
}

template <class T>
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

template <class T>
inline bool Tree<T>::state()
{
    return ( mOK_init && mOK_code );
}

template <class T>
inline bool Tree<T>::canBeActivated()
{
    return ( shouldBeActive() && state() );
}

template <class T>
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

template <class T>
void Tree<T>::deactivate()
{
    mActive = false;
}

template <class T>
bool Tree<T>::isActive()
{
    return (mActive && canBeActivated());
}

template <class T>
void Tree<T>::enableFamily()
{
    activate();
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        (*it)->enableFamily();
    }
}

template <class T>
void Tree<T>::disableFamily()
{
    deactivate();
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        (*it)->disableFamily();
    }
}

template <class T>
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
        for (auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
        {
            if ( cnt >= childPosition )
            {
                mpMyChildrenList->insert( it, newChild );
                break;
            }
            cnt++;
        }
    }
}

template <class T>
void Tree<T>::setParent( T * pParent )
{
    mpParent = pParent;
}

template <class T>
bool Tree<T>::popChild( T * pChild )
{
    for(auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++ )
    {
        if( *it == pChild )
        {
            mpMyChildrenList->remove( pChild );
            return true;
        }
    }
    return false;
}

template <class T>
std::list<T *> * Tree<T>::getChildrenList()
{
    return mpMyChildrenList;
}

template <class T>
QString & Tree<T>::getError()
{
    return mErrorMessage;
}

template <class T>
void Tree<T>::setError( QString error )
{
    mErrorMessage = error;
}

#endif // MUDLET_TREE_H
