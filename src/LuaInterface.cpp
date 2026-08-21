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
    // Pushes the value on success and leaves the stack alone on every failure
    // path. Every caller hands over the profile's live state, where a slot left
    // behind stays for the rest of the session (#9885).
    const int entryTop = lua_gettop(L);
    if (setjmp(buf) == 0) {
        // loadKey() pushes nothing for a key type it does not handle, which the
        // return value does not distinguish - the caller's own top would stand
        // in for the key - and it can also return false after pushing
        if (!loadKey(L, var) || lua_gettop(L) == entryTop) {
            lua_settop(L, entryTop);
            return false;
        }
        //everything is tabled in lua, we need to just find what table
        //we're using, if index == 0, we iterate to the closest table
        if (index) {
            const int topWithKey = lua_gettop(L);
            const int actualIndex = (index < 0) ? topWithKey + index + 1 : index;

            if (actualIndex <= 0 || actualIndex > topWithKey) {
                qWarning().noquote().nospace() << "LuaInterface::loadValue() - Invalid stack index " << index << " for variable \"" << var->getName() << "\". Stack size: " << topWithKey
                                               << ", resolved index: " << actualIndex << ".";
                lua_settop(L, entryTop);
                return false;
            }

            if (!lua_istable(L, index)) {
                qWarning().noquote().nospace() << "LuaInterface::loadValue() - Value at stack index " << index << " is not a table for variable \"" << var->getName()
                                               << "\". Got type: " << lua_typename(L, lua_type(L, index)) << ".";
                lua_settop(L, entryTop);
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
                qWarning().noquote().nospace() << "LuaInterface::loadValue() - No table found on stack for variable \"" << var->getName() << "\" when index=0. Stack size: " << lua_gettop(L) << ".";
                lua_settop(L, entryTop);
                return false;
            }
        }
        if (lua_gettop(L) > entryTop && lua_type(L, -1) == var->getValueType()) {
            return true;
        }
        lua_settop(L, entryTop);
        return false;
    }
    lua_settop(L, entryTop);
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

