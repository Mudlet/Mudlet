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

// Manages selection state for OSC 8 hyperlinks with selection support
// Handles group exclusivity (radio button behavior) and multi-select (checkbox behavior)  
// Maintains per-console selection state and group membership tracking
//
// Ownership and lifetime:
// - The manager does not take ownership of the TConsole
// - Caller must ensure the TConsole outlives this manager instance
// - TConsole reference must be valid throughout the manager's lifetime
class THyperlinkSelectionManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(THyperlinkSelectionManager)

public:
    // Constructor: Associates this manager with a specific console instance
    // @param console: TConsole reference that must outlive this manager (not null)
    explicit THyperlinkSelectionManager(TConsole& console);
    ~THyperlinkSelectionManager();

    // Core selection operations
    bool isSelected(const QString& group, const QString& value) const;
    void setSelected(const QString& group, const QString& value, bool selected);
    void toggleSelection(const QString& group, const QString& value);
    
    // Group management
    QStringList getGroupMembers(const QString& group) const;
    void clearGroup(const QString& group);
    void clearAllSelections();
    
    // Group exclusivity configuration
    // Sets whether a group should enforce exclusive (radio button) or multi-select (checkbox) behavior
    void setGroupExclusive(const QString& group, bool exclusive);
    bool isGroupExclusive(const QString& group) const;

    // URI modification before execution
    // Transforms a hyperlink URI to include selection state metadata
    //
    // When invoked:
    // - Call before executing a hyperlink (e.g., when user clicks on a selectable link)
    // - Typically used in hyperlink click handlers to inject runtime selection state
    //
    // Transformations applied:
    // - For send([[command]]) format: Appends "?selected=true|false" or "&selected=true|false"
    //   (uses '?' if no query params exist, '&' if already present)
    // - For sendCmdLine([[command]]) format: Same query parameter injection as send()
    // - For other URI formats (e.g., openUrl): Returns unchanged
    //
    // Why necessary:
    // - Allows commands to behave differently based on whether link is selected/unselected
    // - Supports checkbox/radio button semantics in terminal hyperlinks
    //
    // @param baseUri: Original URI string from the hyperlink definition
    // @param group: Selection group identifier
    // @param value: Selection value identifier
    // @return Modified URI with selection state appended (for send/sendCmdLine formats),
    //         or unchanged baseUri (for other formats). Empty input returns empty output.
    QString modifyUriForSelection(const QString& baseUri, const QString& group, const QString& value) const;

signals:
    // Emitted when a selection state changes
    void selectionChanged(const QString& group, const QString& value, bool selected);
    
    // Emitted when all selections in a group are cleared
    void groupCleared(const QString& group);
    
    // Emitted when all selections across all groups are cleared
    void allSelectionsCleared();

private:
    TConsole* mpConsole;
    
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
