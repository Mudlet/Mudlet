/***************************************************************************
 *   Copyright (C) 2025 by Excellencedev - ademiluyisuccessandexcellence@gmail.com *
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

#ifndef EDITOR_MOVE_ITEM_COMMAND_H
#define EDITOR_MOVE_ITEM_COMMAND_H

#include "EditorCommand.h"

class EditorMoveItemCommand : public EditorCommand
{
public:
    EditorMoveItemCommand(EditorViewType viewType, int itemID, const QString& itemName, int oldParentID, int oldPosition, int newParentID, int newPosition, Host* host);

    void undo() override;
    void redo() override;

    QList<int> affectedItemIDs() const override;
    void remapItemID(int oldID, int newID) override;

private:
    void moveItem(int itemID, int parentID, int position);

    EditorViewType mViewType;
    int mItemID;
    QString mItemName;
    int mOldParentID;
    int mOldPosition;
    int mNewParentID;
    int mNewPosition;

    static QString generateText(EditorViewType viewType, const QString& itemName);
};

#endif // EDITOR_MOVE_ITEM_COMMAND_H
