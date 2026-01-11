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

#include "EditorUndoStack.h"
#include "EditorAddItemCommand.h"
#include "EditorDeleteItemCommand.h"
#include "EditorCommand.h"

#include <QDebug>
#include <typeinfo>

EditorUndoStack::EditorUndoStack(QObject* parent) : QUndoStack(parent)
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

void EditorUndoStack::emitChangesForCommand(const QUndoCommand* cmd)
{
    if (!cmd) {
        return;
    }

#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorUndoStack::emitChangesForCommand() - Processing command:" << cmd->text();
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
#if defined(DEBUG_UNDO_REDO)
                    qWarning() << "EditorUndoStack::emitChangesForCommand() - Invalid item ID" << id << "found, skipping emission";
#endif
                    break;
                }
            }
            if (allValid) {
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "EditorUndoStack::emitChangesForCommand() - Emitting itemsChanged for view type" << static_cast<int>(it.key()) << "with" << itemIDs.size() << "items:" << itemIDs;
#endif
                emit itemsChanged(it.key(), itemIDs);
            }
        }
    }
}

void EditorUndoStack::collectAffectedItems(const QUndoCommand* cmd, QMap<EditorViewType, QList<int>>& affectedItemsByView)
{
    if (!cmd) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorUndoStack::collectAffectedItems() - Null command pointer";
#endif
        return;
    }

#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorUndoStack::collectAffectedItems() - Examining command:" << cmd->text()
             << "pointer:" << static_cast<const void*>(cmd)
             << "type:" << typeid(*cmd).name();
#endif

    // If this is a EditorCommand, collect its affected items
    if (auto* mudletCmd = dynamic_cast<const EditorCommand*>(cmd)) {
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

void EditorUndoStack::pushCommand(QUndoCommand* cmd)
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorUndoStack::pushCommand() - Pushing command:" << (cmd ? cmd->text() : QStringLiteral("null"));
#endif
    // Set flag to indicate we're in a push operation
    mInPushOperation = true;
    push(cmd);
    mInPushOperation = false;
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorUndoStack::pushCommand() - Stack now has" << count() << "commands, index:" << index();
#endif
}

void EditorUndoStack::beginMacro(const QString& text)
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorUndoStack::beginMacro() - Starting macro:" << text;
#endif
    // Set flag to indicate we're starting a macro push operation
    mInMacroPush = true;
    QUndoStack::beginMacro(text);
}

void EditorUndoStack::endMacro()
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorUndoStack::endMacro() - Ending macro, stack has" << count() << "commands";
#endif
    // Call the base implementation first
    QUndoStack::endMacro();

    // Clear the flag after the macro is complete
    mInMacroPush = false;
}

void EditorUndoStack::undo()
{
    // Track that we're performing an undo operation
    mLastOperationType = LastOperationType::Undo;

    // Get the command that will be undone (if any)
    if (index() > 0) {
        const QUndoCommand* cmd = command(index() - 1);
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorUndoStack::undo() - Undoing command:" << (cmd ? cmd->text() : QStringLiteral("null")) << "at index" << (index() - 1);
        qDebug() << "EditorUndoStack::undo() - Command pointer:" << static_cast<const void*>(cmd) << "Type info:" << (cmd ? typeid(*cmd).name() : "null");
        qDebug() << "EditorUndoStack::undo() - Command has" << (cmd ? cmd->childCount() : 0) << "children";
#endif

        // Check if this is a DeleteItemCommand BEFORE calling undo (matching redo pattern)
        // This prevents accessing potentially invalidated memory after the undo operation
        const EditorDeleteItemCommand* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(cmd);
#if defined(DEBUG_UNDO_REDO)
        if (deleteCmd) {
            qDebug() << "EditorUndoStack::undo() - Command is a DeleteItemCommand, will need ID remapping";
        }
#endif

        // Call the base class undo
        QUndoStack::undo();

#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorUndoStack::undo() - Base undo completed, new index:" << index();
#endif

        // Check if this is a DeleteItemCommand that restored items with new IDs
        if (deleteCmd) {
            QList<QPair<int, int>> idChanges = deleteCmd->getIDChanges();
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorUndoStack::undo() - DeleteItemCommand restored items with ID changes:" << idChanges.size();
#endif
            for (const auto& change : idChanges) {
                int oldID = change.first;
                int newID = change.second;
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "EditorUndoStack::undo() - Remapping ID" << oldID << "->" << newID;
#endif
                remapItemIDs(oldID, newID);
            }
        }
    } else {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorUndoStack::undo() - No command to undo (index is 0)";
#endif
        // No command to undo
        QUndoStack::undo();
    }
}

