#ifndef LUAINTERFACE_H
#define LUAINTERFACE_H

#include "TLuaInterpreter.h"
#include "TTreeWidget.h"
#include "Host.h"
#include <QSet>
#include <QTreeWidgetItem>
#include <setjmp.h>
#include "TVar.h"
#include "VarUnit.h"

extern "C"
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}
class TLuaInterpreter;
class Host;

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

#endif // LUAINTERFACE_H
