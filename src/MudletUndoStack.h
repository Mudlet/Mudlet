#ifndef MUDLET_MUDLETUNDOSTACK_H
#define MUDLET_MUDLETUNDOSTACK_H

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

#include "EditorViewType.h"

#include <QList>
#include <QMap>
#include <QUndoStack>

/*!
 * \brief Wrapper around Qt's QUndoStack with Mudlet-specific features
 *
 * Extends QUndoStack to add:
 * - itemsChanged signal for targeted UI updates
 * - remapItemIDs() method for ID tracking when items are recreated
 * - Overridden undo()/redo() to emit custom signals
 *
 * This class replaces the custom EditorUndoSystem as part of the migration
 * to Qt's built-in undo framework.
 */
class MudletUndoStack : public QUndoStack
{
    Q_OBJECT

public:
    explicit MudletUndoStack(QObject* parent = nullptr);

    /*!
     * \brief Remaps item IDs in all commands on the stack
     *
     * When an item is deleted and restored via undo, it gets a new ID.
     * This method updates all commands on the undo/redo stacks to reflect
     * the ID change.
     *
     * \param oldID The original item ID
     * \param newID The new item ID after recreation
     */
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

signals:
    /*!
     * \brief Emitted when items are modified by undo/redo operations
     *
     * This signal allows the UI to refresh only the affected items,
     * rather than refreshing the entire tree.
     *
     * \param viewType The editor view type (triggers, aliases, etc.)
     * \param affectedItemIDs List of item IDs that were modified
     */
    void itemsChanged(EditorViewType viewType, QList<int> affectedItemIDs);

protected:
    // Expose protected command() method for accessing commands on the stack
    using QUndoStack::command;

private:
    // Helper to emit itemsChanged for a command and its children (collects all items first)
    void emitChangesForCommand(const QUndoCommand* cmd);

    // Helper to recursively collect affected items from a command and its children
    void collectAffectedItems(const QUndoCommand* cmd, QMap<EditorViewType, QList<int>>& affectedItemsByView);

    int mPreviousIndex = 0;  // Track previous index to determine undo vs redo
    bool mInPushOperation = false;  // Track if we're currently pushing a command
    bool mInMacroPush = false;  // Track if we're in a macro push operation (beginMacro -> endMacro)
};

#endif // MUDLET_MUDLETUNDOSTACK_H
