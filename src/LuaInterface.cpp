#include "LuaInterface.h"
#include "TVar.h"
#include "VarUnit.h"
#include <QTime>

static jmp_buf buf;

LuaInterface::LuaInterface(Host * pH)
    :mpHost(pH)
{
    interpreter = mpHost->getLuaInterpreter();
    mHostID = mpHost->getHostID();
    varUnit = new VarUnit();
    //set our panic function
    lua_atpanic( interpreter->pGlobalLua, &onPanic );
}

int LuaInterface::onPanic( lua_State * L )
{
    QString error = "Lua Panic, No error information";
    if ( lua_isstring( L, -1 ) )
    {
        error = lua_tostring( L, -1 );
        //there's never anything but the error on the stack
//        qDebug()<<"stack dump:";
//        for (int j=1;j<=lua_gettop(L);j++){
//            int ltype = lua_type(L, j*-1);
//            QString value;
//            if (ltype == LUA_TNUMBER || ltype == LUA_TSTRING ||
//                    ltype == LUA_TBOOLEAN)
//                value = lua_tostring(L, j*-1);
//            qDebug()<<"index"<<j<<"type:"<<ltype<<"value"<<value;
//        }
        //lua_error(L);
    }
    qDebug()<<"PANIC ERROR:"<<error;
    longjmp(buf, 1);
    return 1;
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

bool LuaInterface::validMove(QTreeWidgetItem * p){
    TVar * newParent = varUnit->getWVar(p);
    if (newParent && newParent->getValueType() != LUA_TTABLE)
        return false;
    return true;
}

void LuaInterface::getAllChildren( TVar * var, QList<TVar *> * list){
    QListIterator<TVar *> it(var->getChildren());
    if (varUnit->isSaved(var))
        list->append(var);
    else
        qDebug()<<"t"<<var->getName()<<"not saved";
    while (it.hasNext()){
        TVar * child = it.next();
        if (child->getValueType() == LUA_TTABLE)
            getAllChildren( child, list);
        else if (varUnit->isSaved(child))
            list->append(child);
        else
            qDebug()<<child->getName()<<"not saved";
    }
}

bool LuaInterface::loadKey(lua_State * L, TVar * var){
    if (setjmp(buf) == 0){
        int kType = var->getKeyType();
        qDebug()<<kType;
        if ( kType == LUA_TNUMBER ){
            lua_pushnumber(L, var->getName().toInt());
        }
        else if ( kType == LUA_TTABLE ){
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        }
        else if ( kType == LUA_TBOOLEAN ){
            lua_pushboolean(L, var->getName().toLower() == "true" ? 1 : 0 );
        }
        else{
            lua_pushstring(L, var->getName().toLatin1().data());
        }
        return lua_type(L, -1) == kType;
    }
    qDebug()<<"panic in loadKey";
    return false;
}

bool LuaInterface::loadValue(lua_State * L, TVar * var, int index){
    //puts a value on stack
    if (setjmp(buf) == 0){
        if (loadKey(L, var)){
            //everything is tabled in lua, we need to just find what table
            //we're using, if index == 0, we iterate to the closest table
            if (index)
                lua_gettable(L, index);
            else{
                for (int j=1;j<=lua_gettop(L);j++){
                    if (lua_type(L,j*-1) == LUA_TTABLE){
                        lua_gettable(L, j*-1);
                    }
                }
            }
        }
        else
            return false;
        return lua_type(L, -1) == var->getValueType();
    }
    qDebug()<<"panic error in loadValue";
    return false;
}

//bool LuaInterface::reparentCVariable(TVar * from , TVar * to, TVar * curVar){
//    //get the old parent on the stack
//    if (setjmp(buf) == 0){
//        int startSize = lua_gettop(L);
//        bool isSaved = varUnit->isSaved(curVar);
//        if (isSaved){
//            QList<TVar *> list;
//            getAllChildren(curVar, &list);
//            QListIterator<TVar *> it(list);
//            while (it.hasNext()){
//                TVar * t = it.next();
//                varUnit->removeSavedVar(t);
//            }
//        }
//        qDebug()<<"stack before we start";
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }
//        QList<TVar *> vars = varOrder(curVar);
//        lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
//        int i=1;
//        int kType;
//        for( ; i<vars.size(); i++ ){
//            kType = vars[i]->getKeyType();
//            if ( kType == LUA_TNUMBER ){
//                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
//            }
//            else if ( kType == LUA_TTABLE ){
//                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
//            }
//            else{
//                lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
//            }
//            lua_gettable(L, -2);
//            if (lua_isnil(L, -1))
//                return false;
//        }
//        //at this point we have the value we want to put under
//        //a new parent at the -1 position of the stack, we want
//        //just this value, so we move it to the bottom and pop
//        //everything else
//        lua_insert(L,lua_gettop(L));
//        lua_pop(L, lua_gettop(L)-1);
//        //push the key back
//        kType = curVar->getKeyType();
//        if ( kType == LUA_TNUMBER ){
//            lua_pushnumber(L, QString(curVar->getName()).toInt());
//        }
//        else if ( kType == LUA_TTABLE ){
//            lua_rawgeti(L, LUA_REGISTRYINDEX, curVar->getName().toInt());
//        }
//        else{
//            lua_pushstring(L, QString(curVar->getName()).toLatin1().data());
//        }
//        //swap the key and value positions
//        lua_insert(L, -2);
//        qDebug()<<"old parent with value on stack";
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }

