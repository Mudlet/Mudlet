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
#include <QRegularExpression>

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

// A name two members of the same table are both shown under says nothing about
// which of them is meant, and whichever one a write reaches may well be the
// other. Two keys can share a name: "%.14g" gives 1/3 and 0.33333333333333 the
// same text, and a key ending at an embedded NUL is named by the part before it.
static bool nameSharedWithASibling(TVar* var)
{
    TVar* parent = var->getParent();
    if (!parent) {
        return false;
    }
    // a number key and a string key that read the same are still distinct
    // lookups, so the key type is part of the name here
    const QString name = var->getName();
    const int keyType = var->getKeyType();
    int matches = 0;
    for (const TVar* sibling : parent->getChildren(false)) {
        if (sibling->getName() == name && sibling->getKeyType() == keyType && ++matches > 1) {
            return true;
        }
    }
    return false;
}

// The write paths build Lua source out of these names - see setValue() - where a
// string key goes inside a quoted literal and a root goes in bare. So a key
// holding a backslash comes back out as an escape sequence, and a root that is
// not an identifier either fails to parse or, with a dot in it, parses as an
// index into some other global.
static bool nameSurvivesGeneratedCode(TVar* var, const bool asRoot)
{
    const QString name = var->getName();
    if (asRoot) {
        static const QRegularExpression identifier(qsl("^[A-Za-z_][A-Za-z0-9_]*$"));
        static const QSet<QString> luaKeywords{qsl("and"),   qsl("break"), qsl("do"),  qsl("else"), qsl("elseif"), qsl("end"),    qsl("false"), qsl("for"),  qsl("function"), qsl("if"),   qsl("in"),
                                               qsl("local"), qsl("nil"),   qsl("not"), qsl("or"),   qsl("repeat"), qsl("return"), qsl("then"),  qsl("true"), qsl("until"),    qsl("while")};
        // a keyword reads as an identifier but parses as itself
        return var->getKeyType() == LUA_TSTRING && identifier.match(name).hasMatch() && !luaKeywords.contains(name);
    }
    if (var->getKeyType() != LUA_TSTRING) {
        return true;
    }
    return !name.contains(QLatin1Char('\\')) && !name.contains(QLatin1Char('"')) && !name.contains(QLatin1Char('\n')) && !name.contains(QLatin1Char('\r'));
}

// Whether a variable can be written back through the name the variable tree gave
// it, which is what every write path has to reach it by. Three things have to
// hold, and for the tables above the variable as much as for the variable:
//  - the name finds that same variable again. Lua names a number key with
//    "%.14g", which a key of 1/3 does not survive, and names a string key
//    through a C string, which ends at an embedded NUL - so the name is a key of
//    its own, and writing through it leaves a second variable beside the real
//    one (#9903). A variable a script has deleted since the tree was built is
//    not found either.
//  - no other member of the same table is shown under that name too.
//  - the name is one the generated Lua source can carry back, which is a
//    narrower thing than the C API can look up.
// What the variable holds is not part of the question: a script is free to have
// changed that since the tree was built, and the name still finds it.
bool LuaInterface::writableByName(TVar* var)
{
    const QList<TVar*> vars = varOrder(var);
    if (vars.isEmpty()) {
        // _G, which nothing writes through a name
        return false;
    }
    for (int i = 0; i < vars.size(); ++i) {
        if (nameSharedWithASibling(vars.at(i)) || !nameSurvivesGeneratedCode(vars.at(i), i == 0)) {
            return false;
        }
    }

    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) == 0) {
        // the table of the level being looked at, plus the key pushed into it,
        // for as many levels as there are names
        if (!lua_checkstack(mL, static_cast<int>(vars.size()) + 2)) {
            qWarning().noquote().nospace() << "LuaInterface::writableByName() WARNING - could not grow the Lua stack to reach \"" << var->getName() << "\", so it is being treated as unwritable.";
            return false;
        }
        lua_pushvalue(mL, LUA_GLOBALSINDEX);
        for (TVar* level : vars) {
            const int topWithTable = lua_gettop(mL);
            // Raw, because the tree was built by iterating the tables, which is
            // raw as well: a value an __index metamethod stands in with is not
            // the variable the tree is showing. loadKey() also pushes nothing
            // for a key type it does not handle, and says so only by leaving the
            // top where it was.
            if (!lua_istable(mL, -1) || !loadKey(mL, level) || lua_gettop(mL) != topWithTable + 1) {
                lua_settop(mL, stackTop);
                return false;
            }
            lua_rawget(mL, -2);
            if (lua_isnoneornil(mL, -1)) {
                lua_settop(mL, stackTop);
                return false;
            }
        }
        lua_settop(mL, stackTop);
        return true;
    }
    lua_settop(mL, stackTop);
    qWarning().noquote().nospace() << "LuaInterface::writableByName() WARNING - Lua panicked while looking \"" << var->getName() << "\" up, so it is being treated as unwritable.";
    return false;
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

