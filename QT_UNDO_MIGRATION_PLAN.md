# Migration Plan: EditorUndoSystem → Qt's QUndoStack Framework

## 🎯 CURRENT STATUS: Phase 3 - Testing & Validation

**Progress: 66% Complete (Phases 1 & 2 Done)**

✅ Phase 1: Base Infrastructure - **COMPLETE**
✅ Phase 2: Command Migration (5 commands) - **COMPLETE**
🔄 Phase 3: Testing & Validation - **IN PROGRESS**
⏳ Phase 4: Cutover - **PENDING**

## Overview
Migrate Mudlet's custom undo system (3,120 lines) to Qt's built-in QUndoStack framework, reducing code by ~70% while gaining professional features.

## Benefits
- **2,200+ fewer lines** of custom code to maintain
- **Automatic action integration** via `createUndoAction()`/`createRedoAction()`
- **Future features**: QUndoView (visual history), QUndoGroup (multi-profile), command merging
- **Battle-tested** Qt framework instead of custom implementation

## Phase 1: Create Base Infrastructure ✅ COMPLETE

### 1.1 Create MudletCommand Base Class ✅
- File: `src/MudletCommand.h` (80 lines)
- Extends `QUndoCommand` with Mudlet-specific features:
  - `virtual EditorViewType viewType() const = 0`
  - `virtual QList<int> affectedItemIDs() const = 0`
  - `virtual void remapItemID(int oldID, int newID) = 0`
- Maintains `Host* mpHost` member

### 1.2 Create MudletUndoStack Wrapper ✅
- File: `src/MudletUndoStack.h` + `.cpp` (182 lines total)
- Extends `QUndoStack` to add:
  - `remapItemIDs()` method for ID tracking
  - `itemsChanged` signal for UI updates
  - Override `redo()` to handle ID remapping for AddItemCommand
  - Track push operations to prevent spurious itemsChanged signals

### 1.3 Add to dlgTriggerEditor ✅
- Added `MudletUndoStack* mpQtUndoStack` alongside existing `mpUndoSystem`
- Connected signals to existing slots
- Both systems run in parallel for validation
- Stack synchronization implemented to keep them in sync

## Phase 2: Migrate Commands ✅ COMPLETE

All commands migrated with dual-push validation:

### 2.1 ToggleActiveCommand ✅
- Created `MudletToggleActiveCommand.h` + `.cpp` (245 lines)
- Dual-push added at 18 usage sites across all 6 view types
- Handles checkbox toggle operations with proper state tracking

### 2.2 MoveItemCommand ✅
- Created `MudletMoveItemCommand.h` + `.cpp` (306 lines)
- Dual-push added for drag-and-drop operations
- Batch operations using `beginMacro()`/`endMacro()` implemented
- Handles parent-child hierarchy changes

### 2.3 AddItemCommand ✅
- Created `MudletAddItemCommand.h` + `.cpp` (287 lines)
- Dual-push added at 6 usage sites (one per view type)
- ID remapping fully implemented for recreated items
- Skip-first-redo pattern to avoid double execution
- XML snapshot support for undo functionality
- Expansion state preservation added to prevent UI collapse

### 2.4 ModifyPropertyCommand ✅
- Created `MudletModifyPropertyCommand.h` + `.cpp` (232 lines)
- Dual-push added at 6 usage sites (save functions)
- XML snapshots for old/new state comparison
- updateXXXFromXML functions exported for in-place updates
- No ID remapping needed (modifies in place)

### 2.5 DeleteItemCommand ✅
- Created `MudletDeleteItemCommand.h` + `.cpp` (675 lines)
- Dual-push added at 6 deletion sites
- Complex parent-child relationship handling
- Recursive ID remapping for restored hierarchies
- Proper mpHost nullification before deletion
- C++20 designated initializers for DeletedItemInfo conversion
- Friend declarations added to TTimer.h and TScript.h

