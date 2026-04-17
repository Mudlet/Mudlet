/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include "BogusActionScanner.h"

#include "TAction.h"

namespace {

// Shape the buggy dlgTriggerEditor::addNewAction(true) path imprints on every
// TAction it creates. Both the outer toolbar and its single child menu share
// this profile; anything deviating is not a product of the bug. mLocation /
// mOrientation are intentionally not checked: the editor's saveAction() writes
// form defaults back to the first entry between the two constructor calls, so
// the persisted values vary. The remaining criteria - plus the strict
// structural shape checked by the caller - are still tight enough to avoid
// flagging anything a user has actually set up.
bool hasEmptyBugImprint(const TAction* node)
{
    return node && node->isFolder() && !node->isActive() && !node->isTemporary() && node->getScript().isEmpty() && node->getCommandButtonUp().isEmpty() && node->getCommandButtonDown().isEmpty()
           && !node->mIsPushDownButton && node->mPackageName.isEmpty();
}

bool nameMatches(const QString& actual, const QString& expected)
{
    return expected.isEmpty() || actual == expected;
}

} // namespace

bool BogusActionScanner::matchesBogusSignature(const TAction* pRoot, const Names& names)
{
    if (!hasEmptyBugImprint(pRoot)) {
        return false;
    }
    if (pRoot->getParent() != nullptr) {
        return false;
    }
    if (!nameMatches(pRoot->getName(), names.toolbar)) {
        return false;
    }

    const auto* children = pRoot->getChildrenList();
    if (!children || children->size() != 1) {
        return false;
    }

    const TAction* pChild = children->front();
    if (!hasEmptyBugImprint(pChild)) {
        return false;
    }
    if (pChild->getChildrenList() && !pChild->getChildrenList()->empty()) {
        return false;
    }
    if (!nameMatches(pChild->getName(), names.menu)) {
        return false;
    }

    return true;
}

QList<TAction*> BogusActionScanner::findBogusEntries(const std::list<TAction*>& rootNodes, const Names& names)
{
    QList<TAction*> matches;
    for (TAction* pRoot : rootNodes) {
        if (matchesBogusSignature(pRoot, names)) {
            matches.append(pRoot);
        }
    }
    return matches;
}
