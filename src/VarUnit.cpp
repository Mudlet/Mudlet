/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021-2022 by Stephen Lyons - slysven@virginmedia..com   *
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


#include "VarUnit.h"

#include "TVar.h"
#include "utils.h"

#include <QDebug>
#include <QLocale>
#include <QTreeWidgetItem>

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}


VarUnit::VarUnit()
: base(nullptr)
{
}

// Never releases mHiddenTableAnchors: the profile's lua_State is closed before
// the LuaInterface owning this unit is replaced (see ~LuaInterface()), and an
// unref into a freed state would crash. The registry entry dies with the state.
VarUnit::~VarUnit() = default;

// A global may hold a dot in its own name - _G["mod.count"] - and the dotted name
// everything here is keyed by then reads exactly like the path of member "count"
// inside table "mod". Those are two different variables, so for such a root that
// name is not an identity: what a hiding walk recorded, and what a save was told
// to keep, was the member path, and answering the root from it hid the root out
// of the Variables view and gave it the member's saved state (#9954).
//
// Only the sets Mudlet itself writes are fenced off. hiddenByUser stays keyed by
// the colliding name because dlgTriggerEditor::slot_hideVariable() records the
// user's own hide through addHidden(var, 1), which writes that very string for a
// dotted root - refusing to answer it would make hiding such a root impossible,
// where the ambiguity that remains is between two hides the user asked for.
// A hidden table is also still recognised by identity in isHidden() below, which
// no name collision reaches.
bool VarUnit::rootNameReadsAsAMemberPath(TVar* var) const
{
    TVar* pParent = var ? var->getParent() : nullptr;
    return pParent && pParent->getName() == qsl("_G") && var->getName().contains(QLatin1Char('.'));
}

bool VarUnit::isHidden(TVar* var)
{
    if (var->getName() == qsl("_G")) { // we never hide global
        return false;
    }
    // By identity as well as by name: a saved variable holding one of Mudlet's
    // or a package's tables reaches it under a name of the user's own, which no
    // name-keyed lookup matches, and a profile save would then write out that
    // table's contents (#9769).
    if (var->pValue && hiddenTables.contains(var->pValue)) {
        if (hiddenTableStillAlive(var->pValue)) {
            return true;
        }
        // The table behind this address has been collected, so the address no
        // longer names a hidden table - typically it now names a fresh variable
        // of the user's. Forget the identity so it is not asked about again.
        forgetHiddenTableAddress(var->pValue);
    }
    const QString fullName = shortVarName(var).join(qsl("."));
    if (hidden.contains(fullName) && !rootNameReadsAsAMemberPath(var)) {
        return true;
    }
    return hiddenByUser.contains(fullName);
}

// A hiding walk re-records every identity it finds, so the walk starts by
// dropping the stale ones - and the anchor table backing them - wholesale here,
// rather than pruning them one at a time as isHidden() queries them.
void VarUnit::clearHiddenTables()
{
    hiddenTables.clear();
    mHiddenTableByName.clear();
    mHiddenTableSlots.clear();
    if (mpAnchorState && mHiddenTableAnchors && mOwnsAnchors) {
        luaL_unref(mpAnchorState, LUA_REGISTRYINDEX, mHiddenTableAnchors);
    }
    mHiddenTableAnchors = 0;
}

