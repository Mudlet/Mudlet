#include "dlgTriggerEditorCommand.h"
#include "Host.h"
#include "LuaInterface.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include <QPointer>

AddTriggerCommand::AddTriggerCommand(QTreeWidgetItem* pItem, TriggerUnit* triggerUnit, TTreeWidget* treeWidget_triggers, bool isFolder, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_triggerUnit = triggerUnit;
    m_treeWidget_triggers = treeWidget_triggers;
    m_isFolder = isFolder;
    m_pItem = pItem;
}

void AddTriggerCommand::undo()
{
    if (!m_pItem) {
        return;
    }
    m_parent = m_pItem->parent();

    if (m_parent) {
        m_parent->removeChild(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddTriggerCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_pItem) {
        m_editor->addTrigger(m_isFolder);
        m_pItem = m_treeWidget_triggers->currentItem();
        m_parent = m_pItem->parent();
    } else {
        int count = m_parent->childCount();
        if (m_isFolder) {
            m_parent->addChild(m_pItem);
        } else {
            m_parent->insertChild(count <= 0 ? 0 : count, m_pItem);
        }
    }

    setText(QObject::tr("Add Trigger"));
}

DeleteTriggerCommand::DeleteTriggerCommand(QTreeWidgetItem* pItem, TriggerUnit* triggerUnit, TTreeWidget* treeWidget_triggers, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_pItem = pItem;
    m_parent = m_pItem->parent();
    m_triggerUnit = triggerUnit;
    m_treeWidget_triggers = treeWidget_triggers;
}

void DeleteTriggerCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (m_parent) {
        const int childID = m_itemTrigger->getID();
        m_pItem->setData(0, Qt::UserRole, childID);
        m_parent->insertChild(m_parent->childCount() <= 0 ? 0 : m_parent->childCount(), m_pItem);
        m_treeWidget_triggers->setCurrentItem(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}
void DeleteTriggerCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_host) {
        return;
    }
    if (m_pItem) {
        const int ID = m_pItem->data(0, Qt::UserRole).toInt();
        TTrigger* p = m_triggerUnit->getTrigger(ID);
        TTrigger* ptr = new TTrigger(p->mName, p->mPatterns, p->getRegexCodePropertyList(), false, m_host);
        ptr->registerTrigger();
        m_itemTrigger = ptr;
        m_editor->delete_trigger();
    }
    setText(QObject::tr("Delete Trigger"));
}

MoveTriggerCommand::MoveTriggerCommand(TriggerUnit* triggerUnit,
                                       TTreeWidget* treeWidget_triggers,
                                       int childID,
                                       int oldParentID,
                                       int newParentID,
                                       int parentPosition,
                                       int childPosition,
                                       int prevParentPosition,
                                       int prevChildPosition,
                                       QUndoCommand* parent)
: QUndoCommand(parent)
{
    m_childID = childID;
    m_oldParentID = oldParentID;
    m_newParentID = newParentID;
    m_parentPosition = parentPosition;
    m_childPosition = childPosition;
    m_prevParentPosition = prevParentPosition;
    m_prevChildPosition = prevChildPosition;
    m_triggerUnit = triggerUnit;
    m_treeWidget_triggers = treeWidget_triggers;
}

void MoveTriggerCommand::undo()
{
    if (!m_host) {
        return;
    }

    m_host->getTriggerUnit()->reParentTrigger(m_childID, m_newParentID, m_oldParentID, m_prevParentPosition, m_prevChildPosition);
    m_ParentItem->removeChild(m_pItem);
    m_PrevParentItem->insertChild(m_PrevParentItem->childCount() <= 0 ? 0 : m_PrevParentItem->childCount(), m_pItem);
    m_treeWidget_triggers->setCurrentItem(m_pItem);
}

