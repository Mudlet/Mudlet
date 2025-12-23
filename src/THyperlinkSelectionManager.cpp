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

#include <QUrl>
#include <QUrlQuery>

THyperlinkSelectionManager::THyperlinkSelectionManager(TConsole& console)
: QObject(&console)
, mpConsole(console)
{
}

THyperlinkSelectionManager::~THyperlinkSelectionManager() = default;

bool THyperlinkSelectionManager::isSelected(const QString& group, const QString& value) const
{
    return mSelectionState.value(group).value(value, false);
}

void THyperlinkSelectionManager::setSelected(const QString& group, const QString& value, bool selected)
{
    registerGroupMember(group, value);
    
    bool previousState = isSelected(group, value);
    
    // If selecting and group is exclusive, clear other selections
    if (selected && isGroupExclusive(group)) {
        handleExclusiveSelection(group, value);
    }
    
    mSelectionState[group][value] = selected;
    
    if (previousState != selected) {
        emit selectionChanged(group, value, selected);
    }
}

void THyperlinkSelectionManager::toggleSelection(const QString& group, const QString& value)
{
    bool currentState = isSelected(group, value);
    setSelected(group, value, !currentState);
}

QStringList THyperlinkSelectionManager::getGroupMembers(const QString& group) const
{
    const QSet<QString>& groupSet = mGroupMembers.value(group, QSet<QString>());
    return QStringList(groupSet.begin(), groupSet.end());
}

void THyperlinkSelectionManager::clearGroup(const QString& group)
{
    if (mSelectionState.contains(group)) {
        mSelectionState[group].clear();
        emit groupCleared(group);
    }
}

void THyperlinkSelectionManager::clearAllSelections()
{
    mSelectionState.clear();
    emit allSelectionsCleared();
}

QString THyperlinkSelectionManager::addSelectedParameter(const QString& command, bool isSelected) const
{
    QUrl url(command);
    QUrlQuery query(url);
    query.removeQueryItem(qsl("selected"));
    query.addQueryItem(qsl("selected"), isSelected ? qsl("true") : qsl("false"));
    
    QString cleanCommand = url.path();
    if (!query.isEmpty()) {
        cleanCommand += qsl("?") + query.query(QUrl::FullyEncoded);
    }
    return cleanCommand;
}

QString THyperlinkSelectionManager::modifyUriForSelection(const QString& baseUri, const QString& group, const QString& value) const
{
    // Query the current selection state from our internal state
    bool isSelected = this->isSelected(group, value);
    
#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "modifyUriForSelection called with baseUri:" << baseUri << "group:" << group << "value:" << value << "isSelected:" << isSelected;
#endif
    
    // Check if it's a send() or sendCmdLine() call
    const QString sendPrefix = qsl("send([[");
    const QString sendSuffix = qsl("]])");
    const QString sendCmdLinePrefix = qsl("sendCmdLine([[");
    const QString sendCmdLineSuffix = qsl("]])");
    
    if (baseUri.startsWith(sendPrefix) && baseUri.endsWith(sendSuffix)) {
        const int prefixLength = sendPrefix.length();
        const int suffixLength = sendSuffix.length();
        QString command = baseUri.mid(prefixLength, baseUri.length() - prefixLength - suffixLength);
        QString cleanCommand = addSelectedParameter(command, isSelected);
        QString result = qsl("send([[%1]])").arg(cleanCommand);
#if defined(DEBUG_OSC_PROCESSING)
        qDebug() << "Modified to:" << result;
#endif
        return result;
    } else if (baseUri.startsWith(sendCmdLinePrefix) && baseUri.endsWith(sendCmdLineSuffix)) {
        const int prefixLength = sendCmdLinePrefix.length();
        const int suffixLength = sendCmdLineSuffix.length();
        QString command = baseUri.mid(prefixLength, baseUri.length() - prefixLength - suffixLength);
        QString cleanCommand = addSelectedParameter(command, isSelected);
        QString result = qsl("sendCmdLine([[%1]])").arg(cleanCommand);
#if defined(DEBUG_OSC_PROCESSING)
        qDebug() << "Modified to:" << result;
#endif
        return result;
    }
    
    // For other URI formats (like openUrl), return as-is
#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "No modification - returning as-is";
#endif
    return baseUri;
}

void THyperlinkSelectionManager::registerGroupMember(const QString& group, const QString& value)
{
    if (!mGroupMembers.contains(group)) {
        mGroupMembers[group] = QSet<QString>();
    }
    
    mGroupMembers[group].insert(value);
}

void THyperlinkSelectionManager::setGroupExclusive(const QString& group, bool exclusive)
{
    mGroupExclusivity[group] = exclusive;
    
    if (exclusive && mSelectionState.contains(group)) {
        QStringList members = getGroupMembers(group);
        members.sort();
        
        bool foundFirst = false;
        for (const QString& member : members) {
            if (isSelected(group, member)) {
                if (!foundFirst) {
                    foundFirst = true;
                } else {
                    mSelectionState[group][member] = false;
                    emit selectionChanged(group, member, false);
                }
            }
        }
    }
}

bool THyperlinkSelectionManager::isGroupExclusive(const QString& group) const
{
    // Default to non-exclusive (checkbox behavior) if not explicitly set
    return mGroupExclusivity.value(group, false);
}

void THyperlinkSelectionManager::handleExclusiveSelection(const QString& group, const QString& value)
{
    const QStringList members = getGroupMembers(group);
    for (const QString& member : members) {
        if (member != value) {
            bool previousState = isSelected(group, member);
            mSelectionState[group][member] = false;
            if (previousState) {
                emit selectionChanged(group, member, false);
            }
        }
    }
}