// Holds the table at valueIndex in a weak-valued registry table, so that
// isHidden() can later ask whether its address still names it. Weak, so the
// anchor never keeps the table alive: its slot reads nil from the moment the
// table is collected, which is before Lua can hand the address to a new one.
void VarUnit::anchorHiddenTable(lua_State* L, int valueIndex, const void* table)
{
    if (!table) {
        return;
    }
    if (!lua_checkstack(L, 3)) {
        // Unanchored, the address would still be trusted long after the table
        // is gone, so the identity goes with the anchor - name-keyed hiding
        // still applies.
        qWarning() << "VarUnit::anchorHiddenTable() WARNING - the Lua stack could not be grown; this table is hidden by name only.";
        forgetHiddenTableAddress(table);
        return;
    }
    const int valueAt = (valueIndex > 0 || valueIndex <= LUA_REGISTRYINDEX) ? valueIndex : lua_gettop(L) + valueIndex + 1;
    mpAnchorState = L;
    // a table two hidden names reach is anchored under each; free the first
    // name's slot rather than stranding it until the next walk
    releaseAnchorSlot(mHiddenTableSlots.take(table));
    if (!mHiddenTableAnchors) {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushstring(L, "v");
        lua_setfield(L, -2, "__mode");
        lua_setmetatable(L, -2);
        mHiddenTableAnchors = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, mHiddenTableAnchors);
    lua_pushvalue(L, valueAt);
    mHiddenTableSlots.insert(table, luaL_ref(L, -2));
    lua_pop(L, 1);
}

// The whole identity handoff for the save-time copy: which addresses are hidden
// and the anchors to answer liveness from. The anchors are borrowed, not owned -
// mOwnsAnchors keeps the copy from releasing the live unit's registry entries
// out from under it.
void VarUnit::shareHiddenTableAnchors(const VarUnit& source)
{
    hiddenTables = source.hiddenTables;
    mpAnchorState = source.mpAnchorState;
    mHiddenTableAnchors = source.mHiddenTableAnchors;
    mHiddenTableSlots = source.mHiddenTableSlots;
    mOwnsAnchors = false;
}

bool VarUnit::hiddenTableStillAlive(const void* table) const
{
    const int slot = mHiddenTableSlots.value(table, 0);
    if (!mpAnchorState || !mHiddenTableAnchors || !slot) {
        // Anchoring failed or never ran, so nothing can tell this address apart
        // from a recycled one. Not trusting it only costs hiding by identity -
        // hiding by name still stands - while trusting it can swallow a fresh
        // variable of the user's.
        qWarning() << "VarUnit::hiddenTableStillAlive() WARNING - a hidden table was never anchored; it is hidden by name only.";
        return false;
    }
    if (!lua_checkstack(mpAnchorState, 2)) {
        return true; // cannot look, so the identity stands as it did
    }
    lua_rawgeti(mpAnchorState, LUA_REGISTRYINDEX, mHiddenTableAnchors);
    if (!lua_istable(mpAnchorState, -1)) {
        // a lifecycle bug (released or clobbered registry ref), not a collected
        // table - do not start un-hiding over it
        qWarning() << "VarUnit::hiddenTableStillAlive() WARNING - the anchor table is gone; the identity stands as it did.";
        lua_pop(mpAnchorState, 1);
        return true;
    }
    lua_rawgeti(mpAnchorState, -1, slot);
    const bool alive = lua_istable(mpAnchorState, -1) && lua_topointer(mpAnchorState, -1) == table;
    lua_pop(mpAnchorState, 2);
    return alive;
}

// Only tables are worth an identity: nothing else has contents for a saved
// variable to drag into a profile save by holding it.
void VarUnit::rememberHiddenTable(TVar* var, const QString& fullName)
{
    if (var->getValueType() != LUA_TTABLE || !var->pValue) {
        return;
    }
    mHiddenTableByName.insert(fullName, var->pValue);
    hiddenTables.insert(var->pValue);
}

void VarUnit::forgetHiddenTable(const QString& fullName)
{
    const auto it = mHiddenTableByName.constFind(fullName);
    if (it == mHiddenTableByName.constEnd()) {
        return;
    }
    forgetHiddenTableAddress(it.value());
}

// Drops every trace of one remembered identity: the address, its anchor slot,
// and the names that would hand it back.
void VarUnit::forgetHiddenTableAddress(const void* table)
{
    hiddenTables.remove(table);
    releaseAnchorSlot(mHiddenTableSlots.take(table));
    mHiddenTableByName.removeIf([table](const auto& it) {
        return it.value() == table;
    });
}

