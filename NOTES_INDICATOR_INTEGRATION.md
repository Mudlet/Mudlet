# NotesIndicator Integration with NotesManager

## Overview

The `NotesIndicator` widget now fully integrates with `NotesManager` for real-time state management via Qt signals and slots.

## State Machine

The indicator has four distinct states:

- **Empty** (`State::Empty`): No notes present (0 tabs)
  - Icon: Grey folder
  - Button is disabled

- **HasContent** (`State::HasContent`): Notes exist but not modified
  - Icon: Blue folder
  - Button is enabled

- **Modified** (`State::Modified`): Notes have unsaved changes and tab is visible
  - Icon: Yellow folder
  - Button is enabled
  - Indicates active editing

- **HasUnread** (`State::HasUnread`): Notes have unsaved changes and tab is not visible
  - Icon: Green folder
  - Button is enabled
  - Indicates modifications made while tab was closed

## Usage

### Basic Setup

```cpp
#include "NotesIndicator.h"
#include "NotesManager.h"

// Create the indicator
NotesIndicator* indicator = new NotesIndicator(parentWidget);
indicator->setSize(16); // Set icon size in pixels

// Set the NotesManager to track
indicator->setNotesManager(host->mpNotesManager);
```

### Managing Tab Visibility

When the notes tab becomes visible or hidden, update the indicator:

```cpp
// When notes tab is shown
indicator->setTabVisible(true);

// When notes tab is hidden
indicator->setTabVisible(false);
```

### Tracking Current Tab

Set which tab is currently active (optional, for future enhancements):

```cpp
indicator->setCurrentTabId(tabId);
```

### Reacting to Button Clicks

Connect to the indicator's signal:

```cpp
connect(indicator, &NotesIndicator::notesButtonClicked, this, [this]() {
    // Open or focus the notes tab
    slot_notes();
});
```

## Signal Connections

The indicator automatically connects to these `NotesManager` signals:

- `tabAdded(const QString& tabId, const QString& tabName)` - Updates state and count
- `tabRemoved(const QString& tabId)` - Updates state and count, clears current tab
- `tabRenamed(const QString& tabId, const QString& newName)` - Updates state
- `contentChanged(const QString& tabId)` - Updates state to reflect modifications

## Initialization Handling

The `NotesIndicator` handles deferred initialization gracefully:

1. **No manager set initially**: Shows Empty state
2. **Manager set later**: Automatically connects and updates state
3. **Manager becomes null**: Automatically disconnects and shows Empty state
4. **Manager changed**: Safely disconnects old manager and connects new one

Use `QPointer<NotesManager>` for automatic null-safety:

```cpp
QPointer<NotesManager> mpNotesManager = host->mpNotesManager;
indicator->setNotesManager(mpNotesManager);
```

## Cleanup

The indicator automatically disconnects from NotesManager signals when:
- The indicator is destroyed
- A new NotesManager is set
- The NotesManager is destroyed

No manual cleanup is required.

## State Transitions

```
                 +-----------------+
                 | No notes tabs  |
                 |      (0)       |
                 +--------+--------+
                          |
                          v
                 +-----------------+
                 | Notes exist    |
                 |   (HasContent) |
                 +--------+--------+
                          |
                          v
            +-------------+-------------+
            | contentChanged signal  |
            +-------------+-------------+
                          |
            +-------------+-------------+
            |                           |
    Tab visible?                  Tab visible?
        YES                          NO
            |                           |
            v                           v
    +----------------+          +----------------+
    |   Modified     |          |   HasUnread    |
    | (dirty & visible)|       | (dirty & hidden) |
    +----------------+          +----------------+
            |                           |
            +-------------+-------------+
                          |
                  Tab becomes visible
                          |
                          v
                resetUnreadState()
                          |
                          v
                +----------------+
                |   Modified     |
                +----------------+
```

## Implementation Details

### Thread Safety

- All operations are on the main thread (Qt's single-threaded UI model)
- Qt's automatic connection type ensures thread safety

### Memory Management

- Uses `QPointer<NotesManager>` for safe pointer handling
- Automatic disconnection prevents memory leaks
- No manual cleanup required

### Performance

- State updates are minimal and efficient
- Icon updates only occur on state changes
- No polling or unnecessary recomputation

## Example Integration in Profile Tab

```cpp
class ProfileTabWidget : public QWidget {
    Q_OBJECT

public:
    ProfileTabWidget(Host* host, QWidget* parent = nullptr)
        : QWidget(parent), mpHost(host)
    {
        mpNotesIndicator = new NotesIndicator(this);
        mpNotesIndicator->setNotesManager(mpHost->mpNotesManager);
        mpNotesIndicator->setTabVisible(false);

        // Layout setup
        QHBoxLayout* tabLayout = new QHBoxLayout();
        tabLayout->addWidget(profileLabel);
        tabLayout->addWidget(mpNotesIndicator);
        tabLayout->addStretch();

        connect(mpNotesIndicator, &NotesIndicator::notesButtonClicked,
                this, &ProfileTabWidget::showNotesTab);
    }

    void showNotesTab() {
        mpNotesIndicator->setTabVisible(true);
        // ... show notes widget
    }

    void hideNotesTab() {
        mpNotesIndicator->setTabVisible(false);
        // ... hide notes widget
    }

private:
    QPointer<Host> mpHost;
    NotesIndicator* mpNotesIndicator = nullptr;
};
```

## Testing

To verify integration works correctly:

1. Create a profile with no notes - indicator should show Empty
2. Add a note tab - indicator should show HasContent
3. Edit the note content - indicator should show Modified (if tab visible)
4. Hide the notes tab - indicator should show HasUnread
5. Show the notes tab again - indicator should show Modified
6. Save the notes - indicator should return to HasContent
7. Delete all note tabs - indicator should return to Empty

## Troubleshooting

**Indicator always shows Empty**
- Verify `setNotesManager()` is called
- Check that NotesManager has tabs
- Ensure manager is not null

**State doesn't update when editing notes**
- Verify contentChanged signal is emitted by NotesManager
- Check that connection was successful (no warnings in Qt output)

**State doesn't change when showing/hiding tab**
- Ensure `setTabVisible(true/false)` is called
- Verify the call happens before/after the tab visibility changes

**Memory leaks reported**
- Ensure indicator is deleted when parent is destroyed
- Verify proper parent-child relationship
- Check that QPointer is working correctly
