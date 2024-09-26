#ifndef MUDLET_DLGTRIGGERSCOMMANDS_H
#define MUDLET_DLGTRIGGERSCOMMANDS_H

#include "ActionUnit.h"
#include "AliasUnit.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "TTreeWidget.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "VarUnit.h"
#include "dlgTriggerEditor.h"
#include <QPointer>
#include <QUndoCommand>

class LuaInterface;
class dlgTriggerEditor;

class AddTriggerCommand : public QUndoCommand
{
public:
    AddTriggerCommand(QTreeWidgetItem* pItem, TriggerUnit* triggerUnit, TTreeWidget* treeWidget_triggers, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_triggers;
    TriggerUnit* m_triggerUnit;
    TTrigger* m_itemTrigger;
    bool m_isFolder;
};

class DeleteTriggerCommand : public QUndoCommand
{
public:
    DeleteTriggerCommand(QTreeWidgetItem* pItem, TriggerUnit* triggerUnit, TTreeWidget* treeWidget_triggers, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    TTrigger* m_itemTrigger;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_triggers;
    TriggerUnit* m_triggerUnit;
    bool m_isFolder;
};

class MoveTriggerCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveTriggerCommand(TriggerUnit* triggerUnit,
                       TTreeWidget* treeWidget_triggers,
                       int childID,
                       int oldParentID,
                       int newParentID,
                       int parentPosition,
                       int childPosition,
                       int prevParentPosition,
                       int prevChildPosition,
                       QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    QTreeWidgetItem* m_ParentItem;
    QTreeWidgetItem* m_PrevParentItem;
    QTreeWidgetItem* m_pItem;
    TTrigger* m_itemTrigger;
    TTreeWidget* m_treeWidget_triggers;
    int m_childID;
    int m_oldParentID;
    int m_newParentID;
    int m_parentPosition;
    int m_childPosition;
    int m_prevParentPosition;
    int m_prevChildPosition;
    QModelIndex m_parent;
    int m_start;
    int m_end;

private:
    TriggerUnit* m_triggerUnit;
    bool m_isFolder;
};

class AddAliasCommand : public QUndoCommand
{
public:
    AddAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_triggers, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_aliases;
    AliasUnit* m_aliasUnit;
    TTrigger* m_itemTrigger;
    bool m_isFolder;
};

class DeleteAliasCommand : public QUndoCommand
{
public:
    DeleteAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_aliases, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    TAlias* m_itemAlias;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_aliases;
    AliasUnit* m_aliasUnit;
};

class MoveAliasCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveAliasCommand(AliasUnit* aliasUnit,
                     TTreeWidget* m_treeWidget_aliases,
                     int childID,
                     int oldParentID,
                     int newParentID,
                     int parentPosition,
                     int childPosition,
                     int prevParentPosition,
                     int prevChildPosition,
                     QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    QTreeWidgetItem* m_ParentItem;
    QTreeWidgetItem* m_PrevParentItem;
    QTreeWidgetItem* m_pItem;
    TTrigger* m_itemAlias;
    TTreeWidget* m_treeWidget_aliases;
    int m_childID;
    int m_oldParentID;
    int m_newParentID;
    int m_parentPosition;
    int m_childPosition;
    int m_prevParentPosition;
    int m_prevChildPosition;
    QModelIndex m_parent;
    int m_start;
    int m_end;

private:
    AliasUnit* m_aliasUnit;
    bool m_isFolder;
};

class AddTimerCommand : public QUndoCommand
{
public:
    AddTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_triggers, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_timers;
    TimerUnit* m_timerUnit;
    TTrigger* m_itemTrigger;
    bool m_isFolder;
};

class DeleteTimerCommand : public QUndoCommand
{
public:
    DeleteTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_timers, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    TTimer* m_itemTimer;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_timers;
    TimerUnit* m_timerUnit;
};

class MoveTimerCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveTimerCommand(TimerUnit* timerUnit,
                     TTreeWidget* m_treeWidget_timers,
                     int childID,
                     int oldParentID,
                     int newParentID,
                     int parentPosition,
                     int childPosition,
                     int prevParentPosition,
                     int prevChildPosition,
                     QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    QTreeWidgetItem* m_ParentItem;
    QTreeWidgetItem* m_PrevParentItem;
    QTreeWidgetItem* m_pItem;
    TTimer* m_itemTimer;
    TTreeWidget* m_treeWidget_timers;
    int m_childID;
    int m_oldParentID;
    int m_newParentID;
    int m_parentPosition;
    int m_childPosition;
    int m_prevParentPosition;
    int m_prevChildPosition;
    QModelIndex m_parent;
    int m_start;
    int m_end;

private:
    TimerUnit* m_timerUnit;
    bool m_isFolder;
};

class AddScriptCommand : public QUndoCommand
{
public:
    AddScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_scripts;
    ScriptUnit* m_scriptUnit;
    TTrigger* m_itemTrigger;
    bool m_isFolder;
};

class DeleteScriptCommand : public QUndoCommand
{
public:
    DeleteScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    TScript* m_itemScript;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_scripts;
    ScriptUnit* m_scriptUnit;
};

class MoveScriptCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveScriptCommand(ScriptUnit* scriptUnit,
                      TTreeWidget* treeWidget_scripts,
                      int childID,
                      int oldParentID,
                      int newParentID,
                      int parentPosition,
                      int childPosition,
                      int prevParentPosition,
                      int prevChildPosition,
                      QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    QTreeWidgetItem* m_ParentItem;
    QTreeWidgetItem* m_PrevParentItem;
    QTreeWidgetItem* m_pItem;
    TScript* m_itemScript;
    TTreeWidget* m_treeWidget_scripts;
    int m_childID;
    int m_oldParentID;
    int m_newParentID;
    int m_parentPosition;
    int m_childPosition;
    int m_prevParentPosition;
    int m_prevChildPosition;
    QModelIndex m_parent;
    int m_start;
    int m_end;

private:
    ScriptUnit* m_scriptUnit;
    bool m_isFolder;
};

class AddKeyCommand : public QUndoCommand
{
public:
    AddKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_keys;
    KeyUnit* m_keyUnit;
    TTrigger* m_itemTrigger;
    bool m_isFolder;
};

class DeleteKeyCommand : public QUndoCommand
{
public:
    DeleteKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    TKey* m_itemKey;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_keys;
    KeyUnit* m_keyUnit;
};

class MoveKeyCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveKeyCommand(KeyUnit* keyUnit,
                   TTreeWidget* treeWidget_keys,
                   int childID,
                   int oldParentID,
                   int newParentID,
                   int parentPosition,
                   int childPosition,
                   int prevParentPosition,
                   int prevChildPosition,
                   QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    QTreeWidgetItem* m_ParentItem;
    QTreeWidgetItem* m_PrevParentItem;
    QTreeWidgetItem* m_pItem;
    TKey* m_itemKey;
    TTreeWidget* m_treeWidget_keys;
    int m_childID;
    int m_oldParentID;
    int m_newParentID;
    int m_parentPosition;
    int m_childPosition;
    int m_prevParentPosition;
    int m_prevChildPosition;
    QModelIndex m_parent;
    int m_start;
    int m_end;

private:
    KeyUnit* m_keyUnit;
    bool m_isFolder;
};
class AddActionCommand : public QUndoCommand
{
public:
    AddActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_actions;
    ActionUnit* m_actionUnit;
    TTrigger* m_itemTrigger;
    bool m_isFolder;
};

class DeleteActionCommand : public QUndoCommand
{
public:
    DeleteActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    TAction* m_itemAction;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_actions;
    ActionUnit* m_actionUnit;
};
class MoveActionCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveActionCommand(ActionUnit* actionUnit,
                      TTreeWidget* treeWidget_actions,
                      int childID,
                      int oldParentID,
                      int newParentID,
                      int parentPosition,
                      int childPosition,
                      int prevParentPosition,
                      int prevChildPosition,
                      QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    QTreeWidgetItem* m_ParentItem;
    QTreeWidgetItem* m_PrevParentItem;
    QTreeWidgetItem* m_pItem;
    TAction* m_itemAction;
    TTreeWidget* m_treeWidget_actions;
    int m_childID;
    int m_oldParentID;
    int m_newParentID;
    int m_parentPosition;
    int m_childPosition;
    int m_prevParentPosition;
    int m_prevChildPosition;
    QModelIndex m_parent;
    int m_start;
    int m_end;

private:
    ActionUnit* m_actionUnit;
    bool m_isFolder;
};
class AddVarCommand : public QUndoCommand
{
public:
    AddVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_variables;
    VarUnit* m_varUnit;
    TTrigger* m_itemTrigger;
    bool m_isFolder;
};

class DeleteVarCommand : public QUndoCommand
{
public:
    DeleteVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    TVar* m_itemVar = nullptr;
    TVar* m_tempVar = nullptr;

private:
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_parent;
    TTreeWidget* m_treeWidget_variables;
    VarUnit* m_varUnit;
};
class MoveVariableCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveVariableCommand(VarUnit* varUnit, TTreeWidget* treeWidget_variables, QTreeWidgetItem* parentItem, QTreeWidgetItem* cItem, QTreeWidgetItem* prevParentItem, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* m_editor = nullptr;
    QPointer<Host> m_host = nullptr;
    QTreeWidgetItem* m_ParentItem;
    QTreeWidgetItem* m_PrevParentItem;
    QTreeWidgetItem* m_pItem;
    TVar* m_itemVar;
    TTreeWidget* m_treeWidget_variables;
    QDropEvent* m_event;

private:
    VarUnit* m_varUnit;
    bool m_isFolder;
};
#endif
