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

#include "MudletUndoStack.h"
#include "MudletEditorCommand.h"
#include "MudletAddItemCommand.h"
#include "MudletDeleteItemCommand.h"

#include <QDebug>

MudletUndoStack::MudletUndoStack(QObject* parent)
    : QUndoStack(parent)
{
    // Connect to indexChanged signal to emit itemsChanged after undo/redo
    connect(this, &QUndoStack::indexChanged, this, [this](int newIndex) {
        // Skip emitting itemsChanged during push() operations or macro push operations
        // The action has already been performed before pushing
        if (mInPushOperation || mInMacroPush) {
            mPreviousIndex = newIndex;
            return;
        }

        // Determine which command was affected based on index movement
        int affectedCommandIndex = -1;

        if (newIndex > mPreviousIndex) {
            // Redo: index increased, emit for command that was redone
            affectedCommandIndex = newIndex - 1;
        } else if (newIndex < mPreviousIndex) {
            // Undo: index decreased, emit for command that was undone
            affectedCommandIndex = mPreviousIndex - 1;
        }

        // Emit itemsChanged for the affected command (including all children for macros)
        if (affectedCommandIndex >= 0 && affectedCommandIndex < count()) {
            const QUndoCommand* cmd = command(affectedCommandIndex);
            emitChangesForCommand(cmd);
        }

        // Update previous index for next change
        mPreviousIndex = newIndex;
    });
}

void MudletUndoStack::emitChangesForCommand(const QUndoCommand* cmd)
{
    if (!cmd) {
        return;
    }

#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletUndoStack::emitChangesForCommand() - Processing command:" << cmd->text();
#endif

    // Collect all affected items by view type from this command and all children
    QMap<EditorViewType, QList<int>> affectedItemsByView;
    collectAffectedItems(cmd, affectedItemsByView);

    // Only emit if we have valid items
    // Emit itemsChanged once per view type with all affected IDs
    for (auto it = affectedItemsByView.constBegin(); it != affectedItemsByView.constEnd(); ++it) {
        const QList<int>& itemIDs = it.value();
        if (!itemIDs.isEmpty()) {
            // Double-check all IDs are valid before emitting
            bool allValid = true;
            for (int id : itemIDs) {
                if (id <= 0) {
                    allValid = false;
                    qWarning() << "MudletUndoStack::emitChangesForCommand() - Invalid item ID" << id << "found, skipping emission";
                    break;
                }
            }
            if (allValid) {
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "MudletUndoStack::emitChangesForCommand() - Emitting itemsChanged for view type"
                         << static_cast<int>(it.key()) << "with" << itemIDs.size() << "items:" << itemIDs;
#endif
                emit itemsChanged(it.key(), itemIDs);
            }
        }
    }
}

void MudletUndoStack::collectAffectedItems(const QUndoCommand* cmd, QMap<EditorViewType, QList<int>>& affectedItemsByView)
{
    if (!cmd) {
        return;
    }

    // If this is a MudletEditorCommand, collect its affected items
    if (auto* mudletCmd = dynamic_cast<const MudletEditorCommand*>(cmd)) {
        EditorViewType viewType = mudletCmd->viewType();
        QList<int> itemIDs = mudletCmd->affectedItemIDs();

        // Add to the map, avoiding duplicates and invalid IDs
        for (int id : itemIDs) {
            // Skip invalid IDs (0 or negative)
            if (id <= 0) {
                continue;
            }
            if (!affectedItemsByView[viewType].contains(id)) {
                affectedItemsByView[viewType].append(id);
            }
        }
    }

    // Recursively collect from child commands (for macros)
    for (int i = 0; i < cmd->childCount(); ++i) {
        collectAffectedItems(cmd->child(i), affectedItemsByView);
    }
}

void MudletUndoStack::pushCommand(QUndoCommand* cmd)
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletUndoStack::pushCommand() - Pushing command:" << (cmd ? cmd->text() : QStringLiteral("null"));
#endif
    // Set flag to indicate we're in a push operation
    mInPushOperation = true;
    push(cmd);
    mInPushOperation = false;
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletUndoStack::pushCommand() - Stack now has" << count() << "commands, index:" << index();
#endif
}

void MudletUndoStack::beginMacro(const QString& text)
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletUndoStack::beginMacro() - Starting macro:" << text;
#endif
    // Set flag to indicate we're starting a macro push operation
    mInMacroPush = true;
    QUndoStack::beginMacro(text);
}

void MudletUndoStack::endMacro()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletUndoStack::endMacro() - Ending macro, stack has" << count() << "commands";
#endif
    // Call the base implementation first
    QUndoStack::endMacro();

    // Clear the flag after the macro is complete
    mInMacroPush = false;
}