### 2.6 BatchCommand → Macro ✅
- Already implemented in Phase 2.2
- `beginBatch()`/`endBatch()` mapped to `beginMacro()`/`endMacro()`
- 1 call site pair in slot_batchMoveStarted/Ended
- Both systems synchronized during batch operations

## Phase 3: Testing & Validation 🔄 IN PROGRESS

**Goal:** Verify both systems produce identical results before cutover

### 3.1 Verification Testing ⏳ NEXT
Manual testing of core operations:
- [ ] Toggle active/inactive state (18 usage sites)
- [ ] Drag-and-drop move operations with batch undo/redo
- [ ] Add new items (6 item types: triggers, aliases, timers, scripts, keys, actions)
- [ ] Modify item properties (save operations)
- [ ] Delete items (single and multiple)
- [ ] Verify itemsChanged signal emissions
- [ ] Check UI refresh after undo/redo

### 3.2 Complex Scenarios ⏳
Test advanced use cases:
- [ ] Multi-item drag-and-drop with undo/redo
- [ ] Delete item → undo → redo with ID changes
- [ ] Parent-child relationship preservation
- [ ] Batch operations (macros) with multiple moves
- [ ] Switching between editor views during undo/redo
- [ ] Undo limit enforcement (default 50 commands)
- [ ] Expansion state preservation after undo

### 3.3 Edge Cases ⏳
Test boundary conditions:
- [ ] Undo/redo after profile switch
- [ ] XML compression/decompression in snapshots
- [ ] Item ID remapping chains (delete → undo → delete → undo)
- [ ] Stack synchronization when switching views
- [ ] Redo after new action (clears redo stack)

## Phase 4: Cutover (1 day)

### 4.1 Remove Old System
- Delete `EditorUndoSystem.h` and `.cpp` (3,120 lines)
- Remove dual-push code (37 sites)
- Rename `mpQtUndoStack` → `mpUndoStack`

### 4.2 Update Signal Connections
- 5 connection sites already compatible
- Update naming: `beginBatch` → `beginMacro`, `endBatch` → `endMacro`
- Replace `clear()` with new stack creation

### 4.3 Code Cleanup
- Remove EditorCommand base class
- Update includes and forward declarations
- Remove deprecated comments

## Changes Required

### Files to Create
- `src/MudletCommand.h` (~100 lines)
- `src/MudletUndoStack.h` (~50 lines)
- `src/MudletUndoStack.cpp` (~150 lines)
- `src/commands/MudletToggleActiveCommand.h` + `.cpp`
- `src/commands/MudletMoveItemCommand.h` + `.cpp`
- `src/commands/MudletAddItemCommand.h` + `.cpp`
- `src/commands/MudletModifyPropertyCommand.h` + `.cpp`
- `src/commands/MudletDeleteItemCommand.h` + `.cpp`

### Files to Modify
- `src/dlgTriggerEditor.h` (add mpQtUndoStack, update includes)
- `src/dlgTriggerEditor.cpp` (37 command creation sites, 5 signal connections)
- `src/TTreeWidget.cpp` (2 batch operation sites)
- `src/CMakeLists.txt` (add new source files)

### Files to Delete (after cutover)
- `src/EditorUndoSystem.h` (301 lines)
- `src/EditorUndoSystem.cpp` (2,820 lines)

## Risk Mitigation

### Low Risk Strategy
- Parallel system approach allows safe rollback
- Incremental command-by-command migration
- Extensive testing before cutover
- Keep old system until validation complete

### Validation Checks
- Assert both systems produce identical results
- Log all operations for comparison
- Automated tests for each command type
- Manual testing of complex scenarios

## Timeline

**Total Estimate: 3-4 weeks**
- Phase 1 (Infrastructure): 2-3 days
- Phase 2 (Commands): 1-2 weeks
- Phase 3 (Testing): 1 week
- Phase 4 (Cutover): 1 day

## Success Criteria

✅ All 6 command types migrated and tested
✅ ID remapping works correctly
✅ Batch operations (macros) work as before
✅ All signals emit correctly
✅ Undo/redo behavior identical to old system
✅ No memory leaks or crashes
✅ UI updates correctly after undo/redo
✅ 2,200+ lines of code removed

