#include "dlgTriggerEditorCommand.h"
#include "Host.h"
#include "LuaInterface.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include <QPointer>

AddTriggerCommand::AddTriggerCommand(QTreeWidgetItem* pItem, TriggerUnit* triggerUnit, TTreeWidget* treeWidget_triggers, bool isFolder, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpTriggerUnit = triggerUnit;
    mpTreeWidget_triggers = treeWidget_triggers;
    mIsFolder = isFolder;
    mpItem = pItem;
}

void AddTriggerCommand::undo()
{
    if (!mpItem) {
        return;
    }
    mpParent = mpItem->parent();

    if (mpParent) {
        mpParent->removeChild(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddTriggerCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpItem) {
        mpEditor->addTrigger(mIsFolder);
        mpItem = mpTreeWidget_triggers->currentItem();
        mpParent = mpItem->parent();
    } else {
        int count = mpParent->childCount();
        if (mIsFolder) {
            mpParent->addChild(mpItem);
        } else {
            mpParent->insertChild(count <= 0 ? 0 : count, mpItem);
        }
    }

    setText(QObject::tr("Add Trigger"));
}

DeleteTriggerCommand::DeleteTriggerCommand(QTreeWidgetItem* pItem, TriggerUnit* triggerUnit, TTreeWidget* treeWidget_triggers, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpItem = pItem;
    mpParent = mpItem->parent();
    mpTriggerUnit = triggerUnit;
    mpTreeWidget_triggers = treeWidget_triggers;
}

void DeleteTriggerCommand::undo()
{
    if (!mpItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (mpParent) {
        const int childID = mpItemTrigger->getID();
        mpItem->setData(0, Qt::UserRole, childID);
        mpParent->insertChild(mpParent->childCount() <= 0 ? 0 : mpParent->childCount(), mpItem);
        mpTreeWidget_triggers->setCurrentItem(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}
void DeleteTriggerCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpHost) {
        return;
    }
    if (mpItem) {
        const int ID = mpItem->data(0, Qt::UserRole).toInt();
        TTrigger* p = mpTriggerUnit->getTrigger(ID);
        TTrigger* ptr = new TTrigger(p->mName, p->mPatterns, p->getRegexCodePropertyList(), false, mpHost);
        ptr->registerTrigger();
        mpItemTrigger = ptr;
        mpEditor->delete_trigger();
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
    mChildID = childID;
    mOldParentID = oldParentID;
    mNewParentID = newParentID;
    mParentPosition = parentPosition;
    mChildPosition = childPosition;
    mPrevParentPosition = prevParentPosition;
    mPrevChildPosition = prevChildPosition;
    mpTriggerUnit = triggerUnit;
    mpTreeWidget_triggers = treeWidget_triggers;
}

void MoveTriggerCommand::undo()
{
    if (!mpHost) {
        return;
    }

    mpHost->getTriggerUnit()->reParentTrigger(mChildID, mNewParentID, mOldParentID, mPrevParentPosition, mPrevChildPosition);
    mpParentItem->removeChild(mpItem);
    mpPrevParentItem->insertChild(mpPrevParentItem->childCount() <= 0 ? 0 : mpParentItem->childCount(), mpItem);
    mpTreeWidget_triggers->setCurrentItem(mpItem);
}

void MoveTriggerCommand::redo()
{
    mpHost->getTriggerUnit()->reParentTrigger(mChildID, mOldParentID, mNewParentID, mParentPosition, mChildPosition);
    if (mpPrevParentItem) {
        mpPrevParentItem->removeChild(mpItem);
    }
    if (mpParentItem) {
        const int count = mpParentItem->childCount();
        mpParentItem->insertChild(count <= 0 ? 0 : count, mpItem);
    }

    setText(QObject::tr("Move Trigger"));
}

AddAliasCommand::AddAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_aliases, bool isFolder, QUndoCommand* parent)
{
    mpAliasUnit = aliasUnit;
    mpTreeWidget_aliases = treeWidget_aliases;
    mIsFolder = isFolder;
    mpItem = pItem;
}

void AddAliasCommand::undo()
{
    if (!mpItem) {
        return;
    }
    mpParent = mpItem->parent();

    if (mpParent) {
        mpParent->removeChild(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddAliasCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpItem) {
        mpEditor->addAlias(mIsFolder);
        mpItem = mpTreeWidget_aliases->currentItem();
        mpParent = mpItem->parent();
    } else {
        int count = mpParent->childCount();
        if (mIsFolder) {
            mpParent->addChild(mpItem);
        } else {
            mpParent->insertChild(count <= 0 ? 0 : count, mpItem);
        }
    }

    setText(QObject::tr("Add Alias"));
}

DeleteAliasCommand::DeleteAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_aliases, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpItem = pItem;
    mpParent = mpItem->parent();
    mpAliasUnit = aliasUnit;
    mpTreeWidget_aliases = treeWidget_aliases;
}

void DeleteAliasCommand::undo()
{
    if (!mpItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (mpParent) {
        const int childID = mpItemAlias->getID();
        mpItem->setData(0, Qt::UserRole, childID);
        mpParent->insertChild(mpParent->childCount() <= 0 ? 0 : mpParent->childCount(), mpItem);
        mpTreeWidget_aliases->setCurrentItem(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteAliasCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpHost) {
        return;
    }
    if (mpItem) {
        const int ID = mpItem->data(0, Qt::UserRole).toInt();
        TAlias* p = mpAliasUnit->getAlias(ID);
        TAlias* ptr = new TAlias(p->mName, mpHost);
        ptr->registerAlias();
        mpItemAlias = ptr;
        mpEditor->delete_alias();
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
    mChildID = childID;
    mOldParentID = oldParentID;
    mNewParentID = newParentID;
    mParentPosition = parentPosition;
    mChildPosition = childPosition;
    mPrevParentPosition = prevParentPosition;
    mPrevChildPosition = prevChildPosition;
    mpAliasUnit = aliasUnit;
    mpTreeWidget_aliases = treeWidget_aliases;
}

void MoveAliasCommand::undo()
{
    if (!mpHost) {
        return;
    }

    mpHost->getAliasUnit()->reParentAlias(mChildID, mNewParentID, mOldParentID, mPrevParentPosition, mPrevChildPosition);
    mpParentItem->removeChild(mpItem);
    mpPrevParentItem->insertChild(mpPrevParentItem->childCount() <= 0 ? 0 : mpPrevParentItem->childCount(), mpItem);
    mpTreeWidget_aliases->setCurrentItem(mpItem);
}

void MoveAliasCommand::redo()
{
    mpHost->getAliasUnit()->reParentAlias(mChildID, mOldParentID, mNewParentID, mParentPosition, mChildPosition);
    if (mpPrevParentItem) {
        mpPrevParentItem->removeChild(mpItem);
    }
    if (mpParentItem) {
        const int count = mpParentItem->childCount();
        mpParentItem->insertChild(count <= 0 ? 0 : count, mpItem);
    }

    setText(QObject::tr("Move Alias"));
}

AddTimerCommand::AddTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_timers, bool isFolder, QUndoCommand* parent)
{
    mpTimerUnit = timerUnit;
    mpTreeWidget_timers = treeWidget_timers;
    mIsFolder = isFolder;
    mpItem = pItem;
}

void AddTimerCommand::undo()
{
    if (!mpItem) {
        return;
    }
    mpParent = mpItem->parent();

    if (mpParent) {
        mpParent->removeChild(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddTimerCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpItem) {
        mpEditor->addTimer(mIsFolder);
        mpItem = mpTreeWidget_timers->currentItem();
        mpParent = mpItem->parent();
    } else {
        int count = mpParent->childCount();
        if (mIsFolder) {
            mpParent->addChild(mpItem);
        } else {
            mpParent->insertChild(count <= 0 ? 0 : count, mpItem);
        }
    }

    setText(QObject::tr("Add Timer"));
}

DeleteTimerCommand::DeleteTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_timers, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpItem = pItem;
    mpParent = mpItem->parent();
    mpTimerUnit = timerUnit;
    mpTreeWidget_timers = treeWidget_timers;
}

void DeleteTimerCommand::undo()
{
    if (!mpItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (mpParent) {
        const int childID = mpItemTimer->getID();
        mpItem->setData(0, Qt::UserRole, childID);
        mpParent->insertChild(mpParent->childCount() <= 0 ? 0 : mpParent->childCount(), mpItem);
        mpTreeWidget_timers->setCurrentItem(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteTimerCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpHost) {
        return;
    }
    if (mpItem) {
        const int ID = mpItem->data(0, Qt::UserRole).toInt();
        TTimer* p = mpTimerUnit->getTimer(ID);
        TTimer* ptr = new TTimer(p->getName(), p->getTime(), mpHost);
        mpHost->getTimerUnit()->registerTimer(ptr);
        mpItemTimer = ptr;
        mpEditor->delete_timer();
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
    mChildID = childID;
    mOldParentID = oldParentID;
    mNewParentID = newParentID;
    mParentPosition = parentPosition;
    mChildPosition = childPosition;
    mPrevParentPosition = prevParentPosition;
    mPrevChildPosition = prevChildPosition;
    mpTimerUnit = timerUnit;
    mpTreeWidget_timers = treeWidget_timers;
}

void MoveTimerCommand::undo()
{
    if (!mpHost) {
        return;
    }

    mpHost->getTimerUnit()->reParentTimer(mChildID, mNewParentID, mOldParentID, mPrevParentPosition, mPrevChildPosition);
    mpParentItem->removeChild(mpItem);
    mpPrevParentItem->insertChild(mpPrevParentItem->childCount() <= 0 ? 0 : mpPrevParentItem->childCount(), mpItem);
    mpTreeWidget_timers->setCurrentItem(mpItem);
}

void MoveTimerCommand::redo()
{
    mpHost->getTimerUnit()->reParentTimer(mChildID, mOldParentID, mNewParentID, mParentPosition, mChildPosition);
    if (mpPrevParentItem) {
        mpPrevParentItem->removeChild(mpItem);
    }
    if (mpParentItem) {
        const int count = mpParentItem->childCount();
        mpParentItem->insertChild(count <= 0 ? 0 : count, mpItem);
    }

    setText(QObject::tr("Move Timer"));
}

AddScriptCommand::AddScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, bool isFolder, QUndoCommand* parent)
{
    mpScriptUnit = scriptUnit;
    mpTreeWidget_scripts = treeWidget_scripts;
    mIsFolder = isFolder;
    mpItem = pItem;
}

void AddScriptCommand::undo()
{
    if (!mpItem) {
        return;
    }
    mpParent = mpItem->parent();

    if (mpParent) {
        mpParent->removeChild(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddScriptCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpItem) {
        mpEditor->addScript(mIsFolder);
        mpItem = mpTreeWidget_scripts->currentItem();
        mpParent = mpItem->parent();
    } else {
        int count = mpParent->childCount();
        if (mIsFolder) {
            mpParent->addChild(mpItem);
        } else {
            mpParent->insertChild(count <= 0 ? 0 : count, mpItem);
        }
    }

    setText(QObject::tr("Add Script"));
}

DeleteScriptCommand::DeleteScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpItem = pItem;
    mpParent = mpItem->parent();
    mpScriptUnit = scriptUnit;
    mpTreeWidget_scripts = treeWidget_scripts;
}

void DeleteScriptCommand::undo()
{
    if (!mpItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (mpParent) {
        const int childID = mpItemScript->getID();
        mpItem->setData(0, Qt::UserRole, childID);
        mpParent->insertChild(mpParent->childCount() <= 0 ? 0 : mpParent->childCount(), mpItem);
        mpTreeWidget_scripts->setCurrentItem(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteScriptCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpHost) {
        return;
    }
    if (mpItem) {
        const int ID = mpItem->data(0, Qt::UserRole).toInt();
        TScript* p = mpScriptUnit->getScript(ID);
        TScript* ptr = new TScript(p->getName(), mpHost);
        ptr->registerScript();
        mpItemScript = ptr;
        mpEditor->delete_script();
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
    mChildID = childID;
    mOldParentID = oldParentID;
    mNewParentID = newParentID;
    mParentPosition = parentPosition;
    mChildPosition = childPosition;
    mPrevParentPosition = prevParentPosition;
    mPrevChildPosition = prevChildPosition;
    mpScriptUnit = scriptUnit;
    mpTreeWidget_scripts = treeWidget_scripts;
}

void MoveScriptCommand::undo()
{
    if (!mpHost) {
        return;
    }

    mpHost->getScriptUnit()->reParentScript(mChildID, mNewParentID, mOldParentID, mPrevParentPosition, mPrevChildPosition);
    mpParentItem->removeChild(mpItem);
    mpPrevParentItem->insertChild(mpPrevParentItem->childCount() <= 0 ? 0 : mpPrevParentItem->childCount(), mpItem);
    mpTreeWidget_scripts->setCurrentItem(mpItem);
}

void MoveScriptCommand::redo()
{
    mpHost->getScriptUnit()->reParentScript(mChildID, mOldParentID, mNewParentID, mParentPosition, mChildPosition);
    if (mpPrevParentItem) {
        mpPrevParentItem->removeChild(mpItem);
    }
    if (mpParentItem) {
        const int count = mpParentItem->childCount();
        mpParentItem->insertChild(count <= 0 ? 0 : count, mpItem);
    }

    setText(QObject::tr("Move Script"));
}

AddKeyCommand::AddKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, bool isFolder, QUndoCommand* parent)
{
    mpKeyUnit = keyUnit;
    mpTreeWidget_keys = treeWidget_keys;
    mIsFolder = isFolder;
    mpItem = pItem;
}

void AddKeyCommand::undo()
{
    if (!mpItem) {
        return;
    }
    mpParent = mpItem->parent();

    if (mpParent) {
        mpParent->removeChild(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddKeyCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpItem) {
        mpEditor->addKey(mIsFolder);
        mpItem = mpTreeWidget_keys->currentItem();
        mpParent = mpItem->parent();
    } else {
        int count = mpParent->childCount();
        if (mIsFolder) {
            mpParent->addChild(mpItem);
        } else {
            mpParent->insertChild(count <= 0 ? 0 : count, mpItem);
        }
    }

    setText(QObject::tr("Add Key"));
}

DeleteKeyCommand::DeleteKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpItem = pItem;
    mpParent = mpItem->parent();
    mpKeyUnit = keyUnit;
    mpTreeWidget_keys = treeWidget_keys;
}

void DeleteKeyCommand::undo()
{
    if (!mpItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (mpParent) {
        const int childID = mpItemKey->getID();
        mpItem->setData(0, Qt::UserRole, childID);
        mpParent->insertChild(mpParent->childCount() <= 0 ? 0 : mpParent->childCount(), mpParent);
        mpTreeWidget_keys->setCurrentItem(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteKeyCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpHost) {
        return;
    }
    if (mpItem) {
        const int ID = mpItem->data(0, Qt::UserRole).toInt();
        TKey* p = mpKeyUnit->getKey(ID);
        TKey* ptr = new TKey(p->getName(), mpHost);
        ptr->registerKey();
        mpItemKey = ptr;
        mpEditor->delete_key();
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
    mChildID = childID;
    mOldParentID = oldParentID;
    mNewParentID = newParentID;
    mParentPosition = parentPosition;
    mChildPosition = childPosition;
    mPrevParentPosition = prevParentPosition;
    mPrevChildPosition = prevChildPosition;
    mpKeyUnit = keyUnit;
    mpTreeWidget_keys = treeWidget_keys;
}

void MoveKeyCommand::undo()
{
    if (!mpHost) {
        return;
    }

    mpHost->getKeyUnit()->reParentKey(mChildID, mNewParentID, mOldParentID, mPrevParentPosition, mPrevChildPosition);
    mpParentItem->removeChild(mpItem);
    mpPrevParentItem->insertChild(mpPrevParentItem->childCount() <= 0 ? 0 : mpPrevParentItem->childCount(), mpItem);
    mpTreeWidget_keys->setCurrentItem(mpItem);
}

void MoveKeyCommand::redo()
{
    mpHost->getKeyUnit()->reParentKey(mChildID, mOldParentID, mNewParentID, mParentPosition, mChildPosition);
    if (mpPrevParentItem) {
        mpPrevParentItem->removeChild(mpItem);
    }
    if (mpParentItem) {
        const int count = mpParentItem->childCount();
        mpParentItem->insertChild(count <= 0 ? 0 : count, mpItem);
    }

    setText(QObject::tr("Move Key"));
}
AddActionCommand::AddActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, bool isFolder, QUndoCommand* parent)
{
    mpActionUnit = actionUnit;
    mpTreeWidget_actions = treeWidget_actions;
    mIsFolder = isFolder;
    mpItem = pItem;
}

void AddActionCommand::undo()
{
    if (!mpItem) {
        return;
    }
    mpParent = mpItem->parent();

    if (mpParent) {
        mpParent->removeChild(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddActionCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpItem) {
        mpEditor->addAction(mpItem);
        mpItem = mpTreeWidget_actions->currentItem();
        mpParent = mpItem->parent();
    } else {
        int count = mpParent->childCount();
        if (mIsFolder) {
            mpParent->addChild(mpItem);
        } else {
            mpParent->insertChild(count <= 0 ? 0 : count, mpItem);
        }
    }

    setText(QObject::tr("Add Action"));
}
DeleteActionCommand::DeleteActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpItem = pItem;
    mpParent = mpItem->parent();
    mpActionUnit = actionUnit;
    mpTreeWidget_actions = treeWidget_actions;
}

void DeleteActionCommand::undo()
{
    if (!mpItem) {
        return;
    }

    TTrigger* pT = nullptr;
    if (mpParent) {
        const int childID = mpItemAction->getID();
        mpItem->setData(0, Qt::UserRole, childID);
        mpParent->insertChild(mpParent->childCount() <= 0 ? 0 : mpParent->childCount(), mpItem);
        mpTreeWidget_actions->setCurrentItem(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteActionCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpHost) {
        return;
    }
    if (mpItem) {
        const int ID = mpItem->data(0, Qt::UserRole).toInt();
        TAction* p = mpActionUnit->getAction(ID);
        TAction* ptr = new TAction(p->getName(), mpHost);
        ptr->registerAction();
        mpItemAction = ptr;
        mpEditor->delete_action();
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
    mChildID = childID;
    mOldParentID = oldParentID;
    mNewParentID = newParentID;
    mParentPosition = parentPosition;
    mChildPosition = childPosition;
    mPrevParentPosition = prevParentPosition;
    mPrevChildPosition = prevChildPosition;
    mpActionUnit = actionUnit;
    mpTreeWidget_actions = treeWidget_actions;
}

void MoveActionCommand::undo()
{
    if (!mpHost) {
        return;
    }

    mpHost->getActionUnit()->reParentAction(mChildID, mNewParentID, mOldParentID, mPrevParentPosition, mPrevChildPosition);
    mpParentItem->removeChild(mpItem);
    mpPrevParentItem->insertChild(mpPrevParentItem->childCount() <= 0 ? 0 : mpPrevParentItem->childCount(), mpItem);
    mpTreeWidget_actions->setCurrentItem(mpItem);
    mpHost->getActionUnit()->updateToolbar();
}

void MoveActionCommand::redo()
{
    mpHost->getActionUnit()->reParentAction(mChildID, mOldParentID, mNewParentID, mParentPosition, mChildPosition);
    if (mpPrevParentItem) {
        mpPrevParentItem->removeChild(mpItem);
    }
    if (mpParentItem) {
        const int count = mpParentItem->childCount();
        mpParentItem->insertChild(count <= 0 ? 0 : count, mpItem);
    }
    mpHost->getActionUnit()->updateToolbar();

    setText(QObject::tr("Move Action"));
}
AddVarCommand::AddVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, bool isFolder, QUndoCommand* parent)
{
    mpVarUnit = varUnit;
    mpTreeWidget_variables = treeWidget_variables;
    mIsFolder = isFolder;
    mpItem = pItem;
}

void AddVarCommand::undo()
{
    if (!mpItem) {
        return;
    }

    mpParent = mpItem->parent();

    if (mpParent) {
        mpParent->removeChild(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void AddVarCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpItem) {
        mpEditor->addVar(mIsFolder);
        mpItem = mpTreeWidget_variables->currentItem();
        mpParent = mpItem->parent();
    } else {
        if (!mpParent) {
            if (mpItem->parent()) {
                mpParent = mpItem->parent();
            }
        }
        int count = mpParent->childCount();
        if (mIsFolder) {
            mpParent->addChild(mpItem);
        } else {
            mpParent->insertChild(count <= 0 ? 0 : count, mpItem);
        }
    }

    setText(QObject::tr("Add Variable"));
}
DeleteVarCommand::DeleteVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, QUndoCommand* parent) : QUndoCommand(parent)
{
    mpItem = pItem;
    mpParent = mpItem->parent();
    mpVarUnit = varUnit;
    mpTreeWidget_variables = treeWidget_variables;
}

void DeleteVarCommand::undo()
{
    if (!mpItem) {
        return;
    }

    if (mpParent) {
        if (!mpTempVar) {
            mpTempVar = new TVar();
            *mpTempVar = *mpItemVar;
        }
        TVar* parent = mpTempVar->getParent();
        parent->addChild(mpTempVar);
        mpVarUnit->addTreeItem(mpItem, mpTempVar);

        mpParent->addChild(mpItem);
        QList<QTreeWidgetItem*> list;
        mpEditor->recurseVariablesDown(mpItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = mpVarUnit->getWVar(treeWidgetItem);
            TVar* vparent = v->getParent();
            const void* pval = vparent->pValue;
            if (v->getParent()->hidden) {
                v->setParent(mpTempVar);
            }
        }
        mpTreeWidget_variables->setCurrentItem(mpItem);
    } else {
        qDebug() << "parent is null ";
    }
}

void DeleteVarCommand::redo()
{
    if (!mpEditor) {
        return;
    }
    if (!mpHost) {
        return;
    }
    if (mpItem) {
        mpTreeWidget_variables->setCurrentItem(mpItem);
        if (!mpItemVar) {
            mpItemVar = new TVar();
            TVar* p = mpVarUnit->getWVar(mpItem);
            *mpItemVar = *p;
        }
        mpEditor->delete_variable();
        mpTempVar = nullptr;
    }
    setText(QObject::tr("Delete Variable"));
}
MoveVariableCommand::MoveVariableCommand(
        VarUnit* varUnit, TTreeWidget* treeWidget_variables, QTreeWidgetItem* parentItem, QTreeWidgetItem* cItem, QTreeWidgetItem* prevParentItem, QUndoCommand* parent)
: QUndoCommand(parent)
{
    mpVarUnit = varUnit;
    mpTreeWidget_variables = treeWidget_variables;
    mpParentItem = parentItem;
    mpItem = cItem;
    mpPrevParentItem = prevParentItem;
}

void MoveVariableCommand::undo()
{
    if (!mpHost) {
        return;
    }
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* varUnit = lI->getVarUnit();
    if (!lI->reparentVariable(mpPrevParentItem, mpItem, mpParentItem)) {
        mpEvent->setDropAction(Qt::IgnoreAction);
        mpEvent->ignore();
    }
    mpParentItem->removeChild(mpItem);
    mpPrevParentItem->insertChild(mpPrevParentItem->childCount() <= 0 ? 0 : mpPrevParentItem->childCount(), mpItem);
    mpTreeWidget_variables->setCurrentItem(mpItem);
}

void MoveVariableCommand::redo()
{
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* varUnit = lI->getVarUnit();
    if (!lI->reparentVariable(mpParentItem, mpItem, mpPrevParentItem)) {
        mpEvent->setDropAction(Qt::IgnoreAction);
        mpEvent->ignore();
    }
    if (mpPrevParentItem) {
        mpPrevParentItem->removeChild(mpItem);
    }
    if (mpParentItem) {
        const int count = mpParentItem->childCount();
        mpParentItem->insertChild(count <= 0 ? 0 : count, mpItem);
    }
    setText(QObject::tr("Move Variable"));
}
