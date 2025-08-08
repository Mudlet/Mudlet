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
#include <QTimer>
#include <QPointer>

// Simple test to verify the core validation logic concept
class SimplePointerValidationTest : public QObject
{
    Q_OBJECT

private slots:
    void testBasicTreeWidgetItemValidation();
    void testPointerSafetyAfterTreeClear();

private:
    // Simplified version of the validation logic from dlgTriggerEditor
    bool isTreeWidgetItemValid(QTreeWidgetItem* item, QTreeWidget* tree);
    bool validateCurrentItem(QTreeWidgetItem*& currentItem, QTreeWidget* tree);
};

bool SimplePointerValidationTest::isTreeWidgetItemValid(QTreeWidgetItem* item, QTreeWidget* tree)
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

bool SimplePointerValidationTest::validateCurrentItem(QTreeWidgetItem*& currentItem, QTreeWidget* tree)
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

void SimplePointerValidationTest::testBasicTreeWidgetItemValidation()
{
    // Create a test tree widget
    QTreeWidget* testTree = new QTreeWidget();
    
    // Add some test items
    QTreeWidgetItem* item1 = new QTreeWidgetItem(testTree, QStringList("Item 1"));
    QTreeWidgetItem* item2 = new QTreeWidgetItem(testTree, QStringList("Item 2"));
    QTreeWidgetItem* childItem = new QTreeWidgetItem(item1, QStringList("Child Item"));
    
    // Test validation of valid items
    QVERIFY(isTreeWidgetItemValid(item1, testTree));
    QVERIFY(isTreeWidgetItemValid(item2, testTree));
    QVERIFY(isTreeWidgetItemValid(childItem, testTree));
    
    // Test validation with nullptr
    QVERIFY(!isTreeWidgetItemValid(nullptr, testTree));
    QVERIFY(!isTreeWidgetItemValid(item1, nullptr));
    
    delete testTree;
}

void SimplePointerValidationTest::testPointerSafetyAfterTreeClear()
{
    // Create a test tree widget
    QTreeWidget* testTree = new QTreeWidget();
    
    // Add test items
    QTreeWidgetItem* item1 = new QTreeWidgetItem(testTree, QStringList("Item 1"));
    QTreeWidgetItem* item2 = new QTreeWidgetItem(testTree, QStringList("Item 2"));
    
    // Simulate having a current item pointer (like mpCurrentTriggerItem)
    QTreeWidgetItem* currentItem = item1;
    
    // Verify item is initially valid
    QVERIFY(currentItem != nullptr);
    QVERIFY(isTreeWidgetItemValid(currentItem, testTree));
    QVERIFY(validateCurrentItem(currentItem, testTree));
    QVERIFY(currentItem != nullptr); // Should still be valid
    
    // Simulate package import clearing the tree - this is the crash scenario
    testTree->clear();  // This frees the QTreeWidgetItem objects
    
    // Now currentItem points to freed memory - this is the bug!
    // In the unfixed version, accessing currentItem would cause heap-use-after-free
    
    // Test the fix - validation should detect invalid pointer and clear it
    bool isValid = validateCurrentItem(currentItem, testTree);
    
    // Verify the fix worked
    QVERIFY(!isValid);  // Should return false for invalid item
    QVERIFY(currentItem == nullptr);  // Pointer should be cleared by validation
    
    // Test that we can safely call validation again
    bool isStillValid = validateCurrentItem(currentItem, testTree);
    QVERIFY(!isStillValid);  // Should safely handle null pointer
    QVERIFY(currentItem == nullptr);  // Should remain null
    
    delete testTree;
}

QTEST_MAIN(SimplePointerValidationTest)
#include "SimplePointerValidationTest.moc"
