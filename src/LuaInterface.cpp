/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2020, 2023 by Stephen Lyons - slysven@virginmedia.com   *
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

#include <QDebug>

#include "LuaInterface.h"
#include "VarUnit.h"
#include "utils.h"

#include <csetjmp>

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

static jmp_buf buf;

LuaInterface::LuaInterface(lua_State* L)
: mL(L)
{
    varUnit.reset(new VarUnit());
    //set our panic function
    lua_atpanic(L, &onPanic);
}

// Does not release lrefs: a profile reset closes the lua_State before this
// object is replaced, so unref'ing here would write into a freed state.
LuaInterface::~LuaInterface() = default;

int LuaInterface::onPanic(lua_State* L)
{
    QString error = "Lua Panic, No error information";
    if (lua_isstring(L, -1)) {
        error = lua_tostring(L, -1);
        qDebug() << "Lua panic:" << error;
    }

    longjmp(buf, 1);
    return 1;
}

VarUnit* LuaInterface::getVarUnit()
{
    return varUnit.data();
}

lua_State* LuaInterface::getState() const
{
    return mL;
}

void LuaInterface::releaseVariableReferences()
{
    for (const int ref : std::as_const(lrefs)) {
        luaL_unref(mL, LUA_REGISTRYINDEX, ref);
    }
    lrefs.clear();
}

QStringList LuaInterface::varName(TVar* var)
{
    QStringList names;
    if (var->getName() == qsl("_G")) {
        names << "";
        return names;
    }
    names << var->getName();
    TVar* pParent = var->getParent();
    while (pParent && pParent->getName() != qsl("_G")) {
        names.insert(0, pParent->getName());
        pParent = pParent->getParent();
    }
    return names;
}

