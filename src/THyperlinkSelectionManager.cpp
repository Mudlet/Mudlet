/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "LuaLiteral.h"
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

QString THyperlinkSelectionManager::modifyUriForSelection(Mudlet::HyperlinkStyling::ActionScheme scheme, const QString& baseCommand, const QString& group, const QString& value) const
{
    const bool isSelected = this->isSelected(group, value);
    const QString command = addSelectedParameter(baseCommand, isSelected);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "modifyUriForSelection called with scheme:" << scheme << "baseCommand:" << baseCommand << "group:" << group << "value:" << value << "isSelected:" << isSelected;
#endif

    switch (scheme) {
    case Mudlet::HyperlinkStyling::ActionSend:
        return qsl("send(%1, false)").arg(LuaLiteral::quote(command));
    case Mudlet::HyperlinkStyling::ActionPrompt:
        return qsl("sendCmdLine(%1)").arg(LuaLiteral::quote(command));
    case Mudlet::HyperlinkStyling::ActionOpenUrl:
    case Mudlet::HyperlinkStyling::ActionNone:
        break;
    }

    return QString();
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
        for (const QString& member : std::as_const(members)) {
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
