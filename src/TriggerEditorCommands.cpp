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

#include "TriggerEditorCommands.h"
#include "dlgTriggerEditor.h"
#include "Host.h"
#include "TTrigger.h"
#include "TAlias.h"
#include "TTimer.h"
#include "TScript.h"
#include "TAction.h"
#include "TKey.h"
#include "TriggerUnit.h"
#include "AliasUnit.h"
#include "TimerUnit.h"
#include "ScriptUnit.h"
#include "ActionUnit.h"
#include "KeyUnit.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include <QBuffer>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

// DeleteItemCommand implementation
DeleteItemCommand::DeleteItemCommand(dlgTriggerEditor* editor, QTreeWidgetItem* item, 
                                   const QString& itemType, QUndoCommand* parent)
    : QUndoCommand(parent)
    , mEditor(editor)
    , mItem(item)
    , mItemType(itemType)
    , mItemId(item->data(0, Qt::UserRole).toInt())
    , mItemName(item->text(0))
    , mParentItem(item->parent())
{
    // Store the item's position in parent
    if (mParentItem) {
        mIndexInParent = mParentItem->indexOfChild(mItem);
    } else {
        // It's a top-level item
        QTreeWidget* treeWidget = mItem->treeWidget();
        if (treeWidget) {
            mIndexInParent = treeWidget->indexOfTopLevelItem(mItem);
        }
    }
    
    setText(QObject::tr("Delete %1 '%2'").arg(itemType, mItemName));
    saveItemData();
}

void DeleteItemCommand::undo()
{
    qDebug() << "DeleteItemCommand::undo() - Restoring" << mItemType << mItemName;
    restoreItem();
}

void DeleteItemCommand::redo()
{
    qDebug() << "DeleteItemCommand::redo() - Deleting" << mItemType << mItemName << "(first:" << mIsFirstRedo << ")";
    if (mIsFirstRedo) {
        // First redo (when command is pushed) - perform the actual deletion
        deleteItem();
        mIsFirstRedo = false;
    } else {
        // Subsequent redo operations
        qDebug() << "Re-deleting" << mItemType << mItemName << "(redo operation)";
        deleteItem();
    }
}

void DeleteItemCommand::saveItemData()
{
    // Serialize the item to XML for later restoration
    if (mItemType == qsl("Trigger")) {
        auto* trigger = mEditor->mpHost->getTriggerUnit()->getTrigger(mItemId);
        if (trigger) {
            XMLexport writer(trigger);
            writer.exportToClipboard(trigger);
            
            // Get the XML from clipboard and store it
            QClipboard* clipboard = QApplication::clipboard();
            mSerializedData = clipboard->text(QClipboard::Clipboard).toUtf8();
        }
    } else if (mItemType == qsl("Alias")) {
        auto* alias = mEditor->mpHost->getAliasUnit()->getAlias(mItemId);
        if (alias) {
            XMLexport writer(alias);
            writer.exportToClipboard(alias);
            
            QClipboard* clipboard = QApplication::clipboard();
            mSerializedData = clipboard->text(QClipboard::Clipboard).toUtf8();
        }
    } else if (mItemType == qsl("Timer")) {
        auto* timer = mEditor->mpHost->getTimerUnit()->getTimer(mItemId);
        if (timer) {
            XMLexport writer(timer);
            writer.exportToClipboard(timer);
            
            QClipboard* clipboard = QApplication::clipboard();
            mSerializedData = clipboard->text(QClipboard::Clipboard).toUtf8();
        }
    } else if (mItemType == qsl("Script")) {
        auto* script = mEditor->mpHost->getScriptUnit()->getScript(mItemId);
        if (script) {
            XMLexport writer(script);
            writer.exportToClipboard(script);
            
            QClipboard* clipboard = QApplication::clipboard();
            mSerializedData = clipboard->text(QClipboard::Clipboard).toUtf8();
        }
    } else if (mItemType == qsl("Action") || mItemType == qsl("Button")) {
        auto* action = mEditor->mpHost->getActionUnit()->getAction(mItemId);
        if (action) {
            XMLexport writer(action);
            writer.exportToClipboard(action);
            
            QClipboard* clipboard = QApplication::clipboard();
            mSerializedData = clipboard->text(QClipboard::Clipboard).toUtf8();
        }
    } else if (mItemType == qsl("Key")) {
        auto* key = mEditor->mpHost->getKeyUnit()->getKey(mItemId);
        if (key) {
            XMLexport writer(key);
            writer.exportToClipboard(key);
            
            QClipboard* clipboard = QApplication::clipboard();
            mSerializedData = clipboard->text(QClipboard::Clipboard).toUtf8();
        }
    }
    
    qDebug() << "Saved" << mSerializedData.size() << "bytes of XML data for" << mItemType << mItemId;
}

