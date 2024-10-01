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
    dlgTriggerEditor* mpEditor = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_triggers;
    TriggerUnit* mpTriggerUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
};

class DeleteTriggerCommand : public QUndoCommand
{
public:
    DeleteTriggerCommand(QTreeWidgetItem* pItem, TriggerUnit* triggerUnit, TTreeWidget* treeWidget_triggers, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_triggers;
    TriggerUnit* mpTriggerUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
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
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;
    QTreeWidgetItem* mpParentItem;
    QTreeWidgetItem* mpPrevParentItem;
    QTreeWidgetItem* mpItem;
    TTrigger* mpItemTrigger;
    TTreeWidget* mpTreeWidget_triggers;
    int mChildID;
    int mOldParentID;
    int mNewParentID;
    int mParentPosition;
    int mChildPosition;
    int mPrevParentPosition;
    int mPrevChildPosition;
    QModelIndex mParent;
    int mStart;
    int mEnd;

private:
    TriggerUnit* mpTriggerUnit;
    bool mIsFolder;
};

class AddAliasCommand : public QUndoCommand
{
public:
    AddAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_triggers, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_aliases;
    AliasUnit* mpAliasUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
};

class DeleteAliasCommand : public QUndoCommand
{
public:
    DeleteAliasCommand(QTreeWidgetItem* pItem, AliasUnit* aliasUnit, TTreeWidget* treeWidget_aliases, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_aliases;
    AliasUnit* mpAliasUnit;
    TAlias* mpItemAlias;
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
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;
    QTreeWidgetItem* mpParentItem;
    QTreeWidgetItem* mpPrevParentItem;
    QTreeWidgetItem* mpItem;
    TTrigger* mpItemAlias;
    TTreeWidget* mpTreeWidget_aliases;
    int mChildID;
    int mOldParentID;
    int mNewParentID;
    int mParentPosition;
    int mChildPosition;
    int mPrevParentPosition;
    int mPrevChildPosition;
    QModelIndex mParent;
    int mStart;
    int mEnd;

private:
    AliasUnit* mpAliasUnit;
    bool mIsFolder;
};

class AddTimerCommand : public QUndoCommand
{
public:
    AddTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_triggers, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_timers;
    TimerUnit* mpTimerUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
};

class DeleteTimerCommand : public QUndoCommand
{
public:
    DeleteTimerCommand(QTreeWidgetItem* pItem, TimerUnit* timerUnit, TTreeWidget* treeWidget_timers, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_timers;
    TimerUnit* mpTimerUnit;
    TTimer* mpItemTimer;
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
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;
    QTreeWidgetItem* mpParentItem;
    QTreeWidgetItem* mpPrevParentItem;
    QTreeWidgetItem* mpItem;
    TTimer* mpItemTimer;
    TTreeWidget* mpTreeWidget_timers;
    int mChildID;
    int mOldParentID;
    int mNewParentID;
    int mParentPosition;
    int mChildPosition;
    int mPrevParentPosition;
    int mPrevChildPosition;
    QModelIndex mParent;
    int mStart;
    int mEnd;

private:
    TimerUnit* mpTimerUnit;
    bool mIsFolder;
};

class AddScriptCommand : public QUndoCommand
{
public:
    AddScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_scripts;
    ScriptUnit* mpScriptUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
};

class DeleteScriptCommand : public QUndoCommand
{
public:
    DeleteScriptCommand(QTreeWidgetItem* pItem, ScriptUnit* scriptUnit, TTreeWidget* treeWidget_scripts, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_scripts;
    ScriptUnit* mpScriptUnit;
    TScript* mpItemScript;
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
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;
    QTreeWidgetItem* mpParentItem;
    QTreeWidgetItem* mpPrevParentItem;
    QTreeWidgetItem* mpItem;
    TScript* mpItemScript;
    TTreeWidget* mpTreeWidget_scripts;
    int mChildID;
    int mOldParentID;
    int mNewParentID;
    int mParentPosition;
    int mChildPosition;
    int mPrevParentPosition;
    int mPrevChildPosition;
    QModelIndex mParent;
    int mStart;
    int mEnd;

private:
    ScriptUnit* mpScriptUnit;
    bool mIsFolder;
};

class AddKeyCommand : public QUndoCommand
{
public:
    AddKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_keys;
    KeyUnit* mpKeyUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
};

class DeleteKeyCommand : public QUndoCommand
{
public:
    DeleteKeyCommand(QTreeWidgetItem* pItem, KeyUnit* keyUnit, TTreeWidget* treeWidget_keys, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_keys;
    KeyUnit* mpKeyUnit;
    TKey* mpItemKey;
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
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;
    QTreeWidgetItem* mpParentItem;
    QTreeWidgetItem* mpPrevParentItem;
    QTreeWidgetItem* mpItem;
    TKey* mpItemKey;
    TTreeWidget* mpTreeWidget_keys;
    int mChildID;
    int mOldParentID;
    int mNewParentID;
    int mParentPosition;
    int mChildPosition;
    int mPrevParentPosition;
    int mPrevChildPosition;
    QModelIndex mParent;
    int mStart;
    int mEnd;

private:
    KeyUnit* mpKeyUnit;
    bool mIsFolder;
};
class AddActionCommand : public QUndoCommand
{
public:
    AddActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_actions;
    ActionUnit* mpActionUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
};

class DeleteActionCommand : public QUndoCommand
{
public:
    DeleteActionCommand(QTreeWidgetItem* pItem, ActionUnit* actionUnit, TTreeWidget* treeWidget_actions, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_actions;
    ActionUnit* mpActionUnit;
    TAction* mpItemAction;
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
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;
    QTreeWidgetItem* mpParentItem;
    QTreeWidgetItem* mpPrevParentItem;
    QTreeWidgetItem* mpItem;
    TAction* mpItemAction;
    TTreeWidget* mpTreeWidget_actions;
    int mChildID;
    int mOldParentID;
    int mNewParentID;
    int mParentPosition;
    int mChildPosition;
    int mPrevParentPosition;
    int mPrevChildPosition;
    QModelIndex mParent;
    int mStart;
    int mEnd;

private:
    ActionUnit* mpActionUnit;
    bool mIsFolder;
};
class AddVarCommand : public QUndoCommand
{
public:
    AddVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, bool isFolder, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_variables;
    VarUnit* mpVarUnit;
    TTrigger* mpItemTrigger;
    bool mIsFolder;
};

class DeleteVarCommand : public QUndoCommand
{
public:
    DeleteVarCommand(QTreeWidgetItem* pItem, VarUnit* varUnit, TTreeWidget* treeWidget_variables, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;

private:
    QTreeWidgetItem* mpItem;
    QTreeWidgetItem* mpParent;
    TTreeWidget* mpTreeWidget_variables;
    VarUnit* mpVarUnit;
    TVar* mpItemVar = nullptr;
    TVar* mpTempVar = nullptr;
};
class MoveVariableCommand : public QUndoCommand, QTreeWidget
{
public:
    MoveVariableCommand(VarUnit* varUnit, TTreeWidget* treeWidget_variables, QTreeWidgetItem* parentItem, QTreeWidgetItem* cItem, QTreeWidgetItem* prevParentItem, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggerEditor* mpEditor = nullptr;
    QPointer<Host> mpHost = nullptr;
    QTreeWidgetItem* mpParentItem;
    QTreeWidgetItem* mpPrevParentItem;
    QTreeWidgetItem* mpItem;
    TVar* mpItemVar;
    TTreeWidget* mpTreeWidget_variables;
    QDropEvent* mpEvent;

private:
    VarUnit* mpVarUnit;
    bool mIsFolder;
};

class TriggerNameTextEditedCommand : public QUndoCommand
{
public:
    TriggerNameTextEditedCommand(dlgTriggersMainArea* triggersMainArea, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    dlgTriggersMainArea* mpTriggersMainArea = nullptr;
    QString mPrevLineEdit_trigger_name;
    QString mLineEdit_trigger_name;
};
#endif