void MudletUndoStack::undo()
{
    // Track that we're performing an undo operation
    mLastOperationType = LastOperationType::Undo;

    // Get the command that will be undone (if any)
    if (index() > 0) {
        const QUndoCommand* cmd = command(index() - 1);
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "MudletUndoStack::undo() - Undoing command:" << (cmd ? cmd->text() : QStringLiteral("null"))
                 << "at index" << (index() - 1);
#endif

        // Call the base class undo
        QUndoStack::undo();

        // Check if this is a DeleteItemCommand that restored items with new IDs
        if (auto* deleteCmd = dynamic_cast<const MudletDeleteItemCommand*>(cmd)) {
            QList<QPair<int, int>> idChanges = deleteCmd->getIDChanges();
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "MudletUndoStack::undo() - DeleteItemCommand restored items with ID changes:" << idChanges.size();
#endif
            for (const auto& change : idChanges) {
                int oldID = change.first;
                int newID = change.second;
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "MudletUndoStack::undo() - Remapping ID" << oldID << "->" << newID;
#endif
                remapItemIDs(oldID, newID);
            }
        }
    } else {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "MudletUndoStack::undo() - No command to undo (index is 0)";
#endif
        // No command to undo
        QUndoStack::undo();
    }
}

void MudletUndoStack::redo()
{
    // Track that we're performing a redo operation
    mLastOperationType = LastOperationType::Redo;

    // Get the command that will be redone (if any)
    if (index() < count()) {
        const QUndoCommand* cmd = command(index());
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "MudletUndoStack::redo() - Redoing command:" << (cmd ? cmd->text() : QStringLiteral("null"))
                 << "at index" << index();
#endif

        // Check if this is an AddItemCommand (need to check before redo since ID may change)
        // Note: AddItemCommands might be wrapped in a macro with ModifyPropertyCommand
        const MudletAddItemCommand* addCmd = dynamic_cast<const MudletAddItemCommand*>(cmd);

        // If not a direct AddItemCommand, check if it's a macro containing one
        if (!addCmd && cmd->childCount() > 0) {
            const QUndoCommand* firstChild = cmd->child(0);
            if (firstChild) {
                addCmd = dynamic_cast<const MudletAddItemCommand*>(firstChild);
            }
        }

        int oldItemID = -1;
        if (addCmd) {
            oldItemID = addCmd->getNewItemID();
        }

        // Call the base class redo
        QUndoStack::redo();

        // Check if the item ID changed during redo
        if (addCmd && addCmd->didItemIDChange()) {
            int newItemID = addCmd->getNewItemID();
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "MudletUndoStack::redo() - AddItemCommand ID changed:" << oldItemID << "->" << newItemID;
#endif
            remapItemIDs(oldItemID, newItemID);
        }
    } else {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "MudletUndoStack::redo() - No command to redo (index >= count)";
#endif
        // No command to redo
        QUndoStack::redo();
    }
}

// Updates all stored item IDs across the entire undo stack when an item gets recreated with a new ID
void MudletUndoStack::remapItemIDs(int oldID, int newID)
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "MudletUndoStack::remapItemIDs() - Remapping" << oldID << "->" << newID << "across" << count() << "commands";
#endif
    // Helper lambda to recursively remap IDs in a command and all its children
    std::function<void(const QUndoCommand*)> remapRecursive = [&](const QUndoCommand* cmd) {
        if (!cmd) {
            return;
        }

        // Remap this command
        if (auto* mudletCmd = dynamic_cast<MudletEditorCommand*>(const_cast<QUndoCommand*>(cmd))) {
            mudletCmd->remapItemID(oldID, newID);
        }

        // Recursively remap all child commands (for macros)
        for (int i = 0; i < cmd->childCount(); ++i) {
            remapRecursive(cmd->child(i));
        }
    };

    // Iterate through all commands on both undo and redo stacks
    // The stack contains commands from index 0 to count()-1
    for (int i = 0; i < count(); ++i) {
        const QUndoCommand* cmd = command(i);
        remapRecursive(cmd);
    }
}

bool MudletUndoStack::wasLastCommandValid() const
{
    // Determine which command was just executed based on the operation type
    int lastCommandIndex = -1;

    if (mLastOperationType == LastOperationType::Undo) {
        // After undo: index() points to the next command to redo, which is the command that was just undone
        lastCommandIndex = index();
    } else if (mLastOperationType == LastOperationType::Redo) {
        // After redo: index() points to the command after the one that was just redone
        lastCommandIndex = index() - 1;
    } else {
        // No operation performed yet
        return true;
    }

    if (lastCommandIndex < 0 || lastCommandIndex >= count()) {
        return true; // No command to check, consider it valid
    }

    const QUndoCommand* cmd = command(lastCommandIndex);
    if (!cmd) {
        return true;
    }

    // Check if it's a MudletDeleteItemCommand and query its validity
    if (auto* deleteCmd = dynamic_cast<const MudletDeleteItemCommand*>(cmd)) {
        return deleteCmd->wasValid();
    }

    // Other command types are always valid (no Lua conflict detection yet)
    return true;
}