//        //redo the parenting in TVar
//        qDebug()<<"new parent is"<<to->getName();
//        from->removeChild(curVar);
//        curVar->setParent(to);
//        to->addChild(curVar);
//        vars = varOrder(curVar);
//        qDebug()<<"new vars"<<vars;
//        lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
//        i=1;
//        int pushCount=0;
//        for( ; i<vars.size()-1; i++ ){
//            kType = vars[i]->getKeyType();
//            qDebug()<<vars[i]->getName();
//            if ( kType == LUA_TNUMBER ){
//                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
//                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
//            }
//            else if ( kType == LUA_TTABLE ){
//                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
//                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
//            }
//            else{
//                lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
//                lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
//            }
//            lua_gettable(L, -3);
//            pushCount+=1;
//            if (lua_isnil(L, -1)){
//                //value didn't exist, make it
//                lua_pop(L, 2);
//                if ( kType == LUA_TNUMBER ){
//                    lua_pushnumber(L, QString(vars[i]->getName()).toInt());
//                }
//                else if ( kType == LUA_TTABLE ){
//                    lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
//                }
//                else{
//                    lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
//                }
//                lua_newtable(L);
//                lua_settable(L, -3);
//                i--;//decrement since we want to reput this table on the stack on next iteration
//            }
//        }

//        qDebug()<<"new stack fully loaded";
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }
//        qDebug()<<"value reset with pushCount of"<<-3-pushCount*2;
//        //assign the old value to the new value
//        //insert our key/table under the old table
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }
//        lua_insert(L,-3-pushCount*2);//puts the key under our old table
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }
//        lua_insert(L,-3-pushCount*2);//puts the new table under our key
//        for (int j=1;j<=lua_gettop(L);j++){
//            lua_pushvalue(L,j*-1);
//            QString key = lua_tostring(L, -1);
//            lua_pop(L, 1);
//            qDebug()<<j<<":"<<lua_type(L,j*-1)<<key;
//        }
//        lua_insert(L,-3-pushCount*2);//puts the new table under our key
//        qDebug()<<"remove the unneeded entries";
//    //    lua_pop(L, pushCount);
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }
//        lua_settable(L, -3);
//        lua_settable(L, -3);
//        lua_setglobal(L, vars[0]->getName().toLatin1().data());
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }


