#ifndef MUDLET_THYPERLINKSELECTIONMANAGER_H
#define MUDLET_THYPERLINKSELECTIONMANAGER_H

/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

class TConsole;

// Manages selection state for OSC 8 hyperlinks
// Handles group exclusivity (radio buttons) and multi-select (checkboxes)
class THyperlinkSelectionManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(THyperlinkSelectionManager)

public:
    explicit THyperlinkSelectionManager(TConsole& console);
    ~THyperlinkSelectionManager();

    bool isSelected(const QString& group, const QString& value) const;
    void setSelected(const QString& group, const QString& value, bool selected);
    void toggleSelection(const QString& group, const QString& value);
    
    QStringList getGroupMembers(const QString& group) const;
    void clearGroup(const QString& group);
    void clearAllSelections();
    
    void setGroupExclusive(const QString& group, bool exclusive);
    bool isGroupExclusive(const QString& group) const;

    // Modifies hyperlink URI to include current selection state
    QString modifyUriForSelection(const QString& baseUri, const QString& group, const QString& value) const;

signals:
    void selectionChanged(const QString& group, const QString& value, bool selected);
    void groupCleared(const QString& group);
    void allSelectionsCleared();

private:
    QString addSelectedParameter(const QString& command, bool isSelected) const;
    
    TConsole& mpConsole;
    
    // Selection state tracking: group -> (value -> selected)
    QHash<QString, QHash<QString, bool>> mSelectionState;
    
    // Group membership tracking: group -> set of values (O(1) membership checks)
    QHash<QString, QSet<QString>> mGroupMembers;
    
    // Group exclusivity settings: group -> exclusive mode (true = radio, false = checkbox)
    QHash<QString, bool> mGroupExclusivity;
    
    // Register a value in a group (called when link is created)
    void registerGroupMember(const QString& group, const QString& value);
    
    // Handle exclusive selection (radio button: deselect others in group)
    void handleExclusiveSelection(const QString& group, const QString& value);
};

#endif // MUDLET_THYPERLINKSELECTIONMANAGER_H
