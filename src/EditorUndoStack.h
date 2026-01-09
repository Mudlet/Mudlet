#ifndef MUDLET_MUDLETUNDOSTACK_H
#define MUDLET_MUDLETUNDOSTACK_H

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

#include <QList>
#include <QMap>
#include <QUndoStack>

#include "EditorCommand.h"

// Wrapper around QUndoStack with Mudlet-specific features:
// - itemsChanged signal for targeted UI updates
// - remapItemIDs() for ID tracking when items are recreated
// - Overridden undo()/redo() to emit custom signals
class EditorUndoStack : public QUndoStack
{
    Q_OBJECT

public:
    explicit EditorUndoStack(QObject* parent = nullptr);

    // Updates all commands on the stack when an item gets a new ID after undo/redo
    void remapItemIDs(int oldID, int newID);

    // Wrapper for push() to track when we're adding a new command vs undo/redo
    void pushCommand(QUndoCommand* cmd);

    // Override beginMacro and endMacro to track macro operations
    void beginMacro(const QString& text);
    void endMacro();

    // Override undo to handle ID remapping for DeleteItemCommand
    void undo();

    // Override redo to handle ID remapping for AddItemCommand
    void redo();

    // Returns false if last command was skipped due to Lua API conflicts
    bool wasLastCommandValid() const;

    // Get the most recently executed command (either undo or redo)
    // Returns nullptr if no commands have been executed yet
    //
    // LIFETIME WARNING: The returned pointer is owned by QUndoStack and becomes invalid if:
    // - The stack is cleared (clear())
    // - The command is removed
    // - Any stack modification occurs (push, beginMacro, etc.)
    //
    // The pointer MUST be used immediately and MUST NOT be stored.
    // Usage pattern:
    //   if (auto* cmd = dynamic_cast<const SomeCommand*>(stack->getLastExecutedCommand())) {
    //       cmd->doSomething();  // Use immediately, don't store
    //   }
    const QUndoCommand* getLastExecutedCommand() const;

signals:
    // Emitted when items are modified by undo/redo (allows targeted UI updates instead of full refresh)
    void itemsChanged(EditorViewType viewType, QList<int> affectedItemIDs);

protected:
    // Expose protected command() method for accessing commands on the stack
    using QUndoStack::command;

private:
    // Helper to emit itemsChanged for a command and its children (collects all items first)
    void emitChangesForCommand(const QUndoCommand* cmd);

    // Helper to recursively collect affected items from a command and its children
    void collectAffectedItems(const QUndoCommand* cmd, QMap<EditorViewType, QList<int>>& affectedItemsByView);

    enum class LastOperationType { None, Undo, Redo };

    int mPreviousIndex = 0;                                         // Track previous index to determine undo vs redo
    bool mInPushOperation = false;                                  // Track if we're currently pushing a command
    bool mInMacroPush = false;                                      // Track if we're in a macro push operation (beginMacro -> endMacro)
    LastOperationType mLastOperationType = LastOperationType::None; // Track last operation type for wasLastCommandValid()
};

#endif // MUDLET_MUDLETUNDOSTACK_H