void DeleteItemCommand::restoreItem()
{
    if (mSerializedData.isEmpty()) {
        qDebug() << "No serialized data to restore for" << mItemType << mItemId;
        return;
    }
    
    // Store the original clipboard contents to restore later
    QClipboard* clipboard = QApplication::clipboard();
    QString originalClipboard = clipboard->text(QClipboard::Clipboard);
    
    try {
        // Put our XML data in clipboard
        clipboard->setText(QString::fromUtf8(mSerializedData), QClipboard::Clipboard);
        
        // Import from clipboard
        XMLimport importer(mEditor->mpHost);
        auto result = importer.importFromClipboard();
        
        // Refresh the appropriate tree view and select the restored item
        if (result.first != dlgTriggerEditor::EditorViewType::cmUnknownView) {
            // Update our stored ID to the new ID for subsequent operations
            int newId = result.second;
            if (newId > 0) {
                mItemId = newId;
            }
            
            switch (result.first) {
            case dlgTriggerEditor::EditorViewType::cmTriggerView:
                mEditor->populateTriggers();
                if (newId > 0) {
                    mEditor->selectTriggerByID(newId);
                }
                break;
            case dlgTriggerEditor::EditorViewType::cmAliasView:
                mEditor->populateAliases();
                if (newId > 0) {
                    mEditor->selectAliasByID(newId);
                }
                break;
            case dlgTriggerEditor::EditorViewType::cmTimerView:
                mEditor->populateTimers();
                if (newId > 0) {
                    mEditor->selectTimerByID(newId);
                }
                break;
            case dlgTriggerEditor::EditorViewType::cmScriptView:
                mEditor->populateScripts();
                if (newId > 0) {
                    mEditor->selectScriptByID(newId);
                }
                break;
            case dlgTriggerEditor::EditorViewType::cmActionView:
                mEditor->populateActions();
                if (newId > 0) {
                    mEditor->selectActionByID(newId);
                }
                break;
            case dlgTriggerEditor::EditorViewType::cmKeysView:
                mEditor->populateKeys();
                if (newId > 0) {
                    mEditor->selectKeyByID(newId);
                }
                break;
            default:
                break;
            }
            
            qDebug() << "Successfully restored" << mItemType << "with new ID" << newId << "(old ID was" << mItemId << ")";
        } else {
            qDebug() << "Failed to restore" << mItemType << mItemId;
        }
    } catch (...) {
        qDebug() << "Exception occurred while restoring" << mItemType << mItemId;
    }
    
    // Always restore original clipboard contents
    clipboard->setText(originalClipboard, QClipboard::Clipboard);
}

void DeleteItemCommand::deleteItem()
{
    qDebug() << "Deleting" << mItemType << mItemName << "(ID:" << mItemId << ")";
    
    // Prevent infinite recursion by temporarily disabling undo commands
    bool wasCreatingFromUndo = mEditor->mCreatingFromUndoCommand;
    mEditor->mCreatingFromUndoCommand = true;
    
    // Call the existing delete methods that handle cleanup properly
    if (mItemType == qsl("Trigger")) {
        QTreeWidgetItem* item = mEditor->findTriggerItemById(mItemId);
        if (item) {
            mEditor->treeWidget_triggers->setCurrentItem(item);
            mEditor->delete_trigger();
        }
    } else if (mItemType == qsl("Alias")) {
        QTreeWidgetItem* item = mEditor->findAliasItemById(mItemId);
        if (item) {
            mEditor->treeWidget_aliases->setCurrentItem(item);
            mEditor->delete_alias();
        }
    } else if (mItemType == qsl("Timer")) {
        QTreeWidgetItem* item = mEditor->findTimerItemById(mItemId);
        if (item) {
            mEditor->treeWidget_timers->setCurrentItem(item);
            mEditor->delete_timer();
        }
    } else if (mItemType == qsl("Script")) {
        QTreeWidgetItem* item = mEditor->findScriptItemById(mItemId);
        if (item) {
            mEditor->treeWidget_scripts->setCurrentItem(item);
            mEditor->delete_script();
        }
    } else if (mItemType == qsl("Action")) {
        QTreeWidgetItem* item = mEditor->findActionItemById(mItemId);
        if (item) {
            mEditor->treeWidget_actions->setCurrentItem(item);
            mEditor->delete_action();
        }
    } else if (mItemType == qsl("Key")) {
        QTreeWidgetItem* item = mEditor->findKeyItemById(mItemId);
        if (item) {
            mEditor->treeWidget_keys->setCurrentItem(item);
            mEditor->delete_key();
        }
    }
    
    // Restore the flag
    mEditor->mCreatingFromUndoCommand = wasCreatingFromUndo;
}

