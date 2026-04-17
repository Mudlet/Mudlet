#ifndef MUDLET_BOGUSACTIONSCANNER_H
#define MUDLET_BOGUSACTIONSCANNER_H

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

#include <QList>
#include <QString>

#include <list>

class TAction;

// Detects action-tree entries matching the signature of the bogus "New toolbar"
// / "New menu" pairs created by the bug fixed in PR #9194, where the editor's
// constructor was inadvertently calling dlgTriggerEditor::addAction(bool)
// instead of QMainWindow::addAction(QAction*). Each profile load added a fresh
// disabled pair to the Buttons tree that users couldn't easily clean up.
//
// The scanner is pure logic over the in-memory action tree so it can be unit
// tested without spinning up the editor. Criteria are deliberately strict - we
// only flag entries that are structurally indistinguishable from the bug's
// output - to avoid deleting legitimate stub items a user happened to create.
class BogusActionScanner
{
public:
    // Expected display names - typically tr("New toolbar") and tr("New menu")
    // as produced by the buggy code path. Injected so the scanner stays
    // independent of Qt translation state, which also makes unit testing
    // trivial. Empty values disable the corresponding name check.
    struct Names
    {
        QString toolbar;
        QString menu;
    };

    // Returns the top-level "toolbar" entries that match the bug signature.
    // The matching single-child "menu" entry is implied and is removed
    // together with its parent when the caller deletes the returned nodes.
    static QList<TAction*> findBogusEntries(const std::list<TAction*>& rootNodes, const Names& names);

    // Exposed for tests: returns true if pRoot is the top-level folder of a
    // bogus toolbar/menu pair.
    static bool matchesBogusSignature(const TAction* pRoot, const Names& names);
};

#endif // MUDLET_BOGUSACTIONSCANNER_H
