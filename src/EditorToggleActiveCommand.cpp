/***************************************************************************
 *   Copyright (C) 2024 by Excellencedev                                 *
 *   ademiluyisuccessandexcellence@gmail.com                               *
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

#include "EditorToggleActiveCommand.h"

#include "Host.h"
#include "TriggerUnit.h"
#include "AliasUnit.h"
#include "TimerUnit.h"
#include "ActionUnit.h"
#include "ScriptUnit.h"
#include "KeyUnit.h"
#include "TTrigger.h"
#include "TAlias.h"
#include "TTimer.h"
#include "TAction.h"
#include "TScript.h"
#include "TKey.h"

#include <QDebug>

EditorToggleActiveCommand::EditorToggleActiveCommand(Host* host, int itemId, EditorViewType viewType, bool active, QUndoCommand* parent)
    : EditorCommand(host, parent)
    , mItemId(itemId)
    , mViewType(viewType)
    , mActive(active)
{
    setText(QObject::tr("Toggle Active"));
}

void EditorToggleActiveCommand::undo()
{
    // Toggle back to original state
    bool targetState = !mActive;
    
    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* t = mpHost->getTriggerUnit()->getTrigger(mItemId);
        if (t) t->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* a = mpHost->getAliasUnit()->getAlias(mItemId);
        if (a) a->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* t = mpHost->getTimerUnit()->getTimer(mItemId);
        if (t) t->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* a = mpHost->getActionUnit()->getAction(mItemId);
        if (a) a->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* s = mpHost->getScriptUnit()->getScript(mItemId);
        if (s) s->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmKeyView: {
        TKey* k = mpHost->getKeyUnit()->getKey(mItemId);
        if (k) k->setIsActive(targetState);
        break;
    }
    default:
        break;
    }
}

void EditorToggleActiveCommand::redo()
{
    // Apply the toggle
    bool targetState = mActive;

    switch (mViewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* t = mpHost->getTriggerUnit()->getTrigger(mItemId);
        if (t) t->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* a = mpHost->getAliasUnit()->getAlias(mItemId);
        if (a) a->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* t = mpHost->getTimerUnit()->getTimer(mItemId);
        if (t) t->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* a = mpHost->getActionUnit()->getAction(mItemId);
        if (a) a->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* s = mpHost->getScriptUnit()->getScript(mItemId);
        if (s) s->setIsActive(targetState);
        break;
    }
    case EditorViewType::cmKeyView: {
        TKey* k = mpHost->getKeyUnit()->getKey(mItemId);
        if (k) k->setIsActive(targetState);
        break;
    }
    default:
        break;
    }
}

int EditorToggleActiveCommand::id() const
{
    return 105; // Unique ID for ToggleActive
}

bool EditorToggleActiveCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id())
        return false;
        
    const EditorToggleActiveCommand* cmd = static_cast<const EditorToggleActiveCommand*>(other);
    if (cmd->mItemId != mItemId || cmd->mViewType != mViewType)
        return false;
        
    // If we toggle the same item again, we just update the target state
    mActive = cmd->mActive;
    return true;
}