// AddItemCommand implementation
AddItemCommand::AddItemCommand(dlgTriggerEditor* editor, const QString& itemType, 
                              bool isGroup, QTreeWidgetItem* parentItem, QUndoCommand* parent)
    : QUndoCommand(parent)
    , mEditor(editor)
    , mItemType(itemType)
    , mIsGroup(isGroup)
{
    // Store parent context to maintain folder structure
    if (parentItem) {
        mParentItemId = parentItem->data(0, Qt::UserRole).toInt();
        mParentItemName = parentItem->text(0);
    }
    
    QString action = isGroup ? QObject::tr("Add %1 Group") : QObject::tr("Add %1");
    setText(action.arg(itemType));
}

void AddItemCommand::undo()
{
    qDebug() << "AddItemCommand::undo() - Deleting created" << mItemType << "with ID" << mCreatedItemId;
    // Just delete the item - no need to save complex data for a simple "Add" operation
    deleteCreatedItem();
}

void AddItemCommand::redo()
{
    qDebug() << "AddItemCommand::redo() - Creating" << mItemType << "(isGroup:" << mIsGroup << ")";
    
    // Redo should simply recreate a new basic item, just like the original Add operation
    createItem();
}

void AddItemCommand::createItem()
{
    // Create items directly without triggering additional undo commands
    qDebug() << "AddItemCommand::createItem() - Creating" << mItemType << "(isGroup:" << mIsGroup << ", parent:" << mParentItemId << ")";
    
    // Set flag to prevent recursion for all item types
    mEditor->mCreatingFromUndoCommand = true;
    
    if (mItemType == qsl("Trigger")) {
        mEditor->addTrigger(mIsGroup);
        QTreeWidgetItem* currentItem = mEditor->treeWidget_triggers->currentItem();
        if (currentItem) {
            mCreatedItemId = currentItem->data(0, Qt::UserRole).toInt();
        }
    } else if (mItemType == qsl("Alias")) {
        mEditor->addAlias(mIsGroup);
        QTreeWidgetItem* currentItem = mEditor->treeWidget_aliases->currentItem();
        if (currentItem) {
            mCreatedItemId = currentItem->data(0, Qt::UserRole).toInt();
        }
    } else if (mItemType == qsl("Timer")) {
        mEditor->addTimer(mIsGroup);
        QTreeWidgetItem* currentItem = mEditor->treeWidget_timers->currentItem();
        if (currentItem) {
            mCreatedItemId = currentItem->data(0, Qt::UserRole).toInt();
        }
    } else if (mItemType == qsl("Script")) {
        mEditor->addScript(mIsGroup);
        QTreeWidgetItem* currentItem = mEditor->treeWidget_scripts->currentItem();
        if (currentItem) {
            mCreatedItemId = currentItem->data(0, Qt::UserRole).toInt();
        }
    } else if (mItemType == qsl("Action") || mItemType == qsl("Button")) {
        mEditor->addAction(mIsGroup);
        QTreeWidgetItem* currentItem = mEditor->treeWidget_actions->currentItem();
        if (currentItem) {
            mCreatedItemId = currentItem->data(0, Qt::UserRole).toInt();
        }
    } else if (mItemType == qsl("Key")) {
        mEditor->addKey(mIsGroup);
        QTreeWidgetItem* currentItem = mEditor->treeWidget_keys->currentItem();
        if (currentItem) {
            mCreatedItemId = currentItem->data(0, Qt::UserRole).toInt();
        }
    } else if (mItemType == qsl("Variable")) {
        mEditor->addVar(mIsGroup);
        // Variables work differently - they don't have numeric IDs
        // We'll handle them separately if needed
    }
    
    // Clear the flag after creation
    mEditor->mCreatingFromUndoCommand = false;
    
    qDebug() << "Created" << mItemType << "with ID" << mCreatedItemId << "with parent ID" << mParentItemId;
}

