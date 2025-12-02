# QPointer Audit - Mudlet Codebase

## Criteria for QPointer Usage
A raw pointer should be changed to `QPointer<T>` when:
1. Storing a pointer to a **QObject you don't own** (owned by someone else)
2. You need to **check if the object was deleted** before using it
3. The pointed object has an **unpredictable lifetime**

## Investigation Status
- [x] Search for `Qt::WA_DeleteOnClose` patterns
- [x] Search for member pointers with null checks before use
- [x] Search for QMenu* member variables
- [x] Search for QDialog* member variables
- [x] Search for QAction* member variables
- [x] Analyze signal/slot connections that might outlive objects

---

## CRITICAL Issues (Use-After-Free Risk)

### 1. ctelnet::mpProgressDialog
- **File**: `src/ctelnet.h:251`
- **Type**: `QProgressDialog*` (raw pointer)
- **Issue**: Created with `Qt::WA_DeleteOnClose` at `ctelnet.cpp:3632`. After `close()` is called in `slot_replyFinished()`, the dialog deletes itself but pointer is never set to nullptr. Later calls to `slot_setDownloadProgress()` access freed memory.
- **Risk**: **CRITICAL** - Use-after-free crash
- **Fix**: Change to `QPointer<QProgressDialog>`

### 2. Host::mpEditorDialog
- **File**: `src/Host.h:538`
- **Type**: `dlgTriggerEditor*` (raw pointer)
- **Issue**: Created externally via `mudlet::createMudletEditor()`, has `Qt::WA_DeleteOnClose`. Multiple null checks exist (lines 459, 1004, 1859, 1986, 2171, 2207) indicating awareness of potential deletion.
- **Risk**: **CRITICAL** - Created and destroyed outside Host's control
- **Fix**: Change to `QPointer<dlgTriggerEditor>`

### 3. Host::mpNotePad
- **File**: `src/Host.h:542`
- **Type**: `dlgNotepad*` (raw pointer)
- **Issue**: Created externally, has `Qt::WA_DeleteOnClose`. Null checks at lines 480-483, 1012.
- **Risk**: **CRITICAL** - Created and destroyed outside Host's control
- **Fix**: Change to `QPointer<dlgNotepad>`

### 4. TConsole::mpMapper
- **File**: `src/TConsole.h:372`
- **Type**: `dlgMapper*` (raw pointer)
- **Issue**: Created externally (Host.cpp:4243, TMainConsole.cpp:775-780). Note: `TMap.h` correctly uses `QPointer<dlgMapper>` for the same object!
- **Risk**: **CRITICAL** - Inconsistent with TMap's safer pattern
- **Fix**: Change to `QPointer<dlgMapper>`

---

## HIGH Priority Issues

### 5. T2DMap::mpDlgRoomProperties
- **File**: `src/T2DMap.h:416`
- **Type**: `dlgRoomProperties*` (raw pointer)
- **Issue**: Created at T2DMap.cpp:3787 with parent=`this`, has `Qt::WA_DeleteOnClose` in its constructor. Null check at line 3713.
- **Risk**: **HIGH** - Dialog can be closed externally
- **Fix**: Change to `QPointer<dlgRoomProperties>`

### 6. T2DMap::mpDlgMapLabel
- **File**: `src/T2DMap.h:417`
- **Type**: `dlgMapLabel*` (raw pointer)
- **Issue**: Created at T2DMap.cpp:2817 with `Qt::WA_DeleteOnClose`. **No null checks** - direct access at lines 2845-2856, 2868, 2890-2891.
- **Risk**: **HIGH** - Currently unsafe, no protection
- **Fix**: Change to `QPointer<dlgMapLabel>`

### 7. T2DMap::arealist_combobox (child of WA_DeleteOnClose dialog)
- **File**: `src/T2DMap.h:250`
- **Type**: `QComboBox*` (raw pointer)
- **Issue**: Points to child widget of dialog with `Qt::WA_DeleteOnClose` (T2DMap.cpp:4118). Lambdas connected at lines 4146-4207 capture and use this pointer after dialog could be deleted.
- **Risk**: **HIGH** - Dangling pointer in lambdas
- **Fix**: Change to `QPointer<QComboBox>` or restructure lambda captures

### 8. TDetachedWindow::mpWindowMenu
- **File**: `src/TDetachedWindow.h:285`
- **Type**: `QMenu*` (raw pointer)
- **Issue**: Created via `menuBar()->addMenu()`. Other similar menus in codebase use `QPointer<QMenu>`.
- **Risk**: **MEDIUM** - Low practical risk but inconsistent with codebase patterns
- **Fix**: Change to `QPointer<QMenu>` for consistency

