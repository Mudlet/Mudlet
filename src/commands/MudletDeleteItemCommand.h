#ifndef MUDLET_MUDLETDELETEITEMCOMMAND_H
#define MUDLET_MUDLETDELETEITEMCOMMAND_H

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
#include "EditorViewType.h"

#include <QList>
#include <QString>

/*!
 * \brief Qt undo command for deleting items
 *
 * Handles deletion of single or multiple items by storing XML snapshots.
 * Uses built-in batching (QList<DeletedItemInfo>) for efficiency - delete
 * operations naturally come as sets with ordering constraints.
 *
 * Key design decisions:
 * - Stores full item hierarchy in XML (children are included automatically)
 * - Handles ID remapping when items are restored (may get new IDs)
 * - Nullifies mpHost before deletion to prevent unregistration issues
 * - Sorts items for correct restoration order (parents before children)
 */
class MudletDeleteItemCommand : public MudletCommand
{
public:
    struct DeletedItemInfo {
        int itemID;
        int parentID;
        int positionInParent;
        QString xmlSnapshot;
        QString itemName;
    };

    MudletDeleteItemCommand(EditorViewType viewType, const QList<DeletedItemInfo>& deletedItems, Host* host);

    void undo() override;
    void redo() override;
    EditorViewType viewType() const override { return mViewType; }
    QList<int> affectedItemIDs() const override;
    void remapItemID(int oldID, int newID) override;

    // Get ID changes that occurred during undo (oldID -> newID mapping)
    // Returns a list of pairs: first = oldID, second = newID
    QList<QPair<int, int>> getIDChanges() const { return mIDChanges; }

    // Check if the last undo/redo operation processed any valid items
    // Returns false if all items were skipped due to Lua API conflicts
    bool wasValid() const { return mLastOperationWasValid; }

private:
    static QString generateText(EditorViewType viewType, int itemCount, const QString& firstName);

    EditorViewType mViewType;
    QList<DeletedItemInfo> mDeletedItems;
    QList<QPair<int, int>> mIDChanges;  // Track ID changes during undo (oldID, newID)
    bool mLastOperationWasValid{true};  // Track if last undo/redo processed any valid items
};

#endif // MUDLET_MUDLETDELETEITEMCOMMAND_H