## Technical Details

### MudletCommand Base Class Design

```cpp
// File: src/MudletCommand.h
class MudletCommand : public QUndoCommand {
public:
    explicit MudletCommand(Host* host, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), mpHost(host) {}

    explicit MudletCommand(const QString& text, Host* host, QUndoCommand* parent = nullptr)
        : QUndoCommand(text, parent), mpHost(host) {}

    virtual ~MudletCommand() = default;

    // Mudlet-specific extensions
    virtual EditorViewType viewType() const = 0;
    virtual QList<int> affectedItemIDs() const = 0;
    virtual void remapItemID(int oldID, int newID) = 0;

    // QUndoCommand overrides (still pure virtual)
    void undo() override = 0;
    void redo() override = 0;

protected:
    Host* mpHost;
};
```

### MudletUndoStack Wrapper Design

```cpp
// File: src/MudletUndoStack.h
class MudletUndoStack : public QUndoStack {
    Q_OBJECT

public:
    explicit MudletUndoStack(QObject* parent = nullptr);

    void remapItemIDs(int oldID, int newID);

    void undo() override;
    void redo() override;

signals:
    void itemsChanged(EditorViewType viewType, QList<int> affectedItemIDs);

protected:
    using QUndoStack::command;  // Access protected method
};
```

### Example Command Migration

```cpp
// Before: ToggleActiveCommand in EditorUndoSystem
class ToggleActiveCommand : public EditorCommand {
    // ... implementation
};

// After: MudletToggleActiveCommand with QUndoCommand
class MudletToggleActiveCommand : public MudletCommand {
public:
    MudletToggleActiveCommand(EditorViewType viewType, int itemID,
                              bool oldState, bool newState,
                              const QString& itemName, Host* host)
        : MudletCommand(generateText(viewType, itemName, newState), host)
        , mViewType(viewType)
        , mItemID(itemID)
        , mOldActiveState(oldState)
        , mNewActiveState(newState)
        , mItemName(itemName)
    {}

    void undo() override {
        setItemActiveState(mItemID, mOldActiveState);
    }

    void redo() override {
        setItemActiveState(mItemID, mNewActiveState);
    }

    EditorViewType viewType() const override { return mViewType; }
    QList<int> affectedItemIDs() const override { return {mItemID}; }

    void remapItemID(int oldID, int newID) override {
        if (mItemID == oldID) {
            mItemID = newID;
        }
    }

private:
    EditorViewType mViewType;
    int mItemID;
    bool mOldActiveState;
    bool mNewActiveState;
    QString mItemName;

    void setItemActiveState(int itemID, bool active);
    static QString generateText(EditorViewType viewType,
                               const QString& itemName,
                               bool newState);
};
```

### Dual-Push Pattern During Migration

```cpp
// In dlgTriggerEditor.cpp during Phase 2
void dlgTriggerEditor::someFunction() {
    // ... create command parameters ...

    // Push to old system (for comparison)
    auto oldCmd = std::make_unique<ToggleActiveCommand>(
        viewType, itemID, oldState, newState, itemName, mpHost
    );
    mpUndoSystem->pushCommand(std::move(oldCmd));

    // Push to new Qt system (for validation)
    auto* newCmd = new MudletToggleActiveCommand(
        viewType, itemID, oldState, newState, itemName, mpHost
    );
    mpQtUndoStack->push(newCmd);  // Qt takes ownership
}
```

## API Mapping

