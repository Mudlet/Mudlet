#ifndef MUDLET_MUDLETCOMMAND_H
#define MUDLET_MUDLETCOMMAND_H

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
#include <QUndoCommand>

class Host;

/*!
 * \brief Base class for all Mudlet undo commands using Qt's QUndoStack framework
 *
 * Extends QUndoCommand with Mudlet-specific functionality:
 * - EditorViewType tracking (which editor view triggered the command)
 * - Affected item IDs (for targeted UI updates)
 * - Item ID remapping (when items are deleted and recreated with new IDs)
 *
 * This class replaces the custom EditorCommand base class as part of the
 * migration to Qt's built-in undo framework.
 */
class MudletCommand : public QUndoCommand
{
public:
    explicit MudletCommand(Host* host, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), mpHost(host) {}

    explicit MudletCommand(const QString& text, Host* host, QUndoCommand* parent = nullptr)
        : QUndoCommand(text, parent), mpHost(host) {}

    ~MudletCommand() override = default;

    // Mudlet-specific extensions

    /*!
     * \brief Returns the editor view type that triggered this command
     * \return The view type (triggers, aliases, timers, etc.)
     */
    virtual EditorViewType viewType() const = 0;

    /*!
     * \brief Returns the list of item IDs affected by this command
     * \return List of item IDs for targeted UI updates
     */
    virtual QList<int> affectedItemIDs() const = 0;

    /*!
     * \brief Remaps an item ID when an item is deleted and recreated
     *
     * When an item is deleted and then restored via undo, it gets a new ID.
     * This method updates any stored IDs in the command to reflect the change.
     *
     * \param oldID The original item ID
     * \param newID The new item ID after recreation
     */
    virtual void remapItemID(int oldID, int newID) = 0;

    // QUndoCommand overrides (kept pure virtual to enforce implementation)
    void undo() override = 0;
    void redo() override = 0;

protected:
    Host* mpHost;
};

#endif // MUDLET_MUDLETCOMMAND_H
