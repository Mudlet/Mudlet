#ifndef MUDLET_MUDLETTOGGLEACTIVECOMMAND_H
#define MUDLET_MUDLETTOGGLEACTIVECOMMAND_H

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

#include "EditorCommand.h"

// Undo command for toggling active/inactive state of items (triggers, aliases, timers, etc.)
class EditorToggleActiveCommand : public EditorCommand
{
public:
    EditorToggleActiveCommand(EditorViewType viewType, int itemID, bool oldState, bool newState, const QString& itemName, Host* host);

    void undo() override;
    void redo() override;

    EditorViewType viewType() const override { return mViewType; }
    QList<int> affectedItemIDs() const override { return {mItemID}; }

    void remapItemID(int oldID, int newID) override;

private:
    void setItemActiveState(int itemID, bool active);
    static QString generateText(EditorViewType viewType, const QString& itemName, bool newState);

    EditorViewType mViewType;
    int mItemID;
    bool mOldActiveState;
    bool mNewActiveState;
    QString mItemName;
    mutable bool mSkipFirstRedo = true; // Skip initial redo() called by QUndoStack::push()
};

#endif // MUDLET_MUDLETTOGGLEACTIVECOMMAND_H
