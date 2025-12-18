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

#include "THyperlinkSelectionManager.h"
#include "TConsole.h"

THyperlinkSelectionManager::THyperlinkSelectionManager(TConsole* pConsole)
: QObject(pConsole)
, mpConsole(pConsole)
{
}

THyperlinkSelectionManager::~THyperlinkSelectionManager() = default;

// Check if a value in a group is currently selected
bool THyperlinkSelectionManager::isSelected(const QString& group, const QString& value) const
{
    if (!mSelectionState.contains(group)) {
        return false;
    }
    return mSelectionState[group].value(value, false);
}

// Set selection state for a value in a group
void THyperlinkSelectionManager::setSelected(const QString& group, const QString& value, bool selected, bool exclusive)
{
    // Register this value in the group if not already registered
    registerGroupMember(group, value);
    
    // Handle exclusive selection (radio button behavior)
    if (selected && exclusive) {
        handleExclusiveSelection(group, value);
    }
    
    // Set the selection state
    mSelectionState[group][value] = selected;
}

// Toggle selection state for a value in a group
void THyperlinkSelectionManager::toggleSelection(const QString& group, const QString& value, bool exclusive)
{
    bool currentState = isSelected(group, value);
    setSelected(group, value, !currentState, exclusive);
}

// Get all values registered in a group
QStringList THyperlinkSelectionManager::getGroupMembers(const QString& group) const
{
    return mGroupMembers.value(group, QStringList());
}

// Clear all selections in a group
void THyperlinkSelectionManager::clearGroup(const QString& group)
{
    if (mSelectionState.contains(group)) {
        mSelectionState[group].clear();
    }
}

// Clear all selections across all groups
void THyperlinkSelectionManager::clearAllSelections()
{
    mSelectionState.clear();
}

// Modify URI to append &selected=true or &selected=false for server callback
QString THyperlinkSelectionManager::modifyUriForSelection(const QString& baseUri, bool isSelected) const
{
    // The baseUri is already in Lua format: send([[command]]) or sendCmdLine([[command]])
    // We need to extract the command, append &selected=true/false, and reconstruct
    
    qDebug() << "modifyUriForSelection called with baseUri:" << baseUri << "isSelected:" << isSelected;
    
    // Check if it's a send() or sendCmdLine() call
    if (baseUri.startsWith(qsl("send([[")) && baseUri.endsWith(qsl("]])"))) {
        // Extract: send([[command]]) -> command
        QString command = baseUri.mid(7, baseUri.length() - 10);
        // Append selection state to command
        QString separator = command.contains('?') ? qsl("&") : qsl("?");
        command += separator + qsl("selected=") + (isSelected ? qsl("true") : qsl("false"));
        // Reconstruct: send([[command&selected=true]])
        QString result = qsl("send([[%1]])").arg(command);
        qDebug() << "Modified to:" << result;
        return result;
    } else if (baseUri.startsWith(qsl("sendCmdLine([[")) && baseUri.endsWith(qsl("]])"))) {
        // Extract: sendCmdLine([[command]]) -> command
        QString command = baseUri.mid(14, baseUri.length() - 17);
        // Append selection state to command
        QString separator = command.contains('?') ? qsl("&") : qsl("?");
        command += separator + qsl("selected=") + (isSelected ? qsl("true") : qsl("false"));
        // Reconstruct: sendCmdLine([[command&selected=true]])
        QString result = qsl("sendCmdLine([[%1]])").arg(command);
        qDebug() << "Modified to:" << result;
        return result;
    }
    
    // For other URI formats (like openUrl), return as-is
    qDebug() << "No modification - returning as-is";
    return baseUri;
}

// Register a value as a member of a group
void THyperlinkSelectionManager::registerGroupMember(const QString& group, const QString& value)
{
    if (!mGroupMembers.contains(group)) {
        mGroupMembers[group] = QStringList();
    }
    
    if (!mGroupMembers[group].contains(value)) {
        mGroupMembers[group].append(value);
    }
}

// Handle exclusive selection: deselect all other values in the group (radio button behavior)
void THyperlinkSelectionManager::handleExclusiveSelection(const QString& group, const QString& value)
{
    // Deselect all other members of this group
    const QStringList members = getGroupMembers(group);
    for (const QString& member : members) {
        if (member != value) {
            mSelectionState[group][member] = false;
        }
    }
}