void MoveTriggerCommand::redo()
{
    m_host->getTriggerUnit()->reParentTrigger(m_childID, m_oldParentID, m_newParentID, m_parentPosition, m_childPosition);
    if (m_PrevParentItem) {
        m_PrevParentItem->removeChild(m_pItem);
    }
    if (m_ParentItem) {
        const int count = m_ParentItem->childCount();
        m_ParentItem->insertChild(count <= 0 ? 0 : count, m_pItem);
    }

    setText(QObject::tr("Move Trigger"));
}

AddAliasCommand::AddAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_aliases, bool isFolder, QUndoCommand* parent)
{
    m_aliasUnit = aliasUnit;
    m_treeWidget_aliases = treeWidget_aliases;
    m_isFolder = isFolder;
    m_pItem = pItem;
}

void AddAliasCommand::undo()
{
    if (!m_pItem) {
        return;
    }
    m_parent = m_pItem->parent();

    if (m_parent) {
        m_parent->removeChild(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddAliasCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_pItem) {
        m_editor->addAlias(m_isFolder);
        m_pItem = m_treeWidget_aliases->currentItem();
        m_parent = m_pItem->parent();
    } else {
        int count = m_parent->childCount();
        if (m_isFolder) {
            m_parent->addChild(m_pItem);
        } else {
            m_parent->insertChild(count <= 0 ? 0 : count, m_pItem);
        }
    }

    setText(QObject::tr("Add Alias"));
}

DeleteAliasCommand::DeleteAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_aliases, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_pItem = pItem;
    m_parent = m_pItem->parent();
    m_aliasUnit = aliasUnit;
    m_treeWidget_aliases = treeWidget_aliases;
}

void DeleteAliasCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (m_parent) {
        const int childID = m_itemAlias->getID();
        m_pItem->setData(0, Qt::UserRole, childID);
        m_parent->insertChild(m_parent->childCount() <= 0 ? 0 : m_parent->childCount(), m_pItem);
        m_treeWidget_aliases->setCurrentItem(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteAliasCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_host) {
        return;
    }
    if (m_pItem) {
        const int ID = m_pItem->data(0, Qt::UserRole).toInt();
        TAlias* p = m_aliasUnit->getAlias(ID);
        TAlias* ptr = new TAlias(p->mName, m_host);
        ptr->registerAlias();
        m_itemAlias = ptr;
        m_editor->delete_alias();
    }
    setText(QObject::tr("Delete Alias"));
}

MoveAliasCommand::MoveAliasCommand(AliasUnit* aliasUnit,
                                   TTreeWidget* treeWidget_aliases,
                                   int childID,
                                   int oldParentID,
                                   int newParentID,
                                   int parentPosition,
                                   int childPosition,
                                   int prevParentPosition,
                                   int prevChildPosition,
                                   QUndoCommand* parent)
: QUndoCommand(parent)
{
    m_childID = childID;
    m_oldParentID = oldParentID;
    m_newParentID = newParentID;
    m_parentPosition = parentPosition;
    m_childPosition = childPosition;
    m_prevParentPosition = prevParentPosition;
    m_prevChildPosition = prevChildPosition;
    m_aliasUnit = aliasUnit;
    m_treeWidget_aliases = treeWidget_aliases;
}

void MoveAliasCommand::undo()
{
    if (!m_host) {
        return;
    }

    m_host->getAliasUnit()->reParentAlias(m_childID, m_newParentID, m_oldParentID, m_prevParentPosition, m_prevChildPosition);
    m_ParentItem->removeChild(m_pItem);
    m_PrevParentItem->insertChild(m_PrevParentItem->childCount() <= 0 ? 0 : m_PrevParentItem->childCount(), m_pItem);
    m_treeWidget_aliases->setCurrentItem(m_pItem);
}

void MoveAliasCommand::redo()
{
    m_host->getAliasUnit()->reParentAlias(m_childID, m_oldParentID, m_newParentID, m_parentPosition, m_childPosition);
    if (m_PrevParentItem) {
        m_PrevParentItem->removeChild(m_pItem);
    }
    if (m_ParentItem) {
        const int count = m_ParentItem->childCount();
        m_ParentItem->insertChild(count <= 0 ? 0 : count, m_pItem);
    }

    setText(QObject::tr("Move Alias"));
}

