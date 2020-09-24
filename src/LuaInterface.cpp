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


#include "LuaInterface.h"


#include "Host.h"
#include "VarUnit.h"

#include <csetjmp>


static jmp_buf buf;

LuaInterface::LuaInterface(Host* pH) : mpHost(pH), L(), depth()
{
    interpreter = mpHost->getLuaInterpreter();
    mHostID = mpHost->getHostID();
    varUnit.reset(new VarUnit());
    //set our panic function
    lua_atpanic(interpreter->pGlobalLua, &onPanic);
}

LuaInterface::~LuaInterface() = default;

int LuaInterface::onPanic(lua_State* L)
{
    QString error = "Lua Panic, No error information";
    if (lua_isstring(L, -1)) {
        error = QString::fromUtf8(lua_tostring(L, -1));
        //there's never anything but the error on the stack, nothing to report
    }
    //FIXME: report error to user qDebug()<<"PANIC ERROR:"<<error;
    longjmp(buf, 1);
    return 1;
}

VarUnit* LuaInterface::getVarUnit()
{
    return varUnit.data();
}

QStringList LuaInterface::varName(TVar* var)
{
    QStringList names;
    if (var->getName() == "_G") {
        names << "";
        return names;
    }
    names << var->getName();
    TVar* p = var->getParent();
    while (p && p->getName() != "_G") {
        names.insert(0, p->getName());
        p = p->getParent();
    }
    return names;
}

bool LuaInterface::validMove(QTreeWidgetItem* p)
{
    TVar* newParent = varUnit->getWVar(p);
    if (newParent && newParent->getValueType() != LUA_TTABLE) {
        return false;
    }
    return true;
}

void LuaInterface::getAllChildren(TVar* var, QList<TVar*>* list)
{
    QListIterator<TVar*> it(var->getChildren(true));
    if (varUnit->isSaved(var) || var->saved) {
        list->append(var);
    }
    while (it.hasNext()) {
        TVar* child = it.next();
        if (child->getValueType() == LUA_TTABLE) {
            getAllChildren(child, list);
        } else if (varUnit->isSaved(child) || var->saved) {
            list->append(child);
        }
    }
}

bool LuaInterface::loadKey(lua_State* L, TVar* var)
{
    if (setjmp(buf) == 0) {
        int kType = var->getKeyType();
        if (var->isReference()) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            if (kType == LUA_TNUMBER) {
                lua_pushnumber(L, var->getName().toInt());
            } else if (kType == LUA_TTABLE) {
            } else if (kType == LUA_TBOOLEAN) {
                lua_pushboolean(L, var->getName().toLower() == "true" ? 1 : 0);
            } else {
                lua_pushstring(L, var->getName().toUtf8().constData());
            }
        }
        return lua_type(L, -1) == kType;
    }
    return false;
}

bool LuaInterface::loadValue(lua_State* L, TVar* var, int index)
{
    //puts a value on stack
    if (setjmp(buf) == 0) {
        if (loadKey(L, var)) {
            //everything is tabled in lua, we need to just find what table
            //we're using, if index == 0, we iterate to the closest table
            if (index) {
                lua_gettable(L, index);
            } else {
                for (int j = 1; j <= lua_gettop(L); j++) {
                    if (lua_type(L, j * -1) == LUA_TTABLE) {
                        lua_gettable(L, j * -1);
                    }
                }
            }
        } else {
            return false;
        }
        if (lua_gettop(L)) {
            return lua_type(L, -1) == var->getValueType();
        }
        return false;
    }
    return false;
}