//        if (isSaved){
//            QList<TVar *> list;
//            list.append(to);
//            getAllChildren(curVar, &list);
//            QListIterator<TVar *> it(list);
//            while (it.hasNext()){
//                TVar * t = it.next();
//                qDebug()<<t->getName();
//                varUnit->addSavedVar(t);
//            }
//        }
//        lua_settop(L, startSize);
//        return true;
//    }
//    else{
//        qDebug()<<"panic in reparentCVariable";
//        return false;
//    }
//}

bool LuaInterface::reparentCVariable(TVar * from , TVar * to, TVar * curVar){
    //get the old parent on the stack
    if (setjmp(buf) == 0){
        if ( !from && !to )//moving from global to global
            return true;
        int stackSize = lua_gettop(L);
        bool isSaved = varUnit->isSaved(curVar);
        if (isSaved){
            QList<TVar *> list;
            getAllChildren(curVar, &list);
            QListIterator<TVar *> it(list);
            while (it.hasNext()){
                TVar * t = it.next();
                varUnit->removeSavedVar(t);
            }
        }
        QList<TVar *> vars = varOrder(curVar);
        lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
        int i=1;
        for( ; i<vars.size(); i++ ){
            if (!loadValue(L, vars[i], -2))
                return false;
        }
//        for (int j=1;j<=lua_gettop(L);j++){
//            qDebug()<<j<<":"<<lua_type(L,j*-1);
//        }

        //redo the parenting in TVar
        from->removeChild(curVar);
        curVar->setParent(to);
        to->addChild(curVar);
        vars = varOrder(curVar);
        //do the actual reparenting part
        if (to == varUnit->getBase()){
            //we're going global
            lua_setglobal(L, curVar->getName().toLatin1().data());
        }
        else{
            lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
            i=1;
            for( ; i<vars.size()-1; i++ ){
                if (!loadValue(L, vars[i], -2))
                    return false;
                lua_remove(L, -2);
            }
//            qDebug()<<"new stack fully loaded";
//            for (int j=1;j<=lua_gettop(L);j++){
//                qDebug()<<j<<":"<<lua_type(L,j*-1);
//            }
            lua_insert(L, -2);
            if (!loadKey(L, curVar))
                return false;
            lua_insert(L, -2);
            lua_settable(L, -3);
            lua_pop(L, 1);
        }
        //delete the old copy
        if (from == varUnit->getBase()){
            lua_pushnil(L);
            lua_setglobal(L, curVar->getName().toLatin1().data());
        }
        else{
            if (!loadKey(L, curVar))
                return false;
            lua_pushnil(L);
            lua_settable(L, -3);
        }
        if (isSaved){
            QList<TVar *> list;
            list.append(to);
            getAllChildren(curVar, &list);
            QListIterator<TVar *> it(list);
            while (it.hasNext()){
                TVar * t = it.next();
                varUnit->addSavedVar(t);
            }
        }
        for (int j=1;j<=lua_gettop(L);j++){
            qDebug()<<j<<":"<<lua_type(L,j*-1);
        }
        lua_settop(L, stackSize);
        return true;
    }
    else{
        qDebug()<<"panic in reparentCVariable";
        return false;
    }
}