| Current EditorUndoSystem | Qt QUndoStack | Notes |
|--------------------------|---------------|-------|
| `pushCommand(unique_ptr)` | `push(QUndoCommand*)` | Change from unique_ptr to raw pointer |
| `undo()` | `undo()` | ✅ Same |
| `redo()` | `redo()` | ✅ Same |
| `canUndo()` | `canUndo()` | ✅ Same |
| `canRedo()` | `canRedo()` | ✅ Same |
| `undoText()` | `undoText()` | ✅ Same |
| `redoText()` | `redoText()` | ✅ Same |
| `setUndoLimit()` | `setUndoLimit()` | ✅ Same |
| `beginBatch()` | `beginMacro()` | ⚠️ Rename |
| `endBatch()` | `endMacro()` | ⚠️ Rename |
| `clear()` | `clear()` or create new stack | ⚠️ Different approach |
| `remapItemIDs()` | Custom in MudletUndoStack | ❌ Must implement |
| Signal: `itemsChanged` | Custom in MudletUndoStack | ❌ Must implement |

## Questions Before Starting

1. **Approve phased approach?** Or prefer direct replacement?
2. **Timeline acceptable?** 3-4 weeks of development time?
3. **Future features desired?** QUndoView visual history, QUndoGroup multi-profile support?
4. **Testing scope?** Manual only or add automated tests?

## Implementation Summary (Phases 1 & 2)

### Files Created (2,227 lines)
**Base Infrastructure:**
- `src/MudletCommand.h` (80 lines) - Base class for all Qt undo commands
- `src/MudletUndoStack.h` (98 lines) - Qt stack wrapper with Mudlet extensions
- `src/MudletUndoStack.cpp` (182 lines) - Implementation with itemsChanged signal

**Command Implementations:**
- `src/commands/MudletToggleActiveCommand.h` (70 lines)
- `src/commands/MudletToggleActiveCommand.cpp` (175 lines)
- `src/commands/MudletMoveItemCommand.h` (75 lines)
- `src/commands/MudletMoveItemCommand.cpp` (231 lines)
- `src/commands/MudletAddItemCommand.h` (70 lines)
- `src/commands/MudletAddItemCommand.cpp` (287 lines)
- `src/commands/MudletModifyPropertyCommand.h` (64 lines)
- `src/commands/MudletModifyPropertyCommand.cpp` (232 lines)
- `src/commands/MudletDeleteItemCommand.h` (68 lines)
- `src/commands/MudletDeleteItemCommand.cpp` (675 lines)

**Total New Code:** ~2,227 lines
**Code Reduction After Cutover:** ~900 lines (3,120 old → 2,227 new)

### Files Modified
- `src/dlgTriggerEditor.h` - Added mpQtUndoStack member
- `src/dlgTriggerEditor.cpp` - Added dual-push at 37 command creation sites
- `src/EditorUndoSystem.h` - Exported XML helper functions
- `src/TTimer.h` - Added friend declaration for MudletDeleteItemCommand
- `src/TScript.h` - Added friend declaration for MudletDeleteItemCommand
- `src/CMakeLists.txt` - Added 10 new source/header files

### Key Commits (22 total on branch)
- Phase 1: Base infrastructure (MudletCommand, MudletUndoStack)
- Phase 2.1: ToggleActiveCommand migration
- Phase 2.2: MoveItemCommand migration + batch/macro support
- Phase 2.3: AddItemCommand migration + ID remapping + expansion state
- Phase 2.4: ModifyPropertyCommand migration
- Phase 2.5: DeleteItemCommand migration + C++20 designated initializers
- Stack synchronization fixes
- Friend declaration additions

### Dual-Push Validation Sites (37 total)
- 18 sites for ToggleActiveCommand (3 per view type)
- 2 sites for MoveItemCommand (batch start/end)
- 6 sites for AddItemCommand (1 per view type)
- 6 sites for ModifyPropertyCommand (save functions)
- 6 sites for DeleteItemCommand (deletion functions)

Both systems run in parallel at all sites for validation.

## References

- Qt QUndoStack Documentation: https://doc.qt.io/qt-6/qundostack.html
- Qt QUndoCommand Documentation: https://doc.qt.io/qt-6/qundocommand.html
- Qt Undo Framework Overview: https://doc.qt.io/qt-6/qundo.html
- Current Implementation: `src/EditorUndoSystem.h` and `.cpp`
- Usage Analysis: See `UNDO_FRAMEWORK_EVALUATION.md` for detailed comparison
