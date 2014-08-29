#ifndef MUDLET_LUAINTERFACE_H
#define MUDLET_LUAINTERFACE_H

/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
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


#include "TVar.h"

#include "pre_guard.h"
#include <QSet>
#include "post_guard.h"

#ifndef LUA_CPP
extern "C"
{
#endif
#include <lua.h>
#ifndef LUA_CPP
}
#endif


class Host;
class TLuaInterpreter;
class VarUnit;

class QTreeWidgetItem;


class LuaInterface
{
public:
    LuaInterface( Host * );
    void iterateTable(lua_State *, int, TVar *, bool );
    void getVars( bool );
    QStringList varName(TVar * var);
    QList<TVar *> varOrder(TVar * var);
    QString getValue( TVar * );
    bool loadKey(lua_State *, TVar *);
    bool loadValue(lua_State *, TVar *, int);
    bool setCValue( QList<TVar *> );
    bool setValue( TVar * );
    void deleteVar( TVar * );
    void renameCVar( QList<TVar *> );
    void renameVar( TVar * );
    void createVar( TVar * );
    VarUnit * getVarUnit();
    bool loadVar( TVar* var );
    bool reparentCVariable(TVar * from , TVar * to, TVar * curVar);
    bool reparentVariable( QTreeWidgetItem *, QTreeWidgetItem *, QTreeWidgetItem * );
    bool validMove( QTreeWidgetItem * );
    void getAllChildren( TVar * var, QList<TVar *> * list);
    static int onPanic( lua_State * );

private:
    Host * mpHost;
    int mHostID;
    int depth;
    TLuaInterpreter *interpreter;
    lua_State *L;
    QSet<TVar> hiddenVars;
    VarUnit * varUnit;
    QList<int> lrefs;
};

#endif // MUDLET_LUAINTERFACE_H