// Names the Variables view will write back through, which is narrower than what
// the write paths can reach: they push keys through the C API, which carries any
// key at all, while a string key holding a quote or a backslash and a root that
// is not a plain identifier are refused here rather than written (#9908).
static bool nameTheEditorWritesThrough(TVar* var, const bool asRoot)
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
//  - the name is one the Variables view writes through, see above.
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
        if (nameSharedWithASibling(vars.at(i)) || !nameTheEditorWritesThrough(vars.at(i), i == 0)) {
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

// Pushes the key one element of a variable's path is reached by. A key the tree
// named through a Lua registry reference - a table or a function used as a key -
// comes back out of the registry, so the write lands on that key itself rather
// than on the text of its reference number. Nothing is pushed when the key
// cannot be built, which the caller has to treat as a failure: a write through a
// key that is not the variable's own lands on some other member of the table.
bool LuaInterface::pushKey(TVar* var, const QString& name, const int keyType)
{
    if (var->isReference()) {
        // registry references are integer refs so must stay toInt()
        lua_rawgeti(mL, LUA_REGISTRYINDEX, name.toInt());
        if (lua_isnoneornil(mL, -1)) {
            // the reference has gone since the tree was built, so there is no
            // key here to write through
            lua_pop(mL, 1);
            return false;
        }
        return true;
    }
    if (keyType == LUA_TNUMBER) {
        lua_pushnumber(mL, name.toDouble());
        return true;
    }
    if (keyType == LUA_TBOOLEAN) {
        lua_pushboolean(mL, name.toLower() == QLatin1String("true") ? 1 : 0);
        return true;
    }
    // Everything else the tree names directly is a string key - a table or a
    // function key is named through a reference, handled above. By length, so a
    // key holding a quote or a newline is the key the walk read, and not
    // something a generated string literal has to survive being written into.
    const QByteArray key = name.toUtf8();
    lua_pushlstring(mL, key.constData(), key.length());
    return true;
}

// Pushes the table the last element of a variable's path lives in, ready for the
// caller to write into. Walks down from _G one level at a time, dropping each
// level as it enters the next, so a single value is left on the stack. On false
// the caller owes the stack back - every write path here restores it anyway.
bool LuaInterface::pushOwningTable(const QList<TVar*>& vars)
{
    if (vars.isEmpty()) {
        return false;
    }
    // headroom for what one level needs at once: the level being looked at, a
    // key pushed into it, the value to write and a copy of the key to look the
    // current value up with. Each level drops the one above it as it is entered,
    // so the depth of the path does not add to this.
    if (!lua_checkstack(mL, 4)) {
        qWarning().noquote().nospace() << "LuaInterface::pushOwningTable() WARNING - the Lua stack could not be grown, so \"" << vars.constLast()->getName() << "\" cannot be written to.";
        return false;
    }
    lua_pushvalue(mL, LUA_GLOBALSINDEX);
    for (int i = 0; i < vars.size() - 1; ++i) {
        TVar* level = vars.at(i);
        if (!lua_istable(mL, -1) || !pushKey(level, level->getName(), level->getKeyType())) {
            return false;
        }
        lua_gettable(mL, -2);
        lua_remove(mL, -2);
    }
    return lua_istable(mL, -1);
}

// Writes a variable's value into Lua. Through the C API, because generating Lua
// source to do it put the value and the keys into that source as text, which a
// value holding "]]" or a key holding a quote does not survive: the chunk failed
// to parse, the write was quietly dropped and the parse error was left behind on
// the Lua stack. A value starting with a newline lost that newline as well, to
// the long-bracket literal it was written into.
bool LuaInterface::setValue(TVar* var)
{
    const QList<TVar*> vars = varOrder(var);
    if (vars.isEmpty()) {
        // a variable named _G - varOrder() gives it no path
        return false;
    }

    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) == 0) {
        if (!pushOwningTable(vars) || !pushKey(var, var->getName(), var->getKeyType())) {
            qWarning().noquote().nospace() << "LuaInterface::setValue(...) WARNING - could not reach \"" << var->getName() << "\" to write to it.";
            lua_settop(mL, stackTop);
            return false;
        }
        switch (var->getValueType()) {
        case LUA_TSTRING: {
            const QByteArray value = var->getValue().toUtf8();
            lua_pushlstring(mL, value.constData(), value.length());
            break;
        }
        case LUA_TNUMBER:
            lua_pushnumber(mL, var->getValue().toDouble());
            break;
        case LUA_TBOOLEAN:
            lua_pushboolean(mL, var->getValue().toLower() == QLatin1String("true") ? 1 : 0);
            break;
        case LUA_TTABLE:
            // A table already there is left as it is: this is the call the
            // editor makes for a table it has just created, and the "= {}" the
            // generated source used would replace one that is already in use
            // with an empty one.
            lua_pushvalue(mL, -1);
            lua_gettable(mL, -3);
            if (lua_istable(mL, -1)) {
                lua_settop(mL, stackTop);
                return true;
            }
            lua_pop(mL, 1);
            lua_newtable(mL);
            break;
        default:
            lua_settop(mL, stackTop);
            return false;
        }
        lua_settable(mL, -3);
        lua_settop(mL, stackTop);
        return true;
    }
    lua_settop(mL, stackTop);
    qWarning().noquote().nospace() << "LuaInterface::setValue(...) WARNING - Lua panicked while writing to \"" << var->getName() << "\"; it has not been changed.";
    return false;
}

