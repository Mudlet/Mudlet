#ifndef MUDLET_VARUNIT_H
#define MUDLET_VARUNIT_H

/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021 by Stephen Lyons - slysven@virginmedia..com        *
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


#include <memory>

#include <QCoreApplication>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QStringList>


class TVar;

class QTreeWidgetItem;

struct lua_State;


class VarUnit
{
    Q_DECLARE_TR_FUNCTIONS(VarUnit) // Needed so we can use tr() even though VarUnit is NOT derived from QObject

public:
    VarUnit();
    ~VarUnit();
    QStringList varName(TVar*);
    QStringList shortVarName(TVar*);
    bool varExists(TVar*);
    bool shouldSave(QTreeWidgetItem*);
    bool shouldSave(TVar*);
    void addVariable(TVar*);
    void addTempVar(QTreeWidgetItem*, TVar*);
    void removeTempVar(QTreeWidgetItem*);
    void removeVariable(TVar*);
    void setBase(TVar*);
    TVar* getBase();
    void clear();
    void buildVarTree(QTreeWidgetItem*, TVar*, bool);
    TVar* getWVar(QTreeWidgetItem*);
    TVar* getTVar(QTreeWidgetItem*);
    void addTreeItem(QTreeWidgetItem*, TVar*);
    void removeTreeItem(QTreeWidgetItem*);
    void addSavedVar(TVar*);
    void removeSavedVar(TVar*);
    void addHidden(TVar*, int);
    void addHidden(const QString&);
    bool isHidden(TVar* var);
    bool isHidden(const QString& fullname);
    void removeHidden(TVar* var);
    void removeHidden(const QString& name);
    bool isSaved(TVar*);
    void renameVariableBookkeeping(TVar*, const QString& oldFullName, const QString& newFullName);
    void addPointer(const void*);
    void clearPointers();
    void clearHiddenTables();
    void anchorHiddenTable(lua_State*, int valueIndex, const void* table);
    void shareHiddenTableAnchors(const VarUnit&);
    QString getUnsaveableReason(TVar*);
    QSet<QString> hidden;
    QSet<QString> hiddenByUser;
    // The identity half of `hidden`: the Lua tables behind those names, so that
    // one of them reached under a name of the user's own is still recognised.
    // An address is only an identity while that table is alive, and Lua hands a
    // collected one's address back out, so a hidden table dropped since the last
    // hiding walk can name a live one - a fresh user variable landing on the
    // address would vanish from the Variables view and from profile saves.
    // isHidden() therefore checks the address against a weak anchor of the
    // table (anchorHiddenTable()) before trusting it, which asks Lua whether
    // that table is still alive without keeping it alive.
    // Never assign this on its own - an identity is only usable alongside its
    // anchor, so the save-time copy takes both at once through
    // shareHiddenTableAnchors().
    QSet<const void*> hiddenTables;
    QSet<QString> savedVars;

private:
    bool rootNameReadsAsAMemberPath(TVar*) const;
    int countTableItems(TVar*);
    void rememberHiddenTable(TVar*, const QString& fullName);
    void forgetHiddenTable(const QString& fullName);
    void forgetHiddenTableAddress(const void* table);
    void releaseAnchorSlot(int slot);
    bool hiddenTableStillAlive(const void* table) const;
    std::unique_ptr<TVar> base;
    QSet<QString> variableSet;
    // ?? variables
    QMap<QTreeWidgetItem*, TVar*> wVars;
    // temporary variables
    QMap<QTreeWidgetItem*, TVar*> tVars;
    QSet<const void*> mPointers;
    // what un-hiding a name has to hand back to hiddenTables
    QHash<QString, const void*> mHiddenTableByName;
    // The weak anchors behind hiddenTables: the interpreter they live in, the
    // registry reference of a weak-valued table holding each hidden table, and
    // which of its slots holds the table behind each address. 0 is not a value
    // luaL_ref() hands out, so it stands for "no anchor" in mHiddenTableAnchors
    // and in the slot values alike.
    lua_State* mpAnchorState = nullptr;
    int mHiddenTableAnchors = 0;
    QHash<const void*, int> mHiddenTableSlots;
    // false on a save-time copy, whose anchors are borrowed from the live unit
    // (shareHiddenTableAnchors()): releasing them would pull the live unit's
    // registry entries out from under it, so every luaL_unref is gated on this.
    bool mOwnsAnchors = true;
};

#endif // MUDLET_VARUNIT_H