bool LuaInterface::reparentCVariable(TVar* from, TVar* to, TVar* curVar)
{
    //get the old parent on the stack
    if (setjmp(buf) == 0) {
        if (!from || !to || (from == to)) {
            // moving from global to global or nowhere
            return true;
        }
        int stackSize = lua_gettop(L);
        bool isSaved = varUnit->isSaved(curVar);
        if (isSaved) {
            QList<TVar*> list;
            getAllChildren(curVar, &list);
            QListIterator<TVar*> it(list);
            while (it.hasNext()) {
                TVar* t = it.next();
                varUnit->removeSavedVar(t);
            }
        }
        QList<TVar*> vars = varOrder(curVar);
        lua_getglobal(L, (vars[0]->getName()).toUtf8().constData());
        int i = 1;
        for (; i < vars.size(); i++) {
            if (!loadValue(L, vars[i], -2)) {
                lua_settop(L, stackSize);
                return false;
            }
        }
        //redo the parenting in TVar
        from->removeChild(curVar);
        curVar->setParent(to);
        to->addChild(curVar);
        vars = varOrder(curVar);
        //do the actual reparenting part
        if (to == varUnit->getBase()) {
            //we're going global
            lua_setglobal(L, curVar->getName().toUtf8().constData());
        } else {
            lua_getglobal(L, (vars[0]->getName()).toUtf8().constData());
            i = 1;
            for (; i < vars.size() - 1; i++) {
                if (!loadValue(L, vars[i], -2)) {
                    lua_settop(L, stackSize);
                    return false;
                }
                lua_remove(L, -2);
            }
            lua_insert(L, -2);
            if (!loadKey(L, curVar)) {
                lua_settop(L, stackSize);
                return false;
            }
            lua_insert(L, -2);
            if (!lua_istable(L, -3)) {
                lua_settop(L, stackSize);
                return false;
            }
            lua_settable(L, -3);
            lua_pop(L, 1);
        }
        //delete the old copy
        if (from == varUnit->getBase()) {
            lua_pushnil(L);
            lua_setglobal(L, curVar->getName().toUtf8().constData());
        } else {
            if (!loadKey(L, curVar)) {
                lua_settop(L, stackSize);
                return false;
            }
            lua_pushnil(L);
            if (!lua_istable(L, -3)) {
                lua_settop(L, stackSize);
                return false;
            }
            lua_settable(L, -3);
        }
        if (isSaved) {
            QList<TVar*> list;
            list.append(to);
            getAllChildren(curVar, &list);
            QListIterator<TVar*> it(list);
            while (it.hasNext()) {
                TVar* t = it.next();
                varUnit->addSavedVar(t);
            }
        }
        lua_settop(L, stackSize);
        return true;
    }
    return false;
}

bool LuaInterface::reparentVariable(QTreeWidgetItem* newP, QTreeWidgetItem* cItem, QTreeWidgetItem* oldP)
{
    //if oldParent doesn't exist:
    //this means we were moved to a table from the global namespace
    //if newParent doesn't exist:
    //we were moved to the global namespace
    //if both exist:
    //this means we were moved from inside a table to inside another table
    //and in both instances, this table was not _G
    TVar* curVar = varUnit->getWVar(cItem);
    if (!curVar) {
        return false;
    }

    L = interpreter->pGlobalLua;
    TVar* newParent = varUnit->getWVar(newP);
    TVar* oldParent = varUnit->getWVar(oldP);
    TVar* from = oldParent;
    TVar* to = newParent;
    if (newParent && newParent->getValueType() != LUA_TTABLE) {
        //FIXME: report why this fails to user
        return false;
    }

    if (!newParent && !oldParent) {
        //happens when we move from _G to _G
        return false;
    }

    if (!oldParent) {
        from = varUnit->getBase();
        // newParent cannot be a nullptr here as we would have returned in
        // previous if - so to won't be either:
        to = newParent;
    } else if (!newParent) {
        // oldParent cannot be a nullptr here as we would have returned in
        // previous if - so from won't be either:
        from = oldParent;
        to = varUnit->getBase();
    }

    // one of from and to must not be a nullptr here - so prior test for BOTH
    // being a nullptr here and returning false in that case was dead code.

    return reparentCVariable(from, to, curVar);
}

QList<TVar*> LuaInterface::varOrder(TVar* var)
{
    QList<TVar*> vars;
    if (var->getName() == "_G") {
        return vars;
    }
    vars << var;
    TVar* p = var->getParent();
    while (p && p->getName() != "_G") {
        vars.insert(0, p);
        p = p->getParent();
    }
    return vars;
}

void LuaInterface::createVar(TVar* var)
{
    setValue(var);
}

bool LuaInterface::setCValue(QList<TVar*> vars)
{
    //make the new stack
    TVar* var = vars.back();
    if (setjmp(buf) == 0) {
        int stackSize = lua_gettop(L);
        lua_getglobal(L, (vars[0]->getName()).toUtf8().constData());
        int i = 1;
        for (; i < vars.size() - 1; i++) {
            if (!loadValue(L, vars[i], -2)) {
                lua_settop(L, stackSize);
                return false;
            }
        }
        //push our value onto the stack
        switch (var->getValueType()) {
        case LUA_TSTRING:
            lua_pushstring(L, var->getValue().toUtf8().constData());
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
            lua_settop(L, stackSize);
            return false;
        }
        //set it up
        if (lua_type(L, -1) != var->getValueType()) {
            lua_settop(L, stackSize);
            return false;
        }
        lua_settable(L, -3);
    }
    return false;
}