// Through the C API for the same reasons setValue() is, and for one more: a
// member reached by a table key was named in the generated source by the text of
// its registry reference number, so the delete nil'ed a string key of that text
// and left the member itself where it was, to reappear in the Variables view.
void LuaInterface::deleteVar(TVar* var)
{
    const QList<TVar*> vars = varOrder(var);
    if (vars.isEmpty()) {
        // a variable named _G - varOrder() gives it no path
        return;
    }

    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) == 0) {
        if (!pushOwningTable(vars) || !pushKey(var, var->getName(), var->getKeyType())) {
            qWarning().noquote().nospace() << "LuaInterface::deleteVar(...) WARNING - could not reach \"" << var->getName() << "\" to delete it.";
            lua_settop(mL, stackTop);
            return;
        }
        lua_pushnil(mL);
        lua_settable(mL, -3);
        lua_settop(mL, stackTop);
        return;
    }
    lua_settop(mL, stackTop);
    qWarning().noquote().nospace() << "LuaInterface::deleteVar(...) WARNING - Lua panicked while deleting \"" << var->getName() << "\"; it has not been deleted.";
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

// Whether the name a rename is about to write to is free. A rename copies the
// value onto the new key and only then nils the old one, so a name another
// member of the same table already answers to loses that member's value without
// a word - the write is refused instead.
bool LuaInterface::newNameIsFree(TVar* var)
{
    if (var->isReference() || (var->getNewName() == var->getName() && var->getNewKeyType() == var->getKeyType())) {
        // a member reached by a reference key is renamed onto the very key it
        // already has, and so is one whose name has not in fact changed
        return true;
    }

    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) == 0) {
        if (!pushOwningTable(varOrder(var)) || !pushKey(var, var->getNewName(), var->getNewKeyType())) {
            // nothing here to be destroyed - and the rename, reaching the same
            // table by the same path, has as little chance of getting there
            lua_settop(mL, stackTop);
            return true;
        }
        // Raw, like the walk that named the members of this table in the first
        // place: a value an __index metamethod stands in with is not a member
        // the rename would destroy.
        lua_rawget(mL, -2);
        const bool free = lua_isnoneornil(mL, -1);
        lua_settop(mL, stackTop);
        return free;
    }
    // Fails closed: the probe is what stands between a rename and a sibling's
    // value, so a panic part way through it leaves the question unanswered, and
    // the answer that cannot destroy anything is that the name is taken.
    lua_settop(mL, stackTop);
    qWarning().noquote().nospace() << "LuaInterface::newNameIsFree() WARNING - Lua panicked while looking \"" << var->getNewName() << "\" up, so it is being treated as a name already in use.";
    return false;
}

