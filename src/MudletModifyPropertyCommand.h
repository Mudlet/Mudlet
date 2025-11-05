#ifndef MUDLET_MUDLETMODIFYPROPERTYCOMMAND_H
#define MUDLET_MUDLETMODIFYPROPERTYCOMMAND_H

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

#include "MudletEditorCommand.h"

#include <QString>

// Undo command for modifying item properties. Stores complete XML snapshots of old and new states.
// Updates items in place (no ID changes). No skip-first-redo needed (change already applied).
class MudletModifyPropertyCommand : public MudletEditorCommand
{
public:
    MudletModifyPropertyCommand(EditorViewType viewType, int itemID, const QString& itemName, const QString& oldStateXML, const QString& newStateXML, Host* host);

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
