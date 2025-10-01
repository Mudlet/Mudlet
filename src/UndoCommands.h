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

#ifndef MUDLET_UNDOCOMMANDS_H
#define MUDLET_UNDOCOMMANDS_H

#include <QUndoCommand>
#include <QVariant>

class TTrigger;
class TAlias;
class TTimer;
class TScript;
class TKey;
class TAction;
class dlgTriggerEditor;

class AddActionCommand : public QUndoCommand
{
public:
    AddActionCommand(TAction* parent, bool isFolder, dlgTriggerEditor* editor,
                        QUndoCommand* parentCmd = nullptr);
    ~AddActionCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TAction* mParent;
    bool mIsFolder;
    TAction* mNewAction;
    int mRow;
};

class DeleteActionCommand : public QUndoCommand
{
public:
    DeleteActionCommand(TAction* action, dlgTriggerEditor* editor,
                         QUndoCommand* parentCmd = nullptr);
    ~DeleteActionCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TAction* mAction;
    TAction* mParent;
    int mRow;
};

class ChangeActionPropertyCommand : public QUndoCommand
{
public:
    enum ActionProperty {
        Name,
        Icon,
        Script,
        CommandButtonUp,
        CommandButtonDown,
        IsPushDownButton,
        ButtonRotation,
        ButtonColumns,
        Css,
        Location,
        Orientation
    };

    ChangeActionPropertyCommand(TAction* action, ActionProperty property,
                                   const QVariant& oldValue, const QVariant& newValue,
                                   dlgTriggerEditor* editor,
                                   QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    void setProperty(const QVariant& value);

    dlgTriggerEditor* mEditor;
    TAction* mAction;
    ActionProperty mProperty;
    QVariant mOldValue;
    QVariant mNewValue;
};

class ActivateActionCommand : public QUndoCommand
{
public:
    ActivateActionCommand(TAction* action, dlgTriggerEditor* editor,
                             QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TAction* mAction;
    bool mOldState;
};

class AddKeyCommand : public QUndoCommand
{
public:
    AddKeyCommand(TKey* parent, bool isFolder, dlgTriggerEditor* editor,
                        QUndoCommand* parentCmd = nullptr);
    ~AddKeyCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TKey* mParent;
    bool mIsFolder;
    TKey* mNewKey;
    int mRow;
};

class DeleteKeyCommand : public QUndoCommand
{
public:
    DeleteKeyCommand(TKey* key, dlgTriggerEditor* editor,
                         QUndoCommand* parentCmd = nullptr);
    ~DeleteKeyCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TKey* mKey;
    TKey* mParent;
    int mRow;
};

class ChangeKeyPropertyCommand : public QUndoCommand
{
public:
    enum KeyProperty {
        Name,
        Command,
        Script,
        KeyCode,
        KeyModifiers
    };

    ChangeKeyPropertyCommand(TKey* key, KeyProperty property,
                                   const QVariant& oldValue, const QVariant& newValue,
                                   dlgTriggerEditor* editor,
                                   QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    void setProperty(const QVariant& value);

    dlgTriggerEditor* mEditor;
    TKey* mKey;
    KeyProperty mProperty;
    QVariant mOldValue;
    QVariant mNewValue;
};

class ActivateKeyCommand : public QUndoCommand
{
public:
    ActivateKeyCommand(TKey* key, dlgTriggerEditor* editor,
                             QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TKey* mKey;
    bool mOldState;
};

class AddScriptCommand : public QUndoCommand
{
public:
    AddScriptCommand(TScript* parent, bool isFolder, dlgTriggerEditor* editor,
                        QUndoCommand* parentCmd = nullptr);
    ~AddScriptCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TScript* mParent;
    bool mIsFolder;
    TScript* mNewScript;
    int mRow;
};

class DeleteScriptCommand : public QUndoCommand
{
public:
    DeleteScriptCommand(TScript* script, dlgTriggerEditor* editor,
                         QUndoCommand* parentCmd = nullptr);
    ~DeleteScriptCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TScript* mScript;
    TScript* mParent;
    int mRow;
};

class ChangeScriptPropertyCommand : public QUndoCommand
{
public:
    enum ScriptProperty {
        Name,
        Script,
        EventHandlerList
    };

