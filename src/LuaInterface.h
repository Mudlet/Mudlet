#ifndef MUDLET_LUAINTERFACE_H
#define MUDLET_LUAINTERFACE_H

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


#include "TVar.h"

#include <QScopedPointer>
#include <QSet>

#include <utility>

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}


class Host;
class TLuaInterpreter;
class VarUnit;

class QTreeWidgetItem;


class LuaInterface
{
public:
    // the deepest table level whose contents are read; anything below that is
    // recorded as an empty table, which for a saved variable means its contents
    // do not get saved
    static constexpr int scmMaxTableDepth = 99;

    explicit LuaInterface(lua_State*);
    ~LuaInterface();
    void iterateTable(lua_State*, int, TVar*, bool);
    void getVars(bool);
    void getSavedVars();
    // the saved variables the last getSavedVars() had to cut short, for the
    // caller to tell the user about - a save holds them as empty tables
    QStringList truncatedSavedTables() const { return mTruncatedSavedTables; }
    // the saved globals the last getSavedVars() cannot vouch for having read
    // whole: it found a value inside, at any depth, that a save cannot carry
    // (anything but a table, string, number or boolean), or a Lua panic stopped
    // it part way through one. A walk cut short answers only for what it reached.
    QSet<QString> savedRootsHoldingUnsaveableValues() const { return mSavedRootsHoldingUnsaveableValues; }
    // the saved globals missing from the tree the last getSavedVars() built,
    // because a Lua panic had to be got past by dropping them or stopped the
    // walk reaching them at all - they are in no profile save taken from that
    // tree, which is the caller's to tell the user about
    QStringList unreadableSavedRoots() const { return mUnreadableSavedRoots; }
    QStringList varName(TVar* var);
    QList<TVar*> varOrder(TVar* var);
    // leaves the Lua stack as it found it, whatever the outcome
    QString getValue(TVar*);
    bool loadKey(lua_State*, TVar*);
    // pushes the value on success, and nothing on any failure
    bool loadValue(lua_State*, TVar*, int);
    bool setValue(TVar*);
    void deleteVar(TVar*);
    bool renameCVar(QList<TVar*>);
    // false when the rename did not happen - the variable keeps the name it had
    // and the node keeps naming it, which the caller has to tell the user about
    bool renameVar(TVar*);
    void createVar(TVar*);
    // whether a variable can be written back through the name the tree gave it,
    // which the editor asks before writing - the write paths themselves do not
    bool writableByName(TVar*);
    VarUnit* getVarUnit();
    // Anything that builds a variable tree and throws it away owes this call:
    // ~LuaInterface cannot make it, see there.
    void releaseVariableReferences();
    bool loadVar(TVar* var);
    bool reparentCVariable(TVar* from, TVar* to, TVar* curVar);
    bool reparentVariable(QTreeWidgetItem*, QTreeWidgetItem*, QTreeWidgetItem*);
    std::pair<bool, QString> validMove(QTreeWidgetItem*);
    void getAllChildren(TVar* var, QList<TVar*>* list);
    lua_State* getState() const;
    static int onPanic(lua_State*);

private:
    TVar* resetVariableTree();
    bool readSavedVars();
    void addSavedRootsMissingFromTheTree();
    bool pushKey(TVar*, const QString& name, const int keyType);
    bool pushOwningTable(const QList<TVar*>&);
    bool newNameIsFree(TVar*);

    int depth = 0;
    lua_State* mL;
    QSet<TVar> hiddenVars;
    QScopedPointer<VarUnit> varUnit;
    QList<int> lrefs;
    // makes iterateTable() a saved-globals-only walk that starts a fresh dedup
    // scope per global...
    bool mSavedVarsOnly = false;
    // ...and these are the top-level keys such a walk may keep
    QSet<QString> mSavedRootNames;
    // which of those keys the walk is inside, meaningful only while one is running
    QString mCurrentSavedRootName;
    // ...and the one a panic ended the last attempt inside, which outlives that
    // attempt because it is what the next one has to leave out
    QString mPanickedSavedRootName;
    // the keys it found a value under that the save cannot carry
    QSet<QString> mSavedRootsHoldingUnsaveableValues;
    // ...and the ones missing from the tree it ended up with
    QStringList mUnreadableSavedRoots;
    QStringList mTruncatedSavedTables;
};

#endif // MUDLET_LUAINTERFACE_H