// Returns the value for a string/number/boolean datatype, and an empty string for
// everything else - including for a variable it cannot read, which it has no way
// to report. Nothing that commits the answer where an empty string cannot be told
// apart from a variable that really is empty may use it (#9769); the save reads
// its values off the tree the walk built, see exportedValue() in XMLexport.cpp.
QString LuaInterface::getValue(TVar* var)
{
    // this walks down to the variable a push at a time, so every way out owes
    // the caller's Lua stack back
    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) == 0) {
        QList<TVar*> const vars = varOrder(var);
        if (vars.empty()) {
            return {};
        }
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
            lua_settop(mL, stackTop);
            return {};
        }
        for (int i = 1; i < vars.size(); i++) {
            if (!loadValue(mL, vars.at(i), -2)) {
                lua_settop(mL, stackTop);
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
        lua_settop(mL, stackTop);
        return value;
    }
    lua_settop(mL, stackTop);
    return {};
}

// The value types a variable can survive a save as: XMLimport hands every element
// it reads to setValue(), which can rebuild nothing else. A value of any other
// type is lost across the save, so a saved global holding one anywhere inside it
// cannot be exported whole (#9857).
static bool serializableValueType(const int valueType)
{
    return valueType == LUA_TTABLE || valueType == LUA_TSTRING || valueType == LUA_TNUMBER || valueType == LUA_TBOOLEAN;
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
        if (mSavedVarsOnly) {
            if (depth == 1) {
                if (!mSavedRootNames.contains(keyName)) {
                    lua_pop(L, 1);
                    continue;
                }
                // each saved global is walked in its own dedup scope, so two of them
                // that reference the same table both get a complete subtree
                varUnit->clearPointers();
                mCurrentSavedRootName = keyName;
            } else if (!serializableValueType(vType)) {
                // the whole global, not just the table this value sits in: a
                // script gets the global back as one object, so one member the
                // save cannot carry makes all of it untrustworthy
                mSavedRootsHoldingUnsaveableValues.insert(mCurrentSavedRootName);
            }
        }
        auto var = new TVar();
        var->setReference(keyIsReference);
        var->setName(keyName, kType);
        var->setValueType(vType);
        var->setParent(tVar);
        var->hidden = hide;
        tVar->addChild(var);
        // whatever branch named the key above left the stack as lua_next did,
        // with the key at -2 and the value at -1
        const void* pKey = lua_topointer(L, -2);
        var->pKey = pKey;
        const void* pValue = lua_topointer(L, -1);
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
            if (hide) {
                // The identity addVariable() just remembered is only exact
                // while this table is alive - anchor it weakly so isHidden()
                // can tell a recycled address from the table itself.
                varUnit->anchorHiddenTable(L, -1, pValue);
            }
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
    if (hide) {
        // this walk is about to hide everything it finds, so it is also where
        // the identities of what was hidden last time stop being worth keeping
        varUnit->clearHiddenTables();
    }
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
    mSavedRootNames.clear();
    mUnreadableSavedRoots.clear();
    mPanickedSavedRootName.clear();
    for (const QString& savedVarName : std::as_const(varUnit->savedVars)) {
        // savedVars holds dotted paths, but a global's own name may contain a
        // dot as well, so both readings count as a name to walk
        mSavedRootNames.insert(savedVarName);
        mSavedRootNames.insert(savedVarName.section(QChar('.'), 0, 0));
    }

    if (mSavedRootNames.isEmpty()) {
        resetVariableTree();
        return;
    }

    mSavedVarsOnly = true;
    bool gaveUp = false;
    // A panic cannot be resumed from where it happened, so the global the walk
    // died inside is dropped and the rest are read again from the top: an
    // unreadable global then costs the save that one variable instead of every
    // variable the walk had still to reach (#9769). Each pass drops one global,
    // so this is bounded by how many the profile saves.
    while (!readSavedVars()) {
        const QString unreadableRoot = mPanickedSavedRootName;
        mPanickedSavedRootName.clear();
        // Without a global to point at, the next attempt would only die in the
        // same place, so the walk stops and reports what it is short of instead.
        if (unreadableRoot.isEmpty() || !mSavedRootNames.remove(unreadableRoot)) {
            gaveUp = true;
            break;
        }
        mUnreadableSavedRoots.append(unreadableRoot);
    }
    // getVars() does not clear this itself, so a later walk on this same
    // interface would inherit the filter
    mSavedVarsOnly = false;

    if (gaveUp) {
        addSavedRootsMissingFromTheTree();
    }
    mUnreadableSavedRoots.sort();
}