bool LuaInterface::reparentVariable(QTreeWidgetItem * newP, QTreeWidgetItem * cItem, QTreeWidgetItem * oldP){
    //if oldParent doesn't exist:
    //this means we were moved to a table from the global namespace
    //if newParent doesn't exist:
    //we were moved to the global namespace
    //if both exist:
    //this means we were moved from inside a table to inside another table
    //and in both instances, this table was not _G
    L = interpreter->pGlobalLua;
    TVar * newParent = varUnit->getWVar(newP);
    TVar * curVar = varUnit->getWVar(cItem);
    TVar * oldParent = varUnit->getWVar(oldP);
    qDebug()<<newParent<<curVar<<oldParent;
    TVar * from = oldParent;
    TVar * to = newParent;
    if ( newParent && newParent->getValueType() != LUA_TTABLE ){
        qDebug()<<"attempt to move to a non-table";
        return false;
    }
    if ( !newParent && !oldParent ){
        //happens when we move from _G to _G
        return false;
    }
    else if ( !oldParent ){
        from = varUnit->getBase();
        to = newParent;
    }
    else if ( !newParent ){
        from = oldParent;
        to = varUnit->getBase();
    }
    if ( !from && !to )
        return false;
    return reparentCVariable( from, to, curVar);
    bool isSaved = varUnit->isSaved(curVar);
    if (isSaved){
        QList<TVar *> list;
        getAllChildren(curVar, &list);
        QListIterator<TVar *> it(list);
        while (it.hasNext()){
            TVar * t = it.next();
            qDebug()<<t->getName();
            varUnit->removeSavedVar(t);
        }
    }
    QList<TVar *> vars = varOrder(curVar);
    QString oldName = vars[0]->getName();
    for(int i=1;i<vars.size();i++){
        if (vars[i]->getKeyType() == LUA_TNUMBER){
            oldName.append("["+vars[i]->getName()+"]");
        }
        else if (vars[i]->getKeyType() == LUA_TTABLE){
            //TODO: reparent c function
            return reparentCVariable( from, to, curVar);
        }
        else{
            oldName.append("[\""+vars[i]->getName()+"\"]");
        }
    }
    from->removeChild(curVar);
    curVar->setParent(to);
    to->addChild(curVar);

    vars = varOrder(curVar);
    QString newName = vars[0]->getName();
    for(int i=1;i<vars.size();i++){
        if (vars[i]->getKeyType() == LUA_TNUMBER){
            newName.append("["+vars[i]->getName()+"]");
        }
        else{
            newName.append("[\""+vars[i]->getName()+"\"]");
        }
    }

    QString addString = QString(newName+" = "+oldName);
    qDebug()<<addString;
    int error = luaL_dostring(L, addString.toLatin1().data());
    qDebug()<<"reparented with"<<error;
    //delete it
    oldName.append(QString(" = nil"));
    luaL_loadstring(L, oldName.toLatin1().data());
    error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error){
        QString emsg = lua_tostring(L, -1);
        qDebug()<<oldName;
        qDebug()<<"error msg"<<emsg;
        return false;
    }
    if (isSaved){
        QList<TVar *> list;
        list.append(to);
        getAllChildren(curVar, &list);
        QListIterator<TVar *> it(list);
        while (it.hasNext()){
            TVar * t = it.next();
            qDebug()<<t->getName();
            varUnit->addSavedVar(t);
        }
    }
    return true;
}

QList<TVar *> LuaInterface::varOrder(TVar * var){
    QList<TVar *> vars;
    if (var->getName() == "_G"){
        return vars;
    }
    vars << var;
    TVar * p = var->getParent();
    while (p && p->getName() != "_G"){
        vars.insert(0,p);
        p = p->getParent();
    }
    return vars;
}

void LuaInterface::createVar( TVar * var ){
    setValue( var );
}

bool LuaInterface::setCValue( QList<TVar *> vars ){
    //make the new stack
    TVar * var = vars.back();
    if (setjmp(buf) == 0){
        lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
        int i=1;
        for( ; i<vars.size(); i++ ){
            int kType = vars[i]->getKeyType();
            if ( kType == LUA_TNUMBER ){
                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
            }
            else if ( kType == LUA_TTABLE || kType == LUA_TFUNCTION){
                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
            }
            else{
                lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
            }
            if (i<vars.size()-1)
                lua_gettable(L, -2);
        }
        //push our value onto the stack
        switch ( var->getValueType() ){
        case LUA_TSTRING:
            lua_pushstring(L, var->getValue().toLatin1().data());
            break;
        case LUA_TNUMBER:
            lua_pushnumber(L, var->getValue().toInt());
            break;
        case LUA_TBOOLEAN:
            lua_pushboolean(L, var->getValue().toLower() == "true" ? 1 : 0);
            break;
        case LUA_TTABLE:
            lua_newtable(L);
            break;
        default:
            return false;
        }
        //set it up
        lua_settable(L, -3);
    }
    else{
        qDebug()<<"panic in setCValue";
        return false;
    }
}

