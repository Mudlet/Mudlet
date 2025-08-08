/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QApplication>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItem>

// Test that directly simulates the save method crash scenario
class SaveMethodCrashTest : public QObject
{
    Q_OBJECT

private slots:
    void testSaveMethodWithClearedTree();
    void testMultipleSaveCallsAfterClear();

private:
    // Simulate the problematic save method pattern (before fix)
    bool unfixedSaveMethod(QTreeWidgetItem* currentItem);
    
    // Simulate the fixed save method pattern (after fix)
    bool fixedSaveMethod(QTreeWidgetItem*& currentItem, QTreeWidget* tree);
    
    // Helper validation methods (same as in dlgTriggerEditor)
    bool isTreeWidgetItemValid(QTreeWidgetItem* item, QTreeWidget* tree);
    bool validateCurrentItem(QTreeWidgetItem*& currentItem, QTreeWidget* tree);
};

bool SaveMethodCrashTest::unfixedSaveMethod(QTreeWidgetItem* currentItem)
{
    // This is the UNSAFE pattern that caused the crash
    if (!currentItem) {
        return false;
    }
    
    // DANGER: This line would cause heap-use-after-free if currentItem
    // points to freed memory after tree clearing
    // Note: For top-level items, parent() returns nullptr, so we'll check differently
    QString itemText = currentItem->text(0);  // ← CRASH HERE in unfixed version if freed
    return true;
}

bool SaveMethodCrashTest::fixedSaveMethod(QTreeWidgetItem*& currentItem, QTreeWidget* tree)
{
    // This is the SAFE pattern after our fix
    if (!currentItem) {
        return false;
    }
    
    // SAFE: Validate the pointer before use
    if (!validateCurrentItem(currentItem, tree)) {
        return false;  // Pointer was invalid and has been cleared
    }
    
    // Now safe to use currentItem 
    QString itemText = currentItem->text(0);
    return true;
}

bool SaveMethodCrashTest::isTreeWidgetItemValid(QTreeWidgetItem* item, QTreeWidget* tree)
{
    if (!item || !tree) {
        return false;
    }

    // Check if item is a top-level item
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (tree->topLevelItem(i) == item) {
            return true;
        }
    }

    // Check if it's a child item recursively
    std::function<bool(QTreeWidgetItem*)> findItemRecursive = [&](QTreeWidgetItem* parent) -> bool {
        for (int i = 0; i < parent->childCount(); ++i) {
            QTreeWidgetItem* child = parent->child(i);
            if (child == item) {
                return true;
            }
            if (findItemRecursive(child)) {
                return true;
            }
        }
        return false;
    };

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (findItemRecursive(tree->topLevelItem(i))) {
            return true;
        }
    }

    return false;
}

bool SaveMethodCrashTest::validateCurrentItem(QTreeWidgetItem*& currentItem, QTreeWidget* tree)
{
    if (!currentItem || !tree) {
        currentItem = nullptr;
        return false;
    }

    if (!isTreeWidgetItemValid(currentItem, tree)) {
        currentItem = nullptr;  // Clear invalid pointer - this is the key fix
        return false;
    }

    return true;
}

void SaveMethodCrashTest::testSaveMethodWithClearedTree()
{
    // Create tree and items
    QTreeWidget* tree = new QTreeWidget();
    QTreeWidgetItem* item1 = new QTreeWidgetItem(tree, QStringList("Test Item 1"));
    QTreeWidgetItem* item2 = new QTreeWidgetItem(tree, QStringList("Test Item 2"));
    
    // Simulate having a current item pointer (like mpCurrentTriggerItem)
    QTreeWidgetItem* currentItem = item1;
    
    // Test that the item works with the unfixed method initially
    bool unfixedResult = unfixedSaveMethod(currentItem);
    QVERIFY(unfixedResult);  // Should work before clearing
    
    // Test that the item works with the fixed method initially  
    QTreeWidgetItem* currentItemCopy = item1;
    bool fixedResult = fixedSaveMethod(currentItemCopy, tree);
    QVERIFY(fixedResult);  // Should work before clearing
    QVERIFY(currentItemCopy == item1);  // Pointer should be unchanged
    
    // NOW SIMULATE THE CRASH SCENARIO
    // Clear the tree (simulating package import) - this frees the QTreeWidgetItem objects
    tree->clear();
    
    // At this point, currentItem points to freed memory!
    // Note: We can't actually call unfixedSaveMethod here in a test because
    // it would cause a real crash. In a real scenario with ASAN, this would
    // show "heap-use-after-free" error.
    
    // But we CAN test that the fixed method handles it safely
    QTreeWidgetItem* currentItemForFixed = item1;  // Still points to freed memory
    bool fixedResultAfterClear = fixedSaveMethod(currentItemForFixed, tree);
    
    // Verify the fix worked
    QVERIFY(!fixedResultAfterClear);  // Should return false (safe failure)
    QVERIFY(currentItemForFixed == nullptr);  // Pointer should be cleared
    
    delete tree;
}

void SaveMethodCrashTest::testMultipleSaveCallsAfterClear()
{
    // Test that we can safely call save methods multiple times after clearing
    QTreeWidget* tree = new QTreeWidget();
    QTreeWidgetItem* item = new QTreeWidgetItem(tree, QStringList("Test Item"));
    
    QTreeWidgetItem* currentItem = item;
    
    // Clear the tree
    tree->clear();
    
    // Test multiple save calls - all should be safe
    bool result1 = fixedSaveMethod(currentItem, tree);
    QVERIFY(!result1);
    QVERIFY(currentItem == nullptr);
    
    bool result2 = fixedSaveMethod(currentItem, tree);
    QVERIFY(!result2);  // Should safely handle null pointer
    QVERIFY(currentItem == nullptr);
    
    bool result3 = fixedSaveMethod(currentItem, tree);
    QVERIFY(!result3);  // Should safely handle null pointer
    QVERIFY(currentItem == nullptr);
    
    delete tree;
}

QTEST_MAIN(SaveMethodCrashTest)
#include "SaveMethodCrashTest.moc"
