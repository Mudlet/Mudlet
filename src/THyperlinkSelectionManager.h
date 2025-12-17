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
#include <QPair>
#include <QString>

class TConsole;

// Manages selection state for OSC 8 hyperlinks with selection support
// Handles group exclusivity (radio button behavior) and multi-select (checkbox behavior)
// Maintains per-console selection state and group membership tracking
class THyperlinkSelectionManager : public QObject
{
    Q_OBJECT

public:
    explicit THyperlinkSelectionManager(TConsole* pConsole);
    ~THyperlinkSelectionManager();

    // Core selection operations
    bool isSelected(const QString& group, const QString& value) const;
    void setSelected(const QString& group, const QString& value, bool selected, bool exclusive);
    void toggleSelection(const QString& group, const QString& value, bool exclusive);
    
    // Group management
    QStringList getGroupMembers(const QString& group) const;
    void clearGroup(const QString& group);
    void clearAllSelections();

    // URI modification before execution
    QString modifyUriForSelection(const QString& baseUri, bool isSelected) const;

private:
    TConsole* mpConsole;
    
    // Selection state tracking: group -> (value -> selected)
    QHash<QString, QHash<QString, bool>> mSelectionState;
    
    // Group membership tracking: group -> list of values
    QHash<QString, QStringList> mGroupMembers;
    
    // Register a value in a group (called when link is created)
    void registerGroupMember(const QString& group, const QString& value);
    
    // Handle exclusive selection (radio button: deselect others in group)
    void handleExclusiveSelection(const QString& group, const QString& value);
};

#endif // MUDLET_THYPERLINKSELECTIONMANAGER_H
