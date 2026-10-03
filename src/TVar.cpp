/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "TVar.h"

/*
 * LUA type values from lua.h for lua 5.1:
 * LUA_TNONE          (-1)
 * LUA_TNIL             0
 * LUA_TBOOLEAN         1
 * LUA_TLIGHTUSERDATA   2
 * LUA_TNUMBER          3
 * LUA_TSTRING          4
 * LUA_TTABLE           5
 * LUA_TFUNCTION        6
 * LUA_TUSERDATA        7
 * LUA_TTHREAD          8
 */

TVar::TVar() {}

TVar::TVar(TVar* p)
: parent(p)
{
}

TVar::TVar(TVar* p, const QString& kName, const int kt, const QString& val, const int vt)
: parent(p)
, name(kName)
, keyType(kt)
, value(val)
, valueType(vt)
{
}

TVar::~TVar()
{
    // Delete all children recursively
    qDeleteAll(children);
    children.clear();
}

void TVar::setReference(const bool s)
{
    reference = s;
}

void TVar::addChild(TVar* c)
{
    children.append(c);
}

QString TVar::getName() const
{
    return name;
}

// std::sort() may walk off the ends of the range it is given unless this is a
// strict weak ordering, so every pair of names has to be placed the same way
// whichever other names are around. Only the out-parameter of toInt() says
// whether a name is a number: its return value cannot, since "0" and a name
// that is no number at all both convert to zero. Deciding it by the value put
// the names into groups that contradicted each other - "2" < "10" < "11a" <
// "2" was a cycle a table of mixed names really produced (#9956).
bool TVarLessThan(TVar* varA, TVar* varB)
{
    const QString a = varA->getName();
    const QString b = varB->getName();
    bool isANumber = false;
    bool isBNumber = false;
    const int aNumber = a.toInt(&isANumber);
    const int bNumber = b.toInt(&isBNumber);

    if (isANumber != isBNumber) {
        // Numbers ahead of names. Which way round is arbitrary - what matters
        // is that it is the same way round for every such pair, so that the two
        // kinds of name form two blocks rather than interleaving by whatever
        // else is in the table.
        return isANumber;
    }
    if (isANumber) {
        return aNumber < bNumber;
    }
    const QString aFolded = a.toLower();
    const QString bFolded = b.toLower();
    if (aFolded != bFolded) {
        return aFolded < bFolded;
    }
    // "A" and "a" fold together, and leaving them equivalent would leave their
    // order down to whatever the sort happened to do with them
    return a < b;
}

QList<TVar*> TVar::getChildren(const bool isToSort)
{
    if (isToSort && children.count() > 1) {
        std::sort(children.begin(), children.end(), TVarLessThan);
    }
    return children;
}

bool TVar::isReference()
{
    return reference;
}

void TVar::setParent(TVar* t)
{
    parent = t;
}

void TVar::removeChild(TVar* t)
{
    children.removeAll(t);
}

int TVar::getKeyType() const
{
    return keyType;
}

QString TVar::getValue() const
{
    return value;
}

int TVar::getValueType() const
{
    return valueType;
}

void TVar::setNewName(const QString& n, const int t)
{
    nName = n;
    newKeyType = t;
}

int TVar::getNewKeyType() const
{
    return newKeyType;
}

QString TVar::getNewName() const
{
    return nName;
}

// Commits a rename that has happened: the variable now answers to the name it
// was renamed to, so this node has to as well. Only call it once Lua holds the
// variable under that name - see abandonNewName() for the other outcome.
void TVar::clearNewName()
{
    name = nName;
    keyType = newKeyType;
    nName = QString();
    newKeyType = LUA_TNIL; // CHECK: Was 0 but perhaps it should have been -1 (LUA_TNONE ?)
}

// Drops a rename that is not going to happen, leaving the node naming the
// variable Lua still has. clearNewName() was used for this too, which put the
// refused name onto the node: every later read and write then went looking for
// a variable of that name - and if the rename was refused because a sibling
// already had it, that sibling is what they would have found.
void TVar::abandonNewName()
{
    nName = QString();
    newKeyType = LUA_TNIL;
}

bool TVar::setValue(const QString& val)
{
    value = val;
    return true;
}

bool TVar::setValue(const QString& val, const int t)
{
    value = val;
    valueType = t;
    return true;
}

bool TVar::setValueType(const int t)
{
    valueType = t;
    return true;
}

bool TVar::setName(const QString& n, const int kt)
{
    name = n;
    keyType = kt;
    return true;
}

bool TVar::setName(const QString& n)
{
    name = n;
    return true;
}

TVar* TVar::getParent()
{
    return parent;
}
