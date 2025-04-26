#ifndef MUDLET_TVAR_H
#define MUDLET_TVAR_H

/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2022 by Stephen Lyons - slysven@virginmedia.com   *
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
#include <QDebug>
#include <QList>
#include <QString>
#include "post_guard.h"

extern "C" {
    #include <lua.h>
}

class TVar
{
public:
    TVar();
    explicit TVar(TVar*);
    TVar(TVar*, const QString&, int, const QString&, int);
    void addChild(TVar*);
    void setParent(TVar*);
    void removeChild(TVar*);
    bool setValue(const QString&);
    bool setValue(const QString&, int);
    bool setValueType(int);
    bool setName(const QString&);
    bool setName(const QString&, int);
    void setNewName(const QString&, int);
    void setReference(bool);
    QList<TVar*> getChildren(bool isToSort = true);
    TVar* getParent();
    QString getValue() const;
    QString getName() const;
    QString getNewName() const;
    void clearNewName();
    int getKeyType() const;
    int getNewKeyType() const;
    int getValueType() const;
    bool isReference();

public:
    bool hidden;
    const void* pKey;
    const void* pValue;
    bool saved;

private:
    bool reference;
    QList<TVar*> children;
    TVar* parent;
    QString name;
    int keyType;
    QString value;
    int valueType;
    int newKeyType;
    QString nName;
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const TVar* var)
{
  QDebugStateSaver saver(debug);
  Q_UNUSED(saver)
  debug.nospace() << "TVar(" << var->getName() << ")";

  switch (var->getKeyType()) {
    case LUA_TNONE: debug.nospace() << ", keyType=none"; break;
    case LUA_TNIL: debug.nospace() << ", keyType=nil"; break;
    case LUA_TBOOLEAN: debug.nospace() << ", keyType=boolean"; break;
    case LUA_TLIGHTUSERDATA: debug.nospace() << ", keyType=lightuserdata"; break;
    case LUA_TNUMBER: debug.nospace() << ", keyType=number"; break;
    case LUA_TSTRING: debug.nospace() << ", keyType=string"; break;
    case LUA_TTABLE: debug.nospace() << ", keyType=table"; break;
    case LUA_TFUNCTION: debug.nospace() << ", keyType=function"; break;
    case LUA_TUSERDATA: debug.nospace() << ", keyType=userdata"; break;
    case LUA_TTHREAD: debug.nospace() << ", keyType=thread"; break;
  }

  switch (var->getValueType()) {
    case LUA_TNONE: debug.nospace() << ", valueType=none"; break;
    case LUA_TNIL: debug.nospace() << ", valueType=nil"; break;
    case LUA_TBOOLEAN: debug.nospace() << ", valueType=boolean"; break;
    case LUA_TLIGHTUSERDATA: debug.nospace() << ", valueType=lightuserdata"; break;  
    case LUA_TNUMBER: debug.nospace() << ", valueType=number"; break;
    case LUA_TSTRING: debug.nospace() << ", valueType=string"; break;
    case LUA_TTABLE: debug.nospace() << ", valueType=table"; break;
    case LUA_TFUNCTION: debug.nospace() << ", valueType=function"; break;
    case LUA_TUSERDATA: debug.nospace() << ", valueType=userdata"; break;
    case LUA_TTHREAD: debug.nospace() << ", valueType=thread"; break;
  }

  debug.nospace() << " value: " << var->getValue();

  return debug;
}
#endif // QT_NO_DEBUG_STREAM



#endif // MUDLET_TVAR_H
