#ifndef MUDLET_MUDLETADDITEMCOMMAND_H
#define MUDLET_MUDLETADDITEMCOMMAND_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org   *
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

#include "MudletCommand.h"

class Host;

/*!
 * \brief Qt undo command for adding new items
 *
 * Handles undo/redo of item additions. Undo exports the item to XML and deletes it.
 * Redo recreates the item from the XML snapshot. Since recreated items may get new IDs,
 * this command tracks ID changes and triggers remapping in the undo stack.
 */
class MudletAddItemCommand : public MudletCommand
{
public:
    MudletAddItemCommand(EditorViewType viewType, int itemID, int parentID,
                         int positionInParent, bool isFolder, const QString& itemName, Host* host);

    void undo() override;
    void redo() override;
    EditorViewType viewType() const override { return mViewType; }
    QList<int> affectedItemIDs() const override { return {mItemID}; }
    void remapItemID(int oldID, int newID) override;

    // Check if item ID changed during redo (when item was recreated)
    bool didItemIDChange() const { return mOldItemID != -1 && mOldItemID != mItemID; }
    int getOldItemID() const { return mOldItemID; }
    int getNewItemID() const { return mItemID; }

private:
    static QString generateText(EditorViewType viewType, const QString& itemName, bool isFolder);

    EditorViewType mViewType;
    int mItemID;
    int mOldItemID = -1; // Tracks old ID before redo, for ID remapping
    int mParentID;
    int mPositionInParent;
    bool mIsFolder;
    QString mItemName;
    QString mItemSnapshot;
    mutable bool mSkipFirstRedo = true;  // Skip initial redo() called by QUndoStack::push()
};

#endif // MUDLET_MUDLETADDITEMCOMMAND_H