// Moves a variable onto its new key. Through the C API for the same reason
// setValue() writes through it: a rename was generated Lua source - "new = old"
// and "old = nil" - with every key along the path spliced into it as text, so a
// key holding a quote or a backslash renamed some other member or nothing at
// all, and a key that closed the subscript ran the rest of itself as Lua.
bool LuaInterface::renameVar(TVar* var)
{
    //this assumes anything like reparenting has been done

    const QList<TVar*> vars = varOrder(var);
    if (vars.isEmpty()) {
        // a variable named _G - varOrder() gives it no path
        var->abandonNewName();
        return false;
    }

    if (var->isReference()) {
        // A key that is a table or a function of its own is not a name, and the
        // tree names such a member by the number of the registry reference
        // holding the key, and renaming it to a string is not an operation that
        // exists - what it did instead was write to a member named by that
        // number, or delete the member outright.
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - not renaming \"" << var->getName() << "\" to \"" << var->getNewName()
                                       << "\": its key is a table or a function, which has no name to change.";
        var->abandonNewName();
        return false;
    }

    if (!newNameIsFree(var)) {
        qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - not renaming \"" << var->getName() << "\" to \"" << var->getNewName()
                                       << "\": another variable of that name is already there, and renaming onto it would destroy it.";
        var->abandonNewName();
        return false;
    }

    // what the bookkeeping this variable is in has to follow it to, worked out
    // before the rename goes anywhere near the name it is keyed by
    QStringList newNameParts = varUnit->shortVarName(var);
    const QString oldFullName = newNameParts.join(qsl("."));
    newNameParts.removeLast();
    newNameParts.append(var->getNewName());
    const QString newFullName = newNameParts.join(qsl("."));

    const int stackTop = lua_gettop(mL);
    if (setjmp(buf) == 0) {
        if (!pushOwningTable(vars) || !pushKey(var, var->getName(), var->getKeyType())) {
            qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - could not reach \"" << var->getName() << "\" to rename it.";
            lua_settop(mL, stackTop);
            var->abandonNewName();
            return false;
        }
        // the old key stays on the stack: nil'ing the variable out from under
        // it is what is left to do once the new key holds the value
        lua_pushvalue(mL, -1);
        lua_gettable(mL, -3);
        if (!pushKey(var, var->getNewName(), var->getNewKeyType())) {
            qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - could not build the name \"" << var->getNewName() << "\" to rename \"" << var->getName() << "\" to.";
            lua_settop(mL, stackTop);
            var->abandonNewName();
            return false;
        }
        lua_insert(mL, -2);
        lua_settable(mL, -4);
        lua_pushnil(mL);
        lua_settable(mL, -3);
        lua_settop(mL, stackTop);
        varUnit->renameVariableBookkeeping(var, oldFullName, newFullName);
        var->clearNewName();
        return true;
    }
    lua_settop(mL, stackTop);
    qWarning().noquote().nospace() << "LuaInterface::renameVar(...) WARNING - Lua panicked while renaming \"" << var->getName() << "\"; it may be left under either name.";
    var->abandonNewName();
    return false;
}

// Returns the value for a string/number/boolean datatype, and an empty string for
// everything else - including for a variable it cannot read, which it has no way
// to report. Nothing that commits the answer where an empty string cannot be told
// apart from a variable that really is empty may use it (#9769); the save reads
// its values off the tree the walk built, see exportedValue() in XMLexport.cpp.
QString LuaInterface::getValue(TVar* var)
{
    // A save calls this once per exported variable, on the profile's live stack,
    // so every exit has to put that stack back where it found it: what one left
    // behind used to be charged to the state for the rest of the session (#9885).
    const int entryTop = lua_gettop(mL);
    if (setjmp(buf) == 0) {
        QList<TVar*> const vars = varOrder(var);
        if (vars.empty()) {
            return {};
        }
        // lua_getglobal() and lua_gettable() below do not grow the stack, and the
        // C API guarantees only LUA_MINSTACK free slots, so reserve the room up
        // front: one per level of nesting - the key loadValue() pushes for a
        // level becomes that level's value - plus one spare
        if (!lua_checkstack(mL, static_cast<int>(vars.size()) + 1)) {
            qWarning().noquote().nospace() << "LuaInterface::getValue() WARNING - could not grow the Lua stack to reach \"" << var->getName() << "\", so no value is being read for it.";
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
        // a key type none of those handles pushes nothing at all, which the top
        // alone cannot be told apart from - whatever the caller left there would
        // stand in for the root
        if (lua_gettop(mL) == entryTop) {
            qDebug() << "LuaInterface::getValue: Couldn't put root value" << firstVariable->getName() << "onto the Lua stack in order to get value of" << var->getName() << ", its key type"
                     << lua_typename(mL, firstVariable->getKeyType()) << "isn't supported here.";
            return {};
        }
        if (lua_isnoneornil(mL, -1)) {
            qDebug() << "LuaInterface::getValue: Root value" << firstVariable->getName() << "is no longer set, so there is no value to get for" << var->getName();
            lua_settop(mL, entryTop);
            return {};
        }
        for (int i = 1; i < vars.size(); i++) {
            if (!loadValue(mL, vars.at(i), -2)) {
                lua_settop(mL, entryTop);
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
        lua_settop(mL, entryTop);
        return value;
    }
    lua_settop(mL, entryTop);
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
