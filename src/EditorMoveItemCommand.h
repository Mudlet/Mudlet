#ifndef MUDLET_MUDLETMOVEITEMCOMMAND_H
#define MUDLET_MUDLETMOVEITEMCOMMAND_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "EditorCommand.h"

class Host;

// Undo command for moving items. Handles reparenting and position changes in the editor tree.
class EditorMoveItemCommand : public EditorCommand
{
public:
    EditorMoveItemCommand(EditorViewType viewType, int itemID, int oldParentID, int newParentID, int oldPosition, int newPosition, const QString& itemName, Host* host);

    void undo() override;
    void redo() override;
    EditorViewType viewType() const override { return mViewType; }
    QList<int> affectedItemIDs() const override { return {mItemID}; }
    void remapItemID(int oldID, int newID) override;

private:
    void moveItem(int fromParentID, int toParentID, int position);
    static QString generateText(EditorViewType viewType, const QString& itemName);

    EditorViewType mViewType;
    int mItemID;
    int mOldParentID;
    int mNewParentID;
    int mOldPosition;
    int mNewPosition;
    QString mItemName;
    mutable bool mSkipFirstRedo = true; // Skip initial redo() called by QUndoStack::push()
};

#endif // MUDLET_MUDLETMOVEITEMCOMMAND_H
