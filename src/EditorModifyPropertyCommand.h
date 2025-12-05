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

#ifndef EDITOR_MODIFY_PROPERTY_COMMAND_H
#define EDITOR_MODIFY_PROPERTY_COMMAND_H

#include "EditorCommand.h"

class EditorModifyPropertyCommand : public EditorCommand
{
public:
    EditorModifyPropertyCommand(EditorViewType viewType, int itemID, const QString& itemName, const QString& oldXml, const QString& newXml, Host* host);

    void undo() override;
    void redo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

    QList<int> affectedItemIDs() const override;
    void remapItemID(int oldID, int newID) override;

private:
    EditorViewType mViewType;
    int mItemID;
    QString mItemName;
    QString mOldXmlSnapshot;
    QString mNewXmlSnapshot;

    static QString generateText(EditorViewType viewType, const QString& itemName);
};

#endif // EDITOR_MODIFY_PROPERTY_COMMAND_H
