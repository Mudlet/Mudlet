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
    bool setValue( TVar * );
    void deleteVar( TVar * );
    void renameVar( TVar * );
    void createVar( TVar * );
    VarUnit * getVarUnit();
    bool reparentVariable( QTreeWidgetItem *, QTreeWidgetItem *, QTreeWidgetItem * );
    bool validMove( QTreeWidgetItem * );
    void getAllChildren( TVar * var, QList<TVar *> * list);
private:
    Host * mpHost;
    int mHostID;
    int depth;
    TLuaInterpreter *interpreter;
    lua_State *L;
    QSet<TVar> hiddenVars;
    VarUnit * varUnit;
};

#endif // LUAINTERFACE_H