void AddItemCommand::deleteCreatedItem()
{
    // Delete the item that was created (undo operation)
    if (mCreatedItemId >= 0) {
        qDebug() << "DeleteCreatedItem: Looking for" << mItemType << "with ID" << mCreatedItemId;
        
        // Find the tree widget item and select it, then delete through normal mechanism
        QTreeWidgetItem* item = nullptr;
        if (mItemType == qsl("Trigger")) {
            item = mEditor->findTriggerItemById(mCreatedItemId);
            if (item) {
                mEditor->treeWidget_triggers->setCurrentItem(item);
                // Set flag to prevent recursion and use the normal delete mechanism
                bool wasCreatingFromUndo = mEditor->mCreatingFromUndoCommand;
                mEditor->mCreatingFromUndoCommand = true;
                mEditor->delete_trigger();
                mEditor->mCreatingFromUndoCommand = wasCreatingFromUndo;
            }
        } else if (mItemType == qsl("Alias")) {
            item = mEditor->findAliasItemById(mCreatedItemId);
            if (item) {
                mEditor->treeWidget_aliases->setCurrentItem(item);
                bool wasCreatingFromUndo = mEditor->mCreatingFromUndoCommand;
                mEditor->mCreatingFromUndoCommand = true;
                mEditor->delete_alias();
                mEditor->mCreatingFromUndoCommand = wasCreatingFromUndo;
            }
        } else if (mItemType == qsl("Timer")) {
            item = mEditor->findTimerItemById(mCreatedItemId);
            if (item) {
                mEditor->treeWidget_timers->setCurrentItem(item);
                bool wasCreatingFromUndo = mEditor->mCreatingFromUndoCommand;
                mEditor->mCreatingFromUndoCommand = true;
                mEditor->delete_timer();
                mEditor->mCreatingFromUndoCommand = wasCreatingFromUndo;
            }
        } else if (mItemType == qsl("Script")) {
            item = mEditor->findScriptItemById(mCreatedItemId);
            if (item) {
                mEditor->treeWidget_scripts->setCurrentItem(item);
                bool wasCreatingFromUndo = mEditor->mCreatingFromUndoCommand;
                mEditor->mCreatingFromUndoCommand = true;
                mEditor->delete_script();
                mEditor->mCreatingFromUndoCommand = wasCreatingFromUndo;
            }
        } else if (mItemType == qsl("Action")) {
            item = mEditor->findActionItemById(mCreatedItemId);
            if (item) {
                mEditor->treeWidget_actions->setCurrentItem(item);
                bool wasCreatingFromUndo = mEditor->mCreatingFromUndoCommand;
                mEditor->mCreatingFromUndoCommand = true;
                mEditor->delete_action();
                mEditor->mCreatingFromUndoCommand = wasCreatingFromUndo;
            }
        } else if (mItemType == qsl("Key")) {
            item = mEditor->findKeyItemById(mCreatedItemId);
            if (item) {
                mEditor->treeWidget_keys->setCurrentItem(item);
                bool wasCreatingFromUndo = mEditor->mCreatingFromUndoCommand;
                mEditor->mCreatingFromUndoCommand = true;
                mEditor->delete_key();
                mEditor->mCreatingFromUndoCommand = wasCreatingFromUndo;
            }
        }
        
        if (item) {
            qDebug() << "Successfully deleted" << mItemType << "with ID" << mCreatedItemId << "(undo operation)";
        } else {
            qDebug() << "Could not find" << mItemType << "with ID" << mCreatedItemId << "for deletion";
        }
        mCreatedItemId = -1;
    }
}


// PropertyChangeCommand implementation
PropertyChangeCommand::PropertyChangeCommand(dlgTriggerEditor* editor, int itemId,
                                           const QString& itemType, const QString& propertyName,
                                           const QVariant& oldValue, const QVariant& newValue,
                                           QUndoCommand* parent)
    : QUndoCommand(parent)
    , mEditor(editor)
    , mItemId(itemId)
    , mItemType(itemType)
    , mPropertyName(propertyName)
    , mOldValue(oldValue)
    , mNewValue(newValue)
{
    setText(QObject::tr("Change %1 %2").arg(itemType, propertyName));
}

