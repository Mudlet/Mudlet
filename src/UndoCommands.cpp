/***************************************************************************
 *   Copyright (C) 2023-2025 by Lecker Kebap - Leris@mudlet.org            *
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

#include "UndoCommands.h"
#include "TTrigger.h"
#include "TAlias.h"
#include "dlgTriggerEditor.h"
#include "TriggerUnit.h"
#include "AliasUnit.h"
#include "TimerUnit.h"
#include "ScriptUnit.h"
#include "KeyUnit.h"
#include "ActionUnit.h"
#include "Host.h"
#include "TKey.h"
#include "TScript.h"
#include "TAction.h"

// =============================================================================
// AddTriggerCommand
// =============================================================================

AddTriggerCommand::AddTriggerCommand(TTrigger* parent, bool isFolder,
                                     dlgTriggerEditor* editor,
                                     QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mParent(parent),
      mIsFolder(isFolder), mNewTrigger(nullptr)
{
    setText(QObject::tr("add trigger"));
}

AddTriggerCommand::~AddTriggerCommand()
{
    // The trigger is owned by the TriggerUnit, so we don't delete it here
    // unless it was never added to the TriggerUnit.
    if (mNewTrigger && !mNewTrigger->getParent() && !mNewTrigger->isTemporary()) {
        delete mNewTrigger;
    }
}

void AddTriggerCommand::undo()
{
    if (mNewTrigger) {
        mRow = mEditor->getTriggerRow(mNewTrigger);
        mEditor->delete_trigger_for_undo(mNewTrigger);
    }
}

void AddTriggerCommand::redo()
{
    if (!mNewTrigger) {
        mNewTrigger = mEditor->addTrigger(mIsFolder, mParent, false);
    } else {
        mEditor->addTrigger(mNewTrigger, mParent, mRow);
    }
}

// =============================================================================
// DeleteTriggerCommand
// =============================================================================

DeleteTriggerCommand::DeleteTriggerCommand(TTrigger* trigger,
                                           dlgTriggerEditor* editor,
                                           QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mTrigger(trigger)
{
    mParent = trigger->getParent();
    mRow = mEditor->getTriggerRow(trigger);
    setText(QObject::tr("delete trigger"));
}

DeleteTriggerCommand::~DeleteTriggerCommand()
{
    // Ownership is handled by the TriggerUnit
}


void DeleteTriggerCommand::undo()
{
    mEditor->addTrigger(mTrigger, mParent, mRow);
}

void DeleteTriggerCommand::redo()
{
    mEditor->delete_trigger_for_undo(mTrigger);
}

// =============================================================================
// ChangeTriggerPropertyCommand
// =============================================================================

ChangeTriggerPropertyCommand::ChangeTriggerPropertyCommand(
    TTrigger* trigger, TriggerProperty property, const QVariant& oldValue,
    const QVariant& newValue, dlgTriggerEditor* editor, QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mTrigger(trigger),
      mProperty(property), mOldValue(oldValue), mNewValue(newValue)
{
    setText(QObject::tr("change trigger property"));
}

void ChangeTriggerPropertyCommand::setProperty(const QVariant& value)
{
    switch (mProperty) {
    case Name:
        mTrigger->setName(value.toString());
        break;
    case Command:
        mTrigger->setCommand(value.toString());
        break;
    case Script:
        mTrigger->setScript(value.toString());
        break;
    case Patterns:
        {
            QList<QVariant> list = value.toList();
            QStringList patterns = list[0].toStringList();
            QList<int> patternKinds;
            for (const QVariant& v : list[1].toList()) {
                patternKinds.append(v.toInt());
            }
            mTrigger->setRegexCodeList(patterns, patternKinds);
        }
        break;
    case IsMultiline:
        mTrigger->setIsMultiline(value.toBool());
        break;
    case ConditionLineDelta:
        mTrigger->setConditionLineDelta(value.toInt());
        break;
    case StayOpen:
        mTrigger->mStayOpen = value.toInt();
        break;
    case PerlSlashGOption:
        mTrigger->mPerlSlashGOption = value.toBool();
        break;
    case FilterTrigger:
        mTrigger->mFilterTrigger = value.toBool();
        break;
    case SoundTrigger:
        mTrigger->mSoundTrigger = value.toBool();
        break;
    case SoundFile:
        mTrigger->setSound(value.toString());
        break;
    case IsColorizerTrigger:
        mTrigger->setIsColorizerTrigger(value.toBool());
        break;
    case FgColor:
        mTrigger->setColorizerFgColor(value.value<QColor>());
        break;
    case BgColor:
        mTrigger->setColorizerBgColor(value.value<QColor>());
        break;
    }
    mEditor->updateTriggerView(mTrigger);
}

void ChangeTriggerPropertyCommand::undo()
{
    setProperty(mOldValue);
}

void ChangeTriggerPropertyCommand::redo()
{
    setProperty(mNewValue);
}

// =============================================================================
// ActivateTriggerCommand
// =============================================================================

ActivateTriggerCommand::ActivateTriggerCommand(TTrigger* trigger,
                                                 dlgTriggerEditor* editor,
                                                 QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mTrigger(trigger)
{
    mOldState = trigger->shouldBeActive();
    setText(QObject::tr("toggle trigger activation"));
}

void ActivateTriggerCommand::undo()
{
    mTrigger->setIsActive(mOldState);
    mEditor->updateTriggerView(mTrigger);
}

void ActivateTriggerCommand::redo()
{
    mTrigger->setIsActive(!mOldState);
    mEditor->updateTriggerView(mTrigger);
}

// =============================================================================
// AddAliasCommand
// =============================================================================

AddAliasCommand::AddAliasCommand(TAlias* parent, bool isFolder,
                                     dlgTriggerEditor* editor,
                                     QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mParent(parent),
      mIsFolder(isFolder), mNewAlias(nullptr)
{
    setText(QObject::tr("add alias"));
}

AddAliasCommand::~AddAliasCommand()
{
    if (mNewAlias && !mNewAlias->getParent() && !mNewAlias->isTemporary()) {
        delete mNewAlias;
    }
}

void AddAliasCommand::undo()
{
    if (mNewAlias) {
        mRow = mEditor->getAliasRow(mNewAlias);
        mEditor->delete_alias_for_undo(mNewAlias);
    }
}

void AddAliasCommand::redo()
{
    if (!mNewAlias) {
        mNewAlias = mEditor->addAlias(mIsFolder, mParent, false);
    } else {
        mEditor->addAlias(mNewAlias, mParent, mRow);
    }
}

// =============================================================================
// DeleteAliasCommand
// =============================================================================

DeleteAliasCommand::DeleteAliasCommand(TAlias* alias,
                                           dlgTriggerEditor* editor,
                                           QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mAlias(alias)
{
    mParent = alias->getParent();
    mRow = mEditor->getAliasRow(alias);
    setText(QObject::tr("delete alias"));
}

DeleteAliasCommand::~DeleteAliasCommand()
{
    // Ownership is handled by the AliasUnit
}


void DeleteAliasCommand::undo()
{
    mEditor->addAlias(mAlias, mParent, mRow);
}

void DeleteAliasCommand::redo()
{
    mEditor->delete_alias_for_undo(mAlias);
}

// =============================================================================
// ChangeAliasPropertyCommand
// =============================================================================

ChangeAliasPropertyCommand::ChangeAliasPropertyCommand(
    TAlias* alias, AliasProperty property, const QVariant& oldValue,
    const QVariant& newValue, dlgTriggerEditor* editor, QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mAlias(alias),
      mProperty(property), mOldValue(oldValue), mNewValue(newValue)
{
    setText(QObject::tr("change alias property"));
}

void ChangeAliasPropertyCommand::setProperty(const QVariant& value)
{
    switch (mProperty) {
    case Name:
        mAlias->setName(value.toString());
        break;
    case Command:
        mAlias->setCommand(value.toString());
        break;
    case Script:
        mAlias->setScript(value.toString());
        break;
    case RegexCode:
        mAlias->setRegexCode(value.toString());
        break;
    }
    mEditor->updateAliasView(mAlias);
}

void ChangeAliasPropertyCommand::undo()
{
    setProperty(mOldValue);
}

void ChangeAliasPropertyCommand::redo()
{
    setProperty(mNewValue);
}

// =============================================================================
// ActivateAliasCommand
// =============================================================================

ActivateAliasCommand::ActivateAliasCommand(TAlias* alias,
                                                 dlgTriggerEditor* editor,
                                                 QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mAlias(alias)
{
    mOldState = alias->shouldBeActive();
    setText(QObject::tr("toggle alias activation"));
}

void ActivateAliasCommand::undo()
{
    mAlias->setIsActive(mOldState);
    mEditor->updateAliasView(mAlias);
}

void ActivateAliasCommand::redo()
{
    mAlias->setIsActive(!mOldState);
    mEditor->updateAliasView(mAlias);
}

// =============================================================================
// AddTimerCommand
// =============================================================================

AddTimerCommand::AddTimerCommand(TTimer* parent, bool isFolder,
                                     dlgTriggerEditor* editor,
                                     QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mParent(parent),
      mIsFolder(isFolder), mNewTimer(nullptr)
{
    setText(QObject::tr("add timer"));
}

AddTimerCommand::~AddTimerCommand()
{
    if (mNewTimer && !mNewTimer->getParent() && !mNewTimer->isTemporary()) {
        delete mNewTimer;
    }
}

void AddTimerCommand::undo()
{
    if (mNewTimer) {
        mRow = mEditor->getTimerRow(mNewTimer);
        mEditor->delete_timer_for_undo(mNewTimer);
    }
}

void AddTimerCommand::redo()
{
    if (!mNewTimer) {
        mNewTimer = mEditor->addTimer(mIsFolder, mParent, false);
    } else {
        mEditor->addTimer(mNewTimer, mParent, mRow);
    }
}

// =============================================================================
// DeleteTimerCommand
// =============================================================================

DeleteTimerCommand::DeleteTimerCommand(TTimer* timer,
                                           dlgTriggerEditor* editor,
                                           QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mTimer(timer)
{
    mParent = timer->getParent();
    mRow = mEditor->getTimerRow(timer);
    setText(QObject::tr("delete timer"));
}

DeleteTimerCommand::~DeleteTimerCommand()
{
    // Ownership is handled by the TimerUnit
}


void DeleteTimerCommand::undo()
{
    mEditor->addTimer(mTimer, mParent, mRow);
}

void DeleteTimerCommand::redo()
{
    mEditor->delete_timer_for_undo(mTimer);
}

// =============================================================================
// ChangeTimerPropertyCommand
// =============================================================================

ChangeTimerPropertyCommand::ChangeTimerPropertyCommand(
    TTimer* timer, TimerProperty property, const QVariant& oldValue,
    const QVariant& newValue, dlgTriggerEditor* editor, QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mTimer(timer),
      mProperty(property), mOldValue(oldValue), mNewValue(newValue)
{
    setText(QObject::tr("change timer property"));
}

void ChangeTimerPropertyCommand::setProperty(const QVariant& value)
{
    switch (mProperty) {
    case Name:
        mTimer->setName(value.toString());
        break;
    case Command:
        mTimer->setCommand(value.toString());
        break;
    case Script:
        mTimer->setScript(value.toString());
        break;
    case Time:
        mTimer->setTime(value.toTime());
        break;
    }
    mEditor->updateTimerView(mTimer);
}

void ChangeTimerPropertyCommand::undo()
{
    setProperty(mOldValue);
}

void ChangeTimerPropertyCommand::redo()
{
    setProperty(mNewValue);
}

// =============================================================================
// ActivateTimerCommand
// =============================================================================

ActivateTimerCommand::ActivateTimerCommand(TTimer* timer,
                                                 dlgTriggerEditor* editor,
                                                 QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mTimer(timer)
{
    mOldState = timer->shouldBeActive();
    setText(QObject::tr("toggle timer activation"));
}

void ActivateTimerCommand::undo()
{
    mTimer->setIsActive(mOldState);
    mEditor->updateTimerView(mTimer);
}

void ActivateTimerCommand::redo()
{
    mTimer->setIsActive(!mOldState);
    mEditor->updateTimerView(mTimer);
}

// =============================================================================
// AddScriptCommand
// =============================================================================

AddScriptCommand::AddScriptCommand(TScript* parent, bool isFolder,
                                     dlgTriggerEditor* editor,
                                     QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mParent(parent),
      mIsFolder(isFolder), mNewScript(nullptr)
{
    setText(QObject::tr("add script"));
}

AddScriptCommand::~AddScriptCommand()
{
    if (mNewScript && !mNewScript->getParent() && !mNewScript->isTemporary()) {
        delete mNewScript;
    }
}

void AddScriptCommand::undo()
{
    if (mNewScript) {
        mRow = mEditor->getScriptRow(mNewScript);
        mEditor->delete_script_for_undo(mNewScript);
    }
}

void AddScriptCommand::redo()
{
    if (!mNewScript) {
        mNewScript = mEditor->addScript(mIsFolder, mParent, false);
    } else {
        mEditor->addScript(mNewScript, mParent, mRow);
    }
}

// =============================================================================
// DeleteScriptCommand
// =============================================================================

DeleteScriptCommand::DeleteScriptCommand(TScript* script,
                                           dlgTriggerEditor* editor,
                                           QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mScript(script)
{
    mParent = script->getParent();
    mRow = mEditor->getScriptRow(script);
    setText(QObject::tr("delete script"));
}

DeleteScriptCommand::~DeleteScriptCommand()
{
    // Ownership is handled by the ScriptUnit
}


void DeleteScriptCommand::undo()
{
    mEditor->addScript(mScript, mParent, mRow);
}

void DeleteScriptCommand::redo()
{
    mEditor->delete_script_for_undo(mScript);
}

// =============================================================================
// ChangeScriptPropertyCommand
// =============================================================================

ChangeScriptPropertyCommand::ChangeScriptPropertyCommand(
    TScript* script, ScriptProperty property, const QVariant& oldValue,
    const QVariant& newValue, dlgTriggerEditor* editor, QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mScript(script),
      mProperty(property), mOldValue(oldValue), mNewValue(newValue)
{
    setText(QObject::tr("change script property"));
}

void ChangeScriptPropertyCommand::setProperty(const QVariant& value)
{
    switch (mProperty) {
    case Name:
        mScript->setName(value.toString());
        break;
    case Script:
        mScript->setScript(value.toString());
        break;
    case EventHandlerList:
        mScript->setEventHandlerList(value.toStringList());
        break;
    }
    mEditor->updateScriptView(mScript);
}

void ChangeScriptPropertyCommand::undo()
{
    setProperty(mOldValue);
}

void ChangeScriptPropertyCommand::redo()
{
    setProperty(mNewValue);
}

// =============================================================================
// ActivateScriptCommand
// =============================================================================

ActivateScriptCommand::ActivateScriptCommand(TScript* script,
                                                 dlgTriggerEditor* editor,
                                                 QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mScript(script)
{
    mOldState = script->shouldBeActive();
    setText(QObject::tr("toggle script activation"));
}

void ActivateScriptCommand::undo()
{
    mScript->setIsActive(mOldState);
    mEditor->updateScriptView(mScript);
}

void ActivateScriptCommand::redo()
{
    mScript->setIsActive(!mOldState);
    mEditor->updateScriptView(mScript);
}

// =============================================================================
// AddKeyCommand
// =============================================================================

AddKeyCommand::AddKeyCommand(TKey* parent, bool isFolder,
                                     dlgTriggerEditor* editor,
                                     QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mParent(parent),
      mIsFolder(isFolder), mNewKey(nullptr)
{
    setText(QObject::tr("add key"));
}

AddKeyCommand::~AddKeyCommand()
{
    if (mNewKey && !mNewKey->getParent() && !mNewKey->isTemporary()) {
        delete mNewKey;
    }
}

void AddKeyCommand::undo()
{
    if (mNewKey) {
        mRow = mEditor->getKeyRow(mNewKey);
        mEditor->delete_key_for_undo(mNewKey);
    }
}

void AddKeyCommand::redo()
{
    if (!mNewKey) {
        mNewKey = mEditor->addKey(mIsFolder, mParent, false);
    } else {
        mEditor->addKey(mNewKey, mParent, mRow);
    }
}

// =============================================================================
// DeleteKeyCommand
// =============================================================================

DeleteKeyCommand::DeleteKeyCommand(TKey* key,
                                           dlgTriggerEditor* editor,
                                           QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mKey(key)
{
    mParent = key->getParent();
    mRow = mEditor->getKeyRow(key);
    setText(QObject::tr("delete key"));
}

DeleteKeyCommand::~DeleteKeyCommand()
{
    // Ownership is handled by the KeyUnit
}


void DeleteKeyCommand::undo()
{
    mEditor->addKey(mKey, mParent, mRow);
}

void DeleteKeyCommand::redo()
{
    mEditor->delete_key_for_undo(mKey);
}

// =============================================================================
// ChangeKeyPropertyCommand
// =============================================================================

ChangeKeyPropertyCommand::ChangeKeyPropertyCommand(
    TKey* key, KeyProperty property, const QVariant& oldValue,
    const QVariant& newValue, dlgTriggerEditor* editor, QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mKey(key),
      mProperty(property), mOldValue(oldValue), mNewValue(newValue)
{
    setText(QObject::tr("change key property"));
}

void ChangeKeyPropertyCommand::setProperty(const QVariant& value)
{
    switch (mProperty) {
    case Name:
        mKey->setName(value.toString());
        break;
    case Command:
        mKey->setCommand(value.toString());
        break;
    case Script:
        mKey->setScript(value.toString());
        break;
    case KeyCode:
        mKey->setKeyCode(static_cast<Qt::Key>(value.toInt()));
        break;
    case KeyModifiers:
        mKey->setKeyModifiers(static_cast<Qt::KeyboardModifiers>(value.toInt()));
        break;
    }
    mEditor->updateKeyView(mKey);
}

void ChangeKeyPropertyCommand::undo()
{
    setProperty(mOldValue);
}

void ChangeKeyPropertyCommand::redo()
{
    setProperty(mNewValue);
}

// =============================================================================
// ActivateKeyCommand
// =============================================================================

ActivateKeyCommand::ActivateKeyCommand(TKey* key,
                                                 dlgTriggerEditor* editor,
                                                 QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mKey(key)
{
    mOldState = key->shouldBeActive();
    setText(QObject::tr("toggle key activation"));
}

void ActivateKeyCommand::undo()
{
    mKey->setIsActive(mOldState);
    mEditor->updateKeyView(mKey);
}

void ActivateKeyCommand::redo()
{
    mKey->setIsActive(!mOldState);
    mEditor->updateKeyView(mKey);
}

// =============================================================================
// AddActionCommand
// =============================================================================

AddActionCommand::AddActionCommand(TAction* parent, bool isFolder,
                                     dlgTriggerEditor* editor,
                                     QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mParent(parent),
      mIsFolder(isFolder), mNewAction(nullptr)
{
    setText(QObject::tr("add button"));
}

AddActionCommand::~AddActionCommand()
{
    if (mNewAction && !mNewAction->getParent() && !mNewAction->isTemporary()) {
        delete mNewAction;
    }
}

void AddActionCommand::undo()
{
    if (mNewAction) {
        mRow = mEditor->getActionRow(mNewAction);
        mEditor->delete_action_for_undo(mNewAction);
    }
}

void AddActionCommand::redo()
{
    if (!mNewAction) {
        mNewAction = mEditor->addAction(mIsFolder, mParent, false);
    } else {
        mEditor->addAction(mNewAction, mParent, mRow);
    }
}

// =============================================================================
// DeleteActionCommand
// =============================================================================

DeleteActionCommand::DeleteActionCommand(TAction* action,
                                           dlgTriggerEditor* editor,
                                           QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mAction(action)
{
    mParent = action->getParent();
    mRow = mEditor->getActionRow(action);
    setText(QObject::tr("delete button"));
}

DeleteActionCommand::~DeleteActionCommand()
{
    // Ownership is handled by the ActionUnit
}


void DeleteActionCommand::undo()
{
    mEditor->addAction(mAction, mParent, mRow);
}

void DeleteActionCommand::redo()
{
    mEditor->delete_action_for_undo(mAction);
}

// =============================================================================
// ChangeActionPropertyCommand
// =============================================================================

ChangeActionPropertyCommand::ChangeActionPropertyCommand(
    TAction* action, ActionProperty property, const QVariant& oldValue,
    const QVariant& newValue, dlgTriggerEditor* editor, QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mAction(action),
      mProperty(property), mOldValue(oldValue), mNewValue(newValue)
{
    setText(QObject::tr("change button property"));
}

void ChangeActionPropertyCommand::setProperty(const QVariant& value)
{
    switch (mProperty) {
    case Name:
        mAction->setName(value.toString());
        break;
    case Icon:
        mAction->setIcon(value.toString());
        break;
    case Script:
        mAction->setScript(value.toString());
        break;
    case CommandButtonUp:
        mAction->setCommandButtonUp(value.toString());
        break;
    case CommandButtonDown:
        mAction->setCommandButtonDown(value.toString());
        break;
    case IsPushDownButton:
        mAction->setIsPushDownButton(value.toBool());
        break;
    case ButtonRotation:
        mAction->setButtonRotation(value.toInt());
        break;
    case ButtonColumns:
        mAction->setButtonColumns(value.toInt());
        break;
    case Css:
        mAction->css = value.toString();
        break;
    case Location:
        mAction->mLocation = value.toInt();
        break;
    case Orientation:
        mAction->mOrientation = value.toInt();
        break;
    }
    mEditor->updateActionView(mAction);
}

void ChangeActionPropertyCommand::undo()
{
    setProperty(mOldValue);
}

void ChangeActionPropertyCommand::redo()
{
    setProperty(mNewValue);
}

// =============================================================================
// ActivateActionCommand
// =============================================================================

ActivateActionCommand::ActivateActionCommand(TAction* action,
                                                 dlgTriggerEditor* editor,
                                                 QUndoCommand* parentCmd)
    : QUndoCommand(parentCmd), mEditor(editor), mAction(action)
{
    mOldState = action->shouldBeActive();
    setText(QObject::tr("toggle button activation"));
}

void ActivateActionCommand::undo()
{
    mAction->setIsActive(mOldState);
    mEditor->updateActionView(mAction);
}

void ActivateActionCommand::redo()
{
    mAction->setIsActive(!mOldState);
    mEditor->updateActionView(mAction);
}