bool LuaInterface::setValue( TVar * var ){
    //This function assumes the var has been modified and then called
    L = interpreter->pGlobalLua;
//    QStringList names = varName( var );
    //if our outer most name is a number, we need to use [] notation
//    QString toDo;

    QList<TVar *> vars = varOrder(var);
    QString newName = vars[0]->getName();
    for(int i=1;i<vars.size();i++){
        if (vars[i]->getKeyType() == LUA_TNUMBER){
            newName.append("["+vars[i]->getName()+"]");
        }
        else if (vars[i]->getKeyType() == LUA_TTABLE){
            return setCValue(vars);
        }
        else{
            newName.append("[\""+vars[i]->getName()+"\"]");
        }
    }
    switch ( var->getValueType() ){
    case LUA_TSTRING:
        newName.append(QString(" = [["+var->getValue()+"]]"));
        break;
    case LUA_TNUMBER:
        newName.append(QString(" = "+var->getValue()));
        break;
    case LUA_TBOOLEAN:
        newName.append(QString(" = "+var->getValue()));
        break;
    case LUA_TTABLE:
        newName.append(QString(" = {}"));
        break;
    default:
        return false;
    }
    luaL_loadstring(L, newName.toLatin1().data());
    qDebug()<<newName;
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error){
        qDebug()<<newName;
        QString emsg = lua_tostring(L, -1);
        qDebug()<<"error msg"<<emsg;
        return false;
    }
    return true;
}

void LuaInterface::deleteVar( TVar * var ){
    L = interpreter->pGlobalLua;
    QList<TVar *> vars = varOrder(var);
    QString oldName = vars[0]->getName();
    for(int i=1;i<vars.size();i++){
        if (vars[i]->getKeyType() == LUA_TNUMBER){
            oldName.append("["+vars[i]->getName()+"]");
        }
        else{
            oldName.append("[\""+vars[i]->getName()+"\"]");
        }
    }
    //delete it
    oldName.append(QString(" = nil"));
    luaL_loadstring(L, oldName.toLatin1().data());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error){
        QString emsg = lua_tostring(L, -1);
        qDebug()<<oldName;
        qDebug()<<"error msg"<<emsg;
    }
}