// sets the value of a Lua variable by running dynamically-generated Lua code
bool LuaInterface::setValue(TVar* var)
{
    //This function assumes the var has been modified and then called
    L = interpreter->pGlobalLua;

    QList<TVar*> vars = varOrder(var);
    QString variableChangeCode = vars[0]->getName();
    for (int i = 1; i < vars.size(); i++) {
        if (vars[i]->isReference()) {
            return setCValue(vars);
        }
        if (vars[i]->getKeyType() == LUA_TNUMBER) {
            variableChangeCode.append(QStringLiteral("[%1]").arg(vars.at(i)->getName()));
        } else {
            variableChangeCode.append(QStringLiteral(R"(["%1"])").arg(vars.at(i)->getName()));
        }
    }
    switch (var->getValueType()) {
    case LUA_TSTRING:
        variableChangeCode.append(QStringLiteral(" = [[%1]]").arg(var->getValue()));
        break;
    case LUA_TNUMBER:
        variableChangeCode.append(QStringLiteral(" = %1").arg(var->getValue()));
        break;
    case LUA_TBOOLEAN:
        variableChangeCode.append(QStringLiteral(" = %1").arg(var->getValue()));
        break;
    case LUA_TTABLE:
        variableChangeCode.append(QLatin1String(" = {}"));
        break;
    default:
        return false;
    }
    int error = luaL_loadstring(L, variableChangeCode.toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::setValue(...) WARNING - Internal Lua (parsing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << variableChangeCode << "\".";
        return false;
    }
    error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::setValue(...) WARNING - Internal Lua (executing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << variableChangeCode << "\".";
        return false;
    }
    return true;
}

void LuaInterface::deleteVar(TVar* var)
{
    L = interpreter->pGlobalLua;
    QList<TVar*> vars = varOrder(var);
    QString oldName = vars[0]->getName();
    for (int i = 1; i < vars.size(); i++) {
        if (vars[i]->getKeyType() == LUA_TNUMBER) {
            oldName.append(QStringLiteral("[%1]").arg(vars[i]->getName()));
        } else {
            oldName.append(QStringLiteral(R"(["%1"])").arg(vars[i]->getName()));
        }
    }
    //delete it
    oldName.append(QStringLiteral(" = nil"));
    int error = luaL_loadstring(L, oldName.toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::deleteVar(...) WARNING - Internal Lua (parsing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << oldName << "\".";
        return;
    }
    error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::deleteVar(...) WARNING - Internal Lua (executing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << oldName << "\".";
    }
}

void LuaInterface::renameCVar(QList<TVar*> vars)
{
    //uses C Api to rename a variable.
    //dangerous function since you can get an api panic
    //and trash the stack
    L = interpreter->pGlobalLua;
    TVar* var = vars.back();
    //make the new stack
    lua_getglobal(L, (vars[0]->getName()).toUtf8().constData());
    if (setjmp(buf) == 0) {
        int i = 1;
        int pushCount = 0;
        int kType;
        for (; i < vars.size() - 1; i++) {
            kType = vars[i]->getKeyType();
            if (kType == LUA_TNUMBER) {
                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
            } else if (kType == LUA_TTABLE) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
            } else {
                lua_pushstring(L, QString(vars[i]->getName()).toUtf8().constData());
            }
            lua_gettable(L, -2);
            if (lua_isnil(L, -1)) {
                //value didn't exist, make it
                lua_pop(L, -1);
                if (kType == LUA_TNUMBER) {
                    lua_pushnumber(L, QString(vars[i]->getName()).toInt());
                } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
                } else {
                    lua_pushstring(L, QString(vars[i]->getName()).toUtf8().constData());
                }
                lua_newtable(L);
                lua_settable(L, -3);
                i--; //decrement since we want to reput this table on the stack on next iteration
            }
        }

        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(L, QString(var->getNewName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(L, var->getNewName().toInt());
        } else if (kType == LUA_TTABLE) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            //FIXME: report error to user qDebug()<<"unknown key type"<<var->getKeyType();
            return;
        }

        //put the old value on the stack
        lua_getglobal(L, (vars[0]->getName()).toUtf8().constData());
        i = 1;
        for (; i < vars.size() - 1; i++) {
            kType = vars[i]->getKeyType();
            if (kType == LUA_TNUMBER) {
                lua_pushnumber(L, QString(vars[i]->getName()).toInt());
            } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
            } else {
                lua_pushstring(L, QString(vars[i]->getName()).toUtf8().constData());
            }
            lua_gettable(L, -2);
            pushCount++;
        }

        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(L, QString(var->getName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(L, var->getName().toInt());
        } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            //FIXME: report error to user qDebug()<<"unknown key type"<<var->getKeyType();
            return;
        }
        lua_gettable(L, -2);
        pushCount++;
        //old value is @ -1 now
        //we want to put our new named key @ -2
        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(L, QString(var->getNewName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(L, var->getNewName().toInt());
        } else if (kType == LUA_TTABLE) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            //FIXME: report error to userqDebug()<<"unknown key type"<<var->getKeyType();
            return;
        }
        pushCount++;
        lua_insert(L, -2);
        lua_settable(L, -3 - pushCount);
        //key & value popped
        //delete it, so we put the old key back on the stack and set to nil
        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(L, QString(var->getName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(L, var->getName().toInt());
        } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            //FIXME: report error to userqDebug()<<"unknown key type"<<var->getKeyType();
            return;
        }
        lua_pushnil(L);
        lua_settable(L, -3);
        var->clearNewName();
    }
}