// What a walk that stopped without naming its culprit is short of: every saved
// global with no node in the tree. One the user has since deleted reads as
// missing too, and it is just as true of that one that this save has not got it.
void LuaInterface::addSavedRootsMissingFromTheTree()
{
    QSet<QString> readRoots;
    TVar* base = varUnit->getBase();
    const QList<TVar*> roots = base ? base->getChildren(false) : QList<TVar*>();
    for (const TVar* root : roots) {
        readRoots.insert(root->getName());
    }

    for (const QString& savedVarName : std::as_const(varUnit->savedVars)) {
        const QString rootName = savedVarName.section(QChar('.'), 0, 0);
        if (!readRoots.contains(rootName) && !mUnreadableSavedRoots.contains(rootName)) {
            mUnreadableSavedRoots.append(rootName);
        }
    }
}

// One attempt at the walk. False means a Lua panic cut it short, leaving the
// tree holding however much of it had been read - and each attempt starts from
// an empty tree and empty findings, so nothing a failed one saw carries over.
bool LuaInterface::readSavedVars()
{
    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) != 0) {
        lua_settop(mL, stackTop);
        // Past depth 1 the walk is inside the global it entered last, so that
        // global is the one that died; at depth 1 it could equally have been
        // naming a key of a global it has no interest in, and blaming the last
        // one entered would drop a variable that reads perfectly well.
        if (depth > 1) {
            mPanickedSavedRootName = mCurrentSavedRootName;
            // should the caller give up rather than go round again, this global
            // stays in the tree holding however much of it was read, which must
            // not be written out as if it were all of it
            mSavedRootsHoldingUnsaveableValues.insert(mCurrentSavedRootName);
        }
        qWarning().noquote().nospace() << "LuaInterface::readSavedVars() WARNING - Lua panicked "
                                       << (depth > 1 ? qsl("while reading the saved variable \"%1\"").arg(mCurrentSavedRootName) : qsl("before or between the saved variables"))
                                       << "; the variable tree is incomplete.";
        return false;
    }

    mTruncatedSavedTables.clear();
    mSavedRootsHoldingUnsaveableValues.clear();
    mCurrentSavedRootName.clear();
    resetVariableTree();
    lua_pushnil(mL);
    iterateTable(mL, LUA_GLOBALSINDEX, varUnit->getBase(), false);
    return true;
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
