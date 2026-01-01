# Notes Indicator Context Menu Implementation

## Overview
Implemented a full-featured context menu for the NotesIndicator widget with note management actions, confirmation dialogs, and keyboard accessibility.

## Files Modified
- `src/NotesIndicator.h` - Added context menu declarations and signals
- `src/NotesIndicator.cpp` - Implemented context menu functionality

## Features Implemented

### 1. Context Menu Actions
All actions are properly wired with icons, tooltips, and mnemonics:

- **New Note Tab** (`&New Note Tab`)
  - Creates a new note tab via NotesManager
  - Icon: SP_FileIcon
  - Shortcut: Ctrl+N
  - Tooltip: "Create and open a new note tab"
  - Always enabled

- **View Notes** (`&View Notes`)
  - Emits `viewNotesRequested()` signal to switch to notes tab
  - Icon: SP_FileDialogDetailedView
  - Tooltip: "Switch to the notes tab"
  - Enabled when notes exist

- **Rename Current Tab** (`&Rename Current Tab`)
  - Opens QInputDialog to rename the active tab
  - Icon: SP_DialogRenameButton
  - Shortcut: F2
  - Tooltip: "Rename the active note tab"
  - Enabled when notes exist and current tab is set

- **Delete Current Tab** (`&Delete Current Tab`)
  - Shows confirmation dialog, then removes the tab
  - Icon: SP_TrashIcon
  - Tooltip: "Remove the active note tab"
  - Enabled when notes exist, current tab is set, and multiple tabs exist
  - Prevents deleting the last tab

- **Clear Current Tab** (`&Clear Current Tab`)
  - Shows confirmation dialog, then clears tab content
  - Icon: SP_DialogResetButton
  - Tooltip: "Empty the content of the active note tab"
  - Enabled when notes exist and current tab is set

### 2. Confirmation Dialogs
Implemented QMessageBox::question dialogs for destructive operations:

**Delete Tab:**
- Message: "Are you sure you want to delete the note tab \"{tabName}\"?"
- Buttons: Yes | No
- Default: No
- Title: "Delete Note Tab"

**Clear Tab:**
- Message: "Are you sure you want to clear all content from the note tab \"{tabName}\"? This action cannot be undone."
- Buttons: Yes | No
- Default: No
- Title: "Clear Note Tab"

Both dialogs prevent accidental data loss by defaulting to "No" and showing the tab name.

### 3. Keyboard Access
Fully keyboard-accessible context menu:

- **Menu Key**: Pressing Qt::Key_Menu opens the context menu at the widget center
- **Mnemonics**: & in action text enables Alt+key shortcuts (e.g., Alt+N for New)
- **Shortcuts**: Ctrl+N for New, F2 for Rename
- **Navigation**: Qt menus support Tab/Arrow key navigation by default

### 4. Context-Aware Enable/Disable
Menu actions are dynamically enabled/disabled based on context:

```cpp
const bool hasNotes = mState != State::Empty;
const bool hasCurrentTab = !mCurrentTabId.isEmpty();
const bool hasMultipleTabs = mpNotesManager && mpNotesManager->getTabsMap().size() > 1;

mActionNewNote->setEnabled(true);
mViewNotesAction->setEnabled(hasNotes);
mActionRenameTab->setEnabled(hasNotes && hasCurrentTab);
mActionDeleteTab->setEnabled(hasNotes && hasCurrentTab && hasMultipleTabs);
mActionClearTab->setEnabled(hasNotes && hasCurrentTab);
```

The context menu state is updated whenever:
- State changes (Empty, HasContent, Modified, HasUnread)
- Current tab ID changes
- Tabs are added or removed

### 5. Menu Styling
Follows Mudlet UI conventions:

- Uses Qt standard icons (consistent across themes)
- Automatically adapts to light and dark themes
- Menu positioning handled by Qt (won't obscure content)
- Follows Qt's menu styling conventions

### 6. Signal Emission
New signals added for integration with parent widgets:

```cpp
signals:
    void notesButtonClicked();      // Existing signal
    void newNoteRequested();        // New signal for creating notes
    void viewNotesRequested();      // New signal for showing notes tab
```

These signals should be connected by the parent widget (e.g., Host, TConsole) to handle navigation and note creation.

## Implementation Details

### Context Menu Structure
The menu is created dynamically with proper organization:

```cpp
QMenu* menu = new QMenu(this);
menu->addAction(mActionNewNote);
menu->addAction(mViewNotesAction);
menu->addSeparator();
menu->addAction(mActionRenameTab);
menu->addAction(mActionDeleteTab);
menu->addSeparator();
menu->addAction(mActionClearTab);
```

### Event Handlers
- `contextMenuEvent()`: Handles right-click to show menu at mouse position
- `keyPressEvent()`: Handles Menu key to show menu at widget center

### State Management
- Actions are created once in `setupContextMenu()` (called from constructor)
- `updateContextMenuState()` called when state changes to enable/disable actions
- Context menu is recreated on each request (managed memory properly with delete)

### Integration with NotesManager
All actions interact with NotesManager:

- `addTab()`: Creates new tab
- `renameTab()`: Renames existing tab
- `removeTab()`: Deletes tab
- `setTabContent()`: Clears tab content
- `getTabsMap()`: Queries tab state for enable/disable logic

## Coding Standards Followed

- ✅ Modern C++20 code
- ✅ No exceptions
- ✅ Qt signal/slot mechanism
- ✅ Proper memory management (actions are children of this)
- ✅ Translation strings with `tr()`
- ✅ Context comments for translators (`//:`)
- ✅ Standard Qt icons
- ✅ Error checking before operations
- ✅ No templates or concepts (per Mudlet guidelines)
- ✅ Files end with newline character

## Acceptance Criteria Met

✅ Context menu displays on right-click with all actions
✅ All actions functional and properly wired
✅ Destructive operations show confirmation dialogs
✅ Menu key opens context menu
✅ Keyboard navigation and mnemonics work
✅ Menu styling consistent with Mudlet UI

## Usage Example

When the NotesIndicator is integrated, connect the new signals:

```cpp
// In parent widget (e.g., TConsole or Host)
connect(notesIndicator, &NotesIndicator::newNoteRequested, this, [this]() {
    // Switch to notes tab and create new tab
    showNotesTab();
});

connect(notesIndicator, &NotesIndicator::viewNotesRequested, this, [this]() {
    // Switch to notes tab
    showNotesTab();
});
```

## Future Enhancements

Possible improvements for future iterations:

1. Add "Don't show again" checkbox to confirmation dialogs
2. Support for tab-specific context menu (right-click on specific tab)
3. Keyboard shortcut configuration in preferences
4. Multi-selection for batch operations
5. Undo/redo for tab operations