bool LuaInterface::loadVar(TVar* var)
{
    //puts the value of a variable on the -1 position of the stack
    if (setjmp(buf) == 0) {
        L = interpreter->pGlobalLua;
        int kType = var->getKeyType();
        int vType = var->getValueType();
        if (vType == LUA_TTABLE) {
            if (kType == LUA_TNUMBER) {
                lua_pushnumber(L, QString(var->getName()).toInt());
            } else if (kType == LUA_TTABLE) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
            } else {
                lua_pushstring(L, QString(var->getName()).toUtf8().constData());
            }
            if (lua_istable(L, -2)) {
                lua_gettable(L, -2);
                return true;
            } else {
                lua_pop(L, 1);
                return false;
            }
        } else if (vType == LUA_TNUMBER) {
            lua_pushnumber(L, QString(var->getValue()).toInt());
        } else if (vType == LUA_TBOOLEAN) {
            lua_pushboolean(L, var->getValue().toLower() == "true" ? 1 : 0);
        } else if (vType == LUA_TSTRING) {
            lua_pushstring(L, QString(var->getName()).toUtf8().constData());
        } else {
            return false;
        }
    } else {
        //FIXME: report error to user qDebug()<<"panic in loadVar";
        return false;
    }
    return true;
}

void LuaInterface::renameVar(TVar* var)
{
    //this assumes anything like reparenting has been done
    L = interpreter->pGlobalLua;
    QList<TVar*> vars = varOrder(var);
    QString oldVariable = vars.at(0)->getName();
    QString newName;
    if (vars.size() > 1) {
        newName = vars[0]->getName();
    }

    for (int i = 1; i < vars.size(); i++) {
        int kType = vars[i]->getKeyType();
        if (kType == LUA_TNUMBER) {
            oldVariable.append(QStringLiteral("[%1]").arg(vars.at(i)->getName()));
            if (i < vars.size() - 1) {
                newName.append(QStringLiteral("[%1]").arg(vars[i]->getName()));
            }

        } else if (kType == LUA_TTABLE) {
            renameCVar(vars);
            return;
        }

        // That leaves LUA_TSTRING:
        oldVariable.append(QStringLiteral(R"(["%1"])").arg(vars.at(i)->getName()));
        if (i < vars.size() - 1) {
            newName.append(QStringLiteral(R"(["%1"])").arg(vars.at(i)->getName()));
        }
    }

    if (vars.size() <= 1) {
        // this variable is at root level on _G
        newName.append(QStringLiteral("_G[\"%1\"]").arg(vars.last()->getNewName()));
    } else {
        // this variable is nested in a table
        if (var->getNewKeyType() == LUA_TNUMBER) {
            newName.append(QStringLiteral("[%1]").arg(vars.last()->getNewName()));
        } else {
            newName.append(QStringLiteral(R"(["%1"])").arg(vars.last()->getNewName()));
        }
    }

    auto renameCode = QStringLiteral("%1 = %2").arg(newName, oldVariable);
    int error = luaL_loadstring(L, renameCode.toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In copying (first) stage, internal Lua (parsing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << renameCode << "\".";
        var->clearNewName();
        return;
    }
    error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In copying (first) stage, internal Lua (executing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << renameCode << "\".";
        var->clearNewName();
        return;
    }

    //delete it
    error = luaL_loadstring(L, oldVariable.append(QLatin1String(" = nil")).toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In deleting (second) stage, internal Lua (parsing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << renameCode << "\".";
        var->clearNewName();
        return;
    }
    error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In deleting (second) stage, internal Lua (executing) error: \""
                                       << lua_tostring(L, -1)
                                       << "\" in code:\n\""
                                       << renameCode << "\".";
    }
    var->clearNewName();
}

QString LuaInterface::getValue(TVar* var)
{
    if (setjmp(buf) == 0) {
        L = interpreter->pGlobalLua;
        QList<TVar*> vars = varOrder(var);
        if (vars.empty()) {
            return QString();
        }
        int pCount = vars.size(); //how many things we need to pop at the end
        //load from _G first
        lua_getglobal(L, (vars[0]->getName()).toUtf8().constData());
        for (int i = 1; i < vars.size(); i++) {
            if (!loadValue(L, vars[i], -2)) {
                return QString();
            }
        }
        int vType = lua_type(L, -1);
        QString value;
        if (vType == LUA_TBOOLEAN) {
            value = lua_toboolean(L, -1) == 0 ? QLatin1String("false") : QLatin1String("true");
        } else if (vType == LUA_TNUMBER || vType == LUA_TSTRING) {
            value = QString::fromUtf8(lua_tostring(L, -1));
        }
        lua_pop(L, pCount);
        return value;
    }
    return QString();
}

void LuaInterface::iterateTable(lua_State* L, int index, TVar* tVar, bool hide)
{
    depth++;
    while (lua_next(L, index)) {
        int vType = lua_type(L, -1);
        int kType = lua_type(L, -2);
        lua_pushvalue(L, -2); //we do this because extracting the key with tostring changes it
        QString keyName;
        QString valueName;
        auto var = new TVar();
        if (kType == LUA_TTABLE) {
            keyName = QString::number(luaL_ref(L, LUA_REGISTRYINDEX)); //this function pops the top item
            lrefs.append(keyName.toInt());
            var->setReference(true);
        } else {
            keyName = QString::fromUtf8(lua_tostring(L, -1));
            if (kType == LUA_TFUNCTION && keyName.isEmpty()) {
                //we lost the reference
                keyName = QString::number(luaL_ref(L, LUA_REGISTRYINDEX));
                lrefs.append(keyName.toInt());
                var->setReference(true);
            } else {
                lua_pop(L, 1);
            }
        }
        if (keyName == "package" && depth == 1) { //don't load in the 'package' table
            lua_pop(L, 1);
            tVar->removeChild(var);
            delete var;
            continue;
        }
        var->setName(keyName, kType);
        var->setValueType(vType);
        var->setParent(tVar);
        var->hidden = hide;
        tVar->addChild(var);
        const void* kp = lua_topointer(L, -1);
        var->kpointer = kp;
        const void* vp = lua_topointer(L, -2);
        var->vpointer = vp;
        if (varUnit->varExists(var) || keyName == "_G") {
            lua_pop(L, 1);
            tVar->removeChild(var);
            delete var;
            continue;
        }
        varUnit->addVariable(var);

        varUnit->addPointer(kp);

        varUnit->addPointer(vp);
        if (vType == LUA_TTABLE) {
            if (depth <= 99 && lua_checkstack(L, 3)) { //depth is historical now
                //put the table on top
                lua_pushnil(L);
                var->setValue("{}", LUA_TTABLE);
                iterateTable(L, -2, var, hide);
                depth--;
            }
        } else if (vType == LUA_TSTRING || vType == LUA_TNUMBER) {
            lua_pushvalue(L, -1);
            valueName = QString::fromUtf8(lua_tostring(L, -1));
            var->setValue(valueName);
            lua_pop(L, 1);
        } else if (vType == LUA_TBOOLEAN) {
            valueName = lua_toboolean(L, -1) == 0 ? "false" : "true";
            var->setValue(valueName);
        } else if (vType == LUA_TFUNCTION
                   && (!keyName.toLower().startsWith("alias") && !keyName.toLower().startsWith("trigger") && !keyName.toLower().startsWith("action") && !keyName.toLower().startsWith("timer")
                       && !keyName.toLower().startsWith("key"))) {
            //functions are compiled to bytecode so there is no reference
            var->setValue("function");
        } else {
            tVar->removeChild(var);
            varUnit->removeVariable(var);
            delete var;
        }
        lua_pop(L, 1);
    }
}

void LuaInterface::getVars(bool hide)
{
    //returns the base item
    QTime t;
    t.start();
    L = interpreter->pGlobalLua;
    lua_pushnil(L);
    depth = 0;
    auto g = new TVar();
    g->setName("_G", LUA_TSTRING);
    g->setValue("{}", LUA_TTABLE);
    QListIterator<int> it(lrefs);
    while (it.hasNext()) {
        int ref = it.next();
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
    }
    varUnit->clear();
    varUnit->setBase(g);
    varUnit->addVariable(g);
    iterateTable(L, LUA_GLOBALSINDEX, g, hide);
    //FIXME: possible to keep and report? qDebug()<<"took"<<t.elapsed()<<"to get variables in";
}
