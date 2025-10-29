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

#include "MudletMoveItemCommand.h"

#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "utils.h"

MudletMoveItemCommand::MudletMoveItemCommand(EditorViewType viewType, int itemID,
                                             int oldParentID, int newParentID,
                                             int oldPosition, int newPosition,
                                             const QString& itemName, Host* host)
    : MudletCommand(generateText(viewType, itemName), host)
    , mViewType(viewType)
    , mItemID(itemID)
    , mOldParentID(oldParentID)
    , mNewParentID(newParentID)
    , mOldPosition(oldPosition)
    , mNewPosition(newPosition)
    , mItemName(itemName)
{
}

void MudletMoveItemCommand::undo()
{
    moveItem(mNewParentID, mOldParentID, mOldPosition);
}

void MudletMoveItemCommand::redo()
{
    // Skip the first redo() which is automatically called by QUndoStack::push()
    // The move has already been performed by TTreeWidget::rowsInserted()
    if (mSkipFirstRedo) {
        mSkipFirstRedo = false;
        return;
    }

    moveItem(mOldParentID, mNewParentID, mNewPosition);
}

void MudletMoveItemCommand::remapItemID(int oldID, int newID)
{
    if (mItemID == oldID) {
        mItemID = newID;
    }
    if (mOldParentID == oldID) {
        mOldParentID = newID;
    }
    if (mNewParentID == oldID) {
        mNewParentID = newID;
    }
}

void MudletMoveItemCommand::moveItem(int fromParentID, int toParentID, int position)
{
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        mpHost->getTriggerUnit()->reParentTrigger(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmAliasView: {
        mpHost->getAliasUnit()->reParentAlias(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmTimerView: {
        mpHost->getTimerUnit()->reParentTimer(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmScriptView: {
        mpHost->getScriptUnit()->reParentScript(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmKeysView: {
        mpHost->getKeyUnit()->reParentKey(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    case EditorViewType::cmActionView: {
        mpHost->getActionUnit()->reParentAction(mItemID, fromParentID, toParentID, TreeItemInsertMode::AtPosition, position);
        break;
    }
    default:
        qWarning() << "MudletMoveItemCommand::moveItem() - Unknown view type";
        break;
    }
}

QString MudletMoveItemCommand::generateText(EditorViewType viewType, const QString& itemName)
{
    QString typeName;
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        //: Undo/redo menu text for moving a trigger to a different folder
        typeName = QObject::tr("trigger");
        break;
    case EditorViewType::cmAliasView:
        //: Undo/redo menu text for moving an alias to a different folder
        typeName = QObject::tr("alias");
        break;
    case EditorViewType::cmTimerView:
        //: Undo/redo menu text for moving a timer to a different folder
        typeName = QObject::tr("timer");
        break;
    case EditorViewType::cmScriptView:
        //: Undo/redo menu text for moving a script to a different folder
        typeName = QObject::tr("script");
        break;
    case EditorViewType::cmKeysView:
        //: Undo/redo menu text for moving a key binding to a different folder
        typeName = QObject::tr("key");
        break;
    case EditorViewType::cmActionView:
        //: Undo/redo menu text for moving a button to a different toolbar
        typeName = QObject::tr("button");
        break;
    default:
        //: Undo/redo menu text for moving an unknown item to a different location
        typeName = QObject::tr("item");
        break;
    }

    //: Undo/redo menu text for moving an item. %1 = item type (e.g. "trigger"), %2 = item name
    return QObject::tr("Move %1 \"%2\"").arg(typeName, itemName);
}
