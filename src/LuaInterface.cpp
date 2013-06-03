#include "LuaInterface.h"
#include "TVar.h"
#include "VarUnit.h"
#include <QTime>

LuaInterface::LuaInterface(Host * pH)
    :mpHost(pH)
{
    interpreter = mpHost->getLuaInterpreter();
    mHostID = mpHost->getHostID();
}

VarUnit * LuaInterface::getVarUnit(int host){
    return varMapping[host];
}

void LuaInterface::iterateTable(lua_State * L, int index, TVar * tVar){
    depth++;
    while(lua_next(L, index)){
        int vType = lua_type(L, -1);
        int kType = lua_type(L, -2);
        lua_pushvalue(L, -2);//we do this because looking at the type can change it
        QString keyName = lua_tostring(L, -1);
        QString valueName;
//        qDebug()<<"key type/value:"<<kType<<keyName;
//        qDebug()<<"value type:"<<vType;
//        for (int i=1;i<=lua_gettop(L);i++){
//            qDebug()<<i<<":"<<lua_type(L,i*-1);
//        }
        lua_pop(L, 1);
        TVar * var = new TVar();
        var->setName(keyName, kType);
        var->setValueType(vType);
        VarUnit * vu = varMapping[mHostID];
        var->setParent(tVar);
        tVar->addChild(var);
        if (vu->varExists(var)){
            lua_pop(L, 1);
            tVar->removeChild(var);
            delete var;
            continue;
        }
        vu->addVariable(var);
        if (vType == LUA_TTABLE){
            if (depth<=5){
                //put the table on top
                lua_pushnil(L);
                var->setValue("_table");
                qDebug()<<"entering table"<<keyName;
                for (int i=1;i<=lua_gettop(L);i++){
                    qDebug()<<i<<":"<<lua_type(L,i*-1);
                }
                iterateTable(L, -2, var);
                depth--;
            }
        }
        else if (vType == LUA_TSTRING || vType == LUA_TNUMBER){
            lua_pushvalue(L,-1);
            valueName = lua_tostring(L,-1);
            var->setValue(valueName);
            lua_pop(L,1);
        }
        else if (vType == LUA_TBOOLEAN){
            valueName = lua_toboolean(L, -1) == 0 ? "false" : "true";
            var->setValue(valueName);
        }
        else{

        }
        lua_pop(L, 1);
    }
}

void LuaInterface::getVars(){
    //returns the base item
    QTime t;
    t.start();
    L = interpreter->pGlobalLua;
    lua_pushnil(L);
    depth = 0;
    TVar * g = new TVar();
    g->setName("_G", LUA_TSTRING);
    g->setValue("_table", 5);
    VarUnit * vars = new VarUnit();
    vars->setBase(g);
    vars->addVariable(g);
    if (varMapping.contains(mHostID))
        varMapping[mHostID]->clear();
    varMapping.insert( mHostID, vars );
    iterateTable(L, LUA_GLOBALSINDEX, g);
    qDebug()<<"took"<<t.elapsed()<<"to get variables in";
}
