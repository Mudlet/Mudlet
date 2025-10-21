# Investigation: Issue #8388 - Scrolling Behavior Oddity

## Issue Summary
**Reporter:** wedsall
**Version:** Mudlet 4.19.1
**OS:** Windows 11 Version 24H2
**Qt Version:** 6.8.1

**Problem:** The first mouse wheel scroll-up jumps further than subsequent scroll-up actions. This behavior resets when new text enters the window.

## Root Cause Analysis

### Overview
Mudlet implements a split-screen feature for scrollback. When a user scrolls up for the first time:
1. The console splits into two panes: upper (for scrollback) and lower (for current output)
2. The lower pane appears, taking up vertical space
3. The upper pane must scroll to accommodate this change

### Code Flow

#### Mouse Wheel Event Handler
**File:** `src/TTextEdit.cpp:2086-2123`

```cpp
void TTextEdit::wheelEvent(QWheelEvent* e)
{
    // ... wheel delta calculations ...

    if (yDelta > 0) {
        mpConsole->scrollUp(yDelta);  // Calls TConsole::scrollUp
        used = true;
    } else if (yDelta < 0) {
        mpConsole->scrollDown(-yDelta);
        used = true;
    }

    e->setAccepted(used);
}
```

#### The Problematic Function
**File:** `src/TConsole.cpp:1047-1073`

```cpp
void TConsole::scrollUp(int lines)
{
    if ((mType & (UserWindow|SubConsole)) && !mScrollingEnabled) {
        return;
    }

    const bool lowerAppears = mLowerPane->isHidden();  // TRUE on first scroll
    mLowerPane->mCursorY = buffer.size();
    mLowerPane->show();
    mLowerPane->updateScreenView();
    mLowerPane->forceUpdate();

    if (lowerAppears) {
        // Schedule additional scroll to make room for lower pane
        QTimer::singleShot(0, this, [this]() {
            mUpperPane->scrollUp(mLowerPane->getRowCount());
        });
        // ... tutorial message ...
    }
    mUpperPane->scrollUp(lines);  // Always executes
    slot_adjustAccessibleNames();
}
```

### The Bug

**On First Scroll (when lower pane is hidden):**
1. `lowerAppears` = `true`
2. Lower pane is shown
3. Timer schedules: `mUpperPane->scrollUp(getRowCount())`
4. Immediately executes: `mUpperPane->scrollUp(lines)`
5. Function returns
6. Event loop processes timer callback
7. **Total scroll distance:** `lines + getRowCount()`

**On Subsequent Scrolls (when lower pane is visible):**
1. `lowerAppears` = `false`
2. Only executes: `mUpperPane->scrollUp(lines)`
3. **Total scroll distance:** `lines`

**Why it resets with new text:**
When new text arrives and the view is scrolled to bottom (`mIsTailMode = true`), the lower pane is hidden again (see `src/TConsole.cpp:1038`). This resets the state, causing the next scroll-up to trigger the split-screen activation again.

## Impact

This creates an inconsistent user experience where:
- First scroll: Jumps by `lines + getRowCount()` (typically 3 + ~20-40 lines)
- Subsequent scrolls: Jump by `lines` only (typically 3 lines)

The first scroll appears to jump 7-13x further than expected.

## Proposed Solutions

### Solution 1: Defer User Scroll on Split-Screen Activation ⭐ RECOMMENDED
When the lower pane first appears, combine both scroll operations into a single deferred action:

```cpp
void TConsole::scrollUp(int lines)
{
    if ((mType & (UserWindow|SubConsole)) && !mScrollingEnabled) {
        return;
    }

    const bool lowerAppears = mLowerPane->isHidden();
    mLowerPane->mCursorY = buffer.size();
    mLowerPane->show();
    mLowerPane->updateScreenView();
    mLowerPane->forceUpdate();

    if (lowerAppears) {
        QTimer::singleShot(0, this, [this, lines]() {
            mUpperPane->scrollUp(mLowerPane->getRowCount() + lines);
        });
        if (mudlet::self()->showSplitscreenTutorial()) {
            // ... tutorial message ...
        }
    } else {
        mUpperPane->scrollUp(lines);
    }
    slot_adjustAccessibleNames();
}
```

**Pros:**
- Single scroll operation per user action
- Maintains original timer-based deferral (may be needed for proper layout)
- Minimal code change
- Consistent scroll distance

**Cons:**
- Timer delay (0ms) might be noticeable on slow systems

### Solution 2: Immediate Combined Scroll
Remove the timer and scroll immediately:

```cpp
void TConsole::scrollUp(int lines)
{
    if ((mType & (UserWindow|SubConsole)) && !mScrollingEnabled) {
        return;
    }

    const bool lowerAppears = mLowerPane->isHidden();
    mLowerPane->mCursorY = buffer.size();
    mLowerPane->show();
    mLowerPane->updateScreenView();
    mLowerPane->forceUpdate();

    if (lowerAppears) {
        mUpperPane->scrollUp(mLowerPane->getRowCount() + lines);
        if (mudlet::self()->showSplitscreenTutorial()) {
            // ... tutorial message ...
        }
    } else {
        mUpperPane->scrollUp(lines);
    }
    slot_adjustAccessibleNames();
}
```

