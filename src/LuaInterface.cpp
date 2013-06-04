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

void LuaInterface::createVar( TVar * var ){
    setValue( var );
}

bool LuaInterface::setValue( TVar * var ){
    //This function assumes the var has been modified and then called
    L = interpreter->pGlobalLua;
    QString cheat = varName(var).join(".");
    qDebug()<<var->getValueType();
    switch ( var->getValueType() ){
    case LUA_TSTRING:
        cheat.append(QString(" = \""+var->getValue()+"\""));
        break;
    case LUA_TNUMBER:
        cheat.append(QString(" = "+var->getValue()));
        break;
    case LUA_TBOOLEAN:
        cheat.append(QString(" = "+var->getValue()));
        break;
    case LUA_TTABLE:
        cheat.append(QString(" = {}"));
        break;
    default:
        return false;
    }
    qDebug()<<cheat;
    int error = luaL_dostring(L, cheat.toLatin1().data());
    if (error)
        return false;
    return true;
}

void LuaInterface::deleteVar( TVar * var ){
    L = interpreter->pGlobalLua;
    QString cheat = varName(var).join(".");
    cheat.append(QString(" = nil"));
    int error = luaL_dostring(L, cheat.toLatin1().data());
    qDebug()<<"deleted"<<var->getName()<<"with error"<<error;
    varUnit->removeVariable(var);
    delete var;
}

void LuaInterface::renameVar( TVar * var ){
    L = interpreter->pGlobalLua;
    QStringList names = varName(var);
    QString oldName = names.join(".");
    names.removeLast();
    QString toAdd = QString(names.join(".")+"."+var->getNewName());
    QString addString = QString(toAdd+" = "+oldName);
    int error = luaL_dostring(L, addString.toLatin1().data());
    qDebug()<<"reassigned"<<var->getName()<<"to"<<var->getNewName()<<"with error"<<error;
    oldName.append(" = nil");
    error = luaL_dostring(L, oldName.toLatin1().data());
    qDebug()<<"deleted"<<var->getName()<<"with error"<<error;
    var->clearNewName();
    varUnit->removeVariable(var);
}

QString LuaInterface::getValue( TVar * var ){
    //let's find it.
    L = interpreter->pGlobalLua;
    QStringList names = varName(var);
    if (names.empty())
        return "";
    int pCount = names.size();//how many things we need to pop at the end
    lua_getglobal(L, names[0].toLatin1().data());
    int i=1;
    for( ; i<names.size()-1; i++ )
        lua_getfield(L, -1, names[i].toLatin1().data());
    if (names.size()>1){
        if ( var->getKeyType() == LUA_TSTRING ){
            lua_getfield(L, -1, names[i].toLatin1().data());
        }
        else if ( var->getKeyType() == LUA_TNUMBER ){
            lua_pushnumber(L, var->getName().toInt());
            lua_gettable(L, -2);
        }
    //    else if ( var->getKeyType() == LUA_TBOOLEAN ){
    //        lua_pushboolean(L, ( var->getName() == "true" ? 1 : 0 ) );
    //        lua_gettable(L, -2);
    //        pCount+=2;
    //    }
        else{
            qDebug()<<"unknown var type. how the hell did you put that on the stack?"<<var->getKeyType();
            lua_pop(L,pCount-1);
            return "";
        }
    }
    int vType = lua_type(L, -1);
    QString value = "";
    if (vType == LUA_TBOOLEAN)
        value = lua_toboolean(L, -1) == 0 ? "false" : "true";
    else if (vType == LUA_TNUMBER || vType == LUA_TSTRING)
        value = lua_tostring(L, -1);
    lua_pop(L,pCount);
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
                var->setValue("{}");
//                qDebug()<<"entering table"<<keyName;
//                for (int i=1;i<=lua_gettop(L);i++){
//                    qDebug()<<i<<":"<<lua_type(L,i*-1);
//                }
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
    g->setValue("{}", LUA_TTABLE);
    varUnit = new VarUnit();
    varUnit->setBase(g);
    varUnit->addVariable(g);
    iterateTable(L, LUA_GLOBALSINDEX, g);
    qDebug()<<"took"<<t.elapsed()<<"to get variables in";
}