AddTimerCommand::AddTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_timers, bool isFolder, QUndoCommand* parent)
{
    m_timerUnit = timerUnit;
    m_treeWidget_timers = treeWidget_timers;
    m_isFolder = isFolder;
    m_pItem = pItem;
}

void AddTimerCommand::undo()
{
    if (!m_pItem) {
        return;
    }
    m_parent = m_pItem->parent();

    if (m_parent) {
        m_parent->removeChild(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddTimerCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_pItem) {
        m_editor->addTimer(m_isFolder);
        m_pItem = m_treeWidget_timers->currentItem();
        m_parent = m_pItem->parent();
    } else {
        int count = m_parent->childCount();
        if (m_isFolder) {
            m_parent->addChild(m_pItem);
        } else {
            m_parent->insertChild(count <= 0 ? 0 : count, m_pItem);
        }
    }

    setText(QObject::tr("Add Timer"));
}

DeleteTimerCommand::DeleteTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_timers, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_pItem = pItem;
    m_parent = m_pItem->parent();
    m_timerUnit = timerUnit;
    m_treeWidget_timers = treeWidget_timers;
}

void DeleteTimerCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (m_parent) {
        const int childID = m_itemTimer->getID();
        m_pItem->setData(0, Qt::UserRole, childID);
        m_parent->insertChild(m_parent->childCount() <= 0 ? 0 : m_parent->childCount(), m_pItem);
        m_treeWidget_timers->setCurrentItem(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteTimerCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_host) {
        return;
    }
    if (m_pItem) {
        const int ID = m_pItem->data(0, Qt::UserRole).toInt();
        TTimer* p = m_timerUnit->getTimer(ID);
        TTimer* ptr = new TTimer(p->getName(), p->getTime(), m_host);
        m_host->getTimerUnit()->registerTimer(ptr);
        m_itemTimer = ptr;
        m_editor->delete_timer();
    }
    setText(QObject::tr("Delete Timer"));
}

MoveTimerCommand::MoveTimerCommand(TimerUnit* timerUnit,
                                   TTreeWidget* treeWidget_timers,
                                   int childID,
                                   int oldParentID,
                                   int newParentID,
                                   int parentPosition,
                                   int childPosition,
                                   int prevParentPosition,
                                   int prevChildPosition,
                                   QUndoCommand* parent)
: QUndoCommand(parent)
{
    m_childID = childID;
    m_oldParentID = oldParentID;
    m_newParentID = newParentID;
    m_parentPosition = parentPosition;
    m_childPosition = childPosition;
    m_prevParentPosition = prevParentPosition;
    m_prevChildPosition = prevChildPosition;
    m_timerUnit = timerUnit;
    m_treeWidget_timers = treeWidget_timers;
}

void MoveTimerCommand::undo()
{
    if (!m_host) {
        return;
    }

    m_host->getTimerUnit()->reParentTimer(m_childID, m_newParentID, m_oldParentID, m_prevParentPosition, m_prevChildPosition);
    m_ParentItem->removeChild(m_pItem);
    m_PrevParentItem->insertChild(m_PrevParentItem->childCount() <= 0 ? 0 : m_PrevParentItem->childCount(), m_pItem);
    m_treeWidget_timers->setCurrentItem(m_pItem);
}

void MoveTimerCommand::redo()
{
    m_host->getTimerUnit()->reParentTimer(m_childID, m_oldParentID, m_newParentID, m_parentPosition, m_childPosition);
    if (m_PrevParentItem) {
        m_PrevParentItem->removeChild(m_pItem);
    }
    if (m_ParentItem) {
        const int count = m_ParentItem->childCount();
        m_ParentItem->insertChild(count <= 0 ? 0 : count, m_pItem);
    }

    setText(QObject::tr("Move Timer"));
}