void PropertyChangeCommand::undo()
{
    applyValue(mOldValue);
}

void PropertyChangeCommand::redo()
{
    applyValue(mNewValue);
}

int PropertyChangeCommand::id() const
{
    // Unique ID for merging similar property changes
    return qHash(QString(qsl("%1_%2_%3")).arg(mItemType, QString::number(mItemId), mPropertyName));
}

bool PropertyChangeCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) {
        return false;
    }
    
    const PropertyChangeCommand* otherCmd = static_cast<const PropertyChangeCommand*>(other);
    if (mItemId != otherCmd->mItemId || mItemType != otherCmd->mItemType || 
        mPropertyName != otherCmd->mPropertyName) {
        return false;
    }
    
    // Merge by keeping the original old value and updating the new value
    mNewValue = otherCmd->mNewValue;
    return true;
}

void PropertyChangeCommand::applyValue(const QVariant& value)
{
    // Prevent infinite recursion during undo/redo operations
    if (mEditor->mCreatingFromUndoCommand) {
        return;
    }
    
    // Set flag to prevent recursion during property application
    mEditor->mCreatingFromUndoCommand = true;
    
    // Apply the property change to the actual item
    if (mItemType == qsl("trigger")) {
        auto* trigger = mEditor->mpHost->getTriggerUnit()->getTrigger(mItemId);
        if (trigger) {
            if (mPropertyName == qsl("name")) {
                trigger->setName(value.toString());
            } else if (mPropertyName == qsl("command")) {
                trigger->setCommand(value.toString());
            } else if (mPropertyName == qsl("patterns")) {
                QStringList patterns = value.toStringList();
                trigger->setRegexCodeList(patterns, trigger->getRegexCodePropertyList());
            } else if (mPropertyName == qsl("patternKinds")) {
                QList<int> patternKinds = value.value<QList<int>>();
                trigger->setRegexCodeList(trigger->getPatternsList(), patternKinds);
            } else if (mPropertyName == qsl("script")) {
                trigger->setScript(value.toString());
            } else if (mPropertyName == qsl("isMultiline")) {
                trigger->setIsMultiline(value.toBool());
            } else if (mPropertyName == qsl("perlSlashGOption")) {
                trigger->mPerlSlashGOption = value.toBool();
            } else if (mPropertyName == qsl("filterTrigger")) {
                trigger->mFilterTrigger = value.toBool();
            } else if (mPropertyName == qsl("conditionLineDelta")) {
                trigger->setConditionLineDelta(value.toInt());
            } else if (mPropertyName == qsl("stayOpen")) {
                trigger->mStayOpen = value.toInt();
            } else if (mPropertyName == qsl("soundTrigger")) {
                trigger->mSoundTrigger = value.toBool();
            } else if (mPropertyName == qsl("soundFile")) {
                trigger->setSound(value.toString());
            } else if (mPropertyName == qsl("isActive")) {
                trigger->setIsActive(value.toBool());
            }
        }
        
        // Update UI without causing recursion
        mEditor->populateTriggers();
        mEditor->selectTriggerByID(mItemId);
        
    } else if (mItemType == qsl("alias")) {
        auto* alias = mEditor->mpHost->getAliasUnit()->getAlias(mItemId);
        if (alias) {
            if (mPropertyName == qsl("name")) {
                alias->setName(value.toString());
            } else if (mPropertyName == qsl("script")) {
                alias->setScript(value.toString());
            } else if (mPropertyName == qsl("isActive")) {
                alias->setIsActive(value.toBool());
            }
            // Add other alias properties as needed
        }
        mEditor->populateAliases();
        mEditor->selectAliasByID(mItemId);
    }
    // Add implementations for other item types as needed
    
    // Clear the recursion prevention flag
    mEditor->mCreatingFromUndoCommand = false;
    
    qDebug() << "Applied property change:" << mItemType << mItemId << mPropertyName << value;
}

// DeleteMultipleItemsCommand implementation
DeleteMultipleItemsCommand::DeleteMultipleItemsCommand(dlgTriggerEditor* editor,
                                                     const QList<QTreeWidgetItem*>& items,
                                                     const QString& itemType,
                                                     QUndoCommand* parent)
    : QUndoCommand(parent)
    , mEditor(editor)
{
    setText(QObject::tr("Delete %1 %2s").arg(QString::number(items.size()), itemType));
    
    // Create individual delete commands for each item
    for (auto* item : items) {
        mDeleteCommands.append(new DeleteItemCommand(editor, item, itemType, this));
    }
}

