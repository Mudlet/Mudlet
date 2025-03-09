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

TVar::TVar()
: hidden(false)
, pKey(nullptr)
, pValue(nullptr)
, saved(false)
, reference(false)
, parent(nullptr)
, name(QString())
, keyType(LUA_TNONE)
, value(QString())
, valueType(LUA_TNONE)
, newKeyType(LUA_TNONE)
, nName(QString())
{
}

TVar::TVar(TVar* p)
: hidden(false)
, pKey(nullptr)
, pValue(nullptr)
, saved(false)
, reference(false)
, parent(p)
, name(QString())
, keyType(LUA_TNONE)
, value(QString())
, valueType(LUA_TNONE)
, newKeyType(LUA_TNONE)
, nName(QString())
{
}

TVar::TVar(TVar* p, const QString& kName, const int kt, const QString& val, const int vt)
: hidden(false)
, pKey(nullptr)
, pValue(nullptr)
, saved(false)
, reference(false)
, parent(p)
, name(kName)
, keyType(kt)
, value(val)
, valueType(vt)
, newKeyType(LUA_TNONE)
, nName(QString())
{
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

bool TVarLessThan(TVar* varA, TVar* varB)
{
    const QString a = varA->getName();
    const QString b = varB->getName();
    bool isAOk = false;
    bool isBOk = false;

    // Previously we do not check the result of a toInt() call on the QStrings
    // but they would happly return a zero value for a QString that can not be
    // converted to a number and then the IF branch would be taken regardless
    // of whether one or both of the QStrings was NOT actually a number
    if (a.toInt(&isAOk) && b.toInt(&isBOk) && isAOk && isBOk) {
        return a.toInt() < b.toInt();
    } else {
        return a.toLower() < b.toLower();
    }
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

void TVar::clearNewName()
{
    name = nName;
    keyType = newKeyType;
    nName = QString();
    newKeyType = LUA_TNIL; // CHECK: Was 0 but perhaps it should have been -1 (LUA_TNONE ?)
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