    ChangeScriptPropertyCommand(TScript* script, ScriptProperty property,
                                   const QVariant& oldValue, const QVariant& newValue,
                                   dlgTriggerEditor* editor,
                                   QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    void setProperty(const QVariant& value);

    dlgTriggerEditor* mEditor;
    TScript* mScript;
    ScriptProperty mProperty;
    QVariant mOldValue;
    QVariant mNewValue;
};

class ActivateScriptCommand : public QUndoCommand
{
public:
    ActivateScriptCommand(TScript* script, dlgTriggerEditor* editor,
                             QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TScript* mScript;
    bool mOldState;
};

class AddTimerCommand : public QUndoCommand
{
public:
    AddTimerCommand(TTimer* parent, bool isFolder, dlgTriggerEditor* editor,
                      QUndoCommand* parentCmd = nullptr);
    ~AddTimerCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TTimer* mParent;
    bool mIsFolder;
    TTimer* mNewTimer;
    int mRow;
};

class DeleteTimerCommand : public QUndoCommand
{
public:
    DeleteTimerCommand(TTimer* timer, dlgTriggerEditor* editor,
                       QUndoCommand* parentCmd = nullptr);
    ~DeleteTimerCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TTimer* mTimer;
    TTimer* mParent;
    int mRow;
};

class ChangeTimerPropertyCommand : public QUndoCommand
{
public:
    enum TimerProperty {
        Name,
        Command,
        Script,
        Time
    };

    ChangeTimerPropertyCommand(TTimer* timer, TimerProperty property,
                                 const QVariant& oldValue, const QVariant& newValue,
                                 dlgTriggerEditor* editor,
                                 QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    void setProperty(const QVariant& value);

    dlgTriggerEditor* mEditor;
    TTimer* mTimer;
    TimerProperty mProperty;
    QVariant mOldValue;
    QVariant mNewValue;
};

class ActivateTimerCommand : public QUndoCommand
{
public:
    ActivateTimerCommand(TTimer* timer, dlgTriggerEditor* editor,
                           QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TTimer* mTimer;
    bool mOldState;
};


class AddAliasCommand : public QUndoCommand
{
public:
    AddAliasCommand(TAlias* parent, bool isFolder, dlgTriggerEditor* editor,
                      QUndoCommand* parentCmd = nullptr);
    ~AddAliasCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TAlias* mParent;
    bool mIsFolder;
    TAlias* mNewAlias;
    int mRow;
};

class DeleteAliasCommand : public QUndoCommand
{
public:
    DeleteAliasCommand(TAlias* alias, dlgTriggerEditor* editor,
                       QUndoCommand* parentCmd = nullptr);
    ~DeleteAliasCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TAlias* mAlias;
    TAlias* mParent;
    int mRow;
};

class ChangeAliasPropertyCommand : public QUndoCommand
{
public:
    enum AliasProperty {
        Name,
        Command,
        Script,
        RegexCode
    };

    ChangeAliasPropertyCommand(TAlias* alias, AliasProperty property,
                                 const QVariant& oldValue, const QVariant& newValue,
                                 dlgTriggerEditor* editor,
                                 QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    void setProperty(const QVariant& value);

    dlgTriggerEditor* mEditor;
    TAlias* mAlias;
    AliasProperty mProperty;
    QVariant mOldValue;
    QVariant mNewValue;
};

class ActivateAliasCommand : public QUndoCommand
{
public:
    ActivateAliasCommand(TAlias* alias, dlgTriggerEditor* editor,
                           QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TAlias* mAlias;
    bool mOldState;
};

class AddTriggerCommand : public QUndoCommand
{
public:
    AddTriggerCommand(TTrigger* parent, bool isFolder, dlgTriggerEditor* editor,
                      QUndoCommand* parentCmd = nullptr);
    ~AddTriggerCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TTrigger* mParent;
    bool mIsFolder;
    TTrigger* mNewTrigger;
    int mRow;
};

class DeleteTriggerCommand : public QUndoCommand
{
public:
    DeleteTriggerCommand(TTrigger* trigger, dlgTriggerEditor* editor,
                       QUndoCommand* parentCmd = nullptr);
    ~DeleteTriggerCommand();

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TTrigger* mTrigger;
    TTrigger* mParent;
    int mRow;
    // We also need to store the state of the trigger's children if it's a folder
    // For simplicity, we will re-implement this later if needed.
};

class ChangeTriggerPropertyCommand : public QUndoCommand
{
public:
    enum TriggerProperty {
        Name,
        Command,
        Script,
        Patterns,
        IsMultiline,
        ConditionLineDelta,
        StayOpen,
        PerlSlashGOption,
        FilterTrigger,
        SoundTrigger,
        SoundFile,
        IsColorizerTrigger,
        FgColor,
        BgColor
    };

    ChangeTriggerPropertyCommand(TTrigger* trigger, TriggerProperty property,
                                 const QVariant& oldValue, const QVariant& newValue,
                                 dlgTriggerEditor* editor,
                                 QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    void setProperty(const QVariant& value);

    dlgTriggerEditor* mEditor;
    TTrigger* mTrigger;
    TriggerProperty mProperty;
    QVariant mOldValue;
    QVariant mNewValue;
};

class ActivateTriggerCommand : public QUndoCommand
{
public:
    ActivateTriggerCommand(TTrigger* trigger, dlgTriggerEditor* editor,
                           QUndoCommand* parentCmd = nullptr);

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    TTrigger* mTrigger;
    bool mOldState;
};

#endif // MUDLET_UNDOCOMMANDS_H