void LuaInterface::renameCVar( QList<TVar *> vars ){
    //uses C Api to rename a variable.
    //dangerous function since you can get an api panic
    //and trash the stack
    L = interpreter->pGlobalLua;
    TVar * var = vars.back();
    //make the new stack
    lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
    if (setjmp(buf) == 0){
        int i=1;
        int pushCount=0;
        int kType;
        for( ; i<vars.size()-1; i++ ){
            kType = vars[i]->getKeyType();
            if ( kType == LUA_TNUMBER ){
                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
            }
            else if ( kType == LUA_TTABLE ){
                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
            }
            else{
                lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
            }
            lua_gettable(L, -2);
            if (lua_isnil(L, -1)){
                //value didn't exist, make it
                lua_pop(L, -1);
                if ( kType == LUA_TNUMBER ){
                    lua_pushnumber(L, QString(vars[i]->getName()).toInt());
                }
                else if ( kType == LUA_TTABLE || kType == LUA_TFUNCTION){
                    lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
                }
                else{
                    lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
                }
                lua_newtable(L);
                lua_settable(L, -3);
                i--;//decrement since we want to reput this table on the stack on next iteration
            }
        }

        kType = var->getKeyType();
        if ( kType == LUA_TSTRING ){
            lua_pushstring(L, QString(var->getNewName()).toLatin1().data());
        }
        else if ( kType == LUA_TNUMBER ){
            lua_pushnumber(L, var->getNewName().toInt());
        }
        else if ( kType == LUA_TTABLE ){
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        }
        else{
            qDebug()<<"unknown key type"<<var->getKeyType();
            //lua_pop(L,pCount);
            return;
        }
        qDebug()<<"new stack fully loaded";
        for (int j=1;j<=lua_gettop(L);j++){
            qDebug()<<j<<":"<<lua_type(L,j*-1);
        }

        //put the old value on the stack
        lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
        i=1;
        for( ; i<vars.size()-1; i++ ){
            kType = vars[i]->getKeyType();
            if ( kType == LUA_TNUMBER ){
                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
            }
            else if ( kType == LUA_TTABLE || kType == LUA_TFUNCTION){
                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
            }
            else{
                lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
            }
            lua_gettable(L, -2);
            pushCount++;
        }

        kType = var->getKeyType();
        if ( kType == LUA_TSTRING ){
            lua_pushstring(L, QString(var->getName()).toLatin1().data());
        }
        else if ( kType == LUA_TNUMBER ){
            lua_pushnumber(L, var->getName().toInt());
        }
        else if ( kType == LUA_TTABLE || kType == LUA_TFUNCTION ){
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        }
        else{
            qDebug()<<"unknown key type"<<var->getKeyType();
            //lua_pop(L,pCount);
            return;
        }
        lua_gettable(L, -2);
        pushCount++;
        qDebug()<<"old value fully loaded";
        for (int j=1;j<=lua_gettop(L);j++){
            qDebug()<<j<<":"<<lua_type(L,j*-1);
        }
        //old value is @ -1 now
        //we want to put our new named key @ -2
        kType = var->getKeyType();
        if ( kType == LUA_TSTRING ){
            qDebug()<<var->getNewName();
            lua_pushstring(L, QString(var->getNewName()).toLatin1().data());
        }
        else if ( kType == LUA_TNUMBER ){
            lua_pushnumber(L, var->getNewName().toInt());
        }
        else if ( kType == LUA_TTABLE ){
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        }
        else{
            qDebug()<<"unknown key type"<<var->getKeyType();
            //lua_pop(L,pCount);
            return;
        }
        pushCount++;
        lua_insert(L, -2);
        qDebug()<<pushCount;
        qDebug()<<"about to settable with stack to"<<-3-pushCount;
        for (int j=1;j<=lua_gettop(L);j++){
            qDebug()<<j<<":"<<lua_type(L,j*-1);
        }
        lua_settable(L, -3-pushCount);
        //key & value popped
        //delete it, so we put the old key back on the stack and set to nil
        kType = var->getKeyType();
        qDebug()<<var->getName();
        if ( kType == LUA_TSTRING ){
            lua_pushstring(L, QString(var->getName()).toLatin1().data());
        }
        else if ( kType == LUA_TNUMBER ){
            lua_pushnumber(L, var->getName().toInt());
        }
        else if ( kType == LUA_TTABLE || kType == LUA_TFUNCTION ){
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        }
        else{
            qDebug()<<"unknown key type"<<var->getKeyType();
            //lua_pop(L,pCount);
            return;
        }
        lua_pushnil(L);
        for (int j=1;j<=lua_gettop(L);j++){
            qDebug()<<j<<":"<<lua_type(L,j*-1);
        }
        lua_settable(L, -3);
        var->clearNewName();
    }
    else{
        qDebug()<<"panic in renameCVar";
    }

}

bool LuaInterface::loadVar( TVar* var ){
    //puts the value of a variable on the -1 position of the stack
    if (setjmp(buf) == 0){
        L = interpreter->pGlobalLua;
        int kType = var->getKeyType();
        int vType = var->getValueType();
        if (vType == LUA_TTABLE){
            if ( kType == LUA_TNUMBER ){
                lua_pushnumber(L, QString(var->getName()).toInt());
            }
            else if ( kType == LUA_TTABLE ){
                lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
            }
            else{
                lua_pushstring(L, QString(var->getName()).toLatin1().data());
            }
            if (lua_istable(L,-2)){
                lua_gettable(L, -2);
                return true;
            }
            else{
                lua_pop(L,1);
                return false;
            }
        }
        else if ( vType == LUA_TNUMBER )
            lua_pushnumber(L, QString(var->getValue()).toInt());
        else if ( vType == LUA_TBOOLEAN )
            lua_pushboolean(L, var->getValue().toLower() == "true" ? 1 : 0);
        else if ( vType == LUA_TSTRING )
            lua_pushstring(L, QString(var->getName()).toLatin1().data());
        else
            return false;
    }
    else{
        qDebug()<<"panic in loadVar";
        return false;
    }
}

