#ifndef MUDLET_MUDLETMODIFYPROPERTYCOMMAND_H
#define MUDLET_MUDLETMODIFYPROPERTYCOMMAND_H

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

#include <QString>

/*!
 * \brief Qt undo command for modifying item properties
 *
 * Handles property changes (name, pattern, script code, enabled state, etc.)
 * by storing complete XML snapshots of the old and new states. Unlike
 * AddItemCommand which creates/deletes items, this updates existing items in place.
 *
 * Key design decisions:
 * - Uses full XML snapshots for simplicity and correctness
 * - No skip-first-redo pattern needed (change already applied before push)
 * - updateXXXFromXML modifies items in place (no ID change, no remapping needed)
 */
class MudletModifyPropertyCommand : public MudletCommand
{
public:
    MudletModifyPropertyCommand(EditorViewType viewType, int itemID,
                                const QString& itemName,
                                const QString& oldStateXML, const QString& newStateXML,
                                Host* host);

    void undo() override;
    void redo() override;
    EditorViewType viewType() const override { return mViewType; }
    QList<int> affectedItemIDs() const override { return {mItemID}; }
    void remapItemID(int oldID, int newID) override;

private:
    static QString generateText(EditorViewType viewType, const QString& itemName);

    EditorViewType mViewType;
    int mItemID;
    QString mItemName;
    QString mOldStateXML;
    QString mNewStateXML;
};

#endif // MUDLET_MUDLETMODIFYPROPERTYCOMMAND_H