---

## MEDIUM Priority Issues (Defensive Improvements)

### 9-18. dlgTriggerEditor sub-area dialogs
- **File**: `src/dlgTriggerEditor.h:555-565`
- **Members**:
  - `mpActionsMainArea` (dlgActionMainArea*)
  - `mpAliasMainArea` (dlgAliasMainArea*)
  - `mpKeysMainArea` (dlgKeysMainArea*)
  - `mpScriptsMainArea` (dlgScriptsMainArea*)
  - `mpTriggersMainArea` (dlgTriggersMainArea*)
  - `mpTimersMainArea` (dlgTimersMainArea*)
  - `mpVarsMainArea` (dlgVarsMainArea*)
  - `mpSourceEditorArea` (dlgSourceEditorArea*)
  - `mpSourceEditorFindArea` (dlgSourceEditorFindArea*)
  - `mpSystemMessageArea` (dlgSystemMessageArea*)
- **Issue**: All are child widgets but have some null checks, suggesting awareness of potential issues.
- **Risk**: **MEDIUM** - Parent-child relationship should protect, but QPointer adds safety
- **Fix**: Consider changing to QPointer for defensive programming

---

## BUG FOUND (Not QPointer Related)

### dlgRoomExits - Double-Delete Bug
- **File**: `src/dlgRoomExits.cpp:287-290`
- **Issue**: Actions created with `new QAction(this)` (parent set) are ALSO manually deleted in destructor. This causes double-deletion since Qt's parent-child system already deletes them.
- **Fix**: Remove manual `delete` statements from destructor

```cpp
// BUGGY CODE in destructor (lines 287-290):
delete mpAction_otherAreaExit;  // DOUBLE-DELETE!
delete mpAction_inAreaExit;     // DOUBLE-DELETE!
delete mpAction_invalidExit;    // DOUBLE-DELETE!
delete mpAction_noExit;         // DOUBLE-DELETE!
```

---

## Confirmed FIXED

### dlgMapper::mpInfoMenu
- **File**: `src/dlgMapper.h:89`
- **Issue**: Raw `QMenu*` pointing to submenu of menu with `Qt::WA_DeleteOnClose`
- **Status**: **FIXED** - Changed to `QPointer<QMenu>`

---

## Safe Cases (No Changes Needed)

### Good Patterns Already Using QPointer
- `TMap.h:250` - `QPointer<dlgMapper> mpMapper` ✓
- `T2DMap.h:251` - `QPointer<QDialog> mpCustomLinesDialog` ✓
- `T2DMap.h:257-259` - `QPointer<QComboBox/QPushButton/QCheckBox>` for dialog children ✓
- `dlgMapper.h:88` - `QPointer<Host> mpHost` ✓
- `dlgProfilePreferences.h:228,233` - `QPointer<QMenu>` ✓

### Safe Raw Pointers (Parent-Child Ownership Clear)
- `dlgMapLabel.h:54-57` - Color/font dialogs use `Qt::WA_DeleteOnClose` correctly
- `TMap.h:382` - `mpProgressDialog` has proper null checks and cleanup
- Most `QAction*` members - Created with `this` as parent, lifecycle managed by Qt

---

## Summary Table

| Priority | File | Member | Current Type | Recommended | Risk |
|----------|------|--------|--------------|-------------|------|
| CRITICAL | ctelnet.h:251 | mpProgressDialog | QProgressDialog* | QPointer | Use-after-free |
| CRITICAL | Host.h:538 | mpEditorDialog | dlgTriggerEditor* | QPointer | External lifecycle |
| CRITICAL | Host.h:542 | mpNotePad | dlgNotepad* | QPointer | External lifecycle |
| CRITICAL | TConsole.h:372 | mpMapper | dlgMapper* | QPointer | Inconsistent |
| HIGH | T2DMap.h:416 | mpDlgRoomProperties | dlgRoomProperties* | QPointer | WA_DeleteOnClose |
| HIGH | T2DMap.h:417 | mpDlgMapLabel | dlgMapLabel* | QPointer | WA_DeleteOnClose |
| HIGH | T2DMap.h:250 | arealist_combobox | QComboBox* | QPointer | Child of deleted dialog |
| MEDIUM | TDetachedWindow.h:285 | mpWindowMenu | QMenu* | QPointer | Consistency |
| BUG | dlgRoomExits.cpp:287-290 | - | - | Remove deletes | Double-delete |
