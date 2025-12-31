#ifndef MUDLET_NOTESTABCONTAINER_H
#define MUDLET_NOTESTABCONTAINER_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2025 by Mudlet Development Team                         *
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

#include <QPointer>
#include <QWidget>

class Host;
class dlgNotepad;
class QVBoxLayout;

class NotesTabContainer : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(NotesTabContainer)
    explicit NotesTabContainer(Host* pH, QWidget* parent = nullptr);
    ~NotesTabContainer();

    // Access the notes widget
    dlgNotepad* getNotesWidget() const { return mpNotesWidget; }
    
    // Show/hide the notes tab
    void showNotesTab();
    void hideNotesTab();
    
    // Font management
    void setFont(const QFont& font);
    
    // State management
    void saveState();
    void restoreState();

signals:
    void notesTabFocused();
    void notesTabHidden();

private:
    void setupUi();
    void initConnections();

    QPointer<Host> mpHost;
    dlgNotepad* mpNotesWidget = nullptr;
    QVBoxLayout* mpLayout = nullptr;
};

#endif // MUDLET_NOTESTABCONTAINER_H
