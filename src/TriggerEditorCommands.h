#ifndef MUDLET_TRIGGEREDITORCOMMANDS_H
#define MUDLET_TRIGGEREDITORCOMMANDS_H

/***************************************************************************
 *   Copyright (C) 2025 by Mudlet developers                              *
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

#include <QUndoCommand>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QString>
#include <memory>

class dlgTriggerEditor;
class TTrigger;
class TAlias;
class TTimer;
class TScript;
class TAction;
class TKey;

// Base class for item deletions
class DeleteItemCommand : public QUndoCommand
{
public:
    DeleteItemCommand(dlgTriggerEditor* editor, QTreeWidgetItem* item, const QString& itemType, QUndoCommand* parent = nullptr);
    ~DeleteItemCommand() override = default;

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    QTreeWidgetItem* mItem;
    QString mItemType;
    int mItemId;
    QString mItemName;
    QTreeWidgetItem* mParentItem;
    int mIndexInParent;
    
    // Store serialized item data for restoration
    QByteArray mSerializedData;
    
    bool mIsFirstRedo = true;
    
    void saveItemData();
    void restoreItem();
    void deleteItem();
};

// Command for adding items
class AddItemCommand : public QUndoCommand
{
public:
    AddItemCommand(dlgTriggerEditor* editor, const QString& itemType, bool isGroup, QTreeWidgetItem* parentItem = nullptr, QUndoCommand* parent = nullptr);
    ~AddItemCommand() override = default;

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    QString mItemType;
    bool mIsGroup;
    int mCreatedItemId = -1;
    int mParentItemId = -1;  // Store parent folder ID to maintain context
    QString mParentItemName; // Store parent name for fallback
    
    void createItem();
    void deleteCreatedItem();
};

// Command for property changes
class PropertyChangeCommand : public QUndoCommand
{
public:
    PropertyChangeCommand(dlgTriggerEditor* editor, int itemId, const QString& itemType,
                         const QString& propertyName, const QVariant& oldValue, 
                         const QVariant& newValue, QUndoCommand* parent = nullptr);
    ~PropertyChangeCommand() override = default;

    void undo() override;
    void redo() override;
    
    // Merge similar property changes
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

private:
    dlgTriggerEditor* mEditor;
    int mItemId;
    QString mItemType;
    QString mPropertyName;
    QVariant mOldValue;
    QVariant mNewValue;
    
    void applyValue(const QVariant& value);
};

// Command for text changes (unified text editing with cursor position preservation)
class TextChangeCommand : public QUndoCommand
{
public:
    TextChangeCommand(QWidget* widget, const QString& oldText, const QString& newText, 
                     const QString& widgetName = QString(), QUndoCommand* parent = nullptr);
    ~TextChangeCommand() override = default;

    void undo() override;
    void redo() override;
    
    // Merge consecutive text changes
    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

private:
    QWidget* mWidget;
    QString mOldText;
    QString mNewText;
    QString mWidgetName;
    int mOldCursorPosition;
    int mNewCursorPosition;
    
    void applyText(const QString& text, int cursorPosition);
    int getCursorPosition() const;
    void setCursorPosition(int position) const;
};

// Command for multi-item operations (delete multiple items)
class DeleteMultipleItemsCommand : public QUndoCommand
{
public:
    DeleteMultipleItemsCommand(dlgTriggerEditor* editor, const QList<QTreeWidgetItem*>& items, 
                              const QString& itemType, QUndoCommand* parent = nullptr);
    ~DeleteMultipleItemsCommand() override = default;

    void undo() override;
    void redo() override;

private:
    dlgTriggerEditor* mEditor;
    QList<DeleteItemCommand*> mDeleteCommands;
};

#endif // MUDLET_TRIGGEREDITORCOMMANDS_H