std::pair<bool, QString> LuaInterface::validMove(QTreeWidgetItem* pWidget)
{
    TVar* pNewParent = varUnit->getWVar(pWidget);
    if (pNewParent && pNewParent->getValueType() != LUA_TTABLE) {
        //: Error message shown when user tries to drag a variable onto a non-table variable
        return {false, QObject::tr("Cannot move variable here - the target is not a table")};
    }
    return {true, QString()};
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
        const int keyType = var->getKeyType();
        if (var->isReference()) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            if (keyType == LUA_TNUMBER) {
                lua_pushnumber(L, var->getName().toDouble());
            } else if (keyType == LUA_TTABLE) {
            } else if (keyType == LUA_TBOOLEAN) {
                lua_pushboolean(L, var->getName().toLower() == "true" ? 1 : 0);
            } else {
                lua_pushstring(L, var->getName().toUtf8().constData());
            }
        }
        return lua_type(L, -1) == keyType;
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
                // Validate stack before attempting table access
                const int stackTop = lua_gettop(L);
                const int actualIndex = (index < 0) ? stackTop + index + 1 : index;

                if (actualIndex <= 0 || actualIndex > stackTop) {
                    qWarning().noquote().nospace() << "LuaInterface::loadValue() - Invalid stack index " << index << " for variable \"" << var->getName() << "\". Stack size: " << stackTop
                                                   << ", resolved index: " << actualIndex << ".";
                    return false;
                }

                if (!lua_istable(L, index)) {
                    qWarning().noquote().nospace() << "LuaInterface::loadValue() - Value at stack index " << index << " is not a table for variable \"" << var->getName()
                                                   << "\". Got type: " << lua_typename(L, lua_type(L, index)) << ".";
                    return false;
                }

                lua_gettable(L, index);
            } else {
                // Find the closest table on the stack
                bool foundTable = false;
                for (int j = 1; j <= lua_gettop(L); j++) {
                    if (lua_type(L, j * -1) == LUA_TTABLE) {
                        lua_gettable(L, j * -1);
                        foundTable = true;
                        break;
                    }
                }
                if (!foundTable) {
                    qWarning().noquote().nospace() << "LuaInterface::loadValue() - No table found on stack for variable \"" << var->getName() << "\" when index=0. Stack size: " << lua_gettop(L)
                                                   << ".";
                    return false;
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
        const int stackSize = lua_gettop(mL);
        const bool isSaved = varUnit->isSaved(curVar);
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
        lua_getglobal(mL, (vars[0]->getName()).toUtf8().constData());
        int i = 1;
        for (; i < vars.size(); i++) {
            if (!loadValue(mL, vars[i], -2)) {
                lua_settop(mL, stackSize);
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
            lua_setglobal(mL, curVar->getName().toUtf8().constData());
        } else {
            lua_getglobal(mL, (vars[0]->getName()).toUtf8().constData());
            i = 1;
            for (; i < vars.size() - 1; i++) {
                if (!loadValue(mL, vars[i], -2)) {
                    lua_settop(mL, stackSize);
                    return false;
                }
                lua_remove(mL, -2);
            }
            lua_insert(mL, -2);
            if (!loadKey(mL, curVar)) {
                lua_settop(mL, stackSize);
                return false;
            }
            lua_insert(mL, -2);
            if (!lua_istable(mL, -3)) {
                lua_settop(mL, stackSize);
                return false;
            }
            lua_settable(mL, -3);
            lua_pop(mL, 1);
        }
        //delete the old copy
        if (from == varUnit->getBase()) {
            lua_pushnil(mL);
            lua_setglobal(mL, curVar->getName().toUtf8().constData());
        } else {
            if (!loadKey(mL, curVar)) {
                lua_settop(mL, stackSize);
                return false;
            }
            lua_pushnil(mL);
            if (!lua_istable(mL, -3)) {
                lua_settop(mL, stackSize);
                return false;
            }
            lua_settable(mL, -3);
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
        lua_settop(mL, stackSize);
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
    if (var->getName() == qsl("_G")) {
        return vars;
    }
    vars << var;
    TVar* pParent = var->getParent();
    while (pParent && pParent->getName() != qsl("_G")) {
        vars.insert(0, pParent);
        pParent = pParent->getParent();
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
        const int stackSize = lua_gettop(mL);
        lua_getglobal(mL, (vars[0]->getName()).toUtf8().constData());
        int i = 1;
        for (; i < vars.size() - 1; i++) {
            if (!loadValue(mL, vars[i], -2)) {
                lua_settop(mL, stackSize);
                return false;
            }
        }
        //push our value onto the stack
        switch (var->getValueType()) {
        case LUA_TSTRING:
            lua_pushstring(mL, var->getValue().toUtf8().constData());
            break;
        case LUA_TNUMBER:
            lua_pushnumber(mL, var->getValue().toDouble());
            break;
        case LUA_TBOOLEAN:
            lua_pushboolean(mL, var->getValue().toLower() == "true" ? 1 : 0);
            break;
        case LUA_TTABLE:
            lua_newtable(mL);
            break;
        default:
            lua_settop(mL, stackSize);
            return false;
        }
        //set it up
        if (lua_type(mL, -1) != var->getValueType()) {
            lua_settop(mL, stackSize);
            return false;
        }
        lua_settable(mL, -3);
    }
    return false;
}

// sets the value of a Lua variable by running dynamically-generated Lua code
bool LuaInterface::setValue(TVar* var)
{
    //This function assumes the var has been modified and then called


    QList<TVar*> vars = varOrder(var);
    QString variableChangeCode = vars[0]->getName();
    for (int i = 1; i < vars.size(); i++) {
        if (vars[i]->isReference()) {
            return setCValue(vars);
        }
        const int keyType = vars[i]->getKeyType();
        if (keyType == LUA_TNUMBER || keyType == LUA_TBOOLEAN) {
            variableChangeCode.append(qsl("[%1]").arg(vars.at(i)->getName()));
        } else {
            variableChangeCode.append(qsl(R"(["%1"])").arg(vars.at(i)->getName()));
        }
    }
    switch (var->getValueType()) {
    case LUA_TSTRING:
        variableChangeCode.append(qsl(" = [[%1]]").arg(var->getValue()));
        break;
    case LUA_TNUMBER:
        variableChangeCode.append(qsl(" = %1").arg(var->getValue()));
        break;
    case LUA_TBOOLEAN:
        variableChangeCode.append(qsl(" = %1").arg(var->getValue()));
        break;
    case LUA_TTABLE:
        variableChangeCode.append(QLatin1String(" = {}"));
        break;
    default:
        return false;
    }
    int error = luaL_loadstring(mL, variableChangeCode.toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::setValue(...) WARNING - Internal Lua (parsing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\"" << variableChangeCode << "\".";
        return false;
    }
    error = lua_pcall(mL, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::setValue(...) WARNING - Internal Lua (executing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\"" << variableChangeCode << "\".";
        return false;
    }
    return true;
}

void LuaInterface::deleteVar(TVar* var)
{
    QList<TVar*> vars = varOrder(var);
    QString oldName = vars[0]->getName();
    for (int i = 1; i < vars.size(); i++) {
        const int keyType = vars[i]->getKeyType();
        if (keyType == LUA_TNUMBER || keyType == LUA_TBOOLEAN) {
            oldName.append(qsl("[%1]").arg(vars[i]->getName()));
        } else {
            oldName.append(qsl(R"(["%1"])").arg(vars[i]->getName()));
        }
    }
    //delete it
    oldName.append(qsl(" = nil"));
    int error = luaL_loadstring(mL, oldName.toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::deleteVar(...) WARNING - Internal Lua (parsing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\"" << oldName << "\".";
        return;
    }
    error = lua_pcall(mL, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::deleteVar(...) WARNING - Internal Lua (executing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\"" << oldName << "\".";
    }
}

void LuaInterface::renameCVar(QList<TVar*> vars)
{
    //uses C Api to rename a variable.
    //dangerous function since you can get an api panic
    //and trash the stack

    TVar* var = vars.back();
    //make the new stack
    lua_getglobal(mL, (vars[0]->getName()).toUtf8().constData());
    if (setjmp(buf) == 0) {
        int i = 1;
        int pushCount = 0;
        int kType;
        for (; i < vars.size() - 1; i++) {
            kType = vars[i]->getKeyType();
            if (kType == LUA_TNUMBER) {
                lua_pushnumber(mL, QString(vars[i]->getName()).toDouble());
            } else if (kType == LUA_TTABLE) {
                // registry references are integer refs so must stay toInt()
                lua_rawgeti(mL, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
            } else {
                lua_pushstring(mL, QString(vars[i]->getName()).toUtf8().constData());
            }
            lua_gettable(mL, -2);
            if (lua_isnil(mL, -1)) {
                //value didn't exist, make it
                lua_pop(mL, -1);
                if (kType == LUA_TNUMBER) {
                    lua_pushnumber(mL, QString(vars[i]->getName()).toDouble());
                } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
                    lua_rawgeti(mL, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
                } else {
                    lua_pushstring(mL, QString(vars[i]->getName()).toUtf8().constData());
                }
                lua_newtable(mL);
                lua_settable(mL, -3);
                i--; //decrement since we want to reput this table on the stack on next iteration
            }
        }

        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(mL, QString(var->getNewName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(mL, var->getNewName().toDouble());
        } else if (kType == LUA_TTABLE) {
            lua_rawgeti(mL, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            qWarning().noquote().nospace() << "LuaInterface::renameCVar() - Unsupported key type: " << lua_typename(mL, kType) << " for variable \"" << var->getName()
                                           << "\". Expected string, number, or table.";
            return;
        }

        //put the old value on the stack
        lua_getglobal(mL, (vars[0]->getName()).toUtf8().constData());
        i = 1;
        for (; i < vars.size() - 1; i++) {
            kType = vars[i]->getKeyType();
            if (kType == LUA_TNUMBER) {
                lua_pushnumber(mL, QString(vars[i]->getName()).toDouble());
            } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
                lua_rawgeti(mL, LUA_REGISTRYINDEX, vars[i]->getName().toInt());
            } else {
                lua_pushstring(mL, QString(vars[i]->getName()).toUtf8().constData());
            }
            lua_gettable(mL, -2);
            pushCount++;
        }

        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(mL, QString(var->getName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(mL, var->getName().toDouble());
        } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
            lua_rawgeti(mL, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            qWarning().noquote().nospace() << "LuaInterface::renameCVar() - Unsupported key type when retrieving old value: " << lua_typename(mL, kType) << " for variable \"" << var->getName()
                                           << "\". Expected string, number, table, or function.";
            return;
        }
        lua_gettable(mL, -2);
        pushCount++;
        //old value is @ -1 now
        //we want to put our new named key @ -2
        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(mL, QString(var->getNewName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(mL, var->getNewName().toDouble());
        } else if (kType == LUA_TTABLE) {
            lua_rawgeti(mL, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            qWarning().noquote().nospace() << "LuaInterface::renameCVar() - Unsupported key type when setting new key: " << lua_typename(mL, kType) << " for variable \"" << var->getName()
                                           << "\". Expected string, number, or table.";
            return;
        }
        pushCount++;
        lua_insert(mL, -2);
        lua_settable(mL, -3 - pushCount);
        //key & value popped
        //delete it, so we put the old key back on the stack and set to nil
        kType = var->getKeyType();
        if (kType == LUA_TSTRING) {
            lua_pushstring(mL, QString(var->getName()).toUtf8().constData());
        } else if (kType == LUA_TNUMBER) {
            lua_pushnumber(mL, var->getName().toDouble());
        } else if (kType == LUA_TTABLE || kType == LUA_TFUNCTION) {
            lua_rawgeti(mL, LUA_REGISTRYINDEX, var->getName().toInt());
        } else {
            qWarning().noquote().nospace() << "LuaInterface::renameCVar() - Unsupported key type when deleting old key: " << lua_typename(mL, kType) << " for variable \"" << var->getName()
                                           << "\". Expected string, number, table, or function.";
            return;
        }
        lua_pushnil(mL);
        lua_settable(mL, -3);
        var->clearNewName();
    }
}

bool LuaInterface::loadVar(TVar* var)
{
    //puts the value of a variable on the -1 position of the stack
    if (setjmp(buf) == 0) {
        const int kType = var->getKeyType();
        const int vType = var->getValueType();
        if (vType == LUA_TTABLE) {
            if (kType == LUA_TNUMBER) {
                lua_pushnumber(mL, QString(var->getName()).toDouble());
            } else if (kType == LUA_TTABLE) {
                lua_rawgeti(mL, LUA_REGISTRYINDEX, var->getName().toInt());
            } else {
                lua_pushstring(mL, QString(var->getName()).toUtf8().constData());
            }
            if (lua_istable(mL, -2)) {
                lua_gettable(mL, -2);
                return true;
            }
            lua_pop(mL, 1);
            return false;
        }

        if (vType == LUA_TNUMBER) {
            lua_pushnumber(mL, QString(var->getValue()).toDouble());
        } else if (vType == LUA_TBOOLEAN) {
            lua_pushboolean(mL, var->getValue().toLower() == "true" ? 1 : 0);
        } else if (vType == LUA_TSTRING) {
            lua_pushstring(mL, QString(var->getName()).toUtf8().constData());
        } else {
            return false;
        }
    } else {
        qWarning().noquote().nospace() << "LuaInterface::loadVar() - Lua panic occurred while loading variable \"" << var->getName() << "\" with key type " << lua_typename(mL, var->getKeyType())
                                       << " and value type " << lua_typename(mL, var->getValueType()) << ".";
        return false;
    }
    return true;
}

void LuaInterface::renameVar(TVar* var)
{
    //this assumes anything like reparenting has been done

    QList<TVar*> vars = varOrder(var);
    QString oldVariable = vars.at(0)->getName();
    QString newName;
    if (vars.size() > 1) {
        newName = vars[0]->getName();
    }

    for (int i = 1; i < vars.size(); i++) {
        const int kType = vars[i]->getKeyType();
        // numbers and booleans use unquoted subscripts: t[3.14], t[true]
        if (kType == LUA_TNUMBER || kType == LUA_TBOOLEAN) {
            oldVariable.append(qsl("[%1]").arg(vars.at(i)->getName()));
            if (i < vars.size() - 1) {
                newName.append(qsl("[%1]").arg(vars[i]->getName()));
            }
        } else if (kType == LUA_TTABLE) {
            renameCVar(vars);
            return;
        } else {
            // that leaves LUA_TSTRING
            oldVariable.append(qsl(R"(["%1"])").arg(vars.at(i)->getName()));
            if (i < vars.size() - 1) {
                newName.append(qsl(R"(["%1"])").arg(vars.at(i)->getName()));
            }
        }
    }

    if (vars.size() <= 1) {
        // this variable is at root level on _G
        newName.append(qsl("_G[\"%1\"]").arg(vars.last()->getNewName()));
    } else {
        // this variable is nested in a table
        if (var->getNewKeyType() == LUA_TNUMBER || var->getNewKeyType() == LUA_TBOOLEAN) {
            newName.append(qsl("[%1]").arg(vars.last()->getNewName()));
        } else {
            newName.append(qsl(R"(["%1"])").arg(vars.last()->getNewName()));
        }
    }

    auto renameCode = qsl("%1 = %2").arg(newName, oldVariable);
    int error = luaL_loadstring(mL, renameCode.toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In copying (first) stage, internal Lua (parsing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\"" << renameCode
                                       << "\".";
        var->clearNewName();
        return;
    }
    error = lua_pcall(mL, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In copying (first) stage, internal Lua (executing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\""
                                       << renameCode << "\".";
        var->clearNewName();
        return;
    }

    //delete it
    error = luaL_loadstring(mL, oldVariable.append(QLatin1String(" = nil")).toUtf8().constData());
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In deleting (second) stage, internal Lua (parsing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\""
                                       << renameCode << "\".";
        var->clearNewName();
        return;
    }
    error = lua_pcall(mL, 0, LUA_MULTRET, 0);
    if (error) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - In deleting (second) stage, internal Lua (executing) error: \"" << lua_tostring(mL, -1) << "\" in code:\n\""
                                       << renameCode << "\".";
    }
    var->clearNewName();
}

// returns the value for a string/number/boolean datatype, or an empty string otherwise
QString LuaInterface::getValue(TVar* var)
{
    if (setjmp(buf) == 0) {
        QList<TVar*> const vars = varOrder(var);
        if (vars.empty()) {
            return {};
        }
        const int pCount = vars.size(); //how many things we need to pop from the stack at the end
        //load from _G first
        auto firstVariable = vars.constFirst();
        if (firstVariable->getKeyType() == LUA_TSTRING) {
            lua_getglobal(mL, (firstVariable->getName()).toUtf8().constData());
        } else if (firstVariable->getKeyType() == LUA_TNUMBER) {
            lua_pushnumber(mL, firstVariable->getName().toDouble());
            lua_gettable(mL, LUA_GLOBALSINDEX);
        } else if (firstVariable->getKeyType() == LUA_TBOOLEAN) {
            lua_pushboolean(mL, firstVariable->getName().toLower() == "true" ? 1 : 0);
            lua_gettable(mL, LUA_GLOBALSINDEX);
        }
        if (lua_isnoneornil(mL, lua_gettop(mL))) {
            qDebug() << "LuaInterface::getValue: Couldn't put root value" << firstVariable->getName() << "onto the Lua stack in order to get value of" << var->getName()
                     << ", perhaps the key type isn't supported?";
            return {};
        }
        for (int i = 1; i < vars.size(); i++) {
            if (!loadValue(mL, vars.at(i), -2)) {
                return {};
            }
        }
        const int valueType = lua_type(mL, -1);
        QString value;
        if (valueType == LUA_TBOOLEAN) {
            value = lua_toboolean(mL, -1) == 0 ? QLatin1String("false") : QLatin1String("true");
        } else if (valueType == LUA_TNUMBER || valueType == LUA_TSTRING) {
            value = lua_tostring(mL, -1);
        }
        lua_pop(mL, pCount);
        return value;
    }
    return {};
}

void LuaInterface::iterateTable(lua_State* L, int index, TVar* tVar, bool hide)
{
    depth++;
    while (lua_next(L, index)) {
        const int vType = lua_type(L, -1);
        const int kType = lua_type(L, -2);
        lua_pushvalue(L, -2); //we do this because extracting the key with tostring changes it
        QString keyName;
        QString valueName;
        bool keyIsReference = false;
        if (kType == LUA_TTABLE) {
            keyName = QString::number(luaL_ref(L, LUA_REGISTRYINDEX)); //this function pops the top item
            lrefs.append(keyName.toInt());
            keyIsReference = true;
        } else if (kType == LUA_TBOOLEAN) {
            //lua_tostring() returns NULL for booleans, name the key ourselves
            keyName = lua_toboolean(L, -1) ? qsl("true") : qsl("false");
            lua_pop(L, 1);
        } else {
            keyName = lua_tostring(L, -1);
            if (kType == LUA_TFUNCTION && keyName.isEmpty()) {
                //we lost the reference
                keyName = QString::number(luaL_ref(L, LUA_REGISTRYINDEX));
                lrefs.append(keyName.toInt());
                keyIsReference = true;
            } else {
                lua_pop(L, 1);
            }
        }
        if (keyName == "package" && depth == 1) { //don't load in the 'package' table
            lua_pop(L, 1);
            continue;
        }
        if (mSavedVarsOnly && depth == 1) {
            if (!mSavedRootNames.contains(keyName)) {
                lua_pop(L, 1);
                continue;
            }
            // each saved global is walked in its own dedup scope, so two of them
            // that reference the same table both get a complete subtree
            varUnit->clearPointers();
        }
        auto var = new TVar();
        var->setReference(keyIsReference);
        var->setName(keyName, kType);
        var->setValueType(vType);
        var->setParent(tVar);
        var->hidden = hide;
        tVar->addChild(var);
        const void* pKey = lua_topointer(L, -1);
        var->pKey = pKey;
        const void* pValue = lua_topointer(L, -2);
        var->pValue = pValue;
        // A table two names reach is walked only under the first, which suits
        // Mudlet's own API - but a saved variable reached second would be left
        // with no node to export from, so it is exempt (#9755). _G reaches
        // itself, hence the name test.
        if (keyName == qsl("_G") || (varUnit->varExists(var) && !varUnit->isSaved(var))) {
            lua_pop(L, 1);
            tVar->removeChild(var);
            delete var;
            continue;
        }
        varUnit->addVariable(var);

        varUnit->addPointer(pKey);

        varUnit->addPointer(pValue);
        if (vType == LUA_TTABLE) {
            var->setValue("{}", LUA_TTABLE);
            const bool tooDeep = depth > scmMaxTableDepth;
            if (!tooDeep && lua_checkstack(L, 3)) {
                //put the table on top
                lua_pushnil(L);
                iterateTable(L, -2, var, hide);
                depth--;
            } else {
                const QString variableName = varUnit->shortVarName(var).join(qsl("."));
                qWarning().noquote().nospace() << "LuaInterface::iterateTable() WARNING - not reading the contents of the table \"" << variableName
                                               << "\": " << (tooDeep ? qsl("it is nested more than %1 tables deep").arg(scmMaxTableDepth) : qsl("the Lua stack could not be grown"))
                                               << ". It is being treated as an empty table.";
                if (mSavedVarsOnly) {
                    mTruncatedSavedTables.append(variableName);
                }
            }
        } else if (vType == LUA_TSTRING || vType == LUA_TNUMBER) {
            lua_pushvalue(L, -1);
            valueName = lua_tostring(L, -1);
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
    // QElapsedTimer t;
    // t.start();
    // onPanic() longjmp()s to the shared buf, so without a setjmp of our own
    // that jump lands in whichever frame set it last - usually one that has
    // already returned, taking the caller's scope down with it.
    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) != 0) {
        // the panic jumped out of iterateTable() with its working values still
        // on the stack of the profile's live interpreter
        lua_settop(mL, stackTop);
        qWarning() << "LuaInterface::getVars() WARNING - Lua panicked while reading the variables in; the variable tree is incomplete.";
        return;
    }
    lua_pushnil(mL);
    TVar* global = resetVariableTree();
    iterateTable(mL, LUA_GLOBALSINDEX, global, hide);
    // FIXME: possible to keep and report? qDebug()<<"took"<<t.elapsed()<<"to get variables in";
}

// The tree a profile save needs: the globals the profile saves, read fresh out
// of Lua. Reading all of _G instead costs the size of _G on every save, and the
// dedup in iterateTable() then only spares a saved table itself - table members
// riding along with it under #9517 have no savedVars entry to be spared by, so
// whichever unrelated global reached them first would keep them (#9755). Walking
// each saved global in its own dedup scope is what avoids that.
void LuaInterface::getSavedVars()
{
    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) != 0) {
        lua_settop(mL, stackTop);
        // getVars() does not clear this itself, so a later walk on this same
        // interface would inherit the filter
        mSavedVarsOnly = false;
        qWarning() << "LuaInterface::getSavedVars() WARNING - Lua panicked while reading the saved variables in; the variable tree is incomplete.";
        return;
    }
    mSavedRootNames.clear();
    mTruncatedSavedTables.clear();
    for (const QString& savedVarName : std::as_const(varUnit->savedVars)) {
        // savedVars holds dotted paths, but a global's own name may contain a
        // dot as well, so both readings count as a name to walk
        mSavedRootNames.insert(savedVarName);
        mSavedRootNames.insert(savedVarName.section(QChar('.'), 0, 0));
    }

    TVar* global = resetVariableTree();
    if (mSavedRootNames.isEmpty()) {
        return;
    }

    mSavedVarsOnly = true;
    lua_pushnil(mL);
    iterateTable(mL, LUA_GLOBALSINDEX, global, false);
    mSavedVarsOnly = false;
}

// Throws away whatever tree there was, along with the Lua registry references it
// held, and returns the _G node a fresh walk hangs off.
TVar* LuaInterface::resetVariableTree()
{
    depth = 0;
    auto global = new TVar();
    global->setName(qsl("_G"), LUA_TSTRING);
    global->setValue(qsl("{}"), LUA_TTABLE);
    releaseVariableReferences();
    varUnit->clear();
    varUnit->setBase(global);
    varUnit->addVariable(global);
    return global;
}