AddScriptCommand::AddScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, bool isFolder, QUndoCommand* parent)
{
    m_scriptUnit = scriptUnit;
    m_treeWidget_scripts = treeWidget_scripts;
    m_isFolder = isFolder;
    m_pItem = pItem;
}

void AddScriptCommand::undo()
{
    if (!m_pItem) {
        return;
    }
    m_parent = m_pItem->parent();

    if (m_parent) {
        m_parent->removeChild(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddScriptCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_pItem) {
        m_editor->addScript(m_isFolder);
        m_pItem = m_treeWidget_scripts->currentItem();
        m_parent = m_pItem->parent();
    } else {
        int count = m_parent->childCount();
        if (m_isFolder) {
            m_parent->addChild(m_pItem);
        } else {
            m_parent->insertChild(count <= 0 ? 0 : count, m_pItem);
        }
    }

    setText(QObject::tr("Add Script"));
}

DeleteScriptCommand::DeleteScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_pItem = pItem;
    m_parent = m_pItem->parent();
    m_scriptUnit = scriptUnit;
    m_treeWidget_scripts = treeWidget_scripts;
}

void DeleteScriptCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (m_parent) {
        const int childID = m_itemScript->getID();
        m_pItem->setData(0, Qt::UserRole, childID);
        m_parent->insertChild(m_parent->childCount() <= 0 ? 0 : m_parent->childCount(), m_pItem);
        m_treeWidget_scripts->setCurrentItem(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteScriptCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_host) {
        return;
    }
    if (m_pItem) {
        const int ID = m_pItem->data(0, Qt::UserRole).toInt();
        TScript* p = m_scriptUnit->getScript(ID);
        TScript* ptr = new TScript(p->getName(), m_host);
        ptr->registerScript();
        m_itemScript = ptr;
        m_editor->delete_script();
    }
    setText(QObject::tr("Delete Script"));
}

MoveScriptCommand::MoveScriptCommand(ScriptUnit* scriptUnit,
                                     TTreeWidget* treeWidget_scripts,
                                     int childID,
                                     int oldParentID,
                                     int newParentID,
                                     int parentPosition,
                                     int childPosition,
                                     int prevParentPosition,
                                     int prevChildPosition,
                                     QUndoCommand* parent)
: QUndoCommand(parent)
{
    m_childID = childID;
    m_oldParentID = oldParentID;
    m_newParentID = newParentID;
    m_parentPosition = parentPosition;
    m_childPosition = childPosition;
    m_prevParentPosition = prevParentPosition;
    m_prevChildPosition = prevChildPosition;
    m_scriptUnit = scriptUnit;
    m_treeWidget_scripts = treeWidget_scripts;
}

void MoveScriptCommand::undo()
{
    if (!m_host) {
        return;
    }

    m_host->getScriptUnit()->reParentScript(m_childID, m_newParentID, m_oldParentID, m_prevParentPosition, m_prevChildPosition);
    m_ParentItem->removeChild(m_pItem);
    m_PrevParentItem->insertChild(m_PrevParentItem->childCount() <= 0 ? 0 : m_PrevParentItem->childCount(), m_pItem);
    m_treeWidget_scripts->setCurrentItem(m_pItem);
}

void MoveScriptCommand::redo()
{
    m_host->getScriptUnit()->reParentScript(m_childID, m_oldParentID, m_newParentID, m_parentPosition, m_childPosition);
    if (m_PrevParentItem) {
        m_PrevParentItem->removeChild(m_pItem);
    }
    if (m_ParentItem) {
        const int count = m_ParentItem->childCount();
        m_ParentItem->insertChild(count <= 0 ? 0 : count, m_pItem);
    }

    setText(QObject::tr("Move Script"));
}

