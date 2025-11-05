#ifndef MUDLET_MUDLETEDITORCOMMAND_H
#define MUDLET_MUDLETEDITORCOMMAND_H

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
#include <QObject>
#include <QUndoCommand>

class Host;

// Namespace wrapper for EditorViewType to enable Q_ENUM_NS
namespace EditorViewTypes {
Q_NAMESPACE

// Editor view type enum - used by commands and dlgTriggerEditor
enum class EditorViewType { cmUnknownView = 0, cmTriggerView = 0x01, cmTimerView = 0x02, cmAliasView = 0x03, cmScriptView = 0x04, cmActionView = 0x05, cmKeysView = 0x06, cmVarsView = 0x07 };
Q_ENUM_NS(EditorViewType)

} // namespace EditorViewTypes

using EditorViewTypes::EditorViewType;

// Base class for editor undo commands. Extends QUndoCommand with:
// - EditorViewType tracking (which editor view triggered the command)
// - Affected item IDs (for targeted UI updates)
// - Item ID remapping (when items are deleted and recreated with new IDs)
class EditorCommand : public QUndoCommand
{
public:
    explicit EditorCommand(Host* host, QUndoCommand* parent = nullptr) : QUndoCommand(parent), mpHost(host) {}

    explicit EditorCommand(const QString& text, Host* host, QUndoCommand* parent = nullptr) : QUndoCommand(text, parent), mpHost(host) {}

    ~EditorCommand() override = default;

    // Returns which editor view type this command belongs to (triggers, aliases, etc.)
    virtual EditorViewType viewType() const = 0;

    // Returns list of item IDs affected by this command (for targeted UI updates)
    virtual QList<int> affectedItemIDs() const = 0;

    // Updates stored IDs when items are deleted and recreated with new IDs
    virtual void remapItemID(int oldID, int newID) = 0;

    // QUndoCommand overrides (kept pure virtual to enforce implementation)
    void undo() override = 0;
    void redo() override = 0;

protected:
    Host* mpHost;
};

#endif // MUDLET_MUDLETEDITORCOMMAND_H
