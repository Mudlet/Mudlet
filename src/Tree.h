#ifndef MUDLET_TREE_H
#define MUDLET_TREE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019 by Stephen Lyons - slysven@virginmedia.com         *
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
    explicit Tree();
    explicit Tree(T* parent);
    virtual ~Tree();

    T* getParent() const { return mpParent; }
    std::list<T*>* getChildrenList() const;
    bool hasChildren() const { return (!mpMyChildrenList->empty()); }
    int getChildCount() const { return mpMyChildrenList->size(); }
    int getID() const { return mID; }
    virtual void setID(const int id) { mID = id; }
    void addChild(T* newChild, int parentPostion = -1, int parentPosition = -1);
    bool popChild(T* removeChild);
    void setParent(T* parent);
    void enableFamily();
    void disableFamily();
    bool isActive() const;
    bool activate();
    void deactivate();
    bool setIsActive(bool);
    bool shouldBeActive() const;
    void setShouldBeActive(bool b);
    bool isTemporary() const;
    void setTemporary(bool state);
    // Returns true if all the ancesters of this node are active. If there are no ancestors it also returns true.
    bool ancestorsActive() const;
    QString& getError();
    void setError(QString);
    bool state() const;
    QString getPackageName() const { return mPackageName; }
    void setPackageName(const QString& n) { mPackageName = n; }
    void setModuleName(const QString& n) { mModuleName = n; }
    QString getModuleName() const { return mModuleName; }
    bool isFolder() { return mFolder; }
    void setIsFolder(bool b) { mFolder = b; }

    T* mpParent;
    std::list<T*>* mpMyChildrenList;
    int mID;
    QString mPackageName;
    QString mModuleName;

protected:
    virtual bool canBeActivated() const;

    bool mOK_init;
    bool mOK_code;

private:
    bool mActive;
    bool mUserActiveState;
    QString mErrorMessage;
    bool mTemporary;
    bool mFolder;
};

template <class T>
Tree<T>::Tree()
: mpParent( nullptr )
, mpMyChildrenList( new std::list<T *> )
, mID( 0 )
, mOK_init( true )
, mOK_code( true )
, mActive( false )
, mUserActiveState( false )
, mTemporary( false )
, mFolder( false )
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
, mTemporary( false )
, mFolder( false )
{
    if (pParent) {
        pParent->addChild(static_cast<T*>(this));
    } else {
        mpParent = nullptr;
    }
}

template <class T>
Tree<T>::~Tree()
{
    while (!mpMyChildrenList->empty()) {
        auto it = mpMyChildrenList->begin();
        T* pChild = *it;
        delete pChild;
    }
    delete mpMyChildrenList;
    if (mpParent) {
        mpParent->popChild(static_cast<T*>(this)); // tell parent about my death
        // FIXME: std::uncaught_exception() is deprecated in C++17 and is to be
        // removed in C++20 - and this throws out a lot of build time warnings
        // that are currently being suppressed by a `-Wno-deprecated-declarations`
        // but which might also be masking issues in other areas:
        if (std::uncaught_exception()) {
            std::cout << "ERROR: Hook destructed during stack rewind because of an uncaught exception." << std::endl;
        }
    }
}

template <class T>
void Tree<T>::setTemporary(const bool state) {
    mTemporary = state;
}

template <class T>
bool Tree<T>::isTemporary() const {
    return mTemporary;
}

template <class T>
bool Tree<T>::ancestorsActive() const
{
    Tree<T>* node(mpParent);
    while (node) {
        if (!node->isActive()) {
            return false;
        }
        node = node->mpParent;
    }
    return true;
}

template <class T>
bool Tree<T>::shouldBeActive() const
{
    return mUserActiveState;
}

template <class T>
void Tree<T>::setShouldBeActive(bool b)
{
    mUserActiveState = b;
}

template <class T>
bool Tree<T>::setIsActive(bool b)
{
    setShouldBeActive(b);
    if (b) {
        return activate();
    } else {
        mActive = false;
        return false;
    }
}

template <class T>
inline bool Tree<T>::state() const
{
    return (mOK_init && mOK_code);
}

template <class T>
inline bool Tree<T>::canBeActivated() const
{
    return (shouldBeActive() && state());
}

template <class T>
bool Tree<T>::activate()
{
    if (canBeActivated()) {
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
bool Tree<T>::isActive() const
{
    return (mActive && canBeActivated());
}

template <class T>
void Tree<T>::enableFamily()
{
    activate();
    for (auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++) {
        (*it)->enableFamily();
    }
}

template <class T>
void Tree<T>::disableFamily()
{
    deactivate();
    for (auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++) {
        (*it)->disableFamily();
    }
}

template <class T>
void Tree<T>::addChild(T* newChild, int parentPosition, int childPosition)
{
    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mpMyChildrenList->size()))) {
        mpMyChildrenList->push_back(newChild);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++) {
            if (cnt >= childPosition) {
                mpMyChildrenList->insert(it, newChild);
                break;
            }
            cnt++;
        }
    }
}

template <class T>
void Tree<T>::setParent(T* pParent)
{
    mpParent = pParent;
}

template <class T>
bool Tree<T>::popChild(T* pChild)
{
    for (auto it = mpMyChildrenList->begin(); it != mpMyChildrenList->end(); it++) {
        if (*it == pChild) {
            mpMyChildrenList->remove(pChild);
            return true;
        }
    }
    return false;
}

template <class T>
std::list<T*>* Tree<T>::getChildrenList() const
{
    return mpMyChildrenList;
}

template <class T>
QString& Tree<T>::getError()
{
    return mErrorMessage;
}

template <class T>
void Tree<T>::setError(QString error)
{
    mErrorMessage = error;
}

#endif // MUDLET_TREE_H