void VarUnit::releaseAnchorSlot(int slot)
{
    if (!slot || !mOwnsAnchors || !mpAnchorState || !mHiddenTableAnchors) {
        return;
    }
    lua_rawgeti(mpAnchorState, LUA_REGISTRYINDEX, mHiddenTableAnchors);
    if (lua_istable(mpAnchorState, -1)) {
        luaL_unref(mpAnchorState, -1, slot);
    }
    lua_pop(mpAnchorState, 1);
}


// A name on its own cannot say whether it is a dotted root's or a member path's,
// so this overload cannot make the distinction the TVar one does. It is only
// asked about the names in savedVars (Host::hideMudletsVariables()), which a
// dotted root can no longer get into - see addSavedVar().
bool VarUnit::isHidden(const QString& fullname)
{
    if (fullname == QLatin1String("_G")) { // we never hide global
        return false;
    }
    if (hidden.contains(fullname)) {
        return true;
    }
    return hiddenByUser.contains(fullname);
}

void VarUnit::addPointer(const void* pointer)
{
    mPointers.insert(pointer);
}

// Resets the seen-pointer set varExists() answers from. iterateTable() does this
// per saved root so a table two roots share is walked in full under each; within
// one walk that set is the cycle guard, with the depth cap as the backstop.
void VarUnit::clearPointers()
{
    mPointers.clear();
}

// The same question as the TVar overload below, asked of the row in the
// Variables view standing for that variable - so it has to give the same answer.
// It used to leave out the size limit, and Qt's tristate cascade ticks a child
// whose ItemIsUserCheckable flag buildVarTree() stripped, so a table over the
// limit reached by ticking its parent stayed ticked, went into savedVars and was
// written into the profile (#9957).
bool VarUnit::shouldSave(QTreeWidgetItem* pWidgetItem)
{
    TVar* var = getWVar(pWidgetItem);

    return var && shouldSave(var);
}

bool VarUnit::shouldSave(TVar* var)
{
    if (var->getValueType() == 6 || var->isReference()) {
        return false;
    }

    if (rootNameReadsAsAMemberPath(var)) {
        return false;
    }

    // Check if table is too large (max 10,000 items)
    if (var->getValueType() == LUA_TTABLE) {
        const int itemCount = countTableItems(var);
        if (itemCount > 10000) {
            return false;
        }
    }

    return true;
}

int VarUnit::countTableItems(TVar* var)
{
    int count = 0;
    const QList<TVar*> children = var->getChildren(false);

    for (TVar* child : children) {
        count++;
        // Recursively count items in nested tables
        if (child->getValueType() == LUA_TTABLE) {
            count += countTableItems(child);
        }
    }

    return count;
}

QString VarUnit::getUnsaveableReason(TVar* var)
{
    if (var->getValueType() == LUA_TFUNCTION) {
        //: Tooltip explaining why a Lua function cannot be saved
        return tr("Lua functions cannot be saved.");
    }

    if (var->isReference()) {
        //: Tooltip explaining why a referenced variable cannot be saved
        return tr("Referenced variables cannot be saved.");
    }

    if (rootNameReadsAsAMemberPath(var)) {
        //: Tooltip explaining why a global whose own name contains a dot cannot be saved
        return tr("Saved variables are remembered by their dotted path, so a global with a dot in its own name "
                  "cannot be told apart from a member of a table and cannot be saved.");
    }

    if (var->getValueType() == LUA_TTABLE) {
        const int itemCount = countTableItems(var);
        if (itemCount > 10000) {
            //: Tooltip explaining why a large table cannot be saved, recommending alternative methods
            return tr("This table has %1 items, exceeding the 10,000 item limit for saved variables. "
                      "Use <b>table.save()</b> and <b>table.load()</b> instead for better performance with large tables.")
                    .arg(QLocale::system().toString(itemCount));
        }
    }

    return QString();
}