AddKeyCommand::AddKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, bool isFolder, QUndoCommand* parent)
{
    m_keyUnit = keyUnit;
    m_treeWidget_keys = treeWidget_keys;
    m_isFolder = isFolder;
    m_pItem = pItem;
}

void AddKeyCommand::undo()
{
    if (!m_pItem) {
        return;
    }
    m_parent = m_pItem->parent();

    if (m_parent) {
        m_parent->removeChild(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddKeyCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_pItem) {
        m_editor->addKey(m_isFolder);
        m_pItem = m_treeWidget_keys->currentItem();
        m_parent = m_pItem->parent();
    } else {
        int count = m_parent->childCount();
        if (m_isFolder) {
            m_parent->addChild(m_pItem);
        } else {
            m_parent->insertChild(count <= 0 ? 0 : count, m_pItem);
        }
    }

    setText(QObject::tr("Add Key"));
}

DeleteKeyCommand::DeleteKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_pItem = pItem;
    m_parent = m_pItem->parent();
    m_keyUnit = keyUnit;
    m_treeWidget_keys = treeWidget_keys;
}

void DeleteKeyCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (m_parent) {
        const int childID = m_itemKey->getID();
        m_pItem->setData(0, Qt::UserRole, childID);
        m_parent->insertChild(m_parent->childCount() <= 0 ? 0 : m_parent->childCount(), m_pItem);
        m_treeWidget_keys->setCurrentItem(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteKeyCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_host) {
        return;
    }
    if (m_pItem) {
        const int ID = m_pItem->data(0, Qt::UserRole).toInt();
        TKey* p = m_keyUnit->getKey(ID);
        TKey* ptr = new TKey(p->getName(), m_host);
        ptr->registerKey();
        m_itemKey = ptr;
        m_editor->delete_key();
    }
    setText(QObject::tr("Delete Key"));
}
MoveKeyCommand::MoveKeyCommand(KeyUnit* keyUnit,
                               TTreeWidget* treeWidget_keys,
                               int childID,
                               int oldParentID,
                               int newParentID,
                               int parentPosition,
                               int childPosition,
                               int prevParentPosition,
                               int prevChildPosition,
                               QUndoCommand* parent)
: QUndoCommand(parent)
{
    m_childID = childID;
    m_oldParentID = oldParentID;
    m_newParentID = newParentID;
    m_parentPosition = parentPosition;
    m_childPosition = childPosition;
    m_prevParentPosition = prevParentPosition;
    m_prevChildPosition = prevChildPosition;
    m_keyUnit = keyUnit;
    m_treeWidget_keys = treeWidget_keys;
}

void MoveKeyCommand::undo()
{
    if (!m_host) {
        return;
    }

    m_host->getKeyUnit()->reParentKey(m_childID, m_newParentID, m_oldParentID, m_prevParentPosition, m_prevChildPosition);
    m_ParentItem->removeChild(m_pItem);
    m_PrevParentItem->insertChild(m_PrevParentItem->childCount() <= 0 ? 0 : m_PrevParentItem->childCount(), m_pItem);
    m_treeWidget_keys->setCurrentItem(m_pItem);
}

void MoveKeyCommand::redo()
{
    m_host->getKeyUnit()->reParentKey(m_childID, m_oldParentID, m_newParentID, m_parentPosition, m_childPosition);
    if (m_PrevParentItem) {
        m_PrevParentItem->removeChild(m_pItem);
    }
    if (m_ParentItem) {
        const int count = m_ParentItem->childCount();
        m_ParentItem->insertChild(count <= 0 ? 0 : count, m_pItem);
    }

    setText(QObject::tr("Move Key"));
}
AddActionCommand::AddActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, bool isFolder, QUndoCommand* parent)
{
    m_actionUnit = actionUnit;
    m_treeWidget_actions = treeWidget_actions;
    m_isFolder = isFolder;
    m_pItem = pItem;
}