void LuaInterface::renameVar( TVar * var ){
    //this assumes anything like reparenting has been done
    L = interpreter->pGlobalLua;
    QList<TVar *> vars = varOrder(var);
    QString oldName = vars[0]->getName();
    QString newName;
    qDebug()<<vars;
    if (vars.size() > 1)
        newName = vars[0]->getName();
    for(int i=1;i<vars.size();i++){
        int kType = vars[i]->getKeyType();
        if (kType == LUA_TNUMBER){
            oldName.append("["+vars[i]->getName()+"]");
            if (i < vars.size()-1 )
                newName.append("["+vars[i]->getName()+"]");
        }
        else if (kType == LUA_TTABLE){
            renameCVar(vars);
            return;
        }
        else{
            oldName.append("[\""+vars[i]->getName()+"\"]");
            if (i < vars.size()-1 )
                newName.append("[\""+vars[i]->getName()+"\"]");
        }
    }
    if (var->getNewKeyType() == LUA_TNUMBER)
        newName.append("["+vars.back()->getNewName()+"]");
    else
        newName.append("[\""+vars.back()->getNewName()+"\"]");
    QString addString = QString(newName+" = "+oldName);
    qDebug()<<addString;
    luaL_loadstring(L, addString.toLatin1().data());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error){
        QString emsg = lua_tostring(L, -1);
        qDebug()<<"reassigned"<<var->getName()<<"to"<<var->getNewName()<<"with error"<<emsg;
        var->clearNewName();
        return;
    }
    //delete it
    oldName.append(QString(" = nil"));
    luaL_loadstring(L, oldName.toLatin1().data());
    error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error){
        QString emsg = lua_tostring(L, -1);
        qDebug()<<oldName;
        qDebug()<<"error msg"<<emsg;
    }
    else
        qDebug()<<oldName;
    var->clearNewName();
}

//QString LuaInterface::getValue( TVar * var ){
//    //let's find it.
//    L = interpreter->pGlobalLua;
//    QList<TVar *> vars = varOrder(var);
//    //QStringList names = varName(var);
//    if (vars.empty())
//        return "";
//    int pCount = vars.size();//how many things we need to pop at the end
//    lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
//    int i=1;
//    for( ; i<vars.size()-1; i++ ){
////                for (int j=1;j<=lua_gettop(L);j++){
////                    qDebug()<<j<<":"<<lua_type(L,j*-1);
////                }
////                qDebug()<<vars[i]->getName()<<vars[i]->getKeyType();
//        int kType = vars[i]->getKeyType();
//        if ( kType == LUA_TNUMBER ){
//            lua_pushnumber(L, QString(vars[i]->getName()).toInt());
//        }
//        else if ( kType == LUA_TTABLE ){
//            lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
//        }
//        else{
//            lua_pushstring(L, QString(vars[i]->getName()).toLatin1().data());
//        }
//        lua_gettable(L, -2);
//    }
//    if (vars.size()>1){

////                for (int i=1;i<=lua_gettop(L);i++){
////                    qDebug()<<i<<":"<<lua_type(L,i*-1);
////                }
//        int kType = var->getKeyType();
//        if ( kType == LUA_TSTRING ){
//            lua_pushstring(L, QString(var->getName()).toLatin1().data());
//        }
//        else if ( kType == LUA_TNUMBER ){
//            lua_pushnumber(L, var->getName().toInt());
//        }
//        else if ( kType == LUA_TTABLE || kType == LUA_TFUNCTION ){
//            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
//        }
//        else{
//            qDebug()<<"unknown key type"<<var->getKeyType();
//            lua_pop(L,pCount);
//            return "";
//        }
//        lua_gettable(L, -2);
//    }
//    int vType = lua_type(L, -1);
//    QString value = "";
//    if (vType == LUA_TBOOLEAN)
//        value = lua_toboolean(L, -1) == 0 ? "false" : "true";
//    else if (vType == LUA_TNUMBER || vType == LUA_TSTRING)
//        value = lua_tostring(L, -1);
//    lua_pop(L,pCount);
//    return value;
//}