void VarUnit::buildVarTree(QTreeWidgetItem* p, TVar* var, bool showHidden)
{
    QList<QTreeWidgetItem*> cList;
    QListIterator<TVar*> it(var->getChildren(true));
    while (it.hasNext()) {
        TVar* child = it.next();
        if (showHidden || !isHidden(child)) {
            QStringList s1;
            s1 << child->getName();
            auto pItem = new QTreeWidgetItem(s1);
            pItem->setText(0, child->getName());
            pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
            pItem->setToolTip(0, utils::richText(tr("Checked variables will be saved and loaded with your profile.")));
            pItem->setCheckState(0, Qt::Unchecked);
            if (isSaved(child)) {
                pItem->setCheckState(0, Qt::Checked);
            }
            if (!shouldSave(child)) {
                pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
                pItem->setForeground(0, QBrush(QColor("grey")));
                const QString reason = getUnsaveableReason(child);
                pItem->setToolTip(0, reason.isEmpty() ? QString() : utils::richText(reason));
            }
            pItem->setData(0, Qt::UserRole, child->getValueType());
            QIcon icon;
            switch (child->getValueType()) {
            case 5:
                icon.addPixmap(QPixmap(qsl(":/icons/table.png")), QIcon::Normal, QIcon::Off);
                break;
            case 6:
                icon.addPixmap(QPixmap(qsl(":/icons/function.png")), QIcon::Normal, QIcon::Off);
                break;
            default:
                icon.addPixmap(QPixmap(qsl(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
                break;
            }
            pItem->setIcon(0, icon);
            wVars.insert(pItem, child);
            cList.append(pItem);
            if (child->getValueType() == 5) {
                buildVarTree(pItem, child, showHidden);
            }
        }
    }
    p->addChildren(cList);
}

void VarUnit::addTreeItem(QTreeWidgetItem* p, TVar* var)
{
    wVars.insert(p, var);
}

void VarUnit::removeTreeItem(QTreeWidgetItem* p)
{
    wVars.remove(p);
    tVars.remove(p);
}

void VarUnit::addTempVar(QTreeWidgetItem* p, TVar* var)
{
    tVars.insert(p, var);
}

void VarUnit::removeTempVar(QTreeWidgetItem* p)
{
    tVars.remove(p);
}

TVar* VarUnit::getTVar(QTreeWidgetItem* p)
{
    if (tVars.contains(p)) {
        return tVars[p];
    }
    return nullptr;
}

TVar* VarUnit::getWVar(QTreeWidgetItem* p)
{
    if (wVars.contains(p)) {
        return wVars[p];
    }
    return nullptr;
}

QStringList VarUnit::varName(TVar* var)
{
    QStringList names;
    names << "_G";
    if (var == base.get() || !var) {
        return names;
    }
    names << var->getName();
    TVar* p = var->getParent();
    while (p && p != base.get()) {
        names.insert(1, p->getName());
        if (p == base.get()) {
            break;
        }
        p = p->getParent();
    }
    return names;
}

QStringList VarUnit::shortVarName(TVar* var)
{
    QStringList names;
    if (!var || var->getName() == qsl("_G")) {
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

void VarUnit::addVariable(TVar* var)
{
    variableSet.insert(varName(var).join(qsl(".")));
    if (var->hidden) {
        const QString shortName = shortVarName(var).join(qsl("."));
        hidden.insert(shortName);
        rememberHiddenTable(var, shortName);
    }
}

void VarUnit::addHidden(TVar* var, int user)
{
    var->hidden = true;
    const QString shortName = shortVarName(var).join(qsl("."));
    if (user) {
        hiddenByUser.insert(shortName);
    } else {
        hidden.insert(shortName);
        rememberHiddenTable(var, shortName);
    }
}

void VarUnit::addHidden(const QString& var)
{
    hiddenByUser.insert(var);
}

void VarUnit::removeHidden(TVar* var)
{
    const QString fullName = shortVarName(var).join(qsl("."));
    hidden.remove(fullName);
    hiddenByUser.remove(fullName);
    forgetHiddenTable(fullName);
    var->hidden = false;
}

void VarUnit::removeHidden(const QString& name)
{
    hidden.remove(name);
    hiddenByUser.remove(name);
    forgetHiddenTable(name);
    // does not remove the reference from TVar, similar to addHidden()
}

// The exact names the rename moves: the variable's own and its real
// descendants', read off the tree rather than matched as string prefixes. A
// sibling whose own key holds a dot ("a.b" beside member "a") joins to the very
// path a descendant of "a" would have, and a prefix rule would drag that
// sibling's marks onto a name nothing has.
static void collectRenamedPaths(TVar* var, const QString& oldPath, const QString& newPath, QHash<QString, QString>& renames)
{
    renames.insert(oldPath, newPath);
    for (TVar* child : var->getChildren(false)) {
        collectRenamedPaths(child, oldPath + QLatin1Char('.') + child->getName(), newPath + QLatin1Char('.') + child->getName(), renames);
    }
}

static void renameNameKeyedEntries(QSet<QString>& names, const QHash<QString, QString>& renames)
{
    for (auto it = renames.constBegin(); it != renames.constEnd(); ++it) {
        if (names.remove(it.key())) {
            names.insert(it.value());
        }
    }
}

// Everything name-keyed this unit remembers about a variable it remembers by the
// dotted name the tree gave it, so a rename has to take those memberships with
// it. Left behind, a saved or hidden mark keeps answering for the old name, and
// catches whichever unrelated variable is born under it next; the renamed
// variable meanwhile loses the mark the user put on it. Renaming a table moves
// what is remembered about its members as well - exactly its members, walked
// from the tree, so an entry that merely reads like a member's path stays put.
void VarUnit::renameVariableBookkeeping(TVar* var, const QString& oldFullName, const QString& newFullName)
{
    if (!var || oldFullName == newFullName || oldFullName.isEmpty() || newFullName.isEmpty()) {
        return;
    }
    QHash<QString, QString> renames;
    collectRenamedPaths(var, oldFullName, newFullName, renames);
    renameNameKeyedEntries(savedVars, renames);
    renameNameKeyedEntries(hidden, renames);
    renameNameKeyedEntries(hiddenByUser, renames);
    // Only the name has changed: the tables are the same tables, so their
    // addresses are still identities and their anchors still answer for them.
    // Forgetting the identities here instead would leave them hidden by name
    // alone.
    for (auto it = renames.constBegin(); it != renames.constEnd(); ++it) {
        const auto found = mHiddenTableByName.constFind(it.key());
        if (found != mHiddenTableByName.constEnd()) {
            const void* table = found.value();
            mHiddenTableByName.erase(found);
            mHiddenTableByName.insert(it.value(), table);
        }
    }
}

void VarUnit::addSavedVar(TVar* var)
{
    if (rootNameReadsAsAMemberPath(var)) {
        // savedVars would key it by the member path's name, so the entry would
        // mark that member saved instead. shouldSave() refuses such a root, so
        // the Variables view never offers this in the first place.
        return;
    }
    const QString fullName = shortVarName(var).join(qsl("."));
    var->saved = true;
    savedVars.insert(fullName);
}

void VarUnit::removeSavedVar(TVar* var)
{
    var->saved = false;
    if (rootNameReadsAsAMemberPath(var)) {
        // ...and taking that entry out again would un-save the member, which
        // clicking such a root's row in the Variables view used to do
        return;
    }
    savedVars.remove(shortVarName(var).join(qsl(".")));
}

bool VarUnit::isSaved(TVar* var)
{
    if (rootNameReadsAsAMemberPath(var)) {
        // nothing keyed by this name is about this root: the entry belongs to
        // the member path, and addSavedVar() will not put such a root there
        return false;
    }
    const QString fullName = shortVarName(var).join(qsl("."));
    return (savedVars.contains(fullName) || var->saved);
}

void VarUnit::removeVariable(TVar* var)
{
    variableSet.remove(varName(var).join(qsl(".")));
}

bool VarUnit::varExists(TVar* var)
{
    return ((var->pKey && mPointers.contains(var->pKey)) || (var->pValue && mPointers.contains(var->pValue)));
}

TVar* VarUnit::getBase()
{
    return base.get();
}

void VarUnit::setBase(TVar* pVariable)
{
    base.reset(pVariable);
}

void VarUnit::clear()
{
    base.reset();
    tVars.clear();
    wVars.clear();
    variableSet.clear();
    mPointers.clear();
}
