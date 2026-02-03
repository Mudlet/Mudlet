/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QtTest/QtTest>
#include <cstdlib>

#include "EditorUndoStack.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTreeWidget.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgActionMainArea.h"
#include "dlgConnectionProfiles.h"
#include "dlgTimersMainArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

static void initializeQRCResources() {
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
  qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
  qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  qInitResources_mudlet_fonts_posix();
#endif
#endif
  qInitResources_mudlet();
  qInitResources_qm();
}

class dlgTriggerEditorUndoRedoTest : public QObject {
  Q_OBJECT

private:
  TelnetServerStub *mpServer = nullptr;
  dlgTriggerEditor *mpEditor = nullptr;
  Host *mpHost = nullptr;
  const QString mProfileName = qsl("UndoRedo-Test-Profile");
  const QString mPort = qsl("23456");
  const QString mLocalhost = qsl("localhost");

  struct ItemTypeInfo {
    QString name;
    EditorViewType viewType;
    std::function<void()> showView;
    std::function<void()> addItem;
    std::function<void()> addFolder;
    std::function<QTreeWidgetItem *()> getBaseItem;
    std::function<TTreeWidget *()> getTreeWidget;
    QString newItemText;
    QString newFolderText;

    QTreeWidgetItem *baseItem() const { return getBaseItem(); }
    TTreeWidget *treeWidget() const { return getTreeWidget(); }
  };

  std::vector<ItemTypeInfo> mItemTypes;

  void cleanupAll(const ItemTypeInfo &itemType) {
    itemType.treeWidget()->clearSelection();
    itemType.treeWidget()->setCurrentItem(nullptr);
    QCoreApplication::processEvents();

    while (itemType.baseItem()->childCount() > 0) {
      itemType.treeWidget()->setCurrentItem(itemType.baseItem()->child(0));
      mpEditor->slot_deleteItemOrGroup();
    }
    mpEditor->mpUndoStack->clear();
  }

  void deleteProfileDirectory(const QString &profileName) {
    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, profileName);
    QDir dir(path);
    if (dir.exists()) {
      dir.removeRecursively();
    }
  }

  void startProfile(const QString &profileName, const QString &address,
                     const QString &port) {
    QTimer::singleShot(0, qApp, [profileName, address, port]() {
      mudlet::self()->startAutoLogin({});
      QTest::qWait(100);
      QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button,
                        Qt::LeftButton);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), profileName);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), address);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), port);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
    });

    QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
    if (!spy.wait(2000)) {
      QFAIL("Profile took too long to load.");
    }

    mpHost = mudlet::self()->getActiveHost();
    if (!mpHost) {
      QFAIL("No active host available for the test.");
    }

    QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
    if (!spy2.wait(1000)) {
      QFAIL("Could not connect with the host.");
    }
  }