QString LuaInterface::getValue( TVar * var ){
    qDebug()<<"attempting getValue";
    if (setjmp(buf) == 0){
        L = interpreter->pGlobalLua;
        QList<TVar *> vars = varOrder(var);
        if (vars.empty())
            return "";
        int pCount = vars.size();//how many things we need to pop at the end
        //load from _G first
        lua_getglobal(L, (vars[0]->getName()).toLatin1().data());
        for(int i=1; i<vars.size(); i++ ){
            if (!loadValue(L, vars[i], -2))
                return "";
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
    else{
        qDebug()<<"panic in getValue";
    }
    return "";
}

void LuaInterface::iterateTable(lua_State * L, int index, TVar * tVar, bool hide){
    depth++;
    while(lua_next(L, index)){
        int vType = lua_type(L, -1);
        int kType = lua_type(L, -2);
        lua_pushvalue(L, -2);//we do this because looking at the type can change it
        QString keyName;
        QString valueName;
        if ( kType == LUA_TTABLE ){
            keyName = QString::number(luaL_ref(L, LUA_REGISTRYINDEX));
            qDebug()<<"table key"<<keyName;
            lrefs.append(keyName.toInt());
        }
        else{
            keyName = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
//        qDebug()<<"key type/value:"<<kType<<keyName;
        //qDebug()<<"value type:"<<vType;
//        for (int i=1;i<=lua_gettop(L);i++){
//            qDebug()<<i<<":"<<lua_type(L,i*-1);
//        }
        TVar * var = new TVar();
        var->setName(keyName, kType);
        var->setValueType(vType);
        var->setParent(tVar);
        var->hidden = hide;
        tVar->addChild(var);
        if (varUnit->varExists(var)){
            lua_pop(L, 1);
//            qDebug()<<"removing dup"<<keyName;
            tVar->removeChild(var);
            delete var;
            continue;
        }
        varUnit->addVariable(var);
        if (vType == LUA_TTABLE){
            if (depth<=5){
                //put the table on top
                lua_pushnil(L);
                var->setValue("{}", LUA_TTABLE);
//                qDebug()<<"entering table"<<keyName;
//                for (int i=1;i<=lua_gettop(L);i++){
//                    qDebug()<<i<<":"<<lua_type(L,i*-1);
//                }
                iterateTable(L, -2, var, hide);
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
        else if (vType == LUA_TFUNCTION &&
                 (!keyName.toLower().startsWith("alias") && !keyName.toLower().startsWith("trigger"))){
            lua_pushvalue(L,-1);
            valueName = lua_tostring(L,-1);
            var->setValue("function");
            lua_pop(L,1);
        }
        else{
            tVar->removeChild(var);
            varUnit->removeVariable(var);
            delete var;
        }
//        qDebug()<<"var added"<<var->getName();
        lua_pop(L, 1);
    }
}

void LuaInterface::getVars( bool hide ){
    //returns the base item
    QTime t;
    t.start();
    L = interpreter->pGlobalLua;
    lua_pushnil(L);
    depth = 0;
    TVar * g = new TVar();
    g->setName("_G", LUA_TSTRING);
    g->setValue("{}", LUA_TTABLE);
    QListIterator<int> it(lrefs);
    while(it.hasNext()){
        int ref = it.next();
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
    }
    varUnit->clear();
    varUnit->setBase(g);
    varUnit->addVariable(g);
    iterateTable( L, LUA_GLOBALSINDEX, g, hide );
    qDebug()<<"took"<<t.elapsed()<<"to get variables in";
}