**Pros:**
- No timer delay
- Simpler code
- Consistent scroll distance

**Cons:**
- May have layout timing issues (unknown if timer was added to fix a specific problem)

### Solution 3: Only Scroll by User Amount on First Activation
Don't add extra scroll for split-screen - just apply user's scroll:

```cpp
void TConsole::scrollUp(int lines)
{
    if ((mType & (UserWindow|SubConsole)) && !mScrollingEnabled) {
        return;
    }

    const bool lowerAppears = mLowerPane->isHidden();
    mLowerPane->mCursorY = buffer.size();
    mLowerPane->show();
    mLowerPane->updateScreenView();
    mLowerPane->forceUpdate();

    if (lowerAppears) {
        // No additional scroll - let the split pane handle its own layout
        if (mudlet::self()->showSplitscreenTutorial()) {
            // ... tutorial message ...
        }
    }
    mUpperPane->scrollUp(lines);
    slot_adjustAccessibleNames();
}
```

**Pros:**
- Simplest change
- Most consistent user experience
- No special-casing

**Cons:**
- May not properly make room for lower pane (visual issues?)
- Changes intended split-screen behavior

## Recommendation

**Solution 1** is recommended as it:
1. Maintains single scroll operation per user action
2. Preserves the timer-based deferral (which may be important for layout timing)
3. Results in consistent scroll behavior from user perspective
4. Requires minimal code changes
5. Properly makes room for the split-screen

## Testing Strategy

1. **Test basic scrolling:**
   - Generate substantial text in console
   - Scroll up once with mouse wheel
   - Record scroll distance
   - Scroll up again
   - Verify both scrolls move by same distance

2. **Test split-screen:**
   - Verify lower pane appears on first scroll
   - Verify upper pane shows scrollback
   - Verify lower pane shows current output

3. **Test reset behavior:**
   - After scrolling up, let new text arrive
   - Verify scrolling to bottom hides lower pane
   - Verify next scroll-up behaves consistently

4. **Test edge cases:**
   - Small amount of text (less than one screen)
   - Very large amount of text
   - Scrolling with Ctrl (speed up)
   - Scrolling with Shift (slow down)

## Git History Investigation

### QTimer::singleShot Pattern Origin

Attempted to trace the history of the `if (lowerAppears)` code block using git blame:

**Finding:** The code exists from the initial repository import (commit 7f58d98, Sept 23 2025). This repository is a fork, and the split-screen functionality predates this repository's creation.

**Historical Context from GitHub Issues/Forums:**
- Issue #1105: Discusses inconsistency in PgUp/PgDown scrolling behavior with split-screen
- Issue #2397: Mousewheel scroll speed enhancements (PR #4445) - added Ctrl/Shift modifiers
- Issue #3204: Multi-view persistence improvements (PR #3844)
- Forum discussion: Users discovered Ctrl+Enter to close split-screen

**Why QTimer::singleShot(0, ...) ?**

While no explicit documentation was found, the deferred execution pattern with `QTimer::singleShot(0, ...)` is a common Qt technique that:

1. **Allows layout updates to complete:** When `mLowerPane->show()` is called, Qt needs to process layout changes and resize events. The timer pushes the scroll operation to the next event loop iteration, ensuring the lower pane's geometry is fully calculated.

2. **Prevents race conditions:** Executing `getRowCount()` immediately might return incorrect values before the layout system updates the pane's actual dimensions.

3. **Ensures proper painting order:** Deferred execution lets Qt complete the show/hide state transitions and repaint operations before scrolling.

However, this design creates the bug because:
- The deferred scroll (for lower pane height) executes **in addition to** the immediate user scroll
- This was likely unintentional - the timer was probably added to fix layout timing issues, not to add extra scrolling

## Additional Notes

### Related Code Locations
- `src/TTextEdit.cpp:2086` - wheelEvent handler
- `src/TConsole.cpp:1047` - scrollUp implementation (main fix location)
- `src/TConsole.cpp:1024` - scrollDown implementation
- `src/TTextEdit.cpp:373` - TTextEdit::scrollUp
- `src/TTextEdit.cpp:386` - TTextEdit::scrollDown
- `src/TTextEdit.cpp:2147` - bufferScrollDown logic

### Mouse Wheel Delta Calculations
The wheel event uses a multiplier of 3x by default:
```cpp
delta.ry() *= (e->modifiers() & Qt::ShiftModifier ? 1.0 :
               (e->modifiers() & Qt::ControlModifier ? ySpeedUp : 3.0));
```

With Ctrl pressed, scroll speed increases to approximately half the screen height.

### Split-Screen Feature
The split-screen is designed to:
- Show current/recent output in the lower pane
- Allow scrollback in the upper pane
- Automatically hide lower pane when scrolling to bottom
- Display tutorial message on first activation
