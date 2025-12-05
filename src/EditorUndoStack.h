#ifndef MUDLET_EDITORUNDOSTACK_H
#define MUDLET_EDITORUNDOSTACK_H

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

#include <QUndoStack>

class Host;

// Custom Undo Stack for the Editor.
// Can be extended to handle specific signals or logic if needed.
class EditorUndoStack : public QUndoStack
{
    Q_OBJECT

public:
    explicit EditorUndoStack(Host* host, QObject* parent = nullptr);
    ~EditorUndoStack() override = default;

private:
    Host* mpHost;
};

#endif // MUDLET_EDITORUNDOSTACK_H