void DeleteMultipleItemsCommand::undo()
{
    // The child commands will be undone automatically
}

void DeleteMultipleItemsCommand::redo()
{
    // The child commands will be redone automatically
}

// TextChangeCommand implementation with cursor position preservation
TextChangeCommand::TextChangeCommand(QWidget* widget, const QString& oldText, const QString& newText, 
                                   const QString& widgetName, QUndoCommand* parent)
    : QUndoCommand(parent), mWidget(widget), mOldText(oldText), mNewText(newText), mWidgetName(widgetName)
{
    // Capture current cursor position when creating the command
    mOldCursorPosition = getCursorPosition();
    mNewCursorPosition = mOldCursorPosition; // Will be updated when text is applied
    
    QString desc = widgetName.isEmpty() ? qsl("Change text") : qsl("Change text in %1").arg(widgetName);
    setText(desc);
}

void TextChangeCommand::undo()
{
    applyText(mOldText, mOldCursorPosition);
}

void TextChangeCommand::redo()
{
    applyText(mNewText, mNewCursorPosition);
}

void TextChangeCommand::applyText(const QString& text, int cursorPosition)
{
    if (!mWidget) {
        qWarning() << "TextChangeCommand::applyText() - widget is null";
        return;
    }

    // Handle different widget types
    if (auto* lineEdit = qobject_cast<QLineEdit*>(mWidget)) {
        // Block signals to prevent triggering more undo commands
        const QSignalBlocker blocker(lineEdit);
        lineEdit->setText(text);
        // Restore cursor position
        lineEdit->setCursorPosition(qMin(cursorPosition, text.length()));
    } else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(mWidget)) {
        const QSignalBlocker blocker(plainTextEdit);
        plainTextEdit->setPlainText(text);
        // Restore cursor position  
        QTextCursor cursor = plainTextEdit->textCursor();
        cursor.setPosition(qMin(cursorPosition, text.length()));
        plainTextEdit->setTextCursor(cursor);
    } else if (auto* textEdit = qobject_cast<QTextEdit*>(mWidget)) {
        const QSignalBlocker blocker(textEdit);
        textEdit->setPlainText(text);
        // Restore cursor position
        QTextCursor cursor = textEdit->textCursor();
        cursor.setPosition(qMin(cursorPosition, text.length()));
        textEdit->setTextCursor(cursor);
    } else {
        qWarning() << "TextChangeCommand::applyText() - unsupported widget type:" << mWidget->metaObject()->className();
    }
}

int TextChangeCommand::getCursorPosition() const
{
    if (!mWidget) {
        return 0;
    }

    if (auto* lineEdit = qobject_cast<QLineEdit*>(mWidget)) {
        return lineEdit->cursorPosition();
    } else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(mWidget)) {
        return plainTextEdit->textCursor().position();
    } else if (auto* textEdit = qobject_cast<QTextEdit*>(mWidget)) {
        return textEdit->textCursor().position();
    }
    
    return 0;
}

void TextChangeCommand::setCursorPosition(int position) const
{
    if (!mWidget) {
        return;
    }

    if (auto* lineEdit = qobject_cast<QLineEdit*>(mWidget)) {
        lineEdit->setCursorPosition(position);
    } else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(mWidget)) {
        QTextCursor cursor = plainTextEdit->textCursor();
        cursor.setPosition(position);
        plainTextEdit->setTextCursor(cursor);
    } else if (auto* textEdit = qobject_cast<QTextEdit*>(mWidget)) {
        QTextCursor cursor = textEdit->textCursor();
        cursor.setPosition(position);
        textEdit->setTextCursor(cursor);
    }
}

int TextChangeCommand::id() const 
{
    // Use widget pointer as unique ID for merging
    return reinterpret_cast<quintptr>(mWidget);
}

bool TextChangeCommand::mergeWith(const QUndoCommand* other) 
{
    if (id() != other->id()) {
        return false;
    }
    
    const TextChangeCommand* textCmd = static_cast<const TextChangeCommand*>(other);
    
    // Only merge if it's the same widget
    if (mWidget != textCmd->mWidget) {
        return false;
    }
    
    // Merge by updating the new text (keep old text from first change)
    mNewText = textCmd->mNewText;
    return true;
}