void EditorUndoStack::redo()
{
    // Track that we're performing a redo operation
    mLastOperationType = LastOperationType::Redo;

    // Get the command that will be redone (if any)
    if (index() < count()) {
        const QUndoCommand* cmd = command(index());
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorUndoStack::redo() - Redoing command:" << (cmd ? cmd->text() : QStringLiteral("null")) << "at index" << index();
#endif

        // Check if this is an AddItemCommand (need to check before redo since ID may change)
        // Note: AddItemCommands might be wrapped in a macro with ModifyPropertyCommand
        const EditorAddItemCommand* addCmd = dynamic_cast<const EditorAddItemCommand*>(cmd);

        // If not a direct AddItemCommand, check if it's a macro containing one
        if (!addCmd && cmd->childCount() > 0) {
            const QUndoCommand* firstChild = cmd->child(0);
            if (firstChild) {
                addCmd = dynamic_cast<const EditorAddItemCommand*>(firstChild);
            }
        }

        int oldItemID = -1;
        if (addCmd) {
            oldItemID = addCmd->getNewItemID();
        }

        // Call the base class redo
        QUndoStack::redo();

        // Check if this is an AddItemCommand that restored items with new IDs
        if (addCmd) {
            QList<QPair<int, int>> idChanges = addCmd->getIDChanges();
            if (!idChanges.isEmpty()) {
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "EditorUndoStack::redo() - AddItemCommand restored items with ID changes:" << idChanges.size();
#endif
                for (const auto& change : idChanges) {
                    int oldID = change.first;
                    int newID = change.second;
#if defined(DEBUG_UNDO_REDO)
                    qDebug() << "EditorUndoStack::redo() - Remapping ID" << oldID << "->" << newID;
#endif
                    remapItemIDs(oldID, newID);
                }
            }
        }
    } else {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorUndoStack::redo() - No command to redo (index >= count)";
#endif
        // No command to redo
        QUndoStack::redo();
    }
}

// Updates all stored item IDs across the entire undo stack when an item gets recreated with a new ID
void EditorUndoStack::remapItemIDs(int oldID, int newID)
{
#if defined(DEBUG_UNDO_REDO)
    qDebug() << "EditorUndoStack::remapItemIDs() - Remapping" << oldID << "->" << newID << "across" << count() << "commands";
#endif
    // Helper lambda to recursively remap IDs in a command and all its children
    std::function<void(const QUndoCommand*)> remapRecursive = [&](const QUndoCommand* cmd) {
        if (!cmd) {
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorUndoStack::remapItemIDs() - Null command in recursive remap";
#endif
            return;
        }

#if defined(DEBUG_UNDO_REDO)
        qDebug() << "EditorUndoStack::remapItemIDs() - Remapping in command:" << cmd->text()
                 << "pointer:" << static_cast<const void*>(cmd);
#endif

        // Remap this command
        if (auto* mudletCmd = dynamic_cast<EditorCommand*>(const_cast<QUndoCommand*>(cmd))) {
            mudletCmd->remapItemID(oldID, newID);
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "EditorUndoStack::remapItemIDs() - Successfully remapped in EditorCommand";
#endif
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

bool EditorUndoStack::wasLastCommandValid() const
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

    // Check if it's a EditorDeleteItemCommand and query its validity
    if (auto* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(cmd)) {
        return deleteCmd->wasValid();
    }

    // Other command types are always valid (no Lua conflict detection yet)
    return true;
}

const QUndoCommand* EditorUndoStack::getLastExecutedCommand() const
{
    if (mLastOperationType == LastOperationType::Undo) {
        // After undo: index() points to the next command to redo
        if (index() >= 0 && index() < count()) {
            return command(index());
        }
    } else if (mLastOperationType == LastOperationType::Redo) {
        // After redo: index() points to the command after the one that was just redone
        if (index() > 0 && index() <= count()) {
            return command(index() - 1);
        }
    }
    return nullptr;
}