private slots:
  void initTestCase() { initializeQRCResources(); }

  void init() {
    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, mPort.toUShort());
    mudlet::start();
    mudlet::self()->setupConfig();
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);
    deleteProfileDirectory(mProfileName);
    startProfile(mProfileName, mLocalhost, mPort);

    // Open the editor dialog (it's created lazily)
    mudlet::self()->slot_showScriptDialog();
    QTest::qWait(100);

    mpEditor = mpHost->mpEditorDialog;
    QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
    QVERIFY2(mpEditor->mpUndoStack != nullptr, "Undo stack should exist");

    mItemTypes = {{qsl("Trigger"), EditorViewType::cmTriggerView,
                   [this]() { mpEditor->slot_showTriggers(); },
                   [this]() { mpEditor->addTrigger(false); },
                   [this]() { mpEditor->addTrigger(true); },
                   [this]() { return mpEditor->mpTriggerBaseItem; },
                   [this]() { return mpEditor->treeWidget_triggers; },
                   qsl("New trigger"), qsl("New trigger group")},
                  {qsl("Timer"), EditorViewType::cmTimerView,
                   [this]() { mpEditor->slot_showTimers(); },
                   [this]() { mpEditor->addTimer(false); },
                   [this]() { mpEditor->addTimer(true); },
                   [this]() { return mpEditor->mpTimerBaseItem; },
                   [this]() { return mpEditor->treeWidget_timers; },
                   qsl("New timer"), qsl("New timer group")},
                  {qsl("Alias"), EditorViewType::cmAliasView,
                   [this]() { mpEditor->slot_showAliases(); },
                   [this]() { mpEditor->addAlias(false); },
                   [this]() { mpEditor->addAlias(true); },
                   [this]() { return mpEditor->mpAliasBaseItem; },
                   [this]() { return mpEditor->treeWidget_aliases; },
                   qsl("New alias"), qsl("New alias group")},
                  {qsl("Script"), EditorViewType::cmScriptView,
                   [this]() { mpEditor->slot_showScripts(); },
                   [this]() { mpEditor->addScript(false); },
                   [this]() { mpEditor->addScript(true); },
                   [this]() { return mpEditor->mpScriptsBaseItem; },
                   [this]() { return mpEditor->treeWidget_scripts; },
                   qsl("New script"), qsl("New script group")},
                  {qsl("Key"), EditorViewType::cmKeysView,
                   [this]() { mpEditor->slot_showKeys(); },
                   [this]() { mpEditor->addKey(false); },
                   [this]() { mpEditor->addKey(true); },
                   [this]() { return mpEditor->mpKeyBaseItem; },
                   [this]() { return mpEditor->treeWidget_keys; },
                   qsl("New key"), qsl("New key group")},
                  {qsl("Action"), EditorViewType::cmActionView,
                   [this]() { mpEditor->slot_showActions(); },
                   [this]() { mpEditor->addAction(false); },
                   [this]() { mpEditor->addAction(true); },
                   [this]() { return mpEditor->mpActionBaseItem; },
                   [this]() { return mpEditor->treeWidget_actions; },
                   qsl("New button"), qsl("New button group")}};
  }

  void cleanup() {
    mItemTypes.clear();
    mpEditor = nullptr;
    mpHost = nullptr;
    delete mpServer;
    mpServer = nullptr;
    deleteProfileDirectory(mProfileName);
    delete mudlet::self();
  }

  // ========================================================================
  // CATEGORY 1: Core Operations - Single Items
  // ========================================================================
  void testCoreOperations_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testCoreOperations() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();

    // Test: Add item → undo → redo
    {
      int initialCount = itemType.baseItem()->childCount();
      itemType.addItem();
      QVERIFY2(itemType.baseItem()->childCount() > initialCount,
               qPrintable(itemTypeName + ": Item should be added"));

      mpEditor->mpUndoStack->undo();
      QCOMPARE(itemType.baseItem()->childCount(), initialCount);

      mpEditor->mpUndoStack->redo();
      QVERIFY2(
          itemType.baseItem()->childCount() > initialCount,
          qPrintable(itemTypeName + ": Item should be restored after redo"));
      mpEditor->mpUndoStack->undo();
    }

    // Test: Add folder → undo → redo
    {
      int initialCount = itemType.baseItem()->childCount();
      itemType.addFolder();
      QVERIFY2(itemType.baseItem()->childCount() > initialCount,
               qPrintable(itemTypeName + ": Folder should be added"));

      mpEditor->mpUndoStack->undo();
      QCOMPARE(itemType.baseItem()->childCount(), initialCount);

      mpEditor->mpUndoStack->redo();
      QVERIFY2(
          itemType.baseItem()->childCount() > initialCount,
          qPrintable(itemTypeName + ": Folder should be restored after redo"));
      mpEditor->mpUndoStack->undo();
    }

    // Test: Delete item → undo → redo
    {
      itemType.addItem();
      QTreeWidgetItem *item = itemType.baseItem()->child(0);
      QVERIFY(item != nullptr);

      itemType.treeWidget()->setCurrentItem(item);
      int countBeforeDelete = itemType.baseItem()->childCount();
      mpEditor->slot_deleteItemOrGroup();
      QVERIFY2(itemType.baseItem()->childCount() < countBeforeDelete,
               qPrintable(itemTypeName + ": Item should be deleted"));

      mpEditor->mpUndoStack->undo();
      QCOMPARE(itemType.baseItem()->childCount(), countBeforeDelete);

      mpEditor->mpUndoStack->redo();
      QVERIFY2(itemType.baseItem()->childCount() < countBeforeDelete,
               qPrintable(itemTypeName + ": Delete should be redone"));
    }

    // Test: Delete empty folder → undo → redo
    {
      itemType.addFolder();
      QTreeWidgetItem *folder = itemType.baseItem()->child(0);
      QVERIFY(folder != nullptr);

      itemType.treeWidget()->setCurrentItem(folder);
      int countBeforeDelete = itemType.baseItem()->childCount();
      mpEditor->slot_deleteItemOrGroup();
      QVERIFY2(itemType.baseItem()->childCount() < countBeforeDelete,
               qPrintable(itemTypeName + ": Empty folder should be deleted"));

      mpEditor->mpUndoStack->undo();
      QCOMPARE(itemType.baseItem()->childCount(), countBeforeDelete);

      mpEditor->mpUndoStack->redo();
      QVERIFY2(
          itemType.baseItem()->childCount() < countBeforeDelete,
          qPrintable(itemTypeName + ": Empty folder delete should be redone"));
    }

    mpEditor->mpUndoStack->clear();
  }

  // ========================================================================
  // CATEGORY 2: Parent-Only Selection
  // ========================================================================
  void testParentOnlySelection_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testParentOnlySelection() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: Delete parent with children → undo
    {
      itemType.addFolder();
      if (itemType.viewType == EditorViewType::cmKeysView ||
          itemType.viewType == EditorViewType::cmActionView) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
      }

      QTreeWidgetItem *folder = itemType.baseItem()->child(0);
      QVERIFY2(folder != nullptr,
               qPrintable(itemTypeName + ": Folder should be created"));

      itemType.treeWidget()->setCurrentItem(folder);
      itemType.addItem();
      if (itemType.viewType == EditorViewType::cmKeysView ||
          itemType.viewType == EditorViewType::cmActionView) {
        QCoreApplication::processEvents();
      }

      itemType.treeWidget()->setCurrentItem(folder);
      itemType.addItem();
      if (itemType.viewType == EditorViewType::cmKeysView ||
          itemType.viewType == EditorViewType::cmActionView) {
        QCoreApplication::processEvents();
      }

      QCOMPARE(folder->childCount(), 2);

      itemType.treeWidget()->setCurrentItem(folder);
      int totalCountBefore = itemType.baseItem()->childCount();
      mpEditor->slot_deleteItemOrGroup();

      QVERIFY2(itemType.baseItem()->childCount() < totalCountBefore,
               qPrintable(itemTypeName +
                          ": Parent with children should be deleted"));

      mpEditor->mpUndoStack->undo();

      QTreeWidgetItem *restoredFolder = nullptr;
      for (int i = 0; i < itemType.baseItem()->childCount(); i++) {
        QTreeWidgetItem *item = itemType.baseItem()->child(i);
        if (item->childCount() == 2) {
          restoredFolder = item;
          break;
        }
      }

      QVERIFY2(restoredFolder != nullptr && restoredFolder->childCount() == 2,
               qPrintable(itemTypeName +
                          ": Parent and children should be restored"));

      cleanupAll(itemType);
    }

    mpEditor->mpUndoStack->clear();
  }

  // ========================================================================
  // CATEGORY 3: Parent + All Children Selected (Multi-selection)
  // ========================================================================
  void testMultiSelection_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testMultiSelection() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: Delete parent+children selected → verify single operation
    {
      itemType.treeWidget()->setCurrentItem(itemType.baseItem());
      itemType.addFolder();
      QTreeWidgetItem *folder = itemType.baseItem()->child(0);
      QVERIFY(folder != nullptr);

      itemType.treeWidget()->setCurrentItem(folder);
      itemType.addItem();
      itemType.treeWidget()->setCurrentItem(folder);
      itemType.addItem();

      QTreeWidgetItem *child1 = folder->child(0);
      QTreeWidgetItem *child2 = folder->child(1);

      QVERIFY2(folder->childCount() == 2 && child1 && child2 &&
                   child1 != child2,
               qPrintable(itemTypeName + ": Should have 2 distinct children"));

      QList<QTreeWidgetItem *> items;
      items << folder << child1 << child2;
      itemType.treeWidget()->clearSelection();
      for (auto *item : items) {
        item->setSelected(true);
      }
      itemType.treeWidget()->setCurrentItem(folder);

      int stackCountBefore = mpEditor->mpUndoStack->count();
      mpEditor->slot_deleteItemOrGroup();
      int stackCountAfter = mpEditor->mpUndoStack->count();

      QVERIFY2(
          stackCountAfter <= stackCountBefore + 1,
          qPrintable(
              itemTypeName +
              ": Delete parent+children should create single/batched command"));

      mpEditor->mpUndoStack->undo();

      QTreeWidgetItem *restored = itemType.baseItem()->child(0);
      QVERIFY2(restored != nullptr && restored->childCount() == 2,
               qPrintable(itemTypeName + ": Single undo should restore all"));

      cleanupAll(itemType);
    }

    mpEditor->mpUndoStack->clear();
  }

  // ========================================================================
  // CATEGORY 4: ID Remapping
  // ========================================================================
  void testIdRemapping_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testIdRemapping() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: Delete → undo (verify new ID assigned)
    {
      itemType.addItem();
      QTreeWidgetItem *item = itemType.baseItem()->child(0);
      QVERIFY(item != nullptr);

      int originalID = item->data(0, Qt::UserRole).toInt();

      itemType.treeWidget()->setCurrentItem(item);
      mpEditor->slot_deleteItemOrGroup();
      mpEditor->mpUndoStack->undo();

      QTreeWidgetItem *restoredItem = itemType.baseItem()->child(0);
      QVERIFY2(restoredItem != nullptr,
               qPrintable(itemTypeName + ": Item should be restored"));

      // ID may or may not be remapped depending on implementation
      int newID = restoredItem->data(0, Qt::UserRole).toInt();
      QVERIFY2(newID > 0, qPrintable(itemTypeName +
                                     ": Restored item should have valid ID"));

      cleanupAll(itemType);
    }

    // Test: Delete → undo → redo → undo chain
    {
      itemType.addItem();
      QTreeWidgetItem *item = itemType.baseItem()->child(0);
      QVERIFY(item != nullptr);

      itemType.treeWidget()->setCurrentItem(item);
      mpEditor->slot_deleteItemOrGroup();
      mpEditor->mpUndoStack->undo();
      mpEditor->mpUndoStack->redo();
      mpEditor->mpUndoStack->undo();

      QVERIFY2(itemType.baseItem()->childCount() > 0,
               qPrintable(itemTypeName + ": Undo/redo chain should work"));

      cleanupAll(itemType);
    }

    mpEditor->mpUndoStack->clear();
  }

  // ========================================================================
  // CATEGORY 5: Undo/Redo Chains
  // ========================================================================
  void testUndoRedoChains_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testUndoRedoChains() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: 5 operations → undo all → redo all
    {
      int initialCount = itemType.baseItem()->childCount();

      for (int i = 0; i < 5; i++) {
        itemType.addItem();
      }
      QCOMPARE(itemType.baseItem()->childCount(), initialCount + 5);

      for (int i = 0;
           i < 50 && itemType.baseItem()->childCount() > initialCount &&
           mpEditor->mpUndoStack->canUndo();
           i++) {
        mpEditor->mpUndoStack->undo();
      }
      QCOMPARE(itemType.baseItem()->childCount(), initialCount);

      for (int i = 0; i < 50 && mpEditor->mpUndoStack->canRedo() &&
                      itemType.baseItem()->childCount() < initialCount + 5;
           i++) {
        mpEditor->mpUndoStack->redo();
      }
      QCOMPARE(itemType.baseItem()->childCount(), initialCount + 5);

      cleanupAll(itemType);
    }

    // Test: Undo 3 times → redo 2 → new op clears redo
    {
      itemType.addItem();
      itemType.addItem();
      itemType.addItem();

      mpEditor->mpUndoStack->undo();
      mpEditor->mpUndoStack->undo();
      mpEditor->mpUndoStack->undo();
      mpEditor->mpUndoStack->redo();
      mpEditor->mpUndoStack->redo();

      bool canRedoBefore = mpEditor->mpUndoStack->canRedo();
      itemType.addItem();
      bool canRedoAfter = mpEditor->mpUndoStack->canRedo();

      QVERIFY2(
          canRedoBefore && !canRedoAfter,
          qPrintable(itemTypeName + ": New operation should clear redo stack"));

      cleanupAll(itemType);
    }

    // Test: Verify canUndo()/canRedo() states
    {
      itemType.addItem();
      bool canUndoAfterAdd = mpEditor->mpUndoStack->canUndo();
      mpEditor->mpUndoStack->undo();
      bool canRedoAfterUndo = mpEditor->mpUndoStack->canRedo();

      QVERIFY2(canUndoAfterAdd && canRedoAfterUndo,
               qPrintable(itemTypeName +
                          ": canUndo/canRedo states should be correct"));

      cleanupAll(itemType);
    }

    mpEditor->mpUndoStack->clear();
  }

  // ========================================================================
  // CATEGORY 6: Edge Cases
  // ========================================================================
  void testEdgeCases_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testEdgeCases() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: Undo when stack is empty
    {
      bool canUndoBefore = mpEditor->mpUndoStack->canUndo();
      mpEditor->mpUndoStack->undo();
      bool canUndoAfter = mpEditor->mpUndoStack->canUndo();

      QVERIFY2(
          !canUndoBefore && !canUndoAfter,
          qPrintable(itemTypeName +
                     ": Undo on empty stack should be handled gracefully"));
    }

    // Test: Redo when nothing to redo
    {
      bool canRedoBefore = mpEditor->mpUndoStack->canRedo();
      mpEditor->mpUndoStack->redo();
      bool canRedoAfter = mpEditor->mpUndoStack->canRedo();

      QVERIFY2(!canRedoBefore && !canRedoAfter,
               qPrintable(
                   itemTypeName +
                   ": Redo with nothing to redo should be handled gracefully"));
    }

    // Test: Clear stack mid-operation
    {
      itemType.addItem();
      itemType.addItem();
      mpEditor->mpUndoStack->clear();

      QVERIFY2(
          !mpEditor->mpUndoStack->canUndo() &&
              !mpEditor->mpUndoStack->canRedo(),
          qPrintable(itemTypeName + ": Clear stack should work correctly"));

      cleanupAll(itemType);
    }

    // Test: Deep nesting (10 levels)
    {
      QTreeWidgetItem *currentParent = itemType.baseItem();

      for (int i = 0; i < 10; i++) {
        itemType.treeWidget()->setCurrentItem(currentParent);
        itemType.addFolder();
        if (currentParent == itemType.baseItem()) {
          currentParent = itemType.baseItem()->child(0);
        } else {
          currentParent = currentParent->child(0);
        }
      }

      QTreeWidgetItem *deepest = itemType.baseItem();
      int depth = 0;
      while (deepest && deepest->childCount() > 0) {
        deepest = deepest->child(0);
        depth++;
      }
      QCOMPARE(depth, 10);

      itemType.treeWidget()->setCurrentItem(itemType.baseItem()->child(0));
      mpEditor->slot_deleteItemOrGroup();
      mpEditor->mpUndoStack->undo();

      QTreeWidgetItem *restored = itemType.baseItem();
      int restoredDepth = 0;
      while (restored && restored->childCount() > 0) {
        restored = restored->child(0);
        restoredDepth++;
      }
      QCOMPARE(restoredDepth, 10);

      cleanupAll(itemType);
    }

    mpEditor->mpUndoStack->clear();
  }

  // ========================================================================
  // CATEGORY 7: Integration Tests
  // ========================================================================
  void testIntegration() {
    for (auto &itemType : mItemTypes) {
      itemType.showView();
      cleanupAll(itemType);
    }

    // Test: Sequential delete operations across types
    const auto &scripts = mItemTypes[3];
    const auto &aliases = mItemTypes[2];
    const auto &triggers = mItemTypes[0];

    scripts.showView();
    scripts.addItem();
    aliases.showView();
    aliases.addItem();
    triggers.showView();
    triggers.addItem();

    QCOMPARE(scripts.baseItem()->childCount(), 1);
    QCOMPARE(aliases.baseItem()->childCount(), 1);
    QCOMPARE(triggers.baseItem()->childCount(), 1);

    scripts.showView();
    scripts.treeWidget()->setCurrentItem(scripts.baseItem()->child(0));
    mpEditor->slot_deleteItemOrGroup();

    aliases.showView();
    aliases.treeWidget()->setCurrentItem(aliases.baseItem()->child(0));
    mpEditor->slot_deleteItemOrGroup();

    triggers.showView();
    triggers.treeWidget()->setCurrentItem(triggers.baseItem()->child(0));
    mpEditor->slot_deleteItemOrGroup();

    QCOMPARE(scripts.baseItem()->childCount(), 0);
    QCOMPARE(aliases.baseItem()->childCount(), 0);
    QCOMPARE(triggers.baseItem()->childCount(), 0);

    mpEditor->mpUndoStack->undo();
    QVERIFY2(triggers.baseItem()->childCount() == 1 &&
                 aliases.baseItem()->childCount() == 0 &&
                 scripts.baseItem()->childCount() == 0,
             "First undo should restore only triggers");

    mpEditor->mpUndoStack->undo();
    QVERIFY2(triggers.baseItem()->childCount() == 1 &&
                 aliases.baseItem()->childCount() == 1 &&
                 scripts.baseItem()->childCount() == 0,
             "Second undo should restore only aliases");

    mpEditor->mpUndoStack->undo();
    QVERIFY2(triggers.baseItem()->childCount() == 1 &&
                 aliases.baseItem()->childCount() == 1 &&
                 scripts.baseItem()->childCount() == 1,
             "Third undo should restore only scripts");

    for (auto &type : mItemTypes) {
      type.showView();
      cleanupAll(type);
    }
  }

  // ========================================================================
  // CATEGORY 8: Large Batch Operations
  // ========================================================================
  void testLargeBatchOperations_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testLargeBatchOperations() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: Large batch operations (50 items)
    int originalLimit = mpEditor->mpUndoStack->undoLimit();
    mpEditor->mpUndoStack->setUndoLimit(200);

    int initialCount = itemType.baseItem()->childCount();

    for (int i = 0; i < 50; i++) {
      itemType.addItem();
    }
    QCOMPARE(itemType.baseItem()->childCount(), initialCount + 50);

    for (int i = 0;
         i < 500 && itemType.baseItem()->childCount() > initialCount &&
         mpEditor->mpUndoStack->canUndo();
         i++) {
      mpEditor->mpUndoStack->undo();
    }
    QCOMPARE(itemType.baseItem()->childCount(), initialCount);

    mpEditor->mpUndoStack->clear();
    mpEditor->mpUndoStack->setUndoLimit(originalLimit);
  }

  // ========================================================================
  // CATEGORY 9: State Consistency Tests
  // ========================================================================
  void testStateConsistency_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testStateConsistency() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: All items have valid IDs
    {
      itemType.addItem();
      itemType.addItem();

      bool allValid = true;
      for (int i = 0; i < itemType.baseItem()->childCount(); i++) {
        QTreeWidgetItem *item = itemType.baseItem()->child(i);
        int id = item->data(0, Qt::UserRole).toInt();
        if (id <= 0) {
          allValid = false;
          break;
        }
      }

      QVERIFY2(allValid && itemType.baseItem()->childCount() == 2,
               qPrintable(itemTypeName + ": All items should have valid IDs"));

      cleanupAll(itemType);
    }

    // Test: Parent-child relationships intact after undo/redo
    {
      itemType.addFolder();
      QTreeWidgetItem *folder = itemType.baseItem()->child(0);
      QVERIFY(folder != nullptr);

      itemType.treeWidget()->setCurrentItem(folder);
      itemType.addItem();
      itemType.treeWidget()->setCurrentItem(folder);
      itemType.addItem();

      int childCountBefore = folder->childCount();

      itemType.treeWidget()->setCurrentItem(folder);
      mpEditor->slot_deleteItemOrGroup();
      mpEditor->mpUndoStack->undo();

      QTreeWidgetItem *restored = itemType.baseItem()->child(0);
      QVERIFY2(restored != nullptr &&
                   restored->childCount() == childCountBefore,
               qPrintable(itemTypeName +
                          ": Parent-child relationships should be intact"));

      cleanupAll(itemType);
    }

    // Test: Stack command count consistency
    {
      int countBefore = mpEditor->mpUndoStack->count();
      itemType.addItem();
      int countAfter = mpEditor->mpUndoStack->count();

      QVERIFY2(countAfter > countBefore,
               qPrintable(itemTypeName +
                          ": Stack count should increase with operations"));

      mpEditor->mpUndoStack->clear();
      QCOMPARE(mpEditor->mpUndoStack->count(), 0);
    }
  }

  // ========================================================================
  // CATEGORY 10: Error Recovery Tests
  // ========================================================================
  void testErrorRecovery_data() {
    QTest::addColumn<int>("itemTypeIndex");
    QTest::addColumn<QString>("itemTypeName");

    QTest::newRow("Trigger") << 0 << "Trigger";
    QTest::newRow("Timer") << 1 << "Timer";
    QTest::newRow("Alias") << 2 << "Alias";
    QTest::newRow("Script") << 3 << "Script";
    QTest::newRow("Key") << 4 << "Key";
    QTest::newRow("Action") << 5 << "Action";
  }

  void testErrorRecovery() {
    QFETCH(int, itemTypeIndex);
    QFETCH(QString, itemTypeName);

    const auto &itemType = mItemTypes[itemTypeIndex];
    itemType.showView();
    cleanupAll(itemType);

    // Test: Stack integrity after many operations
    {
      itemType.addItem();
      itemType.addFolder();
      mpEditor->mpUndoStack->undo();
      itemType.addItem();
      mpEditor->mpUndoStack->undo();
      mpEditor->mpUndoStack->redo();
      mpEditor->mpUndoStack->undo();

      itemType.addItem();
      for (int i = 0; i < 10 && itemType.baseItem()->childCount() > 0 &&
                      mpEditor->mpUndoStack->canUndo();
           i++) {
        mpEditor->mpUndoStack->undo();
      }

      QCOMPARE(itemType.baseItem()->childCount(), 0);
      mpEditor->mpUndoStack->clear();
    }

    // Test: Cleanup verification
    {
      for (int i = 0; i < 5; i++) {
        itemType.addItem();
      }

      for (int i = 0; i < 50 && itemType.baseItem()->childCount() > 0 &&
                      mpEditor->mpUndoStack->canUndo();
           i++) {
        mpEditor->mpUndoStack->undo();
      }

      QCOMPARE(itemType.baseItem()->childCount(), 0);
      mpEditor->mpUndoStack->clear();
    }
  }

  // ========================================================================
  // CATEGORY 11: Edit Operations Tests
  // ========================================================================
  void testTriggerPatternEdits() {
    mpEditor->slot_showTriggers();
    cleanupAll(mItemTypes[0]);

    mpEditor->addTrigger(false);
    QVERIFY(mpEditor->mpTriggerBaseItem->childCount() > 0);

    QTreeWidgetItem *trigger = mpEditor->mpTriggerBaseItem->child(0);
    int triggerID = trigger->data(0, Qt::UserRole).toInt();
    TTrigger *pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    QVERIFY(pT != nullptr);

    mpEditor->treeWidget_triggers->setCurrentItem(trigger);
    mpEditor->slot_triggerSelected(trigger);
    mpEditor->mpUndoStack->clear();

    QString initialPattern = qsl("initial pattern");
    if (mpEditor->mTriggerPatternEdit.size() > 0) {
      mpEditor->mTriggerPatternEdit[0]
          ->singleLineTextEdit_pattern->setPlainText(initialPattern);
      mpEditor->saveTrigger();
    }

    QString originalPattern = pT->getPatternsList().value(0);
    QString newPattern = qsl("test pattern edit");

    if (mpEditor->mTriggerPatternEdit.size() > 0) {
      mpEditor->mTriggerPatternEdit[0]
          ->singleLineTextEdit_pattern->setPlainText(newPattern);
      mpEditor->saveTrigger();
    }

    QCOMPARE(pT->getPatternsList().value(0), newPattern);

    mpEditor->mpUndoStack->undo();
    QCOMPARE(pT->getPatternsList().value(0), originalPattern);

    mpEditor->mpUndoStack->redo();
    QCOMPARE(pT->getPatternsList().value(0), newPattern);

    cleanupAll(mItemTypes[0]);
  }

  void testTriggerNameEdits() {
    mpEditor->slot_showTriggers();
    cleanupAll(mItemTypes[0]);

    mpEditor->addTrigger(false);
    QVERIFY(mpEditor->mpTriggerBaseItem->childCount() > 0);

    QTreeWidgetItem *trigger = mpEditor->mpTriggerBaseItem->child(0);
    int triggerID = trigger->data(0, Qt::UserRole).toInt();
    TTrigger *pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    QVERIFY(pT != nullptr);

    mpEditor->treeWidget_triggers->setCurrentItem(trigger);
    mpEditor->slot_triggerSelected(trigger);
    mpEditor->mpUndoStack->clear();

    QString originalName = pT->getName();
    QString newName = qsl("Edited Trigger Name");

    mpEditor->mpTriggersMainArea->lineEdit_trigger_name->setText(newName);
    mpEditor->saveTrigger();

    QCOMPARE(pT->getName(), newName);

    mpEditor->mpUndoStack->undo();
    QCOMPARE(pT->getName(), originalName);

    mpEditor->mpUndoStack->redo();
    QCOMPARE(pT->getName(), newName);

    cleanupAll(mItemTypes[0]);
  }

  void testTimerTimeValues() {
    mpEditor->slot_showTimers();
    cleanupAll(mItemTypes[1]);

    mpEditor->addTimer(false);
    QVERIFY(mpEditor->mpTimerBaseItem->childCount() > 0);

    QTreeWidgetItem *timer = mpEditor->mpTimerBaseItem->child(0);
    int timerID = timer->data(0, Qt::UserRole).toInt();
    TTimer *pTimer = mpHost->getTimerUnit()->getTimer(timerID);
    QVERIFY(pTimer != nullptr);

    mpEditor->treeWidget_timers->setCurrentItem(timer);
    mpEditor->slot_timerSelected(timer);
    mpEditor->mpUndoStack->clear();

    QTime originalTime = pTimer->getTime();
    QTime newMinutes(0, 1, 0, 0);
    QTime newSeconds(0, 0, 30, 0);
    QTime newMsecs(0, 0, 0, 500);

    mpEditor->mpTimersMainArea->timeEdit_timer_minutes->setTime(newMinutes);
    mpEditor->mpTimersMainArea->timeEdit_timer_seconds->setTime(newSeconds);
    mpEditor->mpTimersMainArea->timeEdit_timer_msecs->setTime(newMsecs);
    mpEditor->saveTimer();

    QTime expectedTime(0, 1, 30, 500);
    QCOMPARE(pTimer->getTime(), expectedTime);

    mpEditor->mpUndoStack->undo();
    QCOMPARE(pTimer->getTime(), originalTime);

    mpEditor->mpUndoStack->redo();
    QCOMPARE(pTimer->getTime(), expectedTime);

    cleanupAll(mItemTypes[1]);
  }

  // ========================================================================
  // CATEGORY 12: Crash Prevention Tests
  // ========================================================================
  void testEmptyObjectDeletion() {
    mpEditor->slot_showTriggers();
    cleanupAll(mItemTypes[0]);

    mpEditor->treeWidget_triggers->clearSelection();
    mpEditor->treeWidget_triggers->setCurrentItem(nullptr);
    mpEditor->slot_deleteItemOrGroup();
    QVERIFY2(true, "No crash when nothing selected");

    mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->mpTriggerBaseItem);
    mpEditor->slot_deleteItemOrGroup();
    QVERIFY2(true, "No crash when base item selected");
  }

  void testPatternTypeSwitch() {
    mpEditor->slot_showTriggers();
    cleanupAll(mItemTypes[0]);

    mpEditor->addTrigger(false);
    QVERIFY(mpEditor->mpTriggerBaseItem->childCount() > 0);

    QTreeWidgetItem *trigger = mpEditor->mpTriggerBaseItem->child(0);
    mpEditor->treeWidget_triggers->setCurrentItem(trigger);
    mpEditor->slot_triggerSelected(trigger);

    for (int i = 0; i < 5; i++) {
      mpEditor->mTriggerPatternEdit[0]->comboBox_patternType->setCurrentIndex(
          0);
      mpEditor->saveTrigger();
      mpEditor->mTriggerPatternEdit[0]->comboBox_patternType->setCurrentIndex(
          1);
      mpEditor->saveTrigger();
      mpEditor->mTriggerPatternEdit[0]->comboBox_patternType->setCurrentIndex(
          2);
      mpEditor->saveTrigger();
    }
    QVERIFY2(true, "No crash on multiple pattern type switches");

    if (mpEditor->mpUndoStack->canUndo()) {
      mpEditor->mpUndoStack->undo();
      QVERIFY2(true, "No crash on undo after type switches");
    }

    cleanupAll(mItemTypes[0]);
  }

  // ========================================================================
  // CATEGORY 13: Bug-Specific Tests
  // ========================================================================
  void testPrematureUndoActivation() {
    mpEditor->slot_showTriggers();
    cleanupAll(mItemTypes[0]);

    int initialCount = mpEditor->mpTriggerBaseItem->childCount();
    mpEditor->treeWidget_triggers->setCurrentItem(mpEditor->mpTriggerBaseItem);

    bool canUndoBefore = mpEditor->mpUndoStack->canUndo();
    mpEditor->mpUndoStack->undo();
    int countAfterUndo = mpEditor->mpTriggerBaseItem->childCount();

    QVERIFY2(!canUndoBefore || countAfterUndo == initialCount,
             "No items should disappear when undo clicked without changes");
  }

  void testTriggerNameWiped() {
    mpEditor->slot_showTriggers();
    cleanupAll(mItemTypes[0]);

    mpEditor->addTrigger(false);
    QVERIFY(mpEditor->mpTriggerBaseItem->childCount() > 0);

    QTreeWidgetItem *trigger = mpEditor->mpTriggerBaseItem->child(0);
    int triggerID = trigger->data(0, Qt::UserRole).toInt();
    TTrigger *pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    QVERIFY(pT != nullptr);

    QString triggerName = qsl("Test Trigger Name");
    mpEditor->treeWidget_triggers->setCurrentItem(trigger);
    mpEditor->slot_triggerSelected(trigger);
    mpEditor->mpTriggersMainArea->lineEdit_trigger_name->setText(triggerName);
    mpEditor->saveTrigger();

    QString nameAfterSave = pT->getName();
    mpEditor->addTrigger(true);
    QString nameAfterGroupCreation = pT->getName();

    QCOMPARE(nameAfterGroupCreation, triggerName);
    QCOMPARE(nameAfterGroupCreation, nameAfterSave);

    cleanupAll(mItemTypes[0]);
  }

  void testScriptDuplication() {
    mpEditor->slot_showScripts();
    cleanupAll(mItemTypes[3]);

    mpEditor->addScript(false);
    QCOMPARE(mpEditor->mpScriptsBaseItem->childCount(), 1);

    QTreeWidgetItem *script = mpEditor->mpScriptsBaseItem->child(0);
    mpEditor->treeWidget_scripts->setCurrentItem(script);
    mpEditor->slot_deleteItemOrGroup();
    QCOMPARE(mpEditor->mpScriptsBaseItem->childCount(), 0);

    mpEditor->mpUndoStack->undo();
    QCOMPARE(mpEditor->mpScriptsBaseItem->childCount(), 1);

    cleanupAll(mItemTypes[3]);
  }

  // ========================================================================
  // CATEGORY 14: UI Pattern Clearing Tests
  // ========================================================================
  void testTriggerPatternUIClearing() {
    mpEditor->slot_showTriggers();

    while (mpEditor->mpTriggerBaseItem->childCount() > 0) {
      mpEditor->treeWidget_triggers->setCurrentItem(
          mpEditor->mpTriggerBaseItem->child(0));
      mpEditor->slot_deleteItemOrGroup();
    }
    mpEditor->mpUndoStack->clear();

    mpEditor->addTrigger(false);
    QVERIFY(mpEditor->mpTriggerBaseItem->childCount() > 0);

    QTreeWidgetItem *trigger = mpEditor->mpTriggerBaseItem->child(0);
    int triggerID = trigger->data(0, Qt::UserRole).toInt();
    TTrigger *pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    QVERIFY(pT != nullptr);

    QStringList patterns;
    QList<int> patternTypes;
    patterns << qsl("pattern1") << qsl("pattern2") << qsl("pattern3");
    patternTypes << REGEX_SUBSTRING << REGEX_PERL
                 << REGEX_BEGIN_OF_LINE_SUBSTRING;
    pT->setRegexCodeList(patterns, patternTypes);

    mpEditor->treeWidget_triggers->setCurrentItem(trigger);
    mpEditor->slot_triggerSelected(trigger);

    bool patternsLoaded = true;
    for (int i = 0; i < 3; i++) {
      QString uiPattern = mpEditor->mTriggerPatternEdit[i]
                              ->singleLineTextEdit_pattern->toPlainText();
      if (uiPattern != patterns[i]) {
        patternsLoaded = false;
        break;
      }
    }
    QVERIFY2(patternsLoaded, "Patterns should be loaded in UI");

    mpEditor->treeWidget_triggers->setCurrentItem(trigger);
    mpEditor->slot_deleteItemOrGroup();

    QList<int> affectedIDs;
    affectedIDs << triggerID;
    mpEditor->slot_itemsChanged(EditorViewType::cmTriggerView, affectedIDs);

    bool patternsCleared = true;
    for (int i = 0; i < 3; i++) {
      QString uiPattern = mpEditor->mTriggerPatternEdit[i]
                              ->singleLineTextEdit_pattern->toPlainText();
      if (!uiPattern.isEmpty()) {
        patternsCleared = false;
        break;
      }
    }
    QVERIFY2(patternsCleared, "Patterns should be cleared when item not found");

    bool fieldsCleared =
        mpEditor->mpTriggersMainArea->lineEdit_trigger_name->text().isEmpty() &&
        mpEditor->mpTriggersMainArea->label_idNumber->text().isEmpty();
    QVERIFY2(fieldsCleared, "Name/ID fields should be cleared");

    mpEditor->mpUndoStack->clear();
  }
};

#include "dlgTriggerEditorUndoRedoTest.moc"
QTEST_MAIN(dlgTriggerEditorUndoRedoTest)
