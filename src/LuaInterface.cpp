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

VarUnit * LuaInterface::getVarUnit(){
    return varUnit;
}

QStringList LuaInterface::varName(TVar * var){
    QStringList names;
    if (var->getName() == "_G"){
        names << "";
        return names;
    }
    names << var->getName();
    TVar * p = var->getParent();
    while (p && p->getName() != "_G"){
        names.insert(0,p->getName());
        p = p->getParent();
    }
    return names;
}

QString LuaInterface::getValue( TVar * var ){
    //let's find it.
    L = interpreter->pGlobalLua;
    QStringList names = varName(var);
    qDebug()<<"trying to find";
    qDebug()<<names;
    int stackSize = lua_gettop(L);
    if (names.empty())
        return "";
    lua_getglobal(L, names[0].toLatin1().data());
    int i=1;
    for( ; i<names.size(); i++ )
    {
        qDebug()<<"grabbing"<<names[i];
        for (int i=1;i<=lua_gettop(L);i++){
            qDebug()<<i<<":"<<lua_type(L,i*-1);
        }
        lua_getfield(L, -1, names[i].toLatin1().data());
        for (int i=1;i<=lua_gettop(L);i++){
            qDebug()<<i<<":"<<lua_type(L,i*-1);
        }
    }
    int vType = lua_type(L, -1);
    QString value = "";
    if (vType == LUA_TBOOLEAN)
        value = lua_toboolean(L, -1) == 0 ? "false" : "true";
    else if (vType == LUA_TNUMBER || vType == LUA_TSTRING)
        value = lua_tostring(L, -1);
    lua_settop(L, stackSize);
    qDebug()<<"our value was"<<value;
    return value;
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
        var->setParent(tVar);
        tVar->addChild(var);
        if (varUnit->varExists(var)){
            lua_pop(L, 1);
            tVar->removeChild(var);
            delete var;
            continue;
        }
        varUnit->addVariable(var);
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
            tVar->removeChild(var);
            varUnit->removeVariable(var);
            delete var;
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
    varUnit = new VarUnit();
    varUnit->setBase(g);
    varUnit->addVariable(g);
    iterateTable(L, LUA_GLOBALSINDEX, g);
    qDebug()<<"took"<<t.elapsed()<<"to get variables in";
}