void AddActionCommand::undo()
{
    if (!m_pItem) {
        return;
    }
    m_parent = m_pItem->parent();

    if (m_parent) {
        m_parent->removeChild(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddActionCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_pItem) {
        m_editor->addAction(m_isFolder);
        m_pItem = m_treeWidget_actions->currentItem();
        m_parent = m_pItem->parent();
    } else {
        int count = m_parent->childCount();
        if (m_isFolder) {
            m_parent->addChild(m_pItem);
        } else {
            m_parent->insertChild(count <= 0 ? 0 : count, m_pItem);
        }
    }

    setText(QObject::tr("Add Action"));
}
DeleteActionCommand::DeleteActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_pItem = pItem;
    m_parent = m_pItem->parent();
    m_actionUnit = actionUnit;
    m_treeWidget_actions = treeWidget_actions;
}

void DeleteActionCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (m_parent) {
        const int childID = m_itemAction->getID();
        m_pItem->setData(0, Qt::UserRole, childID);
        m_parent->insertChild(m_parent->childCount() <= 0 ? 0 : m_parent->childCount(), m_pItem);
        m_treeWidget_actions->setCurrentItem(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteActionCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_host) {
        return;
    }
    if (m_pItem) {
        const int ID = m_pItem->data(0, Qt::UserRole).toInt();
        TAction* p = m_actionUnit->getAction(ID);
        TAction* ptr = new TAction(p->getName(), m_host);
        ptr->registerAction();
        m_itemAction = ptr;
        m_editor->delete_action();
    }
    setText(QObject::tr("Delete Action"));
}
MoveActionCommand::MoveActionCommand(ActionUnit* actionUnit,
                                     TTreeWidget* treeWidget_actions,
                                     int childID,
                                     int oldParentID,
                                     int newParentID,
                                     int parentPosition,
                                     int childPosition,
                                     int prevParentPosition,
                                     int prevChildPosition,
                                     QUndoCommand* parent)
: QUndoCommand(parent)
{
    m_childID = childID;
    m_oldParentID = oldParentID;
    m_newParentID = newParentID;
    m_parentPosition = parentPosition;
    m_childPosition = childPosition;
    m_prevParentPosition = prevParentPosition;
    m_prevChildPosition = prevChildPosition;
    m_actionUnit = actionUnit;
    m_treeWidget_actions = treeWidget_actions;
}

void MoveActionCommand::undo()
{
    if (!m_host) {
        return;
    }

    m_host->getActionUnit()->reParentAction(m_childID, m_newParentID, m_oldParentID, m_prevParentPosition, m_prevChildPosition);
    m_ParentItem->removeChild(m_pItem);
    m_PrevParentItem->insertChild(m_PrevParentItem->childCount() <= 0 ? 0 : m_PrevParentItem->childCount(), m_pItem);
    m_treeWidget_actions->setCurrentItem(m_pItem);
    m_host->getActionUnit()->updateToolbar();
}

void MoveActionCommand::redo()
{
    m_host->getActionUnit()->reParentAction(m_childID, m_oldParentID, m_newParentID, m_parentPosition, m_childPosition);
    if (m_PrevParentItem) {
        m_PrevParentItem->removeChild(m_pItem);
    }
    if (m_ParentItem) {
        const int count = m_ParentItem->childCount();
        m_ParentItem->insertChild(count <= 0 ? 0 : count, m_pItem);
    }
    m_host->getActionUnit()->updateToolbar();

    setText(QObject::tr("Move Action"));
}
AddVarCommand::AddVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, bool isFolder, QUndoCommand* parent)
{
    m_varUnit = varUnit;
    m_treeWidget_variables = treeWidget_variables;
    m_isFolder = isFolder;
    m_pItem = pItem;
}

void AddVarCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    m_parent = m_pItem->parent();

    if (m_parent) {
        m_parent->removeChild(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddVarCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_pItem) {
        m_editor->addVar(m_isFolder);
        m_pItem = m_treeWidget_variables->currentItem();
        m_parent = m_pItem->parent();
    } else {
        if (!m_parent) {
            if (m_pItem->parent()) {
                m_parent = m_pItem->parent();
            }
        }
        int count = m_parent->childCount();
        if (m_isFolder) {
            m_parent->addChild(m_pItem);
        } else {
            m_parent->insertChild(count <= 0 ? 0 : count, m_pItem);
        }
    }

    setText(QObject::tr("Add Variable"));
}
DeleteVarCommand::DeleteVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, QUndoCommand* parent) : QUndoCommand(parent)
{
    m_pItem = pItem;
    m_parent = m_pItem->parent();
    m_varUnit = varUnit;
    m_treeWidget_variables = treeWidget_variables;
}

void DeleteVarCommand::undo()
{
    if (!m_pItem) {
        return;
    }

    if (m_parent) {
        if (!m_tempVar) {
            m_tempVar = new TVar();
            *m_tempVar = *m_itemVar;
        }
        TVar* parent = m_tempVar->getParent();
        parent->addChild(m_tempVar);
        m_varUnit->addTreeItem(m_pItem, m_tempVar);

        m_parent->addChild(m_pItem);
        QList<QTreeWidgetItem*> list;
        m_editor->recurseVariablesDown(m_pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = m_varUnit->getWVar(treeWidgetItem);
            TVar* vparent = v->getParent();
            const void* pval = vparent->pValue;
            if (v->getParent()->hidden) {
                v->setParent(m_tempVar);
            }
        }
        m_treeWidget_variables->setCurrentItem(m_pItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteVarCommand::redo()
{
    if (!m_editor) {
        return;
    }
    if (!m_host) {
        return;
    }
    if (m_pItem) {
        m_treeWidget_variables->setCurrentItem(m_pItem);
        if (!m_itemVar) {
            m_itemVar = new TVar();
            TVar* p = m_varUnit->getWVar(m_pItem);
            *m_itemVar = *p;
        }
        m_editor->delete_variable();
        m_tempVar = nullptr;
    }
    setText(QObject::tr("Delete Variable"));
}
MoveVariableCommand::MoveVariableCommand(
        VarUnit* varUnit, TTreeWidget* treeWidget_variables, QTreeWidgetItem* parentItem, QTreeWidgetItem* cItem, QTreeWidgetItem* prevParentItem, QUndoCommand* parent)
: QUndoCommand(parent)
{
    m_varUnit = varUnit;
    m_treeWidget_variables = treeWidget_variables;
    m_ParentItem = parentItem;
    m_pItem = cItem;
    m_PrevParentItem = prevParentItem;
}

void MoveVariableCommand::undo()
{
    if (!m_host) {
        return;
    }
    LuaInterface* lI = m_host->getLuaInterface();
    VarUnit* varUnit = lI->getVarUnit();
    if (!lI->reparentVariable(m_PrevParentItem, m_pItem, m_ParentItem)) {
        m_event->setDropAction(Qt::IgnoreAction);
        m_event->ignore();
    }
    m_ParentItem->removeChild(m_pItem);
    m_PrevParentItem->insertChild(m_PrevParentItem->childCount() <= 0 ? 0 : m_PrevParentItem->childCount(), m_pItem);
    m_treeWidget_variables->setCurrentItem(m_pItem);
}

void MoveVariableCommand::redo()
{
    LuaInterface* lI = m_host->getLuaInterface();
    VarUnit* varUnit = lI->getVarUnit();
    if (!lI->reparentVariable(m_ParentItem, m_pItem, m_PrevParentItem)) {
        m_event->setDropAction(Qt::IgnoreAction);
        m_event->ignore();
    }
    if (m_PrevParentItem) {
        m_PrevParentItem->removeChild(m_pItem);
    }
    if (m_ParentItem) {
        const int count = m_ParentItem->childCount();
        m_ParentItem->insertChild(count <= 0 ? 0 : count, m_pItem);
    }
    setText(QObject::tr("Move Variable"));
}
